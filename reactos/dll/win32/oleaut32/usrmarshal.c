/*
 * Misc marshalling routines
 *
 * Copyright 2002 Ove Kaaven
 * Copyright 2003 Mike Hearn
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

#include <stdarg.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"

#include "ole2.h"
#include "oleauto.h"
#include "rpcproxy.h"
#include "typelib.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define ALIGNED_LENGTH(_Len, _Align) (((_Len)+(_Align))&~(_Align))
#define ALIGNED_POINTER(_Ptr, _Align) ((LPVOID)ALIGNED_LENGTH((ULONG_PTR)(_Ptr), _Align))
#define ALIGN_LENGTH(_Len, _Align) _Len = ALIGNED_LENGTH(_Len, _Align)
#define ALIGN_POINTER(_Ptr, _Align) _Ptr = ALIGNED_POINTER(_Ptr, _Align)

static CStdPSFactoryBuffer PSFactoryBuffer;

CSTDSTUBBUFFERRELEASE(&PSFactoryBuffer)

extern const ExtendedProxyFileInfo oaidl_ProxyFileInfo;

static const ProxyFileInfo *OLEAUT32_ProxyFileList[] = {
  &oaidl_ProxyFileInfo,
  NULL
};

HRESULT OLEAUTPS_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
  return NdrDllGetClassObject(rclsid, riid, ppv, OLEAUT32_ProxyFileList,
                              &CLSID_PSDispatch, &PSFactoryBuffer);
}

static void dump_user_flags(ULONG *pFlags)
{
    if (HIWORD(*pFlags) == NDR_LOCAL_DATA_REPRESENTATION)
        TRACE("MAKELONG(NDR_LOCAL_REPRESENTATION, ");
    else
        TRACE("MAKELONG(0x%04x, ", HIWORD(*pFlags));
    switch (LOWORD(*pFlags))
    {
        case MSHCTX_LOCAL: TRACE("MSHCTX_LOCAL)"); break;
        case MSHCTX_NOSHAREDMEM: TRACE("MSHCTX_NOSHAREDMEM)"); break;
        case MSHCTX_DIFFERENTMACHINE: TRACE("MSHCTX_DIFFERENTMACHINE)"); break;
        case MSHCTX_INPROC: TRACE("MSHCTX_INPROC)"); break;
        default: TRACE("%d)", LOWORD(*pFlags));
    }
}

/* CLEANLOCALSTORAGE */

#define CLS_FUNCDESC  'f'
#define CLS_LIBATTR   'l'
#define CLS_TYPEATTR  't'
#define CLS_VARDESC   'v'

ULONG WINAPI CLEANLOCALSTORAGE_UserSize(ULONG *pFlags, ULONG Start, CLEANLOCALSTORAGE *pstg)
{
    ALIGN_LENGTH(Start, 3);
    return Start + sizeof(DWORD);
}

unsigned char * WINAPI CLEANLOCALSTORAGE_UserMarshal(ULONG *pFlags, unsigned char *Buffer, CLEANLOCALSTORAGE *pstg)
{
    ALIGN_POINTER(Buffer, 3);
    *(DWORD*)Buffer = pstg->flags;
    switch(pstg->flags)
    {
    case CLS_LIBATTR:
        ITypeLib_ReleaseTLibAttr((ITypeLib*)pstg->pInterface, *(TLIBATTR**)pstg->pStorage);
        break;
    case CLS_TYPEATTR:
        ITypeInfo_ReleaseTypeAttr((ITypeInfo*)pstg->pInterface, *(TYPEATTR**)pstg->pStorage);
        break;
    case CLS_FUNCDESC:
        ITypeInfo_ReleaseFuncDesc((ITypeInfo*)pstg->pInterface, *(FUNCDESC**)pstg->pStorage);
        break;
    case CLS_VARDESC:
        ITypeInfo_ReleaseVarDesc((ITypeInfo*)pstg->pInterface, *(VARDESC**)pstg->pStorage);
        break;

    default:
        ERR("Unknown type %x\n", pstg->flags);
    }

    *(VOID**)pstg->pStorage = NULL;
    IUnknown_Release(pstg->pInterface);
    pstg->pInterface = NULL;

    return Buffer + sizeof(DWORD);
}

unsigned char * WINAPI CLEANLOCALSTORAGE_UserUnmarshal(ULONG *pFlags, unsigned char *Buffer, CLEANLOCALSTORAGE *pstr)
{
    ALIGN_POINTER(Buffer, 3);
    pstr->flags = *(DWORD*)Buffer;
    return Buffer + sizeof(DWORD);
}

void WINAPI CLEANLOCALSTORAGE_UserFree(ULONG *pFlags, CLEANLOCALSTORAGE *pstr)
{
    /* Nothing to do */
}

/* BSTR */

typedef struct
{
    DWORD len;          /* No. of chars not including trailing '\0' */
    DWORD byte_len;     /* len * 2 or 0xffffffff if len == 0 */
    DWORD len2;         /* == len */
} bstr_wire_t;

ULONG WINAPI BSTR_UserSize(ULONG *pFlags, ULONG Start, BSTR *pstr)
{
    TRACE("(%x,%d,%p) => %p\n", *pFlags, Start, pstr, *pstr);
    if (*pstr) TRACE("string=%s\n", debugstr_w(*pstr));
    ALIGN_LENGTH(Start, 3);
    Start += sizeof(bstr_wire_t) + ((SysStringByteLen(*pstr) + 1) & ~1);
    TRACE("returning %d\n", Start);
    return Start;
}

unsigned char * WINAPI BSTR_UserMarshal(ULONG *pFlags, unsigned char *Buffer, BSTR *pstr)
{
    bstr_wire_t *header;
    DWORD len = SysStringByteLen(*pstr);

    TRACE("(%x,%p,%p) => %p\n", *pFlags, Buffer, pstr, *pstr);
    if (*pstr) TRACE("string=%s\n", debugstr_w(*pstr));

    ALIGN_POINTER(Buffer, 3);
    header = (bstr_wire_t*)Buffer;
    header->len = header->len2 = (len + 1) / 2;
    if (*pstr)
    {
        header->byte_len = len;
        memcpy(header + 1, *pstr, header->len * 2);
    }
    else
        header->byte_len = 0xffffffff; /* special case for a null bstr */

    return Buffer + sizeof(*header) + sizeof(OLECHAR) * header->len;
}

unsigned char * WINAPI BSTR_UserUnmarshal(ULONG *pFlags, unsigned char *Buffer, BSTR *pstr)
{
    bstr_wire_t *header;
    TRACE("(%x,%p,%p) => %p\n", *pFlags, Buffer, pstr, *pstr);

    ALIGN_POINTER(Buffer, 3);
    header = (bstr_wire_t*)Buffer;
    if(header->len != header->len2)
        FIXME("len %08x != len2 %08x\n", header->len, header->len2);

    if(*pstr)
    {
        SysFreeString(*pstr);
        *pstr = NULL;
    }

    if(header->byte_len != 0xffffffff)
        *pstr = SysAllocStringByteLen((char*)(header + 1), header->byte_len);

    if (*pstr) TRACE("string=%s\n", debugstr_w(*pstr));
    return Buffer + sizeof(*header) + sizeof(OLECHAR) * header->len;
}

void WINAPI BSTR_UserFree(ULONG *pFlags, BSTR *pstr)
{
    TRACE("(%x,%p) => %p\n", *pFlags, pstr, *pstr);
    if (*pstr)
    {
        SysFreeString(*pstr);
        *pstr = NULL;
    }
}

/* VARIANT */

typedef struct
{
    DWORD clSize;
    DWORD rpcReserverd;
    USHORT vt;
    USHORT wReserved1;
    USHORT wReserved2;
    USHORT wReserved3;
    DWORD switch_is;
} variant_wire_t;

