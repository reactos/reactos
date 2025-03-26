//
// inittime.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Functionality for initializing and destroying LC_TIME category data structures
//
#include <corecrt_internal.h>
#include <locale.h>
#include <stdlib.h>


// Enclaves have no ability to create new locales.
#ifndef _UCRT_ENCLAVE_BUILD

_Success_(return != false)
static bool __cdecl initialize_lc_time(
    _Inout_ __crt_lc_time_data* const lc_time,
    _In_    __crt_locale_data*  const locale_data
    ) throw()
{
    wchar_t const* const locale_name = locale_data->locale_name[LC_TIME];

    __crt_locale_pointers locinfo = { locale_data, nullptr };

    lc_time->_W_ww_locale_name = __acrt_copy_locale_name(locale_name);

    int ret = 0;

    // The days of the week.  Note that the OS days are numbered 1-7, starting
    // with Monday, whereas the lc_time days are numbered (indexed, really) 0-6
    // starting with Sunday (thus the adjustment to the index in the loop).
    for (unsigned int i = 0; i != 7; ++i)
    {
        unsigned int const result_index = (i + 1) % 7;
        ret |= __acrt_GetLocaleInfoA(&locinfo, LC_STR_TYPE, locale_name, LOCALE_SABBREVDAYNAME1 + i, &lc_time->wday_abbr[result_index]);
        ret |= __acrt_GetLocaleInfoA(&locinfo, LC_STR_TYPE, locale_name, LOCALE_SDAYNAME1       + i, &lc_time->wday     [result_index]);

        ret |= __acrt_GetLocaleInfoA(&locinfo, LC_WSTR_TYPE, locale_name, LOCALE_SABBREVDAYNAME1 + i, &lc_time->_W_wday_abbr[result_index]);
        ret |= __acrt_GetLocaleInfoA(&locinfo, LC_WSTR_TYPE, locale_name, LOCALE_SDAYNAME1       + i, &lc_time->_W_wday     [result_index]);
    }

    // The months of the year
    for (unsigned int i = 0; i != 12; ++i)
    {
        ret |= __acrt_GetLocaleInfoA(&locinfo, LC_STR_TYPE,  locale_name, LOCALE_SABBREVMONTHNAME1 + i, &lc_time->month_abbr[i]);
        ret |= __acrt_GetLocaleInfoA(&locinfo, LC_STR_TYPE,  locale_name, LOCALE_SMONTHNAME1       + i, &lc_time->month     [i]);

        ret |= __acrt_GetLocaleInfoA(&locinfo, LC_WSTR_TYPE, locale_name, LOCALE_SABBREVMONTHNAME1 + i, &lc_time->_W_month_abbr[i]);
        ret |= __acrt_GetLocaleInfoA(&locinfo, LC_WSTR_TYPE, locale_name, LOCALE_SMONTHNAME1       + i, &lc_time->_W_month     [i]);
    }

    ret |= __acrt_GetLocaleInfoA(&locinfo, LC_STR_TYPE, locale_name, LOCALE_S1159, &lc_time->ampm[0]);
    ret |= __acrt_GetLocaleInfoA(&locinfo, LC_STR_TYPE, locale_name, LOCALE_S2359, &lc_time->ampm[1]);

    ret |= __acrt_GetLocaleInfoA(&locinfo, LC_WSTR_TYPE, locale_name, LOCALE_S1159, &lc_time->_W_ampm[0]);
    ret |= __acrt_GetLocaleInfoA(&locinfo, LC_WSTR_TYPE, locale_name, LOCALE_S2359, &lc_time->_W_ampm[1]);

    ret |= __acrt_GetLocaleInfoA(&locinfo, LC_STR_TYPE, locale_name, LOCALE_SSHORTDATE,    &lc_time->ww_sdatefmt);
    ret |= __acrt_GetLocaleInfoA(&locinfo, LC_STR_TYPE, locale_name, LOCALE_SLONGDATE,     &lc_time->ww_ldatefmt);
    ret |= __acrt_GetLocaleInfoA(&locinfo, LC_STR_TYPE, locale_name, LOCALE_STIMEFORMAT,   &lc_time->ww_timefmt);
    ret |= __acrt_GetLocaleInfoA(&locinfo, LC_INT_TYPE, locale_name, LOCALE_ICALENDARTYPE, &lc_time->ww_caltype);

    ret |= __acrt_GetLocaleInfoA(&locinfo, LC_WSTR_TYPE, locale_name, LOCALE_SSHORTDATE,  &lc_time->_W_ww_sdatefmt);
    ret |= __acrt_GetLocaleInfoA(&locinfo, LC_WSTR_TYPE, locale_name, LOCALE_SLONGDATE,   &lc_time->_W_ww_ldatefmt);
    ret |= __acrt_GetLocaleInfoA(&locinfo, LC_WSTR_TYPE, locale_name, LOCALE_STIMEFORMAT, &lc_time->_W_ww_timefmt);

    return ret == 0;
}



