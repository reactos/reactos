//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       browser.cxx
//
//  Contents:   Project browser implementation
//
//  Classes:    CBrowserForm
//
//  History:    3-30-95 JuliaC  Created
//              5-23-95 kfl     converted WCHAR to OLECHAE
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#include "browser.hxx"
#include "coreguid.h"
#include "coredisp.h"
#include "project.hxx"
#include "hproject.hxx"
#include "hprojitm.hxx"
#include "imdata.h"
#include "imdataid.h"
#include <ctrlrc.h>

// BUGBUG since acb.hxx is not in this directory,
// It fails to #include "acb.hxx"
extern
HRESULT GetFramedAcb(IUnknown * pUnk, IFormsToolbox ** ppAcb);

DeclareTag(tagBrowser, "Browser", "Browser methods");

int              CBrowserForm::s_iFolder;
int              CBrowserForm::s_iTable;
int              CBrowserForm::s_iQuery;
int              CBrowserForm::s_iForm;
int              CBrowserForm::s_iField;
HBITMAP          CBrowserForm::s_hBmpFolder;
HBITMAP          CBrowserForm::s_hBmpField;
HIMAGELIST       CBrowserForm::s_hIml;           // handle to the image list
UINT             CBrowserForm::s_cUser;

CFormExtender::CLASSDESC    CBrowserForm::s_classdesc =
{
    {                                            // _classdescBase;
        &CLSID_CCDBrowserForm,                   // _pclsid;
    },
    &CLSID_CSubForm,                             // _pclsidForm;
    0,                                           // _ibExtensions;
    offsetof(CBrowserForm, _EventSink),          // _ibEventSink;
    &IID_IFormEvents,                            // _piidEvents
};

//+-------------------------------------------------------------------------
//
//  Method:     CreateBrowserForm
//
//  Synopsis:   Creates a new browser form aggregate instance
//
//--------------------------------------------------------------------------

CBase *
CreateBrowserForm(IUnknown * pUnkOuter)
{
    return new CBrowserForm(pUnkOuter);
}



//+-------------------------------------------------------------------------
//
//  CBrowserForm
//
//--------------------------------------------------------------------------


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::CBrowserForm
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------------

CBrowserForm::CBrowserForm(LPUNKNOWN pUnkOuter) : CFormExtender(pUnkOuter)
{
    TraceTag((tagBrowser, "CBrowserForm::CBrowserForm"));
}


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::~CBrowserForm
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------------

CBrowserForm::~CBrowserForm(void)
{
    TraceTag((tagBrowser, "CBrowserForm::~CBrowserForm"));

    FreeTreeViewResource();
    delete _pExpandedCookies;
}


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::Passivate
//
//  Synopsis:   Shuts down the Browser object.
//
//--------------------------------------------------------------------------

void
CBrowserForm::Passivate(void)
{
    TraceTag((tagBrowser, "CBrowserForm::Passivate"));

    ReleaseInterface(_pCtrlTreeView);
    ReleaseInterface(_pCtrlPageSel);

    ReleaseInterface(_pCtrlViewBtn);
    ReleaseInterface(_pCtrlDataBtn);
    ReleaseInterface(_pCtrlClearBtn);
    ReleaseInterface(_pCtrlFindMDC);
    ReleaseInterface(_pCtrlFindBtn);

    ReleaseInterface(_pProject);
    ReleaseInterface(_pTreeView);

    CFormExtender::Passivate();
}


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::OnLayout
//
//  Synopsis:   Customized layout for the Project Browser.
//
//  Vertical layout algoritm:
//
//  Case 1: Form's height is less than min height of tab
//          Hide all controls.
//
//  Case 2: PageSelector is shown, but its client area can not fit in min
//          height treeview, so treeview and all other controls are hidden.
//
//  Case 3: PageSelector's client area can only fit in treeview, then hide
//          all buttons, and treeview takes all the space
//
//  Case 4: PageSelector's client area can fit in min height of treeview and
//          buttons, buutons are layout at the bottom of PageSelector, treeview
//          take the rest of space.
//
//
//  Horizontal layout algorithm:
//      It is used to do the layout for buttons, label and textbox.
//      The order of controls from left to right is following:
//          View button, Data button, Find label, Find textbox, Clear button
//
//      Each of these control has fixed height and width, expect for textbox
//      which has a min width, if the total width is larger than needed, textbox
//      take all of then.
//
//      For given width, left button has high priority than right button, we try
//      to fit control from left to right. If part of control can not be fiited in
//      the whole control is hidden.
//
//  Note: There is space(gap) between controls
//--------------------------------------------------------------------------

