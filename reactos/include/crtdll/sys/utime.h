/*
 * utime.h
 *
 * Support for the utime function.
 *
 * This file is part of the Mingw32 package.
 *
 * Contributors:
 *  Created by Colin Peters <colin@bird.fu.is.saga-u.ac.jp>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAIMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.4 $
 * $Author: robd $
 * $Date: 2002/11/24 18:09:57 $
 *
 */

#ifndef	__STRICT_ANSI__

#ifndef	_UTIME_H_
#define	_UTIME_H_

#define __need_wchar_t
#define __need_size_t
#include <crtdll/stddef.h>
#include <crtdll/sys/types.h>


/*
 * Structure used by _utime function.
 */
struct _utimbuf
{
	time_t	actime;		/* Access time */
	time_t	modtime;	/* Modification time */
};


#ifndef	_NO_OLDNAMES

/* NOTE: Must be the same as _utimbuf above. */
struct utimbuf
{
	time_t	actime;
	time_t	modtime;
};
#endif	/* Not _NO_OLDNAMES */

#ifdef	__cplusplus
extern "C" {
#endif

int	_utime (const char* szFileName, struct _utimbuf* pTimes);
int _futime (int nHandle, struct _utimbuf *pTimes);

/* The wide character version, only available for MSVCRT versions of the
 * C runtime library. */
int     _wutime (const wchar_t *szFileName, struct _utimbuf *times);


#ifndef	_NO_OLDNAMES
int	utime (const char* szFileName, struct utimbuf* pTimes);
#endif	/* Not _NO_OLDNAMES */


#ifdef	__cplusplus
}
#endif

#endif	/* Not _UTIME_H_ */
#endif	/* Not __STRICT_ANSI__ */
