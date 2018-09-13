#include "shellprv.h"
#pragma  hdrstop

#include <limits.h>

extern TCHAR g_szFileTemplate[];     // used to build type name...
extern void _InitFileFolderClassNames (void);

STDAPI OpenWithListRegister(DWORD dwFlags, LPCTSTR pszExt, LPCTSTR pszVerb, HKEY hkProgid);

// DPA string compare function
static int CALLBACK DPAStringCompare(LPVOID sz1, LPVOID sz2, LPARAM lparam)
{
    return(lstrcmpi((LPTSTR)sz1, (LPTSTR)sz2));
}

// DPA string free function
static int CALLBACK DPAStringFree(LPVOID sz, LPVOID pData)
{
    LocalFree(sz);
    return 0;
}

//
// This is a real hack, but for now we generate an idlist that looks
// something like: C:\*.ext which is the extension for the IDList.
// We use the simple IDList as to not hit the disk...
//
void _GenerateAssociateNotify(LPCTSTR pszExt)
{
    TCHAR szFakePath[MAX_PATH];
    LPITEMIDLIST pidl;

    GetWindowsDirectory(szFakePath, ARRAYSIZE(szFakePath));

    lstrcpy(szFakePath + 3, c_szStar);      // "C:\*"
    lstrcat(szFakePath, pszExt);            // "C:\*.foo"
    pidl = SHSimpleIDListFromPath(szFakePath);
    if (pidl)
    {
        // Now call off to the notify function.
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, pidl, NULL);
        ILFree(pidl);
    }
}

// Given a class key returns the shell\open\command string in szValue
// and the number of chars copied in cbMaxValue. cbMaxValue should
// be initialised to the max siz eof szValue.

// We now expect and return the count in characters (DavePl)

void GetCmdLine(LPCTSTR szKey, LPTSTR szValue, LONG cchValue)
{
    TCHAR szTemp[MAX_PATH+40];   // Leave room for both extension plus junk on at end...
    VDATEINPUTBUF(szValue, TCHAR, cchValue);

    wsprintf(szTemp, TEXT("%s\\%s"), szKey, c_szShellOpenCmd);

    szValue[0] = 0;
    cchValue *= SIZEOF(TCHAR);
    SHRegQueryValue(HKEY_CLASSES_ROOT, szTemp, szValue, &cchValue);
}

// uFlags GCD_ flags from GetClassDescription uFlags

void FillListWithClasses(HWND hwnd, BOOL fComboBox, UINT uFlags)
{
    int i;
    TCHAR szClass[CCH_KEYMAX];
    TCHAR szDisplayName[CCH_KEYMAX];
    LONG lcb;

    SendMessage(hwnd, fComboBox ? CB_RESETCONTENT : LB_RESETCONTENT, 0, 0L);

    if (uFlags & GCD_MUSTHAVEEXTASSOC)
    {
        TCHAR szExt[CCH_KEYMAX];

        // The caller stated that they only want those classes that
        // have have at least one extension associated with it.
        //
        for (i = 0; RegEnumKey(HKEY_CLASSES_ROOT, i, szClass, ARRAYSIZE(szClass)) == ERROR_SUCCESS; i++)
        {
            // Is this an extension
            if (szClass[0] != TEXT('.'))
                continue;   // go process the next one...

            // Get the class name
            lstrcpy(szExt, szClass);
            lcb = SIZEOF(szClass);
            if ((SHRegQueryValue(HKEY_CLASSES_ROOT, szExt, szClass, &lcb) != ERROR_SUCCESS) || (lcb == 0))
                continue;   // Again we are not interested.

            // use uFlags passed in to filter
            if (GetClassDescription(HKEY_CLASSES_ROOT, szClass, szDisplayName, ARRAYSIZE(szDisplayName), uFlags))
            {
                INT_PTR iItem;

                /* If the display name is zero length then don't bother to show in the control. */
                if ( !lstrlen( szDisplayName ) )
                    continue;

                // Now make sure it is not already in the list...
                if ((int)SendMessage(hwnd, fComboBox ? CB_FINDSTRINGEXACT : LB_FINDSTRINGEXACT,
                                     (WPARAM)-1, (LPARAM)(LPTSTR)szDisplayName) >= 0)
                    continue;       // allready in the list.

                // sorted
                iItem = (INT_PTR) SendMessage(hwnd, fComboBox ? CB_ADDSTRING : LB_ADDSTRING,
                                         0, (LONG_PTR)(LPTSTR)szDisplayName);

                if (iItem >= 0)
                    SendMessage(hwnd, fComboBox ? CB_SETITEMDATA : LB_SETITEMDATA, iItem, (LONG_PTR)AddHashItem(NULL, szClass));

            }
        }


    }
    else
    {
        for (i = 0; RegEnumKey(HKEY_CLASSES_ROOT, i, szClass, ARRAYSIZE(szClass)) == ERROR_SUCCESS; i++)
        {
            // use uFlags passed in to filter
            if (GetClassDescription(HKEY_CLASSES_ROOT, szClass, szDisplayName, ARRAYSIZE(szDisplayName), uFlags))
            {
                // sorted
                INT_PTR iItem = (INT_PTR)SendMessage(hwnd, fComboBox ? CB_ADDSTRING : LB_ADDSTRING,
                                             0, (LONG_PTR)(LPTSTR)szDisplayName);

                if (iItem >= 0)
                    SendMessage(hwnd, fComboBox ? CB_SETITEMDATA : LB_SETITEMDATA, iItem, (LONG_PTR)AddHashItem(NULL, szClass));

            }
        }
    }
}

// get the displayable name for file types "classes"
//
//
// uFlags:
//     GCD_MUSTHAVEOPENCMD      only returns things with open verbs
//     GCD_ADDEXETODISPNAME     append the name of the ext that is in the open cmd
//                              (GCD_MUSTHAVEOPENCMD)
//     GCD_ALLOWPSUDEOCLASSES   return psudeo classes, those with stuff haning
//                              off the .ext key

