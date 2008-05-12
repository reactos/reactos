/*
 * Win32 definitions for Windows NT
 *
 * Copyright 1996 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_WINNT_H
#define __WINE_WINNT_H

#include_next <winnt.h>

/* non standard; keep the number high enough (but < 0xff) */
#define LANG_ESPERANTO                   0x8f
#define LANG_WALON                       0x90
#define LANG_CORNISH                     0x91
#define LANG_WELSH                       0x92
#define LANG_BRETON                      0x93

/* FIXME: these are not in the Windows header */
#define LANG_GAELIC         0x94
#define LANG_MALTESE        0x3a
#define LANG_ROMANSH        0x17
#define LANG_SAAMI          0x3b
#define LANG_LOWER_SORBIAN  0x2e
#define LANG_UPPER_SORBIAN  0x2e
#define LANG_SUTU           0x30
#define LANG_TAJIK          0x28
#define LANG_TSONGA         0x31
#define LANG_TSWANA         0x32
#define LANG_VENDA          0x33
#define LANG_XHOSA          0x34
#define LANG_ZULU           0x35

#define WINE_UNUSED   __attribute__((unused))

#endif  /* __WINE_WINNT_H */
