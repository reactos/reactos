/*
 * NDR data marshalling
 *
 * Copyright 2002 Greg Turner
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
 *
 * TODO:
 *  - figure out whether we *really* got this right
 *  - check for errors and throw exceptions
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"

#include "ndr_misc.h"
#include "rpcndr.h"

#include "wine/unicode.h"
#include "wine/rpcfc.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define BUFFER_PARANOIA 20

#if defined(__i386__)
  #define LITTLE_ENDIAN_UINT32_WRITE(pchar, uint32) \
    (*((UINT32 *)(pchar)) = (uint32))

  #define LITTLE_ENDIAN_UINT32_READ(pchar) \
    (*((UINT32 *)(pchar)))
#else
  /* these would work for i386 too, but less efficient */
  #define LITTLE_ENDIAN_UINT32_WRITE(pchar, uint32) \
    (*(pchar)     = LOBYTE(LOWORD(uint32)), \
     *((pchar)+1) = HIBYTE(LOWORD(uint32)), \
     *((pchar)+2) = LOBYTE(HIWORD(uint32)), \
     *((pchar)+3) = HIBYTE(HIWORD(uint32)), \
     (uint32)) /* allow as r-value */

  #define LITTLE_ENDIAN_UINT32_READ(pchar) \
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
  #define NDR_LOCAL_UINT32_WRITE(pchar, uint32) \
    BIG_ENDIAN_UINT32_WRITE(pchar, uint32)
  #define NDR_LOCAL_UINT32_READ(pchar) \
    BIG_ENDIAN_UINT32_READ(pchar)
#else
  #define NDR_LOCAL_UINT32_WRITE(pchar, uint32) \
    LITTLE_ENDIAN_UINT32_WRITE(pchar, uint32)
  #define NDR_LOCAL_UINT32_READ(pchar) \
    LITTLE_ENDIAN_UINT32_READ(pchar)
#endif

/* _Align must be the desired alignment minus 1,
 * e.g. ALIGN_LENGTH(len, 3) to align on a dword boundary. */
#define ALIGNED_LENGTH(_Len, _Align) (((_Len)+(_Align))&~(_Align))
#define ALIGNED_POINTER(_Ptr, _Align) ((LPVOID)ALIGNED_LENGTH((ULONG_PTR)(_Ptr), _Align))
#define ALIGN_LENGTH(_Len, _Align) _Len = ALIGNED_LENGTH(_Len, _Align)
#define ALIGN_POINTER(_Ptr, _Align) _Ptr = ALIGNED_POINTER(_Ptr, _Align)

#define STD_OVERFLOW_CHECK(_Msg) do { \
    TRACE("buffer=%d/%ld\n", _Msg->Buffer - _Msg->BufferStart, _Msg->BufferLength); \
    if (_Msg->Buffer > _Msg->BufferEnd) ERR("buffer overflow %d bytes\n", _Msg->Buffer - _Msg->BufferEnd); \
  } while (0)

#define NDR_TABLE_SIZE 128
#define NDR_TABLE_MASK 127

NDR_MARSHALL NdrMarshaller[NDR_TABLE_SIZE] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  /* 0x10 */
  0,
  /* 0x11 */
  NdrPointerMarshall, NdrPointerMarshall,
  NdrPointerMarshall, NdrPointerMarshall,
  /* 0x15 */
  NdrSimpleStructMarshall, NdrSimpleStructMarshall,
  0, 0, 0,
  NdrComplexStructMarshall,
  /* 0x1b */
  NdrConformantArrayMarshall, 0, 0, 0, 0, 0,
  NdrComplexArrayMarshall,
  /* 0x22 */
  NdrConformantStringMarshall, 0, 0,
  NdrConformantStringMarshall, 0, 0, 0, 0,
  /* 0x2a */
  0, 0, 0, 0, 0,
  /* 0x2f */
  NdrInterfacePointerMarshall,
  /* 0xb0 */
  0, 0, 0, 0,
  NdrUserMarshalMarshall
};
NDR_UNMARSHALL NdrUnmarshaller[NDR_TABLE_SIZE] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  /* 0x10 */
  0,
  /* 0x11 */
  NdrPointerUnmarshall, NdrPointerUnmarshall,
  NdrPointerUnmarshall, NdrPointerUnmarshall,
  /* 0x15 */
  NdrSimpleStructUnmarshall, NdrSimpleStructUnmarshall,
  0, 0, 0,
  NdrComplexStructUnmarshall,
  /* 0x1b */
  NdrConformantArrayUnmarshall, 0, 0, 0, 0, 0,
  NdrComplexArrayUnmarshall,
  /* 0x22 */
  NdrConformantStringUnmarshall, 0, 0,
  NdrConformantStringUnmarshall, 0, 0, 0, 0,
  /* 0x2a */
  0, 0, 0, 0, 0,
  /* 0x2f */
  NdrInterfacePointerUnmarshall,
  /* 0xb0 */
  0, 0, 0, 0,
  NdrUserMarshalUnmarshall
};
NDR_BUFFERSIZE NdrBufferSizer[NDR_TABLE_SIZE] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  /* 0x10 */
  0,
  /* 0x11 */
  NdrPointerBufferSize, NdrPointerBufferSize,
  NdrPointerBufferSize, NdrPointerBufferSize,
  /* 0x15 */
  NdrSimpleStructBufferSize, NdrSimpleStructBufferSize,
  0, 0, 0,
  NdrComplexStructBufferSize,
  /* 0x1b */
  NdrConformantArrayBufferSize, 0, 0, 0, 0, 0,
  NdrComplexArrayBufferSize,
  /* 0x22 */
  NdrConformantStringBufferSize, 0, 0,
  NdrConformantStringBufferSize, 0, 0, 0, 0,
  /* 0x2a */
  0, 0, 0, 0, 0,
  /* 0x2f */
  NdrInterfacePointerBufferSize,
  /* 0xb0 */
  0, 0, 0, 0,
  NdrUserMarshalBufferSize
};
NDR_MEMORYSIZE NdrMemorySizer[NDR_TABLE_SIZE] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  /* 0x10 */
  0,
  /* 0x11 */
  NdrPointerMemorySize, NdrPointerMemorySize,
  NdrPointerMemorySize, NdrPointerMemorySize,
  /* 0x15 */
  NdrSimpleStructMemorySize, NdrSimpleStructMemorySize,
  0, 0, 0,
  NdrComplexStructMemorySize,
  /* 0x1b */
  NdrConformantArrayMemorySize, 0, 0, 0, 0, 0,
  NdrComplexArrayMemorySize,
  /* 0x22 */
  NdrConformantStringMemorySize, 0, 0,
  NdrConformantStringMemorySize, 0, 0, 0, 0,
  /* 0x2a */
  0, 0, 0, 0, 0,
  /* 0x2f */
  NdrInterfacePointerMemorySize,
  /* 0xb0 */
  0, 0, 0, 0,
  NdrUserMarshalMemorySize
};
NDR_FREE NdrFreer[NDR_TABLE_SIZE] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  /* 0x10 */
  0,
  /* 0x11 */
  NdrPointerFree, NdrPointerFree,
  NdrPointerFree, NdrPointerFree,
  /* 0x15 */
  NdrSimpleStructFree, NdrSimpleStructFree,
  0, 0, 0,
  NdrComplexStructFree,
  /* 0x1b */
  NdrConformantArrayFree, 0, 0, 0, 0, 0,
  NdrComplexArrayFree,
  /* 0x22 */
  0, 0, 0, 0, 0, 0, 0, 0,
  /* 0x2a */
  0, 0, 0, 0, 0,
  /* 0x2f */
  NdrInterfacePointerFree,
  /* 0xb0 */
  0, 0, 0, 0,
  NdrUserMarshalFree
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

