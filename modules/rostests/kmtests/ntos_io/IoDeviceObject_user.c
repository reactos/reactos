/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Kernel-Mode Test Suite Device Object test user-mode part
 * COPYRIGHT:   Copyright 2011-2023 Thomas Faber (thomas.faber@reactos.org)
 * COPYRIGHT:   Copyright 2022 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <kmt_test.h>

START_TEST(IoDeviceObject)
{
    DWORD Error;

    /* make sure IoHelper has an existing service key, but is not started */
    Error = KmtLoadDriver(L"IoHelper", FALSE);
    ok_eq_int(Error, ERROR_SUCCESS);
    if (Error)
        return;
    /* Can't use the normal unload function here because we need the
     * service to stick around. */
    KmtUnloadDriverKeepService();

    Error = KmtLoadAndOpenDriver(L"IoDeviceObject", TRUE);
    ok_eq_int(Error, ERROR_SUCCESS);
    if (Error)
        return;

    KmtCloseDriver();
    KmtUnloadDriver();
}