BOOL GetClassDescription(HKEY hkClasses, LPCTSTR pszClass, LPTSTR szDisplayName, int cchDisplayName, UINT uFlags)
{
    TCHAR szExe[MAX_PATH];
    TCHAR szClass[CCH_KEYMAX];
    LPTSTR pszExeFile;
    LONG lcb;

    // Skip things that aren't classes (extensions).

    if (pszClass[0] == TEXT('.'))
    {
        if (uFlags & GCD_ALLOWPSUDEOCLASSES)
        {
            lcb = SIZEOF(szClass);
            if ((SHRegQueryValue(hkClasses, pszClass, szClass, &lcb) != ERROR_SUCCESS) || (lcb == 0))
            {
                // look for .ext\shell\open\command directly (hard wired association)
                // if this extenstion does not name a real class

                GetCmdLine(pszClass, szExe, ARRAYSIZE(szExe));
                if (szExe[0]) {
                    lstrcpyn(szDisplayName, PathFindFileName(szExe), cchDisplayName);
                    return TRUE;
                }

                return FALSE;
            }
            pszClass = szClass;
        }
        else
        {
            return FALSE;       // don't return psudeo class
        }
    }

    // REVIEW: we should really special case the OLE junk here.  if pszClass is
    // CLSID, Interface, TypeLib, etc we should skip it

    // REVIEW: we really need to verify that some extension points at this type to verfy
    // that it is valid.  perhaps the existance of a "shell" key is enough.

    // get the classes displayable name
    lcb = cchDisplayName * SIZEOF(TCHAR);
    if (SHRegQueryValue(hkClasses, pszClass, szDisplayName, &lcb) != ERROR_SUCCESS || (lcb < 2))
        return FALSE;

    if (uFlags & GCD_MUSTHAVEOPENCMD)
    {
        // verify that it has an open command
        GetCmdLine(pszClass, szExe, ARRAYSIZE(szExe));
        if (!szExe[0])
            return FALSE;

        // BUGBUG: currently this is dead functionallity
        if (uFlags & GCD_ADDEXETODISPNAME)
        {
            PathRemoveArgs(szExe);

            // eliminate per instance type things (programs, pif, etc)
            // Skip things that aren't relevant to the shell.
            if (szExe[0] == TEXT('%'))
                return FALSE;

            // skip things with per-instance type associations
            pszExeFile = PathFindFileName(szExe);

            if ((int)((lstrlen(szDisplayName) + lstrlen(pszExeFile) + 2)) < cchDisplayName)
            {
                wsprintf(szDisplayName + lstrlen(szDisplayName), TEXT(" (%s)"), pszExeFile);
            }
        }
    }
    return TRUE;
}

void DeleteListAttoms(HWND hwnd, BOOL fComboBox)
{
    int cItems;
    PHASHITEM phiClass;
    int iGetDataMsg;

    iGetDataMsg = fComboBox ? CB_GETITEMDATA : LB_GETITEMDATA;

    cItems = (int)SendMessage(hwnd, fComboBox ? CB_GETCOUNT : LB_GETCOUNT, 0, 0) - 1;

    /* clean out them atoms except for "(none)".
     */
    for (; cItems >= 0; cItems--)
    {
        phiClass = (PHASHITEM)SendMessage(hwnd, iGetDataMsg, cItems, 0L);
        if (phiClass != (PHASHITEM)LB_ERR && phiClass)
            DeleteHashItem(NULL, phiClass);
    }
}

// BEGIN new stuff

typedef struct {    // oad
    // params
    HWND hwnd;                          // parent window
    POPENASINFO poainfo;
    // local data
    int idDlg;                          // open as dialog type: DLG_OPENAS_NOTYPE or DLG_OPENAS
    HWND hDlg;                          // open as dialog window handle
    HWND hwndList;                      // app list
    LPTSTR lpszExt;
    LPTSTR lpszNoOpenMsg;
    TCHAR szDescription[CCH_KEYMAX];    // file type description
    HRESULT hr;
} OPENAS_DATA, *POPENAS_DATA;


#define AIF_TEMPKEY     0x1     // temp class key created for the selected exe
#define AIF_SHELLNEW    0x2     // class key with shellnew subkey
#define AIF_KEYISAPPKEY 0x4     // the szKey should be passed to AssocApp

#define MAXKEYNAME    128
typedef struct {
    TCHAR szApp[MAX_PATH];    
    TCHAR szFriendly[MAXKEYNAME];       // Friendly name
    TCHAR szKey[MAXKEYNAME];            // value to pass to Assoc*() APIs
    FILETIME ft;
    DWORD flags;
} APPINFO, *PAPPINFO;



#define IsExtension(s)   (*(s) == TEXT('.'))

int _AppInfoGetIconIndex(APPINFO *pai)
{
    TCHAR sz[MAX_PATH];
    int iRet = -1;
    ASSOCF flags = ASSOCF_VERIFY | ASSOCF_NOUSERSETTINGS;
    if (pai->flags & AIF_KEYISAPPKEY)
        flags |= ASSOCF_OPEN_BYEXENAME;
    
    if (SUCCEEDED(AssocQueryString(flags, ASSOCSTR_EXECUTABLE, 
        pai->szKey, NULL, sz, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(sz)))))
    {
        iRet = Shell_GetCachedImageIndex(sz, 0, 0);
        if (-1 == iRet)
        {
            iRet = Shell_GetCachedImageIndex(c_szShell32Dll, II_APPLICATION, 0);
        }
    }

    return iRet;
}

LPTSTR _AppInfoGetFriendly(APPINFO *pai)
{
    if (!*pai->szFriendly)
    {
        ASSOCF flags = ASSOCF_VERIFY | ASSOCF_NOUSERSETTINGS;    
        if (pai->flags & AIF_KEYISAPPKEY)
            flags |= ASSOCF_OPEN_BYEXENAME;

        if (FAILED(AssocQueryString(flags, ASSOCSTR_FRIENDLYAPPNAME, 
            pai->szKey, NULL, pai->szFriendly, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(pai->szFriendly)))))
        {
            StrCpyN(pai->szFriendly, pai->szApp, SIZECHARS(pai->szFriendly));
        }
    }

    return pai->szFriendly;
}

APPINFO *_CreateAppInfo(LPCTSTR pszApp, LPCTSTR pszFriendly, LPCTSTR pszKey, DWORD flags)
{
    APPINFO *pai = (APPINFO *) LocalAlloc(LPTR, SIZEOF(APPINFO));

    if (pai)
    {
        if (pszApp)
        {
            HANDLE hFile = CreateFile(pszApp, GENERIC_READ, FILE_SHARE_READ,
                    NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, 0);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                GetFileTime(hFile, NULL, NULL, &pai->ft);

                CloseHandle(hFile);
            }
            
            //  we only care about the most basic name here
            StrCpyN(pai->szApp, PathFindFileName(pszApp), SIZECHARS(pai->szApp));
            PathRemoveExtension(pai->szApp);


        }

        if (pszKey)
            StrCpyN(pai->szKey, pszKey, SIZECHARS(pai->szKey));

        pai->flags = flags;

        if (pszFriendly)
            StrCpyN(pai->szFriendly, pszFriendly, SIZECHARS(pai->szFriendly));
        else if (*pai->szKey)
        {
            //  BUGBUGREVIEW after BETA2 - ZekeL - 25-JUN-98
            //  this hits the disk, and we would like some
            //  way to enum this list without hitting the disk
            //  so that this dialog is shown quickly
            _AppInfoGetFriendly(pai);
            if (pai->szFriendly[0] == 0)
            {
                LocalFree(pai);
                pai = NULL;
            }
        }
    }

    return pai;
}

