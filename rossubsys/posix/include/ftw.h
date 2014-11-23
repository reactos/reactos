/* $Id: ftw.h,v 1.4 2002/10/29 04:45:08 rex Exp $
 */
/*
 * ftw.h
 *
 * file tree traversal. Conforming to the Single UNIX(r) Specification
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
#ifndef __FTW_H_INCLUDED__
#define __FTW_H_INCLUDED__

/* INCLUDES */

/* OBJECTS */

/* TYPES */
struct FTW
{
 int  base;
 int  level;
};

/* CONSTANTS */
/* Values of the third argument to the application-supplied function
   that is passed as the second argument to ftw() and nftw() */
#define FTW_F   (1) /* File. */
#define FTW_D   (2) /* Directory. */
#define FTW_DNR (3) /* Directory without read permission. */
#define FTW_DP  (4) /* Directory with subdirectories visited. */
#define FTW_NS  (5) /* Unknown type, stat() failed. */
#define FTW_SL  (6) /* Symbolic link. */
#define FTW_SLN (7) /* Symbolic link that names a non-existent file. */

/* Values of the fourth argument to nftw() */
#define FTW_PHYS  (0x00000001) /* Physical walk, does not follow symbolic \
                                  links. Otherwise, nftw() will follow \
                                  links but will not walk down any path \
                                  that crosses itself. */
#define FTW_MOUNT (0x00000002) /* The walk will not cross a mount point. */
#define FTW_DEPTH (0x00000004) /* All subdirectories will be visited before \
                                  the directory itself. */
#define FTW_CHDIR (0x00000008) /* The walk will change to each directory \
                                  before reading it. */

/* PROTOTYPES */
int ftw(const char *,
    int (*)(const char *, const struct stat *, int), int);
int nftw(const char *, int (*)
    (const char *, const struct stat *, int, struct FTW*),
    int, int);

/* MACROS */

#endif /* __FTW_H_INCLUDED__ */

/* EOF */

