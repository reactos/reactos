/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_TYPES
#define _INC_TYPES

#ifndef _WIN32
#error Only Win32 target is supported!
#endif

#include <crtdefs.h>

#ifndef _INO_T_DEFINED
#define _INO_T_DEFINED
typedef unsigned short _ino_t;
#ifndef	NO_OLDNAMES
typedef unsigned short ino_t;
#endif
#endif

#ifndef _DEV_T_DEFINED
#define _DEV_T_DEFINED
typedef unsigned int _dev_t;
#ifndef	NO_OLDNAMES
typedef unsigned int dev_t;
#endif
#endif

#ifndef _OFF_T_DEFINED
#define _OFF_T_DEFINED
  typedef long _off_t;
# ifndef NO_OLDNAMES
  typedef long off_t;
# endif
#endif

#ifdef _WINE
#ifndef _PID_T_
#define	_PID_T_
#ifndef _WIN64
typedef int	_pid_t;
#else
typedef __int64	_pid_t;
#endif
#ifndef	NO_OLDNAMES
#ifndef __pid_t_defined
#define __pid_t_defined
typedef _pid_t	pid_t;
#endif /* __pid_t_defined */
#endif
#endif	/* Not _PID_T_ */
#endif

#endif /* !_INC_TYPES */
