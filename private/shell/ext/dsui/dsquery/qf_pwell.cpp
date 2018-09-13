#include "pch.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Local functions / data
/----------------------------------------------------------------------------*/

#define CCLV_CHECKED        0x00002000
#define CCLV_UNCHECKED      0x00001000

#define DLU_EDGE            6
#define DLU_SEPERATOR       2
#define DLU_FIXEDELEMENT    80

#define CLID_OTHER 1       // other...
#define CLID_FIRST 2

static TCHAR c_szItems[]          = TEXT("Items");
static TCHAR c_szObjectClassN[]   = TEXT("ObjectClass%d");
static TCHAR c_szProperty[]       = TEXT("Property%d");
static TCHAR c_szCondition[]      = TEXT("Condition%d");
static TCHAR c_szValue[]          = TEXT("Value%d");

static COLUMNINFO columns[] = 
{
    0, 0, IDS_CN,          0, c_szName,          
    0, 0, IDS_OBJECTCLASS, DSCOLUMNPROP_OBJECTCLASS, NULL,
    0, DEFAULT_WIDTH_DESCRIPTION, IDS_DESCRIPTION, 0, c_szDescription,
};

static struct
{
    DWORD dwPropertyType;
    BOOL fNoValue;
    INT iFilter;
    INT idsFilter;
}
conditions[] =
{
    PROPERTY_ISUNKNOWN, 0, FILTER_IS,           IDS_IS,
    PROPERTY_ISUNKNOWN, 0, FILTER_ISNOT,        IDS_ISNOT,
    PROPERTY_ISUNKNOWN, 1, FILTER_DEFINED,      IDS_DEFINED,
    PROPERTY_ISUNKNOWN, 1, FILTER_UNDEFINED,    IDS_NOTDEFINED, 

    PROPERTY_ISSTRING,  0, FILTER_STARTSWITH,   IDS_STARTSWITH, 
    PROPERTY_ISSTRING,  0, FILTER_ENDSWITH,     IDS_ENDSWITH,
    PROPERTY_ISSTRING,  0, FILTER_IS,           IDS_IS, 
    PROPERTY_ISSTRING,  0, FILTER_ISNOT,        IDS_ISNOT,
    PROPERTY_ISSTRING,  1, FILTER_DEFINED,      IDS_DEFINED,
    PROPERTY_ISSTRING,  1, FILTER_UNDEFINED,    IDS_NOTDEFINED, 

    PROPERTY_ISNUMBER,  0, FILTER_LESSEQUAL,    IDS_LESSTHAN,   
    PROPERTY_ISNUMBER,  0, FILTER_GREATEREQUAL, IDS_GREATERTHAN,
    PROPERTY_ISNUMBER,  0, FILTER_IS,           IDS_IS,         
    PROPERTY_ISNUMBER,  0, FILTER_ISNOT,        IDS_ISNOT,      
    PROPERTY_ISNUMBER,  1, FILTER_DEFINED,      IDS_DEFINED,
    PROPERTY_ISNUMBER,  1, FILTER_UNDEFINED,    IDS_NOTDEFINED, 

    PROPERTY_ISBOOL,    1, FILTER_ISTRUE,       IDS_ISTRUE,
    PROPERTY_ISBOOL,    1, FILTER_ISFALSE,      IDS_ISFALSE,
    PROPERTY_ISBOOL,    1, FILTER_DEFINED,      IDS_DEFINED,
    PROPERTY_ISBOOL,    1, FILTER_UNDEFINED,    IDS_NOTDEFINED, 
};

static struct
{
    INT cx;
    INT fmt;
}
view_columns[] =
{
    128, LVCFMT_LEFT,
    128, LVCFMT_LEFT,
    128, LVCFMT_LEFT,
};

// Class list used for building the property chooser menu

typedef struct
{
    LPWSTR pName;
    LPTSTR pDisplayName;
    INT cReferences;
} CLASSENTRY, * LPCLASSENTRY;

// State maintained by the property well view

typedef struct
{
    LPCLASSENTRY pClassEntry;       // class entry reference
    LPWSTR pProperty;
    LPWSTR pValue;                  // can be NULL
    INT iCondition;
} PROPERTYWELLITEM, * LPPROPERTYWELLITEM;

typedef struct
{
    LPCQPAGE pQueryPage;
    HDPA    hdpaItems;
    HDSA    hdsaClasses;

    INT     cxEdge;
    INT     cyEdge;

    HWND    hwnd;
    HWND    hwndProperty;
    HWND    hwndPropertyLabel;
    HWND    hwndCondition;
    HWND    hwndConditionLabel;
    HWND    hwndValue;
    HWND    hwndValueLabel;
    HWND    hwndAdd;
    HWND    hwndRemove;
    HWND    hwndList;

    LPCLASSENTRY pClassEntry;
    LPWSTR  pPropertyName;

    IDsDisplaySpecifier *pdds;
  
} PROPERTYWELL, * LPPROPERTYWELL;

BOOL PropertyWell_OnInitDialog(HWND hwnd, LPCQPAGE pQueryPage);
BOOL PropertyWell_OnNCDestroy(LPPROPERTYWELL ppw);
BOOL PropertyWell_OnSize(LPPROPERTYWELL ppw, INT cxWindow, INT cyWindow);
VOID PropertyWell_OnDrawItem(LPPROPERTYWELL ppw, LPDRAWITEMSTRUCT pDrawItem);
VOID PropertyWell_OnChooseProperty(LPPROPERTYWELL ppw);

HRESULT PropertyWell_GetClassList(LPPROPERTYWELL ppw);
VOID PropertyWell_FreeClassList(LPPROPERTYWELL ppw);
LPCLASSENTRY PropertyWell_FindClassEntry(LPPROPERTYWELL ppw, LPWSTR pObjectClass);

HRESULT PropertyWell_AddItem(LPPROPERTYWELL ppw, LPCLASSENTRY pClassEntry, LPWSTR pProperty, INT iCondition, LPWSTR pValue);
VOID PropertyWell_RemoveItem(LPPROPERTYWELL ppw, INT iItem, BOOL fDeleteItem);
VOID PropertyWell_EditItem(LPPROPERTYWELL ppw, INT iItem);
HRESULT PropertyWell_EditProperty(LPPROPERTYWELL ppw, LPCLASSENTRY pClassEntry, LPWSTR pPropertyName, INT iCondition);
VOID PropertyWell_ClearControls(LPPROPERTYWELL ppw);
BOOL PropertyWell_EnableControls(LPPROPERTYWELL ppw, BOOL fEnable);
VOID PropertyWell_SetColumnWidths(LPPROPERTYWELL ppw);
HRESULT PropertyWell_GetQuery(LPPROPERTYWELL ppw, LPWSTR* ppQuery);
HRESULT PropertyWell_Persist(LPPROPERTYWELL ppw, IPersistQuery* pPersistQuery, BOOL fRead);

#define CONDITION_FROM_COMBO(hwnd)    \
            (int)ComboBox_GetItemData(hwnd, ComboBox_GetCurSel(hwnd))

//
// Control help meppings
// 

static DWORD const aFormHelpIDs[] =
{
    IDC_PROPERTYLABEL,  IDH_FIELD,
    IDC_PROPERTY,       IDH_FIELD,
    IDC_CONDITIONLABEL, IDH_CONDITION,
    IDC_CONDITION,      IDH_CONDITION,
    IDC_VALUELABEL,     IDH_VALUE,
    IDC_VALUE,          IDH_VALUE,
    IDC_ADD,            IDH_ADD,
    IDC_REMOVE,         IDH_REMOVE,
    IDC_CONDITIONLIST,  IDH_CRITERIA,
    0, 0,
};


