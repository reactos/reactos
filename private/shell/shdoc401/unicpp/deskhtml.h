#ifndef _DESKHTML_H_
#define _DESKHTML_H_

class CDeskHtmlProp : public IShellExtInit, public IShellPropSheetExt
{
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IShellExtInit ***
    virtual STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hkeyProgID);

    // *** IShellPropSheetExt ***
    virtual STDMETHODIMP AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
    virtual STDMETHODIMP ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam);

protected:
    //
    // Constructor / Destructor
    //
    CDeskHtmlProp();
    ~CDeskHtmlProp();

    // Instance creator
    friend HRESULT CDeskHtmlProp_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut);

    UINT         _cRef;     // Reference count
};

#endif
