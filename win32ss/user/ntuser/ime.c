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
#define MOD_KEYS           (MOD_CONTROL | MOD_SHIFT | MOD_ALT | MOD_WIN)
#define MOD_LEFT_RIGHT     (MOD_LEFT | MOD_RIGHT)

#define LANGID_CHINESE_SIMPLIFIED   MAKELANGID(LANG_CHINESE,  SUBLANG_CHINESE_SIMPLIFIED)
#define LANGID_JAPANESE             MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT)
#define LANGID_KOREAN               MAKELANGID(LANG_KOREAN,   SUBLANG_KOREAN)
#define LANGID_CHINESE_TRADITIONAL  MAKELANGID(LANG_CHINESE,  SUBLANG_CHINESE_TRADITIONAL)
#define LANGID_NEUTRAL              MAKELANGID(LANG_NEUTRAL,  SUBLANG_NEUTRAL)

#define IS_WND_IMELIKE(pwnd) \
    (((pwnd)->pcls->style & CS_IME) || \
     ((pwnd)->pcls->atomClassName == gpsi->atomSysClass[ICLS_IME]))

// The special virtual keys for Japanese: Used for key states.
// https://www.kthree.co.jp/kihelp/index.html?page=app/vkey&type=html
#define VK_DBE_ALPHANUMERIC 0xF0
#define VK_DBE_KATAKANA 0xF1
#define VK_DBE_HIRAGANA 0xF2
#define VK_DBE_SBCSCHAR 0xF3
#define VK_DBE_DBCSCHAR 0xF4
#define VK_DBE_ROMAN 0xF5
#define VK_DBE_NOROMAN 0xF6
#define VK_DBE_ENTERWORDREGISTERMODE 0xF7
#define VK_DBE_ENTERCONFIGMODE 0xF8
#define VK_DBE_FLUSHSTRING 0xF9
#define VK_DBE_CODEINPUT 0xFA
#define VK_DBE_NOCODEINPUT 0xFB
#define VK_DBE_DETERINESTRING 0xFC
#define VK_DBE_ENTERDLGCONVERSIONMODE 0xFD

HIMC ghIMC = NULL;
BOOL gfImeOpen = (BOOL)-1;
DWORD gdwImeConversion = (DWORD)-1;

typedef struct tagIMEHOTKEY
{
    struct tagIMEHOTKEY *pNext;
    DWORD  dwHotKeyId;
    UINT   uVirtualKey;
    UINT   uModifiers;
    HKL    hKL;
} IMEHOTKEY, *PIMEHOTKEY;

PIMEHOTKEY gpImeHotKeyList = NULL;

static LANGID FASTCALL IntGetImeHotKeyLangId(DWORD dwHotKeyId)
{
#define IME_CHOTKEY 0x10
#define IME_JHOTKEY 0x30
#define IME_KHOTKEY 0x50
#define IME_THOTKEY 0x70
#define IME_XHOTKEY 0x90
    static const LANGID s_array[] =
    {
        /* 0x00 */ (WORD)-1,
        /* 0x10 */ LANGID_CHINESE_SIMPLIFIED,
        /* 0x20 */ LANGID_CHINESE_SIMPLIFIED,
        /* 0x30 */ LANGID_JAPANESE,
        /* 0x40 */ LANGID_JAPANESE,
        /* 0x50 */ LANGID_KOREAN,
        /* 0x60 */ LANGID_KOREAN,
        /* 0x70 */ LANGID_CHINESE_TRADITIONAL,
        /* 0x80 */ LANGID_CHINESE_TRADITIONAL
    };

    if (IME_CHOTKEY <= dwHotKeyId && dwHotKeyId < IME_XHOTKEY)
        return s_array[(dwHotKeyId & 0xF0) >> 4];
    return LANGID_NEUTRAL;
}

static VOID FASTCALL IntAddImeHotKey(PIMEHOTKEY *ppList, PIMEHOTKEY pHotKey)
{
    PIMEHOTKEY pNode;

    if (!*ppList)
    {
        *ppList = pHotKey;
        return;
    }

    for (pNode = *ppList; pNode; pNode = pNode->pNext)
    {
        if (!pNode->pNext)
        {
            pNode->pNext = pHotKey;
            return;
        }
    }
}

static PIMEHOTKEY FASTCALL IntGetImeHotKeyById(PIMEHOTKEY pList, DWORD dwHotKeyId)
{
    PIMEHOTKEY pNode;
    for (pNode = pList; pNode; pNode = pNode->pNext)
    {
        if (pNode->dwHotKeyId == dwHotKeyId)
            return pNode;
    }
    return NULL;
}