static unsigned int get_type_size(ULONG *pFlags, VARIANT *pvar)
{
    if (V_VT(pvar) & VT_ARRAY) return 4;

    switch (V_VT(pvar) & ~VT_BYREF) {
    case VT_EMPTY:
    case VT_NULL:
        return 0;
    case VT_I1:
    case VT_UI1:
        return sizeof(CHAR);
    case VT_I2:
    case VT_UI2:
        return sizeof(SHORT);
    case VT_I4:
    case VT_UI4:
        return sizeof(LONG);
    case VT_INT:
    case VT_UINT:
        return sizeof(INT);
    case VT_R4:
        return sizeof(FLOAT);
    case VT_R8:
        return sizeof(DOUBLE);
    case VT_BOOL:
        return sizeof(VARIANT_BOOL);
    case VT_ERROR:
        return sizeof(SCODE);
    case VT_DATE:
        return sizeof(DATE);
    case VT_CY:
        return sizeof(CY);
    case VT_DECIMAL:
        return sizeof(DECIMAL);
    case VT_BSTR:
        return sizeof(BSTR);
    case VT_VARIANT:
        return sizeof(VARIANT);
    case VT_UNKNOWN:
    case VT_DISPATCH:
    case VT_RECORD:
        return 0;
    default:
        FIXME("unhandled VT %d\n", V_VT(pvar));
        return 0;
    }
}

static unsigned int get_type_alignment(ULONG *pFlags, VARIANT *pvar)
{
    unsigned int size = get_type_size(pFlags, pvar);
    if(V_VT(pvar) & VT_BYREF) return 3;
    if(size == 0) return 0;
    if(size <= 4) return size - 1;
    return 7;
}

static unsigned interface_variant_size(ULONG *pFlags, REFIID riid, IUnknown *punk)
{
  ULONG size;
  HRESULT hr;
  /* find the buffer size of the marshalled dispatch interface */
  hr = CoGetMarshalSizeMax(&size, riid, punk, LOWORD(*pFlags), NULL, MSHLFLAGS_NORMAL);
  if (FAILED(hr)) {
    if (!punk)
      WARN("NULL dispatch pointer\n");
    else
      ERR("Dispatch variant buffer size calculation failed, HRESULT=0x%x\n", hr);
    return 0;
  }
  size += sizeof(ULONG); /* we have to store the buffersize in the stream */
  TRACE("wire-size extra of dispatch variant is %d\n", size);
  return size;
}

static ULONG wire_extra_user_size(ULONG *pFlags, ULONG Start, VARIANT *pvar)
{
  if (V_ISARRAY(pvar))
  {
    if (V_ISBYREF(pvar))
      return LPSAFEARRAY_UserSize(pFlags, Start, V_ARRAYREF(pvar));
    else
      return LPSAFEARRAY_UserSize(pFlags, Start, &V_ARRAY(pvar));
  }

  switch (V_VT(pvar)) {
  case VT_BSTR:
    return BSTR_UserSize(pFlags, Start, &V_BSTR(pvar));
  case VT_BSTR | VT_BYREF:
    return BSTR_UserSize(pFlags, Start, V_BSTRREF(pvar));
  case VT_VARIANT | VT_BYREF:
    return VARIANT_UserSize(pFlags, Start, V_VARIANTREF(pvar));
  case VT_UNKNOWN:
    return Start + interface_variant_size(pFlags, &IID_IUnknown, V_UNKNOWN(pvar));
  case VT_UNKNOWN | VT_BYREF:
    return Start + interface_variant_size(pFlags, &IID_IUnknown, *V_UNKNOWNREF(pvar));
  case VT_DISPATCH:
    return Start + interface_variant_size(pFlags, &IID_IDispatch, (IUnknown*)V_DISPATCH(pvar));
  case VT_DISPATCH | VT_BYREF:
    return Start + interface_variant_size(pFlags, &IID_IDispatch, (IUnknown*)*V_DISPATCHREF(pvar));
  case VT_RECORD:
    FIXME("wire-size record\n");
    return Start;
  case VT_SAFEARRAY:
  case VT_SAFEARRAY | VT_BYREF:
    FIXME("wire-size safearray: shouldn't be marshaling this\n");
    return Start;
  default:
    return Start;
  }
}

/* helper: called for VT_DISPATCH variants to marshal the IDispatch* into the buffer. returns Buffer on failure, new position otherwise */
static unsigned char* interface_variant_marshal(ULONG *pFlags, unsigned char *Buffer, REFIID riid, IUnknown *punk)
{
  IStream *working;
  HGLOBAL working_mem;
  void *working_memlocked;
  unsigned char *oldpos;
  ULONG size;
  HRESULT hr;

  TRACE("pFlags=%d, Buffer=%p, pUnk=%p\n", *pFlags, Buffer, punk);

  oldpos = Buffer;

  /* CoMarshalInterface needs a stream, whereas at this level we are operating in terms of buffers.
   * We create a stream on an HGLOBAL, so we can simply do a memcpy to move it to the buffer.
   * in rpcrt4/ndr_ole.c, a simple IStream implementation is wrapped around the buffer object,
   * but that would be overkill here, hence this implementation. We save the size because the unmarshal
   * code has no way to know how long the marshalled buffer is. */

  size = interface_variant_size(pFlags, riid, punk);

  working_mem = GlobalAlloc(0, size);
  if (!working_mem) return oldpos;

  hr = CreateStreamOnHGlobal(working_mem, TRUE, &working);
  if (hr != S_OK) {
    GlobalFree(working_mem);
    return oldpos;
  }

  hr = CoMarshalInterface(working, riid, punk, LOWORD(*pFlags), NULL, MSHLFLAGS_NORMAL);
  if (hr != S_OK) {
    IStream_Release(working); /* this also releases the hglobal */
    return oldpos;
  }

  working_memlocked = GlobalLock(working_mem);
  memcpy(Buffer, &size, sizeof(ULONG)); /* copy the buffersize */
  memcpy(Buffer + sizeof(ULONG), working_memlocked, size - sizeof(ULONG));
  GlobalUnlock(working_mem);

  IStream_Release(working);

  /* size includes the ULONG for the size written above */
  TRACE("done, size=%d\n", size);
  return Buffer + size;
}

/* helper: called for VT_DISPATCH / VT_UNKNOWN variants to unmarshal the buffer. returns Buffer on failure, new position otherwise */
static unsigned char *interface_variant_unmarshal(ULONG *pFlags, unsigned char *Buffer, REFIID riid, IUnknown **ppunk)
{
  IStream *working;
  HGLOBAL working_mem;
  void *working_memlocked;
  unsigned char *oldpos;
  ULONG size;
  HRESULT hr;

  TRACE("pFlags=%d, Buffer=%p, ppUnk=%p\n", *pFlags, Buffer, ppunk);

  oldpos = Buffer;

  /* get the buffersize */
  memcpy(&size, Buffer, sizeof(ULONG));
  TRACE("buffersize=%d\n", size);

  working_mem = GlobalAlloc(0, size);
  if (!working_mem) return oldpos;

  hr = CreateStreamOnHGlobal(working_mem, TRUE, &working);
  if (hr != S_OK) {
    GlobalFree(working_mem);
    return oldpos;
  }

  working_memlocked = GlobalLock(working_mem);

  /* now we copy the contents of the marshalling buffer to working_memlocked, unlock it, and demarshal the stream */
  memcpy(working_memlocked, Buffer + sizeof(ULONG), size);
  GlobalUnlock(working_mem);

  hr = CoUnmarshalInterface(working, riid, (void**)ppunk);
  if (hr != S_OK) {
    IStream_Release(working);
    return oldpos;
  }

  IStream_Release(working); /* this also frees the underlying hglobal */

  /* size includes the ULONG for the size written above */
  TRACE("done, processed=%d bytes\n", size);
  return Buffer + size;
}


ULONG WINAPI VARIANT_UserSize(ULONG *pFlags, ULONG Start, VARIANT *pvar)
{
    int align;
    TRACE("(%x,%d,%p)\n", *pFlags, Start, pvar);
    TRACE("vt=%04x\n", V_VT(pvar));

    ALIGN_LENGTH(Start, 7);
    Start += sizeof(variant_wire_t);
    if(V_VT(pvar) & VT_BYREF)
        Start += 4;

    align = get_type_alignment(pFlags, pvar);
    ALIGN_LENGTH(Start, align);
    if(V_VT(pvar) == (VT_VARIANT | VT_BYREF))
        Start += 4;
    else
        Start += get_type_size(pFlags, pvar);
    Start = wire_extra_user_size(pFlags, Start, pvar);

    TRACE("returning %d\n", Start);
    return Start;
}

