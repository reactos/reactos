/*
 *  FreeLoader
 *  Copyright (C) 2001-2005  Brian Palmer  <brianp@sginet.com>
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

#define COPYRIGHT       "Copyright (C) 1996-" COPYRIGHT_YEAR " ReactOS Project"
#define AUTHOR_EMAIL    "<www.reactos.org>"
#define BY_AUTHOR       "by ReactOS Project"

// FreeLoader version defines
// If you add features then you increment the minor version
// If you add major functionality then you increment the major version and zero the minor version
#define FREELOADER_MAJOR_VERSION    3
#define FREELOADER_MINOR_VERSION    2
#define TOSTRING_(X) #X
#define TOSTRING(X) TOSTRING_(X)
#define VERSION "FreeLoader v" TOSTRING(FREELOADER_MAJOR_VERSION) "." TOSTRING(FREELOADER_MINOR_VERSION)
