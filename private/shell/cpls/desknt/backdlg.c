/*  BACKDLG.C
**
**  Copyright (C) Microsoft, 1993, All Rights Reserved.
**
**
**  History:
**
*/
#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include "desk.h"
#include "deskid.h"
#include "commctrl.h"
#include "comctrlp.h"
#include <commdlg.h>
#include <regstr.h>

#define MAX_RHS    256

TCHAR g_szPattern[] = TEXT("pattern");
TCHAR szDesktop[] = TEXT("desktop");
TCHAR szWallpaper[] = TEXT("wallpaper");
TCHAR szTileWall[] = TEXT("TileWallpaper");
TCHAR szDotBMP[] = TEXT(".bmp");
TCHAR szBMP[] = TEXT("\\*.bmp");
TCHAR szDefExt[] = TEXT("bmp");
BOOL g_bValidBitmap = FALSE;    // the currently selected wallpaper is valid

TCHAR g_szCurPattern[MAX_PATH];
TCHAR g_szCurWallpaper[MAX_PATH];
TCHAR g_szTempItem[MAX_PATH];      // which is more of a waste, stack or data?

BOOL g_Back_bInit = TRUE;       // assume we are in initialization process
BOOL g_Back_bChanged = FALSE;   // changes have been made

static void NukeExt(LPTSTR sz);
static void AddExt(LPTSTR sz, LPTSTR x);
static LPTSTR NEAR PASCAL NiceName(LPTSTR sz);

BOOL CALLBACK PatternDlgProc(HWND hDlg, WORD wMsg, WPARAM wParam, LPARAM lParam);

#include "help.h"

const static DWORD FAR aBckgrndHelpIds[] = {
        IDC_NO_HELP_1,   IDH_COMM_GROUPBOX,
        IDC_NO_HELP_2,   IDH_COMM_GROUPBOX,
        IDC_PATLIST,     IDH_DSKTPBACKGROUND_PATTLIST,
        IDC_EDITPAT,     IDH_DSKTPBACKGROUND_EDITPAT,
        IDC_WALLLIST,    IDH_DSKTPBACKGROUND_WALLLIST,
        IDC_BROWSEWALL,  IDH_DSKTPBACKGROUND_BROWSE,
        IDC_TXT_DISPLAY, IDH_DSKTPBACKGROUND_DISPLAY,
        IDC_TILE,        IDH_DSKTPBACKGROUND_TILE,
        IDC_CENTER,      IDH_DSKTPBACKGROUND_CENTER,
        IDC_BACKPREV,    IDH_DSKTPBACKGROUND_MONITOR,

        0, 0
};

static const TCHAR szRegStr_Desktop[] = REGSTR_PATH_DESKTOP;
static const TCHAR szRegStr_Setup[] = REGSTR_PATH_SETUP TEXT("\\Setup");
static const TCHAR szSharedDir[] = TEXT("SharedDir");

// we're mainly trying to filter multilingual upgrade cases
// where the text for "(None)" is unpredictable
// yes, I want to strangle the bastard who decided to write out the none entry
BOOL NEAR PASCAL IsProbablyAValidPattern( LPCTSTR pat )
{
    BOOL sawanumber = FALSE;

    while( *pat )
    {
        if( ( *pat < TEXT('0') ) || ( *pat > TEXT('9') ) )
        {
            // it's not a number, it better be a space
            if( *pat != TEXT(' ') )
                return FALSE;
        }
        else
            sawanumber = TRUE;

        // NOTE: we avoid the need for AnsiNext by only advancing on US TCHARs
        pat++;
    }

    // TRUE if we saw at least one digit and there were only digits and spaces
    return sawanumber;
}


#ifdef DEBUG

#define REG_INTEGER  1000
int  fTraceRegAccess = 0;

