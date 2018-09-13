// 
// Random stuff
//
//


#include "priv.h"
#include "exdisp.h"
#include "mshtml.h"
#include "htiframe.h"
#include "util.h"
#include "resource.h"
#include "appwizid.h"

#ifdef DOWNLEVEL_PLATFORM

#define DF_DEBUGQI 0
#define TF_QISTUB 0

#ifdef DEBUG
#define DEBUG_WAS_DEFINED
#undef DEBUG
#endif

#include "..\lib\dbutil.h"
#include "..\lib\dbutil.cpp"
#include "..\lib\qistub.cpp"

#ifdef DEBUG_WAS_DEFINED
#define DEBUG
#undef DEBUG_WAS_DEFINED
#endif

#endif

#define COMPILE_MULTIMON_STUBS
#include "multimon.h"
#include "mmhelper.h"

#define CPP_FUNCTIONS
#include <crtfree.h>        // declare new, delete, etc.

#define DATEFORMAT_MAX 40

#ifndef DOWNLEVEL_PLATFORM

#include <shguidp.h>

// BUGBUG (scotth): is this okay to do?
#ifdef ENTERCRITICAL
#undef ENTERCRITICAL
#endif
#ifdef LEAVECRITICAL
#undef LEAVECRITICAL
#endif

#define ENTERCRITICAL
#define LEAVECRITICAL

#include "..\lib\uassist.cpp"
#endif //DOWNLEVEL_PLATFORM

// Prototype
BOOL _IsARPAllowed(void);

const VARIANT c_vaEmpty = {0};
//
// BUGBUG: Remove this ugly const to non-const casting if we can
//  figure out how to put const in IDL files.
//
#define PVAREMPTY ((VARIANT*)&c_vaEmpty)

STDAPI OpenAppMgr(HWND hwnd, int nPage)
{
    HRESULT hres = E_FAIL;

    // Make sure we aren't restricted
    if (!_IsARPAllowed())
    {
        ShellMessageBox(g_hinst, hwnd, MAKEINTRESOURCE(IDS_RESTRICTION),
            MAKEINTRESOURCE(IDS_NAME), MB_OK | MB_ICONEXCLAMATION);
    }
    else if ((nPage >= 0) && (nPage < ARRAYSIZE(g_uiStartPageId)))
    {
        TCHAR szCommand[MAX_PATH];
        
        wsprintf(szCommand, (nPage > 0) ? TEXT("mshta.exe %s %d") : TEXT("mshta.exe %s"), TEXT("res://appwiz.cpl/default.hta"), nPage);

        STARTUPINFO rgStartup = {0};
        PROCESS_INFORMATION rgProcess = {0};

        rgStartup.cb = sizeof( rgStartup );
        rgStartup.wShowWindow = SW_SHOWNORMAL;

        if ( CreateProcess( NULL, szCommand, NULL, NULL, FALSE, 0, NULL, NULL,
            &rgStartup, &rgProcess ))
        {
            WaitForInputIdle( rgProcess.hProcess, 10000 );
            CloseHandle( rgProcess.hProcess );
            CloseHandle( rgProcess.hThread );
        }
        
        hres = S_OK;
    }
    return hres;
}


inline void StrFree(LPWSTR psz)
{
    if (psz)
        SHFree(psz);
}