int _AddAppInfoItem(APPINFO *pai, HWND hwnd)
{
    LV_ITEM item;
    
    item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
    item.iItem = INT_MAX;
    item.iSubItem = 0;
    item.state = 0;
    item.iImage = I_IMAGECALLBACK;

    //  use it if we got it!
    if (*(pai->szFriendly))
        item.pszText = pai->szFriendly;
    else
        item.pszText = LPSTR_TEXTCALLBACK;

    item.lParam = (LPARAM)pai;
    return ListView_InsertItem(hwnd, &item);
}

void _AddFromAppPaths(HWND hwndList)
{
    HKEY hkeyAppPaths;
    if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT, TEXT("Applications"), &hkeyAppPaths))
    {
        //  give enough space to 
        TCHAR szApp[MAX_PATH];   
        int i;
        
        for (i = 0; RegEnumKey(hkeyAppPaths, i, szApp, ARRAYSIZE(szApp)) == ERROR_SUCCESS; i++)
        {
            if (!IsPathInOpenWithKillList(szApp) &&
                SUCCEEDED(AssocQueryString(ASSOCF_INIT_BYEXENAME | ASSOCF_VERIFY, ASSOCSTR_EXECUTABLE, 
                szApp, NULL, szApp, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(szApp)))))
            {
                //  we got a winner!
                APPINFO *pai = _CreateAppInfo(szApp, NULL, szApp, AIF_KEYISAPPKEY);

                if (pai)
                {
                    _AddAppInfoItem(pai, hwndList);
                }
            }
        }

        RegCloseKey(hkeyAppPaths);
    }
}

void _AddFromHKCR(HWND hwndList)
{
    int i;
    TCHAR szClass[MAX_PATH];   
#ifdef WINNT
    BOOL fInExtensions = FALSE;
#endif

    for (i = 0; RegEnumKey(HKEY_CLASSES_ROOT, i, szClass, ARRAYSIZE(szClass)) == ERROR_SUCCESS; i++)
    {
        TCHAR szApp[MAX_PATH];

#ifdef WINNT
        //  UNDOCUMENTED feature.  the enum is sorted,
        //  so we can just restrict ourselves to extensions 
        //  for perf and fun!
        if (fInExtensions && !IsExtension(szClass))
            break;

        if (!fInExtensions && IsExtension(szClass))
            fInExtensions = TRUE;
#endif //            
        if (SUCCEEDED(AssocQueryString(ASSOCF_VERIFY | ASSOCF_NOUSERSETTINGS, ASSOCSTR_EXECUTABLE, 
            szClass, NULL, szApp, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(szApp)))))
        {
            if (!IsPathInOpenWithKillList(szApp))
            {
                DWORD flags = 0;
                APPINFO *pai;
                DWORD cch;
                
                if (IsExtension(szClass) 
                && SUCCEEDED(AssocQueryString(ASSOCF_NOUSERSETTINGS, ASSOCSTR_SHELLNEWVALUE, szClass, NULL, NULL, &cch)))
                    flags |= AIF_SHELLNEW;
                    
                pai = _CreateAppInfo(szApp, NULL, szClass, flags);

                if (pai)
                {
                    _AddAppInfoItem(pai, hwndList);
                }
            }
        }
    }
}
                    

int _CompareApps(APPINFO *p1, APPINFO *p2, LPARAM p)
{
    return StrCmpI(p1->szApp, p2->szApp);
}

int _CompareAppPriorities(APPINFO *p1, APPINFO *p2, LPARAM p)
{
    //  might be nice to have priority in the 
    //  app keys that apps can set...

    //  NOTE - we want the newer file (larger ft)
    //  to have higher pri, so it needs to be
    //  a negative value so it shows up correctly
    //  under sorting
    int i = CompareFileTime(&p2->ft, &p1->ft);

    if (i == 0)
    {
        if (p1->flags & AIF_KEYISAPPKEY)
            return -1;

        if (p2->flags & AIF_KEYISAPPKEY)
            return 1;

        if (p1->flags & AIF_SHELLNEW)
            return -1;
        
        if (p2->flags & AIF_SHELLNEW)
            return 1;

        if (IsExtension(p1->szKey))
            return -1;

        if (IsExtension(p2->szKey))
            return 1;
    }
    return i;
}

#define _GetAppInfoFromLV(h, i)  (APPINFO *)LVUtil_GetLParam((h), (i))

void _PruneDuplicates(HWND hwndList)
{
    int i, iMax = ListView_GetItemCount(hwndList);

    if (iMax > 0)
    {
        APPINFO *paiLast = _GetAppInfoFromLV(hwndList, 0);
    
        for (i = 1; i < iMax; i++)
        {
            APPINFO *pai = _GetAppInfoFromLV(hwndList, i);

            ASSERT(pai);
            ASSERT(paiLast);

            if (0 == _CompareApps(paiLast, pai, 0))
            {
                // we have the same app, then it
                // is just a matter of who is better at 
                //  registering themselves :b

                int iDelete;

                //  higher pri entries will return -1 
                if (0 > _CompareAppPriorities(paiLast, pai, 0))
                    //  delete the current pai
                    iDelete = i;
                else
                {
                    //  delete paiLast, so make pai into paiLast
                    iDelete = i - 1;
                    paiLast = pai;
                }

                //
                //  we need to delete this item as a duplicate
                //  after deletion the list is smaller!
                //  this means that we need back up the 
                //  current index as well.
                //
                ListView_DeleteItem(hwndList, iDelete);
                i--; iMax--;

            }
            else
            {
                paiLast = pai;
            }
        }
    }
}
#define RegCountSubKeys(hk, pc)  (RegQueryInfoKey((hk), NULL, NULL, NULL, (pc), NULL, NULL, NULL, NULL, NULL, NULL, NULL))

#define SZCACHEVERSION        TEXT("NT5OpenAsList")