unsigned char * WINAPI VARIANT_UserMarshal(ULONG *pFlags, unsigned char *Buffer, VARIANT *pvar)
{
    variant_wire_t *header;
    ULONG type_size;
    int align;
    unsigned char *Pos;

    TRACE("(%x,%p,%p)\n", *pFlags, Buffer, pvar);
    TRACE("vt=%04x\n", V_VT(pvar));

    ALIGN_POINTER(Buffer, 7);

    header = (variant_wire_t *)Buffer;

    header->clSize = 0; /* fixed up at the end */
    header->rpcReserverd = 0;
    header->vt = pvar->n1.n2.vt;
    header->wReserved1 = pvar->n1.n2.wReserved1;
    header->wReserved2 = pvar->n1.n2.wReserved2;
    header->wReserved3 = pvar->n1.n2.wReserved3;
    header->switch_is = pvar->n1.n2.vt;
    if(header->switch_is & VT_ARRAY)
        header->switch_is &= ~VT_TYPEMASK;

    Pos = (unsigned char*)(header + 1);
    type_size = get_type_size(pFlags, pvar);
    align = get_type_alignment(pFlags, pvar);
    ALIGN_POINTER(Pos, align);

    if(header->vt & VT_BYREF)
    {
        *(DWORD *)Pos = max(type_size, 4);
        Pos += 4;
        if((header->vt & VT_TYPEMASK) != VT_VARIANT)
        {
            memcpy(Pos, pvar->n1.n2.n3.byref, type_size);
            Pos += type_size;
        }
        else
        {
            *(DWORD*)Pos = 'U' | 's' << 8 | 'e' << 16 | 'r' << 24;
            Pos += 4;
        }
    }
    else
    {
        if((header->vt & VT_TYPEMASK) == VT_DECIMAL)
            memcpy(Pos, pvar, type_size);
        else
            memcpy(Pos, &pvar->n1.n2.n3, type_size);
        Pos += type_size;
    }

    if(header->vt & VT_ARRAY)
    {
        if(header->vt & VT_BYREF)
            Pos = LPSAFEARRAY_UserMarshal(pFlags, Pos, V_ARRAYREF(pvar));
        else
            Pos = LPSAFEARRAY_UserMarshal(pFlags, Pos, &V_ARRAY(pvar));
    }
    else
    {
        switch (header->vt)
        {
        case VT_BSTR:
            Pos = BSTR_UserMarshal(pFlags, Pos, &V_BSTR(pvar));
            break;
        case VT_BSTR | VT_BYREF:
            Pos = BSTR_UserMarshal(pFlags, Pos, V_BSTRREF(pvar));
            break;
        case VT_VARIANT | VT_BYREF:
            Pos = VARIANT_UserMarshal(pFlags, Pos, V_VARIANTREF(pvar));
            break;
        case VT_UNKNOWN:
            /* this should probably call WdtpInterfacePointer_UserMarshal in ole32.dll */
            Pos = interface_variant_marshal(pFlags, Pos, &IID_IUnknown, V_UNKNOWN(pvar));
            break;
        case VT_UNKNOWN | VT_BYREF:
            /* this should probably call WdtpInterfacePointer_UserMarshal in ole32.dll */
            Pos = interface_variant_marshal(pFlags, Pos, &IID_IUnknown, *V_UNKNOWNREF(pvar));
            break;
        case VT_DISPATCH:
            /* this should probably call WdtpInterfacePointer_UserMarshal in ole32.dll */
            Pos = interface_variant_marshal(pFlags, Pos, &IID_IDispatch, (IUnknown*)V_DISPATCH(pvar));
            break;
        case VT_DISPATCH | VT_BYREF:
            /* this should probably call WdtpInterfacePointer_UserMarshal in ole32.dll */
            Pos = interface_variant_marshal(pFlags, Pos, &IID_IDispatch, (IUnknown*)*V_DISPATCHREF(pvar));
            break;
        case VT_RECORD:
            FIXME("handle BRECORD by val\n");
            break;
        case VT_RECORD | VT_BYREF:
            FIXME("handle BRECORD by ref\n");
            break;
        }
    }
    header->clSize = ((Pos - Buffer) + 7) >> 3;
    TRACE("marshalled size=%d\n", header->clSize);
    return Pos;
}

unsigned char * WINAPI VARIANT_UserUnmarshal(ULONG *pFlags, unsigned char *Buffer, VARIANT *pvar)
{
    variant_wire_t *header;
    ULONG type_size;
    int align;
    unsigned char *Pos;

    TRACE("(%x,%p,%p)\n", *pFlags, Buffer, pvar);

    ALIGN_POINTER(Buffer, 7);
    VariantClear(pvar);

    header = (variant_wire_t *)Buffer;

    pvar->n1.n2.vt = header->vt;
    pvar->n1.n2.wReserved1 = header->wReserved1;
    pvar->n1.n2.wReserved2 = header->wReserved2;
    pvar->n1.n2.wReserved3 = header->wReserved3;

    Pos = (unsigned char*)(header + 1);
    type_size = get_type_size(pFlags, pvar);
    align = get_type_alignment(pFlags, pvar);
    ALIGN_POINTER(Pos, align);

    if(header->vt & VT_BYREF)
    {
        Pos += 4;
        pvar->n1.n2.n3.byref = CoTaskMemAlloc(type_size);
        memcpy(pvar->n1.n2.n3.byref, Pos, type_size);
        if((header->vt & VT_TYPEMASK) != VT_VARIANT)
           Pos += type_size;
        else
            Pos += 4;
    }
    else
    {
        if((header->vt & VT_TYPEMASK) == VT_DECIMAL)
            memcpy(pvar, Pos, type_size);
        else
            memcpy(&pvar->n1.n2.n3, Pos, type_size);
        Pos += type_size;
    }

    if(header->vt & VT_ARRAY)
    {
        if(header->vt & VT_BYREF)
            Pos = LPSAFEARRAY_UserUnmarshal(pFlags, Pos, V_ARRAYREF(pvar));
        else
            Pos = LPSAFEARRAY_UserUnmarshal(pFlags, Pos, &V_ARRAY(pvar));
    }
    else
    {
        switch (header->vt)
        {
        case VT_BSTR:
            V_BSTR(pvar) = NULL;
            Pos = BSTR_UserUnmarshal(pFlags, Pos, &V_BSTR(pvar));
            break;
        case VT_BSTR | VT_BYREF:
            *V_BSTRREF(pvar) = NULL;
            Pos = BSTR_UserUnmarshal(pFlags, Pos, V_BSTRREF(pvar));
            break;
        case VT_VARIANT | VT_BYREF:
            Pos = VARIANT_UserUnmarshal(pFlags, Pos, V_VARIANTREF(pvar));
            break;
        case VT_UNKNOWN:
            /* this should probably call WdtpInterfacePointer_UserUnmarshal in ole32.dll */
            Pos = interface_variant_unmarshal(pFlags, Pos, &IID_IUnknown, &V_UNKNOWN(pvar));
            break;
        case VT_UNKNOWN | VT_BYREF:
            /* this should probably call WdtpInterfacePointer_UserUnmarshal in ole32.dll */
            Pos = interface_variant_unmarshal(pFlags, Pos, &IID_IUnknown, V_UNKNOWNREF(pvar));
            break;
        case VT_DISPATCH:
            /* this should probably call WdtpInterfacePointer_UserUnmarshal in ole32.dll */
            Pos = interface_variant_unmarshal(pFlags, Pos, &IID_IDispatch, (IUnknown**)&V_DISPATCH(pvar));
            break;
        case VT_DISPATCH | VT_BYREF:
            /* this should probably call WdtpInterfacePointer_UserUnmarshal in ole32.dll */
            Pos = interface_variant_unmarshal(pFlags, Pos, &IID_IDispatch, (IUnknown**)V_DISPATCHREF(pvar));
            break;
        case VT_RECORD:
            FIXME("handle BRECORD by val\n");
            break;
        case VT_RECORD | VT_BYREF:
            FIXME("handle BRECORD by ref\n");
            break;
        }
    }
    return Pos;
}

