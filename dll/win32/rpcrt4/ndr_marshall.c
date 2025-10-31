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
 *  - String structs
 *  - Byte count pointers
 *  - transmit_as/represent as
 *  - Multi-dimensional arrays
 *  - Conversion functions (NdrConvert)
 *  - Checks for integer addition overflow in user marshall functions
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"

#include "ndr_misc.h"
#include "rpcndr.h"
#include "ndrtypes.h"

#include "wine/debug.h"

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
     *((pchar)+3) = HIBYTE(HIWORD(uint32)))

# define LITTLE_ENDIAN_UINT32_READ(pchar) \
    (MAKELONG( \
      MAKEWORD(*(pchar), *((pchar)+1)), \
      MAKEWORD(*((pchar)+2), *((pchar)+3))))
#endif

#define BIG_ENDIAN_UINT32_WRITE(pchar, uint32) \
  (*((pchar)+3) = LOBYTE(LOWORD(uint32)), \
   *((pchar)+2) = HIBYTE(LOWORD(uint32)), \
   *((pchar)+1) = LOBYTE(HIWORD(uint32)), \
   *(pchar)     = HIBYTE(HIWORD(uint32)))

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

static inline void align_length( ULONG *len, unsigned int align )
{
    *len = (*len + align - 1) & ~(align - 1);
}

static inline void align_pointer( unsigned char **ptr, unsigned int align )
{
    ULONG_PTR mask = align - 1;
    *ptr = (unsigned char *)(((ULONG_PTR)*ptr + mask) & ~mask);
}

static inline void align_pointer_clear( unsigned char **ptr, unsigned int align )
{
    ULONG_PTR mask = align - 1;
    memset( *ptr, 0, (align - (ULONG_PTR)*ptr) & mask );
    *ptr = (unsigned char *)(((ULONG_PTR)*ptr + mask) & ~mask);
}

static inline void align_pointer_offset( unsigned char **ptr, unsigned char *base, unsigned int align )
{
    ULONG_PTR mask = align - 1;
    *ptr = base + (((ULONG_PTR)(*ptr - base) + mask) & ~mask);
}

static inline void align_pointer_offset_clear( unsigned char **ptr, unsigned char *base, unsigned int align )
{
    ULONG_PTR mask = align - 1;
    memset( *ptr, 0, (align - (ULONG_PTR)(*ptr - base)) & mask );
    *ptr = base + (((ULONG_PTR)(*ptr - base) + mask) & ~mask);
}

#define STD_OVERFLOW_CHECK(_Msg) do { \
    TRACE("buffer=%Id/%ld\n", _Msg->Buffer - (unsigned char *)_Msg->RpcMsg->Buffer, _Msg->BufferLength); \
    if (_Msg->Buffer > (unsigned char *)_Msg->RpcMsg->Buffer + _Msg->BufferLength) \
        ERR("buffer overflow %Id bytes\n", _Msg->Buffer - ((unsigned char *)_Msg->RpcMsg->Buffer + _Msg->BufferLength)); \
  } while (0)

#define NDR_POINTER_ID_BASE 0x20000
#define NDR_POINTER_ID(pStubMsg) (NDR_POINTER_ID_BASE + ((pStubMsg)->UniquePtrCount++) * 4)
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

static unsigned char *WINAPI NdrRangeMarshall(PMIDL_STUB_MESSAGE,unsigned char *, PFORMAT_STRING);
static void WINAPI NdrRangeBufferSize(PMIDL_STUB_MESSAGE, unsigned char *, PFORMAT_STRING);
static ULONG WINAPI NdrRangeMemorySize(PMIDL_STUB_MESSAGE, PFORMAT_STRING);
static void WINAPI NdrRangeFree(PMIDL_STUB_MESSAGE, unsigned char *, PFORMAT_STRING);

static ULONG WINAPI NdrByteCountPointerMemorySize(PMIDL_STUB_MESSAGE, PFORMAT_STRING);

static unsigned char * ComplexBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                         unsigned char *pMemory,
                                         PFORMAT_STRING pFormat,
                                         PFORMAT_STRING pPointer);
static unsigned char * ComplexMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                       unsigned char *pMemory,
                                       PFORMAT_STRING pFormat,
                                       PFORMAT_STRING pPointer);
static unsigned char * ComplexUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                         unsigned char *pMemory,
                                         PFORMAT_STRING pFormat,
                                         PFORMAT_STRING pPointer,
                                         unsigned char fMustAlloc);
static ULONG ComplexStructMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                     PFORMAT_STRING pFormat,
                                     PFORMAT_STRING pPointer);
static unsigned char * ComplexFree(PMIDL_STUB_MESSAGE pStubMsg,
                                   unsigned char *pMemory,
                                   PFORMAT_STRING pFormat,
                                   PFORMAT_STRING pPointer);

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
  NdrRangeMarshall,
  NdrBaseTypeMarshall,
  NdrBaseTypeMarshall
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
  NdrRangeUnmarshall,
  NdrBaseTypeUnmarshall,
  NdrBaseTypeUnmarshall
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
  NdrRangeBufferSize,
  NdrBaseTypeBufferSize,
  NdrBaseTypeBufferSize
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
  NdrRangeMemorySize,
  NdrBaseTypeMemorySize,
  NdrBaseTypeMemorySize
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
  NdrRangeFree,
  NdrBaseTypeFree,
  NdrBaseTypeFree
};

typedef struct _NDR_MEMORY_LIST
{
    ULONG magic;
    ULONG size;
    ULONG reserved;
    struct _NDR_MEMORY_LIST *next;
} NDR_MEMORY_LIST;

#define MEML_MAGIC  ('M' << 24 | 'E' << 16 | 'M' << 8 | 'L')

/***********************************************************************
 *            NdrAllocate [RPCRT4.@]
 *
 * Allocates a block of memory using pStubMsg->pfnAllocate.
 *
 * PARAMS
 *  pStubMsg [I/O] MIDL_STUB_MESSAGE structure.
 *  len      [I]   Size of memory block to allocate.
 *
 * RETURNS
 *  The memory block of size len that was allocated.
 *
 * NOTES
 *  The memory block is always 8-byte aligned.
 *  If the function is unable to allocate memory an RPC_X_NO_MEMORY
 *  exception is raised.
 */
void * WINAPI NdrAllocate(MIDL_STUB_MESSAGE *pStubMsg, SIZE_T len)
{
    SIZE_T aligned_len;
    SIZE_T adjusted_len;
    void *p;
    NDR_MEMORY_LIST *mem_list;

    aligned_len = (len + 7) & ~7;
    adjusted_len = aligned_len + sizeof(NDR_MEMORY_LIST);
    /* check for overflow */
    if (adjusted_len < len)
    {
        ERR("overflow of adjusted_len %Id, len %Id\n", adjusted_len, len);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    }

    p = pStubMsg->pfnAllocate(adjusted_len);
    if (!p) RpcRaiseException(RPC_X_NO_MEMORY);

    mem_list = (NDR_MEMORY_LIST *)((char *)p + aligned_len);
    mem_list->magic = MEML_MAGIC;
    mem_list->size = aligned_len;
    mem_list->reserved = 0;
    mem_list->next = pStubMsg->pMemoryList;
    pStubMsg->pMemoryList = mem_list;

    TRACE("-- %p\n", p);
    return p;
}

static void *NdrAllocateZero(MIDL_STUB_MESSAGE *stubmsg, SIZE_T len)
{
    void *mem = NdrAllocate(stubmsg, len);
    memset(mem, 0, len);
    return mem;
}

static void NdrFree(MIDL_STUB_MESSAGE *pStubMsg, unsigned char *Pointer)
{
    TRACE("(%p, %p)\n", pStubMsg, Pointer);

    pStubMsg->pfnFree(Pointer);
}

static inline BOOL IsConformanceOrVariancePresent(PFORMAT_STRING pFormat)
{
    return (*(const ULONG *)pFormat != -1);
}

static inline PFORMAT_STRING SkipConformance(const PMIDL_STUB_MESSAGE pStubMsg, const PFORMAT_STRING pFormat)
{
    return pFormat + 4 + pStubMsg->CorrDespIncrement;
}

static PFORMAT_STRING ReadConformance(MIDL_STUB_MESSAGE *pStubMsg, PFORMAT_STRING pFormat)
{
  align_pointer(&pStubMsg->Buffer, 4);
  if (pStubMsg->Buffer + 4 > pStubMsg->BufferEnd)
      RpcRaiseException(RPC_X_BAD_STUB_DATA);
  pStubMsg->MaxCount = NDR_LOCAL_UINT32_READ(pStubMsg->Buffer);
  pStubMsg->Buffer += 4;
  TRACE("unmarshalled conformance is %Id\n", pStubMsg->MaxCount);
  return SkipConformance(pStubMsg, pFormat);
}

static inline PFORMAT_STRING ReadVariance(MIDL_STUB_MESSAGE *pStubMsg, PFORMAT_STRING pFormat, ULONG MaxValue)
{
  if (pFormat && !IsConformanceOrVariancePresent(pFormat))
  {
    pStubMsg->Offset = 0;
    pStubMsg->ActualCount = pStubMsg->MaxCount;
    goto done;
  }

  align_pointer(&pStubMsg->Buffer, 4);
  if (pStubMsg->Buffer + 8 > pStubMsg->BufferEnd)
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
  pStubMsg->Offset      = NDR_LOCAL_UINT32_READ(pStubMsg->Buffer);
  pStubMsg->Buffer += 4;
  TRACE("offset is %ld\n", pStubMsg->Offset);
  pStubMsg->ActualCount = NDR_LOCAL_UINT32_READ(pStubMsg->Buffer);
  pStubMsg->Buffer += 4;
  TRACE("variance is %ld\n", pStubMsg->ActualCount);

  if ((pStubMsg->ActualCount > MaxValue) ||
      (pStubMsg->ActualCount + pStubMsg->Offset > MaxValue))
  {
    ERR("invalid array bound(s): ActualCount = %ld, Offset = %ld, MaxValue = %ld\n",
        pStubMsg->ActualCount, pStubMsg->Offset, MaxValue);
    RpcRaiseException(RPC_S_INVALID_BOUND);
    return NULL;
  }

done:
  return SkipConformance(pStubMsg, pFormat);
}