BOOL _VerifyCacheVersion(IStream *pstm)
{
    BOOL fRet = FALSE;
    TCHAR sz[SIZEOF(SZCACHEVERSION)];
    DWORD cNow, cThen;
    HKEY hk;
    
    if (!SendMessage(GetShellWindow(), DTM_QUERYHKCRCHANGED, QHKCRID_OPENAS, 0L)
    && SUCCEEDED(Stream_ReadString(pstm, sz, SIZECHARS(sz), TRUE))
    && 0 == StrCmp(sz, SZCACHEVERSION)
    && ERROR_SUCCESS == RegCountSubKeys(HKEY_CLASSES_ROOT, &cNow)
    && SUCCEEDED(IStream_Read(pstm, &cThen, SIZEOF(cThen)))
    && cNow == cThen
    && ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT, TEXT("Applications"), &hk))
    {
        if (ERROR_SUCCESS == RegCountSubKeys(hk, &cNow)
        && SUCCEEDED(IStream_Read(pstm, &cThen, SIZEOF(cThen)))
        && cNow == cThen)
        {
            fRet = TRUE;
        }

        RegCloseKey(hk);
    }
    return fRet;
}

#define _OpenCachedOpenAsList(grf)   SHOpenRegStream(HKEY_CURRENT_USER, STRREG_DISCARDABLE STRREG_POSTSETUP, TEXT("OpenAsList"), grf)
#define _DeleteCachedOpenAsList()    SHSetValue(HKEY_CURRENT_USER, STRREG_DISCARDABLE STRREG_POSTSETUP, TEXT("OpenAsList"), REG_SZ, (LPVOID)"", 0);


APPINFO *_GetNextCachedAppInfo(IStream *pstm)
{
    APPINFO *pai = (APPINFO *)LocalAlloc(LPTR, SIZEOF(APPINFO));

    if (pai)
    {
        if (SUCCEEDED(IStream_Read(pstm, &pai->flags, SIZEOF(pai->flags)))
            && SUCCEEDED(Stream_ReadString(pstm, pai->szApp, SIZECHARS(pai->szApp), TRUE))
            && SUCCEEDED(Stream_ReadString(pstm, pai->szFriendly, SIZECHARS(pai->szFriendly), TRUE))
            && SUCCEEDED(Stream_ReadString(pstm, pai->szKey, SIZECHARS(pai->szKey), TRUE)))
            return pai;
        else
            LocalFree(pai);
    }
    return NULL;
}

BOOL _WriteCachedAppInfo(IStream *pstm, APPINFO *pai)
{
    return (SUCCEEDED(IStream_Write(pstm, &pai->flags, SIZEOF(pai->flags)))
            && SUCCEEDED(Stream_WriteString(pstm, pai->szApp, TRUE))
            && SUCCEEDED(Stream_WriteString(pstm, pai->szFriendly, TRUE))
            && SUCCEEDED(Stream_WriteString(pstm, pai->szKey, TRUE)));
}

BOOL _AddFromCachedOpenAsList(HWND hwndList)
{
    BOOL fRet = FALSE;
    IStream *pstm = _OpenCachedOpenAsList(STGM_READ);

    if (pstm)
    {
        if (_VerifyCacheVersion(pstm))
        {
            int cItems = 0;

            if (SUCCEEDED(IStream_Read(pstm, &cItems, SIZEOF(cItems)))
                && cItems > 0)
            {
                for ( ;cItems; cItems--)
                {
                    APPINFO *pai = _GetNextCachedAppInfo(pstm);

                    if (pai)
                    {
                        //  ignore temp entries
                        if (pai->flags & AIF_TEMPKEY)
                            LocalFree(pai);
                        else
                            _AddAppInfoItem(pai, hwndList);
                    }
                    else
                        break;
                }

                //  if we made it through the entire list that we cached
                //  then we will count our selves lucky and not try anything else
                if (!cItems)
                    fRet = TRUE;
            }
        }

        pstm->lpVtbl->Release(pstm);

    }

    //  if anything went wrong, just kill the cache
    if (!fRet)
        _DeleteCachedOpenAsList();

    return fRet;
}


void _FillListWithApps(HWND hwndList)
{
    if (!_AddFromCachedOpenAsList(hwndList))
    {
        _AddFromAppPaths(hwndList);
        _AddFromHKCR(hwndList);

        //  use a custom sort to delay the friendly names being used
        ListView_SortItems(hwndList, _CompareApps, 0);

        _PruneDuplicates(hwndList);

    }

    //  BUGBUGREVIEW after BETA2 - ZekeL - 25-JUN98
    //  sort using the friendly name.  of course
    //  this required hitting the disk to get the friendly name
    //  but oh well.
    //  ListView_SortItems(hwndList, _CompareAppPriorities, 0);
    ListView_SortItems(hwndList, NULL, 0);

    // Lets set the focus to first item, but not the focus as some users
    // have made the mistake and type in the name and hit return and it
    // runs the first guy in the list which in the current cases tries to
    // run the backup app...
    ListView_SetItemState(hwndList, 0, LVNI_FOCUSED, LVNI_FOCUSED | LVNI_SELECTED);
    SetFocus(hwndList);
}


void _InitOpenAsDlg(POPENAS_DATA poad)
{
    TCHAR szFormat[200];
    TCHAR szFileName[MAX_PATH];
    TCHAR szTemp[MAX_PATH + ARRAYSIZE(szFormat)];
    BOOL fDisableAssociate;
    HIMAGELIST himlLarge, himlSmall;
    LV_COLUMN col = {LVCF_FMT | LVCF_WIDTH, LVCFMT_LEFT};
    BOOL fBlanketDissable;
    RECT rc;
    fBlanketDissable=FALSE;

#ifdef WINNT
    fBlanketDissable = SHRestricted(REST_NOFILEASSOCIATE);
#endif

    // Don't let the file name go beyond the width of one line...
    GetDlgItemText(poad->hDlg, IDD_TEXT, szFormat, ARRAYSIZE(szFormat));
    lstrcpy(szFileName, PathFindFileName(poad->poainfo->pcszFile));
    GetClientRect(GetDlgItem(poad->hDlg, IDD_TEXT), &rc);

    PathCompactPath(NULL, szFileName, rc.right - 4 * GetSystemMetrics(SM_CXBORDER));

    wsprintf(szTemp, szFormat, szFileName);
    SetDlgItemText(poad->hDlg, IDD_TEXT, szTemp);

    // AraBern 07/20/99, specific to TS on NT, but can be used on NT without TS
    if ( fBlanketDissable )
    {
        CheckDlgButton(poad->hDlg, IDD_MAKEASSOC, FALSE);
        EnableWindow(GetDlgItem(poad->hDlg, IDD_MAKEASSOC), FALSE);
    }
    else
    {
        // Don't allow associations to be made for things we consider exes...
        fDisableAssociate = (! (poad->poainfo->dwInFlags & OAIF_ALLOW_REGISTRATION) ||
                        PathIsExe(poad->poainfo->pcszFile));
                        
        // check IDD_MAKEASSOC only for unknown file type and those with OAIF_FORCE_REGISTRATION flag set
        if ((poad->poainfo->dwInFlags & OAIF_FORCE_REGISTRATION) ||
            (poad->idDlg == DLG_OPENAS_NOTYPE && !fDisableAssociate))
            CheckDlgButton(poad->hDlg, IDD_MAKEASSOC, TRUE);

        if (fDisableAssociate)
            EnableWindow(GetDlgItem(poad->hDlg, IDD_MAKEASSOC), FALSE);
    }

    poad->hwndList = GetDlgItem(poad->hDlg, IDD_APPLIST);
    Shell_GetImageLists(&himlLarge, &himlSmall);
    ListView_SetImageList(poad->hwndList, himlLarge, LVSIL_NORMAL);
    ListView_SetImageList(poad->hwndList, himlSmall, LVSIL_SMALL);
    SetWindowLong(poad->hwndList, GWL_EXSTYLE,
            GetWindowLong(poad->hwndList, GWL_EXSTYLE) | WS_EX_CLIENTEDGE);

    GetClientRect(poad->hwndList, &rc);
    col.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL)
            - 4 * GetSystemMetrics(SM_CXEDGE);
    ListView_InsertColumn(poad->hwndList, 0, &col);
    
    _FillListWithApps(poad->hwndList);

    // initialize the OK button
    EnableWindow(GetDlgItem(poad->hDlg, IDOK),
            (ListView_GetNextItem(poad->hwndList, -1, LVNI_SELECTED) != -1));
            
    _InitFileFolderClassNames();
}


