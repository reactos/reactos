/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for ImmGetImeInfoEx
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

//#define DO_PRINT

typedef BOOL (WINAPI *FN_ImmGetImeInfoEx)(PIMEINFOEX, IMEINFOEXCLASS, PVOID);

static VOID PrintInfoEx(PIMEINFOEX pInfoEx)
{
#ifdef DO_PRINT
    trace("---\n");
    trace("hkl: %p\n", pInfoEx->hkl);
    trace("ImeInfo.dwPrivateDataSize: 0x%lX\n", pInfoEx->ImeInfo.dwPrivateDataSize);
    trace("ImeInfo.fdwProperty: 0x%lX\n", pInfoEx->ImeInfo.fdwProperty);
    trace("ImeInfo.fdwConversionCaps: 0x%lX\n", pInfoEx->ImeInfo.fdwConversionCaps);
    trace("ImeInfo.fdwSentenceCaps: 0x%lX\n", pInfoEx->ImeInfo.fdwSentenceCaps);
    trace("ImeInfo.fdwUICaps: 0x%lX\n", pInfoEx->ImeInfo.fdwUICaps);
    trace("ImeInfo.fdwSCSCaps: 0x%lX\n", pInfoEx->ImeInfo.fdwSCSCaps);
    trace("ImeInfo.fdwSelectCaps: 0x%lX\n", pInfoEx->ImeInfo.fdwSelectCaps);
    trace("wszUIClass: '%ls'\n", pInfoEx->wszUIClass);
    trace("fdwInitConvMode: 0x%lX\n", pInfoEx->fdwInitConvMode);
    trace("fInitOpen: %d\n", pInfoEx->fInitOpen);
    trace("fLoadFlag: %d\n", pInfoEx->fLoadFlag);
    trace("dwProdVersion: 0x%lX\n", pInfoEx->dwProdVersion);
    trace("dwImeWinVersion: 0x%lX\n", pInfoEx->dwImeWinVersion);
    trace("wszImeDescription: '%ls'\n", pInfoEx->wszImeDescription);
    trace("wszImeFile: '%ls'\n", pInfoEx->wszImeFile);
    trace("fInitOpen: %d\n", pInfoEx->fInitOpen);
#endif
}

START_TEST(ImmGetImeInfoEx)
{
    IMEINFOEX InfoEx;
    BOOL ret;
    LANGID LangID = GetSystemDefaultLangID();
    HKL hKL = GetKeyboardLayout(0);

    HMODULE hImm32 = GetModuleHandleA("imm32");
    FN_ImmGetImeInfoEx fnImmGetImeInfoEx = GetProcAddress(hImm32, "ImmGetImeInfoEx");
    if (!fnImmGetImeInfoEx)
    {
        skip("ImmGetImeInfoEx not found\n");
        return;
    }

    if (!GetSystemMetrics(SM_IMMENABLED))
    {
        skip("IME is not available\n");
        return;
    }

    // ImeInfoExKeyboardLayout
    FillMemory(&InfoEx, sizeof(InfoEx), 0xCC);
    ret = fnImmGetImeInfoEx(&InfoEx, ImeInfoExKeyboardLayout, &hKL);
    PrintInfoEx(&InfoEx);

    if ((HIWORD(InfoEx.hkl) & 0xe000) == 0xe000)
        ok_long(LOWORD(InfoEx.hkl), LangID);
    else
        ok_long((DWORD)(DWORD_PTR)InfoEx.hkl, MAKELONG(LangID, LangID));
    ok(InfoEx.ImeInfo.dwPrivateDataSize >= 4, "\n");
    ok(InfoEx.wszUIClass[0] != 0, "\n");
    ok_long(InfoEx.dwImeWinVersion, 0x40000);
    ok(InfoEx.wszImeFile[0] != 0, "wszImeFile was empty\n");
    ok_int(ret, TRUE);

    // ImeInfoExImeWindow
    FillMemory(&InfoEx, sizeof(InfoEx), 0xCC);
    ret = fnImmGetImeInfoEx(&InfoEx, ImeInfoExImeWindow, &hKL);
    PrintInfoEx(&InfoEx);

    if ((HIWORD(InfoEx.hkl) & 0xe000) == 0xe000)
        ok_long(LOWORD(InfoEx.hkl), LangID);
    else
        ok_long((DWORD)(DWORD_PTR)InfoEx.hkl, MAKELONG(LangID, LangID));
    ok(InfoEx.ImeInfo.dwPrivateDataSize >= 4, "\n");
    ok(InfoEx.wszUIClass[0] != 0, "\n");
    ok_long(InfoEx.dwImeWinVersion, 0x40000);
    ok(InfoEx.wszImeFile[0] != 0, "wszImeFile was empty\n");
    ok_int(ret, TRUE);

    // TODO: ImeInfoExImeFileName
}
