#ifndef _SHELLPRV_H_
#define _SHELLPRV_H_

#define _SHELL32_	// for DECLSPEC_IMPORT

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

#define NOWINDOWSX
#define STRICT
#define OEMRESOURCE // FSMenu needs the menu triangle

#define _OLE32_		// HACK: Remove DECLSPEC_IMPORT from WINOLEAPI
#define INC_OLE2
#define CONST_VTABLE

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <commdlg.h>
#include <port32.h>         // in    shell\inc
#include <debug.h>          // in    shell\inc
#include <linkinfo.h>
#include <shell2.h>
    
#ifdef PW2
#include <penwin.h>
#endif //PW2

#include "util.h"
#include "cstrings.h"
    

#define USABILITYTEST_CUTANDPASTE       // For the usability test only. Disable it when we ship.

#define OLE_DAD_TARGET			// Enables OLE-drop target


#define CODESEG


#define WIDTHBYTES(cx, cBitsPerPixel)   ((((cx) * (cBitsPerPixel) + 31) / 32) * 4)

// REVIEW, should this be a function? (inline may generate a lot of code)
#define CBBITMAPBITS(cx, cy, cPlanes, cBitsPerPixel)    \
        (((((cx) * (cBitsPerPixel) + 15) & ~15) >> 3)   \
        * (cPlanes) * (cy))

#define InRange(id, idFirst, idLast)  ((UINT)(id-idFirst) <= (UINT)(idLast-idFirst))

#define FIELDOFFSET(type, field)    ((int)(&((type NEAR*)1)->field)-1)

LPSTREAM WINAPI CreateMemStream(LPBYTE lpbInit, UINT cbInit);
BOOL     WINAPI CMemStream_SaveToFile(LPSTREAM pstm, LPCSTR pszFile);


// defcm.c
STDAPI CDefFolderMenu_CreateHKeyMenu(HWND hwndOwner, HKEY hkey, LPCONTEXTMENU * ppcm);

// futil.c
BOOL  IsShared(LPCSTR pszPath, BOOL fUpdateCache);
DWORD GetConnection(LPCSTR lpDev, LPSTR lpPath, UINT cbPath, BOOL bConvertClosed);

// rundll32.c
HWND _CreateStubWindow();
#define STUBM_SETDATA (WM_USER)
#define STUBM_GETDATA (WM_USER + 1)

#define SHELL_PROPSHEET_STUB_CLASS 1
#define SHELL_BITBUCKET_STUB_CLASS 2


