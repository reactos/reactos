/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    prnsetup.h

Abstract:

    This module contains the header information for the Win32 print dialogs.

Revision History:

--*/



//
//  Include Files.
//

#include <help.h>




//
//  Constant Declarations.
//

#ifndef WINNT
  #define COMDLG_ANSI     0x0
  #define COMDLG_WIDE     0x1
#endif

#define PI_PRINTERS_ENUMERATED    0x00000001
#define PI_COLLATE_REQUESTED      0x00000002
#define PI_WPAPER_ENVELOPE        0x00000004     // wPaper is DMPAPER_ENV_x
#define PI_PRINTDLGX_RECURSE      0x00000008     // PrintDlgX calls PrintDlgX

#define PRNPROP (LPCTSTR)         0xA000L

#define DN_PADDINGCHARS           16             // extra devnames padding

#define MMS_PER_INCH              254            // 25.4 mms/inch

#define INCHES_DEFAULT            1000
#define MMS_DEFAULT               2500

#define COPIES_EDIT_SIZE          4
#define PAGE_EDIT_SIZE            5
#define MARGIN_EDIT_SIZE          6

#define CCHPAPERNAME              64
#define CCHBINNAME                24

#define ROTATE_LEFT               270            // dot-matrix
#define ROTATE_RIGHT              90             // HP PCL

#define MAX_DEV_SECT              512
#define BACKSPACE                 0x08
#define CTRL_X_CUT                0x18
#define CTRL_C_COPY               0x03
#define CTRL_V_PASTE              0x16

#define SIZEOF_DEVICE_INFO        32

#define MAX_PRINTERNAME           (MAX_PATH * 2)

#define SCRATCHBUF_SIZE           256

#define MIN_DEVMODE_SIZEA         40             // from spooler\inc\splcom.h



//
//  Constant Declarations for DLG file.
//

#define ID_BOTH_P_PROPERTIES      psh2
#define ID_BOTH_P_NETWORK         psh14
#define ID_BOTH_P_HELP            pshHelp
#define ID_BOTH_S_PRINTER         stc6
#define ID_BOTH_S_STATUS          stc12
#define ID_BOTH_S_TYPE            stc11
#define ID_BOTH_S_WHERE           stc14
#define ID_BOTH_S_COMMENT         stc13

#define ID_PRINT_X_TOFILE         chx1
#define ID_PRINT_X_COLLATE        chx2
#define ID_PRINT_C_QUALITY        cmb1
#define ID_PRINT_C_NAME           cmb4
#define ID_PRINT_E_FROM           edt1
#define ID_PRINT_E_TO             edt2
#define ID_PRINT_E_COPIES         edt3
#define ID_PRINT_G_RANGE          grp1
#define ID_PRINT_G_COPIES         grp2
#define ID_PRINT_G_PRINTER        grp4
#define ID_PRINT_I_COLLATE        ico3
#define ID_PRINT_P_SETUP          psh1
#define ID_PRINT_R_ALL            rad1
#define ID_PRINT_R_SELECTION      rad2
#define ID_PRINT_R_PAGES          rad3
#define ID_PRINT_S_DEFAULT        stc1
#define ID_PRINT_S_FROM           stc2
#define ID_PRINT_S_TO             stc3
#define ID_PRINT_S_QUALITY        stc4
#define ID_PRINT_S_COPIES         stc5

