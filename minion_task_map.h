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

#ifndef _MINION_TASK_MAP_H
#define _MINION_TASK_MAP_H

#include "minion_task.h"

class MinionTaskMap {
  public:
  void setValue(MinionTask, double);
  void setWorkshopTasks(double);
  void clear();

  void toggleLock(MinionTask);
  bool isLocked(MinionTask) const;
  
  double getValue(MinionTask, bool ignoreTaskLock = false) const;
  bool hasAnyTask() const;
  
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);
  
  private:
  EnumMap<MinionTask, double> SERIAL(tasks);
  EnumSet<MinionTask> SERIAL(locked);
};

BOOST_CLASS_VERSION(MinionTaskMap, 1);

#endif
