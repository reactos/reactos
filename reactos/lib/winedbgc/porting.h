/*
 * Porting from wine to ReactOS definitions
 *
 * Copyright 2002 Robert Dickenson
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

#ifndef __PORTING_WINE2ROS_H
#define __PORTING_WINE2ROS_H


#ifndef __GNUC__
#define inline _inline
#endif

long interlocked_xchg_add(long *dest, long incr);

#define strlenW wcslen

#define strstrW wcsstr
#define strtolW wcstol
#define strncasecmp strncmp
#define snprintf _snprintf
#define strcasecmp _stricmp


#endif  /* __PORTING_WINE2ROS_H */
