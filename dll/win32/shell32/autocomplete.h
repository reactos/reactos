/*
 *	AutoComplete interfaces implementation.
 *
 *	Copyright 2004	Maxime Bellengé <maxime.bellenge@laposte.net>
 *	Copyright 2009  Andrew Hill
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

#ifndef _AUTOCOMPLETE_H_
#define _AUTOCOMPLETE_H_

class CAutoComplete :
	public CComCoClass<CAutoComplete, &CLSID_AutoComplete>,
	public CComObjectRootEx<CComMultiThreadModelNoCS>,
	public IAutoComplete2
{
private:
	BOOL					enabled;
	BOOL					initialized;
	HWND					hwndEdit;
	HWND					hwndListBox;
	WNDPROC					wpOrigEditProc;
	WNDPROC					wpOrigLBoxProc;
	WCHAR					*txtbackup;
	WCHAR					*quickComplete;
	CComPtr<IEnumString>	enumstr;
	AUTOCOMPLETEOPTIONS		options;
public:

	CAutoComplete();
	~CAutoComplete();

	static LRESULT APIENTRY ACEditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT APIENTRY ACLBoxSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// IAutoComplete2
	virtual HRESULT WINAPI Enable(BOOL fEnable);
	virtual HRESULT WINAPI Init(HWND hwndEdit, IUnknown *punkACL, LPCOLESTR pwzsRegKeyPath, LPCOLESTR pwszQuickComplete);
	virtual HRESULT WINAPI GetOptions(DWORD *pdwFlag);
	virtual HRESULT WINAPI SetOptions(DWORD dwFlag);

DECLARE_REGISTRY_RESOURCEID(IDR_AUTOCOMPLETE)
DECLARE_NOT_AGGREGATABLE(CAutoComplete)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAutoComplete)
	COM_INTERFACE_ENTRY_IID(IID_IAutoComplete, IAutoComplete)
	COM_INTERFACE_ENTRY_IID(IID_IAutoComplete2, IAutoComplete2)
END_COM_MAP()
};

#endif /* _AUTOCOMPLETE_H_ */
