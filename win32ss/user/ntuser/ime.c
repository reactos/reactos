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

#define IS_WND_IMELIKE(pwnd) \
    (((pwnd)->pcls->style & CS_IME) || \
     ((pwnd)->pcls->atomClassName == gpsi->atomSysClass[ICLS_IME]))

PWND FASTCALL IntGetTopLevelWindow(PWND pwnd)
{
    if (!pwnd)
        return NULL;

    while (pwnd->style & WS_CHILD)
        pwnd = pwnd->spwndParent;

    return pwnd;
}

DWORD
APIENTRY
NtUserSetThreadLayoutHandles(HKL hNewKL, HKL hOldKL)
{
    PTHREADINFO pti;
    PKL pOldKL, pNewKL;

    UserEnterExclusive();

    pti = GetW32ThreadInfo();
    pOldKL = pti->KeyboardLayout;
    if (pOldKL && pOldKL->hkl != hOldKL)
        goto Quit;

    pNewKL = UserHklToKbl(hNewKL);
    if (!pNewKL)
        goto Quit;

    if (IS_IME_HKL(hNewKL) != IS_IME_HKL(hOldKL))
        pti->hklPrev = hOldKL;

    pti->KeyboardLayout = pNewKL;

Quit:
    UserLeave();
    return 0;
}

DWORD FASTCALL UserBuildHimcList(PTHREADINFO pti, DWORD dwCount, HIMC *phList)
{
    PIMC pIMC;
    DWORD dwRealCount = 0;

    if (pti)
    {
        for (pIMC = pti->spDefaultImc; pIMC; pIMC = pIMC->pImcNext)
        {
            if (dwRealCount < dwCount)
                phList[dwRealCount] = UserHMGetHandle(pIMC);

            ++dwRealCount;
        }
    }
    else
    {
        for (pti = GetW32ThreadInfo()->ppi->ptiList; pti; pti = pti->ptiSibling)
        {
            for (pIMC = pti->spDefaultImc; pIMC; pIMC = pIMC->pImcNext)
            {
                if (dwRealCount < dwCount)
                    phList[dwRealCount] = UserHMGetHandle(pIMC);

                ++dwRealCount;
            }
        }
    }

    return dwRealCount;
}

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

NTSTATUS
APIENTRY
NtUserBuildHimcList(DWORD dwThreadId, DWORD dwCount, HIMC *phList, LPDWORD pdwCount)
{
    NTSTATUS ret = STATUS_UNSUCCESSFUL;
    DWORD dwRealCount;
    PTHREADINFO pti;

    UserEnterExclusive();

    if (!IS_IMM_MODE())
    {
        EngSetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        goto Quit;
    }

    if (dwThreadId == 0)
    {
        pti = GetW32ThreadInfo();
    }
    else if (dwThreadId == INVALID_THREAD_ID)
    {
        pti = NULL;
    }
    else
    {
        pti = IntTID2PTI(UlongToHandle(dwThreadId));
        if (!pti || !pti->rpdesk)
            goto Quit;
    }

    _SEH2_TRY
    {
        ProbeForWrite(phList, dwCount * sizeof(HIMC), 1);
        ProbeForWrite(pdwCount, sizeof(DWORD), 1);
        *pdwCount = dwRealCount = UserBuildHimcList(pti, dwCount, phList);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        goto Quit;
    }
    _SEH2_END;

    if (dwCount < dwRealCount)
        ret = STATUS_BUFFER_TOO_SMALL;
    else
        ret = STATUS_SUCCESS;

Quit:
    UserLeave();
    return ret;
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
    DWORD ret = 0;
    PWND pWnd;
    PTHREADINFO pti;

    UserEnterShared();

    pWnd = ValidateHwndNoErr(hWnd);
    if (!pWnd)
        goto Quit;

    if (!IS_IMM_MODE())
    {
        EngSetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        goto Quit;
    }

    pti = PsGetCurrentThreadWin32Thread();
    if (pWnd->head.pti->ppi == pti->ppi)
        ret = (DWORD)(ULONG_PTR)UserGetProp(pWnd, AtomImeLevel, TRUE);

Quit:
    UserLeave();
    return ret;
}

