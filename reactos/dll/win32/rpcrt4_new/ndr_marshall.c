/*
 * NDR data marshalling
 *
 * Copyright 2002 Greg Turner
 * Copyright 2003-2006 CodeWeavers
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
 *
 * TODO:
 *  - Non-conformant strings
 *  - String structs
 *  - Encapsulated unions
 *  - Byte count pointers
 *  - transmit_as/represent as
 *  - Multi-dimensional arrays
 *  - Conversion functions (NdrConvert)
 *  - Checks for integer addition overflow
 *  - Checks for out-of-memory conditions
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"

#include "ndr_misc.h"
#include "rpcndr.h"

#include "wine/unicode.h"
#include "wine/rpcfc.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#if defined(__i386__)
# define LITTLE_ENDIAN_UINT32_WRITE(pchar, uint32) \
    (*((UINT32 *)(pchar)) = (uint32))

# define LITTLE_ENDIAN_UINT32_READ(pchar) \
    (*((UINT32 *)(pchar)))
#else
  /* these would work for i386 too, but less efficient */
# define LITTLE_ENDIAN_UINT32_WRITE(pchar, uint32) \
    (*(pchar)     = LOBYTE(LOWORD(uint32)), \
     *((pchar)+1) = HIBYTE(LOWORD(uint32)), \
     *((pchar)+2) = LOBYTE(HIWORD(uint32)), \
     *((pchar)+3) = HIBYTE(HIWORD(uint32)), \
     (uint32)) /* allow as r-value */

# define LITTLE_ENDIAN_UINT32_READ(pchar) \
    (MAKELONG( \
      MAKEWORD(*(pchar), *((pchar)+1)), \
      MAKEWORD(*((pchar)+2), *((pchar)+3))))
#endif

#define BIG_ENDIAN_UINT32_WRITE(pchar, uint32) \
  (*((pchar)+3) = LOBYTE(LOWORD(uint32)), \
   *((pchar)+2) = HIBYTE(LOWORD(uint32)), \
   *((pchar)+1) = LOBYTE(HIWORD(uint32)), \
   *(pchar)     = HIBYTE(HIWORD(uint32)), \
   (uint32)) /* allow as r-value */

#define BIG_ENDIAN_UINT32_READ(pchar) \
  (MAKELONG( \
    MAKEWORD(*((pchar)+3), *((pchar)+2)), \
    MAKEWORD(*((pchar)+1), *(pchar))))

#ifdef NDR_LOCAL_IS_BIG_ENDIAN
# define NDR_LOCAL_UINT32_WRITE(pchar, uint32) \
    BIG_ENDIAN_UINT32_WRITE(pchar, uint32)
# define NDR_LOCAL_UINT32_READ(pchar) \
    BIG_ENDIAN_UINT32_READ(pchar)
#else
# define NDR_LOCAL_UINT32_WRITE(pchar, uint32) \
    LITTLE_ENDIAN_UINT32_WRITE(pchar, uint32)
# define NDR_LOCAL_UINT32_READ(pchar) \
    LITTLE_ENDIAN_UINT32_READ(pchar)
#endif

/* _Align must be the desired alignment,
 * e.g. ALIGN_LENGTH(len, 4) to align on a dword boundary. */
#define ALIGNED_LENGTH(_Len, _Align) (((_Len)+(_Align)-1)&~((_Align)-1))
#define ALIGNED_POINTER(_Ptr, _Align) ((LPVOID)ALIGNED_LENGTH((ULONG_PTR)(_Ptr), _Align))
#define ALIGN_LENGTH(_Len, _Align) _Len = ALIGNED_LENGTH(_Len, _Align)
#define ALIGN_POINTER(_Ptr, _Align) _Ptr = ALIGNED_POINTER(_Ptr, _Align)

#define STD_OVERFLOW_CHECK(_Msg) do { \
    TRACE("buffer=%d/%d\n", _Msg->Buffer - (unsigned char *)_Msg->RpcMsg->Buffer, _Msg->BufferLength); \
    if (_Msg->Buffer > (unsigned char *)_Msg->RpcMsg->Buffer + _Msg->BufferLength) \
        ERR("buffer overflow %d bytes\n", _Msg->Buffer - ((unsigned char *)_Msg->RpcMsg->Buffer + _Msg->BufferLength)); \
  } while (0)

#define NDR_TABLE_SIZE 128
#define NDR_TABLE_MASK 127

static unsigned char *WINAPI NdrBaseTypeMarshall(PMIDL_STUB_MESSAGE, unsigned char *, PFORMAT_STRING);
static unsigned char *WINAPI NdrBaseTypeUnmarshall(PMIDL_STUB_MESSAGE, unsigned char **, PFORMAT_STRING, unsigned char);
static void WINAPI NdrBaseTypeBufferSize(PMIDL_STUB_MESSAGE, unsigned char *, PFORMAT_STRING);
static void WINAPI NdrBaseTypeFree(PMIDL_STUB_MESSAGE, unsigned char *, PFORMAT_STRING);
static ULONG WINAPI NdrBaseTypeMemorySize(PMIDL_STUB_MESSAGE, PFORMAT_STRING);

static unsigned char *WINAPI NdrContextHandleMarshall(PMIDL_STUB_MESSAGE, unsigned char *, PFORMAT_STRING);
static void WINAPI NdrContextHandleBufferSize(PMIDL_STUB_MESSAGE, unsigned char *, PFORMAT_STRING);
static unsigned char *WINAPI NdrContextHandleUnmarshall(PMIDL_STUB_MESSAGE, unsigned char **, PFORMAT_STRING, unsigned char);

const NDR_MARSHALL NdrMarshaller[NDR_TABLE_SIZE] = {
  0,
  NdrBaseTypeMarshall, NdrBaseTypeMarshall, NdrBaseTypeMarshall,
  NdrBaseTypeMarshall, NdrBaseTypeMarshall, NdrBaseTypeMarshall, NdrBaseTypeMarshall,
  NdrBaseTypeMarshall, NdrBaseTypeMarshall, NdrBaseTypeMarshall, NdrBaseTypeMarshall,
  NdrBaseTypeMarshall, NdrBaseTypeMarshall, NdrBaseTypeMarshall, NdrBaseTypeMarshall,
  /* 0x10 */
  NdrBaseTypeMarshall,
  /* 0x11 */
  NdrPointerMarshall, NdrPointerMarshall,
  NdrPointerMarshall, NdrPointerMarshall,
  /* 0x15 */
  NdrSimpleStructMarshall, NdrSimpleStructMarshall,
  NdrConformantStructMarshall, NdrConformantStructMarshall,
  NdrConformantVaryingStructMarshall,
  NdrComplexStructMarshall,
  /* 0x1b */
  NdrConformantArrayMarshall, 
  NdrConformantVaryingArrayMarshall,
  NdrFixedArrayMarshall, NdrFixedArrayMarshall,
  NdrVaryingArrayMarshall, NdrVaryingArrayMarshall,
  NdrComplexArrayMarshall,
  /* 0x22 */
  NdrConformantStringMarshall, 0, 0,
  NdrConformantStringMarshall,
  NdrNonConformantStringMarshall, 0, 0, 0,
  /* 0x2a */
  NdrEncapsulatedUnionMarshall,
  NdrNonEncapsulatedUnionMarshall,
  NdrByteCountPointerMarshall,
  NdrXmitOrRepAsMarshall, NdrXmitOrRepAsMarshall,
  /* 0x2f */
  NdrInterfacePointerMarshall,
  /* 0x30 */
  NdrContextHandleMarshall,
  /* 0xb1 */
  0, 0, 0,
  NdrUserMarshalMarshall,
  0, 0,
  /* 0xb7 */
  NdrRangeMarshall
};
const NDR_UNMARSHALL NdrUnmarshaller[NDR_TABLE_SIZE] = {
  0,
  NdrBaseTypeUnmarshall, NdrBaseTypeUnmarshall, NdrBaseTypeUnmarshall,
  NdrBaseTypeUnmarshall, NdrBaseTypeUnmarshall, NdrBaseTypeUnmarshall, NdrBaseTypeUnmarshall,
  NdrBaseTypeUnmarshall, NdrBaseTypeUnmarshall, NdrBaseTypeUnmarshall, NdrBaseTypeUnmarshall,
  NdrBaseTypeUnmarshall, NdrBaseTypeUnmarshall, NdrBaseTypeUnmarshall, NdrBaseTypeUnmarshall,
  /* 0x10 */
  NdrBaseTypeUnmarshall,
  /* 0x11 */
  NdrPointerUnmarshall, NdrPointerUnmarshall,
  NdrPointerUnmarshall, NdrPointerUnmarshall,
  /* 0x15 */
  NdrSimpleStructUnmarshall, NdrSimpleStructUnmarshall,
  NdrConformantStructUnmarshall, NdrConformantStructUnmarshall,
  NdrConformantVaryingStructUnmarshall,
  NdrComplexStructUnmarshall,
  /* 0x1b */
  NdrConformantArrayUnmarshall, 
  NdrConformantVaryingArrayUnmarshall,
  NdrFixedArrayUnmarshall, NdrFixedArrayUnmarshall,
  NdrVaryingArrayUnmarshall, NdrVaryingArrayUnmarshall,
  NdrComplexArrayUnmarshall,
  /* 0x22 */
  NdrConformantStringUnmarshall, 0, 0,
  NdrConformantStringUnmarshall,
  NdrNonConformantStringUnmarshall, 0, 0, 0,
  /* 0x2a */
  NdrEncapsulatedUnionUnmarshall,
  NdrNonEncapsulatedUnionUnmarshall,
  NdrByteCountPointerUnmarshall,
  NdrXmitOrRepAsUnmarshall, NdrXmitOrRepAsUnmarshall,
  /* 0x2f */
  NdrInterfacePointerUnmarshall,
  /* 0x30 */
  NdrContextHandleUnmarshall,
  /* 0xb1 */
  0, 0, 0,
  NdrUserMarshalUnmarshall,
  0, 0,
  /* 0xb7 */
  NdrRangeUnmarshall
};
const NDR_BUFFERSIZE NdrBufferSizer[NDR_TABLE_SIZE] = {
  0,
  NdrBaseTypeBufferSize, NdrBaseTypeBufferSize, NdrBaseTypeBufferSize,
  NdrBaseTypeBufferSize, NdrBaseTypeBufferSize, NdrBaseTypeBufferSize, NdrBaseTypeBufferSize,
  NdrBaseTypeBufferSize, NdrBaseTypeBufferSize, NdrBaseTypeBufferSize, NdrBaseTypeBufferSize,
  NdrBaseTypeBufferSize, NdrBaseTypeBufferSize, NdrBaseTypeBufferSize, NdrBaseTypeBufferSize,
  /* 0x10 */
  NdrBaseTypeBufferSize,
  /* 0x11 */
  NdrPointerBufferSize, NdrPointerBufferSize,
  NdrPointerBufferSize, NdrPointerBufferSize,
  /* 0x15 */
  NdrSimpleStructBufferSize, NdrSimpleStructBufferSize,
  NdrConformantStructBufferSize, NdrConformantStructBufferSize,
  NdrConformantVaryingStructBufferSize,
  NdrComplexStructBufferSize,
  /* 0x1b */
  NdrConformantArrayBufferSize, 
  NdrConformantVaryingArrayBufferSize,
  NdrFixedArrayBufferSize, NdrFixedArrayBufferSize,
  NdrVaryingArrayBufferSize, NdrVaryingArrayBufferSize,
  NdrComplexArrayBufferSize,
  /* 0x22 */
  NdrConformantStringBufferSize, 0, 0,
  NdrConformantStringBufferSize,
  NdrNonConformantStringBufferSize, 0, 0, 0,
  /* 0x2a */
  NdrEncapsulatedUnionBufferSize,
  NdrNonEncapsulatedUnionBufferSize,
  NdrByteCountPointerBufferSize,
  NdrXmitOrRepAsBufferSize, NdrXmitOrRepAsBufferSize,
  /* 0x2f */
  NdrInterfacePointerBufferSize,
  /* 0x30 */
  NdrContextHandleBufferSize,
  /* 0xb1 */
  0, 0, 0,
  NdrUserMarshalBufferSize,
  0, 0,
  /* 0xb7 */
  NdrRangeBufferSize
};
const NDR_MEMORYSIZE NdrMemorySizer[NDR_TABLE_SIZE] = {
  0,
  NdrBaseTypeMemorySize, NdrBaseTypeMemorySize, NdrBaseTypeMemorySize,
  NdrBaseTypeMemorySize, NdrBaseTypeMemorySize, NdrBaseTypeMemorySize, NdrBaseTypeMemorySize,
  NdrBaseTypeMemorySize, NdrBaseTypeMemorySize, NdrBaseTypeMemorySize, NdrBaseTypeMemorySize,
  NdrBaseTypeMemorySize, NdrBaseTypeMemorySize, NdrBaseTypeMemorySize, NdrBaseTypeMemorySize,
  /* 0x10 */
  NdrBaseTypeMemorySize,
  /* 0x11 */
  NdrPointerMemorySize, NdrPointerMemorySize,
  NdrPointerMemorySize, NdrPointerMemorySize,
  /* 0x15 */
  NdrSimpleStructMemorySize, NdrSimpleStructMemorySize,
  NdrConformantStructMemorySize, NdrConformantStructMemorySize,
  NdrConformantVaryingStructMemorySize,
  NdrComplexStructMemorySize,
  /* 0x1b */
  NdrConformantArrayMemorySize,
  NdrConformantVaryingArrayMemorySize,
  NdrFixedArrayMemorySize, NdrFixedArrayMemorySize,
  NdrVaryingArrayMemorySize, NdrVaryingArrayMemorySize,
  NdrComplexArrayMemorySize,
  /* 0x22 */
  NdrConformantStringMemorySize, 0, 0,
  NdrConformantStringMemorySize,
  NdrNonConformantStringMemorySize, 0, 0, 0,
  /* 0x2a */
  NdrEncapsulatedUnionMemorySize,
  NdrNonEncapsulatedUnionMemorySize,
  NdrByteCountPointerMemorySize,
  NdrXmitOrRepAsMemorySize, NdrXmitOrRepAsMemorySize,
  /* 0x2f */
  NdrInterfacePointerMemorySize,
  /* 0x30 */
  0,
  /* 0xb1 */
  0, 0, 0,
  NdrUserMarshalMemorySize,
  0, 0,
  /* 0xb7 */
  NdrRangeMemorySize
};
const NDR_FREE NdrFreer[NDR_TABLE_SIZE] = {
  0,
  NdrBaseTypeFree, NdrBaseTypeFree, NdrBaseTypeFree,
  NdrBaseTypeFree, NdrBaseTypeFree, NdrBaseTypeFree, NdrBaseTypeFree,
  NdrBaseTypeFree, NdrBaseTypeFree, NdrBaseTypeFree, NdrBaseTypeFree,
  NdrBaseTypeFree, NdrBaseTypeFree, NdrBaseTypeFree, NdrBaseTypeFree,
  /* 0x10 */
  NdrBaseTypeFree,
  /* 0x11 */
  NdrPointerFree, NdrPointerFree,
  NdrPointerFree, NdrPointerFree,
  /* 0x15 */
  NdrSimpleStructFree, NdrSimpleStructFree,
  NdrConformantStructFree, NdrConformantStructFree,
  NdrConformantVaryingStructFree,
  NdrComplexStructFree,
  /* 0x1b */
  NdrConformantArrayFree, 
  NdrConformantVaryingArrayFree,
  NdrFixedArrayFree, NdrFixedArrayFree,
  NdrVaryingArrayFree, NdrVaryingArrayFree,
  NdrComplexArrayFree,
  /* 0x22 */
  0, 0, 0,
  0, 0, 0, 0, 0,
  /* 0x2a */
  NdrEncapsulatedUnionFree,
  NdrNonEncapsulatedUnionFree,
  0,
  NdrXmitOrRepAsFree, NdrXmitOrRepAsFree,
  /* 0x2f */
  NdrInterfacePointerFree,
  /* 0x30 */
  0,
  /* 0xb1 */
  0, 0, 0,
  NdrUserMarshalFree,
  0, 0,
  /* 0xb7 */
  NdrRangeFree
};

void * WINAPI NdrAllocate(MIDL_STUB_MESSAGE *pStubMsg, size_t len)
{
  /* hmm, this is probably supposed to do more? */
  return pStubMsg->pfnAllocate(len);
}

static void WINAPI NdrFree(MIDL_STUB_MESSAGE *pStubMsg, unsigned char *Pointer)
{
  pStubMsg->pfnFree(Pointer);
}

static inline BOOL IsConformanceOrVariancePresent(PFORMAT_STRING pFormat)
{
    return (*(const ULONG *)pFormat != -1);
}

static PFORMAT_STRING ReadConformance(MIDL_STUB_MESSAGE *pStubMsg, PFORMAT_STRING pFormat)
{
  ALIGN_POINTER(pStubMsg->Buffer, 4);
  pStubMsg->MaxCount = NDR_LOCAL_UINT32_READ(pStubMsg->Buffer);
  pStubMsg->Buffer += 4;
  TRACE("unmarshalled conformance is %ld\n", pStubMsg->MaxCount);
  if (pStubMsg->fHasNewCorrDesc)
    return pFormat+6;
  else
    return pFormat+4;
}

static inline PFORMAT_STRING ReadVariance(MIDL_STUB_MESSAGE *pStubMsg, PFORMAT_STRING pFormat, ULONG MaxValue)
{
  if (pFormat && !IsConformanceOrVariancePresent(pFormat))
  {
    pStubMsg->Offset = 0;
    pStubMsg->ActualCount = pStubMsg->MaxCount;
    goto done;
  }

  ALIGN_POINTER(pStubMsg->Buffer, 4);
  pStubMsg->Offset      = NDR_LOCAL_UINT32_READ(pStubMsg->Buffer);
  pStubMsg->Buffer += 4;
  TRACE("offset is %d\n", pStubMsg->Offset);
  pStubMsg->ActualCount = NDR_LOCAL_UINT32_READ(pStubMsg->Buffer);
  pStubMsg->Buffer += 4;
  TRACE("variance is %d\n", pStubMsg->ActualCount);

  if ((pStubMsg->ActualCount > MaxValue) ||
      (pStubMsg->ActualCount + pStubMsg->Offset > MaxValue))
  {
    ERR("invalid array bound(s): ActualCount = %d, Offset = %d, MaxValue = %d\n",
        pStubMsg->ActualCount, pStubMsg->Offset, MaxValue);
    RpcRaiseException(RPC_S_INVALID_BOUND);
    return NULL;
  }

done:
  if (pStubMsg->fHasNewCorrDesc)
    return pFormat+6;
  else
    return pFormat+4;
}

/* writes the conformance value to the buffer */
static inline void WriteConformance(MIDL_STUB_MESSAGE *pStubMsg)
{
    ALIGN_POINTER(pStubMsg->Buffer, 4);
    NDR_LOCAL_UINT32_WRITE(pStubMsg->Buffer, pStubMsg->MaxCount);
    pStubMsg->Buffer += 4;
}

/* writes the variance values to the buffer */
static inline void WriteVariance(MIDL_STUB_MESSAGE *pStubMsg)
{
    ALIGN_POINTER(pStubMsg->Buffer, 4);
    NDR_LOCAL_UINT32_WRITE(pStubMsg->Buffer, pStubMsg->Offset);
    pStubMsg->Buffer += 4;
    NDR_LOCAL_UINT32_WRITE(pStubMsg->Buffer, pStubMsg->ActualCount);
    pStubMsg->Buffer += 4;
}

/* requests buffer space for the conformance value */
static inline void SizeConformance(MIDL_STUB_MESSAGE *pStubMsg)
{
    ALIGN_LENGTH(pStubMsg->BufferLength, 4);
    pStubMsg->BufferLength += 4;
}

/* requests buffer space for the variance values */
static inline void SizeVariance(MIDL_STUB_MESSAGE *pStubMsg)
{
    ALIGN_LENGTH(pStubMsg->BufferLength, 4);
    pStubMsg->BufferLength += 8;
}

PFORMAT_STRING ComputeConformanceOrVariance(
    MIDL_STUB_MESSAGE *pStubMsg, unsigned char *pMemory,
    PFORMAT_STRING pFormat, ULONG_PTR def, ULONG_PTR *pCount)
{
  BYTE dtype = pFormat[0] & 0xf;
  short ofs = *(const short *)&pFormat[2];
  LPVOID ptr = NULL;
  DWORD data = 0;

  if (!IsConformanceOrVariancePresent(pFormat)) {
    /* null descriptor */
    *pCount = def;
    goto finish_conf;
  }

  switch (pFormat[0] & 0xf0) {
  case RPC_FC_NORMAL_CONFORMANCE:
    TRACE("normal conformance, ofs=%d\n", ofs);
    ptr = pMemory;
    break;
  case RPC_FC_POINTER_CONFORMANCE:
    TRACE("pointer conformance, ofs=%d\n", ofs);
    ptr = pStubMsg->Memory;
    break;
  case RPC_FC_TOP_LEVEL_CONFORMANCE:
    TRACE("toplevel conformance, ofs=%d\n", ofs);
    if (pStubMsg->StackTop) {
      ptr = pStubMsg->StackTop;
    }
    else {
      /* -Os mode, *pCount is already set */
      goto finish_conf;
    }
    break;
  case RPC_FC_CONSTANT_CONFORMANCE:
    data = ofs | ((DWORD)pFormat[1] << 16);
    TRACE("constant conformance, val=%d\n", data);
    *pCount = data;
    goto finish_conf;
  case RPC_FC_TOP_LEVEL_MULTID_CONFORMANCE:
    FIXME("toplevel multidimensional conformance, ofs=%d\n", ofs);
    if (pStubMsg->StackTop) {
      ptr = pStubMsg->StackTop;
    }
    else {
      /* ? */
      goto done_conf_grab;
    }
    break;
  default:
    FIXME("unknown conformance type %x\n", pFormat[0] & 0xf0);
  }

  switch (pFormat[1]) {
  case RPC_FC_DEREFERENCE:
    ptr = *(LPVOID*)((char *)ptr + ofs);
    break;
  case RPC_FC_CALLBACK:
  {
    unsigned char *old_stack_top = pStubMsg->StackTop;
    pStubMsg->StackTop = ptr;

    /* ofs is index into StubDesc->apfnExprEval */
    TRACE("callback conformance into apfnExprEval[%d]\n", ofs);
    pStubMsg->StubDesc->apfnExprEval[ofs](pStubMsg);

    pStubMsg->StackTop = old_stack_top;

    /* the callback function always stores the computed value in MaxCount */
    *pCount = pStubMsg->MaxCount;
    goto finish_conf;
  }
  default:
    ptr = (char *)ptr + ofs;
    break;
  }

  switch (dtype) {
  case RPC_FC_LONG:
  case RPC_FC_ULONG:
    data = *(DWORD*)ptr;
    break;
  case RPC_FC_SHORT:
    data = *(SHORT*)ptr;
    break;
  case RPC_FC_USHORT:
    data = *(USHORT*)ptr;
    break;
  case RPC_FC_CHAR:
  case RPC_FC_SMALL:
    data = *(CHAR*)ptr;
    break;
  case RPC_FC_BYTE:
  case RPC_FC_USMALL:
    data = *(UCHAR*)ptr;
    break;
  default:
    FIXME("unknown conformance data type %x\n", dtype);
    goto done_conf_grab;
  }
  TRACE("dereferenced data type %x at %p, got %d\n", dtype, ptr, data);

done_conf_grab:
  switch (pFormat[1]) {
  case RPC_FC_DEREFERENCE: /* already handled */
  case 0: /* no op */
    *pCount = data;
    break;
  case RPC_FC_ADD_1:
    *pCount = data + 1;
    break;
  case RPC_FC_SUB_1:
    *pCount = data - 1;
    break;
  case RPC_FC_MULT_2:
    *pCount = data * 2;
    break;
  case RPC_FC_DIV_2:
    *pCount = data / 2;
    break;
  default:
    FIXME("unknown conformance op %d\n", pFormat[1]);
    goto finish_conf;
  }

finish_conf:
  TRACE("resulting conformance is %ld\n", *pCount);
  if (pStubMsg->fHasNewCorrDesc)
    return pFormat+6;
  else
    return pFormat+4;
}

/* multiply two numbers together, raising an RPC_S_INVALID_BOUND exception if
 * the result overflows 32-bits */
static inline ULONG safe_multiply(ULONG a, ULONG b)
{
    ULONGLONG ret = (ULONGLONG)a * b;
    if (ret > 0xffffffff)
    {
        RpcRaiseException(RPC_S_INVALID_BOUND);
        return 0;
    }
    return ret;
}


/*
 * NdrConformantString:
 * 
 * What MS calls a ConformantString is, in DCE terminology,
 * a Varying-Conformant String.
 * [
 *   maxlen: DWORD (max # of CHARTYPE characters, inclusive of '\0')
 *   offset: DWORD (actual string data begins at (offset) CHARTYPE's
 *           into unmarshalled string) 
 *   length: DWORD (# of CHARTYPE characters, inclusive of '\0')
 *   [ 
 *     data: CHARTYPE[maxlen]
 *   ] 
 * ], where CHARTYPE is the appropriate character type (specified externally)
 *
 */

/***********************************************************************
 *            NdrConformantStringMarshall [RPCRT4.@]
 */
unsigned char *WINAPI NdrConformantStringMarshall(MIDL_STUB_MESSAGE *pStubMsg,
  unsigned char *pszMessage, PFORMAT_STRING pFormat)
{ 
  ULONG esize, size;

  TRACE("(pStubMsg == ^%p, pszMessage == ^%p, pFormat == ^%p)\n", pStubMsg, pszMessage, pFormat);
  
  if (*pFormat == RPC_FC_C_CSTRING) {
    TRACE("string=%s\n", debugstr_a((char*)pszMessage));
    pStubMsg->ActualCount = strlen((char*)pszMessage)+1;
    esize = 1;
  }
  else if (*pFormat == RPC_FC_C_WSTRING) {
    TRACE("string=%s\n", debugstr_w((LPWSTR)pszMessage));
    pStubMsg->ActualCount = strlenW((LPWSTR)pszMessage)+1;
    esize = 2;
  }
  else {
    ERR("Unhandled string type: %#x\n", *pFormat); 
    /* FIXME: raise an exception. */
    return NULL;
  }

  if (pFormat[1] == RPC_FC_STRING_SIZED)
    pFormat = ComputeConformance(pStubMsg, pszMessage, pFormat + 2, 0);
  else
    pStubMsg->MaxCount = pStubMsg->ActualCount;
  pStubMsg->Offset = 0;
  WriteConformance(pStubMsg);
  WriteVariance(pStubMsg);

  size = safe_multiply(esize, pStubMsg->ActualCount);
  memcpy(pStubMsg->Buffer, pszMessage, size); /* the string itself */
  pStubMsg->Buffer += size;

  STD_OVERFLOW_CHECK(pStubMsg);

  /* success */
  return NULL; /* is this always right? */
}

