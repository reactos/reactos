/*
* PROJECT:         ReactOS CRT API tests
* LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
* PURPOSE:         Tests for crt data exports
* COPYRIGHT:       Copyright 2021 Timo Kreuzer <timo.kreuzer@reactos.org>
*/

#include <apitest.h>
#include <apitest_guard.h>

#define WIN32_NO_STATUS
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#include <mbctype.h>

static OSVERSIONINFOW s_osvi;
extern char __ImageBase;
static void* s_ImageEnd;

int IsLocalSymbol(void* Address)
{
    return ((Address >= (void*)&__ImageBase) && (Address <= s_ImageEnd));
}

#ifndef TEST_STATIC
#define test_is_local_symbol(addr, is) ok_int(IsLocalSymbol(addr), (is))
#else
#define test_is_local_symbol(addr, is)
#endif

void Test___argc(void)
{
    void* p = &__argc;
    test_is_local_symbol(p, FALSE);

    #undef __argc
    ok_ptr(&__argc, p);
#ifdef _M_IX86
    ok_ptr(__p___argc(), p);
#endif
}

void Test___argv(void)
{
    void* p = &__argv;
    test_is_local_symbol(p, FALSE);

    #undef __argv
    ok_ptr(&__argv, p);
#ifdef _M_IX86
    ok_ptr(__p___argv(), p);
#endif
}

void Test___badioinfo(void)
{
    typedef struct _ioinfo ioinfo;
    _CRTIMP extern ioinfo* __badioinfo[];
    ok(__badioinfo != NULL, "__badioinfo is NULL\n");
    ok(__badioinfo[0] != NULL, "__badioinfo is NULL\n");
}

#ifndef _M_ARM
void Test___initenv(void)
{
    _CRTIMP extern char** __initenv;
    ok(__initenv != NULL, "__initenv is NULL\n");
    ok(*__initenv != NULL, "*__initenv is NULL\n");
#ifdef _M_IX86
    _CRTIMP char*** __p___initenv(void);
    ok_ptr(__p___initenv(), &__initenv);
#endif
}

void Test___lc_codepage(void)
{
    _CRTIMP extern unsigned int __lc_codepage;
    ok_int(__lc_codepage, 0);
    ok_int(___lc_codepage_func(), 0);
    __lc_codepage++;
    todo_ros ok_int(___lc_codepage_func(), 0);
    __lc_codepage--;
}

void Test___lc_collate_cp(void)
{
    _CRTIMP extern int __lc_collate_cp;
    test_is_local_symbol(&__lc_collate_cp, FALSE);
    ok_int(__lc_collate_cp, 0);
    ok_int(___lc_collate_cp_func(), 0);
    __lc_collate_cp++;
    ok_int(___lc_collate_cp_func(), 0);
    __lc_collate_cp--;
}
#endif // !_M_ARM

void Test___lc_handle(void)
{
    _CRTIMP int __lc_handle;
    ok_int(__lc_handle, 0);
    _CRTIMP int* ___lc_handle_func();
    ok_int(*___lc_handle_func(), 0);
    __lc_handle++;
    todo_ros ok_int(*___lc_handle_func(), 0);
    __lc_handle--;
}

void Test___mb_cur_max(void)
{
    void* p = &__mb_cur_max;
    test_is_local_symbol(&__mb_cur_max, FALSE);
    ok_int(__mb_cur_max, 1);

    #undef __mb_cur_max
    _CRTIMP extern int __mb_cur_max;
    ok_ptr(&__mb_cur_max, p);

    ok_int(___mb_cur_max_func(), 1);
#ifdef _M_IX86
    _CRTIMP int* __p___mb_cur_max(void);
    ok_int(*__p___mb_cur_max(), 1);
#endif
    __mb_cur_max++;
    if (s_osvi.dwMajorVersion >= 6)
    {
        ok_int(___mb_cur_max_func(), 1);
#ifdef _M_IX86
        ok_int(*__p___mb_cur_max(), 1);
#endif
    }
    else
    {
        todo_ros ok_int(___mb_cur_max_func(), 2);
#ifdef _M_IX86
        todo_ros ok_ptr(__p___mb_cur_max(), p); // wine code handles it like on Vista+
        todo_ros ok_int(*__p___mb_cur_max(), 2);
#endif
    }

    __mb_cur_max--;
}

