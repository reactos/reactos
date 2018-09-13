//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       taborder.cxx
//
//  Contents:   Tab order dialog source file
//
//  Classes:    COrderDlg
//
//  History:    12-Jul-95   t-AnandR    Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

BOOL CALLBACK OrderDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
int RTCCONV CompareLBItems(const void *pvElem1, const void *pvElem2);
HRESULT DoOrderDialog(
        CBase * pBase,
        int c,
        void **ppObjs,
        TCHAR **ppStrs,
        TCHAR *strCap,
        HWND hwndOwner);

enum MOVE_TYPE { MOVE_UP, MOVE_DOWN };

MtDefine(COrderDlg, Dialogs, "COrderDlg")
MtDefine(COrderDlgOnbtnMoveClick_pnItems, Dialogs, "COrderDlg::OnbtnMoveClick pnItems")

class COrderDlg
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(COrderDlg))

    void OnFormLoad(void);
    void OnbtnOkClick(void);
    void OnbtnCancelClick(void);
    void OnbtnMoveClick(MOVE_TYPE Move);

    COrderDlg(
        int c,
        void **ppObjs,
        TCHAR **ppStrs,
        TCHAR *strCap);
    void SetWindowHandle(HWND hwnd) { _hwnd = hwnd; }

private:
    HRESULT SwapLBItems(HWND hwndLB, long lElem1, long lElem2);

    void **             _ppObjs;// Array of objects.
    TCHAR **            _ppStrs;// Array of strings for listbox
    TCHAR *             _strCap;// Caption to display
    HWND                _hwnd;
    long                _c;     // Count of controls in listbox
};

//+-------------------------------------------------------------------------
//
//  Method:     COrderDlg
//
//  Synopsis:   constructor
//
//  History:    12-Jul-95   t-AnandR    Created
//
//--------------------------------------------------------------------------
COrderDlg::COrderDlg(
        int c,
        void **ppObjs,
        TCHAR **ppStrs,
        TCHAR *strCap)

{
    _hwnd = NULL;
    _c = c;
    _ppObjs = ppObjs;
    _ppStrs = ppStrs;
    _strCap = strCap;
}


//+-------------------------------------------------------------------------
//
//  Method:     SwapLBItems
//
//  Synopsis:   Swaps two items from the listbox
//
//  Arguments:  hwndLB  --  Handle to the listbox
//              lElem1  --  Index of element 1
//              lElem2  --  Index of element 2
//
//  History:    18-Jul-95   t-AnandR    Created
//
//--------------------------------------------------------------------------
HRESULT
COrderDlg::SwapLBItems(HWND hwndLB, long lElem1, long lElem2)
{
    long    cl;
    BSTR    bstr = NULL;
    long    lXObj;
    HRESULT hr = S_OK;
    long    lErr;

    // save the item
    cl = SendMessage(hwndLB, LB_GETTEXTLEN, (WPARAM)lElem1, 0);
    if (LB_ERR == cl)
    {
        hr = GetLastWin32Error();
        goto Cleanup;
    }

    if (cl > 0)
    {
        hr = FormsAllocStringLen(NULL, cl, &bstr);
        if (hr)
            goto Cleanup;

        Assert(bstr);
        lErr = SendMessage(hwndLB, LB_GETTEXT, lElem1, (LPARAM)bstr);
        if (LB_ERR == lErr)
        {
            hr = GetLastWin32Error();
            goto Cleanup;
        }

        lXObj = SendMessage(hwndLB, LB_GETITEMDATA, (WPARAM)lElem1, 0);
        if (LB_ERR == lXObj)
        {
            hr = GetLastWin32Error();
            goto Cleanup;
        }

        // delete and re-insert into swapping location
        lErr = SendMessage(hwndLB, LB_DELETESTRING, (WPARAM)lElem1, 0);
        if (LB_ERR == lErr)
        {
            hr = GetLastWin32Error();
            goto Cleanup;
        }

        lErr = SendMessage(hwndLB, LB_INSERTSTRING, lElem2, (LPARAM)bstr);
        if (LB_ERR == lErr)
        {
            hr = GetLastWin32Error();
            goto Cleanup;
        }

        lErr = SendMessage(hwndLB, LB_SETITEMDATA, lElem2, (LPARAM)lXObj);
        if (LB_ERR == lErr)
        {
            hr = GetLastWin32Error();
            goto Cleanup;
        }
    }

Cleanup:
    FormsFreeString(bstr);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     OnFormLoad
//
//  Synopsis:   puts up the dialog and loads info in it
//
//  History:    12-Jul-95   t-AnandR    Created
//
//--------------------------------------------------------------------------
void
COrderDlg::OnFormLoad(void)
{
    ULONG               i;
    LONG                lIndex;
    HWND                hwndLB;
    HWND                hwndTxt;

    // Set caption and window title to given string.
    hwndTxt = GetDlgItem(_hwnd, IDR_TABORDERLBL);
    SendMessage(hwndTxt, WM_SETTEXT, 0, (LPARAM)_strCap);

    // Before setting window text, iterate thru and remove and "&"s,
    // which are used as accelerators for the label.
    // ASSUMPTION: Only one & per string.
    for (i = 0; i < _tcslen(_strCap); i++)
    {
        if (_T('&') == _strCap[i])
        {
            memmove((void *)_strCap,
                    (void *)(_strCap + 1),
                    (_tcslen(_strCap) - i) * sizeof(TCHAR));
            break;
        }
    }

    SetWindowText(_hwnd, _strCap);

    hwndLB = GetDlgItem(_hwnd, IDR_TABORDERLSTBOX);

    // Loop through objects and insert into listbox in the given order.
    // Also put in the object ptr for the itemdata field.
    for (i = 0; i < (ULONG)_c; ++i)
    {
        lIndex = SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)_ppStrs[i]);
        if (LB_ERR == lIndex)
            continue;
        SendMessage(hwndLB, LB_SETITEMDATA, lIndex, (LPARAM)*(_ppObjs+i));
    }

    // If there are no items in the listbox, disable the moveup/movedown buttons
    if (_c <= 1)
    {
        HWND    hwndBtn;

        hwndBtn = GetDlgItem(_hwnd, IDR_BTNMOVEUP);
        EnableWindow(hwndBtn, FALSE);
        hwndBtn = GetDlgItem(_hwnd, IDR_BTNMOVEDOWN);
        EnableWindow(hwndBtn, FALSE);
    }

    // Also set focus on the listbox and select the first item
    SetFocus(hwndLB);
    SendMessage(hwndLB, LB_SETSEL, TRUE, 0);
}