BOOL RunAs(POPENAS_DATA poad)
{
    BOOL fRet = FALSE;
    APPINFO *pai;
    int iItemFocus;
    SHELLEXECUTEINFO ExecInfo = { 0 };
    
    iItemFocus = ListView_GetNextItem(poad->hwndList, -1, LVNI_SELECTED);
    pai = (APPINFO *)LVUtil_GetLParam(poad->hwndList, iItemFocus);

    FillExecInfo(ExecInfo, poad->hwnd, NULL, poad->poainfo->pcszFile, NULL, NULL, SW_NORMAL);
    AssocQueryKey(pai->flags & AIF_KEYISAPPKEY ? ASSOCF_OPEN_BYEXENAME : 0,
        ASSOCKEY_SHELLEXECCLASS, pai->szKey, NULL, &ExecInfo.hkeyClass );


    if (ExecInfo.hkeyClass)
    {
        ExecInfo.fMask |= SEE_MASK_CLASSKEY;

        fRet = ShellExecuteEx(&ExecInfo);

        RegCloseKey(ExecInfo.hkeyClass);
    }

    //  else this should only happen if something really wierd happened
        
    return fRet;
}

// Create a new class key, and set its shell\open\command
BOOL CreateTempClassForExe(PAPPINFO pai, LPCTSTR pszPath)
{
    //  if it is not an LFN app, pass unquoted args.
    //  otherwise, AssocSet() will default correctly.
    LPCWSTR pszArgs = (App_IsLFNAware(pszPath) ? NULL : L"%1");
    WCHAR wszProgid[MAX_PATH], wszPath[MAX_PATH];
    
    //  we should have no class key here!
    ASSERT(!pai->szKey[0]);

    wnsprintf(pai->szKey, SIZECHARS(pai->szKey), TEXT("%s_tempkey"), pai->szApp);

    SHTCharToUnicode(pszPath, wszPath, SIZECHARS(wszPath));
    SHTCharToUnicode(pai->szKey, wszProgid, SIZECHARS(wszProgid));
    {
        ASSOCVERB av = {L"open", NULL, NULL, NULL, pszArgs, NULL};
        ASSOCSHELL as = {&av, 1, 0};
        ASSOCPROGID apid = {SIZEOF(apid), wszProgid, NULL, NULL, &as, NULL};

        if (SUCCEEDED(AssocMakeProgid(ASSOCMAKEF_VOLATILE, wszPath, &apid, NULL)))
        {
            //  we created a temp key,
            pai->flags |= AIF_TEMPKEY;
            return TRUE;
        }
    }   

    // FAILURE
    *(pai->szApp) = TEXT('\0');
    return FALSE;
}

int _AppInfoFindInLV(HWND hwndList, APPINFO *paiIn)
{
    int i, iMax = ListView_GetItemCount(hwndList);

    
    for (i = 0; i < iMax; i++)
    {
        APPINFO *pai = _GetAppInfoFromLV(hwndList, i);

        ASSERT(pai);

        if (0 == _CompareApps(paiIn, pai, 0))
        {
            return i;
        }
    }

    return -1;
}

void OpenAsOther(POPENAS_DATA poad)
{
    TCHAR szApp[MAX_PATH];

    *szApp = '\0';

    // do a file open browse
    if (GetFileNameFromBrowse(poad->hDlg, szApp, ARRAYSIZE(szApp), NULL,
            MAKEINTRESOURCE(IDS_EXE), MAKEINTRESOURCE(IDS_PROGRAMSFILTER), MAKEINTRESOURCE(IDS_OPENAS)))
    {
        APPINFO *pai = _CreateAppInfo(szApp, NULL, NULL, 0);

        if (pai)
        {
            int iItem = _AppInfoFindInLV(poad->hwndList, pai);

            if (-1 == iItem)
            {
                if (CreateTempClassForExe(pai, szApp))
                {
                    iItem = _AddAppInfoItem(pai, poad->hwndList);
                    
                    if (-1 == iItem)
                        LocalFree(pai);
                }
            }
            else
                LocalFree(pai);

            // Select it
            ListView_SetItemState(poad->hwndList, iItem, LVNI_SELECTED | LVNI_FOCUSED, LVNI_SELECTED | LVNI_FOCUSED);
            ListView_EnsureVisible(poad->hwndList, iItem, FALSE);
            SetFocus(poad->hwndList);
        }
    }
}

BOOL _IsNewAssociation(LPCTSTR pszExt, LPCTSTR pszApp, BOOL fForceLM)
{
    BOOL fRet = TRUE;
    TCHAR sz[MAX_PATH];
    ASSOCF flags = fForceLM ? ASSOCF_VERIFY | ASSOCF_NOUSERSETTINGS : ASSOCF_VERIFY;
    
    if (SUCCEEDED(AssocQueryString(flags, ASSOCSTR_EXECUTABLE, pszExt, NULL, sz, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(sz))))
    && (0 == lstrcmpi(pszApp, sz)))
    {
        //
        //  these have the same executable, trust 
        //  that when the exe installed itself, it did
        //  it correctly, and we dont need to overwrite 
        //  their associations with themselves :)
        //
        fRet = FALSE;
    }

    return fRet;
}

