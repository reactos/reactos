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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"

#include "ole2.h"
#include "oleauto.h"
#include "typelib.h"
#include "ocidl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define ALIGNED_LENGTH(_Len, _Align) (((_Len)+(_Align))&~(_Align))
#define ALIGNED_POINTER(_Ptr, _Align) ((LPVOID)ALIGNED_LENGTH((ULONG_PTR)(_Ptr), _Align))
#define ALIGN_LENGTH(_Len, _Align) _Len = ALIGNED_LENGTH(_Len, _Align)
#define ALIGN_POINTER(_Ptr, _Align) _Ptr = ALIGNED_POINTER(_Ptr, _Align)

/* ole32 exports those, not defined in public headers */
ULONG __RPC_USER WdtpInterfacePointer_UserSize(ULONG*, ULONG, ULONG, IUnknown*, REFIID);
unsigned char * __RPC_USER WdtpInterfacePointer_UserMarshal(ULONG*, ULONG, unsigned char*, IUnknown*, REFIID);
unsigned char * __RPC_USER WdtpInterfacePointer_UserUnmarshal(ULONG*, unsigned char*, IUnknown**, REFIID);

static void dump_user_flags(const ULONG *pFlags)
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

    if (!pstg->pInterface)
        return Buffer + sizeof(DWORD);

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

    if (header->byte_len == 0xffffffff)
    {
        SysFreeString(*pstr);
        *pstr = NULL;
    }
    else SysReAllocStringLen( pstr, (OLECHAR *)(header + 1), header->len );

    if (*pstr) TRACE("string=%s\n", debugstr_w(*pstr));
    return Buffer + sizeof(*header) + sizeof(OLECHAR) * header->len;
}

void WINAPI BSTR_UserFree(ULONG *pFlags, BSTR *pstr)
{
    TRACE("(%x,%p) => %p\n", *pFlags, pstr, *pstr);
    SysFreeString(*pstr);
    *pstr = NULL;
}

/* VARIANT */

typedef struct
{
    DWORD clSize;
    DWORD rpcReserved;
    USHORT vt;
    USHORT wReserved1;
    USHORT wReserved2;
    USHORT wReserved3;
    DWORD switch_is;
} variant_wire_t;

unsigned int get_type_size(ULONG *pFlags, VARTYPE vt)
{
    if (vt & VT_ARRAY) return 4;

    switch (vt & ~VT_BYREF) {
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
    case VT_HRESULT:
        return sizeof(LONG);
    case VT_INT:
    case VT_UINT:
        return sizeof(INT);
    case VT_I8:
    case VT_UI8:
        return sizeof(LONGLONG);
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
        return sizeof(ULONG);
    case VT_VARIANT:
        return sizeof(VARIANT);
    case VT_UNKNOWN:
    case VT_DISPATCH:
    case VT_RECORD:
        return 0;
    default:
        FIXME("unhandled VT %d\n", vt);
        return 0;
    }
}

static unsigned int get_type_alignment(ULONG *pFlags, VARTYPE vt)
{
    unsigned int size = get_type_size(pFlags, vt);
    if(vt & VT_BYREF) return 3;
    if(size == 0) return 0;
    if(size <= 4) return size - 1;
    return 7;
}