//+-------------------------------------------------------------------------
//
//  Method:     OnbtnMoveClick
//
//  Synopsis:   Handles Move up/down button events
//
//  Arguments:  Move    --  Move up or down
//
//  History:    12-Jul-95   t-AnandR    Created
//
//--------------------------------------------------------------------------
void
COrderDlg::OnbtnMoveClick(MOVE_TYPE Move)
{
    HWND    hwndLB;
    long *  pnItems;
    long    n;
    long    i;
    long *  plSel;

    hwndLB = GetDlgItem(_hwnd, IDR_TABORDERLSTBOX);

    if (0 == _c)
        return;

    // Just find the index of the currently selected item
    // and move it up.
    n = SendMessage(hwndLB, LB_GETSELCOUNT, 0, 0);
    if (n <= 0)
    {
        // Nothing selected
        return;
    }

    // Get the list of selected items
    pnItems = new(Mt(COrderDlgOnbtnMoveClick_pnItems)) long[n];

    // Not enough memory to do anything, at least avoid the crash.
    if (pnItems == NULL)
        return;

    SendMessage(hwndLB, LB_GETSELITEMS, n, (LPARAM)pnItems);

    // Sort the array of selected items in increasing order
    qsort(pnItems, n, sizeof(long), CompareLBItems);

    switch (Move)
    {
    case MOVE_UP:
        // Iterate through the list of selected items in increasing
        // order, moving each element up by one
        for (plSel = pnItems, i = n; i-- > 0; plSel++)
        {
            // Exit if current is first
            if (0 == *plSel)
                goto Cleanup;

            // Otherwise swap current with previous one
            SwapLBItems(hwndLB, *plSel, *plSel- 1);

            // Reselect the old item
            SendMessage(hwndLB, LB_SETSEL, TRUE, (LPARAM)*plSel - 1);
        }
        break;

    case MOVE_DOWN:
        // Iterate through the list of selected items in deceasing
        // order, moving each element up by one
        for (plSel = pnItems + n - 1, i = n; i-- > 0; plSel--)
        {
            // Exit if current is last
            if (_c - 1 == *plSel)
                goto Cleanup;

            // Otherwise swap current with next one
            SwapLBItems(hwndLB, *plSel, *plSel+ 1);

            // Reselect the old item
            SendMessage(hwndLB, LB_SETSEL, TRUE, (LPARAM)*plSel + 1);
        }
        break;

    default:
        goto Cleanup;
    }

Cleanup:
    delete [] pnItems;
}


