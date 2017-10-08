#pragma once

#include <stdio.h>
#include <conio.h>
#include <float.h>
#include <locale.h>
#include <internal/locale.h>

#undef NtCurrentPeb
#define NtCurrentPeb() (NtCurrentTeb()->Peb)
#define GetProcessHeap() (NtCurrentPeb()->ProcessHeap)

#define HeapAlloc(_Heap, _Flags, _Size) RtlAllocateHeap(_Heap, _Flags, _Size)
#define HeapFree(_Heap, _Flags, _Ptr) RtlFreeHeap(_Heap, _Flags, _Ptr)

#ifdef _LIBCNT_
static inline unsigned int __control87(unsigned int new, unsigned int mask)
{
    return 0;
}
#define _control87 __control87
extern threadlocinfo _LIBCNT_locinfo;
#define get_locinfo() (&_LIBCNT_locinfo)
#else
#define get_locinfo() ((pthreadlocinfo)get_locinfo())
#endif

void
__declspec(noinline)
_internal_handle_float(
    int negative,
    int exp,
    int suppress,
    ULONGLONG d,
    int l_or_L_prefix,
    va_list *ap);

//#include <debug.h>

#define __WINE_DEBUG_H
#undef WINE_DEFAULT_DEBUG_CHANNEL
#define WINE_DEFAULT_DEBUG_CHANNEL(_Ch)
#undef TRACE
#define TRACE(...) /* DPRINT(__VA_ARGS__) */
#define debugstr_a(format) format
