/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for SetConsoleWindowInfo
 * PROGRAMMER:      Hermes Belusca-Maito
 */

#include <apitest.h>
#include <wincon.h>

START_TEST(SetConsoleWindowInfo)
{
    /*
     * The aim of this test is to show that what MSDN says about the validity
     * checks performed on the window size rect given to SetConsoleWindowInfo
     * is partially wrong.
     *
     * Indeed, while it is claimed that:
     *   "The function fails if the specified window rectangle extends beyond
     *    the boundaries of the console screen buffer. This means that the Top
     *    and Left members of the lpConsoleWindow rectangle (or the calculated
     *    top and left coordinates, if bAbsolute is FALSE) cannot be less than
     *    zero. Similarly, the Bottom and Right members (or the calculated
     *    bottom and right coordinates) cannot be greater than (screen buffer
     *    height – 1) and (screen buffer width – 1), respectively. The function
     *    also fails if the Right member (or calculated right coordinate) is
     *    less than or equal to the Left member (or calculated left coordinate)
     *    or if the Bottom member (or calculated bottom coordinate) is less than
     *    or equal to the Top member (or calculated top coordinate)."
     *
     * the really performed tests are fewer, and it appears that the console
     * subsystem knows how to take proper actions when the window size rect
     * has e.g. negative left/top coordinates...
     *
     * NOTE that we all perform those tests in "absolute mode" (second parameter
     * of SetConsoleWindowInfo being TRUE), so that the specified window size rect
     * is in absolute coordinates (i.e. relative to the console screen buffer),
     * and not in coordinates relative to the current window-corner coordinates.
     */

    BOOL Success;
    DWORD dwLastError;
    HANDLE hConOut;
    COORD Resolution;
    CONSOLE_SCREEN_BUFFER_INFO csbi, csbi2;
    SMALL_RECT ConRect;

    /* First, retrieve a handle to the real console output, even if we are redirected */
    hConOut = CreateFileW(L"CONOUT$", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hConOut != INVALID_HANDLE_VALUE, "Opening ConOut\n");
    if (hConOut == INVALID_HANDLE_VALUE)
        return; // We cannot run this test if we failed...

    /* Retrieve console screen buffer info */
    Success = GetConsoleScreenBufferInfo(hConOut, &csbi);
    ok(Success, "Getting SB info\n");
    if (!Success)
        goto Cleanup; // We cannot as well run this test if we failed...

    /*
     * Set the console screen buffer to a correct size
     * that should not completely fill the computer screen.
     */
    Resolution.X = 80;
    Resolution.Y = 25;
    if (Resolution.X != csbi.dwSize.X || Resolution.Y != csbi.dwSize.Y)
    {
        SHORT oldWidth, oldHeight;

        oldWidth  = csbi.srWindow.Right  - csbi.srWindow.Left + 1;
        oldHeight = csbi.srWindow.Bottom - csbi.srWindow.Top  + 1;

        /*
         * If the current console window is too large for
         * the new screen buffer, resize it first.
         */
        if (oldWidth > Resolution.X || oldHeight > Resolution.Y)
        {
            ConRect.Left   = ConRect.Top = 0;
            ConRect.Right  = ConRect.Left + min(oldWidth , Resolution.X) - 1;
            ConRect.Bottom = ConRect.Top  + min(oldHeight, Resolution.Y) - 1;
            Success = SetConsoleWindowInfo(hConOut, TRUE, &ConRect);
            ok(Success, "Setting console wnd info failed with last error error %lu\n", GetLastError());
        }

        /* Now resize the screen buffer */
        Success = SetConsoleScreenBufferSize(hConOut, Resolution);
        ok(Success, "Setting console SB size failed with last error error %lu\n", GetLastError());

        /*
         * Setting a new screen buffer size can change other information,
         * so update the saved console information.
         */
        GetConsoleScreenBufferInfo(hConOut, &csbi);
    }

    /* Update console screen buffer info */
    Success = GetConsoleScreenBufferInfo(hConOut, &csbi);
    ok(Success, "Getting SB info\n");
    if (!Success)
        goto Cleanup; // We cannot as well run this test if we failed...

    /* Test 1: Resize the console window to its possible maximum size (should succeed) */
    ConRect.Left   = ConRect.Top = 0;
    ConRect.Right  = ConRect.Left + min(csbi.dwSize.X, csbi.dwMaximumWindowSize.X) - 1;
    ConRect.Bottom = ConRect.Top  + min(csbi.dwSize.Y, csbi.dwMaximumWindowSize.Y) - 1;
    SetLastError(0xdeadbeef);
    Success = SetConsoleWindowInfo(hConOut, TRUE, &ConRect);
    dwLastError = GetLastError();
    ok(Success, "Setting console wnd info\n");
    ok(dwLastError != ERROR_INVALID_PARAMETER, "GetLastError: %lu\n", dwLastError);

    /* Test 2: Set Right/Bottom members smaller than Left/Top members
     * (should fail, agrees with MSDN) */
    ConRect.Left   = csbi.dwSize.X - 5;
    ConRect.Right  = 0;
    ConRect.Top    = csbi.dwSize.Y - 5;
    ConRect.Bottom = 0;
    SetLastError(0xdeadbeef);
    Success = SetConsoleWindowInfo(hConOut, TRUE, &ConRect);
    dwLastError = GetLastError();
    ok(!Success, "Setting console wnd info should have failed!\n");
    ok(dwLastError == ERROR_INVALID_PARAMETER, "GetLastError: expecting %u got %lu\n",
       ERROR_INVALID_PARAMETER, dwLastError);

    /* Test 3: Set negative Left/Top members, but correct Right/Bottom ones.
     * The Left/Top members are shifted to zero while the Right/Bottom ones
     * are shifted too in accordance.
     * 1st situation where the Right/Bottom members will be ok after the shift
     * (should succeed, disagrees with MSDN) */
    ConRect.Left   = ConRect.Top = -5;
    ConRect.Right  = csbi.dwSize.X - 7;
    ConRect.Bottom = csbi.dwSize.Y - 7;
    // Expected result: ConRect.Left == ConRect.Top == 0 and
    // ConRect.Right == csbi.dwSize.X - 2, ConRect.Bottom == csbi.dwSize.Y - 2;
    SetLastError(0xdeadbeef);
    Success = SetConsoleWindowInfo(hConOut, TRUE, &ConRect);
    dwLastError = GetLastError();
    ok(Success, "Setting console wnd info should have succeeded!\n");
    ok(dwLastError != ERROR_INVALID_PARAMETER, "GetLastError: %lu\n", dwLastError);

    /* Check the new reported window size rect */
    Success = GetConsoleScreenBufferInfo(hConOut, &csbi2);
    ok(Success, "Getting SB info\n");
    if (Success)
    {
        ok(csbi2.srWindow.Left == 0, "srWindow.Left = %d, expected 0\n", csbi2.srWindow.Left);
        ok(csbi2.srWindow.Top == 0, "srWindow.Top = %d, expected 0\n", csbi2.srWindow.Top);

        /* NOTE that here we compare against the old csbi data! */
        ok(csbi2.srWindow.Right == csbi.dwSize.X - 2, "srWindow.Right = %d, expected %d\n",
           csbi2.srWindow.Right,   csbi.dwSize.X - 2);
        ok(csbi2.srWindow.Bottom == csbi.dwSize.Y - 2, "srWindow.Bottom = %d, expected %d\n",
           csbi2.srWindow.Bottom,   csbi.dwSize.Y - 2);
    }

    /* Test 4: Similar to Test 3, but set the Right/Bottom members too large
     * with respect to the screen buffer size, so that after their shift, they
     * still are too large (should fail, agrees with MSDN) */
    ConRect.Left   = ConRect.Top = -5;
    ConRect.Right  = csbi.dwSize.X + 2; // Bigger than SB size
    ConRect.Bottom = csbi.dwSize.Y + 2; // Bigger than SB size
    SetLastError(0xdeadbeef);
    Success = SetConsoleWindowInfo(hConOut, TRUE, &ConRect);
    dwLastError = GetLastError();
    ok(!Success, "Setting console wnd info should have failed!\n");
    ok(dwLastError == ERROR_INVALID_PARAMETER, "GetLastError: expecting %u got %lu\n",
       ERROR_INVALID_PARAMETER, dwLastError);

    /* Check the new reported window size rect */
    Success = GetConsoleScreenBufferInfo(hConOut, &csbi2);
    ok(Success, "Getting SB info\n");
    if (Success)
    {
        /* NOTE that here we compare against the old csbi data! */
        ok(csbi2.srWindow.Left == 0, "srWindow(2).Left = %d, expected equal to %d\n",
           csbi2.srWindow.Left, 0);
        ok(csbi2.srWindow.Top == 0, "srWindow(2).Top = %d, expected equal to %d\n",
           csbi2.srWindow.Top, 0);
        ok(csbi2.srWindow.Right == csbi.dwSize.X - 2, "srWindow(2).Right = %d, expected equal to %d\n",
           csbi2.srWindow.Right, csbi.dwSize.X - 2);
        ok(csbi2.srWindow.Bottom == csbi.dwSize.Y - 2, "srWindow(2).Bottom = %d, expected equal to %d\n",
           csbi2.srWindow.Bottom, csbi.dwSize.Y - 2);
    }

    /* Test 5: Similar to Tests 3 and 4, but we here just check what happens for
     * the Right/Bottom members when they are too large, without caring about the
     * Left/Top members (the latter being set to valid values this time)
     * (should fail, agrees with MSDN) */
    ConRect.Left   = ConRect.Top = 2; // OK
    ConRect.Right  = csbi.dwSize.X + 7; // Bigger than SB size
    ConRect.Bottom = csbi.dwSize.Y + 7; // Bigger than SB size
    SetLastError(0xdeadbeef);
    Success = SetConsoleWindowInfo(hConOut, TRUE, &ConRect);
    dwLastError = GetLastError();
    ok(!Success, "Setting console wnd info should have failed!\n");
    ok(dwLastError == ERROR_INVALID_PARAMETER, "GetLastError: expecting %u got %lu\n",
       ERROR_INVALID_PARAMETER, dwLastError);

    /* Check the new reported window size rect */
    Success = GetConsoleScreenBufferInfo(hConOut, &csbi2);
    ok(Success, "Getting SB info\n");
    if (Success)
    {
        ok(csbi2.srWindow.Left == 0, "srWindow.Left = %d, expected 0\n", csbi2.srWindow.Left);
        ok(csbi2.srWindow.Top == 0, "srWindow.Top = %d, expected 0\n", csbi2.srWindow.Top);

        /* NOTE that here we compare against the old csbi data! */
        ok(csbi2.srWindow.Right == csbi.dwSize.X - 2, "srWindow.Right = %d, expected %d\n",
           csbi2.srWindow.Right,   csbi.dwSize.X - 2);
        ok(csbi2.srWindow.Bottom == csbi.dwSize.Y - 2, "srWindow.Bottom = %d, expected %d\n",
           csbi2.srWindow.Bottom,   csbi.dwSize.Y - 2);
    }

    /* Done! */
Cleanup:
    CloseHandle(hConOut);
}
