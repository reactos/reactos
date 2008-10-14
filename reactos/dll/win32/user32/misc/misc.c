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
 */
/*
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/misc/misc.c
 * PURPOSE:         Misc
 * PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * UPDATE HISTORY:
 *      19-11-2003  Created
 */

/* INCLUDES ******************************************************************/

#include <user32.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
DWORD
STDCALL
GetGuiResources(
  HANDLE hProcess,
  DWORD uiFlags)
{
  return NtUserGetGuiResources(hProcess, uiFlags);
}


/*
 * Private calls for CSRSS
 */
VOID
STDCALL
PrivateCsrssManualGuiCheck(LONG Check)
{
  NtUserCallOneParam(Check, ONEPARAM_ROUTINE_CSRSS_GUICHECK);
}

VOID
STDCALL
PrivateCsrssInitialized(VOID)
{
  NtUserCallNoParam(NOPARAM_ROUTINE_CSRSS_INITIALIZED);
}


/*
 * @implemented
 */
BOOL
STDCALL
RegisterLogonProcess(DWORD dwProcessId, BOOL bRegister)
{
  return NtUserCallTwoParam(dwProcessId,
			    (DWORD)bRegister,
			    TWOPARAM_ROUTINE_REGISTERLOGONPROC);
}

/*
 * @implemented
 */
BOOL
STDCALL
SetLogonNotifyWindow (HWND Wnd, HWINSTA WinSta)
{
  /* Maybe we should call NtUserSetLogonNotifyWindow and let that one inform CSRSS??? */
  CSR_API_MESSAGE Request;
  ULONG CsrRequest;
  NTSTATUS Status;

  CsrRequest = MAKE_CSR_API(SET_LOGON_NOTIFY_WINDOW, CSR_GUI);
  Request.Data.SetLogonNotifyWindowRequest.LogonNotifyWindow = Wnd;

  Status = CsrClientCallServer(&Request,
			       NULL,
                   CsrRequest,
			       sizeof(CSR_API_MESSAGE));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Request.Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return(FALSE);
    }

  return(TRUE);
}

/*
 * @implemented
 */
BOOL WINAPI
UpdatePerUserSystemParameters(
   DWORD dwReserved,
   BOOL bEnable)
{
   return NtUserUpdatePerUserSystemParameters(dwReserved, bEnable);
}

PW32THREADINFO
GetW32ThreadInfo(VOID)
{
    PW32THREADINFO ti;

    ti = (PW32THREADINFO)NtCurrentTeb()->Win32ThreadInfo;
    if (ti == NULL)
    {
        /* create the W32THREADINFO structure */
        NtUserGetThreadState(THREADSTATE_GETTHREADINFO);
        ti = (PW32THREADINFO)NtCurrentTeb()->Win32ThreadInfo;
    }

    return ti;
}

PW32PROCESSINFO
GetW32ProcessInfo(VOID)
{
    PW32THREADINFO ti;
    PW32PROCESSINFO pi = NULL;

    ti = GetW32ThreadInfo();
    if (ti != NULL)
    {
        pi = ti->pi;
    }

    return pi;
}


/*
 * GetUserObjectSecurity
 *
 * Retrieves security information for user object specified
 * with handle 'hObject'. Descriptor returned in self-relative
 * format.
 *
 * Arguments:
 *  1) hObject - handle to an object to retrieve information for
 *  2) pSecurityInfo - type of information to retrieve
 *  3) pSecurityDescriptor - buffer which receives descriptor
 *  4) dwLength - size, in bytes, of buffer 'pSecurityDescriptor'
 *  5) pdwLengthNeeded - reseives actual size of descriptor
 *
 * Return Vaules:
 *  TRUE on success
 *  FALSE on failure, call GetLastError() for more information
 */
/*
 * @implemented
 */
BOOL
WINAPI
GetUserObjectSecurity(
    IN HANDLE hObject,
    IN PSECURITY_INFORMATION pSecurityInfo,
    OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN DWORD dwLength,
    OUT PDWORD pdwLengthNeeded
)
{
DWORD dwWin32Error;
NTSTATUS Status;


    Status = NtQuerySecurityObject(
        hObject,            // Object Handle
        *pSecurityInfo,     // Security Information
        pSecurityDescriptor,// Security Descriptor
        dwLength,           // Buffer Length
        pdwLengthNeeded     // Actual Length
    );

    if ( ! NT_SUCCESS( Status ) ) {
        dwWin32Error = RtlNtStatusToDosError( Status );
        NtCurrentTeb()->LastErrorValue = dwWin32Error;
        return FALSE;
    }

    return TRUE;
}


/*
 * SetUserObjectSecurity
 *
 * Sets new security descriptor to user object specified by
 * handle 'hObject'. Descriptor must be in self-relative format.
 *
 * Arguments:
 *  1) hObject - handle to an object to set information for
 *  2) pSecurityInfo - type of information to apply
 *  3) pSecurityDescriptor - buffer which descriptor to set
 *
 * Return Vaules:
 *  TRUE on success
 *  FALSE on failure, call GetLastError() for more information
 */
