/* $Id:
 */
/*
 * psx/fdtable.h
 *
 * POSIX subsystem file descriptor table data structure
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
#ifndef __PSX_FDTABLE_H_INCLUDED__
#define __PSX_FDTABLE_H_INCLUDED__

/* INCLUDES */
#include <limits.h>
#include <inttypes.h>
#include <psx/safeobj.h>

/* OBJECTS */

/* TYPES */
typedef struct __tagfildes_t
{
 void  *FileHandle;
 int    OpenFlags;
 int    FdFlags;
 size_t ExtraDataSize;
 void  *ExtraData;
} __fildes_t;

typedef struct __tagfdtable_t
{
 __magic_t   Signature;
 int32_t     LowestUnusedFileNo;
 int32_t     UsedDescriptors;
 int32_t     AllocatedDescriptors;
 uint32_t    DescriptorsBitmap[OPEN_MAX / 32];
 __fildes_t *Descriptors;
} __fdtable_t;

/* CONSTANTS */

/* PROTOTYPES */
int __fdtable_init(__fdtable_t *);
int __fdtable_free(__fdtable_t *);

int         __fdtable_entry_isavail(__fdtable_t *, int);
int         __fdtable_entry_nextavail(__fdtable_t *, int);
int         __fdtable_entry_add(__fdtable_t *, int, __fildes_t *, __fildes_t **);
int         __fdtable_entry_remove(__fdtable_t *, int);
__fildes_t *__fdtable_entry_get(__fdtable_t *, int);

/* MACROS */
#define __FDTABLE_MAGIC MAGIC('F', 'D', 'T', 'B')

#endif /* __PSX_FDTABLE_H_INCLUDED__ */

/* EOF */

