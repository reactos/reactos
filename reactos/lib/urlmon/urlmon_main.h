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

/**********************************************************************
 * Dll lifetime tracking declaration for urlmon.dll
 */
extern LONG URLMON_refCount;
static inline void URLMON_LockModule(void) { InterlockedIncrement( &URLMON_refCount ); }
static inline void URLMON_UnlockModule(void) { InterlockedDecrement( &URLMON_refCount ); }

#define ICOM_THIS_MULTI(impl,field,iface) impl* const This=(impl*)((char*)(iface) - offsetof(impl,field))

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

DEFINE_GUID(CLSID_CdlProtocol, 0x3dd53d40, 0x7b8b, 0x11D0, 0xb0,0x13, 0x00,0xaa,0x00,0x59,0xce,0x02);
DEFINE_GUID(CLSID_FileProtocol, 0x79EAC9E7, 0xBAF9, 0x11CE, 0x8C,0x82, 0x00,0xAA,0x00,0x4B,0xA9,0x0B);
DEFINE_GUID(CLSID_FtpProtocol, 0x79EAC9E3, 0xBAF9, 0x11CE, 0x8C,0x82, 0x00,0xAA,0x00,0x4B,0xA9,0x0B);
DEFINE_GUID(CLSID_GopherProtocol, 0x79EAC9E4, 0xBAF9, 0x11CE, 0x8C,0x82, 0x00,0xAA,0x00,0x4B,0xA9,0x0B);
DEFINE_GUID(CLSID_HttpProtocol, 0x79EAC9E2, 0xBAF9, 0x11CE, 0x8C,0x82, 0x00,0xAA,0x00,0x4B,0xA9,0x0B);
DEFINE_GUID(CLSID_HttpsProtocol, 0x79EAC9E5, 0xBAF9, 0x11CE, 0x8C,0x82, 0x00,0xAA,0x00,0x4B,0xA9,0x0B);
DEFINE_GUID(CLSID_MkProtocol, 0x79EAC9E6, 0xBAF9, 0x11CE, 0x8C,0x82, 0x00,0xAA,0x00,0x4B,0xA9,0x0B);


#endif /* __WINE_URLMON_MAIN_H */