static PIMEHOTKEY APIENTRY
IntGetImeHotKeyByKeyAndLang(PIMEHOTKEY pList, UINT uModKeys, UINT uLeftRight,
                            UINT uVirtualKey, LANGID TargetLangId)
{
    PIMEHOTKEY pNode;
    LANGID LangID;
    UINT uModifiers;

    for (pNode = pList; pNode; pNode = pNode->pNext)
    {
        if (pNode->uVirtualKey != uVirtualKey)
            continue;

        LangID = IntGetImeHotKeyLangId(pNode->dwHotKeyId);
        if (LangID != TargetLangId)
            continue;

        uModifiers = pNode->uModifiers;
        if (uModifiers & MOD_IGNORE_ALL_MODIFIER)
            return pNode;

        if ((uModifiers & MOD_KEYS) != uModKeys)
            continue;

        if ((uModifiers & uLeftRight) || (uModifiers & MOD_LEFT_RIGHT) == uLeftRight)
            return pNode;
    }

    return NULL;
}

static VOID FASTCALL IntDeleteImeHotKey(PIMEHOTKEY *ppList, PIMEHOTKEY pHotKey)
{
    PIMEHOTKEY pNode;

    if (*ppList == pHotKey)
    {
        *ppList = pHotKey->pNext;
        ExFreePoolWithTag(pHotKey, USERTAG_IMEHOTKEY);
        return;
    }

    for (pNode = *ppList; pNode; pNode = pNode->pNext)
    {
        if (pNode->pNext == pHotKey)
        {
            pNode->pNext = pHotKey->pNext;
            ExFreePoolWithTag(pHotKey, USERTAG_IMEHOTKEY);
            return;
        }
    }
}

VOID FASTCALL IntFreeImeHotKeys(VOID)
{
    PIMEHOTKEY pNode, pNext;
    for (pNode = gpImeHotKeyList; pNode; pNode = pNext)
    {
        pNext = pNode->pNext;
        ExFreePoolWithTag(pNode, USERTAG_IMEHOTKEY);
    }
    gpImeHotKeyList = NULL;
}

static BOOL APIENTRY
IntSetImeHotKey(DWORD dwHotKeyId, UINT uModifiers, UINT uVirtualKey, HKL hKL, DWORD dwAction)
{
    PIMEHOTKEY pNode;
    LANGID LangId;

    switch (dwAction)
    {
        case SETIMEHOTKEY_DELETE:
            pNode = IntGetImeHotKeyById(gpImeHotKeyList, dwHotKeyId);
            if (!pNode)
                return FALSE;

            IntDeleteImeHotKey(&gpImeHotKeyList, pNode);
            return TRUE;

        case SETIMEHOTKEY_ADD:
            if (uVirtualKey == VK_PACKET)
                return FALSE;

            LangId = IntGetImeHotKeyLangId(dwHotKeyId);
            if (LangId == LANGID_KOREAN)
                return FALSE;

            pNode = IntGetImeHotKeyByKeyAndLang(gpImeHotKeyList,
                                                (uModifiers & MOD_KEYS),
                                                (uModifiers & MOD_LEFT_RIGHT),
                                                uVirtualKey, LangId);
            if (!pNode)
                pNode = IntGetImeHotKeyById(gpImeHotKeyList, dwHotKeyId);

            if (pNode)
            {
                pNode->uModifiers = uModifiers;
                pNode->uVirtualKey = uVirtualKey;
                pNode->hKL = hKL;
                return TRUE;
            }

            pNode = ExAllocatePoolWithTag(PagedPool, sizeof(IMEHOTKEY), USERTAG_IMEHOTKEY);
            if (!pNode)
                return FALSE;

            pNode->pNext = NULL;
            pNode->dwHotKeyId = dwHotKeyId;
            pNode->uModifiers = uModifiers;
            pNode->uVirtualKey = uVirtualKey;
            pNode->hKL = hKL;
            IntAddImeHotKey(&gpImeHotKeyList, pNode);
            return TRUE;

        case SETIMEHOTKEY_DELETEALL:
            IntFreeImeHotKeys();
            return TRUE;

        default:
            return FALSE;
    }
}