#define ID_SETUP_C_NAME           cmb1
#define ID_SETUP_C_SIZE           cmb2
#define ID_SETUP_C_SOURCE         cmb3
#define ID_SETUP_E_LEFT           edt4
#define ID_SETUP_E_TOP            edt5
#define ID_SETUP_E_RIGHT          edt6
#define ID_SETUP_E_BOTTOM         edt7
#define ID_SETUP_G_ORIENTATION    grp1
#define ID_SETUP_G_PAPER          grp2
#define ID_SETUP_G_DUPLEX         grp3
#define ID_SETUP_G_MARGINS        grp4
#define ID_SETUP_I_ORIENTATION    ico1
#define ID_SETUP_I_DUPLEX         ico2
#define ID_SETUP_P_MORE           psh1
#define ID_SETUP_P_PRINTER        psh3
#define ID_SETUP_R_PORTRAIT       rad1
#define ID_SETUP_R_LANDSCAPE      rad2
#define ID_SETUP_R_DEFAULT        rad3
#define ID_SETUP_R_SPECIFIC       rad4
#define ID_SETUP_R_NONE           rad5
#define ID_SETUP_R_LONG           rad6
#define ID_SETUP_R_SHORT          rad7
#define ID_SETUP_S_DEFAULT        stc1
#define ID_SETUP_S_SIZE           stc2
#define ID_SETUP_S_SOURCE         stc3
#define ID_SETUP_S_LEFT           stc15
#define ID_SETUP_S_RIGHT          stc16
#define ID_SETUP_S_TOP            stc17
#define ID_SETUP_S_BOTTOM         stc18
#define ID_SETUP_W_SAMPLE         rct1
#define ID_SETUP_W_SHADOWRIGHT    rct2
#define ID_SETUP_W_SHADOWBOTTOM   rct3




//
//  Typedef Declarations.
//

typedef struct {
    UINT            ApiType;
    LPPRINTDLG      pPD;
    LPPAGESETUPDLG  pPSD;
    DWORD           cPrinters;
    PPRINTER_INFO_2 pPrinters;
    PPRINTER_INFO_2 pCurPrinter;
    HANDLE          hCurPrinter;
    DWORD           Status;
    TCHAR           szDefaultPrinter[MAX_PRINTERNAME];
    WORD            wPaper;
    DWORD           dwRotation;
    UINT            uiOrientationID;
    POINT           PtPaperSizeMMs;
    RECT            RtMinMarginMMs;
    RECT            RtMarginMMs;
    POINT           PtMargins;
    RECT            RtSampleXYWH;
    BOOL            bKillFocus;
    DWORD           ProcessVersion;
#ifdef UNICODE
    LPPRINTDLGA     pPDA;
    LPBOOL          pAllocInfo;
    BOOL            bUseExtDeviceMode;
    BOOL            fPrintTemplateAlloc;
    BOOL            fSetupTemplateAlloc;
    UINT            NestCtr;
#endif
} PRINTINFO, *PPRINTINFO;




//
//  Global Variables.
//

#ifndef WINNT
  UINT msgHELPA;
#endif


#ifndef WINNT
  typedef DWORD (WINAPI *LPFNWNETCONNECTIONDIALOG)(HWND, DWORD);
  LPFNWNETCONNECTIONDIALOG MPR_WNetConnectionDialog = NULL;
  CHAR szWNetConnectionDialog[] = "WNetConnectionDialog";
#endif


static TCHAR  szTextWindows[]     = TEXT("Windows");
static TCHAR  szTextDevices[]     = TEXT("devices");
static TCHAR  szTextDevice[]      = TEXT("device");
static TCHAR  szTextNull[]        = TEXT("");
static TCHAR  szFilePort[]        = TEXT("FILE:");
static TCHAR  szDriver[]          = TEXT("winspool");

#ifndef WINNT
  static TCHAR  szCommdlgHelp[]   = HELPMSGSTRING;
#endif


LPPRINTHOOKPROC glpfnPrintHook = NULL;
LPSETUPHOOKPROC glpfnSetupHook = NULL;

#ifdef WINNT
  WNDPROC lpEditNumOnlyProc = NULL;
#endif
WNDPROC lpEditMarginProc = NULL;
WNDPROC lpStaticProc = NULL;

HKEY hPrinterKey;
TCHAR *szRegistryPrinter = TEXT("Printers");
TCHAR *szRegistryDefaultValueName = TEXT("Default");

#ifndef WINNT
  HANDLE hMPR = NULL;
  TCHAR szMprDll[] = TEXT("mpr.dll");
#endif


