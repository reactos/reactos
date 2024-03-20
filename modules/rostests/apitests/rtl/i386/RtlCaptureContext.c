/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for x86 RtlCaptureContext
 * COPYRIGHT:   Copyright 2022 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <rtltests.h>

VOID
RtlCaptureContextWrapper(
    _Inout_ PCONTEXT InOutContext,
    _Out_ PCONTEXT CapturedContext);

START_TEST(RtlCaptureContext)
{
    // TODO
}
