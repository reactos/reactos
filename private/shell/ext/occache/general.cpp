#include "general.h"
#include "parseinf.h"

#include <mluisupp.h>

///////////////////////////////////////////////////////////////////////////////
// For retriving data from a CONTROLDATA struct

UINT GetTotalNumOfFiles(LPCONTROLPIDL lpcpidl)
{
    return (lpcpidl != NULL ? lpcpidl->ci.cTotalFiles : 0);
}

DWORD GetSizeSaved(LPCONTROLPIDL lpcpidl)
{
    return (lpcpidl != NULL ? lpcpidl->ci.dwTotalSizeSaved : 0);
}

BOOL GetSizeSaved(LPCONTROLPIDL pcpidl, LPTSTR lpszBuf)
{
    Assert(pcpidl != NULL);
    Assert(lpszBuf != NULL);
    if (pcpidl == NULL || lpszBuf == NULL)
        return FALSE;

    DWORD dwTotal = GetSizeSaved(pcpidl);
    if (dwTotal > 0)
    {
        TCHAR szSize[20];
        TCHAR szBuf[MAX_KILOBYTE_ABBREV_LEN + 1];

        dwTotal = (dwTotal < 1024 ? 1024 : dwTotal);
        wsprintf(szSize, "%d", (dwTotal / 1024));

        // insert commas to separate groups of digits
        int nLen = lstrlen(szSize);
        int i = 0, j = (nLen <= 3 ? nLen : (nLen % 3));
        TCHAR *pCh = szSize + j;
        for (; i < j; i++)
            lpszBuf[i] = szSize[i];
        for (; *pCh != '\0'; i++, pCh++)
        {
            if (((pCh - szSize) % 3 == j) && (i > 0))
                lpszBuf[i++] = ',';
            lpszBuf[i] = *pCh;
        }
        lpszBuf[i] = '\0';

        MLLoadString(IDS_KILOBYTE_ABBREV, szBuf, MAX_KILOBYTE_ABBREV_LEN);
        lstrcat(lpszBuf, szBuf);
    }
    else
    {
        lstrcpy(lpszBuf, g_szUnknownData);
    }

    return TRUE;
}

UINT GetStatus(LPCONTROLPIDL pcpidl)
{
    return (pcpidl != NULL ? pcpidl->ci.dwStatus : STATUS_CTRL_UNKNOWN);
}

BOOL GetStatus(LPCONTROLPIDL pcpidl, LPTSTR lpszBuf, int nBufSize)
{
    Assert(pcpidl != NULL);
    Assert(lpszBuf != NULL);
    if (pcpidl == NULL || lpszBuf == NULL)
        return FALSE;

    switch (GetStatus(pcpidl))
    {
    case STATUS_CTRL_UNKNOWN:
        MLLoadString(IDS_STATUS_UNKNOWN, lpszBuf, nBufSize);
        break;
    case STATUS_CTRL_INSTALLED:
        MLLoadString(IDS_STATUS_INSTALLED, lpszBuf, nBufSize);
        break;
    case STATUS_CTRL_SHARED:
        MLLoadString(IDS_STATUS_SHARED, lpszBuf, nBufSize);
        break;
    case STATUS_CTRL_DAMAGED:
        MLLoadString(IDS_STATUS_DAMAGED, lpszBuf, nBufSize);
        break;
    case STATUS_CTRL_UNPLUGGED:
        MLLoadString(IDS_STATUS_UNPLUGGED, lpszBuf, nBufSize);
        break;
    default:
        lstrcpy(lpszBuf, g_szUnknownData);
    }

    return TRUE;
}

BOOL GetTimeInfo(LPCONTROLPIDL lpcpidl, int nFlag, FILETIME* lpTime)
{
    Assert(lpcpidl != NULL && lpTime != NULL);

    if (lpcpidl == NULL || lpTime == NULL)
        return FALSE;

    BOOL fResult = TRUE;

    switch (nFlag)
    {
    case SI_CREATION:
        *lpTime = lpcpidl->ci.timeCreation;
        break;

    case SI_LASTACCESS:
        *lpTime = lpcpidl->ci.timeLastAccessed;
        break;

    default:
        lpTime->dwLowDateTime = lpTime->dwLowDateTime = 0;
        fResult = FALSE;
    }

    return fResult;
}

