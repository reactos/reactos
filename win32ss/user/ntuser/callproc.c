/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Callproc support
 * FILE:             subsystems/win32/win32k/ntuser/callproc.c
 * PROGRAMER:        Thomas Weidenmueller <w3seek@reactos.com>
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserClass);

/* CALLPROC ******************************************************************/

WNDPROC
GetCallProcHandle(IN PCALLPROCDATA CallProc)
{
    /* FIXME: Check for 64 bit architectures... */
    return (WNDPROC)((ULONG_PTR)UserHMGetHandle(CallProc) | 0xFFFF0000);
}

BOOLEAN
DestroyCallProc(_Inout_ PVOID Object)
{
    UserDeleteObject(UserHMGetHandle((PCALLPROCDATA)Object), TYPE_CALLPROC);
    return TRUE;
}

PCALLPROCDATA
CreateCallProc(IN PDESKTOP Desktop,
               IN WNDPROC WndProc,
               IN BOOL Unicode,
               IN PPROCESSINFO pi)
{
    PCALLPROCDATA NewCallProc;
    HANDLE Handle;

    /* We can send any thread pointer to the object manager here,
     * What's important is the process info */
    NewCallProc = (PCALLPROCDATA)UserCreateObject(gHandleTable,
                                             Desktop,
                                             pi->ptiList,
                                             &Handle,
                                             TYPE_CALLPROC,
                                             sizeof(CALLPROCDATA));
    if (NewCallProc != NULL)
    {
        NewCallProc->pfnClientPrevious = WndProc;
        NewCallProc->wType |= Unicode ? UserGetCPDA2U : UserGetCPDU2A ;
        NewCallProc->spcpdNext = NULL;
    }

    /* Release the extra reference (UserCreateObject added 2 references) */
    UserDereferenceObject(NewCallProc);

    return NewCallProc;
}

BOOL
UserGetCallProcInfo(IN HANDLE hCallProc,
                    OUT PWNDPROC_INFO wpInfo)
{
    PCALLPROCDATA CallProc;

    CallProc = UserGetObject(gHandleTable,
                             hCallProc,
                             TYPE_CALLPROC);
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
   PDESKTOP pDesk;
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
      if (!pCls->rpdeskParent)
      {
         TRACE("Null DESKTOP Atom %d\n",pCls->atomClassName);
         pDesk = pti->rpdesk;
      }
      else
         pDesk = pCls->rpdeskParent;
      CallProc = CreateCallProc( pDesk,
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
*/
ULONG_PTR
APIENTRY
NtUserGetCPD(
   HWND hWnd,
   GETCPD Flags,
   ULONG_PTR ProcIn)
{
   PWND Wnd;
   ULONG_PTR Result = 0;

   UserEnterExclusive();
   if (!(Wnd = UserGetWindowObject(hWnd)))
   {   
      goto Cleanup;
   }

   // Processing Window only from User space.
   if ((Flags & ~(UserGetCPDU2A|UserGetCPDA2U)) != UserGetCPDClass)        
      Result = UserGetCPD(Wnd, Flags, ProcIn);

Cleanup:
   UserLeave();
   return Result;
}

