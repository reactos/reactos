//
// wcsftime.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The wcsftime family of functions, which format time data into a wide string,
// and related functionality.
//
#include <corecrt_internal_time.h>
#include <stdlib.h>
#include <locale.h>


//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Day and Month Name and Time Locale Information Fetching Functions
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
extern "C" wchar_t* __cdecl _W_Getdays()
{
    _LocaleUpdate locale_update(nullptr);
    __crt_lc_time_data const* const time_data = locale_update.GetLocaleT()->locinfo->lc_time_curr;

    size_t length = 0;
    for (size_t n = 0; n < 7; ++n)
    {
        length += wcslen(time_data->_W_wday_abbr[n]) + wcslen(time_data->_W_wday[n]) + 2;
    }

    __crt_unique_heap_ptr<wchar_t> buffer(_malloc_crt_t(wchar_t, length + 1));
    if (buffer.get() == nullptr)
        return nullptr;

    wchar_t* it = buffer.get();
    for (size_t n = 0; n < 7; ++n)
    {
        *it++ = L':';
        _ERRCHECK(wcscpy_s(it, (length + 1) - (it - buffer.get()), time_data->_W_wday_abbr[n]));
        it += wcslen(it);
        *it++ = L':';
        _ERRCHECK(wcscpy_s(it, (length + 1) - (it - buffer.get()), time_data->_W_wday[n]));
        it += wcslen(it);
    }
    *it++ = L'\0';

    return buffer.detach();
}



extern "C" wchar_t* __cdecl _W_Getmonths()
{
    _LocaleUpdate locale_update(nullptr);
    __crt_lc_time_data const* const time_data = locale_update.GetLocaleT()->locinfo->lc_time_curr;

    size_t length = 0;
    for (size_t n = 0; n < 12; ++n)
    {
        length += wcslen(time_data->_W_month_abbr[n]) + wcslen(time_data->_W_month[n]) + 2;
    }

    __crt_unique_heap_ptr<wchar_t> buffer(_malloc_crt_t(wchar_t, length + 1));
    if (buffer.get() == nullptr)
        return nullptr;

    wchar_t* it = buffer.get();
    for (size_t n = 0; n < 12; ++n)
    {
        *it++ = L':';
        _ERRCHECK(wcscpy_s(it, (length + 1) - (it - buffer.get()), time_data->_W_month_abbr[n]));
        it += wcslen(it);
        *it++ = L':';
        _ERRCHECK(wcscpy_s(it, (length + 1) - (it - buffer.get()), time_data->_W_month[n]));
        it += wcslen(it);
    }
    *it++ = L'\0';

    return buffer.detach();
}