//+-------------------------------------------------------------------------
//
//  Method:     OnbtnOkClick
//
//  Synopsis:   Handles OK button events
//
//  History:    12-Jul-95   t-AnandR    Created
//
//--------------------------------------------------------------------------
void
COrderDlg::OnbtnOkClick(void)
{
    HWND    hwndLB;

    hwndLB = GetDlgItem(_hwnd, IDR_TABORDERLSTBOX);

    // Loop through objs setting the order index according to position
    // in listbox
    for (LONG i = 0; i < _c; ++i)
    {
        void *  pv;

        // Put the itemdata of the i'th elmt in the i'th position of
        // _ppObjs
        pv = (void *)SendMessage(hwndLB, LB_GETITEMDATA, (WPARAM)i, 0);
        if ((void *)LB_ERR == pv)
        {
            TraceTag((tagError, "TabOrderpage::UpdateObjects -- "
                                      "GetItemData failed"));
            continue;
        }

        *(_ppObjs + i) = pv;
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     OnbtnCancelClick
//
//  Synopsis:   Handles Cancel button events
//
//  History:    19-Jul-95   t-AnandR    Created
//
//--------------------------------------------------------------------------
void
COrderDlg::OnbtnCancelClick(void)
{
    // Do nothing.
}




//+-------------------------------------------------------------------------
//
//  Function:   DoTabOrderDlg
//
//  Synopsis:   Puts up the tab order dialog
//
//--------------------------------------------------------------------------

HRESULT
DoTabOrderDlg(CBase * pBase, CDoc * pDoc, HWND hwndOwner)
{
    HRESULT             hr = S_FALSE;
    // BUGBUG rgardner removed the old controls collection - need to re-imp
/*
    IControls *         pCtrls = NULL;
    IEnumControl *      pEnumCtrl = NULL;
    IControl *          pCtrl = NULL;
    long                c = 0;
    long                i;
    IControlElement **         apControl = NULL;
    TCHAR **            astr = NULL;

    // Prepackage the elements of the form and send to the order dialog.
    hr = pDoc->GetControls(&pCtrls);
    if (hr)
        goto Cleanup;

    hr = pCtrls->Enum(&pEnumCtrl);
    if (hr)
    {
        TraceTag((tagError, "TabOrderPage::SetObjects -- EnumControls failed (%lx)", hr));
        goto Cleanup;
    }

    // Loop through controls seeing which control to put in listbox
    // Only show controls which have a tab index
    while (S_OK == (hr = pEnumCtrl->Next(1, &pCtrl, NULL)))
    {
        //short   sTabIndex;

        // BUGBUG rgardner removed IControl
        // hr = pCtrl->GetTabIndex(&sTabIndex);
        hr = S_FALSE;
        if (!hr)
        {
            c++;
        }

        ClearInterface(&pCtrl);
    }

    hr = pEnumCtrl->Reset();
    if (hr)
    {
        TraceTag((tagError, "TabOrderPage::SetObjects -- IEnumX::Reset failed (%lx)", hr));
        goto Cleanup;
    }

    // Now that we know how many objects there are (c), allocate
    // memory for all of them.
    apControl = new IControl *[c];
    astr = new TCHAR *[c];
    if ((!apControl || !astr) && c)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    while ((hr = pEnumCtrl->Next(1, &pCtrl, NULL)) == S_OK)
    {
        BSTR    bstr;
        short   sTabIndex;

        // BUGBUG rgardner removed IControl
        //hr = pCtrl->GetTabIndex(&sTabIndex);
        hr = S_FALSE;
        if (hr)
        {
            FormsRelease(pCtrl);
            continue;
        }
        // BUGBUG rgardner removed IControl
        //hr = pCtrl->GetName(&bstr);
        hr = S_FALSE;

        Assert(SUCCEEDED(hr));

        // Now insert this string into the correct position in the list
        Assert(sTabIndex < c);
        apControl[sTabIndex] = pCtrl;
        astr[sTabIndex] = bstr;
    }
    pCtrl = NULL;

    TCHAR   achCap[FORMS_BUFLEN];



    if (!LoadString(GetResourceHInst(), IDS_TABORDER, achCap, FORMS_BUFLEN))
    {
        hr = GetLastWin32Error();
        goto Cleanup;
    }

    hr = THR(DoOrderDialog(
        pBase,
        c,
        (void **)apControl,
        astr,
        achCap,
        hwndOwner));
    if (hr)
        goto Cleanup;

    // Now finally set the tab order
    for (i = 0; i < c; i++)
    {
        Assert(apControl[i]);
        // BUGBUG rgardner removed IControl
        //hr = THR(apControl[i]->SetTabIndex((short)i));
        if (hr)
        {
            TraceTag((tagError, "TabOrderpage::UpdateObjects --"
                                      "SetTabIndex failed (%lx)", hr));
        }
    }

Cleanup:
    for (i = 0; i < c; i++)
    {
        ReleaseInterface(apControl[i]);
        FormsFreeString(astr[i]);
    }

    delete [] apControl;
    delete [] astr;

    ReleaseInterface(pCtrls);
    ReleaseInterface(pEnumCtrl);
    ReleaseInterface(pCtrl);
    */
    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Function:   OrderDlgProc
//
//  Synopsis:   Dialog procedure for the tab order dialog
//
//  History:    12-Jul-1995     t-AnandR    Created
//
//--------------------------------------------------------------------------

BOOL CALLBACK
OrderDlgProc(
    HWND    hwndDlg,// handle of dialog box
    UINT    uMsg,   // message
    WPARAM  wParam, // first message parameter
    LPARAM  lParam  // second message parameter
   )
{
    COrderDlg *      pdlg;

    pdlg = (COrderDlg *)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        // Store a pointer to the CtrlSelDlg class in the window params
        // to be retrieved later.
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
        pdlg = (COrderDlg *)lParam;
        pdlg->SetWindowHandle(hwndDlg);
        pdlg->OnFormLoad();
        return FALSE;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDOK:
            // On click of the OK button
            pdlg->OnbtnOkClick();
#if !defined(NO_IME)
            if (! g_fUSSystem)
            {
                CancelUndeterminedIMEString(hwndDlg);
            }
#endif

            EndDialog(hwndDlg, S_OK);
            break;

        case IDCANCEL:
            // For close sysmenu event
            pdlg->OnbtnCancelClick();
#if !defined(NO_IME)
            if (! g_fUSSystem)
            {
                CancelUndeterminedIMEString(hwndDlg);
            }
#endif

            EndDialog(hwndDlg, S_FALSE);
            break;

        case IDR_BTNMOVEUP:
            // Move up event
            pdlg->OnbtnMoveClick(MOVE_UP);
            break;

        case IDR_BTNMOVEDOWN:
            // Move down event
            pdlg->OnbtnMoveClick(MOVE_DOWN);
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    // Only way to get down here is if OrderDlgProc handles the message
    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CompareLBItems
//
//  Synopsis:   Callback function to qsort for determining order for items
//
//  Arguments:  pvElem1 -- 1st element to compare
//              pvElem2 -- 2nd element to compare
//
//  History:    18-Jul-95   t-AnandR    Created
//
//----------------------------------------------------------------------------
int RTCCONV
CompareLBItems(const void *pvElem1, const void *pvElem2)
{
    long *  plElem1 = (long *)pvElem1;
    long *  plElem2 = (long *)pvElem2;

    if (*plElem1 < *plElem2)
        return -1;

    if (*plElem1 == *plElem2)
        return 0;

    return 1;
}


//+-------------------------------------------------------------------------
//
//  Function:   DoOrderDialog
//
//  Synopsis:   Show Generic Order dialog
//
//  Input:      c           Number of items in array
//              ppObjs      Array of pointers to objects
//              ppStrs      Array of strings to show in listbox, in the same
//                          order as the objects in ppObjs.
//              strCap      Caption to display for dialog title/label.
//              hwndOwner   Self-explanatory
//
//  Output      HRESULT, and ppObjs is ordered the way the list finally
//              looks before the user pressed ok or cancel
//
//--------------------------------------------------------------------------

HRESULT
DoOrderDialog(CBase * pBase,
              int     c,
              void  **ppObjs,
              TCHAR **ppStrs,
              TCHAR  *strCap,
              HWND    hwndOwner)
{
    COrderDlg * pdlg;
    HRESULT     hr = S_OK;

    pdlg = new COrderDlg(c, ppObjs, ppStrs, strCap);
    if (NULL == pdlg)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }


    hr = DialogBoxParam(
                GetResourceHInst(),
                MAKEINTRESOURCE(IDR_TABORDERDLG),
                hwndOwner,
                (DLGPROC)OrderDlgProc,
                (LPARAM)pdlg);

    if (-1 == hr)
    {
        hr = GetLastWin32Error();
        goto Cleanup;
    }

Cleanup:
    delete pdlg;
    RRETURN1(hr, S_FALSE);
}