/*-------------------------------------------------------------------------
Purpose: Clear the given app data structure.  Frees any allocated fields.
*/
void ClearAppInfoData(APPINFODATA * pdata)
{
    if (pdata)
    {
        if (pdata->dwMask & AIM_DISPLAYNAME)
            StrFree(pdata->pszDisplayName);
            
        if (pdata->dwMask & AIM_VERSION)
            StrFree(pdata->pszVersion);

        if (pdata->dwMask & AIM_PUBLISHER)
            StrFree(pdata->pszPublisher);
            
        if (pdata->dwMask & AIM_PRODUCTID)
            StrFree(pdata->pszProductID);
            
        if (pdata->dwMask & AIM_REGISTEREDOWNER)
            StrFree(pdata->pszRegisteredOwner);
            
        if (pdata->dwMask & AIM_REGISTEREDCOMPANY)
            StrFree(pdata->pszRegisteredCompany);
            
        if (pdata->dwMask & AIM_LANGUAGE)
            StrFree(pdata->pszLanguage);
            
        if (pdata->dwMask & AIM_SUPPORTURL)
            StrFree(pdata->pszSupportUrl);
            
        if (pdata->dwMask & AIM_SUPPORTTELEPHONE)
            StrFree(pdata->pszSupportTelephone);
            
        if (pdata->dwMask & AIM_HELPLINK)
            StrFree(pdata->pszHelpLink);
            
        if (pdata->dwMask & AIM_INSTALLLOCATION)
            StrFree(pdata->pszInstallLocation);
            
        if (pdata->dwMask & AIM_INSTALLSOURCE)
            StrFree(pdata->pszInstallSource);
            
        if (pdata->dwMask & AIM_INSTALLDATE)
            StrFree(pdata->pszInstallDate);
            
        if (pdata->dwMask & AIM_CONTACT)
            StrFree(pdata->pszContact);

        if (pdata->dwMask & AIM_COMMENTS)
            StrFree(pdata->pszComments);

        if (pdata->dwMask & AIM_IMAGE)
            StrFree(pdata->pszImage);
    }
}


void ClearSlowAppInfo(SLOWAPPINFO * pdata)
{
    if (pdata)
    {
        StrFree(pdata->pszImage);
        pdata->pszImage = NULL;
    }
}


// NOTE: Returns TRUE only if psaiNew has valid info and different from psaiOrig
BOOL IsSlowAppInfoChanged(PSLOWAPPINFO psaiOrig, PSLOWAPPINFO psaiNew)
{
    BOOL bRet = FALSE;

    ASSERT(psaiOrig && psaiNew);

    if (psaiNew)
    {    
        // Compare size first
        if (psaiOrig == NULL)
        {
            bRet = TRUE;
        }
        if (((__int64)psaiNew->ullSize > 0) && (psaiNew->ullSize != psaiOrig->ullSize))
        {
            bRet = TRUE;
        }
        // Now compare the file time
        else if (((0 != psaiNew->ftLastUsed.dwHighDateTime) &&
                  (psaiOrig->ftLastUsed.dwHighDateTime != psaiNew->ftLastUsed.dwHighDateTime))
                 || ((0 != psaiNew->ftLastUsed.dwLowDateTime) &&
                     (psaiOrig->ftLastUsed.dwLowDateTime != psaiNew->ftLastUsed.dwLowDateTime)))
        {
            bRet = TRUE;
        }
        // Compare times used
        else if (psaiOrig->iTimesUsed != psaiNew->iTimesUsed)
        {
            bRet = TRUE;
        }
        // Compare the icon image
        else if ((psaiNew->pszImage != NULL) && (psaiOrig->pszImage != NULL) && lstrcmpi(psaiNew->pszImage, psaiOrig->pszImage))
            bRet = TRUE;

    }
    return bRet;
}

void ClearManagedApplication(MANAGEDAPPLICATION * pma)
{
    if (pma)
    {
        if (pma->pszPackageName)
            LocalFree(pma->pszPackageName);

        if (pma->pszPublisher)
            LocalFree(pma->pszPublisher);

        if (pma->pszPolicyName)
            LocalFree(pma->pszPolicyName);

        if (pma->pszOwner)
            LocalFree(pma->pszOwner);

        if (pma->pszCompany)
            LocalFree(pma->pszCompany);

        if (pma->pszComments)
            LocalFree(pma->pszComments);

        if (pma->pszContact)
            LocalFree(pma->pszContact);
    }
}

/*-------------------------------------------------------------------------
Purpose: Clear the given PUBAPPINFO data structure.  Frees any allocated fields.
*/
void ClearPubAppInfo(PUBAPPINFO * pdata)
{
    if (pdata)
    {
        if ((pdata->dwMask & PAI_SOURCE) && pdata->pszSource)
            StrFree(pdata->pszSource);
    }
}

