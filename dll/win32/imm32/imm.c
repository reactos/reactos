/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing Far-Eastern languages input
 * COPYRIGHT:   Copyright 1998 Patrik Stridvall
 *              Copyright 2002, 2003, 2007 CodeWeavers, Aric Stewart
 *              Copyright 2017 James Tabor <james.tabor@reactos.org>
 *              Copyright 2018 Amine Khaldi <amine.khaldi@reactos.org>
 *              Copyright 2020 Oleg Dubinskiy <oleg.dubinskij2013@yandex.ua>
 *              Copyright 2020-2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

HMODULE g_hImm32Inst = NULL;
PSERVERINFO g_psi = NULL;
SHAREDINFO g_SharedInfo = { NULL };
BYTE g_bClientRegd = FALSE;

BOOL APIENTRY Imm32InitInstance(HMODULE hMod)
{
    NTSTATUS status;

    if (hMod)
        g_hImm32Inst = hMod;

    if (g_bClientRegd)
        return TRUE;

    status = RtlInitializeCriticalSection(&g_csImeDpi);
    if (NT_ERROR(status))
        return FALSE;

    g_bClientRegd = TRUE;
    return TRUE;
}

/***********************************************************************
 *		ImmRegisterClient(IMM32.@)
 *       ( Undocumented, called from user32.dll )
 */
BOOL WINAPI ImmRegisterClient(PSHAREDINFO ptr, HINSTANCE hMod)
{
    g_SharedInfo = *ptr;
    g_psi = g_SharedInfo.psi;
    return Imm32InitInstance(hMod);
}

/***********************************************************************
 *		ImmLoadLayout (IMM32.@)
 */
HKL WINAPI ImmLoadLayout(HKL hKL, PIMEINFOEX pImeInfoEx)
{
    DWORD cbData;
    UNICODE_STRING UnicodeString;
    HKEY hLayoutKey = NULL, hLayoutsKey = NULL;
    LONG error;
    NTSTATUS Status;
    WCHAR szLayout[MAX_PATH];

    TRACE("(%p, %p)\n", hKL, pImeInfoEx);

    if (IS_IME_HKL(hKL) ||
        !IS_CICERO_ENABLED() ||
        ((PW32CLIENTINFO)NtCurrentTeb()->Win32ClientInfo)->W32ClientInfo[0] & 2)
    {
        UnicodeString.Buffer = szLayout;
        UnicodeString.MaximumLength = sizeof(szLayout);
        Status = RtlIntegerToUnicodeString((DWORD_PTR)hKL, 16, &UnicodeString);
        if (!NT_SUCCESS(Status))
            return NULL;

        error = RegOpenKeyW(HKEY_LOCAL_MACHINE, REGKEY_KEYBOARD_LAYOUTS, &hLayoutsKey);
        if (error)
            return NULL;

        error = RegOpenKeyW(hLayoutsKey, szLayout, &hLayoutKey);
    }
    else
    {
        error = RegOpenKeyW(HKEY_LOCAL_MACHINE, REGKEY_IMM, &hLayoutKey);
    }

    if (error)
    {
        ERR("RegOpenKeyW error: 0x%08lX\n", error);
        hKL = NULL;
    }
    else
    {
        cbData = sizeof(pImeInfoEx->wszImeFile);
        error = RegQueryValueExW(hLayoutKey, L"Ime File", 0, 0,
                                 (LPBYTE)pImeInfoEx->wszImeFile, &cbData);
        if (error)
            hKL = NULL;
    }

    RegCloseKey(hLayoutKey);
    if (hLayoutsKey)
        RegCloseKey(hLayoutsKey);
    return hKL;
}

typedef struct _tagImmHkl
{
    struct list entry;
    HKL         hkl;
    HMODULE     hIME;
    IMEINFO     imeInfo;
    WCHAR       imeClassName[17]; /* 16 character max */
    ULONG       uSelected;
    HWND        UIWnd;

    /* Function Pointers */
    BOOL (WINAPI *pImeInquire)(IMEINFO *, WCHAR *, const WCHAR *);
    BOOL (WINAPI *pImeConfigure)(HKL, HWND, DWORD, void *);
    BOOL (WINAPI *pImeDestroy)(UINT);
    LRESULT (WINAPI *pImeEscape)(HIMC, UINT, void *);
    BOOL (WINAPI *pImeSelect)(HIMC, BOOL);
    BOOL (WINAPI *pImeSetActiveContext)(HIMC, BOOL);
    UINT (WINAPI *pImeToAsciiEx)(UINT, UINT, const BYTE *, DWORD *, UINT, HIMC);
    BOOL (WINAPI *pNotifyIME)(HIMC, DWORD, DWORD, DWORD);
    BOOL (WINAPI *pImeRegisterWord)(const WCHAR *, DWORD, const WCHAR *);
    BOOL (WINAPI *pImeUnregisterWord)(const WCHAR *, DWORD, const WCHAR *);
    UINT (WINAPI *pImeEnumRegisterWord)(REGISTERWORDENUMPROCW, const WCHAR *, DWORD, const WCHAR *, void *);
    BOOL (WINAPI *pImeSetCompositionString)(HIMC, DWORD, const void *, DWORD, const void *, DWORD);
    DWORD (WINAPI *pImeConversionList)(HIMC, const WCHAR *, CANDIDATELIST *, DWORD, UINT);
    BOOL (WINAPI *pImeProcessKey)(HIMC, UINT, LPARAM, const BYTE *);
    UINT (WINAPI *pImeGetRegisterWordStyle)(UINT, STYLEBUFW *);
    DWORD (WINAPI *pImeGetImeMenuItems)(HIMC, DWORD, DWORD, IMEMENUITEMINFOW *, IMEMENUITEMINFOW *, DWORD);
} ImmHkl;

typedef struct tagInputContextData
{
    DWORD           dwLock;
    INPUTCONTEXT    IMC;
    DWORD           threadID;

    ImmHkl          *immKbd;
    UINT            lastVK;
    BOOL            threadDefault;
    DWORD           magic;
} InputContextData;

#define WINE_IMC_VALID_MAGIC 0x56434D49

typedef struct _tagIMMThreadData
{
    struct list entry;
    DWORD threadID;
    HIMC defaultContext;
    HWND hwndDefault;
    BOOL disableIME;
    DWORD windowRefs;
} IMMThreadData;

static struct list ImmHklList = LIST_INIT(ImmHklList);
static struct list ImmThreadDataList = LIST_INIT(ImmThreadDataList);

static const WCHAR szwWineIMCProperty[] = L"WineImmHIMCProperty";

static const WCHAR szImeFileW[] = L"Ime File";
static const WCHAR szLayoutTextW[] = L"Layout Text";
static const WCHAR szImeRegFmt[] = L"System\\CurrentControlSet\\Control\\Keyboard Layouts\\%08lx";

static inline BOOL is_himc_ime_unicode(const InputContextData *data)
{
    return !!(data->immKbd->imeInfo.fdwProperty & IME_PROP_UNICODE);
}

static inline BOOL is_kbd_ime_unicode(const ImmHkl *hkl)
{
    return !!(hkl->imeInfo.fdwProperty & IME_PROP_UNICODE);
}

static InputContextData* get_imc_data(HIMC hIMC);

static HMODULE load_graphics_driver(void)
{
    static const WCHAR display_device_guid_propW[] = L"__wine_display_device_guid";
    static const WCHAR key_pathW[] = L"System\\CurrentControlSet\\Control\\Video\\{";
    static const WCHAR displayW[] = L"}\\0000";
    static const WCHAR driverW[] = L"GraphicsDriver";

    HMODULE ret = 0;
    HKEY hkey;
    DWORD size;
    WCHAR path[MAX_PATH];
    WCHAR key[ARRAY_SIZE( key_pathW ) + ARRAY_SIZE( displayW ) + 40];
    UINT guid_atom = HandleToULong( GetPropW( GetDesktopWindow(), display_device_guid_propW ));

    if (!guid_atom) return 0;
    memcpy( key, key_pathW, sizeof(key_pathW) );
    if (!GlobalGetAtomNameW( guid_atom, key + lstrlenW(key), 40 )) return 0;
    lstrcatW( key, displayW );
    if (RegOpenKeyW( HKEY_LOCAL_MACHINE, key, &hkey )) return 0;
    size = sizeof(path);
    if (!RegQueryValueExW( hkey, driverW, NULL, NULL, (BYTE *)path, &size )) ret = LoadLibraryW( path );
    RegCloseKey( hkey );
    TRACE( "%s %p\n", debugstr_w(path), ret );
    return ret;
}

