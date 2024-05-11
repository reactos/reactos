//
// nlsdata.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Globals for the locale and code page implementation.  These are utilized by
// almost all locale-dependent functions.
//
#include <corecrt_internal.h>



extern "C" int __mb_cur_max{1};



extern "C" int* __cdecl __p___mb_cur_max()
{
    __acrt_ptd* ptd = __acrt_getptd();
    __crt_locale_data* locale_info = ptd->_locale_info;

    __acrt_update_locale_info(ptd, &locale_info);
    return &locale_info->_public._locale_mb_cur_max;
}



extern "C" wchar_t __acrt_wide_c_locale_string[]{L"C"};



extern "C" __crt_lc_time_data const __lc_time_c
{
    { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" },

    {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday",
        "Friday", "Saturday"
    },

    {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug",
        "Sep", "Oct", "Nov", "Dec"
    },

    {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October",
        "November", "December"
    },

    { "AM", "PM" },

    "MM/dd/yy",
    "dddd, MMMM dd, yyyy",
    "HH:mm:ss",

    1,
    0,

    { L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat" },

    {
        L"Sunday", L"Monday", L"Tuesday", L"Wednesday",
        L"Thursday", L"Friday", L"Saturday"
    },

    {
        L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", L"Jul",
        L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
    },

    {
        L"January", L"February", L"March", L"April", L"May",
        L"June", L"July", L"August", L"September", L"October",
        L"November", L"December"
    },

    { L"AM", L"PM" },

    L"MM/dd/yy",
    L"dddd, MMMM dd, yyyy",
    L"HH:mm:ss",
    L"en-US"
};



// The initial locale information structure, containing the C locale data.  It
// is used until the first call to setlocale().
extern "C" __crt_locale_data __acrt_initial_locale_data
{
    {
        __newctype + 128,    // _locale_pctype
        1,                   // _locale_mb_cur_max
        CP_ACP               // _locale_lc_codepage
    },
    1,                       // refcount
    CP_ACP,                  // lc_collate_cp
    CP_ACP,                  // lc_time_cp
    1,                       // lc_clike
    {
        { nullptr, nullptr,                     nullptr, nullptr }, // lc_category[LC_ALL]
        { nullptr, __acrt_wide_c_locale_string, nullptr, nullptr }, // lc_category[LC_COLLATE]
        { nullptr, __acrt_wide_c_locale_string, nullptr, nullptr }, // lc_category[LC_CTYPE]
        { nullptr, __acrt_wide_c_locale_string, nullptr, nullptr }, // lc_category[LC_MONETARY]
        { nullptr, __acrt_wide_c_locale_string, nullptr, nullptr }, // lc_category[LC_NUMERIC]
        { nullptr, __acrt_wide_c_locale_string, nullptr, nullptr }  // lc_category[LC_TIME]
    },
    nullptr,                 // lconv_intl_refcount
    nullptr,                 // lconv_num_refcount
    nullptr,                 // lconv_mon_refcount
    &__acrt_lconv_c,         // lconv
    nullptr,                 // ctype1_refcount
    nullptr,                 // ctype1
    __newclmap + 128,        // pclmap
    __newcumap + 128,        // pcumap
    &__lc_time_c,            // lc_time_curr
    {
        nullptr,             // locale_name[LC_ALL]
        nullptr,             // locale_name[LC_COLLATE]
        nullptr,             // locale_name[LC_CTYPE]
        nullptr,             // locale_name[LC_MONETARY]
        nullptr,             // locale_name[LC_NUMERIC]
        nullptr              // locale_name[LC_TIME]
    }
};



// Global pointer to the current per-thread locale information structure.
__crt_state_management::dual_state_global<__crt_locale_data*> __acrt_current_locale_data;



extern "C" __crt_locale_pointers __acrt_initial_locale_pointers
{
    &__acrt_initial_locale_data,
    &__acrt_initial_multibyte_data
};
