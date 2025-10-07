/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite power IRP management test user-mode part
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>
#include "PoIrp.h"

START_TEST(PoIrp)
{
    DWORD Error;

#ifdef _M_AMD64
    if (skip(FALSE, "ROSTESTS-368: Skipping kmtest:PoIrp because it crashes on Windows Server 2003 x64-Testbot.\n"))
#else
    if (skip(GetNTVersion() < _WIN32_WINNT_VISTA, "kmtest:PoIrp is broken on Vista+.\n"))
#endif
        return;

    Error = KmtLoadAndOpenDriver(L"PoIrp", TRUE);
    ok_eq_int(Error, ERROR_SUCCESS);
    if (Error)
        return;

    KmtSendToDriver(IOCTL_RUN_TEST);
    KmtCloseDriver();
    KmtUnloadDriver();
}
