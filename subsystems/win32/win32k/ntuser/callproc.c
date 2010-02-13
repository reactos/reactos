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
    return (WNDPROC)((ULONG_PTR)UserHMGetHandle(CallProc) | 0xFFFF0000);
}

VOID
DestroyCallProc(IN PDESKTOPINFO Desktop,
                IN OUT PCALLPROCDATA CallProc)
{
    UserDeleteObject(UserHMGetHandle(CallProc), otCallProc);
}

PCALLPROCDATA
CloneCallProc(IN PDESKTOP Desktop,
              IN PCALLPROCDATA CallProc)
{
    PCALLPROCDATA NewCallProc;
    HANDLE Handle;

    NewCallProc = (PCALLPROCDATA)UserCreateObject(gHandleTable,
                                             Desktop,
                                             &Handle,
                                             otCallProc,
                                             sizeof(CALLPROCDATA));
    if (NewCallProc != NULL)
    {
        NewCallProc->pfnClientPrevious = CallProc->pfnClientPrevious;
        NewCallProc->wType = CallProc->wType;
        NewCallProc->spcpdNext = NULL;
    }

    return NewCallProc;
}

PCALLPROCDATA
CreateCallProc(IN PDESKTOP Desktop,
               IN WNDPROC WndProc,
               IN BOOL Unicode,
               IN PPROCESSINFO pi)
{
    PCALLPROCDATA NewCallProc;
    HANDLE Handle;

    NewCallProc = (PCALLPROCDATA)UserCreateObject(gHandleTable,
                                             Desktop,
                                             &Handle,
                                             otCallProc,
                                             sizeof(CALLPROCDATA));
    if (NewCallProc != NULL)
    {
        NewCallProc->pfnClientPrevious = WndProc;
        NewCallProc->wType |= Unicode ? UserGetCPDA2U : UserGetCPDU2A ;
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
    wpInfo->IsUnicode = !!(CallProc->wType & UserGetCPDA2U);

    return TRUE;
}

/*
   Based on UserFindCallProc.
 */
PCALLPROCDATA
FASTCALL
UserSearchForCallProc(
   PCALLPROCDATA pcpd,
   WNDPROC WndProc,
   GETCPD Type)
{
   while ( pcpd && (pcpd->pfnClientPrevious != WndProc || pcpd->wType != Type) )
   {
      pcpd = pcpd->spcpdNext;
   }
   return pcpd;
}

/*
   Get Call Proc Data handle for the window proc being requested or create a
   new Call Proc Data handle to be return for the requested window proc.
 */
ULONG_PTR
FASTCALL
UserGetCPD(
   PVOID pvClsWnd,
   GETCPD Flags,
   ULONG_PTR ProcIn)
{
   PCLS pCls;
   PWND pWnd;
   PCALLPROCDATA CallProc = NULL;
   PTHREADINFO pti;

   pti = PsGetCurrentThreadWin32Thread();

   if ( Flags & (UserGetCPDWindow|UserGetCPDDialog) ||
        Flags & UserGetCPDWndtoCls)
   {
      pWnd = pvClsWnd;
      pCls = pWnd->pcls;
   }
   else
      pCls = pvClsWnd;

   // Search Class call proc data list.
   if (pCls->spcpdFirst)
      CallProc = UserSearchForCallProc( pCls->spcpdFirst, (WNDPROC)ProcIn, Flags);

   // No luck, create a new one for the requested proc.
   if (!CallProc)
   {
      CallProc = CreateCallProc( NULL,
                                 (WNDPROC)ProcIn,
                                 !!(Flags & UserGetCPDA2U),
                                 pti->ppi);
      if (CallProc)
      {
          CallProc->spcpdNext = pCls->spcpdFirst;
          (void)InterlockedExchangePointer((PVOID*)&pCls->spcpdFirst,
                                                    CallProc);
          CallProc->wType = Flags;
      }
   }
   return (ULONG_PTR)(CallProc ? GetCallProcHandle(CallProc) : NULL);
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
   ULONG_PTR Result = 0;

   UserEnterExclusive();
   if (!(Window = UserGetWindowObject(hWnd)) || !Window->Wnd)
   {   
      goto Cleanup;
   }
   Wnd = Window->Wnd;

   // Processing Window only from User space.
   if ((Flags & ~(UserGetCPDU2A|UserGetCPDA2U)) != UserGetCPDClass)        
      Result = UserGetCPD(Wnd, Flags, ProcIn);

Cleanup:
   UserLeave();
   return Result;
}

