/*
 *  AutoComplete interfaces implementation.
 *
 *  Copyright 2004  Maxime Bellengé <maxime.bellenge@laposte.net>
 *  Copyright 2009  Andrew Hill
 *  Copyright 2021  Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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
    public IAutoComplete2,
    public IAutoCompleteDropDown,
    public IEnumString
{
private:
    BOOL                    m_enabled;
    BOOL                    m_initialized;
    HWND                    m_hwndEdit;
    HWND                    m_hwndListBox;
    WNDPROC                 m_wpOrigEditProc;
    WNDPROC                 m_wpOrigLBoxProc;
    LPWSTR                  m_txtbackup;        // HeapAlloc'ed
    LPWSTR                  m_quickComplete;    // HeapAlloc'ed
    CComPtr<IEnumString>    m_enumstr;
    AUTOCOMPLETEOPTIONS     m_options;
public:

    CAutoComplete();
    ~CAutoComplete();

    static LRESULT APIENTRY ACEditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT APIENTRY ACLBoxSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void CreateListbox();

    // IAutoComplete2
    virtual HRESULT WINAPI Enable(BOOL fEnable);
    virtual HRESULT WINAPI Init(HWND hwndEdit, IUnknown *punkACL, LPCOLESTR pwzsRegKeyPath, LPCOLESTR pwszQuickComplete);
    virtual HRESULT WINAPI GetOptions(DWORD *pdwFlag);
    virtual HRESULT WINAPI SetOptions(DWORD dwFlag);

    // IAutoCompleteDropDown
    virtual HRESULT STDMETHODCALLTYPE GetDropDownStatus(DWORD *pdwFlags, LPWSTR *ppwszString);
    virtual HRESULT STDMETHODCALLTYPE ResetEnumerator();

    // IEnumString methods
    virtual HRESULT STDMETHODCALLTYPE Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched);
    virtual HRESULT STDMETHODCALLTYPE Skip(ULONG celt);
    virtual HRESULT STDMETHODCALLTYPE Reset();
    virtual HRESULT STDMETHODCALLTYPE Clone(IEnumString **ppenum);

DECLARE_REGISTRY_RESOURCEID(IDR_AUTOCOMPLETE)
DECLARE_NOT_AGGREGATABLE(CAutoComplete)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAutoComplete)
    COM_INTERFACE_ENTRY_IID(IID_IAutoComplete, IAutoComplete)
    COM_INTERFACE_ENTRY_IID(IID_IAutoComplete2, IAutoComplete2)
    COM_INTERFACE_ENTRY_IID(IID_IAutoCompleteDropDown, IAutoCompleteDropDown)
    COM_INTERFACE_ENTRY_IID(IID_IEnumString, IEnumString)
END_COM_MAP()
};

#endif /* _AUTOCOMPLETE_H_ */
