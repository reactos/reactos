/*
 * types.h
 *
 * The definition of constants, data types and global variables.
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
 *  WITHOUT ANY WARRANTY. ALL WARRENTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warrenties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.2 $
 * $Author: ekohl $
 * $Date: 2001/07/03 22:16:07 $
 *
 */

#ifndef	_TYPES_H_
#define	_TYPES_H_

#ifdef	__GNUC__
#undef	__int64
#define	__int64	long long
#endif

#ifndef	_TIME_T_
#define	_TIME_T_
typedef	long	time_t;
#endif


#ifndef	__STRICT_ANSI__

#ifndef	_OFF_T_DEFINED
typedef long _off_t;

#ifndef	_NO_OLDNAMES
#define	off_t	_off_t
#endif

#define	_OFF_T_DEFINED

#endif	/* Not _OFF_T_DEFINED */


#ifndef _DEV_T_DEFINED
typedef short _dev_t;

#ifndef	_NO_OLDNAMES
#define	dev_t	_dev_t
#endif

#define	_DEV_T_DEFINED

#endif	/* Not _DEV_T_DEFINED */


#ifndef _INO_T_DEFINED
typedef short _ino_t;

#ifndef	_NO_OLDNAMES
#define	ino_t	_ino_t
#endif

#define	_INO_T_DEFINED

#endif	/* Not _INO_T_DEFINED */


#endif	/* Not __STRICT_ANSI__ */


#endif	/* Not _TYPES_H_ */
