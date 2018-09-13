#ifndef __UTILS__
#define __UTILS__

#include <windows.h>
#include <advpub.h>
#include <CleanOC.h>
#include <debug.h>
#include "general.h"

#define LStrNICmp(sz1, sz2, cch) (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, sz1, cch, sz2, cch) - 2)

#define MODULE_UNKNOWN_OWNER   "Unknown Owner"
#define MAX_MESSAGE_LEN        2048

// numeric constants
#define GD_EXTRACTDIR    0
#define GD_CACHEDIR      1
#define GD_CONFLICTDIR   2
#define GD_CONTAINERDIR  3
#define GD_WINDOWSDIR   10
#define GD_SYSTEMDIR    11

#define LENGTH_NAME             200
#define MAX_INF_SECTION_SIZE    1024
#define OLEUI_CCHKEYMAX         256
#define OLEUI_CCHKEYMAX_SIZE    (OLEUI_CCHKEYMAX*sizeof(TCHAR))
#define TIMESTAMP_MAXSIZE       64
#define VERSUBBLOCK_SIZE        256

#define MAX_VERSION_SIZE      16

// string constants
#define INPROCSERVER       TEXT("InprocServer")
#define LOCALSERVER        TEXT("LocalServer")
#define INPROCSERVERX86    TEXT("InProcServerX86")
#define LOCALSERVERX86     TEXT("LocalServerX86")
#define INPROCSERVER32     TEXT("InprocServer32")
#define LOCALSERVER32      TEXT("LocalServer32")
#define INFFILE            TEXT("InfFile")
#define UNKNOWNDATA        TEXT("n/a")
#define UNKNOWNOWNER       TEXT("Unknown Owner")
#define VARTRANSLATION     TEXT("\\VarFileInfo\\Translation")
#define FILEVERSION        TEXT("\\FileVersion")
#define STRINGFILEINFO     TEXT("\\StringFileInfo\\")
#define HKCR_CLSID         TEXT("CLSID")
#define HKCR_TYPELIB       TEXT("TypeLib")
#define HKCR_INTERFACE     TEXT("Interface")
#define VALUE_OWNER        TEXT(".Owner")
#define VALUE_ACTIVEXCACHE TEXT("ActiveXCache")
#define VALUE_PATH         TEXT("PATH")
#define VALUE_SYSTEM       TEXT("SystemComponent")
#define CONTAINER_APP      TEXT("IEXPLORE.EXE")
#define DEMO_PAGE          TEXT("DemoTmp.html")
#define KEY_HOOK           TEXT("Hook")
#define KEY_INFFILE        TEXT("InfFile")
#define KEY_INFSECTION     TEXT("InfSection")
#define KEY_DEFAULTUNINSTALL TEXT("DefaultUninstall")
#define KEY_UNINSTALL      TEXT("UNINSTALL")
#define KEY_SETUPHOOK      TEXT("SETUP HOOKS")
#define INF_EXTENSION      TEXT(".INF")
#define ENV_PATH           TEXT("PATH")
#define KEY_ADDCODE        TEXT("Add.Code")
#define DEFAULT_VALUE      TEXT("")
#define DEFAULT_CACHE      TEXT("\\OCCACHE")
#define DEFAULT_CONFLICT   TEXT("\\CONFLICT")
#define DU_INSTALLER_VALUE TEXT("Installer")
#define CDL_INSTALLER      TEXT("MSICD")

// registry paths for ModuleUsage
#define REGSTR_PATH_SHAREDDLLS     TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDlls")
#define REGSTR_PATH_MODULE_USAGE   TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ModuleUsage")
#define REGSTR_PATH_IE             TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths")
#define REGSTR_PATH_IE_SETTINGS    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings")
//#define REGSTR_PATH_ACTIVEX_CACHE  TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ActiveX Cache\\Paths")
#define REGSTR_PATH_ACTIVEX_CACHE  TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ActiveX Cache")
#define SOFTWARECLASSES            TEXT("SOFTWARE\\CLASSES")

// CLSIDLIST_ITEM declaration
struct tagCLSIDLIST_ITEM;
typedef struct tagCLSIDLIST_ITEM CLSIDLIST_ITEM;
typedef CLSIDLIST_ITEM* LPCLSIDLIST_ITEM;
struct tagCLSIDLIST_ITEM
{
    TCHAR szFile[MAX_PATH];
    TCHAR szCLSID[MAX_DIST_UNIT_NAME_LEN];
    BOOL bIsDistUnit;
    LPCLSIDLIST_ITEM pNext;
};

// function prototypes
// void RemoveObsoleteKeys();
void ReverseSlashes(LPTSTR pszStr);
TCHAR* ReverseStrchr(LPCTSTR szString, TCHAR ch);
LONG DeleteKeyAndSubKeys(HKEY hkIn, LPCTSTR pszSubKey);
BOOL FileExist(LPCTSTR lpszFileName);
HRESULT LookUpModuleUsage(
                LPCTSTR lpszFileName, 
                LPCTSTR lpszCLSID,
                LPTSTR lpszOwner = NULL, 
                DWORD dwOwnerSize = 0);
