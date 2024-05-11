//
// cgets.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _cgets() and _cgets_s(), which read a string from the console.
//
#include <conio.h>
#include <corecrt_internal_securecrt.h>
#include <stdlib.h>



extern "C" extern intptr_t __dcrt_lowio_console_input_handle;



// Use of the following buffer variables is primarily for syncronizing with
// _cgets_s. _cget_s fills the MBCS buffer and if the user passes in single
// character buffer and the unicode character is not converted to single byte
// MBC, then _cget_s should buffer that character so that next call to
// _cgetws_s can return the same character.
extern "C" extern wchar_t __console_wchar_buffer;
extern "C" extern int     __console_wchar_buffer_used;



// Reads a string from the console; always null-terminates the buffer.  Returns
// 0 on success; returns an errno error code on failure.
extern "C" errno_t __cdecl _cgets_s(char* const source_string, size_t const size_in_bytes, size_t* const size_read)
{
    _VALIDATE_CLEAR_OSSERR_RETURN_ERRCODE(source_string != nullptr, EINVAL);
    _VALIDATE_CLEAR_OSSERR_RETURN_ERRCODE(size_in_bytes > 0,        EINVAL);
    _RESET_STRING(source_string, size_in_bytes);

    _VALIDATE_CLEAR_OSSERR_RETURN_ERRCODE(size_read != nullptr, EINVAL);

    errno_t error  = 0;
    char*   string = source_string;

    __acrt_lock(__acrt_conio_lock);
    __try
    {
        // The implementation of cgets is slightly tricky. The reason being,
        // the code page for console is different from the CRT code page.
        // What this means is the program may interpret character
        // differently from it's acctual value. To fix this, what we really
        // want to do is read the input as unicode string and then convert
        // it to proper MBCS representation.
        //
        // This fix this we are really converting from Unicode to MBCS.
        // This adds performance problem as we may endup doing this
        // character by character. The basic problem here is that we have
        // no way to know how many UNICODE characters will be needed to fit
        // them in given size of MBCS buffer. To fix this issue we will be
        // converting one Unicode character at a time to MBCS. This makes
        // this slow, but then this is already console input,
        *size_read = 0;

        size_t available = size_in_bytes - 1;
        do
        {
            wchar_t wchar_buff[2];
            size_t sizeRead = 0;

            error = _cgetws_s(wchar_buff, _countof(wchar_buff), &sizeRead);
            if (error != 0)
                break;

            if (wchar_buff[0] == '\0')
                break;

            int size_converted = 0;
            errno_t const wctomb_result = wctomb_s(&size_converted, string, available, wchar_buff[0]);
            if (wctomb_result != 0)
            {
                // Put the wide character back in the buffer so that the
                // unutilized wide character is still in the stream:
                __console_wchar_buffer = wchar_buff[0];
                __console_wchar_buffer_used = 1;
                break;
            }

            string     += size_converted;
            *size_read += size_converted;
            available  -= size_converted;
        }
        while (available > 0);
    }
    __finally
    {
        __acrt_unlock(__acrt_conio_lock);
    }

    *string++ = '\0';

    if (error != 0)
        errno = error;

    return error;
}



// Reads a string from the console via ReadConsole on a cooked console handle.
// string[0] must contain the maximum length of the string.  The number of
// characters written is stored in string[1].  The return value is a pointer to
// string[2] on success; nullptr on failure.
extern "C" char* __cdecl _cgets(_Inout_z_ char* const string)
{
    _VALIDATE_CLEAR_OSSERR_RETURN(string != nullptr, EINVAL, nullptr);
    _VALIDATE_CLEAR_OSSERR_RETURN(string[0] > 0,     EINVAL, nullptr);

    size_t const size_in_bytes = static_cast<size_t>(string[0]);

    size_t size_read = 0;
    // warning 26018: Potential overflow of null terminated buffer using expression string+2
    // Suppressing warning since _cgets is purposefully unsafe.
#pragma warning(suppress:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED)
    errno_t const result = _cgets_s(string + 2, size_in_bytes, &size_read);

    // warning 26018: Potential overflow of null terminated buffer using expression string[1]
    // Suppressing warning since _cgets is purposefully unsafe.
#pragma warning(suppress:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED)
    string[1] = static_cast<char>(size_read);

    return result == 0 ? string + 2 : nullptr;
}
