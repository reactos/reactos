#include "shell32p.h"
#include <winspool.h>

#define MAXCOMPUTERNAME (2 + INTERNET_MAX_HOST_NAME_LENGTH + 1)

#ifdef WINNT

#define MAXNAMELEN MAX_PATH
#define MAXNAMELENBUFFER (MAXNAMELEN + MAXCOMPUTERNAME+1)
//
// The printer name specified in the _SHARE_INFO_502 structure in a call 
// to NetShareAdd() which is in localspl.dll, contains a printer name that 
// is expressed as \\server_name\printer_name,LocalsplOnly.  (',LocalsplOnly' 
// post fix string was recently added for clustering support)  The server 
// name prefix and the post fix string prevent the maximum printer name from 
// being a valid size in a call to NetShareAdd(), because NetShareAdd() will 
// only accept a maximum share name path of 256 characters, therefore the 
// maximum printer name calculation has been changed to.  This change only 
// applies to the windows nt spooler.  Because the remote printers folder can 
// view non shared printers on downlevel print servers we cannot change the 
// maxnamelen define to 220 this would break long printer name printers on
// downlevel print servers.
//
// max local printer name = max shared net path - ( wack + wack + max server name + wack + comma + 'LocalsplOnly' + null )
// max local printer name = 256 - (1 + 1 + 13 + 1 + 1 + 12 + 1)
// max local printer name = 256 - 30
// max local printer name = 226 - 5 to round to some reasonable number
// max local printer name = 221  
//
#define MAXLOCALNAMELEN 221

#else   // WINNT

#define MAXNAMELEN          32
#define MAXNAMELENBUFFER    32
#define MAXLOCALNAMELEN     MAXNAMELEN

#endif  // !WINNT

//
// We need the Win9x PIDL structure under NT, so
// just decalre this outside #ifdef WINNT
//
#define MAXNAMELENW9X       32

// Aargh: NT and W95 have two different formats for PIDLS, try to make both
// understand both...
typedef struct _NTIDPRINTER
{
    USHORT  cb;
    USHORT  uFlags;

    #define PRINTER_MAGIC 0xBEBADB00

    DWORD   dwMagic;
    DWORD   dwType;
    WCHAR   cName[MAXNAMELENBUFFER];
    USHORT  uTerm;
} NTIDPRINTER, *LPNTIDPRINTER;;

// W95 IDPrinter structure
typedef struct _W95IDPRINTER
{
    USHORT  cb;
    char    cName[MAXNAMELENW9X];
    USHORT  uTerm;
} W95IDPRINTER, *LPW95IDPRINTER;

#ifdef WINNT
typedef struct _NTIDPRINTER IDPRINTER, *LPIDPRINTER;
#else
typedef struct _W95IDPRINTER IDPRINTER, *LPIDPRINTER;
#endif

typedef const IDPRINTER *LPCIDPRINTER;



void Printers_FillPidl(LPIDPRINTER pidl, LPCTSTR szName);

HRESULT STDAPICALLTYPE Printer_GetSubObject(REFCLSID rclsid,
                                          LPCTSTR pszContainer,
                                          LPCTSTR pszSubObject,
                                          REFIID iid,
                                          void **ppv);
DWORD Printer_DropFiles(HWND hwndParent, HDROP hDrop, LPCTSTR szPrinter);
void Printers_InitSpooler();
DWORD Printers_EnumPrinters(LPCTSTR pszServer, DWORD dwType, DWORD dwLevel, void **ppPrinters);
#ifdef PRN_FOLDERDATA
DWORD Printers_FolderEnumPrinters(HANDLE hFolder, void **ppPrinters);
void *Printer_FolderGetPrinter(HANDLE hFolder, LPCTSTR pszPrinter);
#endif

void Printer_ViewQueue(HWND hwndStub, LPCTSTR lpszCmdLine, int nCmdShow, LPARAM lParam);

void Printer_Properties(HWND hWnd, LPCTSTR lpszPrinterName, int nCmdShow, LPARAM lParam);

VOID Printer_WarnOnError(HWND hwnd, LPCTSTR pName, int idsError);
BOOL Printer_ModifyPrinter(LPCTSTR lpszPrinterName, DWORD dwCommand);

void *Printer_GetPrinterDriver(HANDLE hPrinter, DWORD dwLevel);
void *Printer_GetPrinter(HANDLE hPrinter, DWORD dwLevel);

