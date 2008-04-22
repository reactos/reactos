/**
 * Dispatch API functions
 *
 * Copyright 2000  Francois Jacques, Macadamian Technologies Inc.
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
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "oleauto.h"
#include "winerror.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static IDispatch * WINAPI StdDispatch_Construct(IUnknown * punkOuter, void * pvThis, ITypeInfo * pTypeInfo);

/******************************************************************************
 *		DispInvoke (OLEAUT32.30)
 *
 * Call an object method using the information from its type library.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: Returns DISP_E_EXCEPTION and updates pexcepinfo if an exception occurs.
 *           DISP_E_BADPARAMCOUNT if the number of parameters is incorrect.
 *           DISP_E_MEMBERNOTFOUND if the method does not exist.
 *           puArgErr is updated if a parameter error (see notes) occurs.
 *           Otherwise, returns the result of calling ITypeInfo_Invoke().
 *
 * NOTES
 *  Parameter errors include the following:
 *| DISP_E_BADVARTYPE
 *| E_INVALIDARG            An argument was invalid
 *| DISP_E_TYPEMISMATCH,
 *| DISP_E_OVERFLOW         An argument was valid but could not be coerced
 *| DISP_E_PARAMNOTOPTIONAL A non optional parameter was not passed
 *| DISP_E_PARAMNOTFOUND    A parameter was passed that was not expected by the method
 *  This call defers to ITypeInfo_Invoke().
 */
HRESULT WINAPI DispInvoke(
	VOID       *_this,        /* [in] Object to call method on */
	ITypeInfo  *ptinfo,       /* [in] Object type info */
	DISPID      dispidMember, /* [in] DISPID of the member (e.g. from GetIDsOfNames()) */
	USHORT      wFlags,       /* [in] Kind of method call (DISPATCH_ flags from "oaidl.h") */
	DISPPARAMS *pparams,      /* [in] Array of method arguments */
	VARIANT    *pvarResult,   /* [out] Destination for the result of the call */
	EXCEPINFO  *pexcepinfo,   /* [out] Destination for exception information */
	UINT       *puArgErr)     /* [out] Destination for bad argument */
{
    TRACE("\n");

    return ITypeInfo_Invoke(ptinfo, _this, dispidMember, wFlags,
                            pparams, pvarResult, pexcepinfo, puArgErr);
}

/******************************************************************************
 *		DispGetIDsOfNames (OLEAUT32.29)
 *
 * Convert a set of parameter names to DISPIDs for DispInvoke().
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: An HRESULT error code.
 *
 * NOTES
 *  This call defers to ITypeInfo_GetIDsOfNames(). The ITypeInfo interface passed
 *  as ptinfo contains the information to map names to DISPIDs.
 */
HRESULT WINAPI DispGetIDsOfNames(
	ITypeInfo  *ptinfo,    /* [in] Object's type info */
	OLECHAR   **rgszNames, /* [in] Array of names to get DISPIDs for */
	UINT        cNames,    /* [in] Number of names in rgszNames */
	DISPID     *rgdispid)  /* [out] Destination for converted DISPIDs */
{
    return ITypeInfo_GetIDsOfNames(ptinfo, rgszNames, cNames, rgdispid);
}

/******************************************************************************
 *		DispGetParam (OLEAUT32.28)
 *
 * Retrieve a parameter from a DISPPARAMS structure and coerce it to the
 * specified variant type.
 *
 * NOTES
 *  Coercion is done using system (0) locale.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_PARAMNOTFOUND, if position is invalid. or
 *           DISP_E_TYPEMISMATCH, if the coercion failed. puArgErr is
 *           set to the index of the argument in pdispparams.
 */
HRESULT WINAPI DispGetParam(
	DISPPARAMS *pdispparams, /* [in] Parameter list */
	UINT        position,    /* [in] Position of parameter to coerce in pdispparams */
	VARTYPE     vtTarg,      /* [in] Type of value to coerce to */
	VARIANT    *pvarResult,  /* [out] Destination for resulting variant */
	UINT       *puArgErr)    /* [out] Destination for error code */
{
    /* position is counted backwards */
    UINT pos;
    HRESULT hr;

    TRACE("position=%d, cArgs=%d, cNamedArgs=%d\n",
          position, pdispparams->cArgs, pdispparams->cNamedArgs);
    if (position < pdispparams->cArgs) {
      /* positional arg? */
      pos = pdispparams->cArgs - position - 1;
    } else {
      /* FIXME: is this how to handle named args? */
      for (pos=0; pos<pdispparams->cNamedArgs; pos++)
        if (pdispparams->rgdispidNamedArgs[pos] == position) break;

      if (pos==pdispparams->cNamedArgs)
        return DISP_E_PARAMNOTFOUND;
    }
    hr = VariantChangeType(pvarResult,
                           &pdispparams->rgvarg[pos],
                           0, vtTarg);
    if (hr == DISP_E_TYPEMISMATCH) *puArgErr = pos;
    return hr;
}