BOOL NTAPI
NtUserGetImeHotKey(DWORD dwHotKeyId, LPUINT lpuModifiers, LPUINT lpuVirtualKey, LPHKL lphKL)
{
    PIMEHOTKEY pNode = NULL;

    UserEnterExclusive();

    _SEH2_TRY
    {
        ProbeForWrite(lpuModifiers, sizeof(UINT), 1);
        ProbeForWrite(lpuVirtualKey, sizeof(UINT), 1);
        if (lphKL)
            ProbeForWrite(lphKL, sizeof(HKL), 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        goto Quit;
    }
    _SEH2_END;

    pNode = IntGetImeHotKeyById(gpImeHotKeyList, dwHotKeyId);
    if (!pNode)
        goto Quit;

    _SEH2_TRY
    {
        *lpuModifiers = pNode->uModifiers;
        *lpuVirtualKey = pNode->uVirtualKey;
        if (lphKL)
            *lphKL = pNode->hKL;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        pNode = NULL;
    }
    _SEH2_END;

Quit:
    UserLeave();
    return !!pNode;
}

BOOL
NTAPI
NtUserSetImeHotKey(
    DWORD  dwHotKeyId,
    UINT   uModifiers,
    UINT   uVirtualKey,
    HKL    hKL,
    DWORD  dwAction)
{
    BOOL ret;
    UserEnterExclusive();
    ret = IntSetImeHotKey(dwHotKeyId, uModifiers, uVirtualKey, hKL, dwAction);
    UserLeave();
    return ret;
}

PWND FASTCALL IntGetTopLevelWindow(PWND pwnd)
{
    if (!pwnd)
        return NULL;

    while (pwnd->style & WS_CHILD)
        pwnd = pwnd->spwndParent;

    return pwnd;
}

HIMC FASTCALL IntAssociateInputContext(PWND pWnd, PIMC pImc)
{
    HIMC hOldImc = pWnd->hImc;
    pWnd->hImc = (pImc ? UserHMGetHandle(pImc) : NULL);
    return hOldImc;
}

DWORD
NTAPI
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
NTAPI
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

static VOID FASTCALL UserSetImeConversionKeyState(PTHREADINFO pti, DWORD dwConversion)
{
    HKL hKL;
    LANGID LangID;
    LPBYTE KeyState;
    BOOL bAlphaNumeric, bKatakana, bHiragana, bFullShape, bRoman, bCharCode;

    if (!pti->KeyboardLayout)
        return;

    hKL = pti->KeyboardLayout->hkl;
    LangID = LOWORD(hKL);
    KeyState = pti->MessageQueue->afKeyState;

    switch (PRIMARYLANGID(LangID))
    {
        case LANG_JAPANESE:
            bAlphaNumeric = !(dwConversion & IME_CMODE_NATIVE);
            bKatakana = !bAlphaNumeric && (dwConversion & IME_CMODE_KATAKANA);
            bHiragana = !bAlphaNumeric && !(dwConversion & IME_CMODE_KATAKANA);
            SET_KEY_DOWN(KeyState, VK_DBE_ALPHANUMERIC, bAlphaNumeric);
            SET_KEY_LOCKED(KeyState, VK_DBE_ALPHANUMERIC, bAlphaNumeric);
            SET_KEY_DOWN(KeyState, VK_DBE_HIRAGANA, bHiragana);
            SET_KEY_LOCKED(KeyState, VK_DBE_HIRAGANA, bHiragana);
            SET_KEY_DOWN(KeyState, VK_DBE_KATAKANA, bKatakana);
            SET_KEY_LOCKED(KeyState, VK_DBE_KATAKANA, bKatakana);

            bFullShape = (dwConversion & IME_CMODE_FULLSHAPE);
            SET_KEY_DOWN(KeyState, VK_DBE_DBCSCHAR, bFullShape);
            SET_KEY_LOCKED(KeyState, VK_DBE_DBCSCHAR, bFullShape);
            SET_KEY_DOWN(KeyState, VK_DBE_SBCSCHAR, !bFullShape);
            SET_KEY_LOCKED(KeyState, VK_DBE_SBCSCHAR, !bFullShape);

            bRoman = (dwConversion & IME_CMODE_ROMAN);
            SET_KEY_DOWN(KeyState, VK_DBE_ROMAN, bRoman);
            SET_KEY_LOCKED(KeyState, VK_DBE_ROMAN, bRoman);
            SET_KEY_DOWN(KeyState, VK_DBE_NOROMAN, !bRoman);
            SET_KEY_LOCKED(KeyState, VK_DBE_NOROMAN, !bRoman);

            bCharCode = (dwConversion & IME_CMODE_CHARCODE);
            SET_KEY_DOWN(KeyState, VK_DBE_CODEINPUT, bCharCode);
            SET_KEY_LOCKED(KeyState, VK_DBE_CODEINPUT, bCharCode);
            SET_KEY_DOWN(KeyState, VK_DBE_NOCODEINPUT, !bCharCode);
            SET_KEY_LOCKED(KeyState, VK_DBE_NOCODEINPUT, !bCharCode);
            break;

        case LANG_KOREAN:
            SET_KEY_LOCKED(KeyState, VK_HANGUL, (dwConversion & IME_CMODE_NATIVE));
            SET_KEY_LOCKED(KeyState, VK_JUNJA, (dwConversion & IME_CMODE_FULLSHAPE));
            SET_KEY_LOCKED(KeyState, VK_HANJA, (dwConversion & IME_CMODE_HANJACONVERT));
            break;

        default:
            break;
    }
}

DWORD
NTAPI
NtUserNotifyIMEStatus(HWND hwnd, BOOL fOpen, DWORD dwConversion)
{
    PWND pwnd;
    PTHREADINFO pti;
    HKL hKL;

    UserEnterExclusive();

    pwnd = ValidateHwndNoErr(hwnd);
    if (!pwnd || !IS_IMM_MODE())
        goto Quit;

    pti = pwnd->head.pti;
    if (!pti || !gptiForeground)
        goto Quit;
    if (pti != gptiForeground && pti->MessageQueue != gptiForeground->MessageQueue)
        goto Quit;
    if (ghIMC == pwnd->hImc && gfImeOpen == !!fOpen && gdwImeConversion == dwConversion)
        goto Quit;

    ghIMC = pwnd->hImc;
    if (ghIMC)
    {
        gfImeOpen = !!fOpen;
        gdwImeConversion = dwConversion;
        UserSetImeConversionKeyState(pti, (fOpen ? dwConversion : IME_CMODE_ALPHANUMERIC));
    }

    if (ISITHOOKED(WH_SHELL))
    {
        hKL = (pti->KeyboardLayout ? pti->KeyboardLayout->hkl : NULL);
        co_HOOK_CallHooks(WH_SHELL, HSHELL_LANGUAGE, (WPARAM)hwnd, (LPARAM)hKL);
    }

    // TODO:

Quit:
    UserLeave();
    return 0;
}

DWORD
NTAPI
NtUserCheckImeHotKey(
    DWORD  VirtualKey,
    LPARAM lParam)
{
    STUB;
    return 0;
}

BOOL
NTAPI
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
NTAPI
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
NTAPI
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
    if (!ret)
        goto Quit;

    _SEH2_TRY
    {
        *pImeInfoEx = ImeInfoEx;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = FALSE;
    }
    _SEH2_END;

Quit:
    UserLeave();
    return ret;
}