static BOOL   bAllIconsLoaded = FALSE;         // if all icons/images loaded

static HANDLE hIconCollate = NULL;             // Image
static HANDLE hIconNoCollate = NULL;           // Image

static HICON  hIconPortrait = NULL;            // Icon
static HICON  hIconLandscape = NULL;           // Icon
static HICON  hIconPDuplexNone = NULL;         // Icon
static HICON  hIconLDuplexNone = NULL;         // Icon
static HICON  hIconPDuplexTumble = NULL;       // Icon
static HICON  hIconLDuplexTumble = NULL;       // Icon
static HICON  hIconPDuplexNoTumble = NULL;     // Icon
static HICON  hIconLDuplexNoTumble = NULL;     // Icon
static HICON  hIconPSStampP = NULL;            // Icon
static HICON  hIconPSStampL = NULL;            // Icon


static TCHAR  cIntlDecimal = CHAR_NULL;        // decimal separator (.)
static TCHAR  cIntlMeasure[5] = TEXT("");      // measurement designator ("/mm)
static int    cchIntlMeasure = 0;              // # of chars in cIntlMeasure
static TCHAR  szDefaultSrc[SCRATCHBUF_SIZE] = TEXT("");




//
//  Context Help IDs.
//

const static DWORD aPrintHelpIDs[] =             // Context Help IDs
{
    // for Print dialog

    grp4,  NO_HELP,
    stc6,  IDH_PRINT_CHOOSE_PRINTER,
    cmb4,  IDH_PRINT_CHOOSE_PRINTER,

    psh2,  IDH_PRINT_PROPERTIES,

    stc8,  IDH_PRINT_SETUP_DETAILS,
    stc12, IDH_PRINT_SETUP_DETAILS,
    stc7,  IDH_PRINT_SETUP_DETAILS,
    stc11, IDH_PRINT_SETUP_DETAILS,
    stc10, IDH_PRINT_SETUP_DETAILS,
    stc14, IDH_PRINT_SETUP_DETAILS,
    stc9,  IDH_PRINT_SETUP_DETAILS,
    stc13, IDH_PRINT_SETUP_DETAILS,

    chx1,  IDH_PRINT_TO_FILE,

    grp1,  NO_HELP,
    ico1,  IDH_PRINT32_RANGE,
    rad1,  IDH_PRINT32_RANGE,
    rad2,  IDH_PRINT32_RANGE,
    rad3,  IDH_PRINT32_RANGE,
    stc2,  IDH_PRINT32_RANGE,
    edt1,  IDH_PRINT32_RANGE,
    stc3,  IDH_PRINT32_RANGE,
    edt2,  IDH_PRINT32_RANGE,

    grp2,  NO_HELP,
    edt3,  IDH_PRINT_COPIES,
    ico3,  IDH_PRINT_COLLATE,
    chx2,  IDH_PRINT_COLLATE,

    // for win3.1 Print template

    stc1,  IDH_PRINT_SETUP_DETAILS,

    stc4,  IDH_PRINT_QUALITY,
    cmb1,  IDH_PRINT_QUALITY,

    stc5,  IDH_PRINT_COPIES,

    psh1,  IDH_PRINT_PRINTER_SETUP,
    psh14, IDH_PRINT_NETWORK,
    pshHelp, IDH_HELP,

    0,     0
};

