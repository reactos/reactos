/*
 * UrlMon
 *
 * Copyright 1999 Ulrich Czekalla for Corel Corporation
 * Copyright 2002 Huw D M Davies for CodeWeavers
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

#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "winuser.h"
#include "objbase.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "ole2.h"
#include "urlmon.h"
#include "wininet.h"
#include "shlwapi.h"
#include "urlmon_main.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

/* native urlmon.dll uses this key, too */
static const WCHAR BSCBHolder[] = { '_','B','S','C','B','_','H','o','l','d','e','r','_',0 };

/*static BOOL registered_wndclass = FALSE;*/

/* filemoniker data structure */
typedef struct URLMonikerImpl{

    IMonikerVtbl*  lpvtbl1;  /* VTable relative to the IMoniker interface.*/
    IBindingVtbl*  lpvtbl2;  /* VTable to IBinding interface */

    ULONG ref; /* reference counter for this object */

    LPOLESTR URLName; /* URL string identified by this URLmoniker */

    HWND hwndCallback;
    IBindCtx *pBC;
    HINTERNET hinternet, hconnect, hrequest;
} URLMonikerImpl;

/*******************************************************************************
 *        URLMoniker_QueryInterface
 *******************************************************************************/
static HRESULT WINAPI URLMonikerImpl_QueryInterface(IMoniker* iface,REFIID riid,void** ppvObject)
{
    URLMonikerImpl *This = (URLMonikerImpl *)iface;

    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppvObject);

    /* Perform a sanity check on the parameters.*/
    if ( (This==0) || (ppvObject==0) )
	return E_INVALIDARG;

    /* Initialize the return parameter */
    *ppvObject = 0;

    /* Compare the riid with the interface IDs implemented by this object.*/
    if (IsEqualIID(&IID_IUnknown, riid)      ||
        IsEqualIID(&IID_IPersist, riid)      ||
        IsEqualIID(&IID_IPersistStream,riid) ||
        IsEqualIID(&IID_IMoniker, riid)
       )
        *ppvObject = iface;

    /* Check that we obtained an interface.*/
    if ((*ppvObject)==0)
        return E_NOINTERFACE;

    /* Query Interface always increases the reference count by one when it is successful */
    IMoniker_AddRef(iface);

    return S_OK;
}

/******************************************************************************
 *        URLMoniker_AddRef
 ******************************************************************************/
static ULONG WINAPI URLMonikerImpl_AddRef(IMoniker* iface)
{
    URLMonikerImpl *This = (URLMonikerImpl *)iface;

    TRACE("(%p)\n",This);

    return InterlockedIncrement(&This->ref);
}

/******************************************************************************
 *        URLMoniker_Release
 ******************************************************************************/
static ULONG WINAPI URLMonikerImpl_Release(IMoniker* iface)
{
    URLMonikerImpl *This = (URLMonikerImpl *)iface;
    ULONG ref;

    TRACE("(%p)\n",This);

    ref = InterlockedDecrement(&This->ref);

    /* destroy the object if there's no more reference on it */
    if (ref == 0) {
        HeapFree(GetProcessHeap(),0,This->URLName);
        HeapFree(GetProcessHeap(),0,This);
    }

    return ref;
}

/******************************************************************************
 *        URLMoniker_GetClassID
 ******************************************************************************/
static HRESULT WINAPI URLMonikerImpl_GetClassID(IMoniker* iface,
						CLSID *pClassID)/* Pointer to CLSID of object */
{
    URLMonikerImpl *This = (URLMonikerImpl *)iface;

    TRACE("(%p,%p)\n",This,pClassID);

    if (pClassID==NULL)
        return E_POINTER;
    /* Windows always returns CLSID_StdURLMoniker */
    *pClassID = CLSID_StdURLMoniker;
    return S_OK;
}

/******************************************************************************
 *        URLMoniker_IsDirty
 ******************************************************************************/
static HRESULT WINAPI URLMonikerImpl_IsDirty(IMoniker* iface)
{
    URLMonikerImpl *This = (URLMonikerImpl *)iface;
    /* Note that the OLE-provided implementations of the IPersistStream::IsDirty
       method in the OLE-provided moniker interfaces always return S_FALSE because
       their internal state never changes. */

    TRACE("(%p)\n",This);

    return S_FALSE;
}

/******************************************************************************
 *        URLMoniker_Load
 *
 * NOTE
 *  Writes a ULONG containing length of unicode string, followed
 *  by that many unicode characters
 ******************************************************************************/
static HRESULT WINAPI URLMonikerImpl_Load(IMoniker* iface,IStream* pStm)
{
    URLMonikerImpl *This = (URLMonikerImpl *)iface;
    
    HRESULT res;
    ULONG len;
    ULONG got;
    TRACE("(%p,%p)\n",This,pStm);

    if(!pStm)
        return E_INVALIDARG;

    res = IStream_Read(pStm, &len, sizeof(ULONG), &got);
    if(SUCCEEDED(res)) {
        if(got == sizeof(ULONG)) {
            HeapFree(GetProcessHeap(), 0, This->URLName);
            This->URLName=HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*(len+1));
            if(!This->URLName)
                res = E_OUTOFMEMORY;
            else {
                res = IStream_Read(pStm, This->URLName, len, NULL);
                This->URLName[len] = 0;
            }
        }
        else
            res = E_FAIL;
    }
    return res;
}

/******************************************************************************
 *        URLMoniker_Save
 ******************************************************************************/
static HRESULT WINAPI URLMonikerImpl_Save(IMoniker* iface,
					  IStream* pStm,/* pointer to the stream where the object is to be saved */
					  BOOL fClearDirty)/* Specifies whether to clear the dirty flag */
{
    URLMonikerImpl *This = (URLMonikerImpl *)iface;

    HRESULT res;
    ULONG len;
    TRACE("(%p,%p,%d)\n",This,pStm,fClearDirty);

    if(!pStm)
        return E_INVALIDARG;

    len = strlenW(This->URLName);
    res=IStream_Write(pStm,&len,sizeof(ULONG),NULL);
    if(SUCCEEDED(res))
        res=IStream_Write(pStm,&This->URLName,len*sizeof(WCHAR),NULL);
    return res;

}

/******************************************************************************
 *        URLMoniker_GetSizeMax
 ******************************************************************************/
static HRESULT WINAPI URLMonikerImpl_GetSizeMax(IMoniker* iface,
						ULARGE_INTEGER* pcbSize)/* Pointer to size of stream needed to save object */
{
    URLMonikerImpl *This = (URLMonikerImpl *)iface;

    TRACE("(%p,%p)\n",This,pcbSize);

    if(!pcbSize)
        return E_INVALIDARG;

    pcbSize->u.LowPart = sizeof(ULONG) + (strlenW(This->URLName) * sizeof(WCHAR));
    pcbSize->u.HighPart = 0;
    return S_OK;
}

