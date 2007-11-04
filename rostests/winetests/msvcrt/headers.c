/*
 * Copyright 2004 Dimitrie O. Paun
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * This file contains tests to ensure the consistency between symbols
 * defined in the regular msvcrt headers, and the corresponding duplicated
 * symbols defined in msvcrt.h (prefixed by MSVCRT_).
 */

#include "dos.h"
#include "math.h"
#include "stdlib.h"
#include "io.h"
#include "errno.h"
#include "fcntl.h"
#include "malloc.h"
#include "limits.h"
#include "mbctype.h"
#include "stdio.h"
#include "wchar.h"
#include "ctype.h"
//#include "crtdbg.h"
#include "share.h"
#include "search.h"
#include "wctype.h"
#include "float.h"
#include "stddef.h"
#include "mbstring.h"
#include "sys/locking.h"
#include "sys/utime.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/timeb.h"
#include "direct.h"
#include "conio.h"
#include "process.h"
#include "string.h"
#include "signal.h"
#include "time.h"
#include "locale.h"
#include "setjmp.h"
#include "wine/test.h"

#ifdef __WINE_USE_MSVCRT
/* Wine-specific msvcrt headers */
#define __WINE_MSVCRT_TEST
#include "eh.h"
#include "msvcrt.h"

#ifdef __GNUC__
#define TYPEOF(type) typeof(type)
#else
#define TYPEOF(type) int
#endif
#define MSVCRT(x)    MSVCRT_##x
#define OFFSET(T,F) ((unsigned int)((char *)&((struct T *)0L)->F - (char *)0L))
#define CHECK_SIZE(e) ok(sizeof(e) == sizeof(MSVCRT(e)), "Element has different sizes\n")
#define CHECK_TYPE(t) { TYPEOF(t) a = 0; TYPEOF(MSVCRT(t)) b = 0; a = b; CHECK_SIZE(t); }
#define CHECK_STRUCT(s) ok(sizeof(struct s) == sizeof(struct MSVCRT(s)), "Struct has different sizes\n")
#define CHECK_FIELD(s,e) ok(OFFSET(s,e) == OFFSET(MSVCRT(s),e), "Bad offset\n")
#define CHECK_DEF(d) ok(d == MSVCRT_##d, "Defines (MSVCRT_)" #d " are different: %d vs. %d\n", d, MSVCRT_##d)

/************* Checking types ***************/
static void test_types(void)
{
    CHECK_TYPE(wchar_t);
    CHECK_TYPE(wint_t);
    CHECK_TYPE(wctype_t);
    CHECK_TYPE(_ino_t);
    CHECK_TYPE(_fsize_t);
    CHECK_TYPE(size_t);
    CHECK_TYPE(intptr_t);
    CHECK_TYPE(uintptr_t);
    CHECK_TYPE(_dev_t);
    CHECK_TYPE(_off_t);
    CHECK_TYPE(clock_t);
    CHECK_TYPE(time_t);
    CHECK_TYPE(__time64_t);
    CHECK_TYPE(fpos_t);
    CHECK_SIZE(FILE);
    CHECK_TYPE(terminate_handler);
    CHECK_TYPE(terminate_function);
    CHECK_TYPE(unexpected_handler);
    CHECK_TYPE(unexpected_function);
    CHECK_TYPE(_se_translator_function);
    CHECK_TYPE(_beginthread_start_routine_t);
    CHECK_TYPE(_onexit_t);
    CHECK_TYPE(__sighandler_t);
}

