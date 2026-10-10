#define _CRTIMP
#include <precomp.h>

errno_t CDECL wcscpy_s(wchar_t* wcDest, size_t numElement, const wchar_t *wcSrc)
{
    size_t size = 0;

    if(!MSVCRT_CHECK_PMT(wcDest)) return EINVAL;
    if(!MSVCRT_CHECK_PMT(numElement)) return EINVAL;

    if(!MSVCRT_CHECK_PMT(wcSrc))
    {
        wcDest[0] = 0;
        return EINVAL;
    }

    size = wcslen(wcSrc) + 1;

    if(!MSVCRT_CHECK_PMT_ERR(size <= numElement, ERANGE))
    {
        wcDest[0] = 0;
        return ERANGE;
    }

    memmove( wcDest, wcSrc, size*sizeof(WCHAR) );

    return 0;
}

#ifdef _WIN64
void* __imp_wcscpy_s = wcscpy_s;
#else
void* _imp__wcscpy_s = wcscpy_s;
#endif