PFORMAT_STRING ReadConformance(MIDL_STUB_MESSAGE *pStubMsg, PFORMAT_STRING pFormat)
{
  pStubMsg->MaxCount = NDR_LOCAL_UINT32_READ(pStubMsg->Buffer);
  pStubMsg->Buffer += 4;
  TRACE("unmarshalled conformance is %ld\n", pStubMsg->MaxCount);
  return pFormat+4;
}

PFORMAT_STRING ComputeConformance(MIDL_STUB_MESSAGE *pStubMsg, unsigned char *pMemory,
                                  PFORMAT_STRING pFormat, ULONG_PTR def)
{
  BYTE dtype = pFormat[0] & 0xf;
  DWORD ofs = (DWORD)pFormat[2] | ((DWORD)pFormat[3] << 8);
  LPVOID ptr = NULL;
  DWORD data = 0;

  if (pFormat[0] == 0xff) {
    /* null descriptor */
    pStubMsg->MaxCount = def;
    goto finish_conf;
  }

  switch (pFormat[0] & 0xf0) {
  case RPC_FC_NORMAL_CONFORMANCE:
    TRACE("normal conformance, ofs=%ld\n", ofs);
    ptr = pMemory + ofs;
    break;
  case RPC_FC_POINTER_CONFORMANCE:
    TRACE("pointer conformance, ofs=%ld\n", ofs);
    ptr = pStubMsg->Memory + ofs;
    break;
  case RPC_FC_TOP_LEVEL_CONFORMANCE:
    TRACE("toplevel conformance, ofs=%ld\n", ofs);
    if (pStubMsg->StackTop) {
      ptr = pStubMsg->StackTop + ofs;
    }
    else {
      /* -Os mode, MaxCount is already set */
      goto finish_conf;
    }
    break;
  case RPC_FC_CONSTANT_CONFORMANCE:
    data = ofs | ((DWORD)pFormat[1] << 16);
    TRACE("constant conformance, val=%ld\n", data);
    pStubMsg->MaxCount = data;
    goto finish_conf;
  case RPC_FC_TOP_LEVEL_MULTID_CONFORMANCE:
    FIXME("toplevel multidimensional conformance, ofs=%ld\n", ofs);
    if (pStubMsg->StackTop) {
      ptr = pStubMsg->StackTop + ofs;
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
    ptr = *(LPVOID*)ptr;
    break;
  case RPC_FC_CALLBACK:
    /* ofs is index into StubDesc->apfnExprEval */
    FIXME("handle callback\n");
    goto finish_conf;
  default:
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
  case RPC_FC_SMALL:
    data = *(CHAR*)ptr;
    break;
  case RPC_FC_USMALL:
    data = *(UCHAR*)ptr;
    break;
  default:
    FIXME("unknown conformance data type %x\n", dtype);
    goto done_conf_grab;
  }
  TRACE("dereferenced data type %x at %p, got %ld\n", dtype, ptr, data);

done_conf_grab:
  switch (pFormat[1]) {
  case 0: /* no op */
    pStubMsg->MaxCount = data;
    break;
  case RPC_FC_DEREFERENCE:
    /* already handled */
    break;
  default:
    FIXME("unknown conformance op %d\n", pFormat[1]);
    goto finish_conf;
  }

finish_conf:
  TRACE("resulting conformance is %ld\n", pStubMsg->MaxCount);
  return pFormat+4;
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
  unsigned long len, esize;
  unsigned char *c;

  TRACE("(pStubMsg == ^%p, pszMessage == ^%p, pFormat == ^%p)\n", pStubMsg, pszMessage, pFormat);
  
  assert(pFormat);
  if (*pFormat == RPC_FC_C_CSTRING) {
    TRACE("string=%s\n", debugstr_a(pszMessage));
    len = strlen(pszMessage)+1;
    esize = 1;
  }
  else if (*pFormat == RPC_FC_C_WSTRING) {
    TRACE("string=%s\n", debugstr_w((LPWSTR)pszMessage));
    len = strlenW((LPWSTR)pszMessage)+1;
    esize = 2;
  }
  else {
    ERR("Unhandled string type: %#x\n", *pFormat); 
    /* FIXME: raise an exception. */
    return NULL;
  }

  if (pFormat[1] != RPC_FC_PAD) {
    FIXME("sized string format=%d\n", pFormat[1]);
  }

  assert( (pStubMsg->BufferLength >= (len*esize + 13)) && (pStubMsg->Buffer != NULL) );

  c = pStubMsg->Buffer;
  memset(c, 0, 12);
  NDR_LOCAL_UINT32_WRITE(c, len); /* max length: strlen + 1 (for '\0') */
  c += 8;                         /* offset: 0 */
  NDR_LOCAL_UINT32_WRITE(c, len); /* actual length: (same) */
  c += 4;
  memcpy(c, pszMessage, len*esize); /* the string itself */
  c += len*esize;
  pStubMsg->Buffer = c;

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
  TRACE("(pStubMsg == ^%p, pMemory == ^%p, pFormat == ^%p)\n", pStubMsg, pMemory, pFormat);

  assert(pFormat);
  if (*pFormat == RPC_FC_C_CSTRING) {
    /* we need 12 octets for the [maxlen, offset, len] DWORDS, + 1 octet for '\0' */
    TRACE("string=%s\n", debugstr_a(pMemory));
    pStubMsg->BufferLength += strlen(pMemory) + 13 + BUFFER_PARANOIA;
  }
  else if (*pFormat == RPC_FC_C_WSTRING) {
    /* we need 12 octets for the [maxlen, offset, len] DWORDS, + 2 octets for L'\0' */
    TRACE("string=%s\n", debugstr_w((LPWSTR)pMemory));
    pStubMsg->BufferLength += strlenW((LPWSTR)pMemory)*2 + 14 + BUFFER_PARANOIA;
  }
  else {
    ERR("Unhandled string type: %#x\n", *pFormat); 
    /* FIXME: raise an exception */
  }

  if (pFormat[1] != RPC_FC_PAD) {
    FIXME("sized string format=%d\n", pFormat[1]);
  }
}

/************************************************************************
 *            NdrConformantStringMemorySize [RPCRT4.@]
 */
unsigned long WINAPI NdrConformantStringMemorySize( PMIDL_STUB_MESSAGE pStubMsg,
  PFORMAT_STRING pFormat )
{
  unsigned long rslt = 0;

  TRACE("(pStubMsg == ^%p, pFormat == ^%p)\n", pStubMsg, pFormat);
   
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

  TRACE("  --> %lu\n", rslt);
  return rslt;
}

/************************************************************************
 *           NdrConformantStringUnmarshall [RPCRT4.@]
 */
unsigned char *WINAPI NdrConformantStringUnmarshall( PMIDL_STUB_MESSAGE pStubMsg,
  unsigned char** ppMemory, PFORMAT_STRING pFormat, unsigned char fMustAlloc )
{
  unsigned long len, esize, ofs;
  unsigned char *pMem;

  TRACE("(pStubMsg == ^%p, *pMemory == ^%p, pFormat == ^%p, fMustAlloc == %u)\n",
    pStubMsg, *ppMemory, pFormat, fMustAlloc);

  assert(pFormat && ppMemory && pStubMsg);

  pStubMsg->Buffer += 4;
  ofs = NDR_LOCAL_UINT32_READ(pStubMsg->Buffer);
  pStubMsg->Buffer += 4;
  len = NDR_LOCAL_UINT32_READ(pStubMsg->Buffer);
  pStubMsg->Buffer += 4;

  if (*pFormat == RPC_FC_C_CSTRING) esize = 1;
  else if (*pFormat == RPC_FC_C_WSTRING) esize = 2;
  else {
    ERR("Unhandled string type: %#x\n", *pFormat);
    /* FIXME: raise an exception */
    esize = 0;
  }

  if (pFormat[1] != RPC_FC_PAD) {
    FIXME("sized string format=%d\n", pFormat[1]);
  }

  if (fMustAlloc) {
    *ppMemory = NdrAllocate(pStubMsg, len*esize + BUFFER_PARANOIA);
  } else {
    if (pStubMsg->ReuseBuffer && !*ppMemory)
      /* for servers, we may just point straight into the RPC buffer, I think
       * (I guess that's what MS does since MIDL code doesn't try to free) */
      *ppMemory = pStubMsg->Buffer - ofs*esize;
    /* for clients, memory should be provided by caller */
  }

  pMem = *ppMemory + ofs*esize;

  if (pMem != pStubMsg->Buffer)
    memcpy(pMem, pStubMsg->Buffer, len*esize);

  pStubMsg->Buffer += len*esize;

  if (*pFormat == RPC_FC_C_CSTRING) {
    TRACE("string=%s\n", debugstr_a(pMem));
  }
  else if (*pFormat == RPC_FC_C_WSTRING) {
    TRACE("string=%s\n", debugstr_w((LPWSTR)pMem));
  }

  return NULL; /* FIXME: is this always right? */
}

/***********************************************************************
 *           PointerMarshall
 */
void WINAPI PointerMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                            unsigned char *Buffer,
                            unsigned char *Pointer,
                            PFORMAT_STRING pFormat)
{
  unsigned type = pFormat[0], attr = pFormat[1];
  PFORMAT_STRING desc;
  NDR_MARSHALL m;

  TRACE("(%p,%p,%p,%p)\n", pStubMsg, Buffer, Pointer, pFormat);
  TRACE("type=%d, attr=%d\n", type, attr);
  pFormat += 2;
  if (attr & RPC_FC_P_SIMPLEPOINTER) desc = pFormat;
  else desc = pFormat + *(SHORT*)pFormat;
  if (attr & RPC_FC_P_DEREF) {
    Pointer = *(unsigned char**)Pointer;
    TRACE("deref => %p\n", Pointer);
  }

  *(LPVOID*)Buffer = 0;

  switch (type) {
  case RPC_FC_RP: /* ref pointer (always non-null) */
    break;
  default:
    FIXME("unhandled ptr type=%02x\n", type);
  }

  m = NdrMarshaller[*desc & NDR_TABLE_MASK];
  if (m) m(pStubMsg, Pointer, desc);
  else FIXME("no marshaller for data type=%02x\n", *desc);

  STD_OVERFLOW_CHECK(pStubMsg);
}

