/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    queryfrm.cpp

Abstract:

    This module contains the implementation for the query form.

Author:

    Dave Hastings (daveh) creation-date 14-Nov-1997

Revision History:

--*/


#include "pubquery.h"
HRESULT
PageProc(
    LPCQPAGE QueryPage,
    HWND hwnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL CALLBACK
DlgProc(
    HWND hwnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    );

//
// creator/destructor
//
CPublishedApplicationQueryForm::CPublishedApplicationQueryForm(LPUNKNOWN IUnknown)
{
    InterlockedIncrement(&g_RefCount);
    m_IUnknown = IUnknown;
}

CPublishedApplicationQueryForm::~CPublishedApplicationQueryForm()
{
    InterlockedDecrement(&g_RefCount);
}

//
// IUnknown
//
STDMETHODIMP CPublishedApplicationQueryForm::QueryInterface(
    REFIID riid,
    PVOID *ppvInterface
    )
/*++

Routine Description:

    This is the query interface function for the query form.

Arguments:

    riid - Supplies the iid of the interface desired.
    ppvInterface - Returns the interface pointer.

Return Value:

--*/
{
    return m_IUnknown->QueryInterface(riid, ppvInterface);
}

STDMETHODIMP_(ULONG) CPublishedApplicationQueryForm::AddRef(
    VOID
    )
/*++

Routine Description:

    This is the AddRef entrypoint for the query form.

Arguments:

    None.

Return Value:

    New reference count

--*/

{
    return InterlockedIncrement(&m_RefCount);
}

STDMETHODIMP_(ULONG) CPublishedApplicationQueryForm::Release(
    VOID
    )
/*++

Routine Description:

    This is the Release function for the query form.

Arguments:

    None.

Return Value:

    the new ref count

--*/
{
    LONG NewRefCount;

    NewRefCount = InterlockedDecrement(&m_RefCount);

    if (NewRefCount == 0) {
        delete this;
    }

    return NewRefCount;
}


//
// IQueryForm
//
STDMETHODIMP CPublishedApplicationQueryForm::Initialize(
    THIS_ HKEY hKeyForm
    )
{
    return S_OK;
}

STDMETHODIMP CPublishedApplicationQueryForm::AddForms(
    THIS_ LPCQADDFORMSPROC pAddFormsProc,
    LPARAM lParam
    )
/*++

Routine Description:

    This function allows the QueryForm to add it's forms to the 
    query.

Arguments:

    pAddFormsProc - Supplies a pointer to the procedure called to add the
        forms.
    lParam - ??.

Return Value:

--*/
{
    CQFORM cqf;

    if (pAddFormsProc == NULL) {
        return E_INVALIDARG;
    }

    cqf.cbStruct = sizeof(CQFORM);
    // bugbug The example leads me to believe the correct value is
    //          CQFF_ISNEVERLISTED.  I didn't find this value in 
    //          the header file.
    cqf.dwFlags = 0x0;
    cqf.clsid = CLSID_PublishedApplicationQueryForm;
    cqf.hIcon = NULL;
    // bugbug NLS
    cqf.pszTitle = L"Available Applications";

    return pAddFormsProc(lParam, &cqf);
}

STDMETHODIMP CPublishedApplicationQueryForm::AddPages(
    THIS_ LPCQADDPAGESPROC pAddPagesProc,
    LPARAM lParam
    )
/*++

Routine Description:

    description-of-function.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/
{
    CQPAGE QueryPage;

    if (pAddPagesProc == NULL) {
        return E_INVALIDARG;
    }

    QueryPage.cbStruct = sizeof(CQPAGE);
    QueryPage.dwFlags = 0;
    QueryPage.pPageProc = PageProc;
    QueryPage.hInstance = Instance;
    QueryPage.idPageName = IDS_PUBAPP_QUERYTITLE;
    QueryPage.idPageTemplate = IDD_PUBAPP_QUERYFORM;
    QueryPage.pDlgProc = DlgProc;
    QueryPage.lParam = NULL;

    return pAddPagesProc(lParam, CLSID_PublishedApplicationQueryForm, &QueryPage);
}

HRESULT
PageProc(
    LPCQPAGE QueryPage,
    HWND hwnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    This is the page proc for the PublishedApplicationQueryForm.
    It handles messages from the frame.

Arguments:

    QueryPage - Supplies the page struct used to add this page.
    hwnd - Supplies the handle of this dialog's page.
    Msg - Supplies the event.
    wParam - Supplies the wParam for this message.
    lParam - Supplies the lParam for this message.

Return Value:

--*/
{
    HRESULT hr = E_NOTIMPL;
    PQUERYPARAMETERS QueryParameters;
    ULONG TextLength;

    switch (Msg) {

        case CQPM_INITIALIZE:

            OutputDebugString(L"PageProc -- Initialize\n");

            hr = S_OK;
            break;

        case CQPM_RELEASE:

            OutputDebugString(L"PageProc -- Release\n");

            hr = S_OK;
            break;

        case CQPM_ENABLE:

            OutputDebugString(L"PageProc -- Enable\n");

            EnableWindow(GetDlgItem(hwnd, IDC_PROGRAMNAME), wParam);
            EnableWindow(hwnd, TRUE);
            hr = S_OK;
            break;

        case CQPM_CLEARFORM:

            OutputDebugString(L"PageProc -- ClearForm\n");

            SetDlgItemText(hwnd, IDC_PROGRAMNAME, L"");
            hr = S_OK;
            break;

        case CQPM_GETPARAMETERS:

            OutputDebugString(L"PageProc -- GetParameters\n");

            //
            // Allocate buffers for the strings and the structure
            //
            QueryParameters = (PQUERYPARAMETERS)CoTaskMemAlloc(sizeof(QUERYPARAMETERS));

            TextLength = SendDlgItemMessage(
                hwnd,
                IDC_PROGRAMNAME,
                WM_GETTEXTLENGTH,
                0,
                0
                );

            QueryParameters->ApplicationName = (LPWSTR)CoTaskMemAlloc(TextLength);

            GetDlgItemText(
                hwnd, 
                IDC_PROGRAMNAME, 
                QueryParameters->ApplicationName, 
                TextLength
                );

            *((PQUERYPARAMETERS *)lParam) = QueryParameters;

            hr = S_OK;
            break;

        case CQPM_PERSIST:
            
            OutputDebugString(L"PageProc -- Persist\n");

            hr = E_NOTIMPL;
            break;

    }

    return hr;
}

BOOL CALLBACK
DlgProc(
    HWND hwnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    return FALSE;
}
