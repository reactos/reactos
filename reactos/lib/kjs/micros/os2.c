/*
 * OS/2 specific functions.
 * Copyright (c) 1998 New Generation Software (NGS) Oy
 *
 * Author: Markku Rossi <mtr@ngs.fi>
 */

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA
 */

/*
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/micros/os2.c,v $
 * $Id: os2.c,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

#include "jsint.h"

#include <os2.h>

/*
 * Global functions.
 */

unsigned int
sleep (unsigned int seconds)
{
#if 0
  DosSleep ((ULONG) seconds * 1000);
#endif
  /* XXX Should count how many seconcs we actually slept and return it. */
  return 0;
}

int
usleep (unsigned int useconds)
{
#if 0
  DosSleep ((ULONG) useconds / 1000);
#endif
  return 0;
}
