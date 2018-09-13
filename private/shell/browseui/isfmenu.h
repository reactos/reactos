// CISFMenuCallback implementation

#ifndef _ISFMENU_H
#define _ISFMENU_H

#include "cowsite.h"

class CISFMenuCallback : public IShellMenuCallback,
                           public CObjectWithSite
{
public:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface (REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG)  Release();

    // *** IShellMenuCallback methods ***
    STDMETHODIMP CallbackSM(LPSMDATA smd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // *** IObjectWithSite methods ***
    STDMETHODIMP SetSite(IUnknown* punkSite);

    CISFMenuCallback();
    HRESULT Initialize(IUnknown* punk);

private:
    virtual ~CISFMenuCallback();

    HRESULT _GetObject(LPSMDATA psmd, REFIID riid, void** ppvObj);
    HRESULT _SetObject(LPSMDATA psmd, REFIID riid, void** ppvObj);
    BOOL _IsVisible(LPITEMIDLIST pidl);
    HRESULT _GetSFInfo(LPSMDATA psmd, PSMINFO psminfo);

    int _cRef;
    IOleCommandTarget* _poct;    // our isfband subject
    IUnknown* _punkSite;
    LPITEMIDLIST _pidl;
};

#endif // _ISFMENU_H
