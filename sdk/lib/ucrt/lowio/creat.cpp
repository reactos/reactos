//
// creat.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _creat() (and _wcreat(), via inclusion), which creates a new file.
//
#include <corecrt_internal_lowio.h>



// Creates a new file.
//
// If the file does not exist, this function creates a new file with the given
// permission setting and opens it for writing.  If the file already exists and
// its permission allows writing, this function truncates it to zero length and
// opens it for writing.
//
// The only XENIX mode bit supported is user write (S_IWRITE).
//
// On success, the handle for the newly created file is returned.  On failure,
// -1 is returned and errno is set.
template <typename Character>
static int __cdecl common_creat(Character const* const path, int const pmode) throw()
{
    typedef __crt_char_traits<Character> traits;
    // creat is just the same as open...
    int fh = -1;
    errno_t e = traits::tsopen_s(&fh, path, _O_CREAT + _O_TRUNC + _O_RDWR, _SH_DENYNO, pmode);
    return e == 0 ? fh : -1;
}

extern "C" int __cdecl _creat(char const* const path, int const pmode)
{
    return common_creat(path, pmode);
}

extern "C" int __cdecl _wcreat(wchar_t const* const path, int const pmode)
{
    return common_creat(path, pmode);
}