void PrintDef_UpdateHwnd(LPCTSTR lpszPrinterName, HWND hWnd);
void PrintDef_UpdateName(LPCTSTR lpszPrinterName, LPCTSTR lpszNewName);
void PrintDef_RefreshQueue(LPCTSTR lpszPrinterName);
void Printer_CheckMenuItem(HMENU hmModify, UINT fState, UINT uChecked, UINT uUnchecked);
void Printer_EnableMenuItems(HMENU hmModify, BOOL bEnable);
BOOL Printers_DeletePrinter(HWND, LPCTSTR, DWORD, LPCTSTR);
BOOL Printer_SetAsDefault(LPCTSTR lpszPrinterName);
DWORD Printer_GetPrinterAttributes(LPCTSTR lpszPrinterName);
BOOL Printers_GetFirstSelected(IShellView *psv, LPTSTR lpszPrinterName);
void Printers_ChooseNewDefault(HWND hWnd);
DWORD Printers_GetSpoolerVersion(LPCTSTR pszMachine);

typedef BOOL (*ENUMPROP)(void *lpData, HANDLE hPrinter, DWORD dwLevel,
        LPBYTE pEnum, DWORD dwSize, DWORD *lpdwNeeded, DWORD *lpdwNum);
void *Printer_EnumProps(HANDLE hPrinter, DWORD dwLevel, DWORD *lpdwNum,
        ENUMPROP lpfnEnum, void *lpData);

typedef struct
{
#ifdef __cplusplus
    void *sf;
    void *pf;
    void *sio;
#else // __cplusplus
    IShellFolder2 sf;
    IPersistFolder2 pf;
    IShellIconOverlay sio;
#endif // __cplusplus
#ifdef WINNT
#ifdef __cplusplus
    void *prnf;
    void *rc;
#else // __cplusplus
    IPrinterFolder prnf;
    IRemoteComputer rc;
#endif // __cplusplus
#endif

    LPTSTR pszServer;
    DWORD dwSpoolerVersion;

#ifdef PRN_FOLDERDATA
    HANDLE hFolder;
    BOOL bRefreshed;
    BOOL bShowAddPrinter;               // whether to display the Add Printer icon in the printers folder
#ifdef __cplusplus
    void *nf;
#else // __cplusplus
    IFolderNotify nf;
#endif // __cplusplus
#else // PRN_FOLDERDATA
    CRITICAL_SECTION csPrinterInfo;     // may be multiple threads (?)
    HDPA hdpaPrinterInfo;               // array of PrinterInfo structs
#endif
    LPITEMIDLIST pidl;
    LONG cRef;
} CPrinterFolder;

#ifdef WINNT // PRINTQ

VOID Printer_SplitFullName(LPTSTR pszScratch, LPCTSTR pszFullName, LPCTSTR *ppszServer, LPCTSTR *ppszPrinter);
BOOL Printer_CheckShowFolder(LPCTSTR pszMachine);
BOOL Printer_CheckNetworkPrinterByName(LPCTSTR pszPrinter, LPCTSTR* ppszLocal);

#endif

BOOL IsDefaultPrinter(LPCTSTR pszPrinter, DWORD dwAttributes);

#ifndef PRN_FOLDERDATA

typedef struct tagPrinterInfo
{
    DWORD dwSize;               // size of pi2 (> sizeof(PRINTER_INFO_2))
    DWORD dwTimeUpdated;        // time PRINTER_INFO_2 was last updated
    UINT  flags;
    PRINTER_INFO_2 pi2;         // printer information
} PrinterInfo, *PPrinterInfo;
#define UPDATE_ON_TIMER 1       // update pi2 on PRINTER_POLL_INVTERVAL
#define UPDATE_NOW      2       // update pi2 now
#define PRINTER_POLL_INTERVAL (5*1000) // 5 seconds

LPPRINTER_INFO_2 CPrinters_SF_GetPrinterInfo2(CPrinterFolder *psf, LPCTSTR pPrinterName);
void CPrinters_SF_FreePrinterInfo2(CPrinterFolder *);
void CPrinters_SF_UpdatePrinterInfo2(CPrinterFolder *,LPCTSTR,UINT);
void CPrinters_SF_RemovePrinterInfo2(CPrinterFolder *,LPCTSTR);
void CPrinters_SF_FreeHDPAPrinterInfo(HDPA);

#define SIZEOF_PRINTERINFO(pi2size) (SIZEOF(PrinterInfo) + (pi2size) - SIZEOF(PRINTER_INFO_2))

#endif // PRINTQ

HRESULT Printer_SetNameOf(CPrinterFolder *psf, HWND hwndOwner, LPCTSTR pOldName, LPTSTR pNewName, LPITEMIDLIST *ppidlOut);
void Printer_MergeMenu(CPrinterFolder *psf, LPQCMINFO pqcm, LPCTSTR pszPrinter, BOOL fForcePause);
HRESULT Printer_InvokeCommand(HWND hwnd, CPrinterFolder *psf, LPIDPRINTER pidp, WPARAM wParam, LPARAM lParam, LPBOOL pfChooseNewDefault);

