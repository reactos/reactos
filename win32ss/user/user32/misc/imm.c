/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            win32ss/user/user32/misc/imm.c
 * PURPOSE:         User32.dll Imm functions
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 *                  Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <user32.h>
#include <strsafe.h>
#include <immdev.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

#define IMM_INIT_MAGIC 0x19650412
#define MAX_CANDIDATEFORM 4

/* Is != NULL when we have loaded the IMM ourselves */
HINSTANCE ghImm32 = NULL; // Win: ghImm32

BOOL gbImmInitializing = FALSE; // Win: bImmInitializing

INT gfConIme = -1; // Win: gfConIme

HWND FASTCALL IntGetTopLevelWindow(HWND hWnd)
{
    DWORD style;

    for (; hWnd; hWnd = GetParent(hWnd))
    {
        style = (DWORD)GetWindowLongPtrW(hWnd, GWL_STYLE);
        if (!(style & WS_CHILD))
            break;
    }

    return hWnd;
}

/* define stub functions */
#undef DEFINE_IMM_ENTRY
#define DEFINE_IMM_ENTRY(type, name, params, retval, retkind) \
    static type WINAPI IMMSTUB_##name params { IMM_RETURN_##retkind((type)retval); }
#include "immtable.h"

// Win: gImmApiEntries
Imm32ApiTable gImmApiEntries = {
/* initialize by stubs */
#undef DEFINE_IMM_ENTRY
#define DEFINE_IMM_ENTRY(type, name, params, retval, retkind) \
    IMMSTUB_##name,
#include "immtable.h"
};

// Win: GetImmFileName
HRESULT
User32GetImmFileName(_Out_ LPWSTR lpBuffer, _In_ size_t cchBuffer)
{
    UINT length = GetSystemDirectoryW(lpBuffer, cchBuffer);
    if (length && length < cchBuffer)
    {
        StringCchCatW(lpBuffer, cchBuffer, L"\\");
        return StringCchCatW(lpBuffer, cchBuffer, L"imm32.dll");
    }
    return StringCchCopyW(lpBuffer, cchBuffer, L"imm32.dll");
}

