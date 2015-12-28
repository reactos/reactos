/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtUserGetTitleBarInfo
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtUserGetTitleBarInfo)
{
    HINSTANCE hinst = GetModuleHandle(NULL);
    HWND hWnd;
    TITLEBARINFO tbi;

    hWnd = CreateWindowA("BUTTON",
                         "Test",
                         BS_PUSHBUTTON | WS_VISIBLE,
                         0,
                         0,
                         50,
                         30,
                         NULL,
                         NULL,
                         hinst,
                         0);

    ASSERT(hWnd);

    /* FALSE case */
    /* no windows handle */
    TEST(NtUserGetTitleBarInfo(NULL, &tbi) == FALSE);
    /* no TITLEBARINFO struct */
    TEST(NtUserGetTitleBarInfo(hWnd, NULL) == FALSE);
    /* nothing */
    TEST(NtUserGetTitleBarInfo(NULL, NULL) == FALSE);
    /* wrong size */
    tbi.cbSize = 0;
    TEST(NtUserGetTitleBarInfo(hWnd, &tbi) == FALSE);

    /* TRUE case */
    tbi.cbSize = sizeof(TITLEBARINFO);
    TEST(NtUserGetTitleBarInfo(hWnd, &tbi) == TRUE);

    DestroyWindow(hWnd);

}

