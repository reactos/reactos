/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/callback.h
 * PURPOSE:         Executive callbacks Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * PORTABILITY:     Checked.
 * UPDATE HISTORY:
 *                  Created 30/05/04
 */

#ifndef __INCLUDE_INTERNAL_CALLBACKS_H
#define __INCLUDE_INTERNAL_CALLBACKS_H

/* Tag */
#define CALLBACK_TAG	TAG('C','L','B','K')
 
/* ROS Callback Object */
typedef struct _INT_CALLBACK_OBJECT {
   KSPIN_LOCK Lock;
   LIST_ENTRY RegisteredCallbacks;
   ULONG AllowMultipleCallbacks;
} _INT_CALLBACK_OBJECT , *PINT_CALLBACK_OBJECT;

/* Structure used to hold Callbacks */
typedef struct _CALLBACK_REGISTRATION {
   LIST_ENTRY RegisteredCallbacks;
   PINT_CALLBACK_OBJECT CallbackObject;
   PCALLBACK_FUNCTION CallbackFunction;
   PVOID CallbackContext;
   ULONG InUse;
   BOOLEAN PendingDeletion;
} CALLBACK_REGISTRATION, *PCALLBACK_REGISTRATION;

/* Add 0x1 flag to differentiate */
#define CALLBACK_ALL_ACCESS		(STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|0x0001)
#define CALLBACK_EXECUTE		(STANDARD_RIGHTS_EXECUTE|SYNCHRONIZE|0x0001)
#define CALLBACK_WRITE			(STANDARD_RIGHTS_WRITE|SYNCHRONIZE|0x0001)
#define CALLBACK_READ			(STANDARD_RIGHTS_READ|SYNCHRONIZE|0x0001)

/* Mapping for Callback Object */
GENERIC_MAPPING ExpCallbackMapping = 
{
   CALLBACK_READ,
   CALLBACK_WRITE,
   CALLBACK_EXECUTE,
   CALLBACK_ALL_ACCESS
};

/* Kernel Default Callbacks */
PINT_CALLBACK_OBJECT SetSystemTimeCallback;
PINT_CALLBACK_OBJECT SetSystemStateCallback;
PINT_CALLBACK_OBJECT PowerStateCallback;

typedef struct {
    PINT_CALLBACK_OBJECT *CallbackObject;
    PWSTR Name;
} SYSTEM_CALLBACKS;

SYSTEM_CALLBACKS ExpInitializeCallback[] = {
   {&SetSystemTimeCallback, L"\\Callback\\SetSystemTime"},
   {&SetSystemStateCallback, L"\\Callback\\SetSystemState"},
   {&PowerStateCallback, L"\\Callback\\PowerState"},
   {NULL, NULL}
};

/* Callback Event */
KEVENT ExpCallbackEvent;

/* Callback Object */
POBJECT_TYPE ExCallbackObjectType;

#endif /* __INCLUDE_INTERNAL_CALLBACKS_H */

/* EOF */
