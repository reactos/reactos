/*
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_URLMON_MAIN_H
#define __WINE_URLMON_MAIN_H

#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "urlmon.h"

#include "wine/unicode.h"

extern HINSTANCE URLMON_hInstance;
extern HRESULT SecManagerImpl_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);
extern HRESULT ZoneMgrImpl_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);
extern HRESULT FileProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);
extern HRESULT HttpProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);
extern HRESULT HttpSProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);
extern HRESULT FtpProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);
extern HRESULT MkProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);

/**********************************************************************
 * Dll lifetime tracking declaration for urlmon.dll
 */
extern LONG URLMON_refCount;
static inline void URLMON_LockModule(void) { InterlockedIncrement( &URLMON_refCount ); }
static inline void URLMON_UnlockModule(void) { InterlockedDecrement( &URLMON_refCount ); }

#define ICOM_THIS_MULTI(impl,field,iface) impl* const This=(impl*)((char*)(iface) - offsetof(impl,field))
#define DEFINE_THIS(cls,ifc,iface) ((cls*)((BYTE*)(iface)-offsetof(cls,lp ## ifc ## Vtbl)))

typedef struct
{	
	const IStreamVtbl	*lpVtbl;
	LONG		ref;
	HANDLE		handle;
	BOOL		closed;
	WCHAR		*pszFileName;
	WCHAR		*pszURL;
} IUMCacheStream;

HRESULT	UMCreateStreamOnCacheFile(LPCWSTR pszURL, DWORD dwSize, LPWSTR pszFileName, HANDLE *phfile, IUMCacheStream **ppstr);
void	UMCloseCacheFileStream(IUMCacheStream *pstr);

IInternetProtocolInfo *get_protocol_info(LPCWSTR url);
HRESULT get_protocol_handler(LPCWSTR url, CLSID *clsid, IClassFactory **ret);
BOOL is_registered_protocol(LPCWSTR);
void register_urlmon_namespace(IClassFactory*,REFIID,LPCWSTR,BOOL);

HRESULT bind_to_storage(LPCWSTR url, IBindCtx *pbc, REFIID riid, void **ppv);
HRESULT bind_to_object(IMoniker *mon, LPCWSTR url, IBindCtx *pbc, REFIID riid, void **ppv);

HRESULT create_binding_protocol(LPCWSTR url, BOOL from_urlmon, IInternetProtocol **protocol);
void set_binding_sink(IInternetProtocol *bind_protocol, IInternetProtocolSink *sink);

static inline void *heap_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline void *heap_alloc_zero(size_t len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

static inline void *heap_realloc(void *mem, size_t len)
{
    return HeapReAlloc(GetProcessHeap(), 0, mem, len);
}

static inline BOOL heap_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

static inline LPWSTR heap_strdupW(LPCWSTR str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD size;

        size = (strlenW(str)+1)*sizeof(WCHAR);
        ret = heap_alloc(size);
        memcpy(ret, str, size);
    }

    return ret;
}

static inline LPWSTR heap_strdupAtoW(const char *str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
        ret = heap_alloc(len*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    }

    return ret;
}

#endif /* __WINE_URLMON_MAIN_H */
