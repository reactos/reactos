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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <string.h>

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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/* FIXME: not supposed to be here */

const CLSID CLSID_PSDispatch = {
  0x20420, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}
};

static CStdPSFactoryBuffer PSFactoryBuffer;

CSTDSTUBBUFFERRELEASE(&PSFactoryBuffer)

extern const ExtendedProxyFileInfo oaidl_ProxyFileInfo;

const ProxyFileInfo* OLEAUT32_ProxyFileList[] = {
  &oaidl_ProxyFileInfo,
  NULL
};

HRESULT OLEAUTPS_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
  return NdrDllGetClassObject(rclsid, riid, ppv, OLEAUT32_ProxyFileList,
                              &CLSID_PSDispatch, &PSFactoryBuffer);
}

/* CLEANLOCALSTORAGE */
/* I'm not sure how this is supposed to work yet */

unsigned long WINAPI CLEANLOCALSTORAGE_UserSize(unsigned long *pFlags, unsigned long Start, CLEANLOCALSTORAGE *pstg)
{
  return Start + sizeof(DWORD);
}

unsigned char * WINAPI CLEANLOCALSTORAGE_UserMarshal(unsigned long *pFlags, unsigned char *Buffer, CLEANLOCALSTORAGE *pstg)
{
  *(DWORD*)Buffer = 0;
  return Buffer + sizeof(DWORD);
}

unsigned char * WINAPI CLEANLOCALSTORAGE_UserUnmarshal(unsigned long *pFlags, unsigned char *Buffer, CLEANLOCALSTORAGE *pstr)
{
  return Buffer + sizeof(DWORD);
}

void WINAPI CLEANLOCALSTORAGE_UserFree(unsigned long *pFlags, CLEANLOCALSTORAGE *pstr)
{
}

/* BSTR */

unsigned long WINAPI BSTR_UserSize(unsigned long *pFlags, unsigned long Start, BSTR *pstr)
{
  TRACE("(%lx,%ld,%p) => %p\n", *pFlags, Start, pstr, *pstr);
  if (*pstr) TRACE("string=%s\n", debugstr_w(*pstr));
  Start += sizeof(FLAGGED_WORD_BLOB) + sizeof(OLECHAR) * (SysStringLen(*pstr) - 1);
  TRACE("returning %ld\n", Start);
  return Start;
}

unsigned char * WINAPI BSTR_UserMarshal(unsigned long *pFlags, unsigned char *Buffer, BSTR *pstr)
{
  wireBSTR str = (wireBSTR)Buffer;

  TRACE("(%lx,%p,%p) => %p\n", *pFlags, Buffer, pstr, *pstr);
  if (*pstr) TRACE("string=%s\n", debugstr_w(*pstr));
  str->fFlags = 0;
  str->clSize = SysStringLen(*pstr);
  if (str->clSize)
    memcpy(&str->asData, *pstr, sizeof(OLECHAR) * str->clSize);
  return Buffer + sizeof(FLAGGED_WORD_BLOB) + sizeof(OLECHAR) * (str->clSize - 1);
}

unsigned char * WINAPI BSTR_UserUnmarshal(unsigned long *pFlags, unsigned char *Buffer, BSTR *pstr)
{
  wireBSTR str = (wireBSTR)Buffer;
  TRACE("(%lx,%p,%p) => %p\n", *pFlags, Buffer, pstr, *pstr);
  if (str->clSize) {
    SysReAllocStringLen(pstr, (OLECHAR*)&str->asData, str->clSize);
  }
  else if (*pstr) {
    SysFreeString(*pstr);
    *pstr = NULL;
  }
  if (*pstr) TRACE("string=%s\n", debugstr_w(*pstr));
  return Buffer + sizeof(FLAGGED_WORD_BLOB) + sizeof(OLECHAR) * (str->clSize - 1);
}

void WINAPI BSTR_UserFree(unsigned long *pFlags, BSTR *pstr)
{
  TRACE("(%lx,%p) => %p\n", *pFlags, pstr, *pstr);
  if (*pstr) {
    SysFreeString(*pstr);
    *pstr = NULL;
  }
}

/* VARIANT */
/* I'm not too sure how to do this yet */

#define VARIANT_wiresize sizeof(struct _wireVARIANT)

static unsigned wire_size(VARTYPE vt)
{
  if (vt & VT_ARRAY) return 0;

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
  case VT_VARIANT:
  case VT_UNKNOWN:
  case VT_DISPATCH:
  case VT_SAFEARRAY:
  case VT_RECORD:
    return 0;
  default:
    FIXME("unhandled VT %d\n", vt);
    return 0;
  }
}