/******************************************************************************
 *                  URLMoniker_BindToObject
 ******************************************************************************/
static HRESULT WINAPI URLMonikerImpl_BindToObject(IMoniker* iface,
						  IBindCtx* pbc,
						  IMoniker* pmkToLeft,
						  REFIID riid,
						  VOID** ppvResult)
{
    URLMonikerImpl *This = (URLMonikerImpl *)iface;

    *ppvResult=0;

    FIXME("(%p)->(%p,%p,%s,%p): stub\n",This,pbc,pmkToLeft,debugstr_guid(riid),
	  ppvResult);

    return E_NOTIMPL;
}

typedef struct {
    enum {OnProgress, OnDataAvailable} callback;
} URLMON_CallbackData;


#if 0
static LRESULT CALLBACK URLMON_WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static void PostOnProgress(URLMonikerImpl *This, UINT progress, UINT maxprogress, DWORD status, LPCWSTR *str)
{
}

static void CALLBACK URLMON_InternetCallback(HINTERNET hinet, /*DWORD_PTR*/ DWORD context, DWORD status,
					     void *status_info, DWORD status_info_len)
{
    URLMonikerImpl *This = (URLMonikerImpl *)context;
    TRACE("handle %p this %p status %08lx\n", hinet, This, status);

    if(This->filesize == -1) {
	switch(status) {
	case INTERNET_STATUS_RESOLVING_NAME:
	    PostOnProgess(This, 0, 0, BINDSTATUS_FINDINGRESOURCE, status_info);
	    break;
	case INTERNET_STATUS_CONNECTING_TO_SERVER:
	    PostOnProgress(This, 0, 0, BINDSTATUS_CONNECTING, NULL);
	    break;
	case INTERNET_STATUS_SENDING_REQUEST:
	    PostOnProgress(This, 0, 0, BINDSTATUS_SENDINGREQUEST, NULL);
	    break;
	case INTERNET_REQUEST_COMPLETE:
	  {
	      DWORD len, lensz = sizeof(len);

	    HttpQueryInfoW(hrequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &len, &lensz, NULL);
	    TRACE("res = %ld gle = %08lx url len = %ld\n", hres, GetLastError(), len);
	    This->filesize = len;
	    break;
	  }
	}
    }

    return;
}
#endif
/******************************************************************************
 *        URLMoniker_BindToStorage
 ******************************************************************************/
static HRESULT WINAPI URLMonikerImpl_BindToStorage(IMoniker* iface,
						   IBindCtx* pbc,
						   IMoniker* pmkToLeft,
						   REFIID riid,
						   VOID** ppvObject)
{
    URLMonikerImpl *This = (URLMonikerImpl *)iface;
    HRESULT hres;
    IBindStatusCallback *pbscb;
    BINDINFO bi;
    DWORD bindf;
    IStream *pstr;

    FIXME("(%p)->(%p,%p,%s,%p): stub\n",This,pbc,pmkToLeft,debugstr_guid(riid),ppvObject);
    if(pmkToLeft) {
	FIXME("pmkToLeft != NULL\n");
	return E_NOTIMPL;
    }
    if(!IsEqualIID(&IID_IStream, riid)) {
	FIXME("unsupported iid\n");
	return E_NOTIMPL;
    }

    /* FIXME This is a bad hack (tm).  We should clearly download to a temporary file.
       We also need to implement IStream ourselves so that IStream_Read can return
       E_PENDING */

    hres = CreateStreamOnHGlobal(0, TRUE, &pstr);

    if(SUCCEEDED(hres)) {
        TRACE("Created dummy stream...\n");

        hres = IBindCtx_GetObjectParam(pbc, (LPOLESTR)BSCBHolder, (IUnknown**)&pbscb);
        if(SUCCEEDED(hres)) {
            TRACE("Got IBindStatusCallback...\n");

            memset(&bi, 0, sizeof(bi));
            bi.cbSize = sizeof(bi);
            bindf = 0;
            hres = IBindStatusCallback_GetBindInfo(pbscb, &bindf, &bi);
            if(SUCCEEDED(hres)) {
                URL_COMPONENTSW url;
                WCHAR *host, *path;
                DWORD len, lensz = sizeof(len), total_read = 0;
                LARGE_INTEGER last_read_pos;
                FORMATETC fmt;
                STGMEDIUM stg;

                TRACE("got bindinfo. bindf = %08lx extrainfo = %s bindinfof = %08lx bindverb = %08lx iid %s\n",
                      bindf, debugstr_w(bi.szExtraInfo), bi.grfBindInfoF, bi.dwBindVerb, debugstr_guid(&bi.iid));
                hres = IBindStatusCallback_OnStartBinding(pbscb, 0, (IBinding*)&This->lpvtbl2);
                TRACE("OnStartBinding rets %08lx\n", hres);

#if 0
		if(!registered_wndclass) {
                    WNDCLASSA urlmon_wndclass = {0, URLMON_WndProc,0, 0, URLMON_hInstance, 0, 0, 0, NULL, "URLMON_Callback_Window_Class"};
		    RegisterClassA(&urlmon_wndclass);
		    registered_wndclass = TRUE;
		}

		This->hwndCallback = CreateWindowA("URLMON_Callback_Window_Class", NULL, 0, 0, 0, 0, 0, 0, 0,
						   URLMON_hInstance, NULL);

#endif
                memset(&url, 0, sizeof(url));
                url.dwStructSize = sizeof(url);
                url.dwSchemeLength = url.dwHostNameLength = url.dwUrlPathLength = 1;
                InternetCrackUrlW(This->URLName, 0, 0, &url);
                host = HeapAlloc(GetProcessHeap(), 0, (url.dwHostNameLength + 1) * sizeof(WCHAR));
                memcpy(host, url.lpszHostName, url.dwHostNameLength * sizeof(WCHAR));
                host[url.dwHostNameLength] = '\0';
                path = HeapAlloc(GetProcessHeap(), 0, (url.dwUrlPathLength + 1) * sizeof(WCHAR));
                memcpy(path, url.lpszUrlPath, url.dwUrlPathLength * sizeof(WCHAR));
                path[url.dwUrlPathLength] = '\0';

                This->hinternet = InternetOpenA("User Agent", 0, NULL, NULL, 0 /*INTERNET_FLAG_ASYNC*/);
/*              InternetSetStatusCallback(This->hinternet, URLMON_InternetCallback);*/

                This->hconnect = InternetConnectW(This->hinternet, host, INTERNET_DEFAULT_HTTP_PORT, NULL, NULL,
                                                  INTERNET_SERVICE_HTTP, 0, (DWORD)This);
                This->hrequest = HttpOpenRequestW(This->hconnect, NULL, path, NULL, NULL, NULL, 0, (DWORD)This);

                hres = IBindStatusCallback_OnProgress(pbscb, 0, 0, 0x22, NULL);
                hres = IBindStatusCallback_OnProgress(pbscb, 0, 0, BINDSTATUS_FINDINGRESOURCE, NULL);
                hres = IBindStatusCallback_OnProgress(pbscb, 0, 0, BINDSTATUS_CONNECTING, NULL);
                hres = IBindStatusCallback_OnProgress(pbscb, 0, 0, BINDSTATUS_SENDINGREQUEST, NULL);
                hres = E_OUTOFMEMORY; /* FIXME */
                if(HttpSendRequestW(This->hrequest, NULL, 0, NULL, 0)) {
                    len = 0;
                    HttpQueryInfoW(This->hrequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &len, &lensz, NULL);

                    TRACE("res = %ld gle = %08lx url len = %ld\n", hres, GetLastError(), len);

                    last_read_pos.u.LowPart = last_read_pos.u.HighPart = 0;
                    fmt.cfFormat = 0;
                    fmt.ptd = NULL;
                    fmt.dwAspect = 0;
                    fmt.lindex = -1;
                    fmt.tymed = TYMED_ISTREAM;
                    stg.tymed = TYMED_ISTREAM;
                    stg.u.pstm = pstr;
                    stg.pUnkForRelease = NULL;

                    while(1) {
                        char buf[4096];
                        DWORD bufread;
                        DWORD written;
                        if(InternetReadFile(This->hrequest, buf, sizeof(buf), &bufread)) {
                            TRACE("read %ld bytes %s...\n", bufread, debugstr_an(buf, 10));
                            if(bufread == 0) break;
                            IStream_Write(pstr, buf, bufread, &written);
                            total_read += bufread;
                            IStream_Seek(pstr, last_read_pos, STREAM_SEEK_SET, NULL);
                            hres = IBindStatusCallback_OnProgress(pbscb, total_read, len, (total_read == bufread) ?
                                                                  BINDSTATUS_BEGINDOWNLOADDATA :
                                                                  BINDSTATUS_DOWNLOADINGDATA, NULL);
                            hres = IBindStatusCallback_OnDataAvailable(pbscb,
                                                                       (total_read == bufread) ? BSCF_FIRSTDATANOTIFICATION :
                                                                       BSCF_INTERMEDIATEDATANOTIFICATION,
                                                                       total_read, &fmt, &stg);
                            last_read_pos.u.LowPart += bufread; /* FIXME */
                        } else
                            break;
		    }
                    hres = IBindStatusCallback_OnProgress(pbscb, total_read, len, BINDSTATUS_ENDDOWNLOADDATA, NULL);
                    hres = IBindStatusCallback_OnDataAvailable(pbscb, BSCF_LASTDATANOTIFICATION, total_read, &fmt, &stg);
                    TRACE("OnDataAvail rets %08lx\n", hres);
                    hres = IBindStatusCallback_OnStopBinding(pbscb, S_OK, NULL);
                    TRACE("OnStop rets %08lx\n", hres);
                    hres = S_OK;
                }
                InternetCloseHandle(This->hrequest);
                InternetCloseHandle(This->hconnect);
                InternetCloseHandle(This->hinternet);
                IBindStatusCallback_Release(pbscb);
            }
        }
    }
    *ppvObject = (VOID*)pstr;
    return hres;
}

