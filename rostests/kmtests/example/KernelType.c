/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Kernel Type example test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
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
