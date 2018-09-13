#include "stdafx.h"
#pragma hdrstop

#ifdef POSTSPLIT

//===========================================================================
// CDeskHtmlProp
//===========================================================================
  
HRESULT CDeskHtmlProp::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IShellExtInit) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IShellExtInit*)this;
        _cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IShellPropSheetExt))
    {
        *ppvObj = (IShellPropSheetExt *)this;
        _cRef++;
        return S_OK;
    }
    *ppvObj = NULL;

    return E_NOINTERFACE;
}

ULONG CDeskHtmlProp::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CDeskHtmlProp::Release()
{
    _cRef--;
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

CDeskHtmlProp::CDeskHtmlProp() : _cRef(1)
{
    DllAddRef();
    OleInitialize(NULL);
}

CDeskHtmlProp::~CDeskHtmlProp()
{
    OleUninitialize();
    DllRelease();
}

HRESULT CDeskHtmlProp::Initialize(LPCITEMIDLIST pidlFolder,
              LPDATAOBJECT pdtobj, HKEY hkeyProgID)
{
    TraceMsg(TF_GENERAL, "DeskHtmlProp - Initialize");
    return S_OK;
}

HRESULT CDeskHtmlProp::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    TraceMsg(TF_GENERAL, "DeskHtmlProp - AddPages");

    RegisterCompPreviewClass();

    HPROPSHEETPAGE hpage;
    HRESULT hres = S_OK;
    CCompPropSheetPage cpsp;

    if ((!SHRestricted(REST_FORCEACTIVEDESKTOPON)) && SHRestricted(REST_NOACTIVEDESKTOP))
        // If no active desktop policy set, don't put up the property page
        return S_OK;

    if (SHRestricted(REST_NOACTIVEDESKTOPCHANGES))
        // If policy is set to lock down active desktop, don't put up the
        // property page
        return S_OK;
    
    if (SHRestricted(REST_NODESKCOMP))
        // If policy is set to not allow components, don't put up the
        // property page
        return S_OK;

    if (SHRestricted(REST_CLASSICSHELL))
        return S_OK;

    hpage = CreatePropertySheetPage(&cpsp);

    if (hpage)
    {
        BOOL bResult = lpfnAddPage(hpage, lParam);
        if (!bResult)
        {
            DestroyPropertySheetPage(hpage);
            hres = E_FAIL;
        }
    }

    return hres;
}

HRESULT CDeskHtmlProp::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam)
{
    TraceMsg(TF_GENERAL, "DeskHtmlProp - ReplacePage");

    RegisterBackPreviewClass();

    HRESULT hres = S_FALSE;

    if ((uPageID == CPLPAGE_DISPLAY_BACKGROUND) &&
        (SHRestricted(REST_FORCEACTIVEDESKTOPON) || !SHRestricted(REST_NOACTIVEDESKTOP)) &&
        !SHRestricted(REST_NOACTIVEDESKTOPCHANGES))
    {
        CBackPropSheetPage bpsp;
        HPROPSHEETPAGE hpage = CreatePropertySheetPage(&bpsp);

        if (hpage)
        {
            if (lpfnReplaceWith(hpage, lParam))
            {
                hres = S_OK;
            }
            else
            {
                TraceMsg(TF_WARNING, "DeskHtml - ReplacePage could not replace a page");
                DestroyPropertySheetPage(hpage);
            }
        }
        else
        {
            TraceMsg(TF_WARNING, "DeskHtml - ReplacePage could not create a page");
        }
    }

    return hres;
}

HRESULT CDeskHtmlProp_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut)
{
    TraceMsg(TF_GENERAL, "DeskHtmlProp - CreateInstance");

    CDeskHtmlProp* pdhd = new CDeskHtmlProp();
    if (pdhd) 
    {
        HRESULT hres = pdhd->QueryInterface(riid, ppvOut);
        pdhd->Release();
        return hres;
    }
    
    *ppvOut = NULL;
    return E_OUTOFMEMORY;
}

#endif
