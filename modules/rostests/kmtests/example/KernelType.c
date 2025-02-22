/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Kernel-Mode Test Suite Kernel Type example test
 * COPYRIGHT:   Copyright 2011-2018 Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

START_TEST(KernelType)
{
    if (KmtIsMultiProcessorBuild)
        trace("This is a MultiProcessor kernel\n");
    else
        trace("This is a Uniprocessor kernel\n");
    if (KmtIsCheckedBuild)
        trace("This is a Checked kernel\n");
    else
        trace("This is a Free kernel\n");
}