/***********************************************************************
 *           NdrConformantStringBufferSize [RPCRT4.@]
 */
void WINAPI NdrConformantStringBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
  unsigned char* pMemory, PFORMAT_STRING pFormat)
{
  ULONG esize;

  TRACE("(pStubMsg == ^%p, pMemory == ^%p, pFormat == ^%p)\n", pStubMsg, pMemory, pFormat);

  SizeConformance(pStubMsg);
  SizeVariance(pStubMsg);

  if (*pFormat == RPC_FC_C_CSTRING) {
    TRACE("string=%s\n", debugstr_a((char*)pMemory));
    pStubMsg->ActualCount = strlen((char*)pMemory)+1;
    esize = 1;
  }
  else if (*pFormat == RPC_FC_C_WSTRING) {
    TRACE("string=%s\n", debugstr_w((LPWSTR)pMemory));
    pStubMsg->ActualCount = strlenW((LPWSTR)pMemory)+1;
    esize = 2;
  }
  else {
    ERR("Unhandled string type: %#x\n", *pFormat); 
    /* FIXME: raise an exception */
    return;
  }

  if (pFormat[1] == RPC_FC_STRING_SIZED)
    pFormat = ComputeConformance(pStubMsg, pMemory, pFormat + 2, 0);
  else
    pStubMsg->MaxCount = pStubMsg->ActualCount;

  pStubMsg->BufferLength += safe_multiply(esize, pStubMsg->ActualCount);
}

/************************************************************************
 *            NdrConformantStringMemorySize [RPCRT4.@]
 */
ULONG WINAPI NdrConformantStringMemorySize( PMIDL_STUB_MESSAGE pStubMsg,
  PFORMAT_STRING pFormat )
{
  ULONG rslt = 0;

  FIXME("(pStubMsg == ^%p, pFormat == ^%p)\n", pStubMsg, pFormat);

  assert(pStubMsg && pFormat);

  if (*pFormat == RPC_FC_C_CSTRING) {
    rslt = NDR_LOCAL_UINT32_READ(pStubMsg->Buffer); /* maxlen */
  }
  else if (*pFormat == RPC_FC_C_WSTRING) {
    rslt = NDR_LOCAL_UINT32_READ(pStubMsg->Buffer)*2; /* maxlen */
  }
  else {
    ERR("Unhandled string type: %#x\n", *pFormat);
    /* FIXME: raise an exception */
  }

  if (pFormat[1] != RPC_FC_PAD) {
    FIXME("sized string format=%d\n", pFormat[1]);
  }

  TRACE("  --> %u\n", rslt);
  return rslt;
}

/************************************************************************
 *           NdrConformantStringUnmarshall [RPCRT4.@]
 */
unsigned char *WINAPI NdrConformantStringUnmarshall( PMIDL_STUB_MESSAGE pStubMsg,
  unsigned char** ppMemory, PFORMAT_STRING pFormat, unsigned char fMustAlloc )
{
  ULONG bufsize, memsize, esize, i;

  TRACE("(pStubMsg == ^%p, *pMemory == ^%p, pFormat == ^%p, fMustAlloc == %u)\n",
    pStubMsg, *ppMemory, pFormat, fMustAlloc);

  assert(pFormat && ppMemory && pStubMsg);

  ReadConformance(pStubMsg, NULL);
  ReadVariance(pStubMsg, NULL, pStubMsg->MaxCount);

  if (*pFormat == RPC_FC_C_CSTRING) esize = 1;
  else if (*pFormat == RPC_FC_C_WSTRING) esize = 2;
  else {
    ERR("Unhandled string type: %#x\n", *pFormat);
    /* FIXME: raise an exception */
    esize = 0;
  }

  memsize = safe_multiply(esize, pStubMsg->MaxCount);
  bufsize = safe_multiply(esize, pStubMsg->ActualCount);

  /* strings must always have null terminating bytes */
  if (bufsize < esize)
  {
    ERR("invalid string length of %d\n", pStubMsg->ActualCount);
    RpcRaiseException(RPC_S_INVALID_BOUND);
    return NULL;
  }
  for (i = bufsize - esize; i < bufsize; i++)
    if (pStubMsg->Buffer[i] != 0)
    {
      ERR("string not null-terminated at byte position %d, data is 0x%x\n",
        i, pStubMsg->Buffer[i]);
      RpcRaiseException(RPC_S_INVALID_BOUND);
      return NULL;
    }

  if (fMustAlloc || !*ppMemory)
    *ppMemory = NdrAllocate(pStubMsg, memsize);

  memcpy(*ppMemory, pStubMsg->Buffer, bufsize);

  pStubMsg->Buffer += bufsize;

  if (*pFormat == RPC_FC_C_CSTRING) {
    TRACE("string=%s\n", debugstr_a((char*)*ppMemory));
  }
  else if (*pFormat == RPC_FC_C_WSTRING) {
    TRACE("string=%s\n", debugstr_w((LPWSTR)*ppMemory));
  }

  return NULL; /* FIXME: is this always right? */
}

/***********************************************************************
 *           NdrNonConformantStringMarshall [RPCRT4.@]
 */
unsigned char *  WINAPI NdrNonConformantStringMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    FIXME("stub\n");
    return NULL;
}

/***********************************************************************
 *           NdrNonConformantStringUnmarshall [RPCRT4.@]
 */
unsigned char *  WINAPI NdrNonConformantStringUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char **ppMemory,
                                PFORMAT_STRING pFormat,
                                unsigned char fMustAlloc)
{
    FIXME("stub\n");
    return NULL;
}

/***********************************************************************
 *           NdrNonConformantStringBufferSize [RPCRT4.@]
 */
void WINAPI NdrNonConformantStringBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    FIXME("stub\n");
}

/***********************************************************************
 *           NdrNonConformantStringMemorySize [RPCRT4.@]
 */
ULONG WINAPI NdrNonConformantStringMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                PFORMAT_STRING pFormat)
{
    FIXME("stub\n");
    return 0;
}

static inline void dump_pointer_attr(unsigned char attr)
{
    if (attr & RPC_FC_P_ALLOCALLNODES)
        TRACE(" RPC_FC_P_ALLOCALLNODES");
    if (attr & RPC_FC_P_DONTFREE)
        TRACE(" RPC_FC_P_DONTFREE");
    if (attr & RPC_FC_P_ONSTACK)
        TRACE(" RPC_FC_P_ONSTACK");
    if (attr & RPC_FC_P_SIMPLEPOINTER)
        TRACE(" RPC_FC_P_SIMPLEPOINTER");
    if (attr & RPC_FC_P_DEREF)
        TRACE(" RPC_FC_P_DEREF");
    TRACE("\n");
}

/***********************************************************************
 *           PointerMarshall [internal]
 */
static void PointerMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                            unsigned char *Buffer,
                            unsigned char *Pointer,
                            PFORMAT_STRING pFormat)
{
  unsigned type = pFormat[0], attr = pFormat[1];
  PFORMAT_STRING desc;
  NDR_MARSHALL m;
  ULONG pointer_id;
  int pointer_needs_marshaling;

  TRACE("(%p,%p,%p,%p)\n", pStubMsg, Buffer, Pointer, pFormat);
  TRACE("type=0x%x, attr=", type); dump_pointer_attr(attr);
  pFormat += 2;
  if (attr & RPC_FC_P_SIMPLEPOINTER) desc = pFormat;
  else desc = pFormat + *(const SHORT*)pFormat;

  switch (type) {
  case RPC_FC_RP: /* ref pointer (always non-null) */
#if 0 /* this causes problems for InstallShield so is disabled - we need more tests */
    if (!Pointer)
      RpcRaiseException(RPC_X_NULL_REF_POINTER);
#endif
    pointer_needs_marshaling = 1;
    break;
  case RPC_FC_UP: /* unique pointer */
  case RPC_FC_OP: /* object pointer - same as unique here */
    if (Pointer)
      pointer_needs_marshaling = 1;
    else
      pointer_needs_marshaling = 0;
    pointer_id = (ULONG)Pointer;
    TRACE("writing 0x%08x to buffer\n", pointer_id);
    NDR_LOCAL_UINT32_WRITE(Buffer, pointer_id);
    break;
  case RPC_FC_FP:
    pointer_needs_marshaling = !NdrFullPointerQueryPointer(
      pStubMsg->FullPtrXlatTables, Pointer, 1, &pointer_id);
    TRACE("writing 0x%08x to buffer\n", pointer_id);
    NDR_LOCAL_UINT32_WRITE(Buffer, pointer_id);
    break;
  default:
    FIXME("unhandled ptr type=%02x\n", type);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
    return;
  }

  TRACE("calling marshaller for type 0x%x\n", (int)*desc);

  if (pointer_needs_marshaling) {
    if (attr & RPC_FC_P_DEREF) {
      Pointer = *(unsigned char**)Pointer;
      TRACE("deref => %p\n", Pointer);
    }
    m = NdrMarshaller[*desc & NDR_TABLE_MASK];
    if (m) m(pStubMsg, Pointer, desc);
    else FIXME("no marshaller for data type=%02x\n", *desc);
  }

  STD_OVERFLOW_CHECK(pStubMsg);
}

/***********************************************************************
 *           PointerUnmarshall [internal]
 */
static void PointerUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                              unsigned char *Buffer,
                              unsigned char **pPointer,
                              PFORMAT_STRING pFormat,
                              unsigned char fMustAlloc)
{
  unsigned type = pFormat[0], attr = pFormat[1];
  PFORMAT_STRING desc;
  NDR_UNMARSHALL m;
  DWORD pointer_id = 0;
  int pointer_needs_unmarshaling;

  TRACE("(%p,%p,%p,%p,%d)\n", pStubMsg, Buffer, pPointer, pFormat, fMustAlloc);
  TRACE("type=0x%x, attr=", type); dump_pointer_attr(attr);
  pFormat += 2;
  if (attr & RPC_FC_P_SIMPLEPOINTER) desc = pFormat;
  else desc = pFormat + *(const SHORT*)pFormat;

  switch (type) {
  case RPC_FC_RP: /* ref pointer (always non-null) */
    pointer_needs_unmarshaling = 1;
    break;
  case RPC_FC_UP: /* unique pointer */
    pointer_id = NDR_LOCAL_UINT32_READ(Buffer);
    TRACE("pointer_id is 0x%08x\n", pointer_id);
    if (pointer_id)
      pointer_needs_unmarshaling = 1;
    else {
      *pPointer = NULL;
      pointer_needs_unmarshaling = 0;
    }
    break;
  case RPC_FC_OP: /* object pointer - we must free data before overwriting it */
    pointer_id = NDR_LOCAL_UINT32_READ(Buffer);
    TRACE("pointer_id is 0x%08x\n", pointer_id);
    if (!fMustAlloc && *pPointer)
    {
        FIXME("free object pointer %p\n", *pPointer);
        *pPointer = NULL;
    }
    if (pointer_id)
      pointer_needs_unmarshaling = 1;
    else
      pointer_needs_unmarshaling = 0;
    break;
  case RPC_FC_FP:
    pointer_id = NDR_LOCAL_UINT32_READ(Buffer);
    TRACE("pointer_id is 0x%08x\n", pointer_id);
    pointer_needs_unmarshaling = !NdrFullPointerQueryRefId(
      pStubMsg->FullPtrXlatTables, pointer_id, 1, (void **)pPointer);
    break;
  default:
    FIXME("unhandled ptr type=%02x\n", type);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
    return;
  }

  if (pointer_needs_unmarshaling) {
    if (attr & RPC_FC_P_DEREF) {
      if (!*pPointer || fMustAlloc)
        *pPointer = NdrAllocate(pStubMsg, sizeof(void *));
      pPointer = *(unsigned char***)pPointer;
      TRACE("deref => %p\n", pPointer);
    }
    m = NdrUnmarshaller[*desc & NDR_TABLE_MASK];
    if (m) m(pStubMsg, pPointer, desc, fMustAlloc);
    else FIXME("no unmarshaller for data type=%02x\n", *desc);

    if (type == RPC_FC_FP)
      NdrFullPointerInsertRefId(pStubMsg->FullPtrXlatTables, pointer_id,
                                *pPointer);
  }

  TRACE("pointer=%p\n", *pPointer);
}

/***********************************************************************
 *           PointerBufferSize [internal]
 */
static void PointerBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                              unsigned char *Pointer,
                              PFORMAT_STRING pFormat)
{
  unsigned type = pFormat[0], attr = pFormat[1];
  PFORMAT_STRING desc;
  NDR_BUFFERSIZE m;
  int pointer_needs_sizing;
  ULONG pointer_id;

  TRACE("(%p,%p,%p)\n", pStubMsg, Pointer, pFormat);
  TRACE("type=0x%x, attr=", type); dump_pointer_attr(attr);
  pFormat += 2;
  if (attr & RPC_FC_P_SIMPLEPOINTER) desc = pFormat;
  else desc = pFormat + *(const SHORT*)pFormat;

  switch (type) {
  case RPC_FC_RP: /* ref pointer (always non-null) */
    break;
  case RPC_FC_OP:
  case RPC_FC_UP:
    /* NULL pointer has no further representation */
    if (!Pointer)
        return;
    break;
  case RPC_FC_FP:
    pointer_needs_sizing = !NdrFullPointerQueryPointer(
      pStubMsg->FullPtrXlatTables, Pointer, 0, &pointer_id);
    if (!pointer_needs_sizing)
      return;
    break;
  default:
    FIXME("unhandled ptr type=%02x\n", type);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
    return;
  }

  if (attr & RPC_FC_P_DEREF) {
    Pointer = *(unsigned char**)Pointer;
    TRACE("deref => %p\n", Pointer);
  }

  m = NdrBufferSizer[*desc & NDR_TABLE_MASK];
  if (m) m(pStubMsg, Pointer, desc);
  else FIXME("no buffersizer for data type=%02x\n", *desc);
}

/***********************************************************************
 *           PointerMemorySize [internal]
 */
static unsigned long PointerMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                       unsigned char *Buffer,
                                       PFORMAT_STRING pFormat)
{
  unsigned type = pFormat[0], attr = pFormat[1];
  PFORMAT_STRING desc;
  NDR_MEMORYSIZE m;

  FIXME("(%p,%p,%p): stub\n", pStubMsg, Buffer, pFormat);
  TRACE("type=0x%x, attr=", type); dump_pointer_attr(attr);
  pFormat += 2;
  if (attr & RPC_FC_P_SIMPLEPOINTER) desc = pFormat;
  else desc = pFormat + *(const SHORT*)pFormat;

  switch (type) {
  case RPC_FC_RP: /* ref pointer (always non-null) */
    break;
  default:
    FIXME("unhandled ptr type=%02x\n", type);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
  }

  if (attr & RPC_FC_P_DEREF) {
    TRACE("deref\n");
  }

  m = NdrMemorySizer[*desc & NDR_TABLE_MASK];
  if (m) m(pStubMsg, desc);
  else FIXME("no memorysizer for data type=%02x\n", *desc);

  return 0;
}

/***********************************************************************
 *           PointerFree [internal]
 */
static void PointerFree(PMIDL_STUB_MESSAGE pStubMsg,
                        unsigned char *Pointer,
                        PFORMAT_STRING pFormat)
{
  unsigned type = pFormat[0], attr = pFormat[1];
  PFORMAT_STRING desc;
  NDR_FREE m;

  TRACE("(%p,%p,%p)\n", pStubMsg, Pointer, pFormat);
  TRACE("type=0x%x, attr=", type); dump_pointer_attr(attr);
  if (attr & RPC_FC_P_DONTFREE) return;
  pFormat += 2;
  if (attr & RPC_FC_P_SIMPLEPOINTER) desc = pFormat;
  else desc = pFormat + *(const SHORT*)pFormat;

  if (!Pointer) return;

  if (type == RPC_FC_FP) {
    int pointer_needs_freeing = NdrFullPointerFree(
      pStubMsg->FullPtrXlatTables, Pointer);
    if (!pointer_needs_freeing)
      return;
  }

  if (attr & RPC_FC_P_DEREF) {
    Pointer = *(unsigned char**)Pointer;
    TRACE("deref => %p\n", Pointer);
  }

  m = NdrFreer[*desc & NDR_TABLE_MASK];
  if (m) m(pStubMsg, Pointer, desc);

  /* hmm... is this sensible?
   * perhaps we should check if the memory comes from NdrAllocate,
   * and deallocate only if so - checking if the pointer is between
   * BufferStart and BufferEnd is probably no good since the buffer
   * may be reallocated when the server wants to marshal the reply */
  switch (*desc) {
  case RPC_FC_BOGUS_STRUCT:
  case RPC_FC_BOGUS_ARRAY:
  case RPC_FC_USER_MARSHAL:
  case RPC_FC_CARRAY:
  case RPC_FC_CVARRAY:
    break;
  default:
    FIXME("unhandled data type=%02x\n", *desc);
    break;
  case RPC_FC_C_CSTRING:
  case RPC_FC_C_WSTRING:
    if (pStubMsg->ReuseBuffer) goto notfree;
    break;
  case RPC_FC_IP:
    goto notfree;
  }

  if (attr & RPC_FC_P_ONSTACK) {
    TRACE("not freeing stack ptr %p\n", Pointer);
    return;
  }
  TRACE("freeing %p\n", Pointer);
  NdrFree(pStubMsg, Pointer);
  return;
notfree:
  TRACE("not freeing %p\n", Pointer);
}

/***********************************************************************
 *           EmbeddedPointerMarshall
 */
static unsigned char * EmbeddedPointerMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                               unsigned char *pMemory,
                                               PFORMAT_STRING pFormat)
{
  unsigned char *Mark = pStubMsg->BufferMark;
  unsigned long Offset = pStubMsg->Offset;
  unsigned ofs, rep, count, stride, xofs;
  unsigned i;
  unsigned char *saved_buffer = NULL;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  if (*pFormat != RPC_FC_PP) return NULL;
  pFormat += 2;

  if (pStubMsg->PointerBufferMark)
  {
    saved_buffer = pStubMsg->Buffer;
    pStubMsg->Buffer = pStubMsg->PointerBufferMark;
    pStubMsg->PointerBufferMark = NULL;
  }

  while (pFormat[0] != RPC_FC_END) {
    switch (pFormat[0]) {
    default:
      FIXME("unknown repeat type %d\n", pFormat[0]);
    case RPC_FC_NO_REPEAT:
      rep = 1;
      stride = 0;
      ofs = 0;
      count = 1;
      xofs = 0;
      pFormat += 2;
      break;
    case RPC_FC_FIXED_REPEAT:
      rep = *(const WORD*)&pFormat[2];
      stride = *(const WORD*)&pFormat[4];
      ofs = *(const WORD*)&pFormat[6];
      count = *(const WORD*)&pFormat[8];
      xofs = 0;
      pFormat += 10;
      break;
    case RPC_FC_VARIABLE_REPEAT:
      rep = (pFormat[1] == RPC_FC_VARIABLE_OFFSET) ? pStubMsg->ActualCount : pStubMsg->MaxCount;
      stride = *(const WORD*)&pFormat[2];
      ofs = *(const WORD*)&pFormat[4];
      count = *(const WORD*)&pFormat[6];
      xofs = (pFormat[1] == RPC_FC_VARIABLE_OFFSET) ? Offset * stride : 0;
      pFormat += 8;
      break;
    }
    for (i = 0; i < rep; i++) {
      PFORMAT_STRING info = pFormat;
      unsigned char *membase = pMemory + ofs + (i * stride);
      unsigned char *bufbase = Mark + ofs + (i * stride);
      unsigned u;

      for (u=0; u<count; u++,info+=8) {
        unsigned char *memptr = membase + *(const SHORT*)&info[0];
        unsigned char *bufptr = bufbase + *(const SHORT*)&info[2];
        unsigned char *saved_memory = pStubMsg->Memory;

        pStubMsg->Memory = pMemory;
        PointerMarshall(pStubMsg, bufptr, *(unsigned char**)memptr, info+4);
        pStubMsg->Memory = saved_memory;
      }
    }
    pFormat += 8 * count;
  }

  if (saved_buffer)
  {
    pStubMsg->PointerBufferMark = pStubMsg->Buffer;
    pStubMsg->Buffer = saved_buffer;
  }

  STD_OVERFLOW_CHECK(pStubMsg);

  return NULL;
}

/***********************************************************************
 *           EmbeddedPointerUnmarshall
 */
static unsigned char * EmbeddedPointerUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                                 unsigned char **ppMemory,
                                                 PFORMAT_STRING pFormat,
                                                 unsigned char fMustAlloc)
{
  unsigned char *Mark = pStubMsg->BufferMark;
  unsigned long Offset = pStubMsg->Offset;
  unsigned ofs, rep, count, stride, xofs;
  unsigned i;
  unsigned char *saved_buffer = NULL;

  TRACE("(%p,%p,%p,%d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);

  if (*pFormat != RPC_FC_PP) return NULL;
  pFormat += 2;

  if (pStubMsg->PointerBufferMark)
  {
    saved_buffer = pStubMsg->Buffer;
    pStubMsg->Buffer = pStubMsg->PointerBufferMark;
    pStubMsg->PointerBufferMark = NULL;
  }

  while (pFormat[0] != RPC_FC_END) {
    TRACE("pFormat[0] = 0x%x\n", pFormat[0]);
    switch (pFormat[0]) {
    default:
      FIXME("unknown repeat type %d\n", pFormat[0]);
    case RPC_FC_NO_REPEAT:
      rep = 1;
      stride = 0;
      ofs = 0;
      count = 1;
      xofs = 0;
      pFormat += 2;
      break;
    case RPC_FC_FIXED_REPEAT:
      rep = *(const WORD*)&pFormat[2];
      stride = *(const WORD*)&pFormat[4];
      ofs = *(const WORD*)&pFormat[6];
      count = *(const WORD*)&pFormat[8];
      xofs = 0;
      pFormat += 10;
      break;
    case RPC_FC_VARIABLE_REPEAT:
      rep = (pFormat[1] == RPC_FC_VARIABLE_OFFSET) ? pStubMsg->ActualCount : pStubMsg->MaxCount;
      stride = *(const WORD*)&pFormat[2];
      ofs = *(const WORD*)&pFormat[4];
      count = *(const WORD*)&pFormat[6];
      xofs = (pFormat[1] == RPC_FC_VARIABLE_OFFSET) ? Offset * stride : 0;
      pFormat += 8;
      break;
    }
    /* ofs doesn't seem to matter in this context */
    for (i = 0; i < rep; i++) {
      PFORMAT_STRING info = pFormat;
      unsigned char *membase = *ppMemory + ofs + (i * stride);
      unsigned char *bufbase = Mark + ofs + (i * stride);
      unsigned u;

      for (u=0; u<count; u++,info+=8) {
        unsigned char *memptr = membase + *(const SHORT*)&info[0];
        unsigned char *bufptr = bufbase + *(const SHORT*)&info[2];
        PointerUnmarshall(pStubMsg, bufptr, (unsigned char**)memptr, info+4, TRUE);
      }
    }
    pFormat += 8 * count;
  }

  if (saved_buffer)
  {
    pStubMsg->PointerBufferMark = pStubMsg->Buffer;
    pStubMsg->Buffer = saved_buffer;
  }

  return NULL;
}

/***********************************************************************
 *           EmbeddedPointerBufferSize
 */
static void EmbeddedPointerBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                      unsigned char *pMemory,
                                      PFORMAT_STRING pFormat)
{
  unsigned long Offset = pStubMsg->Offset;
  unsigned ofs, rep, count, stride, xofs;
  unsigned i;
  ULONG saved_buffer_length = 0;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  if (pStubMsg->IgnoreEmbeddedPointers) return;

  if (*pFormat != RPC_FC_PP) return;
  pFormat += 2;

  if (pStubMsg->PointerLength)
  {
    saved_buffer_length = pStubMsg->BufferLength;
    pStubMsg->BufferLength = pStubMsg->PointerLength;
    pStubMsg->PointerLength = 0;
  }

  while (pFormat[0] != RPC_FC_END) {
    switch (pFormat[0]) {
    default:
      FIXME("unknown repeat type %d\n", pFormat[0]);
    case RPC_FC_NO_REPEAT:
      rep = 1;
      stride = 0;
      ofs = 0;
      count = 1;
      xofs = 0;
      pFormat += 2;
      break;
    case RPC_FC_FIXED_REPEAT:
      rep = *(const WORD*)&pFormat[2];
      stride = *(const WORD*)&pFormat[4];
      ofs = *(const WORD*)&pFormat[6];
      count = *(const WORD*)&pFormat[8];
      xofs = 0;
      pFormat += 10;
      break;
    case RPC_FC_VARIABLE_REPEAT:
      rep = (pFormat[1] == RPC_FC_VARIABLE_OFFSET) ? pStubMsg->ActualCount : pStubMsg->MaxCount;
      stride = *(const WORD*)&pFormat[2];
      ofs = *(const WORD*)&pFormat[4];
      count = *(const WORD*)&pFormat[6];
      xofs = (pFormat[1] == RPC_FC_VARIABLE_OFFSET) ? Offset * stride : 0;
      pFormat += 8;
      break;
    }
    for (i = 0; i < rep; i++) {
      PFORMAT_STRING info = pFormat;
      unsigned char *membase = pMemory + ofs + (i * stride);
      unsigned u;

      for (u=0; u<count; u++,info+=8) {
        unsigned char *memptr = membase + *(const SHORT*)&info[0];
        unsigned char *saved_memory = pStubMsg->Memory;

        pStubMsg->Memory = pMemory;
        PointerBufferSize(pStubMsg, *(unsigned char**)memptr, info+4);
        pStubMsg->Memory = saved_memory;
      }
    }
    pFormat += 8 * count;
  }

  if (saved_buffer_length)
  {
    pStubMsg->PointerLength = pStubMsg->BufferLength;
    pStubMsg->BufferLength = saved_buffer_length;
  }
}

/***********************************************************************
 *           EmbeddedPointerMemorySize [internal]
 */
