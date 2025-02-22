/*
 * COPYRIGHT:   LGPL, See LGPL.txt in the top level directory
 * PROJECT:     ReactOS CRT library
 * FILE:        lib/sdk/crt/time/strftime.c
 * PURPOSE:
 * PROGRAMER:
 */
#include <precomp.h>

static inline BOOL strftime_date(char *str, size_t *pos, size_t max,
        BOOL alternate, const struct tm *mstm, MSVCRT___lc_time_data *time_data)
{
    char *format;
    SYSTEMTIME st;
    size_t ret;

    st.wYear = mstm->tm_year + 1900;
    st.wMonth = mstm->tm_mon + 1;
    st.wDayOfWeek = mstm->tm_wday;
    st.wDay = mstm->tm_mday;
    st.wHour = mstm->tm_hour;
    st.wMinute = mstm->tm_min;
    st.wSecond = mstm->tm_sec;
    st.wMilliseconds = 0;

    format = alternate ? time_data->str.names.date : time_data->str.names.short_date;
    ret = GetDateFormatA(time_data->lcid, 0, &st, format, NULL, 0);
    if(ret && ret<max-*pos)
        ret = GetDateFormatA(time_data->lcid, 0, &st, format, str+*pos, max-*pos);
    if(!ret) {
        *str = 0;
        *_errno() = EINVAL;
        return FALSE;
    }else if(ret > max-*pos) {
        *str = 0;
        *_errno() = ERANGE;
        return FALSE;
    }
    *pos += ret-1;
    return TRUE;
}

static inline BOOL strftime_time(char *str, size_t *pos, size_t max,
        const struct tm *mstm, MSVCRT___lc_time_data *time_data)
{
    SYSTEMTIME st;
    size_t ret;

    st.wYear = mstm->tm_year + 1900;
    st.wMonth = mstm->tm_mon + 1;
    st.wDayOfWeek = mstm->tm_wday;
    st.wDay = mstm->tm_mday;
    st.wHour = mstm->tm_hour;
    st.wMinute = mstm->tm_min;
    st.wSecond = mstm->tm_sec;
    st.wMilliseconds = 0;

    ret = GetTimeFormatA(time_data->lcid, 0, &st, time_data->str.names.time, NULL, 0);
    if(ret && ret<max-*pos)
        ret = GetTimeFormatA(time_data->lcid, 0, &st, time_data->str.names.time,
                str+*pos, max-*pos);
    if(!ret) {
        *str = 0;
        *_errno() = EINVAL;
        return FALSE;
    }else if(ret > max-*pos) {
        *str = 0;
        *_errno() = ERANGE;
        return FALSE;
    }
    *pos += ret-1;
    return TRUE;
}

static inline BOOL strftime_str(char *str, size_t *pos, size_t max, char *src)
{
    size_t len = strlen(src);
    if(len > max-*pos) {
        *str = 0;
        *_errno() = ERANGE;
        return FALSE;
    }

    memcpy(str+*pos, src, len);
    *pos += len;
    return TRUE;
}

static inline BOOL strftime_int(char *str, size_t *pos, size_t max,
        int src, int prec, int l, int h)
{
    size_t len;

    if(src<l || src>h) {
        *str = 0;
        *_errno() = EINVAL;
        return FALSE;
    }

    len = _snprintf(str+*pos, max-*pos, "%0*d", prec, src);
    if(len == -1) {
        *str = 0;
        *_errno() = ERANGE;
        return FALSE;
    }

    *pos += len;
    return TRUE;
}

/*********************************************************************
 *		_Strftime (MSVCRT.@)
 */
