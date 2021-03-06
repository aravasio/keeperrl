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
#include "village_control.h"
#include "collective.h"
#include "creature.h"
#include "level.h"
#include "collective_teams.h"
#include "tribe.h"
#include "effect_type.h"
#include "task.h"
#include "collective_attack.h"
#include "territory.h"
#include "game.h"
#include "collective_name.h"
#include "lasting_effect.h"
#include "body.h"

typedef EnumVariant<AttackTriggerId, TYPES(int),
        ASSIGN(int, AttackTriggerId::ENEMY_POPULATION, AttackTriggerId::GOLD)> OldTrigger;

SERIALIZATION_CONSTRUCTOR_IMPL(VillageControl);


template <class Archive>
void VillageControl::Villain::serialize(Archive& ar, const unsigned int version) {
  serializeAll(ar, minPopulation, minTeamSize, triggers, behaviour, welcomeMessage, ransom);
}

SERIALIZABLE(VillageControl::Villain);

VillageControl::VillageControl(Collective* col, optional<Villain> v) : CollectiveControl(col), villain(v) {
  for (Position v : col->getTerritory().getAll())
    for (Item* it : v.getItems())
      myItems.insert(it);
}

Collective* VillageControl::getEnemyCollective() const {
  return getCollective()->getGame()->getPlayerCollective();
}

bool VillageControl::isEnemy(const Creature* c) {
  if (Collective* col = getEnemyCollective())
    return contains(col->getCreatures(), c);
  else
    return false;
}

void VillageControl::onOtherKilled(const Creature* victim, const Creature* killer) {
  if (victim->getTribe() == getCollective()->getTribe())
    if (isEnemy(killer))
      victims += 0.15; // small increase for same tribe but different village
}

void VillageControl::onMemberKilled(const Creature* victim, const Creature* killer) {
  if (isEnemy(killer))
    victims += 1;
}

void VillageControl::onPickupEvent(const Creature* who, const vector<Item*>& items) {
  if (getCollective()->getTerritory().contains(who->getPosition()))
    if (isEnemy(who) && villain)
      if (contains(villain->triggers, AttackTriggerId::STOLEN_ITEMS)) {
        bool wasTheft = false;
        for (const Item* it : items)
          if (myItems.contains(it)) {
            wasTheft = true;
            ++stolenItemCount;
            myItems.erase(it);
          }
        if (getCollective()->hasLeader() && wasTheft) {
          who->playerMessage(PlayerMessage("You are going to regret this", MessagePriority::HIGH));
        }
    }
}

void VillageControl::launchAttack(vector<Creature*> attackers) {
  if (Collective* enemy = getEnemyCollective()) {
    for (Creature* c : attackers)
//      if (getCollective()->getGame()->canTransferCreature(c, enemy->getLevel()->getModel()))
        getCollective()->getGame()->transferCreature(c, enemy->getLevel()->getModel());
    optional<int> ransom;
    int hisGold = enemy->numResource(CollectiveResourceId::GOLD);
    if (villain->ransom && hisGold >= villain->ransom->second)
      ransom = max<int>(villain->ransom->second,
          (Random.getDouble(villain->ransom->first * 0.6, villain->ransom->first * 1.5)) * hisGold);
    enemy->addAttack(CollectiveAttack(getCollective(), attackers, ransom));
    TeamId team = getCollective()->getTeams().createPersistent(attackers);
    getCollective()->getTeams().activate(team);
    getCollective()->freeTeamMembers(team);
    for (Creature* c : attackers)
      getCollective()->setTask(c, villain->getAttackTask(this));
    attackSizes[team] = attackers.size();
  }
}

void VillageControl::considerCancellingAttack() {
  for (auto team : getCollective()->getTeams().getAll()) {
    vector<Creature*> members = getCollective()->getTeams().getMembers(team);
    if (members.size() < (attackSizes[team] + 1) / 2 || (members.size() == 1 &&
          members[0]->getBody().isSeriouslyWounded())) {
      for (Creature* c : members)
        getCollective()->cancelTask(c);
      getCollective()->getTeams().cancel(team);
    }
  }
}

void VillageControl::onRansomPaid() {
  for (auto team : getCollective()->getTeams().getAll()) {
    vector<Creature*> members = getCollective()->getTeams().getMembers(team);
    for (Creature* c : members)
      getCollective()->cancelTask(c);
    getCollective()->getTeams().cancel(team);
  }
}

