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
    ok(hNewIMC != NULL, "\n");
    pIC = ImmLockIMC(hNewIMC);
    ok(pIC == NULL, "\n");
    ImmUnlockIMC(hNewIMC);
    ok(ImmDestroyContext(hNewIMC), "\n");

    /* ImmGetContext against NULL */
    hIMC = ImmGetContext(NULL);
    ok(hIMC == NULL, "\n");

    /* Create EDIT control */
    style = ES_MULTILINE | ES_LEFT;
    hwndEdit = CreateWindowW(L"EDIT", NULL, style, 0, 0, 100, 20, NULL, NULL,
                             GetModuleHandleW(NULL), NULL);
    ok(hwndEdit != NULL, "\n");

    /* Create STATIC control */
    style = SS_LEFT;
    hwndStatic = CreateWindowW(L"STATIC", NULL, style, 0, 30, 100, 20, NULL, NULL,
                               GetModuleHandleW(NULL), NULL);
    ok(hwndStatic != NULL, "\n");

    /* ImmGetContext/ImmReleaseContext and ImmLockIMC/ImmUnlockIMC */
    hIMC1 = hIMC = ImmGetContext(hwndEdit);
    ok(hIMC != NULL, "\n");
    pIC = ImmLockIMC(hIMC);
    ok(pIC != NULL, "\n");
    ok(pIC->hWnd == NULL, "\n");
    ok(!pIC->fOpen, "\n");
    ok(ImmGetIMCCSize(pIC->hCompStr) != 0, "\n");
    ok(ImmGetIMCCSize(pIC->hCandInfo) != 0, "\n");
    ok(ImmGetIMCCSize(pIC->hGuideLine) != 0, "\n");
    ok(ImmGetIMCCSize(pIC->hPrivate) != 0, "\n");
    ok(ImmGetIMCCSize(pIC->hMsgBuf) != 0, "\n");
    ImmUnlockIMC(hNewIMC);
    SetFocus(hwndEdit);
    pIC = ImmLockIMC(hIMC);
    ok(pIC != NULL, "\n");
    ok(pIC->hWnd == hwndEdit, "\n");
    ok(!pIC->fOpen, "\n");
    ImmUnlockIMC(hNewIMC);
    SetFocus(NULL);
    pIC = ImmLockIMC(hIMC);
    ok(pIC != NULL, "\n");
    ok(pIC->hWnd == hwndEdit, "\n");
    ImmUnlockIMC(hNewIMC);
    ok(ImmSetOpenStatus(hIMC, TRUE), "\n");
    pIC = ImmLockIMC(hIMC);
    ok(pIC != NULL, "\n");
    ok(pIC->fOpen, "\n");
    ImmUnlockIMC(hNewIMC);
    ok(ImmReleaseContext(hwndEdit, hIMC), "\n");

    hIMC2 = hIMC = ImmGetContext(hwndStatic);
    ok(hIMC != NULL, "\n");
    pIC = ImmLockIMC(hIMC);
    ok(pIC != NULL, "\n");
    ok(pIC->hWnd == hwndEdit, "\n");
    ok(ImmGetIMCCSize(pIC->hCompStr) != 0, "\n");
    ok(ImmGetIMCCSize(pIC->hCandInfo) != 0, "\n");
    ok(ImmGetIMCCSize(pIC->hGuideLine) != 0, "\n");
    ok(ImmGetIMCCSize(pIC->hPrivate) != 0, "\n");
    ok(ImmGetIMCCSize(pIC->hMsgBuf) != 0, "\n");
    ImmUnlockIMC(hNewIMC);
    ok(ImmReleaseContext(hwndEdit, hIMC), "\n");

    ok(hIMC1 == hIMC2, "\n");

    /* ImmAssociateContext */
    hNewIMC = ImmCreateContext();
    ok(hNewIMC != NULL, "\n");
    pIC = ImmLockIMC(hNewIMC);
    ok(pIC != NULL, "\n");
    ImmUnlockIMC(hNewIMC);
    hOldIMC = ImmAssociateContext(hwndEdit, hNewIMC);
    ok(hNewIMC != hOldIMC, "\n");
    hIMC = ImmGetContext(hwndEdit);
    ok(hIMC == hNewIMC, "\n");
    ok(hIMC != hOldIMC, "\n");
    pIC = ImmLockIMC(hNewIMC);
    ok(pIC != NULL, "\n");
    ok(pIC->hWnd == NULL, "\n");
    ok(ImmGetIMCCSize(pIC->hCompStr) != 0, "\n");
    ok(ImmGetIMCCSize(pIC->hCandInfo) != 0, "\n");
    ok(ImmGetIMCCSize(pIC->hGuideLine) != 0, "\n");
    ok(ImmGetIMCCSize(pIC->hPrivate) != 0, "\n");
    ok(ImmGetIMCCSize(pIC->hMsgBuf) != 0, "\n");
    ImmUnlockIMC(hNewIMC);
    ok(ImmReleaseContext(hwndEdit, hIMC), "\n");
    ok(ImmDestroyContext(hNewIMC), "\n");

    DestroyWindow(hwndEdit);
    DestroyWindow(hwndStatic);
}
