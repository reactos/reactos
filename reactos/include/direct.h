/*
 * direct.h
 *
 * Functions for manipulating paths and directories (included from dir.h)
 * plus functions for setting the current drive.
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
 * $Author: ariadne $
 * $Date: 1999/02/25 22:51:47 $
 *
 */

#ifndef __STRICT_ANSI__

#ifndef	_DIRECT_H_
#define	_DIRECT_H_

#include <dir.h>

#ifdef	__cplusplus
extern "C" {
#endif

struct _diskfree_t {
  unsigned long total_clusters;
  unsigned long avail_clusters;
  unsigned long sectors_per_cluster;
  unsigned long bytes_per_sector;
};
#define diskfree_t _diskfree_t

/*
 * You really shouldn't be using these. Use the Win32 API functions instead.
 * However, it does make it easier to port older code.
 */


int	_chdrive (int nDrive);
char*	_getdcwd (int nDrive, char* caBuffer, int nBufLen);
unsigned int _getdiskfree(unsigned int _drive, struct _diskfree_t *_diskspace);

#ifdef	__cplusplus
}
#endif

#endif	/* Not _DIRECT_H_ */

#endif	/* Not __STRICT_ANSI__ */