void Test___pioinfo(void)
{

}

#ifndef _M_ARM
void Test___setlc_active(void)
{
    _CRTIMP extern unsigned int __setlc_active;
    ok_int(__setlc_active, 0);

    _CRTIMP unsigned int ___setlc_active_func(void);
    ok_int(___setlc_active_func(), __setlc_active);
    __setlc_active++;
    ok_int(___setlc_active_func(), __setlc_active);
    __setlc_active--;
}

void Test___unguarded_readlc_active(void)
{
    _CRTIMP extern unsigned int __unguarded_readlc_active;
    void* p = &__unguarded_readlc_active;
    ok_int(__unguarded_readlc_active, 0);

    _CRTIMP unsigned int* ___unguarded_readlc_active_add_func(void);
    ok_ptr(___unguarded_readlc_active_add_func(), p);
}
#endif // !_M_ARM

void Test___wargv(void)
{
    void* p = &__wargv;
    test_is_local_symbol(p, FALSE);

    #undef __wargv
    _CRTIMP extern wchar_t ** __wargv;
    ok_ptr(&__wargv, p);
#ifdef _M_IX86
    ok_ptr(__p___wargv(), p);
#endif
}

#ifndef _M_ARM
void Test___winitenv(void)
{
    _CRTIMP extern wchar_t** __winitenv;
    todo_ros ok(__winitenv == NULL, "__winitenv is not NULL\n");
#ifdef _M_IX86
    _CRTIMP wchar_t*** __p___winitenv(void);
    ok_ptr(__p___winitenv(), &__winitenv);
#endif
}
#endif

void Test__acmdln(void)
{
    _CRTIMP extern char* _acmdln;
    ok(_acmdln != NULL, "__winitenv is NULL\n");
#ifdef _M_IX86
    _CRTIMP char** __p__acmdln(void);
    ok_ptr(__p__acmdln(), &_acmdln);
#endif
}

#ifdef _M_IX86
void Test__adjust_fdiv(void)
{
    _CRTIMP extern int _adjust_fdiv;
    ok_int(_adjust_fdiv, 0);
}
#endif

void Test__aexit_rtn(void)
{
    typedef void (*_exit_t)(int exitcode);
    _CRTIMP extern _exit_t _aexit_rtn;
    ok_ptr(_aexit_rtn, _exit);
}

void Test__commode(void)
{
    void* p = &_commode;
    test_is_local_symbol(&_commode, FALSE);
    ok_int(_commode, 0);

    #undef _commode
    _CRTIMP extern int _commode;
    ok_ptr(&_commode, p);
#ifdef _M_IX86
    ok_ptr(__p__commode(), &_commode);
#endif
}

void Test__ctype(void)
{
    _CRTIMP extern const unsigned short _ctype[];
    ok_int(_ctype[0], 0);
    ok_int(_ctype[1], _CONTROL);

    #undef _pctype
    _CRTIMP extern const unsigned short* _pctype;
    ok(_pctype != &_ctype[0], "_pwctype should not match &_wctype[0]");
    if (s_osvi.dwMajorVersion >= 6)
    {
        ok(_pctype != &_ctype[1], "_pwctype should not match &_wctype[1]");
    }
    else
    {
        ok(_pctype == &_ctype[1], "_pwctype should match &_wctype[1]");
    }

    ok(__pctype_func() != _ctype, "__pctype_func() should not match _ctype\n");
    ok_int(__pctype_func()[0], _CONTROL);
    ok_int(__pctype_func()[1], _CONTROL);
#ifdef _M_IX86
    _CRTIMP const unsigned short** __cdecl __p__pctype(void);
    ok_ptr(*__p__pctype(), __pctype_func());
#endif
}

