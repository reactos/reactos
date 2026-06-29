/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _chkesp_failed
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <crtdbg.h>

void _chkesp_failed(void)
{
#ifdef _DEBUG
    /* Report the error to the user */
    _CrtDbgReport(_CRT_ERROR,
                  __FILE__,
                  __LINE__,
                  "",
                  "The stack pointer was invalid after a function call. "
                  "This indicates that a function was called with the "
                  "wrong parmeters or calling convention.\n"
                  "Click 'Retry' to debug the application.");
#endif

    __debugbreak();
}