/*
 * @implemented
 */
BOOL
WINAPI
SetUserObjectSecurity(
    IN HANDLE hObject,
    IN PSECURITY_INFORMATION pSecurityInfo,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
)
{
DWORD dwWin32Error;
NTSTATUS Status;


    Status = NtSetSecurityObject(
        hObject,            // Object Handle
        *pSecurityInfo,     // Security Information
        pSecurityDescriptor // Security Descriptor
    );

    if ( ! NT_SUCCESS( Status ) ) {
        dwWin32Error = RtlNtStatusToDosError( Status );
        NtCurrentTeb()->LastErrorValue = dwWin32Error;
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
STDCALL
EndTask(
	HWND    hWnd,
	BOOL fShutDown,
	BOOL fForce)
{
    SendMessageW(hWnd, WM_CLOSE, 0, 0);

    if (IsWindow(hWnd))
    {
        if (fForce)
            return DestroyWindow(hWnd);
        else
            return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
STDCALL
IsGUIThread(
    BOOL bConvert)
{
  PW32THREADINFO ti = (PW32THREADINFO)NtCurrentTeb()->Win32ThreadInfo;
  if (ti == NULL)
  {
    if(bConvert)
    {
      NtUserGetThreadState(THREADSTATE_GETTHREADINFO);
      if ((PW32THREADINFO)NtCurrentTeb()->Win32ThreadInfo) return TRUE;
      else
         SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }
    return FALSE;
  }
  else
    return TRUE;
}

BOOL
FASTCALL
IsMetaFile(HDC hDc)
{
  DWORD Type = GetObjectType((HGDIOBJ) hDc);
  switch(Type)
  {
    case OBJ_METADC:
    case OBJ_METAFILE:
    case OBJ_ENHMETADC:
    case OBJ_ENHMETAFILE:
      return TRUE;
  }
  return FALSE;
}

PUSER_HANDLE_ENTRY
FASTCALL
GetUser32Handle(HANDLE handle)
{
    INT Index;
    USHORT generation;

    Index = (((UINT)handle & 0xffff) - FIRST_USER_HANDLE) >> 1;

    if (Index < 0 || Index >= gHandleTable->nb_handles)
        return NULL;

    if (!gHandleEntries[Index].type || !gHandleEntries[Index].ptr)
        return NULL;

    generation = (UINT)handle >> 16;

    if (generation == gHandleEntries[Index].generation || !generation || generation == 0xffff)
        return &gHandleEntries[Index];

    return NULL;
}

/*
 * Decide whether an object is located on the desktop or shared heap
 */
static const BOOL g_ObjectHeapTypeShared[VALIDATE_TYPE_EVENT + 1] =
{
    FALSE, /* VALIDATE_TYPE_FREE (not used) */
    TRUE, /* VALIDATE_TYPE_WIN */ /* FIXME: FALSE once WINDOW_OBJECT is deleted! */
    TRUE, /* VALIDATE_TYPE_MENU */
    TRUE, /* VALIDATE_TYPE_CURSOR */
    TRUE, /* VALIDATE_TYPE_MWPOS */
    TRUE, /* VALIDATE_TYPE_HOOK */
    FALSE, /* (not used) */
    TRUE, /* VALIDATE_TYPE_CALLPROC */
    TRUE, /* VALIDATE_TYPE_ACCEL */
    FALSE, /* (not used) */
    FALSE, /* (not used) */
    FALSE, /* (not used) */
    TRUE, /* VALIDATE_TYPE_MONITOR */
    FALSE, /* (not used) */
    FALSE, /* (not used) */
    TRUE  /* VALIDATE_TYPE_EVENT */
};

//
// Validate Handle and return the pointer to the object.
//
PVOID
FASTCALL
ValidateHandle(HANDLE handle, UINT uType)
{
  PVOID ret;
  PUSER_HANDLE_ENTRY pEntry;

  ASSERT(uType <= VALIDATE_TYPE_EVENT);

  pEntry = GetUser32Handle(handle);

  if (pEntry && uType == 0)
      uType = pEntry->type;

// Must have an entry and must be the same type!
  if ( (!pEntry) || (pEntry->type != uType) || !pEntry->ptr )
  {
     switch ( uType )
     {  // Test (with wine too) confirms these results!
        case VALIDATE_TYPE_WIN:
          SetLastError(ERROR_INVALID_WINDOW_HANDLE);
          break;
        case VALIDATE_TYPE_MENU:
          SetLastError(ERROR_INVALID_MENU_HANDLE);
          break;
        case VALIDATE_TYPE_CURSOR:
          SetLastError(ERROR_INVALID_CURSOR_HANDLE);
          break;
        case VALIDATE_TYPE_MWPOS:
          SetLastError(ERROR_INVALID_DWP_HANDLE);
          break;
        case VALIDATE_TYPE_HOOK:
          SetLastError(ERROR_INVALID_HOOK_HANDLE);
          break;
        case VALIDATE_TYPE_ACCEL:
          SetLastError(ERROR_INVALID_ACCEL_HANDLE);
          break;
        default:
          SetLastError(ERROR_INVALID_HANDLE);
          break;
    }
    return NULL;
  }

  if (g_ObjectHeapTypeShared[uType])
    ret = SharedPtrToUser(pEntry->ptr);
  else
    ret = DesktopPtrToUser(pEntry->ptr);

  return ret;
}

//
// Validate Handle and return the pointer to the object.
//
PVOID
FASTCALL
ValidateHandleNoErr(HANDLE handle, UINT uType)
{
  PVOID ret;
  PUSER_HANDLE_ENTRY pEntry;

  ASSERT(uType <= VALIDATE_TYPE_EVENT);

  pEntry = GetUser32Handle(handle);

  if (pEntry && uType == 0)
      uType = pEntry->type;

// Must have an entry and must be the same type!
  if ( (!pEntry) || (pEntry->type != uType) || !pEntry->ptr )
    return NULL;

  if (g_ObjectHeapTypeShared[uType])
    ret = SharedPtrToUser(pEntry->ptr);
  else
    ret = DesktopPtrToUser(pEntry->ptr);

  return ret;
}

//
// Validate a callproc handle and return the pointer to the object.
//
PCALLPROC
FASTCALL
ValidateCallProc(HANDLE hCallProc)
{
    PCALLPROC CallProc = ValidateHandle(hCallProc, VALIDATE_TYPE_CALLPROC);
    if (CallProc != NULL && CallProc->pi == g_kpi)
        return CallProc;

    return NULL;
}

//
// Validate a window handle and return the pointer to the object.
//
PWINDOW
FASTCALL
ValidateHwnd(HWND hwnd)
{
    PWINDOW Wnd;
    PCLIENTINFO ClientInfo = GetWin32ClientInfo();
    ASSERT(ClientInfo != NULL);

    /* See if the window is cached */
    if (hwnd == ClientInfo->CallbackWnd.hWnd)
        return ClientInfo->CallbackWnd.pvWnd;

    Wnd = ValidateHandle((HANDLE)hwnd, VALIDATE_TYPE_WIN);
    if (Wnd != NULL)
    {
        /* FIXME: Check if handle table entry is marked as deleting and
                  return NULL in this case! */

#if 0
        return Wnd;
#else
        /* HACK HACK HACK! This needs to be done until WINDOW_OBJECT is completely
           superseded by the WINDOW structure. We *ASSUME* a pointer to the WINDOW
           structure to be at the beginning of the WINDOW_OBJECT structure!!!

           !!! REMOVE AS SOON AS WINDOW_OBJECT NO LONGER EXISTS !!!
         */

        if (*((PVOID*)Wnd) != NULL)
            return DesktopPtrToUser(*((PVOID*)Wnd));
#endif
    }

    return NULL;
}

//
// Validate a window handle and return the pointer to the object.
//
PWINDOW
FASTCALL
ValidateHwndNoErr(HWND hwnd)
{
    PWINDOW Wnd;
    PCLIENTINFO ClientInfo = GetWin32ClientInfo();
    ASSERT(ClientInfo != NULL);

    /* See if the window is cached */
    if (hwnd == ClientInfo->CallbackWnd.hWnd)
        return ClientInfo->CallbackWnd.pvWnd;

    Wnd = ValidateHandleNoErr((HANDLE)hwnd, VALIDATE_TYPE_WIN);
    if (Wnd != NULL)
    {
        /* FIXME: Check if handle table entry is marked as deleting and
                  return NULL in this case! */

#if 0
        return Wnd;
#else
        /* HACK HACK HACK! This needs to be done until WINDOW_OBJECT is completely
           superseded by the WINDOW structure. We *ASSUME* a pointer to the WINDOW
           structure to be at the beginning of the WINDOW_OBJECT structure!!!

           !!! REMOVE AS SOON AS WINDOW_OBJECT NO LONGER EXISTS !!!
         */

        if (*((PVOID*)Wnd) != NULL)
            return DesktopPtrToUser(*((PVOID*)Wnd));
#endif
    }

    return NULL;
}

PWINDOW
FASTCALL
GetThreadDesktopWnd(VOID)
{
    PWINDOW Wnd = GetThreadDesktopInfo()->Wnd;
    if (Wnd != NULL)
        Wnd = DesktopPtrToUser(Wnd);
    return Wnd;
}

//
// Validate a window handle and return the pointer to the object.
//
PWINDOW
FASTCALL
ValidateHwndOrDesk(HWND hwnd)
{
    if (hwnd == HWND_DESKTOP)
        return GetThreadDesktopWnd();

    return ValidateHwnd(hwnd);
}
