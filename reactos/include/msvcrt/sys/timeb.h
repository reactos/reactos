/*
 * timeb.h
 *
 * Support for the UNIX System V ftime system call.
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
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.3 $
 * $Author: chorns $
 * $Date: 2002/09/08 11:16:44 $
 *
 */

#ifndef	__STRICT_ANSI__

#ifndef	_TIMEB_H_
#define	_TIMEB_H_

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * TODO: Structure not tested.
 */
struct timeb
{
	long	time;
	short	millitm;
	short	_timezone;
	short	dstflag;
};

/* TODO: Not tested. */
void	_ftime (struct timeb* timebBuffer);

#ifndef	_NO_OLDNAMES
void	ftime (struct timeb* timebBuffer);
#endif	/* Not _NO_OLDNAMES */

#ifdef	__cplusplus
}
#endif

#endif	/* Not _TIMEB_H_ */

#endif	/* Not __STRICT_ANSI__ */