/* writes the conformance value to the buffer */
static inline void WriteConformance(MIDL_STUB_MESSAGE *pStubMsg)
{
    align_pointer_clear(&pStubMsg->Buffer, 4);
    if (pStubMsg->Buffer + 4 > (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength)
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    NDR_LOCAL_UINT32_WRITE(pStubMsg->Buffer, pStubMsg->MaxCount);
    pStubMsg->Buffer += 4;
}

/* writes the variance values to the buffer */
static inline void WriteVariance(MIDL_STUB_MESSAGE *pStubMsg)
{
    align_pointer_clear(&pStubMsg->Buffer, 4);
    if (pStubMsg->Buffer + 8 > (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength)
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    NDR_LOCAL_UINT32_WRITE(pStubMsg->Buffer, pStubMsg->Offset);
    pStubMsg->Buffer += 4;
    NDR_LOCAL_UINT32_WRITE(pStubMsg->Buffer, pStubMsg->ActualCount);
    pStubMsg->Buffer += 4;
}

/* requests buffer space for the conformance value */
static inline void SizeConformance(MIDL_STUB_MESSAGE *pStubMsg)
{
    align_length(&pStubMsg->BufferLength, 4);
    if (pStubMsg->BufferLength + 4 < pStubMsg->BufferLength)
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    pStubMsg->BufferLength += 4;
}

/* requests buffer space for the variance values */
static inline void SizeVariance(MIDL_STUB_MESSAGE *pStubMsg)
{
    align_length(&pStubMsg->BufferLength, 4);
    if (pStubMsg->BufferLength + 8 < pStubMsg->BufferLength)
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    pStubMsg->BufferLength += 8;
}

PFORMAT_STRING ComputeConformanceOrVariance(
    MIDL_STUB_MESSAGE *pStubMsg, unsigned char *pMemory,
    PFORMAT_STRING pFormat, ULONG_PTR def, ULONG_PTR *pCount)
{
  BYTE dtype = pFormat[0] & 0xf;
  short ofs = *(const short *)&pFormat[2];
  LPVOID ptr = NULL;
  ULONG_PTR data = 0;

  if (!IsConformanceOrVariancePresent(pFormat)) {
    /* null descriptor */
    *pCount = def;
    goto finish_conf;
  }

  switch (pFormat[0] & 0xf0) {
  case FC_NORMAL_CONFORMANCE:
    TRACE("normal conformance, ofs=%d\n", ofs);
    ptr = pMemory;
    break;
  case FC_POINTER_CONFORMANCE:
    TRACE("pointer conformance, ofs=%d\n", ofs);
    ptr = pStubMsg->Memory;
    break;
  case FC_TOP_LEVEL_CONFORMANCE:
    TRACE("toplevel conformance, ofs=%d\n", ofs);
    if (pStubMsg->StackTop) {
      ptr = pStubMsg->StackTop;
    }
    else {
      /* -Os mode, *pCount is already set */
      goto finish_conf;
    }
    break;
  case FC_CONSTANT_CONFORMANCE:
    data = ofs | ((DWORD)pFormat[1] << 16);
    TRACE("constant conformance, val=%Id\n", data);
    *pCount = data;
    goto finish_conf;
  case FC_TOP_LEVEL_MULTID_CONFORMANCE:
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
    FIXME("unknown conformance type %x, expect crash.\n", pFormat[0] & 0xf0);
    goto finish_conf;
  }

  switch (pFormat[1]) {
  case FC_DEREFERENCE:
    ptr = *(LPVOID*)((char *)ptr + ofs);
    break;
  case FC_CALLBACK:
  {
    unsigned char *old_stack_top = pStubMsg->StackTop;
    ULONG_PTR max_count, old_max_count = pStubMsg->MaxCount;

    pStubMsg->StackTop = ptr;

    /* ofs is index into StubDesc->apfnExprEval */
    TRACE("callback conformance into apfnExprEval[%d]\n", ofs);
    pStubMsg->StubDesc->apfnExprEval[ofs](pStubMsg);

    pStubMsg->StackTop = old_stack_top;

    /* the callback function always stores the computed value in MaxCount */
    max_count = pStubMsg->MaxCount;
    pStubMsg->MaxCount = old_max_count;
    *pCount = max_count;
    goto finish_conf;
  }
  default:
    ptr = (char *)ptr + ofs;
    break;
  }

  switch (dtype) {
  case FC_LONG:
  case FC_ULONG:
    data = *(DWORD*)ptr;
    break;
  case FC_SHORT:
    data = *(SHORT*)ptr;
    break;
  case FC_USHORT:
    data = *(USHORT*)ptr;
    break;
  case FC_CHAR:
  case FC_SMALL:
    data = *(CHAR*)ptr;
    break;
  case FC_BYTE:
  case FC_USMALL:
    data = *(UCHAR*)ptr;
    break;
  case FC_HYPER:
    data = *(ULONGLONG *)ptr;
    break;
  default:
    FIXME("unknown conformance data type %x\n", dtype);
    goto done_conf_grab;
  }
  TRACE("dereferenced data type %x at %p, got %Id\n", dtype, ptr, data);

done_conf_grab:
  switch (pFormat[1]) {
  case FC_DEREFERENCE: /* already handled */
  case 0: /* no op */
    *pCount = data;
    break;
  case FC_ADD_1:
    *pCount = data + 1;
    break;
  case FC_SUB_1:
    *pCount = data - 1;
    break;
  case FC_MULT_2:
    *pCount = data * 2;
    break;
  case FC_DIV_2:
    *pCount = data / 2;
    break;
  default:
    FIXME("unknown conformance op %d\n", pFormat[1]);
    goto finish_conf;
  }

finish_conf:
  TRACE("resulting conformance is %Id\n", *pCount);

  return SkipConformance(pStubMsg, pFormat);
}

static inline PFORMAT_STRING SkipVariance(PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat)
{
    return SkipConformance( pStubMsg, pFormat );
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

static inline void safe_buffer_increment(MIDL_STUB_MESSAGE *pStubMsg, ULONG size)
{
    if ((pStubMsg->Buffer + size < pStubMsg->Buffer) || /* integer overflow of pStubMsg->Buffer */
        (pStubMsg->Buffer + size > (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength))
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    pStubMsg->Buffer += size;
}

static inline void safe_buffer_length_increment(MIDL_STUB_MESSAGE *pStubMsg, ULONG size)
{
    if (pStubMsg->BufferLength + size < pStubMsg->BufferLength) /* integer overflow of pStubMsg->BufferSize */
    {
        ERR("buffer length overflow - BufferLength = %lu, size = %lu\n",
            pStubMsg->BufferLength, size);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    }
    pStubMsg->BufferLength += size;
}

/* copies data from the buffer, checking that there is enough data in the buffer
 * to do so */
static inline void safe_copy_from_buffer(MIDL_STUB_MESSAGE *pStubMsg, void *p, ULONG size)
{
    if ((pStubMsg->Buffer + size < pStubMsg->Buffer) || /* integer overflow of pStubMsg->Buffer */
        (pStubMsg->Buffer + size > pStubMsg->BufferEnd))
    {
        ERR("buffer overflow - Buffer = %p, BufferEnd = %p, size = %lu\n",
            pStubMsg->Buffer, pStubMsg->BufferEnd, size);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    }
    if (p == pStubMsg->Buffer)
        ERR("pointer is the same as the buffer\n");
    memcpy(p, pStubMsg->Buffer, size);
    pStubMsg->Buffer += size;
}

/* copies data to the buffer, checking that there is enough space to do so */
static inline void safe_copy_to_buffer(MIDL_STUB_MESSAGE *pStubMsg, const void *p, ULONG size)
{
    if ((pStubMsg->Buffer + size < pStubMsg->Buffer) || /* integer overflow of pStubMsg->Buffer */
        (pStubMsg->Buffer + size > (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength))
    {
        ERR("buffer overflow - Buffer = %p, BufferEnd = %p, size = %lu\n",
            pStubMsg->Buffer, (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength,
            size);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    }
    memcpy(pStubMsg->Buffer, p, size);
    pStubMsg->Buffer += size;
}

/* verify that string data sitting in the buffer is valid and safe to
 * unmarshall */
static void validate_string_data(MIDL_STUB_MESSAGE *pStubMsg, ULONG bufsize, ULONG esize)
{
    ULONG i;

    /* verify the buffer is safe to access */
    if ((pStubMsg->Buffer + bufsize < pStubMsg->Buffer) ||
        (pStubMsg->Buffer + bufsize > pStubMsg->BufferEnd))
    {
        ERR("bufsize 0x%lx exceeded buffer end %p of buffer %p\n", bufsize,
            pStubMsg->BufferEnd, pStubMsg->Buffer);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    }

    /* strings must always have null terminating bytes */
    if (bufsize < esize)
    {
        ERR("invalid string length of %ld\n", bufsize / esize);
        RpcRaiseException(RPC_S_INVALID_BOUND);
    }

    for (i = bufsize - esize; i < bufsize; i++)
        if (pStubMsg->Buffer[i] != 0)
        {
            ERR("string not null-terminated at byte position %ld, data is 0x%x\n",
                i, pStubMsg->Buffer[i]);
            RpcRaiseException(RPC_S_INVALID_BOUND);
        }
}

static inline void dump_pointer_attr(unsigned char attr)
{
    if (attr & FC_ALLOCATE_ALL_NODES)
        TRACE(" FC_ALLOCATE_ALL_NODES");
    if (attr & FC_DONT_FREE)
        TRACE(" FC_DONT_FREE");
    if (attr & FC_ALLOCED_ON_STACK)
        TRACE(" FC_ALLOCED_ON_STACK");
    if (attr & FC_SIMPLE_POINTER)
        TRACE(" FC_SIMPLE_POINTER");
    if (attr & FC_POINTER_DEREF)
        TRACE(" FC_POINTER_DEREF");
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
  BOOL pointer_needs_marshaling;

  TRACE("(%p,%p,%p,%p)\n", pStubMsg, Buffer, Pointer, pFormat);
  TRACE("type=0x%x, attr=", type); dump_pointer_attr(attr);
  pFormat += 2;
  if (attr & FC_SIMPLE_POINTER) desc = pFormat;
  else desc = pFormat + *(const SHORT*)pFormat;

  switch (type) {
  case FC_RP: /* ref pointer (always non-null) */
    if (!Pointer)
    {
      ERR("NULL ref pointer is not allowed\n");
      RpcRaiseException(RPC_X_NULL_REF_POINTER);
    }
    pointer_needs_marshaling = TRUE;
    break;
  case FC_UP: /* unique pointer */
  case FC_OP: /* object pointer - same as unique here */
    if (Pointer)
      pointer_needs_marshaling = TRUE;
    else
      pointer_needs_marshaling = FALSE;
    pointer_id = Pointer ? NDR_POINTER_ID(pStubMsg) : 0;
    TRACE("writing 0x%08lx to buffer\n", pointer_id);
    NDR_LOCAL_UINT32_WRITE(Buffer, pointer_id);
    break;
  case FC_FP:
    pointer_needs_marshaling = !NdrFullPointerQueryPointer(
      pStubMsg->FullPtrXlatTables, Pointer, 1, &pointer_id);
    TRACE("writing 0x%08lx to buffer\n", pointer_id);
    NDR_LOCAL_UINT32_WRITE(Buffer, pointer_id);
    break;
  default:
    FIXME("unhandled ptr type=%02x\n", type);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
    return;
  }

  TRACE("calling marshaller for type 0x%x\n", (int)*desc);

  if (pointer_needs_marshaling) {
    if (attr & FC_POINTER_DEREF) {
      Pointer = *(unsigned char**)Pointer;
      TRACE("deref => %p\n", Pointer);
    }
    m = NdrMarshaller[*desc & NDR_TABLE_MASK];
    if (m) m(pStubMsg, Pointer, desc);
    else FIXME("no marshaller for data type=%02x\n", *desc);
  }

  STD_OVERFLOW_CHECK(pStubMsg);
}

/* pPointer is the pointer that we will unmarshal into; pSrcPointer is the
 * pointer to memory which we may attempt to reuse if non-NULL. Usually these
 * are the same; for the case when they aren't, see EmbeddedPointerUnmarshall().
 *
 * fMustAlloc seems to determine whether we can allocate from the buffer (if we
 * are on the server side). It's ignored here, since we can't allocate a pointer
 * from the buffer. */
static void PointerUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                              unsigned char *Buffer,
                              unsigned char **pPointer,
                              unsigned char *pSrcPointer,
                              PFORMAT_STRING pFormat,
                              unsigned char fMustAlloc)
{
  unsigned type = pFormat[0], attr = pFormat[1];
  PFORMAT_STRING desc;
  NDR_UNMARSHALL m;
  DWORD pointer_id = 0;
  BOOL pointer_needs_unmarshaling, need_alloc = FALSE, inner_must_alloc = FALSE;

  TRACE("(%p,%p,%p,%p,%p,%d)\n", pStubMsg, Buffer, pPointer, pSrcPointer, pFormat, fMustAlloc);
  TRACE("type=0x%x, attr=", type); dump_pointer_attr(attr);
  pFormat += 2;
  if (attr & FC_SIMPLE_POINTER) desc = pFormat;
  else desc = pFormat + *(const SHORT*)pFormat;

  switch (type) {
  case FC_RP: /* ref pointer (always non-null) */
    pointer_needs_unmarshaling = TRUE;
    break;
  case FC_UP: /* unique pointer */
    pointer_id = NDR_LOCAL_UINT32_READ(Buffer);
    TRACE("pointer_id is 0x%08lx\n", pointer_id);
    if (pointer_id)
      pointer_needs_unmarshaling = TRUE;
    else {
      *pPointer = NULL;
      pointer_needs_unmarshaling = FALSE;
    }
    break;
  case FC_OP: /* object pointer - we must free data before overwriting it */
    pointer_id = NDR_LOCAL_UINT32_READ(Buffer);
    TRACE("pointer_id is 0x%08lx\n", pointer_id);

    /* An object pointer always allocates new memory (it cannot point to the
     * buffer). */
    inner_must_alloc = TRUE;

    if (pSrcPointer)
        FIXME("free object pointer %p\n", pSrcPointer);
    if (pointer_id)
      pointer_needs_unmarshaling = TRUE;
    else
    {
      *pPointer = NULL;    
      pointer_needs_unmarshaling = FALSE;
    }
    break;
  case FC_FP:
    pointer_id = NDR_LOCAL_UINT32_READ(Buffer);
    TRACE("pointer_id is 0x%08lx\n", pointer_id);
    pointer_needs_unmarshaling = !NdrFullPointerQueryRefId(
      pStubMsg->FullPtrXlatTables, pointer_id, 1, (void **)pPointer);
    break;
  default:
    FIXME("unhandled ptr type=%02x\n", type);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
    return;
  }

  if (pointer_needs_unmarshaling) {
    unsigned char **current_ptr = pPointer;
    if (pStubMsg->IsClient) {
      TRACE("client\n");
      /* Try to use the existing (source) pointer to unmarshall the data into
       * so that [in, out] or [out, ref] parameters behave correctly. If the
       * source pointer is NULL and we are not dereferencing, we must force the
       * inner marshalling routine to allocate, since otherwise it will crash. */
      if (pSrcPointer)
      {
        TRACE("setting *pPointer to %p\n", pSrcPointer);
        *pPointer = pSrcPointer;
      }
      else
        need_alloc = inner_must_alloc = TRUE;
    } else {
      TRACE("server\n");
      /* We can use an existing source pointer here only if it is on-stack,
       * probably since otherwise NdrPointerFree() might later try to free a
       * pointer we don't know the provenance of. Otherwise we must always
       * allocate if we are dereferencing. We never need to force the inner
       * routine to allocate here, since it will either write into an existing
       * pointer, or use a pointer to the buffer. */
      if (attr & FC_POINTER_DEREF)
      {
        if (pSrcPointer && (attr & FC_ALLOCED_ON_STACK))
          *pPointer = pSrcPointer;
        else
          need_alloc = TRUE;
      }
      else
        *pPointer = NULL;
    }

    if (attr & FC_ALLOCATE_ALL_NODES)
        FIXME("FC_ALLOCATE_ALL_NODES not implemented\n");

    if (attr & FC_POINTER_DEREF) {
      if (need_alloc)
        *pPointer = NdrAllocateZero(pStubMsg, sizeof(void *));

      current_ptr = *(unsigned char***)current_ptr;
      TRACE("deref => %p\n", current_ptr);
    }
    m = NdrUnmarshaller[*desc & NDR_TABLE_MASK];
    if (m) m(pStubMsg, current_ptr, desc, inner_must_alloc);
    else FIXME("no unmarshaller for data type=%02x\n", *desc);

    if (type == FC_FP)
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
  BOOL pointer_needs_sizing;
  ULONG pointer_id;

  TRACE("(%p,%p,%p)\n", pStubMsg, Pointer, pFormat);
  TRACE("type=0x%x, attr=", type); dump_pointer_attr(attr);
  pFormat += 2;
  if (attr & FC_SIMPLE_POINTER) desc = pFormat;
  else desc = pFormat + *(const SHORT*)pFormat;

  switch (type) {
  case FC_RP: /* ref pointer (always non-null) */
    if (!Pointer)
    {
      ERR("NULL ref pointer is not allowed\n");
      RpcRaiseException(RPC_X_NULL_REF_POINTER);
    }
    break;
  case FC_OP:
  case FC_UP:
    /* NULL pointer has no further representation */
    if (!Pointer)
        return;
    break;
  case FC_FP:
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

  if (attr & FC_POINTER_DEREF) {
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
static ULONG PointerMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                               unsigned char *Buffer, PFORMAT_STRING pFormat)
{
  unsigned type = pFormat[0], attr = pFormat[1];
  PFORMAT_STRING desc;
  NDR_MEMORYSIZE m;
  DWORD pointer_id = 0;
  BOOL pointer_needs_sizing;

  TRACE("(%p,%p,%p)\n", pStubMsg, Buffer, pFormat);
  TRACE("type=0x%x, attr=", type); dump_pointer_attr(attr);
  pFormat += 2;
  if (attr & FC_SIMPLE_POINTER) desc = pFormat;
  else desc = pFormat + *(const SHORT*)pFormat;

  switch (type) {
  case FC_RP: /* ref pointer (always non-null) */
    pointer_needs_sizing = TRUE;
    break;
  case FC_UP: /* unique pointer */
  case FC_OP: /* object pointer - we must free data before overwriting it */
    pointer_id = NDR_LOCAL_UINT32_READ(Buffer);
    TRACE("pointer_id is 0x%08lx\n", pointer_id);
    if (pointer_id)
      pointer_needs_sizing = TRUE;
    else
      pointer_needs_sizing = FALSE;
    break;
  case FC_FP:
  {
    void *pointer;
    pointer_id = NDR_LOCAL_UINT32_READ(Buffer);
    TRACE("pointer_id is 0x%08lx\n", pointer_id);
    pointer_needs_sizing = !NdrFullPointerQueryRefId(
      pStubMsg->FullPtrXlatTables, pointer_id, 1, &pointer);
    break;
  }
  default:
    FIXME("unhandled ptr type=%02x\n", type);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
    return 0;
  }

  if (attr & FC_POINTER_DEREF) {
    align_length(&pStubMsg->MemorySize, sizeof(void*));
    pStubMsg->MemorySize += sizeof(void*);
    TRACE("deref\n");
  }

  if (pointer_needs_sizing) {
    m = NdrMemorySizer[*desc & NDR_TABLE_MASK];
    if (m) m(pStubMsg, desc);
    else FIXME("no memorysizer for data type=%02x\n", *desc);
  }

  return pStubMsg->MemorySize;
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
  unsigned char *current_pointer = Pointer;

  TRACE("(%p,%p,%p)\n", pStubMsg, Pointer, pFormat);
  TRACE("type=0x%x, attr=", type); dump_pointer_attr(attr);
  if (attr & FC_DONT_FREE) return;
  pFormat += 2;
  if (attr & FC_SIMPLE_POINTER) desc = pFormat;
  else desc = pFormat + *(const SHORT*)pFormat;

  if (!Pointer) return;

  if (type == FC_FP) {
    int pointer_needs_freeing = NdrFullPointerFree(
      pStubMsg->FullPtrXlatTables, Pointer);
    if (!pointer_needs_freeing)
      return;
  }

  if (attr & FC_POINTER_DEREF) {
    current_pointer = *(unsigned char**)Pointer;
    TRACE("deref => %p\n", current_pointer);
  }

  m = NdrFreer[*desc & NDR_TABLE_MASK];
  if (m) m(pStubMsg, current_pointer, desc);

  /* this check stops us from trying to free buffer memory. we don't have to
   * worry about clients, since they won't call this function.
   * we don't have to check for the buffer being reallocated because
   * BufferStart and BufferEnd won't be reset when allocating memory for
   * sending the response. we don't have to check for the new buffer here as
   * it won't be used a type memory, only for buffer memory */
  if (Pointer >= pStubMsg->BufferStart && Pointer <= pStubMsg->BufferEnd)
      goto notfree;

  if (attr & FC_ALLOCED_ON_STACK) {
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
  unsigned rep, count, stride;
  unsigned i;
  unsigned char *saved_buffer = NULL;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  if (*pFormat != FC_PP) return NULL;
  pFormat += 2;

  if (pStubMsg->PointerBufferMark)
  {
    saved_buffer = pStubMsg->Buffer;
    pStubMsg->Buffer = pStubMsg->PointerBufferMark;
    pStubMsg->PointerBufferMark = NULL;
  }

  while (pFormat[0] != FC_END) {
    switch (pFormat[0]) {
    default:
      FIXME("unknown repeat type %d; assuming no repeat\n", pFormat[0]);
      /* fallthrough */
    case FC_NO_REPEAT:
      rep = 1;
      stride = 0;
      count = 1;
      pFormat += 2;
      break;
    case FC_FIXED_REPEAT:
      rep = *(const WORD*)&pFormat[2];
      stride = *(const WORD*)&pFormat[4];
      count = *(const WORD*)&pFormat[8];
      pFormat += 10;
      break;
    case FC_VARIABLE_REPEAT:
      rep = (pFormat[1] == FC_VARIABLE_OFFSET) ? pStubMsg->ActualCount : pStubMsg->MaxCount;
      stride = *(const WORD*)&pFormat[2];
      count = *(const WORD*)&pFormat[6];
      pFormat += 8;
      break;
    }
    for (i = 0; i < rep; i++) {
      PFORMAT_STRING info = pFormat;
      unsigned char *membase = pMemory + (i * stride);
      unsigned char *bufbase = Mark + (i * stride);
      unsigned u;

      for (u=0; u<count; u++,info+=8) {
        unsigned char *memptr = membase + *(const SHORT*)&info[0];
        unsigned char *bufptr = bufbase + *(const SHORT*)&info[2];
        unsigned char *saved_memory = pStubMsg->Memory;

        pStubMsg->Memory = membase;
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

/* rpcrt4 does something bizarre with embedded pointers: instead of copying the
 * struct/array/union from the buffer to memory and then unmarshalling pointers
 * into it, it unmarshals pointers into the buffer itself and then copies it to
 * memory. However, it will still attempt to use a user-supplied pointer where
 * appropriate (i.e. one on stack). Therefore we need to pass both pointers to
 * this function and to PointerUnmarshall: the pointer (to the buffer) that we
 * will actually unmarshal into (pDstBuffer), and the pointer (to memory) that
 * we will attempt to use for storage if possible (pSrcMemoryPtrs). */
static unsigned char * EmbeddedPointerUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                                 unsigned char *pDstBuffer,
                                                 unsigned char *pSrcMemoryPtrs,
                                                 PFORMAT_STRING pFormat,
                                                 unsigned char fMustAlloc)
{
  unsigned char *Mark = pStubMsg->BufferMark;
  unsigned rep, count, stride;
  unsigned i;
  unsigned char *saved_buffer = NULL;

  TRACE("(%p,%p,%p,%p,%d)\n", pStubMsg, pDstBuffer, pSrcMemoryPtrs, pFormat, fMustAlloc);

  if (*pFormat != FC_PP) return NULL;
  pFormat += 2;

  if (pStubMsg->PointerBufferMark)
  {
    saved_buffer = pStubMsg->Buffer;
    pStubMsg->Buffer = pStubMsg->PointerBufferMark;
    pStubMsg->PointerBufferMark = NULL;
  }

  while (pFormat[0] != FC_END) {
    TRACE("pFormat[0] = 0x%x\n", pFormat[0]);
    switch (pFormat[0]) {
    default:
      FIXME("unknown repeat type %d; assuming no repeat\n", pFormat[0]);
      /* fallthrough */
    case FC_NO_REPEAT:
      rep = 1;
      stride = 0;
      count = 1;
      pFormat += 2;
      break;
    case FC_FIXED_REPEAT:
      rep = *(const WORD*)&pFormat[2];
      stride = *(const WORD*)&pFormat[4];
      count = *(const WORD*)&pFormat[8];
      pFormat += 10;
      break;
    case FC_VARIABLE_REPEAT:
      rep = (pFormat[1] == FC_VARIABLE_OFFSET) ? pStubMsg->ActualCount : pStubMsg->MaxCount;
      stride = *(const WORD*)&pFormat[2];
      count = *(const WORD*)&pFormat[6];
      pFormat += 8;
      break;
    }
    for (i = 0; i < rep; i++) {
      PFORMAT_STRING info = pFormat;
      unsigned char *bufdstbase = pDstBuffer + (i * stride);
      unsigned char *memsrcbase = pSrcMemoryPtrs + (i * stride);
      unsigned char *bufbase = Mark + (i * stride);
      unsigned u;

      for (u=0; u<count; u++,info+=8) {
        unsigned char **bufdstptr = (unsigned char **)(bufdstbase + *(const SHORT*)&info[2]);
        unsigned char **memsrcptr = (unsigned char **)(memsrcbase + *(const SHORT*)&info[0]);
        unsigned char *bufptr = bufbase + *(const SHORT*)&info[2];
        PointerUnmarshall(pStubMsg, bufptr, bufdstptr, *memsrcptr, info+4, fMustAlloc);
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
  unsigned rep, count, stride;
  unsigned i;
  ULONG saved_buffer_length = 0;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  if (pStubMsg->IgnoreEmbeddedPointers) return;

  if (*pFormat != FC_PP) return;
  pFormat += 2;

  if (pStubMsg->PointerLength)
  {
    saved_buffer_length = pStubMsg->BufferLength;
    pStubMsg->BufferLength = pStubMsg->PointerLength;
    pStubMsg->PointerLength = 0;
  }

  while (pFormat[0] != FC_END) {
    switch (pFormat[0]) {
    default:
      FIXME("unknown repeat type %d; assuming no repeat\n", pFormat[0]);
      /* fallthrough */
    case FC_NO_REPEAT:
      rep = 1;
      stride = 0;
      count = 1;
      pFormat += 2;
      break;
    case FC_FIXED_REPEAT:
      rep = *(const WORD*)&pFormat[2];
      stride = *(const WORD*)&pFormat[4];
      count = *(const WORD*)&pFormat[8];
      pFormat += 10;
      break;
    case FC_VARIABLE_REPEAT:
      rep = (pFormat[1] == FC_VARIABLE_OFFSET) ? pStubMsg->ActualCount : pStubMsg->MaxCount;
      stride = *(const WORD*)&pFormat[2];
      count = *(const WORD*)&pFormat[6];
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

        pStubMsg->Memory = membase;
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
static ULONG EmbeddedPointerMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                       PFORMAT_STRING pFormat)
{
  unsigned char *Mark = pStubMsg->BufferMark;
  unsigned rep, count, stride;
  unsigned i;
  unsigned char *saved_buffer = NULL;

  TRACE("(%p,%p)\n", pStubMsg, pFormat);

  if (pStubMsg->IgnoreEmbeddedPointers) return 0;

  if (pStubMsg->PointerBufferMark)
  {
    saved_buffer = pStubMsg->Buffer;
    pStubMsg->Buffer = pStubMsg->PointerBufferMark;
    pStubMsg->PointerBufferMark = NULL;
  }

  if (*pFormat != FC_PP) return 0;
  pFormat += 2;

  while (pFormat[0] != FC_END) {
    switch (pFormat[0]) {
    default:
      FIXME("unknown repeat type %d; assuming no repeat\n", pFormat[0]);
      /* fallthrough */
    case FC_NO_REPEAT:
      rep = 1;
      stride = 0;
      count = 1;
      pFormat += 2;
      break;
    case FC_FIXED_REPEAT:
      rep = *(const WORD*)&pFormat[2];
      stride = *(const WORD*)&pFormat[4];
      count = *(const WORD*)&pFormat[8];
      pFormat += 10;
      break;
    case FC_VARIABLE_REPEAT:
      rep = (pFormat[1] == FC_VARIABLE_OFFSET) ? pStubMsg->ActualCount : pStubMsg->MaxCount;
      stride = *(const WORD*)&pFormat[2];
      count = *(const WORD*)&pFormat[6];
      pFormat += 8;
      break;
    }
    for (i = 0; i < rep; i++) {
      PFORMAT_STRING info = pFormat;
      unsigned char *bufbase = Mark + (i * stride);
      unsigned u;
      for (u=0; u<count; u++,info+=8) {
        unsigned char *bufptr = bufbase + *(const SHORT*)&info[2];
        PointerMemorySize(pStubMsg, bufptr, info+4);
      }
    }
    pFormat += 8 * count;
  }

  if (saved_buffer)
  {
    pStubMsg->PointerBufferMark = pStubMsg->Buffer;
    pStubMsg->Buffer = saved_buffer;
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
  unsigned rep, count, stride;
  unsigned i;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  if (*pFormat != FC_PP) return;
  pFormat += 2;

  while (pFormat[0] != FC_END) {
    switch (pFormat[0]) {
    default:
      FIXME("unknown repeat type %d; assuming no repeat\n", pFormat[0]);
      /* fallthrough */
    case FC_NO_REPEAT:
      rep = 1;
      stride = 0;
      count = 1;
      pFormat += 2;
      break;
    case FC_FIXED_REPEAT:
      rep = *(const WORD*)&pFormat[2];
      stride = *(const WORD*)&pFormat[4];
      count = *(const WORD*)&pFormat[8];
      pFormat += 10;
      break;
    case FC_VARIABLE_REPEAT:
      rep = (pFormat[1] == FC_VARIABLE_OFFSET) ? pStubMsg->ActualCount : pStubMsg->MaxCount;
      stride = *(const WORD*)&pFormat[2];
      count = *(const WORD*)&pFormat[6];
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

        pStubMsg->Memory = membase;
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

  /* Increment the buffer here instead of in PointerMarshall,
   * as that is used by embedded pointers which already handle the incrementing
   * the buffer, and shouldn't write any additional pointer data to the wire */
  if (*pFormat != FC_RP)
  {
    align_pointer_clear(&pStubMsg->Buffer, 4);
    Buffer = pStubMsg->Buffer;
    safe_buffer_increment(pStubMsg, 4);
  }
  else
    Buffer = pStubMsg->Buffer;

  PointerMarshall(pStubMsg, Buffer, pMemory, pFormat);

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

  if (*pFormat == FC_RP)
  {
    Buffer = pStubMsg->Buffer;
    /* Do the NULL ref pointer check here because embedded pointers can be
     * NULL if the type the pointer is embedded in was allocated rather than
     * being passed in by the client */
    if (pStubMsg->IsClient && !*ppMemory)
    {
      ERR("NULL ref pointer is not allowed\n");
      RpcRaiseException(RPC_X_NULL_REF_POINTER);
    }
  }
  else
  {
    /* Increment the buffer here instead of in PointerUnmarshall,
     * as that is used by embedded pointers which already handle the incrementing
     * the buffer, and shouldn't read any additional pointer data from the
     * buffer */
    align_pointer(&pStubMsg->Buffer, 4);
    Buffer = pStubMsg->Buffer;
    safe_buffer_increment(pStubMsg, 4);
  }

  PointerUnmarshall(pStubMsg, Buffer, ppMemory, *ppMemory, pFormat, fMustAlloc);

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

  /* Increment the buffer length here instead of in PointerBufferSize,
   * as that is used by embedded pointers which already handle the buffer
   * length, and shouldn't write anything more to the wire */
  if (*pFormat != FC_RP)
  {
    align_length(&pStubMsg->BufferLength, 4);
    safe_buffer_length_increment(pStubMsg, 4);
  }

  PointerBufferSize(pStubMsg, pMemory, pFormat);
}

/***********************************************************************
 *           NdrPointerMemorySize [RPCRT4.@]
 */
ULONG WINAPI NdrPointerMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                  PFORMAT_STRING pFormat)
{
    unsigned char *Buffer = pStubMsg->Buffer;
    if (*pFormat != FC_RP)
    {
        align_pointer(&pStubMsg->Buffer, 4);
        safe_buffer_increment(pStubMsg, 4);
    }
    align_length(&pStubMsg->MemorySize, sizeof(void *));
    return PointerMemorySize(pStubMsg, Buffer, pFormat);
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
 *
 * Unmarshall a base type.
 *
 * NOTES
 *  Doesn't check that the buffer is long enough before copying, so the caller
 * should do this.
 */
void WINAPI NdrSimpleTypeUnmarshall( PMIDL_STUB_MESSAGE pStubMsg, unsigned char* pMemory,
                                     unsigned char FormatChar )
{
#define BASE_TYPE_UNMARSHALL(type) \
        align_pointer(&pStubMsg->Buffer, sizeof(type)); \
	TRACE("pMemory: %p\n", pMemory); \
	*(type *)pMemory = *(type *)pStubMsg->Buffer; \
        pStubMsg->Buffer += sizeof(type);

    switch(FormatChar)
    {
    case FC_BYTE:
    case FC_CHAR:
    case FC_SMALL:
    case FC_USMALL:
        BASE_TYPE_UNMARSHALL(UCHAR);
        TRACE("value: 0x%02x\n", *pMemory);
        break;
    case FC_WCHAR:
    case FC_SHORT:
    case FC_USHORT:
        BASE_TYPE_UNMARSHALL(USHORT);
        TRACE("value: 0x%04x\n", *(USHORT *)pMemory);
        break;
    case FC_LONG:
    case FC_ULONG:
    case FC_ERROR_STATUS_T:
    case FC_ENUM32:
        BASE_TYPE_UNMARSHALL(ULONG);
        TRACE("value: 0x%08lx\n", *(ULONG *)pMemory);
        break;
   case FC_FLOAT:
        BASE_TYPE_UNMARSHALL(float);
        TRACE("value: %f\n", *(float *)pMemory);
        break;
    case FC_DOUBLE:
        BASE_TYPE_UNMARSHALL(double);
        TRACE("value: %f\n", *(double *)pMemory);
        break;
    case FC_HYPER:
        BASE_TYPE_UNMARSHALL(ULONGLONG);
        TRACE("value: %s\n", wine_dbgstr_longlong(*(ULONGLONG *)pMemory));
        break;
    case FC_ENUM16:
        align_pointer(&pStubMsg->Buffer, sizeof(USHORT));
        TRACE("pMemory: %p\n", pMemory);
        /* 16-bits on the wire, but int in memory */
        *(UINT *)pMemory = *(USHORT *)pStubMsg->Buffer;
        pStubMsg->Buffer += sizeof(USHORT);
        TRACE("value: 0x%08x\n", *(UINT *)pMemory);
        break;
    case FC_INT3264:
        align_pointer(&pStubMsg->Buffer, sizeof(INT));
        /* 32-bits on the wire, but int_ptr in memory */
        *(INT_PTR *)pMemory = *(INT *)pStubMsg->Buffer;
        pStubMsg->Buffer += sizeof(INT);
        TRACE("value: 0x%08Ix\n", *(INT_PTR *)pMemory);
        break;
    case FC_UINT3264:
        align_pointer(&pStubMsg->Buffer, sizeof(UINT));
        /* 32-bits on the wire, but int_ptr in memory */
        *(UINT_PTR *)pMemory = *(UINT *)pStubMsg->Buffer;
        pStubMsg->Buffer += sizeof(UINT);
        TRACE("value: 0x%08Ix\n", *(UINT_PTR *)pMemory);
        break;
    case FC_IGNORE:
        break;
    default:
        FIXME("Unhandled base type: 0x%02x\n", FormatChar);
    }
#undef BASE_TYPE_UNMARSHALL
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

  align_pointer_clear(&pStubMsg->Buffer, pFormat[1] + 1);

  pStubMsg->BufferMark = pStubMsg->Buffer;
  safe_copy_to_buffer(pStubMsg, pMemory, size);

  if (pFormat[0] != FC_STRUCT)
    EmbeddedPointerMarshall(pStubMsg, pMemory, pFormat+4);

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
  unsigned char *saved_buffer;
  TRACE("(%p,%p,%p,%d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);

  align_pointer(&pStubMsg->Buffer, pFormat[1] + 1);

  if (fMustAlloc)
    *ppMemory = NdrAllocateZero(pStubMsg, size);
  else
  {
    if (!pStubMsg->IsClient && !*ppMemory)
      /* for servers, we just point straight into the RPC buffer */
      *ppMemory = pStubMsg->Buffer;
  }

  saved_buffer = pStubMsg->BufferMark = pStubMsg->Buffer;
  safe_buffer_increment(pStubMsg, size);
  if (pFormat[0] == FC_PSTRUCT)
      EmbeddedPointerUnmarshall(pStubMsg, saved_buffer, *ppMemory, pFormat+4, fMustAlloc);

  TRACE("copying %p to %p\n", saved_buffer, *ppMemory);
  if (*ppMemory != saved_buffer)
      memcpy(*ppMemory, saved_buffer, size);

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

  align_length(&pStubMsg->BufferLength, pFormat[1] + 1);

  safe_buffer_length_increment(pStubMsg, size);
  if (pFormat[0] != FC_STRUCT)
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

  align_pointer(&pStubMsg->Buffer, pFormat[1] + 1);
  pStubMsg->MemorySize += size;
  safe_buffer_increment(pStubMsg, size);

  if (pFormat[0] != FC_STRUCT)
    EmbeddedPointerMemorySize(pStubMsg, pFormat+4);
  return pStubMsg->MemorySize;
}

/***********************************************************************
 *           NdrSimpleStructFree [RPCRT4.@]
 */
void WINAPI NdrSimpleStructFree(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  if (pFormat[0] != FC_STRUCT)
    EmbeddedPointerFree(pStubMsg, pMemory, pFormat+4);
}

/* Array helpers */

static inline void array_compute_and_size_conformance(
    unsigned char fc, PMIDL_STUB_MESSAGE pStubMsg, unsigned char *pMemory,
    PFORMAT_STRING pFormat)
{
  DWORD count;

  switch (fc)
  {
  case FC_CARRAY:
    ComputeConformance(pStubMsg, pMemory, pFormat+4, 0);
    SizeConformance(pStubMsg);
    break;
  case FC_CVARRAY:
    pFormat = ComputeConformance(pStubMsg, pMemory, pFormat + 4, 0);
    pFormat = ComputeVariance(pStubMsg, pMemory, pFormat, 0);
    SizeConformance(pStubMsg);
    break;
  case FC_C_CSTRING:
  case FC_C_WSTRING:
    if (fc == FC_C_CSTRING)
    {
      TRACE("string=%s\n", debugstr_a((const char *)pMemory));
      pStubMsg->ActualCount = strlen((const char *)pMemory)+1;
    }
    else
    {
      TRACE("string=%s\n", debugstr_w((LPCWSTR)pMemory));
      pStubMsg->ActualCount = lstrlenW((LPCWSTR)pMemory)+1;
    }

    if (pFormat[1] == FC_STRING_SIZED)
      pFormat = ComputeConformance(pStubMsg, pMemory, pFormat + 2, 0);
    else
      pStubMsg->MaxCount = pStubMsg->ActualCount;

    SizeConformance(pStubMsg);
    break;
  case FC_BOGUS_ARRAY:
    count = *(const WORD *)(pFormat + 2);
    pFormat += 4;
    if (IsConformanceOrVariancePresent(pFormat)) SizeConformance(pStubMsg);
    pFormat = ComputeConformance(pStubMsg, pMemory, pFormat, count);
    pFormat = ComputeVariance(pStubMsg, pMemory, pFormat, pStubMsg->MaxCount);
    break;
  default:
    ERR("unknown array format 0x%x\n", fc);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
  }
}

static inline void array_buffer_size(
    unsigned char fc, PMIDL_STUB_MESSAGE pStubMsg, unsigned char *pMemory,
    PFORMAT_STRING pFormat, unsigned char fHasPointers)
{
  DWORD i, size;
  DWORD esize;
  unsigned char alignment;

  switch (fc)
  {
  case FC_CARRAY:
    esize = *(const WORD*)(pFormat+2);
    alignment = pFormat[1] + 1;

    pFormat = SkipConformance(pStubMsg, pFormat + 4);

    align_length(&pStubMsg->BufferLength, alignment);

    size = safe_multiply(esize, pStubMsg->MaxCount);
    /* conformance value plus array */
    safe_buffer_length_increment(pStubMsg, size);

    if (fHasPointers)
      EmbeddedPointerBufferSize(pStubMsg, pMemory, pFormat);
    break;
  case FC_CVARRAY:
    esize = *(const WORD*)(pFormat+2);
    alignment = pFormat[1] + 1;

    pFormat = SkipConformance(pStubMsg, pFormat + 4);
    pFormat = SkipVariance(pStubMsg, pFormat);

    SizeVariance(pStubMsg);

    align_length(&pStubMsg->BufferLength, alignment);

    size = safe_multiply(esize, pStubMsg->ActualCount);
    safe_buffer_length_increment(pStubMsg, size);

    if (fHasPointers)
      EmbeddedPointerBufferSize(pStubMsg, pMemory, pFormat);
    break;
  case FC_C_CSTRING:
  case FC_C_WSTRING:
    if (fc == FC_C_CSTRING)
      esize = 1;
    else
      esize = 2;

    SizeVariance(pStubMsg);

    size = safe_multiply(esize, pStubMsg->ActualCount);
    safe_buffer_length_increment(pStubMsg, size);
    break;
  case FC_BOGUS_ARRAY:
    alignment = pFormat[1] + 1;
    pFormat = SkipConformance(pStubMsg, pFormat + 4);
    if (IsConformanceOrVariancePresent(pFormat)) SizeVariance(pStubMsg);
    pFormat = SkipVariance(pStubMsg, pFormat);

    align_length(&pStubMsg->BufferLength, alignment);

    size = pStubMsg->ActualCount;
    for (i = 0; i < size; i++)
      pMemory = ComplexBufferSize(pStubMsg, pMemory, pFormat, NULL);
    break;
  default:
    ERR("unknown array format 0x%x\n", fc);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
  }
}

static inline void array_compute_and_write_conformance(
    unsigned char fc, PMIDL_STUB_MESSAGE pStubMsg, unsigned char *pMemory,
    PFORMAT_STRING pFormat)
{
  ULONG def;
  BOOL conformance_present;

  switch (fc)
  {
  case FC_CARRAY:
    ComputeConformance(pStubMsg, pMemory, pFormat+4, 0);
    WriteConformance(pStubMsg);
    break;
  case FC_CVARRAY:
    pFormat = ComputeConformance(pStubMsg, pMemory, pFormat + 4, 0);
    pFormat = ComputeVariance(pStubMsg, pMemory, pFormat, 0);
    WriteConformance(pStubMsg);
    break;
  case FC_C_CSTRING:
  case FC_C_WSTRING:
    if (fc == FC_C_CSTRING)
    {
      TRACE("string=%s\n", debugstr_a((const char *)pMemory));
      pStubMsg->ActualCount = strlen((const char *)pMemory)+1;
    }
    else
    {
      TRACE("string=%s\n", debugstr_w((LPCWSTR)pMemory));
      pStubMsg->ActualCount = lstrlenW((LPCWSTR)pMemory)+1;
    }
    if (pFormat[1] == FC_STRING_SIZED)
      pFormat = ComputeConformance(pStubMsg, pMemory, pFormat + 2, 0);
    else
      pStubMsg->MaxCount = pStubMsg->ActualCount;
    pStubMsg->Offset = 0;
    WriteConformance(pStubMsg);
    break;
  case FC_BOGUS_ARRAY:
    def = *(const WORD *)(pFormat + 2);
    pFormat += 4;
    conformance_present = IsConformanceOrVariancePresent(pFormat);
    pFormat = ComputeConformance(pStubMsg, pMemory, pFormat, def);
    pFormat = ComputeVariance(pStubMsg, pMemory, pFormat, pStubMsg->MaxCount);
    if (conformance_present) WriteConformance(pStubMsg);
    break;
  default:
    ERR("unknown array format 0x%x\n", fc);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
  }
}

static inline void array_write_variance_and_marshall(
    unsigned char fc, PMIDL_STUB_MESSAGE pStubMsg, unsigned char *pMemory,
    PFORMAT_STRING pFormat, unsigned char fHasPointers)
{
  DWORD i, size;
  DWORD esize;
  unsigned char alignment;

  switch (fc)
  {
  case FC_CARRAY:
    esize = *(const WORD*)(pFormat+2);
    alignment = pFormat[1] + 1;

    pFormat = SkipConformance(pStubMsg, pFormat + 4);

    align_pointer_clear(&pStubMsg->Buffer, alignment);

    size = safe_multiply(esize, pStubMsg->MaxCount);
    if (fHasPointers)
      pStubMsg->BufferMark = pStubMsg->Buffer;
    safe_copy_to_buffer(pStubMsg, pMemory, size);

    if (fHasPointers)
      EmbeddedPointerMarshall(pStubMsg, pMemory, pFormat);
    break;
  case FC_CVARRAY:
    esize = *(const WORD*)(pFormat+2);
    alignment = pFormat[1] + 1;

    pFormat = SkipConformance(pStubMsg, pFormat + 4);
    pFormat = SkipVariance(pStubMsg, pFormat);

    WriteVariance(pStubMsg);

    align_pointer_clear(&pStubMsg->Buffer, alignment);

    size = safe_multiply(esize, pStubMsg->ActualCount);

    if (fHasPointers)
      pStubMsg->BufferMark = pStubMsg->Buffer;
    safe_copy_to_buffer(pStubMsg, pMemory + pStubMsg->Offset, size);

    if (fHasPointers)
      EmbeddedPointerMarshall(pStubMsg, pMemory, pFormat);
    break;
  case FC_C_CSTRING:
  case FC_C_WSTRING:
    if (fc == FC_C_CSTRING)
      esize = 1;
    else
      esize = 2;

    WriteVariance(pStubMsg);

    size = safe_multiply(esize, pStubMsg->ActualCount);
    safe_copy_to_buffer(pStubMsg, pMemory, size); /* the string itself */
    break;
  case FC_BOGUS_ARRAY:
    alignment = pFormat[1] + 1;
    pFormat = SkipConformance(pStubMsg, pFormat + 4);
    if (IsConformanceOrVariancePresent(pFormat)) WriteVariance(pStubMsg);
    pFormat = SkipVariance(pStubMsg, pFormat);

    align_pointer_clear(&pStubMsg->Buffer, alignment);

    size = pStubMsg->ActualCount;
    for (i = 0; i < size; i++)
      pMemory = ComplexMarshall(pStubMsg, pMemory, pFormat, NULL);
    break;
  default:
    ERR("unknown array format 0x%x\n", fc);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
  }
}

static inline ULONG array_read_conformance(
    unsigned char fc, PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat)
{
  DWORD def, esize;

  switch (fc)
  {
  case FC_CARRAY:
    esize = *(const WORD*)(pFormat+2);
    pFormat = ReadConformance(pStubMsg, pFormat+4);
    return safe_multiply(esize, pStubMsg->MaxCount);
  case FC_CVARRAY:
    esize = *(const WORD*)(pFormat+2);
    pFormat = ReadConformance(pStubMsg, pFormat+4);
    return safe_multiply(esize, pStubMsg->MaxCount);
  case FC_C_CSTRING:
  case FC_C_WSTRING:
    if (fc == FC_C_CSTRING)
      esize = 1;
    else
      esize = 2;

    if (pFormat[1] == FC_STRING_SIZED)
      ReadConformance(pStubMsg, pFormat + 2);
    else
      ReadConformance(pStubMsg, NULL);
    return safe_multiply(esize, pStubMsg->MaxCount);
  case FC_BOGUS_ARRAY:
    def = *(const WORD *)(pFormat + 2);
    pFormat += 4;
    if (IsConformanceOrVariancePresent(pFormat)) pFormat = ReadConformance(pStubMsg, pFormat);
    else
    {
        pStubMsg->MaxCount = def;
        pFormat = SkipConformance( pStubMsg, pFormat );
    }
    pFormat = SkipVariance( pStubMsg, pFormat );

    esize = ComplexStructSize(pStubMsg, pFormat);
    return safe_multiply(pStubMsg->MaxCount, esize);
  default:
    ERR("unknown array format 0x%x\n", fc);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
  }
}

static inline ULONG array_read_variance_and_unmarshall(
    unsigned char fc, PMIDL_STUB_MESSAGE pStubMsg, unsigned char **ppMemory,
    PFORMAT_STRING pFormat, unsigned char fMustAlloc,
    unsigned char fUseBufferMemoryServer, unsigned char fUnmarshall)
{
  ULONG bufsize, memsize;
  WORD esize;
  unsigned char alignment;
  unsigned char *saved_buffer, *pMemory;
  ULONG i, offset, count;

  switch (fc)
  {
  case FC_CARRAY:
    esize = *(const WORD*)(pFormat+2);
    alignment = pFormat[1] + 1;

    bufsize = memsize = safe_multiply(esize, pStubMsg->MaxCount);

    pFormat = SkipConformance(pStubMsg, pFormat + 4);

    align_pointer(&pStubMsg->Buffer, alignment);

    if (fUnmarshall)
    {
      if (fMustAlloc)
        *ppMemory = NdrAllocateZero(pStubMsg, memsize);
      else
      {
        if (fUseBufferMemoryServer && !pStubMsg->IsClient && !*ppMemory)
          /* for servers, we just point straight into the RPC buffer */
          *ppMemory = pStubMsg->Buffer;
      }

      saved_buffer = pStubMsg->Buffer;
      safe_buffer_increment(pStubMsg, bufsize);

      pStubMsg->BufferMark = saved_buffer;
      EmbeddedPointerUnmarshall(pStubMsg, saved_buffer, *ppMemory, pFormat, fMustAlloc);

      TRACE("copying %p to %p\n", saved_buffer, *ppMemory);
      if (*ppMemory != saved_buffer)
        memcpy(*ppMemory, saved_buffer, bufsize);
    }
    return bufsize;
  case FC_CVARRAY:
    esize = *(const WORD*)(pFormat+2);
    alignment = pFormat[1] + 1;

    pFormat = SkipConformance(pStubMsg, pFormat + 4);

    pFormat = ReadVariance(pStubMsg, pFormat, pStubMsg->MaxCount);

    align_pointer(&pStubMsg->Buffer, alignment);

    bufsize = safe_multiply(esize, pStubMsg->ActualCount);
    memsize = safe_multiply(esize, pStubMsg->MaxCount);

    if (fUnmarshall)
    {
      offset = pStubMsg->Offset;

      if (!fMustAlloc && !*ppMemory)
        fMustAlloc = TRUE;
      if (fMustAlloc)
        *ppMemory = NdrAllocateZero(pStubMsg, memsize);
      saved_buffer = pStubMsg->Buffer;
      safe_buffer_increment(pStubMsg, bufsize);

      pStubMsg->BufferMark = saved_buffer;
      EmbeddedPointerUnmarshall(pStubMsg, saved_buffer, *ppMemory, pFormat,
                                fMustAlloc);

      memcpy(*ppMemory + offset, saved_buffer, bufsize);
    }
    return bufsize;
  case FC_C_CSTRING:
  case FC_C_WSTRING:
    if (fc == FC_C_CSTRING)
      esize = 1;
    else
      esize = 2;

    ReadVariance(pStubMsg, NULL, pStubMsg->MaxCount);

    if (pFormat[1] != FC_STRING_SIZED && (pStubMsg->MaxCount != pStubMsg->ActualCount))
    {
      ERR("buffer size %ld must equal memory size %Id for non-sized conformant strings\n",
          pStubMsg->ActualCount, pStubMsg->MaxCount);
      RpcRaiseException(RPC_S_INVALID_BOUND);
    }
    if (pStubMsg->Offset)
    {
      ERR("conformant strings can't have Offset (%ld)\n", pStubMsg->Offset);
      RpcRaiseException(RPC_S_INVALID_BOUND);
    }

    memsize = safe_multiply(esize, pStubMsg->MaxCount);
    bufsize = safe_multiply(esize, pStubMsg->ActualCount);

    validate_string_data(pStubMsg, bufsize, esize);

    if (fUnmarshall)
    {
      if (fMustAlloc)
        *ppMemory = NdrAllocate(pStubMsg, memsize);
      else
      {
        if (fUseBufferMemoryServer && !pStubMsg->IsClient &&
            !*ppMemory && (pStubMsg->MaxCount == pStubMsg->ActualCount))
          /* if the data in the RPC buffer is big enough, we just point
           * straight into it */
          *ppMemory = pStubMsg->Buffer;
        else if (!*ppMemory)
          *ppMemory = NdrAllocate(pStubMsg, memsize);
      }

      if (*ppMemory == pStubMsg->Buffer)
        safe_buffer_increment(pStubMsg, bufsize);
      else
        safe_copy_from_buffer(pStubMsg, *ppMemory, bufsize);

      if (*pFormat == FC_C_CSTRING)
        TRACE("string=%s\n", debugstr_a((char*)*ppMemory));
      else
        TRACE("string=%s\n", debugstr_w((LPWSTR)*ppMemory));
    }
    return bufsize;

  case FC_BOGUS_ARRAY:
    alignment = pFormat[1] + 1;
    pFormat = SkipConformance(pStubMsg, pFormat + 4);
    pFormat = ReadVariance(pStubMsg, pFormat, pStubMsg->MaxCount);

    esize = ComplexStructSize(pStubMsg, pFormat);
    memsize = safe_multiply(esize, pStubMsg->MaxCount);

    assert( fUnmarshall );

    if (!fMustAlloc && !*ppMemory)
      fMustAlloc = TRUE;
    if (fMustAlloc)
      *ppMemory = NdrAllocateZero(pStubMsg, memsize);

    align_pointer(&pStubMsg->Buffer, alignment);
    saved_buffer = pStubMsg->Buffer;

    pMemory = *ppMemory;
    count = pStubMsg->ActualCount;
    for (i = 0; i < count; i++)
        pMemory = ComplexUnmarshall(pStubMsg, pMemory, pFormat, NULL, fMustAlloc);
    return pStubMsg->Buffer - saved_buffer;

  default:
    ERR("unknown array format 0x%x\n", fc);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
  }
}

static inline void array_memory_size(
    unsigned char fc, PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat,
    unsigned char fHasPointers)
{
  ULONG i, count, SavedMemorySize;
  ULONG bufsize, memsize;
  DWORD esize;
  unsigned char alignment;

  switch (fc)
  {
  case FC_CARRAY:
    esize = *(const WORD*)(pFormat+2);
    alignment = pFormat[1] + 1;

    pFormat = SkipConformance(pStubMsg, pFormat + 4);

    bufsize = memsize = safe_multiply(esize, pStubMsg->MaxCount);
    pStubMsg->MemorySize += memsize;

    align_pointer(&pStubMsg->Buffer, alignment);
    if (fHasPointers)
      pStubMsg->BufferMark = pStubMsg->Buffer;
    safe_buffer_increment(pStubMsg, bufsize);

    if (fHasPointers)
      EmbeddedPointerMemorySize(pStubMsg, pFormat);
    break;
  case FC_CVARRAY:
    esize = *(const WORD*)(pFormat+2);
    alignment = pFormat[1] + 1;

    pFormat = SkipConformance(pStubMsg, pFormat + 4);

    pFormat = ReadVariance(pStubMsg, pFormat, pStubMsg->MaxCount);

    bufsize = safe_multiply(esize, pStubMsg->ActualCount);
    memsize = safe_multiply(esize, pStubMsg->MaxCount);
    pStubMsg->MemorySize += memsize;

    align_pointer(&pStubMsg->Buffer, alignment);
    if (fHasPointers)
      pStubMsg->BufferMark = pStubMsg->Buffer;
    safe_buffer_increment(pStubMsg, bufsize);

    if (fHasPointers)
      EmbeddedPointerMemorySize(pStubMsg, pFormat);
    break;
  case FC_C_CSTRING:
  case FC_C_WSTRING:
    if (fc == FC_C_CSTRING)
      esize = 1;
    else
      esize = 2;

    ReadVariance(pStubMsg, NULL, pStubMsg->MaxCount);

    if (pFormat[1] != FC_STRING_SIZED && (pStubMsg->MaxCount != pStubMsg->ActualCount))
    {
      ERR("buffer size %ld must equal memory size %Id for non-sized conformant strings\n",
          pStubMsg->ActualCount, pStubMsg->MaxCount);
      RpcRaiseException(RPC_S_INVALID_BOUND);
    }
    if (pStubMsg->Offset)
    {
      ERR("conformant strings can't have Offset (%ld)\n", pStubMsg->Offset);
      RpcRaiseException(RPC_S_INVALID_BOUND);
    }

    memsize = safe_multiply(esize, pStubMsg->MaxCount);
    bufsize = safe_multiply(esize, pStubMsg->ActualCount);

    validate_string_data(pStubMsg, bufsize, esize);

    safe_buffer_increment(pStubMsg, bufsize);
    pStubMsg->MemorySize += memsize;
    break;
  case FC_BOGUS_ARRAY:
    alignment = pFormat[1] + 1;
    pFormat = SkipConformance(pStubMsg, pFormat + 4);
    pFormat = ReadVariance(pStubMsg, pFormat, pStubMsg->MaxCount);

    align_pointer(&pStubMsg->Buffer, alignment);

    SavedMemorySize = pStubMsg->MemorySize;

    esize = ComplexStructSize(pStubMsg, pFormat);
    memsize = safe_multiply(pStubMsg->MaxCount, esize);

    count = pStubMsg->ActualCount;
    for (i = 0; i < count; i++)
        ComplexStructMemorySize(pStubMsg, pFormat, NULL);

    pStubMsg->MemorySize = SavedMemorySize + memsize;
    break;
  default:
    ERR("unknown array format 0x%x\n", fc);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
  }
}

static inline void array_free(
    unsigned char fc, PMIDL_STUB_MESSAGE pStubMsg,
    unsigned char *pMemory, PFORMAT_STRING pFormat, unsigned char fHasPointers)
{
  DWORD i, count;

  switch (fc)
  {
  case FC_CARRAY:
    pFormat = ComputeConformance(pStubMsg, pMemory, pFormat+4, 0);
    if (fHasPointers)
      EmbeddedPointerFree(pStubMsg, pMemory, pFormat);
    break;
  case FC_CVARRAY:
    pFormat = ComputeConformance(pStubMsg, pMemory, pFormat+4, 0);
    pFormat = ComputeVariance(pStubMsg, pMemory, pFormat, 0);
    if (fHasPointers)
      EmbeddedPointerFree(pStubMsg, pMemory, pFormat);
    break;
  case FC_C_CSTRING:
  case FC_C_WSTRING:
    /* No embedded pointers so nothing to do */
    break;
  case FC_BOGUS_ARRAY:
      count = *(const WORD *)(pFormat + 2);
      pFormat = ComputeConformance(pStubMsg, pMemory, pFormat + 4, count);
      pFormat = ComputeVariance(pStubMsg, pMemory, pFormat, pStubMsg->MaxCount);

      count = pStubMsg->ActualCount;
      for (i = 0; i < count; i++)
          pMemory = ComplexFree(pStubMsg, pMemory, pFormat, NULL);
    break;
  default:
    ERR("unknown array format 0x%x\n", fc);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
  }
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
  TRACE("(pStubMsg == ^%p, pszMessage == ^%p, pFormat == ^%p)\n", pStubMsg, pszMessage, pFormat);

  if (pFormat[0] != FC_C_CSTRING && pFormat[0] != FC_C_WSTRING) {
    ERR("Unhandled string type: %#x\n", pFormat[0]);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
  }

  /* allow compiler to optimise inline function by passing constant into
   * these functions */
  if (pFormat[0] == FC_C_CSTRING) {
    array_compute_and_write_conformance(FC_C_CSTRING, pStubMsg, pszMessage,
                                        pFormat);
    array_write_variance_and_marshall(FC_C_CSTRING, pStubMsg, pszMessage,
                                      pFormat, TRUE /* fHasPointers */);
  } else {
    array_compute_and_write_conformance(FC_C_WSTRING, pStubMsg, pszMessage,
                                        pFormat);
    array_write_variance_and_marshall(FC_C_WSTRING, pStubMsg, pszMessage,
                                      pFormat, TRUE /* fHasPointers */);
  }

  return NULL;
}

/***********************************************************************
 *           NdrConformantStringBufferSize [RPCRT4.@]
 */
void WINAPI NdrConformantStringBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
  unsigned char* pMemory, PFORMAT_STRING pFormat)
{
  TRACE("(pStubMsg == ^%p, pMemory == ^%p, pFormat == ^%p)\n", pStubMsg, pMemory, pFormat);

  if (pFormat[0] != FC_C_CSTRING && pFormat[0] != FC_C_WSTRING) {
    ERR("Unhandled string type: %#x\n", pFormat[0]);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
  }

  /* allow compiler to optimise inline function by passing constant into
   * these functions */
  if (pFormat[0] == FC_C_CSTRING) {
    array_compute_and_size_conformance(FC_C_CSTRING, pStubMsg, pMemory,
                                       pFormat);
    array_buffer_size(FC_C_CSTRING, pStubMsg, pMemory, pFormat,
                      TRUE /* fHasPointers */);
  } else {
    array_compute_and_size_conformance(FC_C_WSTRING, pStubMsg, pMemory,
                                       pFormat);
    array_buffer_size(FC_C_WSTRING, pStubMsg, pMemory, pFormat,
                      TRUE /* fHasPointers */);
  }
}

/************************************************************************
 *            NdrConformantStringMemorySize [RPCRT4.@]
 */
ULONG WINAPI NdrConformantStringMemorySize( PMIDL_STUB_MESSAGE pStubMsg,
  PFORMAT_STRING pFormat )
{
  TRACE("(pStubMsg == ^%p, pFormat == ^%p)\n", pStubMsg, pFormat);

  if (pFormat[0] != FC_C_CSTRING && pFormat[0] != FC_C_WSTRING) {
    ERR("Unhandled string type: %#x\n", pFormat[0]);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
  }

  /* allow compiler to optimise inline function by passing constant into
   * these functions */
  if (pFormat[0] == FC_C_CSTRING) {
    array_read_conformance(FC_C_CSTRING, pStubMsg, pFormat);
    array_memory_size(FC_C_CSTRING, pStubMsg, pFormat,
                      TRUE /* fHasPointers */);
  } else {
    array_read_conformance(FC_C_WSTRING, pStubMsg, pFormat);
    array_memory_size(FC_C_WSTRING, pStubMsg, pFormat,
                      TRUE /* fHasPointers */);
  }

  return pStubMsg->MemorySize;
}

/************************************************************************
 *           NdrConformantStringUnmarshall [RPCRT4.@]
 */
unsigned char *WINAPI NdrConformantStringUnmarshall( PMIDL_STUB_MESSAGE pStubMsg,
  unsigned char** ppMemory, PFORMAT_STRING pFormat, unsigned char fMustAlloc )
{
  TRACE("(pStubMsg == ^%p, *pMemory == ^%p, pFormat == ^%p, fMustAlloc == %u)\n",
    pStubMsg, *ppMemory, pFormat, fMustAlloc);

  if (pFormat[0] != FC_C_CSTRING && pFormat[0] != FC_C_WSTRING) {
    ERR("Unhandled string type: %#x\n", *pFormat);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
  }

  /* allow compiler to optimise inline function by passing constant into
   * these functions */
  if (pFormat[0] == FC_C_CSTRING) {
    array_read_conformance(FC_C_CSTRING, pStubMsg, pFormat);
    array_read_variance_and_unmarshall(FC_C_CSTRING, pStubMsg, ppMemory,
                                       pFormat, fMustAlloc,
                                       TRUE /* fUseBufferMemoryServer */,
                                       TRUE /* fUnmarshall */);
  } else {
    array_read_conformance(FC_C_WSTRING, pStubMsg, pFormat);
    array_read_variance_and_unmarshall(FC_C_WSTRING, pStubMsg, ppMemory,
                                       pFormat, fMustAlloc,
                                       TRUE /* fUseBufferMemoryServer */,
                                       TRUE /* fUnmarshall */);
  }

  return NULL;
}

/***********************************************************************
 *           NdrNonConformantStringMarshall [RPCRT4.@]
 */
unsigned char *  WINAPI NdrNonConformantStringMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
  ULONG esize, size, maxsize;

  TRACE("(pStubMsg == ^%p, pMemory == ^%p, pFormat == ^%p)\n", pStubMsg, pMemory, pFormat);

  maxsize = *(const USHORT *)&pFormat[2];

  if (*pFormat == FC_CSTRING)
  {
    ULONG i = 0;
    const char *str = (const char *)pMemory;
    while (i < maxsize && str[i]) i++;
    TRACE("string=%s\n", debugstr_an(str, i));
    pStubMsg->ActualCount = i + 1;
    esize = 1;
  }
  else if (*pFormat == FC_WSTRING)
  {
    ULONG i = 0;
    const WCHAR *str = (const WCHAR *)pMemory;
    while (i < maxsize && str[i]) i++;
    TRACE("string=%s\n", debugstr_wn(str, i));
    pStubMsg->ActualCount = i + 1;
    esize = 2;
  }
  else
  {
    ERR("Unhandled string type: %#x\n", *pFormat);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
  }

  pStubMsg->Offset = 0;
  WriteVariance(pStubMsg);

  size = safe_multiply(esize, pStubMsg->ActualCount);
  safe_copy_to_buffer(pStubMsg, pMemory, size); /* the string itself */

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
  ULONG bufsize, memsize, esize, maxsize;

  TRACE("(pStubMsg == ^%p, *pMemory == ^%p, pFormat == ^%p, fMustAlloc == %u)\n",
    pStubMsg, *ppMemory, pFormat, fMustAlloc);

  maxsize = *(const USHORT *)&pFormat[2];

  ReadVariance(pStubMsg, NULL, maxsize);
  if (pStubMsg->Offset)
  {
    ERR("non-conformant strings can't have Offset (%ld)\n", pStubMsg->Offset);
    RpcRaiseException(RPC_S_INVALID_BOUND);
  }

  if (*pFormat == FC_CSTRING) esize = 1;
  else if (*pFormat == FC_WSTRING) esize = 2;
  else
  {
    ERR("Unhandled string type: %#x\n", *pFormat);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
  }

  memsize = esize * maxsize;
  bufsize = safe_multiply(esize, pStubMsg->ActualCount);

  validate_string_data(pStubMsg, bufsize, esize);

  if (!fMustAlloc && !*ppMemory)
    fMustAlloc = TRUE;
  if (fMustAlloc)
    *ppMemory = NdrAllocate(pStubMsg, memsize);

  safe_copy_from_buffer(pStubMsg, *ppMemory, bufsize);

  if (*pFormat == FC_CSTRING) {
    TRACE("string=%s\n", debugstr_an((char*)*ppMemory, pStubMsg->ActualCount));
  }
  else if (*pFormat == FC_WSTRING) {
    TRACE("string=%s\n", debugstr_wn((LPWSTR)*ppMemory, pStubMsg->ActualCount));
  }

  return NULL;
}

/***********************************************************************
 *           NdrNonConformantStringBufferSize [RPCRT4.@]
 */
void WINAPI NdrNonConformantStringBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
  ULONG esize, maxsize;

  TRACE("(pStubMsg == ^%p, pMemory == ^%p, pFormat == ^%p)\n", pStubMsg, pMemory, pFormat);

  maxsize = *(const USHORT *)&pFormat[2];

  SizeVariance(pStubMsg);

  if (*pFormat == FC_CSTRING)
  {
    ULONG i = 0;
    const char *str = (const char *)pMemory;
    while (i < maxsize && str[i]) i++;
    TRACE("string=%s\n", debugstr_an(str, i));
    pStubMsg->ActualCount = i + 1;
    esize = 1;
  }
  else if (*pFormat == FC_WSTRING)
  {
    ULONG i = 0;
    const WCHAR *str = (const WCHAR *)pMemory;
    while (i < maxsize && str[i]) i++;
    TRACE("string=%s\n", debugstr_wn(str, i));
    pStubMsg->ActualCount = i + 1;
    esize = 2;
  }
  else
  {
    ERR("Unhandled string type: %#x\n", *pFormat);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
  }

  safe_buffer_length_increment(pStubMsg, safe_multiply(esize, pStubMsg->ActualCount));
}

/***********************************************************************
 *           NdrNonConformantStringMemorySize [RPCRT4.@]
 */
ULONG WINAPI NdrNonConformantStringMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                PFORMAT_STRING pFormat)
{
  ULONG bufsize, memsize, esize, maxsize;

  TRACE("(pStubMsg == ^%p, pFormat == ^%p)\n", pStubMsg, pFormat);

  maxsize = *(const USHORT *)&pFormat[2];

  ReadVariance(pStubMsg, NULL, maxsize);

  if (pStubMsg->Offset)
  {
    ERR("non-conformant strings can't have Offset (%ld)\n", pStubMsg->Offset);
    RpcRaiseException(RPC_S_INVALID_BOUND);
  }

  if (*pFormat == FC_CSTRING) esize = 1;
  else if (*pFormat == FC_WSTRING) esize = 2;
  else
  {
    ERR("Unhandled string type: %#x\n", *pFormat);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
  }

  memsize = esize * maxsize;
  bufsize = safe_multiply(esize, pStubMsg->ActualCount);

  validate_string_data(pStubMsg, bufsize, esize);

  safe_buffer_increment(pStubMsg, bufsize);
  pStubMsg->MemorySize += memsize;

  return pStubMsg->MemorySize;
}

/* Complex types */

#include "pshpack1.h"
typedef struct
{
    unsigned char type;
    unsigned char flags_type; /* flags in upper nibble, type in lower nibble */
    ULONG low_value;
    ULONG high_value;
} NDR_RANGE;
#include "poppack.h"

static ULONG EmbeddedComplexSize(MIDL_STUB_MESSAGE *pStubMsg,
                                 PFORMAT_STRING pFormat)
{
  switch (*pFormat) {
  case FC_STRUCT:
  case FC_PSTRUCT:
  case FC_CSTRUCT:
  case FC_BOGUS_STRUCT:
  case FC_SMFARRAY:
  case FC_SMVARRAY:
  case FC_CSTRING:
    return *(const WORD*)&pFormat[2];
  case FC_LGFARRAY:
  case FC_LGVARRAY:
    return *(const ULONG*)&pFormat[2];
  case FC_USER_MARSHAL:
    return *(const WORD*)&pFormat[4];
  case FC_RANGE: {
    switch (((const NDR_RANGE *)pFormat)->flags_type & 0xf) {
    case FC_BYTE:
    case FC_CHAR:
    case FC_SMALL:
    case FC_USMALL:
        return sizeof(UCHAR);
    case FC_WCHAR:
    case FC_SHORT:
    case FC_USHORT:
        return sizeof(USHORT);
    case FC_LONG:
    case FC_ULONG:
    case FC_ENUM32:
        return sizeof(ULONG);
    case FC_FLOAT:
        return sizeof(float);
    case FC_DOUBLE:
        return sizeof(double);
    case FC_HYPER:
        return sizeof(ULONGLONG);
    case FC_ENUM16:
        return sizeof(UINT);
    default:
        ERR("unknown type 0x%x\n", ((const NDR_RANGE *)pFormat)->flags_type & 0xf);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    }
  }
  case FC_NON_ENCAPSULATED_UNION:
    pFormat += 2;
    pFormat = SkipConformance(pStubMsg, pFormat);
    pFormat += *(const SHORT*)pFormat;
    return *(const SHORT*)pFormat;
  case FC_IP:
    return sizeof(void *);
  case FC_WSTRING:
    return *(const WORD*)&pFormat[2] * 2;
  default:
    FIXME("unhandled embedded type %02x\n", *pFormat);
  }
  return 0;
}


static ULONG EmbeddedComplexMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
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
  unsigned char *mem_base = pMemory;
  PFORMAT_STRING desc;
  NDR_MARSHALL m;
  ULONG size;

  while (*pFormat != FC_END) {
    switch (*pFormat) {
    case FC_BYTE:
    case FC_CHAR:
    case FC_SMALL:
    case FC_USMALL:
      TRACE("byte=%d <= %p\n", *(WORD*)pMemory, pMemory);
      safe_copy_to_buffer(pStubMsg, pMemory, 1);
      pMemory += 1;
      break;
    case FC_WCHAR:
    case FC_SHORT:
    case FC_USHORT:
      TRACE("short=%d <= %p\n", *(WORD*)pMemory, pMemory);
      safe_copy_to_buffer(pStubMsg, pMemory, 2);
      pMemory += 2;
      break;
    case FC_ENUM16:
    {
      USHORT val = *(DWORD *)pMemory;
      TRACE("enum16=%ld <= %p\n", *(DWORD*)pMemory, pMemory);
      if (32767 < *(DWORD*)pMemory)
        RpcRaiseException(RPC_X_ENUM_VALUE_OUT_OF_RANGE);
      safe_copy_to_buffer(pStubMsg, &val, 2);
      pMemory += 4;
      break;
    }
    case FC_LONG:
    case FC_ULONG:
    case FC_ENUM32:
      TRACE("long=%ld <= %p\n", *(DWORD*)pMemory, pMemory);
      safe_copy_to_buffer(pStubMsg, pMemory, 4);
      pMemory += 4;
      break;
    case FC_INT3264:
    case FC_UINT3264:
    {
      UINT val = *(UINT_PTR *)pMemory;
      TRACE("int3264=%Id <= %p\n", *(UINT_PTR *)pMemory, pMemory);
      safe_copy_to_buffer(pStubMsg, &val, sizeof(UINT));
      pMemory += sizeof(UINT_PTR);
      break;
    }
    case FC_FLOAT:
      TRACE("float=%f <= %p\n", *(float*)pMemory, pMemory);
      safe_copy_to_buffer(pStubMsg, pMemory, sizeof(float));
      pMemory += sizeof(float);
      break;
    case FC_HYPER:
      TRACE("longlong=%s <= %p\n", wine_dbgstr_longlong(*(ULONGLONG*)pMemory), pMemory);
      safe_copy_to_buffer(pStubMsg, pMemory, 8);
      pMemory += 8;
      break;
    case FC_DOUBLE:
      TRACE("double=%f <= %p\n", *(double*)pMemory, pMemory);
      safe_copy_to_buffer(pStubMsg, pMemory, sizeof(double));
      pMemory += sizeof(double);
      break;
    case FC_RP:
    case FC_UP:
    case FC_OP:
    case FC_FP:
    case FC_POINTER:
    {
      unsigned char *saved_buffer;
      BOOL pointer_buffer_mark_set = FALSE;
      TRACE("pointer=%p <= %p\n", *(unsigned char**)pMemory, pMemory);
      TRACE("pStubMsg->Buffer before %p\n", pStubMsg->Buffer);
      if (*pFormat != FC_POINTER)
        pPointer = pFormat;
      if (*pPointer != FC_RP)
        align_pointer_clear(&pStubMsg->Buffer, 4);
      saved_buffer = pStubMsg->Buffer;
      if (pStubMsg->PointerBufferMark)
      {
        pStubMsg->Buffer = pStubMsg->PointerBufferMark;
        pStubMsg->PointerBufferMark = NULL;
        pointer_buffer_mark_set = TRUE;
      }
      else if (*pPointer != FC_RP)
        safe_buffer_increment(pStubMsg, 4); /* for pointer ID */
      PointerMarshall(pStubMsg, saved_buffer, *(unsigned char**)pMemory, pPointer);
      if (pointer_buffer_mark_set)
      {
        STD_OVERFLOW_CHECK(pStubMsg);
        pStubMsg->PointerBufferMark = pStubMsg->Buffer;
        pStubMsg->Buffer = saved_buffer;
        if (*pPointer != FC_RP)
          safe_buffer_increment(pStubMsg, 4); /* for pointer ID */
      }
      TRACE("pStubMsg->Buffer after %p\n", pStubMsg->Buffer);
      if (*pFormat == FC_POINTER)
        pPointer += 4;
      else
        pFormat += 4;
      pMemory += sizeof(void *);
      break;
    }
    case FC_ALIGNM2:
      align_pointer_offset(&pMemory, mem_base, 2);
      break;
    case FC_ALIGNM4:
      align_pointer_offset(&pMemory, mem_base, 4);
      break;
    case FC_ALIGNM8:
      align_pointer_offset(&pMemory, mem_base, 8);
      break;
    case FC_STRUCTPAD1:
    case FC_STRUCTPAD2:
    case FC_STRUCTPAD3:
    case FC_STRUCTPAD4:
    case FC_STRUCTPAD5:
    case FC_STRUCTPAD6:
    case FC_STRUCTPAD7:
      pMemory += *pFormat - FC_STRUCTPAD1 + 1;
      break;
    case FC_EMBEDDED_COMPLEX:
      pMemory += pFormat[1];
      pFormat += 2;
      desc = pFormat + *(const SHORT*)pFormat;
      size = EmbeddedComplexSize(pStubMsg, desc);
      TRACE("embedded complex (size=%ld) <= %p\n", size, pMemory);
      m = NdrMarshaller[*desc & NDR_TABLE_MASK];
      if (m)
      {
        /* for some reason interface pointers aren't generated as
         * FC_POINTER, but instead as FC_EMBEDDED_COMPLEX, yet
         * they still need the dereferencing treatment that pointers are
         * given */
        if (*desc == FC_IP)
          m(pStubMsg, *(unsigned char **)pMemory, desc);
        else
          m(pStubMsg, pMemory, desc);
      }
      else FIXME("no marshaller for embedded type %02x\n", *desc);
      pMemory += size;
      pFormat += 2;
      continue;
    case FC_PAD:
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
                                         PFORMAT_STRING pPointer,
                                         unsigned char fMustAlloc)
{
  unsigned char *mem_base = pMemory;
  PFORMAT_STRING desc;
  NDR_UNMARSHALL m;
  ULONG size;

  while (*pFormat != FC_END) {
    switch (*pFormat) {
    case FC_BYTE:
    case FC_CHAR:
    case FC_SMALL:
    case FC_USMALL:
      safe_copy_from_buffer(pStubMsg, pMemory, 1);
      TRACE("byte=%d => %p\n", *(WORD*)pMemory, pMemory);
      pMemory += 1;
      break;
    case FC_WCHAR:
    case FC_SHORT:
    case FC_USHORT:
      safe_copy_from_buffer(pStubMsg, pMemory, 2);
      TRACE("short=%d => %p\n", *(WORD*)pMemory, pMemory);
      pMemory += 2;
      break;
    case FC_ENUM16:
    {
      WORD val;
      safe_copy_from_buffer(pStubMsg, &val, 2);
      *(DWORD*)pMemory = val;
      TRACE("enum16=%ld => %p\n", *(DWORD*)pMemory, pMemory);
      if (32767 < *(DWORD*)pMemory)
        RpcRaiseException(RPC_X_ENUM_VALUE_OUT_OF_RANGE);
      pMemory += 4;
      break;
    }
    case FC_LONG:
    case FC_ULONG:
    case FC_ENUM32:
      safe_copy_from_buffer(pStubMsg, pMemory, 4);
      TRACE("long=%ld => %p\n", *(DWORD*)pMemory, pMemory);
      pMemory += 4;
      break;
    case FC_INT3264:
    {
      INT val;
      safe_copy_from_buffer(pStubMsg, &val, 4);
      *(INT_PTR *)pMemory = val;
      TRACE("int3264=%Id => %p\n", *(INT_PTR*)pMemory, pMemory);
      pMemory += sizeof(INT_PTR);
      break;
    }
    case FC_UINT3264:
    {
      UINT val;
      safe_copy_from_buffer(pStubMsg, &val, 4);
      *(UINT_PTR *)pMemory = val;
      TRACE("uint3264=%Id => %p\n", *(UINT_PTR*)pMemory, pMemory);
      pMemory += sizeof(UINT_PTR);
      break;
    }
    case FC_FLOAT:
      safe_copy_from_buffer(pStubMsg, pMemory, sizeof(float));
      TRACE("float=%f => %p\n", *(float*)pMemory, pMemory);
      pMemory += sizeof(float);
      break;
    case FC_HYPER:
      safe_copy_from_buffer(pStubMsg, pMemory, 8);
      TRACE("longlong=%s => %p\n", wine_dbgstr_longlong(*(ULONGLONG*)pMemory), pMemory);
      pMemory += 8;
      break;
    case FC_DOUBLE:
      safe_copy_from_buffer(pStubMsg, pMemory, sizeof(double));
      TRACE("double=%f => %p\n", *(double*)pMemory, pMemory);
      pMemory += sizeof(double);
      break;
    case FC_RP:
    case FC_UP:
    case FC_OP:
    case FC_FP:
    case FC_POINTER:
    {
      unsigned char *saved_buffer;
      BOOL pointer_buffer_mark_set = FALSE;
      TRACE("pointer => %p\n", pMemory);
      if (*pFormat != FC_POINTER)
        pPointer = pFormat;
      if (*pPointer != FC_RP)
        align_pointer(&pStubMsg->Buffer, 4);
      saved_buffer = pStubMsg->Buffer;
      if (pStubMsg->PointerBufferMark)
      {
        pStubMsg->Buffer = pStubMsg->PointerBufferMark;
        pStubMsg->PointerBufferMark = NULL;
        pointer_buffer_mark_set = TRUE;
      }
      else if (*pPointer != FC_RP)
        safe_buffer_increment(pStubMsg, 4); /* for pointer ID */

      PointerUnmarshall(pStubMsg, saved_buffer, (unsigned char**)pMemory, *(unsigned char**)pMemory, pPointer, fMustAlloc);
      if (pointer_buffer_mark_set)
      {
        STD_OVERFLOW_CHECK(pStubMsg);
        pStubMsg->PointerBufferMark = pStubMsg->Buffer;
        pStubMsg->Buffer = saved_buffer;
        if (*pPointer != FC_RP)
          safe_buffer_increment(pStubMsg, 4); /* for pointer ID */
      }
      if (*pFormat == FC_POINTER)
        pPointer += 4;
      else
        pFormat += 4;
      pMemory += sizeof(void *);
      break;
    }
    case FC_ALIGNM2:
      align_pointer_offset_clear(&pMemory, mem_base, 2);
      break;
    case FC_ALIGNM4:
      align_pointer_offset_clear(&pMemory, mem_base, 4);
      break;
    case FC_ALIGNM8:
      align_pointer_offset_clear(&pMemory, mem_base, 8);
      break;
    case FC_STRUCTPAD1:
    case FC_STRUCTPAD2:
    case FC_STRUCTPAD3:
    case FC_STRUCTPAD4:
    case FC_STRUCTPAD5:
    case FC_STRUCTPAD6:
    case FC_STRUCTPAD7:
      memset(pMemory, 0, *pFormat - FC_STRUCTPAD1 + 1);
      pMemory += *pFormat - FC_STRUCTPAD1 + 1;
      break;
    case FC_EMBEDDED_COMPLEX:
      pMemory += pFormat[1];
      pFormat += 2;
      desc = pFormat + *(const SHORT*)pFormat;
      size = EmbeddedComplexSize(pStubMsg, desc);
      TRACE("embedded complex (size=%ld) => %p\n", size, pMemory);
      if (fMustAlloc)
        /* we can't pass fMustAlloc=TRUE into the marshaller for this type
         * since the type is part of the memory block that is encompassed by
         * the whole complex type. Memory is forced to allocate when pointers
         * are set to NULL, so we emulate that part of fMustAlloc=TRUE by
         * clearing the memory we pass in to the unmarshaller */
        memset(pMemory, 0, size);
      m = NdrUnmarshaller[*desc & NDR_TABLE_MASK];
      if (m)
      {
        /* for some reason interface pointers aren't generated as
         * FC_POINTER, but instead as FC_EMBEDDED_COMPLEX, yet
         * they still need the dereferencing treatment that pointers are
         * given */
        if (*desc == FC_IP)
          m(pStubMsg, (unsigned char **)pMemory, desc, FALSE);
        else
          m(pStubMsg, &pMemory, desc, FALSE);
      }
      else FIXME("no unmarshaller for embedded type %02x\n", *desc);
      pMemory += size;
      pFormat += 2;
      continue;
    case FC_PAD:
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
  unsigned char *mem_base = pMemory;
  PFORMAT_STRING desc;
  NDR_BUFFERSIZE m;
  ULONG size;

  while (*pFormat != FC_END) {
    switch (*pFormat) {
    case FC_BYTE:
    case FC_CHAR:
    case FC_SMALL:
    case FC_USMALL:
      safe_buffer_length_increment(pStubMsg, 1);
      pMemory += 1;
      break;
    case FC_WCHAR:
    case FC_SHORT:
    case FC_USHORT:
      safe_buffer_length_increment(pStubMsg, 2);
      pMemory += 2;
      break;
    case FC_ENUM16:
      safe_buffer_length_increment(pStubMsg, 2);
      pMemory += 4;
      break;
    case FC_LONG:
    case FC_ULONG:
    case FC_ENUM32:
    case FC_FLOAT:
      safe_buffer_length_increment(pStubMsg, 4);
      pMemory += 4;
      break;
    case FC_INT3264:
    case FC_UINT3264:
      safe_buffer_length_increment(pStubMsg, 4);
      pMemory += sizeof(INT_PTR);
      break;
    case FC_HYPER:
    case FC_DOUBLE:
      safe_buffer_length_increment(pStubMsg, 8);
      pMemory += 8;
      break;
    case FC_RP:
    case FC_UP:
    case FC_OP:
    case FC_FP:
    case FC_POINTER:
      if (*pFormat != FC_POINTER)
        pPointer = pFormat;
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
      if (*pPointer != FC_RP)
      {
        align_length(&pStubMsg->BufferLength, 4);
        safe_buffer_length_increment(pStubMsg, 4);
      }
      if (*pFormat == FC_POINTER)
        pPointer += 4;
      else
        pFormat += 4;
      pMemory += sizeof(void*);
      break;
    case FC_ALIGNM2:
      align_pointer_offset(&pMemory, mem_base, 2);
      break;
    case FC_ALIGNM4:
      align_pointer_offset(&pMemory, mem_base, 4);
      break;
    case FC_ALIGNM8:
      align_pointer_offset(&pMemory, mem_base, 8);
      break;
    case FC_STRUCTPAD1:
    case FC_STRUCTPAD2:
    case FC_STRUCTPAD3:
    case FC_STRUCTPAD4:
    case FC_STRUCTPAD5:
    case FC_STRUCTPAD6:
    case FC_STRUCTPAD7:
      pMemory += *pFormat - FC_STRUCTPAD1 + 1;
      break;
    case FC_EMBEDDED_COMPLEX:
      pMemory += pFormat[1];
      pFormat += 2;
      desc = pFormat + *(const SHORT*)pFormat;
      size = EmbeddedComplexSize(pStubMsg, desc);
      m = NdrBufferSizer[*desc & NDR_TABLE_MASK];
      if (m)
      {
        /* for some reason interface pointers aren't generated as
         * FC_POINTER, but instead as FC_EMBEDDED_COMPLEX, yet
         * they still need the dereferencing treatment that pointers are
         * given */
        if (*desc == FC_IP)
          m(pStubMsg, *(unsigned char **)pMemory, desc);
        else
          m(pStubMsg, pMemory, desc);
      }
      else FIXME("no buffersizer for embedded type %02x\n", *desc);
      pMemory += size;
      pFormat += 2;
      continue;
    case FC_PAD:
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
  unsigned char *mem_base = pMemory;
  PFORMAT_STRING desc;
  NDR_FREE m;
  ULONG size;

  while (*pFormat != FC_END) {
    switch (*pFormat) {
    case FC_BYTE:
    case FC_CHAR:
    case FC_SMALL:
    case FC_USMALL:
      pMemory += 1;
      break;
    case FC_WCHAR:
    case FC_SHORT:
    case FC_USHORT:
      pMemory += 2;
      break;
    case FC_LONG:
    case FC_ULONG:
    case FC_ENUM16:
    case FC_ENUM32:
    case FC_FLOAT:
      pMemory += 4;
      break;
    case FC_INT3264:
    case FC_UINT3264:
      pMemory += sizeof(INT_PTR);
      break;
    case FC_HYPER:
    case FC_DOUBLE:
      pMemory += 8;
      break;
    case FC_RP:
    case FC_UP:
    case FC_OP:
    case FC_FP:
    case FC_POINTER:
      if (*pFormat != FC_POINTER)
        pPointer = pFormat;
      NdrPointerFree(pStubMsg, *(unsigned char**)pMemory, pPointer);
      if (*pFormat == FC_POINTER)
        pPointer += 4;
      else
        pFormat += 4;
      pMemory += sizeof(void *);
      break;
    case FC_ALIGNM2:
      align_pointer_offset(&pMemory, mem_base, 2);
      break;
    case FC_ALIGNM4:
      align_pointer_offset(&pMemory, mem_base, 4);
      break;
    case FC_ALIGNM8:
      align_pointer_offset(&pMemory, mem_base, 8);
      break;
    case FC_STRUCTPAD1:
    case FC_STRUCTPAD2:
    case FC_STRUCTPAD3:
    case FC_STRUCTPAD4:
    case FC_STRUCTPAD5:
    case FC_STRUCTPAD6:
    case FC_STRUCTPAD7:
      pMemory += *pFormat - FC_STRUCTPAD1 + 1;
      break;
    case FC_EMBEDDED_COMPLEX:
      pMemory += pFormat[1];
      pFormat += 2;
      desc = pFormat + *(const SHORT*)pFormat;
      size = EmbeddedComplexSize(pStubMsg, desc);
      m = NdrFreer[*desc & NDR_TABLE_MASK];
      if (m)
      {
        /* for some reason interface pointers aren't generated as
         * FC_POINTER, but instead as FC_EMBEDDED_COMPLEX, yet
         * they still need the dereferencing treatment that pointers are
         * given */
        if (*desc == FC_IP)
          m(pStubMsg, *(unsigned char **)pMemory, desc);
        else
          m(pStubMsg, pMemory, desc);
      }
      pMemory += size;
      pFormat += 2;
      continue;
    case FC_PAD:
      break;
    default:
      FIXME("unhandled format 0x%02x\n", *pFormat);
    }
    pFormat++;
  }

  return pMemory;
}

static ULONG ComplexStructMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                     PFORMAT_STRING pFormat,
                                     PFORMAT_STRING pPointer)
{
  PFORMAT_STRING desc;
  ULONG size = 0;

  while (*pFormat != FC_END) {
    switch (*pFormat) {
    case FC_BYTE:
    case FC_CHAR:
    case FC_SMALL:
    case FC_USMALL:
      size += 1;
      safe_buffer_increment(pStubMsg, 1);
      break;
    case FC_WCHAR:
    case FC_SHORT:
    case FC_USHORT:
      size += 2;
      safe_buffer_increment(pStubMsg, 2);
      break;
    case FC_ENUM16:
      size += 4;
      safe_buffer_increment(pStubMsg, 2);
      break;
    case FC_LONG:
    case FC_ULONG:
    case FC_ENUM32:
    case FC_FLOAT:
      size += 4;
      safe_buffer_increment(pStubMsg, 4);
      break;
    case FC_INT3264:
    case FC_UINT3264:
      size += sizeof(INT_PTR);
      safe_buffer_increment(pStubMsg, 4);
      break;
    case FC_HYPER:
    case FC_DOUBLE:
      size += 8;
      safe_buffer_increment(pStubMsg, 8);
      break;
    case FC_RP:
    case FC_UP:
    case FC_OP:
    case FC_FP:
    case FC_POINTER:
    {
      unsigned char *saved_buffer;
      BOOL pointer_buffer_mark_set = FALSE;
      if (*pFormat != FC_POINTER)
        pPointer = pFormat;
      if (*pPointer != FC_RP)
        align_pointer(&pStubMsg->Buffer, 4);
      saved_buffer = pStubMsg->Buffer;
      if (pStubMsg->PointerBufferMark)
      {
        pStubMsg->Buffer = pStubMsg->PointerBufferMark;
        pStubMsg->PointerBufferMark = NULL;
        pointer_buffer_mark_set = TRUE;
      }
      else if (*pPointer != FC_RP)
        safe_buffer_increment(pStubMsg, 4); /* for pointer ID */

      if (!pStubMsg->IgnoreEmbeddedPointers)
        PointerMemorySize(pStubMsg, saved_buffer, pPointer);
      if (pointer_buffer_mark_set)
      {
        STD_OVERFLOW_CHECK(pStubMsg);
        pStubMsg->PointerBufferMark = pStubMsg->Buffer;
        pStubMsg->Buffer = saved_buffer;
        if (*pPointer != FC_RP)
          safe_buffer_increment(pStubMsg, 4); /* for pointer ID */
      }
      if (*pFormat == FC_POINTER)
        pPointer += 4;
      else
        pFormat += 4;
      size += sizeof(void *);
      break;
    }
    case FC_ALIGNM2:
      align_length(&size, 2);
      break;
    case FC_ALIGNM4:
      align_length(&size, 4);
      break;
    case FC_ALIGNM8:
      align_length(&size, 8);
      break;
    case FC_STRUCTPAD1:
    case FC_STRUCTPAD2:
    case FC_STRUCTPAD3:
    case FC_STRUCTPAD4:
    case FC_STRUCTPAD5:
    case FC_STRUCTPAD6:
    case FC_STRUCTPAD7:
      size += *pFormat - FC_STRUCTPAD1 + 1;
      break;
    case FC_EMBEDDED_COMPLEX:
      size += pFormat[1];
      pFormat += 2;
      desc = pFormat + *(const SHORT*)pFormat;
      size += EmbeddedComplexMemorySize(pStubMsg, desc);
      pFormat += 2;
      continue;
    case FC_PAD:
      break;
    default:
      FIXME("unhandled format 0x%02x\n", *pFormat);
    }
    pFormat++;
  }

  return size;
}

ULONG ComplexStructSize(PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat)
{
  PFORMAT_STRING desc;
  ULONG size = 0;

  while (*pFormat != FC_END) {
    switch (*pFormat) {
    case FC_BYTE:
    case FC_CHAR:
    case FC_SMALL:
    case FC_USMALL:
      size += 1;
      break;
    case FC_WCHAR:
    case FC_SHORT:
    case FC_USHORT:
      size += 2;
      break;
    case FC_LONG:
    case FC_ULONG:
    case FC_ENUM16:
    case FC_ENUM32:
    case FC_FLOAT:
      size += 4;
      break;
    case FC_INT3264:
    case FC_UINT3264:
      size += sizeof(INT_PTR);
      break;
    case FC_HYPER:
    case FC_DOUBLE:
      size += 8;
      break;
    case FC_RP:
    case FC_UP:
    case FC_OP:
    case FC_FP:
    case FC_POINTER:
      size += sizeof(void *);
      if (*pFormat != FC_POINTER)
        pFormat += 4;
      break;
    case FC_ALIGNM2:
      align_length(&size, 2);
      break;
    case FC_ALIGNM4:
      align_length(&size, 4);
      break;
    case FC_ALIGNM8:
      align_length(&size, 8);
      break;
    case FC_STRUCTPAD1:
    case FC_STRUCTPAD2:
    case FC_STRUCTPAD3:
    case FC_STRUCTPAD4:
    case FC_STRUCTPAD5:
    case FC_STRUCTPAD6:
    case FC_STRUCTPAD7:
      size += *pFormat - FC_STRUCTPAD1 + 1;
      break;
    case FC_EMBEDDED_COMPLEX:
      size += pFormat[1];
      pFormat += 2;
      desc = pFormat + *(const SHORT*)pFormat;
      size += EmbeddedComplexSize(pStubMsg, desc);
      pFormat += 2;
      continue;
    case FC_PAD:
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
  BOOL pointer_buffer_mark_set = FALSE;
  ULONG count = 0;
  ULONG max_count = 0;
  ULONG offset = 0;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  if (!pStubMsg->PointerBufferMark)
  {
    int saved_ignore_embedded = pStubMsg->IgnoreEmbeddedPointers;
    /* save buffer length */
    ULONG saved_buffer_length = pStubMsg->BufferLength;

    /* get the buffer pointer after complex array data, but before
     * pointer data */
    pStubMsg->BufferLength = pStubMsg->Buffer - (unsigned char *)pStubMsg->RpcMsg->Buffer;
    pStubMsg->IgnoreEmbeddedPointers = 1;
    NdrComplexStructBufferSize(pStubMsg, pMemory, pFormat);
    pStubMsg->IgnoreEmbeddedPointers = saved_ignore_embedded;

    /* save it for use by embedded pointer code later */
    pStubMsg->PointerBufferMark = (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength;
    TRACE("difference = 0x%Ix\n", pStubMsg->PointerBufferMark - pStubMsg->Buffer);
    pointer_buffer_mark_set = TRUE;

    /* restore the original buffer length */
    pStubMsg->BufferLength = saved_buffer_length;
  }

  align_pointer_clear(&pStubMsg->Buffer, pFormat[1] + 1);

  pFormat += 4;
  if (*(const SHORT*)pFormat) conf_array = pFormat + *(const SHORT*)pFormat;
  pFormat += 2;
  if (*(const WORD*)pFormat) pointer_desc = pFormat + *(const WORD*)pFormat;
  pFormat += 2;

  pStubMsg->Memory = pMemory;

  if (conf_array)
  {
    ULONG struct_size = ComplexStructSize(pStubMsg, pFormat);
    array_compute_and_write_conformance(conf_array[0], pStubMsg,
                                        pMemory + struct_size, conf_array);
    /* these could be changed in ComplexMarshall so save them for later */
    max_count = pStubMsg->MaxCount;
    count = pStubMsg->ActualCount;
    offset = pStubMsg->Offset;
  }

  pMemory = ComplexMarshall(pStubMsg, pMemory, pFormat, pointer_desc);

  if (conf_array)
  {
    pStubMsg->MaxCount = max_count;
    pStubMsg->ActualCount = count;
    pStubMsg->Offset = offset;
    array_write_variance_and_marshall(conf_array[0], pStubMsg, pMemory,
                                      conf_array, TRUE /* fHasPointers */);
  }

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
  BOOL pointer_buffer_mark_set = FALSE;
  ULONG count = 0;
  ULONG max_count = 0;
  ULONG offset = 0;
  ULONG array_size = 0;

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
    TRACE("difference = 0x%Ix\n", pStubMsg->PointerBufferMark - saved_buffer);
    pointer_buffer_mark_set = TRUE;

    /* restore the original buffer */
    pStubMsg->Buffer = saved_buffer;
  }

  align_pointer(&pStubMsg->Buffer, pFormat[1] + 1);

  pFormat += 4;
  if (*(const SHORT*)pFormat) conf_array = pFormat + *(const SHORT*)pFormat;
  pFormat += 2;
  if (*(const WORD*)pFormat) pointer_desc = pFormat + *(const WORD*)pFormat;
  pFormat += 2;

  if (conf_array)
  {
    array_size = array_read_conformance(conf_array[0], pStubMsg, conf_array);
    size += array_size;

    /* these could be changed in ComplexMarshall so save them for later */
    max_count = pStubMsg->MaxCount;
    count = pStubMsg->ActualCount;
    offset = pStubMsg->Offset;
  }

  if (!fMustAlloc && !*ppMemory)
    fMustAlloc = TRUE;
  if (fMustAlloc)
    *ppMemory = NdrAllocateZero(pStubMsg, size);

  pMemory = ComplexUnmarshall(pStubMsg, *ppMemory, pFormat, pointer_desc, fMustAlloc);

  if (conf_array)
  {
    pStubMsg->MaxCount = max_count;
    pStubMsg->ActualCount = count;
    pStubMsg->Offset = offset;
    if (fMustAlloc)
      memset(pMemory, 0, array_size);
    array_read_variance_and_unmarshall(conf_array[0], pStubMsg, &pMemory,
                                       conf_array, FALSE,
                                       FALSE /* fUseBufferMemoryServer */,
                                       TRUE /* fUnmarshall */);
  }

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
  ULONG count = 0;
  ULONG max_count = 0;
  ULONG offset = 0;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  align_length(&pStubMsg->BufferLength, pFormat[1] + 1);

  if(!pStubMsg->IgnoreEmbeddedPointers && !pStubMsg->PointerLength)
  {
    int saved_ignore_embedded = pStubMsg->IgnoreEmbeddedPointers;
    ULONG saved_buffer_length = pStubMsg->BufferLength;

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
  if (*(const SHORT*)pFormat) conf_array = pFormat + *(const SHORT*)pFormat;
  pFormat += 2;
  if (*(const WORD*)pFormat) pointer_desc = pFormat + *(const WORD*)pFormat;
  pFormat += 2;

  pStubMsg->Memory = pMemory;

  if (conf_array)
  {
    ULONG struct_size = ComplexStructSize(pStubMsg, pFormat);
    array_compute_and_size_conformance(conf_array[0], pStubMsg, pMemory + struct_size,
                                       conf_array);

    /* these could be changed in ComplexMarshall so save them for later */
    max_count = pStubMsg->MaxCount;
    count = pStubMsg->ActualCount;
    offset = pStubMsg->Offset;
  }

  pMemory = ComplexBufferSize(pStubMsg, pMemory, pFormat, pointer_desc);

  if (conf_array)
  {
    pStubMsg->MaxCount = max_count;
    pStubMsg->ActualCount = count;
    pStubMsg->Offset = offset;
    array_buffer_size(conf_array[0], pStubMsg, pMemory, conf_array,
                      TRUE /* fHasPointers */);
  }

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
  ULONG count = 0;
  ULONG max_count = 0;
  ULONG offset = 0;

  TRACE("(%p,%p)\n", pStubMsg, pFormat);

  align_pointer(&pStubMsg->Buffer, pFormat[1] + 1);

  pFormat += 4;
  if (*(const SHORT*)pFormat) conf_array = pFormat + *(const SHORT*)pFormat;
  pFormat += 2;
  if (*(const WORD*)pFormat) pointer_desc = pFormat + *(const WORD*)pFormat;
  pFormat += 2;

  if (conf_array)
  {
    array_read_conformance(conf_array[0], pStubMsg, conf_array);

    /* these could be changed in ComplexStructMemorySize so save them for
     * later */
    max_count = pStubMsg->MaxCount;
    count = pStubMsg->ActualCount;
    offset = pStubMsg->Offset;
  }

  ComplexStructMemorySize(pStubMsg, pFormat, pointer_desc);

  if (conf_array)
  {
    pStubMsg->MaxCount = max_count;
    pStubMsg->ActualCount = count;
    pStubMsg->Offset = offset;
    array_memory_size(conf_array[0], pStubMsg, conf_array,
                      TRUE /* fHasPointers */);
  }

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
  if (*(const SHORT*)pFormat) conf_array = pFormat + *(const SHORT*)pFormat;
  pFormat += 2;
  if (*(const WORD*)pFormat) pointer_desc = pFormat + *(const WORD*)pFormat;
  pFormat += 2;

  pStubMsg->Memory = pMemory;

  pMemory = ComplexFree(pStubMsg, pMemory, pFormat, pointer_desc);

  if (conf_array)
    array_free(conf_array[0], pStubMsg, pMemory, conf_array,
               TRUE /* fHasPointers */);

  pStubMsg->Memory = OldMemory;
}

/***********************************************************************
 *           NdrConformantArrayMarshall [RPCRT4.@]
 */
unsigned char * WINAPI NdrConformantArrayMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                                  unsigned char *pMemory,
                                                  PFORMAT_STRING pFormat)
{
  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  if (pFormat[0] != FC_CARRAY)
  {
    ERR("invalid format = 0x%x\n", pFormat[0]);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
  }

  array_compute_and_write_conformance(FC_CARRAY, pStubMsg, pMemory,
                                      pFormat);
  array_write_variance_and_marshall(FC_CARRAY, pStubMsg, pMemory, pFormat,
                                    TRUE /* fHasPointers */);

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
  TRACE("(%p,%p,%p,%d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);
  if (pFormat[0] != FC_CARRAY)
  {
    ERR("invalid format = 0x%x\n", pFormat[0]);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
  }

  array_read_conformance(FC_CARRAY, pStubMsg, pFormat);
  array_read_variance_and_unmarshall(FC_CARRAY, pStubMsg, ppMemory, pFormat,
                                     fMustAlloc,
                                     TRUE /* fUseBufferMemoryServer */,
                                     TRUE /* fUnmarshall */);

  return NULL;
}

/***********************************************************************
 *           NdrConformantArrayBufferSize [RPCRT4.@]
 */
void WINAPI NdrConformantArrayBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                         unsigned char *pMemory,
                                         PFORMAT_STRING pFormat)
{
  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  if (pFormat[0] != FC_CARRAY)
  {
    ERR("invalid format = 0x%x\n", pFormat[0]);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
  }

  array_compute_and_size_conformance(FC_CARRAY, pStubMsg, pMemory, pFormat);
  array_buffer_size(FC_CARRAY, pStubMsg, pMemory, pFormat,
                    TRUE /* fHasPointers */);
}

/***********************************************************************
 *           NdrConformantArrayMemorySize [RPCRT4.@]
 */
ULONG WINAPI NdrConformantArrayMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                          PFORMAT_STRING pFormat)
{
  TRACE("(%p,%p)\n", pStubMsg, pFormat);
  if (pFormat[0] != FC_CARRAY)
  {
    ERR("invalid format = 0x%x\n", pFormat[0]);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
  }

  array_read_conformance(FC_CARRAY, pStubMsg, pFormat);
  array_memory_size(FC_CARRAY, pStubMsg, pFormat, TRUE /* fHasPointers */);

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
  if (pFormat[0] != FC_CARRAY)
  {
    ERR("invalid format = 0x%x\n", pFormat[0]);
    RpcRaiseException(RPC_X_BAD_STUB_DATA);
  }

  array_free(FC_CARRAY, pStubMsg, pMemory, pFormat,
             TRUE /* fHasPointers */);
}


/***********************************************************************
 *           NdrConformantVaryingArrayMarshall  [RPCRT4.@]
 */
unsigned char* WINAPI NdrConformantVaryingArrayMarshall( PMIDL_STUB_MESSAGE pStubMsg,
                                                         unsigned char* pMemory,
                                                         PFORMAT_STRING pFormat )
{
    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);

    if (pFormat[0] != FC_CVARRAY)
    {
        ERR("invalid format type %x\n", pFormat[0]);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }

    array_compute_and_write_conformance(FC_CVARRAY, pStubMsg, pMemory,
                                        pFormat);
    array_write_variance_and_marshall(FC_CVARRAY, pStubMsg, pMemory,
                                      pFormat, TRUE /* fHasPointers */);

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
    TRACE("(%p, %p, %p, %d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);

    if (pFormat[0] != FC_CVARRAY)
    {
        ERR("invalid format type %x\n", pFormat[0]);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }

    array_read_conformance(FC_CVARRAY, pStubMsg, pFormat);
    array_read_variance_and_unmarshall(FC_CVARRAY, pStubMsg, ppMemory,
                                       pFormat, fMustAlloc,
                                       TRUE /* fUseBufferMemoryServer */,
                                       TRUE /* fUnmarshall */);

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

    if (pFormat[0] != FC_CVARRAY)
    {
        ERR("invalid format type %x\n", pFormat[0]);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }

    array_free(FC_CVARRAY, pStubMsg, pMemory, pFormat,
               TRUE /* fHasPointers */);
}


/***********************************************************************
 *           NdrConformantVaryingArrayBufferSize  [RPCRT4.@]
 */
void WINAPI NdrConformantVaryingArrayBufferSize( PMIDL_STUB_MESSAGE pStubMsg,
                                                 unsigned char* pMemory, PFORMAT_STRING pFormat )
{
    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);

    if (pFormat[0] != FC_CVARRAY)
    {
        ERR("invalid format type %x\n", pFormat[0]);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }

    array_compute_and_size_conformance(FC_CVARRAY, pStubMsg, pMemory,
                                       pFormat);
    array_buffer_size(FC_CVARRAY, pStubMsg, pMemory, pFormat,
                      TRUE /* fHasPointers */);
}


/***********************************************************************
 *           NdrConformantVaryingArrayMemorySize  [RPCRT4.@]
 */
ULONG WINAPI NdrConformantVaryingArrayMemorySize( PMIDL_STUB_MESSAGE pStubMsg,
                                                  PFORMAT_STRING pFormat )
{
    TRACE("(%p, %p)\n", pStubMsg, pFormat);

    if (pFormat[0] != FC_CVARRAY)
    {
        ERR("invalid format type %x\n", pFormat[0]);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return pStubMsg->MemorySize;
    }

    array_read_conformance(FC_CVARRAY, pStubMsg, pFormat);
    array_memory_size(FC_CVARRAY, pStubMsg, pFormat,
                      TRUE /* fHasPointers */);

    return pStubMsg->MemorySize;
}


/***********************************************************************
 *           NdrComplexArrayMarshall [RPCRT4.@]
 */
unsigned char * WINAPI NdrComplexArrayMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                               unsigned char *pMemory,
                                               PFORMAT_STRING pFormat)
{
  BOOL pointer_buffer_mark_set = FALSE;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  if (pFormat[0] != FC_BOGUS_ARRAY)
  {
      ERR("invalid format type %x\n", pFormat[0]);
      RpcRaiseException(RPC_S_INTERNAL_ERROR);
      return NULL;
  }

  if (!pStubMsg->PointerBufferMark)
  {
    /* save buffer fields that may be changed by buffer sizer functions
     * and that may be needed later on */
    int saved_ignore_embedded = pStubMsg->IgnoreEmbeddedPointers;
    ULONG saved_buffer_length = pStubMsg->BufferLength;
    ULONG_PTR saved_max_count = pStubMsg->MaxCount;
    ULONG saved_offset = pStubMsg->Offset;
    ULONG saved_actual_count = pStubMsg->ActualCount;

    /* get the buffer pointer after complex array data, but before
     * pointer data */
    pStubMsg->BufferLength = pStubMsg->Buffer - (unsigned char *)pStubMsg->RpcMsg->Buffer;
    pStubMsg->IgnoreEmbeddedPointers = 1;
    NdrComplexArrayBufferSize(pStubMsg, pMemory, pFormat);
    pStubMsg->IgnoreEmbeddedPointers = saved_ignore_embedded;

    /* save it for use by embedded pointer code later */
    pStubMsg->PointerBufferMark = (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength;
    TRACE("difference = 0x%Ix\n", pStubMsg->Buffer - (unsigned char *)pStubMsg->RpcMsg->Buffer);
    pointer_buffer_mark_set = TRUE;

    /* restore fields */
    pStubMsg->ActualCount = saved_actual_count;
    pStubMsg->Offset = saved_offset;
    pStubMsg->MaxCount = saved_max_count;
    pStubMsg->BufferLength = saved_buffer_length;
  }

  array_compute_and_write_conformance(FC_BOGUS_ARRAY, pStubMsg, pMemory, pFormat);
  array_write_variance_and_marshall(FC_BOGUS_ARRAY, pStubMsg,
                                    pMemory, pFormat, TRUE /* fHasPointers */);

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
  unsigned char *saved_buffer;
  BOOL pointer_buffer_mark_set = FALSE;
  int saved_ignore_embedded;

  TRACE("(%p,%p,%p,%d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);

  if (pFormat[0] != FC_BOGUS_ARRAY)
  {
      ERR("invalid format type %x\n", pFormat[0]);
      RpcRaiseException(RPC_S_INTERNAL_ERROR);
      return NULL;
  }

  saved_ignore_embedded = pStubMsg->IgnoreEmbeddedPointers;
  /* save buffer pointer */
  saved_buffer = pStubMsg->Buffer;
  /* get the buffer pointer after complex array data, but before
   * pointer data */
  pStubMsg->IgnoreEmbeddedPointers = 1;
  pStubMsg->MemorySize = 0;
  NdrComplexArrayMemorySize(pStubMsg, pFormat);
  pStubMsg->IgnoreEmbeddedPointers = saved_ignore_embedded;

  TRACE("difference = 0x%Ix\n", pStubMsg->Buffer - saved_buffer);
  if (!pStubMsg->PointerBufferMark)
  {
    /* save it for use by embedded pointer code later */
    pStubMsg->PointerBufferMark = pStubMsg->Buffer;
    pointer_buffer_mark_set = TRUE;
  }
  /* restore the original buffer */
  pStubMsg->Buffer = saved_buffer;

  array_read_conformance(FC_BOGUS_ARRAY, pStubMsg, pFormat);
  array_read_variance_and_unmarshall(FC_BOGUS_ARRAY, pStubMsg, ppMemory, pFormat, fMustAlloc,
                                     TRUE /* fUseBufferMemoryServer */, TRUE /* fUnmarshall */);

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
  int pointer_length_set = 0;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  if (pFormat[0] != FC_BOGUS_ARRAY)
  {
      ERR("invalid format type %x\n", pFormat[0]);
      RpcRaiseException(RPC_S_INTERNAL_ERROR);
      return;
  }

  if (!pStubMsg->IgnoreEmbeddedPointers && !pStubMsg->PointerLength)
  {
    /* save buffer fields that may be changed by buffer sizer functions
     * and that may be needed later on */
    int saved_ignore_embedded = pStubMsg->IgnoreEmbeddedPointers;
    ULONG saved_buffer_length = pStubMsg->BufferLength;
    ULONG_PTR saved_max_count = pStubMsg->MaxCount;
    ULONG saved_offset = pStubMsg->Offset;
    ULONG saved_actual_count = pStubMsg->ActualCount;

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

  array_compute_and_size_conformance(FC_BOGUS_ARRAY, pStubMsg, pMemory, pFormat);
  array_buffer_size(FC_BOGUS_ARRAY, pStubMsg, pMemory, pFormat, TRUE /* fHasPointers */);

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
  TRACE("(%p,%p)\n", pStubMsg, pFormat);

  if (pFormat[0] != FC_BOGUS_ARRAY)
  {
      ERR("invalid format type %x\n", pFormat[0]);
      RpcRaiseException(RPC_S_INTERNAL_ERROR);
      return 0;
  }

  array_read_conformance(FC_BOGUS_ARRAY, pStubMsg, pFormat);
  array_memory_size(FC_BOGUS_ARRAY, pStubMsg, pFormat, TRUE /* fHasPointers */);
  return pStubMsg->MemorySize;
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

  if (pFormat[0] != FC_BOGUS_ARRAY)
  {
      ERR("invalid format type %x\n", pFormat[0]);
      RpcRaiseException(RPC_S_INTERNAL_ERROR);
      return;
  }

  def = *(const WORD*)&pFormat[2];
  pFormat += 4;

  pFormat = ComputeConformance(pStubMsg, pMemory, pFormat, def);
  TRACE("conformance = %Id\n", pStubMsg->MaxCount);

  pFormat = ComputeVariance(pStubMsg, pMemory, pFormat, pStubMsg->MaxCount);
  TRACE("variance = %ld\n", pStubMsg->ActualCount);

  count = pStubMsg->ActualCount;
  for (i = 0; i < count; i++)
    pMemory = ComplexFree(pStubMsg, pMemory, pFormat, NULL);
}

static void UserMarshalCB(PMIDL_STUB_MESSAGE pStubMsg,
                          USER_MARSHAL_CB_TYPE cbtype, PFORMAT_STRING pFormat,
                          USER_MARSHAL_CB *umcb)
{
  umcb->Flags = MAKELONG(pStubMsg->dwDestContext,
                         pStubMsg->RpcMsg->DataRepresentation);
  umcb->pStubMsg = pStubMsg;
  umcb->pReserve = NULL;
  umcb->Signature = USER_MARSHAL_CB_SIGNATURE;
  umcb->CBType = cbtype;
  umcb->pFormat = pFormat;
  umcb->pTypeFormat = NULL /* FIXME */;
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
  USER_MARSHAL_CB umcb;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  TRACE("index=%d\n", index);

  UserMarshalCB(pStubMsg, USER_MARSHAL_CB_MARSHALL, pFormat, &umcb);

  if (flags & USER_MARSHAL_POINTER)
  {
    align_pointer_clear(&pStubMsg->Buffer, 4);
    NDR_LOCAL_UINT32_WRITE(pStubMsg->Buffer, USER_MARSHAL_PTR_PREFIX);
    pStubMsg->Buffer += 4;
    if (pStubMsg->PointerBufferMark)
    {
      saved_buffer = pStubMsg->Buffer;
      pStubMsg->Buffer = pStubMsg->PointerBufferMark;
      pStubMsg->PointerBufferMark = NULL;
    }
    align_pointer_clear(&pStubMsg->Buffer, 8);
  }
  else
    align_pointer_clear(&pStubMsg->Buffer, (flags & 0xf) + 1);

  pStubMsg->Buffer =
    pStubMsg->StubDesc->aUserMarshalQuadruple[index].pfnMarshall(
      &umcb.Flags, pStubMsg->Buffer, pMemory);

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
  USER_MARSHAL_CB umcb;

  TRACE("(%p,%p,%p,%d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);
  TRACE("index=%d\n", index);

  UserMarshalCB(pStubMsg, USER_MARSHAL_CB_UNMARSHALL, pFormat, &umcb);

  if (flags & USER_MARSHAL_POINTER)
  {
    align_pointer(&pStubMsg->Buffer, 4);
    /* skip pointer prefix */
    pStubMsg->Buffer += 4;
    if (pStubMsg->PointerBufferMark)
    {
      saved_buffer = pStubMsg->Buffer;
      pStubMsg->Buffer = pStubMsg->PointerBufferMark;
      pStubMsg->PointerBufferMark = NULL;
    }
    align_pointer(&pStubMsg->Buffer, 8);
  }
  else
    align_pointer(&pStubMsg->Buffer, (flags & 0xf) + 1);

  if (!fMustAlloc && !*ppMemory)
    fMustAlloc = TRUE;
  if (fMustAlloc)
  {
    *ppMemory = NdrAllocate(pStubMsg, memsize);
    memset(*ppMemory, 0, memsize);
  }

  pStubMsg->Buffer =
    pStubMsg->StubDesc->aUserMarshalQuadruple[index].pfnUnmarshall(
      &umcb.Flags, pStubMsg->Buffer, *ppMemory);

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
  USER_MARSHAL_CB umcb;
  ULONG saved_buffer_length = 0;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  TRACE("index=%d\n", index);

  UserMarshalCB(pStubMsg, USER_MARSHAL_CB_BUFFER_SIZE, pFormat, &umcb);

  if (flags & USER_MARSHAL_POINTER)
  {
    align_length(&pStubMsg->BufferLength, 4);
    /* skip pointer prefix */
    safe_buffer_length_increment(pStubMsg, 4);
    if (pStubMsg->IgnoreEmbeddedPointers)
      return;
    if (pStubMsg->PointerLength)
    {
      saved_buffer_length = pStubMsg->BufferLength;
      pStubMsg->BufferLength = pStubMsg->PointerLength;
      pStubMsg->PointerLength = 0;
    }
    align_length(&pStubMsg->BufferLength, 8);
  }
  else
    align_length(&pStubMsg->BufferLength, (flags & 0xf) + 1);

  if (bufsize) {
    TRACE("size=%ld\n", bufsize);
    safe_buffer_length_increment(pStubMsg, bufsize);
  }
  else
    pStubMsg->BufferLength =
        pStubMsg->StubDesc->aUserMarshalQuadruple[index].pfnBufferSize(
                             &umcb.Flags, pStubMsg->BufferLength, pMemory);

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
    align_pointer(&pStubMsg->Buffer, 4);
    /* skip pointer prefix */
    pStubMsg->Buffer += 4;
    if (pStubMsg->IgnoreEmbeddedPointers)
      return pStubMsg->MemorySize;
    align_pointer(&pStubMsg->Buffer, 8);
  }
  else
    align_pointer(&pStubMsg->Buffer, (flags & 0xf) + 1);

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
  USER_MARSHAL_CB umcb;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  TRACE("index=%d\n", index);

  UserMarshalCB(pStubMsg, USER_MARSHAL_CB_FREE, pFormat, &umcb);

  pStubMsg->StubDesc->aUserMarshalQuadruple[index].pfnFree(
    &umcb.Flags, pMemory);
}

/***********************************************************************
 *           NdrGetUserMarshalInfo [RPCRT4.@]
 */
RPC_STATUS RPC_ENTRY NdrGetUserMarshalInfo(ULONG *flags, ULONG level, NDR_USER_MARSHAL_INFO *umi)
{
    USER_MARSHAL_CB *umcb = CONTAINING_RECORD(flags, USER_MARSHAL_CB, Flags);

    TRACE("(%p,%lu,%p)\n", flags, level, umi);

    if (level != 1)
        return RPC_S_INVALID_ARG;

    memset(&umi->Level1, 0, sizeof(umi->Level1));
    umi->InformationLevel = level;

    if (umcb->Signature != USER_MARSHAL_CB_SIGNATURE)
        return RPC_S_INVALID_ARG;

    umi->Level1.pfnAllocate = umcb->pStubMsg->pfnAllocate;
    umi->Level1.pfnFree = umcb->pStubMsg->pfnFree;
    umi->Level1.pRpcChannelBuffer = umcb->pStubMsg->pRpcChannelBuffer;

    switch (umcb->CBType)
    {
    case USER_MARSHAL_CB_MARSHALL:
    case USER_MARSHAL_CB_UNMARSHALL:
    {
        RPC_MESSAGE *msg = umcb->pStubMsg->RpcMsg;
        unsigned char *buffer_start = msg->Buffer;
        unsigned char *buffer_end =
            (unsigned char *)msg->Buffer + msg->BufferLength;

        if (umcb->pStubMsg->Buffer < buffer_start ||
            umcb->pStubMsg->Buffer > buffer_end)
            return RPC_X_INVALID_BUFFER;

        umi->Level1.Buffer = umcb->pStubMsg->Buffer;
        umi->Level1.BufferSize = buffer_end - umcb->pStubMsg->Buffer;
        break;
    }
    case USER_MARSHAL_CB_BUFFER_SIZE:
    case USER_MARSHAL_CB_FREE:
        break;
    default:
        WARN("unrecognised CBType %d\n", umcb->CBType);
    }

    return RPC_S_OK;
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
  FIXME("(pStubMsg == ^%p, pFormat == ^%p, NumberParams == %ld): stub.\n",
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
    if ((pCStructFormat->type != FC_CPSTRUCT) && (pCStructFormat->type != FC_CSTRUCT))
    {
        ERR("invalid format type %x\n", pCStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }

    pCArrayFormat = (const unsigned char *)&pCStructFormat->offset_to_array_description +
        pCStructFormat->offset_to_array_description;
    if (*pCArrayFormat != FC_CARRAY)
    {
        ERR("invalid array format type %x\n", pCStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }
    esize = *(const WORD*)(pCArrayFormat+2);

    ComputeConformance(pStubMsg, pMemory + pCStructFormat->memory_size,
                       pCArrayFormat + 4, 0);

    WriteConformance(pStubMsg);

    align_pointer_clear(&pStubMsg->Buffer, pCStructFormat->alignment + 1);

    TRACE("memory_size = %d\n", pCStructFormat->memory_size);

    bufsize = safe_multiply(esize, pStubMsg->MaxCount);
    if (pCStructFormat->memory_size + bufsize < pCStructFormat->memory_size) /* integer overflow */
    {
        ERR("integer overflow of memory_size %u with bufsize %lu\n",
            pCStructFormat->memory_size, bufsize);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    }
    /* copy constant sized part of struct */
    pStubMsg->BufferMark = pStubMsg->Buffer;
    safe_copy_to_buffer(pStubMsg, pMemory, pCStructFormat->memory_size + bufsize);

    if (pCStructFormat->type == FC_CPSTRUCT)
        EmbeddedPointerMarshall(pStubMsg, pMemory, pFormat);

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
    unsigned char *saved_buffer;

    TRACE("(%p, %p, %p, %d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);

    pFormat += sizeof(NDR_CSTRUCT_FORMAT);
    if ((pCStructFormat->type != FC_CPSTRUCT) && (pCStructFormat->type != FC_CSTRUCT))
    {
        ERR("invalid format type %x\n", pCStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }
    pCArrayFormat = (const unsigned char *)&pCStructFormat->offset_to_array_description +
        pCStructFormat->offset_to_array_description;
    if (*pCArrayFormat != FC_CARRAY)
    {
        ERR("invalid array format type %x\n", pCStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }
    esize = *(const WORD*)(pCArrayFormat+2);

    pCArrayFormat = ReadConformance(pStubMsg, pCArrayFormat + 4);

    align_pointer(&pStubMsg->Buffer, pCStructFormat->alignment + 1);

    TRACE("memory_size = %d\n", pCStructFormat->memory_size);

    bufsize = safe_multiply(esize, pStubMsg->MaxCount);
    if (pCStructFormat->memory_size + bufsize < pCStructFormat->memory_size) /* integer overflow */
    {
        ERR("integer overflow of memory_size %u with bufsize %lu\n",
            pCStructFormat->memory_size, bufsize);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    }

    if (fMustAlloc)
    {
        SIZE_T size = pCStructFormat->memory_size + bufsize;
        *ppMemory = NdrAllocateZero(pStubMsg, size);
    }
    else
    {
        if (!pStubMsg->IsClient && !*ppMemory)
            /* for servers, we just point straight into the RPC buffer */
            *ppMemory = pStubMsg->Buffer;
    }

    saved_buffer = pStubMsg->BufferMark = pStubMsg->Buffer;
    safe_buffer_increment(pStubMsg, pCStructFormat->memory_size + bufsize);
    if (pCStructFormat->type == FC_CPSTRUCT)
        EmbeddedPointerUnmarshall(pStubMsg, saved_buffer, *ppMemory, pFormat, fMustAlloc);

    TRACE("copying %p to %p\n", saved_buffer, *ppMemory);
    if (*ppMemory != saved_buffer)
        memcpy(*ppMemory, saved_buffer, pCStructFormat->memory_size + bufsize);

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
    if ((pCStructFormat->type != FC_CPSTRUCT) && (pCStructFormat->type != FC_CSTRUCT))
    {
        ERR("invalid format type %x\n", pCStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }
    pCArrayFormat = (const unsigned char *)&pCStructFormat->offset_to_array_description +
        pCStructFormat->offset_to_array_description;
    if (*pCArrayFormat != FC_CARRAY)
    {
        ERR("invalid array format type %x\n", pCStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }
    esize = *(const WORD*)(pCArrayFormat+2);

    pCArrayFormat = ComputeConformance(pStubMsg, pMemory + pCStructFormat->memory_size, pCArrayFormat+4, 0);
    SizeConformance(pStubMsg);

    align_length(&pStubMsg->BufferLength, pCStructFormat->alignment + 1);

    TRACE("memory_size = %d\n", pCStructFormat->memory_size);

    safe_buffer_length_increment(pStubMsg, pCStructFormat->memory_size);
    safe_buffer_length_increment(pStubMsg, safe_multiply(pStubMsg->MaxCount, esize));

    if (pCStructFormat->type == FC_CPSTRUCT)
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
    const NDR_CSTRUCT_FORMAT *pCStructFormat = (const NDR_CSTRUCT_FORMAT *)pFormat;
    PFORMAT_STRING pCArrayFormat;

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);

    pFormat += sizeof(NDR_CSTRUCT_FORMAT);
    if ((pCStructFormat->type != FC_CPSTRUCT) && (pCStructFormat->type != FC_CSTRUCT))
    {
        ERR("invalid format type %x\n", pCStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }

    pCArrayFormat = (const unsigned char *)&pCStructFormat->offset_to_array_description +
        pCStructFormat->offset_to_array_description;
    if (*pCArrayFormat != FC_CARRAY)
    {
        ERR("invalid array format type %x\n", pCStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }

    ComputeConformance(pStubMsg, pMemory + pCStructFormat->memory_size,
                       pCArrayFormat + 4, 0);

    TRACE("memory_size = %d\n", pCStructFormat->memory_size);

    /* copy constant sized part of struct */
    pStubMsg->BufferMark = pStubMsg->Buffer;

    if (pCStructFormat->type == FC_CPSTRUCT)
        EmbeddedPointerFree(pStubMsg, pMemory, pFormat);
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

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);

    pFormat += sizeof(NDR_CVSTRUCT_FORMAT);
    if (pCVStructFormat->type != FC_CVSTRUCT)
    {
        ERR("invalid format type %x\n", pCVStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }

    pCVArrayFormat = (const unsigned char *)&pCVStructFormat->offset_to_array_description +
        pCVStructFormat->offset_to_array_description;

    array_compute_and_write_conformance(*pCVArrayFormat, pStubMsg,
                                        pMemory + pCVStructFormat->memory_size,
                                        pCVArrayFormat);

    align_pointer_clear(&pStubMsg->Buffer, pCVStructFormat->alignment + 1);

    TRACE("memory_size = %d\n", pCVStructFormat->memory_size);

    /* write constant sized part */
    pStubMsg->BufferMark = pStubMsg->Buffer;
    safe_copy_to_buffer(pStubMsg, pMemory, pCVStructFormat->memory_size);

    array_write_variance_and_marshall(*pCVArrayFormat, pStubMsg,
                                      pMemory + pCVStructFormat->memory_size,
                                      pCVArrayFormat, FALSE /* fHasPointers */);

    EmbeddedPointerMarshall(pStubMsg, pMemory, pFormat);

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
    ULONG memsize, bufsize;
    unsigned char *saved_buffer, *saved_array_buffer;
    ULONG offset;
    unsigned char *array_memory;

    TRACE("(%p, %p, %p, %d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);

    pFormat += sizeof(NDR_CVSTRUCT_FORMAT);
    if (pCVStructFormat->type != FC_CVSTRUCT)
    {
        ERR("invalid format type %x\n", pCVStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }

    pCVArrayFormat = (const unsigned char *)&pCVStructFormat->offset_to_array_description +
        pCVStructFormat->offset_to_array_description;

    memsize = array_read_conformance(*pCVArrayFormat, pStubMsg,
                                     pCVArrayFormat);

    align_pointer(&pStubMsg->Buffer, pCVStructFormat->alignment + 1);

    TRACE("memory_size = %d\n", pCVStructFormat->memory_size);

    /* work out how much memory to allocate if we need to do so */
    if (!fMustAlloc && !*ppMemory)
        fMustAlloc = TRUE;
    if (fMustAlloc)
    {
        SIZE_T size = pCVStructFormat->memory_size + memsize;
        *ppMemory = NdrAllocateZero(pStubMsg, size);
    }

    /* mark the start of the constant data */
    saved_buffer = pStubMsg->BufferMark = pStubMsg->Buffer;
    safe_buffer_increment(pStubMsg, pCVStructFormat->memory_size);

    array_memory = *ppMemory + pCVStructFormat->memory_size;
    bufsize = array_read_variance_and_unmarshall(*pCVArrayFormat, pStubMsg,
                                                 &array_memory, pCVArrayFormat,
                                                 FALSE /* fMustAlloc */,
                                                 FALSE /* fUseServerBufferMemory */,
                                                 FALSE /* fUnmarshall */);

    /* save offset in case unmarshalling pointers changes it */
    offset = pStubMsg->Offset;

    /* mark the start of the array data */
    saved_array_buffer = pStubMsg->Buffer;
    safe_buffer_increment(pStubMsg, bufsize);

    EmbeddedPointerUnmarshall(pStubMsg, saved_buffer, *ppMemory, pFormat, fMustAlloc);

    /* copy the constant data */
    memcpy(*ppMemory, saved_buffer, pCVStructFormat->memory_size);
    /* copy the array data */
    TRACE("copying %p to %p\n", saved_array_buffer, *ppMemory + pCVStructFormat->memory_size);
    memcpy(*ppMemory + pCVStructFormat->memory_size + offset,
           saved_array_buffer, bufsize);

    if (*pCVArrayFormat == FC_C_CSTRING)
        TRACE("string=%s\n", debugstr_a((char *)(*ppMemory + pCVStructFormat->memory_size)));
    else if (*pCVArrayFormat == FC_C_WSTRING)
        TRACE("string=%s\n", debugstr_w((WCHAR *)(*ppMemory + pCVStructFormat->memory_size)));

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

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);

    pFormat += sizeof(NDR_CVSTRUCT_FORMAT);
    if (pCVStructFormat->type != FC_CVSTRUCT)
    {
        ERR("invalid format type %x\n", pCVStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }

    pCVArrayFormat = (const unsigned char *)&pCVStructFormat->offset_to_array_description +
        pCVStructFormat->offset_to_array_description;
    array_compute_and_size_conformance(*pCVArrayFormat, pStubMsg,
                                       pMemory + pCVStructFormat->memory_size,
                                       pCVArrayFormat);

    align_length(&pStubMsg->BufferLength, pCVStructFormat->alignment + 1);

    TRACE("memory_size = %d\n", pCVStructFormat->memory_size);

    safe_buffer_length_increment(pStubMsg, pCVStructFormat->memory_size);

    array_buffer_size(*pCVArrayFormat, pStubMsg,
                      pMemory + pCVStructFormat->memory_size, pCVArrayFormat,
                      FALSE /* fHasPointers */);

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

    TRACE("(%p, %p)\n", pStubMsg, pFormat);

    pFormat += sizeof(NDR_CVSTRUCT_FORMAT);
    if (pCVStructFormat->type != FC_CVSTRUCT)
    {
        ERR("invalid format type %x\n", pCVStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return 0;
    }

    pCVArrayFormat = (const unsigned char *)&pCVStructFormat->offset_to_array_description +
        pCVStructFormat->offset_to_array_description;
    array_read_conformance(*pCVArrayFormat, pStubMsg, pCVArrayFormat);

    align_pointer(&pStubMsg->Buffer, pCVStructFormat->alignment + 1);

    TRACE("memory_size = %d\n", pCVStructFormat->memory_size);

    safe_buffer_increment(pStubMsg, pCVStructFormat->memory_size);
    array_memory_size(*pCVArrayFormat, pStubMsg, pCVArrayFormat,
                      FALSE /* fHasPointers */);

    pStubMsg->MemorySize += pCVStructFormat->memory_size;

    EmbeddedPointerMemorySize(pStubMsg, pFormat);

    return pStubMsg->MemorySize;
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

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);

    pFormat += sizeof(NDR_CVSTRUCT_FORMAT);
    if (pCVStructFormat->type != FC_CVSTRUCT)
    {
        ERR("invalid format type %x\n", pCVStructFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }

    pCVArrayFormat = (const unsigned char *)&pCVStructFormat->offset_to_array_description +
        pCVStructFormat->offset_to_array_description;
    array_free(*pCVArrayFormat, pStubMsg,
               pMemory + pCVStructFormat->memory_size, pCVArrayFormat,
               FALSE /* fHasPointers */);

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
    ULONG total_size;
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
    ULONG total_size;

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);

    if ((pSmFArrayFormat->type != FC_SMFARRAY) &&
        (pSmFArrayFormat->type != FC_LGFARRAY))
    {
        ERR("invalid format type %x\n", pSmFArrayFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }

    align_pointer_clear(&pStubMsg->Buffer, pSmFArrayFormat->alignment + 1);

    if (pSmFArrayFormat->type == FC_SMFARRAY)
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
    safe_copy_to_buffer(pStubMsg, pMemory, total_size);

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
    ULONG total_size;
    unsigned char *saved_buffer;

    TRACE("(%p, %p, %p, %d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);

    if ((pSmFArrayFormat->type != FC_SMFARRAY) &&
        (pSmFArrayFormat->type != FC_LGFARRAY))
    {
        ERR("invalid format type %x\n", pSmFArrayFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }

    align_pointer(&pStubMsg->Buffer, pSmFArrayFormat->alignment + 1);

    if (pSmFArrayFormat->type == FC_SMFARRAY)
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

    if (fMustAlloc)
        *ppMemory = NdrAllocateZero(pStubMsg, total_size);
    else
    {
        if (!pStubMsg->IsClient && !*ppMemory)
            /* for servers, we just point straight into the RPC buffer */
            *ppMemory = pStubMsg->Buffer;
    }

    saved_buffer = pStubMsg->BufferMark = pStubMsg->Buffer;
    safe_buffer_increment(pStubMsg, total_size);
    pFormat = EmbeddedPointerUnmarshall(pStubMsg, saved_buffer, *ppMemory, pFormat, fMustAlloc);

    TRACE("copying %p to %p\n", saved_buffer, *ppMemory);
    if (*ppMemory != saved_buffer)
        memcpy(*ppMemory, saved_buffer, total_size);

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
    ULONG total_size;

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);

    if ((pSmFArrayFormat->type != FC_SMFARRAY) &&
        (pSmFArrayFormat->type != FC_LGFARRAY))
    {
        ERR("invalid format type %x\n", pSmFArrayFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }

    align_length(&pStubMsg->BufferLength, pSmFArrayFormat->alignment + 1);

    if (pSmFArrayFormat->type == FC_SMFARRAY)
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
    safe_buffer_length_increment(pStubMsg, total_size);

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

    if ((pSmFArrayFormat->type != FC_SMFARRAY) &&
        (pSmFArrayFormat->type != FC_LGFARRAY))
    {
        ERR("invalid format type %x\n", pSmFArrayFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return 0;
    }

    align_pointer(&pStubMsg->Buffer, pSmFArrayFormat->alignment + 1);

    if (pSmFArrayFormat->type == FC_SMFARRAY)
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
    safe_buffer_increment(pStubMsg, total_size);
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

    if ((pSmFArrayFormat->type != FC_SMFARRAY) &&
        (pSmFArrayFormat->type != FC_LGFARRAY))
    {
        ERR("invalid format type %x\n", pSmFArrayFormat->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }

    if (pSmFArrayFormat->type == FC_SMFARRAY)
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

    if ((pFormat[0] != FC_SMVARRAY) &&
        (pFormat[0] != FC_LGVARRAY))
    {
        ERR("invalid format type %x\n", pFormat[0]);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }

    alignment = pFormat[1] + 1;

    if (pFormat[0] == FC_SMVARRAY)
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

    align_pointer_clear(&pStubMsg->Buffer, alignment);

    bufsize = safe_multiply(esize, pStubMsg->ActualCount);
    pStubMsg->BufferMark = pStubMsg->Buffer;
    safe_copy_to_buffer(pStubMsg, pMemory + pStubMsg->Offset, bufsize);

    EmbeddedPointerMarshall(pStubMsg, pMemory, pFormat);

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
    unsigned char *saved_buffer;
    ULONG offset;

    TRACE("(%p, %p, %p, %d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);

    if ((pFormat[0] != FC_SMVARRAY) &&
        (pFormat[0] != FC_LGVARRAY))
    {
        ERR("invalid format type %x\n", pFormat[0]);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }

    alignment = pFormat[1] + 1;

    if (pFormat[0] == FC_SMVARRAY)
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

    align_pointer(&pStubMsg->Buffer, alignment);

    bufsize = safe_multiply(esize, pStubMsg->ActualCount);
    offset = pStubMsg->Offset;

    if (!fMustAlloc && !*ppMemory)
        fMustAlloc = TRUE;
    if (fMustAlloc)
        *ppMemory = NdrAllocateZero(pStubMsg, size);
    saved_buffer = pStubMsg->BufferMark = pStubMsg->Buffer;
    safe_buffer_increment(pStubMsg, bufsize);

    EmbeddedPointerUnmarshall(pStubMsg, saved_buffer, *ppMemory, pFormat, fMustAlloc);

    memcpy(*ppMemory + offset, saved_buffer, bufsize);

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

    if ((pFormat[0] != FC_SMVARRAY) &&
        (pFormat[0] != FC_LGVARRAY))
    {
        ERR("invalid format type %x\n", pFormat[0]);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }

    alignment = pFormat[1] + 1;

    if (pFormat[0] == FC_SMVARRAY)
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

    align_length(&pStubMsg->BufferLength, alignment);

    safe_buffer_length_increment(pStubMsg, safe_multiply(esize, pStubMsg->ActualCount));

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

    if ((pFormat[0] != FC_SMVARRAY) &&
        (pFormat[0] != FC_LGVARRAY))
    {
        ERR("invalid format type %x\n", pFormat[0]);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return 0;
    }

    alignment = pFormat[1] + 1;

    if (pFormat[0] == FC_SMVARRAY)
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

    align_pointer(&pStubMsg->Buffer, alignment);

    safe_buffer_increment(pStubMsg, safe_multiply(esize, pStubMsg->ActualCount));
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
    DWORD elements;

    TRACE("(%p, %p, %p)\n", pStubMsg, pMemory, pFormat);

    if ((pFormat[0] != FC_SMVARRAY) &&
        (pFormat[0] != FC_LGVARRAY))
    {
        ERR("invalid format type %x\n", pFormat[0]);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return;
    }

    if (pFormat[0] == FC_SMVARRAY)
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

static ULONG get_discriminant(unsigned char fc, const unsigned char *pMemory)
{
    switch (fc)
    {
    case FC_BYTE:
    case FC_CHAR:
    case FC_SMALL:
    case FC_USMALL:
        return *pMemory;
    case FC_WCHAR:
    case FC_SHORT:
    case FC_USHORT:
    case FC_ENUM16:
        return *(const USHORT *)pMemory;
    case FC_LONG:
    case FC_ULONG:
    case FC_ENUM32:
        return *(const ULONG *)pMemory;
    default:
        FIXME("Unhandled base type: 0x%02x\n", fc);
        return 0;
    }
}

static PFORMAT_STRING get_arm_offset_from_union_arm_selector(PMIDL_STUB_MESSAGE pStubMsg,
                                                             ULONG discriminant,
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
            BOOL pointer_buffer_mark_set = FALSE;
            switch(*desc)
            {
            case FC_RP:
            case FC_UP:
            case FC_OP:
            case FC_FP:
                align_pointer_clear(&pStubMsg->Buffer, 4);
                saved_buffer = pStubMsg->Buffer;
                if (pStubMsg->PointerBufferMark)
                {
                  pStubMsg->Buffer = pStubMsg->PointerBufferMark;
                  pStubMsg->PointerBufferMark = NULL;
                  pointer_buffer_mark_set = TRUE;
                }
                else
                  safe_buffer_increment(pStubMsg, 4); /* for pointer ID */

                PointerMarshall(pStubMsg, saved_buffer, *(unsigned char **)pMemory, desc);
                if (pointer_buffer_mark_set)
                {
                  STD_OVERFLOW_CHECK(pStubMsg);
                  pStubMsg->PointerBufferMark = pStubMsg->Buffer;
                  if (saved_buffer + 4 > (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength)
                  {
                      ERR("buffer overflow - saved_buffer = %p, BufferEnd = %p\n",
                          saved_buffer, (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength);
                      RpcRaiseException(RPC_X_BAD_STUB_DATA);
                  }
                  pStubMsg->Buffer = saved_buffer + 4;
                }
                break;
            case FC_IP:
                /* must be dereferenced first */
                m(pStubMsg, *(unsigned char **)pMemory, desc);
                break;
            default:
                m(pStubMsg, pMemory, desc);
            }
        }
        else if (*desc)
            FIXME("no marshaller for embedded type %02x\n", *desc);
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
        return NdrBaseTypeUnmarshall(pStubMsg, ppMemory, &basetype, FALSE);
    }
    else
    {
        PFORMAT_STRING desc = pFormat + *(const SHORT*)pFormat;
        NDR_UNMARSHALL m = NdrUnmarshaller[*desc & NDR_TABLE_MASK];
        if (m)
        {
            unsigned char *saved_buffer = NULL;
            BOOL pointer_buffer_mark_set = FALSE;
            switch(*desc)
            {
            case FC_RP:
            case FC_UP:
            case FC_OP:
            case FC_FP:
                align_pointer(&pStubMsg->Buffer, 4);
                saved_buffer = pStubMsg->Buffer;
                if (pStubMsg->PointerBufferMark)
                {
                  pStubMsg->Buffer = pStubMsg->PointerBufferMark;
                  pStubMsg->PointerBufferMark = NULL;
                  pointer_buffer_mark_set = TRUE;
                }
                else
                  pStubMsg->Buffer += 4; /* for pointer ID */

                if (saved_buffer + 4 > pStubMsg->BufferEnd)
                {
                    ERR("buffer overflow - saved_buffer = %p, BufferEnd = %p\n",
                        saved_buffer, pStubMsg->BufferEnd);
                    RpcRaiseException(RPC_X_BAD_STUB_DATA);
                }

                PointerUnmarshall(pStubMsg, saved_buffer, *(unsigned char ***)ppMemory, **(unsigned char ***)ppMemory, desc, fMustAlloc);
                if (pointer_buffer_mark_set)
                {
                  STD_OVERFLOW_CHECK(pStubMsg);
                  pStubMsg->PointerBufferMark = pStubMsg->Buffer;
                  pStubMsg->Buffer = saved_buffer + 4;
                }
                break;
            case FC_IP:
                /* must be dereferenced first */
                m(pStubMsg, *(unsigned char ***)ppMemory, desc, fMustAlloc);
                break;
            default:
                m(pStubMsg, ppMemory, desc, fMustAlloc);
            }
        }
        else if (*desc)
            FIXME("no marshaller for embedded type %02x\n", *desc);
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
            case FC_RP:
            case FC_UP:
            case FC_OP:
            case FC_FP:
                align_length(&pStubMsg->BufferLength, 4);
                safe_buffer_length_increment(pStubMsg, 4); /* for pointer ID */
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
            case FC_IP:
                /* must be dereferenced first */
                m(pStubMsg, *(unsigned char **)pMemory, desc);
                break;
            default:
                m(pStubMsg, pMemory, desc);
            }
        }
        else if (*desc)
            FIXME("no buffersizer for embedded type %02x\n", *desc);
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
            case FC_RP:
            case FC_UP:
            case FC_OP:
            case FC_FP:
                align_pointer(&pStubMsg->Buffer, 4);
                saved_buffer = pStubMsg->Buffer;
                safe_buffer_increment(pStubMsg, 4);
                align_length(&pStubMsg->MemorySize, sizeof(void *));
                pStubMsg->MemorySize += sizeof(void *);
                if (!pStubMsg->IgnoreEmbeddedPointers)
                    PointerMemorySize(pStubMsg, saved_buffer, pFormat);
                break;
            default:
                return m(pStubMsg, desc);
            }
        }
        else if (*desc)
            FIXME("no marshaller for embedded type %02x\n", *desc);
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
            case FC_RP:
            case FC_UP:
            case FC_OP:
            case FC_FP:
                PointerFree(pStubMsg, *(unsigned char **)pMemory, desc);
                break;
            case FC_IP:
                /* must be dereferenced first */
                m(pStubMsg, *(unsigned char **)pMemory, desc);
                break;
            default:
                m(pStubMsg, pMemory, desc);
            }
        }
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

    align_pointer_clear(&pStubMsg->Buffer, increment);

    switch_value = get_discriminant(switch_type, pMemory);
    TRACE("got switch value 0x%lx\n", switch_value);

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

    align_pointer(&pStubMsg->Buffer, increment);
    switch_value = get_discriminant(switch_type, pStubMsg->Buffer);
    TRACE("got switch value 0x%lx\n", switch_value);

    size = *(const unsigned short*)pFormat + increment;
    if (!fMustAlloc && !*ppMemory)
        fMustAlloc = TRUE;
    if (fMustAlloc)
        *ppMemory = NdrAllocate(pStubMsg, size);

    /* we can't pass fMustAlloc=TRUE into the marshaller for the arm
     * since the arm is part of the memory block that is encompassed by
     * the whole union. Memory is forced to allocate when pointers
     * are set to NULL, so we emulate that part of fMustAlloc=TRUE by
     * clearing the memory we pass in to the unmarshaller */
    if (fMustAlloc)
        memset(*ppMemory, 0, size);

    NdrBaseTypeUnmarshall(pStubMsg, ppMemory, &switch_type, FALSE);
    pMemoryArm = *ppMemory + increment;

    return union_arm_unmarshall(pStubMsg, &pMemoryArm, switch_value, pFormat, FALSE);
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

    align_length(&pStubMsg->BufferLength, increment);
    switch_value = get_discriminant(switch_type, pMemory);
    TRACE("got switch value 0x%lx\n", switch_value);

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

    align_pointer(&pStubMsg->Buffer, increment);
    switch_value = get_discriminant(switch_type, pStubMsg->Buffer);
    TRACE("got switch value 0x%lx\n", switch_value);

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
    TRACE("got switch value 0x%lx\n", switch_value);

    pMemory += increment;

    union_arm_free(pStubMsg, pMemory, switch_value, pFormat);
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
    TRACE("got switch value 0x%Ix\n", pStubMsg->MaxCount);
    /* Marshall discriminant */
    NdrBaseTypeMarshall(pStubMsg, (unsigned char *)&pStubMsg->MaxCount, &switch_type);

    return union_arm_marshall(pStubMsg, pMemory, pStubMsg->MaxCount, pFormat + *(const SHORT*)pFormat);
}

static LONG unmarshall_discriminant(PMIDL_STUB_MESSAGE pStubMsg,
                                    PFORMAT_STRING *ppFormat)
{
    LONG discriminant = 0;

    switch(**ppFormat)
    {
    case FC_BYTE:
    case FC_CHAR:
    case FC_SMALL:
    case FC_USMALL:
    {
        UCHAR d;
        safe_copy_from_buffer(pStubMsg, &d, sizeof(d));
        discriminant = d;
        break;
    }
    case FC_WCHAR:
    case FC_SHORT:
    case FC_USHORT:
    case FC_ENUM16:
    {
        USHORT d;
        align_pointer(&pStubMsg->Buffer, sizeof(USHORT));
        safe_copy_from_buffer(pStubMsg, &d, sizeof(d));
        discriminant = d;
        break;
    }
    case FC_LONG:
    case FC_ULONG:
    {
        ULONG d;
        align_pointer(&pStubMsg->Buffer, sizeof(ULONG));
        safe_copy_from_buffer(pStubMsg, &d, sizeof(d));
        discriminant = d;
        break;
    }
    default:
        FIXME("Unhandled base type: 0x%02x\n", **ppFormat);
    }
    (*ppFormat)++;

    *ppFormat = SkipConformance(pStubMsg, *ppFormat);
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
    LONG discriminant;
    unsigned short size;

    TRACE("(%p, %p, %p, %d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);
    pFormat++;

    /* Unmarshall discriminant */
    discriminant = unmarshall_discriminant(pStubMsg, &pFormat);
    TRACE("unmarshalled discriminant %lx\n", discriminant);

    pFormat += *(const SHORT*)pFormat;

    size = *(const unsigned short*)pFormat;

    if (!fMustAlloc && !*ppMemory)
        fMustAlloc = TRUE;
    if (fMustAlloc)
        *ppMemory = NdrAllocate(pStubMsg, size);

    /* we can't pass fMustAlloc=TRUE into the marshaller for the arm
     * since the arm is part of the memory block that is encompassed by
     * the whole union. Memory is forced to allocate when pointers
     * are set to NULL, so we emulate that part of fMustAlloc=TRUE by
     * clearing the memory we pass in to the unmarshaller */
    if (fMustAlloc)
        memset(*ppMemory, 0, size);

    return union_arm_unmarshall(pStubMsg, ppMemory, discriminant, pFormat, FALSE);
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
    TRACE("got switch value 0x%Ix\n", pStubMsg->MaxCount);
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
    TRACE("unmarshalled discriminant 0x%lx\n", discriminant);

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
    TRACE("got switch value 0x%Ix\n", pStubMsg->MaxCount);

    union_arm_free(pStubMsg, pMemory, pStubMsg->MaxCount, pFormat + *(const SHORT*)pFormat);
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
 *           NdrByteCountPointerMemorySize [internal]
 */
static ULONG WINAPI NdrByteCountPointerMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
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

/***********************************************************************
 *           NdrRangeMarshall [internal]
 */
static unsigned char *WINAPI NdrRangeMarshall(
    PMIDL_STUB_MESSAGE pStubMsg,
    unsigned char *pMemory,
    PFORMAT_STRING pFormat)
{
    const NDR_RANGE *pRange = (const NDR_RANGE *)pFormat;
    unsigned char base_type;

    TRACE("pStubMsg %p, pMemory %p, type 0x%02x\n", pStubMsg, pMemory, *pFormat);

    if (pRange->type != FC_RANGE)
    {
        ERR("invalid format type %x\n", pRange->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }

    base_type = pRange->flags_type & 0xf;

    return NdrBaseTypeMarshall(pStubMsg, pMemory, &base_type);
}

/***********************************************************************
 *           NdrRangeUnmarshall [RPCRT4.@]
 */
unsigned char *WINAPI NdrRangeUnmarshall(
    PMIDL_STUB_MESSAGE pStubMsg,
    unsigned char **ppMemory,
    PFORMAT_STRING pFormat,
    unsigned char fMustAlloc)
{
    const NDR_RANGE *pRange = (const NDR_RANGE *)pFormat;
    unsigned char base_type;

    TRACE("pStubMsg: %p, ppMemory: %p, type: 0x%02x, fMustAlloc: %s\n", pStubMsg, ppMemory, *pFormat, fMustAlloc ? "true" : "false");

    if (pRange->type != FC_RANGE)
    {
        ERR("invalid format type %x\n", pRange->type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
        return NULL;
    }
    base_type = pRange->flags_type & 0xf;

    TRACE("base_type = 0x%02x, low_value = %ld, high_value = %ld\n",
        base_type, pRange->low_value, pRange->high_value);

#define RANGE_UNMARSHALL(mem_type, wire_type, format_spec) \
    do \
    { \
        align_pointer(&pStubMsg->Buffer, sizeof(wire_type)); \
        if (!fMustAlloc && !*ppMemory) \
            fMustAlloc = TRUE; \
        if (fMustAlloc) \
            *ppMemory = NdrAllocate(pStubMsg, sizeof(mem_type)); \
        if (pStubMsg->Buffer + sizeof(wire_type) > pStubMsg->BufferEnd) \
        { \
            ERR("buffer overflow - Buffer = %p, BufferEnd = %p\n", \
                pStubMsg->Buffer, (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength); \
            RpcRaiseException(RPC_X_BAD_STUB_DATA); \
        } \
        if ((*(wire_type *)pStubMsg->Buffer < (mem_type)pRange->low_value) || \
            (*(wire_type *)pStubMsg->Buffer > (mem_type)pRange->high_value)) \
        { \
            ERR("value exceeded bounds: " format_spec ", low: " format_spec ", high: " format_spec "\n", \
                *(wire_type *)pStubMsg->Buffer, (mem_type)pRange->low_value, \
                (mem_type)pRange->high_value); \
            RpcRaiseException(RPC_S_INVALID_BOUND); \
            return NULL; \
        } \
        TRACE("*ppMemory: %p\n", *ppMemory); \
        **(mem_type **)ppMemory = *(wire_type *)pStubMsg->Buffer; \
        pStubMsg->Buffer += sizeof(wire_type); \
    } while (0)

    switch(base_type)
    {
    case FC_CHAR:
    case FC_SMALL:
        RANGE_UNMARSHALL(UCHAR, UCHAR, "%d");
        TRACE("value: 0x%02x\n", **ppMemory);
        break;
    case FC_BYTE:
    case FC_USMALL:
        RANGE_UNMARSHALL(CHAR, CHAR, "%u");
        TRACE("value: 0x%02x\n", **ppMemory);
        break;
    case FC_WCHAR: /* FIXME: valid? */
    case FC_USHORT:
        RANGE_UNMARSHALL(USHORT, USHORT, "%u");
        TRACE("value: 0x%04x\n", **(USHORT **)ppMemory);
        break;
    case FC_SHORT:
        RANGE_UNMARSHALL(SHORT, SHORT, "%d");
        TRACE("value: 0x%04x\n", **(USHORT **)ppMemory);
        break;
    case FC_LONG:
    case FC_ENUM32:
        RANGE_UNMARSHALL(LONG, LONG, "%ld");
        TRACE("value: 0x%08lx\n", **(ULONG **)ppMemory);
        break;
    case FC_ULONG:
        RANGE_UNMARSHALL(ULONG, ULONG, "%lu");
        TRACE("value: 0x%08lx\n", **(ULONG **)ppMemory);
        break;
    case FC_ENUM16:
        RANGE_UNMARSHALL(UINT, USHORT, "%u");
        TRACE("value: 0x%08x\n", **(UINT **)ppMemory);
        break;
    case FC_FLOAT:
    case FC_DOUBLE:
    case FC_HYPER:
    default:
        ERR("invalid range base type: 0x%02x\n", base_type);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
    }

    return NULL;
}

/***********************************************************************
 *           NdrRangeBufferSize [internal]
 */
static void WINAPI NdrRangeBufferSize(
    PMIDL_STUB_MESSAGE pStubMsg,
    unsigned char *pMemory,
    PFORMAT_STRING pFormat)
{
    const NDR_RANGE *pRange = (const NDR_RANGE *)pFormat;
    unsigned char base_type;

    TRACE("pStubMsg %p, pMemory %p, type 0x%02x\n", pStubMsg, pMemory, *pFormat);

    if (pRange->type != FC_RANGE)
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
static ULONG WINAPI NdrRangeMemorySize(
    PMIDL_STUB_MESSAGE pStubMsg,
    PFORMAT_STRING pFormat)
{
    const NDR_RANGE *pRange = (const NDR_RANGE *)pFormat;
    unsigned char base_type;

    if (pRange->type != FC_RANGE)
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
static void WINAPI NdrRangeFree(PMIDL_STUB_MESSAGE pStubMsg,
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
    case FC_BYTE:
    case FC_CHAR:
    case FC_SMALL:
    case FC_USMALL:
        safe_copy_to_buffer(pStubMsg, pMemory, sizeof(UCHAR));
        TRACE("value: 0x%02x\n", *pMemory);
        break;
    case FC_WCHAR:
    case FC_SHORT:
    case FC_USHORT:
        align_pointer_clear(&pStubMsg->Buffer, sizeof(USHORT));
        safe_copy_to_buffer(pStubMsg, pMemory, sizeof(USHORT));
        TRACE("value: 0x%04x\n", *(USHORT *)pMemory);
        break;
    case FC_LONG:
    case FC_ULONG:
    case FC_ERROR_STATUS_T:
    case FC_ENUM32:
        align_pointer_clear(&pStubMsg->Buffer, sizeof(ULONG));
        safe_copy_to_buffer(pStubMsg, pMemory, sizeof(ULONG));
        TRACE("value: 0x%08lx\n", *(ULONG *)pMemory);
        break;
    case FC_FLOAT:
        align_pointer_clear(&pStubMsg->Buffer, sizeof(float));
        safe_copy_to_buffer(pStubMsg, pMemory, sizeof(float));
        break;
    case FC_DOUBLE:
        align_pointer_clear(&pStubMsg->Buffer, sizeof(double));
        safe_copy_to_buffer(pStubMsg, pMemory, sizeof(double));
        break;
    case FC_HYPER:
        align_pointer_clear(&pStubMsg->Buffer, sizeof(ULONGLONG));
        safe_copy_to_buffer(pStubMsg, pMemory, sizeof(ULONGLONG));
        TRACE("value: %s\n", wine_dbgstr_longlong(*(ULONGLONG*)pMemory));
        break;
    case FC_ENUM16:
    {
        USHORT val = *(UINT *)pMemory;
        /* only 16-bits on the wire, so do a sanity check */
        if (*(UINT *)pMemory > SHRT_MAX)
            RpcRaiseException(RPC_X_ENUM_VALUE_OUT_OF_RANGE);
        align_pointer_clear(&pStubMsg->Buffer, sizeof(USHORT));
        safe_copy_to_buffer(pStubMsg, &val, sizeof(val));
        TRACE("value: 0x%04x\n", *(UINT *)pMemory);
        break;
    }
    case FC_INT3264:
    case FC_UINT3264:
    {
        UINT val = *(UINT_PTR *)pMemory;
        align_pointer_clear(&pStubMsg->Buffer, sizeof(UINT));
        safe_copy_to_buffer(pStubMsg, &val, sizeof(val));
        break;
    }
    case FC_IGNORE:
        break;
    default:
        FIXME("Unhandled base type: 0x%02x\n", *pFormat);
    }

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

#define BASE_TYPE_UNMARSHALL(type) do { \
        align_pointer(&pStubMsg->Buffer, sizeof(type)); \
        if (!fMustAlloc && !pStubMsg->IsClient && !*ppMemory) \
        { \
            *ppMemory = pStubMsg->Buffer; \
            TRACE("*ppMemory: %p\n", *ppMemory); \
            safe_buffer_increment(pStubMsg, sizeof(type)); \
        } \
        else \
        {  \
            if (fMustAlloc) \
                *ppMemory = NdrAllocate(pStubMsg, sizeof(type)); \
            TRACE("*ppMemory: %p\n", *ppMemory); \
            safe_copy_from_buffer(pStubMsg, *ppMemory, sizeof(type)); \
        } \
    } while (0)

    switch(*pFormat)
    {
    case FC_BYTE:
    case FC_CHAR:
    case FC_SMALL:
    case FC_USMALL:
        BASE_TYPE_UNMARSHALL(UCHAR);
        TRACE("value: 0x%02x\n", **ppMemory);
        break;
    case FC_WCHAR:
    case FC_SHORT:
    case FC_USHORT:
        BASE_TYPE_UNMARSHALL(USHORT);
        TRACE("value: 0x%04x\n", **(USHORT **)ppMemory);
        break;
    case FC_LONG:
    case FC_ULONG:
    case FC_ERROR_STATUS_T:
    case FC_ENUM32:
        BASE_TYPE_UNMARSHALL(ULONG);
        TRACE("value: 0x%08lx\n", **(ULONG **)ppMemory);
        break;
   case FC_FLOAT:
        BASE_TYPE_UNMARSHALL(float);
        TRACE("value: %f\n", **(float **)ppMemory);
        break;
    case FC_DOUBLE:
        BASE_TYPE_UNMARSHALL(double);
        TRACE("value: %f\n", **(double **)ppMemory);
        break;
    case FC_HYPER:
        BASE_TYPE_UNMARSHALL(ULONGLONG);
        TRACE("value: %s\n", wine_dbgstr_longlong(**(ULONGLONG **)ppMemory));
        break;
    case FC_ENUM16:
    {
        USHORT val;
        align_pointer(&pStubMsg->Buffer, sizeof(USHORT));
        if (!fMustAlloc && !*ppMemory)
            fMustAlloc = TRUE;
        if (fMustAlloc)
            *ppMemory = NdrAllocate(pStubMsg, sizeof(UINT));
        safe_copy_from_buffer(pStubMsg, &val, sizeof(USHORT));
        /* 16-bits on the wire, but int in memory */
        **(UINT **)ppMemory = val;
        TRACE("value: 0x%08x\n", **(UINT **)ppMemory);
        break;
    }
    case FC_INT3264:
        if (sizeof(INT_PTR) == sizeof(INT)) BASE_TYPE_UNMARSHALL(INT);
        else
        {
            INT val;
            align_pointer(&pStubMsg->Buffer, sizeof(INT));
            if (!fMustAlloc && !*ppMemory)
                fMustAlloc = TRUE;
            if (fMustAlloc)
                *ppMemory = NdrAllocate(pStubMsg, sizeof(INT_PTR));
            safe_copy_from_buffer(pStubMsg, &val, sizeof(INT));
            **(INT_PTR **)ppMemory = val;
            TRACE("value: 0x%08Ix\n", **(INT_PTR **)ppMemory);
        }
        break;
    case FC_UINT3264:
        if (sizeof(UINT_PTR) == sizeof(UINT)) BASE_TYPE_UNMARSHALL(UINT);
        else
        {
            UINT val;
            align_pointer(&pStubMsg->Buffer, sizeof(UINT));
            if (!fMustAlloc && !*ppMemory)
                fMustAlloc = TRUE;
            if (fMustAlloc)
                *ppMemory = NdrAllocate(pStubMsg, sizeof(UINT_PTR));
            safe_copy_from_buffer(pStubMsg, &val, sizeof(UINT));
            **(UINT_PTR **)ppMemory = val;
            TRACE("value: 0x%08Ix\n", **(UINT_PTR **)ppMemory);
        }
        break;
    case FC_IGNORE:
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
    case FC_BYTE:
    case FC_CHAR:
    case FC_SMALL:
    case FC_USMALL:
        safe_buffer_length_increment(pStubMsg, sizeof(UCHAR));
        break;
    case FC_WCHAR:
    case FC_SHORT:
    case FC_USHORT:
    case FC_ENUM16:
        align_length(&pStubMsg->BufferLength, sizeof(USHORT));
        safe_buffer_length_increment(pStubMsg, sizeof(USHORT));
        break;
    case FC_LONG:
    case FC_ULONG:
    case FC_ENUM32:
    case FC_INT3264:
    case FC_UINT3264:
        align_length(&pStubMsg->BufferLength, sizeof(ULONG));
        safe_buffer_length_increment(pStubMsg, sizeof(ULONG));
        break;
    case FC_FLOAT:
        align_length(&pStubMsg->BufferLength, sizeof(float));
        safe_buffer_length_increment(pStubMsg, sizeof(float));
        break;
    case FC_DOUBLE:
        align_length(&pStubMsg->BufferLength, sizeof(double));
        safe_buffer_length_increment(pStubMsg, sizeof(double));
        break;
    case FC_HYPER:
        align_length(&pStubMsg->BufferLength, sizeof(ULONGLONG));
        safe_buffer_length_increment(pStubMsg, sizeof(ULONGLONG));
        break;
    case FC_ERROR_STATUS_T:
        align_length(&pStubMsg->BufferLength, sizeof(error_status_t));
        safe_buffer_length_increment(pStubMsg, sizeof(error_status_t));
        break;
    case FC_IGNORE:
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
    TRACE("pStubMsg %p, type 0x%02x\n", pStubMsg, *pFormat);

    switch(*pFormat)
    {
    case FC_BYTE:
    case FC_CHAR:
    case FC_SMALL:
    case FC_USMALL:
        safe_buffer_increment(pStubMsg, sizeof(UCHAR));
        pStubMsg->MemorySize += sizeof(UCHAR);
        return sizeof(UCHAR);
    case FC_WCHAR:
    case FC_SHORT:
    case FC_USHORT:
        align_pointer(&pStubMsg->Buffer, sizeof(USHORT));
        safe_buffer_increment(pStubMsg, sizeof(USHORT));
        align_length(&pStubMsg->MemorySize, sizeof(USHORT));
        pStubMsg->MemorySize += sizeof(USHORT);
        return sizeof(USHORT);
    case FC_LONG:
    case FC_ULONG:
    case FC_ENUM32:
        align_pointer(&pStubMsg->Buffer, sizeof(ULONG));
        safe_buffer_increment(pStubMsg, sizeof(ULONG));
        align_length(&pStubMsg->MemorySize, sizeof(ULONG));
        pStubMsg->MemorySize += sizeof(ULONG);
        return sizeof(ULONG);
    case FC_FLOAT:
        align_pointer(&pStubMsg->Buffer, sizeof(float));
        safe_buffer_increment(pStubMsg, sizeof(float));
        align_length(&pStubMsg->MemorySize, sizeof(float));
        pStubMsg->MemorySize += sizeof(float);
        return sizeof(float);
    case FC_DOUBLE:
        align_pointer(&pStubMsg->Buffer, sizeof(double));
        safe_buffer_increment(pStubMsg, sizeof(double));
        align_length(&pStubMsg->MemorySize, sizeof(double));
        pStubMsg->MemorySize += sizeof(double);
        return sizeof(double);
    case FC_HYPER:
        align_pointer(&pStubMsg->Buffer, sizeof(ULONGLONG));
        safe_buffer_increment(pStubMsg, sizeof(ULONGLONG));
        align_length(&pStubMsg->MemorySize, sizeof(ULONGLONG));
        pStubMsg->MemorySize += sizeof(ULONGLONG);
        return sizeof(ULONGLONG);
    case FC_ERROR_STATUS_T:
        align_pointer(&pStubMsg->Buffer, sizeof(error_status_t));
        safe_buffer_increment(pStubMsg, sizeof(error_status_t));
        align_length(&pStubMsg->MemorySize, sizeof(error_status_t));
        pStubMsg->MemorySize += sizeof(error_status_t);
        return sizeof(error_status_t);
    case FC_ENUM16:
        align_pointer(&pStubMsg->Buffer, sizeof(USHORT));
        safe_buffer_increment(pStubMsg, sizeof(USHORT));
        align_length(&pStubMsg->MemorySize, sizeof(UINT));
        pStubMsg->MemorySize += sizeof(UINT);
        return sizeof(UINT);
    case FC_INT3264:
    case FC_UINT3264:
        align_pointer(&pStubMsg->Buffer, sizeof(UINT));
        safe_buffer_increment(pStubMsg, sizeof(UINT));
        align_length(&pStubMsg->MemorySize, sizeof(UINT_PTR));
        pStubMsg->MemorySize += sizeof(UINT_PTR);
        return sizeof(UINT_PTR);
    case FC_IGNORE:
        align_length(&pStubMsg->MemorySize, sizeof(void *));
        pStubMsg->MemorySize += sizeof(void *);
        return sizeof(void *);
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

    if (*pFormat != FC_BIND_CONTEXT)
    {
        ERR("invalid format type %x\n", *pFormat);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
    }
    align_length(&pStubMsg->BufferLength, 4);
    safe_buffer_length_increment(pStubMsg, cbNDRContext);
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

    if (*pFormat != FC_BIND_CONTEXT)
    {
        ERR("invalid format type %x\n", *pFormat);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
    }
    TRACE("flags: 0x%02x\n", pFormat[1]);

    if (pStubMsg->IsClient)
    {
        if (pFormat[1] & HANDLE_PARAM_IS_VIA_PTR)
            NdrClientContextMarshall(pStubMsg, *(NDR_CCONTEXT **)pMemory, FALSE);
        else
            NdrClientContextMarshall(pStubMsg, pMemory, FALSE);
    }
    else
    {
        NDR_SCONTEXT ctxt = CONTAINING_RECORD(pMemory, struct _NDR_SCONTEXT, userContext);
        NDR_RUNDOWN rundown = pStubMsg->StubDesc->apfnNdrRundownRoutines[pFormat[2]];
        NdrServerContextNewMarshall(pStubMsg, ctxt, rundown, pFormat);
    }

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
    TRACE("pStubMsg %p, ppMemory %p, pFormat %p, fMustAlloc %s\n", pStubMsg,
        ppMemory, pFormat, fMustAlloc ? "TRUE": "FALSE");

    if (*pFormat != FC_BIND_CONTEXT)
    {
        ERR("invalid format type %x\n", *pFormat);
        RpcRaiseException(RPC_S_INTERNAL_ERROR);
    }
    TRACE("flags: 0x%02x\n", pFormat[1]);

    if (pStubMsg->IsClient)
    {
        NDR_CCONTEXT *ccontext;
        if (pFormat[1] & HANDLE_PARAM_IS_VIA_PTR)
            ccontext = *(NDR_CCONTEXT **)ppMemory;
        else
            ccontext = (NDR_CCONTEXT *)ppMemory;
        /* [out]-only or [ret] param */
        if ((pFormat[1] & (HANDLE_PARAM_IS_IN|HANDLE_PARAM_IS_OUT)) == HANDLE_PARAM_IS_OUT)
            *ccontext = NULL;
        NdrClientContextUnmarshall(pStubMsg, ccontext, pStubMsg->RpcMsg->Handle);
    }
    else
    {
        NDR_SCONTEXT ctxt;
        ctxt = NdrServerContextNewUnmarshall(pStubMsg, pFormat);
        if (pFormat[1] & HANDLE_PARAM_IS_VIA_PTR)
            *(void **)ppMemory = NDRSContextValue(ctxt);
        else
            *(void **)ppMemory = *NDRSContextValue(ctxt);
    }

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

    align_pointer_clear(&pStubMsg->Buffer, 4);

    if (pStubMsg->Buffer + cbNDRContext > (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength)
    {
        ERR("buffer overflow - Buffer = %p, BufferEnd = %p\n",
            pStubMsg->Buffer, (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    }

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

    align_pointer(&pStubMsg->Buffer, 4);

    if (pStubMsg->Buffer + cbNDRContext > pStubMsg->BufferEnd)
        RpcRaiseException(RPC_X_BAD_STUB_DATA);

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
    TRACE("(%p, %p, %p)\n", pStubMsg, ContextHandle, RundownRoutine);

    align_pointer(&pStubMsg->Buffer, 4);

    if (pStubMsg->Buffer + cbNDRContext > (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength)
    {
        ERR("buffer overflow - Buffer = %p, BufferEnd = %p\n",
            pStubMsg->Buffer, (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    }

    NDRSContextMarshall2(pStubMsg->RpcMsg->Handle, ContextHandle,
                         pStubMsg->Buffer, RundownRoutine, NULL,
                         RPC_CONTEXT_HANDLE_DEFAULT_FLAGS);
    pStubMsg->Buffer += cbNDRContext;
}

NDR_SCONTEXT WINAPI NdrServerContextUnmarshall(PMIDL_STUB_MESSAGE pStubMsg)
{
    NDR_SCONTEXT ContextHandle;

    TRACE("(%p)\n", pStubMsg);

    align_pointer(&pStubMsg->Buffer, 4);

    if (pStubMsg->Buffer + cbNDRContext > (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength)
    {
        ERR("buffer overflow - Buffer = %p, BufferEnd = %p\n",
            pStubMsg->Buffer, (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    }

    ContextHandle = NDRSContextUnmarshall2(pStubMsg->RpcMsg->Handle,
                                           pStubMsg->Buffer,
                                           pStubMsg->RpcMsg->DataRepresentation,
                                           NULL, RPC_CONTEXT_HANDLE_DEFAULT_FLAGS);
    pStubMsg->Buffer += cbNDRContext;

    return ContextHandle;
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
    RPC_SYNTAX_IDENTIFIER *if_id = NULL;
    ULONG flags = RPC_CONTEXT_HANDLE_DEFAULT_FLAGS;

    TRACE("(%p, %p)\n", pStubMsg, pFormat);

    if (pFormat[1] & NDR_CONTEXT_HANDLE_SERIALIZE)
        flags |= RPC_CONTEXT_HANDLE_SERIALIZE;
    if (pFormat[1] & NDR_CONTEXT_HANDLE_NOSERIALIZE)
        flags |= RPC_CONTEXT_HANDLE_DONT_SERIALIZE;
    if (pFormat[1] & NDR_STRICT_CONTEXT_HANDLE)
    {
        RPC_SERVER_INTERFACE *sif = pStubMsg->StubDesc->RpcInterfaceInformation;
        if_id = &sif->InterfaceId;
    }

    return NDRSContextUnmarshall2(pStubMsg->RpcMsg->Handle, NULL,
                                  pStubMsg->RpcMsg->DataRepresentation, if_id,
                                  flags);
}

void WINAPI NdrServerContextNewMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                        NDR_SCONTEXT ContextHandle,
                                        NDR_RUNDOWN RundownRoutine,
                                        PFORMAT_STRING pFormat)
{
    RPC_SYNTAX_IDENTIFIER *if_id = NULL;
    ULONG flags = RPC_CONTEXT_HANDLE_DEFAULT_FLAGS;

    TRACE("(%p, %p, %p, %p)\n", pStubMsg, ContextHandle, RundownRoutine, pFormat);

    align_pointer(&pStubMsg->Buffer, 4);

    if (pStubMsg->Buffer + cbNDRContext > (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength)
    {
        ERR("buffer overflow - Buffer = %p, BufferEnd = %p\n",
            pStubMsg->Buffer, (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    }

    if (pFormat[1] & NDR_CONTEXT_HANDLE_SERIALIZE)
        flags |= RPC_CONTEXT_HANDLE_SERIALIZE;
    if (pFormat[1] & NDR_CONTEXT_HANDLE_NOSERIALIZE)
        flags |= RPC_CONTEXT_HANDLE_DONT_SERIALIZE;
    if (pFormat[1] & NDR_STRICT_CONTEXT_HANDLE)
    {
        RPC_SERVER_INTERFACE *sif = pStubMsg->StubDesc->RpcInterfaceInformation;
        if_id = &sif->InterfaceId;
    }

    NDRSContextMarshall2(pStubMsg->RpcMsg->Handle, ContextHandle,
                          pStubMsg->Buffer, RundownRoutine, if_id, flags);
    pStubMsg->Buffer += cbNDRContext;
}

NDR_SCONTEXT WINAPI NdrServerContextNewUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                                  PFORMAT_STRING pFormat)
{
    NDR_SCONTEXT ContextHandle;
    RPC_SYNTAX_IDENTIFIER *if_id = NULL;
    ULONG flags = RPC_CONTEXT_HANDLE_DEFAULT_FLAGS;

    TRACE("(%p, %p)\n", pStubMsg, pFormat);

    align_pointer(&pStubMsg->Buffer, 4);

    if (pStubMsg->Buffer + cbNDRContext > (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength)
    {
        ERR("buffer overflow - Buffer = %p, BufferEnd = %p\n",
            pStubMsg->Buffer, (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength);
        RpcRaiseException(RPC_X_BAD_STUB_DATA);
    }

    if (pFormat[1] & NDR_CONTEXT_HANDLE_SERIALIZE)
        flags |= RPC_CONTEXT_HANDLE_SERIALIZE;
    if (pFormat[1] & NDR_CONTEXT_HANDLE_NOSERIALIZE)
        flags |= RPC_CONTEXT_HANDLE_DONT_SERIALIZE;
    if (pFormat[1] & NDR_STRICT_CONTEXT_HANDLE)
    {
        RPC_SERVER_INTERFACE *sif = pStubMsg->StubDesc->RpcInterfaceInformation;
        if_id = &sif->InterfaceId;
    }

    ContextHandle = NDRSContextUnmarshall2(pStubMsg->RpcMsg->Handle,
                                           pStubMsg->Buffer,
                                           pStubMsg->RpcMsg->DataRepresentation,
                                           if_id, flags);
    pStubMsg->Buffer += cbNDRContext;

    return ContextHandle;
}

/***********************************************************************
 *           NdrCorrelationInitialize [RPCRT4.@]
 *
 * Initializes correlation validity checking.
 *
 * PARAMS
 *  pStubMsg    [I] MIDL_STUB_MESSAGE used during unmarshalling.
 *  pMemory     [I] Pointer to memory to use as a cache.
 *  CacheSize   [I] Size of the memory pointed to by pMemory.
 *  Flags       [I] Reserved. Set to zero.
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI NdrCorrelationInitialize(PMIDL_STUB_MESSAGE pStubMsg, void *pMemory, ULONG CacheSize, ULONG Flags)
{
    TRACE("(%p, %p, %ld, 0x%lx)\n", pStubMsg, pMemory, CacheSize, Flags);

    if (pStubMsg->CorrDespIncrement == 0)
        pStubMsg->CorrDespIncrement = 2; /* size of the normal (non-range) /robust payload */

    pStubMsg->fHasNewCorrDesc = TRUE;
    pStubMsg->pCorrInfo = pMemory;
}

/***********************************************************************
 *           NdrCorrelationPass [RPCRT4.@]
 *
 * Performs correlation validity checking.
 *
 * PARAMS
 *  pStubMsg    [I] MIDL_STUB_MESSAGE used during unmarshalling.
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI NdrCorrelationPass(PMIDL_STUB_MESSAGE pStubMsg)
{
    FIXME("(%p): stub\n", pStubMsg);
}

/***********************************************************************
 *           NdrCorrelationFree [RPCRT4.@]
 *
 * Frees any resources used while unmarshalling parameters that need
 * correlation validity checking.
 *
 * PARAMS
 *  pStubMsg    [I] MIDL_STUB_MESSAGE used during unmarshalling.
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI NdrCorrelationFree(PMIDL_STUB_MESSAGE pStubMsg)
{
    /* FIXME: free memory  */
}
