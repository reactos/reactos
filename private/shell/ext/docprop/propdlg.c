
////////////////////////////////////////////////////////////////////////////////
//
// propdlg.c
//
// The Properties dialog for MS Office.
//
// Change history:
//
// Date         Who             What
// --------------------------------------------------------------------------
// 06/09/94     B. Wentz        Created file
// 01/16/95     martinth        Finished sticky dlg stuff.  Lemme just say that
//                              it's pretty lame.  We have to call ApplyStickyDlgCoor
//                              in the first WM_INITDIALOG, don't ask me why,
//                              but otherwise we have redraw problems.  Likewise,
//                              we have to call SetStickyDlgCoor in the first
//                              PSN_RESET/PSN_APPLY, I have no idea why, since
//                              the main dialog shouldn't have been deleted but
//                              it is.  Thus we have to add calls everywhere.
//                              Could it be that the tabs are getting deleted
//                              one by one and the dialog changes size?  Dunno.
//                              But this works, so change at your own risk!;-)
// 07/08/96     MikeHill        Ignore unsupported (non-UDTYPE) properties.
////////////////////////////////////////////////////////////////////////////////

#include "priv.h"
#pragma hdrstop

#include <winnls.h>
#include <prsht.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <shlapip.h>


#include "propdlg.h"
#include "strings.h"
#include "msohelp.h"

// Max size of time/date string
#define TIMEDATEMAX     256

// Check button actions
#define CLEAR   0
#define CHECKED 1
#define GREYED  2

// Number of property sheet pages
#define PAGESMAX        5


// Max size for "short" temp buffers
#define SHORTBUFMAX     128

// The pages
#ifdef _WIN2000_DOCPROP_
#define itabCUSTOM          0
#define itabFIRST           itabCUSTOM
#else //_WIN2000_DOCPROP_
#define itabGENERAL         0
#define itabSUMMARY         1
#define itabSTATISTICS      2
#define itabCONTENTS        3
#define itabCUSTOM          4
#define itabFIRST           itabSUMMARY
#endif //_WIN2000_DOCPROP_

// Defines for printing file sizes
#define DELIMITER   TEXT(',')

#define iszBYTES               0
#define iszORDERKB             1
#define iszORDERMB             2
#define iszORDERGB             3
#define iszORDERTB             4

static TCHAR rgszOrders[iszORDERTB+1][SHORTBUFMAX];
//  "bytes",        // iszBYTES
//  "KB",           // iszORDERKB
//  "MB",           // iszORDERMB
//  "GB",           // iszORDERGB
//  "TB"            // iszORDERTB

// note that szBYTES is defined above...
#define iszPAGES         1
#define iszPARA          2
#define iszLINES         3
#define iszWORDS         4
#define iszCHARS         5
#define iszSLIDES        6
#define iszNOTES         7
#define iszHIDDENSLIDES  8
#define iszMMCLIPS       9
#define iszFORMAT        10

// Strings for the statistics listbox
static TCHAR rgszStats[iszFORMAT+1][SHORTBUFMAX];
//  "Bytes:",             // iszBYTES
//  "Pages:",             // iszPAGES
//  "Paragraphs:",        // iszPARA
//  "Lines:",             // iszLINES
//  "Words:",             // iszWORDS
//  "Characters:",        // iszCHARS
//  "Slides:",            // iszSLIDES
//  "Notes:",             // iszNOTES
//  "Hidden Slides:",     // iszHIDDENSLIDES
//  "Multimedia Clips:",  // iszMMCLIPS
//  "Presentation Format:"// iszFORMAT

#define BASE10          10


// Number of pre-defined custom names
#define NUM_BUILTIN_CUSTOM_NAMES 27

#define iszTEXT         0
#define iszDATE         1
#define iszNUM          2
#define iszBOOL         3
#define iszUNKNOWN      4

// Strings for the types of user-defined properties
static TCHAR rgszTypes[iszUNKNOWN+1][SHORTBUFMAX];
//  "Text",               // iszTEXT
//  "Date",               // iszDATE
//  "Number",             // iszNUM
//  "Yes or No",          // iszBOOL
//  "Unknown"             // iszUNKNOWN

#define iszNAME         0
#define iszVAL          1
#define iszTYPE         2

// Strings for the column headings for the statistics tab
static TCHAR rgszStatHeadings[iszVAL+1][SHORTBUFMAX];
//  "Statistic Name",     // iszNAME
//  "Value"               // iszVAL

// Strings for the column headings for custom tab
static TCHAR rgszHeadings[iszTYPE+1][SHORTBUFMAX];
//  "Property Name",      // iszNAME
//  "Value",              // iszVAL
//  "Type"                // iszTYPE

#define iszTRUE  0
#define iszFALSE 1

// Strings for Booleans
static TCHAR rgszBOOL[iszFALSE+1][SHORTBUFMAX];
//  "Yes",       // iszTRUE
//  "No"         // iszFALSE

#define iszADD          0
#define iszMODIFY       1

// Strings for the Add button
static TCHAR rgszAdd[iszMODIFY+1][SHORTBUFMAX];
//  "Add",        // iszADD
//  "Modify"      // iszMODIFY

#define iszVALUE     0
#define iszSOURCE    1

// Strings for the source/value caption
static TCHAR rgszValue[iszSOURCE+1][SHORTBUFMAX];
//  "Value:",     // iszVALUE
//  "Source:"     // iszSOURCE

// Date formatting codes
#define MMDDYY  TEXT('0')
#define DDMMYY  TEXT('1')
#define YYMMDD  TEXT('2')
#define OLEEPOCH 1900
#define SYSEPOCH 1601
#define ONECENTURY 100
#define YEARINCENTURY(year)     ((year) % ONECENTURY)
#define CENTURYFROMYEAR(year)   ((year) - YEARINCENTURY(year))

//
// Global data, to be deleted when FShowOfficePropDlg exits
//
static LPTSTR glpstzName;
static LPTSTR glpstzValue;
static int giLinkIcon;
static int giInvLinkIcon;
static int giBlankIcon;
static HBRUSH hBrushPropDlg = NULL;

const TCHAR g_szHelpFile[] = TEXT("windows.hlp");

//
// Internal prototypes
//
INT_PTR CALLBACK FGeneralDlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK FSummaryDlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK FStatisticsDlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK FCustomDlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK FContentsDlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK FPropHeaderDlgProc (HWND hwnd, UINT message, LONG lParam);
//static int  CALLBACK ListViewCompareFunc(LPARAM, LPARAM, LPARAM);

void PASCAL SetEditValLpsz (LPPROPVARIANT lppropvar, HWND hdlg, DWORD dwID );
BOOL PASCAL GetEditValLpsz (LPPROPVARIANT lppropvar, HWND hDlg, DWORD dwId);
BOOL PASCAL FAllocAndGetValLpstz (HWND hDlg, DWORD dwId, LPTSTR *lplpstz);
BOOL PASCAL FAllocString (LPTSTR *lplpstz, DWORD cb);
void PASCAL ClearEditControl (HWND hDlg, DWORD dwId);

UDTYPES PASCAL UdtypesGetNumberType (LPTSTR lpstz, NUM *lpnumval,
                                     BOOL (*lpfnFSzToNum)(NUM *, LPTSTR));

void PASCAL PrintTimeInDlg (HWND hDlg, DWORD dwId, FILETIME *pft);

void PASCAL PrintEditTimeInDlg (HWND hDlg, FILETIME *pft);
void PASCAL AddItemToListView (HWND hWnd, DWORD_PTR dw, const TCHAR *lpsz, BOOL fString);

void PASCAL PopulateUDListView (HWND hWnd, LPUDOBJ lpUDObj);
void PASCAL AddUDPropToListView (LPUDOBJ lpUDObj, HWND hWnd, LPTSTR lpszName, LPPROPVARIANT lppropvar, int iItem,
                                 BOOL fLink, BOOL fLinkInvalid, BOOL fMakeVisible);
VOID PASCAL InitListView (HWND hDlg, int irgLast, TCHAR rgsz[][SHORTBUFMAX], BOOL fImageList);

WORD PASCAL WUdtypeToSz (LPPROPVARIANT lppropvar, LPTSTR sz, DWORD cchMax,
                         BOOL (*lpfnFNumToSz)(NUM *, LPTSTR, DWORD));
BOOL PASCAL FSwapControls (HWND hWndVal, HWND hWndLinkVal, HWND hWndBoolTrue, HWND hWndBoolFalse, HWND hWndGroup, HWND hWndType, HWND hWndValText, BOOL fLink, BOOL fBool);
VOID PASCAL PopulateControls (LPUDOBJ lpUDObj, LPTSTR szName, DWORD cLinks, DWQUERYLD lpfnDwQueryLinkData, HWND hDlg,
                              HWND hWndName, HWND hWndVal, HWND hWndValText, HWND hWndLink, HWND hWndLinkVal, HWND hWndType,
                              HWND hWndBoolTrue, HWND hWndBoolFalse, HWND hWndGroup, HWND hWndAdd, HWND hWndDelete, BOOL *pfLink, BOOL *pfAdd);
BOOL PASCAL FSetupAddButton (DWORD iszType, BOOL fLink, BOOL *pfAdd, HWND hWndAdd, HWND hWndVal, HWND hWndName, HWND hDlg);
BOOL PASCAL FCreateListOfLinks (DWORD cLinks, DWQUERYLD lpfnDwQueryLinkData, HWND hWndLinkVal);
BOOL PASCAL FSetTypeControl (UDTYPES udtype, HWND hWndType);
void PASCAL DeleteItem (LPUDOBJ lpUDObj, HWND hWndLV, int iItem, TCHAR sz[]);
void PASCAL ResetTypeControl (HWND hDlg, DWORD dwId, DWORD *piszType);
BOOL PASCAL FDisplayConversionWarning (HWND hDlg);
BOOL PASCAL FLoadTextStrings (void);
BOOL FGetCustomPropFromDlg(LPALLOBJS lpallobjs, HWND hDlg);
VOID SetCustomDlgDefButton(HWND hDlg, int IDNew);
INT PASCAL ISavePropDlgChanges(LPALLOBJS, HWND, HWND);

/* WinHelp stuff. */
static const DWORD rgIdhGeneral[] =
{
    IDD_ITEMICON,     IDH_GENERAL_ICON,
        IDD_NAME,         IDH_GENERAL_NAME_BY_ICON,
        IDD_FILETYPE,     IDH_GENERAL_FILETYPE,
        IDD_FILETYPE_LABEL,     IDH_GENERAL_FILETYPE,
        IDD_LOCATION,     IDH_GENERAL_LOCATION,
        IDD_LOCATION_LABEL,     IDH_GENERAL_LOCATION,
        IDD_FILESIZE,     IDH_GENERAL_FILESIZE,
        IDD_FILESIZE_LABEL,     IDH_GENERAL_FILESIZE,
        IDD_FILENAME,     IDH_GENERAL_MSDOSNAME,
        IDD_FILENAME_LABEL,     IDH_GENERAL_MSDOSNAME,
        IDD_CREATED,      IDH_GENERAL_CREATED,
        IDD_CREATED_LABEL,      IDH_GENERAL_CREATED,
        IDD_LASTMODIFIED, IDH_GENERAL_MODIFIED,
        IDD_LASTMODIFIED_LABEL, IDH_GENERAL_MODIFIED,
        IDD_LASTACCESSED, IDH_GENERAL_ACCESSED,
        IDD_LASTACCESSED_LABEL, IDH_GENERAL_ACCESSED,
        IDD_ATTRIBUTES_LABEL, IDH_GENERAL_ATTRIBUTES,
        IDD_READONLY,     IDH_GENERAL_READONLY,
        IDD_HIDDEN,       IDH_GENERAL_HIDDEN,
        IDD_ARCHIVE,      IDH_GENERAL_ARCHIVE,
        IDD_SYSTEM,       IDH_GENERAL_SYSTEM
};

static const DWORD rgIdhSummary[] =
{
    IDD_SUMMARY_TITLE,    IDH_SUMMARY_TITLE,
        IDD_SUMMARY_TITLE_LABEL,    IDH_SUMMARY_TITLE,
        IDD_SUMMARY_SUBJECT,  IDH_SUMMARY_SUBJECT,
        IDD_SUMMARY_SUBJECT_LABEL,  IDH_SUMMARY_SUBJECT,
        IDD_SUMMARY_AUTHOR,   IDH_SUMMARY_AUTHOR,
        IDD_SUMMARY_AUTHOR_LABEL,   IDH_SUMMARY_AUTHOR,
        IDD_SUMMARY_MANAGER,  IDH_SUMMARY_MANAGER,
        IDD_SUMMARY_MANAGER_LABEL,  IDH_SUMMARY_MANAGER,
        IDD_SUMMARY_COMPANY,  IDH_SUMMARY_COMPANY,
        IDD_SUMMARY_COMPANY_LABEL,  IDH_SUMMARY_COMPANY,
        IDD_SUMMARY_CATEGORY, IDH_SUMMARY_CATEGORY,
        IDD_SUMMARY_CATEGORY_LABEL, IDH_SUMMARY_CATEGORY,
        IDD_SUMMARY_KEYWORDS, IDH_SUMMARY_KEYWORDS,
        IDD_SUMMARY_KEYWORDS_LABEL, IDH_SUMMARY_KEYWORDS,
        IDD_SUMMARY_COMMENTS, IDH_SUMMARY_COMMENTS,
        IDD_SUMMARY_COMMENTS_LABEL, IDH_SUMMARY_COMMENTS,
        IDD_SUMMARY_TEMPLATE, IDH_SUMMARY_TEMPLATE,
        IDD_SUMMARY_TEMPLATETEXT, IDH_SUMMARY_TEMPLATE,
        IDD_SUMMARY_SAVEPREVIEW, IDH_SUMMARY_SAVEPREVIEW
};

static const DWORD rgIdhStatistics[] =
{
    IDD_STATISTICS_CREATED,    IDH_STATISTICS_CREATED,
        IDD_STATISTICS_CREATED_LABEL,    IDH_STATISTICS_CREATED,
        IDD_STATISTICS_CHANGED,    IDH_STATISTICS_MODIFIED,
        IDD_STATISTICS_CHANGED_LABEL,    IDH_STATISTICS_MODIFIED,
        IDD_STATISTICS_ACCESSED,   IDH_STATISTICS_ACCESSED,
        IDD_STATISTICS_ACCESSED_LABEL,   IDH_STATISTICS_ACCESSED,
        IDD_STATISTICS_LASTPRINT,  IDH_STATISTICS_LASTPRINT,
        IDD_STATISTICS_LASTPRINT_LABEL,  IDH_STATISTICS_LASTPRINT,
        IDD_STATISTICS_LASTSAVEBY, IDH_STATISTICS_LASTSAVEBY,
        IDD_STATISTICS_LASTSAVEBY_LABEL, IDH_STATISTICS_LASTSAVEBY,
        IDD_STATISTICS_REVISION,   IDH_STATISTICS_REVISION,
        IDD_STATISTICS_REVISION_LABEL,   IDH_STATISTICS_REVISION,
        IDD_STATISTICS_TOTALEDIT,  IDH_STATISTICS_TOTALEDIT,
        IDD_STATISTICS_TOTALEDIT_LABEL,  IDH_STATISTICS_TOTALEDIT,
        IDD_STATISTICS_LVLABEL,   IDH_STATISTICS_LISTVIEW,
        IDD_STATISTICS_LISTVIEW,   IDH_STATISTICS_LISTVIEW
};

static const DWORD rgIdhContents[] =
{
    IDD_CONTENTS_LISTBOX_LABEL, IDH_CONTENTS_LISTBOX,
        IDD_CONTENTS_LISTBOX, IDH_CONTENTS_LISTBOX
};

static const DWORD rgIdhCustom[] =
{
    IDD_CUSTOM_NAME,      IDH_CUSTOM_NAME,
        IDD_CUSTOM_NAME_LABEL,      IDH_CUSTOM_NAME,
        IDD_CUSTOM_TYPE,      IDH_CUSTOM_TYPE,
        IDD_CUSTOM_TYPE_LABEL,      IDH_CUSTOM_TYPE,
        IDD_CUSTOM_VALUE,     IDH_CUSTOM_VALUE,
        IDD_CUSTOM_VALUETEXT,     IDH_CUSTOM_VALUE,
        IDD_CUSTOM_LINKVALUE, IDH_CUSTOM_LINKVALUE,
        IDD_CUSTOM_BOOLTRUE,  IDH_CUSTOM_BOOLYES,
        IDD_CUSTOM_BOOLFALSE, IDH_CUSTOM_BOOLYES,
        IDD_CUSTOM_ADD,       IDH_CUSTOM_ADDBUTTON,
        IDD_CUSTOM_DELETE,    IDH_CUSTOM_DELETEBUTTON,
        IDD_CUSTOM_LINK,      IDH_CUSTOM_LINKCHECK,
        IDD_CUSTOM_LISTVIEW,  IDH_CUSTOM_LISTVIEW,
        IDD_CUSTOM_LISTVIEW_LABEL,  IDH_CUSTOM_LISTVIEW
};

