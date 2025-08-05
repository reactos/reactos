/*
 * Variable argument definitions
 *
 * Copyright 2022 Jacek Caban
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

#ifndef _INC_STDARG
#define _INC_STDARG

#include <vadefs.h>

#define va_start(v,l)  _crt_va_start(v,l)
#define va_arg(v,l)    _crt_va_arg(v,l)
#define va_end(v)      _crt_va_end(v)
#define va_copy(d,s)   _crt_va_copy(d,s)

#endif /* _INC_STDARG */
