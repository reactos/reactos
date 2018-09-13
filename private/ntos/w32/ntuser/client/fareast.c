/**************************************************************************\
* Module Name: fareast.c
*
* Win32 IMM/IME API functions
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* History:
* 07-May-1996 takaok    Created.
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define COMMON_RETURN_ZERO  \
    return 0;

////////////////////////
// Fake routines
////////////////////////

VOID fakeImm_v1(PVOID dummy)
{
    UNREFERENCED_PARAMETER(dummy);
}

DWORD fakeImm_d1(DWORD dummy)
{
    UNREFERENCED_PARAMETER(dummy);
    return 0;
}

VOID fakeImm_v2(PVOID dummy1, PVOID dummy2)
{
    UNREFERENCED_PARAMETER(dummy1);
    UNREFERENCED_PARAMETER(dummy2);
}

DWORD fakeImm_d2(DWORD dummy1, DWORD dummy2)
{
    UNREFERENCED_PARAMETER(dummy1);
    UNREFERENCED_PARAMETER(dummy2);
    return 0;
}

DWORD fakeImm_d3(DWORD dummy1, DWORD dummy2, DWORD dummy3)
{
    UNREFERENCED_PARAMETER(dummy1);
    UNREFERENCED_PARAMETER(dummy2);
    UNREFERENCED_PARAMETER(dummy3);
    return 0;
}

DWORD fakeImm_bwuwl(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(msg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    return 0;
}


VOID WINAPI fakeImm_wv1(PVOID dummy)
{
    UNREFERENCED_PARAMETER(dummy);
}

DWORD WINAPI fakeImm_wd1(PVOID dummy)
{
    UNREFERENCED_PARAMETER(dummy);
    COMMON_RETURN_ZERO;
}

DWORD WINAPI fakeImm_wd2(PVOID dummy1, PVOID dummy2)
{
    UNREFERENCED_PARAMETER(dummy1);
    UNREFERENCED_PARAMETER(dummy2);
    COMMON_RETURN_ZERO;
}

//
// This stub returns true.
//
BOOL WINAPI fakeImm_bd2(PVOID dummy1, PVOID dummy2)
{
    UNREFERENCED_PARAMETER(dummy1);
    UNREFERENCED_PARAMETER(dummy2);
    return TRUE;
}

DWORD WINAPI fakeImm_wd3(PVOID dummy1, PVOID dummy2, PVOID dummy3)
{
    UNREFERENCED_PARAMETER(dummy1);
    UNREFERENCED_PARAMETER(dummy2);
    UNREFERENCED_PARAMETER(dummy3);

    COMMON_RETURN_ZERO;
}

DWORD WINAPI fakeImm_wd4(PVOID dummy1, PVOID dummy2, PVOID dummy3, PVOID dummy4)
{
    UNREFERENCED_PARAMETER(dummy1);
    UNREFERENCED_PARAMETER(dummy2);
    UNREFERENCED_PARAMETER(dummy3);
    UNREFERENCED_PARAMETER(dummy4);

    COMMON_RETURN_ZERO;
}

DWORD WINAPI fakeImm_wd5(PVOID dummy1, PVOID dummy2, PVOID dummy3, PVOID dummy4, PVOID dummy5)
{
    UNREFERENCED_PARAMETER(dummy1);
    UNREFERENCED_PARAMETER(dummy2);
    UNREFERENCED_PARAMETER(dummy3);
    UNREFERENCED_PARAMETER(dummy4);
    UNREFERENCED_PARAMETER(dummy5);

    COMMON_RETURN_ZERO;
}

DWORD WINAPI fakeImm_wd6(PVOID dummy1, PVOID dummy2, PVOID dummy3, PVOID dummy4, PVOID dummy5, PVOID dummy6)
{
    UNREFERENCED_PARAMETER(dummy1);
    UNREFERENCED_PARAMETER(dummy2);
    UNREFERENCED_PARAMETER(dummy3);
    UNREFERENCED_PARAMETER(dummy4);
    UNREFERENCED_PARAMETER(dummy5);
    UNREFERENCED_PARAMETER(dummy6);

    COMMON_RETURN_ZERO;
}


ImmApiEntries gImmApiEntries = {
    (BOOL (WINAPI* /*ImmWINNLSEnableIME*/)(HWND, BOOL))     fakeImm_wd2,
    (BOOL (WINAPI* /*ImmWINNLSGetEnableStatus*/)(HWND))     fakeImm_wd1,
    (LRESULT (WINAPI* /*SendIMEMessageExW*/)(HWND, LPARAM)) fakeImm_wd2,
    (LRESULT (WINAPI* /*SendIMEMessageExA*/)(HWND, LPARAM)) fakeImm_wd2,
    (BOOL (WINAPI* /*IMPGetIMEW*/)(HWND, LPIMEPROW))        fakeImm_wd2,
    (BOOL (WINAPI* /*IMPGetIMEA*/)(HWND, LPIMEPROA))        fakeImm_wd2,
    (BOOL (WINAPI* /*IMPQueryIMEW*/)(LPIMEPROW))            fakeImm_wd1,
    (BOOL (WINAPI* /*IMPQueryIMEA*/)(LPIMEPROA))            fakeImm_wd1,
    (BOOL (WINAPI* /*IMPSetIMEW*/)(HWND, LPIMEPROW))        fakeImm_wd2,
    (BOOL (WINAPI* /*IMPSetIMEA*/)(HWND, LPIMEPROA))        fakeImm_wd2,

    (HIMC (WINAPI* /*ImmAssociateContext*/)(HWND, HIMC))    fakeImm_wd2,
    (LRESULT (WINAPI* /*ImmEscapeA*/)(HKL, HIMC, UINT, LPVOID)) fakeImm_wd4,
    (LRESULT (WINAPI* /*ImmEscapeW*/)(HKL, HIMC, UINT, LPVOID)) fakeImm_wd4,
    (LONG (WINAPI* /*ImmGetCompositionStringA*/)(HIMC, DWORD, LPVOID, DWORD)) fakeImm_wd4,
    (LONG (WINAPI* /*ImmGetCompositionStringW*/)(HIMC, DWORD, LPVOID, DWORD)) fakeImm_wd4,
    (BOOL (WINAPI* /*ImmGetCompositionWindow*/)(HIMC, LPCOMPOSITIONFORM)) fakeImm_wd2,
    (HIMC (WINAPI* /*ImmGetContext*/)(HWND))                fakeImm_wd1,
    (HWND (WINAPI* /*ImmGetDefaultIMEWnd*/)(HWND))          fakeImm_wd1,
    (BOOL (WINAPI* /*ImmIsIME*/)(HKL))                      fakeImm_wd1,
    (BOOL (WINAPI* /*ImmReleaseContext*/)(HWND, HIMC))      fakeImm_wd2,
    (BOOL (* /*ImmRegisterClient*/)(PSHAREDINFO, HINSTANCE))           fakeImm_bd2,

    (BOOL (WINAPI* /*ImmGetCompositionFontW*/)(HIMC, LPLOGFONTW)) fakeImm_wd2,
    (BOOL (WINAPI* /*ImmGetCompositionFontA*/)(HIMC, LPLOGFONTA)) fakeImm_wd2,
    (BOOL (WINAPI* /*ImmSetCompositionFontW*/)(HIMC, LPLOGFONTW)) fakeImm_wd2,
    (BOOL (WINAPI* /*ImmSetCompositionFontA*/)(HIMC, LPLOGFONTA)) fakeImm_wd2,

    (BOOL (WINAPI* /*ImmSetCompositionWindow*/)(HIMC, LPCOMPOSITIONFORM)) fakeImm_wd2,
    (BOOL (WINAPI* /*ImmNotifyIME*/)(HIMC, DWORD, DWORD, DWORD)) fakeImm_wd4,
    (PINPUTCONTEXT (WINAPI* /*ImmLockIMC*/)(HIMC))          fakeImm_wd1,
    (BOOL (WINAPI* /*ImmUnlockIMC*/)(HIMC))                 fakeImm_wd1,
    (BOOL (WINAPI* /*ImmLoadIME*/)(HKL))                    fakeImm_wd1,
    (BOOL (WINAPI* /*ImmSetOpenStatus*/)(HIMC, BOOL))       fakeImm_wd2,
    (BOOL (WINAPI* /*ImmFreeLayout*/)(DWORD dwFlag))        fakeImm_wd1,
    (BOOL (WINAPI* /*ImmActivateLayout*/)(HKL))             fakeImm_wd1,
    (BOOL (WINAPI* /*ImmSetCandidateWindow*/)(HIMC, LPCANDIDATEFORM))   fakeImm_wd2,
    (BOOL (WINAPI* /*ImmConfigureIMEW*/)(HKL, HWND, DWORD, LPVOID)) fakeImm_wd4,
    (BOOL (WINAPI* /*ImmGetConversionStatus*/)(HIMC, LPDWORD, LPDWORD)) fakeImm_wd3,
    (BOOL (WINAPI* /*ImmSetConversionStatus*/)(HIMC, DWORD, DWORD)) fakeImm_wd3,
    (BOOL (WINAPI* /*ImmSetStatusWindowPos*/)(HIMC, LPPOINT))           fakeImm_wd2,
    (BOOL (WINAPI* /*ImmGetImeInfoEx*/)(PIMEINFOEX, IMEINFOEXCLASS, PVOID)) fakeImm_wd3,
    (PIMEDPI (WINAPI* /*ImmLockImeDpi*/)(HKL))              fakeImm_wd1,
    (VOID (WINAPI* /*ImmUnlockImeDpi*/)(PIMEDPI))           fakeImm_wv1,
    (BOOL (WINAPI* /*ImmGetOpenStatus*/)(HIMC))             fakeImm_wd1,
    (BOOL (* /*ImmSetActiveContext*/)(HWND, HIMC, BOOL))    fakeImm_d3,
    (BOOL (* /*ImmTranslateMessage*/)(HWND, UINT, WPARAM, LPARAM)) fakeImm_bwuwl,
    (BOOL (* /*ImmLoadLayout*/)(HKL, PIMEINFOEX))           fakeImm_d2,
    (DWORD (WINAPI* /*ImmProcessKey*/)(HWND, HKL, UINT, LPARAM, DWORD)) fakeImm_wd5,
    (LRESULT (* /*ImmPutImeMenuItemsIntoMappedFile*/)(HIMC)) fakeImm_d1,
    (DWORD (WINAPI* /*ImmGetProperty*/)(HKL, DWORD))        fakeImm_wd2,
    (BOOL (WINAPI* /*ImmSetCompositionStringA*/)(
         HIMC hImc, DWORD dwIndex, LPCVOID lpComp, DWORD dwCompLen, LPCVOID lpRead, DWORD dwReadLen))
                                                            fakeImm_wd6,
    (BOOL (WINAPI* /*ImmSetCompositionStringW*/)(
         HIMC hImc, DWORD dwIndex, LPCVOID lpComp, DWORD dwCompLen, LPCVOID lpRead, DWORD dwReadLen))
                                                            fakeImm_wd6,
    (BOOL (WINAPI* /*ImmEnumInputContext*/)(
         DWORD idThread, IMCENUMPROC lpfn, LPARAM lParam))  fakeImm_wd3,

    (LRESULT (WINAPI* /*ImmSystemHandler*/)(HIMC, WPARAM, LPARAM))
                                                             fakeImm_wd3,
};