/*-------------------------------------------------------------------------
Purpose: Frees a specific category structure
*/
HRESULT ReleaseShellCategory(SHELLAPPCATEGORY * psac)
{
    ASSERT(psac);

    if (psac->pszCategory)
    {
        SHFree(psac->pszCategory);
        psac->pszCategory = NULL;
    }
    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: Frees the list of categories
*/
HRESULT ReleaseShellCategoryList(SHELLAPPCATEGORYLIST * psacl)
{
    UINT i;
    SHELLAPPCATEGORY * psac;

    ASSERT(psacl);

    psac = psacl->pCategory;
    
    for (i = 0; i < psacl->cCategories; i++, psac++)
    {
        ReleaseShellCategory(psac);
    }
    return S_OK;
}


#define MAX_INT64_SIZE  30              // 2^64 is less than 30 chars long
#define MAX_COMMA_NUMBER_SIZE   (MAX_INT64_SIZE + 10)
#define MAX_COMMA_AS_K_SIZE     (MAX_COMMA_NUMBER_SIZE + 10)
#define HIDWORD(_qw)    (DWORD)((_qw)>>32)
#define LODWORD(_qw)    (DWORD)(_qw)


void Int64ToStr( _int64 n, LPTSTR lpBuffer)
{
    TCHAR   szTemp[MAX_INT64_SIZE];
    _int64  iChr;

    iChr = 0;

    do {
        szTemp[iChr++] = TEXT('0') + (TCHAR)(n % 10);
        n = n / 10;
    } while (n != 0);

    do {
        iChr--;
        *lpBuffer++ = szTemp[iChr];
    } while (iChr != 0);

    *lpBuffer++ = '\0';
}

// takes a DWORD add commas etc to it and puts the result in the buffer
LPTSTR WINAPI AddCommas64(_int64 n, LPTSTR pszResult)
{
    // BUGBUGBC: 40 is bogus, it requires callers to know their buffer must
    //  be 40

    TCHAR  szTemp[MAX_COMMA_NUMBER_SIZE];
    TCHAR  szSep[5];
    NUMBERFMT nfmt;

    nfmt.NumDigits=0;
    nfmt.LeadingZero=0;
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szSep, ARRAYSIZE(szSep));
    nfmt.Grouping = StrToInt(szSep);
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szSep, ARRAYSIZE(szSep));
    nfmt.lpDecimalSep = nfmt.lpThousandSep = szSep;
    nfmt.NegativeOrder= 0;

    Int64ToStr(n, szTemp);

    // BUGBUG:: Should have passed in size..
    if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, szTemp, &nfmt, pszResult, MAX_COMMA_NUMBER_SIZE) == 0)
        lstrcpy(pszResult, szTemp);

    return pszResult;
}

//
// Add Peta 10^15 and Exa 10^18 to support 64-bit integers.
//
const short pwOrders[] = {IDS_BYTES, IDS_ORDERKB, IDS_ORDERMB,
                          IDS_ORDERGB, IDS_ORDERTB, IDS_ORDERPB, IDS_ORDEREB};

/* converts numbers into sort formats
 *      532     -> 523 bytes
 *      1340    -> 1.3KB
 *      23506   -> 23.5KB
 *              -> 2.4MB
 *              -> 5.2GB
 */
