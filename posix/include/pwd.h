/*
 * pwd.h
 *
 * password structure. Based on the Single UNIX(r) Specification, Version 2
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
#ifndef __PWD_H_INCLUDED__
#define __PWD_H_INCLUDED__

/* INCLUDES */

/* OBJECTS */

/* TYPES */
struct passwd
{
 char  *pw_name;  /* user's login name */
 uid_t pw_uid;   /* numerical user ID */
 gid_t pw_gid;   /* numerical group ID */
 char  *pw_dir;   /* initial working directory */
 char  *pw_shell; /* program to use as shell */
};

/* CONSTANTS */

/* PROTOTYPES */
struct passwd *getpwnam(const char *);
struct passwd *getpwuid(uid_t);
int            getpwnam_r(const char *, struct passwd *, char *,
                   size_t, struct passwd **);
int            getpwuid_r(uid_t, struct passwd *, char *,
                   size_t, struct passwd **);
void           endpwent(void);
struct passwd *getpwent(void);
void           setpwent(void);

/* MACROS */

#endif /* __PWD_H_INCLUDED__ */

/* EOF */