static unsigned long EmbeddedPointerMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                               PFORMAT_STRING pFormat)
{
  unsigned long Offset = pStubMsg->Offset;
  unsigned char *Mark = pStubMsg->BufferMark;
  unsigned ofs, rep, count, stride, xofs;
  unsigned i;

  TRACE("(%p,%p)\n", pStubMsg, pFormat);

  if (pStubMsg->IgnoreEmbeddedPointers) return 0;

  FIXME("(%p,%p): stub\n", pStubMsg, pFormat);

  if (*pFormat != RPC_FC_PP) return 0;
  pFormat += 2;

  while (pFormat[0] != RPC_FC_END) {
    switch (pFormat[0]) {
    default:
      FIXME("unknown repeat type %d\n", pFormat[0]);
    case RPC_FC_NO_REPEAT:
      rep = 1;
      stride = 0;
      ofs = 0;
      count = 1;
      xofs = 0;
      pFormat += 2;
      break;
    case RPC_FC_FIXED_REPEAT:
      rep = *(const WORD*)&pFormat[2];
      stride = *(const WORD*)&pFormat[4];
      ofs = *(const WORD*)&pFormat[6];
      count = *(const WORD*)&pFormat[8];
      xofs = 0;
      pFormat += 10;
      break;
    case RPC_FC_VARIABLE_REPEAT:
      rep = (pFormat[1] == RPC_FC_VARIABLE_OFFSET) ? pStubMsg->ActualCount : pStubMsg->MaxCount;
      stride = *(const WORD*)&pFormat[2];
      ofs = *(const WORD*)&pFormat[4];
      count = *(const WORD*)&pFormat[6];
      xofs = (pFormat[1] == RPC_FC_VARIABLE_OFFSET) ? Offset * stride : 0;
      pFormat += 8;
      break;
    }
    /* ofs doesn't seem to matter in this context */
    for (i = 0; i < rep; i++) {
      PFORMAT_STRING info = pFormat;
      unsigned char *bufbase = Mark + ofs + (i * stride);
      unsigned u;
      for (u=0; u<count; u++,info+=8) {
        unsigned char *bufptr = bufbase + *(const SHORT*)&info[2];
        PointerMemorySize(pStubMsg, bufptr, info+4);
      }
    }
    pFormat += 8 * count;
  }

  return 0;
}

/***********************************************************************
 *           EmbeddedPointerFree [internal]
 */
static void EmbeddedPointerFree(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
  unsigned long Offset = pStubMsg->Offset;
  unsigned ofs, rep, count, stride, xofs;
  unsigned i;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  if (*pFormat != RPC_FC_PP) return;
  pFormat += 2;

  while (pFormat[0] != RPC_FC_END) {
    switch (pFormat[0]) {
    default:
      FIXME("unknown repeat type %d\n", pFormat[0]);
    case RPC_FC_NO_REPEAT:
      rep = 1;
      stride = 0;
      ofs = 0;
      count = 1;
      xofs = 0;
      pFormat += 2;
      break;
    case RPC_FC_FIXED_REPEAT:
      rep = *(const WORD*)&pFormat[2];
      stride = *(const WORD*)&pFormat[4];
      ofs = *(const WORD*)&pFormat[6];
      count = *(const WORD*)&pFormat[8];
      xofs = 0;
      pFormat += 10;
      break;
    case RPC_FC_VARIABLE_REPEAT:
      rep = (pFormat[1] == RPC_FC_VARIABLE_OFFSET) ? pStubMsg->ActualCount : pStubMsg->MaxCount;
      stride = *(const WORD*)&pFormat[2];
      ofs = *(const WORD*)&pFormat[4];
      count = *(const WORD*)&pFormat[6];
      xofs = (pFormat[1] == RPC_FC_VARIABLE_OFFSET) ? Offset * stride : 0;
      pFormat += 8;
      break;
    }
    for (i = 0; i < rep; i++) {
      PFORMAT_STRING info = pFormat;
      unsigned char *membase = pMemory + (i * stride);
      unsigned u;

      for (u=0; u<count; u++,info+=8) {
        unsigned char *memptr = membase + *(const SHORT*)&info[0];
        unsigned char *saved_memory = pStubMsg->Memory;

        pStubMsg->Memory = pMemory;
        PointerFree(pStubMsg, *(unsigned char**)memptr, info+4);
        pStubMsg->Memory = saved_memory;
      }
    }
    pFormat += 8 * count;
  }
}

/***********************************************************************
 *           NdrPointerMarshall [RPCRT4.@]
 */
unsigned char * WINAPI NdrPointerMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                          unsigned char *pMemory,
                                          PFORMAT_STRING pFormat)
{
  unsigned char *Buffer;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  /* incremement the buffer here instead of in PointerMarshall,
   * as that is used by embedded pointers which already handle the incrementing
   * the buffer, and shouldn't write any additional pointer data to the wire */
  if (*pFormat != RPC_FC_RP)
  {
    ALIGN_POINTER(pStubMsg->Buffer, 4);
    Buffer = pStubMsg->Buffer;
    pStubMsg->Buffer += 4;
  }
  else
    Buffer = pStubMsg->Buffer;

  PointerMarshall(pStubMsg, Buffer, pMemory, pFormat);

  STD_OVERFLOW_CHECK(pStubMsg);

  return NULL;
}

/***********************************************************************
 *           NdrPointerUnmarshall [RPCRT4.@]
 */
unsigned char * WINAPI NdrPointerUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                            unsigned char **ppMemory,
                                            PFORMAT_STRING pFormat,
                                            unsigned char fMustAlloc)
{
  unsigned char *Buffer;

  TRACE("(%p,%p,%p,%d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);

  /* incremement the buffer here instead of in PointerUnmarshall,
   * as that is used by embedded pointers which already handle the incrementing
   * the buffer, and shouldn't read any additional pointer data from the
   * buffer */
  if (*pFormat != RPC_FC_RP)
  {
    ALIGN_POINTER(pStubMsg->Buffer, 4);
    Buffer = pStubMsg->Buffer;
    pStubMsg->Buffer += 4;
  }
  else
    Buffer = pStubMsg->Buffer;

  PointerUnmarshall(pStubMsg, Buffer, ppMemory, pFormat, fMustAlloc);

  return NULL;
}

/***********************************************************************
 *           NdrPointerBufferSize [RPCRT4.@]
 */
void WINAPI NdrPointerBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                      unsigned char *pMemory,
                                      PFORMAT_STRING pFormat)
{
  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  /* incremement the buffer length here instead of in PointerBufferSize,
   * as that is used by embedded pointers which already handle the buffer
   * length, and shouldn't write anything more to the wire */
  if (*pFormat != RPC_FC_RP)
  {
    ALIGN_LENGTH(pStubMsg->BufferLength, 4);
    pStubMsg->BufferLength += 4;
  }

  PointerBufferSize(pStubMsg, pMemory, pFormat);
}

/***********************************************************************
 *           NdrPointerMemorySize [RPCRT4.@]
 */
ULONG WINAPI NdrPointerMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                  PFORMAT_STRING pFormat)
{
  /* unsigned size = *(LPWORD)(pFormat+2); */
  FIXME("(%p,%p): stub\n", pStubMsg, pFormat);
  PointerMemorySize(pStubMsg, pStubMsg->Buffer, pFormat);
  return 0;
}

/***********************************************************************
 *           NdrPointerFree [RPCRT4.@]
 */
void WINAPI NdrPointerFree(PMIDL_STUB_MESSAGE pStubMsg,
                           unsigned char *pMemory,
                           PFORMAT_STRING pFormat)
{
  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  PointerFree(pStubMsg, pMemory, pFormat);
}

/***********************************************************************
 *           NdrSimpleTypeMarshall [RPCRT4.@]
 */
void WINAPI NdrSimpleTypeMarshall( PMIDL_STUB_MESSAGE pStubMsg, unsigned char* pMemory,
                                   unsigned char FormatChar )
{
    NdrBaseTypeMarshall(pStubMsg, pMemory, &FormatChar);
}

/***********************************************************************
 *           NdrSimpleTypeUnmarshall [RPCRT4.@]
 */
void WINAPI NdrSimpleTypeUnmarshall( PMIDL_STUB_MESSAGE pStubMsg, unsigned char* pMemory,
                                     unsigned char FormatChar )
{
    NdrBaseTypeUnmarshall(pStubMsg, &pMemory, &FormatChar, 0);
}

/***********************************************************************
 *           NdrSimpleStructMarshall [RPCRT4.@]
 */
unsigned char * WINAPI NdrSimpleStructMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                               unsigned char *pMemory,
                                               PFORMAT_STRING pFormat)
{
  unsigned size = *(const WORD*)(pFormat+2);
  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  ALIGN_POINTER(pStubMsg->Buffer, pFormat[1] + 1);

  memcpy(pStubMsg->Buffer, pMemory, size);
  pStubMsg->BufferMark = pStubMsg->Buffer;
  pStubMsg->Buffer += size;

  if (pFormat[0] != RPC_FC_STRUCT)
    EmbeddedPointerMarshall(pStubMsg, pMemory, pFormat+4);

  STD_OVERFLOW_CHECK(pStubMsg);

  return NULL;
}

/***********************************************************************
 *           NdrSimpleStructUnmarshall [RPCRT4.@]
 */
unsigned char * WINAPI NdrSimpleStructUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                                 unsigned char **ppMemory,
                                                 PFORMAT_STRING pFormat,
                                                 unsigned char fMustAlloc)
{
  unsigned size = *(const WORD*)(pFormat+2);
  TRACE("(%p,%p,%p,%d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);

  ALIGN_POINTER(pStubMsg->Buffer, pFormat[1] + 1);

  if (fMustAlloc) {
    *ppMemory = NdrAllocate(pStubMsg, size);
    memcpy(*ppMemory, pStubMsg->Buffer, size);
  } else {
    if (!pStubMsg->IsClient && !*ppMemory)
      /* for servers, we just point straight into the RPC buffer */
      *ppMemory = pStubMsg->Buffer;
    else
      /* for clients, memory should be provided by caller */
      memcpy(*ppMemory, pStubMsg->Buffer, size);
  }

  pStubMsg->BufferMark = pStubMsg->Buffer;
  pStubMsg->Buffer += size;

  if (pFormat[0] != RPC_FC_STRUCT)
    EmbeddedPointerUnmarshall(pStubMsg, ppMemory, pFormat+4, fMustAlloc);

  return NULL;
}

/***********************************************************************
 *           NdrSimpleStructBufferSize [RPCRT4.@]
 */
void WINAPI NdrSimpleStructBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                      unsigned char *pMemory,
                                      PFORMAT_STRING pFormat)
{
  unsigned size = *(const WORD*)(pFormat+2);
  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  ALIGN_LENGTH(pStubMsg->BufferLength, pFormat[1] + 1);

  pStubMsg->BufferLength += size;
  if (pFormat[0] != RPC_FC_STRUCT)
    EmbeddedPointerBufferSize(pStubMsg, pMemory, pFormat+4);
}

/***********************************************************************
 *           NdrSimpleStructMemorySize [RPCRT4.@]
 */
ULONG WINAPI NdrSimpleStructMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                       PFORMAT_STRING pFormat)
{
  unsigned short size = *(const WORD *)(pFormat+2);

  TRACE("(%p,%p)\n", pStubMsg, pFormat);

  ALIGN_POINTER(pStubMsg->Buffer, pFormat[1] + 1);
  pStubMsg->MemorySize += size;
  pStubMsg->Buffer += size;

  if (pFormat[0] != RPC_FC_STRUCT)
    EmbeddedPointerMemorySize(pStubMsg, pFormat+4);
  return size;
}

/***********************************************************************
 *           NdrSimpleStructFree [RPCRT4.@]
 */
void WINAPI NdrSimpleStructFree(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  if (pFormat[0] != RPC_FC_STRUCT)
    EmbeddedPointerFree(pStubMsg, pMemory, pFormat+4);
}


static unsigned long EmbeddedComplexSize(PMIDL_STUB_MESSAGE pStubMsg,
                                         PFORMAT_STRING pFormat)
{
  switch (*pFormat) {
  case RPC_FC_STRUCT:
  case RPC_FC_PSTRUCT:
  case RPC_FC_CSTRUCT:
  case RPC_FC_BOGUS_STRUCT:
  case RPC_FC_SMFARRAY:
  case RPC_FC_SMVARRAY:
    return *(const WORD*)&pFormat[2];
  case RPC_FC_USER_MARSHAL:
    return *(const WORD*)&pFormat[4];
  case RPC_FC_NON_ENCAPSULATED_UNION:
    pFormat += 2;
    if (pStubMsg->fHasNewCorrDesc)
        pFormat += 6;
    else
        pFormat += 4;

    pFormat += *(const SHORT*)pFormat;
    return *(const SHORT*)pFormat;
  case RPC_FC_IP:
    return sizeof(void *);
  default:
    FIXME("unhandled embedded type %02x\n", *pFormat);
  }
  return 0;
}


static unsigned long EmbeddedComplexMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                               PFORMAT_STRING pFormat)
{
  NDR_MEMORYSIZE m = NdrMemorySizer[*pFormat & NDR_TABLE_MASK];

  if (!m)
  {
    FIXME("no memorysizer for data type=%02x\n", *pFormat);
    return 0;
  }

  return m(pStubMsg, pFormat);
}


static unsigned char * ComplexMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                       unsigned char *pMemory,
                                       PFORMAT_STRING pFormat,
                                       PFORMAT_STRING pPointer)
{
  PFORMAT_STRING desc;
  NDR_MARSHALL m;
  unsigned long size;

  while (*pFormat != RPC_FC_END) {
    switch (*pFormat) {
    case RPC_FC_BYTE:
    case RPC_FC_CHAR:
    case RPC_FC_SMALL:
    case RPC_FC_USMALL:
      TRACE("byte=%d <= %p\n", *(WORD*)pMemory, pMemory);
      memcpy(pStubMsg->Buffer, pMemory, 1);
      pStubMsg->Buffer += 1;
      pMemory += 1;
      break;
    case RPC_FC_WCHAR:
    case RPC_FC_SHORT:
    case RPC_FC_USHORT:
      TRACE("short=%d <= %p\n", *(WORD*)pMemory, pMemory);
      memcpy(pStubMsg->Buffer, pMemory, 2);
      pStubMsg->Buffer += 2;
      pMemory += 2;
      break;
    case RPC_FC_LONG:
    case RPC_FC_ULONG:
    case RPC_FC_ENUM32:
      TRACE("long=%d <= %p\n", *(DWORD*)pMemory, pMemory);
      memcpy(pStubMsg->Buffer, pMemory, 4);
      pStubMsg->Buffer += 4;
      pMemory += 4;
      break;
    case RPC_FC_HYPER:
      TRACE("longlong=%s <= %p\n", wine_dbgstr_longlong(*(ULONGLONG*)pMemory), pMemory);
      memcpy(pStubMsg->Buffer, pMemory, 8);
      pStubMsg->Buffer += 8;
      pMemory += 8;
      break;
    case RPC_FC_POINTER:
    {
      unsigned char *saved_buffer;
      int pointer_buffer_mark_set = 0;
      TRACE("pointer=%p <= %p\n", *(unsigned char**)pMemory, pMemory);
      saved_buffer = pStubMsg->Buffer;
      if (pStubMsg->PointerBufferMark)
      {
        pStubMsg->Buffer = pStubMsg->PointerBufferMark;
        pStubMsg->PointerBufferMark = NULL;
        pointer_buffer_mark_set = 1;
      }
      else
        pStubMsg->Buffer += 4; /* for pointer ID */
      PointerMarshall(pStubMsg, saved_buffer, *(unsigned char**)pMemory, pPointer);
      if (pointer_buffer_mark_set)
      {
        STD_OVERFLOW_CHECK(pStubMsg);
        pStubMsg->PointerBufferMark = pStubMsg->Buffer;
        pStubMsg->Buffer = saved_buffer + 4;
      }
      pPointer += 4;
      pMemory += 4;
      break;
    }
    case RPC_FC_ALIGNM4:
      ALIGN_POINTER(pMemory, 4);
      break;
    case RPC_FC_ALIGNM8:
      ALIGN_POINTER(pMemory, 8);
      break;
    case RPC_FC_STRUCTPAD1:
    case RPC_FC_STRUCTPAD2:
    case RPC_FC_STRUCTPAD3:
    case RPC_FC_STRUCTPAD4:
    case RPC_FC_STRUCTPAD5:
    case RPC_FC_STRUCTPAD6:
    case RPC_FC_STRUCTPAD7:
      pMemory += *pFormat - RPC_FC_STRUCTPAD1 + 1;
      break;
    case RPC_FC_EMBEDDED_COMPLEX:
      pMemory += pFormat[1];
      pFormat += 2;
      desc = pFormat + *(const SHORT*)pFormat;
      size = EmbeddedComplexSize(pStubMsg, desc);
      TRACE("embedded complex (size=%ld) <= %p\n", size, pMemory);
      m = NdrMarshaller[*desc & NDR_TABLE_MASK];
      if (m)
      {
        /* for some reason interface pointers aren't generated as
         * RPC_FC_POINTER, but instead as RPC_FC_EMBEDDED_COMPLEX, yet
         * they still need the derefencing treatment that pointers are
         * given */
        if (*desc == RPC_FC_IP)
          m(pStubMsg, *(unsigned char **)pMemory, desc);
        else
          m(pStubMsg, pMemory, desc);
      }
      else FIXME("no marshaller for embedded type %02x\n", *desc);
      pMemory += size;
      pFormat += 2;
      continue;
    case RPC_FC_PAD:
      break;
    default:
      FIXME("unhandled format 0x%02x\n", *pFormat);
    }
    pFormat++;
  }

  return pMemory;
}

static unsigned char * ComplexUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                         unsigned char *pMemory,
                                         PFORMAT_STRING pFormat,
                                         PFORMAT_STRING pPointer)
{
  PFORMAT_STRING desc;
  NDR_UNMARSHALL m;
  unsigned long size;

  while (*pFormat != RPC_FC_END) {
    switch (*pFormat) {
    case RPC_FC_BYTE:
    case RPC_FC_CHAR:
    case RPC_FC_SMALL:
    case RPC_FC_USMALL:
      memcpy(pMemory, pStubMsg->Buffer, 1);
      TRACE("byte=%d => %p\n", *(WORD*)pMemory, pMemory);
      pStubMsg->Buffer += 1;
      pMemory += 1;
      break;
    case RPC_FC_WCHAR:
    case RPC_FC_SHORT:
    case RPC_FC_USHORT:
      memcpy(pMemory, pStubMsg->Buffer, 2);
      TRACE("short=%d => %p\n", *(WORD*)pMemory, pMemory);
      pStubMsg->Buffer += 2;
      pMemory += 2;
      break;
    case RPC_FC_LONG:
    case RPC_FC_ULONG:
    case RPC_FC_ENUM32:
      memcpy(pMemory, pStubMsg->Buffer, 4);
      TRACE("long=%d => %p\n", *(DWORD*)pMemory, pMemory);
      pStubMsg->Buffer += 4;
      pMemory += 4;
      break;
    case RPC_FC_HYPER:
      memcpy(pMemory, pStubMsg->Buffer, 8);
      TRACE("longlong=%s => %p\n", wine_dbgstr_longlong(*(ULONGLONG*)pMemory), pMemory);
      pStubMsg->Buffer += 8;
      pMemory += 8;
      break;
    case RPC_FC_POINTER:
    {
      unsigned char *saved_buffer;
      int pointer_buffer_mark_set = 0;
      TRACE("pointer => %p\n", pMemory);
      ALIGN_POINTER(pStubMsg->Buffer, 4);
      saved_buffer = pStubMsg->Buffer;
      if (pStubMsg->PointerBufferMark)
      {
        pStubMsg->Buffer = pStubMsg->PointerBufferMark;
        pStubMsg->PointerBufferMark = NULL;
        pointer_buffer_mark_set = 1;
      }
      else
        pStubMsg->Buffer += 4; /* for pointer ID */

      PointerUnmarshall(pStubMsg, saved_buffer, (unsigned char**)pMemory, pPointer, TRUE);
      if (pointer_buffer_mark_set)
      {
        STD_OVERFLOW_CHECK(pStubMsg);
        pStubMsg->PointerBufferMark = pStubMsg->Buffer;
        pStubMsg->Buffer = saved_buffer + 4;
      }
      pPointer += 4;
      pMemory += 4;
      break;
    }
    case RPC_FC_ALIGNM4:
      ALIGN_POINTER(pMemory, 4);
      break;
    case RPC_FC_ALIGNM8:
      ALIGN_POINTER(pMemory, 8);
      break;
    case RPC_FC_STRUCTPAD1:
    case RPC_FC_STRUCTPAD2:
    case RPC_FC_STRUCTPAD3:
    case RPC_FC_STRUCTPAD4:
    case RPC_FC_STRUCTPAD5:
    case RPC_FC_STRUCTPAD6:
    case RPC_FC_STRUCTPAD7:
      pMemory += *pFormat - RPC_FC_STRUCTPAD1 + 1;
      break;
    case RPC_FC_EMBEDDED_COMPLEX:
      pMemory += pFormat[1];
      pFormat += 2;
      desc = pFormat + *(const SHORT*)pFormat;
      size = EmbeddedComplexSize(pStubMsg, desc);
      TRACE("embedded complex (size=%ld) => %p\n", size, pMemory);
      m = NdrUnmarshaller[*desc & NDR_TABLE_MASK];
      memset(pMemory, 0, size); /* just in case */
      if (m)
      {
        /* for some reason interface pointers aren't generated as
         * RPC_FC_POINTER, but instead as RPC_FC_EMBEDDED_COMPLEX, yet
         * they still need the derefencing treatment that pointers are
         * given */
        if (*desc == RPC_FC_IP)
          m(pStubMsg, (unsigned char **)pMemory, desc, FALSE);
        else
          m(pStubMsg, &pMemory, desc, FALSE);
      }
      else FIXME("no unmarshaller for embedded type %02x\n", *desc);
      pMemory += size;
      pFormat += 2;
      continue;
    case RPC_FC_PAD:
      break;
    default:
      FIXME("unhandled format %d\n", *pFormat);
    }
    pFormat++;
  }

  return pMemory;
}

static unsigned char * ComplexBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                         unsigned char *pMemory,
                                         PFORMAT_STRING pFormat,
                                         PFORMAT_STRING pPointer)
{
  PFORMAT_STRING desc;
  NDR_BUFFERSIZE m;
  unsigned long size;

  while (*pFormat != RPC_FC_END) {
    switch (*pFormat) {
    case RPC_FC_BYTE:
    case RPC_FC_CHAR:
    case RPC_FC_SMALL:
    case RPC_FC_USMALL:
      pStubMsg->BufferLength += 1;
      pMemory += 1;
      break;
    case RPC_FC_WCHAR:
    case RPC_FC_SHORT:
    case RPC_FC_USHORT:
      pStubMsg->BufferLength += 2;
      pMemory += 2;
      break;
    case RPC_FC_LONG:
    case RPC_FC_ULONG:
    case RPC_FC_ENUM32:
      pStubMsg->BufferLength += 4;
      pMemory += 4;
      break;
    case RPC_FC_HYPER:
      pStubMsg->BufferLength += 8;
      pMemory += 8;
      break;
    case RPC_FC_POINTER:
      if (!pStubMsg->IgnoreEmbeddedPointers)
      {
        int saved_buffer_length = pStubMsg->BufferLength;
        pStubMsg->BufferLength = pStubMsg->PointerLength;
        pStubMsg->PointerLength = 0;
        if(!pStubMsg->BufferLength)
          ERR("BufferLength == 0??\n");
        PointerBufferSize(pStubMsg, *(unsigned char**)pMemory, pPointer);
        pStubMsg->PointerLength = pStubMsg->BufferLength;
        pStubMsg->BufferLength = saved_buffer_length;
      }
      pStubMsg->BufferLength += 4;
      pPointer += 4;
      pMemory += 4;
      break;
    case RPC_FC_ALIGNM4:
      ALIGN_POINTER(pMemory, 4);
      break;
    case RPC_FC_ALIGNM8:
      ALIGN_POINTER(pMemory, 8);
      break;
    case RPC_FC_STRUCTPAD1:
    case RPC_FC_STRUCTPAD2:
    case RPC_FC_STRUCTPAD3:
    case RPC_FC_STRUCTPAD4:
    case RPC_FC_STRUCTPAD5:
    case RPC_FC_STRUCTPAD6:
    case RPC_FC_STRUCTPAD7:
      pMemory += *pFormat - RPC_FC_STRUCTPAD1 + 1;
      break;
    case RPC_FC_EMBEDDED_COMPLEX:
      pMemory += pFormat[1];
      pFormat += 2;
      desc = pFormat + *(const SHORT*)pFormat;
      size = EmbeddedComplexSize(pStubMsg, desc);
      m = NdrBufferSizer[*desc & NDR_TABLE_MASK];
      if (m)
      {
        /* for some reason interface pointers aren't generated as
         * RPC_FC_POINTER, but instead as RPC_FC_EMBEDDED_COMPLEX, yet
         * they still need the derefencing treatment that pointers are
         * given */
        if (*desc == RPC_FC_IP)
          m(pStubMsg, *(unsigned char **)pMemory, desc);
        else
          m(pStubMsg, pMemory, desc);
      }
      else FIXME("no buffersizer for embedded type %02x\n", *desc);
      pMemory += size;
      pFormat += 2;
      continue;
    case RPC_FC_PAD:
      break;
    default:
      FIXME("unhandled format 0x%02x\n", *pFormat);
    }
    pFormat++;
  }

  return pMemory;
}

