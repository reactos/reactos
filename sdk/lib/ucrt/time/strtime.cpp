//
// strtime.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The strtime family of functions, which return the current time as a string.
//
#include <corecrt_internal_securecrt.h>
#include <corecrt_internal_time.h>

#pragma warning(disable:__WARNING_HIGH_PRIORITY_OVERFLOW_POSTCONDITION) // 26045 Potential postcondition violation that could result in overflow
#pragma warning(disable:__WARNING_RETURNING_BAD_RESULT) // 28196 The requirement that 'return!=0' is not satisfied. (The expression does not evaluate to true.)

// Returns the current time as a string of the form "HH:MM:SS".  These functions
// return an error code on failure, or zero on success.  The buffer must be at
// least nine characters in size.
template <typename Character>
_Success_(buffer != 0 && size_in_chars >= 9)
static errno_t __cdecl common_strtime_s(
    _Out_writes_z_(size_in_chars)   Character*  const   buffer,
    _In_  _In_range_(>=, 9)         size_t      const   size_in_chars
    ) throw()
{
    _VALIDATE_RETURN_ERRCODE(buffer != nullptr && size_in_chars > 0, EINVAL);
    _RESET_STRING(buffer, size_in_chars);
    _VALIDATE_RETURN_ERRCODE(size_in_chars >= 9, ERANGE);

    SYSTEMTIME local_time;
    GetLocalTime(&local_time);

    int const hours   = local_time.wHour;
    int const minutes = local_time.wMinute;
    int const seconds = local_time.wSecond;

    static Character const zero_char = static_cast<Character>('0');

#pragma warning(disable:__WARNING_INCORRECT_VALIDATION) // 26014 Prefast doesn't notice the validation.

    // Store the components of the date into the string in HH:MM:SS form:
    buffer[0] = static_cast<Character>(hours   / 10 + zero_char); // Tens of hour
    buffer[1] = static_cast<Character>(hours   % 10 + zero_char); // Units of hour
    buffer[2] = static_cast<Character>(':');
    buffer[3] = static_cast<Character>(minutes / 10 + zero_char); // Tens of minute
    buffer[4] = static_cast<Character>(minutes % 10 + zero_char); // Units of minute
    buffer[5] = static_cast<Character>(':');
    buffer[6] = static_cast<Character>(seconds / 10 + zero_char); // Tens of second
    buffer[7] = static_cast<Character>(seconds % 10 + zero_char); // Units of second
    buffer[8] = static_cast<Character>('\0');

    return 0;
}

extern "C" errno_t __cdecl _strtime_s(char* const buffer, size_t const size_in_chars)
{
    return common_strtime_s(buffer, size_in_chars);
}

extern "C" errno_t __cdecl _wstrtime_s(wchar_t* const buffer, size_t const size_in_chars)
{
    return common_strtime_s(buffer, size_in_chars);
}



// Returns the current time as a string of the form "HH:MM:SS".  These functions
// assume that the provided result buffer is at least nine characters in length.
// Each returns the buffer on success, or null on failure.
template <typename Character>
_Success_(buffer != 0)
_Ret_z_
static Character* __cdecl common_strtime(_Out_writes_z_(9) Character* const buffer) throw()
{
    errno_t const status = common_strtime_s(buffer, 9);
    if (status != 0)
        return nullptr;

    return buffer;
}

extern "C" char* __cdecl _strtime(char* const buffer)
{
    return common_strtime(buffer);
}

extern "C" wchar_t* __cdecl _wstrtime(wchar_t* const buffer)
{
    return common_strtime(buffer);
}
