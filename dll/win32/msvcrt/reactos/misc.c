/*
 * PROJECT:     ReactOS msvcrt.dll
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Miscellaneous functions and data
 * COPYRIGHT:   Copyright Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdlib.h>
#include <errno.h>
#include <msvcrt.h>

extern unsigned int MSVCRT__winver;

unsigned char _mbcasemap[257] =
{
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 0x0, 0x0, 0x0, 0x0, 0x0,
};

int _fileinfo = -1;

unsigned char* __p__mbcasemap(void)
{
    return _mbcasemap;
}

int* __p__fileinfo(void)
{
    return &_fileinfo;
}

int _get_fileinfo(_Out_ int *_Value)
{
    if (!MSVCRT_CHECK_PMT(_Value != NULL))
    {
        return EINVAL;
    }

    *_Value = _fileinfo;
    return 0;
}

int _set_fileinfo(_In_ int _Value)
{
    _fileinfo = _Value;
    return 0;
}

errno_t
__cdecl
_get_winver(_Out_ unsigned int *_Value)
{
    if (!MSVCRT_CHECK_PMT(_Value != NULL))
    {
        return EINVAL;
    }

    *_Value = MSVCRT__winver;
    return 0;
}
