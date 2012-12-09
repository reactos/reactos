#ifndef __CRT_INTERNAL_LOCALE_H
#define __CRT_INTERNAL_LOCALE_H

typedef struct MSVCRT_threadmbcinfostruct *MSVCRT_pthreadmbcinfo;

typedef struct __lc_time_data {
    union {
        char *str[43];
        struct {
            char *short_wday[7];
            char *wday[7];
            char *short_mon[12];
            char *mon[12];
            char *am;
            char *pm;
            char *short_date;
            char *date;
            char *time;
        } names;
    } str;
    LCID lcid;
    int  unk[2];
    wchar_t *wstr[43];
    char data[1];
} MSVCRT___lc_time_data;

int _setmbcp_l(int, LCID, MSVCRT_pthreadmbcinfo) DECLSPEC_HIDDEN;
MSVCRT_pthreadmbcinfo get_mbcinfo(void) DECLSPEC_HIDDEN;
LCID MSVCRT_locale_to_LCID(const char *locale) DECLSPEC_HIDDEN;

#endif //__CRT_INTERNAL_LOCALE_H

