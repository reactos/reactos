/* $Id: psapi.h,v 1.1 2002/06/18 22:15:57 hyperion Exp $
*/
/*
 * internal/psapi.h
 *
 * Process Status Helper API
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

#ifndef __INTERNAL_PSAPI_H_INCLUDED__
#define __INTERNAL_PSAPI_H_INCLUDED__

/* INCLUDES */
#include <ddk/ntddk.h>

/* OBJECTS */

/* TYPES */

/* CONSTANTS */

/* PROTOTYPES */
NTSTATUS
STDCALL
PsaEnumerateProcessIds
(
 OUT ULONG * ProcessIds,
 IN ULONG ProcessIdsLength,
 OUT ULONG * ReturnLength OPTIONAL
);

NTSTATUS
STDCALL
PsaEnumerateSystemModules
(
 OUT PVOID * Modules,
 IN ULONG ModulesLength,
 OUT ULONG * ReturnLength OPTIONAL
);

/* MACROS */

#endif /* __INTERNAL_PSAPI_H_INCLUDED__ */

/* EOF */