static unsigned wire_extra(unsigned long *pFlags, VARIANT *pvar)
{
  ULONG size;
  HRESULT hr;

  if (V_VT(pvar) & VT_ARRAY) {
    FIXME("wire-size safearray\n");
    return 0;
  }
  switch (V_VT(pvar)) {
  case VT_BSTR:
    return BSTR_UserSize(pFlags, 0, &V_BSTR(pvar));
  case VT_BSTR | VT_BYREF:
    return BSTR_UserSize(pFlags, 0, V_BSTRREF(pvar));
  case VT_SAFEARRAY:
  case VT_SAFEARRAY | VT_BYREF:
    FIXME("wire-size safearray\n");
    return 0;
  case VT_VARIANT | VT_BYREF:
    return VARIANT_UserSize(pFlags, 0, V_VARIANTREF(pvar));
  case VT_UNKNOWN:
  case VT_DISPATCH:
    /* find the buffer size of the marshalled dispatch interface */
    hr = CoGetMarshalSizeMax(&size, &IID_IDispatch, (IUnknown*)V_DISPATCH(pvar), LOWORD(*pFlags), NULL, MSHLFLAGS_NORMAL);
    if (FAILED(hr)) {
      ERR("Dispatch variant buffer size calculation failed, HRESULT=0x%lx\n", hr);
      return 0;
    }
    size += sizeof(ULONG); /* we have to store the buffersize in the stream */
    TRACE("wire-size extra of dispatch variant is %ld\n", size);
    return size;
  case VT_RECORD:
    FIXME("wire-size record\n");
    return 0;
  default:
    return 0;
  }
}

/* helper: called for VT_DISPATCH variants to marshal the IDispatch* into the buffer. returns Buffer on failure, new position otherwise */
static unsigned char* dispatch_variant_marshal(unsigned long *pFlags, unsigned char *Buffer, VARIANT *pvar) {
  IStream *working; 
  HGLOBAL working_mem;
  void *working_memlocked;
  unsigned char *oldpos;
  ULONG size;
  HRESULT hr;
  
  TRACE("pFlags=%ld, Buffer=%p, pvar=%p\n", *pFlags, Buffer, pvar);

  oldpos = Buffer;
  
  /* CoMarshalInterface needs a stream, whereas at this level we are operating in terms of buffers.
   * We create a stream on an HGLOBAL, so we can simply do a memcpy to move it to the buffer.
   * in rpcrt4/ndr_ole.c, a simple IStream implementation is wrapped around the buffer object,
   * but that would be overkill here, hence this implementation. We save the size because the unmarshal
   * code has no way to know how long the marshalled buffer is. */

  size = wire_extra(pFlags, pvar);
  
  working_mem = GlobalAlloc(0, size);
  if (!working_mem) return oldpos;

  hr = CreateStreamOnHGlobal(working_mem, TRUE, &working);
  if (hr != S_OK) {
    GlobalFree(working_mem);
    return oldpos;
  }
  
  hr = CoMarshalInterface(working, &IID_IDispatch, (IUnknown*)V_DISPATCH(pvar), LOWORD(*pFlags), NULL, MSHLFLAGS_NORMAL);
  if (hr != S_OK) {
    IStream_Release(working); /* this also releases the hglobal */
    return oldpos;
  }

  working_memlocked = GlobalLock(working_mem);
  memcpy(Buffer, &size, sizeof(ULONG)); /* copy the buffersize */
  Buffer += sizeof(ULONG);
  memcpy(Buffer, working_memlocked, size);
  GlobalUnlock(working_mem);

  IStream_Release(working);

  TRACE("done, size=%ld\n", sizeof(ULONG) + size);
  return Buffer + sizeof(ULONG) + size;
}