/******************************************************************************
 *        URLMoniker_Reduce
 ******************************************************************************/
static HRESULT WINAPI URLMonikerImpl_Reduce(IMoniker* iface,
					    IBindCtx* pbc,
					    DWORD dwReduceHowFar,
					    IMoniker** ppmkToLeft,
					    IMoniker** ppmkReduced)
{
    URLMonikerImpl *This = (URLMonikerImpl *)iface;
    
    TRACE("(%p,%p,%ld,%p,%p)\n",This,pbc,dwReduceHowFar,ppmkToLeft,ppmkReduced);

    if(!ppmkReduced)
        return E_INVALIDARG;

    URLMonikerImpl_AddRef(iface);
    *ppmkReduced = iface;
    return MK_S_REDUCED_TO_SELF;
}

/******************************************************************************
 *        URLMoniker_ComposeWith
 ******************************************************************************/
static HRESULT WINAPI URLMonikerImpl_ComposeWith(IMoniker* iface,
						 IMoniker* pmkRight,
						 BOOL fOnlyIfNotGeneric,
						 IMoniker** ppmkComposite)
{
    URLMonikerImpl *This = (URLMonikerImpl *)iface;
    FIXME("(%p)->(%p,%d,%p): stub\n",This,pmkRight,fOnlyIfNotGeneric,ppmkComposite);

    return E_NOTIMPL;
}

/******************************************************************************
 *        URLMoniker_Enum
 ******************************************************************************/
static HRESULT WINAPI URLMonikerImpl_Enum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker)
{
    URLMonikerImpl *This = (URLMonikerImpl *)iface;
    TRACE("(%p,%d,%p)\n",This,fForward,ppenumMoniker);

    if(!ppenumMoniker)
        return E_INVALIDARG;

    /* Does not support sub-monikers */
    *ppenumMoniker = NULL;
    return S_OK;
}

/******************************************************************************
 *        URLMoniker_IsEqual
 ******************************************************************************/
static HRESULT WINAPI URLMonikerImpl_IsEqual(IMoniker* iface,IMoniker* pmkOtherMoniker)
{
    URLMonikerImpl *This = (URLMonikerImpl *)iface;
    CLSID clsid;
    LPOLESTR urlPath;
    IBindCtx* bind;
    HRESULT res;

    TRACE("(%p,%p)\n",This,pmkOtherMoniker);

    if(pmkOtherMoniker==NULL)
        return E_INVALIDARG;

    IMoniker_GetClassID(pmkOtherMoniker,&clsid);

    if(!IsEqualCLSID(&clsid,&CLSID_StdURLMoniker))
        return S_FALSE;

    res = CreateBindCtx(0,&bind);
    if(FAILED(res))
        return res;

    res = S_FALSE;
    if(SUCCEEDED(IMoniker_GetDisplayName(pmkOtherMoniker,bind,NULL,&urlPath))) {
        int result = lstrcmpiW(urlPath, This->URLName);
        CoTaskMemFree(urlPath);
        if(result == 0)
            res = S_OK;
    }
    IUnknown_Release(bind);
    return res;
}


/******************************************************************************
 *        URLMoniker_Hash
 ******************************************************************************/