void FOfficeInitPropInfo(PROPSHEETPAGE * lpPsp, DWORD dwFlags, LPARAM lParam, LPFNPSPCALLBACK pfnCallback)
{
#ifndef _WIN2000_DOCPROP_
    lpPsp[itabSUMMARY-itabFIRST].dwSize = sizeof(PROPSHEETPAGE);
    lpPsp[itabSUMMARY-itabFIRST].dwFlags = dwFlags;
    lpPsp[itabSUMMARY-itabFIRST].hInstance = g_hmodThisDll;
    lpPsp[itabSUMMARY-itabFIRST].pszTemplate = MAKEINTRESOURCE (IDD_SUMMARY);
    lpPsp[itabSUMMARY-itabFIRST].pszIcon = NULL;
    lpPsp[itabSUMMARY-itabFIRST].pszTitle = NULL;
    lpPsp[itabSUMMARY-itabFIRST].pfnDlgProc = FSummaryDlgProc;
    lpPsp[itabSUMMARY-itabFIRST].pfnCallback = pfnCallback;
    lpPsp[itabSUMMARY-itabFIRST].pcRefParent = NULL;
    lpPsp[itabSUMMARY-itabFIRST].lParam = lParam;
    
    lpPsp[itabSTATISTICS-itabFIRST].dwSize = sizeof(PROPSHEETPAGE);
    lpPsp[itabSTATISTICS-itabFIRST].dwFlags = dwFlags;
    lpPsp[itabSTATISTICS-itabFIRST].hInstance = g_hmodThisDll;
    lpPsp[itabSTATISTICS-itabFIRST].pszTemplate = MAKEINTRESOURCE (IDD_STATISTICS);
    lpPsp[itabSTATISTICS-itabFIRST].pszIcon = NULL;
    lpPsp[itabSTATISTICS-itabFIRST].pszTitle = NULL;
    lpPsp[itabSTATISTICS-itabFIRST].pfnDlgProc = FStatisticsDlgProc;
    lpPsp[itabSTATISTICS-itabFIRST].pfnCallback = pfnCallback;
    lpPsp[itabSTATISTICS-itabFIRST].pcRefParent = NULL;
    lpPsp[itabSTATISTICS-itabFIRST].lParam = lParam;
    
    lpPsp[itabCONTENTS-itabFIRST].dwSize = sizeof(PROPSHEETPAGE);
    lpPsp[itabCONTENTS-itabFIRST].dwFlags = dwFlags;
    lpPsp[itabCONTENTS-itabFIRST].hInstance = g_hmodThisDll;
    lpPsp[itabCONTENTS-itabFIRST].pszTemplate = MAKEINTRESOURCE (IDD_CONTENTS);
    lpPsp[itabCONTENTS-itabFIRST].pszIcon = NULL;
    lpPsp[itabCONTENTS-itabFIRST].pszTitle = NULL;
    lpPsp[itabCONTENTS-itabFIRST].pfnDlgProc = FContentsDlgProc;
    lpPsp[itabCONTENTS-itabFIRST].pfnCallback = pfnCallback;
    lpPsp[itabCONTENTS-itabFIRST].pcRefParent = NULL;
    lpPsp[itabCONTENTS-itabFIRST].lParam = lParam;
#endif // _WIN2000_DOCPROP_

    lpPsp[itabCUSTOM-itabFIRST].dwSize = sizeof(PROPSHEETPAGE);
    lpPsp[itabCUSTOM-itabFIRST].dwFlags = dwFlags;
    lpPsp[itabCUSTOM-itabFIRST].hInstance = g_hmodThisDll;
    lpPsp[itabCUSTOM-itabFIRST].pszTemplate = MAKEINTRESOURCE (IDD_CUSTOM);
    lpPsp[itabCUSTOM-itabFIRST].pszIcon = NULL;
    lpPsp[itabCUSTOM-itabFIRST].pszTitle = NULL;
    lpPsp[itabCUSTOM-itabFIRST].pfnDlgProc = FCustomDlgProc;
    lpPsp[itabCUSTOM-itabFIRST].pfnCallback = pfnCallback;
    lpPsp[itabCUSTOM-itabFIRST].pcRefParent = NULL;
    lpPsp[itabCUSTOM-itabFIRST].lParam = lParam;
    
}

////////////////////////////////////////////////////////////////////////////////
//
// Attach
//
// Purpose:
//  Assigns HPROPSHEETPAGE to appropriate data block member.
//
////////////////////////////////////////////////////////////////////////////////
BOOL FAttach( LPALLOBJS lpallobjs, PROPSHEETPAGE* ppsp, HPROPSHEETPAGE hPage )
{
    #define ASSIGN_PAGE_HANDLE( pfn, phpage ) \
        if( ppsp->pfnDlgProc == pfn ) { *(phpage) = hPage ; return TRUE ; }

#if _WIN2000_DOCPROP_
    ASSIGN_PAGE_HANDLE( FCustomDlgProc,     &lpallobjs->lpUDObj->m_hPage );
#else  // _WIN2000_DOCPROP_
    ASSIGN_PAGE_HANDLE( FSummaryDlgProc,    &lpallobjs->lpSIObj->m_hPage );
    ASSIGN_PAGE_HANDLE( FStatisticsDlgProc, &lpallobjs->lpDSIObj->m_hPage );
    ASSIGN_PAGE_HANDLE( FCustomDlgProc,     &lpallobjs->lpUDObj->m_hPage );
#endif // _WIN2000_DOCPROP_

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
//
// PropPageInit
//
// Purpose:
//  Keep track which pages have been init, such that we can know when we
//  can do the apply.
//
////////////////////////////////////////////////////////////////////////////////
void PropPageInit(LPALLOBJS lpallobjs, int iPage)
{
    if (iPage > lpallobjs->iMaxPageInit)
        lpallobjs->iMaxPageInit = iPage;
}

////////////////////////////////////////////////////////////////////////////////
//
// ApplyChangesBackToFile
//
// Purpose:
//     See if this is now the time to apply the changes back to the file
//
////////////////////////////////////////////////////////////////////////////////
BOOL ApplyChangesBackToFile(
    HWND hDlg, 
    BOOL bFinalEdit /* user clicked OK rather than Apply*/, 
    LPALLOBJS lpallobjs, 
    int iPage)
{
    HRESULT     hres;
    BOOL        fOK = FALSE;
    LPSTORAGE   lpStg;
    WCHAR       wszPath[ MAX_PATH ];
    
    if (iPage != lpallobjs->iMaxPageInit)
        return TRUE;    // no errors
    
    
#ifdef UNICODE
    lstrcpyn(wszPath, lpallobjs->szPath, ARRAYSIZE(wszPath));
#else
    MultiByteToWideChar(CP_ACP, 0, lpallobjs->szPath, -1, wszPath, ARRAYSIZE(wszPath));
#endif

#ifdef WINNT
    hres = StgOpenStorageEx(wszPath,STGM_READWRITE|STGM_SHARE_EXCLUSIVE,STGFMT_ANY,0,NULL,NULL,
                            &IID_IStorage, (void**)&lpStg );
#else
    hres = StgOpenStorage(wszPath, NULL, STGM_READWRITE|STGM_SHARE_EXCLUSIVE, NULL, 0L, &lpStg );
#endif

    if (SUCCEEDED(hres) && lpStg)
    {
        MESSAGE(TEXT("Now trying to save out properties"));

        fOK = (BOOL)DwOfficeSaveProperties( lpStg,
            lpallobjs->lpSIObj,
            lpallobjs->lpDSIObj,
            lpallobjs->lpUDObj,
            0,          // Flags
            STGM_READWRITE | STGM_SHARE_EXCLUSIVE
            );
        
        // Release the Storage (we don't need to commit it;
        // it's in dicrect-mode).
        
        lpStg->lpVtbl->Release (lpStg);
        lpStg= NULL;
        
        
        //
        // if we did properly save out the properties, than we should
        // clear the we have changed things flag...
        //
        if (fOK)
        {
            lpallobjs->fPropDlgChanged = FALSE;
            lpallobjs->fPropDlgPrompted = FALSE;
        }
    }   // if (SUCCEEDED(hres) && lpStorage)
    
    if (!fOK)
    {
        UINT nMsgFlags = bFinalEdit ? MB_OKCANCEL /* give option to not dismiss page*/ : MB_OK;
        
        if (ShellMessageBox(g_hmodThisDll, GetParent(hDlg),
            MAKEINTRESOURCE(idsErrorOnSave), NULL,
            nMsgFlags | MB_ICONHAND, PathFindFileName(lpallobjs->szPath)) == IDOK)
        {
            fOK = TRUE;
        }
        PropSheet_UnChanged(GetParent(hDlg), hDlg);
    }
   
    return fOK;
}   // ApplyChangesBackToFile


////////////////////////////////////////////////////////////////////////////////
//
// FSummaryDlgProc
//
// Purpose:
//  Summary window dialog handler.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef _WIN2000_DOCPROP_
INT_PTR CALLBACK  FSummaryDlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    DWORD dwMask;
    DWORD dwT;
    LPALLOBJS lpallobjs = (LPALLOBJS)GetWindowLongPtr(hDlg, DWLP_USER);
    
    switch (message)
    {
    case WM_INITDIALOG :
        {
            PROPSHEETPAGE *ppspDlg = (PROPSHEETPAGE *) lParam;
            LPSIOBJ lpSIObj;    // All SumInfo data
            LPDSIOBJ lpDSIObj;  // All DocSumInfo (first section)
            LPPROPVARIANT rgpropvarSumInfo, rgpropvarDocSumInfo;
            
            lpallobjs = (LPALLOBJS)ppspDlg->lParam;
            
            PropPageInit(lpallobjs, itabSUMMARY);
            
            SetWindowLongPtr(hDlg, DWLP_USER, ppspDlg->lParam);
            
            lpSIObj = lpallobjs->lpSIObj;
            lpDSIObj = lpallobjs->lpDSIObj;
            dwMask = lpallobjs->dwMask;
            
            //
            // Validate our data.
            //
            if ((lpSIObj == NULL) ||
                (lpSIObj->m_lpData == NULL) ||
                (lpDSIObj == NULL) ||
                (lpDSIObj->m_lpData == NULL))
            {
                AssertSz (0, TEXT("Bad object data in Summary dlg"));
                return FALSE;
            }
            
            //
            // Since we'll be referencing the PropVariants a lot, get
            // a pointer directly to them.
            //
            rgpropvarSumInfo = GETSINFO(lpSIObj)->rgpropvar;
            rgpropvarDocSumInfo = GETSINFO(lpDSIObj)->rgpropvar;
            
            //
            // Copy the properties from the PropVariants to the Edit controls.
            //
            SetEditValLpsz (&rgpropvarSumInfo[ PVSI_TITLE ], hDlg, IDD_SUMMARY_TITLE);
            SetEditValLpsz (&rgpropvarSumInfo[ PVSI_SUBJECT ], hDlg, IDD_SUMMARY_SUBJECT);
            SetEditValLpsz (&rgpropvarSumInfo[ PVSI_AUTHOR ], hDlg, IDD_SUMMARY_AUTHOR);
            SetEditValLpsz (&rgpropvarSumInfo[ PVSI_COMMENTS ], hDlg, IDD_SUMMARY_COMMENTS);
            SetEditValLpsz (&rgpropvarSumInfo[ PVSI_KEYWORDS ], hDlg, IDD_SUMMARY_KEYWORDS);
            
            //
            // Show the name of the template, if it exists, otherwise
            // don't even show the field.
            //
            if (rgpropvarSumInfo[ PVSI_TEMPLATE ].vt == VT_LPTSTR)
            {
                SetEditValLpsz (&rgpropvarSumInfo[ PVSI_TEMPLATE ], hDlg, IDD_SUMMARY_TEMPLATE);
            }
            else
            {
                EnableWindow (GetDlgItem (hDlg, IDD_SUMMARY_TEMPLATETEXT), SW_HIDE);
            }
            
            SetEditValLpsz (&rgpropvarDocSumInfo[ PVDSI_CATEGORY ], hDlg, IDD_SUMMARY_CATEGORY);
            SetEditValLpsz (&rgpropvarDocSumInfo[ PVDSI_COMPANY ], hDlg, IDD_SUMMARY_COMPANY);
            SetEditValLpsz (&rgpropvarDocSumInfo[ PVDSI_MANAGER ], hDlg, IDD_SUMMARY_MANAGER);
            SetEditValLpsz (&rgpropvarSumInfo[ PVSI_COMMENTS ], hDlg, IDD_SUMMARY_COMMENTS);
            
            if (dwMask & OSPD_NOSAVEPREVIEW)
            {
                ShowWindow(GetDlgItem(hDlg, IDD_SUMMARY_SAVEPREVIEW), SW_HIDE);
            }
            else
            {
                if (!GETSINFO(lpSIObj)->fSaveSINail)
                    GETSINFO(lpSIObj)->fSaveSINail = (dwMask & OSPD_SAVEPREVIEW_ON);
                
#ifdef SHELL
                if (!GETSINFO(lpSIObj)->fSaveSINail)
                    
                    EnableWindow (GetDlgItem (hDlg, IDD_SUMMARY_SAVEPREVIEW), FALSE);
                
                else
#endif
                    SendDlgItemMessage(hDlg, IDD_SUMMARY_SAVEPREVIEW, BM_SETCHECK,
                    (WPARAM) GETSINFO(lpSIObj)->fSaveSINail,0);
            }
            
            
            lpallobjs->fPropDlgChanged = FALSE;    // It might have been set by the EN_UPDATE check, so let's reset it here
        }
        return TRUE;
        
    case WM_CTLCOLOREDIT    :
        if ((HWND)lParam != GetDlgItem(hDlg, IDD_SUMMARY_TEMPLATE)) // only change color for the
            break;                                                   // the template
    case WM_CTLCOLORDLG     :
    case WM_CTLCOLORSTATIC  :
        if (hBrushPropDlg == NULL)
            break;
        DeleteObject(hBrushPropDlg);
        if ((hBrushPropDlg = CreateSolidBrush(GetSysColor(COLOR_BTNFACE))) == NULL)
            break;
        SetBkColor ((HDC) wParam, GetSysColor (COLOR_BTNFACE));
        SetTextColor((HDC) wParam, GetSysColor(COLOR_WINDOWTEXT));
        return (INT_PTR) hBrushPropDlg;
        
    case WM_COMMAND:
        
        {
            BOOL fNailChanged = FALSE;
            
            AssertSz( (lpallobjs != NULL && lpallobjs->lpSIObj != NULL),
                TEXT("Invalid global structure in WM_COMMAND" ));
            
            if ((HIWORD (wParam) == BN_CLICKED) && (LOWORD(wParam) == IDD_SUMMARY_SAVEPREVIEW))
            {
                GETSINFO(lpallobjs->lpSIObj)->fSaveSINail = !GETSINFO(lpallobjs->lpSIObj)->fSaveSINail;
                OfficeDirtySIObj (lpallobjs->lpSIObj, TRUE);
                fNailChanged = TRUE;
            }
            
            if (HIWORD (wParam) == EN_UPDATE
                ||
                fNailChanged)
            {
                lpallobjs->fPropDlgChanged = TRUE;
                PropSheet_Changed(GetParent(hDlg), hDlg);
            }
            
            break;
        }
        
    case WM_NOTIFY :
        
        switch (((NMHDR FAR *) lParam)->code)
        {
        case PSN_APPLY :
            
            //
            // Save what user entered.
            //
            
            {
                LPSIOBJ  lpSIObj;       // All SumInfo data
                LPDSIOBJ lpDSIObj;      // All DocSumInfo data
                
                BOOL fSIChanged = FALSE;    // The SumInfo properties have changed
                BOOL fDSIChanged = FALSE;   // The DocSumInfo properties have changed
                
                // SumInfo and DocSumInfo properties.
                LPPROPVARIANT rgpropvarSumInfo, rgpropvarDocSumInfo;
                
                lpSIObj = lpallobjs->lpSIObj;
                rgpropvarSumInfo = GETSINFO(lpSIObj)->rgpropvar;
                
                lpDSIObj = lpallobjs->lpDSIObj;
                rgpropvarDocSumInfo = GETDSINFO(lpDSIObj)->rgpropvar;
                
                fSIChanged |= GetEditValLpsz (&rgpropvarSumInfo[PVSI_TITLE], hDlg, IDD_SUMMARY_TITLE);
                fSIChanged |= GetEditValLpsz (&rgpropvarSumInfo[PVSI_SUBJECT], hDlg, IDD_SUMMARY_SUBJECT);
                fSIChanged |= GetEditValLpsz (&rgpropvarSumInfo[PVSI_AUTHOR], hDlg, IDD_SUMMARY_AUTHOR);
                fSIChanged |= GetEditValLpsz (&rgpropvarSumInfo[PVSI_COMMENTS], hDlg, IDD_SUMMARY_COMMENTS);
                fSIChanged |= GetEditValLpsz (&rgpropvarSumInfo[PVSI_KEYWORDS], hDlg, IDD_SUMMARY_KEYWORDS);
                fSIChanged |= GetEditValLpsz (&rgpropvarSumInfo[PVSI_TEMPLATE], hDlg, IDD_SUMMARY_TEMPLATE);
                fDSIChanged |= GetEditValLpsz (&rgpropvarDocSumInfo[PVDSI_CATEGORY], hDlg, IDD_SUMMARY_CATEGORY);
                fDSIChanged |= GetEditValLpsz (&rgpropvarDocSumInfo[PVDSI_MANAGER], hDlg, IDD_SUMMARY_MANAGER);
                fDSIChanged |= GetEditValLpsz (&rgpropvarDocSumInfo[PVDSI_COMPANY], hDlg, IDD_SUMMARY_COMPANY);
                
                if (fSIChanged)
                {
                    OfficeDirtySIObj (lpSIObj, TRUE);
                }
                
                if (fDSIChanged)
                {
                    OfficeDirtyDSIObj (lpDSIObj, TRUE);
                }
                
                MESSAGE (TEXT("PSN_APPLY - Summary Page"));
                
                if (FSumInfoShouldSave (lpSIObj)
                    || FDocSumShouldSave (lpDSIObj)
                    || lpallobjs->fPropDlgChanged )
                {
                    ApplyChangesBackToFile(hDlg, (BOOL)((PSHNOTIFY*)lParam)->lParam, lpallobjs, itabSUMMARY);
                }
                
            }
            return TRUE;
            
        case PSN_QUERYCANCEL:
            if (lpallobjs->fPropDlgChanged && !lpallobjs->fPropDlgPrompted)
                ISavePropDlgChanges(lpallobjs, hDlg, ((NMHDR FAR *)lParam)->hwndFrom);
            return TRUE;
            
        case PSN_SETACTIVE :
            return TRUE;
            
        } // switch (WM_NOTIFY)
        break;
        
        case WM_CLOSE:
            //
            // This page contains multi-line edit controls, which swallow
            // the ESC key but generat a WM_CLOSE.  We need to pass this
            // message on to our parent (the property sheet) so that it is
            // handled correctly...
            //
            PostMessage( GetParent(hDlg), WM_CLOSE, wParam, lParam );
            break;
            
        case WM_CONTEXTMENU:
            WinHelp((HANDLE)wParam, NULL, HELP_CONTEXTMENU, (DWORD_PTR)rgIdhSummary);
            break;
            
        case WM_HELP:
            WinHelp(((LPHELPINFO)lParam)->hItemHandle, NULL, HELP_WM_HELP, (DWORD_PTR)rgIdhSummary);
            break;
    } // switch (message)
    
    return FALSE;
    
} // FSummaryDlgProc
#endif //_WIN2000_DOCPROP_

