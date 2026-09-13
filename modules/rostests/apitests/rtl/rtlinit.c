/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Initialization for RTL unit test
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <rtltests.h>

VOID NTAPI RtlpInitializeVectoredExceptionHandling(VOID);

VOID
NTAPI
RtlpInitialize(VOID)
{
#ifdef TEST_STATIC
    RtlpInitializeVectoredExceptionHandling();
#endif
}
