/* $Id: spawn.h,v 1.3 2002/10/29 04:45:15 rex Exp $
 */
/*
 * psx/spawn.h
 *
 * spawn POSIX+ processes
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

#ifndef __PSX_SPAWN_H_INCLUDED__
#define __PSX_SPAWN_H_INCLUDED__

/* INCLUDES */
#include <ddk/ntddk.h>
#include <psx/pdata.h>

/* OBJECTS */

/* TYPES */

/* CONSTANTS */

/* PROTOTYPES */
NTSTATUS STDCALL __PdxSpawnPosixProcess
(
 OUT PHANDLE ProcessHandle,
 OUT PHANDLE ThreadHandle,
 IN POBJECT_ATTRIBUTES FileObjectAttributes,
 IN POBJECT_ATTRIBUTES ProcessObjectAttributes,
 IN HANDLE InheritFromProcessHandle,
 IN __PPDX_PDATA ProcessData
);

/* MACROS */

#endif /* __PSX_SPAWN_H_INCLUDED__ */

/* EOF */