////////////////////////////////////////////////////////////////////////////////
//
// FStatisticsDlgProc
//
// Purpose;
//  Displays the Statistics dialog
//
////////////////////////////////////////////////////////////////////////////////
#ifndef _WIN2000_DOCPROP_
INT_PTR CALLBACK FStatisticsDlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hWndLV;
    LPSIOBJ lpSIObj;    // All SumInfo data.
    LPDSIOBJ lpDSIObj;  // All DocSumInfo data.
    LPPROPVARIANT rgpropvarSumInfo, rgpropvarDocSumInfo;
    
    HANDLE hFile;
    FILETIME ftTime;
    DWORD dwT;
    BOOL fListData;    // Did we stick some data in the listview??
    
    switch (message)
    {
    case WM_INITDIALOG :
        {
            PROPSHEETPAGE *ppspDlg = (PROPSHEETPAGE *) lParam;
            LPALLOBJS lpallobjs = (LPALLOBJS)ppspDlg->lParam;
            
            // Page does not do apply so don't worry
            // PropPageInit(lpallobjs, itabSTATISTICS);
            
            lpSIObj = lpallobjs->lpSIObj;    // Get the summary object.
            lpDSIObj = lpallobjs->lpDSIObj;  // Get the doc sum. object.
            
            if (lpSIObj->m_lpData == NULL ||
                lpDSIObj->m_lpData == NULL)
            {
                MESSAGE (TEXT("Invalid SumInfo or DocSumInfo object"));
                return FALSE;
            }
            
            rgpropvarSumInfo = GETSINFO(lpSIObj)->rgpropvar;
            rgpropvarDocSumInfo = GETSINFO(lpDSIObj)->rgpropvar;
            
            // Let the app update the stats if they provided a callback
            if (((LPSINFO) ((LPOFFICESUMINFO) lpSIObj)->m_lpData)->lpfnFUpdateStats != NULL)
                (*(((LPSINFO) ((LPOFFICESUMINFO) lpSIObj)->m_lpData)->lpfnFUpdateStats))(
                GetFocus(),
                lpSIObj, lpDSIObj);
            
            hWndLV = GetDlgItem(hDlg, IDD_STATISTICS_LISTVIEW);
            InitListView (hWndLV, iszVAL, rgszStatHeadings, FALSE);
            
            // Last saved by
            
            if (rgpropvarSumInfo[ PVSI_LASTAUTHOR ].vt == VT_LPTSTR)
            {
                SendDlgItemMessage
                    (hDlg, IDD_STATISTICS_LASTSAVEBY, WM_SETTEXT, 0,
                    (LPARAM) rgpropvarSumInfo[ PVSI_LASTAUTHOR ].pszVal);
            }
            
            
            // Revision #
            
            if (rgpropvarSumInfo[ PVSI_REVNUMBER ].vt == VT_LPTSTR)
            {
                SendDlgItemMessage
                    (hDlg, IDD_STATISTICS_REVISION, WM_SETTEXT, 0,
                    (LPARAM) rgpropvarSumInfo[ PVSI_REVNUMBER ].pszVal);
            }
            
            
            fListData = FALSE;
            if (rgpropvarSumInfo[ PVSI_PAGECOUNT ].vt == VT_I4)
            {
                AddItemToListView (hWndLV, rgpropvarSumInfo[ PVSI_PAGECOUNT ].lVal,
                    rgszStats[iszPAGES], FALSE);
                fListData = TRUE;
            }
            
            if (rgpropvarDocSumInfo[ PVDSI_SLIDECOUNT ].vt == VT_I4)
            {
                AddItemToListView (hWndLV, rgpropvarDocSumInfo[ PVDSI_SLIDECOUNT ].lVal,
                    rgszStats[iszSLIDES], FALSE);
                fListData = TRUE;
            }
            
            if (rgpropvarDocSumInfo[ PVDSI_PARACOUNT ].vt == VT_I4)
            {
                AddItemToListView (hWndLV, rgpropvarDocSumInfo[ PVDSI_PARACOUNT ].lVal,
                    rgszStats[iszPARA], FALSE);
                fListData = TRUE;
            }
            
            if (rgpropvarDocSumInfo[ PVDSI_LINECOUNT ].vt == VT_I4)
            {
                AddItemToListView (hWndLV, rgpropvarDocSumInfo[ PVDSI_LINECOUNT ].lVal,
                    rgszStats[iszLINES], FALSE);
                fListData = TRUE;
            }
            
            if (rgpropvarSumInfo[ PVSI_WORDCOUNT ].vt == VT_I4)
            {
                AddItemToListView (hWndLV, rgpropvarSumInfo[ PVSI_WORDCOUNT ].lVal,
                    rgszStats[iszWORDS], FALSE);
                fListData = TRUE;
            }
            
            if (rgpropvarSumInfo[ PVSI_CHARCOUNT ].vt == VT_I4)
            {
                AddItemToListView (hWndLV, rgpropvarSumInfo[ PVSI_CHARCOUNT ].lVal,
                    rgszStats[iszCHARS], FALSE);
                fListData = TRUE;
            }
            
            if (rgpropvarDocSumInfo[ PVDSI_BYTECOUNT ].vt == VT_I4)
            {
                AddItemToListView (hWndLV, rgpropvarDocSumInfo[ PVDSI_BYTECOUNT ].lVal,
                    rgszStats[iszBYTES], FALSE);
                fListData = TRUE;
            }
            
            if (rgpropvarDocSumInfo[ PVDSI_NOTECOUNT ].vt == VT_I4)
            {
                AddItemToListView (hWndLV, rgpropvarDocSumInfo[ PVDSI_NOTECOUNT ].lVal,
                    rgszStats[iszNOTES], FALSE);
                fListData = TRUE;
            }
            
            if (rgpropvarDocSumInfo[ PVDSI_HIDDENCOUNT ].vt == VT_I4)
            {
                AddItemToListView (hWndLV, rgpropvarDocSumInfo[ PVDSI_HIDDENCOUNT ].lVal,
                    rgszStats[iszHIDDENSLIDES], FALSE);
                fListData = TRUE;
            }
            
            if (rgpropvarDocSumInfo[ PVDSI_MMCLIPCOUNT ].vt == VT_I4)
            {
                AddItemToListView (hWndLV, rgpropvarDocSumInfo[ PVDSI_MMCLIPCOUNT ].lVal,
                    rgszStats[iszMMCLIPS], FALSE);
                fListData = TRUE;
            }
            
            if (rgpropvarDocSumInfo[ PVDSI_PRESFORMAT ].vt == VT_LPTSTR)
            {
                AddItemToListView (hWndLV, (DWORD_PTR) rgpropvarDocSumInfo[ PVDSI_PRESFORMAT ].pszVal,
                    rgszStats[iszFORMAT], TRUE);
                fListData = TRUE;
            }
            
            if (!fListData)
            {
                ShowWindow(GetDlgItem(hDlg, IDD_LINE_2), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDD_STATISTICS_LVLABEL), SW_HIDE);
                ShowWindow(hWndLV, SW_HIDE);
                hWndLV = NULL;
            }
            
            if (!lpallobjs->fFiledataInit)
            {
                hFile = FindFirstFile(lpallobjs->szPath, &lpallobjs->filedata);
                lpallobjs->fFiledataInit = TRUE;
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    FindClose(hFile);
                    lpallobjs->fFindFileSuccess = TRUE;
                }
            }
            
            if (lpallobjs->fFindFileSuccess)
            {
                // Last Access
                PrintTimeInDlg (hDlg, IDD_STATISTICS_ACCESSED, &(lpallobjs->filedata.ftLastAccessTime));
                
                // Last modified
                PrintTimeInDlg (hDlg, IDD_STATISTICS_CHANGED, &(lpallobjs->filedata.ftLastWriteTime));
            }
            
            
            // Create Time
            if (rgpropvarSumInfo[ PVSI_CREATE_DTM ].vt == VT_FILETIME)
            {
                PrintTimeInDlg(hDlg, IDD_STATISTICS_CREATED,
                    &rgpropvarSumInfo[ PVSI_CREATE_DTM ].filetime);
            }
            
            
            // Last Printed Time
            if (rgpropvarSumInfo[ PVSI_LASTPRINTED ].vt == VT_FILETIME)
            {
                PrintTimeInDlg(hDlg, IDD_STATISTICS_LASTPRINT,
                    &rgpropvarSumInfo[ PVSI_LASTPRINTED ].filetime);
            }
            
            // Total Edit Time
            // If we are not allowing time tracking display 0 minutes
            if (GETSINFO(lpSIObj)->fNoTimeTracking)
            {
                ftTime.dwLowDateTime = 0;
                ftTime.dwHighDateTime = 0;
                PrintEditTimeInDlg(hDlg, &ftTime);
            }
            else if (rgpropvarSumInfo[ PVSI_EDITTIME ].vt == VT_FILETIME)
            {
                PrintEditTimeInDlg(hDlg, &rgpropvarSumInfo[ PVSI_EDITTIME ].filetime);
            }
            
            //        lpallobjs->fPropDlgChanged = FALSE;
      }
      return TRUE;
      
    case WM_CTLCOLORBTN     :
    case WM_CTLCOLOREDIT    :
    case WM_CTLCOLORDLG     :
    case WM_CTLCOLORSTATIC  :
        if (hBrushPropDlg == NULL)
            break;
        DeleteObject(hBrushPropDlg);
        if ((hBrushPropDlg = CreateSolidBrush(GetSysColor(COLOR_BTNFACE))) == NULL)
            break;
        SetBkColor ((HDC) wParam, GetSysColor (COLOR_BTNFACE));
        SetTextColor((HDC) wParam, GetSysColor(COLOR_WINDOWTEXT));
        return (INT_PTR) hBrushPropDlg;
        
    case WM_SYSCOLORCHANGE:
        hWndLV = GetDlgItem(hDlg, IDD_STATISTICS_LISTVIEW);
        PostMessage(hWndLV, WM_SYSCOLORCHANGE, wParam, lParam);
        return TRUE;
        break;
        
    case WM_NOTIFY :
        
        switch (((NMHDR FAR *) lParam)->code)
        {
        case PSN_SETACTIVE :
            return TRUE;
            
        case PSN_RESET:
        case PSN_APPLY:
            MESSAGE (TEXT("PSN_APPLY - Statistics Page"));
            
            return TRUE;
            
        } // switch
        break;
        
        case WM_CONTEXTMENU:
            WinHelp((HANDLE)wParam, NULL, HELP_CONTEXTMENU, (DWORD_PTR)rgIdhStatistics);
            break;
            
        case WM_HELP:
            WinHelp(((LPHELPINFO)lParam)->hItemHandle, NULL, HELP_WM_HELP, (DWORD_PTR)rgIdhStatistics);
            break;
    } // switch
    
    return FALSE;
    
} // FStatisticsDlgProc
#endif //_WIN2000_DOCPROP_

////////////////////////////////////////////////////////////////////////////////
//
// FContentsDlgProc
//
// Purpose:
//  Display the contents dialog
//
////////////////////////////////////////////////////////////////////////////////
#ifndef _WIN2000_DOCPROP_
INT_PTR CALLBACK FContentsDlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPDSIOBJ lpDSIObj;                  // All DocSumInfo data.
    LPPROPVARIANT rgpropvarDocSumInfo;  // Just the DocSumInfo properties.
    
    switch (message)
    {
    case WM_INITDIALOG :
        {
            BOOL fInitDialogSuccess = TRUE;
            PROPSHEETPAGE *ppspDlg = (PROPSHEETPAGE *)lParam;
            SHORT cT;
            LRESULT lError;
            TCHAR *psz;
            TCHAR ch;
            
            ULONG cHeadings;    // Number of document headings
            ULONG cDocParts;    // Total count of document parts.
            
            LPPROPVARIANT rgpropvarHeadings;    // The Variant|Vector of heading pairs
            LPTSTR        *rglptstrDocParts;    // The document parts.
            
            ULONG ulHeadingIndex;       // Index into rgpropvarHeadings
            ULONG ulTotalDocPartIndex;  // Index into rglptstrDocParts
            ULONG ulSubDocPartIndex;    // Index into rglptstrDocParts for the current heading
            
            ULONG  cbDocPart = 0;           // # bytes in a particular docpart.
            LPTSTR tszDocumentPart = NULL;  // One DocPart with a pre-pending tab.
            
            lpDSIObj = ((LPALLOBJS)ppspDlg->lParam)->lpDSIObj;  // Get the document sum. object.
            
            // Validate our objects.
            
            if (lpDSIObj == NULL || lpDSIObj->m_lpData == NULL)
                return FALSE;
            
            // Get to the DocSumInfo properties.
            
            rgpropvarDocSumInfo = GETDSINFO(lpDSIObj)->rgpropvar;
            
            // See if we have any headings.
            
            if (rgpropvarDocSumInfo[ PVDSI_HEADINGPAIR ].vt != (VT_VECTOR | VT_VARIANT))
            {
                // Nothing to display.
                return FALSE;
            }
            
            // Get the Heading-Pair array and count.  Validate the size of the array.
            
            cHeadings = rgpropvarDocSumInfo[ PVDSI_HEADINGPAIR ].capropvar.cElems;
            rgpropvarHeadings = rgpropvarDocSumInfo[ PVDSI_HEADINGPAIR ].capropvar.pElems;
            
            if (cHeadings == 0)
            {
                goto ExitCaseWM_INITDIALOG;
            }
            
            if (cHeadings & 0x1)
            {
                // This is an invalid array; the elements always come in (heading,count) pairs,
                // so there should be an even number of elements in the array.
                
                MESSAGE (TEXT("Invalid Headings array (should be an even number of elements)"));
                goto ExitCaseWM_INITDIALOG;
            }
            
            
            // Get the DocParts array and its size.  All elements of this array are LPTSTRs.
            // Note that this array need not exist.
            
            cDocParts = 0;
            if (rgpropvarDocSumInfo[ PVDSI_DOCPARTS ].vt == (VT_VECTOR | VT_LPTSTR))
            {
                rglptstrDocParts = (LPTSTR*) rgpropvarDocSumInfo[ PVDSI_DOCPARTS ].calpstr.pElems;
                cDocParts = rgpropvarDocSumInfo[ PVDSI_DOCPARTS ].calpstr.cElems;
            }
            
            // Loop through the Heading-Pair array.  For each heading, we'll write it
            // to the list box, and then use the associated count to write Document Parts
            // to the list box.  We'll stop when we've processed the array, or when
            // fInitDialogSuccess goes FALSE.
            
            ulTotalDocPartIndex = 0;
            for (ulHeadingIndex = 0;
            (ulHeadingIndex < cHeadings) && fInitDialogSuccess;
            ulHeadingIndex+=2)
            {
                // Verify that the next two elements in the Heading-Pair array are
                // a string and a Long.
                
                if (rgpropvarHeadings[ ulHeadingIndex ].vt != VT_LPTSTR
                    ||
                    rgpropvarHeadings[ ulHeadingIndex + 1 ].vt != VT_I4)
                {
                    MESSAGE (TEXT("Invalid Heading Pair type"));
                    fInitDialogSuccess = FALSE;
                    break;
                }
                
                // Write the Heading to the list box.
                
                lError = SendDlgItemMessage (
                    hDlg, IDD_CONTENTS_LISTBOX, LB_ADDSTRING, (WPARAM) 0,
                    (LPARAM) rgpropvarHeadings[ ulHeadingIndex ].pszVal );
                
                if (lError == LB_ERR || lError == LB_ERRSPACE)
                {
                    MESSAGE(TEXT("Could not write to list box"));
                    fInitDialogSuccess = FALSE;
                    break;
                }
                
                // Verify that the DocParts we've written so far, plus those we're about
                // to write, don't exceed the total in the DocParts array.
                
                if (ulTotalDocPartIndex + rgpropvarHeadings[ ulHeadingIndex + 1 ].lVal
                    > cDocParts)
                {
                    MESSAGE (TEXT("Invalid doc-part count for heading"));
                    fInitDialogSuccess = FALSE;
                    break;
                }
                
                // Now loop through the document parts that are associated with this
                // heading.  We'll insert a tab character in front of each string
                // so that the display looks better.  We'll stop when we've processed
                // all the DocParts under this heading, or when fInitDialogSuccess goes
                // FALSE;
                
                for (ulSubDocPartIndex = ulTotalDocPartIndex;
                fInitDialogSuccess
                    && (ulSubDocPartIndex
                    < ulTotalDocPartIndex + rgpropvarHeadings[ ulHeadingIndex + 1 ].lVal);
                ulSubDocPartIndex++)
                {
                    
                    // Determine how big the DocumentPart string is, including a NULL
                    // terminator, and including a tab character that we're going to insert.
                    
                    cbDocPart  // Count the characters.
#ifdef UNICODE
                        = CchTszLen (rglptstrDocParts[ ulSubDocPartIndex ]) + 2;
#else
                    = CchAnsiSzLen (rglptstrDocParts[ ulSubDocPartIndex ]) + 2;
#endif
                    cbDocPart *= sizeof(TCHAR);    // Convert to a *byte* count.
                    
                    // Alloc a buffer which will hold a tab character followed by the DocPart string.
                    // Since there's no easy way to add cleanup code for this case block,
                    // we must ensure that we free this buffer before any break could possibly
                    // occur.
                    
                    tszDocumentPart = PvMemAlloc (cbDocPart);
                    
                    if (tszDocumentPart == NULL)
                    {
                        AssertSz (0, TEXT("Couldn't alloc memory for doc-part display"));
                        fInitDialogSuccess = FALSE;
                        break;
                    }
                    
                    // Put the tab in the new string, followed by the DocPart string.
                    
                    *tszDocumentPart = TEXT('\t');
                    PbMemCopy(&tszDocumentPart[1],
                        rglptstrDocParts[ ulSubDocPartIndex ],
                        cbDocPart - sizeof(TCHAR)); // Subtract the size of the tab
                    
                    // Write the result to the list box, and free the temporary string.
                    
                    lError = SendDlgItemMessage (
                        hDlg, IDD_CONTENTS_LISTBOX, LB_ADDSTRING, (WPARAM) 0,
                        (LPARAM) tszDocumentPart);
                    
                    if (tszDocumentPart != NULL)
                    {
                        VFreeMemP (tszDocumentPart, cbDocPart);
                        tszDocumentPart = NULL;
                        cbDocPart = 0;
                    }
                    
                    if (lError == LB_ERR || lError == LB_ERRSPACE)
                    {
                        AssertSz (0, TEXT("Could not write to list box"));
                        fInitDialogSuccess = FALSE;
                        break;
                    }
                    
                }   // for (ulSubDocPartIndex = ulTotalDocPartIndex; ...
                
                // Add the cont for all the doc-parts that we just displayed
                // to the total.
                
                ulTotalDocPartIndex = ulSubDocPartIndex;
                
            }   // for (ulHeadingIndex = 0; ulHeadingIndex < cHeadings; ulHeadingIndex++)
            
            fInitDialogSuccess = TRUE;
            
ExitCaseWM_INITDIALOG:
            
            // Now that the headings and document-parts are in the dialog,
            // we don't need to hold on to them any longer.
            
            PropVariantClear (&GETDSINFO(lpDSIObj)->rgpropvar[PVDSI_HEADINGPAIR]);
            PropVariantClear (&GETDSINFO(lpDSIObj)->rgpropvar[PVDSI_DOCPARTS]);
            
            return fInitDialogSuccess;
            
        }   // end - case WM_INITDIALOG
        
    case WM_CTLCOLORBTN     :
    case WM_CTLCOLORDLG     :
    case WM_CTLCOLORSTATIC  :
        if (hBrushPropDlg == NULL)
            break;
        DeleteObject(hBrushPropDlg);
        if ((hBrushPropDlg = CreateSolidBrush(GetSysColor(COLOR_BTNFACE))) == NULL)
            break;
        SetBkColor ((HDC) wParam, GetSysColor (COLOR_BTNFACE));
        SetTextColor((HDC) wParam, GetSysColor(COLOR_WINDOWTEXT));
        return (INT_PTR) hBrushPropDlg;
        
    case WM_NOTIFY :
        
        switch (((NMHDR FAR *) lParam)->code)
        {
        case PSN_SETACTIVE :
            return TRUE;
        case PSN_RESET:
        case PSN_APPLY:
            MESSAGE (TEXT("PSN_APPLY - Contents Page"));
            return TRUE;
            
        } // switch
        break;
        
        case WM_CONTEXTMENU:
            WinHelp((HANDLE)wParam, NULL, HELP_CONTEXTMENU, (DWORD_PTR)rgIdhContents);
            break;
            
        case WM_HELP:
            WinHelp(((LPHELPINFO)lParam)->hItemHandle, NULL, HELP_WM_HELP, (DWORD_PTR)rgIdhContents);
            break;
    } // switch
    
    return FALSE;
    
    
} // FContentsDlgProc
#endif // _WIN2000_DOCPROP_

