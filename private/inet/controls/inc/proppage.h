//=--------------------------------------------------------------------------=
// PropPage.H
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// class declaration for CPropertyPage.
//
#ifndef _PROPPAGE_H_

// things we really need
//
#include "Unknown.H"
#include <olectl.h>
#include "LocalSrv.H"

//=--------------------------------------------------------------------------=
// messages that we'll send to property pages to instruct them to accomplish
// tasks.
//
#define PPM_NEWOBJECTS    (WM_USER + 100)
#define PPM_APPLY         (WM_USER + 101)
#define PPM_EDITPROPERTY  (WM_USER + 102)
#define PPM_FREEOBJECTS   (WM_USER + 103)

//=--------------------------------------------------------------------------=
// structure that control writers will use to define property pages.
//
typedef struct tagPROPERTYPAGEINFO {

    UNKNOWNOBJECTINFO unknowninfo;
    WORD    wDlgResourceId;
    WORD    wTitleId;
    WORD    wDocStringId;
    LPCSTR  szHelpFile;
    DWORD   dwHelpContextId;

} PROPERTYPAGEINFO;

#ifndef INITOBJECTS

#define DEFINE_PROPERTYPAGEOBJECT(name, pclsid, pszon, pfn, wr, wt, wd, pszhf, dwhci) \
    extern PROPERTYPAGEINFO name##Page \

#else // INITOBJECTS

#define DEFINE_PROPERTYPAGEOBJECT(name, pclsid, pszon, pfn, wr, wt, wd, pszhf, dwhci) \
    PROPERTYPAGEINFO name##Page = { {pclsid, pszon, pfn }, wr, wt, wd, pszhf, dwhci } \

#endif // INITOBJECTS


#define TEMPLATENAMEOFPROPPAGE(index)    MAKEINTRESOURCE(((PROPERTYPAGEINFO *)(g_ObjectInfo[index].pInfo))->wDlgResourceId)
#define TITLEIDOFPROPPAGE(index)         (((PROPERTYPAGEINFO *)(g_ObjectInfo[index].pInfo))->wTitleId)
#define DOCSTRINGIDOFPROPPAGE(index)     (((PROPERTYPAGEINFO *)(g_ObjectInfo[index].pInfo))->wDocStringId)
#define HELPCONTEXTOFPROPPAGE(index)     (((PROPERTYPAGEINFO *)(g_ObjectInfo[index].pInfo))->dwHelpContextId)
#define HELPFILEOFPROPPAGE(index)        (((PROPERTYPAGEINFO *)(g_ObjectInfo[index].pInfo))->szHelpFile)

//=--------------------------------------------------------------------------=
//
class CPropertyPage : public CUnknownObject, public IPropertyPage2 {

  public:
    // IUnknown methods
    //
    DECLARE_STANDARD_UNKNOWN();

    // IPropertyPage methods
    //
    STDMETHOD(SetPageSite)(LPPROPERTYPAGESITE pPageSite);
    STDMETHOD(Activate)(HWND hwndParent, LPCRECT lprc, BOOL bModal);
    STDMETHOD(Deactivate)(void);
    STDMETHOD(GetPageInfo)(LPPROPPAGEINFO pPageInfo);
    STDMETHOD(SetObjects)(ULONG cObjects, LPUNKNOWN FAR* ppunk);
    STDMETHOD(Show)(UINT nCmdShow);
    STDMETHOD(Move)(LPCRECT prect);
    STDMETHOD(IsPageDirty)(void);
    STDMETHOD(Apply)(void);
    STDMETHOD(Help)(LPCOLESTR lpszHelpDir);
    STDMETHOD(TranslateAccelerator)(LPMSG lpMsg);

    // IPropertyPage2 methods
    //
    STDMETHOD(EditProperty)(THIS_ DISPID dispid);

    // constructor destructor
    //
    CPropertyPage(IUnknown *pUnkOuter, int iObjectType);
    virtual ~CPropertyPage();

    HINSTANCE GetResourceHandle(void);            // returns current resource handle.

  protected:
    IPropertyPageSite *m_pPropertyPageSite;       // pointer to our ppage site.
    void     MakeDirty();                         // makes the property page dirty.
    HWND     m_hwnd;                              // our hwnd.

    // the following two methods allow a property page implementer to get at all the
    // objects that we need to set here.
    //
    IUnknown *FirstControl(DWORD *dwCookie);
    IUnknown *NextControl(DWORD *dwCookie);

  private:
    IUnknown **m_ppUnkObjects;                    // objects that we're working with.

    unsigned m_fActivated:1;
    unsigned m_fDirty:1;
    int      m_ObjectType;                        // what type of object we are
    UINT     m_cObjects;                          // how many objects we're holding on to

    void     m_ReleaseAllObjects(void);           // clears out all objects we've got.
    HRESULT  m_EnsureLoaded(void);                // forces the load of the page.

    virtual HRESULT InternalQueryInterface(REFIID, void **);

    // default dialog proc for a page.
    //
    static BOOL CALLBACK PropPageDlgProc(HWND, UINT, WPARAM, LPARAM);

    // all page implementers MUST implement the following function.
    //
    virtual BOOL DialogProc(HWND, UINT, WPARAM, LPARAM) PURE;
};

#define _PROPPAGE_H_
#endif // _PROPPAGE_H_



