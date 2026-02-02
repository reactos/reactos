//
// section_markers.c
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Markers for CRT initializer sections.
//
// SPDX-License-Identifier: MIT
//

#include <internal_shared.h>

_CRTALLOC(".CRT$XIA") _PIFV __xi_a[] = { 0 };
_CRTALLOC(".CRT$XIZ") _PIFV __xi_z[] = { 0 };
_CRTALLOC(".CRT$XCA") _PVFV __xc_a[] = { 0 };
_CRTALLOC(".CRT$XCZ") _PVFV __xc_z[] = { 0 };
_CRTALLOC(".CRT$XPA") _PVFV __xp_a[] = { 0 };
_CRTALLOC(".CRT$XPZ") _PVFV __xp_z[] = { 0 };
_CRTALLOC(".CRT$XTA") _PVFV __xt_a[] = { 0 };
_CRTALLOC(".CRT$XTZ") _PVFV __xt_z[] = { 0 };

#pragma comment(linker, "/merge:.CRT=.rdata")