// return true if ok to continue
BOOL OpenAsMakeAssociation(LPCTSTR pszExt, LPCTSTR pszApp, LPCWSTR pszDesc, HKEY hkey)
{
    BOOL fRet = FALSE;
    HRESULT hr = AssocMakeApplicationByKey(ASSOCMAKEF_VERIFY, hkey, NULL);

    //  if the user is choosing the existing association
    //  or if we werent able to setup an Application ,
    //  then we want to leave it alone,     
    if (SUCCEEDED(hr) && (_IsNewAssociation(pszExt, pszApp, FALSE)))
    {
        if (!_IsNewAssociation(pszExt, pszApp, TRUE))
        {
            //  if it is reverting to the machine default
            //  then we want to eliminate the user association
            hr = AssocMakeFileExtsToApplication(0, pszExt, NULL);
        }
        else if (*pszDesc)
        {
            //  this is a brand new association.
            WCHAR wszExt[MAX_PATH];
            WCHAR wszProgid[MAX_PATH];

            ASSERT(lstrlen(pszExt) > 1); // because we always skip the "." below
            SHTCharToUnicode(pszExt, wszExt, SIZECHARS(wszExt));

            wnsprintfW(wszProgid, SIZECHARS(wszProgid), L"%ls_auto_file", wszExt+1);

            //  double NULL terminate
            wszExt[lstrlenW(wszExt) + 1] = '\0';
            
            {
                HKEY hkDst;
                ASSOCPROGID apid = {SIZEOF(apid), wszProgid, pszDesc, NULL, NULL, wszExt};
                WCHAR wszApp[MAX_PATH];
                SHTCharToUnicode(pszApp, wszApp, ARRAYSIZE(wszApp));

                if (SUCCEEDED(AssocMakeProgid(0, wszApp, &apid, &hkDst)))
                {
                    hr = AssocCopyVerbs(hkey, hkDst);
                    RegCloseKey(hkDst);
                }
            }   

        }
        else
        {
            hr = AssocMakeFileExtsToApplication(0, pszExt, PathFindFileName(pszApp));
        }

        //  if the application already
        //  existed, then it will
        //  return S_FALSE;
        fRet = (S_OK == hr);
    }
    
    _GenerateAssociateNotify(pszExt);

    return fRet;
}


VOID _InitNoOpenDlg(POPENAS_DATA poad)
{
    SHFILEINFO sfi;
    HICON hIcon;
    TCHAR szFormat[MAX_PATH], szTemp[MAX_PATH];

    GetDlgItemText(poad->hDlg, IDD_TEXT1, szFormat, ARRAYSIZE(szFormat));
    wnsprintf(szTemp, SIZECHARS(szTemp), szFormat, poad->szDescription, poad->lpszExt);
    SetDlgItemText(poad->hDlg, IDD_TEXT1, szTemp);

    if (poad->lpszNoOpenMsg && *poad->lpszNoOpenMsg)
        SetDlgItemText(poad->hDlg, IDD_TEXT2, poad->lpszNoOpenMsg);

    if (SHGetFileInfo(poad->poainfo->pcszFile, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON)
        && NULL != sfi.hIcon)
    {
        hIcon = sfi.hIcon;
    }
    else
    {
        HIMAGELIST himl;
        Shell_GetImageLists(&himl, NULL);
        hIcon = ImageList_ExtractIcon(g_hinst, himl, II_DOCNOASSOC);
    }
    hIcon = (HICON)SendDlgItemMessage(poad->hDlg, IDD_ICON, STM_SETICON, (WPARAM)hIcon, 0);
    if ( hIcon )
    {
        DestroyIcon(hIcon);
    }
}

BOOL_PTR CALLBACK NoOpenDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    POPENAS_DATA poad = (POPENAS_DATA)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (wMsg) {

    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        poad = (POPENAS_DATA)lParam;
        poad->hDlg = hDlg;
        _InitNoOpenDlg(poad);
        break;

    case WM_COMMAND:
        ASSERT(poad);
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDD_OPENWITH:
            //  this will cause the open with dialog
            //  to follow this dialog
            poad->hr = E_FAIL;
            EndDialog(hDlg, TRUE);
            break;

        case IDCANCEL:
            poad->hr = NOERROR;
            EndDialog(hDlg, TRUE);
            break;
        }
        break;

    default:
        return FALSE;
    }
    
    return TRUE;
}


const static DWORD aOpenAsHelpIDs[] = {  // Context Help IDs
    IDD_ICON,             IDH_FCAB_OPENAS_APPLIST,
    IDD_TEXT,             IDH_FCAB_OPENAS_APPLIST,
    IDD_DESCRIPTIONTEXT,  IDH_FCAB_OPENAS_DESCRIPTION,
    IDD_DESCRIPTION,      IDH_FCAB_OPENAS_DESCRIPTION,
    IDD_APPLIST,          IDH_FCAB_OPENAS_APPLIST,
    IDD_MAKEASSOC,        IDH_FCAB_OPENAS_MAKEASSOC,
    IDD_OTHER,            IDH_FCAB_OPENAS_OTHER,

    0, 0
};

void OpenAs_OnGetDispInfo(LV_DISPINFO *pdi, HWND hwnd)
{
    BOOL fKillMe = FALSE;
    
    if (pdi->item.mask & LVIF_IMAGE)
    {
        pdi->item.iImage = _AppInfoGetIconIndex((APPINFO *)pdi->item.lParam);

        //  if there is problem, 
        if (-1 == pdi->item.iImage)
            fKillMe = TRUE;
    }

    if (!fKillMe && (pdi->item.mask & LVIF_TEXT))
    {
        pdi->item.pszText = _AppInfoGetFriendly((APPINFO *)pdi->item.lParam);

        //
        if (!pdi->item.pszText)
            fKillMe = TRUE;
    }

    //  we dont want any uncooperative guys!
//    if (fKillMe)
//        ListView_DeleteItem(hwnd, pdi->item.iItem);
//    else
        pdi->item.mask |= LVIF_DI_SETITEM;
}