void WINAPI VARIANT_UserFree(ULONG *pFlags, VARIANT *pvar)
{
  VARTYPE vt = V_VT(pvar);
  PVOID ref = NULL;

  TRACE("(%x,%p)\n", *pFlags, pvar);
  TRACE("vt=%04x\n", V_VT(pvar));

  if (vt & VT_BYREF) ref = pvar->n1.n2.n3.byref;

  VariantClear(pvar);
  if (!ref) return;

  if(vt & VT_ARRAY)
    LPSAFEARRAY_UserFree(pFlags, V_ARRAYREF(pvar));
  else
  {
    switch (vt)
    {
    case VT_BSTR | VT_BYREF:
      BSTR_UserFree(pFlags, V_BSTRREF(pvar));
      break;
    case VT_VARIANT | VT_BYREF:
      VARIANT_UserFree(pFlags, V_VARIANTREF(pvar));
      break;
    case VT_RECORD | VT_BYREF:
      FIXME("handle BRECORD by ref\n");
      break;
    case VT_UNKNOWN | VT_BYREF:
    case VT_DISPATCH | VT_BYREF:
      IUnknown_Release(*V_UNKNOWNREF(pvar));
      break;
    }
  }

  CoTaskMemFree(ref);
}

/* LPSAFEARRAY */

/* Get the number of cells in a SafeArray */
static ULONG SAFEARRAY_GetCellCount(const SAFEARRAY *psa)
{
    const SAFEARRAYBOUND* psab = psa->rgsabound;
    USHORT cCount = psa->cDims;
    ULONG ulNumCells = 1;

    while (cCount--)
    {
        /* This is a valid bordercase. See testcases. -Marcus */
        if (!psab->cElements)
            return 0;
        ulNumCells *= psab->cElements;
        psab++;
    }
    return ulNumCells;
}

static inline SF_TYPE SAFEARRAY_GetUnionType(SAFEARRAY *psa)
{
    VARTYPE vt;
    HRESULT hr;

    hr = SafeArrayGetVartype(psa, &vt);
    if (FAILED(hr))
    {
        if(psa->fFeatures & FADF_VARIANT) return SF_VARIANT;

        switch(psa->cbElements)
        {
        case 1: vt = VT_I1; break;
        case 2: vt = VT_I2; break;
        case 4: vt = VT_I4; break;
        case 8: vt = VT_I8; break;
        default:
            RpcRaiseException(hr);
        }
    }

    if (psa->fFeatures & FADF_HAVEIID)
        return SF_HAVEIID;

    switch (vt)
    {
    case VT_I1:
    case VT_UI1:      return SF_I1;
    case VT_BOOL:
    case VT_I2:
    case VT_UI2:      return SF_I2;
    case VT_INT:
    case VT_UINT:
    case VT_I4:
    case VT_UI4:
    case VT_R4:       return SF_I4;
    case VT_DATE:
    case VT_CY:
    case VT_R8:
    case VT_I8:
    case VT_UI8:      return SF_I8;
    case VT_INT_PTR:
    case VT_UINT_PTR: return (sizeof(UINT_PTR) == 4 ? SF_I4 : SF_I8);
    case VT_BSTR:     return SF_BSTR;
    case VT_DISPATCH: return SF_DISPATCH;
    case VT_VARIANT:  return SF_VARIANT;
    case VT_UNKNOWN:  return SF_UNKNOWN;
    /* Note: Return a non-zero size to indicate vt is valid. The actual size
     * of a UDT is taken from the result of IRecordInfo_GetSize().
     */
    case VT_RECORD:   return SF_RECORD;
    default:          return SF_ERROR;
    }
}

ULONG WINAPI LPSAFEARRAY_UserSize(ULONG *pFlags, ULONG StartingSize, LPSAFEARRAY *ppsa)
{
    ULONG size = StartingSize;

    TRACE("("); dump_user_flags(pFlags); TRACE(", %d, %p\n", StartingSize, *ppsa);

    ALIGN_LENGTH(size, 3);
    size += sizeof(ULONG_PTR);
    if (*ppsa)
    {
        SAFEARRAY *psa = *ppsa;
        ULONG ulCellCount = SAFEARRAY_GetCellCount(psa);
        SF_TYPE sftype;
        HRESULT hr;

        size += sizeof(ULONG);
        size += FIELD_OFFSET(struct _wireSAFEARRAY, uArrayStructs);

        sftype = SAFEARRAY_GetUnionType(psa);
        size += sizeof(ULONG);

        size += sizeof(ULONG);
        size += sizeof(ULONG_PTR);
        if (sftype == SF_HAVEIID)
            size += sizeof(IID);

        size += sizeof(psa->rgsabound[0]) * psa->cDims;

        size += sizeof(ULONG);

        switch (sftype)
        {
            case SF_BSTR:
            {
                BSTR* lpBstr;

                for (lpBstr = (BSTR*)psa->pvData; ulCellCount; ulCellCount--, lpBstr++)
                    size = BSTR_UserSize(pFlags, size, lpBstr);

                break;
            }
            case SF_DISPATCH:
            case SF_UNKNOWN:
            case SF_HAVEIID:
                FIXME("size interfaces\n");
                break;
            case SF_VARIANT:
            {
                VARIANT* lpVariant;

                for (lpVariant = (VARIANT*)psa->pvData; ulCellCount; ulCellCount--, lpVariant++)
                    size = VARIANT_UserSize(pFlags, size, lpVariant);

                break;
            }
            case SF_RECORD:
            {
                IRecordInfo* pRecInfo = NULL;

                hr = SafeArrayGetRecordInfo(psa, &pRecInfo);
                if (FAILED(hr))
                    RpcRaiseException(hr);

                if (pRecInfo)
                {
                    FIXME("size record info %p\n", pRecInfo);

                    IRecordInfo_Release(pRecInfo);
                }
                break;
            }
            case SF_I8:
                ALIGN_LENGTH(size, 7);
                /* fallthrough */
            case SF_I1:
            case SF_I2:
            case SF_I4:
                size += ulCellCount * psa->cbElements;
                break;
            default:
                break;
        }

    }

    return size;
}

unsigned char * WINAPI LPSAFEARRAY_UserMarshal(ULONG *pFlags, unsigned char *Buffer, LPSAFEARRAY *ppsa)
{
    HRESULT hr;

    TRACE("("); dump_user_flags(pFlags); TRACE(", %p, &%p\n", Buffer, *ppsa);

    ALIGN_POINTER(Buffer, 3);
    *(ULONG_PTR *)Buffer = *ppsa ? TRUE : FALSE;
    Buffer += sizeof(ULONG_PTR);
    if (*ppsa)
    {
        VARTYPE vt;
        SAFEARRAY *psa = *ppsa;
        ULONG ulCellCount = SAFEARRAY_GetCellCount(psa);
        wireSAFEARRAY wiresa;
        SF_TYPE sftype;
        GUID guid;

        *(ULONG *)Buffer = psa->cDims;
        Buffer += sizeof(ULONG);
        wiresa = (wireSAFEARRAY)Buffer;
        wiresa->cDims = psa->cDims;
        wiresa->fFeatures = psa->fFeatures;
        wiresa->cbElements = psa->cbElements;

        hr = SafeArrayGetVartype(psa, &vt);
        if (FAILED(hr)) vt = 0;

        wiresa->cLocks = (USHORT)psa->cLocks | (vt << 16);

        Buffer += FIELD_OFFSET(struct _wireSAFEARRAY, uArrayStructs);

        sftype = SAFEARRAY_GetUnionType(psa);
        *(ULONG *)Buffer = sftype;
        Buffer += sizeof(ULONG);

        *(ULONG *)Buffer = ulCellCount;
        Buffer += sizeof(ULONG);
        *(ULONG_PTR *)Buffer = (ULONG_PTR)psa->pvData;
        Buffer += sizeof(ULONG_PTR);
        if (sftype == SF_HAVEIID)
        {
            SafeArrayGetIID(psa, &guid);
            memcpy(Buffer, &guid, sizeof(guid));
            Buffer += sizeof(guid);
        }

        memcpy(Buffer, psa->rgsabound, sizeof(psa->rgsabound[0]) * psa->cDims);
        Buffer += sizeof(psa->rgsabound[0]) * psa->cDims;

        *(ULONG *)Buffer = ulCellCount;
        Buffer += sizeof(ULONG);

        if (psa->pvData)
        {
            switch (sftype)
            {
                case SF_BSTR:
                {
                    BSTR* lpBstr;

                    for (lpBstr = (BSTR*)psa->pvData; ulCellCount; ulCellCount--, lpBstr++)
                        Buffer = BSTR_UserMarshal(pFlags, Buffer, lpBstr);

                    break;
                }
                case SF_DISPATCH:
                case SF_UNKNOWN:
                case SF_HAVEIID:
                    FIXME("marshal interfaces\n");
                    break;
                case SF_VARIANT:
                {
                    VARIANT* lpVariant;

                    for (lpVariant = (VARIANT*)psa->pvData; ulCellCount; ulCellCount--, lpVariant++)
                        Buffer = VARIANT_UserMarshal(pFlags, Buffer, lpVariant);

                    break;
                }
                case SF_RECORD:
                {
                    IRecordInfo* pRecInfo = NULL;

                    hr = SafeArrayGetRecordInfo(psa, &pRecInfo);
                    if (FAILED(hr))
                        RpcRaiseException(hr);

                    if (pRecInfo)
                    {
                        FIXME("write record info %p\n", pRecInfo);

                        IRecordInfo_Release(pRecInfo);
                    }
                    break;
                }

                case SF_I8:
                    ALIGN_POINTER(Buffer, 7);
                    /* fallthrough */
                case SF_I1:
                case SF_I2:
                case SF_I4:
                    /* Just copy the data over */
                    memcpy(Buffer, psa->pvData, ulCellCount * psa->cbElements);
                    Buffer += ulCellCount * psa->cbElements;
                    break;
                default:
                    break;
            }
        }

    }
    return Buffer;
}

