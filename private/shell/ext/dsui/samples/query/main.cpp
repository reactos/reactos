/*----------------------------------------------------------------------------
/ Title;
/   main.cpp
/
/ Authors;
/   David De Vorchik (daviddv)
/
/ Notes;
/   Query the DS, put the results into a list view.
/   This code is pretty lax with Trace and error reporting.
/----------------------------------------------------------------------------*/
#include "windows.h"
#include "windowsx.h"
#include "commctrl.h"
#include "resource.h"
#include "shlobj.h"
#include "shlobjp.h"
#include "shlwapi.h"

//
// Private headers from the shell/dsui projects
//

#include "comctrlp.h"
#include "shellapi.h"
#include "shsemip.h"
#include "shlwapip.h"
#include "common.h"

#pragma hdrstop

#define INITGUID
#include "initguid.h"
#include "cmnquery.h"
#include "cmnquryp.h"
#include "dsquery.h"
#include "dsshell.h"


/*-----------------------------------------------------------------------------
/ Globals and stuff
/----------------------------------------------------------------------------*/

struct
{
    INT idsForm;
    CLSID const * pFormCLSID;
}
forms[] =
{
    IDS_FORMUSERS,            &CLSID_DsFindPeople,
    IDS_FORMCOMPUTERS,        &CLSID_DsFindComputer,
    IDS_FORMPRINT,            &CLSID_DsFindPrinter,
    IDS_FORMFILEFOLDERS,      &CLSID_DsFindVolume,
    IDS_FORMDSFOLDERS,        &CLSID_DsFindContainer,
    IDS_FORMOBJECTS,          &CLSID_DsFindObjects,
    IDS_FORMPWELL,            &CLSID_DsFindAdvanced,
    IDS_FORMDC,               &CLSID_DsFindDomainController,
};

#define INI_FILENAME      TEXT("query.ini")
#define DEFAULT_SCOPE     TEXT("LDAP://ntdev")
#define DEFAULT_SAVELOCN  TEXT("c:\\temp")

HINSTANCE g_hInstance = NULL;
UINT g_cfDsObjectNames = 0;
UINT g_cfDsQueryParams = 0;

BOOL CALLBACK Query_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

void Query_OnInitDialog(HWND hDlg, LPTSTR pDefaultScope);
void Query_OnDoSearch(HWND hDlg);
void Query_BrowseForScope(HWND hDlg);
void Query_BrowseForSaveLocation(HWND hDlg);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

struct
{
    UINT ids;
    int  fmt;
    int  cx;
}
columnsResults[] =
{
    IDS_OBJECTNAME,     LVCFMT_LEFT, 192,
    IDS_CLASS,          LVCFMT_LEFT, 64,
    IDS_FLAGS,          LVCFMT_LEFT, 64,
    IDS_PROVIDERFLAGS,  LVCFMT_LEFT, 96,
};

struct
{
    UINT ids;
    int  fmt;
    int  cx;
}
columnsPersistInfo[] =
{
    IDS_SECTION,        LVCFMT_LEFT, 128,
    IDS_TAG,            LVCFMT_LEFT, 128,
    IDS_DATA,           LVCFMT_LEFT, 128,
};


//
// Example persisting iface for poulating the list view
//

class CPersistQuery : public IPersistQuery
{
    private:
        ULONG m_cRefCount;
        HWND m_hwndView;

    public:
        CPersistQuery(HWND hwndView);

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IPersist
        STDMETHOD(GetClassID)(THIS_ CLSID* pClassID);

        // IPersistQuery
        STDMETHOD(WriteString)(THIS_ LPCTSTR pSection, LPCTSTR pValueName, LPCTSTR pValue);
        STDMETHOD(ReadString)(THIS_ LPCTSTR pSection, LPCTSTR pValueName, LPTSTR pBuffer, INT cchBuffer);
        STDMETHOD(WriteInt)(THIS_ LPCTSTR pSection, LPCTSTR pValueName, INT value);
        STDMETHOD(ReadInt)(THIS_ LPCTSTR pSection, LPCTSTR pValueName, LPINT pValue);
        STDMETHOD(WriteStruct)(THIS_ LPCTSTR pSection, LPCTSTR pValueName, LPVOID pStruct, DWORD cbStruct);
        STDMETHOD(ReadStruct)(THIS_ LPCTSTR pSection, LPCTSTR pValueName, LPVOID pStruct, DWORD cbStruct);
        STDMETHOD(Clear)(THIS);
};