/* ImmHkl loading and freeing */
#define LOAD_FUNCPTR(f) if((ptr->p##f = (LPVOID)GetProcAddress(ptr->hIME, #f)) == NULL){WARN("Can't find function %s in ime\n", #f);}
static ImmHkl *IMM_GetImmHkl(HKL hkl)
{
    ImmHkl *ptr;
    WCHAR filename[MAX_PATH];

    TRACE("Seeking ime for keyboard %p\n",hkl);

    LIST_FOR_EACH_ENTRY(ptr, &ImmHklList, ImmHkl, entry)
    {
        if (ptr->hkl == hkl)
            return ptr;
    }
    /* not found... create it */

    ptr = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ImmHkl));

    ptr->hkl = hkl;
    if (ImmGetIMEFileNameW(hkl, filename, MAX_PATH)) ptr->hIME = LoadLibraryW(filename);
    if (!ptr->hIME) ptr->hIME = load_graphics_driver();
    if (ptr->hIME)
    {
        LOAD_FUNCPTR(ImeInquire);
        if (!ptr->pImeInquire || !ptr->pImeInquire(&ptr->imeInfo, ptr->imeClassName, NULL))
        {
            FreeLibrary(ptr->hIME);
            ptr->hIME = NULL;
        }
        else
        {
            LOAD_FUNCPTR(ImeDestroy);
            LOAD_FUNCPTR(ImeSelect);
            if (!ptr->pImeSelect || !ptr->pImeDestroy)
            {
                FreeLibrary(ptr->hIME);
                ptr->hIME = NULL;
            }
            else
            {
                LOAD_FUNCPTR(ImeConfigure);
                LOAD_FUNCPTR(ImeEscape);
                LOAD_FUNCPTR(ImeSetActiveContext);
                LOAD_FUNCPTR(ImeToAsciiEx);
                LOAD_FUNCPTR(NotifyIME);
                LOAD_FUNCPTR(ImeRegisterWord);
                LOAD_FUNCPTR(ImeUnregisterWord);
                LOAD_FUNCPTR(ImeEnumRegisterWord);
                LOAD_FUNCPTR(ImeSetCompositionString);
                LOAD_FUNCPTR(ImeConversionList);
                LOAD_FUNCPTR(ImeProcessKey);
                LOAD_FUNCPTR(ImeGetRegisterWordStyle);
                LOAD_FUNCPTR(ImeGetImeMenuItems);
                /* make sure our classname is WCHAR */
                if (!is_kbd_ime_unicode(ptr))
                {
                    WCHAR bufW[17];
                    MultiByteToWideChar(CP_ACP, 0, (LPSTR)ptr->imeClassName,
                                        -1, bufW, 17);
                    lstrcpyW(ptr->imeClassName, bufW);
                }
            }
        }
    }
    list_add_head(&ImmHklList,&ptr->entry);

    return ptr;
}
#undef LOAD_FUNCPTR

static InputContextData* get_imc_data(HIMC hIMC)
{
    InputContextData *data = (InputContextData *)hIMC;

    if (hIMC == NULL)
        return NULL;

    if(IsBadReadPtr(data, sizeof(InputContextData)) || data->magic != WINE_IMC_VALID_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }
    return data;
}

static HIMC get_default_context( HWND hwnd )
{
    FIXME("Don't use this function\n");
    return FALSE;
}

static BOOL IMM_IsCrossThreadAccess(HWND hWnd,  HIMC hIMC)
{
    InputContextData *data;

    if (hWnd)
    {
        DWORD thread = GetWindowThreadProcessId(hWnd, NULL);
        if (thread != GetCurrentThreadId()) return TRUE;
    }
    data = get_imc_data(hIMC);
    if (data && data->threadID != GetCurrentThreadId())
        return TRUE;

    return FALSE;
}

/***********************************************************************
 *		ImmAssociateContext (IMM32.@)
 */
HIMC WINAPI ImmAssociateContext(HWND hWnd, HIMC hIMC)
{
    HIMC old = NULL;
    InputContextData *data = get_imc_data(hIMC);

    TRACE("(%p, %p):\n", hWnd, hIMC);

    if(hIMC && !data)
        return NULL;

    /*
     * If already associated just return
     */
    if (hIMC && data->IMC.hWnd == hWnd)
        return hIMC;

    if (hIMC && IMM_IsCrossThreadAccess(hWnd, hIMC))
        return NULL;

    if (hWnd)
    {
        HIMC defaultContext = get_default_context( hWnd );
        old = RemovePropW(hWnd,szwWineIMCProperty);

        if (old == NULL)
            old = defaultContext;
        else if (old == (HIMC)-1)
            old = NULL;

        if (hIMC != defaultContext)
        {
            if (hIMC == NULL) /* Meaning disable imm for that window*/
                SetPropW(hWnd,szwWineIMCProperty,(HANDLE)-1);
            else
                SetPropW(hWnd,szwWineIMCProperty,hIMC);
        }

        if (old)
        {
            InputContextData *old_data = (InputContextData *)old;
            if (old_data->IMC.hWnd == hWnd)
                old_data->IMC.hWnd = NULL;
        }
    }

    if (!hIMC)
        return old;

    if(GetActiveWindow() == data->IMC.hWnd)
    {
        SendMessageW(data->IMC.hWnd, WM_IME_SETCONTEXT, FALSE, ISC_SHOWUIALL);
        data->IMC.hWnd = hWnd;
        SendMessageW(data->IMC.hWnd, WM_IME_SETCONTEXT, TRUE, ISC_SHOWUIALL);
    }

    return old;
}


/*
 * Helper function for ImmAssociateContextEx
 */
static BOOL CALLBACK _ImmAssociateContextExEnumProc(HWND hwnd, LPARAM lParam)
{
    HIMC hImc = (HIMC)lParam;
    ImmAssociateContext(hwnd,hImc);
    return TRUE;
}

/***********************************************************************
 *              ImmAssociateContextEx (IMM32.@)
 */
BOOL WINAPI ImmAssociateContextEx(HWND hWnd, HIMC hIMC, DWORD dwFlags)
{
    TRACE("(%p, %p, 0x%x):\n", hWnd, hIMC, dwFlags);

    if (!hWnd)
        return FALSE;

    switch (dwFlags)
    {
    case 0:
        ImmAssociateContext(hWnd,hIMC);
        return TRUE;
    case IACE_DEFAULT:
    {
        HIMC defaultContext = get_default_context( hWnd );
        if (!defaultContext) return FALSE;
        ImmAssociateContext(hWnd,defaultContext);
        return TRUE;
    }
    case IACE_IGNORENOCONTEXT:
        if (GetPropW(hWnd,szwWineIMCProperty))
            ImmAssociateContext(hWnd,hIMC);
        return TRUE;
    case IACE_CHILDREN:
        EnumChildWindows(hWnd,_ImmAssociateContextExEnumProc,(LPARAM)hIMC);
        return TRUE;
    default:
        FIXME("Unknown dwFlags 0x%x\n",dwFlags);
        return FALSE;
    }
}

/***********************************************************************
 *		ImmCreateContext (IMM32.@)
 */
HIMC WINAPI ImmCreateContext(void)
{
    PCLIENTIMC pClientImc;
    HIMC hIMC;

    TRACE("()\n");

    if (!IS_IME_ENABLED())
        return NULL;

    pClientImc = Imm32HeapAlloc(HEAP_ZERO_MEMORY, sizeof(CLIENTIMC));
    if (pClientImc == NULL)
        return NULL;

    hIMC = NtUserCreateInputContext(pClientImc);
    if (hIMC == NULL)
    {
        Imm32HeapFree(pClientImc);
        return NULL;
    }

    RtlInitializeCriticalSection(&pClientImc->cs);

    // FIXME: NtUserGetThreadState and enum ThreadStateRoutines are broken.
    pClientImc->unknown = NtUserGetThreadState(13);

    return hIMC;
}

static VOID APIENTRY Imm32CleanupContextExtra(LPINPUTCONTEXT pIC)
{
    FIXME("We have to do something do here");
}

static PCLIENTIMC APIENTRY Imm32FindClientImc(HIMC hIMC)
{
    // FIXME
    return NULL;
}