LPTSTR WINAPI ShortSizeFormat64(__int64 dw64, LPTSTR szBuf)
{
    int i;
    _int64 wInt;
    UINT wLen, wDec;
    TCHAR szTemp[MAX_COMMA_NUMBER_SIZE], szOrder[20], szFormat[5];

    if (dw64 < 1000) {
        wsprintf(szTemp, TEXT("%d"), LODWORD(dw64));
        i = 0;
        goto AddOrder;
    }

    for (i = 1; i<ARRAYSIZE(pwOrders)-1 && dw64 >= 1000L * 1024L; dw64 >>= 10, i++);
        /* do nothing */

    wInt = dw64 >> 10;
    AddCommas64(wInt, szTemp);
    wLen = lstrlen(szTemp);
    if (wLen < 3)
    {
        wDec = LODWORD(dw64 - wInt * 1024L) * 1000 / 1024;
        // At this point, wDec should be between 0 and 1000
        // we want get the top one (or two) digits.
        wDec /= 10;
        if (wLen == 2)
            wDec /= 10;

        // Note that we need to set the format before getting the
        // intl char.
        lstrcpy(szFormat, TEXT("%02d"));

        szFormat[2] = TEXT('0') + 3 - wLen;
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,
                szTemp+wLen, ARRAYSIZE(szTemp)-wLen);
        wLen = lstrlen(szTemp);
        wLen += wsprintf(szTemp+wLen, szFormat, wDec);
    }

AddOrder:
    LoadString(HINST_THISDLL, pwOrders[i], szOrder, ARRAYSIZE(szOrder));
    wsprintf(szBuf, szOrder, (LPTSTR)szTemp);

    return szBuf;
}


#define c_szUninstallPolicy     L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Uninstall"


/*-------------------------------------------------------------------------
Purpose: Helper function for ARP's policy check
*/
DWORD ARPGetRestricted(LPCWSTR pszPolicy)
{
    return SHGetRestriction(NULL, TEXT("Uninstall"), pszPolicy);
}


/*-------------------------------------------------------------------------
Purpose: Return a policy string value
*/
void ARPGetPolicyString(LPCWSTR pszPolicy, LPWSTR pszBuf, int cch)
{
    DWORD dwSize, dwType;

    *pszBuf = 0;
    
    // Check local machine first and let it override what the
    // HKCU policy has done.
    dwSize = cch * sizeof(WCHAR);
    if (ERROR_SUCCESS != SHGetValueW(HKEY_LOCAL_MACHINE,
                                     c_szUninstallPolicy, pszPolicy,
                                     &dwType, pszBuf, &dwSize))
    {
        // Check current user if we didn't find anything for the local machine.
        dwSize = cch * sizeof(WCHAR);
        SHGetValueW(HKEY_CURRENT_USER,
                    c_szUninstallPolicy, pszPolicy,
                    &dwType, pszBuf, &dwSize);
    }
}


/*-------------------------------------------------------------------------
Purpose: Returns TRUE if it's okay to start ARP.

*/
BOOL _IsARPAllowed(void)
{
    // If the master "no ARP" policy is set OR a combination of ALL the
    // individual pages are set, then don't even bring up ARP at all.
    return !(ARPGetRestricted(L"NoAddRemovePrograms") ||
             (ARPGetRestricted(L"NoRemovePage") && 
              ARPGetRestricted(L"NoAddPage") &&
              ARPGetRestricted(L"NoWindowsSetupPage")));
}


#ifndef DOWNLEVEL_PLATFORM

/*-------------------------------------------------------------------------
Purpose: Take the error message and give user feedback through messagebox
*/
void _ARPErrorMessageBox(DWORD dwError)
{
    TCHAR szErrorMsg[MAX_PATH];
    szErrorMsg[0] = 0;

    LPTSTR pszMsg = NULL;
    switch (dwError) {
        // The following error code cases are ignored.     
        case ERROR_INSTALL_USEREXIT:
        case ERROR_SUCCESS_REBOOT_REQUIRED:
        case ERROR_SUCCESS_REBOOT_INITIATED:            
            ASSERT(pszMsg == NULL);
            break;

        default:
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError, 0L, szErrorMsg,
                          ARRAYSIZE(szErrorMsg), NULL);
            pszMsg = szErrorMsg;
            break;
    }

    if (pszMsg)
    {
        ShellMessageBox( g_hinst, NULL, pszMsg,
                         MAKEINTRESOURCE( IDS_NAME ),
                         MB_OK | MB_ICONEXCLAMATION);
    }
}