void NEAR PASCAL  RegDetails(int iWrite, HKEY hk, LPCTSTR lpszSubKey,
    LPCTSTR lpszValueName, DWORD dwType, LPTSTR  lpszString, int iValue)
{
  TCHAR Buff[256];
  TCHAR far *lpszReadWrite[] = { TEXT("DESK.CPL:Read"), TEXT("DESK.CPL:Write") };

  if(!fTraceRegAccess)
     return;

  switch(dwType)
    {
      case REG_SZ:
          wsprintf(Buff, TEXT("%s String:hk=%#08lx, %s:%s=%s\n\r"), lpszReadWrite[iWrite],
                           hk, lpszSubKey, lpszValueName, lpszString);
          break;

      case REG_INTEGER:
          wsprintf(Buff, TEXT("%s int:hk=%#08lx, %s:%s=%d\n\r"), lpszReadWrite[iWrite],
                           hk, lpszSubKey, lpszValueName, iValue);
          break;

      case REG_BINARY:
          wsprintf(Buff, TEXT("%s Binary:hk=%#08lx, %s:%s=%#0lx;DataSize:%d\r\n"), lpszReadWrite[iWrite],
                           hk, lpszSubKey, lpszValueName, lpszString, iValue);
          break;
    }
  OutputDebugString(Buff);
}

#endif  // DEBUG


//---------------------------------------------------------------------------
//  GetIntFromSubKey
//      hKey is the handle to the subkey
//      (already pointing to the proper location).
//---------------------------------------------------------------------------

int NEAR PASCAL GetIntFromSubkey(HKEY hKey, LPCTSTR lpszValueName, int iDefault)
{
  TCHAR  szValue[20];
  DWORD dwSizeofValueBuff = sizeof(szValue);
  DWORD dwType;
  int   iRetValue = iDefault;

  if((RegQueryValueEx(hKey, lpszValueName, NULL, &dwType,
                        (LPBYTE)szValue,
                &dwSizeofValueBuff) == ERROR_SUCCESS) && dwSizeofValueBuff)
  {
    // BOGUS: This handles only the string type entries now!
    if(dwType == REG_SZ)
        iRetValue = (int)StrToInt(szValue);
#ifdef DEBUG
    else
        OutputDebugString(TEXT("String type expected from Registry\n\r"));
#endif
  }
#ifdef DEBUG
  RegDetails(0, hKey, TEXT(""), lpszValueName, REG_INTEGER, NULL, iRetValue);
#endif
  return(iRetValue);
}

//---------------------------------------------------------------------------
//  GetIntFromReg()
//       Opens the given subkey and gets the int value.
//---------------------------------------------------------------------------

int NEAR PASCAL GetIntFromReg(HKEY   hKey,
                                        LPCTSTR lpszSubkey,
                                        LPCTSTR lpszNameValue, int iDefault)
{
  HKEY hk;
  int   iRetValue = iDefault;

  // See if the key is present.
  if(RegOpenKey(hKey, lpszSubkey, &hk) == ERROR_SUCCESS)
    {
      iRetValue = GetIntFromSubkey(hk, lpszNameValue, iDefault);
      RegCloseKey(hk);
    }
  return(iRetValue);
}

BOOL NEAR PASCAL GetStringFromReg(HKEY   hKey,
                                        LPCTSTR lpszSubkey,
                                        LPCTSTR lpszValueName,
                                        LPCTSTR lpszDefault,
                                        LPTSTR lpszValue,
                                        DWORD cchSizeofValueBuff)
{
  HKEY hk;
  DWORD dwType;
  BOOL  fSuccess = FALSE;

  cchSizeofValueBuff *= sizeof(TCHAR);

  // See if the key is present.
  if(RegOpenKey(hKey, lpszSubkey, &hk) == ERROR_SUCCESS)
    {
      if((RegQueryValueEx(hk, lpszValueName, NULL, &dwType,
                        (LPBYTE)lpszValue,
                &cchSizeofValueBuff) == ERROR_SUCCESS) && cchSizeofValueBuff)
        {
          // BOGUS: This handles only the string type entries now!
#ifdef DEBUG
          if(dwType != REG_SZ)
            {
              OutputDebugString(TEXT("String type expected from Registry\n\r"));
            }
          else
#endif
            fSuccess = TRUE;
        }
      RegCloseKey(hk);
    }

  // If failure, use the default string.
  if(!fSuccess && lpszDefault)
      lstrcpy(lpszValue, lpszDefault);

#ifdef DEBUG
  RegDetails(0, hKey, lpszSubkey, lpszValueName, REG_SZ, lpszValue, 0);
#endif
  return(fSuccess);
}

