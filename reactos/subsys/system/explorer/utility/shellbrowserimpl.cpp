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
 // Credits: Thanks to Leon Finker for his explorer cabinet window example
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
	IDataObject* selection;

	HRESULT hr = ppshv->GetItemObject(SVGIO_SELECTION, IID_IDataObject, (void**)&selection);
	if (FAILED(hr))
		return hr;

	PIDList pidList;

	hr = pidList.GetData(selection);
	if (FAILED(hr)) {
		selection->Release();
		return hr;
	}

	hr = OnDefaultCommand(pidList);

	selection->Release();

	return hr;
}
