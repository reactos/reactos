/*
 * Copyright (C) the Wine project
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

#ifndef __WINE_WINERROR_H
#define __WINE_WINERROR_H

#include_next <winerror.h>
#include <w32api.h>

#ifdef RC_INVOKED
#define _HRESULT_TYPEDEF_(x) (x)
#else
#define _HRESULT_TYPEDEF_(x) ((HRESULT)x)
#endif

/* ERROR_UNKNOWN is a placeholder for error conditions which haven't
 * been tested yet so we're not exactly sure what will be returned.
 * All instances of ERROR_UNKNOWN should be tested under Win95/NT
 * and replaced.
 */
#define ERROR_UNKNOWN                                      99999
#define E_UNSPEC                                           E_FAIL
#if (__W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5)
#define CO_S_NOTALLINTERFACES                              _HRESULT_TYPEDEF_(0x00080012L)
#endif

#endif  /* __WINE_WINERROR_H */
