/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: misc.h,v 1.2 2003/05/18 17:16:17 ea Exp $ */
#ifndef __ENG_MISC_H
#define __ENG_MISC_H

#ifndef __ENG_OBJECTS_H
#include "objects.h"
#endif

typedef struct INTENG_ENTER_LEAVE_TAG
  {
  /* Contents is private to EngEnter/EngLeave */
  SURFOBJ *DestObj;
  SURFGDI *DestGDI;
  SURFOBJ *OutputObj;
  HBITMAP OutputBitmap;
  CLIPOBJ *TrivialClipObj;
  RECTL DestRect;
  BOOL ReadOnly;
  } INTENG_ENTER_LEAVE, *PINTENG_ENTER_LEAVE;

extern BOOL STDCALL IntEngEnter(PINTENG_ENTER_LEAVE EnterLeave,
                                SURFOBJ *DestObj,
                                RECTL *DestRect,
                                BOOL ReadOnly,
                                POINTL *Translate,
                                SURFOBJ **OutputObj);
extern BOOL STDCALL IntEngLeave(PINTENG_ENTER_LEAVE EnterLeave);

#endif
