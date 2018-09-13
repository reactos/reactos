/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    printnew.h

Abstract:

    This module contains the header information for the Win32
    property sheet print common dialogs.

Revision History:

    11-04-97    JulieB    Created.

--*/



#ifdef __cplusplus
extern "C" {
#endif



#ifdef WINNT

//
//  Include Files.
//

#include <dlgs.h>
#include <initguid.h>
#include <winprtp.h>




//
//  Constant Declarations.
//

//
//  Dialog Constants.
//
#define IDD_PRINT_GENERAL          100
#define IDD_PRINT_GENERAL_LARGE    101 

#define IDI_COLLATE               ico1

#define IDC_PRINTER_LIST          1000
#define IDC_PRINTER_LISTVIEW      1001
#define IDC_PRINT_TO_FILE         1002
#define IDC_FIND_PRINTER          1003
#define IDC_STATUS_TEXT           1004
#define IDC_STATUS                1005
#define IDC_LOCATION_TEXT         1006
#define IDC_LOCATION              1007
#define IDC_COMMENT_TEXT          1008
#define IDC_COMMENT               1009

#define IDC_RANGE_ALL             rad1
#define IDC_RANGE_SELECTION       rad2
#define IDC_RANGE_CURRENT         rad3
#define IDC_RANGE_PAGES           rad4
#define IDC_RANGE_EDIT            edt1
#define IDC_RANGE_TEXT1           stc1
#define IDC_RANGE_TEXT2           stc2

#define IDC_COPIES                edt2
#define IDC_COPIES_TEXT           stc3
#define IDC_COLLATE               chx1

#define IDC_STATIC                -1




//
//  Typedef Declarations.
//

typedef struct
{
    UINT           ApiType;
    LPPRINTDLGEX   pPD;
    DWORD          ProcessVersion;
    DWORD          dwFlags;
    UINT           FinalResult;
    HRESULT        hResult;
    BOOL           fApply;
    BOOL           fOld;
    DWORD          dwExtendedError;
    HRESULT        hrOleInit;
#ifdef UNICODE
    LPPRINTDLGEXA  pPDA;
    BOOL           fPrintTemplateAlloc;
#endif
} PRINTINFOEX, *PPRINTINFOEX;




//
//  Global Variables.
//

//
//  PrintUI.
//
PF_PRINTDLGSHEETSPROPPAGES PrintUI_bPrintDlgSheetsPropPages = NULL;
HMODULE hPrintUI = NULL;

//
//  Registry keys.
//
static const TCHAR c_szSettings[] = TEXT("Printers\\Settings");
static const TCHAR c_szViewMode[] = TEXT("ViewMode");




//
//  CPrintBrowser Class.
//

class CPrintBrowser : public IShellBrowser, 
                      public ICommDlgBrowser2, 
                      public IPrintDialogCallback, 
                      public IPrintDialogServices
{
public:

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef) (THIS);
    STDMETHOD_(ULONG, Release) (THIS);

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND *lphwnd);
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode);

    // *** IShellBrowser methods *** (same as IOleInPlaceFrame)
    STDMETHOD(InsertMenusSB) (THIS_ HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
    STDMETHOD(SetMenuSB) (THIS_ HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject);
    STDMETHOD(RemoveMenusSB) (THIS_ HMENU hmenuShared);
    STDMETHOD(SetStatusTextSB) (THIS_ LPCOLESTR lpszStatusText);
    STDMETHOD(EnableModelessSB) (THIS_ BOOL fEnable);
    STDMETHOD(TranslateAcceleratorSB) (THIS_ LPMSG lpmsg, WORD wID);

    // *** IShellBrowser methods ***
    STDMETHOD(BrowseObject) (THIS_ LPCITEMIDLIST pidl, UINT wFlags);
    STDMETHOD(GetViewStateStream) (THIS_ DWORD grfMode, LPSTREAM *pStrm);
    STDMETHOD(GetControlWindow) (THIS_ UINT id, HWND *lphwnd);
    STDMETHOD(SendControlMsg) (THIS_ UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret);
    STDMETHOD(QueryActiveShellView) (THIS_ struct IShellView **ppshv);
    STDMETHOD(OnViewWindowActive) (THIS_ struct IShellView *pshv);
    STDMETHOD(SetToolbarItems) (THIS_ LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags);

    // *** ICommDlgBrowser2 methods ***
    STDMETHOD(OnDefaultCommand) (THIS_ struct IShellView *ppshv);
    STDMETHOD(OnStateChange) (THIS_ struct IShellView *ppshv, ULONG uChange);
    STDMETHOD(IncludeObject) (THIS_ struct IShellView *ppshv, LPCITEMIDLIST lpItem);
    STDMETHOD(Notify) (THIS_ struct IShellView *ppshv, DWORD dwNotifyType);
    STDMETHOD(GetDefaultMenuText) (THIS_ struct IShellView *ppshv, WCHAR *pszText, INT cchMax);
    STDMETHOD(GetViewFlags)(THIS_ DWORD *pdwFlags);

    // *** IPrintDialogCallback methods ***
    STDMETHOD(InitDone) (THIS);
    STDMETHOD(SelectionChange) (THIS);
    STDMETHOD(HandleMessage) (THIS_ HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult);

    // *** IPrintDialogServices methods ***
    STDMETHOD(GetCurrentDevMode) (THIS_ LPDEVMODE pDevMode, UINT *pcbSize);
    STDMETHOD(GetCurrentPrinterName) (THIS_ LPTSTR pPrinterName, UINT *pcchSize);
    STDMETHOD(GetCurrentPortName) (THIS_ LPTSTR pPortName, UINT *pcchSize);

    // *** Our own methods ***
    CPrintBrowser(HWND hDlg);
    ~CPrintBrowser();

    BOOL    OnInitDialog(WPARAM wParam, LPARAM lParam);
    BOOL    OnChildInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam);
    VOID    OnDestroyMessage();
    BOOL    OnCommandMessage(WPARAM wParam, LPARAM lParam);
    BOOL    OnChildCommandMessage(WPARAM wParam, LPARAM lParam);
    BOOL    OnNotifyMessage(WPARAM wParam, LPNMHDR lpnmhdr);
    BOOL    OnSelChange();
    BOOL    OnChangeNotify(LONG lNotification, LPCITEMIDLIST *ppidl);
    BOOL    OnAccelerator(HWND hwndActivePrint, HWND hwndFocus, HACCEL haccPrint, PMSG pMsg);
    VOID    OnNoPrinters(HWND hDlg, UINT iId);
    VOID    OnInitDone();

