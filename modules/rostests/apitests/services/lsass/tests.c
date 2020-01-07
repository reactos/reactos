/*
 * PROJECT:     ReactOS lsass_apitest (remote)
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     tests
 * COPYRIGHT:   Copyright 2020 Andreas Maier (staubim@quantentunnel.de)
 */

#include <apitest.h>
#include <../framework/remotetests.h>

START_TEST(msv1_0)
{
    _RunRemoteTest("msv1_0");
}

