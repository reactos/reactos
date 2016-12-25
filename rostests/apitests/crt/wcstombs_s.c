/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for wcstombs_s
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <stdio.h>
#include <stdlib.h>
#include <specstrings.h>

#define ok_errno(x) ok_hex(errno, (x))

#undef ok_char
#define ok_char(x,y) ok_int((unsigned)(unsigned char)(x),(unsigned)(unsigned char)(y))
#define ok_wchar(x,y) ok_int((unsigned)(unsigned short)(x),(unsigned)(unsigned short)(y))

_Check_return_wat_
_CRTIMP
errno_t
__cdecl
wcstombs_s(
    _Out_opt_ size_t * pcchConverted,
    _Out_writes_bytes_to_opt_(cjDstSize, *pcchConverted)
        char * pmbsDst,
    _In_ size_t cjDstSize,
    _In_z_ const wchar_t * pwszSrc,
    _In_ size_t cjMaxCount);


START_TEST(wcstombs_s)
{
    errno_t ret;
    size_t cchConverted;
    char mbsbuffer[10];

    *_errno() = 0;
    cchConverted = 0xf00bac;
    mbsbuffer[5] = 0xFF;
    ret = wcstombs_s(&cchConverted, mbsbuffer, 6, L"hallo", 5);
    ok_long(ret, 0);
    ok_size_t(cchConverted, 6);
    ok_char(mbsbuffer[5], 0);
    ok_str(mbsbuffer, "hallo");
    ok_errno(0);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    mbsbuffer[0] = 0xFF;
    ret = wcstombs_s(&cchConverted, mbsbuffer, 1, L"", 0);
    ok_long(ret, 0);
    ok_size_t(cchConverted, 1);
    ok_wchar(mbsbuffer[0], 0);
    ok_errno(0);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    mbsbuffer[0] = 0xFF;
    mbsbuffer[1] = 0xFF;
    mbsbuffer[2] = 0xFF;
    mbsbuffer[3] = 0xFF;
    mbsbuffer[4] = 0xFF;
    mbsbuffer[5] = 0xFF;
    ret = wcstombs_s(&cchConverted, mbsbuffer, 5, L"hallo", 5);
    ok_long(ret, ERANGE);
    ok_size_t(cchConverted, 0);
    ok_char(mbsbuffer[5], 0xFF);
    ok_char(mbsbuffer[4], L'o');
    ok_char(mbsbuffer[3], L'l');
    ok_char(mbsbuffer[2], L'l');
    ok_char(mbsbuffer[1], L'a');
    ok_char(mbsbuffer[0], 0);
    ok_errno(ERANGE);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    mbsbuffer[0] = 0xFF;
    mbsbuffer[1] = 0xFF;
    mbsbuffer[2] = 0xFF;
    mbsbuffer[3] = 0xFF;
    mbsbuffer[4] = 0xFF;
    mbsbuffer[5] = 0xFF;
    ret = wcstombs_s(&cchConverted, mbsbuffer, 3, L"hallo", 5);
    ok_long(ret, ERANGE);
    ok_size_t(cchConverted, 0);
    ok_char(mbsbuffer[5], 0xFF);
    ok_char(mbsbuffer[4], 0xFF);
    ok_char(mbsbuffer[3], 0xFF);
    ok_char(mbsbuffer[2], L'l');
    ok_char(mbsbuffer[1], L'a');
    ok_char(mbsbuffer[0], 0);
    ok_errno(ERANGE);

    *_errno() = 0;
    ret = wcstombs_s(0, 0, 0, 0, 0);
    ok_long(ret, EINVAL);
    ok_errno(EINVAL);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    ret = wcstombs_s(&cchConverted, 0, 0, 0, 0);
    ok_long(ret, EINVAL);
    ok_size_t(cchConverted, 0);
    ok_errno(EINVAL);

    *_errno() = 0;
    mbsbuffer[0] = L'x';
    ret = wcstombs_s(0, mbsbuffer, 0, 0, 0);
    ok_long(ret, EINVAL);
    ok_char(mbsbuffer[0], L'x');
    ok_errno(EINVAL);

    *_errno() = 0;
    ret = wcstombs_s(0, mbsbuffer, 10, L"hallo", 5);
    ok_long(ret, 0);
    ok_errno(0);

    *_errno() = 0;
    ret = wcstombs_s(0, mbsbuffer, 0, L"hallo", 5);
    ok_long(ret, EINVAL);
    ok_errno(EINVAL);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    ret = wcstombs_s(&cchConverted, 0, 10, L"hallo", 5);
    ok_long(ret, EINVAL);
    ok_size_t(cchConverted, 0xf00bac);
    ok_errno(EINVAL);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    ret = wcstombs_s(&cchConverted, 0, 0, L"hallo", 5);
    ok_long(ret, 0);
    ok_size_t(cchConverted, 6);
    ok_errno(0);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    ret = wcstombs_s(&cchConverted, mbsbuffer, 10, 0, 5);
    ok_long(ret, EINVAL);
    ok_size_t(cchConverted, 0);
    ok_errno(EINVAL);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    ret = wcstombs_s(&cchConverted, mbsbuffer, 10, L"hallo", 0);
    ok_long(ret, 0);
    ok_size_t(cchConverted, 1);
    ok_errno(0);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    mbsbuffer[0] = 0xAB;
    mbsbuffer[1] = 0xCD;
    mbsbuffer[2] = 0xAB;
    mbsbuffer[3] = 0xCD;
    mbsbuffer[4] = 0xAB;
    mbsbuffer[5] = 0xCD;
    ret = wcstombs_s(&cchConverted, mbsbuffer, 10, L"hallo", 2);
    ok_long(ret, 0);
    ok_size_t(cchConverted, 3);
    ok_char(mbsbuffer[5], 0xCD);
    ok_char(mbsbuffer[4], 0xAB);
    ok_char(mbsbuffer[3], 0xCD);
    ok_char(mbsbuffer[2], 0);
    ok_char(mbsbuffer[1], L'a');
    ok_char(mbsbuffer[0], L'h');
    ok_errno(0);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    ret = wcstombs_s(&cchConverted, mbsbuffer, 10, 0, 0);
    ok_long(ret, 0);
    ok_size_t(cchConverted, 1);
    ok_errno(0);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    ret = wcstombs_s(&cchConverted, mbsbuffer, 10, L"hallo", 7);
    ok_long(ret, 0);
    ok_size_t(cchConverted, 6);
    ok_errno(0);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    ret = wcstombs_s(&cchConverted, 0, 0, L"hallo", 7);
    ok_long(ret, 0);
    ok_size_t(cchConverted, 6);
    ok_errno(0);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    mbsbuffer[0] = 0xAB;
    mbsbuffer[1] = 0xCD;
    mbsbuffer[2] = 0xAB;
    mbsbuffer[3] = 0xCD;
    mbsbuffer[4] = 0xAB;
    mbsbuffer[5] = 0xCD;
    ret = wcstombs_s(&cchConverted, mbsbuffer, 5, L"hallo", _TRUNCATE);
    ok_long(ret, STRUNCATE);
    ok_size_t(cchConverted, 5);
    ok_char(mbsbuffer[5], 0xCD);
    ok_char(mbsbuffer[4], 0);
    ok_char(mbsbuffer[3], L'l');
    ok_char(mbsbuffer[2], L'l');
    ok_char(mbsbuffer[1], L'a');
    ok_char(mbsbuffer[0], L'h');
    ok_errno(0);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    ret = wcstombs_s(&cchConverted, mbsbuffer, 10, L"hallo", -1);
    ok_long(ret, 0);
    ok_size_t(cchConverted, 6);
    ok_errno(0);

}