/************* Checking structs ***************/
static void test_structs(void)
{
    CHECK_STRUCT(tm);
    CHECK_FIELD(tm, tm_sec);
    CHECK_FIELD(tm, tm_min);
    CHECK_FIELD(tm, tm_hour);
    CHECK_FIELD(tm, tm_mday);
    CHECK_FIELD(tm, tm_mon);
    CHECK_FIELD(tm, tm_year);
    CHECK_FIELD(tm, tm_wday);
    CHECK_FIELD(tm, tm_yday);
    CHECK_FIELD(tm, tm_isdst);
    CHECK_STRUCT(_timeb);
    CHECK_FIELD(_timeb, time);
    CHECK_FIELD(_timeb, millitm);
    CHECK_FIELD(_timeb, timezone);
    CHECK_FIELD(_timeb, dstflag);
    CHECK_STRUCT(_iobuf);
    CHECK_FIELD(_iobuf, _ptr);
    CHECK_FIELD(_iobuf, _cnt);
    CHECK_FIELD(_iobuf, _base);
    CHECK_FIELD(_iobuf, _flag);
    CHECK_FIELD(_iobuf, _file);
    CHECK_FIELD(_iobuf, _charbuf);
    CHECK_FIELD(_iobuf, _bufsiz);
    CHECK_FIELD(_iobuf, _tmpfname);
    CHECK_STRUCT(lconv);
    CHECK_FIELD(lconv, decimal_point);
    CHECK_FIELD(lconv, thousands_sep);
    CHECK_FIELD(lconv, grouping);
    CHECK_FIELD(lconv, int_curr_symbol);
    CHECK_FIELD(lconv, currency_symbol);
    CHECK_FIELD(lconv, mon_decimal_point);
    CHECK_FIELD(lconv, mon_thousands_sep);
    CHECK_FIELD(lconv, mon_grouping);
    CHECK_FIELD(lconv, positive_sign);
    CHECK_FIELD(lconv, negative_sign);
    CHECK_FIELD(lconv, int_frac_digits);
    CHECK_FIELD(lconv, frac_digits);
    CHECK_FIELD(lconv, p_cs_precedes);
    CHECK_FIELD(lconv, p_sep_by_space);
    CHECK_FIELD(lconv, n_cs_precedes);
    CHECK_FIELD(lconv, n_sep_by_space);
    CHECK_FIELD(lconv, p_sign_posn);
    CHECK_FIELD(lconv, n_sign_posn);
    CHECK_STRUCT(_exception);
    CHECK_FIELD(_exception, type);
    CHECK_FIELD(_exception, name);
    CHECK_FIELD(_exception, arg1);
    CHECK_FIELD(_exception, arg2);
    CHECK_FIELD(_exception, retval);
    CHECK_STRUCT(_complex);
    CHECK_FIELD(_complex, x);
    CHECK_FIELD(_complex, y);
    CHECK_STRUCT(_div_t);
    CHECK_FIELD(_div_t, quot);
    CHECK_FIELD(_div_t, rem);
    CHECK_STRUCT(_ldiv_t);
    CHECK_FIELD(_ldiv_t, quot);
    CHECK_FIELD(_ldiv_t, rem);
    CHECK_STRUCT(_heapinfo);
    CHECK_FIELD(_heapinfo, _pentry);
    CHECK_FIELD(_heapinfo, _size);
    CHECK_FIELD(_heapinfo, _useflag);
#ifdef __i386__
    CHECK_STRUCT(__JUMP_BUFFER);
    CHECK_FIELD(__JUMP_BUFFER, Ebp);
    CHECK_FIELD(__JUMP_BUFFER, Ebx);
    CHECK_FIELD(__JUMP_BUFFER, Edi);
    CHECK_FIELD(__JUMP_BUFFER, Esi);
    CHECK_FIELD(__JUMP_BUFFER, Esp);
    CHECK_FIELD(__JUMP_BUFFER, Eip);
    CHECK_FIELD(__JUMP_BUFFER, Registration);
    CHECK_FIELD(__JUMP_BUFFER, TryLevel);
    CHECK_FIELD(__JUMP_BUFFER, Cookie);
    CHECK_FIELD(__JUMP_BUFFER, UnwindFunc);
    CHECK_FIELD(__JUMP_BUFFER, UnwindData[6]);
#endif
    CHECK_STRUCT(_diskfree_t);
    CHECK_FIELD(_diskfree_t, total_clusters);
    CHECK_FIELD(_diskfree_t, avail_clusters);
    CHECK_FIELD(_diskfree_t, sectors_per_cluster);
    CHECK_FIELD(_diskfree_t, bytes_per_sector);
    CHECK_STRUCT(_finddata_t);
    CHECK_FIELD(_finddata_t, attrib);
    CHECK_FIELD(_finddata_t, time_create);
    CHECK_FIELD(_finddata_t, time_access);
    CHECK_FIELD(_finddata_t, time_write);
    CHECK_FIELD(_finddata_t, size);
    CHECK_FIELD(_finddata_t, name[260]);
    CHECK_STRUCT(_finddatai64_t);
    CHECK_FIELD(_finddatai64_t, attrib);
    CHECK_FIELD(_finddatai64_t, time_create);
    CHECK_FIELD(_finddatai64_t, time_access);
    CHECK_FIELD(_finddatai64_t, time_write);
    CHECK_FIELD(_finddatai64_t, size);
    CHECK_FIELD(_finddatai64_t, name[260]);
    CHECK_STRUCT(_wfinddata_t);
    CHECK_FIELD(_wfinddata_t, attrib);
    CHECK_FIELD(_wfinddata_t, time_create);
    CHECK_FIELD(_wfinddata_t, time_access);
    CHECK_FIELD(_wfinddata_t, time_write);
    CHECK_FIELD(_wfinddata_t, size);
    CHECK_FIELD(_wfinddata_t, name[260]);
    CHECK_STRUCT(_wfinddatai64_t);
    CHECK_FIELD(_wfinddatai64_t, attrib);
    CHECK_FIELD(_wfinddatai64_t, time_create);
    CHECK_FIELD(_wfinddatai64_t, time_access);
    CHECK_FIELD(_wfinddatai64_t, time_write);
    CHECK_FIELD(_wfinddatai64_t, size);
    CHECK_FIELD(_wfinddatai64_t, name[260]);
    CHECK_STRUCT(_utimbuf);
    CHECK_FIELD(_utimbuf, actime);
    CHECK_FIELD(_utimbuf, modtime);
    CHECK_STRUCT(_stat);
    CHECK_FIELD(_stat, st_dev);
    CHECK_FIELD(_stat, st_ino);
    CHECK_FIELD(_stat, st_mode);
    CHECK_FIELD(_stat, st_nlink);
    CHECK_FIELD(_stat, st_uid);
    CHECK_FIELD(_stat, st_gid);
    CHECK_FIELD(_stat, st_rdev);
    CHECK_FIELD(_stat, st_size);
    CHECK_FIELD(_stat, st_atime);
    CHECK_FIELD(_stat, st_mtime);
    CHECK_FIELD(_stat, st_ctime);
    CHECK_FIELD(_stat, st_dev);
    CHECK_FIELD(_stat, st_ino);
    CHECK_FIELD(_stat, st_mode);
    CHECK_FIELD(_stat, st_nlink);
    CHECK_FIELD(_stat, st_uid);
    CHECK_FIELD(_stat, st_gid);
    CHECK_FIELD(_stat, st_rdev);
    CHECK_FIELD(_stat, st_size);
    CHECK_FIELD(_stat, st_atime);
    CHECK_FIELD(_stat, st_mtime);
    CHECK_FIELD(_stat, st_ctime);
    CHECK_FIELD(_stat, st_dev);
    CHECK_FIELD(_stat, st_ino);
    CHECK_FIELD(_stat, st_mode);
    CHECK_FIELD(_stat, st_nlink);
    CHECK_FIELD(_stat, st_uid);
    CHECK_FIELD(_stat, st_gid);
    CHECK_FIELD(_stat, st_rdev);
    CHECK_FIELD(_stat, st_size);
    CHECK_FIELD(_stat, st_atime);
    CHECK_FIELD(_stat, st_mtime);
    CHECK_FIELD(_stat, st_ctime);
    CHECK_STRUCT(stat);
    CHECK_FIELD(stat, st_dev);
    CHECK_FIELD(stat, st_ino);
    CHECK_FIELD(stat, st_mode);
    CHECK_FIELD(stat, st_nlink);
    CHECK_FIELD(stat, st_uid);
    CHECK_FIELD(stat, st_gid);
    CHECK_FIELD(stat, st_rdev);
    CHECK_FIELD(stat, st_size);
    CHECK_FIELD(stat, st_atime);
    CHECK_FIELD(stat, st_mtime);
    CHECK_FIELD(stat, st_ctime);
    CHECK_FIELD(stat, st_dev);
    CHECK_FIELD(stat, st_ino);
    CHECK_FIELD(stat, st_mode);
    CHECK_FIELD(stat, st_nlink);
    CHECK_FIELD(stat, st_uid);
    CHECK_FIELD(stat, st_gid);
    CHECK_FIELD(stat, st_rdev);
    CHECK_FIELD(stat, st_size);
    CHECK_FIELD(stat, st_atime);
    CHECK_FIELD(stat, st_mtime);
    CHECK_FIELD(stat, st_ctime);
    CHECK_FIELD(stat, st_dev);
    CHECK_FIELD(stat, st_ino);
    CHECK_FIELD(stat, st_mode);
    CHECK_FIELD(stat, st_nlink);
    CHECK_FIELD(stat, st_uid);
    CHECK_FIELD(stat, st_gid);
    CHECK_FIELD(stat, st_rdev);
    CHECK_FIELD(stat, st_size);
    CHECK_FIELD(stat, st_atime);
    CHECK_FIELD(stat, st_mtime);
    CHECK_FIELD(stat, st_ctime);
    CHECK_STRUCT(_stati64);
    CHECK_FIELD(_stati64, st_dev);
    CHECK_FIELD(_stati64, st_ino);
    CHECK_FIELD(_stati64, st_mode);
    CHECK_FIELD(_stati64, st_nlink);
    CHECK_FIELD(_stati64, st_uid);
    CHECK_FIELD(_stati64, st_gid);
    CHECK_FIELD(_stati64, st_rdev);
    CHECK_FIELD(_stati64, st_size);
    CHECK_FIELD(_stati64, st_atime);
    CHECK_FIELD(_stati64, st_mtime);
    CHECK_FIELD(_stati64, st_ctime);
    CHECK_STRUCT(_stat64);
    CHECK_FIELD(_stat64, st_dev);
    CHECK_FIELD(_stat64, st_ino);
    CHECK_FIELD(_stat64, st_mode);
    CHECK_FIELD(_stat64, st_nlink);
    CHECK_FIELD(_stat64, st_uid);
    CHECK_FIELD(_stat64, st_gid);
    CHECK_FIELD(_stat64, st_rdev);
    CHECK_FIELD(_stat64, st_size);
    CHECK_FIELD(_stat64, st_atime);
    CHECK_FIELD(_stat64, st_mtime);
    CHECK_FIELD(_stat64, st_ctime);
}

