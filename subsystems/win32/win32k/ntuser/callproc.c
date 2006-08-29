/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998 - 2006 ReactOS Team
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
/* $Id: class.c 21596 2006-04-15 10:41:58Z greatlrd $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window classes
 * FILE:             subsys/win32k/ntuser/class.c
 * PROGRAMER:        Thomas Weidenmueller <w3seek@reactos.com>
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* CALLPROC ******************************************************************/

WNDPROC
GetCallProcHandle(IN PCALLPROC CallProc)
{
    /* FIXME - check for 64 bit architectures... */
    return (WNDPROC)((ULONG_PTR)ObmObjectToHandle(CallProc) | 0xFFFF0000);
}

VOID
DestroyCallProc(IN PDESKTOP Desktop,
                IN OUT PCALLPROC CallProc)
{
    /* FIXME - use new object manager! */
    HANDLE Handle = ObmObjectToHandle(CallProc);

    ObmDeleteObject(Handle,
                    otCallProc);
}

PCALLPROC
CloneCallProc(IN PDESKTOP Desktop,
              IN PCALLPROC CallProc)
{
    PCALLPROC NewCallProc;
    HANDLE Handle;

    /* FIXME - use new object manager! */
    NewCallProc = (PCALLPROC)ObmCreateObject(gHandleTable,
                                             &Handle,
                                             otCallProc,
                                             sizeof(CALLPROC));
    if (NewCallProc != NULL)
    {
        NewCallProc->pi = CallProc->pi;
        NewCallProc->WndProc = CallProc->WndProc;
        NewCallProc->Unicode = CallProc->Unicode;
    }

    return NewCallProc;
}

PCALLPROC
CreateCallProc(IN PDESKTOP Desktop,
               IN WNDPROC WndProc,
               IN BOOL Unicode,
               IN PW32PROCESSINFO pi)
{
    PCALLPROC NewCallProc;
    HANDLE Handle;

    /* FIXME - use new object manager! */
    NewCallProc = (PCALLPROC)ObmCreateObject(gHandleTable,
                                             &Handle,
                                             otCallProc,
                                             sizeof(CALLPROC));
    if (NewCallProc != NULL)
    {
        NewCallProc->pi = pi;
        NewCallProc->WndProc = WndProc;
        NewCallProc->Unicode = Unicode;
    }

    return NewCallProc;
}

BOOL
UserGetCallProcInfo(IN HANDLE hCallProc,
                    OUT PWNDPROC_INFO wpInfo)
{
    PCALLPROC CallProc;

    /* NOTE: Accessing the WNDPROC_INFO structure may raise an exception! */

    /* FIXME - use new object manager! */
    CallProc = UserGetObject(gHandleTable,
                             hCallProc,
                             otCallProc);
    if (CallProc == NULL)
    {
        return FALSE;
    }

    if (CallProc->pi != GetW32ProcessInfo())
    {
        return FALSE;
    }

    wpInfo->WindowProc = CallProc->WndProc;
    wpInfo->IsUnicode = CallProc->Unicode;

    return TRUE;
}

BOOL NTAPI
NtUserDereferenceWndProcHandle(IN HANDLE wpHandle,
                               OUT PWNDPROC_INFO wpInfo)
{
    BOOL Ret = FALSE;

    UserEnterShared();

    _SEH_TRY
    {
        ProbeForWrite(wpInfo,
                      sizeof(WNDPROC_INFO),
                      sizeof(ULONG));

        Ret = UserGetCallProcInfo(wpHandle,
                                  wpInfo);
    }
    _SEH_HANDLE
    {
        SetLastWin32Error(_SEH_GetExceptionCode());
    }
    _SEH_END;

    UserLeave();

    return Ret;
}