/*-----------------------------------------------------------------------------
/ Query_DlgProc
/ -------------
/   Handle dialog messages for this window.
/
/ In:
/   hDlg -> dialog to be initialized
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
BOOL CALLBACK Query_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fResult = FALSE;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            Query_OnInitDialog(hDlg, (LPTSTR)lParam);
            break;

        case WM_SHOWWINDOW:
        {
#if 0
            if ( wParam )
                PostMessage(hDlg, WM_COMMAND, IDC_SEARCH, 0);
#endif
            break;
        }

        case WM_COMMAND:
        {
            switch ( LOWORD(wParam) )
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
                    break;

                case IDC_SEARCH:
                    Query_OnDoSearch(hDlg);
                    break;

                case IDC_SCOPEBROWSE:
                    Query_BrowseForScope(hDlg);
                    break;                                       

                case IDC_SAVEBROWSE:
                    Query_BrowseForSaveLocation(hDlg);
                    break;                                       

                case IDC_DEFAULTFORM:
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_FORMLIST), 
                                    IsDlgButtonChecked(hDlg, IDC_DEFAULTFORM)== BST_CHECKED);
                    break;
                }

                case IDC_DEFAULTSCOPE:
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_SCOPELOCN), 
                                    IsDlgButtonChecked(hDlg, IDC_DEFAULTSCOPE)== BST_CHECKED);
                    EnableWindow(GetDlgItem(hDlg, IDC_SCOPEBROWSE), 
                                    IsDlgButtonChecked(hDlg, IDC_DEFAULTSCOPE)== BST_CHECKED);
                    break;
                }

                case IDC_SAVEWHERE:
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_SAVELOCN), 
                                    IsDlgButtonChecked(hDlg, IDC_SAVEWHERE)== BST_CHECKED);
                    EnableWindow(GetDlgItem(hDlg, IDC_SAVEBROWSE), 
                                    IsDlgButtonChecked(hDlg, IDC_SAVEWHERE)== BST_CHECKED);         
                    break;
                }
                
                case IDC_SAVEONCLOSE:
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_PERSISTDATA), 
                                    IsDlgButtonChecked(hDlg, IDC_SAVEONCLOSE)== BST_CHECKED);
                    break;
                }

                case IDC_GETRESULTS:
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_RESULTS), 
                                    IsDlgButtonChecked(hDlg, IDC_GETRESULTS)== BST_CHECKED);         
                    break;
                }

                default:
                    break;

            }
        }
    }

    return fResult;
}


/*-----------------------------------------------------------------------------
/ Query_OnInitDialog
/ ------------------
/   Initialize the dialog (the result viewer).
/
/ In:
/   hDlg -> dialog to be initialized
/   pDefaultScope -> default scope to use when browsing
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void Query_OnInitDialog(HWND hDlg, LPTSTR pDefaultScope)
{
    TCHAR szBuffer[MAX_PATH];
    LV_COLUMN lvc;
    INT i;
    TCHAR szScopeLocn[MAX_PATH];
    TCHAR szSaveLocn[MAX_PATH];    
    BOOL fSingleSelect = FALSE;
    BOOL fDefaultForm = FALSE;
    BOOL fDefaultScope = FALSE;
    BOOL fNoSave = FALSE;
    BOOL fSaveWhere = FALSE;
    BOOL fShowHidden = FALSE;
    BOOL fRemoveScopes = FALSE;
    BOOL fRemoveForms = FALSE;
    BOOL fIssueOnOpen = FALSE;   
    BOOL fShowOptional = FALSE;
    BOOL fAdminBits = FALSE;
    BOOL fHideSearchPane = FALSE;
    BOOL fPersistOnOK = FALSE;
    BOOL fHideMenus = FALSE;
    BOOL fHideSearchUI = FALSE;
    BOOL fGetResults = TRUE;
    UINT iDefaultForm = ARRAYSIZE(forms);
    
    // Setup the columns within the ListView

    for ( i = 0 ; i < ARRAYSIZE(columnsResults) ; i++ )
    {
        LoadString(g_hInstance, columnsResults[i].ids, szBuffer, ARRAYSIZE(szBuffer));

        lvc.mask = LVCF_FMT|LVCF_TEXT|LVCF_WIDTH;
        lvc.fmt = columnsResults[i].fmt;
        lvc.cx = columnsResults[i].cx;
        lvc.pszText = szBuffer;

        ListView_InsertColumn(GetDlgItem(hDlg, IDC_RESULTS), i, &lvc);
    }

    ListView_SetExtendedListViewStyle(GetDlgItem(hDlg, IDC_RESULTS), LVS_EX_FULLROWSELECT);

    // setup columns in persist window

    for ( i = 0 ; i < ARRAYSIZE(columnsPersistInfo) ; i++ )
    {
        LoadString(g_hInstance, columnsPersistInfo[i].ids, szBuffer, ARRAYSIZE(szBuffer));

        lvc.mask = LVCF_FMT|LVCF_TEXT|LVCF_WIDTH;
        lvc.fmt = columnsPersistInfo[i].fmt;
        lvc.cx = columnsPersistInfo[i].cx;
        lvc.pszText = szBuffer;

        ListView_InsertColumn(GetDlgItem(hDlg, IDC_PERSISTDATA), i, &lvc);
    }

    ListView_SetExtendedListViewStyle(GetDlgItem(hDlg, IDC_PERSISTDATA), LVS_EX_FULLROWSELECT);

    // Setup forms in the form list

    for ( i = 0 ; i < ARRAYSIZE(forms); i++ )
    {
        LoadString(g_hInstance, forms[i].idsForm, szBuffer, ARRAYSIZE(szBuffer));
        ComboBox_AddString(GetDlgItem(hDlg, IDC_FORMLIST), szBuffer);
    }

    // Load settings from the ini file and prep the dialog box with the settings

    GetPrivateProfileStruct(TEXT("Query"), TEXT("SingleSelect"),   &fSingleSelect,   SIZEOF(fSingleSelect),   INI_FILENAME);
    GetPrivateProfileStruct(TEXT("Query"), TEXT("DefaultScope"),   &fDefaultScope,   SIZEOF(fDefaultScope),   INI_FILENAME);
    GetPrivateProfileStruct(TEXT("Query"), TEXT("NoSave"),         &fNoSave,         SIZEOF(fNoSave),         INI_FILENAME);
    GetPrivateProfileStruct(TEXT("Query"), TEXT("SaveWhere"),      &fSaveWhere,      SIZEOF(fSaveWhere),      INI_FILENAME);
    GetPrivateProfileStruct(TEXT("Query"), TEXT("DefaultForm"),    &iDefaultForm,    SIZEOF(iDefaultForm),    INI_FILENAME);
    GetPrivateProfileStruct(TEXT("Query"), TEXT("ShowHidden"),     &fShowHidden,     SIZEOF(fShowHidden),     INI_FILENAME);
    GetPrivateProfileStruct(TEXT("Query"), TEXT("RemoveScope"),    &fRemoveScopes,   SIZEOF(fRemoveScopes),   INI_FILENAME);
    GetPrivateProfileStruct(TEXT("Query"), TEXT("RemoveForms"),    &fRemoveForms,    SIZEOF(fRemoveForms),    INI_FILENAME);
    GetPrivateProfileStruct(TEXT("Query"), TEXT("IssueOnOpen"),    &fIssueOnOpen,    SIZEOF(fIssueOnOpen),    INI_FILENAME);
    GetPrivateProfileStruct(TEXT("Query"), TEXT("ShowOptional"),   &fShowOptional,   SIZEOF(fShowOptional),   INI_FILENAME);
    GetPrivateProfileStruct(TEXT("Query"), TEXT("AdmimnBits"),     &fAdminBits,      SIZEOF(fAdminBits),      INI_FILENAME);
    GetPrivateProfileStruct(TEXT("Query"), TEXT("HideSearchPane"), &fHideSearchPane, SIZEOF(fHideSearchPane), INI_FILENAME);
    GetPrivateProfileStruct(TEXT("Query"), TEXT("PersistOnOK"),    &fPersistOnOK,    SIZEOF(fPersistOnOK),    INI_FILENAME);
    GetPrivateProfileStruct(TEXT("Query"), TEXT("HideMenus"),      &fHideMenus,      SIZEOF(fHideMenus),      INI_FILENAME);
    GetPrivateProfileStruct(TEXT("Query"), TEXT("AsFilterDlg"),    &fHideSearchUI,   SIZEOF(fHideSearchUI),   INI_FILENAME);
    GetPrivateProfileStruct(TEXT("Query"), TEXT("GetResults"),     &fGetResults,     SIZEOF(fGetResults),     INI_FILENAME);

    GetPrivateProfileString(TEXT("Query"), TEXT("ScopeLocn"), DEFAULT_SCOPE, szScopeLocn, ARRAYSIZE(szScopeLocn), INI_FILENAME);
    GetPrivateProfileString(TEXT("Query"), TEXT("SaveLocn"), DEFAULT_SAVELOCN , szSaveLocn, ARRAYSIZE(szSaveLocn), INI_FILENAME);

    if ( iDefaultForm == ARRAYSIZE(forms) )
        iDefaultForm = 0;
    else
        fDefaultForm = TRUE;

    ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_FORMLIST), iDefaultForm);

    if ( pDefaultScope )
    {
        SetDlgItemText(hDlg, IDC_SCOPELOCN, pDefaultScope);
        fDefaultScope = TRUE;
    }
    else
    {
        SetDlgItemText(hDlg, IDC_SCOPELOCN, szScopeLocn);
    }

    SetDlgItemText(hDlg, IDC_SAVELOCN, szSaveLocn);

    CheckDlgButton(hDlg, IDC_SINGLESELECT,   fSingleSelect   ? BST_CHECKED:BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_DEFAULTFORM,    fDefaultForm    ? BST_CHECKED:BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_DEFAULTSCOPE,   fDefaultScope   ? BST_CHECKED:BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_NOSAVE,         fNoSave         ? BST_CHECKED:BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_SAVEWHERE,      fSaveWhere      ? BST_CHECKED:BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_SHOWHIDDEN,     fShowHidden     ? BST_CHECKED:BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_NOSCOPES,       fRemoveScopes   ? BST_CHECKED:BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_NOFORMS,        fRemoveForms    ? BST_CHECKED:BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_ISSUEONOPEN,    fIssueOnOpen    ? BST_CHECKED:BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_SHOWOPTIONAL,   fShowOptional   ? BST_CHECKED:BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_ADMINBITS,      fAdminBits      ? BST_CHECKED:BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_HIDESEARCHPANE, fHideSearchPane ? BST_CHECKED:BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_SAVEONCLOSE,    fPersistOnOK    ? BST_CHECKED:BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_HIDEMENUS,      fHideMenus      ? BST_CHECKED:BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_HIDESEARCHUI,   fHideSearchUI   ? BST_CHECKED:BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_GETRESULTS,     fGetResults     ? BST_CHECKED:BST_UNCHECKED);

    EnableWindow(GetDlgItem(hDlg, IDC_FORMLIST), fDefaultForm);
    EnableWindow(GetDlgItem(hDlg, IDC_SCOPELOCN), fDefaultScope);
    EnableWindow(GetDlgItem(hDlg, IDC_SCOPEBROWSE), fDefaultScope);
    EnableWindow(GetDlgItem(hDlg, IDC_SAVELOCN), fSaveWhere);
    EnableWindow(GetDlgItem(hDlg, IDC_SAVEBROWSE), fSaveWhere);
    EnableWindow(GetDlgItem(hDlg, IDC_PERSISTDATA), fPersistOnOK);

    EnableWindow(GetDlgItem(hDlg, IDC_RESULTS), fGetResults);
}


/*-----------------------------------------------------------------------------
/ Query_OnDoSearch
/ ----------------
/   Query the DS and populate the specified list view with results we get back.
/
/ In:
/   hDlg -> parent of find dialog
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void Query_OnDoSearch(HWND hDlg)
{
    HRESULT hr;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium = { TYMED_NULL, NULL, NULL };
    DSQUERYINITPARAMS dqip;
    OPENQUERYWINDOW oqw;
    LPTSTR pObjectName, pClass;
    ICommonQuery* pCommonQuery = NULL;
    IDataObject* pDataObject = NULL;
    HWND hwndResults;
    LV_ITEM lvi;
    UINT i;
    TCHAR szBuffer[MAX_PATH];
    TCHAR szScopeLocn[MAX_PATH];
    TCHAR szSaveLocn[MAX_PATH];    
    BOOL fSingleSelect = FALSE;
    BOOL fDefaultForm = FALSE;
    BOOL fDefaultScope = FALSE;
    BOOL fNoSave = FALSE;
    BOOL fSaveWhere = FALSE;
    BOOL fShowHidden = FALSE;
    BOOL fRemoveScopes = FALSE;
    BOOL fRemoveForms = FALSE;
    BOOL fIssueOnOpen = FALSE;   
    BOOL fShowOptional = FALSE;
    BOOL fAdminBits = FALSE;
    BOOL fHideSearchPane = FALSE;
    BOOL fPersistOnOK = FALSE;
    BOOL fHideMenus = FALSE;
    BOOL fHideSearchUI = FALSE;
    BOOL fGetResults = TRUE;
    UINT iDefaultForm = ARRAYSIZE(forms);
    CPersistQuery* pPersistQuery = NULL;

    // Read the state from the dialog and write them out

    fSingleSelect = IsDlgButtonChecked(hDlg, IDC_SINGLESELECT) == BST_CHECKED;
    fDefaultForm = IsDlgButtonChecked(hDlg, IDC_DEFAULTFORM) == BST_CHECKED;
    fDefaultScope = IsDlgButtonChecked(hDlg, IDC_DEFAULTSCOPE) == BST_CHECKED;
    fNoSave = IsDlgButtonChecked(hDlg, IDC_NOSAVE) == BST_CHECKED;
    fSaveWhere = IsDlgButtonChecked(hDlg, IDC_SAVEWHERE) == BST_CHECKED;
    fShowHidden = IsDlgButtonChecked(hDlg, IDC_SHOWHIDDEN) == BST_CHECKED;
    fRemoveScopes = IsDlgButtonChecked(hDlg, IDC_NOSCOPES) == BST_CHECKED;
    fRemoveForms = IsDlgButtonChecked(hDlg, IDC_NOFORMS) == BST_CHECKED;
    fIssueOnOpen = IsDlgButtonChecked(hDlg, IDC_ISSUEONOPEN) == BST_CHECKED;
    fShowOptional = IsDlgButtonChecked(hDlg, IDC_SHOWOPTIONAL) == BST_CHECKED;
    fAdminBits = IsDlgButtonChecked(hDlg, IDC_ADMINBITS) == BST_CHECKED;
    fHideSearchPane = IsDlgButtonChecked(hDlg, IDC_HIDESEARCHPANE) == BST_CHECKED;
    fPersistOnOK = IsDlgButtonChecked(hDlg, IDC_SAVEONCLOSE) == BST_CHECKED;
    fHideMenus = IsDlgButtonChecked(hDlg, IDC_HIDEMENUS) == BST_CHECKED;
    fHideSearchUI = IsDlgButtonChecked(hDlg, IDC_HIDESEARCHUI) == BST_CHECKED;
    fGetResults = IsDlgButtonChecked(hDlg, IDC_GETRESULTS) == BST_CHECKED;

    GetDlgItemText(hDlg, IDC_SCOPELOCN, szScopeLocn, ARRAYSIZE(szScopeLocn));
    GetDlgItemText(hDlg, IDC_SAVELOCN, szSaveLocn, ARRAYSIZE(szSaveLocn));

    if ( fDefaultForm )
        iDefaultForm = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_FORMLIST));

    WritePrivateProfileStruct(TEXT("Query"), TEXT("SingleSelect"),   &fSingleSelect,   SIZEOF(fSingleSelect),   INI_FILENAME);
    WritePrivateProfileStruct(TEXT("Query"), TEXT("DefaultScope"),   &fDefaultScope,   SIZEOF(fDefaultForm),    INI_FILENAME);
    WritePrivateProfileStruct(TEXT("Query"), TEXT("NoSave"),         &fNoSave,         SIZEOF(fNoSave),         INI_FILENAME);
    WritePrivateProfileStruct(TEXT("Query"), TEXT("SaveWhere"),      &fSaveWhere,      SIZEOF(fSaveWhere),      INI_FILENAME);
    WritePrivateProfileStruct(TEXT("Query"), TEXT("DefaultForm"),    &iDefaultForm,    SIZEOF(iDefaultForm),    INI_FILENAME);
    WritePrivateProfileStruct(TEXT("Query"), TEXT("ShowHidden"),     &fShowHidden,     SIZEOF(fShowHidden),     INI_FILENAME);
    WritePrivateProfileStruct(TEXT("Query"), TEXT("RemoveScope"),    &fRemoveScopes,   SIZEOF(fRemoveScopes),   INI_FILENAME);
    WritePrivateProfileStruct(TEXT("Query"), TEXT("RemoveForms"),    &fRemoveForms,    SIZEOF(fRemoveForms),    INI_FILENAME);
    WritePrivateProfileStruct(TEXT("Query"), TEXT("IssueOnOpen"),    &fIssueOnOpen,    SIZEOF(fIssueOnOpen),    INI_FILENAME);
    WritePrivateProfileStruct(TEXT("Query"), TEXT("ShowOptional"),   &fShowOptional,   SIZEOF(fShowOptional),   INI_FILENAME);
    WritePrivateProfileStruct(TEXT("Query"), TEXT("AdmimnBits"),     &fAdminBits,      SIZEOF(fAdminBits),      INI_FILENAME);
    WritePrivateProfileStruct(TEXT("Query"), TEXT("HideSearchPane"), &fHideSearchPane, SIZEOF(fHideSearchPane), INI_FILENAME);
    WritePrivateProfileStruct(TEXT("Query"), TEXT("PersistOnOK"),    &fPersistOnOK,    SIZEOF(fPersistOnOK),    INI_FILENAME);
    WritePrivateProfileStruct(TEXT("Query"), TEXT("HideMenus"),      &fHideMenus,      SIZEOF(fHideMenus),      INI_FILENAME);
    WritePrivateProfileStruct(TEXT("Query"), TEXT("AsFilterDlg"),    &fHideSearchUI,   SIZEOF(fHideSearchUI),   INI_FILENAME);
    WritePrivateProfileStruct(TEXT("Query"), TEXT("GetResults"),     &fGetResults,     SIZEOF(fGetResults),     INI_FILENAME);

    WritePrivateProfileString(TEXT("Query"), TEXT("ScopeLocn"), szScopeLocn, INI_FILENAME);
    WritePrivateProfileString(TEXT("Query"), TEXT("SaveLocn"),  szSaveLocn,  INI_FILENAME);

    // Fix the structures to reflect these options

    if ( FAILED(CoCreateInstance(CLSID_CommonQuery, NULL, CLSCTX_INPROC_SERVER, IID_ICommonQuery, (LPVOID*)&pCommonQuery)) )
        return;

    dqip.cbStruct = SIZEOF(dqip);
    dqip.dwFlags = 0;
    dqip.pDefaultScope = NULL;
    
    oqw.cbStruct = SIZEOF(oqw);
    oqw.dwFlags = 0;
    oqw.clsidHandler = CLSID_DsQuery;
    oqw.pHandlerParameters = &dqip;
    oqw.clsidDefaultForm = CLSID_DsFindObjects;

    if ( fGetResults )
        oqw.dwFlags |= OQWF_OKCANCEL;

    if ( fSingleSelect )
        oqw.dwFlags |= OQWF_SINGLESELECT;

    if ( fDefaultForm )
    {
        oqw.dwFlags |= OQWF_DEFAULTFORM;
        oqw.clsidDefaultForm = *forms[iDefaultForm].pFormCLSID;
    }

    if ( fDefaultScope )
        dqip.pDefaultScope = szScopeLocn;

    if ( fNoSave )
        dqip.dwFlags |= DSQPF_NOSAVE;

    if ( fSaveWhere )
    {
        dqip.dwFlags |= DSQPF_SAVELOCATION;
        dqip.pDefaultSaveLocation = szSaveLocn;
    }

    if ( fShowHidden )
        dqip.dwFlags |= DSQPF_SHOWHIDDENOBJECTS;
    
    if ( fAdminBits )
        dqip.dwFlags |= DSQPF_ENABLEADMINFEATURES|DSQPF_ENABLEADVANCEDFEATURES;

    if ( fRemoveScopes )
        oqw.dwFlags |= OQWF_REMOVESCOPES;

    if ( fRemoveForms )
        oqw.dwFlags |= OQWF_REMOVEFORMS;

    if ( fIssueOnOpen )
        oqw.dwFlags |= OQWF_ISSUEONOPEN;

    if ( fShowOptional )
        oqw.dwFlags |= OQWF_SHOWOPTIONAL;

    if ( fHideSearchPane )
	    oqw.dwFlags |= OQWF_HIDESEARCHPANE;
   
    if ( fPersistOnOK )
    {
        pPersistQuery = new CPersistQuery(GetDlgItem(hDlg, IDC_PERSISTDATA));

        if ( pPersistQuery )
        {
            oqw.dwFlags |= OQWF_SAVEQUERYONOK;
            oqw.pPersistQuery = pPersistQuery;
        }
    }

    if ( fHideMenus )
        oqw.dwFlags |= OQWF_HIDEMENUS;

    if ( fHideSearchUI )
        oqw.dwFlags |= OQWF_HIDESEARCHUI;

    // Now display the dialog, and if we succeeded and get an IDataObject then
    // slurp the results into our list view.

    hwndResults = GetDlgItem(hDlg, IDC_RESULTS);
    ListView_DeleteAllItems(hwndResults);

    SetDlgItemText(hDlg, IDC_CLSIDNAMESPACE, TEXT(""));
    SetDlgItemText(hDlg, IDC_ITEMCOUNT, TEXT(""));
    SetDlgItemText(hDlg, IDC_LDAPFILTER, TEXT(""));

    hr = pCommonQuery->OpenQueryWindow(hDlg, &oqw, &pDataObject);

    if ( SUCCEEDED(hr) && pDataObject )
    {
        // get the DSOBJECTNAMES and fill the list view accordingly

        if ( !g_cfDsObjectNames )
            g_cfDsObjectNames = RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);

        fmte.cfFormat = g_cfDsObjectNames;  

        if ( SUCCEEDED(pDataObject->GetData(&fmte, &medium)) )
        {
            LPDSOBJECTNAMES pDsObjects = (LPDSOBJECTNAMES)medium.hGlobal;

            SHStringFromGUID(pDsObjects->clsidNamespace, szBuffer, ARRAYSIZE(szBuffer));
            SetDlgItemText(hDlg, IDC_CLSIDNAMESPACE, szBuffer);

            wsprintf(szBuffer, TEXT("%d"), pDsObjects->cItems);
            SetDlgItemText(hDlg, IDC_ITEMCOUNT, szBuffer);

            for ( i = 0 ; i != pDsObjects->cItems ; i++ )
            {
                lvi.mask = LVIF_TEXT;
                lvi.iItem = i;
                lvi.iSubItem = 0;
                lvi.pszText = (LPTSTR)ByteOffset(pDsObjects, pDsObjects->aObjects[i].offsetName);
                ListView_InsertItem(hwndResults, &lvi);

                ListView_SetItemText(hwndResults, i, 1, (LPTSTR)ByteOffset(pDsObjects, pDsObjects->aObjects[i].offsetClass));            

                wsprintf(szBuffer, TEXT("%08x"), pDsObjects->aObjects[i].dwFlags);
                ListView_SetItemText(hwndResults, i, 2, szBuffer);

                wsprintf(szBuffer, TEXT("%08x"), pDsObjects->aObjects[i].dwProviderFlags);
                ListView_SetItemText(hwndResults, i, 3, szBuffer);
            }

            ReleaseStgMedium(&medium);
        }

        // now get the DSQUERYPARAMS and lets get the filter string

        if ( !g_cfDsQueryParams )
            g_cfDsQueryParams = RegisterClipboardFormat(CFSTR_DSQUERYPARAMS);

        fmte.cfFormat = g_cfDsQueryParams;  

        if ( SUCCEEDED(pDataObject->GetData(&fmte, &medium)) )
        {
            LPDSQUERYPARAMS pDsQueryParams = (LPDSQUERYPARAMS)medium.hGlobal;
            LPWSTR pFilter = (LPTSTR)ByteOffset(pDsQueryParams, pDsQueryParams->offsetQuery);

            SetDlgItemText(hDlg, IDC_LDAPFILTER, pFilter);
        }

        pDataObject->Release();
    }

    pCommonQuery->Release();

    if ( pPersistQuery )
        pPersistQuery->Release();
}


/*-----------------------------------------------------------------------------
/ Query_BrowseForScope
/ --------------------
/   Query the DS and populate the specified list view with results we get back.
/
/ In:
/   hDlg -> parent of find dialog
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void Query_BrowseForScope(HWND hDlg)
{
    DSBROWSEINFO dsbi;
    TCHAR szBuffer[MAX_PATH];

    GetDlgItemText(hDlg, IDC_SCOPELOCN, szBuffer, ARRAYSIZE(szBuffer));        

    dsbi.cbStruct = SIZEOF(dsbi);
    dsbi.hwndOwner = hDlg;
    dsbi.pszCaption = TEXT("Browse for default scope");
    dsbi.pszTitle = TEXT("Select a location to start the query from");
    dsbi.pszRoot = NULL;
    dsbi.pszPath = szBuffer;
    dsbi.cchPath = ARRAYSIZE(szBuffer);
    dsbi.dwFlags = DSBI_ENTIREDIRECTORY;
    dsbi.pfnCallback = NULL;
    dsbi.lParam = (LPARAM)0;

    if ( lstrlen(szBuffer) )
        dsbi.dwFlags |= DSBI_EXPANDONOPEN;

    if ( IDOK == DsBrowseForContainer(&dsbi) )
        SetDlgItemText(hDlg, IDC_SCOPELOCN, szBuffer);        
}


/*-----------------------------------------------------------------------------
/ Query_BrowseForSaveLocation
/ ---------------------------
/   Browse for a new save location setting the IDC_SAVELOCN control
/   to contain the new string.
/
/ In:
/   hDlg -> parent of find dialog
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void Query_BrowseForSaveLocation(HWND hDlg)
{
    LPITEMIDLIST pidl;
    BROWSEINFO bi;
    TCHAR szBuffer[MAX_PATH];

    bi.hwndOwner = hDlg;
    bi.pidlRoot = NULL;
    bi.pszDisplayName = NULL;
    bi.lpszTitle = TEXT("Select location to save searches to by default.");
    bi.ulFlags = 0;
    bi.lpfn = NULL;
    bi.lParam = 0;
    bi.iImage = 0;

    pidl = SHBrowseForFolder(&bi);

    if ( pidl )
    {    
        if ( SHGetPathFromIDList(pidl, szBuffer) )
            SetDlgItemText(hDlg, IDC_SAVELOCN, szBuffer);

        ILFree(pidl);
    }
}


/*-----------------------------------------------------------------------------
/ Main function for the app - handle initalisation etc.
/----------------------------------------------------------------------------*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    TCHAR szBuffer[MAX_PATH];
    LPTSTR pDefaultScope = NULL;

    g_hInstance = hInstance;

    InitCommonControls();
    CoInitialize(NULL);    

    if ( lpCmdLine && *lpCmdLine )
    {
        MultiByteToWideChar(CP_ACP, 0, lpCmdLine, -1, szBuffer, ARRAYSIZE(szBuffer));
        pDefaultScope = szBuffer;
    }

    DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_SEARCHDS), NULL, Query_DlgProc, (LPARAM)pDefaultScope);

    CoUninitialize();

    return 0;
}


/*-----------------------------------------------------------------------------
/ Constructor / IUnknown methods
/----------------------------------------------------------------------------*/