/***********************************************************************
 *           PointerUnmarshall
 */
void WINAPI PointerUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                              unsigned char *Buffer,
                              unsigned char **pPointer,
                              PFORMAT_STRING pFormat,
                              unsigned char fMustAlloc)
{
  unsigned type = pFormat[0], attr = pFormat[1];
  PFORMAT_STRING desc;
  NDR_UNMARSHALL m;

  TRACE("(%p,%p,%p,%p,%d)\n", pStubMsg, Buffer, pPointer, pFormat, fMustAlloc);
  TRACE("type=%d, attr=%d\n", type, attr);
  pFormat += 2;
  if (attr & RPC_FC_P_SIMPLEPOINTER) desc = pFormat;
  else desc = pFormat + *(SHORT*)pFormat;
  if (attr & RPC_FC_P_DEREF) {
    pPointer = *(unsigned char***)pPointer;
    TRACE("deref => %p\n", pPointer);
  }

  switch (type) {
  case RPC_FC_RP: /* ref pointer (always non-null) */
    break;
  default:
    FIXME("unhandled ptr type=%02x\n", type);
  }

  *pPointer = NULL;

  m = NdrUnmarshaller[*desc & NDR_TABLE_MASK];
  if (m) m(pStubMsg, pPointer, desc, fMustAlloc);
  else FIXME("no unmarshaller for data type=%02x\n", *desc);
  TRACE("pointer=%p\n", *pPointer);
}

/***********************************************************************
 *           PointerBufferSize
 */
void WINAPI PointerBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                              unsigned char *Pointer,
                              PFORMAT_STRING pFormat)
{
  unsigned type = pFormat[0], attr = pFormat[1];
  PFORMAT_STRING desc;
  NDR_BUFFERSIZE m;

  TRACE("(%p,%p,%p)\n", pStubMsg, Pointer, pFormat);
  TRACE("type=%d, attr=%d\n", type, attr);
  pFormat += 2;
  if (attr & RPC_FC_P_SIMPLEPOINTER) desc = pFormat;
  else desc = pFormat + *(SHORT*)pFormat;
  if (attr & RPC_FC_P_DEREF) {
    Pointer = *(unsigned char**)Pointer;
    TRACE("deref => %p\n", Pointer);
  }

  switch (type) {
  case RPC_FC_RP: /* ref pointer (always non-null) */
    break;
  default:
    FIXME("unhandled ptr type=%02x\n", type);
  }

  m = NdrBufferSizer[*desc & NDR_TABLE_MASK];
  if (m) m(pStubMsg, Pointer, desc);
  else FIXME("no buffersizer for data type=%02x\n", *desc);
}

/***********************************************************************
 *           PointerMemorySize [RPCRT4.@]
 */
unsigned long WINAPI PointerMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                       unsigned char *Buffer,
                                       PFORMAT_STRING pFormat)
{
  unsigned type = pFormat[0], attr = pFormat[1];
  PFORMAT_STRING desc;
  NDR_MEMORYSIZE m;

  FIXME("(%p,%p,%p): stub\n", pStubMsg, Buffer, pFormat);
  TRACE("type=%d, attr=%d\n", type, attr);
  pFormat += 2;
  if (attr & RPC_FC_P_SIMPLEPOINTER) desc = pFormat;
  else desc = pFormat + *(SHORT*)pFormat;
  if (attr & RPC_FC_P_DEREF) {
    TRACE("deref\n");
  }

  switch (type) {
  case RPC_FC_RP: /* ref pointer (always non-null) */
    break;
  default:
    FIXME("unhandled ptr type=%02x\n", type);
  }

  m = NdrMemorySizer[*desc & NDR_TABLE_MASK];
  if (m) m(pStubMsg, desc);
  else FIXME("no memorysizer for data type=%02x\n", *desc);

  return 0;
}