void Test__wctype(void)
{
    ok_int(_wctype[0], 0);
    ok_int(_wctype[1], _CONTROL);

    #undef _pwctype
    _CRTIMP extern const unsigned short* _pwctype;
    ok_ptr(_pwctype, &_wctype[1]);

    ok(__pwctype_func() != _wctype, "__pwctype_func() == _wctype\n");
    ok_int(__pctype_func()[0], _CONTROL);
    ok_int(__pctype_func()[1], _CONTROL);
#ifdef _M_IX86
    _CRTIMP const unsigned short** __cdecl __p__pwctype(void);
    ok_ptr(*__p__pwctype(), __pwctype_func());
#endif
}

void Test__daylight(void)
{
    void* p = &_daylight;
    test_is_local_symbol(&_daylight, FALSE);

    #undef _daylight
    _CRTIMP extern int _daylight;
    ok_ptr(&_daylight, p);

#ifdef _M_IX86
    _CRTIMP void* __p__daylight(void);
    ok_ptr(__p__daylight(), &_daylight);
#endif
#if (WINVER >= 0x600)
    _CRTIMP int* __cdecl __daylight(void);
    ok_ptr(&__daylight, &_daylight);
#endif
}

#ifndef _M_ARM
void Test__dstbias(void)
{
    void* p = &_dstbias;
    test_is_local_symbol(&_dstbias, FALSE);

    #undef _dstbias
    _CRTIMP extern long _dstbias;
    ok_ptr(&_dstbias, p);
#ifdef _M_IX86
    _CRTIMP long* __cdecl __p__dstbias(void);
    ok_ptr(__p__dstbias(), &_dstbias);
#endif
#if (WINVER >= 0x600)
    _CRTIMP long* __cdecl __dstbias(void);
    ok_ptr(&__dstbias, &_dstbias);
#endif
}

void Test__environ(void)
{
    void* p = &_environ;
    ok(_environ != NULL, "_environ == NULL\n");

    #undef _environ
    _CRTIMP extern char** _environ;
    ok_ptr(&_environ, p);
#ifdef _M_IX86
    ok_ptr(__p__environ(), &_environ);
#endif
}

void Test__fileinfo(void)
{
    _CRTIMP extern int _fileinfo;
    ok_int(_fileinfo, -1);

#ifdef _M_IX86
    _CRTIMP int* __p__fileinfo();
    ok_ptr(__p__fileinfo(), &_fileinfo);
#endif
}
#endif // !_M_ARM

void Test__fmode(void)
{
    void* p = &_fmode;
    test_is_local_symbol(&_fmode, FALSE);
    ok_int(_fmode, 0);

    #undef _fmode
    _CRTIMP extern int _fmode;
    ok_ptr(&_fmode, p);

#ifdef _M_IX86
    _CRTIMP int* __cdecl __p__fmode();
    ok_ptr(__p__fmode(), p);
#endif

#if (_WIN32_WINNT >= 0x600)
    _fmode = 1234;
    _CRTIMP errno_t __cdecl _get_fmode(_Out_ int* _PMode);
    int mode;
    ok_int(_get_fmode(&mode), 0);
    ok_int(mode, _fmode);
    _fmode = 0;
#endif
}

void Test__iob(void)
{
    void* p = &_iob;
    test_is_local_symbol(&_iob, FALSE);
    ok_ptr(&_iob[0], stdin);
    ok_ptr(&_iob[1], stdout);
    ok_ptr(&_iob[2], stderr);

    #undef _iob
    _CRTIMP extern FILE _iob[];
    ok_ptr(&_iob, p);

    ok_ptr(__iob_func(), &_iob);

#ifdef _M_IX86
    _CRTIMP int* __cdecl __p__iob();
    ok_ptr(__p__iob(), p);
#endif
}

void Test__mbcasemap(void)
{
    void* p = &_mbcasemap;
    ok_int(_mbcasemap[0], 0);

    #undef _mbcasemap
    ok_ptr(_mbcasemap, p);

#ifdef _M_IX86
    _CRTIMP unsigned char* __cdecl __p__mbcasemap();
    ok_ptr(__p__mbcasemap(), &_mbcasemap);
#endif
}