const static DWORD aPrintSetupHelpIDs[] =        // Context Help IDs
{
    // for PrintSetup dialog

    grp4,  NO_HELP,
    stc6,  IDH_PRINT_CHOOSE_PRINTER,
    cmb1,  IDH_PRINT_CHOOSE_PRINTER,

    psh2,  IDH_PRINT_PROPERTIES,

    stc8,  IDH_PRINT_SETUP_DETAILS,
    stc12, IDH_PRINT_SETUP_DETAILS,
    stc7,  IDH_PRINT_SETUP_DETAILS,
    stc11, IDH_PRINT_SETUP_DETAILS,
    stc10, IDH_PRINT_SETUP_DETAILS,
    stc14, IDH_PRINT_SETUP_DETAILS,
    stc9,  IDH_PRINT_SETUP_DETAILS,
    stc13, IDH_PRINT_SETUP_DETAILS,

    grp2,  NO_HELP,
    stc2,  IDH_PAGE_PAPER_SIZE,
    cmb2,  IDH_PAGE_PAPER_SIZE,
    stc3,  IDH_PAGE_PAPER_SOURCE,
    cmb3,  IDH_PAGE_PAPER_SOURCE,

    grp1,  NO_HELP,
    ico1,  IDH_PRINT_SETUP_ORIENT,
    rad1,  IDH_PRINT_SETUP_ORIENT,
    rad2,  IDH_PRINT_SETUP_ORIENT,

    // for win3.1 PrintSetup template

    grp3,  NO_HELP,
    stc1,  IDH_PRINT_CHOOSE_PRINTER,
    rad3,  IDH_PRINT_CHOOSE_PRINTER,
    rad4,  IDH_PRINT_CHOOSE_PRINTER,
    cmb1,  IDH_PRINT_CHOOSE_PRINTER,

    psh1,  IDH_PRINT_PROPERTIES,
    psh14, IDH_PRINT_NETWORK,
    pshHelp, IDH_HELP,

    // for winNT PrintSetup template

    grp2,  NO_HELP,                              // grp2 used for win31 help
    ico2,  IDH_PRINT_SETUP_DUPLEX,
    rad5,  IDH_PRINT_SETUP_DUPLEX,
    rad6,  IDH_PRINT_SETUP_DUPLEX,
    rad7,  IDH_PRINT_SETUP_DUPLEX,

    0,     0
};

const static DWORD aPageSetupHelpIDs[] =         // Context Help IDs
{
    rct1,  IDH_PAGE_SAMPLE,
    rct2,  IDH_PAGE_SAMPLE,
    rct3,  IDH_PAGE_SAMPLE,

    grp2,  NO_HELP,
    stc2,  IDH_PAGE_PAPER_SIZE,
    cmb2,  IDH_PAGE_PAPER_SIZE,
    stc3,  IDH_PAGE_PAPER_SOURCE,
    cmb3,  IDH_PAGE_PAPER_SOURCE,

    grp1,  NO_HELP,
    rad1,  IDH_PAGE_ORIENTATION,
    rad2,  IDH_PAGE_ORIENTATION,

    grp4,  NO_HELP,
    stc15, IDH_PAGE_MARGINS,
    edt4,  IDH_PAGE_MARGINS,
    stc16, IDH_PAGE_MARGINS,
    edt6,  IDH_PAGE_MARGINS,
    stc17, IDH_PAGE_MARGINS,
    edt5,  IDH_PAGE_MARGINS,
    stc18, IDH_PAGE_MARGINS,
    edt7,  IDH_PAGE_MARGINS,

    psh3,  IDH_PAGE_PRINTER,

    psh14, IDH_PRINT_NETWORK,
    pshHelp, IDH_HELP,

    0, 0
};




//
//  Macro Definitions.
//

#define IS_KEY_PRESSED(key)       ( GetKeyState(key) & 0x8000 )

#define ISDIGIT(c)                ((c) >= TEXT('0') && (c) <= TEXT('9'))

//
//  SetField is used to modify new-for-ver-4.0 DEVMODE fields.
//  We don't have to worry about the GET case, because we always check for
//  the existance-of-field bit before looking at the field.
//
#define SetField(_pdm, _fld, _val)     \
        ((_pdm)->dmSpecVersion >= 0x0400 ? (((_pdm)->_fld = (_val)), TRUE) : FALSE)




#ifdef __cplusplus
extern "C" {
#endif



//
//  Function Prototypes.
//

BOOL
PrintDlgX(
    PPRINTINFO pPI);

BOOL
PageSetupDlgX(
    PPRINTINFO pPI);

BOOL
PrintLoadIcons();

int
PrintDisplayPrintDlg(
    PPRINTINFO pPI);

int
PrintDisplaySetupDlg(
    PPRINTINFO pPI);

BOOL_PTR CALLBACK
PrintDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam);