/*-------------------------------------------------------------------------
Purpose: Format the SYSTEMTIME into the following format: "mm/dd/yy h:mm"
*/
BOOL FormatSystemTimeString(LPSYSTEMTIME pst, LPTSTR pszStr, UINT cchStr)
{
    BOOL bRet = FALSE;
    FILETIME ft = {0};

    if (SystemTimeToFileTime(pst, &ft))
    {
        DWORD dwFlags = FDTF_SHORTTIME | FDTF_SHORTDATE;
        bRet = SHFormatDateTime(&ft, &dwFlags, pszStr, cchStr);
    }
    return bRet;
}
#endif //DOWNLEVEL_PLATFORM

/*-------------------------------------------------------------------------
Purpose: Get the correct Date time format for specific locale
*/
BOOL _GetLocaleDateTimeFormat(LPTSTR pszFormat, UINT cchFormat)
{
    TCHAR szTime[DATEFORMAT_MAX];
    TCHAR szDate[DATEFORMAT_MAX];
    if (cchFormat >= (ARRAYSIZE(szTime) + ARRAYSIZE(szDate) + 2))
    {
        LCID lcid = LOCALE_USER_DEFAULT;
        if (GetLocaleInfo(lcid, LOCALE_STIMEFORMAT, szTime, ARRAYSIZE(szTime)) && 
            GetLocaleInfo(lcid, LOCALE_SSHORTDATE, szDate, ARRAYSIZE(szDate)))
        {
            wsprintf(pszFormat, TEXT("%s  %s"), szDate, szTime);
            return TRUE;
        }
    }

    return FALSE;
}

/*-------------------------------------------------------------------------
Purpose: Compare two SYSTEMTIME data

Returnes : 1 : st1 > st2
           0 : st1 == st2
          -1: st1 < st2

NOTE:  We do not compare seconds since ARP does not need that much precision. 
*/
int CompareSystemTime(SYSTEMTIME *pst1, SYSTEMTIME *pst2)
{
    int iRet;

    if (pst1->wYear < pst2->wYear)
        iRet = -1;
    else if (pst1->wYear > pst2->wYear)
        iRet = 1;
    else if (pst1->wMonth < pst2->wMonth)
        iRet = -1;
    else if (pst1->wMonth > pst2->wMonth)
        iRet = 1;
    else if (pst1->wDay < pst2->wDay)
        iRet = -1;
    else if (pst1->wDay > pst2->wDay)
        iRet = 1;
    else if (pst1->wHour < pst2->wHour)
        iRet = -1;
    else if (pst1->wHour > pst2->wHour)
        iRet = 1;
    else if (pst1->wMinute < pst2->wMinute)
        iRet = -1;
    else if (pst1->wMinute > pst2->wMinute)
        iRet = 1;
//    else if (pst1->wSecond < pst2->wSecond)
//        iRet = -1;
//    else if (pst1->wSecond > pst2->wSecond)
//        iRet = 1;
    else
        iRet = 0;

    return(iRet);
}

