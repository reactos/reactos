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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * NOTES:
 *
 * The errorinfo is a per-thread object. The reference is stored in the
 * TEB at offset 0xf80
 */

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "oleauto.h"
#include "winerror.h"

#include "objbase.h"
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
     * Find the lenth of the buffer passed-in in bytes.
     */
    len = strlenW(in);
    bufferSize = len * sizeof (WCHAR);

    /*
     * Allocate a new buffer to hold the string.
     * don't forget to keep an empty spot at the beginning of the
     * buffer for the character count and an extra character at the
     * end for the '\0'.
     */
    newBuffer = (DWORD*)HeapAlloc(GetProcessHeap(),
                                 0,
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
     * Copy the information in the buffer.
     * Since it is valid to pass a NULL pointer here, we'll initialize the
     * buffer to nul if it is the case.
     */
    if (in != 0)
      memcpy(newBuffer, in, bufferSize);
    else
      memset(newBuffer, 0, bufferSize);

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
	ICOM_VTABLE(IErrorInfo)		*lpvtei;
	ICOM_VTABLE(ICreateErrorInfo)	*lpvtcei;
	ICOM_VTABLE(ISupportErrorInfo)	*lpvtsei;
	DWORD				ref;

	GUID m_Guid;
	BSTR bstrSource;
	BSTR bstrDescription;
	BSTR bstrHelpFile;
	DWORD m_dwHelpContext;
} ErrorInfoImpl;

static ICOM_VTABLE(IErrorInfo)		IErrorInfoImpl_VTable;
static ICOM_VTABLE(ICreateErrorInfo)	ICreateErrorInfoImpl_VTable;
static ICOM_VTABLE(ISupportErrorInfo)	ISupportErrorInfoImpl_VTable;

/*
 converts a objectpointer to This
 */
#define _IErrorInfo_Offset ((int)(&(((ErrorInfoImpl*)0)->lpvtei)))
#define _ICOM_THIS_From_IErrorInfo(class, name) class* This = (class*)(((char*)name)-_IErrorInfo_Offset);

#define _ICreateErrorInfo_Offset ((int)(&(((ErrorInfoImpl*)0)->lpvtcei)))
#define _ICOM_THIS_From_ICreateErrorInfo(class, name) class* This = (class*)(((char*)name)-_ICreateErrorInfo_Offset);

#define _ISupportErrorInfo_Offset ((int)(&(((ErrorInfoImpl*)0)->lpvtsei)))
#define _ICOM_THIS_From_ISupportErrorInfo(class, name) class* This = (class*)(((char*)name)-_ISupportErrorInfo_Offset);

/*
 converts This to a objectpointer
 */
#define _IErrorInfo_(This)		(IErrorInfo*)&(This->lpvtei)
#define _ICreateErrorInfo_(This)	(ICreateErrorInfo*)&(This->lpvtcei)
#define _ISupportErrorInfo_(This)	(ISupportErrorInfo*)&(This->lpvtsei)

IErrorInfo * IErrorInfoImpl_Constructor()
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
	_ICOM_THIS_From_IErrorInfo(ErrorInfoImpl, iface);
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
	_ICOM_THIS_From_IErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)->(count=%lu)\n",This,This->ref);
	return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IErrorInfoImpl_Release(
	IErrorInfo* iface)
{
	_ICOM_THIS_From_IErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	if (!InterlockedDecrement(&This->ref))
	{
	  TRACE("-- destroying IErrorInfo(%p)\n",This);
	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}

static HRESULT WINAPI IErrorInfoImpl_GetGUID(
	IErrorInfo* iface,
	GUID * pGUID)
{
	_ICOM_THIS_From_IErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)->(count=%lu)\n",This,This->ref);
	if(!pGUID )return E_INVALIDARG;
	memcpy(pGUID, &This->m_Guid, sizeof(GUID));
	return S_OK;
}

static HRESULT WINAPI IErrorInfoImpl_GetSource(
	IErrorInfo* iface,
	BSTR *pBstrSource)
{
	_ICOM_THIS_From_IErrorInfo(ErrorInfoImpl, iface);
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
	_ICOM_THIS_From_IErrorInfo(ErrorInfoImpl, iface);

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
	_ICOM_THIS_From_IErrorInfo(ErrorInfoImpl, iface);

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
	_ICOM_THIS_From_IErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)->(pdwHelpContext=%p)\n",This, pdwHelpContext);
	if (pdwHelpContext == NULL)
	    return E_INVALIDARG;
	*pdwHelpContext = This->m_dwHelpContext;

	return S_OK;
}

static ICOM_VTABLE(IErrorInfo) IErrorInfoImpl_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
	_ICOM_THIS_From_ICreateErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)\n", This);
	return IErrorInfo_QueryInterface(_IErrorInfo_(This), riid, ppvoid);
}

static ULONG WINAPI ICreateErrorInfoImpl_AddRef(
 	ICreateErrorInfo* iface)
{
	_ICOM_THIS_From_ICreateErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)\n", This);
	return IErrorInfo_AddRef(_IErrorInfo_(This));
}

static ULONG WINAPI ICreateErrorInfoImpl_Release(
	ICreateErrorInfo* iface)
{
	_ICOM_THIS_From_ICreateErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)\n", This);
	return IErrorInfo_Release(_IErrorInfo_(This));
}


static HRESULT WINAPI ICreateErrorInfoImpl_SetGUID(
	ICreateErrorInfo* iface,
	REFGUID rguid)
{
	_ICOM_THIS_From_ICreateErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)->(%s)\n", This, debugstr_guid(rguid));
	memcpy(&This->m_Guid,  rguid, sizeof(GUID));
	return S_OK;
}

