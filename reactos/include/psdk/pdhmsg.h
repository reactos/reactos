/*
 * Performance Data Helper
 *
 * Copyright 2007 Hans Leidekker
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

#ifndef _PDH_MSG_H_
#define _PDH_MSG_H_

#define PDH_CSTATUS_VALID_DATA          0x00000000
#define PDH_CSTATUS_NO_MACHINE          0x800007d0
#define PDH_MORE_DATA                   0x800007d2
#define PDH_NO_DATA                     0x800007d5
#define PDH_CSTATUS_NO_OBJECT           0xc0000bb8
#define PDH_CSTATUS_NO_COUNTER          0xc0000bb9
#define PDH_MEMORY_ALLOCATION_FAILURE   0xc0000bbb
#define PDH_INVALID_HANDLE              0xc0000bbc
#define PDH_INVALID_ARGUMENT            0xc0000bbd
#define PDH_CSTATUS_BAD_COUNTERNAME     0xc0000bc0
#define PDH_INSUFFICIENT_BUFFER         0xc0000bc2
#define PDH_INVALID_DATA                0xc0000bc6
#define PDH_NOT_IMPLEMENTED             0xc0000bd3
#define PDH_STRING_NOT_FOUND            0xc0000bd4

#endif /* _PDH_MSG_H_ */
