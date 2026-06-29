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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_NDR_MISC_H
#define __WINE_NDR_MISC_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "rpc.h"
#include "rpcndr.h"

struct IPSFactoryBuffer;

PFORMAT_STRING ComputeConformanceOrVariance(
    MIDL_STUB_MESSAGE *pStubMsg, unsigned char *pMemory,
    PFORMAT_STRING pFormat, ULONG_PTR def, ULONG_PTR *pCount);

static inline PFORMAT_STRING ComputeConformance(PMIDL_STUB_MESSAGE pStubMsg, unsigned char *pMemory, PFORMAT_STRING pFormat, ULONG def)
{
    return ComputeConformanceOrVariance(pStubMsg, pMemory, pFormat, def, &pStubMsg->MaxCount);
}

static inline PFORMAT_STRING ComputeVariance(PMIDL_STUB_MESSAGE pStubMsg, unsigned char *pMemory, PFORMAT_STRING pFormat, ULONG def)
{
    PFORMAT_STRING ret;
    ULONG_PTR ActualCount = pStubMsg->ActualCount;

    pStubMsg->Offset = 0;
    ret = ComputeConformanceOrVariance(pStubMsg, pMemory, pFormat, def, &ActualCount);
    pStubMsg->ActualCount = (ULONG)ActualCount;
    return ret;
}

typedef unsigned char* (WINAPI *NDR_MARSHALL)  (PMIDL_STUB_MESSAGE, unsigned char*, PFORMAT_STRING);
typedef unsigned char* (WINAPI *NDR_UNMARSHALL)(PMIDL_STUB_MESSAGE, unsigned char**,PFORMAT_STRING, unsigned char);
typedef void           (WINAPI *NDR_BUFFERSIZE)(PMIDL_STUB_MESSAGE, unsigned char*, PFORMAT_STRING);
typedef ULONG          (WINAPI *NDR_MEMORYSIZE)(PMIDL_STUB_MESSAGE,                 PFORMAT_STRING);
typedef void           (WINAPI *NDR_FREE)      (PMIDL_STUB_MESSAGE, unsigned char*, PFORMAT_STRING);

extern const NDR_MARSHALL   NdrMarshaller[];
extern const NDR_UNMARSHALL NdrUnmarshaller[];
extern const NDR_BUFFERSIZE NdrBufferSizer[];
extern const NDR_MEMORYSIZE NdrMemorySizer[];
extern const NDR_FREE       NdrFreer[];

ULONG ComplexStructSize(PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat);

#endif  /* __WINE_NDR_MISC_H */