BOOL APIENTRY Imm32CleanupContext(HIMC hIMC, HKL hKL, BOOL bKeep)
{
    PIMEDPI pImeDpi;
    LPINPUTCONTEXT pIC;
    PCLIENTIMC pClientImc;

    if (g_psi == NULL || !(g_psi->dwSRVIFlags & SRVINFO_IMM32) || hIMC == NULL)
        return FALSE;

    FIXME("We have do something to do here\n");
    pClientImc = Imm32FindClientImc(hIMC);
    if (!pClientImc)
        return FALSE;

    if (pClientImc->hImc == NULL)
    {
        pClientImc->dwFlags |= CLIENTIMC_UNKNOWN1;
        ImmUnlockClientImc(pClientImc);
        if (!bKeep)
            return NtUserDestroyInputContext(hIMC);
        return TRUE;
    }

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
    {
        ImmUnlockClientImc(pClientImc);
        return FALSE;
    }

    FIXME("We have do something to do here\n");

    if (pClientImc->hKL == hKL)
    {
        pImeDpi = ImmLockImeDpi(hKL);
        if (pImeDpi != NULL)
        {
            if (IS_IME_HKL(hKL))
            {
                pImeDpi->ImeSelect(hIMC, FALSE);
            }
            else if (g_psi && (g_psi->dwSRVIFlags & SRVINFO_CICERO_ENABLED))
            {
                FIXME("We have do something to do here\n");
            }
            ImmUnlockImeDpi(pImeDpi);
        }
        pClientImc->hKL = NULL;
    }

    ImmDestroyIMCC(pIC->hPrivate);
    ImmDestroyIMCC(pIC->hMsgBuf);
    ImmDestroyIMCC(pIC->hGuideLine);
    ImmDestroyIMCC(pIC->hCandInfo);
    ImmDestroyIMCC(pIC->hCompStr);

    Imm32CleanupContextExtra(pIC);

    ImmUnlockIMC(hIMC);

    pClientImc->dwFlags |= CLIENTIMC_UNKNOWN1;
    ImmUnlockClientImc(pClientImc);

    if (!bKeep)
        return NtUserDestroyInputContext(hIMC);

    return TRUE;
}

/***********************************************************************
 *		ImmDestroyContext (IMM32.@)
 */
BOOL WINAPI ImmDestroyContext(HIMC hIMC)
{
    HKL hKL;

    TRACE("(%p)\n", hIMC);

    if (!IS_IME_ENABLED() || Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    hKL = GetKeyboardLayout(0);
    return Imm32CleanupContext(hIMC, hKL, FALSE);
}

static inline BOOL EscapeRequiresWA(UINT uEscape)
{
    if (uEscape == IME_ESC_GET_EUDC_DICTIONARY ||
        uEscape == IME_ESC_SET_EUDC_DICTIONARY ||
        uEscape == IME_ESC_IME_NAME ||
        uEscape == IME_ESC_GETHELPFILENAME)
    return TRUE;
    return FALSE;
}

/***********************************************************************
 *		ImmEscapeA (IMM32.@)
 */
LRESULT WINAPI ImmEscapeA(HKL hKL, HIMC hIMC, UINT uEscape, LPVOID lpData)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %p, %d, %p):\n", hKL, hIMC, uEscape, lpData);

    if (immHkl->hIME && immHkl->pImeEscape)
    {
        if (!EscapeRequiresWA(uEscape) || !is_kbd_ime_unicode(immHkl))
            return immHkl->pImeEscape(hIMC,uEscape,lpData);
        else
        {
            WCHAR buffer[81]; /* largest required buffer should be 80 */
            LRESULT rc;
            if (uEscape == IME_ESC_SET_EUDC_DICTIONARY)
            {
                MultiByteToWideChar(CP_ACP,0,lpData,-1,buffer,81);
                rc = immHkl->pImeEscape(hIMC,uEscape,buffer);
            }
            else
            {
                rc = immHkl->pImeEscape(hIMC,uEscape,buffer);
                WideCharToMultiByte(CP_ACP,0,buffer,-1,lpData,80, NULL, NULL);
            }
            return rc;
        }
    }
    else
        return 0;
}

/***********************************************************************
 *		ImmEscapeW (IMM32.@)
 */
LRESULT WINAPI ImmEscapeW(HKL hKL, HIMC hIMC, UINT uEscape, LPVOID lpData)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %p, %d, %p):\n", hKL, hIMC, uEscape, lpData);

    if (immHkl->hIME && immHkl->pImeEscape)
    {
        if (!EscapeRequiresWA(uEscape) || is_kbd_ime_unicode(immHkl))
            return immHkl->pImeEscape(hIMC,uEscape,lpData);
        else
        {
            CHAR buffer[81]; /* largest required buffer should be 80 */
            LRESULT rc;
            if (uEscape == IME_ESC_SET_EUDC_DICTIONARY)
            {
                WideCharToMultiByte(CP_ACP,0,lpData,-1,buffer,81, NULL, NULL);
                rc = immHkl->pImeEscape(hIMC,uEscape,buffer);
            }
            else
            {
                rc = immHkl->pImeEscape(hIMC,uEscape,buffer);
                MultiByteToWideChar(CP_ACP,0,buffer,-1,lpData,80);
            }
            return rc;
        }
    }
    else
        return 0;
}

static PCLIENTIMC APIENTRY Imm32GetClientImcCache(void)
{
    // FIXME: Do something properly here
    return NULL;
}

/***********************************************************************
 *		ImmLockClientImc (IMM32.@)
 */
PCLIENTIMC WINAPI ImmLockClientImc(HIMC hImc)
{
    PCLIENTIMC pClientImc;

    TRACE("(%p)\n", hImc);

    if (hImc == NULL)
        return NULL;

    pClientImc = Imm32GetClientImcCache();
    if (!pClientImc)
    {
        pClientImc = Imm32HeapAlloc(HEAP_ZERO_MEMORY, sizeof(CLIENTIMC));
        if (!pClientImc)
            return NULL;

        RtlInitializeCriticalSection(&pClientImc->cs);

        // FIXME: NtUserGetThreadState and enum ThreadStateRoutines are broken.
        pClientImc->unknown = NtUserGetThreadState(13);

        if (!NtUserUpdateInputContext(hImc, 0, pClientImc))
        {
            Imm32HeapFree(pClientImc);
            return NULL;
        }

        pClientImc->dwFlags |= CLIENTIMC_UNKNOWN2;
    }
    else
    {
        if (pClientImc->dwFlags & CLIENTIMC_UNKNOWN1)
            return NULL;
    }

    InterlockedIncrement(&pClientImc->cLockObj);
    return pClientImc;
}

/***********************************************************************
 *		ImmUnlockClientImc (IMM32.@)
 */
VOID WINAPI ImmUnlockClientImc(PCLIENTIMC pClientImc)
{
    LONG cLocks;

    TRACE("(%p)\n", pClientImc);

    cLocks = InterlockedDecrement(&pClientImc->cLockObj);
    if (cLocks != 0 || !(pClientImc->dwFlags & CLIENTIMC_UNKNOWN1))
        return;

    LocalFree(pClientImc->hImc);
    RtlDeleteCriticalSection(&pClientImc->cs);
    Imm32HeapFree(pClientImc);
}

static HIMC APIENTRY Imm32GetContextEx(HWND hWnd, DWORD dwContextFlags)
{
    HIMC hIMC;
    PCLIENTIMC pClientImc;
    PWND pWnd;

    if (!IS_IME_ENABLED())
        return NULL;

    if (!hWnd)
    {
        // FIXME: NtUserGetThreadState and enum ThreadStateRoutines are broken.
        hIMC = (HIMC)NtUserGetThreadState(4);
        goto Quit;
    }

    pWnd = ValidateHwndNoErr(hWnd);
    if (!pWnd || Imm32IsCrossProcessAccess(hWnd))
        return NULL;

    hIMC = pWnd->hImc;
    if (!hIMC && (dwContextFlags & 1))
        hIMC = (HIMC)NtUserQueryWindow(hWnd, QUERY_WINDOW_DEFAULT_ICONTEXT);

Quit:
    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return NULL;
    if ((dwContextFlags & 2) && (pClientImc->dwFlags & CLIENTIMC_UNKNOWN3))
        hIMC = NULL;
    ImmUnlockClientImc(pClientImc);
    return hIMC;
}

/***********************************************************************
 *		ImmGetCompositionFontA (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionFontA(HIMC hIMC, LPLOGFONTA lplf)
{
    PCLIENTIMC pClientImc;
    BOOL ret = FALSE, bWide;
    LPINPUTCONTEXT pIC;

    TRACE("(%p, %p)\n", hIMC, lplf);

    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return FALSE;

    bWide = (pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    if (pIC->fdwInit & INIT_LOGFONT)
    {
        if (bWide)
            LogFontWideToAnsi(&pIC->lfFont.W, lplf);
        else
            *lplf = pIC->lfFont.A;

        ret = TRUE;
    }

    ImmUnlockIMC(hIMC);
    return ret;
}

/***********************************************************************
 *		ImmGetCompositionFontW (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionFontW(HIMC hIMC, LPLOGFONTW lplf)
{
    PCLIENTIMC pClientImc;
    BOOL bWide;
    LPINPUTCONTEXT pIC;
    BOOL ret = FALSE;

    TRACE("(%p, %p)\n", hIMC, lplf);

    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return FALSE;

    bWide = (pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    if (pIC->fdwInit & INIT_LOGFONT)
    {
        if (bWide)
            *lplf = pIC->lfFont.W;
        else
            LogFontAnsiToWide(&pIC->lfFont.A, lplf);

        ret = TRUE;
    }

    ImmUnlockIMC(hIMC);
    return ret;
}


/* Helpers for the GetCompositionString functions */

/* Source encoding is defined by context, source length is always given in respective
   characters. Destination buffer length is always in bytes. */