static HRESULT WINAPI URLMonikerImpl_Hash(IMoniker* iface,DWORD* pdwHash)
{
    URLMonikerImpl *This = (URLMonikerImpl *)iface;
    
    int  h = 0,i,skip,len;
    int  off = 0;
    LPOLESTR val;

    TRACE("(%p,%p)\n",This,pdwHash);

    if(!pdwHash)
        return E_INVALIDARG;

    val = This->URLName;
    len = lstrlenW(val);

    if(len < 16) {
        for(i = len ; i > 0; i--) {
            h = (h * 37) + val[off++];
        }
    }
    else {
        /* only sample some characters */
        skip = len / 8;
        for(i = len; i > 0; i -= skip, off += skip) {
            h = (h * 39) + val[off];
        }
    }
    *pdwHash = h;
    return S_OK;
}

/******************************************************************************
 *        URLMoniker_IsRunning
 ******************************************************************************/
static HRESULT WINAPI URLMonikerImpl_IsRunning(IMoniker* iface,
					       IBindCtx* pbc,
					       IMoniker* pmkToLeft,
					       IMoniker* pmkNewlyRunning)
{
    URLMonikerImpl *This = (URLMonikerImpl *)iface;
    FIXME("(%p)->(%p,%p,%p): stub\n",This,pbc,pmkToLeft,pmkNewlyRunning);

    return E_NOTIMPL;
}

/******************************************************************************
 *        URLMoniker_GetTimeOfLastChange
 ******************************************************************************/
static HRESULT WINAPI URLMonikerImpl_GetTimeOfLastChange(IMoniker* iface,
							 IBindCtx* pbc,
							 IMoniker* pmkToLeft,
							 FILETIME* pFileTime)
{
    URLMonikerImpl *This = (URLMonikerImpl *)iface;
    FIXME("(%p)->(%p,%p,%p): stub\n",This,pbc,pmkToLeft,pFileTime);

    return E_NOTIMPL;
}

/******************************************************************************
 *        URLMoniker_Inverse
 ******************************************************************************/
static HRESULT WINAPI URLMonikerImpl_Inverse(IMoniker* iface,IMoniker** ppmk)
{
    URLMonikerImpl *This = (URLMonikerImpl *)iface;
    TRACE("(%p,%p)\n",This,ppmk);

    return MK_E_NOINVERSE;
}

/******************************************************************************
 *        URLMoniker_CommonPrefixWith
 ******************************************************************************/
static HRESULT WINAPI URLMonikerImpl_CommonPrefixWith(IMoniker* iface,IMoniker* pmkOther,IMoniker** ppmkPrefix)
{
    URLMonikerImpl *This = (URLMonikerImpl *)iface;
    FIXME("(%p)->(%p,%p): stub\n",This,pmkOther,ppmkPrefix);

    return E_NOTIMPL;
}

/******************************************************************************
 *        URLMoniker_RelativePathTo
 ******************************************************************************/
static HRESULT WINAPI URLMonikerImpl_RelativePathTo(IMoniker* iface,IMoniker* pmOther, IMoniker** ppmkRelPath)
{
    URLMonikerImpl *This = (URLMonikerImpl *)iface;
    FIXME("(%p)->(%p,%p): stub\n",This,pmOther,ppmkRelPath);

    return E_NOTIMPL;
}

/******************************************************************************
 *        URLMoniker_GetDisplayName
 ******************************************************************************/
static HRESULT WINAPI URLMonikerImpl_GetDisplayName(IMoniker* iface,
						    IBindCtx* pbc,
						    IMoniker* pmkToLeft,
						    LPOLESTR *ppszDisplayName)
{
    URLMonikerImpl *This = (URLMonikerImpl *)iface;
    
    int len;
    
    TRACE("(%p,%p,%p,%p)\n",This,pbc,pmkToLeft,ppszDisplayName);
    
    if(!ppszDisplayName)
        return E_INVALIDARG;
    
    /* FIXME: If this is a partial URL, try and get a URL moniker from SZ_URLCONTEXT in the bind context,
        then look at pmkToLeft to try and complete the URL
    */
    len = lstrlenW(This->URLName)+1;
    *ppszDisplayName = CoTaskMemAlloc(len*sizeof(WCHAR));
    if(!*ppszDisplayName)
        return E_OUTOFMEMORY;
    lstrcpyW(*ppszDisplayName, This->URLName);
    return S_OK;
}

/******************************************************************************
 *        URLMoniker_ParseDisplayName
 ******************************************************************************/
static HRESULT WINAPI URLMonikerImpl_ParseDisplayName(IMoniker* iface,
						      IBindCtx* pbc,
						      IMoniker* pmkToLeft,
						      LPOLESTR pszDisplayName,
						      ULONG* pchEaten,
						      IMoniker** ppmkOut)
{
    URLMonikerImpl *This = (URLMonikerImpl *)iface;
    FIXME("(%p)->(%p,%p,%p,%p,%p): stub\n",This,pbc,pmkToLeft,pszDisplayName,pchEaten,ppmkOut);

    return E_NOTIMPL;
}

/******************************************************************************
 *        URLMoniker_IsSystemMoniker
 ******************************************************************************/
static HRESULT WINAPI URLMonikerImpl_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys)
{
    URLMonikerImpl *This = (URLMonikerImpl *)iface;
    TRACE("(%p,%p)\n",This,pwdMksys);

    if(!pwdMksys)
        return E_INVALIDARG;

    *pwdMksys = MKSYS_URLMONIKER;
    return S_OK;
}

static HRESULT WINAPI URLMonikerImpl_IBinding_QueryInterface(IBinding* iface,REFIID riid,void** ppvObject)
{
    ICOM_THIS_MULTI(URLMonikerImpl, lpvtbl2, iface);

    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppvObject);

    /* Perform a sanity check on the parameters.*/
    if ( (This==0) || (ppvObject==0) )
	return E_INVALIDARG;

    /* Initialize the return parameter */
    *ppvObject = 0;

    /* Compare the riid with the interface IDs implemented by this object.*/
    if (IsEqualIID(&IID_IUnknown, riid) || IsEqualIID(&IID_IBinding, riid))
        *ppvObject = iface;

    /* Check that we obtained an interface.*/
    if ((*ppvObject)==0)
        return E_NOINTERFACE;

    /* Query Interface always increases the reference count by one when it is successful */
    IBinding_AddRef(iface);

    return S_OK;

}

static ULONG WINAPI URLMonikerImpl_IBinding_AddRef(IBinding* iface)
{
    ICOM_THIS_MULTI(URLMonikerImpl, lpvtbl2, iface);
    TRACE("(%p)\n",This);

    return URLMonikerImpl_AddRef((IMoniker*)This);
}