/* helper: called for VT_DISPATCH variants to unmarshal the buffer back into a dispatch variant. returns Buffer on failure, new position otherwise */
static unsigned char *dispatch_variant_unmarshal(unsigned long *pFlags, unsigned char *Buffer, VARIANT *pvar) {
  IStream *working;
  HGLOBAL working_mem;
  void *working_memlocked;
  unsigned char *oldpos;
  ULONG size;
  HRESULT hr;
  
  TRACE("pFlags=%ld, Buffer=%p, pvar=%p\n", *pFlags, Buffer, pvar);

  oldpos = Buffer;
  
  /* get the buffersize */
  memcpy(&size, Buffer, sizeof(ULONG));
  TRACE("buffersize=%ld\n", size);
  Buffer += sizeof(ULONG);
  
  working_mem = GlobalAlloc(0, size);
  if (!working_mem) return oldpos;

  hr = CreateStreamOnHGlobal(working_mem, TRUE, &working);
  if (hr != S_OK) {
    GlobalFree(working_mem);
    return oldpos;
  }

  working_memlocked = GlobalLock(working_mem);
  
  /* now we copy the contents of the marshalling buffer to working_memlocked, unlock it, and demarshal the stream */
  memcpy(working_memlocked, Buffer, size);
  GlobalUnlock(working_mem);

  hr = CoUnmarshalInterface(working, &IID_IDispatch, (void**)&V_DISPATCH(pvar));
  if (hr != S_OK) {
    IStream_Release(working);
    return oldpos;
  }

  IStream_Release(working); /* this also frees the underlying hglobal */

  TRACE("done, processed=%ld bytes\n", sizeof(ULONG) + size);
  return Buffer + sizeof(ULONG) + size;
}


unsigned long WINAPI VARIANT_UserSize(unsigned long *pFlags, unsigned long Start, VARIANT *pvar)
{
  TRACE("(%lx,%ld,%p)\n", *pFlags, Start, pvar);
  TRACE("vt=%04x\n", V_VT(pvar));
  Start += VARIANT_wiresize + wire_extra(pFlags, pvar);
  TRACE("returning %ld\n", Start);
  return Start;
}

unsigned char * WINAPI VARIANT_UserMarshal(unsigned long *pFlags, unsigned char *Buffer, VARIANT *pvar)
{
  wireVARIANT var = (wireVARIANT)Buffer;
  unsigned size, extra;
  unsigned char *Pos = Buffer + VARIANT_wiresize;

  TRACE("(%lx,%p,%p)\n", *pFlags, Buffer, pvar);
  TRACE("vt=%04x\n", V_VT(pvar));

  memset(var, 0, sizeof(*var));
  var->clSize = sizeof(*var);
  var->vt = pvar->n1.n2.vt;

  var->rpcReserved = var->vt;
  if ((var->vt & VT_ARRAY) ||
      ((var->vt & VT_TYPEMASK) == VT_SAFEARRAY))
    var->vt = VT_ARRAY | (var->vt & VT_BYREF);

  if (var->vt == VT_DECIMAL) {
    /* special case because decVal is on a different level */
    var->u.decVal = pvar->n1.decVal;
    return Pos;
  }

  size = wire_size(V_VT(pvar));
  extra = wire_extra(pFlags, pvar);
  var->wReserved1 = pvar->n1.n2.wReserved1;
  var->wReserved2 = pvar->n1.n2.wReserved2;
  var->wReserved3 = pvar->n1.n2.wReserved3;
  if (size) {
    if (var->vt & VT_BYREF)
      memcpy(&var->u.cVal, pvar->n1.n2.n3.byref, size);
    else
      memcpy(&var->u.cVal, &pvar->n1.n2.n3, size);
  }
  if (!extra) return Pos;

  switch (var->vt) {
  case VT_BSTR:
    Pos = BSTR_UserMarshal(pFlags, Pos, &V_BSTR(pvar));
    break;
  case VT_BSTR | VT_BYREF:
    Pos = BSTR_UserMarshal(pFlags, Pos, V_BSTRREF(pvar));
    break;
  case VT_VARIANT | VT_BYREF:
    Pos = VARIANT_UserMarshal(pFlags, Pos, V_VARIANTREF(pvar));
    break;
  case VT_DISPATCH | VT_BYREF:
    FIXME("handle DISPATCH by ref\n");
    break;
  case VT_DISPATCH:
    /* this should probably call WdtpInterfacePointer_UserMarshal in ole32.dll */
    Pos = dispatch_variant_marshal(pFlags, Pos, pvar);
    break;
  case VT_RECORD:
    FIXME("handle BRECORD by val\n");
    break;
  case VT_RECORD | VT_BYREF:
    FIXME("handle BRECORD by ref\n");
    break;
  default:
    FIXME("handle unknown complex type\n");
    break;
  }
  var->clSize = Pos - Buffer;
  TRACE("marshalled size=%ld\n", var->clSize);
  return Pos;
}