BOOL _WriteCacheHeader(IStream *pstm, int iMax)
{
    BOOL fRet = FALSE;
    HKEY hk;
    DWORD cNow;
    
    if (SUCCEEDED(Stream_WriteString(pstm, SZCACHEVERSION, TRUE))
    && ERROR_SUCCESS == RegCountSubKeys(HKEY_CLASSES_ROOT, &cNow)
    && SUCCEEDED(IStream_Write(pstm, &cNow, SIZEOF(cNow)))
    && ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT, TEXT("Applications"), &hk))
    {
        if (ERROR_SUCCESS == RegCountSubKeys(hk, &cNow)
        && SUCCEEDED(IStream_Write(pstm, &cNow, SIZEOF(cNow)))
        && SUCCEEDED(IStream_Write(pstm, &iMax, SIZEOF(int))))
        {
            fRet = TRUE;
        }

        RegCloseKey(hk);
    }

    return fRet;
}

void _SaveCachedOpenAsList(HWND hwndList, BOOL fDelete)
{
    if (!fDelete)
    {
        IStream *pstm = _OpenCachedOpenAsList(STGM_READWRITE);

        if (pstm)
        {
            //  if verify returns true, then we dont need to 
            //  update the cache
            if (!_VerifyCacheVersion(pstm))
            {
                ULARGE_INTEGER li;
                int i, iMax = ListView_GetItemCount(hwndList);

                //  back it up so we write at the beginning
                IStream_Reset(pstm);
                //
                //  this is not an accurate assessment of the size, 
                //  but for perf reasons it is better to guess something
                //  closer to what we expect.  
                //  99% of all items are less than 128 bytes
                //
                li.QuadPart = (ULONGLONG)((UINT)iMax * 128);
                
                pstm->lpVtbl->SetSize(pstm, li);

                //  start with the version and total count...
                if (_WriteCacheHeader(pstm, iMax))
                {
                    for (i = 0; i < iMax; i++)
                    {
                        APPINFO *pai = _GetAppInfoFromLV(hwndList, i);

                        ASSERT(pai);

                        if (!_WriteCachedAppInfo(pstm, pai))
                        {
                            fDelete = TRUE;
                            break;
                        }
                    }
                }
                else
                    fDelete = TRUE;
            }

            pstm->lpVtbl->Release(pstm);
        }
    }

    if (fDelete)
        _DeleteCachedOpenAsList();
}

BOOL_PTR CALLBACK OpenAsDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    POPENAS_DATA poad = (POPENAS_DATA)GetWindowLongPtr(hDlg, DWLP_USER);
    APPINFO *pai;
    int iItemFocus;
    HKEY hkeyProgID;
    BOOL fMadeNewAssoc = FALSE;

    switch (wMsg) {

    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        poad = (POPENAS_DATA)lParam;
        poad->hDlg = hDlg;
        _InitOpenAsDlg(poad);
        break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
            HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aOpenAsHelpIDs);
        break;

    case WM_CONTEXTMENU:
        if ((int)SendMessage(hDlg, WM_NCHITTEST, 0, lParam) != HTCLIENT)
            return FALSE;   // don't process it
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
            (ULONG_PTR)(LPVOID)aOpenAsHelpIDs);
        break;

   case WM_NOTIFY:

        switch (((LPNMHDR)lParam)->code)
        {
        case LVN_GETDISPINFO:
            OpenAs_OnGetDispInfo((LV_DISPINFO *)lParam, poad->hwndList);
            break;

        case LVN_DELETEITEM:
            pai = (PAPPINFO)((NM_LISTVIEW *)lParam)->lParam;
            if (pai->flags & AIF_TEMPKEY)
                SHDeleteKey(HKEY_CLASSES_ROOT, pai->szKey);
            LocalFree(pai);
            break;

        case LVN_ITEMCHANGED:
            EnableWindow(GetDlgItem(hDlg, IDOK),
                (ListView_GetNextItem(poad->hwndList, -1, LVNI_SELECTED) != -1));
            break;

        case NM_DBLCLK:
            if (IsWindowEnabled(GetDlgItem(hDlg, IDOK)))
                PostMessage(hDlg, WM_COMMAND, GET_WM_COMMAND_MPS(IDOK, hDlg, 0));
            break;
        }
        break;

    case WM_COMMAND:
        ASSERT(poad);
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDD_OTHER:
            OpenAsOther(poad);
            break;

        case IDOK:
            iItemFocus = ListView_GetNextItem(poad->hwndList, -1, LVNI_SELECTED);
            pai = (APPINFO *)LVUtil_GetLParam(poad->hwndList, iItemFocus);
            
            /* add shell\open of the selected progid to openwithlist. */
            AssocQueryKey(pai->flags & AIF_KEYISAPPKEY ? ASSOCF_NOUSERSETTINGS | ASSOCF_OPEN_BYEXENAME : ASSOCF_NOUSERSETTINGS,
                ASSOCKEY_SHELLEXECCLASS, pai->szKey, NULL, &hkeyProgID);

            if (hkeyProgID) 
            {
                WCHAR szDesc[MAX_PATH];

                AssocQueryStringByKey(ASSOCF_VERIFY, ASSOCSTR_EXECUTABLE, hkeyProgID, NULL,
                    poad->poainfo->szApp, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(poad->poainfo->szApp)));
                    
                // See if we should make an association or not...
                if (!GetDlgItemTextW(poad->hDlg, IDD_DESCRIPTION, szDesc, ARRAYSIZE(szDesc)))
                    *szDesc = 0;
                    
                if (*poad->poainfo->szApp && (poad->poainfo->dwInFlags & OAIF_REGISTER_EXT)
                && (IsDlgButtonChecked(poad->hDlg, IDD_MAKEASSOC)))
                        fMadeNewAssoc = OpenAsMakeAssociation(poad->lpszExt, poad->poainfo->szApp, szDesc, hkeyProgID);

                OpenWithListRegister(0, poad->lpszExt, NULL, hkeyProgID);
            
                RegCloseKey(hkeyProgID);
            }
            
            /* Did we register the association? */
            poad->hr = IsDlgButtonChecked(poad->hDlg, IDD_MAKEASSOC) ? S_OK : S_FALSE;

            /* Exec if requested. */
            if (poad->poainfo->dwInFlags & OAIF_EXEC)
            {
                RunAs(poad);
                SHAddToRecentDocs(SHARD_PATH, poad->poainfo->pcszFile);
            }

            _SaveCachedOpenAsList(poad->hwndList, fMadeNewAssoc);

            EndDialog(hDlg, TRUE);
            break;

        case IDCANCEL:
            poad->hr = E_ABORT;
            
            _SaveCachedOpenAsList(poad->hwndList, fMadeNewAssoc);

            EndDialog(hDlg, FALSE);
            break;

        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

// external API version

