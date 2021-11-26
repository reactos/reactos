/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for ImmGetImeInfoEx
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

//#define DO_PRINT

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

typedef BOOL (WINAPI *FN_ImmGetImeInfoEx)(PIMEINFOEX, IMEINFOEXCLASS, PVOID);

START_TEST(ImmGetImeInfoEx)
{
    IMEINFOEX InfoEx;
    BOOL ret, bMatch;
    size_t ib;
    LANGID LangID = GetSystemDefaultLangID();
    HKL hKL = GetKeyboardLayout(0), hOldKL;

    HMODULE hImm32 = GetModuleHandleA("imm32");
    FN_ImmGetImeInfoEx fnImmGetImeInfoEx =
        (FN_ImmGetImeInfoEx)GetProcAddress(hImm32, "ImmGetImeInfoEx");
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
    hOldKL = hKL;
    FillMemory(&InfoEx, sizeof(InfoEx), 0xCC);
    InfoEx.wszUIClass[0] = InfoEx.wszImeFile[0] = 0;
    ret = fnImmGetImeInfoEx(&InfoEx, ImeInfoExKeyboardLayout, &hKL);
    PrintInfoEx(&InfoEx);
    ok_int(ret, TRUE);
    ok_long((DWORD)(DWORD_PTR)hOldKL, (DWORD)(DWORD_PTR)hKL);
    if (IS_IME_HKL(InfoEx.hkl))
    {
        ok_long(LOWORD(InfoEx.hkl), LangID);
    }
    else
    {
        ok_int(LOWORD(InfoEx.hkl), LangID);
        ok_int(HIWORD(InfoEx.hkl), LangID);
    }
    ok(InfoEx.ImeInfo.dwPrivateDataSize >= 4, "\n");
    ok(InfoEx.wszUIClass[0] != 0, "wszUIClass was empty\n");
    ok_long(InfoEx.dwImeWinVersion, 0x40000);
    ok(InfoEx.wszImeFile[0] != 0, "wszImeFile was empty\n");
    hKL = hOldKL;

    // ImeInfoExImeWindow
    hOldKL = hKL;
    FillMemory(&InfoEx, sizeof(InfoEx), 0xCC);
    InfoEx.wszUIClass[0] = InfoEx.wszImeFile[0] = 0;
    ret = fnImmGetImeInfoEx(&InfoEx, ImeInfoExImeWindow, &hKL);
    PrintInfoEx(&InfoEx);
    ok_int(ret, TRUE);
    if (IS_IME_HKL(InfoEx.hkl))
    {
        ok_long(LOWORD(InfoEx.hkl), LangID);
    }
    else
    {
        ok_int(LOWORD(InfoEx.hkl), LangID);
        ok_int(HIWORD(InfoEx.hkl), LangID);
    }
    ok(InfoEx.ImeInfo.dwPrivateDataSize >= 4, "\n");
    ok(InfoEx.wszUIClass[0] != 0, "wszUIClass was empty\n");
    ok_long(InfoEx.dwImeWinVersion, 0x40000);
    ok(InfoEx.wszImeFile[0] != 0, "wszImeFile was empty\n");
    hKL = hOldKL;

    // TODO: ImeInfoExImeFileName

    // 4
    hOldKL = hKL;
    FillMemory(&InfoEx, sizeof(InfoEx), 0xCC);
    ret = fnImmGetImeInfoEx(&InfoEx, 4, &hKL);
    ok_int(ret, FALSE);
    bMatch = TRUE;
    for (ib = 0; ib < sizeof(InfoEx); ++ib)
    {
        if (((LPBYTE)&InfoEx)[ib] != 0xCC)
        {
            bMatch = FALSE;
            break;
        }
    }
    ok_int(bMatch, TRUE);
}