/******************************************************************************
 * CreateStdDispatch [OLEAUT32.32]
 *
 * Create and return a standard IDispatch object.
 *
 * RETURNS
 *  Success: S_OK. ppunkStdDisp contains the new object.
 *  Failure: An HRESULT error code.
 *
 * NOTES
 *  Outer unknown appears to be completely ignored.
 */
HRESULT WINAPI CreateStdDispatch(
        IUnknown* punkOuter,
        void* pvThis,
	ITypeInfo* ptinfo,
	IUnknown** ppunkStdDisp)
{
    TRACE("(%p, %p, %p, %p)\n", punkOuter, pvThis, ptinfo, ppunkStdDisp);

    *ppunkStdDisp = (LPUNKNOWN)StdDispatch_Construct(punkOuter, pvThis, ptinfo);
    if (!*ppunkStdDisp)
        return E_OUTOFMEMORY;
    return S_OK;
}


/******************************************************************************
 * IDispatch {OLEAUT32}
 *
 * NOTES
 *  The IDispatch interface provides a single interface to dispatch method calls,
 *  regardless of whether the object to be called is in or out of process,
 *  local or remote (e.g. being called over a network). This interface is late-bound
 *  (linked at run-time), as opposed to early-bound (linked at compile time).
 *
 *  The interface is used by objects that wish to called by scripting
 *  languages such as VBA, in order to minimise the amount of COM and C/C++
 *  knowledge required, or by objects that wish to live out of process from code
 *  that will call their methods.
 *
 *  Method, property and parameter names can be localised. The details required to
 *  map names to methods and parameters are collected in a type library, usually
 *  output by an IDL compiler using the objects IDL description. This information is
 *  accessible programatically through the ITypeLib interface (for a type library),
 *  and the ITypeInfo interface (for an object within the type library). Type information
 *  can also be created at run-time using CreateDispTypeInfo().
 *
 * WRAPPERS
 *  Instead of using IDispatch directly, there are several wrapper functions available
 *  to simplify the process of calling an objects methods through IDispatch.
 *
 *  A standard implementation of an IDispatch object is created by calling
 *  CreateStdDispatch(). Numeric Id values for the parameters and methods (DISPIDs)
 *  of an object of interest are retrieved by calling DispGetIDsOfNames(). DispGetParam()
 *  retrieves information about a particular parameter. Finally the DispInvoke()
 *  function is responsible for actually calling methods on an object.
 *
 * METHODS
 */

typedef struct
{
    const IDispatchVtbl *lpVtbl;
    void * pvThis;
    ITypeInfo * pTypeInfo;
    LONG ref;
} StdDispatch;

/******************************************************************************
 * IDispatch_QueryInterface {OLEAUT32}
 *
 * See IUnknown_QueryInterface.
 */
static HRESULT WINAPI StdDispatch_QueryInterface(
  LPDISPATCH iface,
  REFIID riid,
  void** ppvObject)
{
    StdDispatch *This = (StdDispatch *)iface;
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppvObject);

    if (IsEqualIID(riid, &IID_IDispatch) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObject = (LPVOID)This;
	IUnknown_AddRef((LPUNKNOWN)*ppvObject);
	return S_OK;
    }
    return E_NOINTERFACE;
}

/******************************************************************************
 * IDispatch_AddRef {OLEAUT32}
 *
 * See IUnknown_AddRef.
 */
