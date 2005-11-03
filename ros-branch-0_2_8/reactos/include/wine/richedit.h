/*
 * Copyright (C) 2000 Jean-Claude Batista
 * Copyright (C) 2002 Andriy Palamarchuk
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

#include_next <richedit.h>

#ifndef __WINE_RICHEDIT_H
#define __WINE_RICHEDIT_H

#include "pshpack4.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _RICHEDIT_VER
#define _RICHEDIT_VER   0x0210
#endif /* _RICHEDIT_VER */

#define cchTextLimitDefault 0x7fff

#define RICHEDIT_CLASS20A	"RichEdit20A"
#if defined(__GNUC__)
# define RICHEDIT_CLASS20W (const WCHAR []){ 'R','i','c','h','E','d','i','t','2','0','W',0 }
#elif defined(_MSC_VER)
# define RICHEDIT_CLASS20W      L"RichEdit20W"
#else
static const WCHAR RICHEDIT_CLASS20W[] = { 'R','i','c','h','E','d','i','t','2','0','W',0 };
#endif
#define RICHEDIT_CLASS10A	"RICHEDIT"

#ifdef __cplusplus
}
#endif

#include "poppack.h"
#endif /* __WINE_RICHEDIT_H */