/*-----------------------------------------------------------------------------
/ PageProc_PropertyWell
/ ---------------------
/   PageProc for handling the messages for this object.
/
/ In:
/   pPage -> instance data for this form
/   hwnd = window handle for the form dialog
/   uMsg, wParam, lParam = message parameters
/
/ Out:
/   HRESULT (E_NOTIMPL) if not handled
/----------------------------------------------------------------------------*/
HRESULT CALLBACK PageProc_PropertyWell(LPCQPAGE pPage, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;
    LPPROPERTYWELL ppw = (LPPROPERTYWELL)GetWindowLongPtr(hwnd, DWLP_USER);
    LPWSTR pQuery = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_FORMS, "PageProc_PropertyWell");

    // Only handle page messages if we have a property well object, 
    // which is created when the PropWell dlg is init'd.
    if (ppw)
    {
        switch ( uMsg )
        {
            case CQPM_INITIALIZE:
            case CQPM_RELEASE:
                break;

            case CQPM_GETPARAMETERS:
            {
                LPDSQUERYPARAMS* ppDsQueryParams = (LPDSQUERYPARAMS*)lParam;

                // if the add button is enabled then we must prompt the user and see if they 
                // want to add the current criteria to the query

                if ( IsWindowEnabled(ppw->hwndAdd) )
                {
                    TCHAR szProperty[MAX_PATH];
                    TCHAR szValue[MAX_PATH];
                    INT id;
            
                    LoadString(GLOBAL_HINSTANCE, IDS_WINDOWTITLE, szProperty, ARRAYSIZE(szProperty));
                    LoadString(GLOBAL_HINSTANCE, IDS_ENTERCRITERIA, szValue, ARRAYSIZE(szValue));
                
                    id =  MessageBox(hwnd, szValue, szProperty, MB_YESNOCANCEL|MB_ICONWARNING);
                    Trace(TEXT("MessageBox returned %08x"), id);

                    if ( id == IDCANCEL )
                    {
                        ExitGracefully(hr, S_FALSE, "*** Aborting query ****");
                    }
                    else if ( id == IDYES )
                    {
                        GetWindowText(ppw->hwndValue, szValue, ARRAYSIZE(szValue));
                        id = CONDITION_FROM_COMBO(ppw->hwndCondition);

                        hr = PropertyWell_AddItem(ppw, ppw->pClassEntry, ppw->pPropertyName, id, T2W(szValue));
                        FailGracefully(hr, "Failed to add the item to the current query");
                    }
                }

                // zap anything in these fields and ensure the controls reflect the new state

                PropertyWell_ClearControls(ppw);

                if ( SUCCEEDED(PropertyWell_GetQuery(ppw, &pQuery)) && pQuery )
                {
                    if ( !*ppDsQueryParams )
                        hr = QueryParamsAlloc(ppDsQueryParams, pQuery, GLOBAL_HINSTANCE, ARRAYSIZE(columns), columns);
                    else
                        hr = QueryParamsAddQueryString(ppDsQueryParams, pQuery);

                    LocalFreeStringW(&pQuery);
                }

                break;
            }

            case CQPM_ENABLE:
                PropertyWell_EnableControls(ppw, (BOOL)wParam);
                break;

            case CQPM_CLEARFORM:
                ListView_DeleteAllItems(ppw->hwndList);
                PropertyWell_ClearControls(ppw);
                break;

            case CQPM_PERSIST:
                hr = PropertyWell_Persist(ppw, (IPersistQuery*)lParam, (BOOL)wParam);
                break;

            case CQPM_SETDEFAULTPARAMETERS:
            {
                LPOPENQUERYWINDOW poqw = (LPOPENQUERYWINDOW)lParam;

                //
                // if we recieve this message we should ensure that we have the IDsDsiplaySpecifier
                // object and then we can set the credential information.
                //

                if ( ppw && poqw->pHandlerParameters )
                {
                    LPDSQUERYINITPARAMS pdqip = (LPDSQUERYINITPARAMS)poqw->pHandlerParameters;
                    if ( pdqip->dwFlags & DSQPF_HASCREDENTIALS )                 
                    {
                        Trace(TEXT("pUserName : %s"), pdqip->pUserName ? W2T(pdqip->pUserName):TEXT("<not specified>"));
                        Trace(TEXT("pServer : %s"), pdqip->pServer ? W2T(pdqip->pServer):TEXT("<not specified>"));

                        hr = ppw->pdds->SetServer(pdqip->pServer, pdqip->pUserName, pdqip->pPassword, DSSSF_DSAVAILABLE);
                        FailGracefully(hr, "Failed to set the server information");
                    }
                }

                break;
            }

            case DSQPM_GETCLASSLIST:
            {
                DWORD cbStruct, offset;
                LPDSQUERYCLASSLIST pDsQueryClassList = NULL;
                INT i;

                if ( wParam & DSQPM_GCL_FORPROPERTYWELL )
                {
                    TraceMsg("Property well calling property well, ignore!");
                    break;
                }

                if ( !lParam )
                    ExitGracefully(hr, E_FAIL, "lParam == NULL, not supported");

                // Get the list of classes that the user can/has selected properties from,
                // having done this we can then can then generate a suitable query.

                hr = PropertyWell_GetClassList(ppw);
                FailGracefully(hr, "Failed to get the class list");

                cbStruct = SIZEOF(DSQUERYCLASSLIST) + (DSA_GetItemCount(ppw->hdsaClasses)*SIZEOF(DWORD));
                offset = cbStruct;

                for ( i = 0 ; i < DSA_GetItemCount(ppw->hdsaClasses) ; i++ )
                {
                    LPCLASSENTRY pCE = (LPCLASSENTRY)DSA_GetItemPtr(ppw->hdsaClasses, i);
                    TraceAssert(pCE);

                    cbStruct += StringByteSizeW(pCE->pName);
                }

                // Allocate the blob we need to pass out and fill it.

                Trace(TEXT("Allocating class structure %d"), cbStruct);

                pDsQueryClassList = (LPDSQUERYCLASSLIST)CoTaskMemAlloc(cbStruct);
                TraceAssert(pDsQueryClassList);

                if ( !pDsQueryClassList )
                    ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate class list structure");

                Trace(TEXT("pDsQueryClassList %08x"), pDsQueryClassList);                

                pDsQueryClassList->cbStruct = cbStruct;
                pDsQueryClassList->cClasses = DSA_GetItemCount(ppw->hdsaClasses);

                for ( i = 0 ; i < DSA_GetItemCount(ppw->hdsaClasses) ; i++ )
                {
                    LPCLASSENTRY pCE = (LPCLASSENTRY)DSA_GetItemPtr(ppw->hdsaClasses, i);
                    TraceAssert(pCE);

                    Trace(TEXT("Adding class: %s"), W2T(pCE->pName));

                    pDsQueryClassList->offsetClass[i] = offset;
                    StringByteCopyW(pDsQueryClassList, offset, pCE->pName);
                    offset += StringByteSizeW(pCE->pName);
                }

                TraceAssert(pDsQueryClassList);
                *((LPDSQUERYCLASSLIST*)lParam) = pDsQueryClassList;

                break;
            }

            case CQPM_HELP:
            {
                LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;
                WinHelp((HWND)pHelpInfo->hItemHandle,
                        DSQUERY_HELPFILE,
                        HELP_WM_HELP,
                        (DWORD_PTR)aFormHelpIDs);
                break;
            }

            case DSQPM_HELPTOPICS:
            {
                HWND hwndFrame = (HWND)lParam;
                HtmlHelp(hwndFrame, TEXT("omc.chm"), HH_HELP_FINDER, 0);
                break;
            }

            default:
                hr = E_NOTIMPL;
                break;
        }
    }

exit_gracefully:

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ Dialog helper functions
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ DlgProc_PropertyWell
/ --------------------
/   Standard dialog proc for the form, handle any special buttons and other
/   such nastyness we must here.
/
/ In:
/   hwnd, uMsg, wParam, lParam = standard parameters
/
/ Out:
/   INT_PTR
/----------------------------------------------------------------------------*/
INT_PTR CALLBACK DlgProc_PropertyWell(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR fResult = FALSE;
    LPPROPERTYWELL ppw = NULL;
    USES_CONVERSION;

    if ( uMsg == WM_INITDIALOG )
    {
        fResult = PropertyWell_OnInitDialog(hwnd, (LPCQPAGE)lParam);
    }
    else
    {
        ppw = (LPPROPERTYWELL)GetWindowLongPtr(hwnd, DWLP_USER);

        switch ( uMsg )
        {
            case WM_NCDESTROY:
                PropertyWell_OnNCDestroy(ppw);
                break;

            case WM_COMMAND:
            {
                switch ( LOWORD(wParam) )
                {
                    case IDC_PROPERTYLABEL:
                    {
                        if ( HIWORD(wParam) == BN_CLICKED )
                            PropertyWell_OnChooseProperty(ppw);

                        break;
                    }
                    
                    case IDC_PROPERTY:
                    case IDC_CONDITION:
                    case IDC_VALUE:
                    {
                        if ( (HIWORD(wParam) == EN_CHANGE) || (HIWORD(wParam) == CBN_SELCHANGE) )
                            PropertyWell_EnableControls(ppw, TRUE);

                        break;
                    }

                    case IDC_ADD:
                    {
                        TCHAR szProperty[MAX_PATH] = { TEXT('\0') };
                        TCHAR szValue[MAX_PATH] = { TEXT('\0') };
                        INT iCondition;

                        iCondition = CONDITION_FROM_COMBO(ppw->hwndCondition);

                        if ( IsWindowEnabled(ppw->hwndValue) )
                            GetWindowText(ppw->hwndValue, szValue, ARRAYSIZE(szValue));

                        PropertyWell_AddItem(ppw, ppw->pClassEntry, ppw->pPropertyName, iCondition, T2W(szValue));

                        break;
                    }
                    
                    case IDC_REMOVE:
                    {
                        INT item = ListView_GetNextItem(ppw->hwndList, -1, LVNI_ALL|LVNI_SELECTED);
                        PropertyWell_RemoveItem(ppw, item, TRUE);
                    }                    
                }

                break;
            }

            case WM_DRAWITEM:
                PropertyWell_OnDrawItem(ppw, (LPDRAWITEMSTRUCT)lParam);
                break;

            case WM_NOTIFY:
            {
                LPNMHDR pNotify = (LPNMHDR)lParam;

                switch ( pNotify->code )
                {
                    case LVN_DELETEITEM:
                    {
                        NM_LISTVIEW* pNotify = (NM_LISTVIEW*)lParam;
                        PropertyWell_RemoveItem(ppw, pNotify->iItem, FALSE);
                        break;
                    }

                    case LVN_ITEMCHANGED:
                    {
                        PropertyWell_EnableControls(ppw, TRUE);
                        break;
                    }

                    case NM_DBLCLK:
                    {
                        INT item = ListView_GetNextItem(ppw->hwndList, -1, LVNI_ALL|LVNI_SELECTED);
                        PropertyWell_EditItem(ppw, item);
                        break;
                    }

                    case LVN_GETEMPTYTEXT:
                    {
                        NMLVDISPINFO* pNotify = (NMLVDISPINFO*)lParam;
                        if ( pNotify->item.mask & LVIF_TEXT )
                        {
                            LoadString(GLOBAL_HINSTANCE, IDS_CRITERIAEMPTY, pNotify->item.pszText, pNotify->item.cchTextMax);
                            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
                            fResult = TRUE;
                        }
                        break;
                    }
                }

                break;
            }

            case WM_SIZE:
                return PropertyWell_OnSize(ppw, LOWORD(lParam), HIWORD(lParam));
                
            case WM_CONTEXTMENU:
                WinHelp((HWND)wParam, DSQUERY_HELPFILE, HELP_CONTEXTMENU, (DWORD_PTR)aFormHelpIDs);
                fResult = TRUE;
                break;
        }
    }
 
    return fResult;    
}


