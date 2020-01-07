/*
 * PROJECT:     ReactOS lsass_apitest (remote)
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     testlist
 * COPYRIGHT:   Copyright 2020 Andreas Maier (staubim@quantentunnel.de)
 */

extern void func_service(void);
extern void func_msv1_0(void);

const struct test winetest_testlist[] =
{
    { "service", func_service },
    { "msv1_0", func_msv1_0 },

    { 0, 0 }
};
