/* $Id: cpio.h,v 1.2 2002/02/20 09:17:54 hyperion Exp $
 */
/*
 * cpio.h
 *
 * cpio archive values. Conforming to the Single UNIX(r) Specification
 * Version 2, System Interface & Headers Issue 5
 *
 * This file is part of the ReactOS Operating System.
 *
 * Contributors:
 *  Created by KJK::Hyperion <noog@libero.it>
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
 */
#ifndef __CPIO_H_INCLUDED__
#define __CPIO_H_INCLUDED__

/* INCLUDES */

/* TYPES */

/* CONSTANTS */
#define C_IRUSR  (0000400) /* read by owner */
#define C_IWUSR  (0000200) /* write by owner */
#define C_IXUSR  (0000100) /* execute by owner */
#define C_IRGRP  (0000040) /* read by group */
#define C_IWGRP  (0000020) /* write by group */
#define C_IXGRP  (0000010) /* execute by group */
#define C_IROTH  (0000004) /* read by others */
#define C_IWOTH  (0000002) /* write by others */
#define C_IXOTH  (0000001) /* execute by others */
#define C_ISUID  (0004000) /* set user ID */
#define C_ISGID  (0002000) /* set group ID */
#define C_ISVTX  (0001000) /* on directories, restricted deletion flag */
#define C_ISDIR  (0040000) /* directory */
#define C_ISFIFO (0010000) /* FIFO */
#define C_ISREG  (0100000) /* regular file */
#define C_ISBLK  (0060000) /* block special */
#define C_ISCHR  (0020000) /* character special */
#define C_ISCTG  (0110000) /* reserved */
#define C_ISLNK  (0120000) /* symbolic link */
#define C_ISSOCK (0140000) /* socket */

#define MAGIC    "070707"

/* PROTOTYPES */

/* MACROS */

#endif /* __CPIO_H_INCLUDED__ */

/* EOF */