vector<TriggerInfo> VillageControl::getTriggers(const Collective* against) const {
  vector<TriggerInfo> ret;
  if (villain && against == getEnemyCollective())
    for (auto& elem : villain->triggers)
      ret.push_back({elem, villain->getTriggerValue(elem, this)});
  return ret;
}

void VillageControl::considerWelcomeMessage() {
  if (!getCollective()->hasLeader())
    return;
  if (villain)
    if (villain->welcomeMessage)
      switch (*villain->welcomeMessage) {
        case DRAGON_WELCOME:
          for (Position pos : getCollective()->getTerritory().getAll())
            if (Creature* c = pos.getCreature())
              if (c->isAffected(LastingEffect::INVISIBLE) && isEnemy(c) && c->isPlayer()
                  && getCollective()->getLeader()->canSee(c->getPosition())) {
                c->playerMessage(PlayerMessage("\"Well thief! I smell you and I feel your air. "
                      "I hear your breath. Come along!\"", MessagePriority::CRITICAL));
                villain->welcomeMessage.reset();
              }
          break;
      }
}

void VillageControl::checkEntries() {
  if (villain)
    for (auto& trigger : villain->triggers)
      if (trigger.getId() == AttackTriggerId::ENTRY)
        for (Position pos : getCollective()->getTerritory().getAll())
          if (Creature* c = pos.getCreature())
            if (getCollective()->getTribe()->isEnemy(c))
              entries = true;
}

bool VillageControl::canPerformAttack(bool currentlyActive) {
  return !currentlyActive || getCollective()->getGame()->isSingleModel() ||
    getCollective()->getLevel()->getModel() == getCollective()->getGame()->getMainModel().get();
}

void VillageControl::update(bool currentlyActive) {
  considerWelcomeMessage();
  considerCancellingAttack();
  checkEntries();
  if (Collective* enemy = getEnemyCollective())
    maxEnemyPower = max(maxEnemyPower, enemy->getDangerLevel());
  vector<Creature*> allMembers = getCollective()->getCreatures();
  for (auto team : getCollective()->getTeams().getAll()) {
    for (const Creature* c : getCollective()->getTeams().getMembers(team))
      if (!getCollective()->hasTask(c)) {
        getCollective()->getTeams().cancel(team);
        break;
      }
    return;
  }
  double updateFreq = 0.1;
  if (canPerformAttack(currentlyActive) && Random.roll(1 / updateFreq))
    if (villain) {
      double prob = villain->getAttackProbability(this) / updateFreq;
      if (prob > 0 && Random.roll(1 / prob)) {
        vector<Creature*> fighters;
        fighters = getCollective()->getCreatures({MinionTrait::FIGHTER}, {MinionTrait::SUMMONED});
        if (getCollective()->getGame()->isSingleModel())
          fighters = filter(fighters, [this] (const Creature* c) {
              return contains(getCollective()->getTerritory().getAll(), c->getPosition()); });
        Debug() << getCollective()->getName().getShort() << " fighters: " << int(fighters.size())
          << (!getCollective()->getTeams().getAll().empty() ? " attacking " : "");
        if (fighters.size() >= villain->minTeamSize && 
            allMembers.size() >= villain->minPopulation + villain->minTeamSize)
        launchAttack(getPrefix(Random.permutation(fighters),
          Random.get(villain->minTeamSize, min(fighters.size(), allMembers.size() - villain->minPopulation) + 1)));
      }
    }
}

PTask VillageControl::Villain::getAttackTask(VillageControl* self) {
  Collective* enemy = self->getEnemyCollective();
  switch (behaviour.getId()) {
    case VillageBehaviourId::KILL_LEADER:
      return Task::attackLeader(enemy);
    case VillageBehaviourId::KILL_MEMBERS:
      return Task::killFighters(enemy, behaviour.get<int>());
    case VillageBehaviourId::STEAL_GOLD:
      return Task::stealFrom(enemy, self->getCollective());
    case VillageBehaviourId::CAMP_AND_SPAWN:
      return Task::campAndSpawn(enemy, self->getCollective(),
            behaviour.get<CreatureFactory>(), Random.get(3, 7), Range(3, 7), Random.get(3, 7));
  }
}

static double powerClosenessFun(double myPower, double hisPower) {
  if (myPower == 0 || hisPower == 0)
    return 0;
  double a = myPower / hisPower;
  double valueAt2 = 0.5;
  if (a < 0.4)
    return 0;
  if (a < 1)
    return a * a * a; // fast growth close to 1
  else if (a < 2)
    return 1.0 - (a - 1) * (a - 1) * (a - 1) * valueAt2; // slow descent close to 1, valueAt2 at 2
  else
    return valueAt2 / (a - 1); // converges to 0, valueAt2 at 2
}

