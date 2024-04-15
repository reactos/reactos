/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     rand_s implementation
 * COPYRIGHT:   Copyright 2010 Sylvain Petreolle <spetreolle@yahoo.fr>
 *              Copyright 2015 Christoph von Wittich <christoph_vw@reactos.org>
 *              Copyright 2015 Pierre Schweitzer <pierre@reactos.org>
 */

#include <precomp.h>

/*********************************************************************
 *              rand_s (MSVCRT.@)
 */
int CDECL rand_s(unsigned int *pval)
{
    BOOLEAN (WINAPI *pSystemFunction036)(PVOID, ULONG); // RtlGenRandom
    HINSTANCE hadvapi32;

    if (!pval)
    {
        _invalid_parameter(NULL,_CRT_WIDE("rand_s"),_CRT_WIDE(__FILE__),__LINE__, 0);
        *_errno() = EINVAL;
        return EINVAL;
    }

    *pval = 0;
    hadvapi32 = LoadLibraryA("advapi32.dll");
    if (!hadvapi32)
    {
        _invalid_parameter(NULL,_CRT_WIDE("rand_s"),_CRT_WIDE(__FILE__),__LINE__, 0);
        *_errno() = EINVAL;
        return EINVAL;
    }

    pSystemFunction036 = (void*)GetProcAddress(hadvapi32, "SystemFunction036");
    if (!pSystemFunction036)
    {
        _invalid_parameter(NULL,_CRT_WIDE("rand_s"),_CRT_WIDE(__FILE__),__LINE__, 0);
        *_errno() = EINVAL;
        FreeLibrary(hadvapi32);
        return EINVAL;
    }

    if (!pSystemFunction036(pval, sizeof(*pval)))
    {
        _invalid_parameter(NULL,_CRT_WIDE("rand_s"),_CRT_WIDE(__FILE__),__LINE__, 0);
        *_errno() = EINVAL;
        FreeLibrary(hadvapi32);
        return EINVAL;
    }

    FreeLibrary(hadvapi32);
    return 0;
}

// Small hack: import stub to allow GCC's stdc++ to link
#if defined(__GNUC__) && (DLL_EXPORT_VERSION < 0x600)
#ifdef WIN64
const void* __imp_rand_s = rand_s;
#else
const void* _imp_rand_s = rand_s;
#endif
#endif
