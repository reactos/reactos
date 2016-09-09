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

/* TLS data */
extern DWORD tls_index;

struct __thread_data {
    DWORD                                 tid;
    HANDLE                                handle;
    int                                   thread_errno;
    unsigned long                         thread_doserrno;
    int                                   unk1;
    unsigned int                          random_seed;        /* seed for rand() */
    char                                  *strtok_next;        /* next ptr for strtok() */
    wchar_t                               *wcstok_next;        /* next ptr for wcstok() */
    unsigned char                         *mbstok_next;        /* next ptr for mbstok() */
    char                                  *strerror_buffer;    /* buffer for strerror */
    wchar_t                               *wcserror_buffer;    /* buffer for wcserror */
    char                                  *tmpnam_buffer;      /* buffer for tmpname() */
    wchar_t                               *wtmpnam_buffer;     /* buffer for wtmpname() */
    void                                  *unk2[2];
    char                                  *asctime_buffer;     /* buffer for asctime */
    wchar_t                               *wasctime_buffer;    /* buffer for wasctime */
    struct tm                             *time_buffer;        /* buffer for localtime/gmtime */
    char                                  *efcvt_buffer;       /* buffer for ecvt/fcvt */
    int                                   unk3[2];
    void                                  *unk4[3];
    EXCEPTION_POINTERS                    *xcptinfo;
    int                                   fpecode;
    struct MSVCRT_threadmbcinfostruct     *mbcinfo;
    struct MSVCRT_threadlocaleinfostruct  *locinfo;
    BOOL                                  have_locale;
    int                                   unk5[1];
    terminate_function                    terminate_handler;
    unexpected_function                   unexpected_handler;
    _se_translator_function               se_translator;
    void                                  *unk6[3];
    int                                   unk7;
    EXCEPTION_RECORD                      *exc_record;
    void                                  *unk8[100];
};

typedef struct __thread_data thread_data_t;

extern BOOL msvcrt_init_tls(void);
extern BOOL msvcrt_free_tls(void);
extern thread_data_t *msvcrt_get_thread_data(void);
extern void msvcrt_free_tls_mem(void);

#define MSVCRT_ENABLE_PER_THREAD_LOCALE 1
#define MSVCRT_DISABLE_PER_THREAD_LOCALE 2

#endif /* __MSVCRT_INTERNAL_TLS_H */

/* EOF */