static double victimsFun(int victims, int minPopulation) {
  if (!victims)
    return 0;
  else if (victims == 1)
    return 0.1;
  else if (victims <= 3)
    return 0.3;
  else if (victims <= 5)
    return 0.7;
  else
    return 1.0;
}

static double populationFun(int population, int minPopulation) {
  double diff = double(population - minPopulation) / minPopulation;
  if (diff < 0)
    return 0;
  else if (diff < 0.1)
    return 0.1;
  else if (diff < 0.2)
    return 0.3;
  else if (diff < 0.33)
    return 0.6;
  else
    return 1.0;
}

static double goldFun(int gold, int minGold) {
  double diff = double(gold - minGold) / minGold;
  if (diff < 0)
    return 0;
  else if (diff < 0.1)
    return 0.1;
  else if (diff < 0.4)
    return 0.3;
  else if (diff < 1)
    return 0.6;
  else
    return 1.0;
}

static double stolenItemsFun(int numStolen) {
  if (!numStolen)
    return 0;
  else 
    return 1.0;
}

static double getRoomProb(SquareId id) {
  switch (id) {
    case SquareId::THRONE: return 0.001;
    case SquareId::IMPALED_HEAD: return 0.000125;
    default: FAIL << "Unsupported ROOM_BUILT type"; return 0;
  }
}

static double getFinishOffProb(double maxPower, double currentPower, double selfPower) {
  if (maxPower < selfPower || currentPower * 2 >= maxPower)
    return 0;
  double minProb = 0.25;
  return 1 - 2 * (currentPower / maxPower) * (1 - minProb);
}

double VillageControl::Villain::getTriggerValue(const Trigger& trigger, const VillageControl* self) const {
  double powerMaxProb = 1.0 / 10000; // rather small chance that they attack just because you are strong
  double victimsMaxProb = 1.0 / 500;
  double populationMaxProb = 1.0 / 500;
  double goldMaxProb = 1.0 / 1000;
  double stolenMaxProb = 1.0 / 300;
  double entryMaxProb = 1.0 / 20.0;
  double finishOffMaxProb = 1.0 / 1000;
  double proximityMaxProb = 1.0 / 5000;
  if (Collective* collective = self->getEnemyCollective())
    switch (trigger.getId()) {
      case AttackTriggerId::TIMER: 
        return collective->getGlobalTime() >= trigger.get<int>() ? 0.05 : 0;
      case AttackTriggerId::ROOM_BUILT: 
        return collective->getSquares(trigger.get<SquareType>()).size() *
          getRoomProb(trigger.get<SquareType>().getId());
      case AttackTriggerId::POWER: 
        return powerMaxProb *
            powerClosenessFun(self->getCollective()->getDangerLevel(), collective->getDangerLevel());
      case AttackTriggerId::FINISH_OFF:
        return finishOffMaxProb * getFinishOffProb(self->maxEnemyPower, collective->getDangerLevel(),
            self->getCollective()->getDangerLevel());
      case AttackTriggerId::SELF_VICTIMS:
        return victimsMaxProb * victimsFun(self->victims, 0);
      case AttackTriggerId::ENEMY_POPULATION:
        return populationMaxProb * populationFun(
            collective->getCreatures(MinionTrait::FIGHTER).size(), trigger.get<int>());
      case AttackTriggerId::GOLD:
        return goldMaxProb * goldFun(collective->numResource(Collective::ResourceId::GOLD), trigger.get<int>());
      case AttackTriggerId::STOLEN_ITEMS:
        return stolenMaxProb 
          * stolenItemsFun(self->stolenItemCount);
      case AttackTriggerId::ENTRY:
        return entryMaxProb * self->entries;
      case AttackTriggerId::PROXIMITY:
        if (!collective->getGame()->isSingleModel() &&
            collective->getGame()->getModelDistance(collective, self->getCollective()) <= 1)
          return proximityMaxProb;
        else
          return 0;
    }
  return 0;
}

double VillageControl::Villain::getAttackProbability(const VillageControl* self) const {
  double ret = 0;
  for (auto& elem : triggers) {
    double val = getTriggerValue(elem, self);
    CHECK(val >= 0 && val <= 1);
    ret = max(ret, val);
    Debug() << "trigger " << EnumInfo<AttackTriggerId>::getString(elem.getId()) << " village "
        << self->getCollective()->getName().getFull() << " under attack probability " << val;
  }
  return ret;
}

