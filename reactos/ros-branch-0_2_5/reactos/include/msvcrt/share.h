/*
 * share.h
 *
 * Constants for file sharing functions.
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
 * $Date: 2002/11/24 18:06:00 $
 *
 */

#ifndef _SHARE_H_
#define _SHARE_H_


#define SH_COMPAT   0x0000
#define SH_DENYRW   0x0010
#define SH_DENYWR   0x0020
#define SH_DENYRD   0x0030
#define SH_DENYNO   0x0040

#define _SH_COMPAT  SH_COMPAT
#define _SH_DENYRW  SH_DENYRW
#define _SH_DENYWR  SH_DENYWR
#define _SH_DENYRD  SH_DENYRD
#define _SH_DENYNO  SH_DENYNO


#endif  /* Not _SHARE_H_ */