_Success_(return != nullptr)
static __crt_lc_time_data const* __cdecl get_or_create_lc_time(
    _In_ __crt_locale_data* const locale_data
    ) throw()
{
    // C Locale
    if (!locale_data->locale_name[LC_TIME])
    {
        return &__lc_time_c;
    }

    // Non-C Locale
    __crt_unique_heap_ptr<__crt_lc_time_data> lc_time = _calloc_crt_t(__crt_lc_time_data, 1);
    if (!lc_time)
    {
        return nullptr;
    }

    if (!initialize_lc_time(lc_time.get(), locale_data))
    {
        __acrt_locale_free_time(lc_time.get());
        return nullptr;
    }

    lc_time.get()->refcount = 1;

    return lc_time.detach();
}



extern "C" int __cdecl __acrt_locale_initialize_time(
    __crt_locale_data* const locale_data
    )
{
    __crt_lc_time_data const* const lc_time = get_or_create_lc_time(locale_data);
    if (!lc_time)
    {
        return 1;
    }

    if (__acrt_locale_release_lc_time_reference(locale_data->lc_time_curr) == 0)
    {
        _ASSERTE(("lc_time_curr unexpectedly has no remaining references", 0));
    }

    locale_data->lc_time_curr = lc_time;
    return 0;
}

#endif /* _UCRT_ENCLAVE_BUILD */


static void __cdecl free_crt_array_internal(
    _Inout_updates_(array_count) void const** const array,
    _In_                         size_t       const array_count
    ) throw()
{
    for (void const* const* it = array; it != array + array_count; ++it)
    {
        _free_crt(const_cast<void*>(*it));
    }
}



template <typename T, size_t N>
static void __cdecl free_crt_array(_Inout_ T* (&array)[N]) throw()
{
    return free_crt_array_internal(const_cast<void const**>(reinterpret_cast<void const* const*>(array)), N);
}



extern "C" void __cdecl __acrt_locale_free_time(
    __crt_lc_time_data* const lc_time
    )
{
    if (!lc_time)
    {
        return;
    }

    free_crt_array(lc_time->wday_abbr);
    free_crt_array(lc_time->wday);
    free_crt_array(lc_time->month_abbr);
    free_crt_array(lc_time->month);
    free_crt_array(lc_time->ampm);

    _free_crt(lc_time->ww_sdatefmt);
    _free_crt(lc_time->ww_ldatefmt);
    _free_crt(lc_time->ww_timefmt);

    free_crt_array(lc_time->_W_wday_abbr);
    free_crt_array(lc_time->_W_wday);
    free_crt_array(lc_time->_W_month_abbr);
    free_crt_array(lc_time->_W_month);
    free_crt_array(lc_time->_W_ampm);

    _free_crt(lc_time->_W_ww_sdatefmt);
    _free_crt(lc_time->_W_ww_ldatefmt);
    _free_crt(lc_time->_W_ww_timefmt);

    _free_crt(lc_time->_W_ww_locale_name);
}
