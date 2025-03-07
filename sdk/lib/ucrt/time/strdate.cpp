//
// strdate.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The strdate() family of functions, which return the current data as a string.
//
#include <corecrt_internal_securecrt.h>
#include <corecrt_internal_time.h>


// Returns the current date as a string of the form "MM/DD/YY".  These functions
// return an error code on failure, or zero on success.  The buffer must be at
// least nine characters in size.
template <typename Character>
static errno_t __cdecl common_strdate_s(
    _Out_writes_z_(size_in_chars)   Character*  const   buffer,
    _In_  _In_range_(>=, 9)         size_t      const   size_in_chars
    ) throw()
{
    _VALIDATE_RETURN_ERRCODE(buffer != nullptr && size_in_chars > 0, EINVAL);
    _RESET_STRING(buffer, size_in_chars);
    _VALIDATE_RETURN_ERRCODE(size_in_chars >= 9, ERANGE);

    SYSTEMTIME local_time;
    GetLocalTime(&local_time);

    int const month = local_time.wMonth;
    int const day   = local_time.wDay;
    int const year  = local_time.wYear % 100;

    static Character const zero_char = static_cast<Character>('0');

#pragma warning(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY) // 26015

    // Store the components of the date into the string in MM/DD/YY form:
    buffer[0] = static_cast<Character>(month / 10 + zero_char); // Tens of month
    buffer[1] = static_cast<Character>(month % 10 + zero_char); // Units of month
    buffer[2] = static_cast<Character>('/');
    buffer[3] = static_cast<Character>(day   / 10 + zero_char); // Tens of day
    buffer[4] = static_cast<Character>(day   % 10 + zero_char); // Units of day
    buffer[5] = static_cast<Character>('/');
    buffer[6] = static_cast<Character>(year  / 10 + zero_char); // Tens of year
    buffer[7] = static_cast<Character>(year  % 10 + zero_char); // Units of year
    buffer[8] = static_cast<Character>('\0');

    return 0;
}

extern "C" errno_t __cdecl _strdate_s(char* const buffer, size_t const size_in_chars)
{
    return common_strdate_s(buffer, size_in_chars);
}

extern "C" errno_t __cdecl _wstrdate_s(wchar_t* const buffer, size_t const size_in_chars)
{
    return common_strdate_s(buffer, size_in_chars);
}



// Returns the current date as a string of the form "MM/DD/YY".  These functions
// assume that the provided result buffer is at least nine characters in length.
// Each returns the buffer on success, or null on failure.
template <typename Character>
_Success_(return != 0)
static Character* __cdecl common_strdate(_Out_writes_z_(9) Character* const buffer) throw()
{
    errno_t const status = common_strdate_s(buffer, 9);
    if (status != 0)
        return nullptr;

    return buffer;
}

extern "C" char* __cdecl _strdate(char* const buffer)
{
    return common_strdate(buffer);
}

extern "C" wchar_t* __cdecl _wstrdate(wchar_t* const buffer)
{
    return common_strdate(buffer);
}