static unsigned char * ComplexFree(PMIDL_STUB_MESSAGE pStubMsg,
                                   unsigned char *pMemory,
                                   PFORMAT_STRING pFormat,
                                   PFORMAT_STRING pPointer)
{
  PFORMAT_STRING desc;
  NDR_FREE m;
  unsigned long size;

  while (*pFormat != RPC_FC_END) {
    switch (*pFormat) {
    case RPC_FC_BYTE:
    case RPC_FC_CHAR:
    case RPC_FC_SMALL:
    case RPC_FC_USMALL:
      pMemory += 1;
      break;
    case RPC_FC_WCHAR:
    case RPC_FC_SHORT:
    case RPC_FC_USHORT:
      pMemory += 2;
      break;
    case RPC_FC_LONG:
    case RPC_FC_ULONG:
    case RPC_FC_ENUM32:
      pMemory += 4;
      break;
    case RPC_FC_HYPER:
      pMemory += 8;
      break;
    case RPC_FC_POINTER:
      NdrPointerFree(pStubMsg, *(unsigned char**)pMemory, pPointer);
      pPointer += 4;
      pMemory += 4;
      break;
    case RPC_FC_ALIGNM4:
      ALIGN_POINTER(pMemory, 4);
      break;
    case RPC_FC_ALIGNM8:
      ALIGN_POINTER(pMemory, 8);
      break;
    case RPC_FC_STRUCTPAD1:
    case RPC_FC_STRUCTPAD2:
    case RPC_FC_STRUCTPAD3:
    case RPC_FC_STRUCTPAD4:
    case RPC_FC_STRUCTPAD5:
    case RPC_FC_STRUCTPAD6:
    case RPC_FC_STRUCTPAD7:
      pMemory += *pFormat - RPC_FC_STRUCTPAD1 + 1;
      break;
    case RPC_FC_EMBEDDED_COMPLEX:
      pMemory += pFormat[1];
      pFormat += 2;
      desc = pFormat + *(const SHORT*)pFormat;
      size = EmbeddedComplexSize(pStubMsg, desc);
      m = NdrFreer[*desc & NDR_TABLE_MASK];
      if (m)
      {
        /* for some reason interface pointers aren't generated as
         * RPC_FC_POINTER, but instead as RPC_FC_EMBEDDED_COMPLEX, yet
         * they still need the derefencing treatment that pointers are
         * given */
        if (*desc == RPC_FC_IP)
          m(pStubMsg, *(unsigned char **)pMemory, desc);
        else
          m(pStubMsg, pMemory, desc);
      }
      else FIXME("no freer for embedded type %02x\n", *desc);
      pMemory += size;
      pFormat += 2;
      continue;
    case RPC_FC_PAD:
      break;
    default:
      FIXME("unhandled format 0x%02x\n", *pFormat);
    }
    pFormat++;
  }

  return pMemory;
}

static unsigned long ComplexStructMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                       PFORMAT_STRING pFormat)
{
  PFORMAT_STRING desc;
  unsigned long size = 0;

  while (*pFormat != RPC_FC_END) {
    switch (*pFormat) {
    case RPC_FC_BYTE:
    case RPC_FC_CHAR:
    case RPC_FC_SMALL:
    case RPC_FC_USMALL:
      size += 1;
      pStubMsg->Buffer += 1;
      break;
    case RPC_FC_WCHAR:
    case RPC_FC_SHORT:
    case RPC_FC_USHORT:
      size += 2;
      pStubMsg->Buffer += 2;
      break;
    case RPC_FC_LONG:
    case RPC_FC_ULONG:
    case RPC_FC_ENUM32:
      size += 4;
      pStubMsg->Buffer += 4;
      break;
    case RPC_FC_HYPER:
      size += 8;
      pStubMsg->Buffer += 8;
      break;
    case RPC_FC_POINTER:
      size += 4;
      pStubMsg->Buffer += 4;
      if (!pStubMsg->IgnoreEmbeddedPointers)
        FIXME("embedded pointers\n");
      break;
    case RPC_FC_ALIGNM4:
      ALIGN_LENGTH(size, 4);
      ALIGN_POINTER(pStubMsg->Buffer, 4);
      break;
    case RPC_FC_ALIGNM8:
      ALIGN_LENGTH(size, 8);
      ALIGN_POINTER(pStubMsg->Buffer, 8);
      break;
    case RPC_FC_STRUCTPAD1:
    case RPC_FC_STRUCTPAD2:
    case RPC_FC_STRUCTPAD3:
    case RPC_FC_STRUCTPAD4:
    case RPC_FC_STRUCTPAD5:
    case RPC_FC_STRUCTPAD6:
    case RPC_FC_STRUCTPAD7:
      size += *pFormat - RPC_FC_STRUCTPAD1 + 1;
      break;
    case RPC_FC_EMBEDDED_COMPLEX:
      size += pFormat[1];
      pFormat += 2;
      desc = pFormat + *(const SHORT*)pFormat;
      size += EmbeddedComplexMemorySize(pStubMsg, desc);
      pFormat += 2;
      continue;
    case RPC_FC_PAD:
      break;
    default:
      FIXME("unhandled format 0x%02x\n", *pFormat);
    }
    pFormat++;
  }

  return size;
}

/***********************************************************************
 *           NdrComplexStructMarshall [RPCRT4.@]
 */
unsigned char * WINAPI NdrComplexStructMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                                unsigned char *pMemory,
                                                PFORMAT_STRING pFormat)
{
  PFORMAT_STRING conf_array = NULL;
  PFORMAT_STRING pointer_desc = NULL;
  unsigned char *OldMemory = pStubMsg->Memory;
  int pointer_buffer_mark_set = 0;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  if (!pStubMsg->PointerBufferMark)
  {
    int saved_ignore_embedded = pStubMsg->IgnoreEmbeddedPointers;
    /* save buffer length */
    unsigned long saved_buffer_length = pStubMsg->BufferLength;

    /* get the buffer pointer after complex array data, but before
     * pointer data */
    pStubMsg->BufferLength = pStubMsg->Buffer - pStubMsg->BufferStart;
    pStubMsg->IgnoreEmbeddedPointers = 1;
    NdrComplexStructBufferSize(pStubMsg, pMemory, pFormat);
    pStubMsg->IgnoreEmbeddedPointers = saved_ignore_embedded;

    /* save it for use by embedded pointer code later */
    pStubMsg->PointerBufferMark = pStubMsg->BufferStart + pStubMsg->BufferLength;
    TRACE("difference = 0x%x\n", pStubMsg->PointerBufferMark - pStubMsg->Buffer);
    pointer_buffer_mark_set = 1;

    /* restore the original buffer length */
    pStubMsg->BufferLength = saved_buffer_length;
  }

  ALIGN_POINTER(pStubMsg->Buffer, pFormat[1] + 1);

  pFormat += 4;
  if (*(const WORD*)pFormat) conf_array = pFormat + *(const WORD*)pFormat;
  pFormat += 2;
  if (*(const WORD*)pFormat) pointer_desc = pFormat + *(const WORD*)pFormat;
  pFormat += 2;

  pStubMsg->Memory = pMemory;

  ComplexMarshall(pStubMsg, pMemory, pFormat, pointer_desc);

  if (conf_array)
    NdrConformantArrayMarshall(pStubMsg, pMemory, conf_array);

  pStubMsg->Memory = OldMemory;

  if (pointer_buffer_mark_set)
  {
    pStubMsg->Buffer = pStubMsg->PointerBufferMark;
    pStubMsg->PointerBufferMark = NULL;
  }

  STD_OVERFLOW_CHECK(pStubMsg);

  return NULL;
}

/***********************************************************************
 *           NdrComplexStructUnmarshall [RPCRT4.@]
 */
unsigned char * WINAPI NdrComplexStructUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                                  unsigned char **ppMemory,
                                                  PFORMAT_STRING pFormat,
                                                  unsigned char fMustAlloc)
{
  unsigned size = *(const WORD*)(pFormat+2);
  PFORMAT_STRING conf_array = NULL;
  PFORMAT_STRING pointer_desc = NULL;
  unsigned char *pMemory;
  int pointer_buffer_mark_set = 0;

  TRACE("(%p,%p,%p,%d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);

  if (!pStubMsg->PointerBufferMark)
  {
    int saved_ignore_embedded = pStubMsg->IgnoreEmbeddedPointers;
    /* save buffer pointer */
    unsigned char *saved_buffer = pStubMsg->Buffer;

    /* get the buffer pointer after complex array data, but before
     * pointer data */
    pStubMsg->IgnoreEmbeddedPointers = 1;
    NdrComplexStructMemorySize(pStubMsg, pFormat);
    pStubMsg->IgnoreEmbeddedPointers = saved_ignore_embedded;

    /* save it for use by embedded pointer code later */
    pStubMsg->PointerBufferMark = pStubMsg->Buffer;
    TRACE("difference = 0x%lx\n", (unsigned long)(pStubMsg->PointerBufferMark - saved_buffer));
    pointer_buffer_mark_set = 1;

    /* restore the original buffer */
    pStubMsg->Buffer = saved_buffer;
  }

  ALIGN_POINTER(pStubMsg->Buffer, pFormat[1] + 1);

  if (fMustAlloc || !*ppMemory)
  {
    *ppMemory = NdrAllocate(pStubMsg, size);
    memset(*ppMemory, 0, size);
  }

  pFormat += 4;
  if (*(const WORD*)pFormat) conf_array = pFormat + *(const WORD*)pFormat;
  pFormat += 2;
  if (*(const WORD*)pFormat) pointer_desc = pFormat + *(const WORD*)pFormat;
  pFormat += 2;

  pMemory = ComplexUnmarshall(pStubMsg, *ppMemory, pFormat, pointer_desc);

  if (conf_array)
    NdrConformantArrayUnmarshall(pStubMsg, &pMemory, conf_array, fMustAlloc);

  if (pointer_buffer_mark_set)
  {
    pStubMsg->Buffer = pStubMsg->PointerBufferMark;
    pStubMsg->PointerBufferMark = NULL;
  }

  return NULL;
}

/***********************************************************************
 *           NdrComplexStructBufferSize [RPCRT4.@]
 */
void WINAPI NdrComplexStructBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                       unsigned char *pMemory,
                                       PFORMAT_STRING pFormat)
{
  PFORMAT_STRING conf_array = NULL;
  PFORMAT_STRING pointer_desc = NULL;
  unsigned char *OldMemory = pStubMsg->Memory;
  int pointer_length_set = 0;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  ALIGN_LENGTH(pStubMsg->BufferLength, pFormat[1] + 1);

  if(!pStubMsg->IgnoreEmbeddedPointers && !pStubMsg->PointerLength)
  {
    int saved_ignore_embedded = pStubMsg->IgnoreEmbeddedPointers;
    unsigned long saved_buffer_length = pStubMsg->BufferLength;

    /* get the buffer length after complex struct data, but before
     * pointer data */
    pStubMsg->IgnoreEmbeddedPointers = 1;
    NdrComplexStructBufferSize(pStubMsg, pMemory, pFormat);
    pStubMsg->IgnoreEmbeddedPointers = saved_ignore_embedded;

    /* save it for use by embedded pointer code later */
    pStubMsg->PointerLength = pStubMsg->BufferLength;
    pointer_length_set = 1;
    TRACE("difference = 0x%lx\n", pStubMsg->PointerLength - saved_buffer_length);

    /* restore the original buffer length */
    pStubMsg->BufferLength = saved_buffer_length;
  }

  pFormat += 4;
  if (*(const WORD*)pFormat) conf_array = pFormat + *(const WORD*)pFormat;
  pFormat += 2;
  if (*(const WORD*)pFormat) pointer_desc = pFormat + *(const WORD*)pFormat;
  pFormat += 2;

  pStubMsg->Memory = pMemory;

  pMemory = ComplexBufferSize(pStubMsg, pMemory, pFormat, pointer_desc);

  if (conf_array)
    NdrConformantArrayBufferSize(pStubMsg, pMemory, conf_array);

  pStubMsg->Memory = OldMemory;

  if(pointer_length_set)
  {
    pStubMsg->BufferLength = pStubMsg->PointerLength;
    pStubMsg->PointerLength = 0;
  }

}

/***********************************************************************
 *           NdrComplexStructMemorySize [RPCRT4.@]
 */
ULONG WINAPI NdrComplexStructMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                        PFORMAT_STRING pFormat)
{
  unsigned size = *(const WORD*)(pFormat+2);
  PFORMAT_STRING conf_array = NULL;
  PFORMAT_STRING pointer_desc = NULL;

  TRACE("(%p,%p)\n", pStubMsg, pFormat);

  ALIGN_POINTER(pStubMsg->Buffer, pFormat[1] + 1);

  pFormat += 4;
  if (*(const WORD*)pFormat) conf_array = pFormat + *(const WORD*)pFormat;
  pFormat += 2;
  if (*(const WORD*)pFormat) pointer_desc = pFormat + *(const WORD*)pFormat;
  pFormat += 2;

  ComplexStructMemorySize(pStubMsg, pFormat);

  if (conf_array)
    NdrConformantArrayMemorySize(pStubMsg, conf_array);

  return size;
}

/***********************************************************************
 *           NdrComplexStructFree [RPCRT4.@]
 */
void WINAPI NdrComplexStructFree(PMIDL_STUB_MESSAGE pStubMsg,
                                 unsigned char *pMemory,
                                 PFORMAT_STRING pFormat)
{
  PFORMAT_STRING conf_array = NULL;
  PFORMAT_STRING pointer_desc = NULL;
  unsigned char *OldMemory = pStubMsg->Memory;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  pFormat += 4;
  if (*(const WORD*)pFormat) conf_array = pFormat + *(const WORD*)pFormat;
  pFormat += 2;
  if (*(const WORD*)pFormat) pointer_desc = pFormat + *(const WORD*)pFormat;
  pFormat += 2;

  pStubMsg->Memory = pMemory;

  pMemory = ComplexFree(pStubMsg, pMemory, pFormat, pointer_desc);

  if (conf_array)
    NdrConformantArrayFree(pStubMsg, pMemory, conf_array);

  pStubMsg->Memory = OldMemory;
}

/***********************************************************************
 *           NdrConformantArrayMarshall [RPCRT4.@]
 */
unsigned char * WINAPI NdrConformantArrayMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                                  unsigned char *pMemory,
                                                  PFORMAT_STRING pFormat)
{
  DWORD size = 0, esize = *(const WORD*)(pFormat+2);
  unsigned char alignment = pFormat[1] + 1;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  if (pFormat[0] != RPC_FC_CARRAY) FIXME("format=%d\n", pFormat[0]);

  pFormat = ComputeConformance(pStubMsg, pMemory, pFormat+4, 0);

  WriteConformance(pStubMsg);

  ALIGN_POINTER(pStubMsg->Buffer, alignment);

  size = safe_multiply(esize, pStubMsg->MaxCount);
  memcpy(pStubMsg->Buffer, pMemory, size);
  pStubMsg->BufferMark = pStubMsg->Buffer;
  pStubMsg->Buffer += size;

  EmbeddedPointerMarshall(pStubMsg, pMemory, pFormat);

  STD_OVERFLOW_CHECK(pStubMsg);

  return NULL;
}

/***********************************************************************
 *           NdrConformantArrayUnmarshall [RPCRT4.@]
 */
unsigned char * WINAPI NdrConformantArrayUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                                    unsigned char **ppMemory,
                                                    PFORMAT_STRING pFormat,
                                                    unsigned char fMustAlloc)
{
  DWORD size, esize = *(const WORD*)(pFormat+2);
  unsigned char alignment = pFormat[1] + 1;

  TRACE("(%p,%p,%p,%d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);
  if (pFormat[0] != RPC_FC_CARRAY) FIXME("format=%d\n", pFormat[0]);

  pFormat = ReadConformance(pStubMsg, pFormat+4);

  size = safe_multiply(esize, pStubMsg->MaxCount);

  if (fMustAlloc || !*ppMemory)
    *ppMemory = NdrAllocate(pStubMsg, size);

  ALIGN_POINTER(pStubMsg->Buffer, alignment);

  memcpy(*ppMemory, pStubMsg->Buffer, size);

  pStubMsg->BufferMark = pStubMsg->Buffer;
  pStubMsg->Buffer += size;

  EmbeddedPointerUnmarshall(pStubMsg, ppMemory, pFormat, fMustAlloc);

  return NULL;
}

/***********************************************************************
 *           NdrConformantArrayBufferSize [RPCRT4.@]
 */
void WINAPI NdrConformantArrayBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                         unsigned char *pMemory,
                                         PFORMAT_STRING pFormat)
{
  DWORD size, esize = *(const WORD*)(pFormat+2);
  unsigned char alignment = pFormat[1] + 1;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  if (pFormat[0] != RPC_FC_CARRAY) FIXME("format=%d\n", pFormat[0]);

  pFormat = ComputeConformance(pStubMsg, pMemory, pFormat+4, 0);

  SizeConformance(pStubMsg);

  ALIGN_LENGTH(pStubMsg->BufferLength, alignment);

  size = safe_multiply(esize, pStubMsg->MaxCount);
  /* conformance value plus array */
  pStubMsg->BufferLength += size;

  EmbeddedPointerBufferSize(pStubMsg, pMemory, pFormat);
}

/***********************************************************************
 *           NdrConformantArrayMemorySize [RPCRT4.@]
 */
ULONG WINAPI NdrConformantArrayMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                          PFORMAT_STRING pFormat)
{
  DWORD size = 0, esize = *(const WORD*)(pFormat+2);
  unsigned char alignment = pFormat[1] + 1;

  TRACE("(%p,%p)\n", pStubMsg, pFormat);
  if (pFormat[0] != RPC_FC_CARRAY) FIXME("format=%d\n", pFormat[0]);

  pFormat = ReadConformance(pStubMsg, pFormat+4);
  size = safe_multiply(esize, pStubMsg->MaxCount);
  pStubMsg->MemorySize += size;

  ALIGN_POINTER(pStubMsg->Buffer, alignment);
  pStubMsg->BufferMark = pStubMsg->Buffer;
  pStubMsg->Buffer += size;

  EmbeddedPointerMemorySize(pStubMsg, pFormat);

  return pStubMsg->MemorySize;
}

/***********************************************************************
 *           NdrConformantArrayFree [RPCRT4.@]
 */
void WINAPI NdrConformantArrayFree(PMIDL_STUB_MESSAGE pStubMsg,
                                   unsigned char *pMemory,
                                   PFORMAT_STRING pFormat)
{
  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  if (pFormat[0] != RPC_FC_CARRAY) FIXME("format=%d\n", pFormat[0]);

  pFormat = ComputeConformance(pStubMsg, pMemory, pFormat+4, 0);

  EmbeddedPointerFree(pStubMsg, pMemory, pFormat);
}


/***********************************************************************
 *           NdrConformantVaryingArrayMarshall  [RPCRT4.@]
 */
unsigned char* WINAPI NdrConformantVaryingArrayMarshall( PMIDL_STUB_MESSAGE pStubMsg,
                                                         unsigned char* pMemory,
                                                         PFORMAT_STRING pFormat )
{
    ULONG bufsize;
    unsigned char alignment = pFormat[1] + 1;
    DWORD esize = *(const WORD*)(pFormat+2);

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);

    if (pFormat[0] != RPC_FC_CVARRAY)
    {
        ERR("invalid format type %x\n", pFormat[0]);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }

    pFormat = ComputeConformance(pStubMsg, pMemory, pFormat+4, 0);
    pFormat = ComputeVariance(pStubMsg, pMemory, pFormat, 0);

    WriteConformance(pStubMsg);
    WriteVariance(pStubMsg);

    ALIGN_POINTER(pStubMsg->Buffer, alignment);

    bufsize = safe_multiply(esize, pStubMsg->ActualCount);

    memcpy(pStubMsg->Buffer, pMemory + pStubMsg->Offset, bufsize);
    pStubMsg->BufferMark = pStubMsg->Buffer;
    pStubMsg->Buffer += bufsize;

    EmbeddedPointerMarshall(pStubMsg, pMemory, pFormat);

    STD_OVERFLOW_CHECK(pStubMsg);

    return NULL;
}


/***********************************************************************
 *           NdrConformantVaryingArrayUnmarshall  [RPCRT4.@]
 */
unsigned char* WINAPI NdrConformantVaryingArrayUnmarshall( PMIDL_STUB_MESSAGE pStubMsg,
                                                           unsigned char** ppMemory,
                                                           PFORMAT_STRING pFormat,
                                                           unsigned char fMustAlloc )
{
    ULONG bufsize, memsize;
    unsigned char alignment = pFormat[1] + 1;
    DWORD esize = *(const WORD*)(pFormat+2);

    TRACE("(%p, %p, %p, %d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);

    if (pFormat[0] != RPC_FC_CVARRAY)
    {
        ERR("invalid format type %x\n", pFormat[0]);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }

    pFormat = ReadConformance(pStubMsg, pFormat+4);
    pFormat = ReadVariance(pStubMsg, pFormat, pStubMsg->MaxCount);

    ALIGN_POINTER(pStubMsg->Buffer, alignment);

    bufsize = safe_multiply(esize, pStubMsg->ActualCount);
    memsize = safe_multiply(esize, pStubMsg->MaxCount);

    if (!*ppMemory || fMustAlloc)
        *ppMemory = NdrAllocate(pStubMsg, memsize);
    memcpy(*ppMemory + pStubMsg->Offset, pStubMsg->Buffer, bufsize);
    pStubMsg->Buffer += bufsize;

    EmbeddedPointerUnmarshall(pStubMsg, ppMemory, pFormat, fMustAlloc);

    return NULL;
}


/***********************************************************************
 *           NdrConformantVaryingArrayFree  [RPCRT4.@]
 */
void WINAPI NdrConformantVaryingArrayFree( PMIDL_STUB_MESSAGE pStubMsg,
                                           unsigned char* pMemory,
                                           PFORMAT_STRING pFormat )
{
    TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

    if (pFormat[0] != RPC_FC_CVARRAY)
    {
        ERR("invalid format type %x\n", pFormat[0]);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }

    pFormat = ComputeConformance(pStubMsg, pMemory, pFormat+4, 0);
    pFormat = ComputeVariance(pStubMsg, pMemory, pFormat, 0);

    EmbeddedPointerFree(pStubMsg, pMemory, pFormat);
}


/***********************************************************************
 *           NdrConformantVaryingArrayBufferSize  [RPCRT4.@]
 */
void WINAPI NdrConformantVaryingArrayBufferSize( PMIDL_STUB_MESSAGE pStubMsg,
                                                 unsigned char* pMemory, PFORMAT_STRING pFormat )
{
    unsigned char alignment = pFormat[1] + 1;
    DWORD esize = *(const WORD*)(pFormat+2);

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);

    if (pFormat[0] != RPC_FC_CVARRAY)
    {
        ERR("invalid format type %x\n", pFormat[0]);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }

    /* compute size */
    pFormat = ComputeConformance(pStubMsg, pMemory, pFormat+4, 0);
    /* compute length */
    pFormat = ComputeVariance(pStubMsg, pMemory, pFormat, 0);

    SizeConformance(pStubMsg);
    SizeVariance(pStubMsg);

    ALIGN_LENGTH(pStubMsg->BufferLength, alignment);

    pStubMsg->BufferLength += safe_multiply(esize, pStubMsg->ActualCount);

    EmbeddedPointerBufferSize(pStubMsg, pMemory, pFormat);
}


/***********************************************************************
 *           NdrConformantVaryingArrayMemorySize  [RPCRT4.@]
 */
ULONG WINAPI NdrConformantVaryingArrayMemorySize( PMIDL_STUB_MESSAGE pStubMsg,
                                                  PFORMAT_STRING pFormat )
{
    FIXME( "stub\n" );
    return 0;
}


/***********************************************************************
 *           NdrComplexArrayMarshall [RPCRT4.@]
 */
unsigned char * WINAPI NdrComplexArrayMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                               unsigned char *pMemory,
                                               PFORMAT_STRING pFormat)
{
  ULONG i, count, def;
  BOOL variance_present;
  unsigned char alignment;
  int pointer_buffer_mark_set = 0;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  if (pFormat[0] != RPC_FC_BOGUS_ARRAY)
  {
      ERR("invalid format type %x\n", pFormat[0]);
      RpcRaiseException(RPC_S_INTERNAL_ERROR);
      return NULL;
  }

  alignment = pFormat[1] + 1;

  if (!pStubMsg->PointerBufferMark)
  {
    /* save buffer fields that may be changed by buffer sizer functions
     * and that may be needed later on */
    int saved_ignore_embedded = pStubMsg->IgnoreEmbeddedPointers;
    unsigned long saved_buffer_length = pStubMsg->BufferLength;
    unsigned long saved_max_count = pStubMsg->MaxCount;
    unsigned long saved_offset = pStubMsg->Offset;
    unsigned long saved_actual_count = pStubMsg->ActualCount;

    /* get the buffer pointer after complex array data, but before
     * pointer data */
    pStubMsg->BufferLength = pStubMsg->Buffer - pStubMsg->BufferStart;
    pStubMsg->IgnoreEmbeddedPointers = 1;
    NdrComplexArrayBufferSize(pStubMsg, pMemory, pFormat);
    pStubMsg->IgnoreEmbeddedPointers = saved_ignore_embedded;

    /* save it for use by embedded pointer code later */
    pStubMsg->PointerBufferMark = pStubMsg->BufferStart + pStubMsg->BufferLength;
    TRACE("difference = 0x%x\n", pStubMsg->Buffer - pStubMsg->BufferStart);
    pointer_buffer_mark_set = 1;

    /* restore fields */
    pStubMsg->ActualCount = saved_actual_count;
    pStubMsg->Offset = saved_offset;
    pStubMsg->MaxCount = saved_max_count;
    pStubMsg->BufferLength = saved_buffer_length;
  }

  def = *(const WORD*)&pFormat[2];
  pFormat += 4;

  pFormat = ComputeConformance(pStubMsg, pMemory, pFormat, def);
  TRACE("conformance = %ld\n", pStubMsg->MaxCount);

  variance_present = IsConformanceOrVariancePresent(pFormat);
  pFormat = ComputeVariance(pStubMsg, pMemory, pFormat, pStubMsg->MaxCount);
  TRACE("variance = %d\n", pStubMsg->ActualCount);

  WriteConformance(pStubMsg);
  if (variance_present)
    WriteVariance(pStubMsg);

  ALIGN_POINTER(pStubMsg->Buffer, alignment);

  count = pStubMsg->ActualCount;
  for (i = 0; i < count; i++)
    pMemory = ComplexMarshall(pStubMsg, pMemory, pFormat, NULL);

  STD_OVERFLOW_CHECK(pStubMsg);

  if (pointer_buffer_mark_set)
  {
    pStubMsg->Buffer = pStubMsg->PointerBufferMark;
    pStubMsg->PointerBufferMark = NULL;
  }

  return NULL;
}

/***********************************************************************
 *           NdrComplexArrayUnmarshall [RPCRT4.@]
 */
