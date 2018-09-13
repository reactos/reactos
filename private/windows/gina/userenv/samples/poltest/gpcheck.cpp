#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <windows.h>
#include <tchar.h>
#include <lm.h>
#include <ole2.h>
#include <olectl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <commctrl.h>
#include <prsht.h>
#include <dsshell.h>
#include <dsgetdc.h>
#include <mmc.h>
#include <initguid.h>
#include <gpedit.h>
#define _USERENV_NO_LINK_APIS_ 1
#include <userenv.h>
#include <userenvp.h>


#include "poltest.h"


typedef struct _GPOINFO {
    TCHAR szGPOName[50];
    DWORD dwFlags;
} GPOINFO, *LPGPOINFO;

#define GPO_FLAG_VERSION   1
#define GPO_FLAG_REGISTRY  2

LPTSTR CheckSlash (LPTSTR lpDir);

//*************************************************************
//
//  GetDCList()
//
//  Purpose:    Reads the DC list from the registry
//
//  Parameters:
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

LPTSTR GetDCList (BOOL bZero)
{
    HKEY hKey;
    LPTSTR lpList = NULL;
    LPTSTR lpTemp;
    DWORD dwType, dwSize;


    lpList = (LPTSTR) LocalAlloc (LPTR, 256 * sizeof(TCHAR));

    if (!lpList)
        return NULL;


    if (RegOpenKeyEx (HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Poltest"),
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = 256 * sizeof(TCHAR);
        RegQueryValueEx (hKey, TEXT("DCList"), NULL, &dwType,
                         (LPBYTE) lpList, &dwSize);

        RegCloseKey (hKey);
    }

    if (bZero) {
        lpTemp = lpList;

        while (*lpTemp) {
            if (*lpTemp == TEXT(';')) {
                *lpTemp = TEXT('\0');
            }

            lpTemp++;
        }
    }

    if (*lpList == TEXT('\0')) {
        LocalFree (lpList);
        return NULL;
    }

    return lpList;
}

//*************************************************************
//
//  GetDomainName()
//
//  Purpose:    Reads the domain name from the registry
//
//  Parameters:
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

LPTSTR GetDomainName (BOOL bZero)
{
    HKEY hKey;
    LPTSTR lpList = NULL;
    LPTSTR lpTemp;
    DWORD dwType, dwSize;


    lpList = (LPTSTR) LocalAlloc (LPTR, 256 * sizeof(TCHAR));

    if (!lpList)
        return NULL;


    if (RegOpenKeyEx (HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Poltest"),
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = 256 * sizeof(TCHAR);
        RegQueryValueEx (hKey, TEXT("DomainName"), NULL, &dwType,
                         (LPBYTE) lpList, &dwSize);

        RegCloseKey (hKey);
    }

    if (bZero) {
        lpTemp = lpList;

        while (*lpTemp) {
            if (*lpTemp == TEXT('.')) {
                *lpTemp = TEXT('\0');
            }

            lpTemp++;
        }
    }


    if (*lpList == TEXT('\0')) {
        LocalFree (lpList);
        return NULL;
    }

    return lpList;
}

//*************************************************************
//
//  CheckSpecificGPO()
//
//  Purpose:    Checks a specific GPO's for the requested
//              attributes
//
//  Parameters: lpGPOName  - gpo name
//              dwFlags    - GPO_FLAG_*
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL CheckSpecificGPO (LPTSTR lpGPOName, DWORD dwFlags)
{
    LPTSTR lpList;
    LPTSTR lpDomainName;
    LPTSTR lpDomainName2;
    LPTSTR lpDC, lpTemp;
    TCHAR szPath[MAX_PATH];
    TCHAR szLDAPPath[MAX_PATH];
    TCHAR szMsg[512];
    WIN32_FILE_ATTRIBUTE_DATA fad;
    DWORD dwFileSysVersion;
    LPTSTR lpEnd;
    FILETIME filetime;
    SYSTEMTIME systime;
    TCHAR szDate[50];
    TCHAR szTime[50];
    IADs *pADs;
    VARIANT var;
    BSTR bstrProperty;
    HRESULT hr;



    lpList = GetDCList (TRUE);

    if (!lpList) {
        AddString (TEXT("You need to enter the list of DC names"));
        return TRUE;
    }

    lpDomainName = GetDomainName (FALSE);

    if (!lpDomainName) {
        AddString (TEXT("You need to enter a domain name"));
        LocalFree (lpList);
        return TRUE;
    }

    CoInitialize(NULL);

    AddString (TEXT("============================================"));
    wsprintf (szMsg, TEXT("Listing GPO %s's information from all DCs"), lpGPOName);
    AddString (szMsg);


    lpDC = lpList;

    while (*lpDC) {

        AddString (TEXT("------------"));

        wsprintf (szMsg, TEXT("Checking DC:  %s"), lpDC);
        AddString (szMsg);

        wsprintf (szPath, TEXT("\\\\%s\\SysVol\\%s\\policies\\%s"),
                  lpDC, lpDomainName, lpGPOName);

        if (!GetFileAttributesEx (szPath, GetFileExInfoStandard, &fad)) {
            wsprintf (szMsg, TEXT("Failed to find %s with %d"), szPath, GetLastError());
            AddString (szMsg);
            goto LoopAgain;
        }

        lpEnd = CheckSlash (szPath);

        if (dwFlags & GPO_FLAG_VERSION) {
            lstrcpy (lpEnd, TEXT("gpt.ini"));

            dwFileSysVersion = GetPrivateProfileInt(TEXT("General"), TEXT("Version"), 0, szPath);


            lpDomainName2 = GetDomainName (TRUE);

            lstrcpy (szLDAPPath, TEXT("LDAP://"));
            lstrcat (szLDAPPath, lpDC);
            lstrcat (szLDAPPath, TEXT("/CN="));
            lstrcat (szLDAPPath, lpGPOName);
            lstrcat (szLDAPPath, TEXT(",CN=Policies,CN=System"));

            lpTemp = lpDomainName2;

            while (*lpTemp) {

                lstrcat (szLDAPPath, TEXT(",DC="));
                lstrcat (szLDAPPath, lpTemp);
                lpTemp += lstrlen(lpTemp) + 1;
            }

            hr = ADsGetObject(szLDAPPath, IID_IADs, (void **)&pADs);

            if (FAILED(hr)) {
                wsprintf (szMsg, TEXT("ADsGetObject failed with 0x%x"), hr);
                AddString (szMsg);
                goto LoopAgain;
            }


            VariantInit(&var);
            bstrProperty = SysAllocString (L"versionNumber");

            hr = pADs->Get(bstrProperty, &var);

            if (FAILED(hr)) {
                wsprintf (szMsg, TEXT("pADs->Get failed with 0x%x"), hr);
                AddString (szMsg);
                SysFreeString (bstrProperty);
                VariantClear (&var);
                goto LoopAgain;
            }

            wsprintf (szMsg, TEXT("DS Version:  %d"), var.lVal);
            AddString (szMsg);

            SysFreeString (bstrProperty);
            VariantClear (&var);
            pADs->Release();

            wsprintf (szMsg, TEXT("SysVol Version:  %d"), dwFileSysVersion);
            AddString (szMsg);
        }

        if (dwFlags & GPO_FLAG_REGISTRY) {

            lstrcpy (lpEnd, TEXT("machine\\registry.pol"));

            if (GetFileAttributesEx (szPath, GetFileExInfoStandard, &fad)) {

                FileTimeToLocalFileTime (&fad.ftLastWriteTime, &filetime);
                FileTimeToSystemTime (&filetime, &systime);

                GetDateFormat (LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systime,
                               NULL, szDate, 50);

                GetTimeFormat (LOCALE_USER_DEFAULT, 0, &systime,
                               NULL, szTime, 50);

                wsprintf (szMsg, TEXT("Last Registry Update (machine):  %s %s"), szDate, szTime);
                AddString (szMsg);
            }

            lstrcpy (lpEnd, TEXT("user\\registry.pol"));

            if (GetFileAttributesEx (szPath, GetFileExInfoStandard, &fad)) {

                FileTimeToLocalFileTime (&fad.ftLastWriteTime, &filetime);
                FileTimeToSystemTime (&filetime, &systime);

                GetDateFormat (LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systime,
                               NULL, szDate, 50);

                GetTimeFormat (LOCALE_USER_DEFAULT, 0, &systime,
                               NULL, szTime, 50);

                wsprintf (szMsg, TEXT("Last Registry Update (user):        %s %s"), szDate, szTime);
                AddString (szMsg);
            }
        }

LoopAgain:

        lpDC += lstrlen(lpDC) + 1;
    }

    CoUninitialize();

    LocalFree (lpList);
    LocalFree (lpDomainName);

    return TRUE;
}


BOOL CALLBACK DomainInfo (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
        case WM_INITDIALOG:
            {
            LPTSTR lpTemp;

            lpTemp = GetDomainName (FALSE);

            if (lpTemp) {
                SetDlgItemText (hDlg, IDC_NAME, lpTemp);
                LocalFree (lpTemp);
            }

            lpTemp = GetDCList (FALSE);

            if (lpTemp) {
                SetDlgItemText (hDlg, IDC_DCLIST, lpTemp);
                LocalFree (lpTemp);
            }
            }
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                HKEY hKey;
                DWORD dwDisp;
                TCHAR szBuffer[512];


                if (RegCreateKeyEx (HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Poltest"),
                                    0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE,
                                    NULL, &hKey, &dwDisp) == ERROR_SUCCESS) {

                    GetDlgItemText (hDlg, IDC_NAME, szBuffer, 512);
                    RegSetValueEx (hKey, TEXT("DomainName"), NULL, REG_SZ,
                                   (LPBYTE) szBuffer, (lstrlen(szBuffer) + 1) *sizeof(TCHAR));

                    GetDlgItemText (hDlg, IDC_DCLIST, szBuffer, 512);
                    RegSetValueEx (hKey, TEXT("DCList"), NULL, REG_SZ,
                                   (LPBYTE) szBuffer, (lstrlen(szBuffer) + 1) *sizeof(TCHAR));

                    RegCloseKey (hKey);
                }


                EndDialog(hDlg, TRUE);
                return (TRUE);
            }


            if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, FALSE);
                return (TRUE);
            }
            break;
    }

    return (FALSE);
}