private:

    HRESULT CreatePrintShellView();
    UINT    GetViewMode();
    VOID    SetViewMode();
    HRESULT CreateHookDialog();
    BOOL    UpdateStatus(LPCITEMIDLIST pidl);
    BOOL    SelectSVItem();
    BOOL    GetCurrentPrinter();
    VOID    InitPrintToFile();
    VOID    InitPageRangeGroup();
    VOID    InitCopiesAndCollate();
    BOOL    SaveCopiesAndCollateInDevMode(LPDEVMODE pDM, LPTSTR pszPrinter);
    BOOL    SetCopiesOnApply();
    VOID    SaveDevMode();
    BOOL    MergeDevMode(LPTSTR pszPrinterName);
    BOOL    IsValidPageRange(LPTSTR pszString, UINT *pErrorId);
    BOOL    ConvertPageRangesToString(LPTSTR pszString, UINT cchLen);
    UINT    IntegerToString(DWORD Value, LPTSTR pszString, UINT cchLen);
    VOID    ShowError(HWND hDlg, int Id, UINT MessageId, ...);
    UINT    InsertDevicePage(LPCWSTR pszName, PDEVMODE pDevMode);
    UINT    RemoveDevicePage();
    UINT    RemoveAndInsertDevicePage(LPCWSTR pszName, PDEVMODE pDevMode);
    BOOL    FitViewModeBest(HWND hwndListView);
    VOID    SelectPrinterItem(LPITEMIDLIST pidlItem);
    BOOL    IsCurrentPrinter(LPCITEMIDLIST pidl);
    BOOL    OnRename(LPCITEMIDLIST *ppidl);

    UINT cRef;                         // compobj refcount
    HWND hwndDlg;                      // handle of this dialog
    HWND hSubDlg;                      // handle of the hook dialog
    HWND hwndView;                     // current view window
    HWND hwndUpDown;                   // UpDown Control Window handle;
    IShellView *psv;                   // shell view object
    IShellFolderView *psfv;            // shell folder view object
    IShellFolder *psfRoot;             // print folder shell folder
    IShellDetails *psd;                // shell details object
    LPITEMIDLIST pidlRoot;             // pidl for print folder
    IPrinterFolder *ppf;               // printer folder private interface

    HIMAGELIST himl;                   // system imagelist (small images)

    PPRINTDLG_PAGE pPrintDlgPage;      // ptr to PRINTDLG_PAGE struct (printui)
    PPRINTINFOEX pPI;                  // ptr to PRINTINFOEX struct
    LPPRINTDLGEX pPD;                  // caller's PRINTDLGEX struct

    IPrintDialogCallback *pCallback;   // ptr to app's callback interface
    IObjectWithSite *pSite;            // ptr to app's SetSite interface

    LPDEVMODE pDMInit;                 // ptr to the initial DEVMODE struct
    LPDEVMODE pDMCur;                  // ptr to the current DEVMODE struct
    LPDEVMODE pDMSave;                 // ptr to the last good DEVMODE struct

    UINT cchCurPrinter;                // size, in chars, of pszCurPrinter
    LPTSTR pszCurPrinter;              // ptr to name of current printer

    DWORD nCopies;                     // number of copies
    DWORD nPageRanges;                 // number of page ranges in pPageRange
    DWORD nMaxPageRanges;              // max number of page ranges allowed
    LPPRINTPAGERANGE pPageRanges;      // ptr to an array of page range structs

    BOOL fCollateRequested;            // collate is requested
    BOOL fSelChangePending;            // we have a selchange message pending
    BOOL fFirstSel;                    // still need to set first selection
    BOOL fAPWSelected;                 // add printer wizard is selected
    BOOL fNoAccessPrinterSelected;     // a printer we do not have access to is selected
    BOOL fDirtyDevicePages;            // Are device pages dirty

    UINT nInitDone;                    // number of CDM_INITDONE messages

    UINT nListSep;                     // number of characters in szListSep
    TCHAR szListSep[20];               // list separator

    UINT uRegister;                    // change notify register

    TCHAR szScratch[MAX_PATH * 2];     // scratch buffer
    

    UINT uDefViewMode;                 // How the default view mode is mapped
};




