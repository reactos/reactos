/*
 * COPYRIGHT:   LGPL, See LGPL.txt in the top level directory
 * PROJECT:     ReactOS CRT library
 * FILE:        lib/sdk/crt/time/asctime.c
 * PURPOSE:     Implementation of asctime(), _asctime_s()
 * PROGRAMERS:  Timo Kreuzer
 */
#include <precomp.h>
#include <tchar.h>
#include <time.h>
#include "bitsfixup.h"

#define DAYSPERWEEK 7
#define MONSPERYEAR 12
#define HUNDREDYEAROFFSET 19

static const _TCHAR wday_name[DAYSPERWEEK][5] =
{
    _T("Sun "), _T("Mon "), _T("Tue "), _T("Wed "), 
    _T("Thu "), _T("Fri "), _T("Sat ")
};

static const _TCHAR mon_name[MONSPERYEAR][5] = 
{
    _T("Jan "), _T("Feb "), _T("Mar "), _T("Apr "), _T("May "), _T("Jun "),
    _T("Jul "), _T("Aug "), _T("Sep "), _T("Oct "), _T("Nov "), _T("Dec ")
};

#ifdef _UNICODE
typedef unsigned long long _TCHAR4;
typedef unsigned long _TCHAR2;
#else
typedef unsigned long _TCHAR4;
typedef unsigned short _TCHAR2;
#endif

#pragma pack(push,1)
typedef union
{
    _TCHAR text[26];
    struct
    {
        _TCHAR4 WeekDay;
        _TCHAR4 Month;
        _TCHAR2 Day;
        _TCHAR Space1;
        _TCHAR2 Hour;
        _TCHAR Sep1;
        _TCHAR2 Minute;
        _TCHAR Sep2;
        _TCHAR2 Second;
        _TCHAR Space2;
        _TCHAR2 Year[2];
        _TCHAR lb;
        _TCHAR zt;
    };
} timebuf_t;
#pragma pack(pop)

FORCEINLINE
_TCHAR2
IntToChar2(int x)
{
    union
    {
        _TCHAR2 char2;
        _TCHAR array[2];
    } u;

    u.array[0] = '0' + (x / 10);
    u.array[1] = '0' + (x % 10);

    return u.char2;
}

static __inline
void
FillBuf(timebuf_t *buf, const struct tm *ptm)
{
    /* Format looks like this: 
     * "Sun Mar 01 12:34:56 1902\n\0" */
    buf->WeekDay = *(_TCHAR4*)wday_name[ptm->tm_wday];
    buf->Month = *(_TCHAR4*)mon_name[ptm->tm_mon];
    buf->Day = IntToChar2(ptm->tm_mday);
    buf->Space1 = ' ';
    buf->Hour = IntToChar2(ptm->tm_hour);
    buf->Sep1 = ':';
    buf->Minute = IntToChar2(ptm->tm_min);
    buf->Sep2 = ':';
    buf->Second = IntToChar2(ptm->tm_sec);
    buf->Space2 = ' ';
    buf->Year[0] = IntToChar2(ptm->tm_year / 100 + HUNDREDYEAROFFSET);
    buf->Year[1] = IntToChar2(ptm->tm_year % 100);
    buf->lb = '\n';
    buf->zt = '\0';
}

/******************************************************************************
 * \name _tasctime_s
 * \brief Converts a local time into a string and returns a pointer to it.
 * \param buffer Buffer that receives the string (26 characters).
 * \param numberOfElements Size of the buffer in characters.
 * \param time Pointer to the UTC time.
 */
errno_t 
_tasctime_s( 
    _TCHAR* buffer,
    size_t numberOfElements,
    const struct tm *ptm)
{
    /* Validate parameters */
    if (!buffer || numberOfElements < 26 || !ptm ||
        (unsigned int)ptm->tm_sec > 59 ||
        (unsigned int)ptm->tm_min > 59 ||
        (unsigned int)ptm->tm_hour > 23 ||
        (unsigned int)ptm->tm_mday > 31 ||
        (unsigned int)ptm->tm_mon > 11 ||
        (unsigned int)ptm->tm_year > 2038 ||
        (unsigned int)ptm->tm_wday > 6 ||
        (unsigned int)ptm->tm_yday > 365)
    {
#if 0
        _invalid_parameter(NULL,
#ifdef UNICODE
                            L"_wasctime",
#else
                            L"asctime",
#endif
                           _CRT_WIDE(__FILE__),
                           __LINE__,
                           0);
#endif
        return EINVAL;
    }

    /* Fill the buffer */
    FillBuf((timebuf_t*)buffer, ptm);

    return 0;
}

/******************************************************************************
 * \name _tasctime
 * \brief Converts a UTC time into a string and returns a pointer to it.
 * \param ptm Pointer to the UTC time.
 * \remarks The string is stored in thread local buffer, shared between
 *          ctime, gmtime and localtime (32 and 64 bit versions).
 */
_TCHAR *
_tasctime(const struct tm *ptm)
{
    thread_data_t *data = msvcrt_get_thread_data();
    _TCHAR *pstr;

#ifndef _UNICODE
    pstr = data->asctime_buffer;
#else
    pstr = data->wasctime_buffer;
#endif

    if(!pstr)
        pstr = malloc(sizeof(struct tm));

    /* Fill the buffer */
    FillBuf((timebuf_t*)pstr, ptm);

    return pstr;
}
