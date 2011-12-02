/* tls.h */

#ifndef __CRT_INTERNAL_TLS_H
#define __CRT_INTERNAL_TLS_H

#ifndef _CRT_PRECOMP_H
#error DO NOT INCLUDE THIS HEADER DIRECTLY
#endif

#include <stddef.h>
#include <time.h>
#include <locale.h>
#include <windef.h>
#include <winbase.h>
#include <winnt.h>

#include <internal/wine/eh.h>

typedef struct MSVCRT_threadlocaleinfostruct {
    int refcount;
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
    struct MSVCRT_lconv *lconv;
    int *ctype1_refcount;
    unsigned short *ctype1;
    unsigned short *pctype;
    unsigned char *pclmap;
    unsigned char *pcumap;
    struct __lc_time_data *lc_time_curr;
} MSVCRT_threadlocinfo;

typedef struct MSVCRT_threadmbcinfostruct {
    int refcount;
    int mbcodepage;
    int ismbcodepage;
    int mblcid;
    unsigned short mbulinfo[6];
    char mbctype[257];
    char mbcasemap[256];
} MSVCRT_threadmbcinfo;

struct MSVCRT_lconv {
    char* decimal_point;
    char* thousands_sep;
    char* grouping;
    char* int_curr_symbol;
    char* currency_symbol;
    char* mon_decimal_point;
    char* mon_thousands_sep;
    char* mon_grouping;
    char* positive_sign;
    char* negative_sign;
    char int_frac_digits;
    char frac_digits;
    char p_cs_precedes;
    char p_sep_by_space;
    char n_cs_precedes;
    char n_sep_by_space;
    char p_sign_posn;
    char n_sign_posn;
};

typedef struct MSVCRT_threadlocaleinfostruct *MSVCRT_pthreadlocinfo;
typedef struct MSVCRT_threadmbcinfostruct *MSVCRT_pthreadmbcinfo;

typedef struct MSVCRT_localeinfo_struct
{
    MSVCRT_pthreadlocinfo locinfo;
    MSVCRT_pthreadmbcinfo mbcinfo;
} MSVCRT__locale_tstruct, *MSVCRT__locale_t;

/* TLS data */
extern DWORD tls_index;

struct __thread_data {
    DWORD                    tid;
    HANDLE                   handle;
    int                      thread_errno;
    unsigned long            thread_doserrno;
    int                      unk1;
    unsigned int             random_seed;        /* seed for rand() */
    char                     *strtok_next;        /* next ptr for strtok() */
    wchar_t                  *wcstok_next;        /* next ptr for wcstok() */
    unsigned char            *mbstok_next;        /* next ptr for mbstok() */
    char                     *strerror_buffer;    /* buffer for strerror */
    wchar_t                  *wcserror_buffer;    /* buffer for wcserror */
    char                     *tmpnam_buffer;      /* buffer for tmpname() */
    wchar_t                  *wtmpnam_buffer;     /* buffer for wtmpname() */
    void                     *unk2[2];
    char                     *asctime_buffer;     /* buffer for asctime */
    wchar_t                  *wasctime_buffer;    /* buffer for wasctime */
    struct tm                *time_buffer;        /* buffer for localtime/gmtime */
    char                     *efcvt_buffer;       /* buffer for ecvt/fcvt */
    int                      unk3[2];
    void                     *unk4[4];
    int                      fpecode;
    MSVCRT_pthreadmbcinfo    mbcinfo;
    MSVCRT_pthreadlocinfo    locinfo;
    BOOL                     have_locale;
    int                      unk5[1];
    terminate_function       terminate_handler;
    unexpected_function      unexpected_handler;
    _se_translator_function  se_translator;
    void                     *unk6[3];
    int                      unk7;
    EXCEPTION_RECORD         *exc_record;
    void                     *unk8[100];
};

typedef struct __thread_data thread_data_t;

extern inline BOOL msvcrt_init_tls(void);
extern inline BOOL msvcrt_free_tls(void);
extern thread_data_t *msvcrt_get_thread_data(void);
extern inline void msvcrt_free_tls_mem(void);

#define MSVCRT_ENABLE_PER_THREAD_LOCALE 1
#define MSVCRT_DISABLE_PER_THREAD_LOCALE 2

extern MSVCRT__locale_t MSVCRT_locale;
MSVCRT_pthreadlocinfo get_locinfo(void);
void __cdecl MSVCRT__free_locale(MSVCRT__locale_t);
void free_locinfo(MSVCRT_pthreadlocinfo);
void free_mbcinfo(MSVCRT_pthreadmbcinfo);

#endif /* __MSVCRT_INTERNAL_TLS_H */

/* EOF */