/************* Checking defines ***************/
static void test_defines(void)
{
    CHECK_DEF(WEOF);
    CHECK_DEF(EOF);
    CHECK_DEF(TMP_MAX);
    CHECK_DEF(RAND_MAX);
    CHECK_DEF(BUFSIZ);
    CHECK_DEF(STDIN_FILENO);
    CHECK_DEF(STDOUT_FILENO);
    CHECK_DEF(STDERR_FILENO);
    CHECK_DEF(_IOFBF);
    CHECK_DEF(_IONBF);
    CHECK_DEF(_IOLBF);
    CHECK_DEF(FILENAME_MAX);
    CHECK_DEF(_P_WAIT);
    CHECK_DEF(_P_NOWAIT);
    CHECK_DEF(_P_OVERLAY);
    CHECK_DEF(_P_NOWAITO);
    CHECK_DEF(_P_DETACH);
    CHECK_DEF(EPERM);
    CHECK_DEF(ENOENT);
    CHECK_DEF(ESRCH);
    CHECK_DEF(EINTR);
    CHECK_DEF(EIO);
    CHECK_DEF(ENXIO);
    CHECK_DEF(E2BIG);
    CHECK_DEF(ENOEXEC);
    CHECK_DEF(EBADF);
    CHECK_DEF(ECHILD);
    CHECK_DEF(EAGAIN);
    CHECK_DEF(ENOMEM);
    CHECK_DEF(EACCES);
    CHECK_DEF(EFAULT);
    CHECK_DEF(EBUSY);
    CHECK_DEF(EEXIST);
    CHECK_DEF(EXDEV);
    CHECK_DEF(ENODEV);
    CHECK_DEF(ENOTDIR);
    CHECK_DEF(EISDIR);
    CHECK_DEF(EINVAL);
    CHECK_DEF(ENFILE);
    CHECK_DEF(EMFILE);
    CHECK_DEF(ENOTTY);
    CHECK_DEF(EFBIG);
    CHECK_DEF(ENOSPC);
    CHECK_DEF(ESPIPE);
    CHECK_DEF(EROFS);
    CHECK_DEF(EMLINK);
    CHECK_DEF(EPIPE);
    CHECK_DEF(EDOM);
    CHECK_DEF(ERANGE);
    CHECK_DEF(EDEADLK);
    CHECK_DEF(EDEADLOCK);
    CHECK_DEF(ENAMETOOLONG);
    CHECK_DEF(ENOLCK);
    CHECK_DEF(ENOSYS);
    CHECK_DEF(ENOTEMPTY);
    CHECK_DEF(EILSEQ);
    CHECK_DEF(LC_ALL);
    CHECK_DEF(LC_COLLATE);
    CHECK_DEF(LC_CTYPE);
    CHECK_DEF(LC_MONETARY);
    CHECK_DEF(LC_NUMERIC);
    CHECK_DEF(LC_TIME);
    CHECK_DEF(LC_MIN);
    CHECK_DEF(LC_MAX);
    CHECK_DEF(CLOCKS_PER_SEC);
    CHECK_DEF(_HEAPEMPTY);
    CHECK_DEF(_HEAPOK);
    CHECK_DEF(_HEAPBADBEGIN);
    CHECK_DEF(_HEAPBADNODE);
    CHECK_DEF(_HEAPEND);
    CHECK_DEF(_HEAPBADPTR);
    CHECK_DEF(_FREEENTRY);
    CHECK_DEF(_USEDENTRY);
    CHECK_DEF(_OUT_TO_DEFAULT);
    CHECK_DEF(_REPORT_ERRMODE);
    CHECK_DEF(_UPPER);
    CHECK_DEF(_LOWER);
    CHECK_DEF(_DIGIT);
    CHECK_DEF(_SPACE);
    CHECK_DEF(_PUNCT);
    CHECK_DEF(_CONTROL);
    CHECK_DEF(_BLANK);
    CHECK_DEF(_HEX);
    CHECK_DEF(_LEADBYTE);
    CHECK_DEF(_ALPHA);
    CHECK_DEF(_IOREAD);
    CHECK_DEF(_IOWRT);
    CHECK_DEF(_IOMYBUF);
    CHECK_DEF(_IOEOF);
    CHECK_DEF(_IOERR);
    CHECK_DEF(_IOSTRG);
    CHECK_DEF(_IORW);
    CHECK_DEF(_S_IEXEC);
    CHECK_DEF(_S_IWRITE);
    CHECK_DEF(_S_IREAD);
    CHECK_DEF(_S_IFIFO);
    CHECK_DEF(_S_IFCHR);
    CHECK_DEF(_S_IFDIR);
    CHECK_DEF(_S_IFREG);
    CHECK_DEF(_S_IFMT);
    CHECK_DEF(_LK_UNLCK);
    CHECK_DEF(_LK_LOCK);
    CHECK_DEF(_LK_NBLCK);
    CHECK_DEF(_LK_RLCK);
    CHECK_DEF(_LK_NBRLCK);
    CHECK_DEF(_O_RDONLY);
    CHECK_DEF(_O_WRONLY);
    CHECK_DEF(_O_RDWR);
    CHECK_DEF(_O_ACCMODE);
    CHECK_DEF(_O_APPEND);
    CHECK_DEF(_O_RANDOM);
    CHECK_DEF(_O_SEQUENTIAL);
    CHECK_DEF(_O_TEMPORARY);
    CHECK_DEF(_O_NOINHERIT);
    CHECK_DEF(_O_CREAT);
    CHECK_DEF(_O_TRUNC);
    CHECK_DEF(_O_EXCL);
    CHECK_DEF(_O_SHORT_LIVED);
    CHECK_DEF(_O_TEXT);
    CHECK_DEF(_O_BINARY);
    CHECK_DEF(_O_RAW);
    CHECK_DEF(_SW_INEXACT);
    CHECK_DEF(_SW_UNDERFLOW);
    CHECK_DEF(_SW_OVERFLOW);
    CHECK_DEF(_SW_ZERODIVIDE);
    CHECK_DEF(_SW_INVALID);
    CHECK_DEF(_SW_UNEMULATED);
    CHECK_DEF(_SW_SQRTNEG);
    CHECK_DEF(_SW_STACKOVERFLOW);
    CHECK_DEF(_SW_STACKUNDERFLOW);
    CHECK_DEF(_SW_DENORMAL);
    CHECK_DEF(_FPCLASS_SNAN);
    CHECK_DEF(_FPCLASS_QNAN);
    CHECK_DEF(_FPCLASS_NINF);
    CHECK_DEF(_FPCLASS_NN);
    CHECK_DEF(_FPCLASS_ND);
    CHECK_DEF(_FPCLASS_NZ);
    CHECK_DEF(_FPCLASS_PZ);
    CHECK_DEF(_FPCLASS_PD);
    CHECK_DEF(_FPCLASS_PN);
    CHECK_DEF(_FPCLASS_PINF);
    CHECK_DEF(SIGINT);
    CHECK_DEF(SIGILL);
    CHECK_DEF(SIGFPE);
    CHECK_DEF(SIGSEGV);
    CHECK_DEF(SIGTERM);
    CHECK_DEF(SIGBREAK);
    CHECK_DEF(SIGABRT);
    CHECK_DEF(NSIG);
#ifdef __i386__
    CHECK_DEF(_EM_INVALID);
    CHECK_DEF(_EM_DENORMAL);
    CHECK_DEF(_EM_ZERODIVIDE);
    CHECK_DEF(_EM_OVERFLOW);
    CHECK_DEF(_EM_UNDERFLOW);
    CHECK_DEF(_EM_INEXACT);
    CHECK_DEF(_IC_AFFINE);
    CHECK_DEF(_IC_PROJECTIVE);
    CHECK_DEF(_RC_CHOP);
    CHECK_DEF(_RC_UP);
    CHECK_DEF(_RC_DOWN);
    CHECK_DEF(_RC_NEAR);
    CHECK_DEF(_PC_24);
    CHECK_DEF(_PC_53);
    CHECK_DEF(_PC_64);
#endif
}

#endif /* __WINE_USE_MSVCRT */

START_TEST(headers)
{
#ifdef __WINE_USE_MSVCRT
    test_types();
    test_structs();
    test_defines();
#endif
}