//
//  Context Help IDs.
//

DWORD aPrintExHelpIDs[] =
{
    grp1,                    NO_HELP,
    IDC_PRINTER_LISTVIEW,    IDH_PRINT_PRINTER_FOLDER,

    IDC_STATUS_TEXT,         IDH_PRINT_SETUP_DETAILS,
    IDC_STATUS,              IDH_PRINT_SETUP_DETAILS,
    IDC_LOCATION_TEXT,       IDH_PRINT_SETUP_DETAILS,
    IDC_LOCATION,            IDH_PRINT_SETUP_DETAILS,
    IDC_COMMENT_TEXT,        IDH_PRINT_SETUP_DETAILS,
    IDC_COMMENT,             IDH_PRINT_SETUP_DETAILS,

    IDC_PRINT_TO_FILE,       IDH_PRINT_TO_FILE,
    IDC_FIND_PRINTER,        IDH_PRINT_FIND_PRINTER,

    grp2,                    NO_HELP,
    IDOK,                    IDH_PRINT_BTN,

    0, 0
};


DWORD aPrintExChildHelpIDs[] =
{
    grp1,                    NO_HELP,
    IDC_RANGE_ALL,           IDH_PRINT32_RANGE,
    IDC_RANGE_SELECTION,     IDH_PRINT32_RANGE,
    IDC_RANGE_CURRENT,       IDH_PRINT32_RANGE,
    IDC_RANGE_PAGES,         IDH_PRINT32_RANGE,
    IDC_RANGE_EDIT,          IDH_PRINT32_RANGE,
    IDC_RANGE_TEXT1,         IDH_PRINT32_RANGE,
    IDC_RANGE_TEXT2,         IDH_PRINT32_RANGE,

    grp2,                    NO_HELP,
    IDC_COPIES,              IDH_PRINT_COPIES,
    IDC_COPIES_TEXT,         IDH_PRINT_COPIES,
    IDC_COLLATE,             IDH_PRINT_COLLATE,
    IDI_COLLATE,             IDH_PRINT_COLLATE,

    0, 0
};




//
//  Function Prototypes.
//

HRESULT
PrintDlgExX(
    PPRINTINFOEX pPI);

HRESULT
Print_ReturnDefault(
    PPRINTINFOEX pPI);

BOOL
Print_LoadLibraries();

VOID
Print_UnloadLibraries();

BOOL
Print_LoadIcons();

BOOL
Print_InvokePropertySheets(
    PPRINTINFOEX pPI,
    LPPRINTDLGEX pPD);

BOOL_PTR
Print_GeneralDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

BOOL_PTR
Print_GeneralChildDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

LRESULT
Print_MessageHookProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam);

BOOL
Print_InitDialog(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam);

HRESULT
Print_ICoCreateInstance(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID *ppv);

BOOL
Print_SaveDevNames(
    LPTSTR pCurPrinter,
    LPPRINTDLGEX pPD);

VOID
Print_GetPortName(
    LPTSTR pCurPrinter,
    LPTSTR pBuffer,
    int cchBuffer);

HANDLE
Print_GetDevModeWrapper(
    LPTSTR pszDeviceName,
    HANDLE hDevMode);

BOOL
Print_NewPrintDlg(
    PPRINTINFO pPI);


#ifdef UNICODE
  HRESULT
  ThunkPrintDlgEx(
      PPRINTINFOEX pPI,
      LPPRINTDLGEXA pPDA);

  VOID
  FreeThunkPrintDlgEx(
      PPRINTINFOEX pPI);

  VOID
  ThunkPrintDlgExA2W(
      PPRINTINFOEX pPI);

  VOID
  ThunkPrintDlgExW2A(
      PPRINTINFOEX pPI);
#endif

#endif   // WINNT

#ifdef __cplusplus
};  // extern "C"
#endif