DWORD SubtractModuleOwner( LPCTSTR lpszFileName, LPCTSTR lpszGUID );

HRESULT SetSharedDllsCount(
                LPCTSTR lpszFileName, 
                LONG cRef, 
                LONG *pcRefOld = NULL);
HRESULT GetSizeOfFile(LPCTSTR lpszFile, LPDWORD lpSize);
HRESULT CleanOrphanedRegistry(
                LPCTSTR szFileName, 
                LPCTSTR szClientClsId,
                LPCTSTR szTypeLibCLSID);
HRESULT UnregisterOCX(LPCTSTR pszFile);
HRESULT GetDirectory(
                UINT nDirType, 
                LPTSTR szDirBuffer, 
                int nBufSize, 
                LPCTSTR szOCXFullName = NULL);
/*
HRESULT GetTypeLibId(
                LPCTSTR lpszClientClsId, 
                LPTSTR lpszTypeLibId, 
                LONG* pLibIdSize);
*/
HRESULT CleanInterfaceEntries(LPCTSTR lpszTypeLibCLSID);
HRESULT ConvertToLongFileName(
                LPTSTR lpszShortFileName,
                BOOL bToUpper = FALSE);

void RemoveList(LPCLSIDLIST_ITEM lpListHead);
BOOL ReadInfFileNameFromRegistry(LPCTSTR lpszCLSID, LPTSTR lpszInf, LONG nBufLen);
BOOL WriteInfFileNameToRegistry(LPCTSTR lpszCLSID, LPTSTR lpszInf);

HRESULT FindDLLInModuleUsage(
      LPTSTR lpszFileName,
      LPCTSTR lpszCLSID,
      DWORD &iSubKey);
HRESULT
ExpandCommandLine(
    LPCSTR szSrc,
    LPSTR szBuf,
    DWORD cbBuffer,
    const char * szVars[],
    const char * szValues[]);

BOOL PatternMatch(LPCTSTR szModName, LPTSTR szSectionName);

DWORD OCCGetLongPathName( LPTSTR szLong, LPCTSTR szShort, DWORD cchBuffer );

TCHAR *CatPathStrN( TCHAR *szDst, const TCHAR *szHead, const TCHAR *szTail, int cchDst );

//=--------------------------------------------------------------------------=
// allocates a temporary buffer that will disappear when it goes out of scope
// NOTE: be careful of that -- make sure you use the string in the same or
// nested scope in which you created this buffer. people should not use this
// class directly.  use the macro(s) below.
//
class TempBuffer {
  public:
    TempBuffer(ULONG cBytes) {
        m_pBuf = (cBytes <= 120) ? &m_szTmpBuf : CoTaskMemAlloc(cBytes);
        m_fHeapAlloc = (cBytes > 120);
    }
    ~TempBuffer() {
        if (m_pBuf && m_fHeapAlloc) CoTaskMemFree(m_pBuf);
    }
    void *GetBuffer() {
        return m_pBuf;
    }

  private:
    void *m_pBuf;
    // we'll use this temp buffer for small cases.
    //
    char  m_szTmpBuf[120];
    unsigned m_fHeapAlloc:1;
};

//=--------------------------------------------------------------------------=
// string helpers.
//
// given and ANSI String, copy it into a wide buffer.
// be careful about scoping when using this macro!
//
// how to use the below two macros:
//
//  ...
//  LPSTR pszA;
//  pszA = MyGetAnsiStringRoutine();
//  MAKE_WIDEPTR_FROMANSI(pwsz, pszA);
//  MyUseWideStringRoutine(pwsz);
//  ...
//
// similarily for MAKE_ANSIPTR_FROMWIDE.  note that the first param does not
// have to be declared, and no clean up must be done.
//
#define MAKE_WIDEPTR_FROMANSI(ptrname, ansistr) \
    long __l##ptrname = (lstrlen(ansistr) + 1) * sizeof(WCHAR); \
    TempBuffer __TempBuffer##ptrname(__l##ptrname); \
    MultiByteToWideChar(CP_ACP, 0, ansistr, -1, (LPWSTR)__TempBuffer##ptrname.GetBuffer(), __l##ptrname); \
    LPWSTR ptrname = (LPWSTR)__TempBuffer##ptrname.GetBuffer()

//
// Note: allocate lstrlenW(widestr) * 2 because its possible for a UNICODE 
// character to map to 2 ansi characters this is a quick guarantee that enough
// space will be allocated.
//
#define MAKE_ANSIPTR_FROMWIDE(ptrname, widestr) \
    long __l##ptrname = (lstrlenW(widestr) + 1) * 2 * sizeof(char); \
    TempBuffer __TempBuffer##ptrname(__l##ptrname); \
    WideCharToMultiByte(CP_ACP, 0, widestr, -1, (LPSTR)__TempBuffer##ptrname.GetBuffer(), __l##ptrname, NULL, NULL); \
    LPSTR ptrname = (LPSTR)__TempBuffer##ptrname.GetBuffer()

#endif
