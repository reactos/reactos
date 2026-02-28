/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _get_purecall_handler, _set_purecall_handler and _purecall function
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdlib.h>
#include <intrin.h>

static volatile _purecall_handler purecall_handler;

extern "C"
_purecall_handler _cdecl _get_purecall_handler(void)
{
    return purecall_handler;
}

extern "C"
_purecall_handler _cdecl _set_purecall_handler(_In_opt_ _purecall_handler _Handler)
{
    return reinterpret_cast<_purecall_handler>(
        _InterlockedExchangePointer(reinterpret_cast<void* volatile*>(&purecall_handler),
                                    reinterpret_cast<void*>(_Handler)));
}

extern "C"
int __cdecl _purecall(void)
{
    _purecall_handler handler = purecall_handler;
    if (handler)
        handler();
    abort();
}
