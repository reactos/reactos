/*
 * locking.h
 *
 * Constants for the mode parameter of the locking function.
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
 * $Date: 2002/11/24 18:06:01 $
 *
 */

#ifndef __STRICT_ANSI__

#ifndef _LOCKING_H_
#define _LOCKING_H_


#define _LK_UNLCK   0   /* Unlock */
#define _LK_LOCK    1   /* Lock */
#define _LK_NBLCK   2   /* Non-blocking lock */
#define _LK_RLCK    3   /* Lock for read only */
#define _LK_NBRLCK  4   /* Non-blocking lock for read only */

#ifndef NO_OLDNAMES
#define LK_UNLCK    _LK_UNLCK
#define LK_LOCK     _LK_LOCK
#define LK_NBLCK    _LK_NBLCK
#define LK_RLCK     _LK_RLCK
#define LK_NBRLCK   _LK_NBRLCK
#endif  /* Not NO_OLDNAMES */

#endif  /* Not _LOCKING_H_ */

#endif  /* Not __STRICT_ANSI__ */