static ULONG WINAPI StdDispatch_AddRef(LPDISPATCH iface)
{
    StdDispatch *This = (StdDispatch *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n",This, refCount - 1);

    return refCount;
}

/******************************************************************************
 * IDispatch_Release {OLEAUT32}
 *
 * See IUnknown_Release.
 */
static ULONG WINAPI StdDispatch_Release(LPDISPATCH iface)
{
    StdDispatch *This = (StdDispatch *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n", This, refCount + 1);

    if (!refCount)
    {
        ITypeInfo_Release(This->pTypeInfo);
        CoTaskMemFree(This);
    }

    return refCount;
}

/******************************************************************************
 * IDispatch_GetTypeInfoCount {OLEAUT32}
 *
 * Get the count of type information in an IDispatch interface.
 *
 * PARAMS
 *  iface   [I] IDispatch interface
 *  pctinfo [O] Destination for the count
 *
 * RETURNS
 *  Success: S_OK. pctinfo is updated with the count. This is always 1 if
 *           the object provides type information, and 0 if it does not.
 *  Failure: E_NOTIMPL. The object does not provide type information.
 *
 * NOTES
 *  See IDispatch() and IDispatch_GetTypeInfo().
 */
static HRESULT WINAPI StdDispatch_GetTypeInfoCount(LPDISPATCH iface, UINT * pctinfo)
{
    StdDispatch *This = (StdDispatch *)iface;
    TRACE("(%p)\n", pctinfo);

    *pctinfo = This->pTypeInfo ? 1 : 0;
    return S_OK;
}

/******************************************************************************
 * IDispatch_GetTypeInfo {OLEAUT32}
 *
 * Get type information from an IDispatch interface.
 *
 * PARAMS
 *  iface   [I] IDispatch interface
 *  iTInfo  [I] Index of type information.
 *  lcid    [I] Locale of the type information to get
 *  ppTInfo [O] Destination for the ITypeInfo object
 *
 * RETURNS
 *  Success: S_OK. ppTInfo is updated with the objects type information
 *  Failure: DISP_E_BADINDEX, if iTInfo is any value other than 0.
 *
 * NOTES
 *  See IDispatch.
 */
static HRESULT WINAPI StdDispatch_GetTypeInfo(LPDISPATCH iface, UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo)
{
    StdDispatch *This = (StdDispatch *)iface;
    TRACE("(%d, %x, %p)\n", iTInfo, lcid, ppTInfo);

    *ppTInfo = NULL;
    if (iTInfo != 0)
        return DISP_E_BADINDEX;

    if (This->pTypeInfo)
    {
      *ppTInfo = This->pTypeInfo;
      ITypeInfo_AddRef(*ppTInfo);
    }
    return S_OK;
}

/******************************************************************************
 * IDispatch_GetIDsOfNames {OLEAUT32}
 *
 * Convert a methods name and an optional set of parameter names into DISPIDs
 * for passing to IDispatch_Invoke().
 *
 * PARAMS
 *  iface     [I] IDispatch interface
 *  riid      [I] Reserved, set to IID_NULL
 *  rgszNames [I] Name to convert
 *  cNames    [I] Number of names in rgszNames
 *  lcid      [I] Locale of the type information to convert from
 *  rgDispId  [O] Destination for converted DISPIDs.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: DISP_E_UNKNOWNNAME, if any of the names is invalid.
 *           DISP_E_UNKNOWNLCID if lcid is invalid.
 *           Otherwise, an HRESULT error code.
 *
 * NOTES
 *  This call defers to ITypeInfo_GetIDsOfNames(), using the ITypeInfo object
 *  contained within the IDispatch object.
 *  The first member of the names list must be a method name. The names following
 *  the method name are the parameters for that method.
 */
static HRESULT WINAPI StdDispatch_GetIDsOfNames(LPDISPATCH iface, REFIID riid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgDispId)
{
    StdDispatch *This = (StdDispatch *)iface;
    TRACE("(%s, %p, %d, 0x%x, %p)\n", debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    if (!IsEqualGUID(riid, &IID_NULL))
    {
        FIXME(" expected riid == IID_NULL\n");
        return E_INVALIDARG;
    }
    return DispGetIDsOfNames(This->pTypeInfo, rgszNames, cNames, rgDispId);
}

/******************************************************************************
 * IDispatch_Invoke {OLEAUT32}
 *
 * Call an object method.
 *
 * PARAMS
 *  iface        [I] IDispatch interface
 *  dispIdMember [I] DISPID of the method (from GetIDsOfNames())
 *  riid         [I] Reserved, set to IID_NULL
 *  lcid         [I] Locale of the type information to convert parameters with
 *  wFlags,      [I] Kind of method call (DISPATCH_ flags from "oaidl.h")
 *  pDispParams  [I] Array of method arguments
 *  pVarResult   [O] Destination for the result of the call
 *  pExcepInfo   [O] Destination for exception information
 *  puArgErr     [O] Destination for bad argument
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: See DispInvoke() for failure cases.
 *
 * NOTES
 *  See DispInvoke() and IDispatch().
 */
static HRESULT WINAPI StdDispatch_Invoke(LPDISPATCH iface, DISPID dispIdMember, REFIID riid, LCID lcid,
                                         WORD wFlags, DISPPARAMS * pDispParams, VARIANT * pVarResult,
                                         EXCEPINFO * pExcepInfo, UINT * puArgErr)
{
    StdDispatch *This = (StdDispatch *)iface;
    TRACE("(%d, %s, 0x%x, 0x%x, %p, %p, %p, %p)\n", dispIdMember, debugstr_guid(riid), lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    if (!IsEqualGUID(riid, &IID_NULL))
    {
        FIXME(" expected riid == IID_NULL\n");
        return E_INVALIDARG;
    }
    return DispInvoke(This->pvThis, This->pTypeInfo, dispIdMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static const IDispatchVtbl StdDispatch_VTable =
{
  StdDispatch_QueryInterface,
  StdDispatch_AddRef,
  StdDispatch_Release,
  StdDispatch_GetTypeInfoCount,
  StdDispatch_GetTypeInfo,
  StdDispatch_GetIDsOfNames,
  StdDispatch_Invoke
};

static IDispatch * WINAPI StdDispatch_Construct(
  IUnknown * punkOuter,
  void * pvThis,
  ITypeInfo * pTypeInfo)
{
    StdDispatch * pStdDispatch;

    pStdDispatch = CoTaskMemAlloc(sizeof(StdDispatch));
    if (!pStdDispatch)
        return (IDispatch *)pStdDispatch;

    pStdDispatch->lpVtbl = &StdDispatch_VTable;
    pStdDispatch->pvThis = pvThis;
    pStdDispatch->pTypeInfo = pTypeInfo;
    pStdDispatch->ref = 1;

    /* we keep a reference to the type info so prevent it from
     * being destroyed until we are done with it */
    ITypeInfo_AddRef(pTypeInfo);

    return (IDispatch *)pStdDispatch;
}