unsigned char * WINAPI VARIANT_UserUnmarshal(unsigned long *pFlags, unsigned char *Buffer, VARIANT *pvar)
{
  wireVARIANT var = (wireVARIANT)Buffer;
  unsigned size;
  unsigned char *Pos = Buffer + VARIANT_wiresize;

  TRACE("(%lx,%p,%p)\n", *pFlags, Buffer, pvar);
  VariantInit(pvar);
  pvar->n1.n2.vt = var->rpcReserved;
  TRACE("marshalled: clSize=%ld, vt=%04x\n", var->clSize, var->vt);
  TRACE("vt=%04x\n", V_VT(pvar));
  TRACE("reserved: %d, %d, %d\n", var->wReserved1, var->wReserved2, var->wReserved3);
  TRACE("val: %ld\n", var->u.lVal);

  if (var->vt == VT_DECIMAL) {
    /* special case because decVal is on a different level */
    pvar->n1.decVal = var->u.decVal;
    return Pos;
  }

  size = wire_size(V_VT(pvar));
  pvar->n1.n2.wReserved1 = var->wReserved1;
  pvar->n1.n2.wReserved2 = var->wReserved2;
  pvar->n1.n2.wReserved3 = var->wReserved3;
  if (size) {
    if (var->vt & VT_BYREF) {
      pvar->n1.n2.n3.byref = CoTaskMemAlloc(size);
      memcpy(pvar->n1.n2.n3.byref, &var->u.cVal, size);
    }
    else
      memcpy(&pvar->n1.n2.n3, &var->u.cVal, size);
  }
  if (var->clSize <= VARIANT_wiresize) return Pos;

  switch (var->vt) {
  case VT_BSTR:
    Pos = BSTR_UserUnmarshal(pFlags, Pos, &V_BSTR(pvar));
    break;
  case VT_BSTR | VT_BYREF:
    pvar->n1.n2.n3.byref = CoTaskMemAlloc(sizeof(BSTR));
    *(BSTR*)pvar->n1.n2.n3.byref = NULL;
    Pos = BSTR_UserUnmarshal(pFlags, Pos, V_BSTRREF(pvar));
    break;
  case VT_VARIANT | VT_BYREF:
    pvar->n1.n2.n3.byref = CoTaskMemAlloc(sizeof(VARIANT));
    Pos = VARIANT_UserUnmarshal(pFlags, Pos, V_VARIANTREF(pvar));
    break;
  case VT_RECORD:
    FIXME("handle BRECORD by val\n");
    break;
  case VT_RECORD | VT_BYREF:
    FIXME("handle BRECORD by ref\n");
    break;
  case VT_DISPATCH:
    Pos = dispatch_variant_unmarshal(pFlags, Pos, pvar);
    break;
  case VT_DISPATCH | VT_BYREF:
    FIXME("handle DISPATCH by ref\n");
  default:
    FIXME("handle unknown complex type\n");
    break;
  }
  if (Pos != Buffer + var->clSize) {
    ERR("size difference during unmarshal\n");
  }
  return Buffer + var->clSize;
}