int  gOKButtonID;  // need this to store the ID of the OK button, since it's not in the dlg template

////////////////////////////////////////////////////////////////////////////////
//
// FCustomDlgProc
//
// Purpose:
//  Custom tab control
//
////////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK FCustomDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPALLOBJS lpallobjs = (LPALLOBJS)GetWindowLongPtr(hDlg, DWLP_USER);
    
    switch (message)
    {
    case WM_INITDIALOG:
        {
            PROPSHEETPAGE *ppspDlg = (PROPSHEETPAGE *) lParam;
            int irg;
            HICON hIcon, hInvIcon;
            
            lpallobjs = (LPALLOBJS)ppspDlg->lParam;
            
            PropPageInit(lpallobjs, itabCUSTOM);
            
            SetWindowLongPtr(hDlg, DWLP_USER, ppspDlg->lParam);
            gOKButtonID = LOWORD(SendMessage(hDlg, DM_GETDEFID, 0L, 0L));
            
            AssertSz ((sizeof(NUM) == (sizeof(FILETIME))), TEXT("Ok, who changed base type sizes?"));
            
            //
            // Fill out the Name dropdown
            //
            for (irg = 0; irg < NUM_BUILTIN_CUSTOM_NAMES; ++irg)
            {
                if (!CchGetString( idsCustomName1+ irg,
                    lpallobjs->CDP_sz,
                    sizeof(lpallobjs->CDP_sz))
                    )
                {
                    return(FALSE);
                }
                SendDlgItemMessage(hDlg, IDD_CUSTOM_NAME, CB_ADDSTRING, 0, (LPARAM)lpallobjs->CDP_sz);
            }
            
            //
            // Fill out the type drop-down & select the text type
            //
            for (irg = 0; irg <= iszBOOL; irg++)
            {
                SendDlgItemMessage(hDlg, IDD_CUSTOM_TYPE, CB_ADDSTRING, 0, (LPARAM) rgszTypes[irg]);
            }
            
            ResetTypeControl (hDlg, IDD_CUSTOM_TYPE, &lpallobjs->CDP_iszType);
            
            //
            // Set the link checkbox to be off.
            //
            lpallobjs->CDP_fLink = FALSE;
            
            SendDlgItemMessage( hDlg,
                IDD_CUSTOM_LINK,
                BM_SETCHECK,
                (WPARAM) lpallobjs->CDP_fLink,
                0
                );

#ifndef __CUSTOM_LINK_ENABLED__
            ShowWindow( GetDlgItem( hDlg, IDD_CUSTOM_LINK ), SW_HIDE );
#endif  __CUSTOM_LINK_ENABLED__
            
            SendDlgItemMessage( hDlg,
                IDD_CUSTOM_VALUETEXT,
                WM_SETTEXT,
                0,
                (LPARAM) rgszValue[iszVALUE]
                );
            
            //
            // Hang on to the window handle of the value edit control & others
            //
            lpallobjs->CDP_hWndVal = GetDlgItem (hDlg, IDD_CUSTOM_VALUE);
            lpallobjs->CDP_hWndName = GetDlgItem (hDlg, IDD_CUSTOM_NAME);
            lpallobjs->CDP_hWndLinkVal = GetDlgItem (hDlg, IDD_CUSTOM_LINKVALUE);
            lpallobjs->CDP_hWndValText = GetDlgItem (hDlg, IDD_CUSTOM_VALUETEXT);
            lpallobjs->CDP_hWndBoolTrue = GetDlgItem (hDlg, IDD_CUSTOM_BOOLTRUE);
            lpallobjs->CDP_hWndBoolFalse = GetDlgItem (hDlg, IDD_CUSTOM_BOOLFALSE);
            lpallobjs->CDP_hWndGroup = GetDlgItem (hDlg, IDD_CUSTOM_GBOX);
            lpallobjs->CDP_hWndAdd = GetDlgItem (hDlg, IDD_CUSTOM_ADD);
            lpallobjs->CDP_hWndDelete = GetDlgItem (hDlg, IDD_CUSTOM_DELETE);
            lpallobjs->CDP_hWndType = GetDlgItem (hDlg, IDD_CUSTOM_TYPE);
            lpallobjs->CDP_hWndCustomLV = GetDlgItem(hDlg, IDD_CUSTOM_LISTVIEW);
            InitListView (lpallobjs->CDP_hWndCustomLV, iszTYPE, rgszHeadings, TRUE);
            
            //
            // Initially disable the Add & Delete buttons
            //
            EnableWindow (lpallobjs->CDP_hWndAdd, FALSE);
            EnableWindow (lpallobjs->CDP_hWndDelete, FALSE);
            lpallobjs->CDP_fAdd = TRUE;
            
            //
            // Don't let the user enter too much text
            // If you change this value, you must change the buffer
            // size (szDate) in FConvertDate
            //
            SendMessage (lpallobjs->CDP_hWndVal, EM_LIMITTEXT, BUFMAX-1, 0);
            SendMessage (lpallobjs->CDP_hWndName, EM_LIMITTEXT, BUFMAX-1, 0);
            
            //
            // Add the link icon to the image list
            //
            hIcon = LoadIcon (g_hmodThisDll, MAKEINTRESOURCE (IDD_LINK_ICON));
            hInvIcon = LoadIcon (g_hmodThisDll, MAKEINTRESOURCE (IDD_INVLINK_ICON));
            if (hIcon != NULL)
            {
                lpallobjs->CDP_hImlS = ListView_GetImageList( lpallobjs->CDP_hWndCustomLV, TRUE );
                giLinkIcon = MsoImageList_ReplaceIcon( lpallobjs->CDP_hImlS, -1, hIcon );
                Assert ((giLinkIcon != -1));
                
                giInvLinkIcon = MsoImageList_ReplaceIcon (lpallobjs->CDP_hImlS, -1, hInvIcon);
                Assert ((giInvLinkIcon != -1));
            }
            else
            {
                DebugSz (TEXT("Icon load failed"));
            }
            
            //
            // Make a temporary copy of the custom data
            //
            FMakeTmpUDProps (lpallobjs->lpUDObj);
            
            //
            // Fill in the list view box with any data from the object
            //
            PopulateUDListView (lpallobjs->CDP_hWndCustomLV, lpallobjs->lpUDObj);
            
            //
            // See if the client supports links - turn off checkbox if they don't
            //
            lpallobjs->CDP_cLinks =
#ifdef __CUSTOM_LINK_ENABLED__
                (lpallobjs->lpfnDwQueryLinkData != NULL)                      ?
                (*lpallobjs->lpfnDwQueryLinkData) (QLD_CLINKS, 0, NULL, NULL) :
#endif __CUSTOM_LINK_ENABLED__
            0;

            if (!lpallobjs->CDP_cLinks)
            {
                EnableWindow (GetDlgItem (hDlg, IDD_CUSTOM_LINK), FALSE);
                EnableWindow (lpallobjs->CDP_hWndLinkVal, FALSE);
            }
            
            //        lpallobjs->fPropDlgChanged = FALSE;
            //      fItemSel = FALSE;
            return TRUE;
            break;
      }
      
    case WM_CTLCOLORBTN     :
    case WM_CTLCOLORDLG     :
    case WM_CTLCOLORSTATIC  :
        if (hBrushPropDlg == NULL)
            break;
        DeleteObject(hBrushPropDlg);
        if ((hBrushPropDlg = CreateSolidBrush(GetSysColor(COLOR_BTNFACE))) == NULL)
            break;
        SetBkColor ((HDC) wParam, GetSysColor (COLOR_BTNFACE));
        SetTextColor((HDC) wParam, GetSysColor(COLOR_WINDOWTEXT));
        return (INT_PTR) hBrushPropDlg;
        
    case WM_SYSCOLORCHANGE:
        PostMessage(lpallobjs->CDP_hWndCustomLV, WM_SYSCOLORCHANGE, wParam, lParam);
        return TRUE;
        break;
        
        //
        // This message is posted when ever the user does something with the
        // Name field.  That allows the system to finish what they are doing
        // and fill in the edit field if they have to.  See bug 2820.
        //
    case WM_USER+0x1000:
        if (!(lpallobjs->CDP_fLink && (lpallobjs->lpfnDwQueryLinkData == NULL)))
        {
            lpallobjs->CDP_iszType = (int)SendMessage (lpallobjs->CDP_hWndType, CB_GETCURSEL, 0, 0);
            FSetupAddButton (lpallobjs->CDP_iszType, lpallobjs->CDP_fLink, &lpallobjs->CDP_fAdd, lpallobjs->CDP_hWndAdd, lpallobjs->CDP_hWndVal, lpallobjs->CDP_hWndName, hDlg);
            if (FAllocAndGetValLpstz (hDlg, IDD_CUSTOM_NAME, &glpstzName))
            {
                LPUDPROP lpudp = LpudpropFindMatchingName (lpallobjs->lpUDObj, (LPTSTR)PSTR (glpstzName));
                if (lpudp != NULL)
                {
                    if (lpallobjs->CDP_fAdd)
                    {
                        SendMessage (lpallobjs->CDP_hWndAdd, WM_SETTEXT, 0, (LPARAM) rgszAdd[iszMODIFY]);
                        lpallobjs->CDP_fAdd = FALSE;
                    }
                }
            }
            EnableWindow(lpallobjs->CDP_hWndDelete, FALSE);   // If the user touches the Name field, disable Delete button
            // Are we showing an invalid link?
            if (lpallobjs->CDP_fLink && !IsWindowEnabled(GetDlgItem(hDlg,IDD_CUSTOM_LINK)))
            {
                // Turn off the link checkbox
                lpallobjs->CDP_fLink = FALSE;
                SendDlgItemMessage (hDlg, IDD_CUSTOM_LINK, BM_SETCHECK, (WPARAM) lpallobjs->CDP_fLink, 0);
                if (lpallobjs->CDP_cLinks)   // Could be that the app is allowing links
                    EnableWindow (GetDlgItem (hDlg, IDD_CUSTOM_LINK), TRUE);
                // Clear the value window
                ClearEditControl (lpallobjs->CDP_hWndVal, 0);
                FSwapControls (lpallobjs->CDP_hWndVal, lpallobjs->CDP_hWndLinkVal, lpallobjs->CDP_hWndBoolTrue, lpallobjs->CDP_hWndBoolFalse,
                    lpallobjs->CDP_hWndGroup, lpallobjs->CDP_hWndType, lpallobjs->CDP_hWndValText, FALSE, FALSE);
            }
        }
        return(TRUE);
        break;
        
    case WM_COMMAND :
        switch (HIWORD (wParam))
        {
        case BN_CLICKED :
            switch (LOWORD (wParam))
            {
            case IDD_CUSTOM_ADD :
                if (FGetCustomPropFromDlg(lpallobjs, hDlg))
                {
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                }
                
                return(FALSE);     // return 0 'cuz we process the message
                break;
                
            case IDD_CUSTOM_DELETE :
                //              Assert (fItemSel);
                
                //              fItemSel = FALSE;                 // We're about to delete it!
                DeleteItem (lpallobjs->lpUDObj, lpallobjs->CDP_hWndCustomLV, lpallobjs->CDP_iItem, lpallobjs->CDP_sz);
                
                // Turn off the link checkbox if it was on.
                lpallobjs->CDP_fLink = FALSE;
                SendDlgItemMessage (hDlg, IDD_CUSTOM_LINK, BM_SETCHECK, (WPARAM) lpallobjs->CDP_fLink, 0);
                ClearEditControl (lpallobjs->CDP_hWndVal, 0);
                
                FSwapControls (lpallobjs->CDP_hWndVal, lpallobjs->CDP_hWndLinkVal, lpallobjs->CDP_hWndBoolTrue, lpallobjs->CDP_hWndBoolFalse,
                    lpallobjs->CDP_hWndGroup, lpallobjs->CDP_hWndType, lpallobjs->CDP_hWndValText, FALSE, FALSE);
                
                FSetupAddButton (lpallobjs->CDP_iszType, lpallobjs->CDP_fLink, &lpallobjs->CDP_fAdd, lpallobjs->CDP_hWndAdd, lpallobjs->CDP_hWndVal, lpallobjs->CDP_hWndName, hDlg);
                ResetTypeControl (hDlg, IDD_CUSTOM_TYPE, &lpallobjs->CDP_iszType);
                SendMessage(lpallobjs->CDP_hWndName, CB_SETEDITSEL, 0, MAKELPARAM(0,-1));     // Select entire string
                SendMessage(lpallobjs->CDP_hWndName, WM_CLEAR, 0, 0);
                SetFocus(lpallobjs->CDP_hWndName);
                //              lpallobjs->fPropDlgChanged = TRUE;
                PropSheet_Changed(GetParent(hDlg), hDlg);
                return(FALSE);     // return 0 'cuz we process the message
                break;
                
            case IDD_CUSTOM_LINK :
                {
                    BOOL fMod = FALSE;
                    // Should never get a message from a disabled control
                    Assert (lpallobjs->CDP_cLinks);
                    
                    lpallobjs->CDP_fLink = !lpallobjs->CDP_fLink;
                    SendDlgItemMessage (hDlg, IDD_CUSTOM_LINK, BM_SETCHECK, (WPARAM) lpallobjs->CDP_fLink, 0);
                    
                    // If the link box is checked, the value edit needs to change
                    // to a combobox filled with link data
                    if (lpallobjs->CDP_fLink)
                    {
                        Assert ((lpallobjs->lpfnDwQueryLinkData != NULL));
                        
                        FCreateListOfLinks (lpallobjs->CDP_cLinks, lpallobjs->lpfnDwQueryLinkData, lpallobjs->CDP_hWndLinkVal);
                        SendMessage (lpallobjs->CDP_hWndLinkVal, CB_SETCURSEL, 0, 0);
                        FSetTypeControl ((*lpallobjs->lpfnDwQueryLinkData) (QLD_LINKTYPE, 0, NULL, NULL), lpallobjs->CDP_hWndType);
                    }
                    else
                        ClearEditControl (lpallobjs->CDP_hWndVal, 0);
                    
                    FSwapControls (lpallobjs->CDP_hWndVal, lpallobjs->CDP_hWndLinkVal, lpallobjs->CDP_hWndBoolTrue, lpallobjs->CDP_hWndBoolFalse,
                        lpallobjs->CDP_hWndGroup, lpallobjs->CDP_hWndType, lpallobjs->CDP_hWndValText, lpallobjs->CDP_fLink, FALSE);
                    
                    // HACK, we don't want FSetupAddButton to change the text of the add
                    // button
                    if (!lpallobjs->CDP_fAdd)
                        fMod = lpallobjs->CDP_fAdd = TRUE;
                    // Set up the "Add" button correctly
                    FSetupAddButton (lpallobjs->CDP_iszType, lpallobjs->CDP_fLink, &lpallobjs->CDP_fAdd, lpallobjs->CDP_hWndAdd, lpallobjs->CDP_hWndVal, lpallobjs->CDP_hWndName, hDlg);
                    if (fMod)
                        lpallobjs->CDP_fAdd = FALSE;
                    return(FALSE);     // return 0 'cuz we process the message
                    break;
                }
            case IDD_CUSTOM_BOOLTRUE:
            case IDD_CUSTOM_BOOLFALSE:
                {
                    BOOL fMod = FALSE;
                    lpallobjs->CDP_iszType = (int)SendMessage (lpallobjs->CDP_hWndType, CB_GETCURSEL, 0, 0);
                    
                    // HACK, we don't want FSetupAddButton to change the text of the add
                    // button
                    if (!lpallobjs->CDP_fAdd)
                        fMod = lpallobjs->CDP_fAdd = TRUE;
                    FSetupAddButton (lpallobjs->CDP_iszType, lpallobjs->CDP_fLink, &lpallobjs->CDP_fAdd, lpallobjs->CDP_hWndAdd, lpallobjs->CDP_hWndVal, lpallobjs->CDP_hWndName, hDlg);
                    if (fMod)
                        lpallobjs->CDP_fAdd = FALSE;
                    
                    return(FALSE);
                }
                
            default:
                return(TRUE);
            }
            
            case CBN_CLOSEUP:
                // Hack!!
                // We need to post a message to ourselves to check if the user's
                // actions entered text in the edit field.
                PostMessage(hDlg, WM_USER+0x1000, 0L, 0L);
                return(FALSE);
                
            case CBN_SELCHANGE :
                switch (LOWORD (wParam))
                {
                case IDD_CUSTOM_NAME  :
                    // Hack!!
                    // We need to post a message to ourselves to check if the user's
                    // actions entered text in the edit field.
                    PostMessage(hDlg, WM_USER+0x1000, 0L, 0L);
                    return(FALSE);     // return 0 'cuz we process the message
                    break;
                    
                case IDD_CUSTOM_TYPE :
                    {
                        BOOL fMod = FALSE;
                        // If the user picks the Boolean type from the combo box,
                        // we must replace the edit control for the value
                        // with radio buttons.  If the Link checkbox is set,
                        // the type depends on the link value, not user selection
                        lpallobjs->CDP_iszType = (int)SendMessage ((HWND) lParam, CB_GETCURSEL, 0, 0);
                        FSwapControls (lpallobjs->CDP_hWndVal, lpallobjs->CDP_hWndLinkVal, lpallobjs->CDP_hWndBoolTrue, lpallobjs->CDP_hWndBoolFalse,
                            lpallobjs->CDP_hWndGroup, lpallobjs->CDP_hWndType, lpallobjs->CDP_hWndValText, lpallobjs->CDP_fLink, (lpallobjs->CDP_iszType == iszBOOL));
                        // HACK: FSwapControls() resets the type selection to be
                        // the first one (since all other clients need that to
                        // happen).  In this case, the user has selected a new
                        // type, so we need to force it manually to what they picked.
                        SendMessage (lpallobjs->CDP_hWndType, CB_SETCURSEL, lpallobjs->CDP_iszType, 0);
                        // HACK: FSetupAddButton will change the Add button to
                        // say "Add" if lpallobjs->CDP_fAdd is FALSE.  Since we just changed
                        // the button to "Modify", fake it out to not change
                        // the Add button by flipping lpallobjs->CDP_fAdd, then flipping it back.
                        if (!lpallobjs->CDP_fAdd)
                            fMod = lpallobjs->CDP_fAdd = TRUE;
                        
                        FSetupAddButton (lpallobjs->CDP_iszType, lpallobjs->CDP_fLink, &lpallobjs->CDP_fAdd, lpallobjs->CDP_hWndAdd, lpallobjs->CDP_hWndVal, lpallobjs->CDP_hWndName, hDlg);
                        if (fMod)
                            lpallobjs->CDP_fAdd = FALSE;
                        return(FALSE);     // return 0 'cuz we process the message
                    }
                case IDD_CUSTOM_LINKVALUE :
                    // If the user has the "Link" box checked and starts picking
                    // link values, make sure that the "Type" combobox is updated
                    // to the type of the static value of the link.
                    {
                        DWORD irg;
                        
                        AssertSz (lpallobjs->CDP_fLink, TEXT("Link box must be checked in order for this dialog to be visible!"));
                        
                        // Get the link value from the combobox, and store
                        // the link name and static value.
                        irg = (int)SendMessage (lpallobjs->CDP_hWndLinkVal, CB_GETCURSEL, 0, 0);
                        
                        Assert ((lpallobjs->lpfnDwQueryLinkData != NULL));
                        
                        // REVIEW: If apps really need the name, we can get it here....
                        FSetTypeControl ((*lpallobjs->lpfnDwQueryLinkData) (QLD_LINKTYPE, irg, NULL, NULL), lpallobjs->CDP_hWndType);
                        return(FALSE);     // return 0 'cuz we process the message
                    }
                default:
                    return TRUE;      // we didn't process message
                }
                
                case CBN_EDITCHANGE:     // The user typed their own
                    switch (LOWORD (wParam))
                    {
                    case IDD_CUSTOM_NAME  :
                        // Hack!!
                        // We need to post a message to ourselves to check if the user's
                        // actions entered text in the edit field.
                        PostMessage(hDlg, WM_USER+0x1000, 0L, 0L);
                        return(FALSE);     // return 0 'cuz we process the message
                        break;
                    default:
                        return(TRUE);
                        break;
                    }
                    
                    case EN_UPDATE :
                        switch (LOWORD (wParam))
                        {
                            
                        case IDD_CUSTOM_VALUE :
                            {
                                BOOL fMod = FALSE;
                                
                                if (FAllocAndGetValLpstz (hDlg, IDD_CUSTOM_NAME, &glpstzName))
                                {
                                    LPUDPROP lpudp = LpudpropFindMatchingName (lpallobjs->lpUDObj, (LPTSTR)PSTR (glpstzName));
                                    if (lpudp != NULL)
                                    {
                                        if (lpallobjs->CDP_fAdd)
                                        {
                                            SendMessage (lpallobjs->CDP_hWndAdd, WM_SETTEXT, 0, (LPARAM) rgszAdd[iszMODIFY]);
                                            lpallobjs->CDP_fAdd = FALSE;
                                        }
                                    }
                                    // HACK: FSetupAddButton will change the Add button to
                                    // say "Add" if lpallobjs->CDP_fAdd is FALSE.  Since we just changed
                                    // the button to "Modify", fake it out to not change
                                    // the Add button by flipping lpallobjs->CDP_fAdd, then flipping it back.
                                    if (!lpallobjs->CDP_fAdd)
                                        fMod = lpallobjs->CDP_fAdd = TRUE;
                                    
                                    FSetupAddButton (lpallobjs->CDP_iszType, lpallobjs->CDP_fLink, &lpallobjs->CDP_fAdd, lpallobjs->CDP_hWndAdd, lpallobjs->CDP_hWndVal, lpallobjs->CDP_hWndName, hDlg);
                                    if (fMod)
                                        lpallobjs->CDP_fAdd = FALSE;
                                }
                                return(FALSE);     // return 0 'cuz we process the message
                            }
                        default:
                            return TRUE;      // we didn't process message
                        }
                        
                        case EN_KILLFOCUS :
                            switch (LOWORD (wParam))
                            {
                                // If the user finishes entering text in the Name edit control,
                                // be really cool and check to see if the name they entered
                                // is a property that is already defined.  If it is,
                                // change the Add button to Modify.
                            case IDD_CUSTOM_NAME :
                                if (FAllocAndGetValLpstz (hDlg, IDD_CUSTOM_NAME, &glpstzName))
                                {
                                    LPUDPROP lpudp = LpudpropFindMatchingName (lpallobjs->lpUDObj, (LPTSTR)PSTR (glpstzName));
                                    if (lpudp != NULL)
                                    {
                                        if (lpallobjs->CDP_fAdd)
                                        {
                                            SendMessage (lpallobjs->CDP_hWndAdd, WM_SETTEXT, 0, (LPARAM) rgszAdd[iszMODIFY]);
                                            lpallobjs->CDP_fAdd = FALSE;
                                        }
                                    }
                                }
                                return FALSE;
                                
                            default:
                                return TRUE;
                            }
                            default:
                                return TRUE;
        } // switch
        
    case WM_DESTROY:
        MsoImageList_Destroy(lpallobjs->CDP_hImlS);
        return FALSE;
        
    case WM_NOTIFY :
        
        switch (((NMHDR FAR *) lParam)->code)
        {
        case LVN_ITEMCHANGING :
            // If an item is gaining focus, put it in the edit controls at
            // the top of the dialog.
            if (((NM_LISTVIEW FAR *) lParam)->uNewState & LVIS_SELECTED)
            {
                Assert ((((NM_LISTVIEW FAR *) lParam) != NULL));
                lpallobjs->CDP_iItem = ((NM_LISTVIEW FAR *) lParam)->iItem;
                ListView_GetItemText (lpallobjs->CDP_hWndCustomLV, lpallobjs->CDP_iItem, 0, lpallobjs->CDP_sz, BUFMAX);
                PopulateControls (lpallobjs->lpUDObj, lpallobjs->CDP_sz, lpallobjs->CDP_cLinks, lpallobjs->lpfnDwQueryLinkData, hDlg,
                    GetDlgItem (hDlg, IDD_CUSTOM_NAME), lpallobjs->CDP_hWndVal, lpallobjs->CDP_hWndValText,
                    GetDlgItem (hDlg, IDD_CUSTOM_LINK), lpallobjs->CDP_hWndLinkVal, lpallobjs->CDP_hWndType,
                    lpallobjs->CDP_hWndBoolTrue, lpallobjs->CDP_hWndBoolFalse, lpallobjs->CDP_hWndGroup, lpallobjs->CDP_hWndAdd, lpallobjs->CDP_hWndDelete, &lpallobjs->CDP_fLink, &lpallobjs->CDP_fAdd);
                
                return FALSE;
            }
            return TRUE;
            break;
            
#ifdef OFFICE_96
        case LVN_COLUMNCLICK:
            // We only sort the name column
            if (((LPARAM)((NM_LISTVIEW *)lParam)->iSubItem == 0))
                ListView_SortItems(((NM_LISTVIEW *) lParam)->hdr.lpallobjs->CDP_hWndFrom, ListViewCompareFunc,0);
            return TRUE;
#endif
            
        case PSN_APPLY :
            if (IsWindowEnabled(lpallobjs->CDP_hWndAdd))
                FGetCustomPropFromDlg(lpallobjs, hDlg);
            // Swap the temp copy to be the real copy.
            FDeleteTmpUDProps (lpallobjs->lpUDObj);
            MESSAGE (TEXT("PSN_APPLY - Custom Page"));
            
            if (FUserDefShouldSave (lpallobjs->lpUDObj)
                || lpallobjs->fPropDlgChanged )
            {
                if( !ApplyChangesBackToFile(hDlg, (BOOL)((PSHNOTIFY*)lParam)->lParam, lpallobjs, itabCUSTOM) )
                {
                    PostMessage( GetParent(hDlg), PSM_SETCURSEL, (WPARAM)-1, (LPARAM)lpallobjs->lpUDObj->m_hPage );
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE );
                    return TRUE;
                }
            }
            return PSNRET_NOERROR;
            
        case PSN_RESET :
            if (lpallobjs->fPropDlgChanged && !lpallobjs->fPropDlgPrompted)
            {
                // BUGBUG typo here but I don't know what the author wanted
                if (ISavePropDlgChanges(lpallobjs, hDlg, ((NMHDR FAR *)lParam)->hwndFrom) != IDNO);
                return(TRUE);
            }
            // User cancelled the changes, so just delete the tmp stuff.
            FSwapTmpUDProps (lpallobjs->lpUDObj);
            FDeleteTmpUDProps (lpallobjs->lpUDObj);
            return TRUE;
            
        case PSN_SETACTIVE :
            return TRUE;
            
        default:
            break;
        } // switch
        break;
        
        case WM_CONTEXTMENU:
            WinHelp((HANDLE)wParam, NULL, HELP_CONTEXTMENU, (DWORD_PTR)rgIdhCustom);
            break;
            
        case WM_HELP:
            WinHelp(((LPHELPINFO)lParam)->hItemHandle, NULL, HELP_WM_HELP, (DWORD_PTR)rgIdhCustom);
            break;
    } // switch
    
    return FALSE;
    
} // FCustomDlgProc