/***********************************************************************
 *           PointerFree [RPCRT4.@]
 */
void WINAPI PointerFree(PMIDL_STUB_MESSAGE pStubMsg,
                        unsigned char *Pointer,
                        PFORMAT_STRING pFormat)
{
  unsigned type = pFormat[0], attr = pFormat[1];
  PFORMAT_STRING desc;
  NDR_FREE m;

  TRACE("(%p,%p,%p)\n", pStubMsg, Pointer, pFormat);
  TRACE("type=%d, attr=%d\n", type, attr);
  if (attr & RPC_FC_P_DONTFREE) return;
  pFormat += 2;
  if (attr & RPC_FC_P_SIMPLEPOINTER) desc = pFormat;
  else desc = pFormat + *(SHORT*)pFormat;
  if (attr & RPC_FC_P_DEREF) {
    Pointer = *(unsigned char**)Pointer;
    TRACE("deref => %p\n", Pointer);
  }

  if (!Pointer) return;

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
    break;
  default:
    FIXME("unhandled data type=%02x\n", *desc);
  case RPC_FC_CARRAY:
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
unsigned char * WINAPI EmbeddedPointerMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                               unsigned char *pMemory,
                                               PFORMAT_STRING pFormat)
{
  unsigned char *Mark = pStubMsg->BufferMark;
  unsigned long Offset = pStubMsg->Offset;
  unsigned ofs, rep, count, stride, xofs;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  if (*pFormat != RPC_FC_PP) return NULL;
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
      rep = *(WORD*)&pFormat[2];
      stride = *(WORD*)&pFormat[4];
      ofs = *(WORD*)&pFormat[6];
      count = *(WORD*)&pFormat[8];
      xofs = 0;
      pFormat += 10;
      break;
    case RPC_FC_VARIABLE_REPEAT:
      rep = pStubMsg->MaxCount;
      stride = *(WORD*)&pFormat[2];
      ofs = *(WORD*)&pFormat[4];
      count = *(WORD*)&pFormat[6];
      xofs = (pFormat[1] == RPC_FC_VARIABLE_OFFSET) ? Offset * stride : 0;
      pFormat += 8;
      break;
    }
    /* ofs doesn't seem to matter in this context */
    while (rep) {
      PFORMAT_STRING info = pFormat;
      unsigned char *membase = pMemory + xofs;
      unsigned u;
      for (u=0; u<count; u++,info+=8) {
        unsigned char *memptr = membase + *(SHORT*)&info[0];
        unsigned char *bufptr = Mark + *(SHORT*)&info[2];
        PointerMarshall(pStubMsg, bufptr, *(unsigned char**)memptr, info+4);
      }
      rep--;
    }
    pFormat += 8 * count;
  }

  STD_OVERFLOW_CHECK(pStubMsg);

  return NULL;
}

/***********************************************************************
 *           EmbeddedPointerUnmarshall
 */
unsigned char * WINAPI EmbeddedPointerUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                                 unsigned char **ppMemory,
                                                 PFORMAT_STRING pFormat,
                                                 unsigned char fMustAlloc)
{
  unsigned char *Mark = pStubMsg->BufferMark;
  unsigned long Offset = pStubMsg->Offset;
  unsigned ofs, rep, count, stride, xofs;

  TRACE("(%p,%p,%p,%d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);

  if (*pFormat != RPC_FC_PP) return NULL;
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
      rep = *(WORD*)&pFormat[2];
      stride = *(WORD*)&pFormat[4];
      ofs = *(WORD*)&pFormat[6];
      count = *(WORD*)&pFormat[8];
      xofs = 0;
      pFormat += 10;
      break;
    case RPC_FC_VARIABLE_REPEAT:
      rep = pStubMsg->MaxCount;
      stride = *(WORD*)&pFormat[2];
      ofs = *(WORD*)&pFormat[4];
      count = *(WORD*)&pFormat[6];
      xofs = (pFormat[1] == RPC_FC_VARIABLE_OFFSET) ? Offset * stride : 0;
      pFormat += 8;
      break;
    }
    /* ofs doesn't seem to matter in this context */
    while (rep) {
      PFORMAT_STRING info = pFormat;
      unsigned char *membase = *ppMemory + xofs;
      unsigned u;
      for (u=0; u<count; u++,info+=8) {
        unsigned char *memptr = membase + *(SHORT*)&info[0];
        unsigned char *bufptr = Mark + *(SHORT*)&info[2];
        PointerUnmarshall(pStubMsg, bufptr, (unsigned char**)memptr, info+4, fMustAlloc);
      }
      rep--;
    }
    pFormat += 8 * count;
  }

  return NULL;
}

/***********************************************************************
 *           EmbeddedPointerBufferSize
 */
void WINAPI EmbeddedPointerBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                      unsigned char *pMemory,
                                      PFORMAT_STRING pFormat)
{
  unsigned long Offset = pStubMsg->Offset;
  unsigned ofs, rep, count, stride, xofs;

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
      rep = *(WORD*)&pFormat[2];
      stride = *(WORD*)&pFormat[4];
      ofs = *(WORD*)&pFormat[6];
      count = *(WORD*)&pFormat[8];
      xofs = 0;
      pFormat += 10;
      break;
    case RPC_FC_VARIABLE_REPEAT:
      rep = pStubMsg->MaxCount;
      stride = *(WORD*)&pFormat[2];
      ofs = *(WORD*)&pFormat[4];
      count = *(WORD*)&pFormat[6];
      xofs = (pFormat[1] == RPC_FC_VARIABLE_OFFSET) ? Offset * stride : 0;
      pFormat += 8;
      break;
    }
    /* ofs doesn't seem to matter in this context */
    while (rep) {
      PFORMAT_STRING info = pFormat;
      unsigned char *membase = pMemory + xofs;
      unsigned u;
      for (u=0; u<count; u++,info+=8) {
        unsigned char *memptr = membase + *(SHORT*)&info[0];
        PointerBufferSize(pStubMsg, *(unsigned char**)memptr, info+4);
      }
      rep--;
    }
    pFormat += 8 * count;
  }
}

/***********************************************************************
 *           EmbeddedPointerMemorySize
 */
unsigned long WINAPI EmbeddedPointerMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                               PFORMAT_STRING pFormat)
{
  unsigned long Offset = pStubMsg->Offset;
  unsigned char *Mark = pStubMsg->BufferMark;
  unsigned ofs, rep, count, stride, xofs;

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
      rep = *(WORD*)&pFormat[2];
      stride = *(WORD*)&pFormat[4];
      ofs = *(WORD*)&pFormat[6];
      count = *(WORD*)&pFormat[8];
      xofs = 0;
      pFormat += 10;
      break;
    case RPC_FC_VARIABLE_REPEAT:
      rep = pStubMsg->MaxCount;
      stride = *(WORD*)&pFormat[2];
      ofs = *(WORD*)&pFormat[4];
      count = *(WORD*)&pFormat[6];
      xofs = (pFormat[1] == RPC_FC_VARIABLE_OFFSET) ? Offset * stride : 0;
      pFormat += 8;
      break;
    }
    /* ofs doesn't seem to matter in this context */
    while (rep) {
      PFORMAT_STRING info = pFormat;
      unsigned u;
      for (u=0; u<count; u++,info+=8) {
        unsigned char *bufptr = Mark + *(SHORT*)&info[2];
        PointerMemorySize(pStubMsg, bufptr, info+4);
      }
      rep--;
    }
    pFormat += 8 * count;
  }

  return 0;
}

