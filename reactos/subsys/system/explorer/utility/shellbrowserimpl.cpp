/*
 * Copyright 2003 Martin Fuchs
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


 //
 // Explorer clone
 // 
 // shellbrowserimpl.cpp
 //
 // Martin Fuchs, 28.09.2003
 //
 // Credits: Thanks to Leon Finker for his explorer window example
 //


#include "utility.h"
#include "shellclasses.h"
#include "shellbrowserimpl.h"


HRESULT IShellBrowserImpl::QueryInterface(REFIID iid, void** ppvObject)
{
	if (!ppvObject)
		return E_POINTER;

	if (iid == IID_IUnknown)
		*ppvObject = (IUnknown*)static_cast<IShellBrowser*>(this);
	else if (iid == IID_IOleWindow)
		*ppvObject = static_cast<IOleWindow*>(this);
	else if (iid == IID_IShellBrowser)
		*ppvObject = static_cast<IShellBrowser*>(this);
	else if (iid == IID_ICommDlgBrowser)
		*ppvObject = static_cast<ICommDlgBrowser*>(this);
	else {
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}

	return S_OK;
}


 // process default command: look for folders and traverse into them
HRESULT IShellBrowserImpl::OnDefaultCommand(IShellView* ppshv)
{
	static UINT CF_IDLIST = RegisterClipboardFormat(CFSTR_SHELLIDLIST);

	IDataObject* selection;
	HRESULT hr = ppshv->GetItemObject(SVGIO_SELECTION, IID_IDataObject, (void**)&selection);
	if (FAILED(hr))
		return hr;


    FORMATETC fetc;
    fetc.cfFormat = CF_IDLIST;
    fetc.ptd = NULL;
    fetc.dwAspect = DVASPECT_CONTENT;
    fetc.lindex = -1;
    fetc.tymed = TYMED_HGLOBAL;

    hr = selection->QueryGetData(&fetc);
	if (FAILED(hr))
		return hr;


	STGMEDIUM stgm = {sizeof(STGMEDIUM), {0}, 0};

    hr = selection->GetData(&fetc, &stgm);
	if (FAILED(hr))
		return hr;

    DWORD pData = (DWORD)GlobalLock(stgm.hGlobal);
	LPIDA pIDList = (LPIDA)pData;

	HRESULT ret = OnDefaultCommand(pIDList);

	GlobalUnlock(stgm.hGlobal);
    ReleaseStgMedium(&stgm);


	selection->Release();

	return ret;
}