unsigned char * WINAPI NdrComplexArrayUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                                 unsigned char **ppMemory,
                                                 PFORMAT_STRING pFormat,
                                                 unsigned char fMustAlloc)
{
  ULONG i, count, size;
  unsigned char alignment;
  unsigned char *pMemory;
  unsigned char *saved_buffer;
  int pointer_buffer_mark_set = 0;
  int saved_ignore_embedded;

  TRACE("(%p,%p,%p,%d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);

  if (pFormat[0] != RPC_FC_BOGUS_ARRAY)
  {
      ERR("invalid format type %x\n", pFormat[0]);
      RpcRaiseException(RPC_S_INTERNAL_ERROR);
      return NULL;
  }

  alignment = pFormat[1] + 1;

  saved_ignore_embedded = pStubMsg->IgnoreEmbeddedPointers;
  /* save buffer pointer */
  saved_buffer = pStubMsg->Buffer;
  /* get the buffer pointer after complex array data, but before
   * pointer data */
  pStubMsg->IgnoreEmbeddedPointers = 1;
  pStubMsg->MemorySize = 0;
  NdrComplexArrayMemorySize(pStubMsg, pFormat);
  size = pStubMsg->MemorySize;
  pStubMsg->IgnoreEmbeddedPointers = saved_ignore_embedded;

  TRACE("difference = 0x%lx\n", (unsigned long)(pStubMsg->Buffer - saved_buffer));
  if (!pStubMsg->PointerBufferMark)
  {
    /* save it for use by embedded pointer code later */
    pStubMsg->PointerBufferMark = pStubMsg->Buffer;
    pointer_buffer_mark_set = 1;
  }
  /* restore the original buffer */
  pStubMsg->Buffer = saved_buffer;

  pFormat += 4;

  pFormat = ReadConformance(pStubMsg, pFormat);
  pFormat = ReadVariance(pStubMsg, pFormat, pStubMsg->MaxCount);

  if (fMustAlloc || !*ppMemory)
  {
    *ppMemory = NdrAllocate(pStubMsg, size);
    memset(*ppMemory, 0, size);
  }

  ALIGN_POINTER(pStubMsg->Buffer, alignment);

  pMemory = *ppMemory;
  count = pStubMsg->ActualCount;
  for (i = 0; i < count; i++)
    pMemory = ComplexUnmarshall(pStubMsg, pMemory, pFormat, NULL);

  if (pointer_buffer_mark_set)
  {
    pStubMsg->Buffer = pStubMsg->PointerBufferMark;
    pStubMsg->PointerBufferMark = NULL;
  }

  return NULL;
}

/***********************************************************************
 *           NdrComplexArrayBufferSize [RPCRT4.@]
 */
void WINAPI NdrComplexArrayBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                      unsigned char *pMemory,
                                      PFORMAT_STRING pFormat)
{
  ULONG i, count, def;
  unsigned char alignment;
  BOOL variance_present;
  int pointer_length_set = 0;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  if (pFormat[0] != RPC_FC_BOGUS_ARRAY)
  {
      ERR("invalid format type %x\n", pFormat[0]);
      RpcRaiseException(RPC_S_INTERNAL_ERROR);
      return;
  }

  alignment = pFormat[1] + 1;

  if (!pStubMsg->IgnoreEmbeddedPointers && !pStubMsg->PointerLength)
  {
    /* save buffer fields that may be changed by buffer sizer functions
     * and that may be needed later on */
    int saved_ignore_embedded = pStubMsg->IgnoreEmbeddedPointers;
    unsigned long saved_buffer_length = pStubMsg->BufferLength;
    unsigned long saved_max_count = pStubMsg->MaxCount;
    unsigned long saved_offset = pStubMsg->Offset;
    unsigned long saved_actual_count = pStubMsg->ActualCount;

    /* get the buffer pointer after complex array data, but before
     * pointer data */
    pStubMsg->IgnoreEmbeddedPointers = 1;
    NdrComplexArrayBufferSize(pStubMsg, pMemory, pFormat);
    pStubMsg->IgnoreEmbeddedPointers = saved_ignore_embedded;

    /* save it for use by embedded pointer code later */
    pStubMsg->PointerLength = pStubMsg->BufferLength;
    pointer_length_set = 1;

    /* restore fields */
    pStubMsg->ActualCount = saved_actual_count;
    pStubMsg->Offset = saved_offset;
    pStubMsg->MaxCount = saved_max_count;
    pStubMsg->BufferLength = saved_buffer_length;
  }
  def = *(const WORD*)&pFormat[2];
  pFormat += 4;

  pFormat = ComputeConformance(pStubMsg, pMemory, pFormat, def);
  TRACE("conformance = %ld\n", pStubMsg->MaxCount);
  SizeConformance(pStubMsg);

  variance_present = IsConformanceOrVariancePresent(pFormat);
  pFormat = ComputeVariance(pStubMsg, pMemory, pFormat, pStubMsg->MaxCount);
  TRACE("variance = %d\n", pStubMsg->ActualCount);

  if (variance_present)
    SizeVariance(pStubMsg);

  ALIGN_LENGTH(pStubMsg->BufferLength, alignment);

  count = pStubMsg->ActualCount;
  for (i = 0; i < count; i++)
    pMemory = ComplexBufferSize(pStubMsg, pMemory, pFormat, NULL);

  if(pointer_length_set)
  {
    pStubMsg->BufferLength = pStubMsg->PointerLength;
    pStubMsg->PointerLength = 0;
  }
}

/***********************************************************************
 *           NdrComplexArrayMemorySize [RPCRT4.@]
 */
ULONG WINAPI NdrComplexArrayMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                       PFORMAT_STRING pFormat)
{
  ULONG i, count, esize, SavedMemorySize, MemorySize;
  unsigned char alignment;
  unsigned char *Buffer;

  TRACE("(%p,%p)\n", pStubMsg, pFormat);

  if (pFormat[0] != RPC_FC_BOGUS_ARRAY)
  {
      ERR("invalid format type %x\n", pFormat[0]);
      RpcRaiseException(RPC_S_INTERNAL_ERROR);
      return 0;
  }

  alignment = pFormat[1] + 1;

  pFormat += 4;

  pFormat = ReadConformance(pStubMsg, pFormat);
  pFormat = ReadVariance(pStubMsg, pFormat, pStubMsg->MaxCount);

  ALIGN_POINTER(pStubMsg->Buffer, alignment);

  SavedMemorySize = pStubMsg->MemorySize;

  Buffer = pStubMsg->Buffer;
  pStubMsg->MemorySize = 0;
  esize = ComplexStructMemorySize(pStubMsg, pFormat);
  pStubMsg->Buffer = Buffer;

  MemorySize = safe_multiply(pStubMsg->MaxCount, esize);

  count = pStubMsg->ActualCount;
  for (i = 0; i < count; i++)
    ComplexStructMemorySize(pStubMsg, pFormat);

  pStubMsg->MemorySize = SavedMemorySize;

  pStubMsg->MemorySize += MemorySize;
  return MemorySize;
}

/***********************************************************************
 *           NdrComplexArrayFree [RPCRT4.@]
 */
void WINAPI NdrComplexArrayFree(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
  ULONG i, count, def;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  if (pFormat[0] != RPC_FC_BOGUS_ARRAY)
  {
      ERR("invalid format type %x\n", pFormat[0]);
      RpcRaiseException(RPC_S_INTERNAL_ERROR);
      return;
  }

  def = *(const WORD*)&pFormat[2];
  pFormat += 4;

  pFormat = ComputeConformance(pStubMsg, pMemory, pFormat, def);
  TRACE("conformance = %ld\n", pStubMsg->MaxCount);

  pFormat = ComputeVariance(pStubMsg, pMemory, pFormat, pStubMsg->MaxCount);
  TRACE("variance = %d\n", pStubMsg->ActualCount);

  count = pStubMsg->ActualCount;
  for (i = 0; i < count; i++)
    pMemory = ComplexFree(pStubMsg, pMemory, pFormat, NULL);
}

static ULONG UserMarshalFlags(PMIDL_STUB_MESSAGE pStubMsg)
{
  return MAKELONG(pStubMsg->dwDestContext,
                  pStubMsg->RpcMsg->DataRepresentation);
}

#define USER_MARSHAL_PTR_PREFIX \
        ( (DWORD)'U'         | ( (DWORD)'s' << 8 ) | \
        ( (DWORD)'e' << 16 ) | ( (DWORD)'r' << 24 ) )

/***********************************************************************
 *           NdrUserMarshalMarshall [RPCRT4.@]
 */
unsigned char * WINAPI NdrUserMarshalMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                              unsigned char *pMemory,
                                              PFORMAT_STRING pFormat)
{
  unsigned flags = pFormat[1];
  unsigned index = *(const WORD*)&pFormat[2];
  unsigned char *saved_buffer = NULL;
  ULONG uflag = UserMarshalFlags(pStubMsg);
  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  TRACE("index=%d\n", index);

  if (flags & USER_MARSHAL_POINTER)
  {
    ALIGN_POINTER(pStubMsg->Buffer, 4);
    NDR_LOCAL_UINT32_WRITE(pStubMsg->Buffer, USER_MARSHAL_PTR_PREFIX);
    pStubMsg->Buffer += 4;
    if (pStubMsg->PointerBufferMark)
    {
      saved_buffer = pStubMsg->Buffer;
      pStubMsg->Buffer = pStubMsg->PointerBufferMark;
      pStubMsg->PointerBufferMark = NULL;
    }
    ALIGN_POINTER(pStubMsg->Buffer, 8);
  }
  else
    ALIGN_POINTER(pStubMsg->Buffer, (flags & 0xf) + 1);

  pStubMsg->Buffer =
    pStubMsg->StubDesc->aUserMarshalQuadruple[index].pfnMarshall(
      &uflag, pStubMsg->Buffer, pMemory);

  if (saved_buffer)
  {
    STD_OVERFLOW_CHECK(pStubMsg);
    pStubMsg->PointerBufferMark = pStubMsg->Buffer;
    pStubMsg->Buffer = saved_buffer;
  }

  STD_OVERFLOW_CHECK(pStubMsg);

  return NULL;
}

/***********************************************************************
 *           NdrUserMarshalUnmarshall [RPCRT4.@]
 */
unsigned char * WINAPI NdrUserMarshalUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                                 unsigned char **ppMemory,
                                                 PFORMAT_STRING pFormat,
                                                 unsigned char fMustAlloc)
{
  unsigned flags = pFormat[1];
  unsigned index = *(const WORD*)&pFormat[2];
  DWORD memsize = *(const WORD*)&pFormat[4];
  unsigned char *saved_buffer = NULL;
  ULONG uflag = UserMarshalFlags(pStubMsg);
  TRACE("(%p,%p,%p,%d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);
  TRACE("index=%d\n", index);

  if (flags & USER_MARSHAL_POINTER)
  {
    ALIGN_POINTER(pStubMsg->Buffer, 4);
    /* skip pointer prefix */
    pStubMsg->Buffer += 4;
    if (pStubMsg->PointerBufferMark)
    {
      saved_buffer = pStubMsg->Buffer;
      pStubMsg->Buffer = pStubMsg->PointerBufferMark;
      pStubMsg->PointerBufferMark = NULL;
    }
    ALIGN_POINTER(pStubMsg->Buffer, 8);
  }
  else
    ALIGN_POINTER(pStubMsg->Buffer, (flags & 0xf) + 1);

  if (fMustAlloc || !*ppMemory)
    *ppMemory = NdrAllocate(pStubMsg, memsize);

  pStubMsg->Buffer =
    pStubMsg->StubDesc->aUserMarshalQuadruple[index].pfnUnmarshall(
      &uflag, pStubMsg->Buffer, *ppMemory);

  if (saved_buffer)
  {
    STD_OVERFLOW_CHECK(pStubMsg);
    pStubMsg->PointerBufferMark = pStubMsg->Buffer;
    pStubMsg->Buffer = saved_buffer;
  }

  return NULL;
}

/***********************************************************************
 *           NdrUserMarshalBufferSize [RPCRT4.@]
 */
void WINAPI NdrUserMarshalBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                      unsigned char *pMemory,
                                      PFORMAT_STRING pFormat)
{
  unsigned flags = pFormat[1];
  unsigned index = *(const WORD*)&pFormat[2];
  DWORD bufsize = *(const WORD*)&pFormat[6];
  ULONG uflag = UserMarshalFlags(pStubMsg);
  unsigned long saved_buffer_length = 0;
  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  TRACE("index=%d\n", index);

  if (flags & USER_MARSHAL_POINTER)
  {
    ALIGN_LENGTH(pStubMsg->BufferLength, 4);
    /* skip pointer prefix */
    pStubMsg->BufferLength += 4;
    if (pStubMsg->IgnoreEmbeddedPointers)
      return;
    if (pStubMsg->PointerLength)
    {
      saved_buffer_length = pStubMsg->BufferLength;
      pStubMsg->BufferLength = pStubMsg->PointerLength;
      pStubMsg->PointerLength = 0;
    }
    ALIGN_LENGTH(pStubMsg->BufferLength, 8);
  }
  else
    ALIGN_LENGTH(pStubMsg->BufferLength, (flags & 0xf) + 1);

  if (bufsize) {
    TRACE("size=%d\n", bufsize);
    pStubMsg->BufferLength += bufsize;
  }
  else
    pStubMsg->BufferLength =
        pStubMsg->StubDesc->aUserMarshalQuadruple[index].pfnBufferSize(
                             &uflag, pStubMsg->BufferLength, pMemory);

  if (saved_buffer_length)
  {
    pStubMsg->PointerLength = pStubMsg->BufferLength;
    pStubMsg->BufferLength = saved_buffer_length;
  }

}

/***********************************************************************
 *           NdrUserMarshalMemorySize [RPCRT4.@]
 */
ULONG WINAPI NdrUserMarshalMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                      PFORMAT_STRING pFormat)
{
  unsigned flags = pFormat[1];
  unsigned index = *(const WORD*)&pFormat[2];
  DWORD memsize = *(const WORD*)&pFormat[4];
  DWORD bufsize = *(const WORD*)&pFormat[6];

  TRACE("(%p,%p)\n", pStubMsg, pFormat);
  TRACE("index=%d\n", index);

  pStubMsg->MemorySize += memsize;

  if (flags & USER_MARSHAL_POINTER)
  {
    ALIGN_POINTER(pStubMsg->Buffer, 4);
    /* skip pointer prefix */
    pStubMsg->Buffer += 4;
    if (pStubMsg->IgnoreEmbeddedPointers)
      return pStubMsg->MemorySize;
    ALIGN_POINTER(pStubMsg->Buffer, 8);
  }
  else
    ALIGN_POINTER(pStubMsg->Buffer, (flags & 0xf) + 1);

  if (!bufsize)
    FIXME("not implemented for varying buffer size\n");

  pStubMsg->Buffer += bufsize;

  return pStubMsg->MemorySize;
}

/***********************************************************************
 *           NdrUserMarshalFree [RPCRT4.@]
 */
void WINAPI NdrUserMarshalFree(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
/*  unsigned flags = pFormat[1]; */
  unsigned index = *(const WORD*)&pFormat[2];
  ULONG uflag = UserMarshalFlags(pStubMsg);
  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  TRACE("index=%d\n", index);

  pStubMsg->StubDesc->aUserMarshalQuadruple[index].pfnFree(
    &uflag, pMemory);
}

/***********************************************************************
 *           NdrClearOutParameters [RPCRT4.@]
 */
void WINAPI NdrClearOutParameters(PMIDL_STUB_MESSAGE pStubMsg,
                                  PFORMAT_STRING pFormat,
                                  void *ArgAddr)
{
  FIXME("(%p,%p,%p): stub\n", pStubMsg, pFormat, ArgAddr);
}

/***********************************************************************
 *           NdrConvert [RPCRT4.@]
 */
void WINAPI NdrConvert( PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat )
{
  FIXME("(pStubMsg == ^%p, pFormat == ^%p): stub.\n", pStubMsg, pFormat);
  /* FIXME: since this stub doesn't do any converting, the proper behavior
     is to raise an exception */
}

/***********************************************************************
 *           NdrConvert2 [RPCRT4.@]
 */
void WINAPI NdrConvert2( PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat, LONG NumberParams )
{
  FIXME("(pStubMsg == ^%p, pFormat == ^%p, NumberParams == %d): stub.\n",
    pStubMsg, pFormat, NumberParams);
  /* FIXME: since this stub doesn't do any converting, the proper behavior
     is to raise an exception */
}

#include "pshpack1.h"
typedef struct _NDR_CSTRUCT_FORMAT
{
    unsigned char type;
    unsigned char alignment;
    unsigned short memory_size;
    short offset_to_array_description;
} NDR_CSTRUCT_FORMAT, NDR_CVSTRUCT_FORMAT;
#include "poppack.h"

/***********************************************************************
 *           NdrConformantStructMarshall [RPCRT4.@]
 */
