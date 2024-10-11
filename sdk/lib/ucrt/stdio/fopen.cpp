//
// fopen.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Functions that open a file as a stdio stream.
//
#include <corecrt_internal_stdio.h>



// Opens the file named by 'file_name' as a stdio stream.  The 'mode' determines
// the mode in which the file is opened and the 'share_flag' determines the
// sharing mode.  Supported modes are "r" (read), "w" (write), "a" (append),
// "r+" (read and write), "w+" (open empty for read and write), and "a+" (read
// and append).  A "t" or "b" may be appended to the mode string to request text
// or binary mode, respectively.
//
// Returns the FILE* for the newly opened stream on success; returns nullptr on
// failure.
template <typename Character>
static FILE* __cdecl common_fsopen(
    Character const* const file_name,
    Character const* const mode,
    int              const share_flag
    ) throw()
{
    typedef __acrt_stdio_char_traits<Character> stdio_traits;

    _VALIDATE_RETURN(file_name != nullptr, EINVAL, nullptr);
    _VALIDATE_RETURN(mode != nullptr,      EINVAL, nullptr);
    _VALIDATE_RETURN(*mode != 0,           EINVAL, nullptr);

    // We deliberately don't hard-validate for empty strings here. All other
    // invalid path strings are treated as runtime errors by the inner code
    // in _open and openfile.  This is also the appropriate treatment here.
    // Since fopen is the primary access point for file strings it might be
    // subjected to direct user input and thus must be robust to that rather
    // than aborting. The CRT and OS do not provide any other path validator
    // (because Win32 doesn't allow such things to exist in full generality).
    _VALIDATE_RETURN_NOEXC(*file_name != 0, EINVAL, nullptr);

    // Obtain a free stream.  Note that the stream is returned locked:
    __crt_stdio_stream stream = __acrt_stdio_allocate_stream();
    if (!stream.valid())
    {
        errno = EMFILE;
        return nullptr;
    }

    FILE* return_value = nullptr;
    __try
    {
        return_value = stdio_traits::open_file(file_name, mode, share_flag, stream.public_stream());
    }
    __finally
    {
        if (return_value == nullptr)
            __acrt_stdio_free_stream(stream);

        stream.unlock();
    }
    __endtry

    return return_value;
}



// A "secure" version of fsopen, which sets the result and returns zero on
// success and an error code on failure.
template <typename Character>
static errno_t __cdecl common_fopen_s(
    FILE**           const result,
    Character const* const file_name,
    Character const* const mode
    ) throw()
{
    _VALIDATE_RETURN_ERRCODE(result != nullptr, EINVAL);

    *result = common_fsopen(file_name, mode, _SH_SECURE);
    if (*result == nullptr)
        return errno;

    return 0;
}



extern "C" FILE* __cdecl _fsopen(
    char const* const file,
    char const* const mode,
    int         const share_flag
    )
{
    return common_fsopen(file, mode, share_flag);
}

extern "C" FILE* __cdecl fopen(
    char const* const file,
    char const* const mode
    )
{
    return common_fsopen(file, mode, _SH_DENYNO);
}

extern "C" errno_t __cdecl fopen_s(
    FILE**      const result,
    char const* const file,
    char const* const mode
    )
{
    return common_fopen_s(result, file, mode);
}

extern "C" FILE* __cdecl _wfsopen(
    wchar_t const* const file,
    wchar_t const* const mode,
    int            const share_flag
    )
{
    return common_fsopen(file, mode, share_flag);
}

extern "C" FILE* __cdecl _wfopen(
    wchar_t const* const file,
    wchar_t const* const mode
    )
{
    return common_fsopen(file, mode, _SH_DENYNO);
}

extern "C" errno_t __cdecl _wfopen_s(
    FILE**         const result,
    wchar_t const* const file,
    wchar_t const* const mode
    )
{
    return common_fopen_s(result, file, mode);
}
