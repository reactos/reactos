//
// fgets.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Functions that read a string from a file.
//
#include <corecrt_internal_stdio.h>



// Reads a string from a stream.  This function reads a string, up to 'count - 1'
// characters in length, or until a '\n', whichever is reached first.  The string
// is always null-terminated on return.  The '\n' _is_ written to the string if
// it is encountered before space is exhausted.  If EOF is encountered immediately,
// null is returned.  If EOF is encountered after some characters are read, EOF
// terminates input just as '\n' would.
//
// Returns null if the count is nonpositive or if EOF is encountered immediately.
// Otherwise, returns the string.
template <typename Character>
_Success_(return != 0)
static Character* __cdecl common_fgets(
    _Out_writes_z_(count) Character*    const string,
    int                                 const count,
    __crt_stdio_stream                  const stream
    ) throw()
{
    typedef __acrt_stdio_char_traits<Character> stdio_traits;

    _VALIDATE_RETURN(string != nullptr || count == 0, EINVAL, nullptr);
    _VALIDATE_RETURN(count >= 0,                      EINVAL, nullptr);
    _VALIDATE_RETURN(stream.valid(),                  EINVAL, nullptr);

    if (count == 0)
        return nullptr;

    Character* return_value = nullptr;

    _lock_file(stream.public_stream());
    __try
    {
        if (!stdio_traits::validate_stream_is_ansi_if_required(stream.public_stream()))
            __leave;

        // Note that we start iterating at 1, so we read at most 'count - 1'
        // characters from the stream, leaving room for the null terminator:
        Character* it = string;
        for (int i = 1; i != count; ++i)
        {
            int const c = stdio_traits::getc_nolock(stream.public_stream());
            if (c == stdio_traits::eof)
            {
                // If we immediately reach EOF before reading any characters,
                // the C Language Standard mandates that the input buffer should
                // be left unmodified, so we return immediately, without writing
                // anything to the buffer:
                if (it == string)
                    __leave;

                // Otherwise, when we reach EOF, we just need to stop iterating:
                break;
            }

            // We stop reading when we reach a newline.  We do copy the newline:
            *it++ = static_cast<Character>(c);
            if (static_cast<Character>(c) == '\n')
                break;
        }

        *it = '\0';
        return_value = string;
    }
    __finally
    {
        _unlock_file(stream.public_stream());
    }
    __endtry

    return return_value;
}



extern "C" char* __cdecl fgets(
    char* const string,
    int   const count,
    FILE* const stream
    )
{
    return common_fgets(string, count, __crt_stdio_stream(stream));
}

extern "C" wchar_t* __cdecl fgetws(
    wchar_t* const string,
    int      const count,
    FILE*    const stream
    )
{
    return common_fgets(string, count, __crt_stdio_stream(stream));
}