#ifndef DOWNLEVEL_PLATFORM
/*--------------------------------------------------------------------------
Purpose: Window proc for the add later dialog box
*/
BOOL_PTR CALLBACK AddLaterDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            PADDLATERDATA pald = (PADDLATERDATA)lp;

            // We should definitely have this (dli)
            ASSERT(pald);

            SYSTEMTIME stInit = {0};
            // Get the current local time
            GetLocalTime(&stInit);

            // Has this app already expired?
            if ((pald->dwMasks & ALD_EXPIRE) &&
                (CompareSystemTime(&pald->stExpire, &stInit) > 0))
            {
                // NO,
                
                // Assigned time does not make sense if the assigned time has already
                // passed
                if ((pald->dwMasks & ALD_ASSIGNED) &&
                    (CompareSystemTime(&pald->stAssigned, &stInit) <= 0))
                    pald->dwMasks &= ~ALD_ASSIGNED;

                // find the date/time picker window
                HWND hwndPicker = GetDlgItem(hDlg, IDC_PICKER);

                // always check "add later" radio button initially
                CheckDlgButton(hDlg, IDC_ADDLATER, BST_CHECKED);

                TCHAR szFormat[MAX_PATH];
                if (_GetLocaleDateTimeFormat(szFormat, ARRAYSIZE(szFormat)))
                {
                    // set the locale date time format
                    DateTime_SetFormat(hwndPicker, szFormat);

                    // The new time can only be in the future, so set the current time
                    // as the lower limit
                    DateTime_SetRange(hwndPicker, GDTR_MIN, &stInit);

                    // Do we have a schedule (in the future) already?
                    // Schedule in the past means nothing
                    if ((pald->dwMasks & ALD_SCHEDULE) &&
                        (CompareSystemTime(&pald->stSchedule, &stInit) >= 0))
                    {
                        // Set our initial value to this schedule
                        stInit = pald->stSchedule;
                    }

                    // Set the initial value in date/time picker
                    DateTime_SetSystemtime(hwndPicker, GDT_VALID, &stInit);

                    // Uncheck the SCHEDULE flag so that we know we don't have a new
                    // schedule, yet
                    pald->dwMasks &= ~ALD_SCHEDULE;

                    SetWindowLongPtr(hDlg, DWLP_USER, lp);

                    return TRUE;
                }
            }
            else
            {
                // Yes, it's expired, warn the user
                ShellMessageBox(g_hinst, hDlg, MAKEINTRESOURCE(IDS_EXPIRED),
                                MAKEINTRESOURCE(IDS_NAME), MB_OK | MB_ICONEXCLAMATION);

                // Then end the dialog. 
                EndDialog(hDlg, 0);
            }
            return FALSE;
        }
        break;
        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wp, lp))
            {
                case IDC_ADDLATER:
                case IDC_UNSCHEDULE:
                {
                    HWND hwndPicker = GetDlgItem(hDlg, IDC_PICKER);
                    EnableWindow(hwndPicker, IsDlgButtonChecked(hDlg, IDC_ADDLATER));
                }
                break;
                
                case IDOK:
                {
                    PADDLATERDATA pald = (PADDLATERDATA)GetWindowLongPtr(hDlg, DWLP_USER);

                    // we did set window long ptr this should be there. 
                    ASSERT(pald);

                    // did the user choose to add later?
                    if (IsDlgButtonChecked(hDlg, IDC_ADDLATER))
                    {
                        // Yes
                        // Let's find out if the time user has chosen is valid
                        
#define LATER_THAN_ASSIGNED_TIME 1
#define LATER_THAN_EXPIRED_TIME 2
                        int iStatus = 0;
                        HWND hwndPicker = GetDlgItem(hDlg, IDC_PICKER);
                        DateTime_GetSystemtime(hwndPicker, &pald->stSchedule);

                        // Is this time later than the assigned time?
                        if ((pald->dwMasks & ALD_ASSIGNED) &&
                            (CompareSystemTime(&pald->stSchedule, &pald->stAssigned) > 0))
                            iStatus = LATER_THAN_ASSIGNED_TIME;

                        // Is this time later than the expired time?
                        if ((pald->dwMasks & ALD_EXPIRE) &&
                            (CompareSystemTime(&pald->stSchedule, &pald->stExpire) >= 0))
                            iStatus = LATER_THAN_EXPIRED_TIME;

                        // Is either of the above two cases TRUE?
                        if (iStatus > 0)
                        {
                            TCHAR szDateTime[MAX_PATH];
                            
                            // Is the time user chose passed expired time or assigned?
                            BOOL bExpired = (iStatus == LATER_THAN_EXPIRED_TIME);
                            
                            // Get the time string
                            if (FormatSystemTimeString(bExpired ? &pald->stExpire : &pald->stAssigned,
                                szDateTime, ARRAYSIZE(szDateTime)))
                            {
                                TCHAR szFinal[MAX_PATH * 2];
                                TCHAR szWarn[MAX_PATH];
                                LoadString(g_hinst,  bExpired ? IDS_PASSEXPIRED : IDS_PASSASSIGNED,
                                           szWarn, ARRAYSIZE(szWarn));
                                
                                wsprintf(szFinal, szWarn, szDateTime, szDateTime);
                                ShellMessageBox(g_hinst, hDlg, szFinal, 
                                                MAKEINTRESOURCE(IDS_NAME), MB_OK | MB_ICONEXCLAMATION);
                            }
                        }
                        else
                            // No, we are okay to go
                            pald->dwMasks |= ALD_SCHEDULE;
                    }
                }

                //
                // fall through
                //
                case IDCANCEL:
                    EndDialog(hDlg, (GET_WM_COMMAND_ID(wp, lp) == IDOK));
                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

/*-------------------------------------------------------------------------
Purpose: GetNewInstallTime

         Start up the Add Later dialog box to get the new install schedule
         (represented by a SYSTEMTIME data struct) 
*/
BOOL GetNewInstallTime(HWND hwndParent, PADDLATERDATA pal)
{
    return (DialogBoxParam(g_hinst, MAKEINTRESOURCE(DLG_ADDLATER),
                           hwndParent, AddLaterDlgProc, (LPARAM)pal) == IDOK);
}
#endif //DOWNLEVEL_PLATFORM

// Take the name of the potential app folder and see if it ends with numbers or dots
// if it does, let's separate the numbers and see if there is a match.
// It's inspired by cases like Office8.0 or MSVC50 or Bookshelf98
// NOTE: we can't use the key words without the numbers, it might lead to mistake
// in case the user has two versions of the same software on one machine. (there might
// be something we can do though, I am too tired to think about this now)
void InsertSpaceBeforeVersion(LPCTSTR pszIn, LPTSTR pszOut)
{
    ASSERT(IS_VALID_STRING_PTR(pszIn, -1));
    ASSERT(IS_VALID_STRING_PTR(pszOut, -1));

    // Copy the old string into the buffer
    lstrcpy(pszOut, pszIn);

    // Find the end of the string
    LPTSTR pszEnd = pszOut + lstrlen(pszOut);
    ASSERT(pszEnd > pszOut);

    // Go back until we can't see numbers or '.'
    LPTSTR pszLastChar = CharPrev(pszOut, pszEnd);
    LPTSTR pszPrev = pszLastChar;
    while ((pszPrev > pszOut) && (((*pszPrev <= TEXT('9')) && (*pszPrev >= TEXT('0'))) || (*pszPrev == TEXT('.'))))
        pszPrev = CharPrev(pszOut, pszPrev);

    // Did we find any numbers at the end?
    if ((pszPrev < pszLastChar) && IsCharAlphaNumeric(*pszPrev))
    {
        // Yes, let's stick a ' ' in between
        TCHAR szNumbers[MAX_PATH];
        lstrcpy(szNumbers, ++pszPrev);
        *(pszPrev++) = TEXT(' ');
        lstrcpy(pszPrev, szNumbers);
    }
}

#ifndef DOWNLEVEL_PLATFORM
// 
// Basic sanity check on whether the app folder location is valid. 
// Return Value:
// TRUE does not mean it is valid.
// FALSE means it definitely is not valid.
//
BOOL IsValidAppFolderLocation(LPCTSTR pszFolder)
{
    ASSERT(IS_VALID_STRING_PTR(pszFolder, -1));
    BOOL bRet = FALSE;
    if (!PathIsRoot(pszFolder) && PathFileExists(pszFolder) && PathIsDirectory(pszFolder))
    {
        TCHAR szPath[MAX_PATH];
        if (lstrcpy(szPath, pszFolder) && PathStripToRoot(szPath))
            bRet = (GetDriveType(szPath) == DRIVE_FIXED);
    }

    return bRet;
}

#ifdef WINNT
EXTERN_C BOOL IsTerminalServicesRunning(void)
{
    static int s_fIsTerminalServer = -1;

    if (s_fIsTerminalServer == -1)
    {
        BOOL TSAppServer;
        BOOL TSRemoteAdmin;
    
        OSVERSIONINFOEX osVersionInfo;
        DWORDLONG dwlConditionMask = 0;
    
        ZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFOEX));
        osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        osVersionInfo.wSuiteMask = VER_SUITE_TERMINAL;
    
        VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );
    
        TSAppServer = (int)VerifyVersionInfo(&osVersionInfo, VER_SUITENAME, dwlConditionMask);
    
    
        ZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFOEX));
        osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        osVersionInfo.wSuiteMask = VER_SUITE_SINGLEUSERTS;
    
        VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );
    
        TSRemoteAdmin = (int)VerifyVersionInfo(&osVersionInfo, VER_SUITENAME, dwlConditionMask);
    
        if ( !TSRemoteAdmin & TSAppServer )
        {
            s_fIsTerminalServer = TRUE;
        }
        else
        {
            // do not treat tsremoteadmin as TS machine from the application compatability point of view.
            s_fIsTerminalServer = FALSE;
        }
    }

    return s_fIsTerminalServer ? TRUE : FALSE;
}

