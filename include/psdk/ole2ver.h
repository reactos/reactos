/*
 * Version number info
 *
 * Copyright (C) 1999 Paul Quinn
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_OLE2VER_H
#define __WINE_OLE2VER_H

/*
 * other versions rmm/rup:
 * 23/639
 * 23/700
 * 23/730
 * 23/824
 *
 * Win98 SE original files:
 * COMPOBJ: CoBuildVersion 23/700
 * OLE2: OleBuildVersion -> COMPOBJ.CoBuildVersion
 * OLE32: CoBuildVersion and Ole~ 23/824
 *
 * We probably should reorganize the OLE version stuff, i.e.
 * use different values for every *BuildVersion function and Win version.
 */

/* bad: we shouldn't make use of it that globally ! */
#define rmm             23
#define rup		824

#endif  /* __WINE_OLE2VER_H */