//---------------------------------------------------------------------------
//
//  UpdateRegistry:
//      This updates a given value of any data type at a given
//      location in the registry.
//
//  The value name is passed in as an Id to a string in USER's String table.
//
//---------------------------------------------------------------------------

BOOL FAR PASCAL UpdateRegistry(HKEY     hKey,
                                LPCTSTR   lpszSubkey,
                                LPCTSTR   lpszValueName,
                                DWORD   dwDataType,
                                LPVOID  lpvData,
                                DWORD   dwDataSize)
{
  HKEY  hk;

  if(RegCreateKey(hKey, lpszSubkey, &hk) == ERROR_SUCCESS)
    {
      RegSetValueEx(hk, lpszValueName,
                        0L, dwDataType,
                        lpvData,
                        dwDataSize);
#ifdef DEBUG
      RegDetails(1, hKey, lpszSubkey, lpszValueName, dwDataType, lpvData, (int)dwDataSize);
#endif
      RegCloseKey(hk);
      return(TRUE);
    }
  return(FALSE);
}

/*-------------------------------------------------------------
** force the update of the preview.
**-------------------------------------------------------------*/
void NEAR PASCAL UpdatePreview(HWND hDlg, WORD flags)
{
    if (IsDlgButtonChecked(hDlg, IDC_TILE))
        flags |= BP_TILE;

    SendDlgItemMessage(hDlg, IDC_BACKPREV, WM_SETBACKINFO, flags, 0L);

    if (!g_Back_bInit && !g_Back_bChanged)
    {
        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);

        g_Back_bChanged = TRUE;
    }
}

/*------------------------------------------------------------------
** read in all of the entries (LHS only) in a section.
**
** return: handle to local (fixed) memory containing names
**------------------------------------------------------------------*/
HANDLE NEAR PASCAL GetSection(LPTSTR lpIniFile, LPTSTR lpSection)
{
    int nCount;
    short nSize = 4096 * sizeof(TCHAR);
    HANDLE hLocal, hTemp;

    if (!(hLocal = LocalAlloc(LPTR, nSize)))
        return(NULL);

    while (1)
    {
        nCount = GetPrivateProfileString(lpSection, NULL, g_szNULL, (LPTSTR)hLocal, nSize, lpIniFile);

        if (nCount <= nSize-2)
            break;

        // need to grow the buffer
        nSize += (2048 * sizeof(TCHAR));
        hTemp = hLocal;
        if (!(hLocal = LocalReAlloc(hLocal, nSize, LMEM_MOVEABLE)))
        {
            LocalFree(hTemp);
            return(NULL);
        }
    }
    return(hLocal);
}

static void NukeExt(LPTSTR sz)
{
    int len;

    len = lstrlen(sz);

    if (len > 4 && sz[len-4] == TEXT('.'))
        sz[len-4] = 0;
}

static void AddExt(LPTSTR sz, LPTSTR x)
{
    int len;

    len = lstrlen(sz);

    if (len <= 4 || sz[len-4] != TEXT('.'))
    {
        lstrcat(sz, TEXT("."));
        lstrcat(sz, x);
    }
}

static void NameOnly(LPTSTR sz)
{
    LPTSTR p = sz;
    LPTSTR s = NULL;

    while( *p )
    {
        if( ( *p == TEXT('\\') ) || ( *p == TEXT(':') ) )
            s = p;
#ifdef  DBCS
        p = AnsiNext(p);
#else
        p++;
#endif
    }

    if( s )
    {
        p = sz;

        while( *s++ )
        {
            *p++ = *s;
        }
    }
}

static BOOL PathOnly(LPTSTR sz)
{
    LPTSTR p = sz;
    LPTSTR s = NULL;

    while( *p )
    {
        if( *p == TEXT('\\') )
        {
            s = p;
        }
        else if( *p == TEXT(':') )
        {
            s = p + 1;
        }
#ifdef  DBCS
        p = AnsiNext(p);
#else
        p++;
#endif
    }

    if( s )
    {
        if( s == sz )
            s++;

        *s = TEXT('\0');
        return TRUE;
    }

    return FALSE;
}