//
// FGetCustomPropFromDlg
//
// Purpose: To get a custom property from the dialog.
//          I.e. the user hit Add/Modify.
//
BOOL FGetCustomPropFromDlg(LPALLOBJS lpallobjs, HWND hDlg)
{
    UDTYPES udtype;
    NUM dbl;
    LPVOID lpv;
    int iItemT;
    LPTSTR lpstzName;
    LPVOID lpvSaveAsDword;
    DWORD cch;
    BOOL f;
    
    lpstzName = NULL;
    cch = 0;
    
    if (FAllocAndGetValLpstz (hDlg, IDD_CUSTOM_NAME, &glpstzName))
    {
        LPUDPROP lpudp;
        
#ifdef __CUSTOM_LINK_ENABLED__
        lpallobjs->CDP_fLink = SendDlgItemMessage(hDlg, IDD_CUSTOM_LINK, BM_GETCHECK, 0, 0);
#else
        lpallobjs->CDP_fLink = FALSE; // just to be sure.
#endif __CUSTOM_LINK_ENABLED__
        Assert(lpallobjs->CDP_fLink == TRUE || lpallobjs->CDP_fLink == FALSE);
        
        // HACK: If the user enters a name that is already
        // a property name, the default action of the object
        // is to replace the data, treating it as an update.
        // This will cause there to be 2 names in the listview
        // though unless we just update the original one.  So, first
        // see if the new name is in the list already, and if
        // it is, find it in the listview and set up to update it.
        
        lpudp = LpudpropFindMatchingName (lpallobjs->lpUDObj, (LPTSTR)PSTR (glpstzName));
        if (lpudp != NULL)
        {
            LV_FINDINFO lvfi;
            
            lvfi.flags = LVFI_STRING;
            lvfi.psz = (LPTSTR) PSTR (glpstzName);
            iItemT = ListView_FindItem (lpallobjs->CDP_hWndCustomLV, -1, &lvfi);
            
            // If the property is being modified and the link
            // box is not checked, we need to remove the link
            // data and the IMoniker from the object.
            
            //           if (!lpallobjs->CDP_fLink)
            //                {
            //                FUserDefAddProp (lpallobjs->lpUDObj, PSTR (glpstzName), NULL,
            //                                 wUDlpsz, TRUE, FALSE, FALSE);
            //                FUserDefAddProp (lpallobjs->lpUDObj, PSTR (glpstzName), NULL,
            //                                 wUDlpsz, FALSE, FALSE, TRUE);
            //                }
        }
        else
            iItemT = -1;
        
        // Let's get the type, since this might be a MODIFY case
        
        lpallobjs->CDP_iszType = (int)SendMessage(lpallobjs->CDP_hWndType, CB_GETCURSEL,0, 0);
        
        // If the user has checked the link box, then the value
        // must come from the client.
        
        if (lpallobjs->CDP_fLink)
        {
            DWORD irg;
            
            // Get the link name from the combobox, and store
            // the link name and static value.
            
            irg = (int)SendMessage (lpallobjs->CDP_hWndLinkVal, CB_GETCURSEL, 0, 0);
            
            Assert ((lpallobjs->lpfnDwQueryLinkData != NULL));
            Assert (((irg < lpallobjs->CDP_cLinks) && (irg >= 0)));
            
            cch = (DWORD)SendMessage (lpallobjs->CDP_hWndLinkVal, CB_GETLBTEXTLEN, irg, 0)+1; // Include the null-terminator
            
            if (!FAllocString (&lpstzName, cch))
                return(FALSE);
            
            SendMessage (lpallobjs->CDP_hWndLinkVal, CB_GETLBTEXT, irg, (LPARAM) PSTR (lpstzName));
            
            // Set up the static type and value for display
            // in the listbox
            
            udtype = (UDTYPES) (*lpallobjs->lpfnDwQueryLinkData) (QLD_LINKTYPE, irg, NULL, (LPTSTR)PSTR (lpstzName));
            (*lpallobjs->lpfnDwQueryLinkData) (QLD_LINKVAL, irg, &lpv, (LPTSTR)PSTR (lpstzName));
            
            //
            // HACK alert
            //
            // We want lpv to point to the value, not to be overloaded in the case of a dword or bool.
            //
            
            if ((udtype == wUDdw) || (udtype == wUDbool))
            {
                lpvSaveAsDword = lpv; // Really a DWORD
                lpv = &lpvSaveAsDword;
            }
            
            // Add the link name itself to the object
            
            //         FUserDefAddProp (lpallobjs->lpUDObj, PSTR (glpstzName), PSTR (lpstzName),
            //                          wUDlpsz, TRUE, FALSE, FALSE);
            
        }   // if (lpallobjs->CDP_fLink)
        
        else
        {
            if (lpallobjs->CDP_iszType != iszBOOL)
            {
                if (!FAllocAndGetValLpstz (hDlg, IDD_CUSTOM_VALUE, &glpstzValue))
                    return(FALSE);
            }
            
            // Convert the type in the combobox to a UDTYPES
            
            switch (lpallobjs->CDP_iszType)
            {
            case iszTEXT :
                udtype = wUDlpsz;
                (LPTSTR) lpv = (LPTSTR)PSTR (glpstzValue);
                break;
                
            case iszNUM :
                udtype = UdtypesGetNumberType (glpstzValue, &dbl,
                    ((LPUDINFO)lpallobjs->lpUDObj->m_lpData)->lpfnFSzToNum);
                switch (udtype)
                {
                case wUDdw :
                    lpv = (DWORD *) &dbl;
                    break;
                    
                case wUDfloat :
                    (NUM *) lpv = &dbl;
                    break;
                    
                default :
                    (LPTSTR) lpv = (LPTSTR)PSTR (glpstzValue);
                    
                    // If the user doesn't want to convert the value to text, they can press "Cancel" and try again.
                    
                    if (FDisplayConversionWarning (hDlg))
                    {
                        SetFocus(lpallobjs->CDP_hWndType);
                        return(FALSE);
                    }
                    udtype = wUDlpsz;
                    
                }   // switch (udtype)
                break;
                
                case iszDATE :
                    
                    if (FConvertDate (glpstzValue, (LPFILETIME) &dbl))
                    {
                        udtype = wUDdate;
                        (NUM *) lpv = &dbl;
                    }
                    else
                    {
                        udtype = wUDlpsz;
                        (LPTSTR) lpv = (LPTSTR)PSTR (glpstzValue);
                        // If the user doesn't want to convert the value to text, they can press "Cancel" and try again.
                        if (FDisplayConversionWarning (hDlg))
                        {
                            SetFocus(lpallobjs->CDP_hWndType);
                            return(FALSE);
                        }
                    }
                    break;
                    
                case iszBOOL :
                    {
                        udtype = wUDbool;
                        f = (BOOL)(SendMessage (lpallobjs->CDP_hWndBoolTrue, BM_GETSTATE, 0, 0) & BST_CHECKED);
                        lpv = &f;
                        break;
                    }
                    
                default :
                    AssertSz (0,TEXT("IDD_CUSTOM_TYPE combobox is whacked!"));
                    udtype = wUDinvalid;
                    
            }   // switch (lpallobjs->CDP_iszType)
            
        }   // if (lpallobjs->CDP_fLink) ... else
        
        
        // If we got valid input, add the property to the object
        // and listbox.
        
        if (udtype != wUDinvalid)
        {
            // The PropVariant created when we add this property.
            LPPROPVARIANT lppropvar = NULL;
            
            // The link data (link name itself) would have
            // been stored above if the property was a link.
            // This stores the static value that will eventually
            // appear in the list view.
            
            lppropvar = LppropvarUserDefAddProp (lpallobjs->lpUDObj, glpstzName, lpv, udtype,
                (lpstzName != NULL) ? lpstzName : NULL,
                (lpstzName != NULL) ? TRUE : FALSE, FALSE);
            
            // HACK alert
            //
            // Here we want lpv be overloaded in the case of a dword or bool, since
            // AddUDPropToListView calls WUdtypeToSz which assumes lpv is overloaded.
            //
            
            if ((udtype == wUDdw) || (udtype == wUDbool))
            {
                lpv = *(LPVOID *)lpv;
            }
            
            AddUDPropToListView (lpallobjs->lpUDObj, lpallobjs->CDP_hWndCustomLV, (LPTSTR)PSTR (glpstzName), lppropvar, iItemT, lpallobjs->CDP_fLink, fTrue, fTrue);
            
            // For links, dealloc the buffer.
            
            if (lpallobjs->CDP_fLink)
                DeallocValue (&lpv, udtype);
            
            // Clear out the edit fields and disable the Add button again
            SetCustomDlgDefButton(hDlg, gOKButtonID);
            EnableWindow (lpallobjs->CDP_hWndAdd, FALSE);
            SendMessage(lpallobjs->CDP_hWndName, CB_SETEDITSEL, 0, MAKELPARAM(0,-1));     // Select entire string
            SendMessage(lpallobjs->CDP_hWndName, WM_CLEAR, 0, 0);
            EnableWindow (lpallobjs->CDP_hWndDelete, FALSE);
            // See bug 213
            //                    if (fLink)
            //                    {
            //                      fLink = !fLink;
            //                      SendDlgItemMessage (hDlg, IDD_CUSTOM_LINK, BM_SETCHECK, (WPARAM) fLink, 0);
            //                    }
            FSwapControls (lpallobjs->CDP_hWndVal, lpallobjs->CDP_hWndLinkVal,
                lpallobjs->CDP_hWndBoolTrue, lpallobjs->CDP_hWndBoolFalse,
                lpallobjs->CDP_hWndGroup, lpallobjs->CDP_hWndType,
                lpallobjs->CDP_hWndValText, lpallobjs->CDP_fLink, lpallobjs->CDP_iszType == iszBOOL);
            FSetupAddButton (lpallobjs->CDP_iszType, lpallobjs->CDP_fLink, &lpallobjs->CDP_fAdd, lpallobjs->CDP_hWndAdd, lpallobjs->CDP_hWndVal, lpallobjs->CDP_hWndName, hDlg);
            
            // wUDbool doesn't use the edit control....
            if (lpallobjs->CDP_iszType != iszBOOL)
                ClearEditControl (lpallobjs->CDP_hWndVal, 0);
            
        }   // if (udtype != wUDinvalid)
        
        SendDlgItemMessage(hDlg, IDD_CUSTOM_TYPE, CB_SETCURSEL, lpallobjs->CDP_iszType,0);
        SetFocus(lpallobjs->CDP_hWndName);
        //          lpallobjs->fPropDlgChanged = TRUE;
        if (lpstzName != NULL)
            VFreeMemP(lpstzName, CBTSTR (lpstzName));
        return(TRUE);
    }
    return(FALSE);
    
}

