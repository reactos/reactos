/*
 * File System Bind Data object to use as parameter for the bind context to
 * IShellFolder_ParseDisplayName
 *
 * Copyright 2003 Rolf Kalbermatter
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
 */
#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "shlobj.h"
#include "shell32_main.h"

#include "debughlp.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(pidl);

/***********************************************************************
 * IFileSystemBindData implementation
 */
typedef struct
{
	ICOM_VFIELD(IFileSystemBindData);
	DWORD              ref;
	WIN32_FIND_DATAW findFile;
} IFileSystemBindDataImpl;

static HRESULT WINAPI IFileSystemBindData_fnQueryInterface(IFileSystemBindData *iface, REFIID riid, LPVOID* ppvObj);
static ULONG WINAPI IFileSystemBindData_fnAddRef(IFileSystemBindData *iface);
static ULONG WINAPI IFileSystemBindData_fnRelease(IFileSystemBindData *iface);
static HRESULT WINAPI IFileSystemBindData_fnGetFindData(IFileSystemBindData *iface, WIN32_FIND_DATAW *pfd);
static HRESULT WINAPI IFileSystemBindData_fnSetFindData(IFileSystemBindData *iface, const WIN32_FIND_DATAW *pfd);

static struct ICOM_VTABLE(IFileSystemBindData) sbvt =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IFileSystemBindData_fnQueryInterface,
    IFileSystemBindData_fnAddRef,
    IFileSystemBindData_fnRelease,
    IFileSystemBindData_fnGetFindData,
    IFileSystemBindData_fnSetFindData,
};

static const WCHAR wFileSystemBindData[] = {'F','i','l','e',' ','S','y','s','t','e','m',' ','B','i','n','d','D','a','t','a',0};

HRESULT WINAPI IFileSystemBindData_Constructor(const WIN32_FIND_DATAW *pfd, LPBC *ppV)
{
	IFileSystemBindDataImpl *sb;
	HRESULT ret = E_OUTOFMEMORY;

	TRACE("%p, %p\n", pfd, ppV);

	if (!ppV)
	  return E_INVALIDARG;

	*ppV = NULL;

	sb = (IFileSystemBindDataImpl*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IFileSystemBindDataImpl));
	if (!sb)
	  return ret;

	sb->lpVtbl = &sbvt;
	sb->ref = 1;
	IFileSystemBindData_fnSetFindData((IFileSystemBindData*)sb, pfd);

	ret = CreateBindCtx(0, ppV);
	if (SUCCEEDED(ret))
	{
	  BIND_OPTS bindOpts;
	  bindOpts.cbStruct = sizeof(BIND_OPTS);
	  bindOpts.grfFlags = 0;
	  bindOpts.grfMode = STGM_CREATE;
	  bindOpts.dwTickCountDeadline = 0;
	  IBindCtx_SetBindOptions(*ppV, &bindOpts);
	  IBindCtx_RegisterObjectParam(*ppV, (LPOLESTR)wFileSystemBindData, (LPUNKNOWN)sb);

	  IFileSystemBindData_Release((IFileSystemBindData*)sb);
	}
	else
	  HeapFree(GetProcessHeap(), 0, sb);
	return ret;
}

HRESULT WINAPI FileSystemBindData_GetFindData(LPBC pbc, WIN32_FIND_DATAW *pfd)
{
	LPUNKNOWN pUnk;
	IFileSystemBindData *pfsbd = NULL;
	HRESULT ret;

	TRACE("%p, %p\n", pbc, pfd);

	if (!pfd)
	  return E_INVALIDARG;

	ret = IBindCtx_GetObjectParam(pbc, (LPOLESTR)wFileSystemBindData, &pUnk);
	if (SUCCEEDED(ret))
	{
	  ret = IUnknown_QueryInterface(pUnk, &IID_IFileSystemBindData, (LPVOID *)&pfsbd);
	  if (SUCCEEDED(ret))
	  {
	    ret = IFileSystemBindData_GetFindData(pfsbd, pfd);
	    IFileSystemBindData_Release(pfsbd);
	  }
	  IUnknown_Release(pUnk);
	}
	return ret;
}

HRESULT WINAPI FileSystemBindData_SetFindData(LPBC pbc, const WIN32_FIND_DATAW *pfd)
{
	LPUNKNOWN pUnk;
	IFileSystemBindData *pfsbd = NULL;
	HRESULT ret;
	
	TRACE("%p, %p\n", pbc, pfd);

	ret = IBindCtx_GetObjectParam(pbc, (LPOLESTR)wFileSystemBindData, &pUnk);
	if (SUCCEEDED(ret))
	{
	  ret = IUnknown_QueryInterface(pUnk, &IID_IFileSystemBindData, (LPVOID *)&pfsbd);
	  if (SUCCEEDED(ret))
	  {
	    ret = IFileSystemBindData_SetFindData(pfsbd, pfd);
	    IFileSystemBindData_Release(pfsbd);
	  }
	  IUnknown_Release(pUnk);
	}
	return ret;}



static HRESULT WINAPI IFileSystemBindData_fnQueryInterface(IFileSystemBindData *iface, REFIID riid, LPVOID *ppV)
{
	ICOM_THIS(IFileSystemBindDataImpl, iface);
	TRACE("(%p)->(\n\tIID:\t%s, %p)\n", This, debugstr_guid(riid), ppV);

	*ppV = NULL;

	if (IsEqualIID(riid, &IID_IUnknown))
	{
	  *ppV = This;
	}
	else if (IsEqualIID(riid, &IID_IFileSystemBindData))
	{
	  *ppV = (IFileSystemBindData*)This;
	}

	if (*ppV)
	{
	  IUnknown_AddRef((IUnknown*)(*ppV));
	  TRACE("-- Interface: (%p)->(%p)\n", ppV, *ppV);
	  return S_OK;
	}
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

static ULONG WINAPI IFileSystemBindData_fnAddRef(IFileSystemBindData *iface)
{
	ICOM_THIS(IFileSystemBindDataImpl, iface);
	TRACE("(%p)\n", This);
	return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IFileSystemBindData_fnRelease(IFileSystemBindData *iface)
{
	ICOM_THIS(IFileSystemBindDataImpl, iface);
	TRACE("(%p)\n", This);

	if (!InterlockedDecrement(&This->ref))
	{
	  TRACE(" destroying ISFBindPidl(%p)\n",This);
	  HeapFree(GetProcessHeap(), 0, This);
	  return 0;
	}
	return This->ref;
}

static HRESULT WINAPI IFileSystemBindData_fnGetFindData(IFileSystemBindData *iface, WIN32_FIND_DATAW *pfd)
{
	ICOM_THIS(IFileSystemBindDataImpl, iface);
	TRACE("(%p), %p\n", This, pfd);

	if (!pfd)
	  return E_INVALIDARG;

	memcpy(pfd, &This->findFile, sizeof(WIN32_FIND_DATAW));
	return NOERROR;
}

static HRESULT WINAPI IFileSystemBindData_fnSetFindData(IFileSystemBindData *iface, const WIN32_FIND_DATAW *pfd)
{
	ICOM_THIS(IFileSystemBindDataImpl, iface);
	TRACE("(%p), %p\n", This, pfd);

	if (pfd)
	  memcpy(&This->findFile, pfd, sizeof(WIN32_FIND_DATAW));
	else
	  memset(&This->findFile, 0, sizeof(WIN32_FIND_DATAW));
	return NOERROR;
}