BOOL_PTR CALLBACK
PrintSetupDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam);

LRESULT
PrintEditNumberOnlyProc(
    HWND hWnd,
    UINT msg,
    WPARAM wP,
    LPARAM lP);

LRESULT
PrintEditMarginProc(
    HWND hWnd,
    UINT msg,
    WPARAM wP,
    LPARAM lP);

LRESULT
PrintPageSetupPaintProc(
    HWND hWnd,
    UINT msg,
    WPARAM wP,
    LPARAM lP);

HANDLE
PrintLoadResource(
    HANDLE hInst,
    LPTSTR pResName,
    LPTSTR pType);

VOID
PrintGetDefaultPrinterName(
    LPTSTR pDefaultPrinter,
    UINT cchSize);

BOOL
PrintReturnDefault(
    PPRINTINFO pPI);

BOOL
PrintInitGeneral(
    HWND hDlg,
    UINT Id,
    PPRINTINFO pPI);

DWORD
PrintInitPrintDlg(
    HWND hDlg,
    WPARAM wParam,
    PPRINTINFO pPI);

DWORD
PrintInitSetupDlg(
    HWND hDlg,
    WPARAM wParam,
    PPRINTINFO pPI);

VOID
PrintUpdateSetupDlg(
    HWND hDlg,
    PPRINTINFO pPI,
    LPDEVMODE pDM,
    BOOL fResetContent);

BOOL
PrintSetCopies(
    HWND hDlg,
    PPRINTINFO pPI,
    UINT Id);

VOID
PrintSetMinMargins(
    HWND hDlg,
    PPRINTINFO pPI,
    LPDEVMODE pDM);

VOID
PrintSetupMargins(
    HWND hDlg,
    PPRINTINFO pPI);

VOID
PrintSetMargin(
    HWND hDlg,
    PPRINTINFO pPI,
    UINT Id,
    LONG lValue);

VOID
PrintGetMargin(
    HWND hEdt,
    PPRINTINFO pPI,
    LONG lMin,
    LONG *plMargin,
    LONG *plSample);

BOOL
PrintInitBannerAndQuality(
    HWND hDlg,
    PPRINTINFO pPI,
    LPPRINTDLG pPD);

BOOL
PrintCreateBanner(
    HWND hDlg,
    LPDEVNAMES pDN,
    LPTSTR psBanner,
    UINT cchBanner);

VOID
PrintInitQuality(
    HANDLE hCmb,
    LPPRINTDLG pPD,
    SHORT nQuality);

VOID
PrintChangeProperties(
    HWND hDlg,
    UINT Id,
    PPRINTINFO pPI);

VOID
PrintPrinterChanged(
    HWND hDlg,
    UINT Id,
    PPRINTINFO pPI);

VOID
PrintCancelPrinterChanged(
    PPRINTINFO pPI,
    LPTSTR pPrinterName);

VOID
PrintUpdateStatus(
    HWND hDlg,
    PPRINTINFO pPI);

BOOL
PrintGetSetupInfo(
    HWND hDlg,
    LPPRINTDLG pPD);

PPRINTER_INFO_2
PrintSearchForPrinter(
    PPRINTINFO pPI,
    LPCTSTR lpsPrinterName);

#ifdef UNICODE
  VOID
  PrintGetExtDeviceMode(
      HWND hDlg,
      PPRINTINFO pPI);
#endif

BOOL
PrintEnumAndSelect(
    HWND hDlg,
    UINT Id,
    PPRINTINFO pPI,
    LPTSTR lpsPrinterToSelect,
    BOOL bEnumPrinters);

VOID
PrintBuildDevNames(
    PPRINTINFO pPI);