CPersistQuery::CPersistQuery(HWND hwndView)
{
    m_cRefCount = 1;
    m_hwndView = hwndView;
}

//
// IUnknown methods
//

STDMETHODIMP CPersistQuery::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    if ( IsEqualIID(riid, IID_IPersistQuery) )
    {
        *ppvObject = (LPVOID)(IPersistQuery*)this;
        return S_OK;
    }

    return E_NOTIMPL;
}

STDMETHODIMP_(ULONG) CPersistQuery::AddRef()
{
    return ++m_cRefCount;
}

STDMETHODIMP_(ULONG) CPersistQuery::Release()
{
    if ( --m_cRefCount == 0 )
    {
        delete this;
        return 0;
    }

    return m_cRefCount;
}


/*-----------------------------------------------------------------------------
/ IPersist methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CPersistQuery::GetClassID(THIS_ CLSID* pClassID)
{
    return E_NOTIMPL;
}


/*-----------------------------------------------------------------------------
/ IPersistQuery methods
/----------------------------------------------------------------------------*/

VOID _AddResultToView(HWND hwndView, LPCTSTR pSection, LPCTSTR pTag, LPCTSTR pValue)
{
    LV_ITEM lvi;
    INT i;

    lvi.mask = LVIF_TEXT;
    lvi.iItem = 0x7fffffff;
    lvi.iSubItem = 0;
    lvi.pszText = (LPTSTR)pSection;
    i = ListView_InsertItem(hwndView, &lvi);

    if ( i >= 0 )
    {
        ListView_SetItemText(hwndView, i, 1, (LPTSTR)pTag);
        ListView_SetItemText(hwndView, i, 2, (LPTSTR)pValue);
    }
}

