//
// gets.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines gets() and gets_s(), which read a line from stdin.
//
#include <corecrt_internal_stdio.h>
#include <corecrt_internal_securecrt.h>



// Reads a line of text from stdin, storing it in the 'result' buffer.  This
// function supports two modes:
//
// [1] Insecure, which is used by the abominable gets() and _getws() functions.
//     In this mode, the buffer is not bounds-checked; it is just assumed that
//     the buffer is large enough.  Text is read until a newline is reached or
//     EOF is reached.  This mode is enabled by passing _CRT_UNBOUNDED_BUFFER_SIZE
//     as the buffer size.
//
// [2] Secure, which is used by the gets_s() and _getws_s() functions.  In this
//     mode, the buffer is bound-checked.  If there is insufficient space in the
//     buffer for the entire line of text, the line is read and discarded, the
//     buffer is zero'ed, and nullptr is returned.  If the buffer is larger than
//     is required, the remaining space of the buffer is zero-filled.
//
// On success, 'result' is returned.  On failure, nullptr is returned.  If the
// 'return_early_if_eof_is_first' flag is true and the first read encounters EOF,
// the buffer is left unchanged and nullptr is returned.
template <typename Character>
_Success_(return != nullptr)
static Character* __cdecl common_gets(
    _Out_writes_z_(result_size_in_characters) Character* const result,
    _In_                                      size_t     const result_size_in_characters,
    _In_                                      bool       const return_early_if_eof_is_first
    ) throw()
{
    typedef __acrt_stdio_char_traits<Character> stdio_traits;

    _VALIDATE_RETURN(result != nullptr,             EINVAL, nullptr);
    _VALIDATE_RETURN(result_size_in_characters > 0, EINVAL, nullptr);

    Character* return_value = result;

    _lock_file(stdin);
    __try
    {
        if (!stdio_traits::validate_stream_is_ansi_if_required(stdin))
        {
            return_value = nullptr;
            __leave;
        }

        // Special case:  if the first character is EOF, treat it specially if
        // we were asked to do so:
        typename stdio_traits::int_type c = stdio_traits::getc_nolock(stdin);
        if (c == stdio_traits::eof)
        {
            return_value = nullptr;
            if (return_early_if_eof_is_first)
                __leave;
        }

        // For the insecure version, we do no buffer size check and no debug fill:
        if (result_size_in_characters == _CRT_UNBOUNDED_BUFFER_SIZE)
        {
#pragma warning(push)
#pragma warning(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY) // 26015 - knowingly unsafe
            Character* result_it = result;
            while (c != '\n' && c != stdio_traits::eof)
            {
                *result_it++ = static_cast<Character>(c);
                c = stdio_traits::getc_nolock(stdin);
            }
            *result_it = '\0';
#pragma warning(pop)
        }
        // For the secure version, we track the buffer size.  If we run out of
        // buffer space, we still read in the rest of the current line until we
        // reach '\n' or EOF, but we discard the characters and reset the buffer
        // to be zero-filled.
        else
        {
            size_t available = result_size_in_characters;

            Character* result_it = result;
            while (c != '\n' && c != stdio_traits::eof)
            {
                if (available > 0)
                {
                    --available;
                    *result_it++ = static_cast<Character>(c);
                }

                c = stdio_traits::getc_nolock(stdin);
            }

            // If we ran out of space, clear the buffer and return failure:
            if (available == 0)
            {
                _RESET_STRING(result, result_size_in_characters);
                _RETURN_BUFFER_TOO_SMALL_ERROR(result, result_size_in_characters, nullptr);
            }

            *result_it = '\0';
            _FILL_STRING(result, result_size_in_characters, result_size_in_characters - available + 1);
        }
    }
    __finally
    {
        _unlock_file(stdin);
    }
    __endtry

    return return_value;
}



// Reads a line of text from stdin and stores it in the result buffer.  If the
// line is longer than will fit in the buffer, the line is discarded, the buffer
// is reset, and nullptr is returned.
extern "C" char* __cdecl gets_s(char* const result, size_t const result_size_in_characters)
{
    return common_gets(result, result_size_in_characters, false);
}

extern "C" wchar_t* __cdecl _getws_s(wchar_t* const result, size_t const result_size_in_characters)
{
    return common_gets(result, result_size_in_characters, false);
}



// Reads a line of text from stdin and stores it in the result buffer.  This
// function assumes there is sufficient room in the buffer.  Do not use this
// function; use gets_s(), fgets(), _getws_s(), or fgetws() instead.  If EOF
// is encountered on the first read from the stream, the buffer is left
// unmodified and nullptr is returned.
extern "C" char* __cdecl gets(char* const result)
{
    return common_gets(result, _CRT_UNBOUNDED_BUFFER_SIZE, true);
}

extern "C" wchar_t* __cdecl _getws(wchar_t* const result)
{
    return common_gets(result, _CRT_UNBOUNDED_BUFFER_SIZE, true);
}