HANDLE
PrintGetDevMode(
    HWND hDlg,
    HANDLE hPrinter,
    LPTSTR lpsDeviceName,
    HANDLE hDevMode);

VOID
PrintReturnICDC(
    LPPRINTDLG pPD,
    LPDEVNAMES pDN,
    LPDEVMODE pDM);

VOID
PrintMeasureItem(
    HANDLE hDlg,
    LPMEASUREITEMSTRUCT mis);

VOID
PrintInitOrientation(
    HWND hDlg,
    PPRINTINFO pPI,
    LPDEVMODE pDM);

VOID
PrintSetOrientation(
    HWND hDlg,
    PPRINTINFO pPI,
    LPDEVMODE pDM,
    UINT uiOldId,
    UINT uiNewId);

VOID
PrintUpdatePageSetup(
    HWND hDlg,
    PPRINTINFO pPI,
    LPDEVMODE pDM,
    UINT uiOldId,
    UINT uiNewId);

VOID
PrintInitDuplex(
    HWND hDlg,
    LPDEVMODE pDM);

VOID
PrintSetDuplex(
    HWND hDlg,
    LPDEVMODE pDM,
    UINT nRad);

VOID
PrintInitPaperCombo(
    PPRINTINFO pPI,
    HWND hCmb,
    HWND hStc,
    PPRINTER_INFO_2 pPrinter,
    LPDEVMODE pDM,
    WORD fwCap1,
    WORD cchSize1,
    WORD fwCap2);

VOID
PrintEditError(
    HWND hDlg,
    int Id,
    UINT MessageId,
    ...);

VOID
PrintOpenPrinter(
    PPRINTINFO pPI,
    LPTSTR pPrinterName);

BOOL
PrintClosePrinters(
    PPRINTINFO pPI);

VOID SetCopiesEditWidth(
    HWND hDlg,
    HWND hControl);

#ifdef UNICODE
  VOID
  UpdateSpoolerInfo(
      PPRINTINFO pPI);
#endif

PPRINTER_INFO_2
PrintGetPrinterInfo2(
    HANDLE hPrinter);

int
ConvertStringToInteger(
    LPCTSTR pSrc);

VOID
FreePrinterArray(
    PPRINTINFO pPI);

VOID
TermPrint(void);

VOID
TransferPSD2PD(
    PPRINTINFO pPI);

VOID
TransferPD2PSD(
    PPRINTINFO pPI);

#ifdef UNICODE
  VOID
  TransferPSD2PDA(
      PPRINTINFO pPI);

  VOID
  TransferPDA2PSD(
      PPRINTINFO pPI);

  BOOL
  ThunkPageSetupDlg(
      PPRINTINFO pPI,
      LPPAGESETUPDLGA pPSDA);

  VOID
  FreeThunkPageSetupDlg(
      PPRINTINFO pPI);

  BOOL
  ThunkPrintDlg(
      PPRINTINFO pPI,
      LPPRINTDLGA pPDA);

  VOID
  FreeThunkPrintDlg(
      PPRINTINFO pPI);

  VOID
  ThunkPrintDlgA2W(
      PPRINTINFO pPI);

  VOID
  ThunkPrintDlgW2A(
      PPRINTINFO pPI);

  VOID
  ThunkDevNamesA2W(
      LPDEVNAMES pDNA,
      LPDEVNAMES pDNW);

  VOID
  ThunkDevNamesW2A(
      LPDEVNAMES pDNW,
      LPDEVNAMES pDNA);

  VOID
  ThunkDevModeA2W(
      LPDEVMODEA pDMA,
      LPDEVMODEW pDMW);

  VOID
  ThunkDevModeW2A(
      LPDEVMODEW pDMW,
      LPDEVMODEA pDMA);

  LPDEVMODEW
  AllocateUnicodeDevMode(
      LPDEVMODEA pANSIDevMode);

  LPDEVMODEA
  AllocateAnsiDevMode(
      LPDEVMODEW pUnicodeDevMode);
#endif


#ifdef __cplusplus
};  // extern "C"
#endif
