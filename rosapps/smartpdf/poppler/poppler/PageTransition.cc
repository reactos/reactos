/* PageTransition.cc
 * Copyright (C) 2005, Net Integration Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <iostream>

#include "Error.h"
#include "Object.h"
#include "PageTransition.h"
#include "Private.h"

namespace Poppler {

class PageTransitionData
{
  public:
    PageTransition::Type type;
    int duration;
    PageTransition::Alignment alignment;
    PageTransition::Direction direction;
    int angle;
    double scale;
    bool rectangular;
};

PageTransition::PageTransition(const PageTransitionParams &params)
{
  data = new PageTransitionData();
  data->type = Replace;
  data->duration = 1;
  data->alignment = Horizontal;
  data->direction = Inward;
  data->angle = 0;
  data->scale = 1.0;
  data->rectangular = false;

  // Paranoid safety checks
  if (params.dictObj == 0) {
    error(-1, "PageTransition::PageTransition() called with params.dictObj==0");
    return;
  }
  if (params.dictObj->isDict() == false) {
    error(-1, "PageTransition::PageTransition() called with params.dictObj->isDict()==false");
    return;
  }

  // Obtain a pointer to the dictionary and start parsing.
  Dict *transDict = params.dictObj->getDict();
  Object obj;

  if (transDict->lookup("S", &obj)->isName()) {
    const char *s = obj.getName();
    if (strcmp("R", s) == 0)
      data->type = Replace;
    else if (strcmp("Split", s) == 0)
      data->type = Split;
    else if (strcmp("Blinds", s) == 0)
      data->type = Blinds;
    else if (strcmp("Box", s) == 0)
      data->type = Box;
    else if (strcmp("Wipe", s) == 0)
      data->type = Wipe;
    else if (strcmp("Dissolve", s) == 0)
      data->type = Dissolve;
    else if (strcmp("Glitter", s) == 0)
      data->type = Glitter;
    else if (strcmp("Fly", s) == 0)
      data->type = Fly;
    else if (strcmp("Push", s) == 0)
      data->type = Push;
    else if (strcmp("Cover", s) == 0)
      data->type = Cover;
    else if (strcmp("Uncover", s) == 0)
      data->type = Push;
    else if (strcmp("Fade", s) == 0)
      data->type = Cover;
  }
  obj.free();
  
  if (transDict->lookup("D", &obj)->isInt()) {
    data->duration = obj.getInt();
  }
  obj.free();

  if (transDict->lookup("Dm", &obj)->isName()) {
    const char *dm = obj.getName();
    if ( strcmp( "H", dm ) == 0 )
      data->alignment = Horizontal;
    else if ( strcmp( "V", dm ) == 0 )
      data->alignment = Vertical;
  }
  obj.free();
  
  if (transDict->lookup("M", &obj)->isName()) {
    const char *m = obj.getName();
    if ( strcmp( "I", m ) == 0 )
      data->direction = Inward;
    else if ( strcmp( "O", m ) == 0 )
      data->direction = Outward;
  }
  obj.free();
  
  if (transDict->lookup("Di", &obj)->isInt()) {
    data->angle = obj.getInt();
  }
  obj.free();
  
  if (transDict->lookup("Di", &obj)->isName()) {
    if ( strcmp( "None", obj.getName() ) == 0 )
      data->angle = 0;
  }
  obj.free();
  
  if (transDict->lookup("SS", &obj)->isReal()) {
    data->scale = obj.getReal();
  }
  obj.free();
  
  if (transDict->lookup("B", &obj)->isBool()) {
    data->rectangular = obj.getBool();
  }
  obj.free();
}

PageTransition::PageTransition(const PageTransition &pt)
{
  data = new PageTransitionData();
  data->type = pt.data->type;
  data->duration = pt.data->duration;
  data->alignment = pt.data->alignment;
  data->direction = pt.data->direction;
  data->angle = pt.data->angle;
  data->scale = pt.data->scale;
  data->rectangular = pt.data->rectangular;
}

PageTransition::~PageTransition()
{
}

PageTransition::Type PageTransition::type() const
{
  return data->type;
}

int PageTransition::duration() const
{
  return data->duration;
}

PageTransition::Alignment PageTransition::alignment() const
{
  return data->alignment;
}

PageTransition::Direction PageTransition::direction() const
{
  return data->direction;
}

int PageTransition::angle() const
{
  return data->angle;
}

double PageTransition::scale() const
{
  return data->scale;
}
bool PageTransition::isRectangular() const
{
  return data->rectangular;
}

}