BOOL FASTCALL UserGetImeInfoEx(LPVOID pUnknown, PIMEINFOEX pInfoEx, IMEINFOEXCLASS SearchType)
{
    PKL pkl, pklHead;

    if (!gspklBaseLayout)
        return FALSE;

    pkl = pklHead = gspklBaseLayout;

    /* Find the matching entry from the list and get info */
    if (SearchType == ImeInfoExKeyboardLayout)
    {
        do
        {
            if (pInfoEx->hkl == pkl->hkl)
            {
                if (!pkl->piiex)
                    break;

                *pInfoEx = *pkl->piiex;
                return TRUE;
            }

            pkl = pkl->pklNext;
        } while (pkl != pklHead);
    }
    else if (SearchType == ImeInfoExImeFileName)
    {
        do
        {
            if (pkl->piiex &&
                _wcsnicmp(pkl->piiex->wszImeFile, pInfoEx->wszImeFile,
                          RTL_NUMBER_OF(pkl->piiex->wszImeFile)) == 0)
            {
                *pInfoEx = *pkl->piiex;
                return TRUE;
            }

            pkl = pkl->pklNext;
        } while (pkl != pklHead);
    }
    else
    {
        /* Do nothing */
    }

    return FALSE;
}

BOOL
APIENTRY
NtUserGetImeInfoEx(
    PIMEINFOEX pImeInfoEx,
    IMEINFOEXCLASS SearchType)
{
    IMEINFOEX ImeInfoEx;
    BOOL ret = FALSE;

    UserEnterShared();

    if (!IS_IMM_MODE())
        goto Quit;

    _SEH2_TRY
    {
        ProbeForWrite(pImeInfoEx, sizeof(*pImeInfoEx), 1);
        ImeInfoEx = *pImeInfoEx;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        goto Quit;
    }
    _SEH2_END;

    ret = UserGetImeInfoEx(NULL, &ImeInfoEx, SearchType);
    if (ret)
    {
        _SEH2_TRY
        {
            *pImeInfoEx = ImeInfoEx;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            ret = FALSE;
        }
        _SEH2_END;
    }

Quit:
    UserLeave();
    return ret;
}

BOOL
APIENTRY
NtUserSetAppImeLevel(HWND hWnd, DWORD dwLevel)
{
    BOOL ret = FALSE;
    PWND pWnd;
    PTHREADINFO pti;

    UserEnterExclusive();

    pWnd = ValidateHwndNoErr(hWnd);
    if (!pWnd)
        goto Quit;

    if (!IS_IMM_MODE())
    {
        EngSetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        goto Quit;
    }

    pti = PsGetCurrentThreadWin32Thread();
    if (pWnd->head.pti->ppi == pti->ppi)
        ret = UserSetProp(pWnd, AtomImeLevel, (HANDLE)(ULONG_PTR)dwLevel, TRUE);

Quit:
    UserLeave();
    return ret;
}

BOOL FASTCALL UserSetImeInfoEx(LPVOID pUnknown, PIMEINFOEX pImeInfoEx)
{
    PKL pklHead, pkl;

    pkl = pklHead = gspklBaseLayout;

    do
    {
        if (pkl->hkl != pImeInfoEx->hkl)
        {
            pkl = pkl->pklNext;
            continue;
        }

        if (!pkl->piiex)
            return FALSE;

        if (!pkl->piiex->fLoadFlag)
            *pkl->piiex = *pImeInfoEx;

        return TRUE;
    } while (pkl != pklHead);

    return FALSE;
}

BOOL
APIENTRY
NtUserSetImeInfoEx(PIMEINFOEX pImeInfoEx)
{
    BOOL ret = FALSE;
    IMEINFOEX ImeInfoEx;

    UserEnterExclusive();

    if (!IS_IMM_MODE())
        goto Quit;

    _SEH2_TRY
    {
        ProbeForRead(pImeInfoEx, sizeof(*pImeInfoEx), 1);
        ImeInfoEx = *pImeInfoEx;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        goto Quit;
    }
    _SEH2_END;

    ret = UserSetImeInfoEx(NULL, &ImeInfoEx);

Quit:
    UserLeave();
    return ret;
}

