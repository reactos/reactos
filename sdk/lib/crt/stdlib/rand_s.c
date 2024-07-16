/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     rand_s implementation
 * COPYRIGHT:   Copyright 2010 Sylvain Petreolle <spetreolle@yahoo.fr>
 *              Copyright 2015 Christoph von Wittich <christoph_vw@reactos.org>
 *              Copyright 2015 Pierre Schweitzer <pierre@reactos.org>
 */

#include <precomp.h>

typedef BOOLEAN (WINAPI *PFN_SystemFunction036)(PVOID, ULONG); // RtlGenRandom
PFN_SystemFunction036 g_pfnSystemFunction036 = NULL;

/*********************************************************************
 *              rand_s (MSVCRT.@)
 */
int CDECL rand_s(unsigned int *pval)
{
    HINSTANCE hadvapi32;

    if (!pval)
    {
        _invalid_parameter(NULL,_CRT_WIDE("rand_s"),_CRT_WIDE(__FILE__),__LINE__, 0);
        *_errno() = EINVAL;
        return EINVAL;
    }

    *pval = 0;

    if (g_pfnSystemFunction036 == NULL)
    {
        PFN_SystemFunction036 pSystemFunction036;

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

        g_pfnSystemFunction036 = pSystemFunction036;
    }

    if (!g_pfnSystemFunction036(pval, sizeof(*pval)))
    {
        _invalid_parameter(NULL,_CRT_WIDE("rand_s"),_CRT_WIDE(__FILE__),__LINE__, 0);
        *_errno() = EINVAL;
        return EINVAL;
    }

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