/*-----------------------------------------------------------------------------
/ PropertyWell_OnInitDlg
/ ----------------------
/   Initialize the dialog, constructing the property DPA so that we can
/   build the store the query.
/
/ In:
/   hwnd = window handle being initialized
/   pDsQuery -> CDsQuery object to associate with
/
/ Out:
/   BOOL
/----------------------------------------------------------------------------*/
BOOL PropertyWell_OnInitDialog(HWND hwnd, LPCQPAGE pQueryPage)
{
    HRESULT hr;
    LPPROPERTYWELL ppw;
    TCHAR szBuffer[MAX_PATH];
    LV_COLUMN lvc;
    INT i;

    TraceEnter(TRACE_PWELL, "PropertyWell_OnInitDialog");

    // Allocate the state structure and fill it

    ppw = (LPPROPERTYWELL)LocalAlloc(LPTR, SIZEOF(PROPERTYWELL));

    if ( !ppw )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to alloc PROPERTYWELL struct");

    Trace(TEXT("ppw = %08x"), ppw);
    SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)ppw);
    
    // now initialize the structure

    ppw->pQueryPage = pQueryPage;
    //ppw->hdpaItems = NULL;
    //ppw->hdsaClasses = NULL;    

    ppw->cxEdge = GetSystemMetrics(SM_CXEDGE);
    ppw->cyEdge = GetSystemMetrics(SM_CYEDGE);

    ppw->hwnd = hwnd;
    ppw->hwndProperty = GetDlgItem(hwnd, IDC_PROPERTY);
    ppw->hwndPropertyLabel = GetDlgItem(hwnd, IDC_PROPERTYLABEL);
    ppw->hwndCondition = GetDlgItem(hwnd, IDC_CONDITION);
    ppw->hwndConditionLabel = GetDlgItem(hwnd, IDC_CONDITIONLABEL);
    ppw->hwndValue = GetDlgItem(hwnd, IDC_VALUE);
    ppw->hwndValueLabel = GetDlgItem(hwnd, IDC_VALUELABEL);
    ppw->hwndAdd = GetDlgItem(hwnd, IDC_ADD);
    ppw->hwndRemove = GetDlgItem(hwnd, IDC_REMOVE);
    ppw->hwndList = GetDlgItem(hwnd, IDC_CONDITIONLIST);

    ppw->hdpaItems = DPA_Create(16);

    if ( !ppw->hdpaItems )
        ExitGracefully(hr, E_FAIL, "Failed to create DPA");

    // ppw->pClassItem = NULL;
    // ppw->pPropertyName = NULL;

    hr = CoCreateInstance(CLSID_DsDisplaySpecifier, NULL, CLSCTX_INPROC_SERVER, IID_IDsDisplaySpecifier, (void **)&ppw->pdds);
    FailGracefully(hr, "Failed to CoCreate the IDsDisplaySpecifier object");
    
    ListView_SetExtendedListViewStyle(ppw->hwndList, LVS_EX_FULLROWSELECT|LVS_EX_LABELTIP);

    // Add the conditions to the condition picker, then add the columns to the
    // condition list.

    for ( i = 0 ; i < ARRAYSIZE(view_columns) ; i++ )
    {
        lvc.mask = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT;
        lvc.fmt = view_columns[i].fmt;
        lvc.cx = view_columns[i].cx;
        lvc.pszText = TEXT("Bla");
        ListView_InsertColumn(ppw->hwndList, i, &lvc);
    }

    Edit_LimitText(ppw->hwndValue, MAX_PATH);

    PropertyWell_EnableControls(ppw, TRUE);

exit_gracefully:

    TraceLeaveValue(TRUE);
}


/*-----------------------------------------------------------------------------
/ PropertyWell_OnNCDestroy
/ ------------------------
/   The dialog is being nuked, therefore remove our reference to the CDsQuery
/   and free any allocations we have with this window.
/
/ In:
/   ppw -> window defn to use
/
/ Out:
/   BOOL
/----------------------------------------------------------------------------*/
BOOL PropertyWell_OnNCDestroy(LPPROPERTYWELL ppw)
{
    BOOL fResult = TRUE;

    TraceEnter(TRACE_PWELL, "PropertyWell_OnNCDestroy");

    if ( ppw )
    {
        if ( ppw->hdpaItems )
        {
            TraceAssert(0 == DPA_GetPtrCount(ppw->hdpaItems));
            DPA_Destroy(ppw->hdpaItems);
        }

        PropertyWell_FreeClassList(ppw);
        LocalFreeStringW(&ppw->pPropertyName);
        DoRelease(ppw->pdds);

        SetWindowLongPtr(ppw->hwnd, DWLP_USER, (LONG_PTR)NULL);
        LocalFree((HLOCAL)ppw);        
    }

    TraceLeaveValue(fResult);
}


