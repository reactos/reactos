/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#ifndef __DJ_sys_djtypes_h_
#define __DJ_sys_djtypes_h_

#define __DJ_clock_t	typedef int clock_t;
#define __DJ_gid_t	typedef int gid_t;
#define __DJ_off_t	typedef int off_t;
#define __DJ_pid_t	typedef int pid_t;
#define __DJ_size_t	typedef long unsigned int size_t;
#define __DJ_ssize_t	typedef int ssize_t;
#define __DJ_time_t	typedef unsigned int time_t;
#define __DJ_uid_t	typedef int uid_t;
#define __DJ_va_list	typedef void *va_list;

#if defined(__cplusplus) && ( (__GNUC_MINOR__ >= 8 && __GNUC__ == 2 ) || __GNUC__ >= 3 )
/* wchar_t is now a keyword in C++ */
#define __DJ_wchar_t
#else
/* but remains a typedef in C */
#define __DJ_wchar_t    typedef int wchar_t;
#endif

#define __DJ_wint_t     typedef int wint_t;

#endif