HRESULT 
OpenAsDialog(
    HWND        hwnd, 
    POPENASINFO poainfo)
{
    INT_PTR iRet;
    OPENAS_DATA oad = { 0 };
    int idDlg = DLG_OPENAS_NOTYPE;

    DebugMsg(DM_TRACE, TEXT("Enter OpenAs for %s"), poainfo->pcszFile);

#ifdef WINNT
    // Depending on policy, do not allow user to change file type association.
    if ( SHRestricted(REST_NOFILEASSOCIATE) )
    {
    	poainfo->dwInFlags &= ~OAIF_ALLOW_REGISTRATION & ~OAIF_REGISTER_EXT;
    }
#endif

    oad.hwnd = hwnd;
    oad.poainfo = poainfo;
    oad.lpszExt = PathFindExtension(oad.poainfo->pcszFile);

    // We don't allow association for files without extension or with only "." as extension
    if (!oad.lpszExt || !*oad.lpszExt || !lstrcmp(oad.lpszExt, TEXT(".")))
    {
        idDlg = DLG_OPENAS;
        poainfo->dwInFlags &= ~OAIF_ALLOW_REGISTRATION;
    }

    // Known file type(has verb): use DLG_OPENAS
    // NoOpen file type(has NoOpen value): use DLG_NOOPEN
    // Unknown file type(All others): use DLG_OPENAS_NOTYPE
    if (*oad.lpszExt && *(oad.lpszExt+1))
    {
        HRESULT hr;
        WCHAR wsz[MAX_PATH];
        IQueryAssociations *pqa;
        
        hr = AssocCreate(CLSID_QueryAssociations, &IID_IQueryAssociations, (LPVOID *)&pqa);
        if (FAILED(hr))
            return hr;
        
        SHTCharToUnicode(oad.lpszExt, wsz, SIZECHARS(wsz));        
        if (SUCCEEDED(pqa->lpVtbl->Init(pqa, 0, wsz, NULL, NULL)))
        {
            DWORD cch;

            idDlg = DLG_OPENAS;
            
            pqa->lpVtbl->GetString(pqa, 0, ASSOCSTR_FRIENDLYDOCNAME, NULL, 
                wsz, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(wsz)));
            SHUnicodeToTChar(wsz, oad.szDescription, SIZECHARS(oad.szDescription));
            
            if (SUCCEEDED(pqa->lpVtbl->GetString(pqa, 0, ASSOCSTR_NOOPEN, NULL, wsz, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(wsz)))) &&
                FAILED(pqa->lpVtbl->GetString(pqa, 0, ASSOCSTR_COMMAND, NULL, NULL, &cch)))
            {
                TCHAR sz[MAX_PATH];
                
                SHUnicodeToTChar(wsz, sz, SIZECHARS(sz));
                oad.lpszNoOpenMsg = sz;
                
                iRet = DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_NOOPEN), 
                                      hwnd, NoOpenDlgProc, (LPARAM)&oad);

                if ((-1 != iRet) && SUCCEEDED(oad.hr))
                {
                    // user selected cancel
                    pqa->lpVtbl->Release(pqa);
                    oad.idDlg = DLG_NOOPEN;
                    return oad.hr;
                }
            }
        }
        
        pqa->lpVtbl->Release(pqa);
    }
    
    oad.idDlg = idDlg;
    iRet = DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(idDlg), hwnd, 
                          OpenAsDlgProc, (LPARAM)(POPENAS_DATA)&oad);
    if (-1 != iRet)
        return oad.hr;
    else 
        return E_FAIL;
}

void WINAPI OpenAs_RunDLL(HWND hwnd, HINSTANCE hAppInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    OPENASINFO oainfo = { 0 };

#ifdef UNICODE
    UINT iLen = lstrlenA(lpszCmdLine)+1;
    LPWSTR  lpwszCmdLine;

    lpwszCmdLine = (LPWSTR)LocalAlloc(LPTR,iLen*SIZEOF(WCHAR));
    if (lpwszCmdLine)
    {
        MultiByteToWideChar(CP_ACP, 0,
                            lpszCmdLine, -1,
                            lpwszCmdLine, iLen);

        DebugMsg(DM_TRACE, TEXT("OpenAs_RunDLL is called with (%s)"), lpwszCmdLine);

        oainfo.pcszFile = lpwszCmdLine;
        oainfo.dwInFlags = (OAIF_ALLOW_REGISTRATION |
                            OAIF_REGISTER_EXT |
                            OAIF_EXEC);

        OpenAsDialog(hwnd, &oainfo);

        LocalFree(lpwszCmdLine);
    }
#else
    DebugMsg(DM_TRACE, TEXT("OpenAs_RunDLL is called with (%s)"), lpszCmdLine);

    oainfo.pcszFile = lpszCmdLine;
    oainfo.dwInFlags = (OAIF_ALLOW_REGISTRATION |
                        OAIF_REGISTER_EXT |
                        OAIF_EXEC);

    OpenAsDialog(hwnd, &oainfo);
#endif
}


void WINAPI OpenAs_RunDLLW(HWND hwnd, HINSTANCE hAppInstance, LPWSTR lpwszCmdLine, int nCmdShow)
{
    OPENASINFO oainfo = { 0 };

#ifdef UNICODE
    DebugMsg(DM_TRACE, TEXT("OpenAs_RunDLL is called with (%s)"), lpwszCmdLine);

    oainfo.pcszFile = lpwszCmdLine;
    oainfo.dwInFlags = (OAIF_ALLOW_REGISTRATION |
                        OAIF_REGISTER_EXT |
                        OAIF_EXEC);

    OpenAsDialog(hwnd, &oainfo);
#else
    UINT iLen = WideCharToMultiByte(CP_ACP, 0,
                                    lpwszCmdLine, -1,
                                    NULL, 0, NULL, NULL)+1;
    LPSTR  lpszCmdLine;

    lpszCmdLine = (LPSTR)LocalAlloc(LPTR,iLen);
    if (lpszCmdLine)
    {
        WideCharToMultiByte(CP_ACP, 0,
                            lpwszCmdLine, -1,
                            lpszCmdLine, iLen,
                            NULL, NULL);

        DebugMsg(DM_TRACE, TEXT("OpenAs_RunDLL is called with (%s)"), lpszCmdLine);

        oainfo.pcszFile = lpszCmdLine;
        oainfo.dwInFlags = (OAIF_ALLOW_REGISTRATION |
                            OAIF_REGISTER_EXT |
                            OAIF_EXEC);

        OpenAsDialog(hwnd, &oainfo);

        LocalFree(lpszCmdLine);
    }
#endif
}

#ifdef DEBUG
//
// Type checking
//
const static RUNDLLPROCA lpfnRunDLL = OpenAs_RunDLL;
const static RUNDLLPROCW lpfnRunDLLW = OpenAs_RunDLLW;
#endif