static LPTSTR NEAR PASCAL NiceName(LPTSTR sz)
{
    NukeExt(sz);

    if (IsCharUpper(sz[0]) && IsCharUpper(sz[1]))
    {
        CharLower(sz);
        CharUpperBuff(sz, 1);
    }

    return sz;
}

int NEAR PASCAL AddAFileToLB( HWND hwndList, LPCTSTR szDir, LPTSTR szFile )
{
    int index = LB_ERR;

    LPTSTR szPath = (LPTSTR)LocalAlloc( LPTR,
        (( szDir? lstrlen( szDir ) : 0 ) + lstrlen( szFile ) + 2) * sizeof(TCHAR) );

    if( szPath )
    {
        if( szDir )
        {
            lstrcpy( (LPTSTR)szPath, (LPTSTR)szDir );
            lstrcat( (LPTSTR)szPath, TEXT("\\") );
        }
        else
            *szPath = TEXT('\0');

        lstrcat( (LPTSTR)szPath, szFile );
        NameOnly( szFile );
        NiceName( szFile );

        index = (int)SendMessage( hwndList, LB_ADDSTRING, 0,
            (LPARAM)(LPTSTR)szFile );

        if( index >= 0 )
        {
            SendMessage( hwndList, LB_SETITEMDATA, (WPARAM)index,
                (LPARAM)(LPTSTR)szPath );
        }
        else
            LocalFree( (HANDLE)(LONG)szPath );
    }

    return index;
}

void NEAR PASCAL AddFilesToLB(HWND hwndList, LPTSTR pszDir, LPTSTR szSpec)
{
    WIN32_FIND_DATA fd;
    HANDLE h;
    TCHAR szBuf[MAX_PATH];

    lstrcpy(szBuf, pszDir);
    lstrcat(szBuf, szSpec);

    h = FindFirstFile(szBuf, &fd);

    if (h != INVALID_HANDLE_VALUE)
    {
        do
        {
            AddAFileToLB(hwndList, pszDir, fd.cFileName);
        }
        while (FindNextFile(h, &fd));

        FindClose(h);
    }
}

/*-------------------------------------------------------------
** set a new wallpaper and notify the right places.
**
** the new name is in g_szCurWallpaper
**-------------------------------------------------------------*/
void NEAR PASCAL SetNewWallpaper(HWND hDlg, LPTSTR szFile, BOOL bCanAdd)
{
    HWND hwndList = GetDlgItem(hDlg, IDC_WALLLIST);

    if(!szFile || !lstrcmpi(szFile, g_szNone))
        szFile = TEXT("");

    if(
#ifndef UNICODE
        !IsDBCSLeadByte(szFile[0]) &&
#endif
        (szFile[1] == TEXT(':')) )
    {
        TCHAR szDrive[3];
        TCHAR szNet[MAX_PATH];
        int lenNet = ARRAYSIZE(szNet);

        lstrcpyn(szDrive, szFile, ARRAYSIZE(szDrive));
        if ((WNetGetConnection( szDrive, szNet, &lenNet ) ==
            NO_ERROR) && (szNet[0] == TEXT('\\')) && (szNet[1] == TEXT('\\')))
        {
            lstrcat(szNet, szFile+2);
            lstrcpy(szFile, szNet);
        }
    }

    lstrcpy(g_szCurWallpaper, szFile);
    UpdatePreview(hDlg, BP_NEWWALL);

    if(bCanAdd && *szFile && g_bValidBitmap)
    {
        TCHAR szName[MAX_PATH];
        int sel;

        lstrcpy(szName, szFile);
        NameOnly(szName);
        NiceName(szName);

        if ((sel = (int)SendMessage(hwndList, LB_FINDSTRINGEXACT, (WPARAM)-1,
            (LPARAM)(LPTSTR)szName)) == LB_ERR)
        {
            sel = AddAFileToLB(hwndList, NULL, szFile);
        }

        SendMessage(hwndList, LB_SETCURSEL, (WPARAM)sel, 0L);
    }

    {
        BOOL bEnable = (*szFile) ? TRUE : FALSE;

        EnableWindow( GetDlgItem(hDlg, IDC_TXT_DISPLAY), bEnable );
        EnableWindow( GetDlgItem(hDlg, IDC_TILE), bEnable );
        EnableWindow( GetDlgItem(hDlg, IDC_CENTER), bEnable );
    }
}