#define FADF_AUTOSETFLAGS (FADF_HAVEIID | FADF_RECORD | FADF_HAVEVARTYPE | \
                           FADF_BSTR | FADF_UNKNOWN | FADF_DISPATCH | \
                           FADF_VARIANT | FADF_CREATEVECTOR)

unsigned char * WINAPI LPSAFEARRAY_UserUnmarshal(ULONG *pFlags, unsigned char *Buffer, LPSAFEARRAY *ppsa)
{
    ULONG_PTR ptr;
    wireSAFEARRAY wiresa;
    ULONG cDims;
    HRESULT hr;
    SF_TYPE sftype;
    ULONG cell_count;
    GUID guid;
    VARTYPE vt;
    SAFEARRAYBOUND *wiresab;

    TRACE("("); dump_user_flags(pFlags); TRACE(", %p, %p\n", Buffer, ppsa);

    ALIGN_POINTER(Buffer, 3);
    ptr = *(ULONG_PTR *)Buffer;
    Buffer += sizeof(ULONG_PTR);

    if (!ptr)
    {
        *ppsa = NULL;

        TRACE("NULL safe array unmarshaled\n");

        return Buffer;
    }

    cDims = *(ULONG *)Buffer;
    Buffer += sizeof(ULONG);

    wiresa = (wireSAFEARRAY)Buffer;
    Buffer += FIELD_OFFSET(struct _wireSAFEARRAY, uArrayStructs);

    if (cDims != wiresa->cDims)
        RpcRaiseException(RPC_S_INVALID_BOUND);

    /* FIXME: there should be a limit on how large cDims can be */

    vt = HIWORD(wiresa->cLocks);

    sftype = *(ULONG *)Buffer;
    Buffer += sizeof(ULONG);

    cell_count = *(ULONG *)Buffer;
    Buffer += sizeof(ULONG);
    ptr = *(ULONG_PTR *)Buffer;
    Buffer += sizeof(ULONG_PTR);
    if (sftype == SF_HAVEIID)
    {
        memcpy(&guid, Buffer, sizeof(guid));
        Buffer += sizeof(guid);
    }

    wiresab = (SAFEARRAYBOUND *)Buffer;
    Buffer += sizeof(wiresab[0]) * wiresa->cDims;

    if(vt)
        *ppsa = SafeArrayCreateEx(vt, wiresa->cDims, wiresab, NULL);
    else
    {
        SafeArrayAllocDescriptor(wiresa->cDims, ppsa);
        if(*ppsa)
            memcpy((*ppsa)->rgsabound, wiresab, sizeof(SAFEARRAYBOUND) * wiresa->cDims);
    }
    if (!*ppsa)
        RpcRaiseException(E_OUTOFMEMORY);

    /* be careful about which flags we set since they could be a security
     * risk */
    (*ppsa)->fFeatures &= FADF_AUTOSETFLAGS;
    (*ppsa)->fFeatures |= (wiresa->fFeatures & ~(FADF_AUTOSETFLAGS));
    /* FIXME: there should be a limit on how large wiresa->cbElements can be */
    (*ppsa)->cbElements = wiresa->cbElements;
    (*ppsa)->cLocks = LOWORD(wiresa->cLocks);

    hr = SafeArrayAllocData(*ppsa);
    if (FAILED(hr))
        RpcRaiseException(hr);

    if ((*(ULONG *)Buffer != cell_count) || (SAFEARRAY_GetCellCount(*ppsa) != cell_count))
        RpcRaiseException(RPC_S_INVALID_BOUND);
    Buffer += sizeof(ULONG);

    if (ptr)
    {
        switch (sftype)
        {
            case SF_BSTR:
            {
                BSTR* lpBstr;

                for (lpBstr = (BSTR*)(*ppsa)->pvData; cell_count; cell_count--, lpBstr++)
                    Buffer = BSTR_UserUnmarshal(pFlags, Buffer, lpBstr);

                break;
            }
            case SF_DISPATCH:
            case SF_UNKNOWN:
            case SF_HAVEIID:
                FIXME("marshal interfaces\n");
                break;
            case SF_VARIANT:
            {
                VARIANT* lpVariant;

                for (lpVariant = (VARIANT*)(*ppsa)->pvData; cell_count; cell_count--, lpVariant++)
                    Buffer = VARIANT_UserUnmarshal(pFlags, Buffer, lpVariant);

                break;
            }
            case SF_RECORD:
            {
                FIXME("set record info\n");

                break;
            }

            case SF_I8:
                ALIGN_POINTER(Buffer, 7);
                /* fallthrough */
            case SF_I1:
            case SF_I2:
            case SF_I4:
                /* Just copy the data over */
                memcpy((*ppsa)->pvData, Buffer, cell_count * (*ppsa)->cbElements);
                Buffer += cell_count * (*ppsa)->cbElements;
                break;
            default:
                break;
        }
    }

    TRACE("safe array unmarshaled: %p\n", *ppsa);

    return Buffer;
}

void WINAPI LPSAFEARRAY_UserFree(ULONG *pFlags, LPSAFEARRAY *ppsa)
{
    TRACE("("); dump_user_flags(pFlags); TRACE(", &%p\n", *ppsa);

    SafeArrayDestroy(*ppsa);
}

/* IDispatch */
/* exactly how Invoke is marshalled is not very clear to me yet,
 * but the way I've done it seems to work for me */

