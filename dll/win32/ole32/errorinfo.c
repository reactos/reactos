/*
 * ErrorInfo API
 *
 * Copyright 2000 Patrik Stridvall, Juergen Schmied
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
 * NOTES:
 *
 * The errorinfo is a per-thread object. The reference is stored in the
 * TEB at offset 0xf80.
 */

#include <stdarg.h>
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "oleauto.h"
#include "winerror.h"

#include "wine/unicode.h"
#include "compobj_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/* this code is from SysAllocStringLen (ole2disp.c in oleaut32) */
static BSTR WINAPI ERRORINFO_SysAllocString(const OLECHAR* in)
{
    DWORD  bufferSize;
    DWORD* newBuffer;
    WCHAR* stringBuffer;
    DWORD len;

    if (in == NULL)
	return NULL;
    /*
     * Find the length of the buffer passed-in, in bytes.
     */
    len = strlenW(in);
    bufferSize = len * sizeof (WCHAR);

    /*
     * Allocate a new buffer to hold the string.
     * don't forget to keep an empty spot at the beginning of the
     * buffer for the character count and an extra character at the
     * end for the '\0'.
     */
    newBuffer = HeapAlloc(GetProcessHeap(), 0,
                                 bufferSize + sizeof(WCHAR) + sizeof(DWORD));

    /*
     * If the memory allocation failed, return a null pointer.
     */
    if (newBuffer==0)
      return 0;

    /*
     * Copy the length of the string in the placeholder.
     */
    *newBuffer = bufferSize;

    /*
     * Skip the byte count.
     */
    newBuffer++;

    /*
     * Copy the information in the buffer.  It is not possible to pass 
     * a NULL pointer here. 
     */
    memcpy(newBuffer, in, bufferSize);

    /*
     * Make sure that there is a nul character at the end of the
     * string.
     */
    stringBuffer = (WCHAR*)newBuffer;
    stringBuffer[len] = 0;

    return (LPWSTR)stringBuffer;
}

/* this code is from SysFreeString (ole2disp.c in oleaut32)*/
static VOID WINAPI ERRORINFO_SysFreeString(BSTR in)
{
    DWORD* bufferPointer;

    /* NULL is a valid parameter */
    if(!in) return;

    /*
     * We have to be careful when we free a BSTR pointer, it points to
     * the beginning of the string but it skips the byte count contained
     * before the string.
     */
    bufferPointer = (DWORD*)in;

    bufferPointer--;

    /*
     * Free the memory from it's "real" origin.
     */
    HeapFree(GetProcessHeap(), 0, bufferPointer);
}


typedef struct ErrorInfoImpl
{
	const IErrorInfoVtbl           *lpvtei;
	const ICreateErrorInfoVtbl     *lpvtcei;
	const ISupportErrorInfoVtbl    *lpvtsei;
	LONG				ref;

	GUID m_Guid;
	BSTR bstrSource;
	BSTR bstrDescription;
	BSTR bstrHelpFile;
	DWORD m_dwHelpContext;
} ErrorInfoImpl;

static const IErrorInfoVtbl        IErrorInfoImpl_VTable;
static const ICreateErrorInfoVtbl  ICreateErrorInfoImpl_VTable;
static const ISupportErrorInfoVtbl ISupportErrorInfoImpl_VTable;

/*
 converts an object pointer to This
 */

static inline ErrorInfoImpl *impl_from_IErrorInfo( IErrorInfo *iface )
{
    return (ErrorInfoImpl *)((char*)iface - FIELD_OFFSET(ErrorInfoImpl, lpvtei));
}

static inline ErrorInfoImpl *impl_from_ICreateErrorInfo( ICreateErrorInfo *iface )
{
    return (ErrorInfoImpl *)((char*)iface - FIELD_OFFSET(ErrorInfoImpl, lpvtcei));
}

static inline ErrorInfoImpl *impl_from_ISupportErrorInfo( ISupportErrorInfo *iface )
{
    return (ErrorInfoImpl *)((char*)iface - FIELD_OFFSET(ErrorInfoImpl, lpvtsei));
}


/*
 converts This to an object pointer
 */
#define _IErrorInfo_(This)		(IErrorInfo*)&(This->lpvtei)
#define _ICreateErrorInfo_(This)	(ICreateErrorInfo*)&(This->lpvtcei)
#define _ISupportErrorInfo_(This)	(ISupportErrorInfo*)&(This->lpvtsei)