// @unimplemented
// Win: _InitializeImmEntryTable
static BOOL IntInitializeImmEntryTable(VOID)
{
    WCHAR ImmFile[MAX_PATH];
    HMODULE imm32 = ghImm32;

    /* Check whether the IMM table has already been initialized */
    if (IMM_FN(ImmWINNLSEnableIME) != IMMSTUB_ImmWINNLSEnableIME)
        return TRUE;

    User32GetImmFileName(ImmFile, _countof(ImmFile));
    TRACE("File %S\n", ImmFile);

    /* If IMM32 is already loaded, use it without increasing reference count. */
    if (imm32 == NULL)
        imm32 = GetModuleHandleW(ImmFile);

    /*
     * Loading imm32.dll will call imm32!DllMain function.
     * imm32!DllMain calls User32InitializeImmEntryTable.
     * Thus, if imm32.dll was loaded, the table has been loaded.
     */
    if (imm32 == NULL)
    {
        imm32 = ghImm32 = LoadLibraryW(ImmFile);
        if (imm32 == NULL)
        {
            ERR("Did not load imm32.dll!\n");
            return FALSE;
        }
        return TRUE;
    }

/* load imm procedures */
#undef DEFINE_IMM_ENTRY
#define DEFINE_IMM_ENTRY(type, name, params, retval, retkind) \
    do { \
        FN_##name proc = (FN_##name)GetProcAddress(imm32, #name); \
        if (!proc) { \
            ERR("Could not load %s\n", #name); \
            return FALSE; \
        } \
        IMM_FN(name) = proc; \
    } while (0);
#include "immtable.h"

    return TRUE;
}

// Win: InitializeImmEntryTable
BOOL WINAPI InitializeImmEntryTable(VOID)
{
    gbImmInitializing = TRUE;
    return IntInitializeImmEntryTable();
}

// Win: User32InitializeImmEntryTable
BOOL WINAPI User32InitializeImmEntryTable(DWORD magic)
{
    TRACE("Imm (%x)\n", magic);

    if (magic != IMM_INIT_MAGIC)
        return FALSE;

    /* Check whether the IMM table has already been initialized */
    if (IMM_FN(ImmWINNLSEnableIME) != IMMSTUB_ImmWINNLSEnableIME)
        return TRUE;

    IntInitializeImmEntryTable();

    if (ghImm32 == NULL && !gbImmInitializing)
    {
        WCHAR ImmFile[MAX_PATH];
        User32GetImmFileName(ImmFile, _countof(ImmFile));
        ghImm32 = LoadLibraryW(ImmFile);
        if (ghImm32 == NULL)
        {
            ERR("Did not load imm32.dll!\n");
            return FALSE;
        }
    }

    return IMM_FN(ImmRegisterClient)(&gSharedInfo, ghImm32);
}

// Win: ImeIsUsableContext
static BOOL User32CanSetImeWindowToImc(HIMC hIMC, HWND hImeWnd)
{
    PIMC pIMC = ValidateHandle(hIMC, TYPE_INPUTCONTEXT);
    return pIMC && (!pIMC->hImeWnd || pIMC->hImeWnd == hImeWnd || !ValidateHwnd(pIMC->hImeWnd));
}

// Win: GetIMEShowStatus
static BOOL User32GetImeShowStatus(VOID)
{
    return (BOOL)NtUserCallNoParam(NOPARAM_ROUTINE_GETIMESHOWSTATUS);
}

/* Sends a message to the IME UI window. */
/* Win: SendMessageToUI(pimeui, uMsg, wParam, lParam, !unicode) */
static LRESULT
User32SendImeUIMessage(PIMEUI pimeui, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL unicode)
{
    LRESULT ret = 0;
    HWND hwndUI = pimeui->hwndUI;
    PWND pwnd, pwndUI;

    ASSERT(pimeui->spwnd != NULL);

    pwnd = pimeui->spwnd;
    pwndUI = ValidateHwnd(hwndUI);
    if (!pwnd || (pwnd->state & WNDS_DESTROYED) || (pwnd->state2 & WNDS2_INDESTROY) ||
        !pwndUI || (pwndUI->state & WNDS_DESTROYED) || (pwndUI->state2 & WNDS2_INDESTROY))
    {
        return 0;
    }

    InterlockedIncrement(&pimeui->nCntInIMEProc);

    if (unicode)
        ret = SendMessageW(hwndUI, uMsg, wParam, lParam);
    else
        ret = SendMessageA(hwndUI, uMsg, wParam, lParam);

    InterlockedDecrement(&pimeui->nCntInIMEProc);

    return ret;
}

// Win: SendOpenStatusNotify
static VOID User32NotifyOpenStatus(PIMEUI pimeui, HWND hwndIMC, BOOL bOpen)
{
    WPARAM wParam = (bOpen ? IMN_OPENSTATUSWINDOW : IMN_CLOSESTATUSWINDOW);

    ASSERT(pimeui->spwnd != NULL);

    pimeui->fShowStatus = bOpen;

    if (LOWORD(GetWin32ClientInfo()->dwExpWinVer) >= 0x400)
        SendMessageW(hwndIMC, WM_IME_NOTIFY, wParam, 0);
    else
        User32SendImeUIMessage(pimeui, WM_IME_NOTIFY, wParam, 0, TRUE);
}

// Win: ImeMarkUsedContext
static VOID User32SetImeWindowOfImc(HIMC hIMC, HWND hImeWnd)
{
    PIMC pIMC = ValidateHandle(hIMC, TYPE_INPUTCONTEXT);
    if (!pIMC || pIMC->hImeWnd == hImeWnd)
        return;

    NtUserUpdateInputContext(hIMC, UIC_IMEWINDOW, (ULONG_PTR)hImeWnd);
}

// Win: ImeSetImc
static VOID User32UpdateImcOfImeUI(PIMEUI pimeui, HIMC hNewIMC)
{
    HWND hImeWnd;
    HIMC hOldIMC = pimeui->hIMC;

    ASSERT(pimeui->spwnd != NULL);
    hImeWnd = UserHMGetHandle(pimeui->spwnd);

    if (hNewIMC == hOldIMC)
        return;

    if (hOldIMC)
        User32SetImeWindowOfImc(hOldIMC, NULL);

    pimeui->hIMC = hNewIMC;

    if (hNewIMC)
        User32SetImeWindowOfImc(hNewIMC, hImeWnd);
}

/* Handles WM_IME_NOTIFY message of the default IME window. */
/* Win: ImeNotifyHandler */
static LRESULT ImeWnd_OnImeNotify(PIMEUI pimeui, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret = 0;
    HIMC hIMC;
    LPINPUTCONTEXT pIC;
    HWND hwndUI, hwndIMC, hImeWnd, hwndOwner;

    ASSERT(pimeui->spwnd != NULL);

    switch (wParam)
    {
        case IMN_SETCONVERSIONMODE:
        case IMN_SETOPENSTATUS:
            hIMC = pimeui->hIMC;
            pIC = IMM_FN(ImmLockIMC)(hIMC);
            if (pIC)
            {
                hwndIMC = pimeui->hwndIMC;
                if (IsWindow(hwndIMC))
                {
                    NtUserNotifyIMEStatus(hwndIMC, pIC->fOpen, pIC->fdwConversion);
                }
                else if (gfConIme == TRUE && pimeui->spwnd)
                {
                    hImeWnd = UserHMGetHandle(pimeui->spwnd);
                    hwndOwner = GetWindow(hImeWnd, GW_OWNER);
                    if (hwndOwner)
                    {
                        NtUserNotifyIMEStatus(hwndOwner, pIC->fOpen, pIC->fdwConversion);
                    }
                }

                IMM_FN(ImmUnlockIMC)(hIMC);
            }
            /* FALL THROUGH */
        default:
            ret = User32SendImeUIMessage(pimeui, WM_IME_NOTIFY, wParam, lParam, TRUE);
            break;

        case IMN_PRIVATE:
            hwndUI = pimeui->hwndUI;
            if (IsWindow(hwndUI))
                ret = SendMessageW(hwndUI, WM_IME_NOTIFY, wParam, lParam);
            break;
    }

    return ret;
}

/* Creates the IME UI window. */
/* Win: CreateIMEUI */
static HWND User32CreateImeUIWindow(PIMEUI pimeui, HKL hKL)
{
    IMEINFOEX ImeInfoEx;
    PIMEDPI pImeDpi;
    WNDCLASSW wc;
    HWND hwndUI = NULL;
    CHAR szUIClass[32];
    PWND pwnd = pimeui->spwnd;

    ASSERT(pimeui->spwnd != NULL);

    if (!pwnd || !IMM_FN(ImmGetImeInfoEx)(&ImeInfoEx, ImeInfoExKeyboardLayout, &hKL))
        return NULL;

    pImeDpi = IMM_FN(ImmLockImeDpi)(hKL);
    if (!pImeDpi)
        return NULL;

    if (!GetClassInfoW(pImeDpi->hInst, ImeInfoEx.wszUIClass, &wc))
        goto Quit;

    if (ImeInfoEx.ImeInfo.fdwProperty & IME_PROP_UNICODE)
    {
        hwndUI = CreateWindowW(ImeInfoEx.wszUIClass, ImeInfoEx.wszUIClass, WS_POPUP | WS_DISABLED,
                               0, 0, 0, 0, UserHMGetHandle(pwnd), 0, wc.hInstance, NULL);
    }
    else
    {
        WideCharToMultiByte(CP_ACP, 0, ImeInfoEx.wszUIClass, -1,
                            szUIClass, _countof(szUIClass), NULL, NULL);
        szUIClass[_countof(szUIClass) - 1] = 0;

        hwndUI = CreateWindowA(szUIClass, szUIClass, WS_POPUP | WS_DISABLED,
                               0, 0, 0, 0, UserHMGetHandle(pwnd), 0, wc.hInstance, NULL);
    }

    if (hwndUI)
        NtUserSetWindowLongPtr(hwndUI, IMMGWLP_IMC, (LONG_PTR)pimeui->hIMC, FALSE);

Quit:
    IMM_FN(ImmUnlockImeDpi)(pImeDpi);
    return hwndUI;
}

/* Initializes the default IME window. */
/* Win: ImeWndCreateHandler */
static INT ImeWnd_OnCreate(PIMEUI pimeui, LPCREATESTRUCT lpCS)
{
    PWND pParentWnd, pWnd = pimeui->spwnd;
    HIMC hIMC = NULL;

    if (!pWnd || (pWnd->style & (WS_DISABLED | WS_POPUP)) != (WS_DISABLED | WS_POPUP))
        return -1;

    pParentWnd = ValidateHwnd(lpCS->hwndParent);
    if (pParentWnd)
    {
        hIMC = pParentWnd->hImc;
        if (hIMC && !User32CanSetImeWindowToImc(hIMC, UserHMGetHandle(pWnd)))
            hIMC = NULL;
    }

    User32UpdateImcOfImeUI(pimeui, hIMC);

    pimeui->fShowStatus = FALSE;
    pimeui->nCntInIMEProc = 0;
    pimeui->fActivate = FALSE;
    pimeui->fDestroy = FALSE;
    pimeui->hwndIMC = NULL;
    pimeui->hKL = GetWin32ClientInfo()->hKL;
    pimeui->fCtrlShowStatus = TRUE;
    pimeui->dwLastStatus = 0;

    return 0;
}

/* Destroys the IME UI window. */
/* Win: DestroyIMEUI */
static VOID User32DestroyImeUIWindow(PIMEUI pimeui)
{
    HWND hwndUI = pimeui->hwndUI;

    if (IsWindow(hwndUI))
    {
        pimeui->fDestroy = TRUE;
        NtUserDestroyWindow(hwndUI);
    }

    pimeui->fShowStatus = pimeui->fDestroy = FALSE;
    pimeui->hwndUI = NULL;
}

/* Handles WM_IME_SELECT message of the default IME window. */
/* Win: ImeSelectHandler */
static VOID ImeWnd_OnImeSelect(PIMEUI pimeui, WPARAM wParam, LPARAM lParam)
{
    HKL hKL;
    HWND hwndUI, hwndIMC = pimeui->hwndIMC;

    if (wParam)
    {
        pimeui->hKL = hKL = (HKL)lParam;

        if (!pimeui->fActivate)
            return;

        pimeui->hwndUI = hwndUI = User32CreateImeUIWindow(pimeui, hKL);
        if (hwndUI)
            User32SendImeUIMessage(pimeui, WM_IME_SELECT, wParam, lParam, TRUE);

        if (User32GetImeShowStatus() && pimeui->fCtrlShowStatus)
        {
            if (!pimeui->fShowStatus && pimeui->fActivate && IsWindow(hwndIMC))
                User32NotifyOpenStatus(pimeui, hwndIMC, TRUE);
        }
    }
    else
    {
        if (pimeui->fShowStatus && pimeui->fActivate && IsWindow(hwndIMC))
            User32NotifyOpenStatus(pimeui, hwndIMC, FALSE);

        User32SendImeUIMessage(pimeui, WM_IME_SELECT, wParam, lParam, TRUE);
        User32DestroyImeUIWindow(pimeui);
        pimeui->hKL = NULL;
    }
}

/* Handles WM_IME_CONTROL message of the default IME window. */
/* Win: ImeControlHandler(pimeui, wParam, lParam, !unicode) */
static LRESULT
ImeWnd_OnImeControl(PIMEUI pimeui, WPARAM wParam, LPARAM lParam, BOOL unicode)
{
    HIMC hIMC = pimeui->hIMC;
    DWORD dwConversion, dwSentence;
    POINT pt;

    if (IS_CICERO_MODE())
    {
        if (wParam == IMC_OPENSTATUSWINDOW)
        {
            IMM_FN(CtfImmRestoreToolbarWnd)(pimeui->dwLastStatus);
            pimeui->dwLastStatus = 0;
        }
        else if (wParam == IMC_CLOSESTATUSWINDOW)
        {
            pimeui->dwLastStatus = IMM_FN(CtfImmHideToolbarWnd)();
        }
    }

    if (!hIMC)
        return 0;

    switch (wParam)
    {
        case IMC_GETCONVERSIONMODE:
            if (!IMM_FN(ImmGetConversionStatus)(hIMC, &dwConversion, &dwSentence))
                return 1;
            return dwConversion;

        case IMC_GETSENTENCEMODE:
            if (!IMM_FN(ImmGetConversionStatus)(hIMC, &dwConversion, &dwSentence))
                return 1;
            return dwSentence;

        case IMC_GETOPENSTATUS:
            return IMM_FN(ImmGetOpenStatus)(hIMC);

        case IMC_SETCONVERSIONMODE:
            if (!IMM_FN(ImmGetConversionStatus)(hIMC, &dwConversion, &dwSentence) ||
                !IMM_FN(ImmSetConversionStatus)(hIMC, (DWORD)lParam, dwSentence))
            {
                return 1;
            }
            break;

        case IMC_SETSENTENCEMODE:
            if (!IMM_FN(ImmGetConversionStatus)(hIMC, &dwConversion, &dwSentence) ||
                !IMM_FN(ImmSetConversionStatus)(hIMC, dwConversion, (DWORD)lParam))
            {
                return 1;
            }
            break;

        case IMC_SETOPENSTATUS:
            if (!IMM_FN(ImmSetOpenStatus)(hIMC, (BOOL)lParam))
                return 1;
            break;

        case IMC_GETCANDIDATEPOS:
        case IMC_GETCOMPOSITIONWINDOW:
        case IMC_GETSOFTKBDPOS:
        case IMC_SETSOFTKBDPOS:
        case IMC_GETSTATUSWINDOWPOS:
            return User32SendImeUIMessage(pimeui, WM_IME_CONTROL, wParam, lParam, unicode);

        case IMC_SETCANDIDATEPOS:
            if (!IMM_FN(ImmSetCandidateWindow)(hIMC, (LPCANDIDATEFORM)lParam))
                return 1;
            break;

        case IMC_GETCOMPOSITIONFONT:
            if (unicode)
            {
                if (!IMM_FN(ImmGetCompositionFontW)(hIMC, (LPLOGFONTW)lParam))
                    return 1;
            }
            else
            {
                if (!IMM_FN(ImmGetCompositionFontA)(hIMC, (LPLOGFONTA)lParam))
                    return 1;
            }
            break;

        case IMC_SETCOMPOSITIONFONT:
            if (unicode)
            {
                if (!IMM_FN(ImmSetCompositionFontW)(hIMC, (LPLOGFONTW)lParam))
                    return 1;
            }
            else
            {
                if (!IMM_FN(ImmSetCompositionFontA)(hIMC, (LPLOGFONTA)lParam))
                    return 1;
            }
            break;

        case IMC_SETCOMPOSITIONWINDOW:
            if (!IMM_FN(ImmSetCompositionWindow)(hIMC, (LPCOMPOSITIONFORM)lParam))
                return 1;
            break;

        case IMC_SETSTATUSWINDOWPOS:
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            if (!IMM_FN(ImmSetStatusWindowPos)(hIMC, &pt))
                return 1;
            break;

        case IMC_CLOSESTATUSWINDOW:
            if (pimeui->fShowStatus && User32GetImeShowStatus())
            {
                pimeui->fShowStatus = FALSE;
                User32SendImeUIMessage(pimeui, WM_IME_NOTIFY, IMN_CLOSESTATUSWINDOW, 0, TRUE);
            }
            pimeui->fCtrlShowStatus = FALSE;
            break;

        case IMC_OPENSTATUSWINDOW:
            if (!pimeui->fShowStatus && User32GetImeShowStatus())
            {
                pimeui->fShowStatus = TRUE;
                User32SendImeUIMessage(pimeui, WM_IME_NOTIFY, IMN_OPENSTATUSWINDOW, 0, TRUE);
            }
            pimeui->fCtrlShowStatus = TRUE;
            break;

        default:
            break;
    }

    return 0;
}

/* Modify the IME activation status. */
/* Win: FocusSetIMCContext */
static VOID FASTCALL User32SetImeActivenessOfWindow(HWND hWnd, BOOL bActive)
{
    HIMC hIMC;

    if (!hWnd || !IsWindow(hWnd))
    {
        IMM_FN(ImmSetActiveContext)(NULL, NULL, bActive);
        return;
    }

    hIMC = IMM_FN(ImmGetContext)(hWnd);
    IMM_FN(ImmSetActiveContext)(hWnd, hIMC, bActive);
    IMM_FN(ImmReleaseContext)(hWnd, hIMC);
}

/* Win: CtfLoadThreadLayout */
VOID FASTCALL CtfLoadThreadLayout(PIMEUI pimeui)
{
    IMM_FN(CtfImmTIMActivate)(pimeui->hKL);
    pimeui->hKL = GetWin32ClientInfo()->hKL;
    IMM_FN(ImmLoadIME)(pimeui->hKL);
    pimeui->hwndUI = NULL;
}

/* Open the IME help or check the existence of the IME help. */
static LRESULT FASTCALL
User32DoImeHelp(PIMEUI pimeui, WPARAM wParam, LPARAM lParam)
{
    WCHAR szHelpFile[MAX_PATH];
    DWORD ret, dwEsc = IME_ESC_GETHELPFILENAME;
    size_t cch;

    /* Is there any IME help file? */
    ret = IMM_FN(ImmEscapeW)(pimeui->hKL, pimeui->hIMC, IME_ESC_QUERY_SUPPORT, &dwEsc);
    if (!ret || !lParam)
        return ret;

    /* Get the help filename */
    if (IMM_FN(ImmEscapeW)(pimeui->hKL, pimeui->hIMC, IME_ESC_GETHELPFILENAME, szHelpFile))
    {
        /* Check filename extension */
        cch = wcslen(szHelpFile);
        if (cch > 4 && _wcsicmp(&szHelpFile[cch - 4], L".HLP") == 0)
        {
            /* Open the old-style help */
            TRACE("szHelpFile: %s\n", debugstr_w(szHelpFile));
            WinHelpW(NULL, szHelpFile, HELP_FINDER, 0);
        }
        else
        {
            /* Open the new-style help */
            FIXME("(%p, %p, %p): %s\n", pimeui, wParam, lParam, debugstr_w(szHelpFile));
            ret = FALSE;
        }
    }

    return ret;
}

/* Handles WM_IME_SYSTEM message of the default IME window. */
/* Win: ImeSystemHandler */
static LRESULT ImeWnd_OnImeSystem(PIMEUI pimeui, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret = 0;
    LPINPUTCONTEXTDX pIC;
    HIMC hIMC = pimeui->hIMC;
    LPCANDIDATEFORM pCandForm;
    LPCOMPOSITIONFORM pCompForm;
    DWORD dwConversion, dwSentence;
    HWND hImeWnd;
    BOOL bCompForm;
    CANDIDATEFORM CandForm;
    COMPOSITIONFORM CompForm;
    UINT iCandForm;

    ASSERT(pimeui->spwnd != NULL);

    switch (wParam)
    {
        case IMS_NOTIFYIMESHOW:
            if (User32GetImeShowStatus() == !lParam)
            {
                hImeWnd = UserHMGetHandle(pimeui->spwnd);
                NtUserCallHwndParamLock(hImeWnd, lParam, TWOPARAM_ROUTINE_IMESHOWSTATUSCHANGE);
            }
            break;

        case IMS_UPDATEIMEUI:
            if (!hIMC)
                break;

            bCompForm = TRUE;
            pIC = IMM_FN(ImmLockIMC)(hIMC);
            if (pIC)
            {
                bCompForm = !(pIC->dwUIFlags & 0x2);
                IMM_FN(ImmUnlockIMC)(hIMC);
            }

            if (!IsWindow(pimeui->hwndIMC))
                break;

            if (bCompForm && IMM_FN(ImmGetCompositionWindow)(hIMC, &CompForm))
            {
                if (CompForm.dwStyle)
                    IMM_FN(ImmSetCompositionWindow)(hIMC, &CompForm);
            }

            for (iCandForm = 0; iCandForm < MAX_CANDIDATEFORM; ++iCandForm)
            {
                if (IMM_FN(ImmGetCandidateWindow)(hIMC, iCandForm, &CandForm))
                {
                    if (CandForm.dwStyle)
                        IMM_FN(ImmSetCandidateWindow)(hIMC, &CandForm);
                }
            }
            break;

        case IMS_SETCANDFORM:
            pIC = IMM_FN(ImmLockIMC)(hIMC);
            if (!pIC)
                break;

            pCandForm = &pIC->cfCandForm[lParam];
            IMM_FN(ImmSetCandidateWindow)(hIMC, pCandForm);
            IMM_FN(ImmUnlockIMC)(hIMC);
            break;

        case IMS_SETCOMPFONT:
            pIC = IMM_FN(ImmLockIMC)(hIMC);
            if (!pIC)
                break;

            IMM_FN(ImmSetCompositionFontW)(hIMC, &pIC->lfFont.W);
            IMM_FN(ImmUnlockIMC)(hIMC);
            break;

        case IMS_SETCOMPFORM:
            pIC = IMM_FN(ImmLockIMC)(hIMC);
            if (!pIC)
                break;

            pCompForm = &pIC->cfCompForm;
            pIC->dwUIFlags |= 0x8;
            IMM_FN(ImmSetCompositionWindow)(hIMC, pCompForm);
            IMM_FN(ImmUnlockIMC)(hIMC);
            break;

        case IMS_CONFIGURE:
            IMM_FN(ImmConfigureIMEW)((HKL)lParam, pimeui->hwndIMC, IME_CONFIG_GENERAL, NULL);
            break;

        case IMS_SETOPENSTATUS:
            if (hIMC)
                IMM_FN(ImmSetOpenStatus)(hIMC, (BOOL)lParam);
            break;

        case IMS_FREELAYOUT:
            ret = IMM_FN(ImmFreeLayout)((HKL)lParam);
            break;

        case 0x13:
            FIXME("\n");
            break;

        case IMS_GETCONVSTATUS:
            IMM_FN(ImmGetConversionStatus)(hIMC, &dwConversion, &dwSentence);
            ret = dwConversion;
            break;

        case IMS_IMEHELP:
            return User32DoImeHelp(pimeui, wParam, lParam);

        case IMS_IMEACTIVATE:
            User32SetImeActivenessOfWindow((HWND)lParam, TRUE);
            break;

        case IMS_IMEDEACTIVATE:
            User32SetImeActivenessOfWindow((HWND)lParam, FALSE);
            break;

        case IMS_ACTIVATELAYOUT:
            ret = IMM_FN(ImmActivateLayout)((HKL)lParam);
            break;

        case IMS_GETIMEMENU:
            ret = IMM_FN(ImmPutImeMenuItemsIntoMappedFile)((HIMC)lParam);
            break;

        case 0x1D:
            FIXME("\n");
            break;

        case IMS_GETCONTEXT:
            ret = (ULONG_PTR)IMM_FN(ImmGetContext)((HWND)lParam);
            break;

        case IMS_SENDNOTIFICATION:
        case IMS_COMPLETECOMPSTR:
        case IMS_SETLANGBAND:
        case IMS_UNSETLANGBAND:
            ret = IMM_FN(ImmSystemHandler)(hIMC, wParam, lParam);
            break;

        case IMS_LOADTHREADLAYOUT:
            CtfLoadThreadLayout(pimeui);
            break;

        default:
            break;
    }

    return ret;
}

/* Handles WM_IME_SETCONTEXT message of the default IME window. */
/* Win: ImeSetContextHandler */
LRESULT ImeWnd_OnImeSetContext(PIMEUI pimeui, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret;
    HIMC hIMC;
    LPINPUTCONTEXTDX pIC;
    HWND hwndFocus, hwndOldImc, hwndNewImc, hImeWnd, hwndActive, hwndOwner;
    PWND pwndFocus, pImeWnd, pwndOwner;
    COMPOSITIONFORM CompForm;

    pimeui->fActivate = !!wParam;
    hwndOldImc = pimeui->hwndIMC;
    ASSERT(pimeui->spwnd != NULL);

    if (wParam)
    {
        if (!pimeui->hwndUI)
            pimeui->hwndUI = User32CreateImeUIWindow(pimeui, pimeui->hKL);

        if (gfConIme == -1)
        {
            gfConIme = (INT)NtUserGetThreadState(THREADSTATE_CHECKCONIME);
            if (gfConIme)
                pimeui->fCtrlShowStatus = FALSE;
        }

        hImeWnd = UserHMGetHandle(pimeui->spwnd);

        if (gfConIme)
        {
            hwndOwner = GetWindow(hImeWnd, GW_OWNER);
            pwndOwner = ValidateHwnd(hwndOwner);
            if (pwndOwner)
            {
                User32UpdateImcOfImeUI(pimeui, pwndOwner->hImc);

                if (pimeui->hwndUI)
                    SetWindowLongPtrW(pimeui->hwndUI, IMMGWLP_IMC, (LONG_PTR)pwndOwner->hImc);
            }

            return User32SendImeUIMessage(pimeui, WM_IME_SETCONTEXT, wParam, lParam, TRUE);
        }

        hwndFocus = (HWND)NtUserQueryWindow(hImeWnd, QUERY_WINDOW_FOCUS);

        hIMC = IMM_FN(ImmGetContext)(hwndFocus);

        if (hIMC && !User32CanSetImeWindowToImc(hIMC, hImeWnd))
        {
            User32UpdateImcOfImeUI(pimeui, NULL);
            return 0;
        }

        User32UpdateImcOfImeUI(pimeui, hIMC);

        if (pimeui->hwndUI)
            SetWindowLongPtrW(pimeui->hwndUI, IMMGWLP_IMC, (LONG_PTR)hIMC);

        if (hIMC)
        {
            pIC = IMM_FN(ImmLockIMC)(hIMC);
            if (!pIC)
                return 0;

            if (hwndFocus != pIC->hWnd)
            {
                IMM_FN(ImmUnlockIMC)(hIMC);
                return 0;
            }

            if ((pIC->dwUIFlags & 0x40000) && hwndOldImc != hwndFocus)
            {
                RtlZeroMemory(&CompForm, sizeof(CompForm));
                IMM_FN(ImmSetCompositionWindow)(hIMC, &CompForm);

                pIC->dwUIFlags &= ~0x40000;
            }

            IMM_FN(ImmUnlockIMC)(hIMC);

            hImeWnd = UserHMGetHandle(pimeui->spwnd);
            if (NtUserSetImeOwnerWindow(hImeWnd, hwndFocus))
                pimeui->hwndIMC = hwndFocus;
        }
        else
        {
            pimeui->hwndIMC = hwndFocus;

            hImeWnd = UserHMGetHandle(pimeui->spwnd);
            NtUserSetImeOwnerWindow(hImeWnd, NULL);
        }
    }

    ret = User32SendImeUIMessage(pimeui, WM_IME_SETCONTEXT, wParam, lParam, TRUE);

    if (!pimeui->spwnd)
        return 0;

    if (!pimeui->fCtrlShowStatus || !User32GetImeShowStatus())
        return ret;

    hImeWnd = UserHMGetHandle(pimeui->spwnd);
    hwndFocus = (HWND)NtUserQueryWindow(hImeWnd, QUERY_WINDOW_FOCUS);
    pwndFocus = ValidateHwnd(hwndFocus);

    if (wParam)
    {
        pImeWnd = ValidateHwnd(hImeWnd);
        if (pwndFocus && pImeWnd && pImeWnd->head.pti == pwndFocus->head.pti)
        {
            hwndNewImc = pimeui->hwndIMC;
            if (pimeui->fShowStatus)
            {
                if (hwndOldImc && hwndNewImc && hwndOldImc != hwndNewImc &&
                    IntGetTopLevelWindow(hwndOldImc) != IntGetTopLevelWindow(hwndNewImc))
                {
                    User32NotifyOpenStatus(pimeui, hwndOldImc, FALSE);
                    User32NotifyOpenStatus(pimeui, hwndNewImc, TRUE);
                }
            }
            else
            {
                if (ValidateHwnd(hwndNewImc))
                    User32NotifyOpenStatus(pimeui, hwndNewImc, TRUE);
            }
        }

        pImeWnd = pimeui->spwnd;
        hImeWnd = (pImeWnd ? UserHMGetHandle(pImeWnd) : NULL);
        if (hImeWnd)
            NtUserCallHwndLock(hImeWnd, HWNDLOCK_ROUTINE_CHECKIMESHOWSTATUSINTHRD);
    }
    else
    {
        pImeWnd = pimeui->spwnd;
        hImeWnd = UserHMGetHandle(pImeWnd);
        hwndActive = (HWND)NtUserQueryWindow(hImeWnd, QUERY_WINDOW_ACTIVE);
        if (!pwndFocus || !hwndActive || pImeWnd->head.pti != pwndFocus->head.pti)
        {
            if (IsWindow(hwndOldImc))
            {
                User32NotifyOpenStatus(pimeui, hwndOldImc, FALSE);
            }
            else
            {
                pimeui->fShowStatus = FALSE;
                User32SendImeUIMessage(pimeui, WM_IME_NOTIFY, IMN_CLOSESTATUSWINDOW, 0, TRUE);
            }
        }
    }

    return ret;
}

/* The window procedure of the default IME window */
/* Win: ImeWndProcWorker(pWnd, msg, wParam, lParam, !unicode) */
LRESULT WINAPI
ImeWndProc_common(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, BOOL unicode) // ReactOS
{
    PWND pWnd;
    PIMEUI pimeui;
    LRESULT ret;

    pWnd = ValidateHwnd(hwnd);
    if (pWnd == NULL)
    {
        ERR("hwnd was %p\n", hwnd);
        return 0;
    }

    if (!pWnd->fnid)
    {
        NtUserSetWindowFNID(hwnd, FNID_IME);
    }
    else if (pWnd->fnid != FNID_IME)
    {
        ERR("fnid was 0x%x\n", pWnd->fnid);
        return 0;
    }

    pimeui = (PIMEUI)GetWindowLongPtrW(hwnd, 0);
    if (pimeui == NULL)
    {
        pimeui = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IMEUI));
        if (pimeui == NULL)
        {
            ERR("HeapAlloc failed\n");
            NtUserSetWindowFNID(hwnd, FNID_DESTROY);
            DestroyWindow(hwnd);
            return 0;
        }

        SetWindowLongPtrW(hwnd, 0, (LONG_PTR)pimeui);
        pimeui->spwnd = pWnd;
    }

    if (IS_CICERO_MODE())
    {
        ret = IMM_FN(CtfImmDispatchDefImeMessage)(hwnd, msg, wParam, lParam);
        if (ret)
            return ret;
    }

    if (pimeui->nCntInIMEProc > 0)
    {
        switch (msg)
        {
            case WM_IME_CHAR:
            case WM_IME_COMPOSITIONFULL:
            case WM_IME_CONTROL:
            case WM_IME_REQUEST:
            case WM_IME_SELECT:
            case WM_IME_SETCONTEXT:
            case WM_IME_STARTCOMPOSITION:
            case WM_IME_COMPOSITION:
            case WM_IME_ENDCOMPOSITION:
                return 0;

            case WM_IME_NOTIFY:
                if (wParam < IMN_PRIVATE || IS_IME_HKL(pimeui->hKL) || !IS_CICERO_MODE())
                    return 0;
                break;

            case WM_IME_SYSTEM:
                switch (wParam)
                {
                    case 0x03:
                    case 0x10:
                    case 0x13:
                        break;

                    default:
                        return 0;
                }
                break;

            default:
                goto Finish;
        }
    }

    if ((pWnd->state2 & WNDS2_INDESTROY) || (pWnd->state & WNDS_DESTROYED))
    {
        switch (msg)
        {
            case WM_DESTROY:
            case WM_NCDESTROY:
            case WM_FINALDESTROY:
                break;

            default:
                return 0;
        }
    }

    switch (msg)
    {
        case WM_CREATE:
            return ImeWnd_OnCreate(pimeui, (LPCREATESTRUCT)lParam);

        case WM_DESTROY:
            User32DestroyImeUIWindow(pimeui);
            return 0;

        case WM_NCDESTROY:
        case WM_FINALDESTROY:
            pimeui->spwnd = NULL;
            HeapFree(GetProcessHeap(), 0, pimeui);
            NtUserSetWindowFNID(hwnd, FNID_DESTROY);
            break;

        case WM_ERASEBKGND:
            return TRUE;

        case WM_PAINT:
            return 0;

        case WM_IME_STARTCOMPOSITION:
        case WM_IME_COMPOSITION:
        case WM_IME_ENDCOMPOSITION:
            return User32SendImeUIMessage(pimeui, msg, wParam, lParam, unicode);

        case WM_IME_CONTROL:
            return ImeWnd_OnImeControl(pimeui, wParam, lParam, unicode);

        case WM_IME_NOTIFY:
            return ImeWnd_OnImeNotify(pimeui, wParam, lParam);

        case WM_IME_REQUEST:
            return 0;

        case WM_IME_SELECT:
            ImeWnd_OnImeSelect(pimeui, wParam, lParam);
            return (LRESULT)pimeui;

        case WM_IME_SETCONTEXT:
            return ImeWnd_OnImeSetContext(pimeui, wParam, lParam);

        case WM_IME_SYSTEM:
            return ImeWnd_OnImeSystem(pimeui, wParam, lParam);

        default:
            break;
    }

