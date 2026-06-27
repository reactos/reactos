//
// tlssup.c
//
//      Copyright (c) 2025 Timo Kreuzer
//
// TLS callback support
//
// SPDX-License-Identifier: MIT
//

#include <internal_shared.h>

// Dummy TLS undex
unsigned int _tls_index;

// The TLS data
_CRTALLOC(".tls") char *_tls_start = NULL;
_CRTALLOC(".tls$ZZZ") char *_tls_end = NULL;

// Describes the range of TLS callbacks.
_CRTALLOC(".CRT$XLA") PIMAGE_TLS_CALLBACK __xl_a = 0;
_CRTALLOC(".CRT$XLZ") PIMAGE_TLS_CALLBACK __xl_z = 0;

//
// The TLS directory.
// The linker will point the executables TLS data directory entry to this.
// Must be force-included with "#pragma comment(linker, "/INCLUDE:_tls_used")"
// or by referencing it from another compilation unit.
//
const IMAGE_TLS_DIRECTORY _tls_used =
{
    (ULONG_PTR)&_tls_start,
    (ULONG_PTR)&_tls_end,
    (ULONG_PTR)&_tls_index,
    (ULONG_PTR)(&__xl_a + 1),
    (ULONG)0,
    (ULONG)0
};
