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

#ifndef _OPTIONS_H
#define _OPTIONS_H

#include "view.h"
#include "util.h"

enum class OptionId {
  HINTS,
  ASCII,
  MUSIC,
  KEEP_SAVEFILES,
  SMOOTH_MOVEMENT,
  FULLSCREEN,

  FAST_IMMIGRATION,
  ADVENTURER_NAME,

  KEEPER_NAME,
  SHOW_MAP,
  START_WITH_NIGHT,
  STARTING_RESOURCE,
};

enum class OptionSet {
  GENERAL,
  KEEPER,
  ADVENTURER,
};

ENUM_HASH(OptionId);

class Options {
  public:
  typedef variant<bool, string> Value;
  Options(const string& path);
  bool getBoolValue(OptionId);
  string getStringValue(OptionId);
  void handle(View*, OptionSet, int lastIndex = 0);
  bool handleOrExit(View*, OptionSet, int lastIndex = -1);
  typedef function<void(bool)> Trigger;
  void addTrigger(OptionId, Trigger trigger);
  void setDefaultString(OptionId, const string&);

  private:
  void setValue(OptionId, Value);
  void changeValue(OptionId, const Options::Value&, View*);
  string getValueString(OptionId, Options::Value);
  Value getValue(OptionId);
  unordered_map<OptionId, Value> readValues();
  void writeValues(const unordered_map<OptionId, Value>&);
  string filename;
  unordered_map<OptionId, string> defaultStrings;
};


#endif