static IErrorInfo * IErrorInfoImpl_Constructor(void)
{
	ErrorInfoImpl * ei = HeapAlloc(GetProcessHeap(), 0, sizeof(ErrorInfoImpl));
	if (ei)
	{
	  ei->lpvtei = &IErrorInfoImpl_VTable;
	  ei->lpvtcei = &ICreateErrorInfoImpl_VTable;
	  ei->lpvtsei = &ISupportErrorInfoImpl_VTable;
	  ei->ref = 1;
	  ei->bstrSource = NULL;
	  ei->bstrDescription = NULL;
	  ei->bstrHelpFile = NULL;
	  ei->m_dwHelpContext = 0;
	}
	return (IErrorInfo *)ei;
}


static HRESULT WINAPI IErrorInfoImpl_QueryInterface(
	IErrorInfo* iface,
	REFIID     riid,
	VOID**     ppvoid)
{
	ErrorInfoImpl *This = impl_from_IErrorInfo(iface);
	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvoid);

	*ppvoid = NULL;

	if(IsEqualIID(riid, &IID_IErrorInfo))
	{
	  *ppvoid = _IErrorInfo_(This);
	}
	else if(IsEqualIID(riid, &IID_ICreateErrorInfo))
	{
	  *ppvoid = _ICreateErrorInfo_(This);
	}
	else if(IsEqualIID(riid, &IID_ISupportErrorInfo))
	{
	  *ppvoid = _ISupportErrorInfo_(This);
	}

	if(*ppvoid)
	{
	  IUnknown_AddRef( (IUnknown*)*ppvoid );
	  TRACE("-- Interface: (%p)->(%p)\n",ppvoid,*ppvoid);
	  return S_OK;
	}
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

