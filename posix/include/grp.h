/* $Id: grp.h,v 1.2 2002/02/20 09:17:54 hyperion Exp $
 */
/*
 * grp.h
 *
 * group structure. Conforming to the Single UNIX(r) Specification
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
#ifndef __GRP_H_INCLUDED__
#define __GRP_H_INCLUDED__

/* INCLUDES */

/* OBJECTS */

/* TYPES */
struct group
{
 char   *gr_name; /* the name of the group */
 gid_t   gr_gid;  /* numerical group ID */
 char  **gr_mem;  /* pointer to a null-terminated array of character
                     pointers to member names */
};

/* CONSTANTS */

/* PROTOTYPES */
struct group  *getgrgid(gid_t);
struct group  *getgrnam(const char *);
int            getgrgid_r(gid_t, struct group *, char *,
                   size_t, struct group **);
int            getgrnam_r(const char *, struct group *, char *,
                   size_t , struct group **);
struct group  *getgrent(void);
void           endgrent(void);
void           setgrent(void);

/* MACROS */

#endif /* __GRP_H_INCLUDED__ */

/* EOF */