/////////////////////////////////////////////////////////////////////////
//
// SetCustomDlgDefButton
//
// Set the new default button
//
/////////////////////////////////////////////////////////////////////////
VOID SetCustomDlgDefButton(HWND hDlg, int IDNew)
{
    int IDOld;
    
    if ((IDOld = LOWORD(SendMessage(hDlg, DM_GETDEFID, 0L, 0L))) != IDNew)
    {
        // Set the new default push button's control ID.
        SendMessage(hDlg, DM_SETDEFID, IDNew, 0L);
        
        // Set the new style.
        SendDlgItemMessage(hDlg, IDNew, BM_SETSTYLE, BS_DEFPUSHBUTTON, MAKELPARAM(TRUE,0));
        
        SendDlgItemMessage(hDlg, IDOld, BM_SETSTYLE, BS_PUSHBUTTON, MAKELPARAM(TRUE,0));
    }
}

#ifdef OFFICE_96
////////////////////////////////////////////////////////////////////////////////
//
// ListViewCompareFunc
//
// Purpose:
//    Compares two items in a listview
//    We only sort the name column.
//
// Returns
//    A negative value if item 1 should come before item 2
//    A positive value if item 1 should come after item 2
//    Zero if the two items are equivalent
//
////////////////////////////////////////////////////////////////////////////////
int CALLBACK ListViewCompareFunc
(
 LPARAM lParam1,      // lParam of the LV_ITEM struct  (property name)
 LPARAM lParam2,      // lParam of the LV_ITEM struct  (property name)
 LPARAM lParamSort)   // Index of column to sore
{
    return(lstrcmp((LPTSTR)lParam1, (LPTSTR)lParam2));
}
#endif

////////////////////////////////////////////////////////////////////////////////
//
// PrintTimeInDlg
//
// Purpose:
//  Prints the locale-specific time representation in control in the dialog.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _WIN2000_DOCPROP_
void PASCAL
PrintTimeInDlg (
                HWND hDlg,                           // Dialog handle
                DWORD dwId,                          // Control id
                LPFILETIME lpft)                     // The time
{
    SYSTEMTIME st;
    TCHAR szBuf[80], szTmp[64];
    const TCHAR *c_szSpace = TEXT(" ");
    
    if ((lpft != NULL) && (lpft->dwLowDateTime != 0) && (lpft->dwHighDateTime != 0))
    {
        FILETIME ft;
        
        FileTimeToLocalFileTime(lpft, &ft);   // get in local time
        FileTimeToSystemTime(&ft, &st);
        
        GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, szBuf,ARRAYSIZE(szBuf));

        // BUGBUG: this doesn't handle international: time may need to come first
        // don't bother with the time if it is NULL
        if (st.wHour || st.wMinute || st.wSecond)
        {
            lstrcat(szBuf, c_szSpace);
            GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, szTmp, ARRAYSIZE(szTmp));
            lstrcat(szBuf, szTmp);
        }
        
        SetDlgItemText(hDlg, dwId, szBuf);
    }
} // PrintTimeInDlg
#endif //_WIN2000_DOCPROP_

////////////////////////////////////////////////////////////////////////////////
//
// PrintEditTimeInDlg
//
// Purpose:
//  Prints the total edit time in the dialog.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef _WIN2000_DOCPROP_
void PASCAL PrintEditTimeInDlg (
                                HWND hDlg,                           // Dialog handle
                                LPFILETIME lpft)                     // The time
{
    TCHAR sz[100];
    
    // Remember that the 64 bit number in lpft is units of 100ns
    VFtToSz(lpft, sz, ARRAYSIZE(sz), TRUE);
    SetDlgItemText(hDlg, IDD_STATISTICS_TOTALEDIT, sz);
    
} // PrintEditTimeInDlg
#endif _WIN2000_DOCPROP_



////////////////////////////////////////////////////////////////////////////////
//
//  SetEditValLpsz
//
//  Purpose:
//      If this PropVariant holds a LPTSTR, it is written
//      to the caller-specified Edit Control.  If it is not
//      an LPTSTR, it is not treated as an error (it might
//      simply be a VT_EMPTY ... a non-existent property).
//
////////////////////////////////////////////////////////////////////////////////
#ifndef _WIN2000_DOCPROP_
void PASCAL SetEditValLpsz(
                           LPPROPVARIANT    lppropvar,          // PropVariant
                           HWND             hDlg,               // Dialog handle
                           DWORD            dwID )              // Edit control ID
{
    
    if (lppropvar->vt == VT_LPTSTR)
    {
        SendDlgItemMessage
            (hDlg, dwID, WM_SETTEXT, 0, (LPARAM) lppropvar->pszVal);
        
        PropVariantClear( lppropvar );
    }
}
#endif

////////////////////////////////////////////////////////////////////////////////
//
//  GetEditValLpsz
//
//  Purpose:
//      Reads the string from the caller-specified edit
//      control, and loads it into a PropVariant.
//      If the Edit control says that it has not been
//      modified, then we don't read anything, and 
//      return FALSE.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef _WIN2000_DOCPROP_
BOOL PASCAL GetEditValLpsz(
                           LPPROPVARIANT lppropvar,             // Sum info PropVariants
                           HWND hDlg,                           // Dialog handle
                           DWORD dwId)                          // Edit control id
{
    BOOL  fChanged = FALSE;
    DWORD cb, cch;
    
    // Did the data change?
    
    if ((BOOL) SendDlgItemMessage (hDlg, dwId, EM_GETMODIFY, 0, 0))
    {
        LPTSTR tszNew;
        
        // Yes, it changed.  Ask the Edit control how big the
        // string is.
        
        cch = (DWORD)SendDlgItemMessage (hDlg, dwId, WM_GETTEXTLENGTH, 0, 0);
        
        cb = ( cch + 1 ) * sizeof (TCHAR);  // Includes the NULL.
        
        // Allocate a new string for this PropVariant, and set 
        // the VT.
        
        if ( !(tszNew = CoTaskMemAlloc (cb)))
        {
            goto Exit;
        }
        
        PropVariantClear( lppropvar );
        
        lppropvar->vt = VT_LPTSTR;
        (LPTSTR) lppropvar->pszVal = tszNew;
        
        // Get the string from the edit control into the buffer
        // (the size of the buffer in characters is cch+1, including
        // the NULL).
        
        SendDlgItemMessage (hDlg, dwId, WM_GETTEXT, (WPARAM) cch+1,
            (LPARAM) lppropvar->pszVal);
        
        fChanged = TRUE;
        
    }   // if ((BOOL) SendDlgItemMessage (hDlg, dwId, EM_GETMODIFY, 0, 0))
    
    
    //  ----
    //  Exit
    //  ----
    
Exit:
    
    return (fChanged);
    
} // GetEditValLpsz
#endif //_WIN2000_DOCPROP_

////////////////////////////////////////////////////////////////////////////////
//
// FAllocAndGetValLpstz
//
// Purpose:
//  Gets the value from the edit box into the local buffer.
//
////////////////////////////////////////////////////////////////////////////////
BOOL PASCAL FAllocAndGetValLpstz (
                                  HWND hDlg,                           // Handle of dialog control is in
                                  DWORD dwId,                          // Id of control
                                  LPTSTR *lplpstz)                      // Buffer
{
    DWORD cch;
    
    cch = (DWORD)SendDlgItemMessage (hDlg, dwId, WM_GETTEXTLENGTH, 0, 0);
    cch++;
    
    if (FAllocString (lplpstz, cch))
    {
        // Get the entry.  Remember to null-terminate it.
        cch = (DWORD)SendDlgItemMessage (hDlg, dwId, WM_GETTEXT, cch, (LPARAM) PSTR (*lplpstz));
        ((LPTSTR)(PSTR (*lplpstz)))[cch] = TEXT('\0');
        
        return TRUE;
    }
    
    return FALSE;
    
} // FAllocAndGetValLpstz

////////////////////////////////////////////////////////////////////////////////
//
// FAllocString
//
// Purpose:
//  Allocates a string big enough to to hold cch char's.  Only allocates if needed.
//
////////////////////////////////////////////////////////////////////////////////
BOOL PASCAL FAllocString (
                          LPTSTR *lplpstz,
                          DWORD cch)
{
    // Figure out how many bytes we need to allocate.
    
    DWORD cbNew = (cch * sizeof(TCHAR));
    
    // And how many bytes we need to free.
    
    DWORD cbOld = *lplpstz == NULL
        ? 0
        : (CchTszLen (*lplpstz) + 1) * sizeof(TCHAR);
    
    
    // If we need to free or allocate data.
    
    if (*lplpstz == NULL || cbNew > cbOld)
    {
        LPTSTR lpszNew;
        
        // Allocate the new data.
        
        lpszNew = PvMemAlloc(cbNew);
        if (lpszNew == NULL)
        {
            return FALSE;
        }
        
        // Free the old data.
        
        if (*lplpstz != NULL)
            VFreeMemP(*lplpstz, cbOld);
        
        *lplpstz = lpszNew;
        
    }
    
    // Make this a valid (empty) string.
    
    **lplpstz = TEXT('\0');
    
    return TRUE;
    
} // FAllocString


////////////////////////////////////////////////////////////////////////////////
//
// ClearEditControl
//
// Purpose:
//  Clears any text from an edit control
//
////////////////////////////////////////////////////////////////////////////////
void PASCAL
ClearEditControl
(HWND hDlg,                           // Dialog handle
 DWORD dwId)                          // Id of edit control
{
    // Really cheesey.  Clear the edit control by selecting
    // everything then clearing the selection
    if (dwId == 0)
    {
        SendMessage (hDlg, EM_SETSEL, 0, -1);
        SendMessage (hDlg, WM_CLEAR, 0, 0);
    }
    else
    {
        SendDlgItemMessage (hDlg, dwId, EM_SETSEL, 0, -1);
        SendDlgItemMessage (hDlg, dwId, WM_CLEAR, 0, 0);
    }
    
} // ClearEditControl

////////////////////////////////////////////////////////////////////////////////
//
// UdtypesGetNumberType
//
// Purpose:
//  Gets the number type from the string and returns the value, either
//  a float or dword in numval.
//
////////////////////////////////////////////////////////////////////////////////
UDTYPES PASCAL
UdtypesGetNumberType
(LPTSTR lpstz,                                   // String containing the number
 NUM *lpnumval,                              // The value of the number
 BOOL (*lpfnFSzToNum)(NUM *, LPTSTR))   // Sz To Num routine, can be null
{
    TCHAR *pc;
    
    errno = 0;
    *(DWORD *) lpnumval = strtol ((LPTSTR)PSTR (lpstz), &pc, 10);
    
    if ((!errno) && (*pc == TEXT('\0')))
        return wUDdw;
    
    // Try doing a float conversion if int fails
    
    if (lpfnFSzToNum != NULL)
    {
        if ((*lpfnFSzToNum)(lpnumval, (LPTSTR)PSTR(lpstz)))
            return wUDfloat;
    }
    
    return wUDinvalid;
    
} // UdtypesGetNumberType


////////////////////////////////////////////////////////////////////////////////
//
// YearIndexFromShortDateFormat
//
// 
//  Determines the zero-based position index of the year component
//  of a textual representation of the date based on the specified date format.
//  This value may be used as the iYear arg to ScanDateNums function.
//
////////////////////////////////////////////////////////////////////////////////
int YearIndexFromShortDateFormat( TCHAR chFmt )
{
    switch( chFmt )
    {
        case MMDDYY:
        case DDMMYY:
            return 2;
        case YYMMDD:
            return 0;
    }
    return -1;
}

