/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/native/utils/keytrans.h
 * PURPOSE:         Keyboard translation functionality
 * PROGRAMMER:      Tinus
 */

#pragma once

#include <ntddkbd.h>

NTSTATUS
IntTranslateKey(HANDLE hConsoleInput, PKEYBOARD_INPUT_DATA InputData, KEY_EVENT_RECORD *Event);

/* EOF */
