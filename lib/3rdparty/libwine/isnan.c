/*
 * isnan function
 *
 * Copyright 2008 Jacek Caban for CodeWeavers
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

#include "config.h"
#include "wine/port.h"

#if !defined(HAVE_ISNAN) && !defined(isnan)

#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>

int isnan(double x)
{
  return isnand(x);
}

#elif defined(HAVE_FLOAT_H) && defined(HAVE__ISNAN)
#include <float.h>

int isnan(double x)
{
  return _isnan(x);
}

#else
#error No isnan() implementation available.
#endif

#endif /* !defined(HAVE_ISNAN) && !defined(isnan) */