/***********************************************************************
 *           EmbeddedPointerFree
 */
void WINAPI EmbeddedPointerFree(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
  unsigned long Offset = pStubMsg->Offset;
  unsigned ofs, rep, count, stride, xofs;

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
      rep = *(WORD*)&pFormat[2];
      stride = *(WORD*)&pFormat[4];
      ofs = *(WORD*)&pFormat[6];
      count = *(WORD*)&pFormat[8];
      xofs = 0;
      pFormat += 10;
      break;
    case RPC_FC_VARIABLE_REPEAT:
      rep = pStubMsg->MaxCount;
      stride = *(WORD*)&pFormat[2];
      ofs = *(WORD*)&pFormat[4];
      count = *(WORD*)&pFormat[6];
      xofs = (pFormat[1] == RPC_FC_VARIABLE_OFFSET) ? Offset * stride : 0;
      pFormat += 8;
      break;
    }
    /* ofs doesn't seem to matter in this context */
    while (rep) {
      PFORMAT_STRING info = pFormat;
      unsigned char *membase = pMemory + xofs;
      unsigned u;
      for (u=0; u<count; u++,info+=8) {
        unsigned char *memptr = membase + *(SHORT*)&info[0];
        PointerFree(pStubMsg, *(unsigned char**)memptr, info+4);
      }
      rep--;
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
  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  pStubMsg->BufferMark = pStubMsg->Buffer;
  PointerMarshall(pStubMsg, pStubMsg->Buffer, pMemory, pFormat);
  pStubMsg->Buffer += 4;

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
  TRACE("(%p,%p,%p,%d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);

  pStubMsg->BufferMark = pStubMsg->Buffer;
  PointerUnmarshall(pStubMsg, pStubMsg->Buffer, ppMemory, pFormat, fMustAlloc);
  pStubMsg->Buffer += 4;

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
  pStubMsg->BufferLength += 4;
  PointerBufferSize(pStubMsg, pMemory, pFormat);
}

/***********************************************************************
 *           NdrPointerMemorySize [RPCRT4.@]
 */
unsigned long WINAPI NdrPointerMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
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
 *           NdrSimpleStructMarshall [RPCRT4.@]
 */
unsigned char * WINAPI NdrSimpleStructMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                               unsigned char *pMemory,
                                               PFORMAT_STRING pFormat)
{
  unsigned size = *(LPWORD)(pFormat+2);
  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

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
  unsigned size = *(LPWORD)(pFormat+2);
  TRACE("(%p,%p,%p,%d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);

  if (fMustAlloc) {
    *ppMemory = NdrAllocate(pStubMsg, size);
    memcpy(*ppMemory, pStubMsg->Buffer, size);
  } else {
    if (pStubMsg->ReuseBuffer && !*ppMemory)
      /* for servers, we may just point straight into the RPC buffer, I think
       * (I guess that's what MS does since MIDL code doesn't try to free) */
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
 *           NdrSimpleStructUnmarshall [RPCRT4.@]
 */
void WINAPI NdrSimpleTypeMarshall( PMIDL_STUB_MESSAGE pStubMsg, unsigned char* pMemory,
                                   unsigned char FormatChar )
{
    FIXME("stub\n");
}


/***********************************************************************
 *           NdrSimpleStructUnmarshall [RPCRT4.@]
 */
void WINAPI NdrSimpleTypeUnmarshall( PMIDL_STUB_MESSAGE pStubMsg, unsigned char* pMemory,
                                     unsigned char FormatChar )
{
    FIXME("stub\n");
}


/***********************************************************************
 *           NdrSimpleStructBufferSize [RPCRT4.@]
 */
void WINAPI NdrSimpleStructBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                      unsigned char *pMemory,
                                      PFORMAT_STRING pFormat)
{
  unsigned size = *(LPWORD)(pFormat+2);
  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  pStubMsg->BufferLength += size;
  if (pFormat[0] != RPC_FC_STRUCT)
    EmbeddedPointerBufferSize(pStubMsg, pMemory, pFormat+4);
}

/***********************************************************************
 *           NdrSimpleStructMemorySize [RPCRT4.@]
 */
unsigned long WINAPI NdrSimpleStructMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                               PFORMAT_STRING pFormat)
{
  /* unsigned size = *(LPWORD)(pFormat+2); */
  FIXME("(%p,%p): stub\n", pStubMsg, pFormat);
  if (pFormat[0] != RPC_FC_STRUCT)
    EmbeddedPointerMemorySize(pStubMsg, pFormat+4);
  return 0;
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


unsigned long WINAPI EmbeddedComplexSize(PMIDL_STUB_MESSAGE pStubMsg,
                                         PFORMAT_STRING pFormat)
{
  switch (*pFormat) {
  case RPC_FC_STRUCT:
  case RPC_FC_PSTRUCT:
  case RPC_FC_CSTRUCT:
  case RPC_FC_BOGUS_STRUCT:
    return *(WORD*)&pFormat[2];
  case RPC_FC_USER_MARSHAL:
    return *(WORD*)&pFormat[4];
  default:
    FIXME("unhandled embedded type %02x\n", *pFormat);
  }
  return 0;
}


unsigned char * WINAPI ComplexMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                       unsigned char *pMemory,
                                       PFORMAT_STRING pFormat,
                                       PFORMAT_STRING pPointer)
{
  PFORMAT_STRING desc;
  NDR_MARSHALL m;
  unsigned long size;

  while (*pFormat != RPC_FC_END) {
    switch (*pFormat) {
    case RPC_FC_SHORT:
    case RPC_FC_USHORT:
      TRACE("short=%d <= %p\n", *(WORD*)pMemory, pMemory);
      memcpy(pStubMsg->Buffer, pMemory, 2);
      pStubMsg->Buffer += 2;
      pMemory += 2;
      break;
    case RPC_FC_LONG:
    case RPC_FC_ULONG:
      TRACE("long=%ld <= %p\n", *(DWORD*)pMemory, pMemory);
      memcpy(pStubMsg->Buffer, pMemory, 4);
      pStubMsg->Buffer += 4;
      pMemory += 4;
      break;
    case RPC_FC_POINTER:
      TRACE("pointer=%p <= %p\n", *(unsigned char**)pMemory, pMemory);
      NdrPointerMarshall(pStubMsg, *(unsigned char**)pMemory, pPointer);
      pPointer += 4;
      pMemory += 4;
      break;
    case RPC_FC_ALIGNM4:
      ALIGN_POINTER(pMemory, 3);
      break;
    case RPC_FC_ALIGNM8:
      ALIGN_POINTER(pMemory, 7);
      break;
    case RPC_FC_EMBEDDED_COMPLEX:
      pMemory += pFormat[1];
      pFormat += 2;
      desc = pFormat + *(SHORT*)pFormat;
      size = EmbeddedComplexSize(pStubMsg, desc);
      TRACE("embedded complex (size=%ld) <= %p\n", size, pMemory);
      m = NdrMarshaller[*desc & NDR_TABLE_MASK];
      if (m) m(pStubMsg, pMemory, desc);
      else FIXME("no marshaller for embedded type %02x\n", *desc);
      pMemory += size;
      pFormat += 2;
      continue;
    case RPC_FC_PAD:
      break;
    default:
      FIXME("unhandled format %02x\n", *pFormat);
    }
    pFormat++;
  }

  return pMemory;
}

unsigned char * WINAPI ComplexUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                         unsigned char *pMemory,
                                         PFORMAT_STRING pFormat,
                                         PFORMAT_STRING pPointer,
                                         unsigned char fMustAlloc)
{
  PFORMAT_STRING desc;
  NDR_UNMARSHALL m;
  unsigned long size;

  while (*pFormat != RPC_FC_END) {
    switch (*pFormat) {
    case RPC_FC_SHORT:
    case RPC_FC_USHORT:
      memcpy(pMemory, pStubMsg->Buffer, 2);
      TRACE("short=%d => %p\n", *(WORD*)pMemory, pMemory);
      pStubMsg->Buffer += 2;
      pMemory += 2;
      break;
    case RPC_FC_LONG:
    case RPC_FC_ULONG:
      memcpy(pMemory, pStubMsg->Buffer, 4);
      TRACE("long=%ld => %p\n", *(DWORD*)pMemory, pMemory);
      pStubMsg->Buffer += 4;
      pMemory += 4;
      break;
    case RPC_FC_POINTER:
      *(unsigned char**)pMemory = NULL;
      TRACE("pointer => %p\n", pMemory);
      NdrPointerUnmarshall(pStubMsg, (unsigned char**)pMemory, pPointer, fMustAlloc);
      pPointer += 4;
      pMemory += 4;
      break;
    case RPC_FC_ALIGNM4:
      ALIGN_POINTER(pMemory, 3);
      break;
    case RPC_FC_ALIGNM8:
      ALIGN_POINTER(pMemory, 7);
      break;
    case RPC_FC_EMBEDDED_COMPLEX:
      pMemory += pFormat[1];
      pFormat += 2;
      desc = pFormat + *(SHORT*)pFormat;
      size = EmbeddedComplexSize(pStubMsg, desc);
      TRACE("embedded complex (size=%ld) => %p\n", size, pMemory);
      m = NdrUnmarshaller[*desc & NDR_TABLE_MASK];
      memset(pMemory, 0, size); /* just in case */
      if (m) m(pStubMsg, &pMemory, desc, fMustAlloc);
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

unsigned char * WINAPI ComplexBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                         unsigned char *pMemory,
                                         PFORMAT_STRING pFormat,
                                         PFORMAT_STRING pPointer)
{
  PFORMAT_STRING desc;
  NDR_BUFFERSIZE m;
  unsigned long size;

  while (*pFormat != RPC_FC_END) {
    switch (*pFormat) {
    case RPC_FC_SHORT:
    case RPC_FC_USHORT:
      pStubMsg->BufferLength += 2;
      pMemory += 2;
      break;
    case RPC_FC_LONG:
    case RPC_FC_ULONG:
      pStubMsg->BufferLength += 4;
      pMemory += 4;
      break;
    case RPC_FC_POINTER:
      NdrPointerBufferSize(pStubMsg, *(unsigned char**)pMemory, pPointer);
      pPointer += 4;
      pMemory += 4;
      break;
    case RPC_FC_ALIGNM4:
      ALIGN_POINTER(pMemory, 3);
      break;
    case RPC_FC_ALIGNM8:
      ALIGN_POINTER(pMemory, 7);
      break;
    case RPC_FC_EMBEDDED_COMPLEX:
      pMemory += pFormat[1];
      pFormat += 2;
      desc = pFormat + *(SHORT*)pFormat;
      size = EmbeddedComplexSize(pStubMsg, desc);
      m = NdrBufferSizer[*desc & NDR_TABLE_MASK];
      if (m) m(pStubMsg, pMemory, desc);
      else FIXME("no buffersizer for embedded type %02x\n", *desc);
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

unsigned char * WINAPI ComplexFree(PMIDL_STUB_MESSAGE pStubMsg,
                                   unsigned char *pMemory,
                                   PFORMAT_STRING pFormat,
                                   PFORMAT_STRING pPointer)
{
  PFORMAT_STRING desc;
  NDR_FREE m;
  unsigned long size;

  while (*pFormat != RPC_FC_END) {
    switch (*pFormat) {
    case RPC_FC_SHORT:
    case RPC_FC_USHORT:
      pMemory += 2;
      break;
    case RPC_FC_LONG:
    case RPC_FC_ULONG:
      pMemory += 4;
      break;
    case RPC_FC_POINTER:
      NdrPointerFree(pStubMsg, *(unsigned char**)pMemory, pPointer);
      pPointer += 4;
      pMemory += 4;
      break;
    case RPC_FC_ALIGNM4:
      ALIGN_POINTER(pMemory, 3);
      break;
    case RPC_FC_ALIGNM8:
      ALIGN_POINTER(pMemory, 7);
      break;
    case RPC_FC_EMBEDDED_COMPLEX:
      pMemory += pFormat[1];
      pFormat += 2;
      desc = pFormat + *(SHORT*)pFormat;
      size = EmbeddedComplexSize(pStubMsg, desc);
      m = NdrFreer[*desc & NDR_TABLE_MASK];
      if (m) m(pStubMsg, pMemory, desc);
      else FIXME("no freer for embedded type %02x\n", *desc);
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

unsigned long WINAPI ComplexStructSize(PMIDL_STUB_MESSAGE pStubMsg,
                                       PFORMAT_STRING pFormat)
{
  PFORMAT_STRING desc;
  unsigned long size = 0;

  while (*pFormat != RPC_FC_END) {
    switch (*pFormat) {
    case RPC_FC_SHORT:
    case RPC_FC_USHORT:
      size += 2;
      break;
    case RPC_FC_LONG:
    case RPC_FC_ULONG:
      size += 4;
      break;
    case RPC_FC_POINTER:
      size += 4;
      break;
    case RPC_FC_ALIGNM4:
      ALIGN_LENGTH(size, 3);
      break;
    case RPC_FC_ALIGNM8:
      ALIGN_LENGTH(size, 7);
      break;
    case RPC_FC_EMBEDDED_COMPLEX:
      size += pFormat[1];
      pFormat += 2;
      desc = pFormat + *(SHORT*)pFormat;
      size += EmbeddedComplexSize(pStubMsg, desc);
      pFormat += 2;
      continue;
    case RPC_FC_PAD:
      break;
    default:
      FIXME("unhandled format %d\n", *pFormat);
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

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  pFormat += 4;
  if (*(WORD*)pFormat) conf_array = pFormat + *(WORD*)pFormat;
  pFormat += 2;
  if (*(WORD*)pFormat) pointer_desc = pFormat + *(WORD*)pFormat;
  pFormat += 2;

  pStubMsg->Memory = pMemory;

  ComplexMarshall(pStubMsg, pMemory, pFormat, pointer_desc);

  if (conf_array)
    NdrConformantArrayMarshall(pStubMsg, pMemory, conf_array);

  pStubMsg->Memory = OldMemory;

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
  unsigned size = *(LPWORD)(pFormat+2);
  PFORMAT_STRING conf_array = NULL;
  PFORMAT_STRING pointer_desc = NULL;
  unsigned char *pMemory;

  TRACE("(%p,%p,%p,%d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);

  if (fMustAlloc || !*ppMemory)
    *ppMemory = NdrAllocate(pStubMsg, size);

  pFormat += 4;
  if (*(WORD*)pFormat) conf_array = pFormat + *(WORD*)pFormat;
  pFormat += 2;
  if (*(WORD*)pFormat) pointer_desc = pFormat + *(WORD*)pFormat;
  pFormat += 2;

  pMemory = ComplexUnmarshall(pStubMsg, *ppMemory, pFormat, pointer_desc, fMustAlloc);

  if (conf_array)
    NdrConformantArrayUnmarshall(pStubMsg, &pMemory, conf_array, fMustAlloc);

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

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  pFormat += 4;
  if (*(WORD*)pFormat) conf_array = pFormat + *(WORD*)pFormat;
  pFormat += 2;
  if (*(WORD*)pFormat) pointer_desc = pFormat + *(WORD*)pFormat;
  pFormat += 2;

  pStubMsg->Memory = pMemory;

  pMemory = ComplexBufferSize(pStubMsg, pMemory, pFormat, pointer_desc);

  if (conf_array)
    NdrConformantArrayBufferSize(pStubMsg, pMemory, conf_array);

  pStubMsg->Memory = OldMemory;
}

/***********************************************************************
 *           NdrComplexStructMemorySize [RPCRT4.@]
 */
unsigned long WINAPI NdrComplexStructMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                                PFORMAT_STRING pFormat)
{
  /* unsigned size = *(LPWORD)(pFormat+2); */
  PFORMAT_STRING conf_array = NULL;
  PFORMAT_STRING pointer_desc = NULL;

  FIXME("(%p,%p): stub\n", pStubMsg, pFormat);

  pFormat += 4;
  if (*(WORD*)pFormat) conf_array = pFormat + *(WORD*)pFormat;
  pFormat += 2;
  if (*(WORD*)pFormat) pointer_desc = pFormat + *(WORD*)pFormat;
  pFormat += 2;

  return 0;
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
  if (*(WORD*)pFormat) conf_array = pFormat + *(WORD*)pFormat;
  pFormat += 2;
  if (*(WORD*)pFormat) pointer_desc = pFormat + *(WORD*)pFormat;
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
  DWORD size = 0, esize = *(LPWORD)(pFormat+2);
  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  if (pFormat[0] != RPC_FC_CARRAY) FIXME("format=%d\n", pFormat[0]);

  pFormat = ComputeConformance(pStubMsg, pMemory, pFormat+4, 0);
  size = pStubMsg->MaxCount;

  NDR_LOCAL_UINT32_WRITE(pStubMsg->Buffer, size);
  pStubMsg->Buffer += 4;

  memcpy(pStubMsg->Buffer, pMemory, size*esize);
  pStubMsg->BufferMark = pStubMsg->Buffer;
  pStubMsg->Buffer += size*esize;

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
  DWORD size = 0, esize = *(LPWORD)(pFormat+2);
  TRACE("(%p,%p,%p,%d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);
  if (pFormat[0] != RPC_FC_CARRAY) FIXME("format=%d\n", pFormat[0]);

  pFormat = ReadConformance(pStubMsg, pFormat+4);
  size = pStubMsg->MaxCount;

  if (fMustAlloc) {
    *ppMemory = NdrAllocate(pStubMsg, size*esize);
    memcpy(*ppMemory, pStubMsg->Buffer, size*esize);
  } else {
    if (pStubMsg->ReuseBuffer && !*ppMemory)
      /* for servers, we may just point straight into the RPC buffer, I think
       * (I guess that's what MS does since MIDL code doesn't try to free) */
      *ppMemory = pStubMsg->Buffer;
    else
      /* for clients, memory should be provided by caller */
      memcpy(*ppMemory, pStubMsg->Buffer, size*esize);
  }

  pStubMsg->BufferMark = pStubMsg->Buffer;
  pStubMsg->Buffer += size*esize;

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
  DWORD size = 0, esize = *(LPWORD)(pFormat+2);
  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  if (pFormat[0] != RPC_FC_CARRAY) FIXME("format=%d\n", pFormat[0]);

  pFormat = ComputeConformance(pStubMsg, pMemory, pFormat+4, 0);
  size = pStubMsg->MaxCount;

  pStubMsg->BufferLength += size*esize;

  EmbeddedPointerBufferSize(pStubMsg, pMemory, pFormat);
}

/***********************************************************************
 *           NdrConformantArrayMemorySize [RPCRT4.@]
 */
unsigned long WINAPI NdrConformantArrayMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                                  PFORMAT_STRING pFormat)
{
  DWORD size = 0;
  FIXME("(%p,%p): stub\n", pStubMsg, pFormat);
  if (pFormat[0] != RPC_FC_CARRAY) FIXME("format=%d\n", pFormat[0]);

  pFormat = ReadConformance(pStubMsg, pFormat+4);
  size = pStubMsg->MaxCount;

  EmbeddedPointerMemorySize(pStubMsg, pFormat);

  return 0;
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

  EmbeddedPointerFree(pStubMsg, pMemory, pFormat);
}


/***********************************************************************
 *           NdrConformantVaryingArrayMarshall  [RPCRT4.@]
 */
unsigned char* WINAPI NdrConformantVaryingArrayMarshall( PMIDL_STUB_MESSAGE pStubMsg,
                                                         unsigned char* pMemory,
                                                         PFORMAT_STRING pFormat )
{
    FIXME( "stub\n" );
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
    FIXME( "stub\n" );
    return NULL;
}


/***********************************************************************
 *           NdrConformantVaryingArrayFree  [RPCRT4.@]
 */
void WINAPI NdrConformantVaryingArrayFree( PMIDL_STUB_MESSAGE pStubMsg,
                                           unsigned char* pMemory,
                                           PFORMAT_STRING pFormat )
{
    FIXME( "stub\n" );
}


/***********************************************************************
 *           NdrConformantVaryingArrayBufferSize  [RPCRT4.@]
 */
void WINAPI NdrConformantVaryingArrayBufferSize( PMIDL_STUB_MESSAGE pStubMsg,
                                                 unsigned char* pMemory, PFORMAT_STRING pFormat )
{
    FIXME( "stub\n" );
}


/***********************************************************************
 *           NdrConformantVaryingArrayMemorySize  [RPCRT4.@]
 */
unsigned long WINAPI NdrConformantVaryingArrayMemorySize( PMIDL_STUB_MESSAGE pStubMsg,
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
  DWORD size = 0, count, def;
  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  def = *(WORD*)&pFormat[2];
  pFormat += 4;

  pFormat = ComputeConformance(pStubMsg, pMemory, pFormat, def);
  size = pStubMsg->MaxCount;
  TRACE("conformance=%ld\n", size);

  if (*(DWORD*)pFormat != 0xffffffff)
    FIXME("compute variance\n");
  pFormat += 4;

  NDR_LOCAL_UINT32_WRITE(pStubMsg->Buffer, size);
  pStubMsg->Buffer += 4;

  for (count=0; count<size; count++)
    pMemory = ComplexMarshall(pStubMsg, pMemory, pFormat, NULL);

  STD_OVERFLOW_CHECK(pStubMsg);

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
  DWORD size = 0, count, esize;
  unsigned char *pMemory;
  TRACE("(%p,%p,%p,%d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);

  pFormat += 4;

  pFormat = ReadConformance(pStubMsg, pFormat);
  size = pStubMsg->MaxCount;
  TRACE("conformance=%ld\n", size);

  pFormat += 4;

  esize = ComplexStructSize(pStubMsg, pFormat);

  if (fMustAlloc || !*ppMemory)
    *ppMemory = NdrAllocate(pStubMsg, size*esize);

  pMemory = *ppMemory;
  for (count=0; count<size; count++)
    pMemory = ComplexUnmarshall(pStubMsg, pMemory, pFormat, NULL, fMustAlloc);

  return NULL;
}

/***********************************************************************
 *           NdrComplexArrayBufferSize [RPCRT4.@]
 */
void WINAPI NdrComplexArrayBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                      unsigned char *pMemory,
                                      PFORMAT_STRING pFormat)
{
  DWORD size = 0, count, def;
  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  def = *(WORD*)&pFormat[2];
  pFormat += 4;

  pFormat = ComputeConformance(pStubMsg, pMemory, pFormat, def);
  size = pStubMsg->MaxCount;
  TRACE("conformance=%ld\n", size);

  if (*(DWORD*)pFormat != 0xffffffff)
    FIXME("compute variance\n");
  pFormat += 4;

  for (count=0; count<size; count++)
    pMemory = ComplexBufferSize(pStubMsg, pMemory, pFormat, NULL);
}

/***********************************************************************
 *           NdrComplexArrayMemorySize [RPCRT4.@]
 */
unsigned long WINAPI NdrComplexArrayMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                               PFORMAT_STRING pFormat)
{
  DWORD size = 0;
  FIXME("(%p,%p): stub\n", pStubMsg, pFormat);

  pFormat += 4;

  pFormat = ReadConformance(pStubMsg, pFormat);
  size = pStubMsg->MaxCount;
  TRACE("conformance=%ld\n", size);

  pFormat += 4;

  return 0;
}

/***********************************************************************
 *           NdrComplexArrayFree [RPCRT4.@]
 */
void WINAPI NdrComplexArrayFree(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
  DWORD size = 0, count, def;
  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);

  def = *(WORD*)&pFormat[2];
  pFormat += 4;

  pFormat = ComputeConformance(pStubMsg, pMemory, pFormat, def);
  size = pStubMsg->MaxCount;
  TRACE("conformance=%ld\n", size);

  if (*(DWORD*)pFormat != 0xffffffff)
    FIXME("compute variance\n");
  pFormat += 4;

  for (count=0; count<size; count++)
    pMemory = ComplexFree(pStubMsg, pMemory, pFormat, NULL);
}

unsigned long UserMarshalFlags(PMIDL_STUB_MESSAGE pStubMsg)
{
  return MAKELONG(pStubMsg->dwDestContext,
                  pStubMsg->RpcMsg->DataRepresentation);
}

/***********************************************************************
 *           NdrUserMarshalMarshall [RPCRT4.@]
 */
unsigned char * WINAPI NdrUserMarshalMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                              unsigned char *pMemory,
                                              PFORMAT_STRING pFormat)
{
/*  unsigned flags = pFormat[1]; */
  unsigned index = *(WORD*)&pFormat[2];
  unsigned long uflag = UserMarshalFlags(pStubMsg);
  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  TRACE("index=%d\n", index);

  pStubMsg->Buffer =
    pStubMsg->StubDesc->aUserMarshalQuadruple[index].pfnMarshall(
      &uflag, pStubMsg->Buffer, pMemory);

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
/*  unsigned flags = pFormat[1];*/
  unsigned index = *(WORD*)&pFormat[2];
  DWORD memsize = *(WORD*)&pFormat[4];
  unsigned long uflag = UserMarshalFlags(pStubMsg);
  TRACE("(%p,%p,%p,%d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);
  TRACE("index=%d\n", index);

  if (fMustAlloc || !*ppMemory)
    *ppMemory = NdrAllocate(pStubMsg, memsize);

  pStubMsg->Buffer =
    pStubMsg->StubDesc->aUserMarshalQuadruple[index].pfnUnmarshall(
      &uflag, pStubMsg->Buffer, *ppMemory);

  return NULL;
}

/***********************************************************************
 *           NdrUserMarshalBufferSize [RPCRT4.@]
 */
void WINAPI NdrUserMarshalBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                      unsigned char *pMemory,
                                      PFORMAT_STRING pFormat)
{
/*  unsigned flags = pFormat[1];*/
  unsigned index = *(WORD*)&pFormat[2];
  DWORD bufsize = *(WORD*)&pFormat[6];
  unsigned long uflag = UserMarshalFlags(pStubMsg);
  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  TRACE("index=%d\n", index);

  if (bufsize) {
    TRACE("size=%ld\n", bufsize);
    pStubMsg->BufferLength += bufsize;
    return;
  }

  pStubMsg->BufferLength =
    pStubMsg->StubDesc->aUserMarshalQuadruple[index].pfnBufferSize(
      &uflag, pStubMsg->BufferLength, pMemory);
}

/***********************************************************************
 *           NdrUserMarshalMemorySize [RPCRT4.@]
 */
unsigned long WINAPI NdrUserMarshalMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                              PFORMAT_STRING pFormat)
{
  unsigned index = *(WORD*)&pFormat[2];
/*  DWORD memsize = *(WORD*)&pFormat[4]; */
  FIXME("(%p,%p): stub\n", pStubMsg, pFormat);
  TRACE("index=%d\n", index);

  return 0;
}

/***********************************************************************
 *           NdrUserMarshalFree [RPCRT4.@]
 */
void WINAPI NdrUserMarshalFree(PMIDL_STUB_MESSAGE pStubMsg,
                                unsigned char *pMemory,
                                PFORMAT_STRING pFormat)
{
/*  unsigned flags = pFormat[1]; */
  unsigned index = *(WORD*)&pFormat[2];
  unsigned long uflag = UserMarshalFlags(pStubMsg);
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
void WINAPI NdrConvert2( PMIDL_STUB_MESSAGE pStubMsg, PFORMAT_STRING pFormat, long NumberParams )
{
  FIXME("(pStubMsg == ^%p, pFormat == ^%p, NumberParams == %ld): stub.\n",
    pStubMsg, pFormat, NumberParams);
  /* FIXME: since this stub doesn't do any converting, the proper behavior
     is to raise an exception */
}