void Test__mbctype(void)
{
    void* p = &_mbctype;
    ok_int(_mbctype[0], 0);

    #undef _mbctype
    ok_ptr(&_mbctype, p);

#ifdef _M_IX86
    _CRTIMP unsigned char* __cdecl __p__mbctype();
    todo_ros ok_ptr(__p__mbctype(), &_mbctype); // wine implements thiss like on Vista
#endif
}

#ifndef _M_ARM
void Test__osplatform(void)
{
    ok_int(_osplatform, s_osvi.dwPlatformId);
#if (WINVER >= 0x600)
    _CRTIMP unsigned int __cdecl _get_osplatform(void);
    ok_ptr(_get_osplatform(), _osplatform);
#endif
}
#endif

void Test__osver(void)
{
    ok_int(_osver, s_osvi.dwBuildNumber);

#ifdef _M_IX86
    _CRTIMP int* __cdecl __p__osver();
    ok_ptr(__p__osver(), &_osver);
#endif
}

void Test__pgmptr(void)
{
    void* p = &_pgmptr;
    ok(_pgmptr != NULL, "_pgmptr should not be NULL\n");

    #undef _pgmptr
    _CRTIMP extern char* _pgmptr;
    ok_ptr(&_pgmptr, p);
#ifdef _M_IX86
    ok_ptr(__p__pgmptr(), &_pgmptr);
#endif
#if (WINVER >= 0x600)
    _CRTIMP char* __cdecl _get_pgmptr(void);
    ok_ptr(_get_pgmptr(), _pgmptr);
#endif
}

void Test__sys_errlist(void)
{
    void* p = &_sys_errlist;
    ok_int(strcmp(_sys_errlist[0], strerror(0)), 0);
    ok_int(strcmp(_sys_errlist[42], strerror(42)), 0);

    #undef _sys_errlist
    _CRTIMP extern char* _sys_errlist[];
    ok_ptr(&_sys_errlist, p);
}

void Test__sys_nerr(void)
{
    void* p = &_sys_nerr;
    ok_int(_sys_nerr, 43);

    #undef _sys_nerr
    _CRTIMP extern int _sys_nerr;
    ok_ptr(&_sys_nerr, p);
}

void Test__timezone(void)
{
    void* p = &_timezone;
    test_is_local_symbol(&_timezone, FALSE);

    #undef _timezone
    _CRTIMP extern long _timezone;
    ok_ptr(&_timezone, p);

#ifdef _M_IX86
    _CRTIMP char** __p__timezone();
    ok_ptr(__p__timezone(), &_timezone);
#endif
}

void Test__tzname(void)
{
    void* p = &_tzname;
    test_is_local_symbol(&_tzname, FALSE);
    ok(_tzname[0] != NULL, "_tzname[0] == NULL\n");
    ok(_tzname[0] != NULL, "_tzname[0] == NULL\n");

    #undef _tzname
    _CRTIMP extern char * _tzname[2];
    ok_ptr(_tzname, p);

#ifdef _M_IX86
    _CRTIMP char** __p__tzname();
    ok_ptr(__p__tzname(), &_tzname);
#endif
#if (WINVER >= 0x600)
    _CRTIMP char* __cdecl __tzname(void);
    ok_ptr(__tzname(), _wenviron);
#endif
}

void Test__wcmdln(void)
{
    _CRTIMP extern wchar_t* _wcmdln;
#ifdef _M_IX86
    _CRTIMP wchar_t** __p__wcmdln(void);
    ok_ptr(__p__wcmdln(), &_wcmdln);
#endif
}

#ifndef _M_ARM
void Test__wenviron(void)
{
    void* p = &_wenviron;
    todo_ros ok(_wenviron == NULL, "_wenviron is not NULL\n");

    #undef _wenviron
    _CRTIMP extern wchar_t** _wenviron;
    ok_ptr(&_wenviron, p);
#ifdef _M_IX86
    ok_ptr(__p__wenviron(), &_wenviron);
#endif
#if (WINVER >= 0x600)
    _CRTIMP unsigned int __cdecl _get_wenviron(void);
    ok_int(_get_wenviron(), _wenviron);
#endif
}
#endif