////////////////////////////////////////////////////////////////////////////////
//
//  IsGregorian
//
//  Purpose:
//      Reports whether the specified calendar is a gregorian calendar.
//
////////////////////////////////////////////////////////////////////////////////
BOOL IsGregorian( CALID calid )
{
    switch (calid)
    {
        case CAL_GREGORIAN:
        case CAL_GREGORIAN_US:
        case CAL_GREGORIAN_ME_FRENCH:
        case CAL_GREGORIAN_ARABIC:
        case CAL_GREGORIAN_XLIT_ENGLISH:
        case CAL_GREGORIAN_XLIT_FRENCH:
            return TRUE;

        //  these are non-gregorian:
        //case CAL_JAPAN
        //case CAL_TAIWAN
        //case CAL_KOREA
        //case CAL_HIJRI
        //case CAL_THAI
        //case CAL_HEBREW
    }
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
//
//  GregorianYearFromAbbreviatedYear
//
//  Purpose:
//      Based on current locale settings, calculates the year corresponding to the
//  specified 1- or 2-digit abbreviated value.
//
////////////////////////////////////////////////////////////////////////////////
int GregorianYearFromAbbreviatedYear( LCID lcid, CALID calid, int nAbbreviatedYear )
{
    TCHAR szData[16];   
    LONG  nYearHigh = -1;
    int   nBaseCentury;
    int   nYearInCentury = 0;

    //  We're handling two-digit values for gregorian calendars only
    if (nAbbreviatedYear < 100)
    {
        // BUGBUG: why do we not support non-gregorian?
        if( !IsGregorian( calid )
            || !GetCalendarInfo( lcid, calid, CAL_ITWODIGITYEARMAX|CAL_RETURN_NUMBER,
                              NULL, 0, &nYearHigh ) )
        {
            // In the absence of a default, use 2029 as the cutoff, just like monthcal.
            nYearHigh = 2029;
        }

        //
        //  Copy the century of nYearHigh into nAbbreviatedYear.
        //
        nAbbreviatedYear += (nYearHigh - nYearHigh % 100);
        //
        //  If it exceeds the max, then drop to previous century.
        //
        if (nAbbreviatedYear > nYearHigh)
            nAbbreviatedYear -= 100;
    }

    return nAbbreviatedYear;
}

////////////////////////////////////////////////////////////////////////////////
//
// FConvertDate
//
// Purpose:
//  Converts the given string to a date.
//
////////////////////////////////////////////////////////////////////////////////
BOOL PASCAL FConvertDate
(LPTSTR lpstz,                         // String having the date
 LPFILETIME lpft)                     // The date in FILETIME format
{
    
    FILETIME ft;
    SYSTEMTIME st;
    TCHAR szSep[3];
    TCHAR szFmt[10];
    TCHAR szCalID[8];
    unsigned int ai[3];
    int   iYear =-1; // index of ai member that represents the year value
    CALID calid;
    TCHAR szDate[256];
    TCHAR szMonth[256];
    TCHAR *pch;
    TCHAR *pchT;
    DWORD cch;
    DWORD i;


    if (!(GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_IDATE, szFmt, ARRAYSIZE(szFmt))) ||
        !(GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_SDATE, szSep, ARRAYSIZE(szSep))) ||
        !(GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_ICALENDARTYPE, szCalID, ARRAYSIZE(szCalID))) )
        return FALSE;

    iYear = YearIndexFromShortDateFormat(szFmt[0]);
    
    // Augh!  It's an stz so we need to pass the DWORDs at the start
    if (!ScanDateNums(lpstz, szSep, ai, sizeof(ai)/sizeof(unsigned int),iYear))
    {
        // Could be that the string contains the short version of the month, e.g. 03-Mar-95
        PbMemCopy(szDate, lpstz, CBTSTR(lpstz)); 
        pch = szDate;
        
        // Let's get to the first character of the month, if there is one
        while((isdigit(*pch) || (*pch == szSep[0])) && (*pch != 0))
            ++pch;
        
        // If we got to the end of the string, there really was an error
        if (*pch == 0)
            return(FALSE);
        
        // Let's find the length of the month string
        pchT = pch+1;
        while ((*pchT != szSep[0]) && (*pchT != 0))
            ++pchT;
        cch = (DWORD)(pchT - pch);
        
        // Loop through all the months and see if we match one
        // There can be 13 months
        for (i = 1; i <= 13; ++i)
        {
            if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVMONTHNAME1+i-1,
                szMonth, ARRAYSIZE(szMonth)))
                return(FALSE);
            
            if (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH,
                pch, cch, szMonth, CchTszLen(szMonth)) == 2)
                break;
        }
        
        if (i > 13)
            return(FALSE);
        
        // We found the month. wsprintf zero-terminates
        cch = wsprintf(pch, TEXT("%u"), i);
        pch += cch;
        while (*pch++ = *(pch+1));
        
        // Try and convert again
        if (!ScanDateNums(szDate, szSep, ai, 3, iYear))
            return(FALSE);
        
    } // if (!ScanDateNums(lpstz, szSep, ai, 3))
    
    FillBuf (&st, 0, sizeof(st));
    
    switch (szFmt[0])
    {
    case MMDDYY:
        st.wMonth = (WORD)ai[0];
        st.wDay = (WORD)ai[1];
        st.wYear = (WORD)ai[2];
        break;
    case DDMMYY:
        st.wDay = (WORD)ai[0];
        st.wMonth = (WORD)ai[1];
        st.wYear = (WORD)ai[2];
        break;
    case YYMMDD:
        st.wYear = (WORD)ai[0];
        st.wMonth = (WORD)ai[1];
        st.wDay = (WORD)ai[2];
        break;
    default:
        return FALSE;
    }
    
    if (st.wYear < ONECENTURY)
    {
        calid = strtol( szCalID, NULL, 10 );
        st.wYear = (WORD)GregorianYearFromAbbreviatedYear( 
            LOCALE_USER_DEFAULT, calid, st.wYear );
    }
    
    if (!SystemTimeToFileTime (&st, &ft))
        return(FALSE);

    return(LocalFileTimeToFileTime(&ft, lpft));
    
} // FConvertDate


////////////////////////////////////////////////////////////////////////////////
//
// PopulateUDListView
//
// Purpose:
//  Populates the entire ListView with the User-defined properties
//  in the given object.
//
////////////////////////////////////////////////////////////////////////////////
void PASCAL
PopulateUDListView
(HWND hWnd,                   // Handle of list view window
 LPUDOBJ lpUDObj)             // UD Prop object
{
    LPUDITER lpudi;
    LPPROPVARIANT lppropvar;
    BOOL fLink;
    BOOL fLinkInvalid;
    
    // Iterate through the list of user-defined properties, adding each
    // one to the listview.
    
    for( lpudi = LpudiUserDefCreateIterator (lpUDObj);
    FUserDefIteratorValid (lpudi);
    FUserDefIteratorNext (lpudi)
        )
    {
        // Get the name of this property.
        
        LPTSTR tszPropertyName
            = LpszUserDefIteratorName( lpudi, 1, (LPTSTR) UD_PTRWIZARD );
        
        // If the property has no name, or the name indicates that it
        // is a hidden property, then move on to the next property.
        
        if( tszPropertyName == NULL
            ||
            *tszPropertyName == HIDDENPREFIX )
        {
            continue;
        }
        
        lppropvar = LppropvarUserDefGetIteratorVal (lpudi, &fLink, &fLinkInvalid);
        if (lppropvar == NULL)
            return;
        
        // If this isn't a supported type, don't display it.
        if( !ISUDTYPE(lppropvar->vt) )
            continue;
        
        
#ifdef SHELL
        //
        // In the Shell, we want all links to show up as invalid, so set that here...
        //
        
        fLinkInvalid = TRUE;
#endif
        
        AddUDPropToListView (lpUDObj, hWnd, LpszUserDefIteratorName (lpudi, 1, (TCHAR *) UD_PTRWIZARD),
            lppropvar, -1, fLink, fLinkInvalid, FALSE);
        
    } // for( lpudi = LpudiUserDefCreateIterator (lpUDObj); ...
    
    FUserDefDestroyIterator (&lpudi);
    
} // PopulateUDListView


////////////////////////////////////////////////////////////////////////////////
//
// AddUDPropToListView
//
// Purpose:
//  Adds the given property to the list view or updates an existing one
//  if iItem >= 0
//
////////////////////////////////////////////////////////////////////////////////
void PASCAL AddUDPropToListView (
                                 LPUDOBJ lpUDObj,
                                 HWND hWnd,                   // Handle of list view
                                 LPTSTR lpszName,             // Name of property
                                 LPPROPVARIANT lppropvar,     // The property value.
                                 int iItem,                   // Index to add item at
                                 BOOL fLink,                  // Indicates the value is a link
                                 BOOL fLinkInvalid,           // Is the link invalid?
                                 BOOL fMakeVisible)           // Should the property be forced to be visible
{
    LV_ITEM lvi;
    TCHAR sz[BUFMAX];
    WORD irg;
    BOOL fSuccess;
    BOOL fUpdate;
    
    // If iItem >= 0, then the item should be updated, otherwise,
    // it should be added.
    
    if (fUpdate = (iItem >= 0))
    {
        lvi.iItem = iItem;
        if (fLink)
            lvi.iImage = (fLinkInvalid) ? giInvLinkIcon : giLinkIcon;
        else
            lvi.iImage = giBlankIcon;
        
        lvi.mask = LVIF_IMAGE;
        lvi.iSubItem = iszNAME;
        
        fSuccess = ListView_SetItem (hWnd, &lvi);
        Assert (fSuccess);           // We don't *really* care, just want to know when it happens
    }
    else
    {
        // This always adds to the end of the list....
        lvi.iItem = ListView_GetItemCount (hWnd);
        
        // First add the label to the list
        lvi.iSubItem = iszNAME;
        lvi.pszText = lpszName;
        
        if (fLink)
            lvi.iImage = (fLinkInvalid) ? giInvLinkIcon : giLinkIcon;
        else
            lvi.iImage = giBlankIcon;
        lvi.mask = LVIF_TEXT | LVIF_IMAGE;
        
        lvi.iItem = ListView_InsertItem (hWnd, &lvi);
        if (lvi.iItem == 0)
            ListView_SetItemState(hWnd, 0, LVIS_FOCUSED, LVIS_FOCUSED);
    }
    
    // Convert the data to a string and print it
    
    lvi.mask = LVIF_TEXT;
    irg = WUdtypeToSz (lppropvar, sz, BUFMAX, ((LPUDINFO)lpUDObj->m_lpData)->lpfnFNumToSz);
    lvi.pszText = sz;
    lvi.iSubItem = iszVAL;
    fSuccess = ListView_SetItem (hWnd, &lvi);
    Assert (fSuccess);           // We don't *really* care, just want to know when it happens
    
    // Put the type in the listview
    
    lvi.iSubItem = iszTYPE;
    lvi.pszText = (LPTSTR) rgszTypes[irg];
    fSuccess = ListView_SetItem (hWnd, &lvi);
    Assert (fSuccess);           // We don't *really* care, just want to know when it happens
    if (fMakeVisible)
    {
        fSuccess = ListView_EnsureVisible(hWnd, lvi.iItem, FALSE);
        Assert (fSuccess);           // We don't *really* care, just want to know when it happens
    }
    
    //  if (fUpdate)
    //  {
    //    ListView_RedrawItems (hWnd, lvi.iItem, lvi.iItem);
    //    UpdateWindow (hWnd);
    //  }
    
} // AddUDPropToListView


////////////////////////////////////////////////////////////////////////////////
//
// AddItemToListView
//
// Purpose:
//  Adds the given string and number to the end of a listview
//
////////////////////////////////////////////////////////////////////////////////
void PASCAL
AddItemToListView
(HWND hWnd,                           // ListView handle
 DWORD_PTR dw,                         // Number to add
 const TCHAR *lpsz,                    // String to add
 BOOL fString)                        // Indicates if dw is actually a string
{
    LV_ITEM lvi;
    TCHAR sz[BUFMAX];
    BOOL fSuccess;
    
    if (!fString)
        // _itoa (dw, sz, BASE10);
        wsprintf(sz, TEXT("%lu"), dw);
    
    // This always adds to the end of the list....
    lvi.iItem = ListView_GetItemCount (hWnd);
    
    // First add the label to the list
    lvi.mask = LVIF_TEXT;
    lvi.iSubItem = iszNAME;
    lvi.pszText = (LPTSTR) lpsz;
    lvi.iItem = ListView_InsertItem (hWnd, &lvi);
    if (lvi.iItem == 0)     // Adding the 1st item
        ListView_SetItemState(hWnd, 0, LVIS_FOCUSED, LVIS_FOCUSED);
    
    Assert ((lvi.iItem != -1));
    
    // Then add the value
    lvi.mask = LVIF_TEXT;
    lvi.iSubItem = iszVAL;
    lvi.pszText = (fString) ? (LPTSTR) dw : sz;
    fSuccess = ListView_SetItem (hWnd, &lvi);
    
    Assert (fSuccess);
    
} // AddItemToListView


////////////////////////////////////////////////////////////////////////////////
//
// InitListView
//
// Purpose:
//  Initializes a list view control
//
////////////////////////////////////////////////////////////////////////////////
void PASCAL
InitListView
(HWND hWndLV,                   // Handle of parent dialog
 int irgLast,                 // Index of last column in array
 TCHAR rgsz[][SHORTBUFMAX],    // Array of column headings
 BOOL fImageList)              // Should the listview have an image list
{
    HICON hIcon;
    RECT rect;
    HIMAGELIST hImlS;
    LV_COLUMN lvc;
    int irg;
    
    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    
    // Initially force all columns to be the same size & fill the control.
    GetClientRect(hWndLV, &rect);
    // Subtract fudge factor
    lvc.cx = (rect.right-rect.left)/(irgLast+1)-(GetSystemMetrics(SM_CXVSCROLL)/(irgLast+1));
    
    // Add in all the columns.
    for (irg = 0; irg <= irgLast; irg++)
    {
        lvc.pszText = rgsz[irg];
        lvc.iSubItem = irg;
        ListView_InsertColumn (hWndLV, irg, &lvc);
    }
    
    if (!fImageList)
        return;
    
    hIcon = LoadIcon (g_hmodThisDll, MAKEINTRESOURCE (IDD_BLANK_ICON));
    if (hIcon != NULL)
    {
        hImlS = MsoImageList_Create (16, 16, TRUE, ICONSMAX, 0);
        ListView_SetImageList (hWndLV, hImlS, LVSIL_SMALL);
        giBlankIcon = MsoImageList_ReplaceIcon (hImlS, -1, hIcon);
        Assert ((giBlankIcon != -1));
    }
    
} // InitListView


////////////////////////////////////////////////////////////////////////////////
//
// FSwapControls
//
// Purpose:
//  Swaps the controls needed to display link info.
//
////////////////////////////////////////////////////////////////////////////////
BOOL PASCAL
FSwapControls
(HWND hWndVal,                        // Handle of Value window
 HWND hWndLinkVal,                    // Handle of Link Value Combo box
 HWND hWndBoolTrue,                   // Handle of True radio button
 HWND hWndBoolFalse,                  // Handle of False radio button
 HWND hWndGroup,                      // Handle of Group box
 HWND hWndType,                       // Handle of Type window
 HWND hWndValText,
 BOOL fLink,                          // Flag indicating a link
 BOOL fBool)                          // Flag indicating a bool
{
    if (fLink)
    {
        SendMessage (hWndValText, WM_SETTEXT, 0, (LPARAM) rgszValue[iszSOURCE]);
        ShowWindow (hWndVal, SW_HIDE);
        ShowWindow (hWndBoolTrue, SW_HIDE);
        ShowWindow (hWndBoolFalse, SW_HIDE);
        ShowWindow (hWndGroup, SW_HIDE);
        ShowWindow (hWndLinkVal, SW_SHOW);
        EnableWindow (hWndType, FALSE);
        ClearEditControl (hWndVal, 0);
    }
    else
    {
        SendMessage (hWndValText, WM_SETTEXT, 0, (LPARAM) rgszValue[iszVALUE]);
        ShowWindow (hWndLinkVal, SW_HIDE);
        EnableWindow (hWndType, TRUE);
        
        if (fBool)
        {
            ShowWindow (hWndVal, SW_HIDE);
            ShowWindow (hWndBoolTrue, SW_SHOW);
            ShowWindow (hWndBoolFalse, SW_SHOW);
            ShowWindow (hWndGroup, SW_SHOW);
            SendMessage (hWndBoolTrue, BM_SETCHECK, (WPARAM) CHECKED, 0);
            SendMessage (hWndBoolFalse, BM_SETCHECK, (WPARAM) CLEAR, 0);
            SendMessage (hWndType, CB_SETCURSEL, iszBOOL, 0);
            ClearEditControl (hWndVal, 0);
        }
        else
        {
            ShowWindow (hWndVal, SW_SHOW);
            EnableWindow(hWndVal, TRUE);
            ShowWindow (hWndBoolTrue, SW_HIDE);
            ShowWindow (hWndBoolFalse, SW_HIDE);
            ShowWindow (hWndGroup, SW_HIDE);
            SendMessage (hWndType, CB_SETCURSEL, iszTEXT, 0);
        }
    }
    
    return TRUE;
    
} // FSwapControls

