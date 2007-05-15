/*
 * NDR definitions
 *
 * Copyright 2001 Ove Kåven, TransGaming Technologies
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_NDR_MISC_H
#define __WINE_NDR_MISC_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "rpc.h"
#include "rpcndr.h"

struct IPSFactoryBuffer;

#define ComputeConformance(pStubMsg, pMemory, pFormat, def) ComputeConformanceOrVariance(pStubMsg, pMemory, pFormat, def, &pStubMsg->MaxCount)
#define ComputeVariance(pStubMsg, pMemory, pFormat, def) ComputeConformanceOrVariance(pStubMsg, pMemory, pFormat, def, &pStubMsg->ActualCount)
PFORMAT_STRING ComputeConformanceOrVariance(
    MIDL_STUB_MESSAGE *pStubMsg, unsigned char *pMemory,
    PFORMAT_STRING pFormat, ULONG_PTR def, ULONG *pCount);

typedef unsigned char* (WINAPI *NDR_MARSHALL)  (PMIDL_STUB_MESSAGE, unsigned char*, PFORMAT_STRING);
typedef unsigned char* (WINAPI *NDR_UNMARSHALL)(PMIDL_STUB_MESSAGE, unsigned char**,PFORMAT_STRING, unsigned char);
typedef void           (WINAPI *NDR_BUFFERSIZE)(PMIDL_STUB_MESSAGE, unsigned char*, PFORMAT_STRING);
typedef unsigned long  (WINAPI *NDR_MEMORYSIZE)(PMIDL_STUB_MESSAGE,                 PFORMAT_STRING);
typedef void           (WINAPI *NDR_FREE)      (PMIDL_STUB_MESSAGE, unsigned char*, PFORMAT_STRING);

extern NDR_MARSHALL   NdrMarshaller[];
extern NDR_UNMARSHALL NdrUnmarshaller[];
extern NDR_BUFFERSIZE NdrBufferSizer[];
extern NDR_MEMORYSIZE NdrMemorySizer[];
extern NDR_FREE       NdrFreer[];

#endif  /* __WINE_NDR_MISC_H */
