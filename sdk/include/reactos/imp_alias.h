/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Helpers to define dllimport slots bound to static functions
 * COPYRIGHT:   Copyright 2026 Ahmed Arif <arif.ing@outlook.com>
 */

#pragma once

/*
 * __imp_<sym> carries <sym>'s platform decoration: on i386 cdecl foo gets __imp__foo, one-argument stdcall
 * bar gets __imp__bar@4. Spell the slot names through asm labels, a plain C variable would be decorated
 * again. The bound-to function is referenced through an asm label as well, so the macros work no matter
 * how (or whether) the surrounding TU declares it.
 */

#if defined(__i386__)
#define IMP_TARGET_CDECL(name)         "_" #name
#define IMP_SYMBOL_CDECL(name)         "__imp__" #name
#define IMP_SYMBOL_STDCALL(name, size) "__imp__" #name "@" #size
#else
#define IMP_TARGET_CDECL(name)         #name
#define IMP_SYMBOL_CDECL(name)         "__imp_" #name
#define IMP_SYMBOL_STDCALL(name, size) "__imp_" #name
#endif

/* Slot for a cdecl function, bound to the function of the same name */
#define IMP_ALIAS_CDECL(name) \
    extern char __imp_alias_target_##name[] __asm__(IMP_TARGET_CDECL(name)); \
    const void *__imp_alias_##name __asm__(IMP_SYMBOL_CDECL(name)) = \
        (const void *)&__imp_alias_target_##name

/* Slot for a stdcall function with `size` argument bytes, bound to an ABI-compatible target */
#define IMP_ALIAS_STDCALL(name, size, target) \
    const void *__imp_alias_##name __asm__(IMP_SYMBOL_STDCALL(name, size)) = \
        (const void *)&target
