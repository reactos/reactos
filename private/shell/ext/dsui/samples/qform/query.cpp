/*----------------------------------------------------------------------------
/ Title;
/   query.cpp
/
/ Authors;
/   David De Vorchik (daviddv)
/
/ Notes;
/   This is a simple COM object that provides Query Form to be
/   plugged into the Directory Service Query framework.
/----------------------------------------------------------------------------*/
#include "pch.h"
#include "query.h"
#include "dsquery.h"
#include "resource.h"
#include "iids.h"
#pragma hdrstop


HRESULT PageProc_ExamplePage(LPCQPAGE pPage, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProc_ExamplePage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HRESULT GetQueryParams(HWND hWnd, LPDSQUERYPARAMS* ppDsQueryParams);


/*-----------------------------------------------------------------------------
/ CExampleQueryFormClassFactory
/----------------------------------------------------------------------------*/

#undef CLASS_NAME
#define CLASS_NAME CExampleQueryFormClassFactory
#include "unknown.inc"

STDMETHODIMP CExampleQueryFormClassFactory::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[] =
    {
        &IID_IClassFactory, (LPCLASSFACTORY)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}


/*-----------------------------------------------------------------------------
/ IClassFactory methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CExampleQueryFormClassFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID* ppvObject)
{
    HRESULT hr;
    CExampleQueryForm* pExampleQueryForm;

    if ( !ppvObject )
        ExitGracefully(hr, E_INVALIDARG, "ppvObject is NULL");

    if ( pUnkOuter )
        ExitGracefully(hr, CLASS_E_NOAGGREGATION, "Aggregation is not supported");

    // Construct an object, QI for the interface that the caller wants,
    // if that fails then destroy the object, otherwise we assume the RefCount > 0

    pExampleQueryForm = new CExampleQueryForm;

    if ( !pExampleQueryForm )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate CExampleQueryForm");

    hr = pExampleQueryForm->QueryInterface(riid, ppvObject);

    if ( FAILED(hr) )
        delete pExampleQueryForm;

exit_gracefully:

    return hr;
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CExampleQueryFormClassFactory::LockServer(BOOL fLock)
{
    return S_OK;
}


/*-----------------------------------------------------------------------------
/ CExampleQueryForm
/----------------------------------------------------------------------------*/

// IUnknown bits

#undef CLASS_NAME
#define CLASS_NAME CExampleQueryForm
#include "unknown.inc"

STDMETHODIMP CExampleQueryForm::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[] =
    {
        &IID_IQueryForm, (IQueryForm*)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}

/*-----------------------------------------------------------------------------
/ IQueryForm methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CExampleQueryForm::Initialize(THIS_ HKEY hkForm)
{
    // This method is called to initialize the query form object, it is called before
    // any pages are added.  hkForm should be ignored, in the future however it
    // will be a way to persist form state.

    return S_OK;
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CExampleQueryForm::AddForms(THIS_ LPCQADDFORMSPROC pAddFormsProc, LPARAM lParam)
{
    CQFORM cqf;

    // This method is called to allow the form handler to register its query form(s),
    // each form is identifiered by a CLSID and registered via the pAddFormProc.  Here
    // we are going to register a test form.
    
    // When registering a form which is only applicable to a specific task, eg. Find a Domain
    // object, it is advised that the form be marked as hidden (CQFF_ISNEVERLISTED) which 
    // will cause it not to appear in the form picker control.  Then when the
    // client wants to use this form, they specify the form identifier and ask for the
    // picker control to be hidden. 

    if ( !pAddFormsProc )
        return E_INVALIDARG;

    cqf.cbStruct = sizeof(cqf);
    cqf.dwFlags = 0x0;
    cqf.clsid = CLSID_ExampleQueryForm;
    cqf.hIcon = NULL;

// BUGBUG: This should be loaded from resource
    cqf.pszTitle = TEXT("An example form");

    return pAddFormsProc(lParam, &cqf);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CExampleQueryForm::AddPages(THIS_ LPCQADDPAGESPROC pAddPagesProc, LPARAM lParam)
{
    CQPAGE cqp;

    // AddPages is called after AddForms, it allows us to add the pages for the
    // forms we have registered.  Each page is presented on a seperate tab within
    // the dialog.  A form is a dialog with a DlgProc and a PageProc.  
    //
    // When registering a page the entire structure passed to the callback is copied, 
    // the amount of data to be copied is defined by the cbStruct field, therefore
    // a page implementation can grow this structure to store extra information.   When
    // the page dialog is constructed via CreateDialog the CQPAGE strucuture is passed
    // as the create param.

    if ( !pAddPagesProc )
        return E_INVALIDARG;

    cqp.cbStruct = sizeof(cqp);
    cqp.dwFlags = 0x0;
    cqp.pPageProc = PageProc_ExamplePage;
    cqp.hInstance = GLOBAL_HINSTANCE;
    cqp.idPageName = IDS_EXAMPLEPAGE;
    cqp.idPageTemplate = IDD_EXAMPLEPAGE;
    cqp.pDlgProc = DlgProc_ExamplePage;        
    cqp.lParam = (LPARAM)this;

    return pAddPagesProc(lParam, CLSID_ExampleQueryForm, &cqp);
}


/*---------------------------------------------------------------------------*/

// The PageProc is used to perform general house keeping and communicate between
// the frame and the page. 
//
// All un-handled, or unknown reasons should result in an E_NOIMPL response
// from the proc.  
//
// In:
//  pPage -> CQPAGE structure (copied from the original passed to pAddPagesProc)
//  hwnd = handle of the dialog for the page
//  uMsg, wParam, lParam = message parameters for this event
//
// Out:
//  HRESULT
//
// uMsg reasons:
// ------------
//  CQPM_INIIIALIZE
//  CQPM_RELEASE
//      These are issued as a result of the page being declared or freed, they 
//      allow the caller to AddRef, Release or perform basic initialization
//      of the form object.
//
// CQPM_ENABLE
//      Enable is when the query form needs to enable or disable the controls
//      on its page.  wParam contains TRUE/FALSE indicating the state that
//      is required.
//
// CQPM_GETPARAMETERS
//      To collect the parameters for the query each page on the active form 
//      receives this event.  lParam is an LPVOID* which is set to point to the
//      parameter block to pass to the handler, if the pointer is non-NULL 
//      on entry the form needs to appened its query information to it.  The
//      parameter block is handler specific. 
//
//      Returning S_FALSE from this event causes the query to be canceled.
//
// CQPM_CLEARFORM
//      When the page window is created for the first time, or the user clicks
//      the clear search the page receives a CQPM_CLEARFORM notification, at 
//      which point it needs to clear out the edit controls it has and
//      return to a default state.
//
// CQPM_PERSIST:
//      When loading of saving a query, each page is called with an IPersistQuery
//      interface which allows them to read or write the configuration information
//      to save or restore their state.  lParam is a pointer to the IPersistQuery object,
//      and wParam is TRUE/FALSE indicating read or write accordingly.
//
// CQPM_HELP:
//      This is received when help is being requested on the dialog.  wParam == 0,
//      lParam is a pointer to a LPHELPINFO structure.
//
// CQPM_SETDEFAULTPARAMETERS:
//      The form page rx's this to pass on the OPENQUERYWINDOWINFO structure.  wParam 
//      is true/false indicating if the page rx'ng this message is the default form (eg.
//      the one the caller of OpenQueryWindow info intended this structure for). lParam
//      is a pointer to the OPENQUERYWINDOWINFO structure, of which ppbFormParameters is
//      a pointer to an IPropertyBag object (used by the object to receive parameters)
//
// When querying the Directory Service the DS query handler supports some private
// form messages of its own:
//
// DSQPM_GETCLASSLIST:
//      In response to this the form should return the object classes it expects to
//      query, this is used both by the Advanced tab and the Choose Columns dialog 
//      so they can present the relevent property lists to the user.  wParam is reserved,
//      lParam is DSQUERYCLASSLIST** structure, if the pointer is already NULL then you
//      must add your classes to the list already present in the structure. 

HRESULT PageProc_ExamplePage(LPCQPAGE pQueryPage, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;
    CExampleQueryForm* pQueryForm = (CExampleQueryForm*)pQueryPage->lParam;

    switch ( uMsg )
    {
        // Initialize so AddRef the object we are associated with so that
        // we don't get unloaded.

        case CQPM_INITIALIZE:
            pQueryForm->AddRef();
            break;

        // Release, therefore Release the object we are associated with to
        // ensure correct destruction etc.

        case CQPM_RELEASE:
            pQueryForm->Release();
            break;

        // Enable so fix the state of our two controls within the window.

        case CQPM_ENABLE:
            EnableWindow(GetDlgItem(hwnd, IDC_EDITCN), wParam);
            break;

        // Fill out the parameter structure to return to the caller, this is 
        // handler specific.  In our case we constructure a query of the CN
        // and objectClass properties, and we show a columns displaying both
        // of these.  For further information about the DSQUERYPARAMs structure
        // see dsquery.h

        case CQPM_GETPARAMETERS:
            hr = GetQueryParams(hwnd, (LPDSQUERYPARAMS*)lParam);
            break;

        // Clear form, therefore set the window text for these two controls
        // to zero.

        case CQPM_CLEARFORM:
            SetDlgItemText(hwnd, IDC_EDITCN, TEXT(""));
            break;
            
        // persistance is not currently supported by this form.            
                  
        case CQPM_PERSIST:
        {
            BOOL fRead = (BOOL)wParam;
            IPersistQuery* pPersistQuery = (IPersistQuery*)lParam;

            if ( !pPersistQuery )
                ExitGracefully(hr, E_INVALIDARG, "No IPersistQuery interface given");

            hr = E_NOTIMPL;             // NYI
            break;
        }

        default:
        {
            hr = E_NOTIMPL;
            break;
        }
    }

exit_gracefully:

    return hr;
}


/*---------------------------------------------------------------------------*/

// The DlgProc is a standard Win32 dialog proc associated with the form
// window.  

BOOL CALLBACK DlgProc_ExamplePage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fResult = FALSE;
    LPCQPAGE pQueryPage;

    if ( uMsg == WM_INITDIALOG )
    {
        // pQueryPage will be of use later, so hang onto it by storing it
        // in the DWL_USER field of the dialog box instance.

        pQueryPage = (LPCQPAGE)lParam;
        SetWindowLong(hwnd, DWL_USER, (LONG)pQueryPage);

        Edit_LimitText(GetDlgItem(hwnd, IDC_EDITCN), MAX_PATH-1);
    }
    else
    {
        // pQueryPage can be retreived from the DWL_USER field of the
        // dialog structure, note however that in some cases this will
        // be NULL as it is set on WM_INITDIALOG.

        pQueryPage = (LPCQPAGE)GetWindowLong(hwnd, DWL_USER);
    }

    return fResult;
}

