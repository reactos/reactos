/* $Id: dirent.h,v 1.4 2002/10/29 04:45:13 rex Exp $
 */
/*
 * psx/dirent.h
 *
 * internal dirent.h
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
#ifndef __PSX_DIRENT_H_INCLUDED__
#define __PSX_DIRENT_H_INCLUDED__

/* INCLUDES */
#include <ddk/ntddk.h>
#include <dirent.h>
#include <psx/safeobj.h>

/* OBJECTS */

/* TYPES */
struct __internal_DIR
{
 __magic_t                  signature;      /* signature to verify object's validity across calls */
 union __any_dirent{
  struct dirent   de_ansi;
  struct _Wdirent de_unicode;
 }                          ent;            /* storage for return buffer of readdir() */
 int                        fildes;         /* file descriptor of the directory */
 FILE_DIRECTORY_INFORMATION info;           /* directory entry information */
 WCHAR                      name[MAX_PATH]; /* filename buffer */
};

/* CONSTANTS */
#define __IDIR_MAGIC MAGIC('I', 'D', 'I', 'R')

/* PROTOTYPES */

/* MACROS */

#endif /* __PSX_DIRENT_H_INCLUDED__ */

/* EOF */

