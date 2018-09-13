#ifndef _COMBOBOX_OC_H_
#define _COMBOBOX_OC_H_

#include "unicpp/stdafx.h"
#include <mshtmdid.h>
#include <mshtml.h>
#include <shdispid.h>
#include "atldisp.h"
#include "unicpp\sdspatch.h"
#include "shcombox.h"
#include "ids.h"

#define FCIDM_VIEWADDRESS       (FCIDM_BROWSER_VIEW + 0x0005)

#define DEFAULT_WINDOW_STYLE             (WS_BORDER | WS_CHILD | WS_CLIPCHILDREN | CBS_DROPDOWN | CBS_AUTOHSCROLL)
#define SZ_REGVALUE_LAST_SELECTION       TEXT("Last Selection")

// CProxy_ComboBoxExEvents
template <class T>
class CProxy_ComboBoxExEvents
                    : public IConnectionPointImpl<T, &DIID_DComboBoxExEvents, CComDynamicUnkArray>
{
public:
//methods:
//DComboBoxExEvents : IDispatch
public:
    virtual void Fire_EnterPressed(void)
    {
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                DISPPARAMS disp = { NULL, NULL, 0, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_ENTERPRESSED, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
    }
};

class ATL_NO_VTABLE CComboBoxExOC
{
public:
    CComboBoxExOC(LPCTSTR pszRegKey, int csidlDefaultPidl);
    ~CComboBoxExOC();

    virtual HRESULT TranslateAcceleratorInternal(MSG *pMsg, IOleClientSite * pocs);
    virtual void Fire_EnterPressed(void) = 0;

protected:
    // Helper functions;
    virtual HRESULT _PopulateTopItem(void);
    virtual HRESULT _Populate(void) = 0;    // PURE virtual
    virtual BOOL _IsSecure(void) = 0;    // PURE virtual
    virtual HRESULT _AddDefaultItem(void) = 0;    // PURE virtual
    virtual HRESULT _CustomizeName(UINT idResource, LPTSTR pszDisplayName, DWORD cchSize) = 0;    // PURE virtual
    virtual HRESULT _GetSelectText(LPTSTR pszSelectText, DWORD cchSize, BOOL fDisplayName) = 0;    // PURE virtual
    virtual HRESULT _IsDefaultSelection(LPCTSTR pszLastSelection) = 0;
    virtual HRESULT _RestoreIfUserInputValue(LPCTSTR pszLastSelection) = 0;

    virtual STDMETHODIMP get_Enabled(OUT VARIANT_BOOL * pfEnabled);
    virtual STDMETHODIMP put_Enabled(IN VARIANT_BOOL fEnabled);


    // Window Creation
    HWND _CreateComboBoxEx(IOleClientSite * pClientSite, HWND hWnd, HWND hWndParent, RECT& rcPos, LPCTSTR pszWindowName, DWORD dwStyle, DWORD dwExStyle, UINT_PTR nID);
    virtual HWND _Create(HWND hWndParent, RECT& rcPos, LPCTSTR pszWindowName, DWORD dwStyle, DWORD dwExStyle, UINT_PTR nID) = 0; // PURE Virtual
    virtual DWORD _GetWindowStyles(void) { return DEFAULT_WINDOW_STYLE; };
    virtual HRESULT _SetSelect(LPCTSTR pszDisplay, LPCTSTR pszReplace);

    // Window Messages
    LRESULT cb_ForwardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT cb_OnDropDownMessage(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    // ComboBox Util Functions
    virtual HRESULT _AddFileType(LPCTSTR pszDisplayName, LPCTSTR pszExt, LPCITEMIDLIST pidlIcon, INT_PTR nPos, int iIndent);
    HRESULT _AddResourceAndCsidlStr(UINT idString, int nCsidlItem, int nCsidlIcon, INT_PTR nPos, int iIndent);
    HRESULT _AddResourceItem(int idString, DWORD dwData, int nCsidlIcon, INT_PTR nPos, int iIndent);
    HRESULT _AddCsidlItem(int nCsidlItem, int nCsidlIcon, INT_PTR nPos, int iIndent);
    HRESULT _AddCsidlItemStr(int nCsidlItem, int nCsidlIcon, INT_PTR nPos, int iIndent);
    HRESULT _AddCsidlIcon(LPCTSTR pszDisplayName, LPVOID pvData, int nCsidlIcon, INT_PTR nPos, int iIndent);
    HRESULT _AddPidl(LPITEMIDLIST pidl, LPITEMIDLIST pidlIcon, INT_PTR nPos, int iIndent);
    HRESULT _AddToComboBox(LPCTSTR pszDisplayName, LPVOID pvData, LPCITEMIDLIST pidlIcon, INT_PTR nPos, int iIndent, INT_PTR *pnPosAdded);
    HRESULT _AddToComboBoxKnownImage(LPCTSTR pszDisplayName, LPVOID pvData, int iImage, int iSelectedImage, INT_PTR nPos, int iIndent, INT_PTR *pnPosAdded);

    // Util Functions
    HRESULT _Load(IPropertyBag * pPropBag, IErrorLog * pErrorLog);
    HRESULT _putString(LPCSTR pszString);
    HRESULT _LoadAndConvertString(UINT idString, LPTSTR pszDisplayName, DWORD chSize);


    // Private Member Variables
    HWND                _hwndComboBox;
    HWND                _hwndEdit;
    LPCSTR              _pszInitialString;          // Initial String to display in Address Box.
    LPCTSTR             _pszPersistString;          // Where in the registry to persist the state.
    DWORD               _dwDropDownSize;            // Drop Down size.
    BITBOOL             _fEnableEdit    : 1;        // Is Edit Allowed?
    BITBOOL             _fInRecursion   : 1;
    VARIANT_BOOL        _fEnabled;                  // Is the control enabled (not grayed out)?
    LPCTSTR             _pszRegKey;                 // Persist data here.
    int                 _csidlDefaultPidl;          // 
};


#endif // _COMBOBOX_OC_H_