//
// Imm32's instance handle
//
// NULL if not initialized.
//

HMODULE ghImm32;

#define IMMMODULENAME L"IMM32.DLL"
#define PATHDLM     L'\\'
#define IMMMODULENAMELEN    ((sizeof PATHDLM + sizeof IMMMODULENAME) / sizeof(WCHAR))

VOID GetImmFileName(PWSTR wszImmFile)
{
    UINT i = GetSystemDirectoryW(wszImmFile, MAX_PATH);
    if (i > 0 && i < MAX_PATH - IMMMODULENAMELEN) {
        wszImmFile += i;
        if (wszImmFile[-1] != PATHDLM) {
            *wszImmFile++ = PATHDLM;
        }
    }
    wcscpy(wszImmFile, IMMMODULENAME);
}

#define REGISTER(name,cast) \
    gImmApiEntries.name = (cast)GetProcAddress(hImm, #name); \
    if (gImmApiEntries.name == NULL) { \
        RIPMSG1(RIP_WARNING, "gImmApiEntries.%s got to be NULL", #name); \
        gImmApiEntries.name = (PVOID)fakeImm_v1; \
        return; \
    } else

///////////////////////////////////////////////////////
// _InitializeImmEntryTable(HMODULE)
//
//  Initialize IMM entry table:
///////////////////////////////////////////////////////
VOID _InitializeImmEntryTable(VOID)
{
    HMODULE hImm = ghImm32;
    WCHAR wszImmFile[MAX_PATH];

    if (((PVOID)gImmApiEntries.ImmWINNLSEnableIME) != ((PVOID)fakeImm_wd2)) {
        // already initialized.
        return;
    }

    GetImmFileName(wszImmFile);

    if (hImm == NULL) {
        // check if IMM DLL is already attached to the process
        hImm = GetModuleHandleW(wszImmFile);
    }

    if (hImm == NULL) {
        hImm = ghImm32 = LoadLibraryW(wszImmFile);
        if (hImm == NULL) {
            RIPMSG1(RIP_WARNING, "_InitializeImmEntryTable: failed to load Imm32.Dll: err=%d\n", GetLastError());
            return;
        }

        // at this point, Init routine of IMM32 has been called, thus User32InitializeImmEntry.. called.
        // all what we have to do is just return here.
        return;
    }

    if (hImm == NULL) {
        RIPMSG0(RIP_WARNING, "Failed to attach IMM32.DLL.\n");
        return;
    }

    // get the addresses of the procedures
    REGISTER(ImmWINNLSEnableIME, BOOL (WINAPI*)(HWND, BOOL));
    REGISTER(ImmWINNLSGetEnableStatus, BOOL (*)(HWND));
    REGISTER(ImmSendIMEMessageExW, LRESULT (*)(HWND, LPARAM));
    REGISTER(ImmSendIMEMessageExA, LRESULT (*)(HWND, LPARAM));
    REGISTER(ImmIMPGetIMEW, BOOL(*)(HWND, LPIMEPROW));
    REGISTER(ImmIMPGetIMEA, BOOL(*)(HWND, LPIMEPROA))
    REGISTER(ImmIMPQueryIMEW, BOOL(*)(LPIMEPROW))
    REGISTER(ImmIMPQueryIMEA, BOOL(*)(LPIMEPROA));
    REGISTER(ImmIMPSetIMEW, BOOL(*)(HWND, LPIMEPROW));
    REGISTER(ImmIMPSetIMEA, BOOL(*)(HWND, LPIMEPROA));

    REGISTER(ImmAssociateContext, HIMC (WINAPI*)(HWND, HIMC));
    REGISTER(ImmEscapeA, LRESULT(WINAPI*)(HKL, HIMC, UINT, LPVOID));
    REGISTER(ImmEscapeW, LRESULT(WINAPI*)(HKL, HIMC, UINT, LPVOID));
    REGISTER(ImmGetCompositionStringA, LONG (WINAPI*)(HIMC, DWORD, LPVOID, DWORD));
    REGISTER(ImmGetCompositionStringW, LONG (WINAPI*)(HIMC, DWORD, LPVOID, DWORD));
    REGISTER(ImmGetCompositionWindow, BOOL (WINAPI*)(HIMC, LPCOMPOSITIONFORM));
    REGISTER(ImmGetContext, HIMC (WINAPI*)(HWND));
    REGISTER(ImmGetDefaultIMEWnd, HWND (WINAPI*)(HWND));
    REGISTER(ImmIsIME, BOOL (WINAPI*)(HKL));
    REGISTER(ImmReleaseContext, BOOL (WINAPI*)(HWND, HIMC));
    REGISTER(ImmRegisterClient, BOOL(*)(PSHAREDINFO, HINSTANCE));

    REGISTER(ImmGetCompositionFontW, BOOL (WINAPI*)(HIMC, LPLOGFONTW));
    REGISTER(ImmGetCompositionFontA, BOOL (WINAPI*)(HIMC, LPLOGFONTA));
    REGISTER(ImmSetCompositionFontW, BOOL (WINAPI*)(HIMC, LPLOGFONTW));
    REGISTER(ImmSetCompositionFontA, BOOL (WINAPI*)(HIMC, LPLOGFONTA));

    REGISTER(ImmSetCompositionWindow, BOOL(WINAPI*)(HIMC, LPCOMPOSITIONFORM));
    REGISTER(ImmNotifyIME, BOOL (WINAPI*)(HIMC, DWORD, DWORD, DWORD));
    REGISTER(ImmLockIMC, PINPUTCONTEXT(WINAPI*)(HIMC));
    REGISTER(ImmUnlockIMC, BOOL (WINAPI*)(HIMC));
    REGISTER(ImmLoadIME, BOOL (WINAPI*)(HKL));
    REGISTER(ImmSetOpenStatus, BOOL (WINAPI*)(HIMC, BOOL));
    REGISTER(ImmFreeLayout, BOOL (WINAPI*)(DWORD));
    REGISTER(ImmActivateLayout, BOOL (WINAPI*)(HKL));
    REGISTER(ImmSetCandidateWindow, BOOL (WINAPI*)(HIMC, LPCANDIDATEFORM));
    REGISTER(ImmConfigureIMEW, BOOL (WINAPI*)(HKL, HWND, DWORD, LPVOID));
    REGISTER(ImmGetConversionStatus, BOOL (WINAPI*)(HIMC, LPDWORD, LPDWORD));
    REGISTER(ImmSetConversionStatus, BOOL (WINAPI*)(HIMC, DWORD, DWORD));
    REGISTER(ImmSetStatusWindowPos, BOOL (WINAPI*)(HIMC, LPPOINT));
    REGISTER(ImmGetImeInfoEx, BOOL (WINAPI*)(PIMEINFOEX, IMEINFOEXCLASS, PVOID));
    REGISTER(ImmLockImeDpi, PIMEDPI (WINAPI*)(HKL));
    REGISTER(ImmUnlockImeDpi, VOID (WINAPI*)(PIMEDPI));
    REGISTER(ImmGetOpenStatus, BOOL (WINAPI*)(HIMC));
    REGISTER(ImmSetActiveContext, BOOL (*)(HWND, HIMC, BOOL));
    REGISTER(ImmTranslateMessage, BOOL (*)(HWND, UINT, WPARAM, LPARAM));
    REGISTER(ImmLoadLayout, BOOL (*)(HKL, PIMEINFOEX));
    REGISTER(ImmProcessKey, DWORD (*)(HWND, HKL, UINT, LPARAM, DWORD));
    REGISTER(ImmPutImeMenuItemsIntoMappedFile, LRESULT(*)(HIMC));
    REGISTER(ImmGetProperty, DWORD (WINAPI*)(HKL, DWORD));
    REGISTER(ImmSetCompositionStringA,
             BOOL (WINAPI*)(HIMC hImc, DWORD dwIndex, LPCVOID lpComp,
                   DWORD dwCompLen, LPCVOID lpRead, DWORD dwReadLen))
    REGISTER(ImmSetCompositionStringW,
             BOOL (WINAPI*)(HIMC hImc, DWORD dwIndex, LPCVOID lpComp,
                   DWORD dwCompLen, LPCVOID lpRead, DWORD dwReadLen));
    REGISTER(ImmEnumInputContext,
             BOOL (WINAPI*)(DWORD idThread, IMCENUMPROC lpfn, LPARAM lParam));
    REGISTER(ImmSystemHandler,
            LRESULT (WINAPI*)(HIMC, WPARAM, LPARAM));
}

BOOL bImmInitializing = FALSE;

VOID InitializeImmEntryTable(VOID)
{
    bImmInitializing = TRUE;
    _InitializeImmEntryTable();
}

BOOL User32InitializeImmEntryTable(DWORD magic)
{
    if (magic != IMM_MAGIC_CALLER_ID) {
        RIPMSG1(RIP_WARNING, "User32InitializeImmEntryTable: magic # does not match: 0x%08x", magic);
        return FALSE;
    }

    if (((PVOID)gImmApiEntries.ImmWINNLSEnableIME) != ((PVOID)fakeImm_wd2)) {
        // already initialized
        return TRUE;
    }

    _InitializeImmEntryTable();

    if (ghImm32 == NULL) {
        if (!bImmInitializing) {
            // increment the load counter of IMM32.DLL; application may call FreeLibrary later
            WCHAR wszImmFile[MAX_PATH];
            GetImmFileName(wszImmFile);
            ghImm32 = LoadLibraryW(wszImmFile);
        }
    }
    // for IMM initialization
    return fpImmRegisterClient(&gSharedInfo, ghImm32);
}

//
// for historical reasons, these entries are put in user32.dll
//
UINT WINAPI WINNLSGetIMEHotkey(HWND hwndIme)
{
    UNREFERENCED_PARAMETER(hwndIme);

    //
    // Win95/NT3.51 behavior, i.e. always return 0.
    //
    return 0;
}

BOOL WINAPI WINNLSEnableIME(HWND hwnd, BOOL bFlag)
{
    return gImmApiEntries.ImmWINNLSEnableIME(hwnd, bFlag);
}

BOOL WINAPI WINNLSGetEnableStatus(HWND hwnd)
{
    return gImmApiEntries.ImmWINNLSGetEnableStatus(hwnd);
}

LRESULT WINAPI SendIMEMessageExW(HWND hwnd, LPARAM lParam)
{
    return gImmApiEntries.ImmSendIMEMessageExW(hwnd, lParam);
}

LRESULT WINAPI SendIMEMessageExA(HWND hwnd, LPARAM lParam)
{
    return gImmApiEntries.ImmSendIMEMessageExA(hwnd, lParam);
}

BOOL WINAPI IMPGetIMEW(HWND hwnd, LPIMEPROW lpImeProW)
{
    return gImmApiEntries.ImmIMPGetIMEW(hwnd, lpImeProW);
}

BOOL WINAPI IMPGetIMEA(HWND hwnd, LPIMEPROA lpImeProA)
{
    return gImmApiEntries.ImmIMPGetIMEA(hwnd, lpImeProA);
}

BOOL WINAPI IMPQueryIMEW(LPIMEPROW lpImeProW)
{
    return gImmApiEntries.ImmIMPQueryIMEW(lpImeProW);
}

BOOL WINAPI IMPQueryIMEA(LPIMEPROA lpImeProA)
{
    return gImmApiEntries.ImmIMPQueryIMEA(lpImeProA);
}

BOOL WINAPI IMPSetIMEW(HWND hwnd, LPIMEPROW lpImeProW)
{
    return gImmApiEntries.ImmIMPSetIMEW(hwnd, lpImeProW);
}

BOOL WINAPI IMPSetIMEA(HWND hwnd, LPIMEPROA lpImeProA)
{
    return gImmApiEntries.ImmIMPSetIMEA(hwnd, lpImeProA);
}