static ULONG WINAPI URLMonikerImpl_IBinding_Release(IBinding* iface)
{
    ICOM_THIS_MULTI(URLMonikerImpl, lpvtbl2, iface);
    TRACE("(%p)\n",This);

    return URLMonikerImpl_Release((IMoniker*)This);
}

static HRESULT WINAPI URLMonikerImpl_IBinding_Abort(IBinding* iface)
{
    ICOM_THIS_MULTI(URLMonikerImpl, lpvtbl2, iface);
    FIXME("(%p): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI URLMonikerImpl_IBinding_GetBindResult(IBinding* iface, CLSID* pclsidProtocol, DWORD* pdwResult, LPOLESTR* pszResult, DWORD* pdwReserved)
{
    ICOM_THIS_MULTI(URLMonikerImpl, lpvtbl2, iface);
    FIXME("(%p)->(%p, %p, %p, %p): stub\n", This, pclsidProtocol, pdwResult, pszResult, pdwReserved);

    return E_NOTIMPL;
}

static HRESULT WINAPI URLMonikerImpl_IBinding_GetPriority(IBinding* iface, LONG* pnPriority)
{
    ICOM_THIS_MULTI(URLMonikerImpl, lpvtbl2, iface);
    FIXME("(%p)->(%p): stub\n", This, pnPriority);

    return E_NOTIMPL;
}

static HRESULT WINAPI URLMonikerImpl_IBinding_Resume(IBinding* iface)
{
    ICOM_THIS_MULTI(URLMonikerImpl, lpvtbl2, iface);
    FIXME("(%p): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI URLMonikerImpl_IBinding_SetPriority(IBinding* iface, LONG nPriority)
{
    ICOM_THIS_MULTI(URLMonikerImpl, lpvtbl2, iface);
    FIXME("(%p)->(%ld): stub\n", This, nPriority);

    return E_NOTIMPL;
}

static HRESULT WINAPI URLMonikerImpl_IBinding_Suspend(IBinding* iface)
{
    ICOM_THIS_MULTI(URLMonikerImpl, lpvtbl2, iface);
    FIXME("(%p): stub\n", This);

    return E_NOTIMPL;
}

/********************************************************************************/
/* Virtual function table for the URLMonikerImpl class which  include IPersist,*/
/* IPersistStream and IMoniker functions.                                       */
static IMonikerVtbl VT_URLMonikerImpl =
{
    URLMonikerImpl_QueryInterface,
    URLMonikerImpl_AddRef,
    URLMonikerImpl_Release,
    URLMonikerImpl_GetClassID,
    URLMonikerImpl_IsDirty,
    URLMonikerImpl_Load,
    URLMonikerImpl_Save,
    URLMonikerImpl_GetSizeMax,
    URLMonikerImpl_BindToObject,
    URLMonikerImpl_BindToStorage,
    URLMonikerImpl_Reduce,
    URLMonikerImpl_ComposeWith,
    URLMonikerImpl_Enum,
    URLMonikerImpl_IsEqual,
    URLMonikerImpl_Hash,
    URLMonikerImpl_IsRunning,
    URLMonikerImpl_GetTimeOfLastChange,
    URLMonikerImpl_Inverse,
    URLMonikerImpl_CommonPrefixWith,
    URLMonikerImpl_RelativePathTo,
    URLMonikerImpl_GetDisplayName,
    URLMonikerImpl_ParseDisplayName,
    URLMonikerImpl_IsSystemMoniker
};

static IBindingVtbl VTBinding_URLMonikerImpl =
{
    URLMonikerImpl_IBinding_QueryInterface,
    URLMonikerImpl_IBinding_AddRef,
    URLMonikerImpl_IBinding_Release,
    URLMonikerImpl_IBinding_Abort,
    URLMonikerImpl_IBinding_Suspend,
    URLMonikerImpl_IBinding_Resume,
    URLMonikerImpl_IBinding_SetPriority,
    URLMonikerImpl_IBinding_GetPriority,
    URLMonikerImpl_IBinding_GetBindResult
};

/******************************************************************************
 *         URLMoniker_Construct (local function)
 *******************************************************************************/
static HRESULT URLMonikerImpl_Construct(URLMonikerImpl* This, LPCOLESTR lpszLeftURLName, LPCOLESTR lpszURLName)
{
    HRESULT hres;
    DWORD sizeStr;

    TRACE("(%p,%s,%s)\n",This,debugstr_w(lpszLeftURLName),debugstr_w(lpszURLName));
    memset(This, 0, sizeof(*This));

    /* Initialize the virtual function table. */
    This->lpvtbl1      = &VT_URLMonikerImpl;
    This->lpvtbl2      = &VTBinding_URLMonikerImpl;
    This->ref          = 0;

    if(lpszLeftURLName) {
        hres = UrlCombineW(lpszLeftURLName, lpszURLName, NULL, &sizeStr, 0);
        if(FAILED(hres)) {
            return hres;
        }
        sizeStr++;
    }
    else
        sizeStr = lstrlenW(lpszURLName)+1;

    This->URLName=HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*(sizeStr));

    if (This->URLName==NULL)
        return E_OUTOFMEMORY;

    if(lpszLeftURLName) {
        hres = UrlCombineW(lpszLeftURLName, lpszURLName, This->URLName, &sizeStr, 0);
        if(FAILED(hres)) {
            HeapFree(GetProcessHeap(), 0, This->URLName);
            return hres;
        }
    }
    else
        strcpyW(This->URLName,lpszURLName);

    return S_OK;
}

/***********************************************************************
 *           CreateAsyncBindCtx (URLMON.@)
 */
HRESULT WINAPI CreateAsyncBindCtx(DWORD reserved, IBindStatusCallback *callback,
    IEnumFORMATETC *format, IBindCtx **pbind)
{
    HRESULT hres;
    BIND_OPTS bindopts;
    IBindCtx *bctx;

    TRACE("(%08lx %p %p %p)\n", reserved, callback, format, pbind);

    if(!callback)
        return E_INVALIDARG;
    if(format)
        FIXME("format is not supported yet\n");

    hres = CreateBindCtx(0, &bctx);
    if(FAILED(hres))
        return hres;

    bindopts.cbStruct = sizeof(BIND_OPTS);
    bindopts.grfFlags = BIND_MAYBOTHERUSER;
    bindopts.grfMode = STGM_READWRITE | STGM_SHARE_EXCLUSIVE;
    bindopts.dwTickCountDeadline = 0;
    IBindCtx_SetBindOptions(bctx, &bindopts);

    hres = IBindCtx_RegisterObjectParam(bctx, (LPOLESTR)BSCBHolder, (IUnknown*)callback);
    if(FAILED(hres)) {
        IBindCtx_Release(bctx);
        return hres;
    }

    *pbind = bctx;

    return S_OK;
}
/***********************************************************************
 *           CreateAsyncBindCtxEx (URLMON.@)
 *
 * Create an asynchronous bind context.
 *
 * FIXME
 *   Not implemented.
 */ 
HRESULT WINAPI CreateAsyncBindCtxEx(IBindCtx *ibind, DWORD options,
    IBindStatusCallback *callback, IEnumFORMATETC *format, IBindCtx** pbind,
    DWORD reserved)
{
     FIXME("stub, returns failure\n");
     return E_INVALIDARG;
}


/***********************************************************************
 *           CreateURLMoniker (URLMON.@)
 *
 * Create a url moniker.
 *
 * PARAMS
 *    pmkContext [I] Context
 *    szURL      [I] Url to create the moniker for
 *    ppmk       [O] Destination for created moniker.
 *
 * RETURNS
 *    Success: S_OK. ppmk contains the created IMoniker object.
 *    Failure: MK_E_SYNTAX if szURL is not a valid url, or
 *             E_OUTOFMEMORY if memory allocation fails.
 */
HRESULT WINAPI CreateURLMoniker(IMoniker *pmkContext, LPCWSTR szURL, IMoniker **ppmk)
{
    URLMonikerImpl *obj;
    HRESULT hres;
    IID iid = IID_IMoniker;
    LPOLESTR lefturl = NULL;

    TRACE("(%p, %s, %p)\n", pmkContext, debugstr_w(szURL), ppmk);

    if(!(obj = HeapAlloc(GetProcessHeap(), 0, sizeof(*obj))))
	return E_OUTOFMEMORY;

    if(pmkContext) {
        CLSID clsid;
        IBindCtx* bind;
        IMoniker_GetClassID(pmkContext, &clsid);
        if(IsEqualCLSID(&clsid, &CLSID_StdURLMoniker) && SUCCEEDED(CreateBindCtx(0, &bind))) {
            URLMonikerImpl_GetDisplayName(pmkContext, bind, NULL, &lefturl);
            IBindCtx_Release(bind);
        }
    }
        
    hres = URLMonikerImpl_Construct(obj, lefturl, szURL);
    CoTaskMemFree(lefturl);
    if(SUCCEEDED(hres))
	hres = URLMonikerImpl_QueryInterface((IMoniker*)obj, &iid, (void**)ppmk);
    else
	HeapFree(GetProcessHeap(), 0, obj);
    return hres;
}


/***********************************************************************
 *           CoInternetGetSession (URLMON.@)
 *
 * Create a new internet session and return an IInternetSession interface
 * representing it.
 *
 * PARAMS
 *    dwSessionMode      [I] Mode for the internet session
 *    ppIInternetSession [O] Destination for creates IInternetSession object
 *    dwReserved         [I] Reserved, must be 0.
 *
 * RETURNS
 *    Success: S_OK. ppIInternetSession contains the IInternetSession interface.
 *    Failure: E_INVALIDARG, if any argument is invalid, or
 *             E_OUTOFMEMORY if memory allocation fails.
 */
HRESULT WINAPI CoInternetGetSession(DWORD dwSessionMode, IInternetSession **ppIInternetSession, DWORD dwReserved)
{
    FIXME("(%ld, %p, %ld): stub\n", dwSessionMode, ppIInternetSession, dwReserved);

    if(dwSessionMode) {
      ERR("dwSessionMode: %ld, must be zero\n", dwSessionMode);
    }

    if(dwReserved) {
      ERR("dwReserved: %ld, must be zero\n", dwReserved);
    }

    *ppIInternetSession=NULL;
    return E_OUTOFMEMORY;
}

/***********************************************************************
 *           CoInternetQueryInfo (URLMON.@)
 *
 * Retrieves information relevant to a specified URL
 *
 * RETURNS
 *    S_OK 			success
 *    S_FALSE			buffer too small
 *    INET_E_QUERYOPTIONUNKNOWN	invalid option
 *
 */
HRESULT WINAPI CoInternetQueryInfo(LPCWSTR pwzUrl, QUERYOPTION QueryOption,
  DWORD dwQueryFlags, LPVOID pvBuffer, DWORD cbBuffer, DWORD * pcbBuffer,
  DWORD dwReserved)
{
  FIXME("(%s, %x, %lx, %p, %lx, %p, %lx): stub\n", debugstr_w(pwzUrl),
    QueryOption, dwQueryFlags, pvBuffer, cbBuffer, pcbBuffer, dwReserved);
  return S_OK;
}

static BOOL URLMON_IsBinary(LPVOID pBuffer, DWORD cbSize)
{
    unsigned int i, binarycount = 0;
    unsigned char *buff = pBuffer;
    for(i=0; i<cbSize; i++) {
        if(buff[i] < 32)
            binarycount++;
    }
    return binarycount > (cbSize-binarycount);
}

/***********************************************************************
 *           FindMimeFromData (URLMON.@)
 *
 * Determines the Multipurpose Internet Mail Extensions (MIME) type from the data provided.
 *
 * NOTE
 *  See http://msdn.microsoft.com/workshop/networking/moniker/overview/appendix_a.asp
 */
HRESULT WINAPI FindMimeFromData(LPBC pBC, LPCWSTR pwzUrl, LPVOID pBuffer,
   DWORD cbSize, LPCWSTR pwzMimeProposed, DWORD dwMimeFlags,
   LPWSTR* ppwzMimeOut, DWORD dwReserved)
{
    static const WCHAR szBinaryMime[] = {'a','p','p','l','i','c','a','t','i','o','n','/','o','c','t','e','t','-','s','t','r','e','a','m','\0'};
    static const WCHAR szTextMime[] = {'t','e','x','t','/','p','l','a','i','n','\0'};
    static const WCHAR szContentType[] = {'C','o','n','t','e','n','t',' ','T','y','p','e','\0'};
    WCHAR szTmpMime[256];
    LPCWSTR mimeType = NULL;
    HKEY hKey = NULL;

    TRACE("(%p,%s,%p,%ld,%s,0x%lx,%p,0x%lx)\n", pBC, debugstr_w(pwzUrl), pBuffer, cbSize,
          debugstr_w(pwzMimeProposed), dwMimeFlags, ppwzMimeOut, dwReserved);

    if((!pwzUrl && (!pBuffer || cbSize <= 0)) || !ppwzMimeOut)
        return E_INVALIDARG;

    if(pwzMimeProposed)
        mimeType = pwzMimeProposed;
    else {
        /* Try and find the mime type in the registry */
        if(pwzUrl) {
            LPWSTR ext = strrchrW(pwzUrl, '.');
            if(ext) {
                DWORD dwSize;
                if(!RegOpenKeyExW(HKEY_CLASSES_ROOT, ext, 0, 0, &hKey)) {
                    if(!RegQueryValueExW(hKey, szContentType, NULL, NULL, (LPBYTE)szTmpMime, &dwSize)) {
                        mimeType = szTmpMime;
                    }
                    RegCloseKey(hKey);
                }
            }
        }
    }
    if(!mimeType && pBuffer && cbSize > 0)
        mimeType = URLMON_IsBinary(pBuffer, cbSize)?szBinaryMime:szTextMime;

    TRACE("Using %s\n", debugstr_w(mimeType));
    *ppwzMimeOut = CoTaskMemAlloc((lstrlenW(mimeType)+1)*sizeof(WCHAR));
    if(!*ppwzMimeOut) return E_OUTOFMEMORY;
    lstrcpyW(*ppwzMimeOut, mimeType);
    return S_OK;
}

/***********************************************************************
 *           IsAsyncMoniker (URLMON.@)
 */
HRESULT WINAPI IsAsyncMoniker(IMoniker *pmk)
{
    IUnknown *am;
    
    TRACE("(%p)\n", pmk);
    if(!pmk)
        return E_INVALIDARG;
    if(SUCCEEDED(IMoniker_QueryInterface(pmk, &IID_IAsyncMoniker, (void**)&am))) {
        IUnknown_Release(am);
        return S_OK;
    }
    return S_FALSE;
}

/***********************************************************************
 *           RegisterBindStatusCallback (URLMON.@)
 *
 * Register a bind status callback.
 *
 * PARAMS
 *  pbc           [I] Binding context
 *  pbsc          [I] Callback to register
 *  ppbscPrevious [O] Destination for previous callback
 *  dwReserved    [I] Reserved, must be 0.
 *
 * RETURNS
 *    Success: S_OK.
 *    Failure: E_INVALIDARG, if any argument is invalid, or
 *             E_OUTOFMEMORY if memory allocation fails.
 */
HRESULT WINAPI RegisterBindStatusCallback(
    IBindCtx *pbc,
    IBindStatusCallback *pbsc,
    IBindStatusCallback **ppbscPrevious,
    DWORD dwReserved)
{
    IBindStatusCallback *prev;

    TRACE("(%p,%p,%p,%lu)\n", pbc, pbsc, ppbscPrevious, dwReserved);

    if (pbc == NULL || pbsc == NULL)
        return E_INVALIDARG;

    if (SUCCEEDED(IBindCtx_GetObjectParam(pbc, (LPOLESTR)BSCBHolder, (IUnknown **)&prev)))
    {
        IBindCtx_RevokeObjectParam(pbc, (LPOLESTR)BSCBHolder);
        if (ppbscPrevious)
            *ppbscPrevious = prev;
        else
            IBindStatusCallback_Release(prev);
    }

	return IBindCtx_RegisterObjectParam(pbc, (LPOLESTR)BSCBHolder, (IUnknown *)pbsc);
}

/***********************************************************************
 *           RevokeBindStatusCallback (URLMON.@)
 *
 * Unregister a bind status callback.
 *
 *  pbc           [I] Binding context
 *  pbsc          [I] Callback to unregister
 *
 * RETURNS
 *    Success: S_OK.
 *    Failure: E_INVALIDARG, if any argument is invalid, or
 *             E_FAIL if pbsc wasn't registered with pbc.
 */
HRESULT WINAPI RevokeBindStatusCallback(
    IBindCtx *pbc,
    IBindStatusCallback *pbsc)
{
    IBindStatusCallback *callback;
    HRESULT hr = E_FAIL;

	TRACE("(%p,%p)\n", pbc, pbsc);

    if (pbc == NULL || pbsc == NULL)
        return E_INVALIDARG;

    if (SUCCEEDED(IBindCtx_GetObjectParam(pbc, (LPOLESTR)BSCBHolder, (IUnknown **)&callback)))
    {
        if (callback == pbsc)
        {
            IBindCtx_RevokeObjectParam(pbc, (LPOLESTR)BSCBHolder);
            hr = S_OK;
        }
        IBindStatusCallback_Release(pbsc);
    }

    return hr;
}

/***********************************************************************
 *           ReleaseBindInfo (URLMON.@)
 *
 * Release the resources used by the specified BINDINFO structure.
 *
 * PARAMS
 *  pbindinfo [I] BINDINFO to release.
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI ReleaseBindInfo(BINDINFO* pbindinfo)
{
    FIXME("(%p)stub!\n", pbindinfo);
}

/***********************************************************************
 *           URLDownloadToFileA (URLMON.@)
 *
 * Downloads URL szURL to rile szFileName and call lpfnCB callback to
 * report progress.
 *
 * PARAMS
 *  pCaller    [I] controlling IUnknown interface.
 *  szURL      [I] URL of the file to download
 *  szFileName [I] file name to store the content of the URL
 *  dwReserved [I] reserved - set to 0
 *  lpfnCB     [I] callback for progress report
 *
 * RETURNS
 *  S_OK on success
 *  E_OUTOFMEMORY when going out of memory
 */
HRESULT WINAPI URLDownloadToFileA(LPUNKNOWN pCaller,
				  LPCSTR szURL,
				  LPCSTR szFileName,
				  DWORD dwReserved,
				  LPBINDSTATUSCALLBACK lpfnCB)
{
    UNICODE_STRING szURL_w, szFileName_w;

    if ((szURL == NULL) || (szFileName == NULL)) {
	FIXME("(%p,%s,%s,%08lx,%p) cannot accept NULL strings !\n", pCaller, debugstr_a(szURL), debugstr_a(szFileName), dwReserved, lpfnCB);
	return E_INVALIDARG; /* The error code is not specified in this case... */
    }
    
    if (RtlCreateUnicodeStringFromAsciiz(&szURL_w, szURL)) {
	if (RtlCreateUnicodeStringFromAsciiz(&szFileName_w, szFileName)) {
	    HRESULT ret = URLDownloadToFileW(pCaller, szURL_w.Buffer, szFileName_w.Buffer, dwReserved, lpfnCB);

	    RtlFreeUnicodeString(&szURL_w);
	    RtlFreeUnicodeString(&szFileName_w);
	    
	    return ret;
	} else {
	    RtlFreeUnicodeString(&szURL_w);
	}
    }
    
    FIXME("(%p,%s,%s,%08lx,%p) could not allocate W strings !\n", pCaller, szURL, szFileName, dwReserved, lpfnCB);
    return E_OUTOFMEMORY;
}

/***********************************************************************
 *           URLDownloadToFileW (URLMON.@)
 *
 * Downloads URL szURL to rile szFileName and call lpfnCB callback to
 * report progress.
 *
 * PARAMS
 *  pCaller    [I] controlling IUnknown interface.
 *  szURL      [I] URL of the file to download
 *  szFileName [I] file name to store the content of the URL
 *  dwReserved [I] reserved - set to 0
 *  lpfnCB     [I] callback for progress report
 *
 * RETURNS
 *  S_OK on success
 *  E_OUTOFMEMORY when going out of memory
 */
HRESULT WINAPI URLDownloadToFileW(LPUNKNOWN pCaller,
				  LPCWSTR szURL,
				  LPCWSTR szFileName,
				  DWORD dwReserved,
				  LPBINDSTATUSCALLBACK lpfnCB)
{
    HINTERNET hinternet, hcon, hreq;
    BOOL r;
    CHAR buffer[0x1000];
    DWORD sz, total, written;
    DWORD total_size = 0xFFFFFFFF, arg_size = sizeof(total_size);
    URL_COMPONENTSW url;
    WCHAR host[0x80], path[0x100];
    HANDLE hfile;
    static const WCHAR wszAppName[]={'u','r','l','m','o','n','.','d','l','l',0};

    /* Note: all error codes would need to be checked agains real Windows behaviour... */
    TRACE("(%p,%s,%s,%08lx,%p) stub!\n", pCaller, debugstr_w(szURL), debugstr_w(szFileName), dwReserved, lpfnCB);

    if ((szURL == NULL) || (szFileName == NULL)) {
	FIXME(" cannot accept NULL strings !\n");
	return E_INVALIDARG;
    }

    /* Would be better to use the application name here rather than 'urlmon' :-/ */
    hinternet = InternetOpenW(wszAppName, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (hinternet == NULL) {
	return E_OUTOFMEMORY;
    }                                                                                                                             

    memset(&url, 0, sizeof(url));
    url.dwStructSize = sizeof(url);
    url.lpszHostName = host;
    url.dwHostNameLength = sizeof(host);
    url.lpszUrlPath = path;
    url.dwUrlPathLength = sizeof(path);

    if (!InternetCrackUrlW(szURL, 0, 0, &url)) {
	InternetCloseHandle(hinternet);
	return E_OUTOFMEMORY;
    }

    if (lpfnCB) {
	if (IBindStatusCallback_OnProgress(lpfnCB, 0, 0, BINDSTATUS_CONNECTING, url.lpszHostName) == E_ABORT) {
	    InternetCloseHandle(hinternet);
	    return S_OK;
	}
    }
    
    hcon = InternetConnectW(hinternet, url.lpszHostName, url.nPort,
                            url.lpszUserName, url.lpszPassword,
                            INTERNET_SERVICE_HTTP, 0, 0);
    if (!hcon) {
	InternetCloseHandle(hinternet);
	return E_OUTOFMEMORY;
    }
    
    hreq = HttpOpenRequestW(hcon, NULL, url.lpszUrlPath, NULL, NULL, NULL, 0, 0);
    if (!hreq) {
	InternetCloseHandle(hinternet);
	InternetCloseHandle(hcon);
	return E_OUTOFMEMORY;
    }                                                                                                                             

    if (!HttpSendRequestW(hreq, NULL, 0, NULL, 0)) {
	InternetCloseHandle(hinternet);
	InternetCloseHandle(hcon);
	InternetCloseHandle(hreq);
	return E_OUTOFMEMORY;
    }
    
    if (HttpQueryInfoW(hreq, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
		       &total_size, &arg_size, NULL)) {
	TRACE(" total size : %ld\n", total_size);
    }
    
    hfile = CreateFileW(szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, NULL );
    if (hfile == INVALID_HANDLE_VALUE) {
	return E_ACCESSDENIED;
    }
    
    if (lpfnCB) {
	if (IBindStatusCallback_OnProgress(lpfnCB, 0, total_size != 0xFFFFFFFF ? total_size : 0,
					   BINDSTATUS_BEGINDOWNLOADDATA, szURL) == E_ABORT) {
	    InternetCloseHandle(hreq);
	    InternetCloseHandle(hcon);
	    InternetCloseHandle(hinternet);
	    CloseHandle(hfile);
	    return S_OK;
	}
    }
    
    total = 0;
    while (1) {
	r = InternetReadFile(hreq, buffer, sizeof(buffer), &sz);
	if (!r) {
	    InternetCloseHandle(hreq);
	    InternetCloseHandle(hcon);
	    InternetCloseHandle(hinternet);
	    
	    CloseHandle(hfile);
	    return E_OUTOFMEMORY;	    
	}
	if (!sz)
	    break;
	
	total += sz;

	if (lpfnCB) {
	    if (IBindStatusCallback_OnProgress(lpfnCB, total, total_size != 0xFFFFFFFF ? total_size : 0,
					       BINDSTATUS_DOWNLOADINGDATA, szURL) == E_ABORT) {
		InternetCloseHandle(hreq);
		InternetCloseHandle(hcon);
		InternetCloseHandle(hinternet);
		CloseHandle(hfile);
		return S_OK;
	    }
	}
	
	if (!WriteFile(hfile, buffer, sz, &written, NULL)) {
	    InternetCloseHandle(hreq);
	    InternetCloseHandle(hcon);
	    InternetCloseHandle(hinternet);
	    
	    CloseHandle(hfile);
	    return E_OUTOFMEMORY;
	}
    }

    if (lpfnCB) {
	if (IBindStatusCallback_OnProgress(lpfnCB, total, total_size != 0xFFFFFFFF ? total_size : 0,
					   BINDSTATUS_ENDDOWNLOADDATA, szURL) == E_ABORT) {
	    InternetCloseHandle(hreq);
	    InternetCloseHandle(hcon);
	    InternetCloseHandle(hinternet);
	    CloseHandle(hfile);
	    return S_OK;
	}
    }
    
    InternetCloseHandle(hreq);
    InternetCloseHandle(hcon);
    InternetCloseHandle(hinternet);
    
    CloseHandle(hfile);

    return S_OK;
}

/***********************************************************************
 *           HlinkSimpleNavigateToString (URLMON.@)
 */
HRESULT WINAPI HlinkSimpleNavigateToString( LPCWSTR szTarget,
    LPCWSTR szLocation, LPCWSTR szTargetFrameName, IUnknown *pUnk,
    IBindCtx *pbc, IBindStatusCallback *pbsc, DWORD grfHLNF, DWORD dwReserved)
{
    FIXME("%s\n", debugstr_w( szTarget ) );
    return E_NOTIMPL;
}

/***********************************************************************
 *           HlinkNavigateString (URLMON.@)
 */
HRESULT WINAPI HlinkNavigateString( IUnknown *pUnk, LPCWSTR szTarget )
{
    TRACE("%p %s\n", pUnk, debugstr_w( szTarget ) );
    return HlinkSimpleNavigateToString( 
               szTarget, NULL, NULL, pUnk, NULL, NULL, 0, 0 );
}