Finish:
    if (unicode)
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

// Win: ImeWndProcA
LRESULT WINAPI ImeWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return ImeWndProc_common(hwnd, msg, wParam, lParam, FALSE);
}

// Win: ImeWndProcW
LRESULT WINAPI ImeWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return ImeWndProc_common(hwnd, msg, wParam, lParam, TRUE);
}

// Win: UpdatePerUserImmEnabling
BOOL WINAPI UpdatePerUserImmEnabling(VOID)
{
    HMODULE imm32;
    BOOL ret;

    ret = NtUserCallNoParam(NOPARAM_ROUTINE_UPDATEPERUSERIMMENABLING);
    if (!ret || !(gpsi->dwSRVIFlags & SRVINFO_IMM32))
        return FALSE;

    imm32 = GetModuleHandleW(L"imm32.dll");
    if (imm32)
        return TRUE;

    imm32 = LoadLibraryW(L"imm32.dll");
    if (imm32 == NULL)
    {
        ERR("Imm32 not installed!\n");
        ret = FALSE;
    }

    return ret;
}

BOOL
WINAPI
RegisterIMEClass(VOID)
{
    ATOM atom;
    WNDCLASSEXW WndClass = { sizeof(WndClass) };

    WndClass.lpszClassName  = L"IME";
    WndClass.style          = CS_GLOBALCLASS;
    WndClass.lpfnWndProc    = ImeWndProcW;
    WndClass.cbWndExtra     = sizeof(LONG_PTR);
    WndClass.hCursor        = LoadCursorW(NULL, IDC_ARROW);

    atom = RegisterClassExWOWW(&WndClass, 0, FNID_IME, 0, FALSE);
    if (!atom)
    {
        ERR("Failed to register IME Class!\n");
        return FALSE;
    }

    RegisterDefaultClasses |= ICLASS_TO_MASK(ICLS_IME);
    TRACE("RegisterIMEClass atom = %u\n", atom);
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
IMPSetIMEW(HWND hwnd, LPIMEPROW ime)
{
    return IMM_FN(ImmIMPSetIMEW)(hwnd, ime);
}

/*
 * @implemented
 */
BOOL
WINAPI
IMPQueryIMEW(LPIMEPROW ime)
{
    return IMM_FN(ImmIMPQueryIMEW)(ime);
}

/*
 * @implemented
 */
BOOL
WINAPI
IMPGetIMEW(HWND hwnd, LPIMEPROW ime)
{
    return IMM_FN(ImmIMPGetIMEW)(hwnd, ime);
}

/*
 * @implemented
 */
BOOL
WINAPI
IMPSetIMEA(HWND hwnd, LPIMEPROA ime)
{
    return IMM_FN(ImmIMPSetIMEA)(hwnd, ime);
}

/*
 * @implemented
 */
BOOL
WINAPI
IMPQueryIMEA(LPIMEPROA ime)
{
    return IMM_FN(ImmIMPQueryIMEA)(ime);
}

/*
 * @implemented
 */
BOOL
WINAPI
IMPGetIMEA(HWND hwnd, LPIMEPROA ime)
{
    return IMM_FN(ImmIMPGetIMEA)(hwnd, ime);
}

/*
 * @implemented
 */
LRESULT
WINAPI
SendIMEMessageExW(HWND hwnd, LPARAM lParam)
{
    return IMM_FN(ImmSendIMEMessageExW)(hwnd, lParam);
}

/*
 * @implemented
 */
LRESULT
WINAPI
SendIMEMessageExA(HWND hwnd, LPARAM lParam)
{
    return IMM_FN(ImmSendIMEMessageExA)(hwnd, lParam);
}

/*
 * @implemented
 */
BOOL
WINAPI
WINNLSEnableIME(HWND hwnd, BOOL enable)
{
    return IMM_FN(ImmWINNLSEnableIME)(hwnd, enable);
}

/*
 * @implemented
 */
BOOL
WINAPI
WINNLSGetEnableStatus(HWND hwnd)
{
    return IMM_FN(ImmWINNLSGetEnableStatus)(hwnd);
}

/*
 * @implemented
 */
UINT
WINAPI
WINNLSGetIMEHotkey(HWND hwnd)
{
    return FALSE;
}
