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
    ok_int(hNewIMC != NULL, TRUE);
    pIC = ImmLockIMC(hNewIMC);
    ok_int(pIC == NULL, TRUE);
    ImmUnlockIMC(hNewIMC);
    ok_int(ImmDestroyContext(hNewIMC), TRUE);

    /* ImmGetContext against NULL */
    hIMC = ImmGetContext(NULL);
    ok_int(hIMC == NULL, TRUE);

    /* Create EDIT control */
    style = ES_MULTILINE | ES_LEFT;
    hwndEdit = CreateWindowW(L"EDIT", NULL, style, 0, 0, 100, 20, NULL, NULL,
                             GetModuleHandleW(NULL), NULL);
    ok_int(hwndEdit != NULL, TRUE);

    /* Create STATIC control */
    style = SS_LEFT;
    hwndStatic = CreateWindowW(L"STATIC", NULL, style, 0, 30, 100, 20, NULL, NULL,
                               GetModuleHandleW(NULL), NULL);
    ok_int(hwndStatic != NULL, TRUE);

    /* ImmGetContext/ImmReleaseContext and ImmLockIMC/ImmUnlockIMC */
    hIMC1 = hIMC = ImmGetContext(hwndEdit);
    ok_int(hIMC != NULL, TRUE);
    pIC = ImmLockIMC(hIMC);
    ok_int(pIC != NULL, TRUE);
    ImmUnlockIMC(hNewIMC);
    ok_int(ImmReleaseContext(hwndEdit, hIMC), TRUE);

    hIMC2 = hIMC = ImmGetContext(hwndStatic);
    ok_int(hIMC != NULL, TRUE);
    pIC = ImmLockIMC(hIMC);
    ok_int(pIC != NULL, TRUE);
    ImmUnlockIMC(hNewIMC);
    ok_int(ImmReleaseContext(hwndEdit, hIMC), TRUE);

    ok_int(hIMC1 == hIMC2, TRUE);

    /* ImmAssociateContext */
    hNewIMC = ImmCreateContext();
    ok_int(hNewIMC != NULL, TRUE);
    hOldIMC = ImmAssociateContext(hwndEdit, hNewIMC);
    ok_int(hNewIMC != hOldIMC, TRUE);
    hIMC = ImmGetContext(hwndEdit);
    ok_int(hIMC == hNewIMC, TRUE);
    ok_int(hIMC != hOldIMC, TRUE);
    ok_int(ImmReleaseContext(hwndEdit, hIMC), TRUE);
    ok_int(ImmDestroyContext(hNewIMC), TRUE);

    DestroyWindow(hwndEdit);
    DestroyWindow(hwndStatic);
}
