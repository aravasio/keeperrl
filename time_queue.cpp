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

#include "time_queue.h"
#include "creature.h"
#include "view_object.h"

template <class Archive> 
void TimeQueue::serialize(Archive& ar, const unsigned int version) { 
  serializeAll(ar, creatures);
}

SERIALIZABLE(TimeQueue);

void TimeQueue::addCreature(PCreature c) {
  getQueue().insert(c.get());
  creatures.push_back(std::move(c));
}

// Queue is initialized in a lazy manner because during deserialization the comparator doesn't 
// work, as the Creatures are still being deserialized.
TimeQueue::Queue& TimeQueue::getQueue() {
  if (!queue) {
    queue.emplace([](const Creature* c1, const Creature* c2) {
        return c1->getLocalTime() < c2->getLocalTime() ||
          (c1->getLocalTime() == c2->getLocalTime() && c1->getUniqueId() > c2->getUniqueId());});
    for (PCreature& c : creatures)
      queue->insert(c.get());
  }
  return *queue;
}
  
PCreature TimeQueue::removeCreature(Creature* cRef) {
  for (int i : All(creatures))
    if (creatures[i].get() == cRef) {
      getQueue().erase(cRef);
      PCreature ret = std::move(creatures[i]);
      creatures.erase(creatures.begin() + i);
      return ret;
    }
  FAIL << "Creature not found";
  return nullptr;
}

vector<Creature*> TimeQueue::getAllCreatures() const {
  return extractRefs(creatures);
}

Creature* TimeQueue::getNextCreature() {
  if (creatures.empty())
    return nullptr;
  else
    return *getQueue().begin();
}

void TimeQueue::beforeUpdateTime(Creature* c) {
  getQueue().erase(c);
}

void TimeQueue::afterUpdateTime(Creature* c) {
  getQueue().insert(c);
}