void WINAPI VARIANT_UserFree(unsigned long *pFlags, VARIANT *pvar)
{
  VARTYPE vt = V_VT(pvar);
  PVOID ref = NULL;

  TRACE("(%lx,%p)\n", *pFlags, pvar);
  TRACE("vt=%04x\n", V_VT(pvar));

  if (vt & VT_BYREF) ref = pvar->n1.n2.n3.byref;

  VariantClear(pvar);
  if (!ref) return;

  switch (vt) {
  case VT_BSTR | VT_BYREF:
    BSTR_UserFree(pFlags, ref);
    break;
  case VT_VARIANT | VT_BYREF:
    VARIANT_UserFree(pFlags, ref);
    break;
  case VT_RECORD | VT_BYREF:
    FIXME("handle BRECORD by ref\n");
    break;
  default:
    FIXME("handle unknown complex type\n");
    break;
  }

  CoTaskMemFree(ref);
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

  TRACE("(%p)->(%ld,%s,%lx,%x,%p,%p,%p,%p)\n", This,
        dispIdMember, debugstr_guid(riid),
        lcid, wFlags, pDispParams, pVarResult,
        pExcepInfo, puArgErr);

  /* [out] args can't be null, use dummy vars if needed */
  if (!pVarResult) pVarResult = &VarResult;

  /* count by-ref args */
  for (cVarRef=0,u=0; u<pDispParams->cArgs; u++) {
    VARIANTARG* arg = &pDispParams->rgvarg[u];
    if (V_VT(arg) & VT_BYREF) {
      cVarRef++;
    }
  }
  if (cVarRef) {
    rgVarRefIdx = CoTaskMemAlloc(sizeof(UINT)*cVarRef);
    rgVarRef = CoTaskMemAlloc(sizeof(VARIANTARG)*cVarRef);
    /* make list of by-ref args */
    for (cVarRef=0,u=0; u<pDispParams->cArgs; u++) {
      VARIANTARG* arg = &pDispParams->rgvarg[u];
      if (V_VT(arg) & VT_BYREF) {
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
  HRESULT hr;
  VARIANTARG *rgvarg, *arg;
  UINT u;

  /* let the real Invoke operate on a copy of the in parameters,
   * so we don't risk losing pointers to allocated memory */
  rgvarg = pDispParams->rgvarg;
  arg = CoTaskMemAlloc(sizeof(VARIANTARG)*pDispParams->cArgs);
  for (u=0; u<pDispParams->cArgs; u++) {
    VariantInit(&arg[u]);
    VariantCopy(&arg[u], &rgvarg[u]);
  }
  pDispParams->rgvarg = arg;

  /* initialize out parameters, so that they can be marshalled
   * in case the real Invoke doesn't initialize them */
  VariantInit(pVarResult);
  memset(pExcepInfo, 0, sizeof(*pExcepInfo));
  *pArgErr = 0;

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
  /* clear the duplicate argument list */
  for (u=0; u<pDispParams->cArgs; u++) {
    VariantClear(&arg[u]);
  }
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
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT __RPC_STUB ITypeInfo_GetTypeAttr_Stub(
    ITypeInfo* This,
    LPTYPEATTR* ppTypeAttr,
    CLEANLOCALSTORAGE* pDummy)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT CALLBACK ITypeInfo_GetFuncDesc_Proxy(
    ITypeInfo* This,
    UINT index,
    FUNCDESC** ppFuncDesc)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT __RPC_STUB ITypeInfo_GetFuncDesc_Stub(
    ITypeInfo* This,
    UINT index,
    LPFUNCDESC* ppFuncDesc,
    CLEANLOCALSTORAGE* pDummy)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT CALLBACK ITypeInfo_GetVarDesc_Proxy(
    ITypeInfo* This,
    UINT index,
    VARDESC** ppVarDesc)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT __RPC_STUB ITypeInfo_GetVarDesc_Stub(
    ITypeInfo* This,
    UINT index,
    LPVARDESC* ppVarDesc,
    CLEANLOCALSTORAGE* pDummy)
{
  FIXME("not implemented\n");
  return E_FAIL;
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
  FIXME("not implemented\n");
  return E_FAIL;
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
  FIXME("not implemented\n");
  return E_FAIL;
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
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT __RPC_STUB ITypeInfo_GetContainingTypeLib_Stub(
    ITypeInfo* This,
    ITypeLib** ppTLib,
    UINT* pIndex)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

void CALLBACK ITypeInfo_ReleaseTypeAttr_Proxy(
    ITypeInfo* This,
    TYPEATTR* pTypeAttr)
{
  FIXME("not implemented\n");
}

HRESULT __RPC_STUB ITypeInfo_ReleaseTypeAttr_Stub(
    ITypeInfo* This)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

void CALLBACK ITypeInfo_ReleaseFuncDesc_Proxy(
    ITypeInfo* This,
    FUNCDESC* pFuncDesc)
{
  FIXME("not implemented\n");
}

HRESULT __RPC_STUB ITypeInfo_ReleaseFuncDesc_Stub(
    ITypeInfo* This)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

void CALLBACK ITypeInfo_ReleaseVarDesc_Proxy(
    ITypeInfo* This,
    VARDESC* pVarDesc)
{
  FIXME("not implemented\n");
}

HRESULT __RPC_STUB ITypeInfo_ReleaseVarDesc_Stub(
    ITypeInfo* This)
{
  FIXME("not implemented\n");
  return E_FAIL;
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
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT __RPC_STUB ITypeLib_GetTypeInfoCount_Stub(
    ITypeLib* This,
    UINT* pcTInfo)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT CALLBACK ITypeLib_GetLibAttr_Proxy(
    ITypeLib* This,
    TLIBATTR** ppTLibAttr)
{
  FIXME("not implemented\n");
  return E_FAIL;
}

HRESULT __RPC_STUB ITypeLib_GetLibAttr_Stub(
    ITypeLib* This,
    LPTLIBATTR* ppTLibAttr,
    CLEANLOCALSTORAGE* pDummy)
{
  FIXME("not implemented\n");
  return E_FAIL;
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
  FIXME("not implemented\n");
}

HRESULT __RPC_STUB ITypeLib_ReleaseTLibAttr_Stub(
    ITypeLib* This)
{
  FIXME("not implemented\n");
  return E_FAIL;
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