unsigned char *  WINAPI NdrConformantStructMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    const NDR_CSTRUCT_FORMAT *pCStructFormat = (const NDR_CSTRUCT_FORMAT *)pFormat;
    PFORMAT_STRING pCArrayFormat;
    ULONG esize, bufsize;

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);

    pFormat += sizeof(NDR_CSTRUCT_FORMAT);
    if ((pCStructFormat->type != RPC_FC_CPSTRUCT) && (pCStructFormat->type != RPC_FC_CSTRUCT))
    {
        ERR("invalid format type %x\n", pCStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }

    pCArrayFormat = (const unsigned char *)&pCStructFormat->offset_to_array_description +
        pCStructFormat->offset_to_array_description;
    if (*pCArrayFormat != RPC_FC_CARRAY)
    {
        ERR("invalid array format type %x\n", pCStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }
    esize = *(const WORD*)(pCArrayFormat+2);

    ComputeConformance(pStubMsg, pMemory + pCStructFormat->memory_size,
                       pCArrayFormat + 4, 0);

    WriteConformance(pStubMsg);

    ALIGN_POINTER(pStubMsg->Buffer, pCStructFormat->alignment + 1);

    TRACE("memory_size = %d\n", pCStructFormat->memory_size);

    bufsize = safe_multiply(esize, pStubMsg->MaxCount);
    /* copy constant sized part of struct */
    pStubMsg->BufferMark = pStubMsg->Buffer;
    memcpy(pStubMsg->Buffer, pMemory, pCStructFormat->memory_size + bufsize);
    pStubMsg->Buffer += pCStructFormat->memory_size + bufsize;

    if (pCStructFormat->type == RPC_FC_CPSTRUCT)
        EmbeddedPointerMarshall(pStubMsg, pMemory, pFormat);

    STD_OVERFLOW_CHECK(pStubMsg);

    return NULL;
}

/***********************************************************************
 *           NdrConformantStructUnmarshall [RPCRT4.@]
 */
unsigned char *  WINAPI NdrConformantStructUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char **ppMemory,
                                PFORMAT_STRING pFormat,
                                unsigned char fMustAlloc)
{
    const NDR_CSTRUCT_FORMAT *pCStructFormat = (const NDR_CSTRUCT_FORMAT *)pFormat;
    PFORMAT_STRING pCArrayFormat;
    ULONG esize, bufsize;

    TRACE("(%p, %p, %p, %d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);

    pFormat += sizeof(NDR_CSTRUCT_FORMAT);
    if ((pCStructFormat->type != RPC_FC_CPSTRUCT) && (pCStructFormat->type != RPC_FC_CSTRUCT))
    {
        ERR("invalid format type %x\n", pCStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }
    pCArrayFormat = (const unsigned char *)&pCStructFormat->offset_to_array_description +
        pCStructFormat->offset_to_array_description;
    if (*pCArrayFormat != RPC_FC_CARRAY)
    {
        ERR("invalid array format type %x\n", pCStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }
    esize = *(const WORD*)(pCArrayFormat+2);

    pCArrayFormat = ReadConformance(pStubMsg, pCArrayFormat + 4);

    ALIGN_POINTER(pStubMsg->Buffer, pCStructFormat->alignment + 1);

    TRACE("memory_size = %d\n", pCStructFormat->memory_size);

    bufsize = safe_multiply(esize, pStubMsg->MaxCount);
    /* work out how much memory to allocate if we need to do so */
    if (!*ppMemory || fMustAlloc)
    {
        SIZE_T size = pCStructFormat->memory_size + bufsize;
        *ppMemory = NdrAllocate(pStubMsg, size);
    }

    /* now copy the data */
    pStubMsg->BufferMark = pStubMsg->Buffer;
    memcpy(*ppMemory, pStubMsg->Buffer, pCStructFormat->memory_size + bufsize);
    pStubMsg->Buffer += pCStructFormat->memory_size + bufsize;

    if (pCStructFormat->type == RPC_FC_CPSTRUCT)
        EmbeddedPointerUnmarshall(pStubMsg, ppMemory, pFormat, fMustAlloc);

    return NULL;
}

/***********************************************************************
 *           NdrConformantStructBufferSize [RPCRT4.@]
 */
void WINAPI NdrConformantStructBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    const NDR_CSTRUCT_FORMAT * pCStructFormat = (const NDR_CSTRUCT_FORMAT *)pFormat;
    PFORMAT_STRING pCArrayFormat;
    ULONG esize;

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);

    pFormat += sizeof(NDR_CSTRUCT_FORMAT);
    if ((pCStructFormat->type != RPC_FC_CPSTRUCT) && (pCStructFormat->type != RPC_FC_CSTRUCT))
    {
        ERR("invalid format type %x\n", pCStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }
    pCArrayFormat = (const unsigned char *)&pCStructFormat->offset_to_array_description +
        pCStructFormat->offset_to_array_description;
    if (*pCArrayFormat != RPC_FC_CARRAY)
    {
        ERR("invalid array format type %x\n", pCStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }
    esize = *(const WORD*)(pCArrayFormat+2);

    pCArrayFormat = ComputeConformance(pStubMsg, pMemory + pCStructFormat->memory_size, pCArrayFormat+4, 0);
    SizeConformance(pStubMsg);

    ALIGN_LENGTH(pStubMsg->BufferLength, pCStructFormat->alignment + 1);

    TRACE("memory_size = %d\n", pCStructFormat->memory_size);

    pStubMsg->BufferLength += pCStructFormat->memory_size +
        safe_multiply(pStubMsg->MaxCount, esize);

    if (pCStructFormat->type == RPC_FC_CPSTRUCT)
        EmbeddedPointerBufferSize(pStubMsg, pMemory, pFormat);
}

/***********************************************************************
 *           NdrConformantStructMemorySize [RPCRT4.@]
 */
ULONG WINAPI NdrConformantStructMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                PFORMAT_STRING pFormat)
{
    FIXME("stub\n");
    return 0;
}

/***********************************************************************
 *           NdrConformantStructFree [RPCRT4.@]
 */
void WINAPI NdrConformantStructFree(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    FIXME("stub\n");
}

/***********************************************************************
 *           NdrConformantVaryingStructMarshall [RPCRT4.@]
 */
unsigned char *  WINAPI NdrConformantVaryingStructMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    const NDR_CVSTRUCT_FORMAT *pCVStructFormat = (const NDR_CVSTRUCT_FORMAT *)pFormat;
    PFORMAT_STRING pCVArrayFormat;
    ULONG esize, bufsize;

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);

    pFormat += sizeof(NDR_CVSTRUCT_FORMAT);
    if (pCVStructFormat->type != RPC_FC_CVSTRUCT)
    {
        ERR("invalid format type %x\n", pCVStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }

    pCVArrayFormat = (const unsigned char *)&pCVStructFormat->offset_to_array_description +
        pCVStructFormat->offset_to_array_description;
    switch (*pCVArrayFormat)
    {
    case RPC_FC_CVARRAY:
        esize = *(const WORD*)(pCVArrayFormat+2);

        pCVArrayFormat = ComputeConformance(pStubMsg, pMemory + pCVStructFormat->memory_size,
                                            pCVArrayFormat + 4, 0);
        pCVArrayFormat = ComputeVariance(pStubMsg, pMemory + pCVStructFormat->memory_size,
                                         pCVArrayFormat, 0);
        break;
    case RPC_FC_C_CSTRING:
        TRACE("string=%s\n", debugstr_a((char*)pMemory + pCVStructFormat->memory_size));
        pStubMsg->ActualCount = strlen((char*)pMemory + pCVStructFormat->memory_size)+1;
        esize = sizeof(char);
        if (pCVArrayFormat[1] == RPC_FC_STRING_SIZED)
            pCVArrayFormat = ComputeConformance(pStubMsg, pMemory + pCVStructFormat->memory_size,
                                                pCVArrayFormat + 2, 0);
        else
            pStubMsg->MaxCount = pStubMsg->ActualCount;
        break;
    case RPC_FC_C_WSTRING:
        TRACE("string=%s\n", debugstr_w((LPWSTR)pMemory + pCVStructFormat->memory_size));
        pStubMsg->ActualCount = strlenW((LPWSTR)pMemory + pCVStructFormat->memory_size)+1;
        esize = sizeof(WCHAR);
        if (pCVArrayFormat[1] == RPC_FC_STRING_SIZED)
            pCVArrayFormat = ComputeConformance(pStubMsg, pMemory + pCVStructFormat->memory_size,
                                                pCVArrayFormat + 2, 0);
        else
            pStubMsg->MaxCount = pStubMsg->ActualCount;
        break;
    default:
        ERR("invalid array format type %x\n", *pCVArrayFormat);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }

    WriteConformance(pStubMsg);

    ALIGN_POINTER(pStubMsg->Buffer, pCVStructFormat->alignment + 1);

    TRACE("memory_size = %d\n", pCVStructFormat->memory_size);

    /* write constant sized part */
    pStubMsg->BufferMark = pStubMsg->Buffer;
    memcpy(pStubMsg->Buffer, pMemory, pCVStructFormat->memory_size);
    pStubMsg->Buffer += pCVStructFormat->memory_size;

    WriteVariance(pStubMsg);

    bufsize = safe_multiply(esize, pStubMsg->ActualCount);

    /* write array part */
    memcpy(pStubMsg->Buffer, pMemory + pCVStructFormat->memory_size, bufsize);
    pStubMsg->Buffer += bufsize;

    EmbeddedPointerMarshall(pStubMsg, pMemory, pFormat);

    STD_OVERFLOW_CHECK(pStubMsg);

    return NULL;
}

/***********************************************************************
 *           NdrConformantVaryingStructUnmarshall [RPCRT4.@]
 */
unsigned char *  WINAPI NdrConformantVaryingStructUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char **ppMemory,
                                PFORMAT_STRING pFormat,
                                unsigned char fMustAlloc)
{
    const NDR_CVSTRUCT_FORMAT *pCVStructFormat = (const NDR_CVSTRUCT_FORMAT *)pFormat;
    PFORMAT_STRING pCVArrayFormat;
    ULONG esize, bufsize;
    unsigned char cvarray_type;

    TRACE("(%p, %p, %p, %d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);

    pFormat += sizeof(NDR_CVSTRUCT_FORMAT);
    if (pCVStructFormat->type != RPC_FC_CVSTRUCT)
    {
        ERR("invalid format type %x\n", pCVStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }

    pCVArrayFormat = (const unsigned char *)&pCVStructFormat->offset_to_array_description +
        pCVStructFormat->offset_to_array_description;
    cvarray_type = *pCVArrayFormat;
    switch (cvarray_type)
    {
    case RPC_FC_CVARRAY:
        esize = *(const WORD*)(pCVArrayFormat+2);
        pCVArrayFormat = ReadConformance(pStubMsg, pCVArrayFormat + 4);
        break;
    case RPC_FC_C_CSTRING:
        esize = sizeof(char);
        if (pCVArrayFormat[1] == RPC_FC_STRING_SIZED)
            pCVArrayFormat = ReadConformance(pStubMsg, pCVArrayFormat + 2);
        else
            pCVArrayFormat = ReadConformance(pStubMsg, NULL);
        break;
    case RPC_FC_C_WSTRING:
        esize = sizeof(WCHAR);
        if (pCVArrayFormat[1] == RPC_FC_STRING_SIZED)
            pCVArrayFormat = ReadConformance(pStubMsg, pCVArrayFormat + 2);
        else
            pCVArrayFormat = ReadConformance(pStubMsg, NULL);
        break;
    default:
        ERR("invalid array format type %x\n", *pCVArrayFormat);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }

    ALIGN_POINTER(pStubMsg->Buffer, pCVStructFormat->alignment + 1);

    TRACE("memory_size = %d\n", pCVStructFormat->memory_size);

    /* work out how much memory to allocate if we need to do so */
    if (!*ppMemory || fMustAlloc)
    {
        SIZE_T size = pCVStructFormat->memory_size + safe_multiply(esize, pStubMsg->MaxCount);
        *ppMemory = NdrAllocate(pStubMsg, size);
    }

    /* copy the constant data */
    pStubMsg->BufferMark = pStubMsg->Buffer;
    memcpy(*ppMemory, pStubMsg->Buffer, pCVStructFormat->memory_size);
    pStubMsg->Buffer += pCVStructFormat->memory_size;

    pCVArrayFormat = ReadVariance(pStubMsg, pCVArrayFormat, pStubMsg->MaxCount);

    bufsize = safe_multiply(esize, pStubMsg->ActualCount);

    if ((cvarray_type == RPC_FC_C_CSTRING) ||
        (cvarray_type == RPC_FC_C_WSTRING))
    {
        ULONG i;
        /* strings must always have null terminating bytes */
        if (bufsize < esize)
        {
            ERR("invalid string length of %d\n", pStubMsg->ActualCount);
            RpcRaiseException(RPC_S_INVALID_BOUND);
            return NULL;
        }
        for (i = bufsize - esize; i < bufsize; i++)
            if (pStubMsg->Buffer[i] != 0)
            {
                ERR("string not null-terminated at byte position %d, data is 0x%x\n",
                    i, pStubMsg->Buffer[i]);
                RpcRaiseException(RPC_S_INVALID_BOUND);
                return NULL;
            }
    }

    /* copy the array data */
    memcpy(*ppMemory + pCVStructFormat->memory_size, pStubMsg->Buffer,
           bufsize);
    pStubMsg->Buffer += bufsize;

    if (cvarray_type == RPC_FC_C_CSTRING)
        TRACE("string=%s\n", debugstr_a((char *)(*ppMemory + pCVStructFormat->memory_size)));
    else if (cvarray_type == RPC_FC_C_WSTRING)
        TRACE("string=%s\n", debugstr_w((WCHAR *)(*ppMemory + pCVStructFormat->memory_size)));

    EmbeddedPointerUnmarshall(pStubMsg, ppMemory, pFormat, fMustAlloc);

    return NULL;
}

/***********************************************************************
 *           NdrConformantVaryingStructBufferSize [RPCRT4.@]
 */
void WINAPI NdrConformantVaryingStructBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    const NDR_CVSTRUCT_FORMAT *pCVStructFormat = (const NDR_CVSTRUCT_FORMAT *)pFormat;
    PFORMAT_STRING pCVArrayFormat;
    ULONG esize;

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);

    pFormat += sizeof(NDR_CVSTRUCT_FORMAT);
    if (pCVStructFormat->type != RPC_FC_CVSTRUCT)
    {
        ERR("invalid format type %x\n", pCVStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }

    pCVArrayFormat = (const unsigned char *)&pCVStructFormat->offset_to_array_description +
        pCVStructFormat->offset_to_array_description;
    switch (*pCVArrayFormat)
    {
    case RPC_FC_CVARRAY:
        esize = *(const WORD*)(pCVArrayFormat+2);

        pCVArrayFormat = ComputeConformance(pStubMsg, pMemory + pCVStructFormat->memory_size,
                                            pCVArrayFormat + 4, 0);
        pCVArrayFormat = ComputeVariance(pStubMsg, pMemory + pCVStructFormat->memory_size,
                                         pCVArrayFormat, 0);
        break;
    case RPC_FC_C_CSTRING:
        TRACE("string=%s\n", debugstr_a((char*)pMemory + pCVStructFormat->memory_size));
        pStubMsg->ActualCount = strlen((char*)pMemory + pCVStructFormat->memory_size)+1;
        esize = sizeof(char);
        if (pCVArrayFormat[1] == RPC_FC_STRING_SIZED)
            pCVArrayFormat = ComputeConformance(pStubMsg, pMemory + pCVStructFormat->memory_size,
                                                pCVArrayFormat + 2, 0);
        else
            pStubMsg->MaxCount = pStubMsg->ActualCount;
        break;
    case RPC_FC_C_WSTRING:
        TRACE("string=%s\n", debugstr_w((LPWSTR)pMemory + pCVStructFormat->memory_size));
        pStubMsg->ActualCount = strlenW((LPWSTR)pMemory + pCVStructFormat->memory_size)+1;
        esize = sizeof(WCHAR);
        if (pCVArrayFormat[1] == RPC_FC_STRING_SIZED)
            pCVArrayFormat = ComputeConformance(pStubMsg, pMemory + pCVStructFormat->memory_size,
                                                pCVArrayFormat + 2, 0);
        else
            pStubMsg->MaxCount = pStubMsg->ActualCount;
        break;
    default:
        ERR("invalid array format type %x\n", *pCVArrayFormat);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }

    SizeConformance(pStubMsg);

    ALIGN_LENGTH(pStubMsg->BufferLength, pCVStructFormat->alignment + 1);

    TRACE("memory_size = %d\n", pCVStructFormat->memory_size);

    pStubMsg->BufferLength += pCVStructFormat->memory_size;
    SizeVariance(pStubMsg);
    pStubMsg->BufferLength += safe_multiply(pStubMsg->MaxCount, esize);

    EmbeddedPointerBufferSize(pStubMsg, pMemory, pFormat);
}

/***********************************************************************
 *           NdrConformantVaryingStructMemorySize [RPCRT4.@]
 */
ULONG WINAPI NdrConformantVaryingStructMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                PFORMAT_STRING pFormat)
{
    const NDR_CVSTRUCT_FORMAT *pCVStructFormat = (const NDR_CVSTRUCT_FORMAT *)pFormat;
    PFORMAT_STRING pCVArrayFormat;
    ULONG esize;
    unsigned char cvarray_type;

    TRACE("(%p, %p)\n", pStubMsg, pFormat);

    pFormat += sizeof(NDR_CVSTRUCT_FORMAT);
    if (pCVStructFormat->type != RPC_FC_CVSTRUCT)
    {
        ERR("invalid format type %x\n", pCVStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return 0;
    }

    pCVArrayFormat = (const unsigned char *)&pCVStructFormat->offset_to_array_description +
        pCVStructFormat->offset_to_array_description;
    cvarray_type = *pCVArrayFormat;
    switch (cvarray_type)
    {
    case RPC_FC_CVARRAY:
        esize = *(const WORD*)(pCVArrayFormat+2);
        pCVArrayFormat = ReadConformance(pStubMsg, pCVArrayFormat + 4);
        break;
    case RPC_FC_C_CSTRING:
        esize = sizeof(char);
        if (pCVArrayFormat[1] == RPC_FC_STRING_SIZED)
            pCVArrayFormat = ReadConformance(pStubMsg, pCVArrayFormat + 2);
        else
            pCVArrayFormat = ReadConformance(pStubMsg, NULL);
        break;
    case RPC_FC_C_WSTRING:
        esize = sizeof(WCHAR);
        if (pCVArrayFormat[1] == RPC_FC_STRING_SIZED)
            pCVArrayFormat = ReadConformance(pStubMsg, pCVArrayFormat + 2);
        else
            pCVArrayFormat = ReadConformance(pStubMsg, NULL);
        break;
    default:
        ERR("invalid array format type %x\n", *pCVArrayFormat);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return 0;
    }

    ALIGN_POINTER(pStubMsg->Buffer, pCVStructFormat->alignment + 1);

    TRACE("memory_size = %d\n", pCVStructFormat->memory_size);

    pStubMsg->Buffer += pCVStructFormat->memory_size;
    pCVArrayFormat = ReadVariance(pStubMsg, pCVArrayFormat, pStubMsg->MaxCount);
    pStubMsg->Buffer += pCVStructFormat->memory_size + safe_multiply(esize, pStubMsg->ActualCount);

    pStubMsg->MemorySize += pCVStructFormat->memory_size + safe_multiply(esize, pStubMsg->MaxCount);

    EmbeddedPointerMemorySize(pStubMsg, pFormat);

    return pCVStructFormat->memory_size + pStubMsg->MaxCount * esize;
}

/***********************************************************************
 *           NdrConformantVaryingStructFree [RPCRT4.@]
 */
void WINAPI NdrConformantVaryingStructFree(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    const NDR_CVSTRUCT_FORMAT *pCVStructFormat = (const NDR_CVSTRUCT_FORMAT *)pFormat;
    PFORMAT_STRING pCVArrayFormat;
    ULONG esize;

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);

    pFormat += sizeof(NDR_CVSTRUCT_FORMAT);
    if (pCVStructFormat->type != RPC_FC_CVSTRUCT)
    {
        ERR("invalid format type %x\n", pCVStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }

    pCVArrayFormat = (const unsigned char *)&pCVStructFormat->offset_to_array_description +
        pCVStructFormat->offset_to_array_description;
    switch (*pCVArrayFormat)
    {
    case RPC_FC_CVARRAY:
        esize = *(const WORD*)(pCVArrayFormat+2);

        pCVArrayFormat = ComputeConformance(pStubMsg, pMemory + pCVStructFormat->memory_size,
                                            pCVArrayFormat + 4, 0);
        pCVArrayFormat = ComputeVariance(pStubMsg, pMemory + pCVStructFormat->memory_size,
                                         pCVArrayFormat, 0);
        break;
    case RPC_FC_C_CSTRING:
        TRACE("string=%s\n", debugstr_a((char*)pMemory + pCVStructFormat->memory_size));
        pStubMsg->ActualCount = strlen((char*)pMemory + pCVStructFormat->memory_size)+1;
        esize = sizeof(char);
        if (pCVArrayFormat[1] == RPC_FC_STRING_SIZED)
            pCVArrayFormat = ComputeConformance(pStubMsg, pMemory + pCVStructFormat->memory_size,
                                                pCVArrayFormat + 2, 0);
        else
            pStubMsg->MaxCount = pStubMsg->ActualCount;
        break;
    case RPC_FC_C_WSTRING:
        TRACE("string=%s\n", debugstr_w((LPWSTR)pMemory + pCVStructFormat->memory_size));
        pStubMsg->ActualCount = strlenW((LPWSTR)pMemory + pCVStructFormat->memory_size)+1;
        esize = sizeof(WCHAR);
        if (pCVArrayFormat[1] == RPC_FC_STRING_SIZED)
            pCVArrayFormat = ComputeConformance(pStubMsg, pMemory + pCVStructFormat->memory_size,
                                                pCVArrayFormat + 2, 0);
        else
            pStubMsg->MaxCount = pStubMsg->ActualCount;
        break;
    default:
        ERR("invalid array format type %x\n", *pCVArrayFormat);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }

    TRACE("memory_size = %d\n", pCVStructFormat->memory_size);

    EmbeddedPointerFree(pStubMsg, pMemory, pFormat);
}

#include "pshpack1.h"
typedef struct
{
    unsigned char type;
    unsigned char alignment;
    unsigned short total_size;
} NDR_SMFARRAY_FORMAT;

typedef struct
{
    unsigned char type;
    unsigned char alignment;
    unsigned long total_size;
} NDR_LGFARRAY_FORMAT;
#include "poppack.h"

/***********************************************************************
 *           NdrFixedArrayMarshall [RPCRT4.@]
 */
unsigned char *  WINAPI NdrFixedArrayMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    const NDR_SMFARRAY_FORMAT *pSmFArrayFormat = (const NDR_SMFARRAY_FORMAT *)pFormat;
    unsigned long total_size;

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);

    if ((pSmFArrayFormat->type != RPC_FC_SMFARRAY) &&
        (pSmFArrayFormat->type != RPC_FC_LGFARRAY))
    {
        ERR("invalid format type %x\n", pSmFArrayFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }

    ALIGN_POINTER(pStubMsg->Buffer, pSmFArrayFormat->alignment + 1);

    if (pSmFArrayFormat->type == RPC_FC_SMFARRAY)
    {
        total_size = pSmFArrayFormat->total_size;
        pFormat = (const unsigned char *)(pSmFArrayFormat + 1);
    }
    else
    {
        const NDR_LGFARRAY_FORMAT *pLgFArrayFormat = (const NDR_LGFARRAY_FORMAT *)pFormat;
        total_size = pLgFArrayFormat->total_size;
        pFormat = (const unsigned char *)(pLgFArrayFormat + 1);
    }

    memcpy(pStubMsg->Buffer, pMemory, total_size);
    pStubMsg->BufferMark = pStubMsg->Buffer;
    pStubMsg->Buffer += total_size;

    pFormat = EmbeddedPointerMarshall(pStubMsg, pMemory, pFormat);

    return NULL;
}

/***********************************************************************
 *           NdrFixedArrayUnmarshall [RPCRT4.@]
 */
unsigned char *  WINAPI NdrFixedArrayUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char **ppMemory,
                                PFORMAT_STRING pFormat,
                                unsigned char fMustAlloc)
{
    const NDR_SMFARRAY_FORMAT *pSmFArrayFormat = (const NDR_SMFARRAY_FORMAT *)pFormat;
    unsigned long total_size;

    TRACE("(%p, %p, %p, %d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);

    if ((pSmFArrayFormat->type != RPC_FC_SMFARRAY) &&
        (pSmFArrayFormat->type != RPC_FC_LGFARRAY))
    {
        ERR("invalid format type %x\n", pSmFArrayFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }

    ALIGN_POINTER(pStubMsg->Buffer, pSmFArrayFormat->alignment + 1);

    if (pSmFArrayFormat->type == RPC_FC_SMFARRAY)
    {
        total_size = pSmFArrayFormat->total_size;
        pFormat = (const unsigned char *)(pSmFArrayFormat + 1);
    }
    else
    {
        const NDR_LGFARRAY_FORMAT *pLgFArrayFormat = (const NDR_LGFARRAY_FORMAT *)pFormat;
        total_size = pLgFArrayFormat->total_size;
        pFormat = (const unsigned char *)(pLgFArrayFormat + 1);
    }

    if (fMustAlloc || !*ppMemory)
        *ppMemory = NdrAllocate(pStubMsg, total_size);
    memcpy(*ppMemory, pStubMsg->Buffer, total_size);
    pStubMsg->BufferMark = pStubMsg->Buffer;
    pStubMsg->Buffer += total_size;

    pFormat = EmbeddedPointerUnmarshall(pStubMsg, ppMemory, pFormat, fMustAlloc);

    return NULL;
}

/***********************************************************************
 *           NdrFixedArrayBufferSize [RPCRT4.@]
 */
void WINAPI NdrFixedArrayBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    const NDR_SMFARRAY_FORMAT *pSmFArrayFormat = (const NDR_SMFARRAY_FORMAT *)pFormat;
    unsigned long total_size;

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);

    if ((pSmFArrayFormat->type != RPC_FC_SMFARRAY) &&
        (pSmFArrayFormat->type != RPC_FC_LGFARRAY))
    {
        ERR("invalid format type %x\n", pSmFArrayFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }

    ALIGN_LENGTH(pStubMsg->BufferLength, pSmFArrayFormat->alignment + 1);

    if (pSmFArrayFormat->type == RPC_FC_SMFARRAY)
    {
        total_size = pSmFArrayFormat->total_size;
        pFormat = (const unsigned char *)(pSmFArrayFormat + 1);
    }
    else
    {
        const NDR_LGFARRAY_FORMAT *pLgFArrayFormat = (const NDR_LGFARRAY_FORMAT *)pFormat;
        total_size = pLgFArrayFormat->total_size;
        pFormat = (const unsigned char *)(pLgFArrayFormat + 1);
    }
    pStubMsg->BufferLength += total_size;

    EmbeddedPointerBufferSize(pStubMsg, pMemory, pFormat);
}

/***********************************************************************
 *           NdrFixedArrayMemorySize [RPCRT4.@]
 */
ULONG WINAPI NdrFixedArrayMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                PFORMAT_STRING pFormat)
{
    const NDR_SMFARRAY_FORMAT *pSmFArrayFormat = (const NDR_SMFARRAY_FORMAT *)pFormat;
    ULONG total_size;

    TRACE("(%p, %p)\n", pStubMsg, pFormat);

    if ((pSmFArrayFormat->type != RPC_FC_SMFARRAY) &&
        (pSmFArrayFormat->type != RPC_FC_LGFARRAY))
    {
        ERR("invalid format type %x\n", pSmFArrayFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return 0;
    }

    ALIGN_POINTER(pStubMsg->Buffer, pSmFArrayFormat->alignment + 1);

    if (pSmFArrayFormat->type == RPC_FC_SMFARRAY)
    {
        total_size = pSmFArrayFormat->total_size;
        pFormat = (const unsigned char *)(pSmFArrayFormat + 1);
    }
    else
    {
        const NDR_LGFARRAY_FORMAT *pLgFArrayFormat = (const NDR_LGFARRAY_FORMAT *)pFormat;
        total_size = pLgFArrayFormat->total_size;
        pFormat = (const unsigned char *)(pLgFArrayFormat + 1);
    }
    pStubMsg->BufferMark = pStubMsg->Buffer;
    pStubMsg->Buffer += total_size;
    pStubMsg->MemorySize += total_size;

    EmbeddedPointerMemorySize(pStubMsg, pFormat);

    return total_size;
}

/***********************************************************************
 *           NdrFixedArrayFree [RPCRT4.@]
 */
void WINAPI NdrFixedArrayFree(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    const NDR_SMFARRAY_FORMAT *pSmFArrayFormat = (const NDR_SMFARRAY_FORMAT *)pFormat;

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);

    if ((pSmFArrayFormat->type != RPC_FC_SMFARRAY) &&
        (pSmFArrayFormat->type != RPC_FC_LGFARRAY))
    {
        ERR("invalid format type %x\n", pSmFArrayFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }

    if (pSmFArrayFormat->type == RPC_FC_SMFARRAY)
        pFormat = (const unsigned char *)(pSmFArrayFormat + 1);
    else
    {
        const NDR_LGFARRAY_FORMAT *pLgFArrayFormat = (const NDR_LGFARRAY_FORMAT *)pFormat;
        pFormat = (const unsigned char *)(pLgFArrayFormat + 1);
    }

    EmbeddedPointerFree(pStubMsg, pMemory, pFormat);
}

/***********************************************************************
 *           NdrVaryingArrayMarshall [RPCRT4.@]
 */
unsigned char *  WINAPI NdrVaryingArrayMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    unsigned char alignment;
    DWORD elements, esize;
    ULONG bufsize;

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);

    if ((pFormat[0] != RPC_FC_SMVARRAY) &&
        (pFormat[0] != RPC_FC_LGVARRAY))
    {
        ERR("invalid format type %x\n", pFormat[0]);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }

    alignment = pFormat[1] + 1;

    if (pFormat[0] == RPC_FC_SMVARRAY)
    {
        pFormat += 2;
        pFormat += sizeof(WORD);
        elements = *(const WORD*)pFormat;
        pFormat += sizeof(WORD);
    }
    else
    {
        pFormat += 2;
        pFormat += sizeof(DWORD);
        elements = *(const DWORD*)pFormat;
        pFormat += sizeof(DWORD);
    }

    esize = *(const WORD*)pFormat;
    pFormat += sizeof(WORD);

    pFormat = ComputeVariance(pStubMsg, pMemory, pFormat, 0);
    if ((pStubMsg->ActualCount > elements) ||
        (pStubMsg->ActualCount + pStubMsg->Offset > elements))
    {
        RpcRaiseException(RPC_S_INVALID_BOUND);
        return NULL;
    }

    WriteVariance(pStubMsg);

    ALIGN_POINTER(pStubMsg->Buffer, alignment);

    bufsize = safe_multiply(esize, pStubMsg->ActualCount);
    memcpy(pStubMsg->Buffer, pMemory + pStubMsg->Offset, bufsize);
    pStubMsg->BufferMark = pStubMsg->Buffer;
    pStubMsg->Buffer += bufsize;

    EmbeddedPointerMarshall(pStubMsg, pMemory, pFormat);

    STD_OVERFLOW_CHECK(pStubMsg);

    return NULL;
}

/***********************************************************************
 *           NdrVaryingArrayUnmarshall [RPCRT4.@]
 */
unsigned char *  WINAPI NdrVaryingArrayUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char **ppMemory,
                                PFORMAT_STRING pFormat,
                                unsigned char fMustAlloc)
{
    unsigned char alignment;
    DWORD size, elements, esize;
    ULONG bufsize;

    TRACE("(%p, %p, %p, %d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);

    if ((pFormat[0] != RPC_FC_SMVARRAY) &&
        (pFormat[0] != RPC_FC_LGVARRAY))
    {
        ERR("invalid format type %x\n", pFormat[0]);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }

    alignment = pFormat[1] + 1;

    if (pFormat[0] == RPC_FC_SMVARRAY)
    {
        pFormat += 2;
        size = *(const WORD*)pFormat;
        pFormat += sizeof(WORD);
        elements = *(const WORD*)pFormat;
        pFormat += sizeof(WORD);
    }
    else
    {
        pFormat += 2;
        size = *(const DWORD*)pFormat;
        pFormat += sizeof(DWORD);
        elements = *(const DWORD*)pFormat;
        pFormat += sizeof(DWORD);
    }

    esize = *(const WORD*)pFormat;
    pFormat += sizeof(WORD);

    pFormat = ReadVariance(pStubMsg, pFormat, elements);

    ALIGN_POINTER(pStubMsg->Buffer, alignment);

    bufsize = safe_multiply(esize, pStubMsg->ActualCount);

    if (!*ppMemory || fMustAlloc)
        *ppMemory = NdrAllocate(pStubMsg, size);
    memcpy(*ppMemory + pStubMsg->Offset, pStubMsg->Buffer, bufsize);
    pStubMsg->Buffer += bufsize;

    EmbeddedPointerUnmarshall(pStubMsg, ppMemory, pFormat, fMustAlloc);

    return NULL;
}

/***********************************************************************
 *           NdrVaryingArrayBufferSize [RPCRT4.@]
 */
void WINAPI NdrVaryingArrayBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    unsigned char alignment;
    DWORD elements, esize;

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);

    if ((pFormat[0] != RPC_FC_SMVARRAY) &&
        (pFormat[0] != RPC_FC_LGVARRAY))
    {
        ERR("invalid format type %x\n", pFormat[0]);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }

    alignment = pFormat[1] + 1;

    if (pFormat[0] == RPC_FC_SMVARRAY)
    {
        pFormat += 2;
        pFormat += sizeof(WORD);
        elements = *(const WORD*)pFormat;
        pFormat += sizeof(WORD);
    }
    else
    {
        pFormat += 2;
        pFormat += sizeof(DWORD);
        elements = *(const DWORD*)pFormat;
        pFormat += sizeof(DWORD);
    }

    esize = *(const WORD*)pFormat;
    pFormat += sizeof(WORD);

    pFormat = ComputeVariance(pStubMsg, pMemory, pFormat, 0);
    if ((pStubMsg->ActualCount > elements) ||
        (pStubMsg->ActualCount + pStubMsg->Offset > elements))
    {
        RpcRaiseException(RPC_S_INVALID_BOUND);
        return;
    }

    SizeVariance(pStubMsg);

    ALIGN_LENGTH(pStubMsg->BufferLength, alignment);

    pStubMsg->BufferLength += safe_multiply(esize, pStubMsg->ActualCount);

    EmbeddedPointerBufferSize(pStubMsg, pMemory, pFormat);
}

/***********************************************************************
 *           NdrVaryingArrayMemorySize [RPCRT4.@]
 */
ULONG WINAPI NdrVaryingArrayMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                PFORMAT_STRING pFormat)
{
    unsigned char alignment;
    DWORD size, elements, esize;

    TRACE("(%p, %p)\n", pStubMsg, pFormat);

    if ((pFormat[0] != RPC_FC_SMVARRAY) &&
        (pFormat[0] != RPC_FC_LGVARRAY))
    {
        ERR("invalid format type %x\n", pFormat[0]);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return 0;
    }

    alignment = pFormat[1] + 1;

    if (pFormat[0] == RPC_FC_SMVARRAY)
    {
        pFormat += 2;
        size = *(const WORD*)pFormat;
        pFormat += sizeof(WORD);
        elements = *(const WORD*)pFormat;
        pFormat += sizeof(WORD);
    }
    else
    {
        pFormat += 2;
        size = *(const DWORD*)pFormat;
        pFormat += sizeof(DWORD);
        elements = *(const DWORD*)pFormat;
        pFormat += sizeof(DWORD);
    }

    esize = *(const WORD*)pFormat;
    pFormat += sizeof(WORD);

    pFormat = ReadVariance(pStubMsg, pFormat, elements);

    ALIGN_POINTER(pStubMsg->Buffer, alignment);

    pStubMsg->Buffer += safe_multiply(esize, pStubMsg->ActualCount);
    pStubMsg->MemorySize += size;

    EmbeddedPointerMemorySize(pStubMsg, pFormat);

    return pStubMsg->MemorySize;
}