STDMETHODIMP CPersistQuery::WriteString(THIS_ LPCTSTR pSection, LPCTSTR pKey, LPCTSTR pValue)
{
    _AddResultToView(m_hwndView, pSection, pKey, pValue);
    return S_OK;
}

STDMETHODIMP CPersistQuery::ReadString(THIS_ LPCTSTR pSection, LPCTSTR pKey, LPTSTR pBuffer, INT cchBuffer)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPersistQuery::WriteInt(THIS_ LPCTSTR pSection, LPCTSTR pKey, INT value)
{
    TCHAR szBuffer[64];
    wsprintf(szBuffer, TEXT("%d"), value);
    _AddResultToView(m_hwndView, pSection, pKey, szBuffer);
    return S_OK;
}

STDMETHODIMP CPersistQuery::ReadInt(THIS_ LPCTSTR pSection, LPCTSTR pKey, LPINT pValue)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPersistQuery::WriteStruct(THIS_ LPCTSTR pSection, LPCTSTR pKey, LPVOID pStruct, DWORD cbStruct)
{
    TCHAR szBuffer[128];
    wsprintf(szBuffer, TEXT("Structure (%d byte in size)"), cbStruct);
    _AddResultToView(m_hwndView, pSection, pKey, szBuffer);
    return S_OK;
}

STDMETHODIMP CPersistQuery::ReadStruct(THIS_ LPCTSTR pSection, LPCTSTR pKey, LPVOID pStruct, DWORD cbStruct)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPersistQuery::Clear(THIS)
{
    ListView_DeleteAllItems(m_hwndView);
    return S_OK;
}