HRESULT CALLBACK IDispatch_Invoke_Proxy(
    IDispatch* This,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS* pDispParams,
    VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo,
    UINT* puArgErr)
{
  HRESULT hr;
  VARIANT VarResult;
  UINT* rgVarRefIdx = NULL;
  VARIANTARG* rgVarRef = NULL;
  UINT u, cVarRef;
  UINT uArgErr;
  EXCEPINFO ExcepInfo;

  TRACE("(%p)->(%d,%s,%x,%x,%p,%p,%p,%p)\n", This,
        dispIdMember, debugstr_guid(riid),
        lcid, wFlags, pDispParams, pVarResult,
        pExcepInfo, puArgErr);

  /* [out] args can't be null, use dummy vars if needed */
  if (!pVarResult) pVarResult = &VarResult;
  if (!puArgErr) puArgErr = &uArgErr;
  if (!pExcepInfo) pExcepInfo = &ExcepInfo;

  /* count by-ref args */
  for (cVarRef=0,u=0; u<pDispParams->cArgs; u++) {
    VARIANTARG* arg = &pDispParams->rgvarg[u];
    if (V_ISBYREF(arg)) {
      cVarRef++;
    }
  }
  if (cVarRef) {
    rgVarRefIdx = CoTaskMemAlloc(sizeof(UINT)*cVarRef);
    rgVarRef = CoTaskMemAlloc(sizeof(VARIANTARG)*cVarRef);
    /* make list of by-ref args */
    for (cVarRef=0,u=0; u<pDispParams->cArgs; u++) {
      VARIANTARG* arg = &pDispParams->rgvarg[u];
      if (V_ISBYREF(arg)) {
	rgVarRefIdx[cVarRef] = u;
	VariantInit(&rgVarRef[cVarRef]);
	cVarRef++;
      }
    }
  } else {
    /* [out] args still can't be null,
     * but we can point these anywhere in this case,
     * since they won't be written to when cVarRef is 0 */
    rgVarRefIdx = puArgErr;
    rgVarRef = pVarResult;
  }
  TRACE("passed by ref: %d args\n", cVarRef);
  hr = IDispatch_RemoteInvoke_Proxy(This,
				    dispIdMember,
				    riid,
				    lcid,
				    wFlags,
				    pDispParams,
				    pVarResult,
				    pExcepInfo,
				    puArgErr,
				    cVarRef,
				    rgVarRefIdx,
				    rgVarRef);
  if (cVarRef) {
    for (u=0; u<cVarRef; u++) {
      unsigned i = rgVarRefIdx[u];
      VariantCopy(&pDispParams->rgvarg[i],
		  &rgVarRef[u]);
      VariantClear(&rgVarRef[u]);
    }
    CoTaskMemFree(rgVarRef);
    CoTaskMemFree(rgVarRefIdx);
  }

  if(pExcepInfo == &ExcepInfo)
  {
    SysFreeString(pExcepInfo->bstrSource);
    SysFreeString(pExcepInfo->bstrDescription);
    SysFreeString(pExcepInfo->bstrHelpFile);
  }
  return hr;
}

HRESULT __RPC_STUB IDispatch_Invoke_Stub(
    IDispatch* This,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    DWORD dwFlags,
    DISPPARAMS* pDispParams,
    VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo,
    UINT* pArgErr,
    UINT cVarRef,
    UINT* rgVarRefIdx,
    VARIANTARG* rgVarRef)
{
  HRESULT hr = S_OK;
  VARIANTARG *rgvarg, *arg;
  UINT u;

  /* initialize out parameters, so that they can be marshalled
   * in case the real Invoke doesn't initialize them */
  VariantInit(pVarResult);
  memset(pExcepInfo, 0, sizeof(*pExcepInfo));
  *pArgErr = 0;

  /* let the real Invoke operate on a copy of the in parameters,
   * so we don't risk losing pointers to allocated memory */
  rgvarg = pDispParams->rgvarg;
  arg = CoTaskMemAlloc(sizeof(VARIANTARG)*pDispParams->cArgs);
  if (!arg) return E_OUTOFMEMORY;

  /* init all args so we can call VariantClear on all the args if the copy
   * below fails */
  for (u = 0; u < pDispParams->cArgs; u++)
    VariantInit(&arg[u]);

  for (u = 0; u < pDispParams->cArgs; u++) {
    hr = VariantCopy(&arg[u], &rgvarg[u]);
    if (FAILED(hr))
        break;
  }

  if (SUCCEEDED(hr)) {
    pDispParams->rgvarg = arg;

    hr = IDispatch_Invoke(This,
			  dispIdMember,
			  riid,
			  lcid,
			  dwFlags,
			  pDispParams,
			  pVarResult,
			  pExcepInfo,
			  pArgErr);

    /* copy ref args to out list */
    for (u=0; u<cVarRef; u++) {
      unsigned i = rgVarRefIdx[u];
      VariantInit(&rgVarRef[u]);
      VariantCopy(&rgVarRef[u], &arg[i]);
      /* clear original if equal, to avoid double-free */
      if (V_BYREF(&rgVarRef[u]) == V_BYREF(&rgvarg[i]))
        VariantClear(&rgvarg[i]);
    }
  }

  /* clear the duplicate argument list */
  for (u=0; u<pDispParams->cArgs; u++)
    VariantClear(&arg[u]);

  pDispParams->rgvarg = rgvarg;
  CoTaskMemFree(arg);

  return hr;
}

/* IEnumVARIANT */

HRESULT CALLBACK IEnumVARIANT_Next_Proxy(
    IEnumVARIANT* This,
    ULONG celt,
    VARIANT* rgVar,
    ULONG* pCeltFetched)
{
  ULONG fetched;
  if (!pCeltFetched)
    pCeltFetched = &fetched;
  return IEnumVARIANT_RemoteNext_Proxy(This,
				       celt,
				       rgVar,
				       pCeltFetched);
}

HRESULT __RPC_STUB IEnumVARIANT_Next_Stub(
    IEnumVARIANT* This,
    ULONG celt,
    VARIANT* rgVar,
    ULONG* pCeltFetched)
{
  HRESULT hr;
  *pCeltFetched = 0;
  hr = IEnumVARIANT_Next(This,
			 celt,
			 rgVar,
			 pCeltFetched);
  if (hr == S_OK) *pCeltFetched = celt;
  return hr;
}

/* TypeInfo related freers */

static void free_embedded_typedesc(TYPEDESC *tdesc);
static void free_embedded_arraydesc(ARRAYDESC *adesc)
{
    switch(adesc->tdescElem.vt)
    {
    case VT_PTR:
    case VT_SAFEARRAY:
        free_embedded_typedesc(adesc->tdescElem.u.lptdesc);
        CoTaskMemFree(adesc->tdescElem.u.lptdesc);
        break;
    case VT_CARRAY:
        free_embedded_arraydesc(adesc->tdescElem.u.lpadesc);
        CoTaskMemFree(adesc->tdescElem.u.lpadesc);
        break;
    }
}

static void free_embedded_typedesc(TYPEDESC *tdesc)
{
    switch(tdesc->vt)
    {
    case VT_PTR:
    case VT_SAFEARRAY:
        free_embedded_typedesc(tdesc->u.lptdesc);
        CoTaskMemFree(tdesc->u.lptdesc);
        break;
    case VT_CARRAY:
        free_embedded_arraydesc(tdesc->u.lpadesc);
        CoTaskMemFree(tdesc->u.lpadesc);
        break;
    }
}

static void free_embedded_elemdesc(ELEMDESC *edesc)
{
    free_embedded_typedesc(&edesc->tdesc);
    if(edesc->u.paramdesc.wParamFlags & PARAMFLAG_FHASDEFAULT)
        CoTaskMemFree(edesc->u.paramdesc.pparamdescex);
}

/* ITypeComp */

HRESULT CALLBACK ITypeComp_Bind_Proxy(
    ITypeComp* This,
    LPOLESTR szName,
    ULONG lHashVal,
    WORD wFlags,
    ITypeInfo** ppTInfo,
    DESCKIND* pDescKind,
    BINDPTR* pBindPtr)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT __RPC_STUB ITypeComp_Bind_Stub(
    ITypeComp* This,
    LPOLESTR szName,
    ULONG lHashVal,
    WORD wFlags,
    ITypeInfo** ppTInfo,
    DESCKIND* pDescKind,
    LPFUNCDESC* ppFuncDesc,
    LPVARDESC* ppVarDesc,
    ITypeComp** ppTypeComp,
    CLEANLOCALSTORAGE* pDummy)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT CALLBACK ITypeComp_BindType_Proxy(
    ITypeComp* This,
    LPOLESTR szName,
    ULONG lHashVal,
    ITypeInfo** ppTInfo,
    ITypeComp** ppTComp)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT __RPC_STUB ITypeComp_BindType_Stub(
    ITypeComp* This,
    LPOLESTR szName,
    ULONG lHashVal,
    ITypeInfo** ppTInfo)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

/* ITypeInfo */

HRESULT CALLBACK ITypeInfo_GetTypeAttr_Proxy(
    ITypeInfo* This,
    TYPEATTR** ppTypeAttr)