void NEAR PASCAL InitBackgroundDialog(HWND hDlg)
{
    HANDLE hSection;
    HWND hwndList;
    PTSTR pszBuffer;
    TCHAR szBuf[MAX_PATH];
    TCHAR szCurPatBits[MAX_PATH];

    g_szCurPattern[0] = 0;
    g_szCurWallpaper[0] = 0;
    g_Back_bChanged = FALSE;

    /*
    ** initialize the pattern list
    */
    // get the current pattern.
    // GetProfileString(szDesktop, g_szPattern, g_szNULL, szCurPatBits, ARRAYSIZE(szCurPatBits));
    GetStringFromReg(HKEY_CURRENT_USER, szRegStr_Desktop,
                                        g_szPattern, g_szNULL, szCurPatBits,
                                        ARRAYSIZE(szCurPatBits));
    if (!(*szCurPatBits))
        lstrcpy(g_szCurPattern, g_szNone);
    else
        *g_szCurPattern = 0;

    hwndList = GetDlgItem(hDlg, IDC_PATLIST);
    if (hSection = GetSection(g_szControlIni, g_szPatterns))
    {
        BOOL bAddedNone = FALSE;
        /* Put the patterns into the combo box. */
        for (pszBuffer=LocalLock(hSection); *pszBuffer; pszBuffer+=lstrlen(pszBuffer)+1)
        {
            if (GetPrivateProfileString(g_szPatterns, pszBuffer, g_szNULL, szBuf, ARRAYSIZE(szBuf), g_szControlIni))
            {
                BOOL bIsNone = !bAddedNone && !lstrcmpi( g_szNone, szBuf );

                /* if there's a right-hand side, add it to the list box */
                if( bIsNone || IsProbablyAValidPattern( szBuf ) )
                {
                    if( bIsNone )
                        bAddedNone = TRUE;

                    SendMessage(hwndList, LB_ADDSTRING, 0, (DWORD)(LPTSTR)pszBuffer);

                    // if haven't found current pattern name, maybe this is it.
                    if (!(*g_szCurPattern) && (!lstrcmpi(szBuf, szCurPatBits)))
                    {
                        // same pattern bits.  we have a name
                        lstrcpy(g_szCurPattern, pszBuffer);
                    }
                }
            }
        }
        LocalUnlock(hSection);
        LocalFree(hSection);
    }

    // if our patternTEXT('s bits weren')t in the list, use a fake name
    if (!(*g_szCurPattern))
        LoadString(hInstance, IDS_UNLISTEDPAT, g_szCurPattern, ARRAYSIZE(g_szCurPattern));

    SendMessage(hwndList, LB_SELECTSTRING, (WPARAM)-1, (DWORD)(LPTSTR)g_szCurPattern);
    UpdatePreview(hDlg, BP_NEWPAT);

    // exclude TEXT("none") pattern
    if( (int)SendDlgItemMessage(hDlg,IDC_PATLIST,LB_GETCURSEL,0,0l) <= 0 )
    {
        HWND epat = GetDlgItem( hDlg, IDC_EDITPAT );

        if( GetFocus() == epat )
        {
            SendMessage( hDlg, WM_NEXTDLGCTL,
                (WPARAM)GetDlgItem( hDlg, IDC_PATLIST ), (LPARAM)TRUE );
        }

        EnableWindow( epat, FALSE );
    }

    /*
    ** initialize the tile/center buttons
    */
    if(GetIntFromReg(HKEY_CURRENT_USER, szRegStr_Desktop, szTileWall, 1))
        CheckRadioButton(hDlg, IDC_CENTER, IDC_TILE, IDC_TILE);
    else
        CheckRadioButton(hDlg, IDC_CENTER, IDC_TILE, IDC_CENTER);

    /*
    ** initialize the wallpaper list
    */
    hwndList = GetDlgItem(hDlg, IDC_WALLLIST);

    GetWindowsDirectory(szBuf, ARRAYSIZE(szBuf));
    // override with net home dir on shared copies of windows
    GetStringFromReg(HKEY_LOCAL_MACHINE, szRegStr_Setup, szSharedDir,
        (LPTSTR)NULL, szBuf, ARRAYSIZE(szBuf));
    AddFilesToLB(hwndList, szBuf, szBMP);

    //GetProfileString(szDesktop, szWallpaper, g_szNone, szBuf, sizeof(szBuf));
    GetStringFromReg(HKEY_CURRENT_USER, szRegStr_Desktop, szWallpaper, g_szNone, szBuf, ARRAYSIZE(szBuf));

    SetNewWallpaper(hDlg, szBuf, TRUE); // will add and select if not in list

    // and don't forget the 'none' option
    if (SendMessage(hwndList, LB_INSERTSTRING, 0, (DWORD)(LPTSTR)g_szNone) !=
        LB_ERR)
    {
        int sel = (int)SendMessage(hwndList, LB_GETCURSEL, 0, 0L);

        if (sel == -1)
            sel = 0;

        SendMessage(hwndList, LB_SETCURSEL, (WPARAM)sel, 0L);
        if (!sel) {
            EnableWindow( GetDlgItem(hDlg, IDC_TILE), FALSE );
            EnableWindow( GetDlgItem(hDlg, IDC_CENTER), FALSE );
            EnableWindow( GetDlgItem(hDlg, IDC_TXT_DISPLAY), FALSE );
        }
    }

    // allow people to drag wallpapers to this page
    DragAcceptFiles(hDlg, TRUE);
}

