/*
 * WOW Generic Thunk API
 *
 * Copyright (C) 1999 Ulrich Weigand
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

#ifndef WINE_WOWNT32_H_
#define WINE_WOWNT32_H_

#ifndef HWND_16
#define HWND_16(h32)      (LOWORD(h32))
#endif

#ifndef HWND_32
#define HWND_32(h16)      ((HWND)      (ULONG_PTR)(h16))
#endif

#endif /* _WOWNT32_H_ */