static HRESULT WINAPI ICreateErrorInfoImpl_SetSource(
	ICreateErrorInfo* iface,
	LPOLESTR szSource)
{
	_ICOM_THIS_From_ICreateErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)\n",This);
	if (This->bstrSource != NULL)
	    ERRORINFO_SysFreeString(This->bstrSource);
	This->bstrSource = ERRORINFO_SysAllocString(szSource);

	return S_OK;
}

static HRESULT WINAPI ICreateErrorInfoImpl_SetDescription(
	ICreateErrorInfo* iface,
	LPOLESTR szDescription)
{
	_ICOM_THIS_From_ICreateErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)\n",This);
	if (This->bstrDescription != NULL)
	    ERRORINFO_SysFreeString(This->bstrDescription);
	This->bstrDescription = ERRORINFO_SysAllocString(szDescription);

	return S_OK;
}

static HRESULT WINAPI ICreateErrorInfoImpl_SetHelpFile(
	ICreateErrorInfo* iface,
	LPOLESTR szHelpFile)
{
	_ICOM_THIS_From_ICreateErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)\n",This);
	if (This->bstrHelpFile != NULL)
	    ERRORINFO_SysFreeString(This->bstrHelpFile);
	This->bstrHelpFile = ERRORINFO_SysAllocString(szHelpFile);

	return S_OK;
}

static HRESULT WINAPI ICreateErrorInfoImpl_SetHelpContext(
	ICreateErrorInfo* iface,
 	DWORD dwHelpContext)
{
	_ICOM_THIS_From_ICreateErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)\n",This);
	This->m_dwHelpContext = dwHelpContext;

	return S_OK;
}

static ICOM_VTABLE(ICreateErrorInfo) ICreateErrorInfoImpl_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
	_ICOM_THIS_From_ISupportErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)\n", This);

	return IErrorInfo_QueryInterface(_IErrorInfo_(This), riid, ppvoid);
}

static ULONG WINAPI ISupportErrorInfoImpl_AddRef(
 	ISupportErrorInfo* iface)
{
	_ICOM_THIS_From_ISupportErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)\n", This);
	return IErrorInfo_AddRef(_IErrorInfo_(This));
}

static ULONG WINAPI ISupportErrorInfoImpl_Release(
	ISupportErrorInfo* iface)
{
	_ICOM_THIS_From_ISupportErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)\n", This);
	return IErrorInfo_Release(_IErrorInfo_(This));
}


static HRESULT WINAPI ISupportErrorInfoImpl_InterfaceSupportsErrorInfo(
	ISupportErrorInfo* iface,
	REFIID riid)
{
	_ICOM_THIS_From_ISupportErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)->(%s)\n", This, debugstr_guid(riid));
	return (IsEqualIID(riid, &This->m_Guid)) ? S_OK : S_FALSE;
}

static ICOM_VTABLE(ISupportErrorInfo) ISupportErrorInfoImpl_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  ISupportErrorInfoImpl_QueryInterface,
  ISupportErrorInfoImpl_AddRef,
  ISupportErrorInfoImpl_Release,


  ISupportErrorInfoImpl_InterfaceSupportsErrorInfo
};
/***********************************************************************
 *		CreateErrorInfo (OLE32.@)
 */
HRESULT WINAPI CreateErrorInfo(ICreateErrorInfo **pperrinfo)
{
	IErrorInfo * pei;
	HRESULT res;
	TRACE("(%p): stub:\n", pperrinfo);
	if(! pperrinfo ) return E_INVALIDARG;
	if(!(pei=IErrorInfoImpl_Constructor()))return E_OUTOFMEMORY;

	res = IErrorInfo_QueryInterface(pei, &IID_ICreateErrorInfo, (LPVOID*)pperrinfo);
	IErrorInfo_Release(pei);
	return res;
}

/***********************************************************************
 *		GetErrorInfo (OLE32.@)
 */
HRESULT WINAPI GetErrorInfo(ULONG dwReserved, IErrorInfo **pperrinfo)
{
	APARTMENT * apt = COM_CurrentInfo();

	TRACE("(%ld, %p, %p)\n", dwReserved, pperrinfo, COM_CurrentInfo()->ErrorInfo);

	if(!pperrinfo) return E_INVALIDARG;
	if (!apt || !apt->ErrorInfo)
	{
	   *pperrinfo = NULL;
	   return S_FALSE;
	}

	*pperrinfo = (IErrorInfo*)(apt->ErrorInfo);
	/* clear thread error state */
	apt->ErrorInfo = NULL;
	return S_OK;
}

/***********************************************************************
 *		SetErrorInfo (OLE32.@)
 */
HRESULT WINAPI SetErrorInfo(ULONG dwReserved, IErrorInfo *perrinfo)
{
	IErrorInfo * pei;
	APARTMENT * apt = COM_CurrentInfo();

	TRACE("(%ld, %p)\n", dwReserved, perrinfo);
	
	if (!apt) apt = COM_CreateApartment(COINIT_UNINITIALIZED);

	/* release old errorinfo */
	pei = (IErrorInfo*)apt->ErrorInfo;
	if(pei) IErrorInfo_Release(pei);

	/* set to new value */
	apt->ErrorInfo = perrinfo;
	if(perrinfo) IErrorInfo_AddRef(perrinfo);
	return S_OK;
}