//the intl tools cannot handle embedded nulls in strings
//hack: use the vertical bar and convert
void NEAR PASCAL
ConvertPipesToNull(LPTSTR szFilter)
{
#ifdef DBCS
    if (IsDBCSLeadByte('|'))
        return;
#endif

    while (*szFilter)
    {
        LPTSTR p = CharNext(szFilter);

        if (*szFilter == TEXT('|'))
            *szFilter = TEXT('\0');

        szFilter = p;
    }
}

void NEAR PASCAL BrowseForWallpaper(HWND hDlg)
{
    TCHAR szPath[MAX_PATH];
    static TCHAR szWorkDir[MAX_PATH] = TEXT("");
    OPENFILENAME ofn;

    TCHAR szTitle[60];
    TCHAR szFilter[64];

    LoadString(hInstance, IDS_BROWSETITLE, szTitle, ARRAYSIZE(szTitle));
    if (LoadString(hInstance, IDS_BROWSEFILTER, szFilter, ARRAYSIZE(szFilter)))
        ConvertPipesToNull(szFilter);


    if( !PathOnly( szWorkDir ) )
        GetWindowsDirectory(szWorkDir, ARRAYSIZE(szWorkDir));

    szPath[0] = TEXT('\0');

    ofn.lStructSize       = sizeof(ofn);
    ofn.hwndOwner         = hDlg;
    ofn.hInstance         = NULL;
    ofn.lpstrFilter       = szFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nFilterIndex      = 1;
    ofn.nMaxCustFilter    = 0;
    ofn.lpstrFile         = szPath;
    ofn.nMaxFile          = ARRAYSIZE(szPath);
    ofn.lpstrInitialDir   = szWorkDir;
    ofn.lpstrTitle        = szTitle;
    ofn.Flags             = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
    ofn.lpfnHook          = NULL;
    ofn.lpstrDefExt       = szDefExt;
    ofn.lpstrFileTitle    = NULL;

    if (GetOpenFileName(&ofn) && (lstrcmpi(g_szCurWallpaper, szPath) != 0))
    {
        CharUpper(szPath); // will be nicenamed (best we can do...)
        SetNewWallpaper(hDlg, szPath, TRUE);
    }

    GetCurrentDirectory(ARRAYSIZE(szWorkDir), szWorkDir);
}