BOOL APIENTRY
NtUserSetImeOwnerWindow(HWND hImeWnd, HWND hwndFocus)
{
    BOOL ret = FALSE;
    PWND pImeWnd, pwndFocus, pwndTopLevel, pwnd, pwndActive;
    PTHREADINFO ptiIme;

    UserEnterExclusive();

    pImeWnd = ValidateHwndNoErr(hImeWnd);
    if (!IS_IMM_MODE() || !pImeWnd || pImeWnd->fnid != FNID_IME)
        goto Quit;

    pwndFocus = ValidateHwndNoErr(hwndFocus);
    if (pwndFocus)
    {
        if (IS_WND_IMELIKE(pwndFocus))
            goto Quit;

        pwndTopLevel = IntGetTopLevelWindow(pwndFocus);

        for (pwnd = pwndTopLevel; pwnd; pwnd = pwnd->spwndOwner)
        {
            if (pwnd->pcls->atomClassName == gpsi->atomSysClass[ICLS_IME])
            {
                pwndTopLevel = NULL;
                break;
            }
        }

        pImeWnd->spwndOwner = pwndTopLevel;
        // TODO:
    }
    else
    {
        ptiIme = pImeWnd->head.pti;
        pwndActive = ptiIme->MessageQueue->spwndActive;

        if (!pwndActive || pwndActive != pImeWnd->spwndOwner)
        {
            if (pwndActive && ptiIme == pwndActive->head.pti && !IS_WND_IMELIKE(pwndActive))
            {
                pImeWnd->spwndOwner = pwndActive;
            }
            else
            {
                // TODO:
            }

            // TODO:
        }
    }

    ret = TRUE;

Quit:
    UserLeave();
    return ret;
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

PIMC FASTCALL UserCreateInputContext(ULONG_PTR dwClientImcData)
{
    PIMC pIMC;
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
    PDESKTOP pdesk = pti->rpdesk;

    if (!IS_IMM_MODE() || (pti->TIF_flags & TIF_DISABLEIME)) // Disabled?
        return NULL;

    if (!pdesk) // No desktop?
        return NULL;

    // pti->spDefaultImc should be already set if non-first time.
    if (dwClientImcData && !pti->spDefaultImc)
        return NULL;

    // Create an input context user object.
    pIMC = UserCreateObject(gHandleTable, pdesk, pti, NULL, TYPE_INPUTCONTEXT, sizeof(IMC));
    if (!pIMC)
        return NULL;

    // Release the extra reference (UserCreateObject added 2 references).
    UserDereferenceObject(pIMC);

    if (dwClientImcData) // Non-first time.
    {
        // Insert pIMC to the second position (non-default) of the list.
        pIMC->pImcNext = pti->spDefaultImc->pImcNext;
        pti->spDefaultImc->pImcNext = pIMC;
    }
    else // First time. It's the default IMC.
    {
        // Add the first one (default) to the list.
        pti->spDefaultImc = pIMC;
        pIMC->pImcNext = NULL;
    }

    pIMC->dwClientImcData = dwClientImcData; // Set it.
    return pIMC;
}

HIMC
APIENTRY
NtUserCreateInputContext(ULONG_PTR dwClientImcData)
{
    PIMC pIMC;
    HIMC ret = NULL;

    if (!dwClientImcData)
        return NULL;

    UserEnterExclusive();

    if (!IS_IMM_MODE())
        goto Quit;

    pIMC = UserCreateInputContext(dwClientImcData);
    if (pIMC)
        ret = UserHMGetHandle(pIMC);

Quit:
    UserLeave();
    return ret;
}

HIMC FASTCALL IntAssociateInputContext(PWND pWnd, PIMC pImc)
{
    HIMC hOldImc = pWnd->hImc;
    pWnd->hImc = (pImc ? UserHMGetHandle(pImc) : NULL);
    return hOldImc;
}

DWORD FASTCALL IntAssociateInputContextEx(PWND pWnd, PIMC pIMC, DWORD dwFlags)
{
    DWORD ret = 0;
    PWINDOWLIST pwl;
    BOOL bIgnoreNullImc = (dwFlags & IACE_IGNORENOCONTEXT);
    PTHREADINFO pti = pWnd->head.pti;
    PWND pwndTarget, pwndFocus = pti->MessageQueue->spwndFocus;
    HWND *phwnd;
    HIMC hIMC;

    if (dwFlags & IACE_DEFAULT)
    {
        pIMC = pti->spDefaultImc;
    }
    else
    {
        if (pIMC && pti != pIMC->head.pti)
            return 2;
    }

    if (pWnd->head.pti->ppi != GetW32ThreadInfo()->ppi ||
        (pIMC && pIMC->head.rpdesk != pWnd->head.rpdesk))
    {
        return 2;
    }

    if ((dwFlags & IACE_CHILDREN) && pWnd->spwndChild)
    {
        pwl = IntBuildHwndList(pWnd->spwndChild, IACE_CHILDREN | IACE_LIST, pti);
        if (pwl)
        {
            for (phwnd = pwl->ahwnd; *phwnd != HWND_TERMINATOR; ++phwnd)
            {
                pwndTarget = ValidateHwndNoErr(*phwnd);
                if (!pwndTarget)
                    continue;

                hIMC = (pIMC ? UserHMGetHandle(pIMC) : NULL);
                if (pwndTarget->hImc == hIMC || (bIgnoreNullImc && !pwndTarget->hImc))
                    continue;

                IntAssociateInputContext(pwndTarget, pIMC);
                if (pwndTarget == pwndFocus)
                    ret = 1;
            }

            IntFreeHwndList(pwl);
        }
    }

    if (!bIgnoreNullImc || pWnd->hImc)
    {
        hIMC = (pIMC ? UserHMGetHandle(pIMC) : NULL);
        if (pWnd->hImc != hIMC)
        {
            IntAssociateInputContext(pWnd, pIMC);
            if (pWnd == pwndFocus)
                ret = 1;
        }
    }

    return ret;
}

DWORD
APIENTRY
NtUserAssociateInputContext(HWND hWnd, HIMC hIMC, DWORD dwFlags)
{
    DWORD ret = 2;
    PWND pWnd;
    PIMC pIMC;

    UserEnterExclusive();

    pWnd = ValidateHwndNoErr(hWnd);
    if (!pWnd || !IS_IMM_MODE())
        goto Quit;

    pIMC = (hIMC ? UserGetObjectNoErr(gHandleTable, hIMC, TYPE_INPUTCONTEXT) : NULL);
    ret = IntAssociateInputContextEx(pWnd, pIMC, dwFlags);

Quit:
    UserLeave();
    return ret;
}

BOOL FASTCALL UserUpdateInputContext(PIMC pIMC, DWORD dwType, DWORD_PTR dwValue)
{
    PTHREADINFO pti = GetW32ThreadInfo();
    PTHREADINFO ptiIMC = pIMC->head.pti;

    if (pti->ppi != ptiIMC->ppi) // Different process?
        return FALSE;

    switch (dwType)
    {
        case UIC_CLIENTIMCDATA:
            if (pIMC->dwClientImcData)
                return FALSE; // Already set

            pIMC->dwClientImcData = dwValue;
            break;

        case UIC_IMEWINDOW:
            if (!ValidateHwndNoErr((HWND)dwValue))
                return FALSE; // Invalid HWND

            pIMC->hImeWnd = (HWND)dwValue;
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

BOOL
APIENTRY
NtUserUpdateInputContext(
    HIMC hIMC,
    DWORD dwType,
    DWORD_PTR dwValue)
{
    PIMC pIMC;
    BOOL ret = FALSE;

    UserEnterExclusive();

    if (!IS_IMM_MODE())
        goto Quit;

    pIMC = UserGetObject(gHandleTable, hIMC, TYPE_INPUTCONTEXT);
    if (!pIMC)
        goto Quit;

    ret = UserUpdateInputContext(pIMC, dwType, dwValue);

Quit:
    UserLeave();
    return ret;
}

DWORD_PTR
APIENTRY
NtUserQueryInputContext(HIMC hIMC, DWORD dwType)
{
    PIMC pIMC;
    PTHREADINFO ptiIMC;
    DWORD_PTR ret = 0;

    UserEnterExclusive();

    if (!IS_IMM_MODE())
        goto Quit;

    pIMC = UserGetObject(gHandleTable, hIMC, TYPE_INPUTCONTEXT);
    if (!pIMC)
        goto Quit;

    ptiIMC = pIMC->head.pti;

    switch (dwType)
    {
        case QIC_INPUTPROCESSID:
            ret = (DWORD_PTR)PsGetThreadProcessId(ptiIMC->pEThread);
            break;

        case QIC_INPUTTHREADID:
            ret = (DWORD_PTR)PsGetThreadId(ptiIMC->pEThread);
            break;

        case QIC_DEFAULTWINDOWIME:
            if (ptiIMC->spwndDefaultIme)
                ret = (DWORD_PTR)UserHMGetHandle(ptiIMC->spwndDefaultIme);
            break;

        case QIC_DEFAULTIMC:
            if (ptiIMC->spDefaultImc)
                ret = (DWORD_PTR)UserHMGetHandle(ptiIMC->spDefaultImc);
            break;
    }

Quit:
    UserLeave();
    return ret;
}

/* EOF */