static INT
CopyCompStringIMEtoClient(const InputContextData *data, LPCVOID src, INT src_len, LPVOID dst,
                          INT dst_len, BOOL unicode)
{
    int char_size = unicode ? sizeof(WCHAR) : sizeof(char);
    INT ret;

    if (is_himc_ime_unicode(data) ^ unicode)
    {
        if (unicode)
            ret = MultiByteToWideChar(CP_ACP, 0, src, src_len, dst, dst_len / sizeof(WCHAR));
        else
            ret = WideCharToMultiByte(CP_ACP, 0, src, src_len, dst, dst_len, NULL, NULL);
        ret *= char_size;
    }
    else
    {
        if (dst_len)
        {
            ret = min(src_len * char_size, dst_len);
            memcpy(dst, src, ret);
        }
        else
            ret = src_len * char_size;
    }

    return ret;
}

/* Composition string encoding is defined by context, returned attributes correspond to
   string, converted according to passed mode. String length is in characters, attributes
   are in byte arrays. */
static INT
CopyCompAttrIMEtoClient(const InputContextData *data, const BYTE *src, INT src_len,
                        LPCVOID comp_string, INT str_len, BYTE *dst, INT dst_len, BOOL unicode)
{
    union
    {
        const void *str;
        const WCHAR *strW;
        const char *strA;
    } string;
    INT rc;

    string.str = comp_string;

    if (is_himc_ime_unicode(data) && !unicode)
    {
        rc = WideCharToMultiByte(CP_ACP, 0, string.strW, str_len, NULL, 0, NULL, NULL);
        if (dst_len)
        {
            int i, j = 0, k = 0;

            if (rc < dst_len)
                dst_len = rc;
            for (i = 0; i < str_len; ++i)
            {
                int len;

                len = WideCharToMultiByte(CP_ACP, 0, string.strW + i, 1, NULL, 0, NULL, NULL);
                for (; len > 0; --len)
                {
                    dst[j++] = src[k];

                    if (j >= dst_len)
                        goto end;
                }
                ++k;
            }
        end:
            rc = j;
        }
    }
    else if (!is_himc_ime_unicode(data) && unicode)
    {
        rc = MultiByteToWideChar(CP_ACP, 0, string.strA, str_len, NULL, 0);
        if (dst_len)
        {
            int i, j = 0;

            if (rc < dst_len)
                dst_len = rc;
            for (i = 0; i < str_len; ++i)
            {
                if (IsDBCSLeadByte(string.strA[i]))
                    continue;

                dst[j++] = src[i];

                if (j >= dst_len)
                    break;
            }
            rc = j;
        }
    }
    else
    {
        memcpy(dst, src, min(src_len, dst_len));
        rc = src_len;
    }

    return rc;
}

static INT
CopyCompClauseIMEtoClient(InputContextData *data, LPBYTE source, INT slen, LPBYTE ssource,
                          LPBYTE target, INT tlen, BOOL unicode )
{
    INT rc;

    if (is_himc_ime_unicode(data) && !unicode)
    {
        if (tlen)
        {
            int i;

            if (slen < tlen)
                tlen = slen;
            tlen /= sizeof (DWORD);
            for (i = 0; i < tlen; ++i)
            {
                ((DWORD *)target)[i] = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)ssource,
                                                          ((DWORD *)source)[i],
                                                          NULL, 0,
                                                          NULL, NULL);
            }
            rc = sizeof (DWORD) * i;
        }
        else
            rc = slen;
    }
    else if (!is_himc_ime_unicode(data) && unicode)
    {
        if (tlen)
        {
            int i;

            if (slen < tlen)
                tlen = slen;
            tlen /= sizeof (DWORD);
            for (i = 0; i < tlen; ++i)
            {
                ((DWORD *)target)[i] = MultiByteToWideChar(CP_ACP, 0, (LPSTR)ssource,
                                                          ((DWORD *)source)[i],
                                                          NULL, 0);
            }
            rc = sizeof (DWORD) * i;
        }
        else
            rc = slen;
    }
    else
    {
        memcpy( target, source, min(slen,tlen));
        rc = slen;
    }

    return rc;
}

static INT
CopyCompOffsetIMEtoClient(InputContextData *data, DWORD offset, LPBYTE ssource, BOOL unicode)
{
    int rc;

    if (is_himc_ime_unicode(data) && !unicode)
    {
        rc = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)ssource, offset, NULL, 0, NULL, NULL);
    }
    else if (!is_himc_ime_unicode(data) && unicode)
    {
        rc = MultiByteToWideChar(CP_ACP, 0, (LPSTR)ssource, offset, NULL, 0);
    }
    else
        rc = offset;

    return rc;
}