/***********************************************************************
 *           NdrVaryingArrayFree [RPCRT4.@]
 */
void WINAPI NdrVaryingArrayFree(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    unsigned char alignment;
    DWORD elements;

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);

    if ((pFormat[0] != RPC_FC_SMVARRAY) &&
        (pFormat[0] != RPC_FC_LGVARRAY))
    {
        ERR("invalid format type %x\n", pFormat[0]);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }

    alignment = pFormat[1] + 1;

    if (pFormat[0] == RPC_FC_SMVARRAY)
    {
        pFormat += 2;
        pFormat += sizeof(WORD);
        elements = *(const WORD*)pFormat;
        pFormat += sizeof(WORD);
    }
    else
    {
        pFormat += 2;
        pFormat += sizeof(DWORD);
        elements = *(const DWORD*)pFormat;
        pFormat += sizeof(DWORD);
    }

    pFormat += sizeof(WORD);

    pFormat = ComputeVariance(pStubMsg, pMemory, pFormat, 0);
    if ((pStubMsg->ActualCount > elements) ||
        (pStubMsg->ActualCount + pStubMsg->Offset > elements))
    {
        RpcRaiseException(RPC_S_INVALID_BOUND);
        return;
    }

    EmbeddedPointerFree(pStubMsg, pMemory, pFormat);
}

static ULONG get_discriminant(unsigned char fc, unsigned char *pMemory)
{
    switch (fc)
    {
    case RPC_FC_BYTE:
    case RPC_FC_CHAR:
    case RPC_FC_SMALL:
    case RPC_FC_USMALL:
        return *(UCHAR *)pMemory;
    case RPC_FC_WCHAR:
    case RPC_FC_SHORT:
    case RPC_FC_USHORT:
    case RPC_FC_ENUM16:
        return *(USHORT *)pMemory;
    case RPC_FC_LONG:
    case RPC_FC_ULONG:
    case RPC_FC_ENUM32:
        return *(ULONG *)pMemory;
    default:
        FIXME("Unhandled base type: 0x%02x\n", fc);
        return 0;
    }
}

static PFORMAT_STRING get_arm_offset_from_union_arm_selector(PMIDL_STUB_MESSAGE pStubMsg,
                                                             unsigned long discriminant,
                                                             PFORMAT_STRING pFormat)
{
    unsigned short num_arms, arm, type;

    num_arms = *(const SHORT*)pFormat & 0x0fff;
    pFormat += 2;
    for(arm = 0; arm < num_arms; arm++)
    {
        if(discriminant == *(const ULONG*)pFormat)
        {
            pFormat += 4;
            break;
        }
        pFormat += 6;
    }

    type = *(const unsigned short*)pFormat;
    TRACE("type %04x\n", type);
    if(arm == num_arms) /* default arm extras */
    {
        if(type == 0xffff)
        {
            ERR("no arm for 0x%lx and no default case\n", discriminant);
            RpcRaiseException(RPC_S_INVALID_TAG);
            return NULL;
        }
        if(type == 0)
        {
            TRACE("falling back to empty default case for 0x%lx\n", discriminant);
            return NULL;
        }
    }
    return pFormat;
}

static unsigned char *union_arm_marshall(PMIDL_STUB_MESSAGE pStubMsg, unsigned char *pMemory, ULONG discriminant, PFORMAT_STRING pFormat)
{
    unsigned short type;

    pFormat += 2;

    pFormat = get_arm_offset_from_union_arm_selector(pStubMsg, discriminant, pFormat);
    if(!pFormat)
        return NULL;

    type = *(const unsigned short*)pFormat;
    if((type & 0xff00) == 0x8000)
    {
        unsigned char basetype = LOBYTE(type);
        return NdrBaseTypeMarshall(pStubMsg, pMemory, &basetype);
    }
    else
    {
        PFORMAT_STRING desc = pFormat + *(const SHORT*)pFormat;
        NDR_MARSHALL m = NdrMarshaller[*desc & NDR_TABLE_MASK];
        if (m)
        {
            unsigned char *saved_buffer = NULL;
            int pointer_buffer_mark_set = 0;
            switch(*desc)
            {
            case RPC_FC_RP:
            case RPC_FC_UP:
            case RPC_FC_OP:
            case RPC_FC_FP:
                ALIGN_POINTER(pStubMsg->Buffer, 4);
                saved_buffer = pStubMsg->Buffer;
                if (pStubMsg->PointerBufferMark)
                {
                  pStubMsg->Buffer = pStubMsg->PointerBufferMark;
                  pStubMsg->PointerBufferMark = NULL;
                  pointer_buffer_mark_set = 1;
                }
                else
                  pStubMsg->Buffer += 4; /* for pointer ID */

                PointerMarshall(pStubMsg, saved_buffer, *(unsigned char **)pMemory, desc);
                if (pointer_buffer_mark_set)
                {
                  STD_OVERFLOW_CHECK(pStubMsg);
                  pStubMsg->PointerBufferMark = pStubMsg->Buffer;
                  pStubMsg->Buffer = saved_buffer + 4;
                }
                break;
            default:
                m(pStubMsg, pMemory, desc);
            }
        }
        else FIXME("no marshaller for embedded type %02x\n", *desc);
    }
    return NULL;
}

static unsigned char *union_arm_unmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char **ppMemory,
                                ULONG discriminant,
                                PFORMAT_STRING pFormat,
                                unsigned char fMustAlloc)
{
    unsigned short type;

    pFormat += 2;

    pFormat = get_arm_offset_from_union_arm_selector(pStubMsg, discriminant, pFormat);
    if(!pFormat)
        return NULL;

    type = *(const unsigned short*)pFormat;
    if((type & 0xff00) == 0x8000)
    {
        unsigned char basetype = LOBYTE(type);
        return NdrBaseTypeUnmarshall(pStubMsg, ppMemory, &basetype, fMustAlloc);
    }
    else
    {
        PFORMAT_STRING desc = pFormat + *(const SHORT*)pFormat;
        NDR_UNMARSHALL m = NdrUnmarshaller[*desc & NDR_TABLE_MASK];
        if (m)
        {
            unsigned char *saved_buffer = NULL;
            int pointer_buffer_mark_set = 0;
            switch(*desc)
            {
            case RPC_FC_RP:
            case RPC_FC_UP:
            case RPC_FC_OP:
            case RPC_FC_FP:
                **(void***)ppMemory = NULL;
                ALIGN_POINTER(pStubMsg->Buffer, 4);
                saved_buffer = pStubMsg->Buffer;
                if (pStubMsg->PointerBufferMark)
                {
                  pStubMsg->Buffer = pStubMsg->PointerBufferMark;
                  pStubMsg->PointerBufferMark = NULL;
                  pointer_buffer_mark_set = 1;
                }
                else
                  pStubMsg->Buffer += 4; /* for pointer ID */

                PointerUnmarshall(pStubMsg, saved_buffer, *(unsigned char ***)ppMemory, desc, fMustAlloc);
                if (pointer_buffer_mark_set)
                {
                  STD_OVERFLOW_CHECK(pStubMsg);
                  pStubMsg->PointerBufferMark = pStubMsg->Buffer;
                  pStubMsg->Buffer = saved_buffer + 4;
                }
                break;
            default:
                m(pStubMsg, ppMemory, desc, fMustAlloc);
            }
        }
        else FIXME("no marshaller for embedded type %02x\n", *desc);
    }
    return NULL;
}

static void union_arm_buffer_size(PMIDL_STUB_MESSAGE pStubMsg,
                                  unsigned char *pMemory,
                                  ULONG discriminant,
                                  PFORMAT_STRING pFormat)
{
    unsigned short type;

    pFormat += 2;

    pFormat = get_arm_offset_from_union_arm_selector(pStubMsg, discriminant, pFormat);
    if(!pFormat)
        return;

    type = *(const unsigned short*)pFormat;
    if((type & 0xff00) == 0x8000)
    {
        unsigned char basetype = LOBYTE(type);
        NdrBaseTypeBufferSize(pStubMsg, pMemory, &basetype);
    }
    else
    {
        PFORMAT_STRING desc = pFormat + *(const SHORT*)pFormat;
        NDR_BUFFERSIZE m = NdrBufferSizer[*desc & NDR_TABLE_MASK];
        if (m)
        {
            switch(*desc)
            {
            case RPC_FC_RP:
            case RPC_FC_UP:
            case RPC_FC_OP:
            case RPC_FC_FP:
                ALIGN_LENGTH(pStubMsg->BufferLength, 4);
                pStubMsg->BufferLength += 4; /* for pointer ID */
                if (!pStubMsg->IgnoreEmbeddedPointers)
                {
                    int saved_buffer_length = pStubMsg->BufferLength;
                    pStubMsg->BufferLength = pStubMsg->PointerLength;
                    pStubMsg->PointerLength = 0;
                    if(!pStubMsg->BufferLength)
                        ERR("BufferLength == 0??\n");
                    PointerBufferSize(pStubMsg, *(unsigned char **)pMemory, desc);
                    pStubMsg->PointerLength = pStubMsg->BufferLength;
                    pStubMsg->BufferLength = saved_buffer_length;
                }
                break;
            default:
                m(pStubMsg, pMemory, desc);
            }
        }
        else FIXME("no buffersizer for embedded type %02x\n", *desc);
    }
}

static ULONG union_arm_memory_size(PMIDL_STUB_MESSAGE pStubMsg,
                                   ULONG discriminant,
                                   PFORMAT_STRING pFormat)
{
    unsigned short type, size;

    size = *(const unsigned short*)pFormat;
    pStubMsg->Memory += size;
    pFormat += 2;

    pFormat = get_arm_offset_from_union_arm_selector(pStubMsg, discriminant, pFormat);
    if(!pFormat)
        return 0;

    type = *(const unsigned short*)pFormat;
    if((type & 0xff00) == 0x8000)
    {
        return NdrBaseTypeMemorySize(pStubMsg, pFormat);
    }
    else
    {
        PFORMAT_STRING desc = pFormat + *(const SHORT*)pFormat;
        NDR_MEMORYSIZE m = NdrMemorySizer[*desc & NDR_TABLE_MASK];
        unsigned char *saved_buffer;
        if (m)
        {
            switch(*desc)
            {
            case RPC_FC_RP:
            case RPC_FC_UP:
            case RPC_FC_OP:
            case RPC_FC_FP:
                ALIGN_POINTER(pStubMsg->Buffer, 4);
                saved_buffer = pStubMsg->Buffer;
                pStubMsg->Buffer += 4;
                ALIGN_LENGTH(pStubMsg->MemorySize, 4);
                pStubMsg->MemorySize += 4;
                if (!pStubMsg->IgnoreEmbeddedPointers)
                    PointerMemorySize(pStubMsg, saved_buffer, pFormat);
                break;
            default:
                return m(pStubMsg, desc);
            }
        }
        else FIXME("no marshaller for embedded type %02x\n", *desc);
    }

    TRACE("size %d\n", size);
    return size;
}

static void union_arm_free(PMIDL_STUB_MESSAGE pStubMsg,
                           unsigned char *pMemory,
                           ULONG discriminant,
                           PFORMAT_STRING pFormat)
{
    unsigned short type;

    pFormat += 2;

    pFormat = get_arm_offset_from_union_arm_selector(pStubMsg, discriminant, pFormat);
    if(!pFormat)
        return;

    type = *(const unsigned short*)pFormat;
    if((type & 0xff00) != 0x8000)
    {
        PFORMAT_STRING desc = pFormat + *(const SHORT*)pFormat;
        NDR_FREE m = NdrFreer[*desc & NDR_TABLE_MASK];
        if (m)
        {
            switch(*desc)
            {
            case RPC_FC_RP:
            case RPC_FC_UP:
            case RPC_FC_OP:
            case RPC_FC_FP:
                PointerFree(pStubMsg, *(unsigned char **)pMemory, desc);
                break;
            default:
                m(pStubMsg, pMemory, desc);
            }
        }
        else FIXME("no freer for embedded type %02x\n", *desc);
    }
}

/***********************************************************************
 *           NdrEncapsulatedUnionMarshall [RPCRT4.@]
 */
unsigned char *  WINAPI NdrEncapsulatedUnionMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    unsigned char switch_type;
    unsigned char increment;
    ULONG switch_value;

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);
    pFormat++;

    switch_type = *pFormat & 0xf;
    increment = (*pFormat & 0xf0) >> 4;
    pFormat++;

    ALIGN_POINTER(pStubMsg->Buffer, increment);

    switch_value = get_discriminant(switch_type, pMemory);
    TRACE("got switch value 0x%x\n", switch_value);

    NdrBaseTypeMarshall(pStubMsg, pMemory, &switch_type);
    pMemory += increment;

    return union_arm_marshall(pStubMsg, pMemory, switch_value, pFormat);
}

/***********************************************************************
 *           NdrEncapsulatedUnionUnmarshall [RPCRT4.@]
 */
unsigned char *  WINAPI NdrEncapsulatedUnionUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char **ppMemory,
                                PFORMAT_STRING pFormat,
                                unsigned char fMustAlloc)
{
    unsigned char switch_type;
    unsigned char increment;
    ULONG switch_value;
    unsigned short size;
    unsigned char *pMemoryArm;

    TRACE("(%p, %p, %p, %d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);
    pFormat++;

    switch_type = *pFormat & 0xf;
    increment = (*pFormat & 0xf0) >> 4;
    pFormat++;

    ALIGN_POINTER(pStubMsg->Buffer, increment);
    switch_value = get_discriminant(switch_type, pStubMsg->Buffer);
    TRACE("got switch value 0x%x\n", switch_value);

    size = *(const unsigned short*)pFormat + increment;
    if(!*ppMemory || fMustAlloc)
        *ppMemory = NdrAllocate(pStubMsg, size);

    NdrBaseTypeUnmarshall(pStubMsg, ppMemory, &switch_type, FALSE);
    pMemoryArm = *ppMemory + increment;

    return union_arm_unmarshall(pStubMsg, &pMemoryArm, switch_value, pFormat, fMustAlloc);
}

/***********************************************************************
 *           NdrEncapsulatedUnionBufferSize [RPCRT4.@]
 */
void WINAPI NdrEncapsulatedUnionBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    unsigned char switch_type;
    unsigned char increment;
    ULONG switch_value;

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);
    pFormat++;

    switch_type = *pFormat & 0xf;
    increment = (*pFormat & 0xf0) >> 4;
    pFormat++;

    ALIGN_LENGTH(pStubMsg->BufferLength, increment);
    switch_value = get_discriminant(switch_type, pMemory);
    TRACE("got switch value 0x%x\n", switch_value);

    /* Add discriminant size */
    NdrBaseTypeBufferSize(pStubMsg, (unsigned char *)&switch_value, &switch_type);
    pMemory += increment;

    union_arm_buffer_size(pStubMsg, pMemory, switch_value, pFormat);
}

/***********************************************************************
 *           NdrEncapsulatedUnionMemorySize [RPCRT4.@]
 */
ULONG WINAPI NdrEncapsulatedUnionMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                PFORMAT_STRING pFormat)
{
    unsigned char switch_type;
    unsigned char increment;
    ULONG switch_value;

    switch_type = *pFormat & 0xf;
    increment = (*pFormat & 0xf0) >> 4;
    pFormat++;

    ALIGN_POINTER(pStubMsg->Buffer, increment);
    switch_value = get_discriminant(switch_type, pStubMsg->Buffer);
    TRACE("got switch value 0x%x\n", switch_value);

    pStubMsg->Memory += increment;

    return increment + union_arm_memory_size(pStubMsg, switch_value, pFormat + *(const SHORT*)pFormat);
}

/***********************************************************************
 *           NdrEncapsulatedUnionFree [RPCRT4.@]
 */
void WINAPI NdrEncapsulatedUnionFree(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    unsigned char switch_type;
    unsigned char increment;
    ULONG switch_value;

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);
    pFormat++;

    switch_type = *pFormat & 0xf;
    increment = (*pFormat & 0xf0) >> 4;
    pFormat++;

    switch_value = get_discriminant(switch_type, pMemory);
    TRACE("got switch value 0x%x\n", switch_value);

    pMemory += increment;

    return union_arm_free(pStubMsg, pMemory, switch_value, pFormat);
}

/***********************************************************************
 *           NdrNonEncapsulatedUnionMarshall [RPCRT4.@]
 */
unsigned char *  WINAPI NdrNonEncapsulatedUnionMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    unsigned char switch_type;

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);
    pFormat++;

    switch_type = *pFormat;
    pFormat++;

    pFormat = ComputeConformance(pStubMsg, pMemory, pFormat, 0);
    TRACE("got switch value 0x%lx\n", pStubMsg->MaxCount);
    /* Marshall discriminant */
    NdrBaseTypeMarshall(pStubMsg, (unsigned char *)&pStubMsg->MaxCount, &switch_type);

    return union_arm_marshall(pStubMsg, pMemory, pStubMsg->MaxCount, pFormat + *(const SHORT*)pFormat);
}

static long unmarshall_discriminant(PMIDL_STUB_MESSAGE pStubMsg,
                                    PFORMAT_STRING *ppFormat)
{
    long discriminant = 0;

    switch(**ppFormat)
    {
    case RPC_FC_BYTE:
    case RPC_FC_CHAR:
    case RPC_FC_SMALL:
    case RPC_FC_USMALL:
        discriminant = *(UCHAR *)pStubMsg->Buffer;
        pStubMsg->Buffer += sizeof(UCHAR);
        break;
    case RPC_FC_WCHAR:
    case RPC_FC_SHORT:
    case RPC_FC_USHORT:
        ALIGN_POINTER(pStubMsg->Buffer, sizeof(USHORT));
        discriminant = *(USHORT *)pStubMsg->Buffer;
        pStubMsg->Buffer += sizeof(USHORT);
        break;
    case RPC_FC_LONG:
    case RPC_FC_ULONG:
        ALIGN_POINTER(pStubMsg->Buffer, sizeof(ULONG));
        discriminant = *(ULONG *)pStubMsg->Buffer;
        pStubMsg->Buffer += sizeof(ULONG);
        break;
    default:
        FIXME("Unhandled base type: 0x%02x\n", **ppFormat);
    }
    (*ppFormat)++;

    if (pStubMsg->fHasNewCorrDesc)
        *ppFormat += 6;
    else
        *ppFormat += 4;
    return discriminant;
}

/**********************************************************************
 *           NdrNonEncapsulatedUnionUnmarshall [RPCRT4.@]
 */
unsigned char *  WINAPI NdrNonEncapsulatedUnionUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char **ppMemory,
                                PFORMAT_STRING pFormat,
                                unsigned char fMustAlloc)
{
    long discriminant;
    unsigned short size;

    TRACE("(%p, %p, %p, %d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);
    pFormat++;

    /* Unmarshall discriminant */
    discriminant = unmarshall_discriminant(pStubMsg, &pFormat);
    TRACE("unmarshalled discriminant %lx\n", discriminant);

    pFormat += *(const SHORT*)pFormat;

    size = *(const unsigned short*)pFormat;

    if(!*ppMemory || fMustAlloc)
        *ppMemory = NdrAllocate(pStubMsg, size);

    return union_arm_unmarshall(pStubMsg, ppMemory, discriminant, pFormat, fMustAlloc);
}

/***********************************************************************
 *           NdrNonEncapsulatedUnionBufferSize [RPCRT4.@]
 */
void WINAPI NdrNonEncapsulatedUnionBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    unsigned char switch_type;

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);
    pFormat++;

    switch_type = *pFormat;
    pFormat++;

    pFormat = ComputeConformance(pStubMsg, pMemory, pFormat, 0);
    TRACE("got switch value 0x%lx\n", pStubMsg->MaxCount);
    /* Add discriminant size */
    NdrBaseTypeBufferSize(pStubMsg, (unsigned char *)&pStubMsg->MaxCount, &switch_type);

    union_arm_buffer_size(pStubMsg, pMemory, pStubMsg->MaxCount, pFormat + *(const SHORT*)pFormat);
}

/***********************************************************************
 *           NdrNonEncapsulatedUnionMemorySize [RPCRT4.@]
 */
ULONG WINAPI NdrNonEncapsulatedUnionMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                PFORMAT_STRING pFormat)
{
    ULONG discriminant;

    pFormat++;
    /* Unmarshall discriminant */
    discriminant = unmarshall_discriminant(pStubMsg, &pFormat);
    TRACE("unmarshalled discriminant 0x%x\n", discriminant);

    return union_arm_memory_size(pStubMsg, discriminant, pFormat + *(const SHORT*)pFormat);
}

/***********************************************************************
 *           NdrNonEncapsulatedUnionFree [RPCRT4.@]
 */
void WINAPI NdrNonEncapsulatedUnionFree(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);
    pFormat++;
    pFormat++;

    pFormat = ComputeConformance(pStubMsg, pMemory, pFormat, 0);
    TRACE("got switch value 0x%lx\n", pStubMsg->MaxCount);

    return union_arm_free(pStubMsg, pMemory, pStubMsg->MaxCount, pFormat + *(const SHORT*)pFormat);
}

/***********************************************************************
 *           NdrByteCountPointerMarshall [RPCRT4.@]
 */
unsigned char *  WINAPI NdrByteCountPointerMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    FIXME("stub\n");
    return NULL;
}

/***********************************************************************
 *           NdrByteCountPointerUnmarshall [RPCRT4.@]
 */
unsigned char *  WINAPI NdrByteCountPointerUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char **ppMemory,
                                PFORMAT_STRING pFormat,
                                unsigned char fMustAlloc)
{
    FIXME("stub\n");
    return NULL;
}

/***********************************************************************
 *           NdrByteCountPointerBufferSize [RPCRT4.@]
 */
void WINAPI NdrByteCountPointerBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    FIXME("stub\n");
}

/***********************************************************************
 *           NdrByteCountPointerMemorySize [RPCRT4.@]
 */
ULONG WINAPI NdrByteCountPointerMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                PFORMAT_STRING pFormat)
{
    FIXME("stub\n");
    return 0;
}

/***********************************************************************
 *           NdrByteCountPointerFree [RPCRT4.@]
 */
void WINAPI NdrByteCountPointerFree(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    FIXME("stub\n");
}

/***********************************************************************
 *           NdrXmitOrRepAsMarshall [RPCRT4.@]
 */
unsigned char *  WINAPI NdrXmitOrRepAsMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    FIXME("stub\n");
    return NULL;
}

/***********************************************************************
 *           NdrXmitOrRepAsUnmarshall [RPCRT4.@]
 */
unsigned char *  WINAPI NdrXmitOrRepAsUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char **ppMemory,
                                PFORMAT_STRING pFormat,
                                unsigned char fMustAlloc)
{
    FIXME("stub\n");
    return NULL;
}

/***********************************************************************
 *           NdrXmitOrRepAsBufferSize [RPCRT4.@]
 */
void WINAPI NdrXmitOrRepAsBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    FIXME("stub\n");
}

/***********************************************************************
 *           NdrXmitOrRepAsMemorySize [RPCRT4.@]
 */
ULONG WINAPI NdrXmitOrRepAsMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                PFORMAT_STRING pFormat)
{
    FIXME("stub\n");
    return 0;
}

/***********************************************************************
 *           NdrXmitOrRepAsFree [RPCRT4.@]
 */
void WINAPI NdrXmitOrRepAsFree(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
    FIXME("stub\n");
}

#include "pshpack1.h"
typedef struct
{
    unsigned char type;
    unsigned char flags_type; /* flags in upper nibble, type in lower nibble */
    ULONG low_value;
    ULONG high_value;
} NDR_RANGE;
#include "poppack.h"

/***********************************************************************
 *           NdrRangeMarshall [internal]
 */
