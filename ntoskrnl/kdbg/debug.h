/*
 * PROJECT:     ReactOS KDBG Kernel Debugger
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Useful debugging macros
 * COPYRIGHT:   Copyright 2023 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/*
 * NOTE: Define NDEBUG before including this header
 * to disable debugging macros.
 */

#pragma once

#ifndef __RELFILE__
#define __RELFILE__ __FILE__
#endif

/* Print stuff only on Debug Builds */
#if DBG

    /* These are always printed */
    #define DPRINT1(fmt, ...)   \
        KdbPrintf("(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__)

    /* These are printed only if NDEBUG is NOT defined */
    #ifndef NDEBUG
        #define DPRINT(fmt, ...) \
            KdbPrintf("(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__)
    #else
#if defined(_MSC_VER)
        #define DPRINT  __noop
#else
        #define DPRINT(...) do { if(0) { KdbPrintf(__VA_ARGS__); } } while(0)
#endif
    #endif

#else /* not DBG */

    /* On non-debug builds, we never show these */
#if defined(_MSC_VER)
    #define DPRINT1 __noop
    #define DPRINT  __noop
#else
    #define DPRINT1(...) do { if(0) { KdbPrintf(__VA_ARGS__); } } while(0)
    #define DPRINT(...) do { if(0) { KdbPrintf(__VA_ARGS__); } } while(0)
#endif /* _MSC_VER */

#endif /* not DBG */

/* EOF */