#endif //WINNT
#endif //DOWNLEVEL_PLATFORM

// returns TRUE if pszFile is a local file and on a fixed drive
BOOL PathIsLocalAndFixed(LPCTSTR pszFile)
{
    if (!pszFile || !pszFile[0])
        return FALSE;

    if (PathIsUNC(pszFile))
        return FALSE;
    
    TCHAR szDrive[MAX_PATH];
    lstrcpy(szDrive, pszFile); 
    if (PathStripToRoot(szDrive) && GetDriveType(szDrive) != DRIVE_FIXED)
        return FALSE;

    return TRUE;
}


// This function will duplicate an APPCATEGORYINFOLIST and allocate the new copy
// using COM memory allocation functions
STDAPI  _DuplicateCategoryList(APPCATEGORYINFOLIST * pacl, APPCATEGORYINFOLIST * paclNew)
{
    HRESULT hres = E_FAIL;
    ASSERT(pacl && paclNew);
    ZeroMemory(paclNew, SIZEOF(APPCATEGORYINFOLIST));

    if (pacl && (pacl->cCategory > 0) && pacl->pCategoryInfo)
    {
        DWORD dwDesiredSize = pacl->cCategory * SIZEOF(APPCATEGORYINFO);
        APPCATEGORYINFO * paci = pacl->pCategoryInfo;
        paclNew->pCategoryInfo = (APPCATEGORYINFO *)SHAlloc(dwDesiredSize);
        if (paclNew->pCategoryInfo)
        {
            UINT iCategory = 0;
            paclNew->cCategory = 0;
            APPCATEGORYINFO * paciNew = paclNew->pCategoryInfo;
            while (paci && (iCategory < pacl->cCategory))
            {
                if (paci->pszDescription)
                {
                    hmemcpy(paciNew, paci, SIZEOF(APPCATEGORYINFO));
                    if (FAILED(SHStrDup(paci->pszDescription, &(paciNew->pszDescription))))
                    {
                        // We may be out of memory, stop here. 
                        ZeroMemory(paciNew, SIZEOF(APPCATEGORYINFO));
                        break;
                    }
                    
                    paciNew++;
                    paclNew->cCategory++;
                }
                
                iCategory++;
                paci++;
            }

            hres = S_OK;
        }
        else
            hres = E_OUTOFMEMORY;
    }
    return hres;
}

STDAPI _DestroyCategoryList(APPCATEGORYINFOLIST * pacl)
{
    if (pacl && pacl->pCategoryInfo)
    {
        UINT iCategory = 0;
        APPCATEGORYINFO * paci = pacl->pCategoryInfo;
        while (paci && (iCategory < pacl->cCategory))
        {
            if (paci->pszDescription)
            {
                SHFree(paci->pszDescription);
            }
            iCategory++;
            paci++;
        }
        SHFree(pacl->pCategoryInfo);
    }

    return S_OK;
}