size_t CDECL _Strftime(char *str, size_t max, const char *format,
        const struct tm *mstm, void *_Lc_time_arg)
{
    MSVCRT___lc_time_data *time_data = (MSVCRT___lc_time_data*)_Lc_time_arg;
    size_t ret, tmp;
    BOOL alternate;

    TRACE("(%p %ld %s %p %p)\n", str, max, format, mstm, time_data);

    if(!str || !format) {
        if(str && max)
            *str = 0;
        *_errno() = EINVAL;
        return 0;
    }

    if(!time_data)
        time_data = get_locinfo()->lc_time_curr;

    for(ret=0; *format && ret<max; format++) {
        if(*format != '%') {
            str[ret++] = *format;
            continue;
        }

        format++;
        if(*format == '#') {
            alternate = TRUE;
            format++;
        }else {
            alternate = FALSE;
        }

        if(!mstm)
            goto einval_error;

        switch(*format) {
        case 'c':
            if(!strftime_date(str, &ret, max, alternate, mstm, time_data))
                return 0;
            if(ret < max)
                str[ret++] = ' ';
            if(!strftime_time(str, &ret, max, mstm, time_data))
                return 0;
            break;
        case 'x':
            if(!strftime_date(str, &ret, max, alternate, mstm, time_data))
                return 0;
            break;
        case 'X':
            if(!strftime_time(str, &ret, max, mstm, time_data))
                return 0;
            break;
        case 'a':
            if(mstm->tm_wday<0 || mstm->tm_wday>6)
                goto einval_error;
            if(!strftime_str(str, &ret, max, time_data->str.names.short_wday[mstm->tm_wday]))
                return 0;
            break;
        case 'A':
            if(mstm->tm_wday<0 || mstm->tm_wday>6)
                goto einval_error;
            if(!strftime_str(str, &ret, max, time_data->str.names.wday[mstm->tm_wday]))
                return 0;
            break;
        case 'b':
            if(mstm->tm_mon<0 || mstm->tm_mon>11)
                goto einval_error;
            if(!strftime_str(str, &ret, max, time_data->str.names.short_mon[mstm->tm_mon]))
                return 0;
            break;
        case 'B':
            if(mstm->tm_mon<0 || mstm->tm_mon>11)
                goto einval_error;
            if(!strftime_str(str, &ret, max, time_data->str.names.mon[mstm->tm_mon]))
                return 0;
            break;
        case 'd':
            if(!strftime_int(str, &ret, max, mstm->tm_mday, alternate ? 0 : 2, 0, 31))
                return 0;
            break;
        case 'H':
            if(!strftime_int(str, &ret, max, mstm->tm_hour, alternate ? 0 : 2, 0, 23))
                return 0;
            break;
        case 'I':
            tmp = mstm->tm_hour;
            if(tmp > 12)
                tmp -= 12;
            else if(!tmp)
                tmp = 12;
            if(!strftime_int(str, &ret, max, tmp, alternate ? 0 : 2, 1, 12))
                return 0;
            break;
        case 'j':
            if(!strftime_int(str, &ret, max, mstm->tm_yday+1, alternate ? 0 : 3, 1, 366))
                return 0;
            break;
        case 'm':
            if(!strftime_int(str, &ret, max, mstm->tm_mon+1, alternate ? 0 : 2, 1, 12))
                return 0;
            break;
        case 'M':
            if(!strftime_int(str, &ret, max, mstm->tm_min, alternate ? 0 : 2, 0, 59))
                return 0;
            break;
        case 'p':
            if(mstm->tm_hour<0 || mstm->tm_hour>23)
                goto einval_error;
            if(!strftime_str(str, &ret, max, mstm->tm_hour<12 ?
                        time_data->str.names.am : time_data->str.names.pm))
                return 0;
            break;
        case 'S':
            if(!strftime_int(str, &ret, max, mstm->tm_sec, alternate ? 0 : 2, 0, 59))
                return 0;
            break;
        case 'w':
            if(!strftime_int(str, &ret, max, mstm->tm_wday, 0, 0, 6))
                return 0;
            break;
        case 'y':
            if(!strftime_int(str, &ret, max, mstm->tm_year%100, alternate ? 0 : 2, 0, 99))
                return 0;
            break;
        case 'Y':
            tmp = 1900+mstm->tm_year;
            if(!strftime_int(str, &ret, max, tmp, alternate ? 0 : 4, 0, 9999))
                return 0;
            break;
        case 'z':
        case 'Z':
            _tzset();
            if(_get_tzname(&tmp, str+ret, max-ret, mstm->tm_isdst ? 1 : 0))
                return 0;
            ret += tmp;
            break;
        case 'U':
        case 'W':
            if(mstm->tm_wday<0 || mstm->tm_wday>6 || mstm->tm_yday<0 || mstm->tm_yday>365)
                goto einval_error;
            if(*format == 'U')
                tmp = mstm->tm_wday;
            else if(!mstm->tm_wday)
                tmp = 6;
            else
                tmp = mstm->tm_wday-1;

            tmp = mstm->tm_yday/7 + (tmp <= ((unsigned)mstm->tm_yday%7));
            if(!strftime_int(str, &ret, max, tmp, alternate ? 0 : 2, 0, 53))
                return 0;
            break;
        case '%':
            str[ret++] = '%';
            break;
        default:
            WARN("unknown format %c\n", *format);
            goto einval_error;
        }
    }

    if(ret == max) {
        if(max)
            *str = 0;
        *_errno() = ERANGE;
        return 0;
    }

    str[ret] = 0;
    return ret;

einval_error:
    *str = 0;
    *_errno() = EINVAL;
    return 0;
}

/*********************************************************************
 *		strftime (MSVCRT.@)
 */
size_t CDECL strftime( char *str, size_t max, const char *format,
                                     const struct tm *mstm )
{
    return _Strftime(str, max, format, mstm, NULL);
}

/*********************************************************************
 *		wcsftime (MSVCRT.@)
 */
size_t CDECL wcsftime( wchar_t *str, size_t max,
                                     const wchar_t *format, const struct tm *mstm )
{
    char *s, *fmt;
    size_t len;

    TRACE("%p %ld %s %p\n", str, max, debugstr_w(format), mstm );

    len = WideCharToMultiByte( CP_ACP, 0, format, -1, NULL, 0, NULL, NULL );
    if (!(fmt = malloc( len ))) return 0;
    WideCharToMultiByte( CP_ACP, 0, format, -1, fmt, len, NULL, NULL );

    if ((s = malloc( max*4 )))
    {
        if (!strftime( s, max*4, fmt, mstm )) s[0] = 0;
        len = MultiByteToWideChar( CP_ACP, 0, s, -1, str, max );
        if (len) len--;
        free( s );
    }
    else len = 0;

    free( fmt );
    return len;
}