/*-----------------------------------------------------------------------------
/ PropertyWell_OnSize
/ -------------------
/   The property well dialog is being sized, therefore lets move the 
/   controls around within it to reflect the new size.
/
/ In:
/   ppw -> property well to size
/   cx, cy = new size
/
/ Out:
/   BOOL
/----------------------------------------------------------------------------*/
BOOL PropertyWell_OnSize(LPPROPERTYWELL ppw, INT cxWindow, INT cyWindow)
{
    RECT rect;
    SIZE size;
    INT x, cx;
    INT xProperty, xCondition, xValue;
    INT iSeperator, iEdge, iElement, iFixedElement;

    TraceEnter(TRACE_PWELL, "PropertyWell_OnSize");
    Trace(TEXT("New size cxWindow %d, cyWindow %d"), cxWindow, cyWindow);

    iSeperator = (DLU_SEPERATOR * LOWORD(GetDialogBaseUnits())) / 4;
    iEdge = (DLU_EDGE * LOWORD(GetDialogBaseUnits())) / 4;
    iFixedElement = (DLU_FIXEDELEMENT * LOWORD(GetDialogBaseUnits())) / 4;
    
    x = cxWindow - (iEdge*2) - (iSeperator*2);
    
    iElement = x / 3;
    iFixedElement = min(iElement, iFixedElement);
    iElement = x - (iFixedElement*2);

    // Move the controls around accordingly

    xProperty = iEdge;
    GetRealWindowInfo(ppw->hwndProperty, &rect, &size);
    SetWindowPos(ppw->hwndProperty, NULL, xProperty, rect.top, iFixedElement, size.cy, SWP_NOZORDER);
    GetRealWindowInfo(ppw->hwndPropertyLabel, &rect, &size);
    SetWindowPos(ppw->hwndPropertyLabel, NULL, xProperty, rect.top, 0, 0, SWP_NOZORDER|SWP_NOSIZE);

    xCondition = iEdge + iFixedElement + iSeperator;
    GetRealWindowInfo(ppw->hwndCondition, &rect, &size);
    SetWindowPos(ppw->hwndCondition, NULL, xCondition, rect.top, iFixedElement, size.cy, SWP_NOZORDER);
    GetRealWindowInfo(ppw->hwndConditionLabel, &rect, &size);
    SetWindowPos(ppw->hwndConditionLabel, NULL, xCondition, rect.top, 0, 0, SWP_NOZORDER|SWP_NOSIZE);

    xValue = cxWindow - iEdge - iElement;
    GetRealWindowInfo(ppw->hwndValue, &rect, &size);
    SetWindowPos(ppw->hwndValue, NULL, xValue, rect.top, iElement, size.cy, SWP_NOZORDER);
    GetRealWindowInfo(ppw->hwndValueLabel, &rect, &size);
    SetWindowPos(ppw->hwndValueLabel, NULL, xValue, rect.top, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
    
    // Move the add / remove buttons

    GetRealWindowInfo(ppw->hwndRemove, &rect, &size);
    x  = cxWindow - iEdge - size.cx;
    SetWindowPos(ppw->hwndRemove, NULL, x, rect.top, 0, 0, SWP_NOZORDER|SWP_NOSIZE);

    GetRealWindowInfo(ppw->hwndAdd, &rect, &size);
    x -= size.cx + iSeperator;
    SetWindowPos(ppw->hwndAdd, NULL, x, rect.top, 0, 0, SWP_NOZORDER|SWP_NOSIZE);

    // Move the list view control + size accordingly
        
    GetRealWindowInfo(ppw->hwndList, &rect, &size);
    SetWindowPos(ppw->hwndList, NULL, iEdge, rect.top, cxWindow - (iEdge*2), size.cy, SWP_NOZORDER);

    PropertyWell_SetColumnWidths(ppw);

    TraceLeaveValue(FALSE);
}


/*-----------------------------------------------------------------------------
/ PropertyWell_OnDrawItem
/ -----------------------
/   The property button is owner drawn, therefore lets handle rendering that
/   we assume that the base implementation (eg. the button control) is
/   handling storing the text, font and other interesting information we
/   will just render the face as required.
/
/ In:
/   ppw -> property well to size
/   pDrawItem -> DRAWITEMSTRUCT used for rendering
/
/ Out:
/   void
/----------------------------------------------------------------------------*/
VOID PropertyWell_OnDrawItem(LPPROPERTYWELL ppw, LPDRAWITEMSTRUCT pDrawItem)
{   
    SIZE thin = { ppw->cxEdge / 2, ppw->cyEdge / 2 };
    RECT rc = pDrawItem->rcItem;
    HDC hdc = pDrawItem->hDC;
    BOOL fDisabled = pDrawItem->itemState & ODS_DISABLED; 
    BOOL fSelected = pDrawItem->itemState & ODS_SELECTED;
    BOOL fFocus = (pDrawItem->itemState & ODS_FOCUS) 
#if (_WIN32_WINNT >= 0x0500)
                    && !(pDrawItem->itemState & ODS_NOFOCUSRECT)
#endif
                        && !(pDrawItem->itemState & ODS_DISABLED);
    TCHAR szBuffer[64];
    HBRUSH hbr;
    INT i, x, y;
    SIZE sz;
    UINT fuFlags = DST_PREFIXTEXT;

    TraceEnter(TRACE_PWELL, "PropertyWell_OnDrawItem");

    if ( pDrawItem->CtlID != IDC_PROPERTYLABEL )
        goto exit_gracefully;

    // render the button edges (assumes that we have an NT4 look)

    thin.cx = max(thin.cx, 1);
    thin.cy = max(thin.cy, 1);

    if ( fSelected )
    {
        DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT|BF_ADJUST);
        OffsetRect(&rc, 1, 1);
    }
    else
    {
        DrawEdge(hdc, &rc, EDGE_RAISED, BF_RECT|BF_ADJUST);
    }

    FillRect(hdc, &rc, GetSysColorBrush(COLOR_3DFACE));

    // put the focus rect in if we are focused...

    if ( fFocus )
    {
        InflateRect(&rc, -thin.cx, -thin.cy);
        DrawFocusRect(hdc, &rc);
        InflateRect(&rc, thin.cx, thin.cy);
    }

    InflateRect(&rc, 1-thin.cx, -ppw->cyEdge);    
    rc.left += ppw->cxEdge*2;

    // paint the arrow to the right of the control

    x = rc.right - ppw->cxEdge - 13;
    y = rc.top + ((rc.bottom - rc.top)/2) - 2;

    if ( fDisabled )
    {
        hbr = (HBRUSH)GetSysColorBrush(COLOR_3DHILIGHT);
        hbr = (HBRUSH)SelectObject(hdc, hbr);

        x++;
        y++;
        PatBlt(hdc, x+1, y,   7, 1, PATCOPY);
        PatBlt(hdc, x+2, y+1, 5, 1, PATCOPY);
        PatBlt(hdc, x+3, y+2, 3, 1, PATCOPY);
        PatBlt(hdc, x+4, y+3, 1, 1, PATCOPY);

        SelectObject(hdc, hbr);
        x--;
        y--;
    }

    hbr = (HBRUSH)GetSysColorBrush(fDisabled ? COLOR_3DSHADOW : COLOR_BTNTEXT);
    hbr = (HBRUSH)SelectObject(hdc, hbr);

    PatBlt(hdc, x,   y+1, 7, 1, PATCOPY);
    PatBlt(hdc, x+1, y+2, 5, 1, PATCOPY);
    PatBlt(hdc, x+2, y+3, 3, 1, PATCOPY);
    PatBlt(hdc, x+3, y+4, 1, 1, PATCOPY);

    SelectObject(hdc, hbr);
    rc.right = x;

    // render the label in the remaining area (clipped accordingly)

    i = GetWindowText(ppw->hwndPropertyLabel, szBuffer, ARRAYSIZE(szBuffer));
    GetTextExtentPoint(hdc, szBuffer, i, &sz);

    x = rc.left+(((rc.right-rc.left)-sz.cx)/2);

    if ( fDisabled )
        fuFlags |= DSS_DISABLED;

#if (_WIN32_WINNT >= 0x0500)
    if ( pDrawItem->itemState & ODS_NOACCEL )
        fuFlags |= DSS_HIDEPREFIX;
#endif
        
    DrawState(hdc, NULL, NULL,  
                (LPARAM)szBuffer, (WPARAM)0, 
                    x, rc.top, sz.cx, sz.cy, 
                        fuFlags);
exit_gracefully:

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ PropertyWell_OnChooseProperty
/ -----------------------------
/   Display the class / property list and build the menu from it, this calls on
/   several helper functions.
/
/ In:
/   ppw -> property well to size
/
/ Out:
/   void
/----------------------------------------------------------------------------*/

//
// call the property enumerator and populate the our DPA
//

typedef struct
{
    UINT wID;
    HDPA hdpaAttributes;
    INT iClass;
    HMENU hMenu;
} PROPENUMSTRUCT, * LPPROPENUMSTRUCT;

HRESULT CALLBACK _FillMenuCB(LPARAM lParam, LPCWSTR pAttributeName, LPCWSTR pDisplayName, DWORD dwFlags)
{
    HRESULT hres = S_OK;
    PROPENUMSTRUCT *ppes = (PROPENUMSTRUCT *)lParam;
    MENUITEMINFO mii = { 0 };
    UINT_PTR iProperty;
    USES_CONVERSION;

    TraceEnter(TRACE_PWELL, "_FillMenuCB");

    if ( !(dwFlags & DSECAF_NOTLISTED) )
    {    
        hres = StringDPA_AppendStringW(ppes->hdpaAttributes, pAttributeName, &iProperty);
        FailGracefully(hres, "Failed to add the attribute to the DPA");

        mii.cbSize = SIZEOF(mii);
        mii.fMask = MIIM_TYPE|MIIM_ID|MIIM_DATA;
        mii.dwItemData = MAKELPARAM(ppes->iClass, iProperty);
        mii.fType = MFT_STRING;
        mii.wID = ppes->wID++;
        mii.dwTypeData = (LPTSTR)W2CT(pDisplayName);
        mii.cch = lstrlenW(pDisplayName);
   
        if ( !InsertMenuItem(ppes->hMenu, 0x7fff, TRUE, &mii) )
            ExitGracefully(hres, E_FAIL, "Failed to add the item to the menu");
    }
    else
    {
        TraceMsg("Property marked as hidden, so not appending to the DPA");
    }
    
    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}

VOID PropertyWell_OnChooseProperty(LPPROPERTYWELL ppw)
{
    HRESULT hr;
    HMENU hMenuToTrack, hMenu = NULL;
    PROPENUMSTRUCT pes = { 0 };
    RECT rcItem;
    LPCLASSENTRY pCE;
    LPWSTR pszAttribute;
    UINT uID;
    INT iItem, iClass;
    MENUITEMINFO mii = { 0 };
    DECLAREWAITCURSOR;
    USES_CONVERSION;

    TraceEnter(TRACE_PWELL, "PropertyWell_OnChooseProperty");

    SetWaitCursor();

    // construct a menu, and populate with the elements from teh class list, 
    // which we store in a DSA assocaited with this query form

    hr = PropertyWell_GetClassList(ppw);
    FailGracefully(hr, "Failed to get the class list");

    pes.wID = CLID_FIRST;
    pes.hdpaAttributes = DPA_Create(4);
    if ( !pes.hdpaAttributes )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate string DPA");

    hMenuToTrack = hMenu = CreatePopupMenu();
    TraceAssert(hMenu);

    if ( !hMenu )
        ExitGracefully(hr, E_FAIL, "Failed to create class menu");

    for ( pes.iClass = 0; pes.iClass < DSA_GetItemCount(ppw->hdsaClasses); pes.iClass++ )
    {
        pCE = (LPCLASSENTRY)DSA_GetItemPtr(ppw->hdsaClasses, pes.iClass);
        TraceAssert(pCE);

        // Create the sub-menu for this entry in the cache and populate it with the list of
        // properties we picked from the schema.

        pes.hMenu = CreatePopupMenu();
        TraceAssert(pes.hMenu);

        if ( !pes.hMenu )
            ExitGracefully(hr, E_FAIL, "Failed when creating the sub menu for the property list");

        if ( FAILED(EnumClassAttributes(ppw->pdds, pCE->pName, _FillMenuCB, (LPARAM)&pes)) )
        {
            DestroyMenu(pes.hMenu);
            ExitGracefully(hr, E_FAIL, "Failed when building the property menu");
        }                
            
        // Now add that sub-menu to the main menu with a caption that reflects the name of
        // the class we are picking from.

        mii.cbSize = SIZEOF(mii);
        mii.fMask = MIIM_SUBMENU|MIIM_TYPE;
        mii.fType = MFT_STRING;
        mii.hSubMenu = pes.hMenu;
        mii.dwTypeData = pCE->pDisplayName;
        mii.cch = MAX_PATH;

        if ( !InsertMenuItem(hMenu, 0x7fff, TRUE, &mii) )
        {
            DestroyMenu(pes.hMenu);
            ExitGracefully(hr, E_FAIL, "Failed when building the class menu");
        }
    }

    ResetWaitCursor();

    // having constructed the menu lets display it just below the button
    // we are invoked from, if the user selects something then lets put
    // it into the edit line which will enable the rest of the UI.
    
    GetWindowRect(ppw->hwndPropertyLabel, &rcItem);

    if ( GetMenuItemCount(hMenu) == 1 )
    {
        TraceMsg("Single class in menu, therefore just showing properties");
        hMenuToTrack = GetSubMenu(hMenu, 0);
        TraceAssert(hMenuToTrack);
    }
       
    uID = TrackPopupMenu(hMenuToTrack,
                         TPM_TOPALIGN|TPM_RETURNCMD, 
                         rcItem.left, rcItem.bottom,
                         0, ppw->hwnd, NULL);   
    if ( !uID )
    {
        TraceMsg("Menu canceled nothing selected");
    }
    else 
    {
        mii.cbSize = SIZEOF(mii);
        mii.fMask = MIIM_DATA;

        if ( !GetMenuItemInfo(hMenu, uID, FALSE, &mii) )
            ExitGracefully(hr, E_FAIL, "Failed to get item data");

        // unpick the item data and get the iClass and iProperty of the item 
        // we have selected, that way we can then populate the control
        // with the property name.
    
        pCE = (LPCLASSENTRY)DSA_GetItemPtr(ppw->hdsaClasses, LOWORD(mii.dwItemData));
        TraceAssert(pCE);

        pszAttribute = StringDPA_GetStringW(pes.hdpaAttributes, HIWORD(mii.dwItemData));
        Trace(TEXT("Attribute selected : %s"), W2T(pszAttribute));

        hr = PropertyWell_EditProperty(ppw, pCE, pszAttribute, -1);
        FailGracefully(hr, "Failed to set edit property");
    }
        
    hr = S_OK;                // success

exit_gracefully:

    if ( hMenu )
        DestroyMenu(hMenu);

    StringDPA_Destroy(&pes.hdpaAttributes);

    ResetWaitCursor();

    TraceLeave();
} 


/*-----------------------------------------------------------------------------
/ Class/Property maps 
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ PropertyWell_GetClassList
/ -------------------------
/   Obtain the list of visible classes for the for this user.  If the 
/   list is already present then just return S_OK.
/
/ In:
/   ppw -> property well structure
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

//
// Return all display specifiers who have a class display name and a list of
// attributes to be displayed in the UI.
//

WCHAR c_szQuery[] = L"(&(classDisplayName=*)(attributeDisplayNames=*))";

LPWSTR pProperties[] = 
{
    L"name",
    L"classDisplayName",
};

#define PAGE_SIZE 128

HRESULT PropertyWell_GetClassList(LPPROPERTYWELL ppw)
{
    HRESULT hr;
    IQueryFrame* pQueryFrame = NULL;
    IDirectorySearch* pds = NULL;
    ADS_SEARCH_COLUMN column;
    ADS_SEARCHPREF_INFO prefInfo[3];
    ADS_SEARCH_HANDLE hSearch = NULL;
    CLASSENTRY ce;
    LPDSQUERYCLASSLIST pDsQueryClassList = NULL;
    LPWSTR pName = NULL;
    LPWSTR pDisplayName = NULL;
    WCHAR szBufferW[MAX_PATH];
    INT i;
    DECLAREWAITCURSOR;
    USES_CONVERSION;

    TraceEnter(TRACE_PWELL, "PropertyWell_GetClassList");

    SetWaitCursor();

    if ( !ppw->hdsaClasses )
    {
        // Construct a DSA for us to store the class information we need.

        ppw->hdsaClasses = DSA_Create(SIZEOF(CLASSENTRY), 4);
        TraceAssert(ppw->hdsaClasses);

        if ( !ppw->hdsaClasses )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to create class DSA");

        // Call the query form we are part of to see if they want to declare any classes
        // for us to show in the drop down.  We use the CQFWM_GETFRAME to get the
        // IQueryFrame interface from the form and then call all the forms
        // with a magic bit so we (the property well) ignore the
        // request for the class list.

        if ( SendMessage(GetParent(ppw->hwnd), CQFWM_GETFRAME, 0, (LPARAM)&pQueryFrame) )
        {
            if ( SUCCEEDED(pQueryFrame->CallForm(NULL, DSQPM_GETCLASSLIST, 
                                                          DSQPM_GCL_FORPROPERTYWELL, (LPARAM)&pDsQueryClassList)) )
            {
                if ( pDsQueryClassList )
                {
                    for ( i = 0 ; i < pDsQueryClassList->cClasses ; i++ )
                    {
                        LPWSTR pObjectClass = (LPWSTR)ByteOffset(pDsQueryClassList, pDsQueryClassList->offsetClass[i]);
                        TraceAssert(pObjectClass);

                        TraceAssert(ppw->pdds != NULL);
                        ppw->pdds->GetFriendlyClassName(pObjectClass, szBufferW, ARRAYSIZE(szBufferW));

                        ce.pName = NULL;
                        ce.pDisplayName = NULL;
                        ce.cReferences = 0;

                        if ( FAILED(LocalAllocStringW(&ce.pName, pObjectClass)) ||
                                FAILED(LocalAllocStringW2T(&ce.pDisplayName, szBufferW)) ||
                                    ( -1 == DSA_AppendItem(ppw->hdsaClasses, &ce)) )
                        {
                            LocalFreeStringW(&ce.pName);
                            LocalFreeString(&ce.pDisplayName);
                        }
                    }
                }
            }
        }

        // if we didn't get anything from the form we are hosted on then let us
        // troll around in the display specifier container collecting all the
        // objects from there.

        if ( DSA_GetItemCount(ppw->hdsaClasses) == 0 )
        {
            // Set the query prefernece to single level scope, and async retrevial rather
            // than waiting for all objects

            TraceAssert(ppw->pdds);            
            hr = ppw->pdds->GetDisplaySpecifier(NULL, IID_IDirectorySearch, (LPVOID*)&pds);
            FailGracefully(hr, "Failed to get IDsSearch on the display-spec container");

            prefInfo[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
            prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
            prefInfo[0].vValue.Integer = ADS_SCOPE_ONELEVEL;

            prefInfo[1].dwSearchPref = ADS_SEARCHPREF_ASYNCHRONOUS;
            prefInfo[1].vValue.dwType = ADSTYPE_BOOLEAN;
            prefInfo[1].vValue.Boolean = TRUE;

            prefInfo[2].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;         // paged results
            prefInfo[2].vValue.dwType = ADSTYPE_INTEGER;
            prefInfo[2].vValue.Integer = PAGE_SIZE;

            hr = pds->SetSearchPreference(prefInfo, ARRAYSIZE(prefInfo));
            FailGracefully(hr, "Failed to set search preferences");

            hr = pds->ExecuteSearch(c_szQuery, pProperties, ARRAYSIZE(pProperties), &hSearch);
            FailGracefully(hr, "Failed in ExecuteSearch");

            while ( TRUE )
            {
                LocalFreeStringW(&pName);
                LocalFreeStringW(&pDisplayName);

                // Get the next row from the result set, it consists of
                // two columns.  The first column is the class name of
                // the object (<className-Display>) and the second
                // is the friendly name of the class we are trying
                // to display.

                hr = pds->GetNextRow(hSearch);
                FailGracefully(hr, "Failed to get the next row");

                if ( hr == S_ADS_NOMORE_ROWS )
                {
                    TraceMsg("No more results, no more rows");
                    break;
                }

                if ( SUCCEEDED(pds->GetColumn(hSearch, pProperties[0], &column)) )
                {
                    hr = StringFromSearchColumn(&column, &pName);
                    pds->FreeColumn(&column);
                    FailGracefully(hr, "Failed to get the name object");
                }

                if ( SUCCEEDED(pds->GetColumn(hSearch, pProperties[1], &column)) )
                {
                    hr = StringFromSearchColumn(&column, &pDisplayName);
                    pds->FreeColumn(&column);
                    FailGracefully(hr, "Failed to get the display name from the object");
                }

                Trace(TEXT("Display name %s for class %s"), W2T(pDisplayName), W2T(pName));                

                // now allocate an item and put it into the menu so we can
                // allow the user to select an object from the class.
           
                TraceAssert(pName);
                TraceAssert(wcschr(pName, TEXT('-')) != NULL);
                *wcschr(pName, TEXT('-')) = L'\0';               // truncate at the - to give the class name

                ce.pName = NULL;
                ce.pDisplayName = NULL;
                ce.cReferences = 0;

                if ( *pName )
                {
                    if ( FAILED(LocalAllocStringW(&ce.pName, pName)) ||
                            FAILED(LocalAllocStringW2T(&ce.pDisplayName, pDisplayName)) ||
                                ( -1 == DSA_AppendItem(ppw->hdsaClasses, &ce)) )
                    {
                        LocalFreeStringW(&ce.pName);
                        LocalFreeString(&ce.pDisplayName);
                    }
                }
            }
        }
    }

    hr = S_OK;

exit_gracefully:    

    LocalFreeStringW(&pName);
    LocalFreeStringW(&pDisplayName);

    if ( pDsQueryClassList )
        CoTaskMemFree(pDsQueryClassList);

    if ( hSearch )
        pds->CloseSearchHandle(hSearch);

    DoRelease(pQueryFrame);
    DoRelease(pds);

    PropertyWell_EnableControls(ppw, TRUE);

    ResetWaitCursor();

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ PropertyWell_FreeClassList
/ --------------------------
/   Tidy up the class list by walking the DSA if we have one allocated
/   and release all dangling elements.
/
/ In:
/   ppw -> property well structure
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

INT _FreeClassListCB(LPVOID pItem, LPVOID pData)
{
    LPCLASSENTRY pCE = (LPCLASSENTRY)pItem;

    LocalFreeStringW(&pCE->pName);
    LocalFreeString(&pCE->pDisplayName);

    return 1;
}

VOID PropertyWell_FreeClassList(LPPROPERTYWELL ppw)
{
    HRESULT hr;

    TraceEnter(TRACE_PWELL, "PropertyWell_FreeClassList");

    if ( ppw->hdsaClasses )
        DSA_DestroyCallback(ppw->hdsaClasses, _FreeClassListCB, NULL);

    ppw->hdsaClasses = NULL;

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ PropertyWell_FindClass
/ ----------------------
/   Find the class the caller wants.  They give us a property well 
/   and a class name, we return them a class entry structure or NULL.
/
/ In:
/   ppw -> property well structure
/   pObjectClass = class to locate
/
/ Out:
/   LPCLASSETNRY
/----------------------------------------------------------------------------*/
LPCLASSENTRY PropertyWell_FindClassEntry(LPPROPERTYWELL ppw, LPWSTR pObjectClass)
{
    LPCLASSENTRY pResult = NULL;
    INT i;

    TraceEnter(TRACE_PWELL, "PropertyWell_FindClass");

    if ( SUCCEEDED(PropertyWell_GetClassList(ppw)) )
    {
        for ( i = 0 ; i < DSA_GetItemCount(ppw->hdsaClasses) ; i++ )
        {
            LPCLASSENTRY pClassEntry = (LPCLASSENTRY)DSA_GetItemPtr(ppw->hdsaClasses, i);
            TraceAssert(pClassEntry);

            if ( !StrCmpIW(pClassEntry->pName, pObjectClass) )
            {
                pResult = pClassEntry;
                break;
            }
        }
    }

    TraceLeaveValue(pResult);
}


/*-----------------------------------------------------------------------------
/ Rule list helper functions
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ PropertyWell_AddItem
/ --------------------
/   Add an item to the list of rules.
/
/ In:
/   ppw -> window defn to use
/   pProperty = property name
/   iCondition = id of condition to apply
/   pValue = string value to compare against
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT PropertyWell_AddItem(LPPROPERTYWELL ppw, LPCLASSENTRY pClassEntry, LPWSTR pProperty, INT iCondition, LPWSTR pValue)
{
    HRESULT hr;
    INT item = -1;
    LV_ITEM lvi;
    LPPROPERTYWELLITEM pItem = NULL;
    TCHAR szBuffer[80];
    WCHAR szBufferW[80];
    USES_CONVERSION;

    TraceEnter(TRACE_PWELL, "PropertyWell_AddItem");
    Trace(TEXT("Property: %s, Condition: %d, Value: %s"), W2T(pProperty), iCondition, W2T(pValue));

    // Allocate an item structure to be stored into the list view DPA.

    pItem = (LPPROPERTYWELLITEM)LocalAlloc(LPTR, SIZEOF(PROPERTYWELLITEM));
    TraceAssert(pItem);

    Trace(TEXT("pItem %0x8"), pItem);

    if ( !pItem )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate item");

    pItem->pClassEntry = pClassEntry;
    pClassEntry->cReferences += 1;

    // pItem->pProperty = NULL;
    // pItem->pValue = NULL;
    pItem->iCondition = iCondition;

    hr = LocalAllocStringW(&pItem->pProperty, pProperty);
    FailGracefully(hr, "Failed to add property to DPA item");

    if ( pValue && pValue[0] )
    {
        hr = LocalAllocStringW(&pItem->pValue, pValue);
        FailGracefully(hr, "Failed to add value to DPA item");
    }

    // Add the item to the list view, lParam pItem structure we just allocated,
    // therefore when calling delete we can tidy up accordingly

    TraceAssert(ppw->pdds);            
    hr = GetFriendlyAttributeName(ppw->pdds, pClassEntry->pName, pProperty, szBufferW, ARRAYSIZE(szBufferW));

    lvi.mask = LVIF_TEXT|LVIF_STATE|LVIF_PARAM;
    lvi.iItem = 0x7fffffff;
    lvi.iSubItem = 0;
    lvi.state = LVIS_SELECTED;
    lvi.stateMask = LVIS_SELECTED;
    lvi.pszText = W2T(szBufferW);
    lvi.lParam = (LPARAM)pItem;

    item = ListView_InsertItem(ppw->hwndList, &lvi);
    Trace(TEXT("item %d"), item);

    if ( item < 0 )
        ExitGracefully(hr, E_FAIL, "Failed to put item into list view");

    LoadString(GLOBAL_HINSTANCE, conditions[iCondition].idsFilter, szBuffer, ARRAYSIZE(szBuffer));
    ListView_SetItemText(ppw->hwndList, item, 1, szBuffer);
    
    if ( pValue )
        ListView_SetItemText(ppw->hwndList, item, 2, W2T(pValue));

    DPA_InsertPtr(ppw->hdpaItems, item, pItem);

    hr = S_OK;              // succeeeded

exit_gracefully:

    if ( FAILED(hr) && (item == -1) && pItem )
    {
        LocalFreeStringW(&pItem->pProperty);
        LocalFreeStringW(&pItem->pValue);
        LocalFree((HLOCAL)pItem);
    }

    if ( SUCCEEDED(hr) )
    {
        PropertyWell_ClearControls(ppw);
        ListView_EnsureVisible(ppw->hwndList, item, FALSE);
        PropertyWell_SetColumnWidths(ppw);
    }
   
    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ PropertyWell_RemoveItem
/ -----------------------
/   Remvoe the given item from the list.  If fDeleteItem is true then we
/   delete the list view entry, which in turn will call us again to 
/   remove the actual data from our DPA.
/
/ In:
/   ppw -> window defn to use
/   iItem = item to be removed
/   fDelelete = call ListView_DeleteItem 
/
/ Out:
/   BOOL
/----------------------------------------------------------------------------*/
void PropertyWell_RemoveItem(LPPROPERTYWELL ppw, INT iItem, BOOL fDeleteItem)
{
    INT item;
    LV_ITEM lvi;
    LPPROPERTYWELLITEM pItem;

    TraceEnter(TRACE_PWELL, "PropertyWell_RemoveItem");
    Trace(TEXT("iItem %d, fDeleteItem %s"), iItem, fDeleteItem ? TEXT("TRUE"):TEXT("FALSE"));

    if ( ppw && (iItem >= 0) )
    {
        if ( fDeleteItem )
        {
            // Now delete the item from the view, note that as a result of this we will
            // be called again (from the WM_NOTIFY handler) to tidy up the structure.

            item = ListView_GetNextItem(ppw->hwndList, iItem, LVNI_BELOW);

            if ( item == -1 )
                item = ListView_GetNextItem(ppw->hwndList, iItem, LVNI_ABOVE);

            if ( item != -1 )
            {
                ListView_SetItemState(ppw->hwndList, item, LVIS_SELECTED, LVIS_SELECTED);
                ListView_EnsureVisible(ppw->hwndList, item, FALSE);
            }

            ListView_DeleteItem(ppw->hwndList, iItem);
            PropertyWell_SetColumnWidths(ppw);
            PropertyWell_EnableControls(ppw, TRUE);
        }
        else
        {
            // Get the item from that index in the DPA, release the memory that it
            // owns and then release it.

            pItem = (LPPROPERTYWELLITEM)DPA_FastGetPtr(ppw->hdpaItems, iItem);
            TraceAssert(pItem);

            if ( pItem )
            {
                pItem->pClassEntry->cReferences -= 1;
                TraceAssert(pItem->pClassEntry->cReferences >= 0);
                
                LocalFreeStringW(&pItem->pProperty);
                LocalFreeStringW(&pItem->pValue);
                LocalFree((HLOCAL)pItem);

                DPA_DeletePtr(ppw->hdpaItems, iItem);
            }
        }
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ PropertyWell_EditItem
/ ---------------------
/   Edit the given item in the list.  In doing so we remove from the list
/   and populate the edit controls with data that represents this
/   rule.
/
/ In:
/   ppw -> window defn to use
/   iItem = item to edit
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void PropertyWell_EditItem(LPPROPERTYWELL ppw, INT iItem)
{
    LPPROPERTYWELLITEM pItem;
    USES_CONVERSION;

    TraceEnter(TRACE_PWELL, "PropertyWell_EditItem");

    if ( ppw && (iItem >= 0) )
    {
        LPPROPERTYWELLITEM pItem = (LPPROPERTYWELLITEM)DPA_FastGetPtr(ppw->hdpaItems, iItem);
        TraceAssert(pItem);

        PropertyWell_EditProperty(ppw, pItem->pClassEntry, pItem->pProperty, pItem->iCondition);
        
        if ( pItem->pValue )
            SetWindowText(ppw->hwndValue, W2T(pItem->pValue));

        PropertyWell_RemoveItem(ppw, iItem, TRUE);
        PropertyWell_EnableControls(ppw, TRUE);
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ PropertyWell_EditProperty
/ -------------------------
/   Set the property edit control and reflect that change into the 
/   other controls in the dialog (the conditions and editor).
/
/ In:
/   ppw -> property well
/   pClassEntry -> class entry structure
/   pPropertyName -> property name to edit
/   iCondition = condition to select
/
/ Out:
/   void
/----------------------------------------------------------------------------*/
HRESULT PropertyWell_EditProperty(LPPROPERTYWELL ppw, LPCLASSENTRY pClassEntry, LPWSTR pPropertyName, INT iCondition)
{
    HRESULT hr;
    TCHAR szBuffer[MAX_PATH];
    WCHAR szBufferW[MAX_PATH];
    INT i, iItem, iCurSel = 0;
    DWORD dwPropertyType;
    USES_CONVERSION;

    TraceEnter(TRACE_PWELL, "PropertyWell_EditProperty");
    Trace(TEXT("Property name '%s', iCondition %d"), W2T(pPropertyName), iCondition);

    // set the property name for this value, then look it up in the cache to get 
    // information about the operators we can apply.

    ppw->pClassEntry = pClassEntry;           // set state for the item we are editing

    LocalFreeStringW(&ppw->pPropertyName);
    hr = LocalAllocStringW(&ppw->pPropertyName, pPropertyName);
    FailGracefully(hr, "Failed to alloc the property name");
   
    TraceAssert(ppw->pdds);            
    GetFriendlyAttributeName(ppw->pdds, pClassEntry->pName, pPropertyName, szBufferW, ARRAYSIZE(szBufferW));
    SetWindowText(ppw->hwndProperty, W2T(szBufferW));

    ComboBox_ResetContent(ppw->hwndCondition);
    SetWindowText(ppw->hwndValue, TEXT(""));

    dwPropertyType = PropertyIsFromAttribute(pPropertyName, ppw->pdds);

    for ( i = 0 ; i < ARRAYSIZE(conditions); i++ )
    {
        if ( conditions[i].dwPropertyType == dwPropertyType )
        {
            LoadString(GLOBAL_HINSTANCE, conditions[i].idsFilter, szBuffer, ARRAYSIZE(szBuffer));
            iItem = ComboBox_AddString(ppw->hwndCondition, szBuffer);

            if ( iItem >= 0 )
            {
                ComboBox_SetItemData(ppw->hwndCondition, iItem, i);           // i == condition index

                if ( i == iCondition )
                {
                    Trace(TEXT("Setting current selection to %d"), iItem);
                    iCurSel = iItem;
                }
            }
        }
    }

    ComboBox_SetCurSel(ppw->hwndCondition, iCurSel);
    SetWindowText(ppw->hwndValue, TEXT(""));

    hr = S_OK;

exit_gracefully:

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ PropertyWell_EnableControls
/ ---------------------------
/   Check the controls within the view and determine what controls
/   should be enabled within it.  If fDisable == TRUE then disable all the
/   controls in the dialog regardless of the dependancies on other controls.
/
/   The return value indicates if the control sare in a state whereby
/   we can add the criteria to the query.
/
/ In:
/   ppw -> window defn to use
/   fEnable = FALSE then disable all controls in dialog
/
/ Out:
/   BOOL
/----------------------------------------------------------------------------*/
BOOL PropertyWell_EnableControls(LPPROPERTYWELL ppw, BOOL fEnable)
{
    BOOL fEnableCondition = FALSE;
    BOOL fEnableValue = FALSE;
    BOOL fEnableAdd = FALSE;
    BOOL fEnableRemove = FALSE;
    INT iCondition;
    DWORD dwButtonStyle;
    HWND hWndParent;

    TraceEnter(TRACE_PWELL, "PropertyWell_EnableControls");

    EnableWindow(ppw->hwndPropertyLabel, fEnable);
    EnableWindow(ppw->hwndProperty, fEnable);
    EnableWindow(ppw->hwndList, fEnable);

    if ( fEnable )
    {
        fEnableCondition = (ppw->pPropertyName != NULL);

        if ( fEnableCondition )
        {
            iCondition = CONDITION_FROM_COMBO(ppw->hwndCondition);
            fEnableValue = !conditions[iCondition].fNoValue;

            if ( !fEnableValue || GetWindowTextLength(ppw->hwndValue) )
                fEnableAdd = TRUE;
        }

        if ( ListView_GetSelectedCount(ppw->hwndList) && 
                    ( -1 != ListView_GetNextItem(ppw->hwndList, -1, LVNI_SELECTED|LVNI_ALL)) )
        {
            fEnableRemove = TRUE;
        }
    }

    if ( !fEnableAdd && !fEnableValue ) 
    {
        dwButtonStyle = (DWORD) GetWindowLong(ppw->hwndAdd, GWL_STYLE);
        if (dwButtonStyle & BS_DEFPUSHBUTTON)
        {
            SendMessage(ppw->hwndAdd, BM_SETSTYLE, MAKEWPARAM(BS_PUSHBUTTON, 0), MAKELPARAM(TRUE, 0));

            hWndParent = GetParent(ppw->hwnd);
            if (hWndParent) 
            {
                SendDlgItemMessage(hWndParent, CQID_FINDNOW, BM_SETSTYLE, MAKEWPARAM(BS_DEFPUSHBUTTON, 0), MAKELPARAM(TRUE, 0));
                SetFocus(GetDlgItem(hWndParent, CQID_FINDNOW));
            }
        }
    }

    if (!fEnableRemove) {

        dwButtonStyle = (DWORD) GetWindowLong(ppw->hwndRemove, GWL_STYLE);

        if (dwButtonStyle & BS_DEFPUSHBUTTON) {
            SendMessage(
                ppw->hwndRemove,
                BM_SETSTYLE,
                MAKEWPARAM(BS_PUSHBUTTON, 0),
                MAKELPARAM(TRUE, 0)
            );

            hWndParent = GetParent(ppw->hwnd);
            if (hWndParent) {
                SendDlgItemMessage(
                    hWndParent,
                    CQID_FINDNOW,
                    BM_SETSTYLE,
                    MAKEWPARAM(BS_DEFPUSHBUTTON, 0),
                    MAKELPARAM(TRUE, 0)
                );
                SetFocus(GetDlgItem(hWndParent, CQID_FINDNOW));
            }

        }

    }

    EnableWindow(ppw->hwndConditionLabel, fEnableCondition);
    EnableWindow(ppw->hwndCondition, fEnableCondition);
    EnableWindow(ppw->hwndValueLabel, fEnableValue);
    EnableWindow(ppw->hwndValue, fEnableValue);
    EnableWindow(ppw->hwndAdd, fEnableAdd);
    EnableWindow(ppw->hwndRemove, fEnableRemove);

    if ( fEnableAdd )
        SetDefButton(ppw->hwnd, IDC_ADD);

    TraceLeaveValue(fEnableAdd);
}


/*-----------------------------------------------------------------------------
/ PropertyWell_ClearControls
/ --------------------------
/   Zap the contents of the edit controls.
/
/ In:
/   ppw -> window defn to use
/
/ Out:
/   BOOL
/----------------------------------------------------------------------------*/
VOID PropertyWell_ClearControls(LPPROPERTYWELL ppw)
{
    TraceEnter(TRACE_PWELL, "PropertyWell_ClearControls");

    LocalFreeStringW(&ppw->pPropertyName);
    SetWindowText(ppw->hwndProperty, TEXT(""));

    ComboBox_ResetContent(ppw->hwndCondition);
    SetWindowText(ppw->hwndValue, TEXT(""));
    PropertyWell_EnableControls(ppw, TRUE);

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ PropertyWell_SetColumnWidths
/ ----------------------------
/   Fix the widths of the columns in the list view section of the property
/   well so that the most is visible.
/
/ In:
/   ppw -> window defn to use
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
VOID PropertyWell_SetColumnWidths(LPPROPERTYWELL ppw)
{
    RECT rect2;
    INT cx;

    TraceEnter(TRACE_PWELL, "PropertyWell_SetColumnWidths");

    GetClientRect(ppw->hwndList, &rect2);
    InflateRect(&rect2, -GetSystemMetrics(SM_CXBORDER)*2, 0);

    cx = MulDiv((rect2.right - rect2.left), 20, 100);

    ListView_SetColumnWidth(ppw->hwndList, 0, cx);
    ListView_SetColumnWidth(ppw->hwndList, 1, cx);
    ListView_SetColumnWidth(ppw->hwndList, 2, rect2.right - (cx*2));

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ PropertyWell_GetQuery
/ ---------------------
/   Take the items in the property well and construct a query from them,
/   the query is an AND of all the fields present in the list.  The conditon
/   table in lists the prefix, condition and postfix for each of the possible
/   conditions in the combo box.
/
/ In:
/   ppw -> property well to construct from
/   ppQuery -> receives the query string
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

static void _GetQuery(LPPROPERTYWELL ppw, LPWSTR pQuery, UINT* pLen)
{
    INT i;
    USES_CONVERSION;
    
    TraceEnter(TRACE_PWELL, "_GetQuery");

    if ( pQuery )
        *pQuery = TEXT('\0');

    TraceAssert(ppw->hdsaClasses);
    TraceAssert(ppw->hdpaItems);

    for ( i = 0 ; i < DSA_GetItemCount(ppw->hdsaClasses); i++ )
    {
        LPCLASSENTRY pClassEntry = (LPCLASSENTRY)DSA_GetItemPtr(ppw->hdsaClasses, i);
        TraceAssert(pClassEntry);

        if ( pClassEntry->cReferences )
        {
            Trace(TEXT("Class %s referenced %d times"), W2T(pClassEntry->pName), pClassEntry->cReferences);
            GetFilterString(pQuery, pLen, FILTER_IS, L"objectCategory", pClassEntry->pName);
        }
    }

    for ( i = 0 ; i < DPA_GetPtrCount(ppw->hdpaItems); i++ )
    {
        LPPROPERTYWELLITEM pItem = (LPPROPERTYWELLITEM)DPA_FastGetPtr(ppw->hdpaItems, i);
        TraceAssert(pItem);

        GetFilterString(pQuery, pLen, conditions[pItem->iCondition].iFilter, pItem->pProperty, pItem->pValue);
    }

    TraceLeave();
}

HRESULT PropertyWell_GetQuery(LPPROPERTYWELL ppw, LPWSTR* ppQuery)
{
    HRESULT hr;
    UINT cchQuery = 0;
    USES_CONVERSION;

    TraceEnter(TRACE_PWELL, "PropertyWell_GetQuery");
    
    *ppQuery = NULL;

    hr = PropertyWell_GetClassList(ppw);
    FailGracefully(hr, "Failed to get the class list");

    _GetQuery(ppw, NULL, &cchQuery);
    Trace(TEXT("cchQuery %d"), cchQuery);

    if ( cchQuery )
    {
        hr = LocalAllocStringLenW(ppQuery, cchQuery);
        FailGracefully(hr, "Failed to allocate buffer for query string");

        _GetQuery(ppw, *ppQuery, NULL);
        Trace(TEXT("Resulting query is %s"), W2T(*ppQuery));
    }

    hr = S_OK;

exit_gracefully:

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ PropertyWell_Persist
/ --------------------
/   Persist the contents of the property well, either read them or write
/   them depending on the given flag.
/
/ In:
/   ppw -> property well to work with
/   pPersistQuery -> IPersistQuery structure to work with
/   fRead = read or write
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT PropertyWell_Persist(LPPROPERTYWELL ppw, IPersistQuery* pPersistQuery, BOOL fRead)
{
    HRESULT hr;
    LPPROPERTYWELLITEM pItem;
    TCHAR szBuffer[80];
    INT iItems;
    INT i;
    USES_CONVERSION;

    TraceEnter(TRACE_PWELL, "PropertyWell_Persist");

    if ( !pPersistQuery )
        ExitGracefully(hr, E_INVALIDARG, "No persist object");

    if ( fRead )
    {
        // Read the items from the IPersistQuery object, first get the number of items
        // we need to get back.  Then loop through them all getting the property, condition
        // and value.

        hr = pPersistQuery->ReadInt(c_szMsPropertyWell, c_szItems, &iItems);
        FailGracefully(hr, "Failed to get item count");

        Trace(TEXT("Attempting to read %d items"), iItems);

        for ( i = 0 ; i < iItems ; i++ )
        {
            LPCLASSENTRY pClassEntry;
            TCHAR szObjectClass[MAX_PATH];
            TCHAR szProperty[MAX_PATH];
            TCHAR szValue[MAX_PATH];
            INT iCondition;

            wsprintf(szBuffer, c_szObjectClassN, i);
            hr = pPersistQuery->ReadString(c_szMsPropertyWell, szBuffer, szObjectClass, ARRAYSIZE(szObjectClass));
            FailGracefully(hr, "Failed to read object class");

            pClassEntry = PropertyWell_FindClassEntry(ppw, T2W(szObjectClass));
            TraceAssert(pClassEntry);

            if ( !pClassEntry )
                ExitGracefully(hr, E_FAIL, "Failed to get objectClass / map to available class");

            wsprintf(szBuffer, c_szProperty, i);
            hr = pPersistQuery->ReadString(c_szMsPropertyWell, szBuffer, szProperty, ARRAYSIZE(szProperty));
            FailGracefully(hr, "Failed to read property");

            wsprintf(szBuffer, c_szCondition, i);
            hr = pPersistQuery->ReadInt(c_szMsPropertyWell, szBuffer, &iCondition);
            FailGracefully(hr, "Failed to write condition");

            wsprintf(szBuffer, c_szValue, i);
            
            if ( FAILED(pPersistQuery->ReadString(c_szMsPropertyWell, szBuffer, szValue, ARRAYSIZE(szValue))) )
            {
                TraceMsg("No value defined in incoming stream");
                szValue[0] = TEXT('\0');
            }

            hr = PropertyWell_AddItem(ppw, pClassEntry, T2W(szProperty), iCondition, T2W(szValue));
            FailGracefully(hr, "Failed to add search criteria to query");
        }
    }
    else
    {
        // Write the content of the property well out, store the items then for
        // each store Condition%d, Value%d, Property%d.

        iItems = DPA_GetPtrCount(ppw->hdpaItems);

        Trace(TEXT("Attempting to store %d items"), iItems);

        hr = pPersistQuery->WriteInt(c_szMsPropertyWell, c_szItems, iItems);
        FailGracefully(hr, "Failed to write item count");

        for ( i = 0 ; i < iItems ; i++ )
        {
            pItem = (LPPROPERTYWELLITEM)DPA_FastGetPtr(ppw->hdpaItems, i);

            wsprintf(szBuffer, c_szObjectClassN, i);
            hr = pPersistQuery->WriteString(c_szMsPropertyWell, szBuffer, W2T(pItem->pClassEntry->pName));
            FailGracefully(hr, "Failed to write property");

            wsprintf(szBuffer, c_szProperty, i);
            hr = pPersistQuery->WriteString(c_szMsPropertyWell, szBuffer, W2T(pItem->pProperty));
            FailGracefully(hr, "Failed to write property");

            wsprintf(szBuffer, c_szCondition, i);
            hr = pPersistQuery->WriteInt(c_szMsPropertyWell, szBuffer, pItem->iCondition);
            FailGracefully(hr, "Failed to write condition");

            if ( pItem->pValue )
            {
                wsprintf(szBuffer, c_szValue, i);
                hr = pPersistQuery->WriteString(c_szMsPropertyWell, szBuffer, W2T(pItem->pValue));
                FailGracefully(hr, "Failed to write value");
            }
        }
    }

    hr = S_OK;

exit_gracefully:

    TraceLeaveResult(hr);
}
