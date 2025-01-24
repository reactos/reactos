/*
 * COPYRIGHT:   LGPL, See LGPL.txt in the top level directory
 * PROJECT:     ReactOS CRT library
 * FILE:        lib/sdk/crt/time/timezone.c
 * PURPOSE:     Implementation of time zone functions
 * PROGRAMERS:  Timo Kreuzer
 */
#include "precomp.h"

char _tz_is_set = 0;

/* buffers must hold 64 characters! */
static char tz_name[64] = "PST";
static char tz_dst_name[64] = "PDT";

long dst_begin = 0;
long dst_end = 0;

/******************************************************************************
 * \var _tzname
 */
char * _tzname[2] = {
  tz_name,
  tz_dst_name,
};

/******************************************************************************
 * \var _daylight
 */
int _daylight = 0;

/******************************************************************************
 * \name __p__daylight
 * \brief Returns a pointer to the _daylight variable;
 */
void *
__p__daylight(void)
{
   return &_daylight;
}

/******************************************************************************
 * \var _timezone
 * \brief
 */
long _timezone = 28800;

/******************************************************************************
 * \name __p__timezone
 * \brief Returns a pointer to the _timezone variable;
 */
long *
__p__timezone(void)
{
   return &_timezone;
}

/******************************************************************************
 * \var _dstbias
 * \brief
 */
long _dstbias = 0;

/******************************************************************************
 * \name __p__dstbias
 * \brief Returns a pointer to the _dstbias variable;
 */
long *
__p__dstbias(void)
{
    return &_dstbias;
}

/******************************************************************************
 * \name __p__tzname
 * \brief Returns a pointer to the _tzname buffer;
 */
char **
__p__tzname(void)
{
    return _tzname;
}

/******************************************************************************
 * \name _tzset
 * \brief Initializes the variables _daylight, _timezone, and _tzname from the
 *        "TZ" environment variable if available or else by calling
 *        GetTimeZoneInformation.
 * \sa https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/tzset?view=msvc-170
 */
void
_tzset(void)
{
    const char * str;

    if (_tz_is_set)
    {
        return;
    }

    /* Try to read the timezone from environment */
    str = getenv("TZ");
    if (str && str[0] != 0)
    {
        long hour = 0, min = 0, sec = 0;
        size_t len = strnlen(str, 16);
        int sign = 1;

        dst_begin = 0;

        for (;;)
        {
            /* Copy timezone name */
            strncpy(tz_name, str, 3);
            str += 3;
            len -= 3;

            if (len < 1) break;

            if (*str == '+' || *str == '-')
            {
                sign = *str == '-' ? -1 : 1;
                str++;
                len--;
            }

            if (len < 1) break;

            hour = atol(str);

            while (*str != 0 && *str != ':') str++;
            if (*str == 0) break;

            min = atol(++str);

            while (*str != 0 && *str != ':') str++;
            if (*str == 0) break;

            sec = atol(++str);

            while (*str != 0 && *str <= '9') str++;
            if (*str == 0) break;

            /* Copy DST name */
            strncpy(tz_dst_name, str, 3);

            // FIXME: set dst_begin etc

            /* We are finished */
            break;
        }

        _timezone = sign * (((hour * 60) + min) * 60 + sec);

    }
    else
    {
        TIME_ZONE_INFORMATION tzi;
        DWORD ret;

        ret = GetTimeZoneInformation(&tzi);
        if (ret == TIME_ZONE_ID_INVALID)
        {
            return;
        }

        ret = WideCharToMultiByte(CP_ACP,
                                  0,
                                  tzi.StandardName,
                                  -1,
                                  tz_name,
                                  sizeof(tz_name),
                                  NULL,
                                  NULL);

        ret = WideCharToMultiByte(CP_ACP,
                                  0,
                                  tzi.DaylightName,
                                  -1,
                                  tz_dst_name,
                                  sizeof(tz_dst_name),
                                  NULL,
                                  NULL);

        _timezone = tzi.Bias * 60;

        if (tzi.DaylightDate.wMonth)
        {
            struct tm _tm;

            _daylight = 1;
            _dstbias = (tzi.DaylightBias - tzi.StandardBias) * 60;
            _tm.tm_year = 70;
            _tm.tm_mon = tzi.DaylightDate.wMonth - 1;
            _tm.tm_mday = tzi.DaylightDate.wDay;
            _tm.tm_hour = tzi.DaylightDate.wHour;
            _tm.tm_min = tzi.DaylightDate.wMinute;
            _tm.tm_sec = tzi.DaylightDate.wSecond;
            dst_begin = (long)_mkgmtime(&_tm);
            _tm.tm_mon = tzi.StandardDate.wMonth - 1;
            _tm.tm_mday = tzi.StandardDate.wDay;
            _tm.tm_hour = tzi.StandardDate.wHour;
            _tm.tm_min = tzi.StandardDate.wMinute;
            _tm.tm_sec = tzi.StandardDate.wSecond;
            dst_end = (long)_mkgmtime(&_tm);
        }
        else
        {
            _daylight = 0;
            _dstbias = 0;
        }

    }
    _tz_is_set = 1;
}

/*********************************************************************
 *		_get_tzname (MSVCRT.@)
 */
int CDECL _get_tzname(size_t *ret, char *buf, size_t bufsize, int index)
{
    char *str_timezone;

    switch (index)
    {
    case 0:
        str_timezone = tz_name;
        break;

    case 1:
        str_timezone = tz_dst_name;
        break;

    default:
        *_errno() = EINVAL;
        return EINVAL;
    }

    if (!ret || (!buf && (bufsize > 0)) || (buf && !bufsize))
    {
        *_errno() = EINVAL;
        return EINVAL;
    }

    *ret = strlen(str_timezone) + 1;
    if(!buf && !bufsize)
        return 0;

    strncpy(buf, str_timezone, bufsize);
    return 0;
}
