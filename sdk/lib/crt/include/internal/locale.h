#ifndef __CRT_INTERNAL_LOCALE_H
#define __CRT_INTERNAL_LOCALE_H

#include <crtdefs.h> // for LC_ID

typedef struct MSVCRT_threadlocaleinfostruct {
    LONG refcount;
    unsigned int lc_codepage;
    unsigned int lc_collate_cp;
    unsigned long lc_handle[6];
    LC_ID lc_id[6];
    struct {
        char *locale;
        wchar_t *wlocale;
        int *refcount;
        int *wrefcount;
    } lc_category[6];
    int lc_clike;
    int mb_cur_max;
    int *lconv_intl_refcount;
    int *lconv_num_refcount;
    int *lconv_mon_refcount;
    struct lconv *lconv;
    int *ctype1_refcount;
    unsigned short *ctype1;
    const unsigned short *pctype;
    unsigned char *pclmap;
    unsigned char *pcumap;
    struct __lc_time_data *lc_time_curr;
} MSVCRT_threadlocinfo;

typedef struct MSVCRT_threadmbcinfostruct {
    LONG refcount;
    int mbcodepage;
    int ismbcodepage;
    int mblcid;
    unsigned short mbulinfo[6];
    unsigned char mbctype[257];
    char mbcasemap[256];
} MSVCRT_threadmbcinfo;

#define MSVCRT_lconv lconv

typedef struct MSVCRT_threadlocaleinfostruct *MSVCRT_pthreadlocinfo;
typedef struct MSVCRT_threadmbcinfostruct *MSVCRT_pthreadmbcinfo;

typedef struct MSVCRT_localeinfo_struct
{
    MSVCRT_pthreadlocinfo locinfo;
    MSVCRT_pthreadmbcinfo mbcinfo;
} MSVCRT__locale_tstruct, *MSVCRT__locale_t;

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
LCID MSVCRT_locale_to_LCID(const char*, unsigned short*) DECLSPEC_HIDDEN;

void __init_global_locale();
extern MSVCRT__locale_t global_locale;
#define MSVCRT_locale __get_MSVCRT_locale()
FORCEINLINE MSVCRT__locale_t __get_MSVCRT_locale()
{
    if(!global_locale)
        __init_global_locale();
    return global_locale;
}

MSVCRT_pthreadlocinfo get_locinfo(void);
void __cdecl MSVCRT__free_locale(MSVCRT__locale_t);
void free_locinfo(MSVCRT_pthreadlocinfo);
void free_mbcinfo(MSVCRT_pthreadmbcinfo);

#endif //__CRT_INTERNAL_LOCALE_H

