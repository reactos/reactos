
/*
 * Core Audio audio session types definitions
 *
 * Copyright 2009 Maarten Lankhorst
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
 *
 */

#ifndef __AUDIOSESSIONTYPES__
#define __AUDIOSESSIONTYPES__

typedef enum _AUDCLNT_SHAREMODE
{
    AUDCLNT_SHAREMODE_SHARED,
    AUDCLNT_SHAREMODE_EXCLUSIVE,
} AUDCLNT_SHAREMODE;

#define AUDCLNT_STREAMFLAGS_CROSSPROCESS 0x00010000
#define AUDCLNT_STREAMFLAGS_LOOPBACK 0x00020000
#define AUDCLNT_STREAMFLAGS_EVENTCALLBACK 0x00040000
#define AUDCLNT_STREAMFLAGS_NOPERSIST 0x00080000
#define AUDCLNT_STREAMFLAGS_RATEADJUST 0x00100000
#define AUDCLNT_SESSIONFLAGS_EXPIREWHENUNOWNED 0x10000000
#define AUDCLNT_SESSIONFLAGS_DISPLAY_HIDE 0x20000000
#define AUDCLNT_SESSIONFLAGS_DISPLAY_HIDEWHENEXPIRED 0x40000000

typedef enum _AudioSessionState
{
    AudioSessionStateInactive = 0,
    AudioSessionStateActive,
    AudioSessionStateExpired,
} AudioSessionState;

#endif
