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
#ifndef _KNOWN_TILES_H
#define _KNOWN_TILES_H

#include "util.h"
#include "position_map.h"

class KnownTiles {
  public:
  void addTile(Position);
  bool isKnown(Position) const;
  const set<Position>& getBorderTiles() const;
  void limitToModel(const Model*);

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  private:
  PositionMap<bool> SERIAL(known);
  set<Position> SERIAL(border);
};

#endif