// CPrintRoot_GetPIDLType
typedef enum
{
    HOOD_COL_PRINTER = 0,
    HOOD_COL_FILE    = 1
} PIDLTYPE ;

// Needed in the callbck function...
extern PIDLTYPE CPrintRoot_GetPIDLType(LPCITEMIDLIST pidl);

//
// printer1.c:
//
void Printer_PrintHDROPFiles(HWND hwnd, HDROP hdrop, LPCITEMIDLIST pidlPrinter);
LPTSTR Printer_FindIcon(CPrinterFolder *psf, LPCTSTR pszPrinterName, LPTSTR pszModule, LONG cbModule, LPINT piIcon);
void Printer_LoadIcons(LPCTSTR pszPrinterName, HICON *phLargeIcon, HICON *phSmallIcon);


//
// prcache.c:
//
HANDLE Printer_OpenPrinter(LPCTSTR lpszPrinterName);

//
// NT requires administrative access to pause, resume, purge the printer.
// However, if we are just retrieving information about the printer then
// we want to open with non-admin access so we will hit win32spl's cache.
//
// For win95, just use Printer_OpenPrinter.
//
#ifdef WINNT
HANDLE Printer_OpenPrinterAdmin(LPCTSTR lpszPrinterName);
#else
#define Printer_OpenPrinterAdmin Printer_OpenPrinter
#endif

void Printer_ClosePrinter(HANDLE hPrinter);
BOOL Printer_GPI2CB(void *lpData, HANDLE hPrinter, DWORD dwLevel,
    LPBYTE pBuf, DWORD dwSize, DWORD *lpdwNeeded, DWORD *lpdwNum);
void *Printer_GetPrinterInfo(HANDLE hPrinter, DWORD dwLevel );
void *Printer_GetPrinterInfoStr(LPCTSTR lpszPrinterName, DWORD dwLevel);
void Printer_SHChangeNotifyRename(LPTSTR pOldName, LPTSTR pNewName);

//
// printobj.c:
//
typedef void (*LPFNPRINTACTION)(HWND, LPCTSTR, int, LPARAM);
void Printer_OneWindowAction(HWND hwndStub, LPCTSTR lpName, HDSA *lphdsa, LPFNPRINTACTION lpfn, LPARAM lParam, BOOL fModal);
void Printer_PropAction(HWND hwndStub, LPTSTR lpName, int nCmdShow);
#include "printobj.h"

//
// prtprop.c:
//
int Printer_IllegalName(LPTSTR lpFriendlyName);

//
// prqwnd.c:
//

typedef struct _StatusStuff
{
    DWORD bit;          // bit of a bitfield
    UINT  uStringID;    // the string id this bit maps to
} STATUSSTUFF, * LPSTATUSSTUFF;
typedef const STATUSSTUFF *LPCSTATUSSTUFF;

#define PRINTER_HACK_WORK_OFFLINE 0x80000000

UINT Printer_BitsToString(
    DWORD          bits,      // the bitfield we're looking at
    UINT           idsSep,    // string id of separator
    LPCSTATUSSTUFF pSS,       // a mapping of bits to string ids
    LPTSTR         lpszBuf,   // output buffer
    UINT           cchMax);   // size of output buffer

//
// defines needed in a couple different modules
//

STDAPI CPrinters_DFMCallBackBG(LPSHELLFOLDER psf, HWND hwndOwner,
                IDataObject * pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam);
STDAPI_(IShellFolderViewCB*) Printer_CreateSFVCB(IShellFolder2* psf, CPrinterFolder *pPrinterFolder, LPCITEMIDLIST pidl);

// printer.c
LPITEMIDLIST Printers_GetPidl(LPCITEMIDLIST pidlParent, LPCTSTR pszName);
//prqwnd.c
LPITEMIDLIST Printjob_GetPidl(LPCTSTR szName, LPSHCNF_PRINTJOB_DATA pData);

// printer1.c

LPITEMIDLIST Printers_GetInstalledNetPrinter(LPCTSTR lpNetPath);
void Printer_PrintFile(HWND hWnd, LPCTSTR szFilePath, LPCITEMIDLIST pidl);

// printobj.c
LPITEMIDLIST Printers_PrinterSetup(HWND hwndStub, UINT uAction, LPTSTR lpBuffer, LPCTSTR pszServerName);

extern CRITICAL_SECTION g_csPrinters;
