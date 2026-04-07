/*
 * PROJECT:     GCC C++ support library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     wctob/btowc/wcrtomb/mbrtowc shims for GCC 15 libstdc++
 * COPYRIGHT:   Copyright 2026 ReactOS Team
 *
 * GCC 15's libstdc++ references wctob, btowc (version-gated to >= 0x600),
 * wcrtomb, mbrtowc (version-gated to >= 0x600), lseek64, fstat64 (POSIX
 * names not in msvcrt), and __msvcrt_assert (UCRT64 libmingwex convention).
 *
 * Note: wctype is now exported by msvcrt.spec unconditionally, so we no
 * longer provide it here.
 */

#include <stdlib.h>
#include <string.h>

#undef wctob
#undef btowc
#undef wcrtomb
#undef mbrtowc

int __cdecl wctob(unsigned int c)
{
    char buf;
    if (wctomb(&buf, (wchar_t)c) == 1)
        return (unsigned char)buf;
    return -1; /* EOF */
}

unsigned int __cdecl btowc(int c)
{
    wchar_t wc;
    unsigned char uc;

    if (c == -1)
        return (unsigned int)-1; /* WEOF */

    uc = (unsigned char)c;
    if (mbtowc(&wc, (const char *)&uc, 1) == 1)
        return (unsigned int)wc;
    return (unsigned int)-1; /* WEOF */
}

/*
 * wcrtomb — convert wide char to multibyte (restartable).
 * ReactOS CRT only supports single-byte locales, so mbstate_t is unused.
 */
size_t __cdecl wcrtomb(char *s, wchar_t wc, void *ps)
{
    (void)ps;

    if (!s)
        return 1; /* stateless encoding: reset is a no-op, returns 1 */

    return (size_t)wctomb(s, wc);
}

/*
 * mbrtowc — convert multibyte to wide char (restartable).
 */
size_t __cdecl mbrtowc(wchar_t *pwc, const char *s, size_t n, void *ps)
{
    int ret;
    (void)ps;

    if (!s)
        return 0; /* stateless encoding */

    if (n == 0)
        return (size_t)-2;

    ret = mbtowc(pwc, s, n);
    if (ret < 0)
        return (size_t)-1;
    return (size_t)ret;
}

/*
 * lseek64 / fstat64 — POSIX names that GCC 15 libstdc++ basic_file.o expects.
 * Map to the MSVC CRT underscore-prefixed versions.
 */
long long __cdecl _lseeki64(long fd, long long offset, long origin);
long long __cdecl lseek64(long fd, long long offset, long origin)
{
    return _lseeki64(fd, offset, origin);
}

/* _fstat64 takes (int fd, struct _stat64 *buf). We just forward as raw bytes. */
int __cdecl _fstat64(long fd, void *buf);
int __cdecl fstat64(long fd, void *buf)
{
    return _fstat64(fd, buf);
}

/*
 * UCRT64's libmingwex _assert.o references __imp___msvcrt_assert.
 * ReactOS msvcrt exports _assert; provide the __msvcrt_ alias.
 */
void __cdecl _assert(const char *expr, const char *file, unsigned int line);

void __cdecl __msvcrt_assert(const char *expr, const char *file, unsigned int line)
{
    _assert(expr, file, line);
}

#ifdef _M_IX86
void *_imp__wctob = wctob;
void *_imp__btowc = btowc;
void *_imp__wcrtomb = wcrtomb;
void *_imp__mbrtowc = mbrtowc;
void *_imp____msvcrt_assert = __msvcrt_assert;
#else
void *__imp_wctob = wctob;
void *__imp_btowc = btowc;
void *__imp_wcrtomb = wcrtomb;
void *__imp_mbrtowc = mbrtowc;
void *__imp___msvcrt_assert = __msvcrt_assert;
#endif
