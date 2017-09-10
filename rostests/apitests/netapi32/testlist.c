/*
 * PROJECT:     ReactOS netapi32.dll API Tests
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Test list
 * COPYRIGHT:   Copyright 2017 Colin Finck <colin@reactos.org>
 */

#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_DsRoleGetPrimaryDomainInformation(void);

const struct test winetest_testlist[] =
{
    { "DsRoleGetPrimaryDomainInformation", func_DsRoleGetPrimaryDomainInformation },

    { 0, 0 }
};