BOOL ManageDomainInfo(HWND hWnd)
{

    DialogBox (GetModuleHandle(NULL), TEXT("DOMAIN_INFO"), hWnd, DomainInfo);

    return TRUE;
}

DWORD WINAPI CheckGPOThread (LPGPOINFO lpInfo)
{
    CheckSpecificGPO (lpInfo->szGPOName, lpInfo->dwFlags);
    
    LocalFree (lpInfo);

    return 0;
}

BOOL CALLBACK GPONameDlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
        case WM_INITDIALOG:
            {
            HKEY hKey;
            DWORD dwType, dwSize, dwTemp;
            TCHAR szGPOName[50];

            szGPOName[0] = TEXT('\0');
            if (RegOpenKeyEx (HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Poltest"),
                              0, KEY_READ, &hKey) == ERROR_SUCCESS) {

                dwSize = 50 * sizeof(TCHAR);
                RegQueryValueEx (hKey, TEXT("GPOName"), NULL, &dwType,
                                 (LPBYTE) szGPOName, &dwSize);

                dwSize = sizeof(dwTemp);
                dwTemp = 0;
                RegQueryValueEx (hKey, TEXT("GPONameOptions"), NULL, &dwType,
                                 (LPBYTE) &dwTemp, &dwSize);


                RegCloseKey (hKey);
            }


            SetDlgItemText (hDlg, IDC_NAME, szGPOName);

            if (dwTemp & GPO_FLAG_VERSION) {
                CheckDlgButton (hDlg, IDC_VERSION, BST_CHECKED);
            }

            if (dwTemp & GPO_FLAG_REGISTRY) {
                CheckDlgButton (hDlg, IDC_REGISTRY, BST_CHECKED);
            }


            }
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {

                HKEY hKey;
                DWORD dwDisp, dwTemp;
                TCHAR szGPOName[50];
                LPGPOINFO lpInfo;
                HANDLE hThread;
                DWORD dwID;

                dwTemp = 0;

                if (IsDlgButtonChecked (hDlg, IDC_VERSION)) {
                    dwTemp |= GPO_FLAG_VERSION;
                }

                if (IsDlgButtonChecked (hDlg, IDC_REGISTRY)) {
                    dwTemp |= GPO_FLAG_REGISTRY;
                }


                if (RegCreateKeyEx (HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Poltest"),
                                    0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE,
                                    NULL, &hKey, &dwDisp) == ERROR_SUCCESS) {

                    GetDlgItemText (hDlg, IDC_NAME, szGPOName, 50);
                    RegSetValueEx (hKey, TEXT("GPOName"), NULL, REG_SZ,
                                   (LPBYTE) szGPOName, (lstrlen(szGPOName) + 1) *sizeof(TCHAR));
                    RegSetValueEx (hKey, TEXT("GPONameOptions"), NULL, REG_SZ,
                                   (LPBYTE) &dwTemp, sizeof(dwTemp));

                    RegCloseKey (hKey);
                }

                lpInfo = (LPGPOINFO) LocalAlloc (LPTR, sizeof(GPOINFO));

                if (lpInfo) {
                    lstrcpy (lpInfo->szGPOName, szGPOName);
                    lpInfo->dwFlags = dwTemp;

                    hThread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE)CheckGPOThread,
                                            lpInfo, 0, &dwID);

                    if (hThread) {
                        CloseHandle (hThread);
                    }

                }
                EndDialog(hDlg, TRUE);
                return (TRUE);
            }


            if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, FALSE);
                return (TRUE);
            }

            if (LOWORD(wParam) == IDC_BROWSE) {
                GPOBROWSEINFO info;
                TCHAR szName[500];
                TCHAR szPath[512];
                LPGROUPPOLICYOBJECT pGPO;

                szName[0] = TEXT('\0');
                ZeroMemory (&info, sizeof(info));
                info.dwSize = sizeof(info);
                info.dwFlags = GPO_BROWSE_NOCOMPUTERS;
                info.hwndOwner = hDlg;
                info.lpDSPath = szPath;
                info.dwDSPathSize = 512;
                info.lpName = szName;
                info.dwNameSize = 500;

                if (SUCCEEDED(BrowseForGPO (&info))) {

                    if (SUCCEEDED(CoCreateInstance (CLSID_GroupPolicyObject, NULL,
                                  CLSCTX_SERVER, IID_IGroupPolicyObject,
                                  (void**)&pGPO))) {


                        if (SUCCEEDED(pGPO->OpenDSGPO (szPath, GPO_OPEN_READ_ONLY))){
                            pGPO->GetName (szName, 500);
                            SetDlgItemText (hDlg, IDC_NAME, szName);
                        }
                        pGPO->Release();
                    }
                }
            }
            break;
    }

    return (FALSE);
}


BOOL CheckGPO(HWND hWnd)
{

    DialogBox (GetModuleHandle(NULL), TEXT("GPO_NAME"), hWnd, GPONameDlgProc);

    return TRUE;
}

//*************************************************************
//
//  CheckSlash()
//
//  Purpose:    Checks for an ending slash and adds one if
//              it is missing.
//
//  Parameters: lpDir   -   directory
//
//  Return:     Pointer to the end of the string
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericflo    Created
//
//*************************************************************
LPTSTR CheckSlash (LPTSTR lpDir)
{
    DWORD dwStrLen;
    LPTSTR lpEnd;

    lpEnd = lpDir + lstrlen(lpDir);

    if (*(lpEnd - 1) != TEXT('\\')) {
        *lpEnd =  TEXT('\\');
        lpEnd++;
        *lpEnd =  TEXT('\0');
    }

    return lpEnd;
}