// bitbuck.c
void  RelayMessageToChildren(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

// newmenu.c
BOOL WINAPI NewObjMenu_InitMenuPopup(HMENU hmenu, int iStart);
void WINAPI NewObjMenu_DrawItem(DRAWITEMSTRUCT *lpdi);
LRESULT WINAPI NewObjMenu_MeasureItem(MEASUREITEMSTRUCT *lpmi);
HRESULT WINAPI NewObjMenu_DoItToMe(HWND hwnd, LPCITEMIDLIST pidlParent, LPITEMIDLIST * ppidl);
#define NewObjMenu_TryNullFileHack(hwnd, szFile) CreateWriteCloseFile(hwnd, szFile, NULL, 0)
BOOL CreateWriteCloseFile(HWND hwnd, LPSTR szFileName, LPVOID lpData, DWORD cbData);
void WINAPI NewObjMenu_Destroy(HMENU hmenu, int iStart);

// exec stuff

/* common exe code with error handling */
#define SECL_USEFULLPATHDIR 	0x00000001
#define SECL_NO_UI          	0x00000002
BOOL ShellExecCmdLine(HWND hwnd, LPCSTR lpszCommand, LPCSTR lpszDir,
	int nShow, LPCSTR lpszTitle, DWORD dwFlags);
#define ISSHELLEXECSUCCEEDED(hinst) ((UINT)hinst>32)
#define ISWINEXECSUCCEEDED(hinst)   ((UINT)hinst>=32)
void _ShellExecuteError(LPSHELLEXECUTEINFO pei, LPCSTR lpTitle, DWORD dwErr);

HRESULT SHBindToIDListParent(LPCITEMIDLIST pidl, REFIID riid, LPVOID *ppv, LPCITEMIDLIST *ppidlLast);

// fsnotify.c (private stuff) ----------------------

BOOL SHChangeNotifyInit();
void SHChangeNotifyTerminate(BOOL bLastTerm);

void _Shell32ThreadAddRef(BOOL bEnterCrit);
void _Shell32ThreadRelease(UINT nClients);
void _Shell32ThreadAwake(void);

// Entry points for managing registering name to IDList translations.
void NPTRegisterNameToPidlTranslation(LPCSTR pszPath, LPCITEMIDLIST pidl);
void NPTTerminate(void);
LPCSTR NPTMapNameToPidl(LPCSTR pszPath, LPCITEMIDLIST *ppidl);


// Reg_GetStructEx
BOOL Reg_GetStructEx(HKEY hkey, LPCSTR pszSubKey, LPCSTR pszValue, LPVOID pData, DWORD *pcbData, UINT uFlags);
#define RGS_IGNORECLEANBOOT 0x00000001

// path.c (private stuff) ---------------------

#define PQD_NOSTRIPDOTS	0x00000001

void PathQualifyDef(LPSTR psz, LPCSTR szDefDir, DWORD dwFlags);

BOOL PathRelativePathTo(LPSTR pszPath, LPCSTR pszFrom, DWORD dwAttrFrom, LPCSTR pszTo, DWORD dwAttrTo);
BOOL PathStripToRoot(LPSTR szRoot);
BOOL PathAddExtension(LPSTR pszPath, LPCSTR pszExtension);
void PathRemoveExtension(LPSTR pszPath);
void PathStripPath(LPSTR lpszPath);
// is a path component (not fully qualified) part of a path long
BOOL   PathIsLFNFileSpec(LPCSTR lpName);
BOOL PathIsRemovable(LPCSTR pszPath);
BOOL WINAPI PathYetAnotherMakeUniqueName(LPSTR  pszUniqueName, LPCSTR pszPath, LPCSTR pszShort, LPCSTR pszFileSpec);
BOOL PathMergePathName(LPSTR pPath, LPCSTR pName);
// does the string contain '?' or '*'
BOOL   IsWild(LPCSTR lpszPath);
// check for bogus characters, length
BOOL   IsInvalidPath(LPCSTR pPath);
LPSTR  PathSkipRoot(LPCSTR pPath);
BOOL WINAPI PathIsBinaryExe(LPCSTR szFile);

// is this drive a LFN volume
BOOL   IsLFNDrive(LPCSTR pszDriveRoot);



#define GCT_INVALID             0x0000
#define GCT_LFNCHAR             0x0001
#define GCT_SHORTCHAR           0x0002
#define GCT_WILD                0x0004
#define GCT_SEPERATOR           0x0008
UINT PathGetCharType(unsigned char ch);

void PathRemoveArgs(LPSTR pszPath);
BOOL PathMakePretty(LPSTR lpPath);

BOOL PathIsFileSpec(LPCSTR lpszPath);
BOOL PathIsLink(LPCSTR szFile);
BOOL PathIsSlow(LPCSTR szFile);

BOOL PathRenameExtension(LPSTR pszPath, LPCSTR pszExt);

int    SHCreateDirectory(HWND hwnd, LPCSTR szDest);

void SpecialFolderIDTerminate();
LPCITEMIDLIST GetSpecialFolderIDList(HWND hwndOwner, int nFolder, BOOL fCreate);

extern HINSTANCE g_hinst;

//
// NOTE these are the size of the icons in our ImageList, not the system
// icon size.
//
extern int g_cxIcon, g_cyIcon;
extern int g_cxSmIcon, g_cySmIcon;

extern HIMAGELIST himlIcons;
extern HIMAGELIST himlIconsSmall;

// for control panel and printers folder:
extern char const c_szNull[];
extern char const c_szDotDot[];
extern char const c_szRunDll[];
extern char const c_szNewObject[];
extern CRITICAL_SECTION g_csPrinters;

//IsDllLoaded in init.c
//not needed because of new Win16 proccess model, but nice to have in DEBUG
#ifdef DEBUG
extern BOOL IsDllLoaded(HMODULE hDll, LPCSTR pszDLL);
#else
#define IsDllLoaded(hDll, szDLL)    (hDll != NULL)
#endif

// for sharing DLL
typedef BOOL (*PFNISPATHSHARED)(LPCSTR lpPath, BOOL fRefresh);

extern PFNISPATHSHARED g_pfnIsPathShared;

BOOL ShareDLL_Init(void);

// For Version DLL

typedef BOOL (* PFNVERQUERYVALUE)(const LPVOID pBlock,
        LPCSTR lpSubBlock, LPVOID * lplpBuffer, LPDWORD lpuLen);
typedef DWORD (* PFNGETFILEVERSIONINFOSIZE) (
        LPCSTR lptstrFilename, LPDWORD lpdwHandle);
typedef BOOL (* PFNGETFILEVERSIONINFO) (
        LPCSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData);
typedef DWORD (* PFNVERLANGUAGENAME)(DWORD wLang,
        LPSTR szLang,DWORD nSize);

extern PFNVERQUERYVALUE g_pfnVerQueryValue;
extern PFNGETFILEVERSIONINFOSIZE g_pfnGetFileVersionInfoSize;
extern PFNGETFILEVERSIONINFO g_pfnGetFileVersionInfo;
extern PFNVERLANGUAGENAME g_pfnVerLanguageName;

BOOL VersionDLL_Init(void);

// For ComDlg32

typedef BOOL (* PFNGETOPENFILENAME)(OPENFILENAME * pofn);
extern PFNGETOPENFILENAME g_pfnGetOpenFileName;
BOOL Comdlg32DLL_Init(void);


// For Winspool DLL

typedef BOOL (* PFNADDPORT) (LPSTR, HWND, LPSTR);
typedef BOOL (* PFNCLOSEPRINTER) (HANDLE);
typedef BOOL (* PFNCONFIGUREPORT) (LPSTR, HWND, LPSTR);
typedef BOOL (* PFNDELETEPORT) (LPSTR, HWND, LPSTR);
typedef BOOL (* PFNDELETEPRINTER) (LPSTR);
typedef BOOL (* PFNDELETEPRINTERDRIVER) (LPSTR, LPSTR, LPSTR);
typedef int  (* PFNDEVICECAPABILITIES) (LPCSTR, LPCSTR, WORD, LPSTR, CONST DEVMODE *);
typedef BOOL (* PFNENUMJOBS) (HANDLE, DWORD, DWORD, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD);
typedef BOOL (* PFNENUMMONITORS) (LPSTR, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD);
typedef BOOL (* PFNENUMPORTS) (LPSTR, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD);
typedef BOOL (* PFNENUMPRINTPROCESSORDATATYPES) (LPSTR, LPSTR, DWORD, LPSTR, DWORD, LPDWORD, LPDWORD);
typedef BOOL (* PFNENUMPRINTPROCESSORS) (LPSTR, LPSTR, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD);
typedef BOOL (* PFNENUMPRINTERDRIVERS) (LPSTR, LPSTR, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD);
typedef BOOL (* PFNENUMPRINTERS) (DWORD, LPSTR, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD);
typedef BOOL (* PFNENUMPRINTERPROPERTYSHEETS) (HANDLE, HWND, LPFNADDPROPSHEETPAGE, LPARAM);
typedef BOOL (* PFNGETPRINTER) (HANDLE, DWORD, LPBYTE, DWORD, LPDWORD);
typedef BOOL (* PFNGETPRINTERDRIVER) (HANDLE, LPSTR, DWORD, LPBYTE, DWORD, LPDWORD);
typedef BOOL (* PFNOPENPRINTER) (LPSTR, LPHANDLE, LPVOID);
typedef BOOL (* PFNPRINTERPROPERTIES) (HWND, HANDLE);
typedef BOOL (* PFNSETJOB) (HANDLE, DWORD, DWORD, LPBYTE, DWORD);
typedef BOOL (* PFNSETPRINTER) (HANDLE, DWORD, LPBYTE, DWORD);

extern PFNADDPORT g_pfnAddPort;
extern PFNCLOSEPRINTER g_pfnClosePrinter;
extern PFNCONFIGUREPORT g_pfnConfigurePort;
extern PFNDELETEPORT g_pfnDeletePort;
extern PFNDELETEPRINTER g_pfnDeletePrinter;
extern PFNDELETEPRINTERDRIVER g_pfnDeletePrinterDriver;
extern PFNDEVICECAPABILITIES g_pfnDeviceCapabilities;
extern PFNENUMJOBS g_pfnEnumJobs;
extern PFNENUMMONITORS g_pfnEnumMonitors;
extern PFNENUMPORTS g_pfnEnumPorts;
extern PFNENUMPRINTPROCESSORDATATYPES g_pfnEnumPrintProcessorDataTypes;
extern PFNENUMPRINTPROCESSORS g_pfnEnumPrintProcessors;
extern PFNENUMPRINTERDRIVERS g_pfnEnumPrinterDrivers;
extern PFNENUMPRINTERS g_pfnEnumPrinters;
extern PFNENUMPRINTERPROPERTYSHEETS g_pfnEnumPrinterPropertySheets;
extern PFNGETPRINTER g_pfnGetPrinter;
extern PFNGETPRINTERDRIVER g_pfnGetPrinterDriver;
extern PFNOPENPRINTER g_pfnOpenPrinter;
extern PFNPRINTERPROPERTIES g_pfnPrinterProperties;
extern PFNSETJOB g_pfnSetJob;
extern PFNSETPRINTER g_pfnSetPrinter;

BOOL WinspoolDLL_Init(void);
void WinspoolDLL_Term(void);

// Now for link info stuff
typedef LINKINFOAPI BOOL (WINAPI * PFNCREATELINKINFO)(PCSTR, PLINKINFO *);
typedef LINKINFOAPI void (WINAPI * PFNDESTROYLINKINFO)(PLINKINFO);
typedef LINKINFOAPI BOOL (WINAPI * PFNRESOLVELINKINFO)(PCLINKINFO, PSTR, DWORD, HWND, PDWORD, PLINKINFO *);
typedef LINKINFOAPI BOOL (WINAPI * PFNGETLINKINFODATA)(PCLINKINFO, LINKINFODATATYPE, const VOID **);

extern PFNCREATELINKINFO g_pfnCreateLinkInfo;
extern PFNDESTROYLINKINFO g_pfnDestroyLinkInfo;
extern PFNRESOLVELINKINFO g_pfnResolveLinkInfo;
extern PFNGETLINKINFODATA g_pfnGetLinkInfoData;

BOOL LinkInfoDLL_Init(void);
void LinkInfoDLL_Term(void);

// Note we also dynamically load MPR but in a slightly different way
// we bacially have wrappers for all of the functions we call which call
// through to the real function.
void MprDLL_Term(void);

// other stuff
#define HINST_THISDLL   g_hinst

// fileicon.c
BOOL FileIconInit(void);
void FileIconTerm(void);


// binder.c

#define CCH_MENUMAX     80          // DOC: max size of a menu string
#define CCH_KEYMAX      64          // DOC: max size of a reg key (under shellex)
#define CCH_PROCNAMEMAX 80          // DOC: max lenght of proc name in handler

void    Binder_Initialize(void);   // per task clean up routine
void    Binder_Terminate(void);   // per task clean up routine
DWORD Binder_Timeout(void);
void Binder_Timer(void);

LPVOID  HandlerFromString(LPCSTR szBuffer, LPCSTR szProcName);
void    ReplaceParams(LPSTR szDst, LPCSTR szFile);
LONG    GetFileClassName(LPCSTR lpszFileName, LPSTR lpszFileType, UINT wFileTypeLen);
UINT    HintsFromFlags(UINT uFileFlags);



// filedrop.c
DWORD WINAPI File_DragFilesOver(LPCSTR pszFileName, LPCSTR pszDir, HDROP  hDrop);
DWORD WINAPI File_DropFiles(LPCSTR pszFileName, LPCSTR pszDir, LPCSTR pszSubObject, HWND hwndParent, HDROP hDrop, POINT pt);

// hdrop.c
DWORD _DropOnDirectory(HWND hwndOwner, LPCSTR pszDirTarget, HDROP hDrop, DWORD dwEffect);
void _TransferDelete(HWND hwnd, HDROP hDrop, UINT fOptions);
HMENU _LoadPopupMenu(UINT id);
HMENU _GetMenuFromID(HMENU hmMain, UINT uID);

// exec.c
int OpenAsDialog(HWND hwnd, LPCSTR lpszFile);
int AssociateDialog(HWND hwnd, LPCSTR lpszFile);

// Share some of the code with the OpenAs command...

// fsassoc.c
#define GCD_MUSTHAVEOPENCMD	0x0001
#define GCD_ADDEXETODISPNAME	0x0002	// must be used with GCD_MUSTHAVEOPENCMD
#define GCD_ALLOWPSUDEOCLASSES  0x0004	// .ext type extensions

// Only valid when used with FillListWithClasses
#define GCD_MUSTHAVEEXTASSOC    0x0008  // There must be at least one extension assoc

BOOL GetClassDescription(HKEY hkClasses, LPCSTR pszClass, LPSTR szDisplayName, int cbDisplayName, UINT uFlags);
void FillListWithClasses(HWND hwnd, BOOL fComboBox, UINT uFlags);
void DeleteListAttoms(HWND hwnd, BOOL fComboBox);

HKEY NetOpenProviderClass(HDROP);
void OpenNetResourceProperties(HWND, HDROP);

//
// Functions to help the cabinets sync to each other                      /* ;Internal */
//                                                                        /* ;Internal */
BOOL WINAPI SignalFileOpen(LPCITEMIDLIST pidl);                           /* ;Internal */

// BUGBUG:  Temporary hack while recycler gets implemented.

#define FSIDM_NUKEONDELETE  0x4901

// msgbox.c
// Constructs strings like ShellMessagebox "xxx %1%s yyy %2%s..."
// BUGBUG: convert to use george's new code in setup
LPSTR WINCAPI ShellConstructMessageString(HINSTANCE hAppInst, LPCSTR lpcText, ...);

// fileicon.c
int	SHAddIconsToCache(HICON hIcon, HICON hIconSmall, LPCSTR pszIconPath, int iIconIndex, UINT uIconFlags);
HICON	SimulateDocIcon(HIMAGELIST himl, HICON hIcon, BOOL fSmall);
HRESULT SHDefExtractIcon(LPCSTR szIconFile, int iIndex, UINT uFlags,
	HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);

//  Copy.c
#define SPEED_SLOW  400
DWORD GetPathSpeed(LPCSTR pszPath);

// shlobjs.c
BOOL InvokeFolderCommandUsingPidl(LPCMINVOKECOMMANDINFO pici,
	LPCSTR pszPath, LPCITEMIDLIST pidl, HKEY hkClass, ULONG fExecuteFlags);

// printer.c
LPITEMIDLIST Printers_GetPidl(LPCSTR szName);
//prqwnd.c
LPITEMIDLIST Printjob_GetPidl(LPCSTR szName, LPSHCNF_PRINTJOB_DATA pData);

// printer1.c
#define PRINTACTION_OPEN           0
#define PRINTACTION_PROPERTIES     1
#define PRINTACTION_NETINSTALL     2
#define PRINTACTION_NETINSTALLLINK 3
#define PRINTACTION_TESTPAGE       4
#define PRINTACTION_OPENNETPRN     5
BOOL Printers_DoCommandEx(HWND hwnd, UINT uAction, LPCSTR lpBuf1, LPCSTR lpBuf2, BOOL fModal);
#define Printers_DoCommand(hwnd, uA, lpB1, lpB2) Printers_DoCommandEx(hwnd, uA, lpB1, lpB2, FALSE)
LPITEMIDLIST Printers_GetInstalledNetPrinter(LPCSTR lpNetPath);
void Printer_PrintFile(HWND hWnd, LPCSTR szFilePath, LPCITEMIDLIST pidl);

// printobj.c
LPITEMIDLIST Printers_PrinterSetup(HWND hwndStub, UINT uAction, LPSTR lpBuffer);

// wuutil.c
void cdecl  SetFolderStatusText(HWND hwndStatus, int iField, UINT ids,...);

// helper macros for using a IStream* from "C"

#define Stream_Read(ps, pv, cb)     SUCCEEDED((ps)->lpVtbl->Read(ps, pv, cb, NULL))
#define Stream_Write(ps, pv, cb)    SUCCEEDED((ps)->lpVtbl->Write(ps, pv, cb, NULL))
#define Stream_Flush(ps)            SUCCEEDED((ps)->lpVtbl->Commit(ps, 0))
#define Stream_Seek(ps, li, d, p)   SUCCEEDED((ps)->lpVtbl->Seek(ps, li, d, p))
#define Stream_Close(ps)            (void)(ps)->lpVtbl->Release(ps)

//
// Notes:
//  1. Never "return" from the critical section.
//  2. Never "SendMessage" or "Yiels" from the critical section.
//  3. Never call USER API which may yield.
//  4. Always make the critical section as small as possible.
//
void Shell_EnterCriticalSection(void);
void Shell_LeaveCriticalSection(void);

#ifdef DEBUG
extern int   g_CriticalSectionCount;
extern DWORD g_CriticalSectionOwner;
#undef SendMessage
#define SendMessage  SendMessageD
LRESULT WINAPI SendMessageD(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
#endif

#define ENTERCRITICAL   Shell_EnterCriticalSection();
#define LEAVECRITICAL   Shell_LeaveCriticalSection();
#define ASSERTCRITICAL  Assert(g_CriticalSectionCount > 0 && GetCurrentThreadId() == g_CriticalSectionOwner);
#define ASSERTNONCRITICAL  Assert(GetCurrentThreadId() != g_CriticalSectionOwner);

//
// STATIC macro
//
#ifndef STATIC
#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif
#endif


//
// Defining FULL_DEBUG allows us debug memory problems.
//
#if defined(FULL_DEBUG)
#include <deballoc.h>
#endif // defined(FULL_DEBUG)


#define FillExecInfo(_info, _hwnd, _verb, _file, _params, _dir, _show) \
	(_info).hwnd		= _hwnd;	\
	(_info).lpVerb		= _verb;	\
	(_info).lpFile		= _file;	\
	(_info).lpParameters	= _params;	\
	(_info).lpDirectory	= _dir;		\
	(_info).nShow		= _show;	\
	(_info).fMask		= 0;		\
        (_info).cbSize          = sizeof(SHELLEXECUTEINFO);

// Define some registry caching apis.  This will allow us to minimize the
// changes needed in the shell code and still try to reduce the number of
// calls that we make to the registry.
//
LONG SHRegOpenKey(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult);

#undef  RegOpenKey
#define RegOpenKey       	SHRegOpenKey

#define SHRegCloseKey		RegCloseKey
#define SHRegQueryValue		RegQueryValue
#define SHRegQueryValueEx	RegQueryValueEx

// Used to tell the caching mechanism when we are entering and leaving
// things like enumerating folder.
#define SHRH_PROCESSDETACH 0x0000       // the process is going away.
#define SHRH_ENTER      0x0001          // We are entering a loop
#define SHRH_LEAVE      0x0002          // We are leaving the loop

void SHRegHints(int hint);

#ifdef DEBUG
#if 1
    __inline DWORD clockrate() {LARGE_INTEGER li; QueryPerformanceFrequency(&li); return li.LowPart;}
    __inline DWORD clock()     {LARGE_INTEGER li; QueryPerformanceCounter(&li);   return li.LowPart;}
#else
    __inline DWORD clockrate() {return 1000;}
    __inline DWORD clock()     {return GetTickCount();}
#endif

    #define TIMEVAR(t)    DWORD t ## T; DWORD t ## N
    #define TIMEIN(t)     t ## T = 0, t ## N = 0
    #define TIMESTART(t)  t ## T -= clock(), t ## N ++
    #define TIMESTOP(t)   t ## T += clock()
    #define TIMEFMT(t)    ((DWORD)(t) / clockrate()), (((DWORD)(t) * 1000 / clockrate())%1000)
    #define TIMEOUT(t)    if (t ## N) DebugMsg(DM_TRACE, #t ": %ld calls, %ld.%03ld sec (%ld.%03ld)", t ## N, TIMEFMT(t ## T), TIMEFMT(t ## T / t ## N))
#else
    #define TIMEVAR(t)
    #define TIMEIN(t)
    #define TIMESTART(t)
    #define TIMESTOP(t)
    #define TIMEFMT(t)
    #define TIMEOUT(t)
#endif

// in extract.c
extern DWORD WINAPI GetExeType(LPCSTR pszFile);

#define EI_LARGE_SMALL          1 // extract large and small icons.
#define EI_LARGE_SMALL_SHELL    2 // extract large and small icons (shell size)

UINT WINAPI ExtractIcons(LPCSTR szFileName, int nIconIndex, int cxIcon, int cyIcon, HICON *phicon, UINT nIcons, UINT flags);

// rdrag.c
extern BOOL WINAPI DragQueryInfo(HDROP hDrop, LPDRAGINFO lpdi);

UINT WINAPI SHSysErrorMessageBox(HWND hwndOwner, LPCSTR pszTitle, UINT idTemplate, DWORD err, LPCSTR pszParam, UINT dwFlags);

//======Hash Item=============================================================
typedef struct _HashTable * PHASHTABLE;
#define PHASHITEM LPCSTR

typedef void (CALLBACK *HASHITEMCALLBACK)(LPCSTR sz, UINT wUsage);

LPCSTR      WINAPI FindHashItem  (PHASHTABLE pht, LPCSTR lpszStr);
LPCSTR      WINAPI AddHashItem   (PHASHTABLE pht, LPCSTR lpszStr);
LPCSTR      WINAPI DeleteHashItem(PHASHTABLE pht, LPCSTR lpszStr);
#define     GetHashItemName(pht, sz, lpsz, cb)  lstrcpyn(lpsz, sz, cb)

PHASHTABLE  WINAPI CreateHashItemTable(UINT wBuckets, UINT wExtra, BOOL fCaseSensitive);
void        WINAPI DestroyHashItemTable(PHASHTABLE pht);

void        WINAPI SetHashItemData(PHASHTABLE pht, LPCSTR lpszStr, int n, DWORD dwData);
DWORD       WINAPI GetHashItemData(PHASHTABLE pht, LPCSTR lpszStr, int n);

void        WINAPI EnumHashItems(PHASHTABLE pht, HASHITEMCALLBACK callback);

#ifdef DEBUG
void        WINAPI DumpHashItemTable(PHASHTABLE pht);
#endif
// Heap tracking stuff.
#ifdef MEMMON
#ifndef INC_MEMMON
#define INC_MEMMON
#define LocalAlloc	SHLocalAlloc
#define LocalFree	SHLocalFree
#define LocalReAlloc	SHLocalReAlloc

HLOCAL WINAPI SHLocalAlloc(UINT uFlags, UINT cb);
HLOCAL WINAPI SHLocalReAlloc(HLOCAL hOld, UINT cbNew, UINT uFlags);
HLOCAL WINAPI SHLocalFree(HLOCAL h);
#endif
#endif

#ifdef __cplusplus
}       /* End of extern "C" { */
static inline void * __cdecl operator new(unsigned int size) { return (void *)LocalAlloc(LPTR, size); }
static inline void __cdecl operator delete(void *ptr) { LocalFree(ptr); }
extern "C" inline __cdecl _purecall(void) {return 0;}
#pragma intrinsic(memcpy)
#pragma intrinsic(memcmp)
#endif /* __cplusplus */

#endif // _SHELLPRV_H_
