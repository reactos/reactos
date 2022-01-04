/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for imm32 HIMC
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

START_TEST(himc)
{
    DWORD style;
    HWND hwndEdit, hwndStatic;
    HIMC hNewIMC, hOldIMC, hIMC, hIMC1, hIMC2;
    LPINPUTCONTEXT pIC;

    /* ImmCreateContext/ImmDestroyContext and ImmLockIMC/ImmUnlockIMC */
    hNewIMC = ImmCreateContext();
    ok(hNewIMC != NULL, "ImmCreateContext failed\n");
    pIC = ImmLockIMC(hNewIMC);
    ok(pIC == NULL, "ImmLockIMC succeeded unexpectedly\n");
    ImmUnlockIMC(hNewIMC);
    ok(ImmDestroyContext(hNewIMC), "ImmDestroyContext failed\n");

    /* ImmGetContext against NULL */
    hIMC = ImmGetContext(NULL);
    ok(hIMC == NULL, "ImmGetContext failed\n");

    /* Create EDIT control */
    style = ES_MULTILINE | ES_LEFT;
    hwndEdit = CreateWindowW(L"EDIT", NULL, style, 0, 0, 100, 20, NULL, NULL,
                             GetModuleHandleW(NULL), NULL);
    ok(hwndEdit != NULL, "CreateWindowW failed\n");

    /* Create STATIC control */
    style = SS_LEFT;
    hwndStatic = CreateWindowW(L"STATIC", NULL, style, 0, 30, 100, 20, NULL, NULL,
                               GetModuleHandleW(NULL), NULL);
    ok(hwndStatic != NULL, "CreateWindowW failed\n");

    /* ImmGetContext/ImmReleaseContext and ImmLockIMC/ImmUnlockIMC */
    hIMC1 = hIMC = ImmGetContext(hwndEdit);
    ok(hIMC != NULL, "ImmGetContext failed\n");
    pIC = ImmLockIMC(hIMC);
    ok(pIC != NULL, "ImmLockIMC failed\n");
    if (pIC != NULL)
    {
        ok(pIC->hWnd == NULL, "pIC->hWnd = %p\n", pIC->hWnd);
        ok(!pIC->fOpen, "pIC->fOpen = %d\n", pIC->fOpen);
        ok(ImmGetIMCCSize(pIC->hCompStr) != 0, "hCompStr size is 0\n");
        ok(ImmGetIMCCSize(pIC->hCandInfo) != 0, "hCandInfo size is 0\n");
        ok(ImmGetIMCCSize(pIC->hGuideLine) != 0, "hGuideLine size is 0\n");
        ok(ImmGetIMCCSize(pIC->hPrivate) != 0, "hPrivate size is 0\n");
        ok(ImmGetIMCCSize(pIC->hMsgBuf) != 0, "hMsgBuf size is 0\n");
    }
    else
    {
        skip("No pIC\n");
    }
    ImmUnlockIMC(hNewIMC);
    SetFocus(hwndEdit);
    pIC = ImmLockIMC(hIMC);
    ok(pIC != NULL, "ImmLockIMC failed\n");
    if (pIC != NULL)
    {
        ok(pIC->hWnd == hwndEdit, "pIC->hWnd = %p, expected %p\n", pIC->hWnd, hwndEdit);
        ok(!pIC->fOpen, "pIC->fOpen = %d\n", pIC->fOpen);
    }
    else
    {
        skip("No pIC\n");
    }
    ImmUnlockIMC(hNewIMC);
    SetFocus(NULL);
    pIC = ImmLockIMC(hIMC);
    ok(pIC != NULL, "ImmLockIMC failed\n");
    if (pIC != NULL)
    {
        ok(pIC->hWnd == hwndEdit, "pIC->hWnd = %p, expected %p\n", pIC->hWnd, hwndEdit);
    }
    else
    {
        skip("No pIC\n");
    }
    ImmUnlockIMC(hNewIMC);
    ok(ImmSetOpenStatus(hIMC, TRUE), "ImmSetOpenStatus failed\n");
    pIC = ImmLockIMC(hIMC);
    ok(pIC != NULL, "ImmLockIMC failed\n");
    if (pIC != NULL)
    {
        ok(pIC->fOpen, "pIC->fOpen = %d\n", pIC->fOpen);
    }
    else
    {
        skip("No pIC\n");
    }
    ImmUnlockIMC(hNewIMC);
    ok(ImmReleaseContext(hwndEdit, hIMC), "ImmReleaseContext failed\n");

    hIMC2 = hIMC = ImmGetContext(hwndStatic);
    ok(hIMC != NULL, "ImmGetContext failed\n");
    pIC = ImmLockIMC(hIMC);
    ok(pIC != NULL, "ImmLockIMC failed\n");
    if (pIC != NULL)
    {
        ok(pIC->hWnd == hwndEdit, "pIC->hWnd = %p, expected %p\n", pIC->hWnd, hwndEdit);
        ok(ImmGetIMCCSize(pIC->hCompStr) != 0, "hCompStr size is 0\n");
        ok(ImmGetIMCCSize(pIC->hCandInfo) != 0, "hCandInfo size is 0\n");
        ok(ImmGetIMCCSize(pIC->hGuideLine) != 0, "hGuideLine size is 0\n");
        ok(ImmGetIMCCSize(pIC->hPrivate) != 0, "hPrivate size is 0\n");
        ok(ImmGetIMCCSize(pIC->hMsgBuf) != 0, "hMsgBuf size is 0\n");
    }
    else
    {
        skip("No pIC\n");
    }
    ImmUnlockIMC(hNewIMC);
    ok(ImmReleaseContext(hwndEdit, hIMC), "ImmReleaseContext failed\n");

    ok(hIMC1 == hIMC2, "hIMC1 = %p, expected %p\n", hIMC1, hIMC2);

    /* ImmAssociateContext */
    hNewIMC = ImmCreateContext();
    ok(hNewIMC != NULL, "ImmCreateContext failed \n");
    pIC = ImmLockIMC(hNewIMC);
    ok(pIC != NULL, "ImmLockIMC failed\n");
    ImmUnlockIMC(hNewIMC);
    hOldIMC = ImmAssociateContext(hwndEdit, hNewIMC);
    ok(hNewIMC != hOldIMC, "hNewIMC = %p, expected not %p\n", hNewIMC, hOldIMC);
    hIMC = ImmGetContext(hwndEdit);
    ok(hIMC == hNewIMC, "hIMC = %p, expected %p\n", hIMC, hNewIMC);
    ok(hIMC != hOldIMC, "hIMC = %p, expected not %p\n", hIMC, hOldIMC);
    pIC = ImmLockIMC(hNewIMC);
    ok(pIC != NULL, "ImmLockIMC failed\n");
    if (pIC != NULL)
    {
        ok(pIC->hWnd == NULL, "pIC->hWnd = %p\n", pIC->hWnd);
        ok(ImmGetIMCCSize(pIC->hCompStr) != 0, "hCompStr size is 0\n");
        ok(ImmGetIMCCSize(pIC->hCandInfo) != 0, "hCandInfo size is 0\n");
        ok(ImmGetIMCCSize(pIC->hGuideLine) != 0, "hGuideLine size is 0\n");
        ok(ImmGetIMCCSize(pIC->hPrivate) != 0, "hPrivate size is 0\n");
        ok(ImmGetIMCCSize(pIC->hMsgBuf) != 0, "hMsgBuf size is 0\n");
    }
    else
    {
        skip("No pIC\n");
    }
    ImmUnlockIMC(hNewIMC);
    ok(ImmReleaseContext(hwndEdit, hIMC), "ImmReleaseContext failed\n");
    ok(ImmDestroyContext(hNewIMC), "ImmDestroyContext failed\n");

    DestroyWindow(hwndEdit);
    DestroyWindow(hwndStatic);
}
