/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Input Method Editor and Input Method Manager support
 * FILE:             win32ss/user/ntuser/ime.c
 * PROGRAMERS:       Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                   Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserMisc);

#define INVALID_THREAD_ID  ((ULONG)-1)

UINT FASTCALL
IntImmProcessKey(PUSER_MESSAGE_QUEUE MessageQueue, PWND pWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    PKL pKbdLayout;

    ASSERT_REFS_CO(pWnd);

    if ( Msg == WM_KEYDOWN ||
         Msg == WM_SYSKEYDOWN ||
         Msg == WM_KEYUP ||
         Msg == WM_SYSKEYUP )
    {
       //Vk = wParam & 0xff;
       pKbdLayout = pWnd->head.pti->KeyboardLayout;
       if (pKbdLayout == NULL) return 0;
       //
       if (!(gpsi->dwSRVIFlags & SRVINFO_IMM32)) return 0;
       // need ime.h!
    }
    // Call User32:
    // Anything but BOOL!
    //ImmRet = co_IntImmProcessKey(UserHMGetHandle(pWnd), pKbdLayout->hkl, Vk, lParam, HotKey);
    FIXME(" is UNIMPLEMENTED.\n");
    return 0;
}

BOOL WINAPI
NtUserGetImeHotKey(IN DWORD dwHotKey,
                   OUT LPUINT lpuModifiers,
                   OUT LPUINT lpuVKey,
                   OUT LPHKL lphKL)
{
   STUB

   return FALSE;
}

DWORD
APIENTRY
NtUserNotifyIMEStatus(HWND hwnd, BOOL fOpen, DWORD dwConversion)
{
    TRACE("NtUserNotifyIMEStatus(%p, %d, 0x%lX)\n", hwnd, fOpen, dwConversion);
    return 0;
}

DWORD
APIENTRY
NtUserSetImeHotKey(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2,
   DWORD Unknown3,
   DWORD Unknown4)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserCheckImeHotKey(
    DWORD  VirtualKey,
    LPARAM lParam)
{
    STUB;
    return 0;
}

BOOL
APIENTRY
NtUserDisableThreadIme(
    DWORD dwThreadID)
{
    PTHREADINFO pti, ptiCurrent;
    PPROCESSINFO ppi;
    BOOL ret = FALSE;

    UserEnterExclusive();

    if (!IS_IMM_MODE())
    {
        EngSetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        goto Quit;
    }

    ptiCurrent = GetW32ThreadInfo();

    if (dwThreadID == INVALID_THREAD_ID)
    {
        ppi = ptiCurrent->ppi;
        ppi->W32PF_flags |= W32PF_DISABLEIME;

Retry:
        for (pti = ppi->ptiList; pti; pti = pti->ptiSibling)
        {
            pti->TIF_flags |= TIF_DISABLEIME;

            if (pti->spwndDefaultIme)
            {
                co_UserDestroyWindow(pti->spwndDefaultIme);
                pti->spwndDefaultIme = NULL;
                goto Retry; /* The contents of ppi->ptiList may be changed. */
            }
        }
    }
    else
    {
        if (dwThreadID == 0)
        {
            pti = ptiCurrent;
        }
        else
        {
            pti = IntTID2PTI(UlongToHandle(dwThreadID));

            /* The thread needs to reside in the current process. */
            if (!pti || pti->ppi != ptiCurrent->ppi)
                goto Quit;
        }

        pti->TIF_flags |= TIF_DISABLEIME;

        if (pti->spwndDefaultIme)
        {
            co_UserDestroyWindow(pti->spwndDefaultIme);
            pti->spwndDefaultIme = NULL;
        }
    }

    ret = TRUE;

Quit:
    UserLeave();
    return ret;
}

DWORD
APIENTRY
NtUserGetAppImeLevel(HWND hWnd)
{
    STUB;
    return 0;
}

BOOL
APIENTRY
NtUserGetImeInfoEx(
    PIMEINFOEX pImeInfoEx,
    IMEINFOEXCLASS SearchType)
{
    STUB;
    return FALSE;
}


DWORD
APIENTRY
NtUserSetAppImeLevel(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserSetImeInfoEx(
    PIMEINFOEX pImeInfoEx)
{
    STUB;
    return 0;
}

DWORD APIENTRY
NtUserSetImeOwnerWindow(PIMEINFOEX pImeInfoEx, BOOL fFlag)
{
   STUB
   return 0;
}

PVOID
AllocInputContextObject(PDESKTOP pDesk,
                        PTHREADINFO pti,
                        SIZE_T Size,
                        PVOID* HandleOwner)
{
    PTHRDESKHEAD ObjHead;

    ASSERT(Size > sizeof(*ObjHead));
    ASSERT(pti != NULL);

    ObjHead = UserHeapAlloc(Size);
    if (!ObjHead)
        return NULL;

    RtlZeroMemory(ObjHead, Size);

    ObjHead->pSelf = ObjHead;
    ObjHead->rpdesk = pDesk;
    ObjHead->pti = pti;
    IntReferenceThreadInfo(pti);
    *HandleOwner = pti;
    pti->ppi->UserHandleCount++;

    return ObjHead;
}

VOID UserFreeInputContext(PVOID Object)
{
    PIMC pIMC = Object, pImc0;
    PTHREADINFO pti;

    if (!pIMC)
        return;

    pti = pIMC->head.pti;

    /* Find the IMC in the list and remove it */
    for (pImc0 = pti->spDefaultImc; pImc0; pImc0 = pImc0->pImcNext)
    {
        if (pImc0->pImcNext == pIMC)
        {
            pImc0->pImcNext = pIMC->pImcNext;
            break;
        }
    }

    UserHeapFree(pIMC);

    pti->ppi->UserHandleCount--;
    IntDereferenceThreadInfo(pti);
}

BOOLEAN UserDestroyInputContext(PVOID Object)
{
    PIMC pIMC = Object;
    if (pIMC)
    {
        UserMarkObjectDestroy(pIMC);
        UserDeleteObject(pIMC->head.h, TYPE_INPUTCONTEXT);
    }
    return TRUE;
}

BOOL APIENTRY NtUserDestroyInputContext(HIMC hIMC)
{
    PIMC pIMC;
    BOOL ret = FALSE;

    UserEnterExclusive();

    if (!(gpsi->dwSRVIFlags & SRVINFO_IMM32))
    {
        EngSetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        UserLeave();
        return FALSE;
    }

    pIMC = UserGetObject(gHandleTable, hIMC, TYPE_INPUTCONTEXT);
    if (pIMC)
        ret = UserDereferenceObject(pIMC);

    UserLeave();
    return ret;
}

/* EOF */