{
    CLEANLOCALSTORAGE stg;
    TRACE("(%p, %p)\n", This, ppTypeAttr);

    stg.flags = 0;
    stg.pStorage = NULL;
    stg.pInterface = NULL;

    return ITypeInfo_RemoteGetTypeAttr_Proxy(This, ppTypeAttr, &stg);
}

HRESULT __RPC_STUB ITypeInfo_GetTypeAttr_Stub(
    ITypeInfo* This,
    LPTYPEATTR* ppTypeAttr,
    CLEANLOCALSTORAGE* pDummy)
{
    HRESULT hr;
    TRACE("(%p, %p)\n", This, ppTypeAttr);

    hr = ITypeInfo_GetTypeAttr(This, ppTypeAttr);
    if(hr != S_OK)
        return hr;

    pDummy->flags = CLS_TYPEATTR;
    ITypeInfo_AddRef(This);
    pDummy->pInterface = (IUnknown*)This;
    pDummy->pStorage = ppTypeAttr;
    return hr;
}

HRESULT CALLBACK ITypeInfo_GetFuncDesc_Proxy(
    ITypeInfo* This,
    UINT index,
    FUNCDESC** ppFuncDesc)
{
    CLEANLOCALSTORAGE stg;
    TRACE("(%p, %d, %p)\n", This, index, ppFuncDesc);

    stg.flags = 0;
    stg.pStorage = NULL;
    stg.pInterface = NULL;

    return ITypeInfo_RemoteGetFuncDesc_Proxy(This, index, ppFuncDesc, &stg);
}

HRESULT __RPC_STUB ITypeInfo_GetFuncDesc_Stub(
    ITypeInfo* This,
    UINT index,
    LPFUNCDESC* ppFuncDesc,
    CLEANLOCALSTORAGE* pDummy)
{
    HRESULT hr;
    TRACE("(%p, %d, %p)\n", This, index, ppFuncDesc);

    hr = ITypeInfo_GetFuncDesc(This, index, ppFuncDesc);
    if(hr != S_OK)
        return hr;

    pDummy->flags = CLS_FUNCDESC;
    ITypeInfo_AddRef(This);
    pDummy->pInterface = (IUnknown*)This;
    pDummy->pStorage = ppFuncDesc;
    return hr;
}

HRESULT CALLBACK ITypeInfo_GetVarDesc_Proxy(
    ITypeInfo* This,
    UINT index,
    VARDESC** ppVarDesc)
{
    CLEANLOCALSTORAGE stg;
    TRACE("(%p, %d, %p)\n", This, index, ppVarDesc);

    stg.flags = 0;
    stg.pStorage = NULL;
    stg.pInterface = NULL;

    return ITypeInfo_RemoteGetVarDesc_Proxy(This, index, ppVarDesc, &stg);
}

HRESULT __RPC_STUB ITypeInfo_GetVarDesc_Stub(
    ITypeInfo* This,
    UINT index,
    LPVARDESC* ppVarDesc,
    CLEANLOCALSTORAGE* pDummy)
{
    HRESULT hr;
    TRACE("(%p, %d, %p)\n", This, index, ppVarDesc);

    hr = ITypeInfo_GetVarDesc(This, index, ppVarDesc);
    if(hr != S_OK)
        return hr;

    pDummy->flags = CLS_VARDESC;
    ITypeInfo_AddRef(This);
    pDummy->pInterface = (IUnknown*)This;
    pDummy->pStorage = ppVarDesc;
    return hr;
}

HRESULT CALLBACK ITypeInfo_GetNames_Proxy(
    ITypeInfo* This,
    MEMBERID memid,
    BSTR* rgBstrNames,
    UINT cMaxNames,
    UINT* pcNames)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT __RPC_STUB ITypeInfo_GetNames_Stub(
    ITypeInfo* This,
    MEMBERID memid,
    BSTR* rgBstrNames,
    UINT cMaxNames,
    UINT* pcNames)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT CALLBACK ITypeInfo_GetIDsOfNames_Proxy(
    ITypeInfo* This,
    LPOLESTR* rgszNames,
    UINT cNames,
    MEMBERID* pMemId)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT __RPC_STUB ITypeInfo_GetIDsOfNames_Stub(
    ITypeInfo* This)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT CALLBACK ITypeInfo_Invoke_Proxy(
    ITypeInfo* This,
    PVOID pvInstance,
    MEMBERID memid,
    WORD wFlags,
    DISPPARAMS* pDispParams,
    VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo,
    UINT* puArgErr)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT __RPC_STUB ITypeInfo_Invoke_Stub(
    ITypeInfo* This)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT CALLBACK ITypeInfo_GetDocumentation_Proxy(
    ITypeInfo* This,
    MEMBERID memid,
    BSTR* pBstrName,
    BSTR* pBstrDocString,
    DWORD* pdwHelpContext,
    BSTR* pBstrHelpFile)
{
    DWORD help_context;
    BSTR name, doc_string, help_file;
    HRESULT hr;
    TRACE("(%p, %08x, %p, %p, %p, %p)\n", This, memid, pBstrName, pBstrDocString, pdwHelpContext, pBstrHelpFile);

    /* FIXME: presumably refPtrFlags is supposed to be a bitmask of which ptrs we actually want? */
    hr = ITypeInfo_RemoteGetDocumentation_Proxy(This, memid, 0, &name, &doc_string, &help_context, &help_file);
    if(SUCCEEDED(hr))
    {
        if(pBstrName) *pBstrName = name;
        else SysFreeString(name);

        if(pBstrDocString) *pBstrDocString = doc_string;
        else SysFreeString(doc_string);

        if(pBstrHelpFile) *pBstrHelpFile = help_file;
        else SysFreeString(help_file);

        if(pdwHelpContext) *pdwHelpContext = help_context;
    }
    return hr;
}

HRESULT __RPC_STUB ITypeInfo_GetDocumentation_Stub(
    ITypeInfo* This,
    MEMBERID memid,
    DWORD refPtrFlags,
    BSTR* pBstrName,
    BSTR* pBstrDocString,
    DWORD* pdwHelpContext,
    BSTR* pBstrHelpFile)
{
    TRACE("(%p, %08x, %08x, %p, %p, %p, %p)\n", This, memid, refPtrFlags, pBstrName, pBstrDocString,
          pdwHelpContext, pBstrHelpFile);
    return ITypeInfo_GetDocumentation(This, memid, pBstrName, pBstrDocString, pdwHelpContext, pBstrHelpFile);
}

HRESULT CALLBACK ITypeInfo_GetDllEntry_Proxy(
    ITypeInfo* This,
    MEMBERID memid,
    INVOKEKIND invKind,
    BSTR* pBstrDllName,
    BSTR* pBstrName,
    WORD* pwOrdinal)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT __RPC_STUB ITypeInfo_GetDllEntry_Stub(
    ITypeInfo* This,
    MEMBERID memid,
    INVOKEKIND invKind,
    DWORD refPtrFlags,
    BSTR* pBstrDllName,
    BSTR* pBstrName,
    WORD* pwOrdinal)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT CALLBACK ITypeInfo_AddressOfMember_Proxy(
    ITypeInfo* This,
    MEMBERID memid,
    INVOKEKIND invKind,
    PVOID* ppv)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT __RPC_STUB ITypeInfo_AddressOfMember_Stub(
    ITypeInfo* This)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT CALLBACK ITypeInfo_CreateInstance_Proxy(
    ITypeInfo* This,
    IUnknown* pUnkOuter,
    REFIID riid,
    PVOID* ppvObj)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT __RPC_STUB ITypeInfo_CreateInstance_Stub(
    ITypeInfo* This,
    REFIID riid,
    IUnknown** ppvObj)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT CALLBACK ITypeInfo_GetContainingTypeLib_Proxy(
    ITypeInfo* This,
    ITypeLib** ppTLib,
    UINT* pIndex)
{
    ITypeLib *pTL;
    UINT index;
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", This, ppTLib, pIndex );

    hr = ITypeInfo_RemoteGetContainingTypeLib_Proxy(This, &pTL, &index);
    if(SUCCEEDED(hr))
    {
        if(pIndex)
            *pIndex = index;

        if(ppTLib)
            *ppTLib = pTL;
        else
            ITypeLib_Release(pTL);
    }
    return hr;
}

