/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtUserUpdatePerUserSystemParameters
 * PROGRAMMERS:
 */

#include <win32nt.h>



DWORD WINAPI
UpdatePerUserSystemParameters(DWORD dw1, DWORD dw2);

START_TEST(NtUserUpdatePerUserSystemParameters)
{
    BOOL bScrRd;

    TEST(NtUserUpdatePerUserSystemParameters(0, 0) == 0);
    TEST(NtUserUpdatePerUserSystemParameters(0, 1) == 0);
    TEST(NtUserUpdatePerUserSystemParameters(1, 0) == 0);
    TEST(NtUserUpdatePerUserSystemParameters(1, 1) == 0);
    TEST(NtUserUpdatePerUserSystemParameters(0, 2) == 0);
    TEST(NtUserUpdatePerUserSystemParameters(0, -1) == 0);

//    NtUserSystemParametersInfo(SPI_SETSCREENREADER, 1, 0, 0);
    NtUserSystemParametersInfo(SPI_GETSCREENREADER, 0, &bScrRd, 0);
    printf("bScrRd = %d\n", bScrRd);

}