////////////////////////////////////////////////////////////////////////////////
//
// PopulateControls
//
// Purpose:
//  Populates the edit controls with the appropriate date from the object
//
////////////////////////////////////////////////////////////////////////////////
VOID PASCAL PopulateControls (
                              LPUDOBJ lpUDObj,                     // Pointer to object
                              LPTSTR szName,                        // Name of the item to populate controls with
                              DWORD cLinks,                        // Number of links
                              DWQUERYLD lpfnDwQueryLinkData,       // Pointer to app link callback
                              HWND hDlg,                           // Handle of the dialog
                              HWND hWndName,                       // Handle of the Name window
                              HWND hWndVal,                        // Handle of Value window
                              HWND hWndValText,                    // Handle of Value LTEXT
                              HWND hWndLink,                       // Handle of Link checkbox
                              HWND hWndLinkVal,                    // Handle of Link Value window
                              HWND hWndType,                       // Handle of Type window
                              HWND hWndBoolTrue,                   // Handle of True radio button
                              HWND hWndBoolFalse,                  // Handle of False radio button
                              HWND hWndGroup,                      // Handle of Group window
                              HWND hWndAdd,                        // Handle of Add button
                              HWND hWndDelete,                     // Handle of Delete button
                              BOOL *pfLink,                        // Indicates that the value is a link
                              BOOL *pfAdd)                         // Indicates the state of the Add button
{
    UDTYPES udtype;
    LPVOID lpv;
    LPPROPVARIANT lppropvar;            // A property from the UDObj linked-list.
    BOOL f,fT;
    TCHAR sz[BUFMAX];
    LPUDPROP lpudp;
    
    // Grab the type for the string and set up the dialog to have the right
    // controls to display it.
    udtype = UdtypesUserDefType (lpUDObj, szName);
    AssertSz ((udtype != wUDinvalid), TEXT("User defined properties or ListView corrupt"));
    
    // Get a name-specified property from the UD linked-list.
    
    lppropvar = LppropvarUserDefGetPropVal (lpUDObj, szName, pfLink, &fT);
    Assert (lppropvar != NULL || udtype == wUDbool || udtype == wUDdw);
    if (lppropvar == NULL)
        return;
    
    lpv = LpvoidUserDefGetPropVal (lpUDObj, szName, 1, NULL, UD_STATIC | UD_PTRWIZARD, pfLink, &fT);
    Assert((lpv != NULL) || (udtype == wUDbool) || (udtype == wUDdw));
    
    FSwapControls (hWndVal, hWndLinkVal, hWndBoolTrue, hWndBoolFalse, hWndGroup, hWndType, hWndValText, *pfLink, (udtype == wUDbool));
    
    SendMessage (hWndType, CB_SETCURSEL, (WPARAM) WUdtypeToSz (lppropvar, (TCHAR *) sz, BUFMAX,
        ((LPUDINFO)lpUDObj->m_lpData)->lpfnFNumToSz), 0);
    SendMessage (hWndLink, BM_SETCHECK, (WPARAM) *pfLink, 0);
    if (cLinks)                       // Let's make sure we enable the window if links are allowed
        EnableWindow(hWndLink, TRUE);
    
    if (*pfLink)
    {
        FCreateListOfLinks (cLinks, lpfnDwQueryLinkData, hWndLinkVal);
        lpv = LpvoidUserDefGetPropVal (lpUDObj, szName, 1, NULL, UD_LINK | UD_PTRWIZARD, pfLink, &fT);
        Assert (lpv != NULL || udtype == wUDbool || udtype == wUDdw);
        //    if (lpfnDwQueryLinkData == NULL)
        //    {
        //      SetCustomDlgDefButton(hDlg, gOKButtonID);
        //      EnableWindow (hWndAdd, FALSE);
        //      SendMessage (hWndLinkVal, CB_INSERTSTRING, (WPARAM) -1, (LPARAM) lpv);
        //    }
        AssertSz ((lpv != NULL), TEXT("Dialog is corrupt in respect to Custom Properties database"));
        
        // This code is added for bug 188 and the code is ugly !! :)
        lpudp = LpudpropFindMatchingName (lpUDObj, szName);
        if ((lpudp != NULL) && (lpudp->fLinkInvalid))
        {
            SetCustomDlgDefButton(hDlg, IDD_CUSTOM_DELETE);
            SendMessage(hWndName, WM_SETTEXT, 0, (LPARAM)szName);
            SendMessage(hWndVal, WM_SETTEXT, 0, (LPARAM)lpv);
            EnableWindow(hWndDelete, TRUE);
            EnableWindow(hWndAdd, FALSE);
            EnableWindow(hWndLink, FALSE);
            EnableWindow(hWndType, FALSE);
            ShowWindow(hWndLinkVal, SW_HIDE);
            ShowWindow(hWndVal, SW_SHOW);
            EnableWindow(hWndVal, FALSE);
            return;
        }
        
        // Select the current link for this property in the combobox.  If the link
        // name no longer exists (there's some contrived cases where this can
        // happen) then this will select nothing.
        SendMessage (hWndLinkVal, CB_SELECTSTRING, 0, (LPARAM) lpv);
        EnableWindow(hWndLink, TRUE);
    }
    else if (udtype == wUDbool)
    {
        SendMessage ((lpv) ? hWndBoolTrue : hWndBoolFalse, BM_SETCHECK, CHECKED, 0);
        SendMessage ((lpv) ? hWndBoolFalse : hWndBoolTrue, BM_SETCHECK, CLEAR, 0);
        EnableWindow(hWndType, TRUE);
    }
    else
    {
        SendMessage (hWndVal, WM_SETTEXT, 0, (LPARAM) sz);
        EnableWindow (hWndVal, TRUE);
        EnableWindow(hWndType, TRUE);
    }
    
    if (*pfAdd)
    {
        SendMessage (hWndAdd, WM_SETTEXT, 0, (LPARAM) rgszAdd[iszMODIFY]);
        *pfAdd = FALSE;
    }
    
    // HACK: Because the EN_UPDATE handler for hWndName checks fAdd to
    // see if the button should be set to Add, when we set the text
    // in the edit control, the button will change to Add unless
    // fAdd is set to TRUE.  Temporarily set the flag to TRUE to force
    // the button to not change.  Restore the original value after the
    // text has been set.
    f = *pfAdd;
    *pfAdd = TRUE;
    SendMessage (hWndName, WM_SETTEXT, 0, (LPARAM) szName);
    *pfAdd = f;
    // If we can fill the data in the controls, turn on the
    // Delete button too.
    //  fItemSel = TRUE;
    EnableWindow (hWndDelete, TRUE);
    SetCustomDlgDefButton(hDlg, gOKButtonID);
    EnableWindow (hWndAdd, FALSE);
} // PopulateControls


////////////////////////////////////////////////////////////////////////////////
//
// FSetupAddButton
//
// Purpose:
//  Sets up the Add button correctly based on the type & flags.
//
////////////////////////////////////////////////////////////////////////////////
BOOL PASCAL
FSetupAddButton
(DWORD iszType,                       // Index of the type in combobox
 BOOL fLink,                          // Indicates a link
 BOOL *pfAdd,                         // Indicates if the Add button is showing
 HWND hWndAdd,                        // Handle of Add button
 HWND hWndVal,                        // Handle of value button
 HWND hWndName,                       // Handle of Name
 HWND hDlg)                           // Handle of dialog
{
    // Once the user starts typing, we can enable the Add button
    // if there is text in the name & the value (unless this
    // is a link or boolean, in which case we don't care about
    // the value).
    BOOL f;
    
    if ((iszType != iszBOOL) && (!fLink))
    {
        if (SendMessage (hWndVal, EM_LINELENGTH, 0, 0) != 0)
        {
            f = (SendMessage (hWndName, WM_GETTEXTLENGTH, 0, 0) != 0);
            if (f)
                SetCustomDlgDefButton(hDlg, IDD_CUSTOM_ADD);
            else
                SetCustomDlgDefButton(hDlg, gOKButtonID);
            EnableWindow (hWndAdd, f);
        }
        else
        {
            SetCustomDlgDefButton(hDlg, gOKButtonID);
            EnableWindow (hWndAdd, FALSE);
        }
    }
    // If it's a bool or link, just check to see that the name
    // has stuff in it.
    else
    {
        f = SendMessage (hWndName, WM_GETTEXTLENGTH, 0, 0) != 0;
        if (f)
            SetCustomDlgDefButton(hDlg, IDD_CUSTOM_ADD);
        else
            SetCustomDlgDefButton(hDlg, gOKButtonID);
        EnableWindow (hWndAdd, f);
    }
    
    if (!*pfAdd)
    {
        SendMessage (hWndAdd, WM_SETTEXT, 0, (LPARAM) rgszAdd[iszADD]);
        *pfAdd = TRUE;
    }
    
    return TRUE;
    
}  // FSetupAddButton


////////////////////////////////////////////////////////////////////////////////
//
// WUdtypeToSz
//
// Purpose:
//  Converts the given type into a string representation.  Returns the
//  index in the type combobox of the type.
//
////////////////////////////////////////////////////////////////////////////////
WORD PASCAL WUdtypeToSz (
                         LPPROPVARIANT lppropvar,    // Value with the type to be converted.
                         LPTSTR sz,                  // Buffer to put converted val in
                         DWORD cchMax,               // Size of buffer (in chars)
                         BOOL (*lpfnFNumToSz)(NUM *, LPTSTR, DWORD))
{
    SYSTEMTIME st;
    WORD irg;
    FILETIME ft;
    
    Assert (lppropvar != NULL);
    
    switch (lppropvar->vt)
    {
    case wUDlpsz :
        PbSzNCopy (sz, lppropvar->pwszVal, cchMax);
        irg = iszTEXT;
        break;
        
    case wUDdate :
        if (FScanMem((LPBYTE)&lppropvar->filetime,
            0, sizeof(FILETIME))) // if the date struct is all 0's
        {
            *sz = 0;                       // display the empty string
        }
        else if (!FileTimeToLocalFileTime(&lppropvar->filetime, &ft)
            || !FileTimeToSystemTime (&ft, &st)
            || (!GetDateFormat (LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, sz, cchMax)))
        {
#ifdef DEBUG
            DWORD dwErr = GetLastError();    
#endif DEBUG            
            irg = iszUNKNOWN;
            *sz = 0;
            break;
        }
        
        irg = iszDATE;
        break;
        
    case wUDdw :
        Assert(cchMax >= 11);
        Assert(lppropvar->vt == VT_I4);
        
        wsprintf (sz, TEXT("%ld"), lppropvar->lVal);
        irg = iszNUM;
        break;
        
    case wUDfloat :
        if (lpfnFNumToSz != NULL)
            irg = (*lpfnFNumToSz)((NUM*)&lppropvar->dblVal, sz, cchMax) ? iszNUM : iszUNKNOWN;
        else
        {
            irg = iszUNKNOWN;
            *sz = 0;
        }
        break;
        
    case wUDbool :
        PbSzNCopy (sz,
            lppropvar->boolVal ? (LPTSTR) &rgszBOOL[iszTRUE] : (LPTSTR) &rgszBOOL[iszFALSE],
            cchMax);
        irg = iszBOOL;
        break;
        
    default :
        irg = iszUNKNOWN;
        
    } // switch
    
    return irg;
    
} // WUdtypeToSz


////////////////////////////////////////////////////////////////////////////////
//
// FCreateListOfLinks
//
// Purpose:
//  Creates the dropdown list of linkable items.
//
////////////////////////////////////////////////////////////////////////////////
BOOL PASCAL FCreateListOfLinks(
                               DWORD cLinks,                                // Number of links
                               DWQUERYLD lpfnDwQueryLinkData,               // Link data callback
                               HWND hWndLinkVal)                            // Link Value window handle
{
    DWORD irg;
    LPTSTR lpstz;
    
    // If the combobox is already filled, don't fill it
    if (irg = (int)SendMessage(hWndLinkVal, CB_GETCOUNT,0, 0))
    {
        Assert(irg == cLinks);
        return(TRUE);
    }
    
    lpstz = NULL;
    
    // Call back the client app to get the list of linkable
    // values, and put them in the value combobox.
    for (irg = 0; irg < cLinks; irg++)
    {
        lpstz = (TCHAR *) ((*lpfnDwQueryLinkData) (QLD_LINKNAME, irg, &lpstz, NULL));
        if (lpstz != NULL)
        {
            SendMessage (hWndLinkVal, CB_INSERTSTRING, (WPARAM) -1, (LPARAM) PSTR (lpstz));
            VFreeMemP(lpstz, CBTSTR(lpstz));
            // REVIEW: We probably ought to figure out a way to be more efficient here....
        }
    }
    
    return TRUE;
    
} // FCreateListOfLinks


////////////////////////////////////////////////////////////////////////////////
//
// FSetTypeControl
//
// Purpose:
//  Sets the type control to have the given type selected.
//
////////////////////////////////////////////////////////////////////////////////
BOOL PASCAL FSetTypeControl (
                             UDTYPES udtype,                      // Type to set the type to
                             HWND hWndType)                       // Handle of type control
{
    WORD iType;
    
    switch (udtype)
    {
    case wUDlpsz :
        iType = iszTEXT;
        break;
    case wUDfloat :
    case wUDdw    :
        iType = iszNUM;
        break;
    case wUDbool  :
        iType = iszBOOL;
        break;
    case wUDdate :
        iType = iszDATE;
        break;
    default:
        return FALSE;
    }
    SendMessage (hWndType, CB_SETCURSEL, (WPARAM) iType, 0);
    
    return TRUE;
    
} // FSetTypeControl


////////////////////////////////////////////////////////////////////////////////
//
// DeleteItem
//
// Purpose:
//  Deletes an item from the UD object and the listview.
//
////////////////////////////////////////////////////////////////////////////////
void PASCAL DeleteItem (
                        LPUDOBJ lpUDObj,
                        HWND hWndLV,
                        int iItem,
                        TCHAR sz[])
{
    int i;
    
    ListView_DeleteItem (hWndLV, iItem);
    FUserDefDeleteProp (lpUDObj, sz);
    
    // We just nuked the item with the focus, so let's get the new one
    // if there are still items in the listview
    if ((i = ListView_GetItemCount(hWndLV)) != 0)
    {
        // Figure out the index of the item to get the focus
        i = (i == iItem) ? iItem - 1 : iItem;
        ListView_SetItemState(hWndLV, i, LVIS_FOCUSED, LVIS_FOCUSED);
    }
    
} // DeleteItem


////////////////////////////////////////////////////////////////////////////////
//
// ResetTypeControl
//
// Purpose:
//  Resets the value of the type control to Text.
//
////////////////////////////////////////////////////////////////////////////////
void PASCAL ResetTypeControl (
                              HWND hDlg,                           // Handle of dialog
                              DWORD dwId,                          // Id of control
                              DWORD *piszType)                     // The type we've reset to
{
    SendDlgItemMessage (hDlg, dwId, CB_SETCURSEL, iszTEXT, 0);
    *piszType = iszTEXT;
} // ResetTypeControl


////////////////////////////////////////////////////////////////////////////////
//
// FDisplayConversionWarning
//
// Purpose:
//  Displays a warning about types being converted.  Returns TRUE if
//  the user presses "Cancel"
//
////////////////////////////////////////////////////////////////////////////////
BOOL PASCAL FDisplayConversionWarning(HWND hDlg)                   // Handle of parent window
{
    return (IdDoAlert(hDlg, idsPEWarningText, MB_ICONEXCLAMATION | MB_OKCANCEL) == IDCANCEL);
} // FDisplayConversionWarning


////////////////////////////////////////////////////////////////////////////////
//
// LoadTextStrings
//
// Purpose:
//  Loads all of the text needed by the dialogs from the DLL.
//
////////////////////////////////////////////////////////////////////////////////
BOOL PASCAL FLoadTextStrings (void)
{
    register int cLoads = 0;
    register int cAttempts = 0;
    
    // CchGetString returns a cch, so make it into a 1 or 0
    // then add up the results,making sure we load as many as
    // we try.

    cLoads += (CchGetString (idsPEB, rgszOrders[iszBYTES], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPEKB, rgszOrders[iszORDERKB], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPEMB, rgszOrders[iszORDERMB], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPEGB, rgszOrders[iszORDERGB], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPETB, rgszOrders[iszORDERTB], SHORTBUFMAX) && TRUE);
    cAttempts++;
    
    cLoads += (CchGetString (idsPEBytes, rgszStats[iszBYTES], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPEPages, rgszStats[iszPAGES], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPEPara, rgszStats[iszPARA], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPELines, rgszStats[iszLINES], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPEWords, rgszStats[iszWORDS], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPEChars, rgszStats[iszCHARS], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPESlides, rgszStats[iszSLIDES], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPENotes, rgszStats[iszNOTES], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPEHiddenSlides, rgszStats[iszHIDDENSLIDES], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPEMMClips, rgszStats[iszMMCLIPS], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPEFormat, rgszStats[iszFORMAT], SHORTBUFMAX) && TRUE);
    cAttempts++;
    
    cLoads += (CchGetString (idsPEText, rgszTypes[iszTEXT], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPEDate, rgszTypes[iszDATE], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPENumber, rgszTypes[iszNUM], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPEBool, rgszTypes[iszBOOL], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPEUnknown, rgszTypes[iszUNKNOWN], SHORTBUFMAX) && TRUE);
    cAttempts++;
    
    cLoads += (CchGetString (idsPEStatName, rgszStatHeadings[iszNAME], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPEValue, rgszStatHeadings[iszVAL], SHORTBUFMAX) && TRUE);
    cAttempts++;
    
    cLoads += (CchGetString (idsPEPropName, rgszHeadings[iszNAME], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPEValue, rgszHeadings[iszVAL], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPEType, rgszHeadings[iszTYPE], SHORTBUFMAX) && TRUE);
    cAttempts++;
    
    cLoads += (CchGetString (idsPETrue, rgszBOOL[iszTRUE], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPEFalse, rgszBOOL[iszFALSE], SHORTBUFMAX) && TRUE);
    cAttempts++;
    
    cLoads += (CchGetString (idsPEAdd, rgszAdd[iszADD], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPEModify, rgszAdd[iszMODIFY], SHORTBUFMAX) && TRUE);
    cAttempts++;
    
    cLoads += (CchGetString (idsPESource, rgszValue[iszSOURCE], SHORTBUFMAX) && TRUE);
    cAttempts++;
    cLoads += (CchGetString (idsPEValueColon, rgszValue[iszVALUE], BUFMAX) && TRUE);
    cAttempts++;
    
    
    return (cLoads == cAttempts);
    
} // LoadTextStrings

//
// Function: ISavePropDlgChanges
//
// Parameters:
//
//    hwndDlg - dialog window handle
//    hwndFrom - window handle from the NMHDR struct (see code above)
//
// Returns:
//
//       TRUE since we handled the message.
//
// History:
//
//    Created 09/16/94  martinth
//
int PASCAL ISavePropDlgChanges(LPALLOBJS lpallobjs, HWND hwndDlg, HWND hwndFrom)
{
    TCHAR   sz[BUFMAX];
    int     iRet = IDABORT; // MessageBox return.
    LRESULT lRet = 0L;      // (FALSE == dismiss property sheet).
    
    if (CchGetString(idsCustomWarning, sz, ARRAYSIZE(sz)) == 0)
        return(FALSE);
    
    lpallobjs->fPropDlgPrompted = TRUE;  // no warning next time!
    iRet = MessageBox( hwndDlg, sz, TEXT("Warning"),
                       MB_ICONEXCLAMATION | MB_YESNOCANCEL );    

    switch( iRet )
    {
    case IDYES:
        PropSheet_Apply(hwndFrom);  // Let's get them changes
        break;
    // case IDNO:                   // do nothing
    case IDCANCEL:                  // cancel and disallow sheet destroy.
	lRet = TRUE;
        break;
    }
    SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, lRet );
    return iRet;
}