void
CBrowserForm::OnLayout(void)
{
    long                cxForm = 0;
    long                cyForm = 0;
    IOleObject *        pOO;
    SIZEL               sizel;
    HRESULT             hr;
    ITabStripExpert *   pTabStripExpert = NULL;
    RECTL               rcl;
    long                lMinTreeViewHeight = HimetricFromVPix(20);  // BUGBUG
    long                lMinPageHeight = HimetricFromVPix(25);  // BUGBUG

    TraceTag((tagBrowser, "CBrowserForm::OnLayout"));

    hr = QueryInterface(IID_IOleObject, (void **) &pOO);
    if (hr)
        return;

    IGNORE_HR(pOO->GetExtent(DVASPECT_CONTENT, &sizel));
    cxForm = sizel.cx;
    cyForm = sizel.cy;

    pOO->Release();


    //
    //  case 1: hide all controls
    //
    if (cyForm < lMinPageHeight)
    {
        // BUGBUG : HideControl(_pCtrlPageSel) does not work
        IGNORE_HR(_pCtrlPageSel->SetVisible(FALSE));

        HideControl(_pCtrlTreeView);
        HideAllOtherControls();
        goto Cleanup;
    }

    IGNORE_HR(_pCtrlPageSel->SetVisible(TRUE));

    // do the layout for page selector
    _pCtrlPageSel->Move(0,0, cxForm, cyForm);

    hr = _pCtrlPageSel->QueryInterface(IID_ITabStripExpert,(void **)&pTabStripExpert);
    Assert(hr == NOERROR);
    hr = pTabStripExpert->GetClientArea(&rcl);
    if (hr)
        goto Cleanup;

    //
    //  case 2: hide treeview and all buttons
    //

    if ( (rcl.bottom - rcl.top) < lMinTreeViewHeight + 2*BROWSER_BORDER)
    {
        HideControl(_pCtrlTreeView);
        HideAllOtherControls();
        goto Cleanup;
    }

    //
    //  case 3: hide all buttons and fit treeview in pageselector
    //

    if ( (rcl.bottom - rcl.top) < (lMinTreeViewHeight +
                                BROWSER_GAP_Y +
                                BROWSER_BTN_HEIGHT +
                                2*BROWSER_BORDER ))
    {

         // if only treeview can fitted in, hide all buttons
         HideAllOtherControls();

        // do the layout for treeview
        _pCtrlTreeView->Move(
                rcl.left + BROWSER_BORDER,
                rcl.top + BROWSER_BORDER,
                rcl.right - rcl.left - 2*BROWSER_BORDER,
                rcl.bottom - rcl.top - 2*BROWSER_BORDER);
        goto Cleanup;
    }


    //
    //  case 4: Show all controls, do proper layout
    //

    // do the layout for treeview
    _pCtrlTreeView->Move(
            rcl.left + BROWSER_BORDER,
            rcl.top + BROWSER_BORDER,
            rcl.right - rcl.left - 2*BROWSER_BORDER,
            rcl.bottom - rcl.top - BROWSER_BTN_HEIGHT -
                    BROWSER_GAP_Y - 2*BROWSER_BORDER);

    // do the layout for other controls
    ArrangeOtherControls(
            rcl.left + BROWSER_BORDER,
            rcl.bottom - BROWSER_BTN_HEIGHT - BROWSER_BORDER,
            rcl.right - rcl.left - 2*BROWSER_BORDER);

Cleanup:
    ReleaseInterface(pTabStripExpert);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::MoveControl
//
//  Synopsis:   With the given width, check whether control can be shown
//              if yes, move the control, update the available witdh,
//              and return TRUE, otherwise, return FALSE.
//
//              return value indicate whether the control is visible.
//
//--------------------------------------------------------------------------
BOOL
CBrowserForm::MoveControl(
        IControl *pCtrl,
        long *pleft,
        long top,
        long width,
        long height,
        long lxGap,
        long * plAvalWidth)
{
    Assert(plAvalWidth);
    if (width <= *plAvalWidth)
    {
        pCtrl->Move(*pleft, top, width, height);
        *plAvalWidth = *plAvalWidth - width - lxGap;
        *pleft = *pleft + width + lxGap;
        return TRUE;
    }

    HideControl(pCtrl);

    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::ArrangeOtherControls
//
//  Synopsis:   Do the arrangement for other controls horizontaly
//
//--------------------------------------------------------------------------

void
CBrowserForm::ArrangeOtherControls(
        long left,
        long top,
        long width)
{
    struct CTRL_LAYOUT_INFO
    {
        IControl *  pCtrl;      // control pointer
        long        width;      // control width
        long        height;     // control height
        long        lxGap;      // gap follow the control
        BOOL        fTextBox;   // indicate if it is a textbox
    } acli[] =
    {
        {   _pCtrlViewBtn,
            BROWSER_VIEWBTN_WIDTH,
            BROWSER_BTN_HEIGHT,
            0,
            FALSE,
        },
        {   _pCtrlDataBtn,
            BROWSER_DATABTN_WIDTH,
            BROWSER_BTN_HEIGHT,
            0,
            FALSE,
        },
        {   _pCtrlFindBtn,
            BROWSER_FINDBTN_WIDTH,
            BROWSER_BTN_HEIGHT,
            0,
            FALSE,
        },
        {   _pCtrlFindMDC,
            HimetricFromVPix(30),
            BROWSER_BTN_HEIGHT,
            BROWSER_GAP_X,
            TRUE,
        },
        {   _pCtrlClearBtn,
            HimetricFromVPix(53),
            BROWSER_BTN_HEIGHT,
            0,
            FALSE,
        }
    };

    int     i;
    BOOL    fVisible    = TRUE;
    long    widthTemp   = width;
    long    leftTemp    = left;
    long    lCtrlWidth;

    for (i=0; i< ARRAY_SIZE(acli); i++)
    {
        if (fVisible)
        {
            //
            // check whether it is text box, if so, use the max width
            //  otherwise, use the default one
            //

            if (acli[i].fTextBox)
            {
                if (widthTemp >= (acli[i].width +
                         acli[i+1].width +
                         acli[i].lxGap))
                {
                    lCtrlWidth = widthTemp -
                            acli[i+1].width -
                            acli[i].lxGap;
                }
                else
                {
                    lCtrlWidth = (acli[i].width < widthTemp)?
                                    widthTemp : acli[i].width;
                }
            }
            else
            {
                lCtrlWidth = acli[i].width;
            }

            fVisible = MoveControl(
                    acli[i].pCtrl,
                    &leftTemp,
                    top,
                    lCtrlWidth,
                    acli[i].height,
                    acli[i].lxGap,
                    &widthTemp);
        }
        else
        {
            HideControl(acli[i].pCtrl);
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::HideAllOtherControls
//
//  Synopsis:   Hide all controls in the bottom of browser form
//
//--------------------------------------------------------------------------

void
CBrowserForm::HideAllOtherControls()
{
    HideControl(_pCtrlViewBtn);
    HideControl(_pCtrlDataBtn);
    HideControl(_pCtrlFindMDC);
    HideControl(_pCtrlFindBtn);
    HideControl(_pCtrlClearBtn);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::HideControl
//
//  Synopsis:   Hide control by moving it out of place
//
//--------------------------------------------------------------------------

void
CBrowserForm::HideControl(IControl *pCtrl)
{
    pCtrl->SetLeft(-100);
    pCtrl->SetTop(-100);
    pCtrl->SetWidth(1);
    pCtrl->SetHeight(1);
}



//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::InitBrowser
//
//  Synopsis:   Initializtion of project browser
//
//--------------------------------------------------------------------------

HRESULT
CBrowserForm::InitBrowser(void)
{
    HRESULT                 hr;
    ICommandButton *        pIViewBtn = NULL;
    ICommandButton *        pIDataBtn = NULL;
    ICommandButton *        pIClearBtn = NULL;
    ICommandButton *        pIFindBtn = NULL;
    TCHAR                   szClear[FORMS_BUFLEN+1];
    TCHAR                   szField[FORMS_BUFLEN+1];
    TCHAR                   szProperty[FORMS_BUFLEN+1];
    TCHAR                   szAction[FORMS_BUFLEN+1];
    int                     iRet;

    ITabStrip *             pIPageSel = NULL;
    ITabs *                 pITabs = NULL;
    IPicture *              pPicture = NULL;
    HBITMAP                 hBmpView;
    HBITMAP                 hBmpData;
    HBITMAP                 hBmpFind;

    TraceTag((tagBrowser, "CBrowserForm::InitBrowser"));

    //
    //  Create and Initialize page selector
    //

    hr = THR(Add(CLSID_CTabStrip,  &_pCtrlPageSel));
    if (hr)
        goto Cleanup;

    hr = _pCtrlPageSel->QueryInterface(IID_ITabStrip,(void **)&pIPageSel);
    if (hr)
        goto Cleanup;

    hr = THR(pIPageSel->GetTabs(&pITabs));
    if (hr)
        goto Cleanup;

    pITabs->Clear();

    iRet = LoadString(GetResourceHInst(), IDS_BROWSER_FIELD, szField, ARRAY_SIZE(szField));
    if ( 0 == iRet )
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    pITabs->Add(NULL, szField, NULL);

    iRet = LoadString(GetResourceHInst(),
                IDS_BROWSER_PROPERTY,
                szProperty,
                ARRAY_SIZE(szProperty));
    if ( 0 == iRet )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pITabs->Add(NULL, szProperty, NULL);

    iRet = LoadString(GetResourceHInst(),
                IDS_BROWSER_ACTION,
                szAction,
                ARRAY_SIZE(szAction));
    if ( 0 == iRet )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pITabs->Add(NULL, szAction, NULL);

    // Create Treeview
    hr = THR(Add(CLSID_CTreeViewControl, &_pCtrlTreeView));
    if (hr)
        goto Cleanup;

    // save the treeview interface for later use
    hr = _pCtrlTreeView->QueryInterface(IID_ISysCtrlWrapped, (void **)&_pTreeView);
    if (hr)
        goto Cleanup;

    hr = THR(Add(CLSID_CCommandButton, &_pCtrlViewBtn));
    if (hr)
        goto Cleanup;

    hr = THR(Add(CLSID_CCommandButton, &_pCtrlDataBtn));
    if (hr)
        goto Cleanup;

    hr = THR(Add(CLSID_CCommandButton, &_pCtrlClearBtn));
    if (hr)
        goto Cleanup;

    hr = THR(Add(CLSID_CCommandButton, &_pCtrlFindBtn));

    if (hr)
        goto Cleanup;

    hr = THR(Add(CLSID_CMorphDataControl, &_pCtrlFindMDC));
    if (hr)
        goto Cleanup;

    //
    // set picture for view button
    //

    hr = _pCtrlViewBtn->QueryInterface(IID_ICommandButton,(void **)&pIViewBtn);
    if (hr)
        goto Cleanup;

    hr = pIViewBtn->SetCaption(NULL);
    if (hr)
        goto Cleanup;

    hBmpView = LoadBitmap(g_hInstCore, MAKEINTRESOURCE(IDR_BROWSER_VIEW));
    if (!hBmpView)
        goto Cleanup;

    hr = THR(GetPictureFromBitmap(hBmpView, &pPicture));
    if (hr)
        goto Cleanup;

    hr = THR(pIViewBtn->SetPicture((IDispatch *)pPicture));
    if (hr)
        goto Cleanup;

    // Clear for next use
    ClearInterface(&pPicture);

    //
    // set picture for data button
    //

    hr = _pCtrlDataBtn->QueryInterface(IID_ICommandButton,(void **)&pIDataBtn);
    if (hr)
        goto Cleanup;

    hr = pIDataBtn->SetCaption(NULL);
    if (hr)
        goto Cleanup;

    hBmpData = LoadBitmap(GetResourceHInst(), MAKEINTRESOURCE(IDR_BROWSER_DATA));
    if (!hBmpView)
        goto Cleanup;

    hr = THR(GetPictureFromBitmap(hBmpData, &pPicture));
    if (hr)
        goto Cleanup;

    hr = THR(pIDataBtn->SetPicture((IDispatch *)pPicture));
    if (hr)
        goto Cleanup;

    // Clear for next use
    ClearInterface(&pPicture);

    //
    // set picture search button
    //

    hr = _pCtrlFindBtn->QueryInterface(IID_ICommandButton,(void **)&pIFindBtn);
    if (hr)
        goto Cleanup;

    hr = pIFindBtn->SetCaption(NULL);
    if (hr)
        goto Cleanup;

    hBmpFind = LoadBitmap(g_hInstCore, MAKEINTRESOURCE(IDR_BROWSER_FIND));
    if (!hBmpView)
        goto Cleanup;

    hr = THR(GetPictureFromBitmap(hBmpFind, &pPicture));
    if (hr)
        goto Cleanup;

    hr = THR(pIFindBtn->SetPicture((IDispatch *)pPicture));
    if (hr)
        goto Cleanup;

    //
    // set caption for clear button
    //

    hr = _pCtrlClearBtn->QueryInterface(IID_ICommandButton,(void **)&pIClearBtn);
    if (hr)
        goto Cleanup;

    iRet = LoadString(GetResourceHInst(), IDS_BROWSER_CLEAR, szClear, ARRAY_SIZE(szClear));
    if ( 0 == iRet )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = pIClearBtn->SetCaption(szClear);

Cleanup:

    ReleaseInterface(pIViewBtn);
    ReleaseInterface(pIDataBtn);
    ReleaseInterface(pIFindBtn);
    ReleaseInterface(pIClearBtn);
    ReleaseInterface(pIPageSel);
    ReleaseInterface(pITabs);
    ReleaseInterface(pPicture);
    RRETURN(hr);
}



//
//  IUnknown methods
//


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::PrivateQueryInterface
//
//  Synopsis:   Per IUnknown::QueryInterface, forwarded from private
//              unknown so it can be overridden.
//
//--------------------------------------------------------------------------

STDMETHODIMP
CBrowserForm::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    TraceTag((tagBrowser, "CBrowserForm::PrivateQueryInterface"));

    if (iid == IID_IProjBrowser)
    {
        *ppv = (IProjBrowser *) this;
    }
    else
    {
        return CFormExtender::PrivateQueryInterface(iid, ppv);
    }

    ((IUnknown *) *ppv)->AddRef();
    return S_OK;
}




//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::MakeAllConnections
//
//  Synopsis:   Hooks up the all connections
//
//--------------------------------------------------------------------------

HRESULT
CBrowserForm::MakeAllConnections(void)
{
    HRESULT hr;
    int     i;
    ITabStripExpert *   pTabStripX = NULL;

    TraceTag((tagBrowser, "CBrowserForm::MakeAllConnections"));

    static struct BUTTON_CONNECTION_INFO
    {
        int             cbOffsetSourcePtr;
        int             cbOffsetSink;
        const IID *     piid;
    } abci[] =
    {
        offsetof(CBrowserForm, _pCtrlViewBtn),  offsetof(CBrowserForm, _ViewBtnSink),   &IID_IDispatch,
        offsetof(CBrowserForm, _pCtrlDataBtn),  offsetof(CBrowserForm, _DataBtnSink),   &IID_IDispatch,
        offsetof(CBrowserForm, _pCtrlClearBtn), offsetof(CBrowserForm, _ClearBtnSink),  &IID_IDispatch,
        offsetof(CBrowserForm, _pCtrlFindBtn),  offsetof(CBrowserForm, _FindBtnSink),   &IID_IDispatch,
        offsetof(CBrowserForm, _pCtrlTreeView), offsetof(CBrowserForm, _TreeViewSink),  &IID_ITreeViewEvents,
        offsetof(CBrowserForm, _pCtrlPageSel),  offsetof(CBrowserForm, _PropertySink),  &IID_IPropertyNotifySink,
        offsetof(CBrowserForm, _pCtrlPageSel),  offsetof(CBrowserForm, _BrowserPSEventSink),  &IID_ITabStripEvents,
   };

    for (i = ARRAY_SIZE(abci) - 1; i >= 0; i--)
    {
        hr = THR(ConnectSink(
                *OFFSET_PTR(IUnknown **, this, abci[i].cbOffsetSourcePtr),
                *abci[i].piid,
                OFFSET_PTR(IUnknown *, this, abci[i].cbOffsetSink),
                NULL));
        if (hr)
            goto Cleanup;
    }


    hr = THR(_pCtrlPageSel->QueryInterface(IID_ITabStripExpert, (void **) &pTabStripX));
    if (hr)
        goto Cleanup;

    hr = THR(pTabStripX->SetTabStripExpertEvents(&_TSExpertEventSink));

Cleanup:
    ReleaseInterface(pTabStripX);
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::SetPage
//
//  Synopsis:   Make sure the project and page selector are sync
//
//--------------------------------------------------------------------------

HRESULT
CBrowserForm::SetPage(void)
{
    HRESULT hr;
    ITabStrip *             pIPageSel = NULL;
    long                    iPage;
    fmProjectLeafType       type;


    TraceTag((tagBrowser, "CBrowserForm::SetPage"));

    hr = _pCtrlPageSel->QueryInterface(IID_ITabStrip,(void **)&pIPageSel);
    Assert(hr == NOERROR);

    hr = pIPageSel->GetValue(&iPage);
    if (hr)
        goto Cleanup;

    switch(iPage)
    {
        case BROWSER_TAB_FIELD:
            type = fmProjectLeafTypeFields;
            break;

        case BROWSER_TAB_PROPERTY:
            type = fmProjectLeafTypeProperties;
            break;

        case BROWSER_TAB_ACTION:
            type = fmProjectLeafTypeMethods;
            break;

        default:
            // This should not happen
            Assert(FALSE);
    }
    hr = _pProject->FilterLeaves(type);

Cleanup:
    ReleaseInterface(pIPageSel);
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::DoContextMenu
//
//  Synopsis:   Disable Page Selector's context menu
//
//--------------------------------------------------------------------------

 HRESULT
 CBrowserForm::DoContextMenu(
         OLE_XPOS_HIMETRIC  x,
         OLE_YPOS_HIMETRIC  y,
         VARIANT_BOOL *     pfEnableDefault)
{
    TraceTag((tagBrowser, "CBrowserForm::DoContextMenu"));

    *pfEnableDefault = VB_FALSE;
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::OnViewButtonClick
//
//  Synopsis:   Handle the view button click event
//
//--------------------------------------------------------------------------
void
CBrowserForm::OnViewButtonClick(void)
{
    TraceTag((tagBrowser, "CBrowserForm::OnViewButtonClick"));
}


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::OnDataButtonClick
//
//  Synopsis:   Handle the data button click event
//
//--------------------------------------------------------------------------
void
CBrowserForm::OnDataButtonClick(void)
{
    TraceTag((tagBrowser, "CBrowserForm::OnDataButtonClick"));
}


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::OnFindButtonClick
//
//  Synopsis:   Handle the find button click event
//
//--------------------------------------------------------------------------
void
CBrowserForm::OnFindButtonClick(void)
{
    HRESULT         hr;
    BSTR            bstrPattern = NULL;
    IMorphDataControl * pIMDC = NULL;
    HTREEITEM       hItemSelect = NULL;
    HTREEITEM       hRoot;
    ULONG           ulCookies[PROJECT_MAX_COOKIE_ARYSIZE];
    UINT            uDepth;
    HTREEITEM       hItemFind;
    BOOL            fExpanded;
    BOOL            fFindFirst = FALSE;
    LRESULT         lResult;
    VARIANT         var;

    TraceTag((tagBrowser, "CBrowserForm::OnFindButtonClick"));

    hr = _pCtrlFindMDC->QueryInterface(IID_IMorphDataControl,(void **)&pIMDC);
    if (hr)
        goto Cleanup;

    Assert(pIMDC);
    hr = pIMDC->GetText(&bstrPattern);
    if (hr)
        goto Cleanup;

    if (!bstrPattern)
    {
        // no serach string
        hr = E_FAIL;
        goto Cleanup;
    }

    // set forcus for treeview
    VariantInit(&var);
    V_VT(&var) = VT_EMPTY;
    hr = _pCtrlTreeView->SetFocus(var);

    //
    // if there is current selection, search start form
    // selection, otherwise, search starts form begin
    //
    _pTreeView->SendWrappedMsg(
                    TVM_GETNEXTITEM,
                    TVGN_CARET,
                    NULL,
                    (long *)&hItemSelect);
    if (hItemSelect)
    {
        //
        // there is selected item, search from it
        //

        hr = THR(FindCookies(hItemSelect, ulCookies, &uDepth));
        if (hr)
            goto Cleanup;

        //
        //  if selected item is folder, start from it first child
        //

        hr = THR(_pProject->Goto(ulCookies, uDepth));
        if (hr)
            goto Cleanup;

        hr = _pProject->IsItemExpanded(&fExpanded);
        if (hr)
            goto Cleanup;

        if (fExpanded)
        {
            hr = _pProject->Push();
            if (hr)
                goto Cleanup;
        }

        // do the search
        hr = _pProject->FindNext(bstrPattern);

        // if search fails, search it again from begin
        if (hr)
        {
            fFindFirst = TRUE;
        }
    }
    else
    {
        fFindFirst = TRUE;
    }


    // search start form beginning
    if (fFindFirst)
    {
        hr = THR(_pProject->GotoRoot());
        if (hr)
            goto Cleanup;

        // do the search
        hr = _pProject->FindNext(bstrPattern);
        if (hr)
            goto Cleanup;
    }

    //
    // select the treeview item corresponding to the node
    //

    hr = THR(_pProject->GetItemPath(ulCookies, &uDepth));
    if (hr)
        goto Cleanup;

    // Note: the root is the first node in the top level. (All Table)
   _pTreeView->SendWrappedMsg(
                    TVM_GETNEXTITEM,
                    (WPARAM) TVGN_ROOT,
                    NULL,
                    (long *)&hRoot);
    if (!hRoot)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(FindTreeViewItem(ulCookies, uDepth, hRoot, &hItemFind));
    if (hr)
        goto Cleanup;

    _pTreeView->SendWrappedMsg(
                    TVM_SELECTITEM,
                    (WPARAM) TVGN_CARET,
                    (LPARAM) hItemFind,
                    &lResult);
    if (!lResult)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

   _pTreeView->SendWrappedMsg(
                    TVM_ENSUREVISIBLE,
                    0,
                    (LPARAM) hItemFind,
                    &lResult);

    hr = lResult? S_OK: E_FAIL;

Cleanup:
    ReleaseInterface(pIMDC);
    FormsFreeString(bstrPattern);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::OnClearButtonClick
//
//  Synopsis:   Handle the clear button click event
//
//--------------------------------------------------------------------------
void
CBrowserForm::OnClearButtonClick(void)
{
}



//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::InitProject
//
//  Synopsis:   Initialize project object
//
//--------------------------------------------------------------------------

HRESULT
CBrowserForm::InitProject(void)
{
    HRESULT             hr;

    TraceTag((tagBrowser, "CBrowserForm::InitProject"));

    _pProject = new CProject(this);

    if (NULL == _pProject)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // cast is required here
    hr = ((CProject *) _pProject)->Init();
    if (hr)
        goto Error;

Cleanup:

    RRETURN(hr);

Error:
    ClearInterface(&_pProject);
    goto Cleanup;
}


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::AddTreeViewItem
//
//  Synopsis:   Add item to TreeView control
//
//--------------------------------------------------------------------------
HRESULT
CBrowserForm::AddTreeViewItem(
        HTREEITEM   hParent,
        HTREEITEM   hInsertAfter,
        int         iImage,
        int         iSelectedImage,
        LPTSTR    szText,
        LPARAM      lParam,
        HTREEITEM * phItem)
{
    TV_ITEM                 tvItem;
    TV_INSERTSTRUCT         tvIns;

    TraceTag((tagBrowser, "CBrowserForm::AddTreeViewItem"));

    memset(&tvItem, 0, sizeof(TV_ITEM));

    // Set which attribytes we are going to fill out.
    tvItem.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM ;

    // Set the attribytes
    tvItem.pszText          = szText;
    tvItem.cchTextMax       = _tcsclen(szText);
    tvItem.iImage           = iImage;
    tvItem.iSelectedImage   = iSelectedImage;
    tvItem.lParam           = lParam;

    // Fill out the TV_INSERTSTRUCT
    tvIns.hParent           = hParent;
    tvIns.hInsertAfter      = hInsertAfter;
    tvIns.item              = tvItem;

    // And insert the item, returning its handle
    _pTreeView->SendWrappedMsg(
                    TVM_INSERTITEM,
                    0,
                    (LPARAM) &tvIns,
                    (long *) phItem);

    return *phItem ? S_OK: E_FAIL;
}


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::OnTreeViewVisible
//
//  Synopsis:   Called when the treeview becomes visible/not visible
//
//--------------------------------------------------------------------------
HRESULT
CBrowserForm::OnTreeViewVisible(BOOL fVisible)
{
    HRESULT         hr;
    HTREEITEM       hRoot = NULL;

    TraceTag((tagBrowser, "CBrowserForm::OnTreeViewVisible"));

    if (fVisible)
    {
        hr = AllocTreeViewResource();
        if (hr)
            goto Cleanup;

        if (!_fFindHostProj)
        {
            // BUGBUG: When we know how to set the host project,
            // this will be removed
            _fFindHostProj = TRUE;
            hr = FindHostProject();
            if (hr)
                goto Cleanup;
        }

        hr = _pProject->GotoRoot();
        if (hr)
            goto Cleanup;

        if (_pExpandedCookies)
        {
            // Restore the treeview after hidding
            hr = THR(RestoreTreeViewLevel((HTREEITEM) TVI_ROOT));
        }
        else
        {
            // Populate top-level nodes.
            hr = THR(AddNodeChildren((HTREEITEM) TVI_ROOT));
        }
    }
    else
    {
        FreeTreeViewResource();

        if (!_pExpandedCookies)
        {
            // the first time of treeview is hidden
            _pExpandedCookies = new CAryCookie();
        }
        _pExpandedCookies->DeleteAll();

        // Note: the root is the first node in the top level. (All Table)
       _pTreeView->SendWrappedMsg(
                        TVM_GETNEXTITEM,
                        (WPARAM) TVGN_ROOT,
                        NULL,
                        (long *)&hRoot);
        if (!hRoot)
        {
            hr = E_FAIL;
            goto Cleanup;
        }
        else
        {
            hr = THR(SaveExpandedNodeCookies(hRoot));
        }
    }

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::OnTreeViewExpanding
//
//  Synopsis:   Handle TreeView expanding event
//
//--------------------------------------------------------------------------
HRESULT
CBrowserForm::OnTreeViewExpanding(HTREEITEM hExpandingItem)
{
    HRESULT             hr = S_OK;
    HTREEITEM           hChildItem;
    ULONG               ulCookies[PROJECT_MAX_COOKIE_ARYSIZE];
    UINT                uDepth;
    TV_ITEM             item;
    LRESULT             lResult;

    TraceTag((tagBrowser, "CBrowserForm::OnTreeViewExpanding"));

    _pTreeView->SendWrappedMsg(
                    TVM_GETNEXTITEM,
                    TVGN_CHILD,
                    (LPARAM)hExpandingItem,
                    (long *)&hChildItem);
    if (!hChildItem)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    memset(&item, 0, sizeof(TV_ITEM));
    item.hItem = hChildItem;
    item.mask = TVIF_PARAM;

    _pTreeView->SendWrappedMsg(TVM_GETITEM, 0, (LPARAM) &item, &lResult);
    if (!lResult)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (item.lParam > BROWSER_BAD_COOKIE)
        goto Cleanup;

    // remove dummy node
    _pTreeView->SendWrappedMsg(
                    TVM_DELETEITEM,
                    0,
                    (LPARAM) hChildItem,
                    &lResult);
    if (!lResult)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(FindCookies(hExpandingItem,ulCookies, &uDepth));
    if (hr)
        goto Cleanup;

    // do the expanding
    hr = THR(ExpandingTreeViewNode(hExpandingItem, ulCookies, uDepth));

Cleanup:
    RRETURN(hr);
}



//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::ExpandingTreeViewNode
//
//  Synopsis:   Expanding the TreeView node
//
//--------------------------------------------------------------------------

HRESULT
CBrowserForm::ExpandingTreeViewNode(
        HTREEITEM hParentItem,
        ULONG * pulCookie,
        UINT  uDepth)
{
    HRESULT         hr;

    TraceTag((tagBrowser, "CBrowserForm::ExpandingTreeViewNode"));

    Assert(_pProject);

    hr = THR(_pProject->Goto(pulCookie, uDepth));
    if (hr)
        goto Cleanup;

    hr = THR(_pProject->Push());
    if (hr)
        goto Cleanup;

    hr = THR(AddNodeChildren(hParentItem));

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::AddNodeChildren
//
//  Synopsis:   Adding children to the give node
//
//--------------------------------------------------------------------------

HRESULT
CBrowserForm::AddNodeChildren(HTREEITEM hParentItem)
{
    BSTR            bstrName = NULL;
    fmProjectItemType    pit;
    ULONG           ulCookie;
    HTREEITEM       hChildItem;
    HTREEITEM       hDummyItem;
    HRESULT         hr = S_OK;

    TraceTag((tagBrowser, "CBrowserForm::AddNodeChildren"));

    Assert(_pProject);

    while (S_OK == hr)
    {
        hr = _pProject->NextItem();
        if (hr)
        {
             return S_OK;    // reach to the end of loop
        }

        hr = THR(_pProject->GetItemDetails(
                        &pit,
                        &bstrName,
                        &ulCookie));
        if (hr)
            goto Cleanup;

        if ( (fmProjectItemTypeRoot != pit) && (fmProjectItemTypeFolder != pit))
        {
            hr = THR(AddTreeViewItem(
                        hParentItem,
                        (HTREEITEM) TVI_LAST,
                        s_iField,
                        s_iField,
                        bstrName,
                        (LPARAM)(ulCookie),
                        &hChildItem));
            if (hr)
                goto Cleanup;
        }
        else
        {
            TCHAR   szDummy[FORMS_BUFLEN+1];
            int     iRet;

            hr = THR(AddTreeViewItem(
                        hParentItem,
                        (HTREEITEM) TVI_LAST,
                        s_iFolder,
                        s_iFolder,
                        bstrName,
                        (LPARAM)(ulCookie),
                        &hChildItem));
            if (hr)
                goto Cleanup;



            iRet = LoadString(GetResourceHInst(),
                        IDS_BROWSER_DUMMY,
                        szDummy,
                        ARRAY_SIZE(szDummy));
            if ( 0 == iRet )
            {
                hr = E_FAIL;
                goto Cleanup;
            }

            hr = THR(AddTreeViewItem(
                        hChildItem,
                        (HTREEITEM) TVI_LAST,
                        s_iField,
                        s_iField,
                        szDummy,
                        BROWSER_BAD_COOKIE,
                        &hDummyItem));

            if (hr)
                goto Cleanup;
        }

        FormsFreeString(bstrName);
        bstrName = NULL;
    }

Cleanup:
    FormsFreeString(bstrName);
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::FindCookies
//
//  Synopsis:   Find the cookie array for the given node
//
//--------------------------------------------------------------------------

HRESULT
CBrowserForm::FindCookies(HTREEITEM hItem,
                    ULONG * pulCookie,
                    UINT * puDepth)
{
    HRESULT         hr = S_OK;
    TV_ITEM         item;
    HTREEITEM       hParent;
    HTREEITEM       hTemp;
    UINT            index =0;
    LRESULT         lResult;

    TraceTag((tagBrowser, "CBrowserForm::FindCookies"));

    Assert(hItem);

    hTemp = hItem;
    while ( NULL != hTemp)
    {
        memset(&item, 0, sizeof(TV_ITEM));
        item.hItem = hTemp;
        item.mask = TVIF_PARAM;

        _pTreeView->SendWrappedMsg(
                        TVM_GETITEM,
                        0,
                        (LPARAM) &item,
                        &lResult);
        if (!lResult)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        pulCookie[index++] = (ULONG)item.lParam;

        _pTreeView->SendWrappedMsg(
                        TVM_GETNEXTITEM,
                        (WPARAM) TVGN_PARENT,
                        (LPARAM) hTemp,
                        (long *)&hParent);
        hTemp = hParent;
    }

    *puDepth = index;
Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::AllocTreeViewResource
//
//  Synopsis:   Initialize the TreeView control by allocing
//              its resource
//
//--------------------------------------------------------------------------
HRESULT
CBrowserForm::AllocTreeViewResource(void)
{
    HRESULT         hr = S_OK;
    LRESULT         lResult;

    TraceTag((tagBrowser, "CBrowserForm::AllocTreeViewResource"));

    Assert(_pTreeView);

    if (!s_cUser)
    {

        // create image list
        s_hIml = ImageList_Create(TREEVIEW_BITMAP_WIDTH,
                            TREEVIEW_BITMAP_HEIGHT,
                            FALSE,
                            TREEVIEW_IMAGELIST_INIT,
                            TREEVIEW_IMAGELIST_INIT );
        if (s_hIml == NULL)
        {
            TraceTag((tagError, "ImageList_Create failed"));
            hr = E_FAIL;
            goto Cleanup;
        }

        s_hBmpFolder = LoadBitmap(g_hInstCore, MAKEINTRESOURCE(IDR_TV_FOLDER));
        if ( NULL == s_hBmpFolder)
        {
            TraceTag((tagError, "LoadBitmap for Folder failed"));
            hr = E_FAIL;
            goto Cleanup;
        }

        s_iFolder = ImageList_Add(s_hIml, s_hBmpFolder, NULL);
        if (-1 == s_iFolder)
        {
            TraceTag((tagError, "ImageList_Add for folder failed"));
            hr = E_FAIL;
            goto Cleanup;
        }

        s_iTable = ImageList_Add(s_hIml, s_hBmpFolder, NULL);
        if (-1 == s_iTable)
        {
            TraceTag((tagError, "ImageList_Add for table failed"));
            hr = E_FAIL;
            goto Cleanup;
        }

        s_iQuery = ImageList_Add(s_hIml, s_hBmpFolder, NULL);
        if (-1 == s_iQuery)
        {
            TraceTag((tagError, "ImageList_Add for query failed"));
            hr = E_FAIL;
            goto Cleanup;
        }

        s_iForm = ImageList_Add(s_hIml, s_hBmpFolder, NULL);
        if (-1 == s_iForm)
        {
            TraceTag((tagError, "ImageList_Add for table failed"));
            hr = E_FAIL;
            goto Cleanup;
        }

        s_hBmpField = LoadBitmap(g_hInstCore, MAKEINTRESOURCE(IDR_TV_FIELD));
        if ( NULL == s_hBmpField)
        {
            TraceTag((tagError, "LoadBitmap for Field failed"));
            hr = E_FAIL;
            goto Cleanup;
        }

        s_iField = ImageList_Add(s_hIml, s_hBmpField, NULL);
        if (-1 == s_iField)
        {
            TraceTag((tagError, "ImageList_Add for field failed"));
            hr = E_FAIL;
            goto Cleanup;
        }
    }

    // Associate the image list with the tree
    _pTreeView->SendWrappedMsg(
                    TVM_SETIMAGELIST,
                     s_iFolder,
                     (LPARAM)(UINT) s_hIml,
                     &lResult);
    // Increament the TreeView user count
    s_cUser++;

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::FreeTreeViewResource
//
//  Synopsis:   Release the resource for the TreeView
//
//--------------------------------------------------------------------------

void
CBrowserForm::FreeTreeViewResource(void)
{
    TraceTag((tagBrowser, "CBrowserForm::FreeTreeViewResource"));

    if (!s_cUser)
        return;

    // Decrease one user
    s_cUser--;

    // If no more user, free the resource
    if (!s_cUser)
    {
        if (s_hIml)
        {
            ImageList_Destroy(s_hIml);
            s_hIml = NULL;
        }
        if (s_hBmpFolder)
        {
            DeleteObject(s_hBmpFolder);
            s_hBmpFolder = NULL;
        }
        if (s_hBmpField)
        {
            DeleteObject(s_hBmpField);
            s_hBmpField = NULL;
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::SaveExpandedNodeCookies
//
//  Synopsis:   Save cookies for the expanded nodes, it is used for
//              restore the treeview shape.
//
//--------------------------------------------------------------------------

HRESULT
CBrowserForm::SaveExpandedNodeCookies(HTREEITEM hItem)
{
    HTREEITEM       hItemTemp;
    HTREEITEM       hChild = NULL;
    HTREEITEM       hNextItem;
    HRESULT         hr = S_OK;
    TV_ITEM         item;
    LRESULT         lResult;

    TraceTag((tagBrowser, "CBrowserForm::SaveExpandedNodeCookies"));

    Assert(_pTreeView);
    Assert(_pExpandedCookies);

    // loop on the item's sibling
    hItemTemp = hItem;
    while ((hr == S_OK) && hItemTemp)
    {
        //
        //  check if item is expanded, if so, save the cookie
        //

        memset(&item, 0, sizeof(TV_ITEM));
        item.hItem = hItemTemp;
        item.mask = TVIF_PARAM;
        _pTreeView->SendWrappedMsg(
                        TVM_GETITEM,
                        0,
                        (LPARAM) &item,
                        &lResult);
        if (!lResult)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if (item.state & TVIS_EXPANDED)
        {
            _pExpandedCookies->AppendIndirect(&(item.lParam));
        }

        //
        // if item has child, save its cookie
        //
        _pTreeView->SendWrappedMsg(
                        TVM_GETNEXTITEM,
                        (WPARAM) TVGN_CHILD,
                        (LPARAM) hItemTemp,
                        (long *)&hChild);
        if (hChild)
        {
            hr = SaveExpandedNodeCookies(hChild);
            if (hr)
                goto Cleanup;
        }

        // get the next item, it is NULL when reach end of loop
        _pTreeView->SendWrappedMsg(
                        TVM_GETNEXTITEM,
                        (WPARAM) TVGN_NEXT,
                        (LPARAM) hItemTemp,
                        (long *)&hNextItem);
        hItemTemp = hNextItem;
    }

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::RestoreTreeViewLevel
//
//  Synopsis:   Recursive function, used to restore the treeview
//
//--------------------------------------------------------------------------

HRESULT
CBrowserForm::RestoreTreeViewLevel(HTREEITEM hItem)
{
    BSTR            bstrName = NULL;
    fmProjectItemType    pit;
    ULONG           ulCookie;
    HTREEITEM       hChildItem;
    HTREEITEM       hDummyItem;
    HRESULT         hr = S_OK;
    int             iBmp = s_iFolder;
    int             iIndex;
    BOOL            fExpanded;
    LRESULT         lResult;

    TraceTag((tagBrowser, "CBrowserForm::RestoreTreeViewLevel"));

    Assert(_pProject);

    while (S_OK == hr)
    {
        hr = _pProject->NextItem();
        if (hr)
            return S_OK;

        hr = THR(_pProject->GetItemDetails(
                        &pit,
                        &bstrName,
                        &ulCookie));
        if (hr)
            goto Cleanup;

        if ( (fmProjectItemTypeRoot != pit) && (fmProjectItemTypeFolder != pit))
        {
            iBmp = s_iField;
        }

        hr = THR(AddTreeViewItem(
                    hItem,
                    (HTREEITEM) TVI_LAST,
                    iBmp,
                    iBmp,
                    bstrName,
                    (LPARAM)(ulCookie),
                    &hChildItem));
        if (hr)
            goto Cleanup;

        if (iBmp == s_iFolder)
        {

            hr = _pProject->IsItemExpanded(&fExpanded);
            if (hr)
                goto Cleanup;

            if (fExpanded)
            {
                hr = _pProject->Push();
                if (hr)
                    goto Cleanup;

                hr = THR(RestoreTreeViewLevel(hChildItem));
                if (hr)
                    goto Cleanup;

                hr = _pProject->Pop();
                if (hr)
                    goto Cleanup;

                iIndex = _pExpandedCookies->FindIndirect(&ulCookie);
                if (iIndex >=0 )
                {
                    // It is OK for the Expand fail

                    _pTreeView->SendWrappedMsg(
                                    TVM_EXPAND,
                                    TVE_EXPAND,
                                    (LPARAM) hChildItem,
                                    &lResult);
                }
            }
            else
            {
                TCHAR   szDummy[FORMS_BUFLEN+1];
                int     iRet;



                iRet = LoadString(GetResourceHInst(),
                            IDS_BROWSER_DUMMY,
                            szDummy,
                            ARRAY_SIZE(szDummy));
                if ( 0 == iRet )
                {
                    hr = E_FAIL;
                    goto Cleanup;
                }

                hr = THR(AddTreeViewItem(
                            hChildItem,
                            (HTREEITEM) TVI_LAST,
                            s_iField,
                            s_iField,
                            szDummy,
                            BROWSER_BAD_COOKIE,
                            &hDummyItem));
                if (hr)
                    goto Cleanup;
            }
        }

        FormsFreeString(bstrName);
        bstrName = NULL;
    }

Cleanup:
    FormsFreeString(bstrName);
    RRETURN(hr);
}

#pragma warning (disable: 4702)
// BUGBUG Do not know what cause this warning

//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::FindTreeViewItem
//
//  Synopsis:   Recursive function, find treeview item specified by cookie path
//
//--------------------------------------------------------------------------

HRESULT
CBrowserForm::FindTreeViewItem(
                    ULONG * pulCookies,     // point to cookie array
                    UINT uDepth,            // depth of cookie arry
                    HTREEITEM hItemStart,   // Start search point
                    HTREEITEM * phItemFind) // find treeview item
{
    HTREEITEM       hItemTemp;
    HTREEITEM       hChild = NULL;
    HTREEITEM       hNextItem;
    HRESULT         hr = S_OK;
    TV_ITEM         item;
    LRESULT         lResult;

    TraceTag((tagBrowser, "CBrowserForm::FindTreeViewItem"));

    Assert(_pTreeView);

    // loop on the item's sibling
    hItemTemp = hItemStart;
    while ((hr == S_OK) && hItemTemp)
    {
        //
        //  check if item is expanded, if so, save the cookie
        //

        memset(&item, 0, sizeof(TV_ITEM));
        item.hItem = hItemTemp;
        item.mask = TVIF_PARAM;
        _pTreeView->SendWrappedMsg(TVM_GETITEM, 0, (LPARAM) &item, &lResult);
        if (!lResult)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if ( (item.lParam) == (LPARAM) pulCookies[0] )
        {
            // Find one on the cookie path
            if (1 == uDepth)
            {
                *phItemFind = hItemTemp;
                return S_OK;
            }
            else
            {
                _pTreeView->SendWrappedMsg(
                                TVM_GETNEXTITEM,
                                (WPARAM) TVGN_CHILD,
                                (LPARAM) hItemTemp,
                                (long *)&hChild);
                if (hChild)
                {
                    hr = FindTreeViewItem(pulCookies + 1, uDepth-1, hChild, phItemFind);
                }
                else
                {
                    hr = E_FAIL;  // BUGBUG
                }
                goto Cleanup;   // End of search
            }
        }

        // get the next item, it is NULL when reach end of loop
        _pTreeView->SendWrappedMsg(
                        TVM_GETNEXTITEM,
                        (WPARAM) TVGN_NEXT,
                        (LPARAM) hItemTemp,
                        (long *)&hNextItem);

        hItemTemp = hNextItem;
    }

Cleanup:
    RRETURN(hr);
}

#pragma warning (default: 4702)

//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::DeleteTreeViewItem
//
//  Synopsis:   Delete a leaf item from treeview specified by cookie path
//
//--------------------------------------------------------------------------

HRESULT
CBrowserForm::DeleteTreeViewItem(ULONG * pulCookies, UINT uDepth)
{
    HRESULT         hr;
    HTREEITEM       hItem = NULL;
    HTREEITEM       hRoot;
    LRESULT         lResult;

    TraceTag((tagBrowser, "CBrowserForm::DeleteTreeViewItem"));

    // Note: the root is the first node in the top level. (All Table)
    _pTreeView->SendWrappedMsg(
                    TVM_GETNEXTITEM,
                    (WPARAM) TVGN_ROOT,
                    NULL,
                    (long *)&hRoot);
    if (!hRoot)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = FindTreeViewItem(pulCookies, uDepth, hRoot, &hItem);
    if (hr)
        goto Cleanup;

    _pTreeView->SendWrappedMsg(TVM_DELETEITEM, 0, (LPARAM) hItem, &lResult);
    if (!lResult)
        hr = E_FAIL;

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::AddLeafTreeViewItem
//
//  Synopsis:   Add a leaf item to treeview specified by cookie path
//
//--------------------------------------------------------------------------

HRESULT
CBrowserForm::AddLeafTreeViewItem(ULONG * pulCookies, UINT uDepth)
{
    HRESULT         hr;
    HTREEITEM       hParentItem = NULL;
    HTREEITEM       hItemAdd = NULL;
    HTREEITEM       hRoot;
    BSTR            bstrName = NULL;
    fmProjectItemType    pit;
    ULONG           ulCookie;

    TraceTag((tagBrowser, "CBrowserForm::AddLeafTreeViewItem"));

    // Note: the root is the first node in the top level. (All Table)
    _pTreeView->SendWrappedMsg(
                    TVM_GETNEXTITEM,
                    (WPARAM) TVGN_ROOT,
                    NULL,
                    (long *)&hRoot);
    if (!hRoot)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(_pProject->GetItemDetails(
                    &pit,
                    &bstrName,
                    &ulCookie));
    if (hr)
        goto Cleanup;

    hr = THR(FindTreeViewItem(pulCookies, uDepth-1, hRoot, &hParentItem));
    if (hr)
        goto Cleanup;

    hr = THR(AddTreeViewItem(
                hParentItem,
                (HTREEITEM) TVI_LAST,
                s_iField,
                s_iField,
                bstrName,
                (LPARAM)(ulCookie),
                &hItemAdd));
Cleanup:
    FormsFreeString(bstrName);
    RRETURN(hr);
}



//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::IsEmpty
//
//  Synopsis:   IProjBrowser
//
//              Check to see if host project exists
//
//--------------------------------------------------------------------------

STDMETHODIMP
CBrowserForm::IsEmpty(VARIANT_BOOL * pfEmpty)
{
    HRESULT     hr;

    if (!pfEmpty)
        return E_INVALIDARG;

    hr = _pProject->GotoRoot();
    if (hr)
        goto Cleanup;

    hr = ((CProject *)_pProject)->NextItemIgnoreFilter();

Cleanup:
//    *pfEmpty = hr ? VB_TRUE : VB_FALSE;
    // BUGBUG to be fix when we can check if host project exists,
    *pfEmpty =  VB_FALSE ;

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::FindHostProject
//
//  Synopsis:   Find host project object, if no host project, use default one
//
//--------------------------------------------------------------------------

HRESULT
CBrowserForm::FindHostProject()
{
    IFormsToolbox *     pAcb = NULL;
    IHostProject *      pHostProject = NULL;
    HRESULT             hr = S_OK;
    IMsoDataHost *      pMsoDataHost = NULL;
    INamespace *        pNamespace = NULL;
    IServiceProvider *  pServiceProvider = NULL;

    TraceTag((tagBrowser, "CBrowserForm::FindHostProject"));

    // Cook-up a new Host Project.
    pHostProject = new CHostProject();
    if (!pHostProject)
        return E_OUTOFMEMORY;

    // Get framed Acb where Browser is in.
    hr = GetFramedAcb(PunkOuter(), &pAcb);
    if (hr)
        goto Cleanup;

    hr = pAcb->QueryInterface(IID_IServiceProvider, (void **)&pServiceProvider);
    if (hr)
        goto Cleanup;

    hr = pServiceProvider->QueryService(SID_IMsoDataHost, IID_IMsoDataHost, (void **)&pMsoDataHost);
    // BUGBUG ignore error code, it fails in form3drt
    if (pMsoDataHost)
    {
        // BUGBUG we ignore error code
        hr = pMsoDataHost->GetNamespace(&pNamespace);
    }

    hr = ((CHostProject *) pHostProject)->Init(pNamespace);
    if (hr)
        goto Cleanup;

    hr = _pProject->SetHostProject(pHostProject);

Cleanup:
    ReleaseInterface(pAcb);
    ReleaseInterface(pMsoDataHost);
    ReleaseInterface(pServiceProvider);
    ReleaseInterface(pHostProject);
    ReleaseInterface(pNamespace);

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBrowserForm::GetPictureFromBitmap
//
//  Synopsis:   Creates an IPicture object from a bitmap.
//
//  Note:       This function is copied from CCtrlSelector
//--------------------------------------------------------------------------

HRESULT
CBrowserForm::GetPictureFromBitmap(HBITMAP hbmp, IPicture ** ppPicture)
{
    HRESULT             hr;
    PICTDESC            pd;
    IPicture *          pPicture = NULL;

    pd.cbSizeofstruct = sizeof(PICTDESC);
    pd.picType = PICTYPE_BITMAP;
    pd.bmp.hbitmap = hbmp;
    pd.bmp.hpal = NULL;

    hr = THR(OleCreatePictureIndirect(
                        &pd,
                        IID_IPicture,
                        TRUE,
                        (void **) &pPicture));
    if (hr)
        goto Cleanup;

    pPicture->AddRef();
    *ppPicture = pPicture;

Cleanup:
    ReleaseInterface(pPicture);
    RRETURN(hr);
}