void NEAR PASCAL HandleWallpaperDrop(HWND hDlg, HDROP hDrop)
{
    TCHAR szPath[MAX_PATH];

    if (DragQueryFile(hDrop, 1, szPath, ARRAYSIZE(szPath)) &&
        (lstrcmpi(g_szCurWallpaper, szPath) != 0))
    {
        int len = lstrlen(szPath);

        if (len > 4 && !lstrcmpi(szPath+len-4, szDotBMP))
            SetNewWallpaper(hDlg, szPath, TRUE);
    }

    DragFinish(hDrop);
}

void NEAR PASCAL EditThePattern( HWND hDlg )
{
    int  iTemp = (int)SendMessage( GetDlgItem( hDlg, IDC_PATLIST ),
        LB_GETCURSEL, 0, 0L );

    if( iTemp > 0 )                 // 0 is the "none" pattern
    {
        TCHAR szBuf[MAX_PATH];

        // GROSS: pass the combobox handle to the pattern box
        //          so it may edit patterns and refill us
        DialogBoxParam(hInstance, (LPCTSTR)MAKEINTRESOURCE(DLG_PATTERN),
            GetParent(hDlg), (DLGPROC)PatternDlgProc,
            (LPARAM)GetDlgItem(hDlg, IDC_PATLIST) );

        // update the preview
        iTemp = (int)SendDlgItemMessage(hDlg,IDC_PATLIST,
            LB_GETCURSEL,0,0l);
        if(iTemp >= 0)
        {
            SendDlgItemMessage(hDlg, IDC_PATLIST, LB_GETTEXT,
                iTemp, (LPARAM)(LPTSTR)szBuf);

            lstrcpy(g_szCurPattern, szBuf);
            UpdatePreview(hDlg, BP_NEWPAT);
        }

        // exclude "none" pattern
        if( ( iTemp <= 0 ) &&
            ( GetFocus() == GetDlgItem( hDlg, IDC_EDITPAT ) ) )
        {
            SendMessage( hDlg, WM_NEXTDLGCTL,
                (WPARAM)GetDlgItem( hDlg, IDC_PATLIST ), (LPARAM)TRUE );
        }

        // exclude "none" pattern
        EnableWindow( GetDlgItem( hDlg, IDC_EDITPAT ), ( iTemp > 0 ) );
    }
}

