/*
 * Shared Resource/DllGetVersion version information
 *
 * Copyright (C) 2004 Robert Shearman
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

#pragma once

#if (NTDDI_VERSION < NTDDI_LONGHORN)

/* Version from Windows Server 2003 SP2 */

#define WINE_FILEVERSION_MAJOR         6
#define WINE_FILEVERSION_MINOR         0
#define WINE_FILEVERSION_BUILD      3790
#define WINE_FILEVERSION_PLATFORMID 3959

/* FIXME: when libs/wpp gets fixed to support concatenation we can remove
 * this and define it in version.rc */
#define WINE_FILEVERSION_STR "6.0.3790.3959"

#else

/* Version from Windows Vista RTM */

#define WINE_FILEVERSION_MAJOR         6
#define WINE_FILEVERSION_MINOR         0
#define WINE_FILEVERSION_BUILD      6000
#define WINE_FILEVERSION_PLATFORMID 16386

/* FIXME: when libs/wpp gets fixed to support concatenation we can remove
 * this and define it in version.rc */
#define WINE_FILEVERSION_STR "6.0.6000.16386"

#endif // NTDDI_VERSION
