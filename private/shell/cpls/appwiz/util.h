#ifndef _UTIL_H_
#define _UTIL_H_

#include <appmgmt.h>

STDAPI InstallAppFromFloppyOrCDROM(HWND hwnd);

DWORD ARPGetRestricted(LPCWSTR pszPolicy);
void ARPGetPolicyString(LPCWSTR pszPolicy, LPWSTR pszBuf, int cch);

STDAPI OpenAppMgr(HWND hwnd, int nPage);

void ClearAppInfoData(APPINFODATA * pdata);
void ClearSlowAppInfo(SLOWAPPINFO * pdata);
void ClearPubAppInfo(PUBAPPINFO * pdata);
void ClearManagedApplication(MANAGEDAPPLICATION * pma);

HRESULT ReleaseShellCategoryList(SHELLAPPCATEGORYLIST * psacl);
HRESULT ReleaseShellCategory(SHELLAPPCATEGORY * psac);

// These values can be set in the FILETIME structure to indicate the
// app has never been used.
#define NOTUSED_HIGHDATETIME    0xFFFFFFFF
#define NOTUSED_LOWDATETIME     0xFFFFFFFF

LPTSTR WINAPI ShortSizeFormat64(__int64 dw64, LPTSTR szBuf);

#define ALD_ASSIGNED    0x00000001
#define ALD_EXPIRE      0x00000002
#define ALD_SCHEDULE    0x00000004

#ifndef DOWNLEVEL_PLATFORM
typedef struct tagAddLaterData
{
    DWORD dwMasks;
    SYSTEMTIME stAssigned;         // (in) assigned time 
    SYSTEMTIME stExpire;           // (in) expired time
    SYSTEMTIME stSchedule;         // (in/out) scheduled time 
} ADDLATERDATA, *PADDLATERDATA;

BOOL GetNewInstallTime(HWND hwndParent, PADDLATERDATA pal);
BOOL FormatSystemTimeString(LPSYSTEMTIME pst, LPTSTR pszStr, UINT cchStr);

#ifdef WINNT
EXTERN_C BOOL IsTerminalServicesRunning(void);
#endif // WINNT
#endif // DOWNLEVEL_PLATFORM

#define NUMSTARTPAGES 3
extern const UINT g_uiStartPageId[NUMSTARTPAGES];

// Take an app key name or folder name and separate the number(version) from the name
void InsertSpaceBeforeVersion(LPCTSTR pszIn, LPTSTR pszOut);

// Is this path a valid folder location?
BOOL IsValidAppFolderLocation(LPCTSTR pszFolder);

BOOL PathIsLocalAndFixed(LPCTSTR pszFile);

BOOL IsSlowAppInfoChanged(PSLOWAPPINFO psaiOrig, PSLOWAPPINFO psaiNew);

// Comparison function for systemtimes
int CompareSystemTime(SYSTEMTIME *pst1, SYSTEMTIME *pst2);

void _ARPErrorMessageBox(DWORD dwError);

STDAPI _DuplicateCategoryList(APPCATEGORYINFOLIST * pacl, APPCATEGORYINFOLIST * paclNew);
STDAPI _DestroyCategoryList(APPCATEGORYINFOLIST * pacl);

#endif // _UTIL_H_
