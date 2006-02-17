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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_URLMON_MAIN_H
#define __WINE_URLMON_MAIN_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

extern HINSTANCE URLMON_hInstance;
extern HRESULT SecManagerImpl_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);
extern HRESULT ZoneMgrImpl_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);
extern HRESULT FileProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);
extern HRESULT HttpProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);
extern HRESULT FtpProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);

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

HRESULT get_protocol_iface(LPCWSTR url, IUnknown **ret);

HRESULT start_binding(LPCWSTR url, IBindCtx *pbc, REFIID riid, void **ppv);

#endif /* __WINE_URLMON_MAIN_H */
