/*
 * PROJECT:     ReactOS C++ runtime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ARM64 C++ runtime aliases
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <kxarm64.h>

// The 64 bit names are the same as on x64

    // void __cdecl `eh vector constructor iterator'(void *,unsigned __int64,int,void (__cdecl*)(void *),void (__cdecl*)(void *))
    IMPORT |??_L@YAXPEAX_KHP6AX0@Z2@Z|, WEAK |?MSVCRTEX_eh_vector_constructor_iterator@@YAXPEAX_KHP6AX0@Z2@Z|

    // void __cdecl `eh vector constructor iterator'(void *,unsigned __int64,unsigned __int64,void (__cdecl*)(void *),void (__cdecl*)(void *))
    IMPORT |??_L@YAXPEAX_K1P6AX0@Z2@Z|, WEAK |?MSVCRTEX_eh_vector_constructor_iterator@@YAXPEAX_KHP6AX0@Z2@Z|

    // void __cdecl `eh vector destructor iterator'(void *,unsigned __int64,unsigned __int64,void (__cdecl*)(void *))
    IMPORT |??_M@YAXPEAX_K1P6AX0@Z@Z|, WEAK |?MSVCRTEX_eh_vector_destructor_iterator@@YAXPEAX_KHP6AX0@Z@Z|

    // void __cdecl `eh vector destructor iterator'(void *,unsigned __int64,int,void (__cdecl*)(void *))
    IMPORT |??_M@YAXPEAX_KHP6AX0@Z@Z|, WEAK |?MSVCRTEX_eh_vector_destructor_iterator@@YAXPEAX_KHP6AX0@Z@Z|

    // void __cdecl operator delete(void *,struct std::nothrow_t const &)
    IMPORT |??3@YAXPEAXAEBUnothrow_t@std@@@Z|, WEAK |??3@YAXPEAX@Z|

    // void __cdecl operator delete[](void *,struct std::nothrow_t const &)
    IMPORT |??_V@YAXPEAXAEBUnothrow_t@std@@@Z|, WEAK |??3@YAXPEAX@Z|

    // void __cdecl operator delete(void *,unsigned __int64)
    IMPORT |??3@YAXPEAX_K@Z|, WEAK |??3@YAXPEAX@Z|

    END
