/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
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

#ifndef __VERSION_H
#define __VERSION_H


/* just some stuff */
#define VERSION			"FreeLoader v1.8.6"
#define COPYRIGHT		"Copyright (C) 1998-2003 Brian Palmer <brianp@sginet.com>"
#define AUTHOR_EMAIL	"<brianp@sginet.com>"
#define BY_AUTHOR		"by Brian Palmer"

// FreeLoader version defines
//
// NOTE:
// If you fix bugs then you increment the patch version
// If you add features then you increment the minor version and zero the patch version
// If you add major functionality then you increment the major version and zero the minor & patch versions
//
#define FREELOADER_MAJOR_VERSION	1
#define FREELOADER_MINOR_VERSION	8
#define FREELOADER_PATCH_VERSION	6


PUCHAR	GetFreeLoaderVersionString(VOID);


#endif  // defined __VERSION_H