/* WdtpInterfacePointer_UserSize takes care of 2 additional DWORDs to store marshalling buffer size */
#ifdef __REACTOS__
static unsigned interface_user_size(ULONG *pFlags, ULONG Start, REFIID riid, IUnknown *punk)
#else
static unsigned interface_variant_size(ULONG *pFlags, REFIID riid, IUnknown *punk)
#endif
{
    ULONG size = 0;

    if (punk)
    {
        size = WdtpInterfacePointer_UserSize(pFlags, LOWORD(*pFlags), 0, punk, riid);
        if (!size)
        {
            ERR("interface variant buffer size calculation failed\n");
            return 0;
        }
    }
    size += sizeof(ULONG);
    TRACE("wire-size extra of interface variant is %d\n", size);
#ifdef __REACTOS__
    return Start + size;
#else
    return size;
#endif
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
#ifdef __REACTOS__
    return interface_user_size(pFlags, Start, &IID_IUnknown, V_UNKNOWN(pvar));
#else
    return Start + interface_variant_size(pFlags, &IID_IUnknown, V_UNKNOWN(pvar));
#endif
  case VT_UNKNOWN | VT_BYREF:
#ifdef __REACTOS__
    return interface_user_size(pFlags, Start, &IID_IUnknown, *V_UNKNOWNREF(pvar));
#else
    return Start + interface_variant_size(pFlags, &IID_IUnknown, *V_UNKNOWNREF(pvar));
#endif
  case VT_DISPATCH:
#ifdef __REACTOS__
    return interface_user_size(pFlags, Start, &IID_IDispatch, (IUnknown*)V_DISPATCH(pvar));
#else
    return Start + interface_variant_size(pFlags, &IID_IDispatch, (IUnknown*)V_DISPATCH(pvar));
#endif
  case VT_DISPATCH | VT_BYREF:
#ifdef __REACTOS__
    return interface_user_size(pFlags, Start, &IID_IDispatch, (IUnknown*)*V_DISPATCHREF(pvar));
#else
    return Start + interface_variant_size(pFlags, &IID_IDispatch, (IUnknown*)*V_DISPATCHREF(pvar));
#endif
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

/* helper: called for VT_DISPATCH variants to marshal the IDispatch* into the buffer */
#ifdef __REACTOS__
static unsigned char* interface_user_marshal(ULONG *pFlags, unsigned char *Buffer,
#else
static unsigned char* interface_variant_marshal(ULONG *pFlags, unsigned char *Buffer,
#endif
                                                REFIID riid, IUnknown *punk)
{
  TRACE("pFlags=%d, Buffer=%p, pUnk=%p\n", *pFlags, Buffer, punk);

  /* first DWORD is used to store pointer itself, truncated on win64 */
  if(!punk)
  {
      memset(Buffer, 0, sizeof(ULONG));
      return Buffer + sizeof(ULONG);
  }
  else
  {
      *(DWORD*)Buffer = (DWORD_PTR)punk;
      Buffer += sizeof(DWORD);
  }

  return WdtpInterfacePointer_UserMarshal(pFlags, LOWORD(*pFlags), Buffer, punk, riid);
}

/* helper: called for VT_DISPATCH / VT_UNKNOWN variants to unmarshal the buffer */
#ifdef __REACTOS__
static unsigned char *interface_user_unmarshal(ULONG *pFlags, unsigned char *Buffer,
#else
static unsigned char *interface_variant_unmarshal(ULONG *pFlags, unsigned char *Buffer,
#endif
                                                  REFIID riid, IUnknown **ppunk)
{
  DWORD ptr;
  
  TRACE("pFlags=%d, Buffer=%p, ppUnk=%p\n", *pFlags, Buffer, ppunk);

  /* skip pointer part itself */
  ptr = *(DWORD*)Buffer;
  Buffer += sizeof(DWORD);

  if(!ptr)
      return Buffer;

  return WdtpInterfacePointer_UserUnmarshal(pFlags, Buffer, ppunk, riid);
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

    align = get_type_alignment(pFlags, V_VT(pvar));
    ALIGN_LENGTH(Start, align);
    if(V_VT(pvar) == (VT_VARIANT | VT_BYREF))
        Start += 4;
    else
        Start += get_type_size(pFlags, V_VT(pvar));
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
    header->rpcReserved = 0;
    header->vt = pvar->n1.n2.vt;
    header->wReserved1 = pvar->n1.n2.wReserved1;
    header->wReserved2 = pvar->n1.n2.wReserved2;
    header->wReserved3 = pvar->n1.n2.wReserved3;
    header->switch_is = pvar->n1.n2.vt;
    if(header->switch_is & VT_ARRAY)
        header->switch_is &= ~VT_TYPEMASK;

    Pos = (unsigned char*)(header + 1);
    type_size = get_type_size(pFlags, V_VT(pvar));
    align = get_type_alignment(pFlags, V_VT(pvar));
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
#ifdef __REACTOS__
            Pos = interface_user_marshal(pFlags, Pos, &IID_IUnknown, V_UNKNOWN(pvar));
#else
            Pos = interface_variant_marshal(pFlags, Pos, &IID_IUnknown, V_UNKNOWN(pvar));
#endif
            break;
        case VT_UNKNOWN | VT_BYREF:
#ifdef __REACTOS__
            Pos = interface_user_marshal(pFlags, Pos, &IID_IUnknown, *V_UNKNOWNREF(pvar));
#else
            Pos = interface_variant_marshal(pFlags, Pos, &IID_IUnknown, *V_UNKNOWNREF(pvar));
#endif
            break;
        case VT_DISPATCH:
#ifdef __REACTOS__
            Pos = interface_user_marshal(pFlags, Pos, &IID_IDispatch, (IUnknown*)V_DISPATCH(pvar));
#else
            Pos = interface_variant_marshal(pFlags, Pos, &IID_IDispatch, (IUnknown*)V_DISPATCH(pvar));
#endif
            break;
        case VT_DISPATCH | VT_BYREF:
#ifdef __REACTOS__
            Pos = interface_user_marshal(pFlags, Pos, &IID_IDispatch, (IUnknown*)*V_DISPATCHREF(pvar));
#else
            Pos = interface_variant_marshal(pFlags, Pos, &IID_IDispatch, (IUnknown*)*V_DISPATCHREF(pvar));
#endif
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

    header = (variant_wire_t *)Buffer; 
    
    Pos = (unsigned char*)(header + 1);
    type_size = get_type_size(pFlags, header->vt);
    align = get_type_alignment(pFlags, header->vt);
    ALIGN_POINTER(Pos, align);

    if(header->vt & VT_BYREF)
    {
        ULONG mem_size;
        Pos += 4;

        switch (header->vt & ~VT_BYREF)
        {
        /* these types have a different memory size compared to wire size */
        case VT_UNKNOWN:
        case VT_DISPATCH:
        case VT_BSTR:
            mem_size = sizeof(void *);
            break;
        default:
            mem_size = type_size;
            break;
        }

        if (V_VT(pvar) != header->vt)
        {
            VariantClear(pvar);
            V_BYREF(pvar) = CoTaskMemAlloc(mem_size);
            memset(V_BYREF(pvar), 0, mem_size);
        }
        else if (!V_BYREF(pvar))
        {
            V_BYREF(pvar) = CoTaskMemAlloc(mem_size);
            memset(V_BYREF(pvar), 0, mem_size);
        }

        if(!(header->vt & VT_ARRAY)
                && (header->vt & VT_TYPEMASK) != VT_BSTR
                && (header->vt & VT_TYPEMASK) != VT_VARIANT
                && (header->vt & VT_TYPEMASK) != VT_UNKNOWN
                && (header->vt & VT_TYPEMASK) != VT_DISPATCH
                && (header->vt & VT_TYPEMASK) != VT_RECORD)
            memcpy(V_BYREF(pvar), Pos, type_size);

        if((header->vt & VT_TYPEMASK) != VT_VARIANT)
            Pos += type_size;
        else
            Pos += 4;
    }
    else
    {
        VariantClear(pvar);
        if(header->vt & VT_ARRAY)
            V_ARRAY(pvar) = NULL;
        else if((header->vt & VT_TYPEMASK) == VT_BSTR)
            V_BSTR(pvar) = NULL;
        else if((header->vt & VT_TYPEMASK) == VT_UNKNOWN)
            V_UNKNOWN(pvar) = NULL;
        else if((header->vt & VT_TYPEMASK) == VT_DISPATCH)
            V_DISPATCH(pvar) = NULL;
        else if((header->vt & VT_TYPEMASK) == VT_RECORD)
            V_RECORD(pvar) = NULL;
        else if((header->vt & VT_TYPEMASK) == VT_DECIMAL)
            memcpy(pvar, Pos, type_size);
        else
            memcpy(&pvar->n1.n2.n3, Pos, type_size);
        Pos += type_size;
    }

    pvar->n1.n2.vt = header->vt;
    pvar->n1.n2.wReserved1 = header->wReserved1;
    pvar->n1.n2.wReserved2 = header->wReserved2;
    pvar->n1.n2.wReserved3 = header->wReserved3;

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
            Pos = BSTR_UserUnmarshal(pFlags, Pos, &V_BSTR(pvar));
            break;
        case VT_BSTR | VT_BYREF:
            Pos = BSTR_UserUnmarshal(pFlags, Pos, V_BSTRREF(pvar));
            break;
        case VT_VARIANT | VT_BYREF:
            Pos = VARIANT_UserUnmarshal(pFlags, Pos, V_VARIANTREF(pvar));
            break;
        case VT_UNKNOWN:
#ifdef __REACTOS__
            Pos = interface_user_unmarshal(pFlags, Pos, &IID_IUnknown, &V_UNKNOWN(pvar));
#else
            Pos = interface_variant_unmarshal(pFlags, Pos, &IID_IUnknown, &V_UNKNOWN(pvar));
#endif
            break;
        case VT_UNKNOWN | VT_BYREF:
#ifdef __REACTOS__
            Pos = interface_user_unmarshal(pFlags, Pos, &IID_IUnknown, V_UNKNOWNREF(pvar));
#else
            Pos = interface_variant_unmarshal(pFlags, Pos, &IID_IUnknown, V_UNKNOWNREF(pvar));
#endif
            break;
        case VT_DISPATCH:
#ifdef __REACTOS__
            Pos = interface_user_unmarshal(pFlags, Pos, &IID_IDispatch, (IUnknown**)&V_DISPATCH(pvar));
#else
            Pos = interface_variant_unmarshal(pFlags, Pos, &IID_IDispatch, (IUnknown**)&V_DISPATCH(pvar));
#endif
            break;
        case VT_DISPATCH | VT_BYREF:
#ifdef __REACTOS__
            Pos = interface_user_unmarshal(pFlags, Pos, &IID_IDispatch, (IUnknown**)V_DISPATCHREF(pvar));
#else
            Pos = interface_variant_unmarshal(pFlags, Pos, &IID_IDispatch, (IUnknown**)V_DISPATCHREF(pvar));
#endif
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
  {
    if (vt & VT_BYREF)
      LPSAFEARRAY_UserFree(pFlags, V_ARRAYREF(pvar));
    else
      LPSAFEARRAY_UserFree(pFlags, &V_ARRAY(pvar));
  }
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
      if (*V_UNKNOWNREF(pvar))
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

static DWORD elem_wire_size(LPSAFEARRAY lpsa, SF_TYPE sftype)
{
#ifdef __REACTOS__
    switch (sftype)
    {
    case SF_BSTR:
    case SF_HAVEIID:
    case SF_UNKNOWN:
    case SF_DISPATCH:
#else
    if (sftype == SF_BSTR)
#endif
        return sizeof(DWORD);
#ifdef __REACTOS__

    case SF_VARIANT:
#else
    else if (sftype == SF_VARIANT)
#endif
        return sizeof(variant_wire_t) - sizeof(DWORD);
#ifdef __REACTOS__

    default:
#else
    else
#endif
        return lpsa->cbElements;
#ifdef __REACTOS__
    }
#endif
}

static DWORD elem_mem_size(wireSAFEARRAY wiresa, SF_TYPE sftype)
{
#ifdef __REACTOS__
    switch (sftype)
    {
    case SF_HAVEIID:
    case SF_UNKNOWN:
    case SF_DISPATCH:
        return sizeof(void *);

    case SF_BSTR:
#else
    if (sftype == SF_BSTR)
#endif
        return sizeof(BSTR);
#ifdef __REACTOS__

    case SF_VARIANT:
#else
    else if (sftype == SF_VARIANT)
#endif
        return sizeof(VARIANT);
#ifdef __REACTOS__

    default:
#else
    else
#endif
        return wiresa->cbElements;
#ifdef __REACTOS__
    }
#endif
}

ULONG WINAPI LPSAFEARRAY_UserSize(ULONG *pFlags, ULONG StartingSize, LPSAFEARRAY *ppsa)
{
    ULONG size = StartingSize;

    TRACE("("); dump_user_flags(pFlags); TRACE(", %d, %p\n", StartingSize, *ppsa);

    ALIGN_LENGTH(size, 3);
    size += sizeof(ULONG);
    if (*ppsa)
    {
        SAFEARRAY *psa = *ppsa;
        ULONG ulCellCount = SAFEARRAY_GetCellCount(psa);
        SF_TYPE sftype;
        HRESULT hr;

        size += sizeof(ULONG);
        size += 2 * sizeof(USHORT) + 2 * sizeof(ULONG);

        sftype = SAFEARRAY_GetUnionType(psa);
        size += sizeof(ULONG);

        size += sizeof(ULONG);
        size += sizeof(ULONG);
        if (sftype == SF_HAVEIID)
            size += sizeof(IID);

        size += sizeof(psa->rgsabound[0]) * psa->cDims;

        size += sizeof(ULONG);

        switch (sftype)
        {
            case SF_BSTR:
            {
                BSTR* lpBstr;

                for (lpBstr = psa->pvData; ulCellCount; ulCellCount--, lpBstr++)
                    size = BSTR_UserSize(pFlags, size, lpBstr);

                break;
            }
            case SF_DISPATCH:
            case SF_UNKNOWN:
            case SF_HAVEIID:
#ifdef __REACTOS__
            {
                IUnknown **lpUnk;
                GUID guid;

                if (sftype == SF_HAVEIID)
                    SafeArrayGetIID(psa, &guid);
                else if (sftype == SF_UNKNOWN)
                    guid = IID_IUnknown;
                else
                    guid = IID_IDispatch;

                for (lpUnk = psa->pvData; ulCellCount; ulCellCount--, lpUnk++)
                    size = interface_user_size(pFlags, size, &guid, *lpUnk);

#else
                FIXME("size interfaces\n");
#endif
                break;
#ifdef __REACTOS__
            }
#endif
            case SF_VARIANT:
            {
                VARIANT* lpVariant;

                for (lpVariant = psa->pvData; ulCellCount; ulCellCount--, lpVariant++)
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
    *(ULONG *)Buffer = *ppsa ? 0x1 : 0x0;
    Buffer += sizeof(ULONG);
    if (*ppsa)
    {
        VARTYPE vt;
        SAFEARRAY *psa = *ppsa;
        ULONG ulCellCount = SAFEARRAY_GetCellCount(psa);
        SAFEARRAYBOUND *bound;
        SF_TYPE sftype;
        GUID guid;
        INT i;

        sftype = SAFEARRAY_GetUnionType(psa);

        *(ULONG *)Buffer = psa->cDims;
        Buffer += sizeof(ULONG);
        *(USHORT *)Buffer = psa->cDims;
        Buffer += sizeof(USHORT);
        *(USHORT *)Buffer = psa->fFeatures;
        Buffer += sizeof(USHORT);
        *(ULONG *)Buffer = elem_wire_size(psa, sftype);
        Buffer += sizeof(ULONG);

        hr = SafeArrayGetVartype(psa, &vt);
#ifdef __REACTOS__
        if ((psa->fFeatures & FADF_HAVEIID) || FAILED(hr)) vt = 0;
#else
        if (FAILED(hr)) vt = 0;
#endif

        *(ULONG *)Buffer = (USHORT)psa->cLocks | (vt << 16);
        Buffer += sizeof(ULONG);

        *(ULONG *)Buffer = sftype;
        Buffer += sizeof(ULONG);

        *(ULONG *)Buffer = ulCellCount;
        Buffer += sizeof(ULONG);
        *(ULONG *)Buffer = psa->pvData ? 0x2 : 0x0;
        Buffer += sizeof(ULONG);
        if (sftype == SF_HAVEIID)
        {
            SafeArrayGetIID(psa, &guid);
            memcpy(Buffer, &guid, sizeof(guid));
            Buffer += sizeof(guid);
        }

        /* bounds are marshaled in opposite order comparing to storage layout */
        bound = (SAFEARRAYBOUND*)Buffer;
        for (i = 0; i < psa->cDims; i++)
        {
            memcpy(bound++, &psa->rgsabound[psa->cDims-i-1], sizeof(psa->rgsabound[0]));
        }
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

                    for (lpBstr = psa->pvData; ulCellCount; ulCellCount--, lpBstr++)
                        Buffer = BSTR_UserMarshal(pFlags, Buffer, lpBstr);

                    break;
                }
                case SF_DISPATCH:
                case SF_UNKNOWN:
                case SF_HAVEIID:
#ifdef __REACTOS__
                {
                    IUnknown **lpUnk;
                    const GUID *iid;

                    if (sftype == SF_HAVEIID)
                        iid = &guid;
                    else if (sftype == SF_UNKNOWN)
                        iid = &IID_IUnknown;
                    else
                        iid = &IID_IDispatch;

                    for (lpUnk = psa->pvData; ulCellCount; ulCellCount--, lpUnk++)
                            Buffer = interface_user_marshal(pFlags, Buffer, iid, *lpUnk);

#else
                    FIXME("marshal interfaces\n");
#endif
                    break;
#ifdef __REACTOS__
                }
#endif
                case SF_VARIANT:
                {
                    VARIANT* lpVariant;

                    for (lpVariant = psa->pvData; ulCellCount; ulCellCount--, lpVariant++)
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
    ULONG ptr;
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
    ptr = *(ULONG *)Buffer;
    Buffer += sizeof(ULONG);

    if (!ptr)
    {
        SafeArrayDestroy(*ppsa);
        *ppsa = NULL;

        TRACE("NULL safe array unmarshaled\n");

        return Buffer;
    }

    cDims = *(ULONG *)Buffer;
    Buffer += sizeof(ULONG);

    wiresa = (wireSAFEARRAY)Buffer;
    Buffer += 2 * sizeof(USHORT) + 2 * sizeof(ULONG);

    if (cDims != wiresa->cDims)
        RpcRaiseException(RPC_S_INVALID_BOUND);

    /* FIXME: there should be a limit on how large cDims can be */

    vt = HIWORD(wiresa->cLocks);

    sftype = *(ULONG *)Buffer;
    Buffer += sizeof(ULONG);

    cell_count = *(ULONG *)Buffer;
    Buffer += sizeof(ULONG);
    ptr = *(ULONG *)Buffer;
    Buffer += sizeof(ULONG);
    if (sftype == SF_HAVEIID)
    {
        memcpy(&guid, Buffer, sizeof(guid));
        Buffer += sizeof(guid);
    }

    wiresab = (SAFEARRAYBOUND *)Buffer;
    Buffer += sizeof(wiresab[0]) * wiresa->cDims;

    if(*ppsa && (*ppsa)->cDims==wiresa->cDims)
    {
        if(((*ppsa)->fFeatures & ~FADF_AUTOSETFLAGS) != (wiresa->fFeatures & ~FADF_AUTOSETFLAGS))
            RpcRaiseException(DISP_E_BADCALLEE);

        if(SAFEARRAY_GetCellCount(*ppsa)*(*ppsa)->cbElements != cell_count*elem_mem_size(wiresa, sftype))
        {
            if((*ppsa)->fFeatures & (FADF_AUTO|FADF_STATIC|FADF_EMBEDDED|FADF_FIXEDSIZE))
                RpcRaiseException(DISP_E_BADCALLEE);

            hr = SafeArrayDestroyData(*ppsa);
            if(FAILED(hr))
                RpcRaiseException(hr);
        }
        memcpy((*ppsa)->rgsabound, wiresab, sizeof(*wiresab)*wiresa->cDims);

        if((*ppsa)->fFeatures & FADF_HAVEVARTYPE)
            ((DWORD*)(*ppsa))[-1] = vt;
    }
    else if(vt)
    {
        SafeArrayDestroy(*ppsa);
        *ppsa = SafeArrayCreateEx(vt, wiresa->cDims, wiresab, NULL);
        if (!*ppsa) RpcRaiseException(E_OUTOFMEMORY);
    }
    else
    {
        SafeArrayDestroy(*ppsa);
        if (FAILED(SafeArrayAllocDescriptor(wiresa->cDims, ppsa)))
            RpcRaiseException(E_OUTOFMEMORY);
        memcpy((*ppsa)->rgsabound, wiresab, sizeof(SAFEARRAYBOUND) * wiresa->cDims);
    }

    /* be careful about which flags we set since they could be a security
     * risk */
    (*ppsa)->fFeatures &= FADF_AUTOSETFLAGS;
    (*ppsa)->fFeatures |= (wiresa->fFeatures & ~(FADF_AUTOSETFLAGS));
    /* FIXME: there should be a limit on how large wiresa->cbElements can be */
    (*ppsa)->cbElements = elem_mem_size(wiresa, sftype);

    /* SafeArrayCreateEx allocates the data for us, but
     * SafeArrayAllocDescriptor doesn't */
    if(!(*ppsa)->pvData)
    {
        hr = SafeArrayAllocData(*ppsa);
        if (FAILED(hr))
            RpcRaiseException(hr);
    }

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

                for (lpBstr = (*ppsa)->pvData; cell_count; cell_count--, lpBstr++)
                    Buffer = BSTR_UserUnmarshal(pFlags, Buffer, lpBstr);

                break;
            }
            case SF_DISPATCH:
            case SF_UNKNOWN:
            case SF_HAVEIID:
#ifdef __REACTOS__
            {
                IUnknown **lpUnk;
                const GUID *iid;

                if (sftype == SF_HAVEIID)
                    iid = &guid;
                else if (sftype == SF_UNKNOWN)
                    iid = &IID_IUnknown;
                else
                    iid = &IID_IDispatch;

                for (lpUnk = (*ppsa)->pvData; cell_count; cell_count--, lpUnk++)
                    Buffer = interface_user_unmarshal(pFlags, Buffer, iid, lpUnk);

#else
                FIXME("marshal interfaces\n");
#endif
                break;
#ifdef __REACTOS__
            }
#endif
            case SF_VARIANT:
            {
                VARIANT* lpVariant;

                for (lpVariant = (*ppsa)->pvData; cell_count; cell_count--, lpVariant++)
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
    *ppsa = NULL;
}


ULONG WINAPI HFONT_UserSize(ULONG *pFlags, ULONG Start, HFONT *phfont)
{
    FIXME(":stub\n");
    return 0;
}

unsigned char * WINAPI HFONT_UserMarshal(ULONG *pFlags, unsigned char *Buffer, HFONT *phfont)
{
    FIXME(":stub\n");
    return NULL;
}

unsigned char * WINAPI HFONT_UserUnmarshal(ULONG *pFlags, unsigned char *Buffer, HFONT *phfont)
{
    FIXME(":stub\n");
    return NULL;
}

void WINAPI HFONT_UserFree(ULONG *pFlags, HFONT *phfont)
{
    FIXME(":stub\n");
    return;
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
	VariantCopy(&rgVarRef[cVarRef], arg);
	VariantClear(arg);
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
    /* copy ref args to arg array */
    for (u=0; u<cVarRef; u++) {
      unsigned i = rgVarRefIdx[u];
      VariantCopy(&arg[i], &rgVarRef[u]);
    }

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

    /* copy ref args from arg array */
    for (u=0; u<cVarRef; u++) {
      unsigned i = rgVarRefIdx[u];
      VariantCopy(&rgVarRef[u], &arg[i]);
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
    LPOLESTR name,
    ULONG lHashVal,
    WORD flags,
    ITypeInfo **ti,
    DESCKIND *desckind,
    BINDPTR *bindptr)
{
    CLEANLOCALSTORAGE stg = { 0 };
    ITypeComp *typecomp;
    FUNCDESC *funcdesc;
    VARDESC *vardesc;
    HRESULT hr;

    TRACE("(%p, %s, %#x, %#x, %p, %p, %p)\n", This, debugstr_w(name), lHashVal, flags, ti,
        desckind, bindptr);

    *desckind = DESCKIND_NONE;
    memset(bindptr, 0, sizeof(*bindptr));

    hr = ITypeComp_RemoteBind_Proxy(This, name, lHashVal, flags, ti, desckind,
        &funcdesc, &vardesc, &typecomp, &stg);

    if (hr == S_OK)
    {
        switch (*desckind)
        {
        case DESCKIND_FUNCDESC:
            bindptr->lpfuncdesc = funcdesc;
            break;
        case DESCKIND_VARDESC:
        case DESCKIND_IMPLICITAPPOBJ:
            bindptr->lpvardesc = vardesc;
            break;
        case DESCKIND_TYPECOMP:
            bindptr->lptcomp = typecomp;
            break;
        default:
            ;
        }
    }

    return hr;
}

HRESULT __RPC_STUB ITypeComp_Bind_Stub(
    ITypeComp* This,
    LPOLESTR name,
    ULONG lHashVal,
    WORD flags,
    ITypeInfo **ti,
    DESCKIND *desckind,
    FUNCDESC **funcdesc,
    VARDESC **vardesc,
    ITypeComp **typecomp,
    CLEANLOCALSTORAGE *stg)
{
    BINDPTR bindptr;
    HRESULT hr;

    TRACE("(%p, %s, %#x, %#x, %p, %p, %p, %p, %p, %p)\n", This, debugstr_w(name),
        lHashVal, flags, ti, desckind, funcdesc, vardesc, typecomp, stg);

    memset(stg, 0, sizeof(*stg));
    memset(&bindptr, 0, sizeof(bindptr));

    *funcdesc = NULL;
    *vardesc = NULL;
    *typecomp = NULL;
    *ti = NULL;

    hr = ITypeComp_Bind(This, name, lHashVal, flags, ti, desckind, &bindptr);
    if(hr != S_OK)
        return hr;

    switch (*desckind)
    {
    case DESCKIND_FUNCDESC:
        *funcdesc = bindptr.lpfuncdesc;
        stg->pInterface = (IUnknown*)*ti;
        stg->pStorage = funcdesc;
        stg->flags = CLS_FUNCDESC;
        break;
    case DESCKIND_VARDESC:
    case DESCKIND_IMPLICITAPPOBJ:
        *vardesc = bindptr.lpvardesc;
        stg->pInterface = (IUnknown*)*ti;
        stg->pStorage = vardesc;
        stg->flags = CLS_VARDESC;
        break;
    case DESCKIND_TYPECOMP:
        *typecomp = bindptr.lptcomp;
        break;
    default:
        ;
    }

    if (stg->pInterface)
        IUnknown_AddRef(stg->pInterface);

    return hr;
}

HRESULT CALLBACK ITypeComp_BindType_Proxy(
    ITypeComp* This,
    LPOLESTR name,
    ULONG lHashVal,
    ITypeInfo **ti,
    ITypeComp **typecomp)
{
    HRESULT hr;

    TRACE("(%p, %s, %#x, %p, %p)\n", This, debugstr_w(name), lHashVal, ti, typecomp);

    hr = ITypeComp_RemoteBindType_Proxy(This, name, lHashVal, ti);
    if (hr == S_OK)
        ITypeInfo_GetTypeComp(*ti, typecomp);
    else if (typecomp)
        *typecomp = NULL;

    return hr;
}

HRESULT __RPC_STUB ITypeComp_BindType_Stub(
    ITypeComp* This,
    LPOLESTR name,
    ULONG lHashVal,
    ITypeInfo **ti)
{
    ITypeComp *typecomp = NULL;
    HRESULT hr;

    TRACE("(%p, %s, %#x, %p)\n", This, debugstr_w(name), lHashVal, ti);

    hr = ITypeComp_BindType(This, name, lHashVal, ti, &typecomp);

    if (typecomp)
        ITypeComp_Release(typecomp);

    return hr;
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
    TRACE("(%p, %08x, %p, %d, %p)\n", This, memid, rgBstrNames, cMaxNames, pcNames);

    return ITypeInfo_RemoteGetNames_Proxy(This, memid, rgBstrNames, cMaxNames, pcNames);
}

HRESULT __RPC_STUB ITypeInfo_GetNames_Stub(
    ITypeInfo* This,
    MEMBERID memid,
    BSTR* rgBstrNames,
    UINT cMaxNames,
    UINT* pcNames)
{
    TRACE("(%p, %08x, %p, %d, %p)\n", This, memid, rgBstrNames, cMaxNames, pcNames);

    return ITypeInfo_GetNames(This, memid, rgBstrNames, cMaxNames, pcNames);
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

HRESULT CALLBACK ITypeInfo_GetDocumentation_Proxy(ITypeInfo *This, MEMBERID memid,
                                                  BSTR *name, BSTR *doc_string,
                                                  DWORD *help_context, BSTR *help_file)
{
    DWORD dummy_help_context, flags = 0;
    BSTR dummy_name, dummy_doc_string, dummy_help_file;
    HRESULT hr;
    TRACE("(%p, %08x, %p, %p, %p, %p)\n", This, memid, name, doc_string, help_context, help_file);

    if(!name) name = &dummy_name;
    else flags = 1;

    if(!doc_string) doc_string = &dummy_doc_string;
    else flags |= 2;

    if(!help_context) help_context = &dummy_help_context;
    else flags |= 4;

    if(!help_file) help_file = &dummy_help_file;
    else flags |= 8;

    hr = ITypeInfo_RemoteGetDocumentation_Proxy(This, memid, flags, name, doc_string, help_context, help_file);

    /* We don't need to free the dummy BSTRs since the stub ensures that these will be NULLs. */

    return hr;
}

HRESULT __RPC_STUB ITypeInfo_GetDocumentation_Stub(ITypeInfo *This, MEMBERID memid,
                                                   DWORD flags, BSTR *name, BSTR *doc_string,
                                                   DWORD *help_context, BSTR *help_file)
{
    TRACE("(%p, %08x, %08x, %p, %p, %p, %p)\n", This, memid, flags, name, doc_string, help_context, help_file);

    *name = *doc_string = *help_file = NULL;
    *help_context = 0;

    if(!(flags & 1)) name = NULL;
    if(!(flags & 2)) doc_string = NULL;
    if(!(flags & 4)) help_context = NULL;
    if(!(flags & 8)) help_file = NULL;

    return ITypeInfo_GetDocumentation(This, memid, name, doc_string, help_context, help_file);
}

HRESULT CALLBACK ITypeInfo_GetDllEntry_Proxy(ITypeInfo *This, MEMBERID memid,
                                             INVOKEKIND invkind, BSTR *dll_name,
                                             BSTR* name, WORD *ordinal)
{
    DWORD flags = 0;
    BSTR dummy_dll_name, dummy_name;
    WORD dummy_ordinal;
    HRESULT hr;
    TRACE("(%p, %08x, %x, %p, %p, %p)\n", This, memid, invkind, dll_name, name, ordinal);

    if(!dll_name) dll_name = &dummy_dll_name;
    else flags = 1;

    if(!name) name = &dummy_name;
    else flags |= 2;

    if(!ordinal) ordinal = &dummy_ordinal;
    else flags |= 4;

    hr = ITypeInfo_RemoteGetDllEntry_Proxy(This, memid, invkind, flags, dll_name, name, ordinal);

    /* We don't need to free the dummy BSTRs since the stub ensures that these will be NULLs. */

    return hr;
}

HRESULT __RPC_STUB ITypeInfo_GetDllEntry_Stub(ITypeInfo *This, MEMBERID memid,
                                              INVOKEKIND invkind, DWORD flags,
                                              BSTR *dll_name, BSTR *name,
                                              WORD *ordinal)
{
    TRACE("(%p, %08x, %x, %p, %p, %p)\n", This, memid, invkind, dll_name, name, ordinal);

    *dll_name = *name = NULL;
    *ordinal = 0;

    if(!(flags & 1)) dll_name = NULL;
    if(!(flags & 2)) name = NULL;
    if(!(flags & 4)) ordinal = NULL;

    return ITypeInfo_GetDllEntry(This, memid, invkind, dll_name, name, ordinal);
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

HRESULT CALLBACK ITypeInfo2_GetDocumentation2_Proxy(ITypeInfo2 *This, MEMBERID memid,
                                                    LCID lcid, BSTR *help_string,
                                                    DWORD *help_context, BSTR *help_dll)
{
    DWORD dummy_help_context, flags = 0;
    BSTR dummy_help_string, dummy_help_dll;
    HRESULT hr;
    TRACE("(%p, %08x, %08x, %p, %p, %p)\n", This, memid, lcid, help_string, help_context, help_dll);

    if(!help_string) help_string = &dummy_help_string;
    else flags = 1;

    if(!help_context) help_context = &dummy_help_context;
    else flags |= 2;

    if(!help_dll) help_dll = &dummy_help_dll;
    else flags |= 4;

    hr = ITypeInfo2_RemoteGetDocumentation2_Proxy(This, memid, lcid, flags, help_string, help_context, help_dll);

    /* We don't need to free the dummy BSTRs since the stub ensures that these will be NULLs. */

    return hr;
}

HRESULT __RPC_STUB ITypeInfo2_GetDocumentation2_Stub(ITypeInfo2 *This, MEMBERID memid,
                                                     LCID lcid, DWORD flags,
                                                     BSTR *help_string, DWORD *help_context,
                                                     BSTR *help_dll)
{
    TRACE("(%p, %08x, %08x, %08x, %p, %p, %p)\n", This, memid, lcid, flags, help_string, help_context, help_dll);

    *help_string = *help_dll = NULL;
    *help_context = 0;

    if(!(flags & 1)) help_string = NULL;
    if(!(flags & 2)) help_context = NULL;
    if(!(flags & 4)) help_dll = NULL;

    return ITypeInfo2_GetDocumentation2(This, memid, lcid, help_string, help_context, help_dll);
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

HRESULT CALLBACK ITypeLib_GetDocumentation_Proxy(ITypeLib *This, INT index, BSTR *name,
                                                 BSTR *doc_string, DWORD *help_context,
                                                 BSTR *help_file)
{
    DWORD dummy_help_context, flags = 0;
    BSTR dummy_name, dummy_doc_string, dummy_help_file;
    HRESULT hr;
    TRACE("(%p, %d, %p, %p, %p, %p)\n", This, index, name, doc_string, help_context, help_file);

    if(!name) name = &dummy_name;
    else flags = 1;

    if(!doc_string) doc_string = &dummy_doc_string;
    else flags |= 2;

    if(!help_context) help_context = &dummy_help_context;
    else flags |= 4;

    if(!help_file) help_file = &dummy_help_file;
    else flags |= 8;

    hr = ITypeLib_RemoteGetDocumentation_Proxy(This, index, flags, name, doc_string, help_context, help_file);

    /* We don't need to free the dummy BSTRs since the stub ensures that these will be NULLs. */

    return hr;
}

HRESULT __RPC_STUB ITypeLib_GetDocumentation_Stub(ITypeLib *This, INT index, DWORD flags,
                                                  BSTR *name, BSTR *doc_string,
                                                  DWORD *help_context, BSTR *help_file)
{
    TRACE("(%p, %d, %08x, %p, %p, %p, %p)\n", This, index, flags, name, doc_string, help_context, help_file);

    *name = *doc_string = *help_file = NULL;
    *help_context = 0;

    if(!(flags & 1)) name = NULL;
    if(!(flags & 2)) doc_string = NULL;
    if(!(flags & 4)) help_context = NULL;
    if(!(flags & 8)) help_file = NULL;

    return ITypeLib_GetDocumentation(This, index, name, doc_string, help_context, help_file);
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

HRESULT CALLBACK ITypeLib2_GetDocumentation2_Proxy(ITypeLib2 *This, INT index,
                                                   LCID lcid, BSTR *help_string,
                                                   DWORD *help_context, BSTR *help_dll)
{
    DWORD dummy_help_context, flags = 0;
    BSTR dummy_help_string, dummy_help_dll;
    HRESULT hr;
    TRACE("(%p, %d, %08x, %p, %p, %p)\n", This, index, lcid, help_string, help_context, help_dll);

    if(!help_string) help_string = &dummy_help_string;
    else flags = 1;

    if(!help_context) help_context = &dummy_help_context;
    else flags |= 2;

    if(!help_dll) help_dll = &dummy_help_dll;
    else flags |= 4;

    hr = ITypeLib2_RemoteGetDocumentation2_Proxy(This, index, lcid, flags, help_string, help_context, help_dll);

    /* We don't need to free the dummy BSTRs since the stub ensures that these will be NULLs. */

    return hr;
}

HRESULT __RPC_STUB ITypeLib2_GetDocumentation2_Stub(ITypeLib2 *This, INT index, LCID lcid,
                                                    DWORD flags, BSTR *help_string,
                                                    DWORD *help_context, BSTR *help_dll)
{
    TRACE("(%p, %d, %08x, %08x, %p, %p, %p)\n", This, index, lcid, flags, help_string, help_context, help_dll);

    *help_string = *help_dll = NULL;
    *help_context = 0;

    if(!(flags & 1)) help_string = NULL;
    if(!(flags & 2)) help_context = NULL;
    if(!(flags & 4)) help_dll = NULL;

    return ITypeLib2_GetDocumentation2(This, index, lcid, help_string, help_context, help_dll);
}

HRESULT CALLBACK IPropertyBag_Read_Proxy(
    IPropertyBag* This,
    LPCOLESTR pszPropName,
    VARIANT *pVar,
    IErrorLog *pErrorLog)
{
  IUnknown *pUnk = NULL;
  TRACE("(%p, %s, %p, %p)\n", This, debugstr_w(pszPropName), pVar, pErrorLog);

  if(!pVar)
    return E_POINTER;

  if(V_VT(pVar) & (VT_BYREF | VT_ARRAY | VT_VECTOR))
  {
    FIXME("Variant type %x is byref, array or vector. Not implemented.\n", V_VT(pVar));
    return E_NOTIMPL;
  }

  switch(V_VT(pVar))
  {
    case VT_DISPATCH:
      pUnk = (IUnknown*)V_DISPATCH(pVar);
      break;
    case VT_UNKNOWN:
      pUnk = V_UNKNOWN(pVar);
      break;
    case VT_SAFEARRAY:
      FIXME("Safearray support not yet implemented.\n");
      return E_NOTIMPL;
    default:
      FIXME("Unknown V_VT %d - support not yet implemented.\n", V_VT(pVar));
      return E_NOTIMPL;
  }

  return IPropertyBag_RemoteRead_Proxy(This, pszPropName, pVar, pErrorLog,
                                       V_VT(pVar), pUnk);
}

HRESULT __RPC_STUB IPropertyBag_Read_Stub(
    IPropertyBag* This,
    LPCOLESTR pszPropName,
    VARIANT *pVar,
    IErrorLog *pErrorLog,
    DWORD varType,
    IUnknown *pUnkObj)
{
  static const WCHAR emptyWstr[] = {0};
  IDispatch *disp;
  HRESULT hr;
  TRACE("(%p, %s, %p, %p, %x, %p)\n", This, debugstr_w(pszPropName), pVar,
                                     pErrorLog, varType, pUnkObj);

  if(varType & (VT_BYREF | VT_ARRAY | VT_VECTOR))
  {
    FIXME("Variant type %x is byref, array or vector. Not implemented.\n", V_VT(pVar));
    return E_NOTIMPL;
  }

  V_VT(pVar) = varType;
  switch(varType)
  {
    case VT_DISPATCH:
      hr = IUnknown_QueryInterface(pUnkObj, &IID_IDispatch, (LPVOID*)&disp);
      if(FAILED(hr))
        return hr;
      IUnknown_Release(pUnkObj);
      V_DISPATCH(pVar) = disp;
      break;
    case VT_UNKNOWN:
      V_UNKNOWN(pVar) = pUnkObj;
      break;
    case VT_BSTR:
      V_BSTR(pVar) = SysAllocString(emptyWstr);
      break;
    case VT_SAFEARRAY:
      FIXME("Safearray support not yet implemented.\n");
      return E_NOTIMPL;
    default:
      break;
  }
  hr = IPropertyBag_Read(This, pszPropName, pVar, pErrorLog);
  if(FAILED(hr))
    VariantClear(pVar);

  return hr;
}

/* call_as/local stubs for ocidl.idl */

HRESULT CALLBACK IClassFactory2_CreateInstanceLic_Proxy(
    IClassFactory2* This,
    IUnknown *pUnkOuter,
    IUnknown *pUnkReserved,
    REFIID riid,
    BSTR bstrKey,
    PVOID *ppvObj)
{
    TRACE("(%p, %s, %p)\n", pUnkOuter, debugstr_guid(riid), ppvObj);

    *ppvObj = NULL;

    if (pUnkOuter)
    {
        ERR("aggregation is not allowed on remote objects\n");
        return CLASS_E_NOAGGREGATION;
    }

    return IClassFactory2_RemoteCreateInstanceLic_Proxy(This, riid, bstrKey, (IUnknown**)ppvObj);
}

HRESULT __RPC_STUB IClassFactory2_CreateInstanceLic_Stub(
    IClassFactory2* This,
    REFIID riid,
    BSTR bstrKey,
    IUnknown **ppvObj)
{
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppvObj);
    return IClassFactory2_CreateInstanceLic(This, NULL, NULL, riid, bstrKey, (void**)ppvObj);
}

HRESULT CALLBACK IEnumConnections_Next_Proxy(
    IEnumConnections* This,
    ULONG cConnections,
    LPCONNECTDATA rgcd,
    ULONG *pcFetched)
{
    ULONG fetched;

    TRACE("(%u, %p %p)\n", cConnections, rgcd, pcFetched);

    if (!pcFetched)
        pcFetched = &fetched;

    return IEnumConnections_RemoteNext_Proxy(This, cConnections, rgcd, pcFetched);
}

HRESULT __RPC_STUB IEnumConnections_Next_Stub(
    IEnumConnections* This,
    ULONG cConnections,
    LPCONNECTDATA rgcd,
    ULONG *pcFetched)
{
    HRESULT hr;

    TRACE("(%u, %p, %p)\n", cConnections, rgcd, pcFetched);

    *pcFetched = 0;
    hr = IEnumConnections_Next(This, cConnections, rgcd, pcFetched);
    if (hr == S_OK)
        *pcFetched = cConnections;

    return hr;
}

HRESULT CALLBACK IEnumConnectionPoints_Next_Proxy(
    IEnumConnectionPoints* This,
    ULONG cConnections,
    IConnectionPoint **ppCP,
    ULONG *pcFetched)
{
    ULONG fetched;

    TRACE("(%u, %p %p)\n", cConnections, ppCP, pcFetched);

    if (!pcFetched)
        pcFetched = &fetched;

    return IEnumConnectionPoints_RemoteNext_Proxy(This, cConnections, ppCP, pcFetched);
}

HRESULT __RPC_STUB IEnumConnectionPoints_Next_Stub(
    IEnumConnectionPoints* This,
    ULONG cConnections,
    IConnectionPoint **ppCP,
    ULONG *pcFetched)
{
    HRESULT hr;

    TRACE("(%u, %p, %p)\n", cConnections, ppCP, pcFetched);

    *pcFetched = 0;
    hr = IEnumConnectionPoints_Next(This, cConnections, ppCP, pcFetched);
    if (hr == S_OK)
        *pcFetched = cConnections;

    return hr;
}

HRESULT CALLBACK IPersistMemory_Load_Proxy(
    IPersistMemory* This,
    LPVOID pMem,
    ULONG cbSize)
{
    TRACE("(%p, %u)\n", pMem, cbSize);

    if (!pMem)
        return E_POINTER;

    return IPersistMemory_RemoteLoad_Proxy(This, pMem, cbSize);
}

HRESULT __RPC_STUB IPersistMemory_Load_Stub(
    IPersistMemory* This,
    BYTE *pMem,
    ULONG cbSize)
{
    TRACE("(%p, %u)\n", pMem, cbSize);
    return IPersistMemory_Load(This, pMem, cbSize);
}

HRESULT CALLBACK IPersistMemory_Save_Proxy(
    IPersistMemory* This,
    LPVOID pMem,
    BOOL fClearDirty,
    ULONG cbSize)
{
    TRACE("(%p, %d, %u)\n", pMem, fClearDirty, cbSize);

    if (!pMem)
        return E_POINTER;

    return IPersistMemory_RemoteSave_Proxy(This, pMem, fClearDirty, cbSize);
}

HRESULT __RPC_STUB IPersistMemory_Save_Stub(
    IPersistMemory* This,
    BYTE *pMem,
    BOOL fClearDirty,
    ULONG cbSize)
{
    TRACE("(%p, %d, %u)\n", pMem, fClearDirty, cbSize);
    return IPersistMemory_Save(This, pMem, fClearDirty, cbSize);
}

void CALLBACK IAdviseSinkEx_OnViewStatusChange_Proxy(
    IAdviseSinkEx* This,
    DWORD dwViewStatus)
{
    TRACE("(%p, 0x%08x)\n", This, dwViewStatus);
    IAdviseSinkEx_RemoteOnViewStatusChange_Proxy(This, dwViewStatus);
}

HRESULT __RPC_STUB IAdviseSinkEx_OnViewStatusChange_Stub(
    IAdviseSinkEx* This,
    DWORD dwViewStatus)
{
    TRACE("(%p, 0x%08x)\n", This, dwViewStatus);
    IAdviseSinkEx_OnViewStatusChange(This, dwViewStatus);
    return S_OK;
}

HRESULT CALLBACK IEnumOleUndoUnits_Next_Proxy(
    IEnumOleUndoUnits* This,
    ULONG cElt,
    IOleUndoUnit **rgElt,
    ULONG *pcEltFetched)
{
    ULONG fetched;

    TRACE("(%u, %p %p)\n", cElt, rgElt, pcEltFetched);

    if (!pcEltFetched)
        pcEltFetched = &fetched;

    return IEnumOleUndoUnits_RemoteNext_Proxy(This, cElt, rgElt, pcEltFetched);
}

HRESULT __RPC_STUB IEnumOleUndoUnits_Next_Stub(
    IEnumOleUndoUnits* This,
    ULONG cElt,
    IOleUndoUnit **rgElt,
    ULONG *pcEltFetched)
{
    HRESULT hr;

    TRACE("(%u, %p, %p)\n", cElt, rgElt, pcEltFetched);

    *pcEltFetched = 0;
    hr = IEnumOleUndoUnits_Next(This, cElt, rgElt, pcEltFetched);
    if (hr == S_OK)
        *pcEltFetched = cElt;

    return hr;
}

HRESULT CALLBACK IQuickActivate_QuickActivate_Proxy(
    IQuickActivate* This,
    QACONTAINER *pQaContainer,
    QACONTROL *pQaControl)
{
    FIXME("not implemented\n");
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IQuickActivate_QuickActivate_Stub(
    IQuickActivate* This,
    QACONTAINER *pQaContainer,
    QACONTROL *pQaControl)
{
    FIXME("not implemented\n");
    return E_NOTIMPL;
}