LPCTSTR GetStringInfo(LPCONTROLPIDL lpcpidl, int nFlag)
{
    switch (nFlag)
    {
    case SI_CONTROL:
        return (lpcpidl != NULL ? lpcpidl->ci.szName : NULL);
    case SI_LOCATION:
        return (lpcpidl != NULL ? lpcpidl->ci.szFile : NULL);
    case SI_VERSION:
        return (lpcpidl != NULL ? lpcpidl->ci.szVersion : NULL);
    case SI_CLSID:
        return (lpcpidl != NULL ? lpcpidl->ci.szCLSID : NULL);
    case SI_CREATION:
        return (lpcpidl != NULL ? lpcpidl->ci.szCreation : NULL);
    case SI_LASTACCESS:
        return (lpcpidl != NULL ? lpcpidl->ci.szLastAccess : NULL);
    case SI_TYPELIBID:
        return (lpcpidl != NULL ? lpcpidl->ci.szTypeLibID : NULL);
    case SI_CODEBASE:
        return (lpcpidl != NULL ? lpcpidl->ci.szCodeBase : NULL);

    }

    return NULL;
}

BOOL GetDependentFile(
                  LPCONTROLPIDL lpcpidl, 
                  UINT iFile, 
                  LPTSTR lpszFile, 
                  DWORD *pdwSize)
{
    if (lpszFile == NULL || 
        pdwSize == NULL ||
        iFile >= GetTotalNumOfFiles(lpcpidl))
    {
        return FALSE;
    }

    LPDEPENDENTFILEINFO pInfo = &(lpcpidl->ci.dependentFile);
    lstrcpy(lpszFile, ((LPDEPENDENTFILEINFO)(pInfo + iFile))->szFile);
    *pdwSize = ((LPDEPENDENTFILEINFO)(pInfo + iFile))->dwSize;

    return TRUE;
}

