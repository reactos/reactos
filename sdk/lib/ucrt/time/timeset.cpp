//
// timeset.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The time zone values for the default time zone, and other global time data.
//
#include <corecrt_internal_time.h>

#undef _daylight
#undef _dstbias
#undef _timezone
#undef _tzname



__crt_state_management::dual_state_global<long> _timezone; // Pacific Time Zone
__crt_state_management::dual_state_global<int>  _daylight; // Daylight Saving Time (DST) in timezone
__crt_state_management::dual_state_global<long> _dstbias;  // DST offset in seconds

// Note that NT POSIX's TZNAME_MAX is only 10.

#define _PST_STRING "PST"
#define _PDT_STRING "PDT"

#define L_PST_STRING L"PST"
#define L_PDT_STRING L"PDT"

static char tzstd_program[_TZ_STRINGS_SIZE] = { _PST_STRING };
static char tzdst_program[_TZ_STRINGS_SIZE] = { _PDT_STRING };

static wchar_t w_tzstd_program[_TZ_STRINGS_SIZE] = { L_PST_STRING };
static wchar_t w_tzdst_program[_TZ_STRINGS_SIZE] = { L_PDT_STRING };

#ifdef _CRT_GLOBAL_STATE_ISOLATION
    static char tzstd_os[_TZ_STRINGS_SIZE] = { _PST_STRING };
    static char tzdst_os[_TZ_STRINGS_SIZE] = { _PDT_STRING };

    static wchar_t w_tzstd_os[_TZ_STRINGS_SIZE] = { L_PST_STRING };
    static wchar_t w_tzdst_os[_TZ_STRINGS_SIZE] = { L_PDT_STRING };
#endif

static char* tzname_states[__crt_state_management::state_index_count][2] =
{
    { tzstd_program, tzdst_program },
    #ifdef _CRT_GLOBAL_STATE_ISOLATION
    { tzstd_os,      tzdst_os      }
    #endif
};

static wchar_t* w_tzname_states[__crt_state_management::state_index_count][2] =
{
    { w_tzstd_program, w_tzdst_program },
    #ifdef _CRT_GLOBAL_STATE_ISOLATION
    { w_tzstd_os,      w_tzdst_os      }
    #endif
};

static __crt_state_management::dual_state_global<char **> _tzname;
static __crt_state_management::dual_state_global<wchar_t **> w_tzname;

// Initializer for the timeset globals:
_CRT_LINKER_FORCE_INCLUDE(__acrt_timeset_initializer);

extern "C" int __cdecl __acrt_initialize_timeset()
{
    _timezone.initialize(8 * 3600L);
    _daylight.initialize(1);
    _dstbias.initialize (-3600);

    char*** const first_state = _tzname.dangerous_get_state_array();
    for (unsigned i = 0; i != __crt_state_management::state_index_count; ++i)
    {
        first_state[i] = tzname_states[i];
    }

    wchar_t*** const w_first_state = w_tzname.dangerous_get_state_array();
    for (unsigned i = 0; i != __crt_state_management::state_index_count; ++i)
    {
        w_first_state[i] = w_tzname_states[i];
    }

    return 0;
}

extern "C"  errno_t __cdecl _get_daylight(int* const result)
{
    _VALIDATE_RETURN_ERRCODE(result != nullptr, EINVAL);

    // This variable is correctly inited at startup, so no need to check if
    // CRT init finished.
    *result = _daylight.value();
    return 0;
}

extern "C" errno_t __cdecl _get_dstbias(long* result)
{
    _VALIDATE_RETURN_ERRCODE(result != nullptr, EINVAL);

    // This variable is correctly inited at startup, so no need to check if
    // CRT init finished.
    *result = _dstbias.value();
    return 0;
}

extern "C" errno_t __cdecl _get_timezone(long* result)
{
    _VALIDATE_RETURN_ERRCODE(result != nullptr, EINVAL);

    // This variable is correctly inited at startup, so no need to check if
    // CRT init finished.
    *result = _timezone.value();
    return 0;
}

extern "C" errno_t __cdecl _get_tzname(
    size_t* const length,
    char*   const buffer,
    size_t  const size_in_bytes,
    int     const index)
{
    _VALIDATE_RETURN_ERRCODE(
        (buffer != nullptr && size_in_bytes > 0) ||
        (buffer == nullptr && size_in_bytes == 0),
        EINVAL);

    if (buffer != nullptr)
        buffer[0] = '\0';

    _VALIDATE_RETURN_ERRCODE(length != nullptr,        EINVAL);
    _VALIDATE_RETURN_ERRCODE(index == 0 || index == 1, EINVAL);

    // _tzname is correctly inited at startup, so no need to check if
    // CRT init finished.
    *length = strlen(_tzname.value()[index]) + 1;

    // If the buffer pointer is null, the caller is interested only in the size
    // of the string, not in the actual value of the string:
    if (buffer == nullptr)
        return 0;

    if (*length > size_in_bytes)
        return ERANGE;

    return strcpy_s(buffer, size_in_bytes, _tzname.value()[index]);
}



// Day names must be three character abbreviations strung together
extern "C" const char __dnames[] =
{
    "SunMonTueWedThuFriSat"
};

// Month names must be three character abbreviations strung together
extern "C" const char __mnames[] =
{
    "JanFebMarAprMayJunJulAugSepOctNovDec"
};



// Accessors for the global time data
extern "C" int* __cdecl __daylight()
{
    return &_daylight.value();
}

extern "C" long* __cdecl __dstbias()
{
    return &_dstbias.value();
}

extern "C" long* __cdecl __timezone()
{
    return &_timezone.value();
}

extern "C" char** __cdecl __tzname()
{
    return _tzname.value();
}

extern "C" wchar_t** __cdecl __wide_tzname()
{
    return w_tzname.value();
}
