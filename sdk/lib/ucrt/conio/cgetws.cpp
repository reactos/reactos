//
// cgetws.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _cgetws() and _cgetws_s(), which read a wide string from the console.
//
#include <conio.h>
#include <corecrt_internal_lowio.h>
#include <corecrt_internal_securecrt.h>
#include <stdlib.h>

// Use of the following buffer variables is primarily for syncronizing with
// _cgets_s. _cget_s fills the MBCS buffer and if the user passes in single
// character buffer and the unicode character is not converted to single byte
// MBC, then _cget_s should buffer that character so that next call to
// _cgetws_s can return the same character.
extern "C" { wchar_t __console_wchar_buffer      = 0; }
extern "C" { int     __console_wchar_buffer_used = 0; }



// Reads a string from the console; always null-terminates the buffer.  Returns
// 0 on success; returns an errno error code on failure.
extern "C" errno_t __cdecl _cgetws_s(wchar_t* const string_buffer, size_t const size_in_words, size_t* const size_read)
{
    _VALIDATE_CLEAR_OSSERR_RETURN_ERRCODE(string_buffer != nullptr, EINVAL);
    _VALIDATE_CLEAR_OSSERR_RETURN_ERRCODE(size_in_words > 0, EINVAL);
    _RESET_STRING(string_buffer, size_in_words);

    _VALIDATE_CLEAR_OSSERR_RETURN_ERRCODE(size_read != nullptr, EINVAL);

    __acrt_lock(__acrt_conio_lock);
    errno_t retval = 0;
    __try
    {
        wchar_t* string = string_buffer;

        // We need to decrement size_in_words because ReadConsole reads as many
        // characters as the parameter passed.  It doesn't null-terminate:
        size_t size_remaining = size_in_words - 1;
        *size_read = 0;

        // If size_in_words was 1, then there's only room for the null terminator:
        if (size_remaining == 0)
            __leave;

        // If there is a buffered character, first fill with the buffered
        // character then proceed with reading from the console:
        if (__console_wchar_buffer_used != 0)
        {
            *string++ = __console_wchar_buffer;
            --size_remaining;
            (*size_read)++;

            if (__console_wchar_buffer == L'\0')
                size_remaining = 0;

            __console_wchar_buffer = 0;
        }

        if (size_remaining == 0)
            __leave;

        /*
        * __dcrt_lowio_console_input_handle, the handle to the console input, is created the first
        * time that either _getch() or _cgets() or _kbhit() is called.
        */

        if (__dcrt_lowio_ensure_console_input_initialized() == FALSE)
        {
            __acrt_errno_map_os_error(GetLastError());
            retval = errno;
            __leave;
        }

        ULONG old_state;
        __dcrt_get_input_console_mode(&old_state);
        __dcrt_set_input_console_mode(ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_ECHO_INPUT);

        ULONG wchars_read;
        BOOL read_console_result = __dcrt_read_console(
            string,
            static_cast<DWORD>(size_remaining),
            &wchars_read);

        if (!read_console_result)
        {
            __acrt_errno_map_os_error(GetLastError());
            retval = errno;
            __leave;
        }

        // Set the length of the string and null terminate it:
        if (wchars_read >= 2 && string[wchars_read - 2] == L'\r')
        {
            *size_read += wchars_read - 2;
            string[wchars_read - 2] = L'\0';
        }
        else if (wchars_read == size_remaining && string[wchars_read - 1] == L'\r')
        {
            // Special case 1:  \r\n straddles the boundary:
            string[wchars_read - 1] = L'\0';
            *size_read += wchars_read - 1;
        }
        else if (wchars_read == 1 && string[0] == L'\n')
        {
            // Special case 2:  Read a single \n:
            string[0] = L'\0';
            *size_read += 0;
        }
        else
        {
            *size_read += wchars_read;
            string[wchars_read] = L'\0';
        }

        __dcrt_set_input_console_mode(old_state);
    }
    __finally
    {
        __acrt_unlock(__acrt_conio_lock);
    }
    __endtry

    return retval;
}



// Reads a string from the console via ReadConsole on a cooked console handle.
// string[0] must contain the maximum length of the string.  The number of
// characters written is stored in string[1].  The return value is a pointer to
// string[2] on success; nullptr on failure.
//
// Note that _cgetws() does NOT check the pushback character buffer, so it will
// not return any character that is pushed back by a call to _ungetwch().
extern "C" wchar_t* __cdecl _cgetws(_Inout_z_ wchar_t* const string)
{
    _VALIDATE_CLEAR_OSSERR_RETURN(string != nullptr, EINVAL, nullptr);
    _VALIDATE_CLEAR_OSSERR_RETURN(string[0] > 0,     EINVAL, nullptr);

    size_t const size_in_words = static_cast<size_t>(string[0]);

    size_t size_read = 0;
    // warning 26018: Potential overflow of null terminated buffer using expression string+2
    // Suppressing warning since _cgetws is purposefully unsafe.
#pragma warning(suppress:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED)
    errno_t const result = _cgetws_s(string + 2, size_in_words, &size_read);

    // warning 26018: Potential overflow of null terminated buffer using expression string[1]
    // Suppressing warning since _cgetws is purposefully unsafe.
#pragma warning(suppress:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED)
    string[1] = static_cast<wchar_t>(size_read);

    return result == 0 ? string + 2 : nullptr;
}