unsigned char *WINAPI NdrRangeMarshall(
    PMIDL_STUB_MESSAGE pStubMsg,
    unsigned char *pMemory,
    PFORMAT_STRING pFormat)
{
    NDR_RANGE *pRange = (NDR_RANGE *)pFormat;
    unsigned char base_type;

    TRACE("pStubMsg %p, pMemory %p, type 0x%02x\n", pStubMsg, pMemory, *pFormat);

    if (pRange->type != RPC_FC_RANGE)
    {
        ERR("invalid format type %x\n", pRange->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }

    base_type = pRange->flags_type & 0xf;

    return NdrBaseTypeMarshall(pStubMsg, pMemory, &base_type);
}

/***********************************************************************
 *           NdrRangeUnmarshall
 */
unsigned char *WINAPI NdrRangeUnmarshall(
    PMIDL_STUB_MESSAGE pStubMsg,
    unsigned char **ppMemory,
    PFORMAT_STRING pFormat,
    unsigned char fMustAlloc)
{
    NDR_RANGE *pRange = (NDR_RANGE *)pFormat;
    unsigned char base_type;

    TRACE("pStubMsg: %p, ppMemory: %p, type: 0x%02x, fMustAlloc: %s\n", pStubMsg, ppMemory, *pFormat, fMustAlloc ? "true" : "false");

    if (pRange->type != RPC_FC_RANGE)
    {
        ERR("invalid format type %x\n", pRange->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }
    base_type = pRange->flags_type & 0xf;

    TRACE("base_type = 0x%02x, low_value = %d, high_value = %d\n",
        base_type, pRange->low_value, pRange->high_value);

#define RANGE_UNMARSHALL(type, format_spec) \
    do \
    { \
        ALIGN_POINTER(pStubMsg->Buffer, sizeof(type)); \
        if (fMustAlloc || !*ppMemory) \
            *ppMemory = NdrAllocate(pStubMsg, sizeof(type)); \
        if ((*(type *)pStubMsg->Buffer < (type)pRange->low_value) || \
            (*(type *)pStubMsg->Buffer > (type)pRange->high_value)) \
        { \
            ERR("value exceeded bounds: " format_spec ", low: " format_spec ", high: " format_spec "\n", \
                *(type *)pStubMsg->Buffer, (type)pRange->low_value, \
                (type)pRange->high_value); \
            RpcRaiseException(RPC_S_INVALID_BOUND); \
            return NULL; \
        } \
        TRACE("*ppMemory: %p\n", *ppMemory); \
        **(type **)ppMemory = *(type *)pStubMsg->Buffer; \
        pStubMsg->Buffer += sizeof(type); \
    } while (0)

    switch(base_type)
    {
    case RPC_FC_CHAR:
    case RPC_FC_SMALL:
        RANGE_UNMARSHALL(UCHAR, "%d");
        TRACE("value: 0x%02x\n", **(UCHAR **)ppMemory);
        break;
    case RPC_FC_BYTE:
    case RPC_FC_USMALL:
        RANGE_UNMARSHALL(CHAR, "%u");
        TRACE("value: 0x%02x\n", **(UCHAR **)ppMemory);
        break;
    case RPC_FC_WCHAR: /* FIXME: valid? */
    case RPC_FC_USHORT:
        RANGE_UNMARSHALL(USHORT, "%u");
        TRACE("value: 0x%04x\n", **(USHORT **)ppMemory);
        break;
    case RPC_FC_SHORT:
        RANGE_UNMARSHALL(SHORT, "%d");
        TRACE("value: 0x%04x\n", **(USHORT **)ppMemory);
        break;
    case RPC_FC_LONG:
        RANGE_UNMARSHALL(LONG, "%d");
        TRACE("value: 0x%08x\n", **(ULONG **)ppMemory);
        break;
    case RPC_FC_ULONG:
        RANGE_UNMARSHALL(ULONG, "%u");
        TRACE("value: 0x%08x\n", **(ULONG **)ppMemory);
        break;
    case RPC_FC_ENUM16:
    case RPC_FC_ENUM32:
        FIXME("Unhandled enum type\n");
        break;
    case RPC_FC_ERROR_STATUS_T: /* FIXME: valid? */
    case RPC_FC_FLOAT:
    case RPC_FC_DOUBLE:
    case RPC_FC_HYPER:
    default:
        ERR("invalid range base type: 0x%02x\n", base_type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
    }

    return NULL;
}

/***********************************************************************
 *           NdrRangeBufferSize [internal]
 */
void WINAPI NdrRangeBufferSize(
    PMIDL_STUB_MESSAGE pStubMsg,
    unsigned char *pMemory,
    PFORMAT_STRING pFormat)
{
    NDR_RANGE *pRange = (NDR_RANGE *)pFormat;
    unsigned char base_type;

    TRACE("pStubMsg %p, pMemory %p, type 0x%02x\n", pStubMsg, pMemory, *pFormat);

    if (pRange->type != RPC_FC_RANGE)
    {
        ERR("invalid format type %x\n", pRange->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
    }
    base_type = pRange->flags_type & 0xf;

    NdrBaseTypeBufferSize(pStubMsg, pMemory, &base_type);
}

/***********************************************************************
 *           NdrRangeMemorySize [internal]
 */
ULONG WINAPI NdrRangeMemorySize(
    PMIDL_STUB_MESSAGE pStubMsg,
    PFORMAT_STRING pFormat)
{
    NDR_RANGE *pRange = (NDR_RANGE *)pFormat;
    unsigned char base_type;

    if (pRange->type != RPC_FC_RANGE)
    {
        ERR("invalid format type %x\n", pRange->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return 0;
    }
    base_type = pRange->flags_type & 0xf;

    return NdrBaseTypeMemorySize(pStubMsg, &base_type);
}

/***********************************************************************
 *           NdrRangeFree [internal]
 */
void WINAPI NdrRangeFree(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
   TRACE("pStubMsg %p pMemory %p type 0x%02x\n", pStubMsg, pMemory, *pFormat);

   /* nothing to do */
}

/***********************************************************************
 *           NdrBaseTypeMarshall [internal]
 */
static unsigned char *WINAPI NdrBaseTypeMarshall(
    PMIDL_STUB_MESSAGE pStubMsg,
    unsigned char *pMemory,
    PFORMAT_STRING pFormat)
{
    TRACE("pStubMsg %p, pMemory %p, type 0x%02x\n", pStubMsg, pMemory, *pFormat);

    switch(*pFormat)
    {
    case RPC_FC_BYTE:
    case RPC_FC_CHAR:
    case RPC_FC_SMALL:
    case RPC_FC_USMALL:
        *(UCHAR *)pStubMsg->Buffer = *(UCHAR *)pMemory;
        pStubMsg->Buffer += sizeof(UCHAR);
        TRACE("value: 0x%02x\n", *(UCHAR *)pMemory);
        break;
    case RPC_FC_WCHAR:
    case RPC_FC_SHORT:
    case RPC_FC_USHORT:
        ALIGN_POINTER(pStubMsg->Buffer, sizeof(USHORT));
        *(USHORT *)pStubMsg->Buffer = *(USHORT *)pMemory;
        pStubMsg->Buffer += sizeof(USHORT);
        TRACE("value: 0x%04x\n", *(USHORT *)pMemory);
        break;
    case RPC_FC_LONG:
    case RPC_FC_ULONG:
    case RPC_FC_ERROR_STATUS_T:
    case RPC_FC_ENUM32:
        ALIGN_POINTER(pStubMsg->Buffer, sizeof(ULONG));
        *(ULONG *)pStubMsg->Buffer = *(ULONG *)pMemory;
        pStubMsg->Buffer += sizeof(ULONG);
        TRACE("value: 0x%08x\n", *(ULONG *)pMemory);
        break;
    case RPC_FC_FLOAT:
        ALIGN_POINTER(pStubMsg->Buffer, sizeof(float));
        *(float *)pStubMsg->Buffer = *(float *)pMemory;
        pStubMsg->Buffer += sizeof(float);
        break;
    case RPC_FC_DOUBLE:
        ALIGN_POINTER(pStubMsg->Buffer, sizeof(double));
        *(double *)pStubMsg->Buffer = *(double *)pMemory;
        pStubMsg->Buffer += sizeof(double);
        break;
    case RPC_FC_HYPER:
        ALIGN_POINTER(pStubMsg->Buffer, sizeof(ULONGLONG));
        *(ULONGLONG *)pStubMsg->Buffer = *(ULONGLONG *)pMemory;
        pStubMsg->Buffer += sizeof(ULONGLONG);
        TRACE("value: %s\n", wine_dbgstr_longlong(*(ULONGLONG*)pMemory));
        break;
    case RPC_FC_ENUM16:
        /* only 16-bits on the wire, so do a sanity check */
        if (*(UINT *)pMemory > USHRT_MAX)
            RpcRaiseException(RPC_X_ENUM_VALUE_OUT_OF_RANGE);
        ALIGN_POINTER(pStubMsg->Buffer, sizeof(USHORT));
        *(USHORT *)pStubMsg->Buffer = *(UINT *)pMemory;
        pStubMsg->Buffer += sizeof(USHORT);
        TRACE("value: 0x%04x\n", *(UINT *)pMemory);
        break;
    default:
        FIXME("Unhandled base type: 0x%02x\n", *pFormat);
    }

    STD_OVERFLOW_CHECK(pStubMsg);

    /* FIXME: what is the correct return value? */
    return NULL;
}

/***********************************************************************
 *           NdrBaseTypeUnmarshall [internal]
 */
static unsigned char *WINAPI NdrBaseTypeUnmarshall(
    PMIDL_STUB_MESSAGE pStubMsg,
    unsigned char **ppMemory,
    PFORMAT_STRING pFormat,
    unsigned char fMustAlloc)
{
    TRACE("pStubMsg: %p, ppMemory: %p, type: 0x%02x, fMustAlloc: %s\n", pStubMsg, ppMemory, *pFormat, fMustAlloc ? "true" : "false");

#define BASE_TYPE_UNMARSHALL(type) \
        ALIGN_POINTER(pStubMsg->Buffer, sizeof(type)); \
        if (fMustAlloc || !*ppMemory) \
            *ppMemory = NdrAllocate(pStubMsg, sizeof(type)); \
        TRACE("*ppMemory: %p\n", *ppMemory); \
        **(type **)ppMemory = *(type *)pStubMsg->Buffer; \
        pStubMsg->Buffer += sizeof(type);

    switch(*pFormat)
    {
    case RPC_FC_BYTE:
    case RPC_FC_CHAR:
    case RPC_FC_SMALL:
    case RPC_FC_USMALL:
        BASE_TYPE_UNMARSHALL(UCHAR);
        TRACE("value: 0x%02x\n", **(UCHAR **)ppMemory);
        break;
    case RPC_FC_WCHAR:
    case RPC_FC_SHORT:
    case RPC_FC_USHORT:
        BASE_TYPE_UNMARSHALL(USHORT);
        TRACE("value: 0x%04x\n", **(USHORT **)ppMemory);
        break;
    case RPC_FC_LONG:
    case RPC_FC_ULONG:
    case RPC_FC_ERROR_STATUS_T:
    case RPC_FC_ENUM32:
        BASE_TYPE_UNMARSHALL(ULONG);
        TRACE("value: 0x%08x\n", **(ULONG **)ppMemory);
        break;
   case RPC_FC_FLOAT:
        BASE_TYPE_UNMARSHALL(float);
        TRACE("value: %f\n", **(float **)ppMemory);
        break;
    case RPC_FC_DOUBLE:
        BASE_TYPE_UNMARSHALL(double);
        TRACE("value: %f\n", **(double **)ppMemory);
        break;
    case RPC_FC_HYPER:
        BASE_TYPE_UNMARSHALL(ULONGLONG);
        TRACE("value: %s\n", wine_dbgstr_longlong(**(ULONGLONG **)ppMemory));
        break;
    case RPC_FC_ENUM16:
        ALIGN_POINTER(pStubMsg->Buffer, sizeof(USHORT));
        if (fMustAlloc || !*ppMemory)
            *ppMemory = NdrAllocate(pStubMsg, sizeof(UINT));
        TRACE("*ppMemory: %p\n", *ppMemory);
        /* 16-bits on the wire, but int in memory */
        **(UINT **)ppMemory = *(USHORT *)pStubMsg->Buffer;
        pStubMsg->Buffer += sizeof(USHORT);
        TRACE("value: 0x%08x\n", **(UINT **)ppMemory);
        break;
    default:
        FIXME("Unhandled base type: 0x%02x\n", *pFormat);
    }
#undef BASE_TYPE_UNMARSHALL

    /* FIXME: what is the correct return value? */

    return NULL;
}

/***********************************************************************
 *           NdrBaseTypeBufferSize [internal]
 */
static void WINAPI NdrBaseTypeBufferSize(
    PMIDL_STUB_MESSAGE pStubMsg,
    unsigned char *pMemory,
    PFORMAT_STRING pFormat)
{
    TRACE("pStubMsg %p, pMemory %p, type 0x%02x\n", pStubMsg, pMemory, *pFormat);

    switch(*pFormat)
    {
    case RPC_FC_BYTE:
    case RPC_FC_CHAR:
    case RPC_FC_SMALL:
    case RPC_FC_USMALL:
        pStubMsg->BufferLength += sizeof(UCHAR);
        break;
    case RPC_FC_WCHAR:
    case RPC_FC_SHORT:
    case RPC_FC_USHORT:
    case RPC_FC_ENUM16:
        ALIGN_LENGTH(pStubMsg->BufferLength, sizeof(USHORT));
        pStubMsg->BufferLength += sizeof(USHORT);
        break;
    case RPC_FC_LONG:
    case RPC_FC_ULONG:
    case RPC_FC_ENUM32:
        ALIGN_LENGTH(pStubMsg->BufferLength, sizeof(ULONG));
        pStubMsg->BufferLength += sizeof(ULONG);
        break;
    case RPC_FC_FLOAT:
        ALIGN_LENGTH(pStubMsg->BufferLength, sizeof(float));
        pStubMsg->BufferLength += sizeof(float);
        break;
    case RPC_FC_DOUBLE:
        ALIGN_LENGTH(pStubMsg->BufferLength, sizeof(double));
        pStubMsg->BufferLength += sizeof(double);
        break;
    case RPC_FC_HYPER:
        ALIGN_LENGTH(pStubMsg->BufferLength, sizeof(ULONGLONG));
        pStubMsg->BufferLength += sizeof(ULONGLONG);
        break;
    case RPC_FC_ERROR_STATUS_T:
        ALIGN_LENGTH(pStubMsg->BufferLength, sizeof(error_status_t));
        pStubMsg->BufferLength += sizeof(error_status_t);
        break;
    default:
        FIXME("Unhandled base type: 0x%02x\n", *pFormat);
    }
}

/***********************************************************************
 *           NdrBaseTypeMemorySize [internal]
 */
static ULONG WINAPI NdrBaseTypeMemorySize(
    PMIDL_STUB_MESSAGE pStubMsg,
    PFORMAT_STRING pFormat)
{
    switch(*pFormat)
    {
    case RPC_FC_BYTE:
    case RPC_FC_CHAR:
    case RPC_FC_SMALL:
    case RPC_FC_USMALL:
        pStubMsg->Buffer += sizeof(UCHAR);
        pStubMsg->MemorySize += sizeof(UCHAR);
        return sizeof(UCHAR);
    case RPC_FC_WCHAR:
    case RPC_FC_SHORT:
    case RPC_FC_USHORT:
        pStubMsg->Buffer += sizeof(USHORT);
        pStubMsg->MemorySize += sizeof(USHORT);
        return sizeof(USHORT);
    case RPC_FC_LONG:
    case RPC_FC_ULONG:
        pStubMsg->Buffer += sizeof(ULONG);
        pStubMsg->MemorySize += sizeof(ULONG);
        return sizeof(ULONG);
    case RPC_FC_FLOAT:
        pStubMsg->Buffer += sizeof(float);
        pStubMsg->MemorySize += sizeof(float);
        return sizeof(float);
    case RPC_FC_DOUBLE:
        pStubMsg->Buffer += sizeof(double);
        pStubMsg->MemorySize += sizeof(double);
        return sizeof(double);
    case RPC_FC_HYPER:
        pStubMsg->Buffer += sizeof(ULONGLONG);
        pStubMsg->MemorySize += sizeof(ULONGLONG);
        return sizeof(ULONGLONG);
    case RPC_FC_ERROR_STATUS_T:
        pStubMsg->Buffer += sizeof(error_status_t);
        pStubMsg->MemorySize += sizeof(error_status_t);
        return sizeof(error_status_t);
    case RPC_FC_ENUM16:
    case RPC_FC_ENUM32:
        pStubMsg->Buffer += sizeof(INT);
        pStubMsg->MemorySize += sizeof(INT);
        return sizeof(INT);
    default:
        FIXME("Unhandled base type: 0x%02x\n", *pFormat);
       return 0;
    }
}

/***********************************************************************
 *           NdrBaseTypeFree [internal]
 */
static void WINAPI NdrBaseTypeFree(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
   TRACE("pStubMsg %p pMemory %p type 0x%02x\n", pStubMsg, pMemory, *pFormat);

   /* nothing to do */
}

/***********************************************************************
 *           NdrContextHandleBufferSize [internal]
 */
static void WINAPI NdrContextHandleBufferSize(
    PMIDL_STUB_MESSAGE pStubMsg,
    unsigned char *pMemory,
    PFORMAT_STRING pFormat)
{
    TRACE("pStubMsg %p, pMemory %p, type 0x%02x\n", pStubMsg, pMemory, *pFormat);

    if (*pFormat != RPC_FC_BIND_CONTEXT)
    {
        ERR("invalid format type %x\n", *pFormat);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
    }
    ALIGN_LENGTH(pStubMsg->BufferLength, 4);
    pStubMsg->BufferLength += cbNDRContext;
}

/***********************************************************************
 *           NdrContextHandleMarshall [internal]
 */
static unsigned char *WINAPI NdrContextHandleMarshall(
    PMIDL_STUB_MESSAGE pStubMsg,
    unsigned char *pMemory,
    PFORMAT_STRING pFormat)
{
    TRACE("pStubMsg %p, pMemory %p, type 0x%02x\n", pStubMsg, pMemory, *pFormat);

    if (*pFormat != RPC_FC_BIND_CONTEXT)
    {
        ERR("invalid format type %x\n", *pFormat);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
    }

    if (pFormat[1] & 0x80)
        NdrClientContextMarshall(pStubMsg, *(NDR_CCONTEXT **)pMemory, FALSE);
    else
        NdrClientContextMarshall(pStubMsg, (NDR_CCONTEXT *)pMemory, FALSE);

    return NULL;
}

/***********************************************************************
 *           NdrContextHandleUnmarshall [internal]
 */
static unsigned char *WINAPI NdrContextHandleUnmarshall(
    PMIDL_STUB_MESSAGE pStubMsg,
    unsigned char **ppMemory,
    PFORMAT_STRING pFormat,
    unsigned char fMustAlloc)
{
    if (*pFormat != RPC_FC_BIND_CONTEXT)
    {
        ERR("invalid format type %x\n", *pFormat);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
    }

  **(NDR_CCONTEXT **)ppMemory = NULL;
  NdrClientContextUnmarshall(pStubMsg, *(NDR_CCONTEXT **)ppMemory, pStubMsg->RpcMsg->Handle);

  return NULL;
}

/***********************************************************************
 *           NdrClientContextMarshall [RPCRT4.@]
 */
void WINAPI NdrClientContextMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                     NDR_CCONTEXT ContextHandle,
                                     int fCheck)
{
    TRACE("(%p, %p, %d)\n", pStubMsg, ContextHandle, fCheck);

    ALIGN_POINTER(pStubMsg->Buffer, 4);

    /* FIXME: what does fCheck do? */
    NDRCContextMarshall(ContextHandle,
                        pStubMsg->Buffer);

    pStubMsg->Buffer += cbNDRContext;
}

/***********************************************************************
 *           NdrClientContextUnmarshall [RPCRT4.@]
 */
void WINAPI NdrClientContextUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                       NDR_CCONTEXT * pContextHandle,
                                       RPC_BINDING_HANDLE BindHandle)
{
    TRACE("(%p, %p, %p)\n", pStubMsg, pContextHandle, BindHandle);

    ALIGN_POINTER(pStubMsg->Buffer, 4);

    NDRCContextUnmarshall(pContextHandle,
                          BindHandle,
                          pStubMsg->Buffer,
                          pStubMsg->RpcMsg->DataRepresentation);

    pStubMsg->Buffer += cbNDRContext;
}

void WINAPI NdrServerContextMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                     NDR_SCONTEXT ContextHandle,
                                     NDR_RUNDOWN RundownRoutine )
{
    FIXME("(%p, %p, %p): stub\n", pStubMsg, ContextHandle, RundownRoutine);
}

NDR_SCONTEXT WINAPI NdrServerContextUnmarshall(PMIDL_STUB_MESSAGE pStubMsg)
{
    FIXME("(%p): stub\n", pStubMsg);
    return NULL;
}

void WINAPI NdrContextHandleSize(PMIDL_STUB_MESSAGE pStubMsg,
                                 unsigned char* pMemory,
                                 PFORMAT_STRING pFormat)
{
    FIXME("(%p, %p, %p): stub\n", pStubMsg, pMemory, pFormat);
}

NDR_SCONTEXT WINAPI NdrContextHandleInitialize(PMIDL_STUB_MESSAGE pStubMsg,
                                               PFORMAT_STRING pFormat)
{
    FIXME("(%p, %p): stub\n", pStubMsg, pFormat);
    return NULL;
}

void WINAPI NdrServerContextNewMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                        NDR_SCONTEXT ContextHandle,
                                        NDR_RUNDOWN RundownRoutine,
                                        PFORMAT_STRING pFormat)
{
    FIXME("(%p, %p, %p, %p): stub\n", pStubMsg, ContextHandle, RundownRoutine, pFormat);
}

NDR_SCONTEXT WINAPI NdrServerContextNewUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                                  PFORMAT_STRING pFormat)
{
    FIXME("(%p, %p): stub\n", pStubMsg, pFormat);
    return NULL;
}

#define NDR_CONTEXT_HANDLE_MAGIC 0x4352444e

typedef struct ndr_context_handle
{
    DWORD      attributes;
    GUID       uuid;
} ndr_context_handle;

struct context_handle_entry
{
    struct list entry;
    DWORD magic;
    RPC_BINDING_HANDLE handle;
    ndr_context_handle wire_data;
};

static struct list context_handle_list = LIST_INIT(context_handle_list);

static CRITICAL_SECTION ndr_context_cs;
static CRITICAL_SECTION_DEBUG ndr_context_debug =
{
    0, 0, &ndr_context_cs,
    { &ndr_context_debug.ProcessLocksList, &ndr_context_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": ndr_context") }
};
static CRITICAL_SECTION ndr_context_cs = { &ndr_context_debug, -1, 0, 0, 0, 0 };

static struct context_handle_entry *get_context_entry(NDR_CCONTEXT CContext)
{
    struct context_handle_entry *che = (struct context_handle_entry*) CContext;

    if (che->magic != NDR_CONTEXT_HANDLE_MAGIC)
        return NULL;
    return che;
}

static struct context_handle_entry *context_entry_from_guid(LPGUID uuid)
{
    struct context_handle_entry *che;
    LIST_FOR_EACH_ENTRY(che, &context_handle_list, struct context_handle_entry, entry)
        if (IsEqualGUID(&che->wire_data.uuid, uuid))
            return che;
    return NULL;
}

RPC_BINDING_HANDLE WINAPI NDRCContextBinding(NDR_CCONTEXT CContext)
{
    struct context_handle_entry *che;
    RPC_BINDING_HANDLE handle = NULL;

    TRACE("%p\n", CContext);

    EnterCriticalSection(&ndr_context_cs);
    che = get_context_entry(CContext);
    if (che)
        handle = che->handle;
    LeaveCriticalSection(&ndr_context_cs);

    if (!handle)
        RpcRaiseException(ERROR_INVALID_HANDLE);
    return handle;
}

void WINAPI NDRCContextMarshall(NDR_CCONTEXT CContext, void *pBuff)
{
    struct context_handle_entry *che;

    TRACE("%p %p\n", CContext, pBuff);

    if (CContext)
    {
        EnterCriticalSection(&ndr_context_cs);
        che = get_context_entry(CContext);
        memcpy(pBuff, &che->wire_data, sizeof (ndr_context_handle));
        LeaveCriticalSection(&ndr_context_cs);
    }
    else
    {
        ndr_context_handle *wire_data = (ndr_context_handle *)pBuff;
        wire_data->attributes = 0;
        wire_data->uuid = GUID_NULL;
    }
}

static UINT ndr_update_context_handle(NDR_CCONTEXT *CContext,
                                      RPC_BINDING_HANDLE hBinding,
                                      ndr_context_handle *chi)
{
    struct context_handle_entry *che = NULL;

    /* a null UUID means we should free the context handle */
    if (IsEqualGUID(&chi->uuid, &GUID_NULL))
    {
        if (*CContext)
        {
            che = get_context_entry(*CContext);
            if (!che)
                return ERROR_INVALID_HANDLE;
            list_remove(&che->entry);
            RpcBindingFree(&che->handle);
            HeapFree(GetProcessHeap(), 0, che);
            che = NULL;
        }
    }
    /* if there's no existing entry matching the GUID, allocate one */
    else if (!(che = context_entry_from_guid(&chi->uuid)))
    {
        che = HeapAlloc(GetProcessHeap(), 0, sizeof *che);
        if (!che)
            return ERROR_NOT_ENOUGH_MEMORY;
        che->magic = NDR_CONTEXT_HANDLE_MAGIC;
        RpcBindingCopy(hBinding, &che->handle);
        list_add_tail(&context_handle_list, &che->entry);
        memcpy(&che->wire_data, chi, sizeof *chi);
    }

    *CContext = che;

    return ERROR_SUCCESS;
}

/***********************************************************************
 *           NDRCContextUnmarshall [RPCRT4.@]
 */
void WINAPI NDRCContextUnmarshall(NDR_CCONTEXT *CContext,
                                  RPC_BINDING_HANDLE hBinding,
                                  void *pBuff, ULONG DataRepresentation)
{
    UINT r;

    TRACE("*%p=(%p) %p %p %08x\n",
          CContext, *CContext, hBinding, pBuff, DataRepresentation);

    EnterCriticalSection(&ndr_context_cs);
    r = ndr_update_context_handle(CContext, hBinding, pBuff);
    LeaveCriticalSection(&ndr_context_cs);
    if (r)
        RpcRaiseException(r);
}

/***********************************************************************
 *           NDRSContextMarshall [RPCRT4.@]
 */
void WINAPI NDRSContextMarshall(NDR_SCONTEXT CContext,
                               void *pBuff,
                               NDR_RUNDOWN userRunDownIn)
{
    FIXME("(%p %p %p): stub\n", CContext, pBuff, userRunDownIn);
}

/***********************************************************************
 *           NDRSContextMarshallEx [RPCRT4.@]
 */
void WINAPI NDRSContextMarshallEx(RPC_BINDING_HANDLE hBinding,
                                  NDR_SCONTEXT CContext,
                                  void *pBuff,
                                  NDR_RUNDOWN userRunDownIn)
{
    FIXME("(%p %p %p %p): stub\n", hBinding, CContext, pBuff, userRunDownIn);
}

/***********************************************************************
 *           NDRSContextMarshall2 [RPCRT4.@]
 */
void WINAPI NDRSContextMarshall2(RPC_BINDING_HANDLE hBinding,
                                 NDR_SCONTEXT CContext,
                                 void *pBuff,
                                 NDR_RUNDOWN userRunDownIn,
                                 void *CtxGuard, ULONG Flags)
{
    FIXME("(%p %p %p %p %p %u): stub\n",
          hBinding, CContext, pBuff, userRunDownIn, CtxGuard, Flags);
}

/***********************************************************************
 *           NDRSContextUnmarshall [RPCRT4.@]
 */
NDR_SCONTEXT WINAPI NDRSContextUnmarshall(void *pBuff,
                                          ULONG DataRepresentation)
{
    FIXME("(%p %08x): stub\n", pBuff, DataRepresentation);
    return NULL;
}

/***********************************************************************
 *           NDRSContextUnmarshallEx [RPCRT4.@]
 */
NDR_SCONTEXT WINAPI NDRSContextUnmarshallEx(RPC_BINDING_HANDLE hBinding,
                                            void *pBuff,
                                            ULONG DataRepresentation)
{
    FIXME("(%p %p %08x): stub\n", hBinding, pBuff, DataRepresentation);
    return NULL;
}

/***********************************************************************
 *           NDRSContextUnmarshall2 [RPCRT4.@]
 */
NDR_SCONTEXT WINAPI NDRSContextUnmarshall2(RPC_BINDING_HANDLE hBinding,
                                           void *pBuff,
                                           ULONG DataRepresentation,
                                           void *CtxGuard, ULONG Flags)
{
    FIXME("(%p %p %08x %p %u): stub\n",
          hBinding, pBuff, DataRepresentation, CtxGuard, Flags);
    return NULL;
}