/*---------------------------------------------------------------------------*/

// Build a parameter block to pass to the query handler.  Each page is called
// with a pointer to a pointer which it must update with the revised query
// block.   For the first page this pointer is NULL, for subsequent pages
// the pointer is non-zero and the page must append its data into the
// allocation.
//
// Returning either and error or S_FALSE stops the query.   An error is
// reported to the user, S_FALSE stops silently.

#define FILTER_PREFIX   TEXT("(cn=")
#define FILTER_POSTFIX  TEXT(")")

struct
{
    INT fmt;
    INT cx;
    INT uID;
    LPCTSTR pDisplayProperty;
} 
columns[] =
{
    0, 50, IDS_CN, TEXT("cn"),
    0, 50, IDS_OBJECTCLASS, TEXT("objectClass"),
};

HRESULT GetQueryParams(HWND hWnd, LPDSQUERYPARAMS* ppDsQueryParams)
{
    HRESULT hr;
    LPDSQUERYPARAMS pDsQueryParams = NULL;
    TCHAR szCN[MAX_PATH];
    TCHAR szFilter[MAX_PATH+ARRAYSIZE(FILTER_PREFIX)+ARRAYSIZE(FILTER_POSTFIX)];
    ULONG offset, cbStruct = 0;
    INT i;
    
    // This page doesn't support appending its query data to an
    // existing DSQUERYPARAMS strucuture, only creating a new block,
    // therefore bail if we see the pointer is not NULL.

    if ( *ppDsQueryParams )
        ExitGracefully(hr, E_INVALIDARG, "Chaining not supported");

    // Compute the size of the argument block

    if ( !GetDlgItemText(hWnd, IDC_EDITCN, szCN, ARRAYSIZE(szCN)) )
        ExitGracefully(hr, S_FALSE, "No parameters");

    lstrcpy(szFilter, FILTER_PREFIX);
    lstrcat(szFilter, szCN);
    lstrcat(szFilter, FILTER_POSTFIX);

    offset = cbStruct = sizeof(DSQUERYPARAMS) + ((ARRAYSIZE(columns)-1)*sizeof(DSCOLUMN));
   
    cbStruct += StringByteSize(szFilter);
    cbStruct += StringByteSize(columns[0].pDisplayProperty);
    cbStruct += StringByteSize(columns[1].pDisplayProperty);

    // Allocate it and populate it with the data, the header is fixed
    // but the strings are referenced by offset.  StringByteSize and StringByteCopy
    // make handling this considerably easier.

    pDsQueryParams = (LPDSQUERYPARAMS)CoTaskMemAlloc(cbStruct);

    if ( !pDsQueryParams )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to alloc the parameter block");

    pDsQueryParams->cbStruct = cbStruct;
    pDsQueryParams->dwFlags = 0;
    pDsQueryParams->hInstance = GLOBAL_HINSTANCE;
    pDsQueryParams->offsetQuery = offset;
    pDsQueryParams->iColumns = ARRAYSIZE(columns);

    // Copy the filter string and bump the offset

    StringByteCopy(pDsQueryParams, offset, szFilter);
    offset += StringByteSize(szFilter);

    // Fill in the array of columns to dispaly, the cx is a percentage of the
    // current view, the propertie names to display are UNICODE strings and
    // are referenced by offset, therefore we bump the offset as we copy
    // each one.

    for ( i = 0 ; i < ARRAYSIZE(columns); i++ )
    {
        pDsQueryParams->aColumns[i].fmt = columns[i].fmt;
        pDsQueryParams->aColumns[i].cx = columns[i].cx;
        pDsQueryParams->aColumns[i].idsName = columns[i].uID;
        pDsQueryParams->aColumns[i].offsetProperty = offset;

        StringByteCopy(pDsQueryParams, offset, columns[i].pDisplayProperty);
        offset += StringByteSize(columns[i].pDisplayProperty);
    }
   
    // Success, therefore set the pointer to referenece this parameter
    // block and return S_OK!

    *ppDsQueryParams = pDsQueryParams;
    hr = S_OK;

exit_gracefully:

    if ( FAILED(hr) && pDsQueryParams )
        CoTaskMemFree(pDsQueryParams);
    
    return hr;
}