void GetContentBools( LPCONTROLPIDL lpcpidl, BOOL *pbHasActiveX, BOOL *pbHasJava )
{
    if ( lpcpidl != NULL )
    {
        *pbHasActiveX = lpcpidl->ci.dwHasActiveX != 0;
        *pbHasJava = lpcpidl->ci.dwHasJava != 0;
    }
    else
    {
        *pbHasActiveX = *pbHasJava = FALSE;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Other helper functions

void GenerateEvent(
              LONG lEventId, 
              LPITEMIDLIST pidlFolder, 
              LPITEMIDLIST pidlIn, 
              LPITEMIDLIST pidlNewIn)
{
    LPITEMIDLIST pidl = ILCombine(pidlFolder, pidlIn);
    if (pidl)
    {
        if (pidlNewIn)
        {
            LPITEMIDLIST pidlNew = ILCombine(pidlFolder, pidlNewIn);
            if (pidlNew)
            {
                SHChangeNotify(lEventId, SHCNF_IDLIST, pidl, pidlNew);
                ILFree(pidlNew);
            }
        }
        else
        {
            SHChangeNotify(lEventId, SHCNF_IDLIST, pidl, NULL);
        }
        SHChangeNotifyHandleEvents();
        ILFree(pidl);
    }
}

HICON GetDefaultOCIcon(LPCONTROLPIDL lpcpidl)
{
    DWORD idIcon = IDI_DEFAULTOCXICON;

    if ( lpcpidl->ci.dwIsDistUnit )
    {
        if ( lpcpidl->ci.dwHasJava )
        {
            if ( lpcpidl->ci.dwHasActiveX )
                idIcon = IDI_DEFAULTMIXEDICON;
            else
                idIcon = IDI_DEFAULTJAVAICON;
        }
    }

    return LoadIcon(g_hInst, MAKEINTRESOURCE(idIcon));
}



HCURSOR StartWaitCur()
{
    HCURSOR hCur = LoadCursor(NULL, IDC_WAIT);
    return (hCur != NULL ? SetCursor(hCur) : NULL);
}

void EndWaitCur(HCURSOR hCurOld)
{
    if (hCurOld != NULL)
        SetCursor(hCurOld);
}


// The place to get the # of days before a control becomes expired.
const LPCTSTR g_lpszKeyExpire = TEXT("SOFTWARE\\Microsoft\\Windows"
    "\\CurrentVersion\\Internet Settings\\ActiveX Cache\\Expire");
const LPCTSTR g_szValueExpire = TEXT("DaysBeforeExpire");
const LPCTSTR g_szValueAutoExpire = TEXT("DaysBeforeAutoExpire");
ULONG g_nDaysGeneral = 0;
ULONG g_nDaysAuto = 0;

void GetDaysBeforeExpire(ULONG *pnDays, BOOL fGeneral)
{
    HKEY  hkey;
    DWORD dwSize  = sizeof(ULONG);
    LONG  lResult;

    ASSERT(pnDays != NULL);

    if ( fGeneral && g_nDaysGeneral )
    {
        *pnDays = g_nDaysGeneral;
        return;
    }
    else if ( !fGeneral && g_nDaysAuto )
    {
        *pnDays = g_nDaysAuto;
        return;
    }

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_lpszKeyExpire, 0, KEY_READ,
        &hkey);
    if (lResult == ERROR_SUCCESS) {
        lResult = RegQueryValueEx(hkey, (fGeneral ? g_szValueExpire : g_szValueAutoExpire), NULL, NULL,
            (LPBYTE)pnDays, &dwSize);
        RegCloseKey(hkey);
    }
    if (lResult != ERROR_SUCCESS)
        *pnDays = (fGeneral ? DEFAULT_DAYS_BEFORE_EXPIRE : DEFAULT_DAYS_BEFORE_AUTOEXPIRE);

    if ( fGeneral )
        g_nDaysGeneral = *pnDays;
    else
        g_nDaysAuto = *pnDays;
}

void GetDaysBeforeExpireGeneral(ULONG *pnDays)
{
    GetDaysBeforeExpire(pnDays, TRUE);
}

void GetDaysBeforeExpireAuto(ULONG *pnDays)
{
    GetDaysBeforeExpire(pnDays, FALSE);
}

HRESULT WINAPI RemoveControlByHandle2(
                         HANDLE hControlHandle,
                         BOOL bForceRemove, /* = FALSE */
                         BOOL bSilent)
{
    CCacheItem *pci = (CCacheItem *)hControlHandle;
    CoFreeUnusedLibraries();
    return pci->RemoveFiles( pci->m_szTypeLibID, bForceRemove, pci->ItemType() == CCacheDistUnit::s_dwType, bSilent );
}

HRESULT WINAPI RemoveControlByName2(
                         LPCTSTR lpszFile,
                         LPCTSTR lpszCLSID,
                         LPCTSTR lpszTypeLibID,
                         BOOL bForceRemove, /* = FALSE */
                         DWORD dwIsDistUnit, /* = FALSE */
                         BOOL bSilent)
{
    if (lpszFile == NULL || lpszCLSID == NULL)
        return HRESULT_FROM_WIN32(ERROR_BAD_ARGUMENTS);

    CoFreeUnusedLibraries();

    HRESULT hr = S_OK;
    CParseInf parseInf;

    if (!dwIsDistUnit)
    {
        hr = parseInf.DoParse(lpszFile, lpszCLSID);
    }
    else
    {
        hr = parseInf.DoParseDU(lpszFile, lpszCLSID);
    }
    if (SUCCEEDED(hr))
    {
        hr = parseInf.RemoveFiles(lpszTypeLibID, bForceRemove, dwIsDistUnit, bSilent);
    }

    return hr;
}