void Test__winmajor(void)
{
    ok_int(_winmajor, s_osvi.dwMajorVersion);
#ifdef _M_IX86
    _CRTIMP unsigned int* __cdecl __p__winmajor();
    ok_ptr(__p__winmajor(), &_winmajor);
#endif
#if (WINVER >= 0x600)
    _CRTIMP unsigned int __cdecl _get_winmajor(void);
    ok_int(_get_winmajor(), _winmajor);
#endif
}

void Test__winminor(void)
{
    ok_int(_winminor, s_osvi.dwMinorVersion);
#ifdef _M_IX86
    _CRTIMP unsigned int* __cdecl __p__winminor();
    ok_ptr(__p__winminor(), &_winminor);
#endif
#if (WINVER >= 0x600)
    _CRTIMP unsigned int __cdecl _get_winminor(void);
    ok_int(_get_winminor(), _winminor);
#endif
}

#ifndef _M_ARM
void Test__winver(void)
{
    ok_int(_winver, (s_osvi.dwMajorVersion << 8) | s_osvi.dwMinorVersion);
#ifdef _M_IX86
    _CRTIMP unsigned int* __cdecl __p__winver();
    ok_ptr(__p__winver(), &_winver);
#endif
#if (WINVER >= 0x600)
    _CRTIMP unsigned int __cdecl _get_winver(void);
    ok_int(_get_winver(), _winver);
#endif
}
#endif

void Test__wpgmptr(void)
{
    void* p = _wpgmptr;
    ok_ptr(_wpgmptr, NULL);

    #undef _wpgmptr
    ok_ptr(_wpgmptr, p);

#ifdef _M_IX86
    _CRTIMP wchar_t ** __cdecl __p__wpgmptr();
    ok_ptr(__p__wpgmptr(), &_wpgmptr);
#endif
#if (WINVER >= 0x600)
    _CRTIMP unsigned int __cdecl _get_wpgmptr(void);
    ok_int(_get_wpgmptr(), _wpgmptr);
#endif
}

START_TEST(crtdata)
{
    /* Initialize image size */
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)&__ImageBase;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((PUCHAR)dosHeader + dosHeader->e_lfanew);
    s_ImageEnd = (PUCHAR)dosHeader + ntHeaders->OptionalHeader.SizeOfImage;

    /* initialize version info */
    s_osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
    GetVersionExW(&s_osvi);

    Test___argc();
    Test___argv();
    Test___badioinfo();
#ifndef _M_ARM
    Test___initenv();
    Test___lc_codepage();
    Test___lc_collate_cp();
#endif
    Test___lc_handle();
    Test___mb_cur_max();
    Test___pioinfo();
#ifndef _M_ARM
    Test___setlc_active();
    Test___unguarded_readlc_active();
#endif
    Test___wargv();
#ifndef _M_ARM
    Test___winitenv();
#endif
    Test__acmdln();
#ifdef _M_IX86
    Test__adjust_fdiv();
#endif
    Test__aexit_rtn();
    Test__commode();
    Test__ctype();
    Test__daylight();
#ifndef _M_ARM
    Test__dstbias();
    Test__environ();
    Test__fileinfo();
#endif
    Test__fmode();
    Test__iob();
    Test__mbcasemap();
    Test__mbctype();
#ifndef _M_ARM
    Test__osplatform();
#endif
    Test__osver();
    Test__pgmptr();
    Test__sys_errlist();
    Test__sys_nerr();
    Test__timezone();
    Test__tzname();
    Test__wcmdln();
    Test__wctype();
#ifndef _M_ARM
    Test__wenviron();
#endif
    Test__winmajor();
    Test__winminor();
#ifndef _M_ARM
    Test__winver();
#endif
    Test__wpgmptr();
}