static LONG
ImmGetCompositionStringT(HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen, BOOL unicode)
{
    LONG rc = 0;
    InputContextData *data = get_imc_data(hIMC);
    LPCOMPOSITIONSTRING compstr;
    LPBYTE compdata;

    TRACE("(%p, 0x%x, %p, %d)\n", hIMC, dwIndex, lpBuf, dwBufLen);

    if (!data)
       return FALSE;

    if (!data->IMC.hCompStr)
       return FALSE;

    compdata = ImmLockIMCC(data->IMC.hCompStr);
    compstr = (LPCOMPOSITIONSTRING)compdata;

    switch (dwIndex)
    {
    case GCS_RESULTSTR:
        TRACE("GCS_RESULTSTR\n");
        rc = CopyCompStringIMEtoClient(data, compdata + compstr->dwResultStrOffset, compstr->dwResultStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPSTR:
        TRACE("GCS_COMPSTR\n");
        rc = CopyCompStringIMEtoClient(data, compdata + compstr->dwCompStrOffset, compstr->dwCompStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPATTR:
        TRACE("GCS_COMPATTR\n");
        rc = CopyCompAttrIMEtoClient(data, compdata + compstr->dwCompAttrOffset, compstr->dwCompAttrLen,
                                     compdata + compstr->dwCompStrOffset, compstr->dwCompStrLen,
                                     lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPCLAUSE:
        TRACE("GCS_COMPCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(data, compdata + compstr->dwCompClauseOffset,compstr->dwCompClauseLen,
                                       compdata + compstr->dwCompStrOffset,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_RESULTCLAUSE:
        TRACE("GCS_RESULTCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(data, compdata + compstr->dwResultClauseOffset,compstr->dwResultClauseLen,
                                       compdata + compstr->dwResultStrOffset,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_RESULTREADSTR:
        TRACE("GCS_RESULTREADSTR\n");
        rc = CopyCompStringIMEtoClient(data, compdata + compstr->dwResultReadStrOffset, compstr->dwResultReadStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_RESULTREADCLAUSE:
        TRACE("GCS_RESULTREADCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(data, compdata + compstr->dwResultReadClauseOffset,compstr->dwResultReadClauseLen,
                                       compdata + compstr->dwResultStrOffset,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPREADSTR:
        TRACE("GCS_COMPREADSTR\n");
        rc = CopyCompStringIMEtoClient(data, compdata + compstr->dwCompReadStrOffset, compstr->dwCompReadStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPREADATTR:
        TRACE("GCS_COMPREADATTR\n");
        rc = CopyCompAttrIMEtoClient(data, compdata + compstr->dwCompReadAttrOffset, compstr->dwCompReadAttrLen,
                                     compdata + compstr->dwCompReadStrOffset, compstr->dwCompReadStrLen,
                                     lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPREADCLAUSE:
        TRACE("GCS_COMPREADCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(data, compdata + compstr->dwCompReadClauseOffset,compstr->dwCompReadClauseLen,
                                       compdata + compstr->dwCompStrOffset,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_CURSORPOS:
        TRACE("GCS_CURSORPOS\n");
        rc = CopyCompOffsetIMEtoClient(data, compstr->dwCursorPos, compdata + compstr->dwCompStrOffset, unicode);
        break;
    case GCS_DELTASTART:
        TRACE("GCS_DELTASTART\n");
        rc = CopyCompOffsetIMEtoClient(data, compstr->dwDeltaStart, compdata + compstr->dwCompStrOffset, unicode);
        break;
    default:
        FIXME("Unhandled index 0x%x\n",dwIndex);
        break;
    }

    ImmUnlockIMCC(data->IMC.hCompStr);

    return rc;
}

/***********************************************************************
 *		ImmGetCompositionStringA (IMM32.@)
 */
LONG WINAPI
ImmGetCompositionStringA(HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen)
{
    return ImmGetCompositionStringT(hIMC, dwIndex, lpBuf, dwBufLen, FALSE);
}

/***********************************************************************
 *		ImmGetCompositionStringW (IMM32.@)
 */
LONG WINAPI
ImmGetCompositionStringW(HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen)
{
    return ImmGetCompositionStringT(hIMC, dwIndex, lpBuf, dwBufLen, TRUE);
}

/***********************************************************************
 *		ImmGetCompositionWindow (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionWindow(HIMC hIMC, LPCOMPOSITIONFORM lpCompForm)
{
    LPINPUTCONTEXT pIC;
    BOOL ret = FALSE;

    TRACE("(%p, %p)\n", hIMC, lpCompForm);

    pIC = ImmLockIMC(hIMC);
    if (!pIC)
        return FALSE;

    if (pIC->fdwInit & INIT_COMPFORM)
    {
        *lpCompForm = pIC->cfCompForm;
        ret = TRUE;
    }

    ImmUnlockIMC(hIMC);
    return ret;
}

/***********************************************************************
 *		ImmGetContext (IMM32.@)
 */
HIMC WINAPI ImmGetContext(HWND hWnd)
{
    TRACE("(%p)\n", hWnd);
    if (hWnd == NULL)
        return NULL;
    return Imm32GetContextEx(hWnd, 2);
}

/***********************************************************************
 *		ImmGetConversionListA (IMM32.@)
 */
DWORD WINAPI
ImmGetConversionListA(HKL hKL, HIMC hIMC, LPCSTR pSrc, LPCANDIDATELIST lpDst,
                      DWORD dwBufLen, UINT uFlag)
{
    DWORD ret = 0;
    UINT cb;
    LPWSTR pszSrcW = NULL;
    LPCANDIDATELIST pCL = NULL;
    PIMEDPI pImeDpi;

    TRACE("(%p, %p, %s, %p, %lu, 0x%lX)\n", hKL, hIMC, debugstr_a(pSrc),
          lpDst, dwBufLen, uFlag);

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (pImeDpi == NULL)
        return 0;

    if (!(pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE))
    {
        ret = pImeDpi->ImeConversionList(hIMC, pSrc, lpDst, dwBufLen, uFlag);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (pSrc)
    {
        pszSrcW = Imm32WideFromAnsi(pSrc);
        if (pszSrcW == NULL)
            goto Quit;
    }

    cb = pImeDpi->ImeConversionList(hIMC, pszSrcW, NULL, 0, uFlag);
    if (cb == 0)
        goto Quit;

    pCL = Imm32HeapAlloc(0, cb);
    if (pCL == NULL)
        goto Quit;

    cb = pImeDpi->ImeConversionList(hIMC, pszSrcW, pCL, cb, uFlag);
    if (cb == 0)
        goto Quit;

    ret = CandidateListWideToAnsi(pCL, lpDst, dwBufLen, CP_ACP);

Quit:
    if (pszSrcW)
        Imm32HeapFree(pszSrcW);
    if (pCL)
        Imm32HeapFree(pCL);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmGetConversionListW (IMM32.@)
 */
DWORD WINAPI
ImmGetConversionListW(HKL hKL, HIMC hIMC, LPCWSTR pSrc, LPCANDIDATELIST lpDst,
                      DWORD dwBufLen, UINT uFlag)
{
    DWORD ret = 0;
    INT cb;
    PIMEDPI pImeDpi;
    LPCANDIDATELIST pCL = NULL;
    LPSTR pszSrcA = NULL;

    TRACE("(%p, %p, %s, %p, %lu, 0x%lX)\n", hKL, hIMC, debugstr_w(pSrc),
          lpDst, dwBufLen, uFlag);

    pImeDpi = ImmLockOrLoadImeDpi(hKL);
    if (!pImeDpi)
        return 0;

    if (pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE)
    {
        ret = pImeDpi->ImeConversionList(hIMC, pSrc, lpDst, dwBufLen, uFlag);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (pSrc)
    {
        pszSrcA = Imm32AnsiFromWide(pSrc);
        if (pszSrcA == NULL)
            goto Quit;
    }

    cb = pImeDpi->ImeConversionList(hIMC, pszSrcA, NULL, 0, uFlag);
    if (cb == 0)
        goto Quit;

    pCL = Imm32HeapAlloc(0, cb);
    if (!pCL)
        goto Quit;

    cb = pImeDpi->ImeConversionList(hIMC, pszSrcA, pCL, cb, uFlag);
    if (!cb)
        goto Quit;

    ret = CandidateListAnsiToWide(pCL, lpDst, dwBufLen, CP_ACP);

Quit:
    if (pszSrcA)
        Imm32HeapFree(pszSrcA);
    if (pCL)
        Imm32HeapFree(pCL);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmGetConversionStatus (IMM32.@)
 */
BOOL WINAPI ImmGetConversionStatus(HIMC hIMC, LPDWORD lpfdwConversion, LPDWORD lpfdwSentence)
{
    LPINPUTCONTEXT pIC;

    TRACE("(%p %p %p)\n", hIMC, lpfdwConversion, lpfdwSentence);

    pIC = ImmLockIMC(hIMC);
    if (!pIC)
        return FALSE;

    if (lpfdwConversion)
        *lpfdwConversion = pIC->fdwConversion;
    if (lpfdwSentence)
        *lpfdwSentence = pIC->fdwSentence;

    ImmUnlockIMC(hIMC);
    return TRUE;
}

/***********************************************************************
 *		CtfImmIsCiceroEnabled (IMM32.@)
 */
BOOL WINAPI CtfImmIsCiceroEnabled(VOID)
{
    return IS_CICERO_ENABLED();
}

/***********************************************************************
 *		ImmGetVirtualKey (IMM32.@)
 */
UINT WINAPI ImmGetVirtualKey(HWND hWnd)
{
    HIMC hIMC;
    LPINPUTCONTEXTDX pIC;
    UINT ret = VK_PROCESSKEY;

    TRACE("(%p)\n", hWnd);

    hIMC = ImmGetContext(hWnd);
    pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (!pIC)
        return ret;

    if (pIC->bNeedsTrans)
        ret = pIC->nVKey;

    ImmUnlockIMC(hIMC);
    return ret;
}

/***********************************************************************
 *		ImmInstallIMEA (IMM32.@)
 */
HKL WINAPI ImmInstallIMEA(LPCSTR lpszIMEFileName, LPCSTR lpszLayoutText)
{
    HKL hKL = NULL;
    LPWSTR pszFileNameW = NULL, pszLayoutTextW = NULL;

    TRACE("(%s, %s)\n", debugstr_a(lpszIMEFileName), debugstr_a(lpszLayoutText));

    pszFileNameW = Imm32WideFromAnsi(lpszIMEFileName);
    if (pszFileNameW == NULL)
        goto Quit;

    pszLayoutTextW = Imm32WideFromAnsi(lpszLayoutText);
    if (pszLayoutTextW == NULL)
        goto Quit;

    hKL = ImmInstallIMEW(pszFileNameW, pszLayoutTextW);

Quit:
    if (pszFileNameW)
        Imm32HeapFree(pszFileNameW);
    if (pszLayoutTextW)
        Imm32HeapFree(pszLayoutTextW);
    return hKL;
}

/***********************************************************************
 *		ImmInstallIMEW (IMM32.@)
 */
HKL WINAPI ImmInstallIMEW(LPCWSTR lpszIMEFileName, LPCWSTR lpszLayoutText)
{
    INT lcid = GetUserDefaultLCID();
    INT count;
    HKL hkl;
    DWORD rc;
    HKEY hkey;
    WCHAR regKey[ARRAY_SIZE(szImeRegFmt)+8];

    TRACE ("(%s, %s):\n", debugstr_w(lpszIMEFileName),
                          debugstr_w(lpszLayoutText));

    /* Start with 2.  e001 will be blank and so default to the wine internal IME */
    count = 2;

    while (count < 0xfff)
    {
        DWORD disposition = 0;

        hkl = (HKL)MAKELPARAM( lcid, 0xe000 | count );
        wsprintfW( regKey, szImeRegFmt, (ULONG_PTR)hkl);

        rc = RegCreateKeyExW(HKEY_LOCAL_MACHINE, regKey, 0, NULL, 0, KEY_WRITE, NULL, &hkey, &disposition);
        if (rc == ERROR_SUCCESS && disposition == REG_CREATED_NEW_KEY)
            break;
        else if (rc == ERROR_SUCCESS)
            RegCloseKey(hkey);

        count++;
    }

    if (count == 0xfff)
    {
        WARN("Unable to find slot to install IME\n");
        return 0;
    }

    if (rc == ERROR_SUCCESS)
    {
        rc = RegSetValueExW(hkey, szImeFileW, 0, REG_SZ, (const BYTE*)lpszIMEFileName,
                            (lstrlenW(lpszIMEFileName) + 1) * sizeof(WCHAR));
        if (rc == ERROR_SUCCESS)
            rc = RegSetValueExW(hkey, szLayoutTextW, 0, REG_SZ, (const BYTE*)lpszLayoutText,
                                (lstrlenW(lpszLayoutText) + 1) * sizeof(WCHAR));
        RegCloseKey(hkey);
        return hkl;
    }
    else
    {
        WARN("Unable to set IME registry values\n");
        return 0;
    }
}

/***********************************************************************
 *		ImmReleaseContext (IMM32.@)
 */
BOOL WINAPI ImmReleaseContext(HWND hWnd, HIMC hIMC)
{
    TRACE("(%p, %p)\n", hWnd, hIMC);
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(hIMC);
    return TRUE; // Do nothing. This is correct.
}

/***********************************************************************
 *		ImmSetCandidateWindow (IMM32.@)
 */
BOOL WINAPI ImmSetCandidateWindow(HIMC hIMC, LPCANDIDATEFORM lpCandidate)
{
    HWND hWnd;
    LPINPUTCONTEXT pIC;

    TRACE("(%p, %p)\n", hIMC, lpCandidate);

    if (lpCandidate->dwIndex >= MAX_CANDIDATEFORM)
        return FALSE;

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    hWnd = pIC->hWnd;
    pIC->cfCandForm[lpCandidate->dwIndex] = *lpCandidate;

    ImmUnlockIMC(hIMC);

    Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, 0, IMC_SETCANDIDATEPOS,
                      IMN_SETCANDIDATEPOS, (1 << (BYTE)lpCandidate->dwIndex));
    return TRUE;
}

/***********************************************************************
 *		ImmSetCompositionFontA (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionFontA(HIMC hIMC, LPLOGFONTA lplf)
{
    LOGFONTW lfW;
    PCLIENTIMC pClientImc;
    BOOL bWide;
    LPINPUTCONTEXTDX pIC;
    LCID lcid;
    HWND hWnd;
    PTEB pTeb;

    TRACE("(%p, %p)\n", hIMC, lplf);

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return FALSE;

    bWide = (pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    if (bWide)
    {
        LogFontAnsiToWide(lplf, &lfW);
        return ImmSetCompositionFontW(hIMC, &lfW);
    }

    pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    pTeb = NtCurrentTeb();
    if (pTeb->Win32ClientInfo[2] < 0x400)
    {
        lcid = GetSystemDefaultLCID();
        if (PRIMARYLANGID(lcid) == LANG_JAPANESE && !(pIC->dwUIFlags & 2) &&
            pIC->cfCompForm.dwStyle != CFS_DEFAULT)
        {
            PostMessageA(pIC->hWnd, WM_IME_REPORT, IR_CHANGECONVERT, 0);
        }
    }

    pIC->lfFont.A = *lplf;
    pIC->fdwInit |= INIT_LOGFONT;
    hWnd = pIC->hWnd;

    ImmUnlockIMC(hIMC);

    Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, 0, IMC_SETCOMPOSITIONFONT,
                      IMN_SETCOMPOSITIONFONT, 0);
    return TRUE;
}

/***********************************************************************
 *		ImmSetCompositionFontW (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionFontW(HIMC hIMC, LPLOGFONTW lplf)
{
    LOGFONTA lfA;
    PCLIENTIMC pClientImc;
    BOOL bWide;
    HWND hWnd;
    LPINPUTCONTEXTDX pIC;
    PTEB pTeb;
    LCID lcid;

    TRACE("(%p, %p)\n", hIMC, lplf);

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return FALSE;

    bWide = (pClientImc->dwFlags & CLIENTIMC_WIDE);
    ImmUnlockClientImc(pClientImc);

    if (!bWide)
    {
        LogFontWideToAnsi(lplf, &lfA);
        return ImmSetCompositionFontA(hIMC, &lfA);
    }

    pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    pTeb = NtCurrentTeb();
    if (pTeb->Win32ClientInfo[2] < 0x400)
    {
        lcid = GetSystemDefaultLCID();
        if (PRIMARYLANGID(lcid) == LANG_JAPANESE &&
            !(pIC->dwUIFlags & 2) &&
            pIC->cfCompForm.dwStyle != CFS_DEFAULT)
        {
            PostMessageW(pIC->hWnd, WM_IME_REPORT, IR_CHANGECONVERT, 0);
        }
    }

    pIC->lfFont.W = *lplf;
    pIC->fdwInit |= INIT_LOGFONT;
    hWnd = pIC->hWnd;

    ImmUnlockIMC(hIMC);

    Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, 0, IMC_SETCOMPOSITIONFONT,
                      IMN_SETCOMPOSITIONFONT, 0);
    return TRUE;
}

/***********************************************************************
 *		ImmSetCompositionStringA (IMM32.@)
 */
BOOL WINAPI
ImmSetCompositionStringA(HIMC hIMC, DWORD dwIndex, LPCVOID lpComp, DWORD dwCompLen,
                         LPCVOID lpRead, DWORD dwReadLen)
{
    DWORD comp_len;
    DWORD read_len;
    WCHAR *CompBuffer = NULL;
    WCHAR *ReadBuffer = NULL;
    BOOL rc;
    InputContextData *data = get_imc_data(hIMC);

    TRACE("(%p, %d, %p, %d, %p, %d):\n",
            hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);

    if (!data)
        return FALSE;

    if (!(dwIndex == SCS_SETSTR ||
          dwIndex == SCS_CHANGEATTR ||
          dwIndex == SCS_CHANGECLAUSE ||
          dwIndex == SCS_SETRECONVERTSTRING ||
          dwIndex == SCS_QUERYRECONVERTSTRING))
        return FALSE;

    if (!is_himc_ime_unicode(data))
        return data->immKbd->pImeSetCompositionString(hIMC, dwIndex, lpComp,
                        dwCompLen, lpRead, dwReadLen);

    comp_len = MultiByteToWideChar(CP_ACP, 0, lpComp, dwCompLen, NULL, 0);
    if (comp_len)
    {
        CompBuffer = HeapAlloc(GetProcessHeap(),0,comp_len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpComp, dwCompLen, CompBuffer, comp_len);
    }

    read_len = MultiByteToWideChar(CP_ACP, 0, lpRead, dwReadLen, NULL, 0);
    if (read_len)
    {
        ReadBuffer = HeapAlloc(GetProcessHeap(),0,read_len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpRead, dwReadLen, ReadBuffer, read_len);
    }

    rc =  ImmSetCompositionStringW(hIMC, dwIndex, CompBuffer, comp_len,
                                   ReadBuffer, read_len);

    HeapFree(GetProcessHeap(), 0, CompBuffer);
    HeapFree(GetProcessHeap(), 0, ReadBuffer);

    return rc;
}

/***********************************************************************
 *		ImmSetCompositionStringW (IMM32.@)
 */
BOOL WINAPI
ImmSetCompositionStringW(HIMC hIMC, DWORD dwIndex, LPCVOID lpComp, DWORD dwCompLen,
                         LPCVOID lpRead, DWORD dwReadLen)
{
    DWORD comp_len;
    DWORD read_len;
    CHAR *CompBuffer = NULL;
    CHAR *ReadBuffer = NULL;
    BOOL rc;
    InputContextData *data = get_imc_data(hIMC);

    TRACE("(%p, %d, %p, %d, %p, %d):\n",
            hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);

    if (!data)
        return FALSE;

    if (!(dwIndex == SCS_SETSTR ||
          dwIndex == SCS_CHANGEATTR ||
          dwIndex == SCS_CHANGECLAUSE ||
          dwIndex == SCS_SETRECONVERTSTRING ||
          dwIndex == SCS_QUERYRECONVERTSTRING))
        return FALSE;

    if (is_himc_ime_unicode(data))
        return data->immKbd->pImeSetCompositionString(hIMC, dwIndex, lpComp,
                        dwCompLen, lpRead, dwReadLen);

    comp_len = WideCharToMultiByte(CP_ACP, 0, lpComp, dwCompLen, NULL, 0, NULL,
                                   NULL);
    if (comp_len)
    {
        CompBuffer = HeapAlloc(GetProcessHeap(),0,comp_len);
        WideCharToMultiByte(CP_ACP, 0, lpComp, dwCompLen, CompBuffer, comp_len,
                            NULL, NULL);
    }

    read_len = WideCharToMultiByte(CP_ACP, 0, lpRead, dwReadLen, NULL, 0, NULL,
                                   NULL);
    if (read_len)
    {
        ReadBuffer = HeapAlloc(GetProcessHeap(),0,read_len);
        WideCharToMultiByte(CP_ACP, 0, lpRead, dwReadLen, ReadBuffer, read_len,
                            NULL, NULL);
    }

    rc =  ImmSetCompositionStringA(hIMC, dwIndex, CompBuffer, comp_len,
                                   ReadBuffer, read_len);

    HeapFree(GetProcessHeap(), 0, CompBuffer);
    HeapFree(GetProcessHeap(), 0, ReadBuffer);

    return rc;
}

/***********************************************************************
 *		ImmSetCompositionWindow (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionWindow(HIMC hIMC, LPCOMPOSITIONFORM lpCompForm)
{
    LPINPUTCONTEXT pIC;
    HWND hWnd;

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    pIC->cfCompForm = *lpCompForm;
    pIC->fdwInit |= INIT_COMPFORM;

    hWnd = pIC->hWnd;

    ImmUnlockIMC(hIMC);

    Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, 0,
                      IMC_SETCOMPOSITIONWINDOW, IMN_SETCOMPOSITIONWINDOW, 0);
    return TRUE;
}

/***********************************************************************
 *		ImmSetConversionStatus (IMM32.@)
 */
BOOL WINAPI ImmSetConversionStatus(HIMC hIMC, DWORD fdwConversion, DWORD fdwSentence)
{
    HKL hKL;
    LPINPUTCONTEXT pIC;
    DWORD dwOldConversion, dwOldSentence;
    BOOL fConversionChange = FALSE, fSentenceChange = FALSE;
    HWND hWnd;

    TRACE("(%p, 0x%lX, 0x%lX)\n", hIMC, fdwConversion, fdwSentence);

    hKL = GetKeyboardLayout(0);
    if (!IS_IME_HKL(hKL) && IS_CICERO_ENABLED())
    {
        FIXME("Cicero\n");
        return FALSE;
    }

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    if (pIC->fdwConversion != fdwConversion)
    {
        dwOldConversion = pIC->fdwConversion;
        pIC->fdwConversion = fdwConversion;
        fConversionChange = TRUE;
    }

    if (pIC->fdwSentence != fdwSentence)
    {
        dwOldSentence = pIC->fdwSentence;
        pIC->fdwSentence = fdwSentence;
        fSentenceChange = TRUE;
    }

    hWnd = pIC->hWnd;
    ImmUnlockIMC(hIMC);

    if (fConversionChange)
    {
        Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, dwOldConversion,
                          IMC_SETCONVERSIONMODE, IMN_SETCONVERSIONMODE, 0);
        NtUserNotifyIMEStatus(hWnd, hIMC, fdwConversion);
    }

    if (fSentenceChange)
    {
        Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, dwOldSentence,
                          IMC_SETSENTENCEMODE, IMN_SETSENTENCEMODE, 0);
    }

    return TRUE;
}

/***********************************************************************
 *		ImmSetOpenStatus (IMM32.@)
 */
BOOL WINAPI ImmSetOpenStatus(HIMC hIMC, BOOL fOpen)
{
    DWORD dwConversion;
    LPINPUTCONTEXT pIC;
    HWND hWnd;
    BOOL bHasChange = FALSE;

    TRACE("(%p, %d)\n", hIMC, fOpen);

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (pIC == NULL)
        return FALSE;

    if (pIC->fOpen != fOpen)
    {
        pIC->fOpen = fOpen;
        hWnd = pIC->hWnd;
        dwConversion = pIC->fdwConversion;
        bHasChange = TRUE;
    }

    ImmUnlockIMC(hIMC);

    if (bHasChange)
    {
        Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, 0,
                          IMC_SETOPENSTATUS, IMN_SETOPENSTATUS, 0);
        NtUserNotifyIMEStatus(hWnd, hIMC, dwConversion);
    }

    return TRUE;
}

/***********************************************************************
 *		ImmSetStatusWindowPos (IMM32.@)
 */
BOOL WINAPI ImmSetStatusWindowPos(HIMC hIMC, LPPOINT lpptPos)
{
    LPINPUTCONTEXT pIC;
    HWND hWnd;

    TRACE("(%p, {%ld, %ld})\n", hIMC, lpptPos->x, lpptPos->y);

    if (Imm32IsCrossThreadAccess(hIMC))
        return FALSE;

    pIC = ImmLockIMC(hIMC);
    if (!pIC)
        return FALSE;

    hWnd = pIC->hWnd;
    pIC->ptStatusWndPos = *lpptPos;
    pIC->fdwInit |= INIT_STATUSWNDPOS;

    ImmUnlockIMC(hIMC);

    Imm32NotifyAction(hIMC, hWnd, NI_CONTEXTUPDATED, 0,
                      IMC_SETSTATUSWINDOWPOS, IMN_SETSTATUSWINDOWPOS, 0);
    return TRUE;
}

/***********************************************************************
 *              ImmCreateSoftKeyboard(IMM32.@)
 */
HWND WINAPI ImmCreateSoftKeyboard(UINT uType, UINT hOwner, int x, int y)
{
    FIXME("(%d, %d, %d, %d): stub\n", uType, hOwner, x, y);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/***********************************************************************
 *              ImmDestroySoftKeyboard(IMM32.@)
 */
BOOL WINAPI ImmDestroySoftKeyboard(HWND hSoftWnd)
{
    TRACE("(%p)\n", hSoftWnd);
    return DestroyWindow(hSoftWnd);
}

/***********************************************************************
 *              ImmShowSoftKeyboard(IMM32.@)
 */
BOOL WINAPI ImmShowSoftKeyboard(HWND hSoftWnd, int nCmdShow)
{
    TRACE("(%p, %d)\n", hSoftWnd, nCmdShow);
    if (hSoftWnd)
        return ShowWindow(hSoftWnd, nCmdShow);
    return FALSE;
}

/***********************************************************************
 *		ImmGetImeMenuItemsA (IMM32.@)
 */
DWORD WINAPI
ImmGetImeMenuItemsA(HIMC hIMC, DWORD dwFlags, DWORD dwType,
                    LPIMEMENUITEMINFOA lpImeParentMenu, LPIMEMENUITEMINFOA lpImeMenu,
                    DWORD dwSize)
{
    InputContextData *data = get_imc_data(hIMC);
    TRACE("(%p, %i, %i, %p, %p, %i):\n", hIMC, dwFlags, dwType,
        lpImeParentMenu, lpImeMenu, dwSize);

    if (!data)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    if (data->immKbd->hIME && data->immKbd->pImeGetImeMenuItems)
    {
        if (!is_himc_ime_unicode(data) || (!lpImeParentMenu && !lpImeMenu))
            return data->immKbd->pImeGetImeMenuItems(hIMC, dwFlags, dwType,
                                (IMEMENUITEMINFOW*)lpImeParentMenu,
                                (IMEMENUITEMINFOW*)lpImeMenu, dwSize);
        else
        {
            IMEMENUITEMINFOW lpImeParentMenuW;
            IMEMENUITEMINFOW *lpImeMenuW, *parent = NULL;
            DWORD rc;

            if (lpImeParentMenu)
                parent = &lpImeParentMenuW;
            if (lpImeMenu)
            {
                int count = dwSize / sizeof(LPIMEMENUITEMINFOA);
                dwSize = count * sizeof(IMEMENUITEMINFOW);
                lpImeMenuW = HeapAlloc(GetProcessHeap(), 0, dwSize);
            }
            else
                lpImeMenuW = NULL;

            rc = data->immKbd->pImeGetImeMenuItems(hIMC, dwFlags, dwType,
                                parent, lpImeMenuW, dwSize);

            if (lpImeParentMenu)
            {
                memcpy(lpImeParentMenu,&lpImeParentMenuW,sizeof(IMEMENUITEMINFOA));
                lpImeParentMenu->hbmpItem = lpImeParentMenuW.hbmpItem;
                WideCharToMultiByte(CP_ACP, 0, lpImeParentMenuW.szString,
                    -1, lpImeParentMenu->szString, IMEMENUITEM_STRING_SIZE,
                    NULL, NULL);
            }
            if (lpImeMenu && rc)
            {
                unsigned int i;
                for (i = 0; i < rc; i++)
                {
                    memcpy(&lpImeMenu[i],&lpImeMenuW[1],sizeof(IMEMENUITEMINFOA));
                    lpImeMenu[i].hbmpItem = lpImeMenuW[i].hbmpItem;
                    WideCharToMultiByte(CP_ACP, 0, lpImeMenuW[i].szString,
                        -1, lpImeMenu[i].szString, IMEMENUITEM_STRING_SIZE,
                        NULL, NULL);
                }
            }
            HeapFree(GetProcessHeap(),0,lpImeMenuW);
            return rc;
        }
    }
    else
        return 0;
}

/***********************************************************************
 *		ImmGetImeMenuItemsW (IMM32.@)
 */
DWORD WINAPI
ImmGetImeMenuItemsW(HIMC hIMC, DWORD dwFlags, DWORD dwType,
                    LPIMEMENUITEMINFOW lpImeParentMenu, LPIMEMENUITEMINFOW lpImeMenu,
                    DWORD dwSize)
{
    InputContextData *data = get_imc_data(hIMC);
    TRACE("(%p, %i, %i, %p, %p, %i):\n", hIMC, dwFlags, dwType,
        lpImeParentMenu, lpImeMenu, dwSize);

    if (!data)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    if (data->immKbd->hIME && data->immKbd->pImeGetImeMenuItems)
    {
        if (is_himc_ime_unicode(data) || (!lpImeParentMenu && !lpImeMenu))
            return data->immKbd->pImeGetImeMenuItems(hIMC, dwFlags, dwType,
                                lpImeParentMenu, lpImeMenu, dwSize);
        else
        {
            IMEMENUITEMINFOA lpImeParentMenuA;
            IMEMENUITEMINFOA *lpImeMenuA, *parent = NULL;
            DWORD rc;

            if (lpImeParentMenu)
                parent = &lpImeParentMenuA;
            if (lpImeMenu)
            {
                int count = dwSize / sizeof(LPIMEMENUITEMINFOW);
                dwSize = count * sizeof(IMEMENUITEMINFOA);
                lpImeMenuA = HeapAlloc(GetProcessHeap(), 0, dwSize);
            }
            else
                lpImeMenuA = NULL;

            rc = data->immKbd->pImeGetImeMenuItems(hIMC, dwFlags, dwType,
                                (IMEMENUITEMINFOW*)parent,
                                (IMEMENUITEMINFOW*)lpImeMenuA, dwSize);

            if (lpImeParentMenu)
            {
                memcpy(lpImeParentMenu,&lpImeParentMenuA,sizeof(IMEMENUITEMINFOA));
                lpImeParentMenu->hbmpItem = lpImeParentMenuA.hbmpItem;
                MultiByteToWideChar(CP_ACP, 0, lpImeParentMenuA.szString,
                    -1, lpImeParentMenu->szString, IMEMENUITEM_STRING_SIZE);
            }
            if (lpImeMenu && rc)
            {
                unsigned int i;
                for (i = 0; i < rc; i++)
                {
                    memcpy(&lpImeMenu[i],&lpImeMenuA[1],sizeof(IMEMENUITEMINFOA));
                    lpImeMenu[i].hbmpItem = lpImeMenuA[i].hbmpItem;
                    MultiByteToWideChar(CP_ACP, 0, lpImeMenuA[i].szString,
                        -1, lpImeMenu[i].szString, IMEMENUITEM_STRING_SIZE);
                }
            }
            HeapFree(GetProcessHeap(),0,lpImeMenuA);
            return rc;
        }
    }
    else
        return 0;
}

/***********************************************************************
 *		ImmLockIMC(IMM32.@)
 */
LPINPUTCONTEXT WINAPI ImmLockIMC(HIMC hIMC)
{
    InputContextData *data = get_imc_data(hIMC);

    if (!data)
        return NULL;
    data->dwLock++;
    return &data->IMC;
}

/***********************************************************************
 *		ImmUnlockIMC(IMM32.@)
 */
BOOL WINAPI ImmUnlockIMC(HIMC hIMC)
{
    PCLIENTIMC pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return FALSE;

    if (pClientImc->hImc)
        LocalUnlock(pClientImc->hImc);

    InterlockedDecrement(&pClientImc->cLockObj);
    ImmUnlockClientImc(pClientImc);
    return TRUE;
}

/***********************************************************************
 *		ImmGetIMCLockCount(IMM32.@)
 */
DWORD WINAPI ImmGetIMCLockCount(HIMC hIMC)
{
    DWORD ret = 0;
    PCLIENTIMC pClientImc;

    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc == NULL)
        return 0;

    if (pClientImc->hImc)
        ret = (LocalFlags(pClientImc->hImc) & LMEM_LOCKCOUNT);

    ImmUnlockClientImc(pClientImc);
    return ret;
}

/***********************************************************************
 *		ImmCreateIMCC(IMM32.@)
 */
HIMCC  WINAPI ImmCreateIMCC(DWORD size)
{
    if (size < 4)
        size = 4;
    return LocalAlloc(LHND, size);
}

/***********************************************************************
 *       ImmDestroyIMCC(IMM32.@)
 */
HIMCC WINAPI ImmDestroyIMCC(HIMCC block)
{
    if (block)
        return LocalFree(block);
    return NULL;
}

/***********************************************************************
 *		ImmLockIMCC(IMM32.@)
 */
LPVOID WINAPI ImmLockIMCC(HIMCC imcc)
{
    if (imcc)
        return LocalLock(imcc);
    return NULL;
}

/***********************************************************************
 *		ImmUnlockIMCC(IMM32.@)
 */
BOOL WINAPI ImmUnlockIMCC(HIMCC imcc)
{
    if (imcc)
        return LocalUnlock(imcc);
    return FALSE;
}

/***********************************************************************
 *		ImmGetIMCCLockCount(IMM32.@)
 */
DWORD WINAPI ImmGetIMCCLockCount(HIMCC imcc)
{
    return LocalFlags(imcc) & LMEM_LOCKCOUNT;
}

/***********************************************************************
 *		ImmReSizeIMCC(IMM32.@)
 */
HIMCC  WINAPI ImmReSizeIMCC(HIMCC imcc, DWORD size)
{
    if (!imcc)
        return NULL;
    return LocalReAlloc(imcc, size, LHND);
}

/***********************************************************************
 *		ImmGetIMCCSize(IMM32.@)
 */
DWORD WINAPI ImmGetIMCCSize(HIMCC imcc)
{
    if (imcc)
        return LocalSize(imcc);
    return 0;
}

/***********************************************************************
 *		ImmDisableTextFrameService(IMM32.@)
 */
BOOL WINAPI ImmDisableTextFrameService(DWORD dwThreadId)
{
    FIXME("Stub\n");
    return FALSE;
}

/***********************************************************************
 *              ImmEnumInputContext(IMM32.@)
 */
BOOL WINAPI ImmEnumInputContext(DWORD dwThreadId, IMCENUMPROC lpfn, LPARAM lParam)
{
    HIMC *phList;
    DWORD dwIndex, dwCount;
    BOOL ret = TRUE;
    HIMC hIMC;

    TRACE("(%lu, %p, %p)\n", dwThreadId, lpfn, lParam);

    dwCount = Imm32AllocAndBuildHimcList(dwThreadId, &phList);
    if (!dwCount)
        return FALSE;

    for (dwIndex = 0; dwIndex < dwCount; ++dwIndex)
    {
        hIMC = phList[dwIndex];
        ret = (*lpfn)(hIMC, lParam);
        if (!ret)
            break;
    }

    Imm32HeapFree(phList);
    return ret;
}

/***********************************************************************
 *              ImmDisableLegacyIME(IMM32.@)
 */
BOOL WINAPI ImmDisableLegacyIME(void)
{
    FIXME("stub\n");
    return TRUE;
}

/***********************************************************************
 *              ImmSetActiveContext(IMM32.@)
 */
BOOL WINAPI ImmSetActiveContext(HWND hwnd, HIMC hIMC, BOOL fFlag)
{
    FIXME("(%p, %p, %d): stub\n", hwnd, hIMC, fFlag);
    return FALSE;
}

/***********************************************************************
 *              ImmSetActiveContextConsoleIME(IMM32.@)
 */
BOOL WINAPI ImmSetActiveContextConsoleIME(HWND hwnd, BOOL fFlag)
{
    HIMC hIMC;
    TRACE("(%p, %d)\n", hwnd, fFlag);

    hIMC = ImmGetContext(hwnd);
    if (hIMC)
        return ImmSetActiveContext(hwnd, hIMC, fFlag);
    return FALSE;
}

/***********************************************************************
 *		CtfImmIsTextFrameServiceDisabled(IMM32.@)
 */
BOOL WINAPI CtfImmIsTextFrameServiceDisabled(VOID)
{
    PTEB pTeb = NtCurrentTeb();
    if (((PW32CLIENTINFO)pTeb->Win32ClientInfo)->CI_flags & CI_TFSDISABLED)
        return TRUE;
    return FALSE;
}

BOOL WINAPI User32InitializeImmEntryTable(DWORD);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    HKL hKL;
    HIMC hIMC;
    PTEB pTeb;

    TRACE("(%p, 0x%X, %p)\n", hinstDLL, fdwReason, lpReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            //Imm32GenerateRandomSeed(hinstDLL, 1, lpReserved); // Non-sense
            if (!Imm32InitInstance(hinstDLL))
            {
                ERR("Imm32InitInstance failed\n");
                return FALSE;
            }
            if (!User32InitializeImmEntryTable(IMM_INIT_MAGIC))
            {
                ERR("User32InitializeImmEntryTable failed\n");
                return FALSE;
            }
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            if (!IS_IME_ENABLED())
                return TRUE;

            pTeb = NtCurrentTeb();
            if (pTeb->Win32ThreadInfo == NULL)
                return TRUE;

            hKL = GetKeyboardLayout(0);
            // FIXME: NtUserGetThreadState and enum ThreadStateRoutines are broken.
            hIMC = (HIMC)NtUserGetThreadState(4);
            Imm32CleanupContext(hIMC, hKL, TRUE);
            break;

        case DLL_PROCESS_DETACH:
            RtlDeleteCriticalSection(&g_csImeDpi);
            TRACE("imm32.dll is unloaded\n");
            break;
    }

    return TRUE;
}
