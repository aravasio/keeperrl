/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "stdafx.h"

#include "model.h"
#include "player.h"
#include "village_control.h"
#include "statistics.h"
#include "options.h"
#include "technology.h"
#include "level.h"
#include "name_generator.h"
#include "item_factory.h"
#include "creature.h"
#include "square.h"
#include "view_id.h"
#include "collective.h"
#include "music.h"
#include "trigger.h"
#include "level_maker.h"
#include "map_memory.h"
#include "level_builder.h"
#include "tribe.h"
#include "time_queue.h"
#include "visibility_map.h"
#include "creature_name.h"
#include "creature_attributes.h"
#include "view.h"
#include "view_index.h"
#include "map_memory.h"
#include "stair_key.h"
#include "territory.h"
#include "game.h"
#include "progress_meter.h"

template <class Archive> 
void Model::serialize(Archive& ar, const unsigned int version) {
  CHECK(!serializationLocked);
  serializeAll(ar, levels, collectives, timeQueue, deadCreatures, currentTime, woodCount, game, lastTick);
  serializeAll(ar, stairNavigation, cemetery);
  if (progressMeter)
    progressMeter->addProgress();
}

ProgressMeter* Model::progressMeter = nullptr;

void Model::lockSerialization() {
  serializationLocked = true;
}

SERIALIZABLE(Model);

void Model::addWoodCount(int cnt) {
  woodCount += cnt;
}

int Model::getWoodCount() const {
  return woodCount;
}

vector<Collective*> Model::getCollectives() const {
  return extractRefs(collectives);
}

void Model::updateSunlightMovement() {
  for (PLevel& l : levels)
    l->updateSunlightMovement();
}

void Model::update(double totalTime) {
  if (Creature* creature = timeQueue->getNextCreature()) {
    currentTime = creature->getLocalTime();
    if (currentTime > totalTime)
      return;
    while (totalTime > lastTick + 1) {
      lastTick += 1;
      tick(lastTick);
    }
    if (!creature->isDead()) {
#ifndef RELEASE
      CreatureAction::checkUsage(true);
#endif
      creature->makeMove();
#ifndef RELEASE
      CreatureAction::checkUsage(false);
#endif
    }
    if (!creature->isDead() && creature->getLevel()->getModel() == this)
      CHECK(creature->getPosition().getCreature() == creature);
  } else
    currentTime = totalTime;
}

void Model::tick(double time) {
  for (Creature* c : timeQueue->getAllCreatures()) {
    c->tick();
  }
  for (PLevel& l : levels)
    l->tick();
  for (PCollective& col : collectives)
    col->tick();
}

void Model::addCreature(PCreature c, double delay) {
  c->setLocalTime(getTime() + 1 + delay + Random.getDouble());
  if (c->isPlayer())
    game->setPlayer(c.get());
  timeQueue->addCreature(std::move(c));
}

Level* Model::buildLevel(LevelBuilder&& b, PLevelMaker maker) {
  LevelBuilder builder(std::move(b));
  levels.push_back(builder.build(this, maker.get(), Random.getLL()));
  return levels.back().get();
}

Model::Model() {
  clearDeadCreatures();
}

void Model::clearDeadCreatures() {
  deadCreatures.clear();
  cemetery = LevelBuilder(Random, 100, 100, "Dead creatures", false)
      .build(this, LevelMaker::emptyLevel(Random).get(), Random.getLL());
}

Model::~Model() {
}

double Model::getTime() const {
  return currentTime;
}

void Model::setGame(Game* g) {
  game = g;
}

Game* Model::getGame() const {
  return game;
}

Level* Model::getLinkedLevel(Level* from, StairKey key) const {
  for (Level* target : getLevels())
    if (target != from && target->hasStairKey(key))
      return target;
  FAIL << "Failed to find next level for " << key.getInternalKey() << " " << from->getName();
  return nullptr;
}

void Model::calculateStairNavigation() {
  // Floyd-Warshall algorithm
  for (const Level* l1 : getLevels())
    for (const Level* l2 : getLevels())
      if (l1 != l2)
        if (auto stairKey = getStairsBetween(l1, l2))
          stairNavigation[make_pair(l1, l2)] = *stairKey;
  for (const Level* li : getLevels())
    for (const Level* l1 : getLevels())
      if (li != l1)
        for (const Level* l2 : getLevels())
          if (l2 != l1 && l2 != li && !stairNavigation.count(make_pair(l1, l2)) && stairNavigation.count(make_pair(li, l2)) &&
              stairNavigation.count(make_pair(l1, li)))
            stairNavigation[make_pair(l1, l2)] = stairNavigation.at(make_pair(l1, li));
  for (const Level* l1 : getLevels())
    for (const Level* l2 : getLevels())
      if (l1 != l2)
        CHECK(stairNavigation.count(make_pair(l1, l2))) <<
            "No stair path between levels " << l1->getName() << " " << l2->getName();
}

optional<StairKey> Model::getStairsBetween(const Level* from, const Level* to) {
  for (StairKey key : from->getAllStairKeys())
    if (to->hasStairKey(key))
      return key;
  return none;
}

optional<Position> Model::getStairs(const Level* from, const Level* to) {
  CHECK(from != to);
  if (!contains(getLevels(), from) || ! contains(getLevels(), to) || !stairNavigation.count({from, to}))
    return none;
  return Random.choose(from->getLandingSquares(stairNavigation.at({from, to})));
}

vector<Level*> Model::getLevels() const {
  return extractRefs(levels);
}

Level* Model::getTopLevel() const {
  return levels[0].get();
}

void Model::killCreature(Creature* c, Creature* attacker) {
  if (attacker)
    attacker->onKilled(c);
  c->getTribe()->onMemberKilled(c, attacker);
  deadCreatures.push_back(timeQueue->removeCreature(c));
  cemetery->landCreature(cemetery->getAllPositions(), c);
}

PCreature Model::extractCreature(Creature* c) {
  PCreature ret = timeQueue->removeCreature(c);
  c->getLevel()->removeCreature(c);
  return ret;
}

void Model::transferCreature(PCreature c, Vec2 travelDir) {
  CHECK(getTopLevel()->landCreature(StairKey::transferLanding(), std::move(c), travelDir));
}

bool Model::canTransferCreature(Creature* c, Vec2 travelDir) {
  for (Position pos : getTopLevel()->getLandingSquares(StairKey::transferLanding()))
    if (pos.canEnter(c))
      return true;
  return false;
}

vector<Creature*> Model::getAllCreatures() const { 
  return timeQueue->getAllCreatures();
}

void Model::beforeUpdateTime(Creature* c) {
  timeQueue->beforeUpdateTime(c);
}

void Model::afterUpdateTime(Creature* c) {
  timeQueue->afterUpdateTime(c);
}