extern "C" void* __cdecl _W_Gettnames()
{
    _LocaleUpdate locale_update(nullptr);
    __crt_lc_time_data const* const src = locale_update.GetLocaleT()->locinfo->lc_time_curr;



    #define PROCESS_STRING(STR, CHAR, CPY, LEN)                                        \
        while (bytes % sizeof(CHAR) != 0)                                              \
        {                                                                              \
            ++bytes;                                                                   \
        }                                                                              \
        if (phase == 1)                                                                \
        {                                                                              \
            dest->STR = ((CHAR *) dest) + bytes / sizeof(CHAR);                        \
            _ERRCHECK(CPY(dest->STR, (total_bytes - bytes) / sizeof(CHAR), src->STR)); \
        }                                                                              \
        bytes += (LEN(src->STR) + 1) * sizeof(CHAR);

    #define PROCESS_NARROW_STRING(STR) \
        PROCESS_STRING(STR, char, strcpy_s, strlen)

    #define PROCESS_WIDE_STRING(STR) \
        PROCESS_STRING(STR, wchar_t, wcscpy_s, wcslen)

    #define PROCESS_NARROW_ARRAY(ARR)                           \
        for (size_t idx = 0; idx < _countof(src->ARR); ++idx)   \
        {                                                       \
            PROCESS_NARROW_STRING(ARR[idx])                     \
        }

    #define PROCESS_WIDE_ARRAY(ARR)                             \
        for (size_t idx = 0; idx < _countof(src->ARR); ++idx)   \
        {                                                       \
            PROCESS_WIDE_STRING(ARR[idx])                       \
        }


    size_t total_bytes = 0;
    size_t bytes       = sizeof(__crt_lc_time_data);

    __crt_lc_time_data* dest  = nullptr;
    for (int phase = 0; phase < 2; ++phase)
    {
        if (phase == 1)
        {
            dest = static_cast<__crt_lc_time_data*>(_malloc_crt(bytes));

            if (!dest) {
                return nullptr;
            }

            memset(dest, 0, bytes);

            total_bytes = bytes;

            bytes = sizeof(__crt_lc_time_data);
        }

        PROCESS_NARROW_ARRAY(wday_abbr)
        PROCESS_NARROW_ARRAY(wday)
        PROCESS_NARROW_ARRAY(month_abbr)
        PROCESS_NARROW_ARRAY(month)
        PROCESS_NARROW_ARRAY(ampm)
        PROCESS_NARROW_STRING(ww_sdatefmt)
        PROCESS_NARROW_STRING(ww_ldatefmt)
        PROCESS_NARROW_STRING(ww_timefmt)

        if (phase == 1)
        {
            dest->ww_caltype = src->ww_caltype;
            dest->refcount = 0;
        }

        PROCESS_WIDE_ARRAY(_W_wday_abbr)
        PROCESS_WIDE_ARRAY(_W_wday)
        PROCESS_WIDE_ARRAY(_W_month_abbr)
        PROCESS_WIDE_ARRAY(_W_month)
        PROCESS_WIDE_ARRAY(_W_ampm)
        PROCESS_WIDE_STRING(_W_ww_sdatefmt)
        PROCESS_WIDE_STRING(_W_ww_ldatefmt)
        PROCESS_WIDE_STRING(_W_ww_timefmt)
        PROCESS_WIDE_STRING(_W_ww_locale_name)
    }

    return dest;
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Local Functions Used In Time String Formatting
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// Values for __crt_lc_time_data ww_* fields for store_winword:
#define WW_SDATEFMT 0
#define WW_LDATEFMT 1
#define WW_TIMEFMT  2

// Note: annotation does not account for fact < *count bytes may be written if null terminator hit
#define _CrtWcstime_Writes_and_advances_ptr_(count) \
    _Outptr_result_buffer_(count)


// Copies the supplied time string 'in' into the output buffer 'out' until either
// (a) the end of the time string is reached or (b) '*count' becomes zero (and we
// run out of buffer space).  The '*out' pointer is updated to point to the next
// character in the buffer (one-past-the-end of the insertion), and the '*count'
// value is updated to reflect the number of characters written.
static void __cdecl store_string(
    _In_reads_or_z_(*count)                      wchar_t const*       in,
    _CrtWcstime_Writes_and_advances_ptr_(*count) wchar_t**      const out,
    _Inout_                                      size_t*        const count
    ) throw()
{
    while (*count != 0 && *in != L'\0')
    {
        *(*out)++ = *in++;
        --*count;
    }
}



// Converts a positive integer ('value') into a string and stores it in the
// 'output' buffer.  It stops writing when either (a) the full value has been
// printed, or (b) '*count' becomes zero (and we run out of buffer space).   The
// '*out' pointer is updated to point to the next character in the buffer (one-
// past-the-end of the insertion), and the '*count' value is updated to reflect
// the number of characters written.
static void __cdecl store_number_without_lead_zeroes(
                                                 int             value,
    _CrtWcstime_Writes_and_advances_ptr_(*count) wchar_t** const out,
    _Inout_                                      size_t*   const count
    ) throw()
{
    // Put the digits in the buffer in reverse order:
    wchar_t* out_it = *out;
    if (*count > 1)
    {
        do
        {
            *out_it++ = static_cast<wchar_t>(value % 10 + L'0');

            value /= 10;
            --*count;
        }
        while (value > 0 && *count > 1);
    }
    else
    {
        // Indicate buffer too small.
        *out -= *count;
        *count = 0;
        return;
    }

    wchar_t* left  = *out;
    wchar_t* right = out_it - 1;

    // Update the output iterator to point to the next space:
    *out = out_it;

    // Reverse the buffer:
    while (left < right)
    {
        wchar_t const x = *right;
        *right-- = *left;
        *left++  = x;
    }
}



// Converts a positive integer ('value') into a string and stores it in the
// 'output' buffer.  Both '*out' and '*count' are updated to reflect the
// write.
static void __cdecl store_number(
                                                 int             value,
                                                 int             digits,
    _CrtWcstime_Writes_and_advances_ptr_(*count) wchar_t** const out,
    _Inout_                                      size_t*   const count,
                                                 wchar_t   const pad_character
    ) throw()
{
    if (pad_character == '\0')
    {
        store_number_without_lead_zeroes(value, out, count);
        return;
    }

    if (static_cast<size_t>(digits) < *count)
    {
        int temp = 0;
        for (digits--; digits + 1 != 0; --digits)
        {
            if (value != 0)
            {
                (*out)[digits] = static_cast<wchar_t>(L'0' + value % 10);
            }
            else
            {
                (*out)[digits] = pad_character;
            }

            value /= 10;
            temp++;
        }

        *out   += temp;
        *count -= temp;
    }
    else
    {
        *count = 0;
    }
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Local Functions Used for ISO Week-Based Year Computations
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
enum
{
    sunday    = 0,
    monday    = 1,
    tuesday   = 2,
    wednesday = 3,
    thursday  = 4,
    friday    = 5,
    saturday  = 6
};

static int compute_week_of_year(int const wstart, int const wday, int const yday) throw()
{
    int const adjusted_wday{(wday + 7 - wstart) % 7};
    return (yday + 7 - adjusted_wday) / 7;
}

static int __cdecl compute_iso_week_internal(int year, int wday, int yday) throw()
{
    int  const week_number{compute_week_of_year(monday, wday, yday)};
    bool const is_leap_year{__crt_time_is_leap_year(year)};

    int const yunleap{yday - is_leap_year};
    int const jan1{(371 - yday + wday) % 7};
    int const dec32{(jan1 + is_leap_year + 365) % 7};

    if ((364 <= yunleap && dec32 == tuesday  ) ||
        (363 <= yunleap && dec32 == wednesday) ||
        (362 <= yunleap && dec32 == thursday ))
    {
        return -1; // Push into the next year
    }
    else if (jan1 == tuesday || jan1 == wednesday || jan1 == thursday)
    {
        return week_number + 1;
    }

    return week_number;
}

static int __cdecl compute_iso_week(int const year, int const wday, int const yday) throw()
{
    int const week_number{compute_iso_week_internal(year, wday, yday)};

    if (week_number == 0)
        return compute_iso_week_internal(year - 1, wday + 7 - yday, __crt_time_is_leap_year(year - 1) ? 366 : 365);

    if (0 < week_number)
        return week_number;

    return 1;
}

static int __cdecl compute_iso_year(int const year, int const wday, int const yday) throw()
{
    int const week_number{compute_iso_week_internal(year, wday, yday)};

    if (week_number == 0)
        return year - 1;

    if (0 < week_number)
        return year;

    return year + 1;
}



// store_winword and expand_time are mutually recursive
_Success_(return)
static bool __cdecl expand_time(
                                                 _locale_t                 locale,
                                                 wchar_t                   specifier,
                                                 tm const*                 tmptr,
    _CrtWcstime_Writes_and_advances_ptr_(*count) wchar_t**                 out,
    _Inout_                                      size_t*                   count,
                                                 __crt_lc_time_data const* lc_time,
                                                 bool                      alternate_form
    ) throw();



// Formats the date and time in the supplied WinWord format and stores the
// formatted result in the supplied buffer.  For simple localized Gregorian
// calendars (calendar type 1), the WinWord format is converted token-by-token
// to wcsftime conversion specifiers.  expand_time is then called to do the
// heavy lifting.  For other calendar types, the Windows APIs GetDateFormatEx
// and GetTimeFormatEx are instead used to do all the formatting, so this
// function does not need to know about era and period strings, year offsets,
// etc.  Returns true on success; false on failure.
_Success_(return)
static bool __cdecl store_winword(
                                                 _locale_t                 const locale,
                                                 int                       const field_code,
                                                 tm const*                 const tmptr,
    _CrtWcstime_Writes_and_advances_ptr_(*count) wchar_t**                 const out,
    _Inout_                                      size_t*                   const count,
                                                 __crt_lc_time_data const* const lc_time
    ) throw()
{
    wchar_t const* format;
    switch (field_code)
    {
    case WW_SDATEFMT:
        format = lc_time->_W_ww_sdatefmt;
        break;

    case WW_LDATEFMT:
        format = lc_time->_W_ww_ldatefmt;
        break;

    case WW_TIMEFMT:
    default:
        format = lc_time->_W_ww_timefmt;
        break;
    }

    if (lc_time->ww_caltype != 1)
    {
        // We have something other than the basic Gregorian calendar
        bool const is_time_format = field_code == WW_TIMEFMT;

        // We leave the verification of the SYSTEMTIME up to the Windows API
        // that we call; if one of those functions returns zero to indicate
        // failure, we fall through and call expand_time() again.
        SYSTEMTIME system_time;
        system_time.wYear   = static_cast<WORD>(tmptr->tm_year + 1900);
        system_time.wMonth  = static_cast<WORD>(tmptr->tm_mon + 1);
        system_time.wDay    = static_cast<WORD>(tmptr->tm_mday);
        system_time.wHour   = static_cast<WORD>(tmptr->tm_hour);
        system_time.wMinute = static_cast<WORD>(tmptr->tm_min);
        system_time.wSecond = static_cast<WORD>(tmptr->tm_sec);
        system_time.wMilliseconds = 0;

        // Find buffer size required:
        int cch;
        if (is_time_format)
            cch = __acrt_GetTimeFormatEx(lc_time->_W_ww_locale_name, 0, &system_time, format, nullptr, 0);
        else
            cch = __acrt_GetDateFormatEx(lc_time->_W_ww_locale_name, 0, &system_time, format, nullptr, 0, nullptr);

        if (cch != 0)
        {
            __crt_scoped_stack_ptr<wchar_t> const buffer(_malloca_crt_t(wchar_t, cch));
            if (buffer.get() != nullptr)
            {
                // Do actual date/time formatting:
                if (is_time_format)
                    cch = __acrt_GetTimeFormatEx(lc_time->_W_ww_locale_name, 0, &system_time, format, buffer.get(), cch);
                else
                    cch = __acrt_GetDateFormatEx(lc_time->_W_ww_locale_name, 0, &system_time, format, buffer.get(), cch, nullptr);

                // Copy to output buffer:
                wchar_t const* buffer_it = buffer.get();
                while (--cch > 0 && *count > 0)
                {
                    *(*out)++ = *buffer_it++;
                    (*count)--;
                }

                return true;
            }
        }

        // If an error occurs, just fall through to localized Gregorian...
    }

    while (*format && *count != 0)
    {
        wchar_t specifier = 0;
        bool no_lead_zeros = false;

        // Count the number of repetitions of this character
        int repeat = 0;
        wchar_t const* p = format;
        for (; *p++ == *format; ++repeat);
            // Leave p pointing to the beginning of the next token
            p--;

        // Switch on ASCII format character and determine specifier:
        switch (*format)
        {
        case L'M':
        {
            switch (repeat)
            {
            case 1: no_lead_zeros = true; // fall through
            case 2: specifier = L'm'; break;
            case 3: specifier = L'b'; break;
            case 4: specifier = L'B'; break;
            }
            break;
        }

        case L'd':
        {
            switch (repeat)
            {
            case 1: no_lead_zeros = true; // fall through
            case 2: specifier = L'd'; break;
            case 3: specifier = L'a'; break;
            case 4: specifier = L'A'; break;
            }
            break;
        }

        case L'y':
        {
            switch (repeat)
            {
            case 2: specifier = L'y'; break;
            case 4: specifier = L'Y'; break;
            }
            break;
        }

        case L'h':
        {
            switch (repeat)
            {
            case 1: no_lead_zeros = true; // fall through
            case 2: specifier = L'I'; break;
            }
            break;
        }

        case L'H':
        {
            switch (repeat)
            {
            case 1: no_lead_zeros = true; // fall through
            case 2: specifier = L'H'; break;
            }
            break;
        }

        case L'm':
        {
            switch (repeat)
            {
            case 1: no_lead_zeros = true; // fall through
            case 2: specifier = L'M'; break;
            }
            break;
        }

        case L's': // for compatibility; not strictly WinWord
        {
            switch (repeat)
            {
            case 1: no_lead_zeros = true; // fall through
            case 2: specifier = L'S'; break;
            }
            break;
        }

        case L'A':
        case L'a':
        {
            if (!_wcsicmp(format, L"am/pm"))
            {
                p = format + 5;
            }
            else if (!_wcsicmp(format, L"a/p"))
            {
                p = format + 3;
            }

            specifier = L'p';
            break;
        }

        case L't': // t or tt time marker suffix
        {
            wchar_t* ampmstr = tmptr->tm_hour <= 11
                ? lc_time->_W_ampm[0]
                : lc_time->_W_ampm[1];

            if (repeat == 1 && *count > 0)
            {
                *(*out)++ = *ampmstr++;
                (*count)--;
            }
            else
            {
                while (*ampmstr != 0 && *count > 0)
                {
                    *(*out)++ = *ampmstr++;
                    --*count;
                }
            }
            format = p;
            continue;
        }

        case L'\'': // literal string
        {
            if (repeat % 2 == 0) // even number
            {
                format += repeat;
            }
            else // odd number
            {
                format += repeat;
                while (*format && *count != 0)
                {
                    if (*format == L'\'')
                    {
                        format++;
                        break;
                    }

                    *(*out)++ = *format++;
                    --*count;
                }
            }

            continue;
        }

        default: // non-control char, print it
        {
            break;
        }
        }

        // expand specifier, or copy literal if specifier not found
        if (specifier)
        {
            _VALIDATE_RETURN_NOEXC(expand_time(locale, specifier, tmptr, out, count, lc_time, no_lead_zeros), EINVAL, false);

            format = p; // bump format up to the next token
        }
        else
        {
            *(*out)++ = *format++;
            --*count;
        }
    }

    return true;
}



// Expands the conversion specifier using the time struct and stores the result
// into the supplied buffer.  The expansion is locale-dependent.  Returns true
// on success; false on failure.
static bool __cdecl expand_time(
    _locale_t                 const locale,
    wchar_t                   const specifier,
    tm const*                 const timeptr,
    wchar_t**                 const string,
    size_t*                   const left,
    __crt_lc_time_data const* const lc_time,
    bool                      const alternate_form
    ) throw()
{
    switch (specifier)
    {
    case L'a': // abbreviated weekday name
    {
        _VALIDATE_RETURN(timeptr->tm_wday >= 0 && timeptr->tm_wday <= 6, EINVAL, false);
        store_string(lc_time->_W_wday_abbr[timeptr->tm_wday], string, left);
        return true;
    }

    case L'A': // full weekday name
    {
        _VALIDATE_RETURN(timeptr->tm_wday >= 0 && timeptr->tm_wday <= 6, EINVAL, false);
        store_string(lc_time->_W_wday[timeptr->tm_wday], string, left);
        return true;
    }

    case L'b': // abbreviated month name
    {
        _VALIDATE_RETURN(timeptr->tm_mon >= 0 && timeptr->tm_mon <= 11, EINVAL, false);
        store_string(lc_time->_W_month_abbr[timeptr->tm_mon], string, left);
        return true;
    }

    case L'B': // full month name
    {
        _VALIDATE_RETURN(timeptr->tm_mon >= 0 && timeptr->tm_mon <= 11, EINVAL, false);
        store_string(lc_time->_W_month[timeptr->tm_mon], string, left);
        return true;
    }

    case L'c': // appropriate date and time representation
    {
        // In the C locale, %c is equivalent to "%a %b %e %T %Y".  This format
        // is not achievable using the Windows API date and time format APIs
        // (it's hard to interleave date and time together, and there's no way
        // to format %e).  Therefore, we special case this specifier for the C
        // locale.
        if (lc_time == &__lc_time_c && !alternate_form)
        {
            _VALIDATE_RETURN_NOEXC(expand_time(locale, L'a', timeptr, string, left, lc_time, false), EINVAL, false);
            store_string(L" ", string, left);
            _VALIDATE_RETURN_NOEXC(expand_time(locale, L'b', timeptr, string, left, lc_time, false), EINVAL, false);
            store_string(L" ", string, left);
            _VALIDATE_RETURN_NOEXC(expand_time(locale, L'e', timeptr, string, left, lc_time, false), EINVAL, false);
            store_string(L" ", string, left);
            _VALIDATE_RETURN_NOEXC(expand_time(locale, L'T', timeptr, string, left, lc_time, false), EINVAL, false);
            store_string(L" ", string, left);
            _VALIDATE_RETURN_NOEXC(expand_time(locale, L'Y', timeptr, string, left, lc_time, false), EINVAL, false);
        }
        // Otherwise, if we're not in the C locale, use the locale-provided
        // format strings:
        else
        {
            int const field_code = alternate_form ? WW_LDATEFMT : WW_SDATEFMT;

            _VALIDATE_RETURN_NOEXC(store_winword(locale, field_code, timeptr, string, left, lc_time), EINVAL, false);
            store_string(L" ", string, left);
            _VALIDATE_RETURN_NOEXC(store_winword(locale, WW_TIMEFMT, timeptr, string, left, lc_time), EINVAL, false);
        }

        return true;
    }

    case L'C': // century in decimal (00-99)
    {
        _VALIDATE_RETURN(timeptr->tm_year >= -1900 && timeptr->tm_year <= 8099, EINVAL, false);
        store_number(__crt_get_century(timeptr->tm_year), 2, string, left, alternate_form ? '\0' : '0');
        return true;
    }

    case L'd': // day of the month in decimal (01-31)
    {
        _VALIDATE_RETURN(timeptr->tm_mday >= 1 && timeptr->tm_mday <= 31, EINVAL, false);
        store_number(timeptr->tm_mday, 2, string, left, alternate_form ? '\0' : '0');
        return true;
    }

    case L'D': // equivalent to "%m/%d/%y"
    {
        _VALIDATE_RETURN_NOEXC(expand_time(locale, L'm', timeptr, string, left, lc_time, alternate_form), EINVAL, false);
        store_string(L"/", string, left);
        _VALIDATE_RETURN_NOEXC(expand_time(locale, L'd', timeptr, string, left, lc_time, alternate_form), EINVAL, false);
        store_string(L"/", string, left);
        _VALIDATE_RETURN_NOEXC(expand_time(locale, L'y', timeptr, string, left, lc_time, alternate_form), EINVAL, false);
        return true;
    }

    case L'e': // day of month as a decimal number (1-31); space padded:
    {
        _VALIDATE_RETURN(timeptr->tm_mday >= 1 && timeptr->tm_mday <= 31, EINVAL, false);
        store_number(timeptr->tm_mday, 2, string, left, alternate_form ? '\0' : ' ');
        return true;
    }

    case L'F': // equivalent to "%Y-%m-%d" (ISO 8601):
    {
        _VALIDATE_RETURN_NOEXC(expand_time(locale, L'Y', timeptr, string, left, lc_time, alternate_form), EINVAL, false);
        store_string(L"-", string, left);
        _VALIDATE_RETURN_NOEXC(expand_time(locale, L'm', timeptr, string, left, lc_time, alternate_form), EINVAL, false);
        store_string(L"-", string, left);
        _VALIDATE_RETURN_NOEXC(expand_time(locale, L'd', timeptr, string, left, lc_time, alternate_form), EINVAL, false);
        return true;
    }

    case L'g': // last two digits of the week-based year:
    {
        _VALIDATE_RETURN(timeptr->tm_year >= -1900 && timeptr->tm_year <= 8099, EINVAL, false);
        int const iso_year{compute_iso_year(timeptr->tm_year, timeptr->tm_wday, timeptr->tm_yday) + 1900};
        store_number(iso_year % 100, 2, string, left, '0');
        return true;
    }

    case L'G': // week-based year:
    {
        _VALIDATE_RETURN(timeptr->tm_year >= -1900 && timeptr->tm_year <= 8099, EINVAL, false);
        int const iso_year{compute_iso_year(timeptr->tm_year, timeptr->tm_wday, timeptr->tm_yday) + 1900};
        store_number(iso_year, 4, string, left, '0');
        return true;
    }

    case L'h': // equivalent to "%b":
    {
        return expand_time(locale, L'b', timeptr, string, left, lc_time, alternate_form);
    }

    case L'H': // 24-hour decimal (00-23)
    {
        _VALIDATE_RETURN(timeptr->tm_hour >= 0 && timeptr->tm_hour <= 23, EINVAL, false);
        store_number(timeptr->tm_hour, 2, string, left, alternate_form ? '\0' : '0');
        return true;
    }

    case L'I': // 12-hour decimal (01-12)
    {
        _VALIDATE_RETURN(timeptr->tm_hour >= 0 && timeptr->tm_hour <= 23, EINVAL, false);
        unsigned hour = timeptr->tm_hour % 12;
        if (hour == 0)
            hour = 12;

        store_number(hour, 2, string, left, alternate_form ? '\0' : '0');
        return true;
    }

    case L'j': // yday in decimal (001-366)
    {
        _VALIDATE_RETURN(timeptr->tm_yday >= 0 && timeptr->tm_yday <= 365, EINVAL, false);
        store_number(timeptr->tm_yday + 1, 3, string, left, alternate_form ? '\0' : '0');
        return true;
    }

    case L'm': // month in decimal (01-12)
    {
        _VALIDATE_RETURN(timeptr->tm_mon >= 0 && timeptr->tm_mon <= 11, EINVAL, false);
        store_number(timeptr->tm_mon + 1, 2, string, left, alternate_form ? '\0' : '0');
        return true;
    }

    case L'M': // minute in decimal (00-59)
    {
        _VALIDATE_RETURN(timeptr->tm_min >= 0 && timeptr->tm_min <= 59, EINVAL, false);
        store_number(timeptr->tm_min, 2, string, left, alternate_form ? '\0' : '0');
        return true;
    }

    case L'n': // newline character
    {
        store_string(L"\n", string, left);
        return true;
    }

    case L'p': // AM/PM designation
    {
        _VALIDATE_RETURN(timeptr->tm_hour >= 0 && timeptr->tm_hour <= 23, EINVAL, false);
        wchar_t const* const ampm_string = timeptr->tm_hour <= 11
            ? lc_time->_W_ampm[0]
            : lc_time->_W_ampm[1];

        store_string(ampm_string, string, left);
        return true;
    }

    case L'r': // Locale-specific 12-hour clock time
    {
        // In the C locale, %r is equivalent to "%I:%M:%S %p".  This is the only
        // locale in which we guarantee that %r is a 12-hour time; in all other
        // locales we only have one time format which may or may not be a 12-hour
        // format.
        if (lc_time == &__lc_time_c)
        {
            _VALIDATE_RETURN_NOEXC(expand_time(locale, 'I', timeptr, string, left, lc_time, alternate_form), EINVAL, false);
            store_string(L":", string, left);
            _VALIDATE_RETURN_NOEXC(expand_time(locale, 'M', timeptr, string, left, lc_time, alternate_form), EINVAL, false);
            store_string(L":", string, left);
            _VALIDATE_RETURN_NOEXC(expand_time(locale, 'S', timeptr, string, left, lc_time, alternate_form), EINVAL, false);
            store_string(L" ", string, left);
            _VALIDATE_RETURN_NOEXC(expand_time(locale, 'p', timeptr, string, left, lc_time, alternate_form), EINVAL, false);
        }
        else
        {
            _VALIDATE_RETURN_NOEXC(expand_time(locale, 'X', timeptr, string, left, lc_time, alternate_form), EINVAL, false);
        }

        return true;
    }

    case L'R': // Equivalent to "%H:%M"
    {
        _VALIDATE_RETURN_NOEXC(expand_time(locale, 'H', timeptr, string, left, lc_time, alternate_form), EINVAL, false);
        store_string(L":", string, left);
        _VALIDATE_RETURN_NOEXC(expand_time(locale, 'M', timeptr, string, left, lc_time, alternate_form), EINVAL, false);
        return true;
    }

    case L'S': // seconds in decimal (00-60) allowing for a leap second
    {
        _VALIDATE_RETURN(timeptr->tm_sec >= 0 && timeptr->tm_sec <= 60, EINVAL, false);
        store_number(timeptr->tm_sec, 2, string, left, alternate_form ? '\0' : '0');
        return true;
    }

    case L't': // tab character
    {
        store_string(L"\t", string, left);
        return true;
    }

    case L'T': // Equivalent to "%H:%M:%S" (ISO 8601)
    {
        _VALIDATE_RETURN_NOEXC(expand_time(locale, 'H', timeptr, string, left, lc_time, alternate_form), EINVAL, false);
        store_string(L":", string, left);
        _VALIDATE_RETURN_NOEXC(expand_time(locale, 'M', timeptr, string, left, lc_time, alternate_form), EINVAL, false);
        store_string(L":", string, left);
        _VALIDATE_RETURN_NOEXC(expand_time(locale, 'S', timeptr, string, left, lc_time, alternate_form), EINVAL, false);
        return true;
    }

    case L'u': // week day in decimal (1-7)
    case L'w': // week day in decimal (0-6)
    {
        _VALIDATE_RETURN(timeptr->tm_wday >= 0 && timeptr->tm_wday <= 6, EINVAL, false);

        int const weekday_number = timeptr->tm_wday == 0 && specifier == L'u'
            ? 7
            : timeptr->tm_wday;

        store_number(weekday_number, 1, string, left, alternate_form ? '\0' : '0');
        return true;
    }

    case L'U': // sunday week number (00-53)
    case L'W': // monday week number (00-53)
    {
        _VALIDATE_RETURN(timeptr->tm_wday >= 0 && timeptr->tm_wday <= 6, EINVAL, false);
        int wdaytemp = timeptr->tm_wday;
        if (specifier == L'W')
        {
            if (timeptr->tm_wday == 0) // Monday-based
                wdaytemp = 6;
            else
                wdaytemp = timeptr->tm_wday - 1;
        }

        _VALIDATE_RETURN(timeptr->tm_yday >= 0 && timeptr->tm_yday <= 365, EINVAL, false);
        unsigned week_number = 0;
        if (timeptr->tm_yday >= wdaytemp)
        {
            week_number = timeptr->tm_yday / 7;
            if (timeptr->tm_yday % 7 >= wdaytemp)
                ++week_number;
        }

        store_number(week_number, 2, string, left, alternate_form ? '\0' : '0');
        return true;
    }

    case L'V': // ISO 8601 week number (01-53):
    {
        int const iso_week{compute_iso_week(timeptr->tm_year, timeptr->tm_wday, timeptr->tm_yday)};
        store_number(iso_week, 2, string, left, alternate_form ? '\0' : '0');
        return true;
    }

    case L'x': // date display
    {
        int const field_code = alternate_form ? WW_LDATEFMT : WW_SDATEFMT;
        _VALIDATE_RETURN_NOEXC(store_winword(locale, field_code, timeptr, string, left, lc_time), EINVAL, false);
        return true;
    }
    case L'X': // time display
    {
        _VALIDATE_RETURN_NOEXC(store_winword(locale, WW_TIMEFMT, timeptr, string, left, lc_time), EINVAL, false);
        return true;
    }

    case L'y': // year without century (00-99)
    {
        _VALIDATE_RETURN(timeptr->tm_year >= -1900 && timeptr->tm_year <= 8099, EINVAL, false);
        unsigned const two_digit_year = __crt_get_2digit_year(timeptr->tm_year);
        store_number(two_digit_year, 2, string, left, alternate_form ? '\0' : '0');
        return true;
    }

    case L'Y': // year with century
    {
        _VALIDATE_RETURN(timeptr->tm_year >= -1900 && timeptr->tm_year <= 8099, EINVAL, false);
        unsigned const full_year = timeptr->tm_year + 1900;

        store_number(full_year, 4, string, left, alternate_form ? '\0' : '0');
        return true;
    }

    case L'z': // time zone in ISO 8601 form ("-0430" = 4 hours 30 minutes)
    {
        __tzset();

        // Get the current time zone offset from UTC and, if we are currently in
        // daylight savings time, adjust appropriately:
        long offset{};
        _VALIDATE_RETURN(_get_timezone(&offset) == 0, EINVAL, false);

        if (timeptr->tm_isdst)
        {
            long dst_bias{};
            _VALIDATE_RETURN(_get_dstbias(&dst_bias) == 0, EINVAL, false);

            offset += dst_bias;
        }

        long const positive_offset{offset < 0 ? -offset : offset};
        long const hours_offset  {(positive_offset / 60) / 60};
        long const minutes_offset{(positive_offset / 60) % 60};

        // This looks wrong, but it is correct:  The offset is the difference
        // between UTC and the local time zone, so it is a positive value if
        // the local time zone is behind UTC.
        wchar_t const* const sign_string{offset <= 0 ? L"+" : L"-"};

        store_string(sign_string, string, left);
        store_number(hours_offset,   2, string, left, '0');
        store_number(minutes_offset, 2, string, left, '0');
        return true;
    }

    case L'Z': // time zone name, if any
    {
        __tzset();
        store_string(__wide_tzname()[timeptr->tm_isdst ? 1 : 0], string, left);
        return true;
    }

    case L'%': // percent sign
    {
        store_string(L"%", string, left);
        return true;
    }

    default: // unknown format directive
    {
        // We do not raise the invalid parameter handler here.  Our caller will
        // raise the invalid parameter handler when we return failure.
        return false;
    }
    }

    // Unreachable.  All switch case statements return.
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// The _wcsftime family of functions
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// These functions format a time as a string using a given locale.  They place
// characters into the user's output buffer, expanding time format directives as
// described in the provided control string.  The lc_time_arg and locale are
// used for locale data.
//
// If the total number of characters that need to be written (including the null
// terminator) is less than the max_size, then the number of characters written
// (not including the null terminator) is returned.  Otherwise, zero is returned.
extern "C" size_t __cdecl _Wcsftime_l(
    wchar_t*       const string,
    size_t         const max_size,
    wchar_t const* const format,
    tm const*      const timeptr,
    void*          const lc_time_arg,
    _locale_t      const locale
    )
{
    _VALIDATE_RETURN(string != nullptr, EINVAL, 0)
    _VALIDATE_RETURN(max_size != 0,     EINVAL, 0)
    *string = L'\0';

    _VALIDATE_RETURN(format != nullptr, EINVAL, 0)

    _LocaleUpdate locale_update(locale);

    __crt_lc_time_data const* const lc_time = lc_time_arg == 0
        ? locale_update.GetLocaleT()->locinfo->lc_time_curr
        : static_cast<__crt_lc_time_data*>(lc_time_arg);

    // Copy the input string to the output string expanding the format
    // designations appropriately.  Stop copying when one of the following
    // is true: (1) we hit a null char in the input stream, or (2) there's
    // no room left in the output stream.

    wchar_t*       string_it = string;
    wchar_t const* format_it = format;

    bool failed = false;
    size_t remaining = max_size;

    while (remaining > 0)
    {
        switch (*format_it)
        {
        case L'\0':
        {
            // End of format input string
            goto done;
        }

        case L'%':
        {
            // Format directive.  Take appropriate action based on format control character.
            _VALIDATE_RETURN(timeptr != nullptr, EINVAL, 0);

            ++format_it; // Skip '%'

            // Process flags:
            bool alternate_form = false;
            if (*format_it == L'#')
            {
                alternate_form = true;
                ++format_it;
            }

            // Skip ISO E and O alternative representation format modifiers.  We
            // do not support alternative formats in any locale.
            if (*format_it == L'E' || *format_it == L'O')
            {
                ++format_it;
            }

            if (!expand_time(locale_update.GetLocaleT(), *format_it, timeptr, &string_it, &remaining, lc_time, alternate_form))
            {
                // if we don't have any space left, do not set the failure flag:
                // we will simply return ERANGE and do not call the invalid
                // parameter handler (see below)
                if (remaining > 0)
                    failed = true;

                goto done;
            }

            ++format_it; // Skip format char
            break;
        }

        default:
        {
            // store character, bump pointers, decrement the char count:
            *string_it++ = *format_it++;
            --remaining;
            break;
        }
        }
    }


    // All done.  See if we terminated because we hit a null char or because
    // we ran out of space:
    done:

    if (!failed && remaining > 0)
    {
        // Store a terminating null char and return the number of chars we
        // stored in the output string:
        *string_it = L'\0';
        return max_size - remaining;
    }
    else
    {
        // Error:  return an empty string:
        *string = L'\0';

        // Now return our error/insufficient buffer indication:
        if (!failed && remaining <= 0)
        {
            // Do not report this as an error to allow the caller to resize:
            errno = ERANGE;
        }
        else
        {
            _VALIDATE_RETURN(false, EINVAL, 0);
        }

        return 0;
    }
}

extern "C" size_t __cdecl _Wcsftime(
    wchar_t*       const buffer,
    size_t         const max_size,
    wchar_t const* const format,
    tm const*      const timeptr,
    void*          const lc_time_arg
    )
{
    return _Wcsftime_l(buffer, max_size, format, timeptr, lc_time_arg, nullptr);
}

extern "C" size_t __cdecl _wcsftime_l(
    wchar_t*       const buffer,
    size_t         const max_size,
    wchar_t const* const format,
    tm const*      const timeptr,
    _locale_t      const locale
    )
{
    return _Wcsftime_l(buffer, max_size, format, timeptr, nullptr, locale);
}

extern "C" size_t __cdecl wcsftime(
    wchar_t*       const buffer,
    size_t         const max_size,
    wchar_t const* const format,
    tm const*      const timeptr
    )
{
    return _Wcsftime_l(buffer, max_size, format, timeptr, nullptr, nullptr);
}



/*
 * Copyright (c) 1992-2013 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
V6.40:0009 */