HRESULT __RPC_STUB ITypeInfo_GetContainingTypeLib_Stub(
    ITypeInfo* This,
    ITypeLib** ppTLib,
    UINT* pIndex)
{
    TRACE("(%p, %p, %p)\n", This, ppTLib, pIndex );
    return ITypeInfo_GetContainingTypeLib(This, ppTLib, pIndex);
}

void CALLBACK ITypeInfo_ReleaseTypeAttr_Proxy(
    ITypeInfo* This,
    TYPEATTR* pTypeAttr)
{
    TRACE("(%p, %p)\n", This, pTypeAttr);
    free_embedded_typedesc(&pTypeAttr->tdescAlias);
    CoTaskMemFree(pTypeAttr);
}

HRESULT __RPC_STUB ITypeInfo_ReleaseTypeAttr_Stub(
    ITypeInfo* This)
{
    TRACE("nothing to do\n");
    return S_OK;
}

void CALLBACK ITypeInfo_ReleaseFuncDesc_Proxy(
    ITypeInfo* This,
    FUNCDESC* pFuncDesc)
{
    SHORT param;
    TRACE("(%p, %p)\n", This, pFuncDesc);

    for(param = 0; param < pFuncDesc->cParams; param++)
        free_embedded_elemdesc(pFuncDesc->lprgelemdescParam + param);
    if(param)
        CoTaskMemFree(pFuncDesc->lprgelemdescParam);

    free_embedded_elemdesc(&pFuncDesc->elemdescFunc);

    if(pFuncDesc->cScodes != 0 && pFuncDesc->cScodes != -1)
        CoTaskMemFree(pFuncDesc->lprgscode);

    CoTaskMemFree(pFuncDesc);
}

HRESULT __RPC_STUB ITypeInfo_ReleaseFuncDesc_Stub(
    ITypeInfo* This)
{
    TRACE("nothing to do\n");
    return S_OK;
}

void CALLBACK ITypeInfo_ReleaseVarDesc_Proxy(
    ITypeInfo* This,
    VARDESC* pVarDesc)
{
    TRACE("(%p, %p)\n", This, pVarDesc);

    CoTaskMemFree(pVarDesc->lpstrSchema);

    if(pVarDesc->varkind == VAR_CONST)
        CoTaskMemFree(pVarDesc->u.lpvarValue);

    free_embedded_elemdesc(&pVarDesc->elemdescVar);
    CoTaskMemFree(pVarDesc);
}

HRESULT __RPC_STUB ITypeInfo_ReleaseVarDesc_Stub(
    ITypeInfo* This)
{
    TRACE("nothing to do\n");
    return S_OK;
}


/* ITypeInfo2 */

HRESULT CALLBACK ITypeInfo2_GetDocumentation2_Proxy(
    ITypeInfo2* This,
    MEMBERID memid,
    LCID lcid,
    BSTR* pbstrHelpString,
    DWORD* pdwHelpStringContext,
    BSTR* pbstrHelpStringDll)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT __RPC_STUB ITypeInfo2_GetDocumentation2_Stub(
    ITypeInfo2* This,
    MEMBERID memid,
    LCID lcid,
    DWORD refPtrFlags,
    BSTR* pbstrHelpString,
    DWORD* pdwHelpStringContext,
    BSTR* pbstrHelpStringDll)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

/* ITypeLib */

UINT CALLBACK ITypeLib_GetTypeInfoCount_Proxy(
    ITypeLib* This)
{
    UINT count = 0;
    TRACE("(%p)\n", This);

    ITypeLib_RemoteGetTypeInfoCount_Proxy(This, &count);

    return count;
}

HRESULT __RPC_STUB ITypeLib_GetTypeInfoCount_Stub(
    ITypeLib* This,
    UINT* pcTInfo)
{
    TRACE("(%p, %p)\n", This, pcTInfo);
    *pcTInfo = ITypeLib_GetTypeInfoCount(This);
    return S_OK;
}

HRESULT CALLBACK ITypeLib_GetLibAttr_Proxy(
    ITypeLib* This,
    TLIBATTR** ppTLibAttr)
{
    CLEANLOCALSTORAGE stg;
    TRACE("(%p, %p)\n", This, ppTLibAttr);

    stg.flags = 0;
    stg.pStorage = NULL;
    stg.pInterface = NULL;

    return ITypeLib_RemoteGetLibAttr_Proxy(This, ppTLibAttr, &stg);
}

HRESULT __RPC_STUB ITypeLib_GetLibAttr_Stub(
    ITypeLib* This,
    LPTLIBATTR* ppTLibAttr,
    CLEANLOCALSTORAGE* pDummy)
{
    HRESULT hr;
    TRACE("(%p, %p)\n", This, ppTLibAttr);

    hr = ITypeLib_GetLibAttr(This, ppTLibAttr);
    if(hr != S_OK)
        return hr;

    pDummy->flags = CLS_LIBATTR;
    ITypeLib_AddRef(This);
    pDummy->pInterface = (IUnknown*)This;
    pDummy->pStorage = ppTLibAttr;
    return hr;
}

HRESULT CALLBACK ITypeLib_GetDocumentation_Proxy(
    ITypeLib* This,
    INT index,
    BSTR* pBstrName,
    BSTR* pBstrDocString,
    DWORD* pdwHelpContext,
    BSTR* pBstrHelpFile)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT __RPC_STUB ITypeLib_GetDocumentation_Stub(
    ITypeLib* This,
    INT index,
    DWORD refPtrFlags,
    BSTR* pBstrName,
    BSTR* pBstrDocString,
    DWORD* pdwHelpContext,
    BSTR* pBstrHelpFile)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT CALLBACK ITypeLib_IsName_Proxy(
    ITypeLib* This,
    LPOLESTR szNameBuf,
    ULONG lHashVal,
    BOOL* pfName)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT __RPC_STUB ITypeLib_IsName_Stub(
    ITypeLib* This,
    LPOLESTR szNameBuf,
    ULONG lHashVal,
    BOOL* pfName,
    BSTR* pBstrLibName)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT CALLBACK ITypeLib_FindName_Proxy(
    ITypeLib* This,
    LPOLESTR szNameBuf,
    ULONG lHashVal,
    ITypeInfo** ppTInfo,
    MEMBERID* rgMemId,
    USHORT* pcFound)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT __RPC_STUB ITypeLib_FindName_Stub(
    ITypeLib* This,
    LPOLESTR szNameBuf,
    ULONG lHashVal,
    ITypeInfo** ppTInfo,
    MEMBERID* rgMemId,
    USHORT* pcFound,
    BSTR* pBstrLibName)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

void CALLBACK ITypeLib_ReleaseTLibAttr_Proxy(
    ITypeLib* This,
    TLIBATTR* pTLibAttr)
{
    TRACE("(%p, %p)\n", This, pTLibAttr);
    CoTaskMemFree(pTLibAttr);
}

HRESULT __RPC_STUB ITypeLib_ReleaseTLibAttr_Stub(
    ITypeLib* This)
{
    TRACE("nothing to do\n");
    return S_OK;
}


/* ITypeLib2 */

HRESULT CALLBACK ITypeLib2_GetLibStatistics_Proxy(
    ITypeLib2* This,
    ULONG* pcUniqueNames,
    ULONG* pcchUniqueNames)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT __RPC_STUB ITypeLib2_GetLibStatistics_Stub(
    ITypeLib2* This,
    ULONG* pcUniqueNames,
    ULONG* pcchUniqueNames)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT CALLBACK ITypeLib2_GetDocumentation2_Proxy(
    ITypeLib2* This,
    INT index,
    LCID lcid,
    BSTR* pbstrHelpString,
    DWORD* pdwHelpStringContext,
    BSTR* pbstrHelpStringDll)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT __RPC_STUB ITypeLib2_GetDocumentation2_Stub(
    ITypeLib2* This,
    INT index,
    LCID lcid,
    DWORD refPtrFlags,
    BSTR* pbstrHelpString,
    DWORD* pdwHelpStringContext,
    BSTR* pbstrHelpStringDll)
{
  FIXME("not implemented\n");
  return E_FAIL;
}