BOOL
NTAPI
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
NTAPI
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

BOOL NTAPI
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

BOOL NTAPI NtUserDestroyInputContext(HIMC hIMC)
{
    PIMC pIMC;
    BOOL ret = FALSE;
    HWND *phwnd;
    PWND pWnd;
    PWINDOWLIST pwl;
    PTHREADINFO pti;

    UserEnterExclusive();

    if (!IS_IMM_MODE())
    {
        EngSetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        goto Quit;
    }

    pIMC = UserGetObjectNoErr(gHandleTable, hIMC, TYPE_INPUTCONTEXT);
    if (!pIMC)
        goto Quit;

    pti = pIMC->head.pti;
    if (pti != GetW32ThreadInfo() || pIMC == pti->spDefaultImc)
        goto Quit;

    UserMarkObjectDestroy(pIMC);

    pwl = IntBuildHwndList(pti->rpdesk->pDeskInfo->spwnd->spwndChild,
                           IACE_CHILDREN | IACE_LIST, pti);
    if (pwl)
    {
        for (phwnd = pwl->ahwnd; *phwnd != HWND_TERMINATOR; ++phwnd)
        {
            pWnd = ValidateHwndNoErr(*phwnd);
            if (!pWnd)
                continue;

            if (pWnd->hImc == hIMC)
                IntAssociateInputContext(pWnd, pti->spDefaultImc);
        }

        IntFreeHwndList(pwl);
    }

    ret = UserDeleteObject(hIMC, TYPE_INPUTCONTEXT);

Quit:
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
NTAPI
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
NTAPI
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
NTAPI
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
NTAPI
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