static ULONG WINAPI IErrorInfoImpl_AddRef(
 	IErrorInfo* iface)
{
	ErrorInfoImpl *This = impl_from_IErrorInfo(iface);
	TRACE("(%p)->(count=%u)\n",This,This->ref);
	return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IErrorInfoImpl_Release(
	IErrorInfo* iface)
{
	ErrorInfoImpl *This = impl_from_IErrorInfo(iface);
        ULONG ref = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(count=%u)\n",This,ref+1);

	if (!ref)
	{
	  TRACE("-- destroying IErrorInfo(%p)\n",This);
	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return ref;
}

static HRESULT WINAPI IErrorInfoImpl_GetGUID(
	IErrorInfo* iface,
	GUID * pGUID)
{
	ErrorInfoImpl *This = impl_from_IErrorInfo(iface);
	TRACE("(%p)->(count=%u)\n",This,This->ref);
	if(!pGUID )return E_INVALIDARG;
	*pGUID = This->m_Guid;
	return S_OK;
}

static HRESULT WINAPI IErrorInfoImpl_GetSource(
	IErrorInfo* iface,
	BSTR *pBstrSource)
{
	ErrorInfoImpl *This = impl_from_IErrorInfo(iface);
	TRACE("(%p)->(pBstrSource=%p)\n",This,pBstrSource);
	if (pBstrSource == NULL)
	    return E_INVALIDARG;
	*pBstrSource = ERRORINFO_SysAllocString(This->bstrSource);
	return S_OK;
}

static HRESULT WINAPI IErrorInfoImpl_GetDescription(
	IErrorInfo* iface,
	BSTR *pBstrDescription)
{
	ErrorInfoImpl *This = impl_from_IErrorInfo(iface);

	TRACE("(%p)->(pBstrDescription=%p)\n",This,pBstrDescription);
	if (pBstrDescription == NULL)
	    return E_INVALIDARG;
	*pBstrDescription = ERRORINFO_SysAllocString(This->bstrDescription);

	return S_OK;
}

static HRESULT WINAPI IErrorInfoImpl_GetHelpFile(
	IErrorInfo* iface,
	BSTR *pBstrHelpFile)
{
	ErrorInfoImpl *This = impl_from_IErrorInfo(iface);

	TRACE("(%p)->(pBstrHelpFile=%p)\n",This, pBstrHelpFile);
	if (pBstrHelpFile == NULL)
	    return E_INVALIDARG;
	*pBstrHelpFile = ERRORINFO_SysAllocString(This->bstrHelpFile);

	return S_OK;
}

static HRESULT WINAPI IErrorInfoImpl_GetHelpContext(
	IErrorInfo* iface,
	DWORD *pdwHelpContext)
{
	ErrorInfoImpl *This = impl_from_IErrorInfo(iface);
	TRACE("(%p)->(pdwHelpContext=%p)\n",This, pdwHelpContext);
	if (pdwHelpContext == NULL)
	    return E_INVALIDARG;
	*pdwHelpContext = This->m_dwHelpContext;

	return S_OK;
}

static const IErrorInfoVtbl IErrorInfoImpl_VTable =
{
  IErrorInfoImpl_QueryInterface,
  IErrorInfoImpl_AddRef,
  IErrorInfoImpl_Release,

  IErrorInfoImpl_GetGUID,
  IErrorInfoImpl_GetSource,
  IErrorInfoImpl_GetDescription,
  IErrorInfoImpl_GetHelpFile,
  IErrorInfoImpl_GetHelpContext
};


static HRESULT WINAPI ICreateErrorInfoImpl_QueryInterface(
	ICreateErrorInfo* iface,
	REFIID     riid,
	VOID**     ppvoid)
{
	ErrorInfoImpl *This = impl_from_ICreateErrorInfo(iface);
	TRACE("(%p)\n", This);
	return IErrorInfo_QueryInterface(_IErrorInfo_(This), riid, ppvoid);
}

static ULONG WINAPI ICreateErrorInfoImpl_AddRef(
 	ICreateErrorInfo* iface)
{
	ErrorInfoImpl *This = impl_from_ICreateErrorInfo(iface);
	TRACE("(%p)\n", This);
	return IErrorInfo_AddRef(_IErrorInfo_(This));
}

static ULONG WINAPI ICreateErrorInfoImpl_Release(
	ICreateErrorInfo* iface)
{
	ErrorInfoImpl *This = impl_from_ICreateErrorInfo(iface);
	TRACE("(%p)\n", This);
	return IErrorInfo_Release(_IErrorInfo_(This));
}


static HRESULT WINAPI ICreateErrorInfoImpl_SetGUID(
	ICreateErrorInfo* iface,
	REFGUID rguid)
{
	ErrorInfoImpl *This = impl_from_ICreateErrorInfo(iface);
	TRACE("(%p)->(%s)\n", This, debugstr_guid(rguid));
	This->m_Guid = *rguid;
	return S_OK;
}

static HRESULT WINAPI ICreateErrorInfoImpl_SetSource(
	ICreateErrorInfo* iface,
	LPOLESTR szSource)
{
	ErrorInfoImpl *This = impl_from_ICreateErrorInfo(iface);
	TRACE("(%p): %s\n",This, debugstr_w(szSource));
	if (This->bstrSource != NULL)
	    ERRORINFO_SysFreeString(This->bstrSource);
	This->bstrSource = ERRORINFO_SysAllocString(szSource);

	return S_OK;
}

static HRESULT WINAPI ICreateErrorInfoImpl_SetDescription(
	ICreateErrorInfo* iface,
	LPOLESTR szDescription)
{
	ErrorInfoImpl *This = impl_from_ICreateErrorInfo(iface);
	TRACE("(%p): %s\n",This, debugstr_w(szDescription));
	if (This->bstrDescription != NULL)
	    ERRORINFO_SysFreeString(This->bstrDescription);
	This->bstrDescription = ERRORINFO_SysAllocString(szDescription);

	return S_OK;
}

static HRESULT WINAPI ICreateErrorInfoImpl_SetHelpFile(
	ICreateErrorInfo* iface,
	LPOLESTR szHelpFile)
{
	ErrorInfoImpl *This = impl_from_ICreateErrorInfo(iface);
	TRACE("(%p,%s)\n",This,debugstr_w(szHelpFile));
	if (This->bstrHelpFile != NULL)
	    ERRORINFO_SysFreeString(This->bstrHelpFile);
	This->bstrHelpFile = ERRORINFO_SysAllocString(szHelpFile);
	return S_OK;
}

static HRESULT WINAPI ICreateErrorInfoImpl_SetHelpContext(
	ICreateErrorInfo* iface,
 	DWORD dwHelpContext)
{
	ErrorInfoImpl *This = impl_from_ICreateErrorInfo(iface);
	TRACE("(%p,%d)\n",This,dwHelpContext);
	This->m_dwHelpContext = dwHelpContext;
	return S_OK;
}

static const ICreateErrorInfoVtbl ICreateErrorInfoImpl_VTable =
{
  ICreateErrorInfoImpl_QueryInterface,
  ICreateErrorInfoImpl_AddRef,
  ICreateErrorInfoImpl_Release,

  ICreateErrorInfoImpl_SetGUID,
  ICreateErrorInfoImpl_SetSource,
  ICreateErrorInfoImpl_SetDescription,
  ICreateErrorInfoImpl_SetHelpFile,
  ICreateErrorInfoImpl_SetHelpContext
};

static HRESULT WINAPI ISupportErrorInfoImpl_QueryInterface(
	ISupportErrorInfo* iface,
	REFIID     riid,
	VOID**     ppvoid)
{
	ErrorInfoImpl *This = impl_from_ISupportErrorInfo(iface);
	TRACE("(%p)\n", This);

	return IErrorInfo_QueryInterface(_IErrorInfo_(This), riid, ppvoid);
}

static ULONG WINAPI ISupportErrorInfoImpl_AddRef(
 	ISupportErrorInfo* iface)
{
	ErrorInfoImpl *This = impl_from_ISupportErrorInfo(iface);
	TRACE("(%p)\n", This);
	return IErrorInfo_AddRef(_IErrorInfo_(This));
}

static ULONG WINAPI ISupportErrorInfoImpl_Release(
	ISupportErrorInfo* iface)
{
	ErrorInfoImpl *This = impl_from_ISupportErrorInfo(iface);
	TRACE("(%p)\n", This);
	return IErrorInfo_Release(_IErrorInfo_(This));
}


static HRESULT WINAPI ISupportErrorInfoImpl_InterfaceSupportsErrorInfo(
	ISupportErrorInfo* iface,
	REFIID riid)
{
	ErrorInfoImpl *This = impl_from_ISupportErrorInfo(iface);
	TRACE("(%p)->(%s)\n", This, debugstr_guid(riid));
	return (IsEqualIID(riid, &This->m_Guid)) ? S_OK : S_FALSE;
}

static const ISupportErrorInfoVtbl ISupportErrorInfoImpl_VTable =
{
  ISupportErrorInfoImpl_QueryInterface,
  ISupportErrorInfoImpl_AddRef,
  ISupportErrorInfoImpl_Release,


  ISupportErrorInfoImpl_InterfaceSupportsErrorInfo
};

/***********************************************************************
 *		CreateErrorInfo (OLE32.@)
 *
 * Creates an object used to set details for an error info object.
 *
 * PARAMS
 *  pperrinfo [O]. Address where error info creation object will be stored.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 */
HRESULT WINAPI CreateErrorInfo(ICreateErrorInfo **pperrinfo)
{
	IErrorInfo * pei;
	HRESULT res;
	TRACE("(%p)\n", pperrinfo);
	if(! pperrinfo ) return E_INVALIDARG;
	if(!(pei=IErrorInfoImpl_Constructor()))return E_OUTOFMEMORY;

	res = IErrorInfo_QueryInterface(pei, &IID_ICreateErrorInfo, (LPVOID*)pperrinfo);
	IErrorInfo_Release(pei);
	return res;
}

/***********************************************************************
 *		GetErrorInfo (OLE32.@)
 *
 * Retrieves the error information object for the current thread.
 *
 * PARAMS
 *  dwReserved [I]. Reserved. Must be zero.
 *  pperrinfo  [O]. Address where error information object will be stored on return.
 *
 * RETURNS
 *  Success: S_OK if an error information object was set for the current thread.
 *           S_FALSE if otherwise.
 *  Failure: E_INVALIDARG if dwReserved is not zero.
 *
 * NOTES
 *  This function causes the current error info object for the thread to be
 *  cleared if one was set beforehand.
 */
HRESULT WINAPI GetErrorInfo(ULONG dwReserved, IErrorInfo **pperrinfo)
{
	TRACE("(%d, %p, %p)\n", dwReserved, pperrinfo, COM_CurrentInfo()->errorinfo);

	if (dwReserved)
	{
		ERR("dwReserved (0x%x) != 0\n", dwReserved);
		return E_INVALIDARG;
	}

	if(!pperrinfo) return E_INVALIDARG;

	if (!COM_CurrentInfo()->errorinfo)
	{
	   *pperrinfo = NULL;
	   return S_FALSE;
	}

	*pperrinfo = COM_CurrentInfo()->errorinfo;
        
	/* clear thread error state */
	COM_CurrentInfo()->errorinfo = NULL;
	return S_OK;
}

/***********************************************************************
 *		SetErrorInfo (OLE32.@)
 *
 * Sets the error information object for the current thread.
 *
 * PARAMS
 *  dwReserved [I] Reserved. Must be zero.
 *  perrinfo   [I] Error info object.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG if dwReserved is not zero.
 */
HRESULT WINAPI SetErrorInfo(ULONG dwReserved, IErrorInfo *perrinfo)
{
	IErrorInfo * pei;

	TRACE("(%d, %p)\n", dwReserved, perrinfo);

	if (dwReserved)
	{
		ERR("dwReserved (0x%x) != 0\n", dwReserved);
		return E_INVALIDARG;
	}

	/* release old errorinfo */
	pei = COM_CurrentInfo()->errorinfo;
	if (pei) IErrorInfo_Release(pei);

	/* set to new value */
	COM_CurrentInfo()->errorinfo = perrinfo;
	if (perrinfo) IErrorInfo_AddRef(perrinfo);
        
	return S_OK;
}