BOOL APIENTRY  BackgroundDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;
    TCHAR szTiled[] = TEXT("0");
    TCHAR szBuf[MAX_PATH];
    TCHAR szBuf2[50];
    int  iTemp;

    switch(message)
    {
        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code)
            {
                case PSN_APPLY: {
                    DWORD dwRet = PSNRET_NOERROR;
                    if (g_Back_bChanged)
                    {
                        HCURSOR old = SetCursor( LoadCursor( NULL, IDC_WAIT ) );
                        HWND cover;

                        if (!g_bValidBitmap)
                        {
                            LoadString(hInstance, IDS_BADWALLPAPER, szBuf, ARRAYSIZE(szBuf));
                            GetWindowText(GetParent(hDlg), szBuf2, ARRAYSIZE(szBuf2));
                            MessageBox(hDlg, szBuf, szBuf2, MB_OK | MB_ICONEXCLAMATION);
                            dwRet = PSNRET_INVALID_NOCHANGEPAGE;
                        }

                        // do this after whimpering
                        cover = CreateCoverWindow( COVER_NOPAINT );

                        // need to write out tile first
                        szTiled[0] += (TCHAR)IsDlgButtonChecked(hDlg, IDC_TILE);
                        UpdateRegistry(HKEY_CURRENT_USER, szRegStr_Desktop,
                            szTileWall, REG_SZ, szTiled, SIZEOF(TCHAR)*(lstrlen(szTiled)+1));

                        if (g_bValidBitmap)
                        {
                            SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, g_szCurWallpaper,
                                SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
                        }

                        if (GetPrivateProfileString(g_szPatterns, g_szCurPattern, g_szNULL, szBuf, ARRAYSIZE(szBuf), g_szControlIni))
                        {
                            SystemParametersInfo(SPI_SETDESKPATTERN, 0, szBuf,
                                        SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
                        }

                        // we're back to no changes
                        g_Back_bChanged = FALSE;

                        if( cover )
                            PostMessage( cover, WM_CLOSE, 0, 0L );

                        SetCursor( old );
                    }
                    SetWindowLong(hDlg, DWL_MSGRESULT, dwRet );
                    return TRUE;
                }

                case PSN_RESET:
                    break;
            }
            break;

        case WM_INITDIALOG:
            g_Back_bInit = TRUE;
            InitBackgroundDialog(hDlg);
            g_Back_bInit = FALSE;               // no longer initializing
            break;

        case WM_SYSCOLORCHANGE:
        case WM_DISPLAYCHANGE:
            g_Back_bInit = TRUE;    // fake init so we don't do PSM_CHANGED
            UpdatePreview(hDlg, BP_REINIT | BP_NEWPAT );
            g_Back_bInit = FALSE;
            break;

        case WM_DESTROY:
        {
            int count = (int)SendDlgItemMessage(hDlg, IDC_WALLLIST,
                LB_GETCOUNT, 0, 0L);

            while (count--)
            {
                LPTSTR sz = (LPTSTR)SendDlgItemMessage(hDlg, IDC_WALLLIST,
                    LB_GETITEMDATA, count, 0L);

                if (sz)
                    LocalFree ((HANDLE)(LONG)sz);
            }
            break;
        }

        case WM_HELP:
            WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, NULL,
                HELP_WM_HELP, (DWORD) (LPTSTR) aBckgrndHelpIds);
            return TRUE;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
                (DWORD) (LPTSTR) aBckgrndHelpIds);
            return TRUE;

        case WM_DROPFILES:
            HandleWallpaperDrop(hDlg, (HDROP)wParam);
            return TRUE;

        case WM_QUERYNEWPALETTE:
        case WM_PALETTECHANGED:
            SendDlgItemMessage(hDlg, IDC_BACKPREV, message, wParam, lParam);
            return TRUE;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_PATLIST:
                    if(HIWORD(wParam) == LBN_SELCHANGE)
                    {
                        iTemp = (int)SendDlgItemMessage(hDlg,IDC_PATLIST,
                            LB_GETCURSEL,0,0l);
                        if(iTemp >= 0)
                        {
                            SendDlgItemMessage(hDlg, IDC_PATLIST, LB_GETTEXT,
                                iTemp, (LPARAM)(LPTSTR)szBuf);

                            if (lstrcmpi(szBuf, g_szCurPattern) == 0)
                                break;

                            lstrcpy(g_szCurPattern, szBuf);
                            UpdatePreview(hDlg, BP_NEWPAT);
                        }

                        EnableWindow( GetDlgItem( hDlg, IDC_EDITPAT ),
                            ( iTemp > 0 ) );  // exclude "none" pattern
                    }
                    else if(HIWORD(wParam) == LBN_DBLCLK)
                    {
                        EditThePattern( hDlg );
                    }
                    break;

                case IDC_WALLLIST:
                    if(HIWORD(wParam) == LBN_SELCHANGE)
                    {
                        LPTSTR pBuf = NULL;

                        iTemp = (int)SendDlgItemMessage(hDlg,IDC_WALLLIST,
                            LB_GETCURSEL,0,0l);

                        if(iTemp >= 0)
                        {
                            pBuf = (LPTSTR)SendDlgItemMessage(hDlg,
                                IDC_WALLLIST, LB_GETITEMDATA, iTemp, 0L);
                        }

                        SetNewWallpaper(hDlg, pBuf, FALSE);
                    }
                    break;

                case IDC_CENTER:
                case IDC_TILE:
                    if ((HIWORD(wParam) == BN_CLICKED) &&
                                (!IsDlgButtonChecked(hDlg, LOWORD(wParam))))
                    {
                        CheckRadioButton(hDlg, IDC_CENTER, IDC_TILE, LOWORD(wParam));
                        UpdatePreview(hDlg, 0);
                    }
                    break;

                case IDC_BROWSEWALL:
                    BrowseForWallpaper(hDlg);
                    break;

                case IDC_EDITPAT:
                    EditThePattern( hDlg );
                    break;
            }
            break;
    }
    return FALSE;
}
