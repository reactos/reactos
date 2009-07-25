/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window classes
 * FILE:             subsystems/win32/win32k/ntuser/class.c
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
GetCallProcHandle(IN PCALLPROCDATA CallProc)
{
    /* FIXME - check for 64 bit architectures... */
    return (WNDPROC)((ULONG_PTR)UserObjectToHandle(CallProc) | 0xFFFF0000);
}

VOID
DestroyCallProc(IN PDESKTOPINFO Desktop,
                IN OUT PCALLPROCDATA CallProc)
{
    /* FIXME - use new object manager! */
    HANDLE Handle = UserObjectToHandle(CallProc);

    UserDeleteObject(Handle,
                    otCallProc);
}

PCALLPROCDATA
CloneCallProc(IN PDESKTOPINFO Desktop,
              IN PCALLPROCDATA CallProc)
{
    PCALLPROCDATA NewCallProc;
    HANDLE Handle;

    /* FIXME - use new object manager! */
    NewCallProc = (PCALLPROCDATA)UserCreateObject(gHandleTable,
                                             &Handle,
                                             otCallProc,
                                             sizeof(CALLPROCDATA));
    if (NewCallProc != NULL)
    {
        NewCallProc->hdr.Handle = Handle; /* FIXME: Remove hack */
        NewCallProc->pfnClientPrevious = CallProc->pfnClientPrevious;
        NewCallProc->Unicode = CallProc->Unicode;
        NewCallProc->spcpdNext = NULL;
    }

    return NewCallProc;
}

PCALLPROCDATA
CreateCallProc(IN PDESKTOPINFO Desktop,
               IN WNDPROC WndProc,
               IN BOOL Unicode,
               IN PPROCESSINFO pi)
{
    PCALLPROCDATA NewCallProc;
    HANDLE Handle;

    /* FIXME - use new object manager! */
    NewCallProc = (PCALLPROCDATA)UserCreateObject(gHandleTable,
                                             &Handle,
                                             otCallProc,
                                             sizeof(CALLPROCDATA));
    if (NewCallProc != NULL)
    {
        NewCallProc->hdr.Handle = Handle; /* FIXME: Remove hack */
        NewCallProc->pfnClientPrevious = WndProc;
        NewCallProc->Unicode = Unicode;
        NewCallProc->spcpdNext = NULL;
    }

    return NewCallProc;
}

BOOL
UserGetCallProcInfo(IN HANDLE hCallProc,
                    OUT PWNDPROC_INFO wpInfo)
{
    PCALLPROCDATA CallProc;

    /* NOTE: Accessing the WNDPROC_INFO structure may raise an exception! */

    /* FIXME - use new object manager! */
    CallProc = UserGetObject(gHandleTable,
                             hCallProc,
                             otCallProc);
    if (CallProc == NULL)
    {
        return FALSE;
    }

/* Use Handle pEntry->ppi!
    if (CallProc->pi != GetW32ProcessInfo())
    {
        return FALSE;
    }*/

    wpInfo->WindowProc = CallProc->pfnClientPrevious;
    wpInfo->IsUnicode = CallProc->Unicode;

    return TRUE;
}

/* SYSCALLS *****************************************************************/

/* 
   Retrieve the WinProcA/W or CallProcData handle for Class, Dialog or Window.
   This Function called from user space uses Window handle for class, window
   and dialog procs only.

   Note:
      ProcIn is the default proc from pCls/pDlg/pWnd->lpfnXxyz, caller is
      looking for another type of proc if the original lpfnXxyz proc is preset
      to Ansi or Unicode.

      Example:
        If pWnd is created from Ansi and lpfnXxyz is assumed to be Ansi, caller
        will ask for Unicode Proc return Proc or CallProcData handle.

   This function should replaced NtUserGetClassLong and NtUserGetWindowLong.
 */
ULONG_PTR
APIENTRY
NtUserGetCPD(
   HWND hWnd,
   GETCPD Flags,
   ULONG_PTR ProcIn)
{
   PWINDOW_OBJECT Window;
   PWND Wnd;
   PCLS Class;
   ULONG_PTR Result = 0;
   BOOL Ansi;

   UserEnterExclusive();
   if (!(Window = UserGetWindowObject(hWnd)) || !Window->Wnd)
   {   
      goto Cleanup;
   }
   Wnd = Window->Wnd;
   Class = Wnd->pcls;
   /* Ex: Retrieve the Unicode Proc since the default is Ansi. */
   Ansi = (Flags & UserGetCPDA2U); // Ansi to Unicode request from user.

   if ( Flags & (UserGetCPDWindow|UserGetCPDDialog))
   {
      Result = UserGetWindowLong( hWnd, GWL_WNDPROC, Ansi);
   }
   else if (Flags & (UserGetCPDClass|UserGetCPDWndtoCls))
   {
      Result = UserGetClassLongPtr( Class, GCLP_WNDPROC, Ansi);
   }

Cleanup:
   UserLeave();
   return Result;
}

