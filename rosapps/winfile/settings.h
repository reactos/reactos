/*
 *  ReactOS winfile
 *
 *  settings.h
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#ifdef __cplusplus
extern "C" {
#endif

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"


#define CONFIRM_FILE_DELETE    0x0001
#define CONFIRM_DIR_DELETE     0x0002
#define CONFIRM_FILE_REPLACE   0x0004
#define CONFIRM_MOUSE_ACTIONS  0x0008
#define CONFIRM_DISK_COMMANDS  0x0010
#define CONFIRM_MODIFY_SYSTEM  0x0020

#define VIEW_DIRECTORIES       0x0001
#define VIEW_PROGRAMS          0x0002
#define VIEW_DOCUMENTS         0x0004
#define VIEW_OTHER             0x0008
#define VIEW_SYSTEM            0x0010

#define MAX_TYPE_MASK_LEN 50

extern DWORD Confirmation;
extern DWORD ViewType;
extern TCHAR ViewTypeMaskStr[MAX_TYPE_MASK_LEN];
//extern LPCTSTR lpViewTypeMaskStr;



#ifdef __cplusplus
};
#endif

#endif // __SETTINGS_H__
