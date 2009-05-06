/*
 * ReactOS Explorer
 *
 * Copyright 2009 Andrew Hill <ash77 at domain reactos.org>
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

/*
This class handles the combo box of the address band.
*/
#include "precomp.h"
#include "browseui_resource.h"
#include "newinterfaces.h"
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include "addresseditbox.h"
/*
TODO:
    Add auto completion support
	Subclass windows in Init method
	Connect to browser connection point
	Handle navigation complete messages to set edit box text
	Handle listbox dropdown message and fill contents
	Add drag and drop of icon in edit box
	Handle enter in edit box to browse to typed path
	Handle change notifies to update appropriately
	Add handling of enter in edit box
	Fix so selection in combo listbox navigates
	Fix so editing text and typing enter navigates
*/

CAddressEditBox::CAddressEditBox() :
		fEditWindow(NULL, this, 1),
		fComboBoxExWindow(NULL, this, 2)
{
}

CAddressEditBox::~CAddressEditBox()
{
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::SetOwner(IUnknown *)
{
	// connect to browser connection point
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::FileSysChange(long param8, long paramC)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::Refresh(long param8)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::Init(HWND comboboxEx, HWND editControl, long param14, IUnknown *param18)
{
	fComboBoxExWindow.SubclassWindow(comboboxEx);
	fEditWindow.SubclassWindow(editControl);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::SetCurrentDir(long paramC)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::ParseNow(long paramC)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::Execute(long paramC)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::Save(long paramC)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
	// handle fill of listbox here
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::IsWindowOwner(HWND hWnd)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::GetTypeInfoCount(UINT *pctinfo)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
	// on navigate complete, change edit section of combobox
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::GetClassID(CLSID *pClassID)
{
	if (pClassID == NULL)
		return E_POINTER;
	*pClassID = CLSID_AddressEditBox;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::IsDirty()
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::Load(IStream *pStm)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::Save(IStream *pStm, BOOL fClearDirty)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CAddressEditBox::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
	return E_NOTIMPL;
}

HRESULT CreateAddressEditBox(REFIID riid, void **ppv)
{
	CComObject<CAddressEditBox>				*theMenuBar;
	HRESULT									hResult;

	if (ppv == NULL)
		return E_POINTER;
	*ppv = NULL;
	ATLTRY (theMenuBar = new CComObject<CAddressEditBox>);
	if (theMenuBar == NULL)
		return E_OUTOFMEMORY;
	hResult = theMenuBar->QueryInterface (riid, (void **)ppv);
	if (FAILED (hResult))
	{
		delete theMenuBar;
		return hResult;
	}
	return S_OK;
}
