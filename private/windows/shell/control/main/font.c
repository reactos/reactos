/** FILE: font.c *********** Module Header ********************************
 *
 *      Control panel applet for Font configuration.  This file holds code for
 *  the items concerning fonts.
 *
 * History:
 *  12:30 on Tues  23 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *          Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
 *            Updated code to latest Win 3.1 sources
 *  04 April 1994  -by-  Steve Cathcart   [stevecat]
 *            Added support for PostScript Type 1 fonts
 *
 *  Copyright (C) 1990-1994 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                           Include files
//==========================================================================
// C Runtime
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Application specific
#include "main.h"

#undef COLOR
#define CONST const

// Windows SDK
#include <wingdip.h>    // For private GDI entry point: GetFontResourceInfo()
#include <shellapi.h>

#include <commdlg.h>

//==========================================================================
//                          Local Definitions
//==========================================================================

#define SECURE              // Enable Initialization for C2 compatibility


#define WM_DROPFILES    0x0233

#define FILESONLY               0x0000
#define DIRSONLY                0xC010

typedef int (*GFDPROC)(HDC, LPTSTR, LPTSTR);

GFDPROC lpfnGetFontDir;

#define FONTPATHKEY TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontPath")

//==========================================================================
//                         External Declarations
//==========================================================================

/* Data */
extern TCHAR szTrueType[];
extern TCHAR szFonts[];
extern TCHAR szTTF[];
extern TCHAR szFON[];

// Functions


//==========================================================================
//                         Local Data Declarations
//==========================================================================
HANDLE  hLogFonts;
TCHAR   szOldFileName[PATHMAX];
SHORT   nCurSel;

TCHAR   szTest[ LF_FACESIZE + 55 ]; /* size of face name + alphabet  */
TCHAR   szEnable[] = TEXT("TTEnable");
TCHAR   szOnly[] = TEXT("TTOnly");

BOOL    bTrueTypeSave;
BOOL    bTrueTypeOnly;

RECT    Orig;
RECT    SampleRect;

HWND    hLBoxInstalled;

BOOL    bDeleteFont;
BOOL    bDeletePS;
TCHAR   szSaveSharedDir[PATHMAX];

BOOL    fDefCharUsed;
BOOL    gfFontsProtected = FALSE;

char    szFontsDirA[PATHMAX];           //  ANSI string!

TCHAR   szFontsKey[] = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts");

//==========================================================================
//                       Local Function Prototypes
//==========================================================================

BOOL   GetTTFilename (LPTSTR pszFile, int cb, LPTSTR pszTTFile);
SHORT  InitFontDlg   (HWND hDlg);
void   DoAddFont     (HWND hDlg);

BOOL APIENTRY RemoveFontDlg (HWND hDlg, UINT nMsg, DWORD wParam, LONG lParam);

//==========================================================================
//                              Functions
//==========================================================================

/////////////////////////////////////////////////////////////////////////////
//
// MyOpenSystemFile
//
//
/////////////////////////////////////////////////////////////////////////////

HANDLE MyOpenSystemFile (LPTSTR lpName, LPTSTR lpPathName, WORD wFlags)
{
    int    nGetDirErr;
    HANDLE nRet;
    TCHAR  szCurDir[PATHMAX];
    TCHAR  szWinDir[PATHMAX];
    LPTSTR lpEnd;

    //
    //  Set the default dir to the Windows dir if this is not a full path name
    //

    if (lpName[1] != TEXT(':') || lpName[2] != TEXT('\\'))
    {
        nGetDirErr = GetCurrentDirectory (CharSizeOf(szCurDir), szCurDir);

        //
        // Make sure szWinDir has a backslash, and then remove it if it
        // is not the root dir; then make the windows dir the current dir
        // so that OpenFile will not look in strange dirs for the file
        //

        lstrcpy (szWinDir, pszWinDir);
        if ((lpEnd = BackslashTerm (szWinDir)) - szWinDir != 3)
            *(lpEnd-1) = TEXT('\0');
        SetCurrentDirectory (szWinDir);
    }
    else
        nGetDirErr = 0;

    nRet = OpenFileWithShare (lpName, lpPathName, wFlags);

    //
    // Restore the dir and return
    //

    if (nGetDirErr != 0  &&  nGetDirErr < CharSizeOf(szCurDir))
        SetCurrentDirectory (szCurDir);

    return(nRet);
}


/////////////////////////////////////////////////////////////////////////////
//
// MyOpenFontFile
//
//  Sets the current directory to the "Shared" or "Fonts" directory before
//  attempting to open an undecorated filename.  Under Windows NT, this is
//  the "%systemroot%\system" directory (e.g. "d:\nt\system").
//
/////////////////////////////////////////////////////////////////////////////

HANDLE MyOpenFontFile (LPTSTR lpName, LPTSTR lpPathName, WORD wFlags)
{
    int    nGetDirErr;
    HANDLE nRet;
    TCHAR  szCurDir[PATHMAX];
    TCHAR  szSysDir[PATHMAX];
    LPTSTR lpEnd;

    //
    //  Set the default dir to the System dir if this is not a full path name
    //

    if (lpName[1] != TEXT(':') || lpName[2] != TEXT('\\'))
    {
        nGetDirErr = GetCurrentDirectory (CharSizeOf(szCurDir), szCurDir);

        //
        // Make sure szWinDir has a backslash, and then remove it if it
        // is not the root dir; then make the windows dir the current dir
        // so that OpenFile will not look in strange dirs for the file
        //

        lstrcpy (szSysDir, szSharedDir);
        if ((lpEnd = BackslashTerm (szSysDir)) - szSysDir != 3)
            *(lpEnd-1) = TEXT('\0');
        SetCurrentDirectory (szSysDir);
    }
    else
        nGetDirErr = 0;

    //
    //  Get the file path first
    //

    if (lpPathName)
        OpenFileWithShare (lpName, lpPathName, OF_PARSE);

    //
    //  Now try to open it
    //

    nRet = OpenFileWithShare (lpName, NULL, wFlags);

    //
    // Restore the dir and return
    //

    if (nGetDirErr != 0  &&  nGetDirErr < CharSizeOf(szCurDir))
        SetCurrentDirectory (szCurDir);

    return(nRet);
}


/////////////////////////////////////////////////////////////////////////////
//
// DeleteFontFile
//
//  Sets the current directory to the "Shared" or "Fonts" directory before
//  attempting to Delete an undecorated filename.  Under Windows NT, this is
//  the "%systemroot%\system" directory (e.g. "d:\nt\system").
//
/////////////////////////////////////////////////////////////////////////////

BOOL DeleteFontFile (LPTSTR lpName)
{
    int    nGetDirErr;
    BOOL   bRet;
    TCHAR  szCurDir[PATHMAX];
    TCHAR  szSysDir[PATHMAX];
    LPTSTR lpEnd;

    //
    //  Set the default dir to the System dir if this is not a full path name
    //

    if (lpName[1] != TEXT(':') || lpName[2] != TEXT('\\'))
    {
        nGetDirErr = GetCurrentDirectory (CharSizeOf(szCurDir), szCurDir);

        //
        // Make sure szWinDir has a backslash, and then remove it if it
        // is not the root dir; then make the windows dir the current dir
        // so that OpenFile will not look in strange dirs for the file
        //

        lstrcpy (szSysDir, szSharedDir);
        if ((lpEnd = BackslashTerm (szSysDir)) - szSysDir != 3)
            *(lpEnd-1) = TEXT('\0');
        SetCurrentDirectory (szSysDir);
    }
    else
        nGetDirErr = 0;

    bRet = DeleteFile (lpName);

    //
    // Restore the dir and return
    //

    if (nGetDirErr != 0  &&  nGetDirErr < CharSizeOf(szCurDir))
        SetCurrentDirectory (szCurDir);

    return (bRet);
}

/////////////////////////////////////////////////////////////////////////////
//
// FixupNulls
//
//   Convert hashes into nulls.
//   Replace hash characters in a string with NULLS. '\0's
//
/////////////////////////////////////////////////////////////////////////////

VOID FixupNulls (LPTSTR p)
{
    while (*p)
    {
    if (*p == TEXT('#'))
        *p++ = TEXT('\0');
    else
        p = CharNext (p);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
// OpenFileWithShare
//
//
/////////////////////////////////////////////////////////////////////////////

HANDLE OpenFileWithShare (LPTSTR lpszFile, LPTSTR lpPathName, WORD wFlags)
{
  HANDLE   fh;

    if ((fh = MyOpenFile (lpszFile, lpPathName, wFlags | OF_SHARE_DENY_NONE)) == INVALID_HANDLE_VALUE)
        fh = MyOpenFile (lpszFile, lpPathName, wFlags);

    return (fh);
}


/////////////////////////////////////////////////////////////////////////////
//
// FileLength
//
//  Determine file length, in K.
//
/////////////////////////////////////////////////////////////////////////////

LONG FileLength (LPTSTR pszFile)
{
  LONG     lFileSize;
  HANDLE   fh;

    if ((fh = OpenFileWithShare (pszFile, NULL, OF_READ)) != INVALID_HANDLE_VALUE)
    {
        lFileSize = MyFileSeek (fh, 0L, SEEK_END);
        MyCloseFile (fh);
    }
    else
        lFileSize = 0;

    return (((lFileSize + 1023) / 1024));
}


/////////////////////////////////////////////////////////////////////////////
//
// InitFontDlg
//
//  Initialize font applet main dialog and global variables
//
/////////////////////////////////////////////////////////////////////////////

SHORT InitFontDlg (HWND hDlg)
{
    TCHAR   *pszItem;                       /* pointer into buffer */
    BOOL    bFonts = FALSE;
    TCHAR   szTemp[MAX_PATH], szTemp2[MAX_PATH];
    BOOL    bErrMem = FALSE;
    HANDLE  hLocalBuf;
    TCHAR   *pLocalBuf;
    TCHAR   *pEnd;
    int     nCount, i;
    int     nSize;


    hLBoxInstalled = GetDlgItem(hDlg, LBOX_INSTALLED);

    nCount = GetSection(NULL, szFonts, &hLocalBuf, &nSize);

    if (!hLocalBuf)
    {
        EnableWindow (hDlg = GetParent (hDlg), TRUE);
        ErrMemDlg (hDlg);
        return FALSE;
    }
    pLocalBuf = hLocalBuf;
    pEnd = pLocalBuf + nCount;

    /* fill listbox */
    for (pszItem = pLocalBuf; pszItem < pEnd; pszItem += lstrlen (pszItem)+1)
    {
        if (!*pszItem)
            continue;

        GetProfileString (szFonts, pszItem, szNull, szTemp2, CharSizeOf(szTemp2));
        if (*szTemp2)     /* there's a RHS here */
        {
            bFonts = TRUE;
            i = SendMessage (hLBoxInstalled, LB_ADDSTRING, 0, (LONG)pszItem);

            //
            //  Attach font type to each listed font
            //

            SendMessage (hLBoxInstalled, LB_SETITEMDATA, i, (LONG) IF_OTHER);
        }
    }

    FreeMem (hLocalBuf, nSize);

    if (EnumType1Fonts (hLBoxInstalled))
    {
        bFonts = TRUE;
    }

    if (!bFonts)
    {
        bErrMem = !LoadString (hModule, MYFONT, szTemp, CharSizeOf(szTemp));

        //
        //  for activation after msgbox returns
        //

        if (bErrMem)
        {
            EnableWindow (hDlg = GetParent (hDlg), TRUE);
            ErrMemDlg (hDlg);
        }
        else
            MessageBox (hDlg, szTemp, szCtlPanel, MB_OK | MB_ICONINFORMATION);
        return (bErrMem ? FALSE : -1);     /* ERROR EXIT */
    }

    hLogFonts = 0;
    nCurSel =  -1;      /* Demands initial paint of samples */

    LoadString (hModule, MYFONT + 13, szTemp2, CharSizeOf(szTemp2));

    wsprintf (szTemp, szTemp2, 0L);

    SetDlgItemText (hDlg, FONT_DISKSPACE, szTemp);

    SendMessage (hLBoxInstalled, LB_SETCURSEL, 0, 0L);
    ShowWindow (hDlg, SHOW_OPENWINDOW);
    PostMessage (hDlg, WM_COMMAND, MAKELONG (LBOX_INSTALLED, LBN_SELCHANGE), 0L);

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// TrueTypeDlg
//
//
/////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY TrueTypeDlg (HANDLE hDlg, UINT message, DWORD wParam, LONG lParam)
{
    TCHAR szLHS[120];
    BOOL bTTOnlyChange;

    switch (message)
    {

    case WM_INITDIALOG:

#ifdef ALLOW_NO_TT
        bTrueTypeSave = GetProfileInt (szTrueType, szEnable, TRUE);
        CheckDlgButton (hDlg, FONT_TRUETYPE, bTrueTypeSave);
#else
        bTrueTypeSave = TRUE;
#endif  // ALLOW_NO_TT

        bTrueTypeOnly = GetProfileInt (szTrueType, szOnly, FALSE);
        CheckDlgButton (hDlg, FONT_TRUETYPEONLY, bTrueTypeOnly);
        EnableWindow (GetDlgItem(hDlg, FONT_TRUETYPEONLY), bTrueTypeSave);
        break;

    case WM_COMMAND:

       switch (LOWORD(wParam))
       {
#ifdef ALLOW_NO_TT
       case FONT_TRUETYPE:

            if (!IsDlgButtonChecked (hDlg,FONT_TRUETYPE))
            {
               bTrueTypeOnly = IsDlgButtonChecked (hDlg, FONT_TRUETYPEONLY);
               EnableWindow (GetDlgItem (hDlg, FONT_TRUETYPEONLY), FALSE);
               CheckDlgButton (hDlg, FONT_TRUETYPEONLY, FALSE);
            }
            else
            {
               EnableWindow (GetDlgItem (hDlg, FONT_TRUETYPEONLY),TRUE);
               CheckDlgButton (hDlg, FONT_TRUETYPEONLY, bTrueTypeOnly);
            }
#endif  // ALLOW_NO_TT

      /* Fall through */
      case FONT_TRUETYPEONLY:

            SetDlgItemText (GetParent (hDlg), IDOK, pszClose);
            break;

      case IDOK:

            /* Write TrueType settings to WIN.INI */

            HourGlass (TRUE);

#ifdef ALLOW_NO_TT
            wsprintf (szLHS,TEXT("%d"), IsDlgButtonChecked (hDlg,FONT_TRUETYPE));
            WriteProfileString (szTrueType, szEnable, szLHS);
#endif  // ALLOW_NO_TT

            wsprintf (szLHS,TEXT("%d"), bTrueTypeOnly = IsDlgButtonChecked (hDlg,
                                                     FONT_TRUETYPEONLY));
            bTTOnlyChange =
                    bTrueTypeOnly != (BOOL)GetProfileInt(szTrueType, szOnly,0);
            WriteProfileString (szTrueType, szOnly, szLHS);

            {
                ULONG ul = SetFontEnumeration(0);
                if (bTrueTypeOnly)
                    ul |= FE_FILTER_TRUETYPE;
                else
                    ul &= ~FE_FILTER_TRUETYPE;
                SetFontEnumeration(ul);
            }


#ifdef ALLOW_NO_TT
            if (bTrueTypeSave != (BOOL) IsDlgButtonChecked (hDlg,FONT_TRUETYPE))
            {
                DialogBoxParam(hModule,(LPTSTR) MAKEINTRESOURCE (DLG_RESTART),hDlg,
                       (DLGPROC) RestartDlg, MAKELONG(IDS_TRUETYPECHANGE,0));
            }
#endif  // ALLOW_NO_TT

            if (bTTOnlyChange)
            {
                SendWinIniChange (szTrueType);
                SendWinIniChange (szFonts);
            }
            HourGlass(FALSE);

             /* Fall through */

        case IDCANCEL:

             EndDialog (hDlg, 0L);
             break;

        case IDD_HELP:

             goto DoHelp;

        default:

             return (FALSE);
       }
       return (TRUE);

    default:

        if (message == wHelpMessage || message == wBrowseMessage)
        {
DoHelp:
            CPHelp (hDlg);
            return (TRUE);
        }
        else
            return (FALSE);
    }

    return (FALSE);
}


/////////////////////////////////////////////////////////////////////////////
//
// TTEnabled
//
//    Determine if TT fonts are enabled in GDI.  For Windows NT this
//    routine should always return TRUE.
//
/////////////////////////////////////////////////////////////////////////////

BOOL TTEnabled(void)
{
    RASTERIZER_STATUS info;

    GetRasterizerCaps(&info, sizeof(info));
    return(info.wFlags & TT_ENABLED);
}


/////////////////////////////////////////////////////////////////////////////
//
// FontSelChange
//
//  Change selected font in sample list box and messages for font.
//
/////////////////////////////////////////////////////////////////////////////

void FontSelChange (HWND hDlg)
{
    int      i;
    int      iFontType;
    int      nNumFonts, nFonts;
    TCHAR    szLHS[PATHMAX];
    TCHAR    szFile[PATHMAX];
    TCHAR    szPathName[PATHMAX];
    TCHAR    szPfxFile[PATHMAX];
    HWND     hwndSample, hwndList;
    WORD     wStat;
    LONG     FileSize = 0;
    BOOL     bTrueType;
    DWORD    dwBufSize;


    hwndSample = GetDlgItem(hDlg, FONT_SAMPLE);
    hwndList = GetDlgItem(hDlg, LBOX_INSTALLED);

    SendMessage (hwndSample, WM_SETREDRAW, FALSE, 0L);
    SendMessage (hwndSample, LB_RESETCONTENT, 0, 0L);

    switch (nNumFonts = (int)SendMessage (hwndList, LB_GETSELCOUNT, 0, 0L))
    {
    case 0:
        if (GetFocus() == GetDlgItem (hDlg, DELFONT))
            SendMessage (hDlg, WM_NEXTDLGCTL, 1, 0L);
        wStat = MYFONT+24;
        break;

    case 1:
        SendMessage (hwndList, LB_GETSELITEMS, 1, (LONG)(LPTSTR)&i);
        SendMessage (hwndList, LB_GETTEXT, i, (LONG)szLHS);

        iFontType = SendMessage (hwndList, LB_GETITEMDATA, i, 0L);

        if (iFontType == IF_TYPE1)
        {
            if (GetT1Install (hDlg, szLHS, szFile, szPfxFile))
            {
                // Get size of selected file
                FileSize = FileLength (szPfxFile);
                FileSize += FileLength (szFile);
    
                wStat = MYFONT + 36;
    
                //
                //  Clear "previous" selection variables so we don't try
                //  to do a PassedInspection on a file we never did an
                //  InspectFontFile on.
                //
    
                //
                //  Get rid of the old font
                //
    
                PassedInspection (hLogFonts, szOldFileName);
    
                hLogFonts = NULL;
    
                szOldFileName[0] = TEXT('\0');
    
                goto ShowFontSelection;
            }
            else
            {
                wStat = MYFONT+26;
                break;
            }
        }

        GetProfileString (szFonts, szLHS, szNull, szFile, CharSizeOf(szFile));
        StripBlanks (szFile);

        if (MyOpenFontFile (szFile, szPathName, OF_EXIST) == INVALID_HANDLE_VALUE)
        {
            lstrcat (szFile, TEXT(".fon"));
            if (MyOpenFontFile (szFile, szPathName, OF_EXIST) == INVALID_HANDLE_VALUE)
            {
                wStat = MYFONT+26;
                break;
            }
        }
//        OemToChar (szPathName, szPathName);

        //
        //  Get rid of the old font
        //

        PassedInspection (hLogFonts, szOldFileName);

        //
        //  Get size of selected file
        //

        FileSize = FileLength (szPathName);

        if ((hLogFonts = InspectFontFile (szPathName, &nFonts)))
        {
            dwBufSize = sizeof(BOOL);
            bTrueType = FALSE;

            //
            //  Try to get the name of the TrueType file from the resources
            //  of this file.  If it succeeds, then this is a TrueType
            //  companion file (i.e. a .FOT file) which references a .TTF file
            //

            if (bTrueType = GetTTFilename (szPathName, sizeof(szFile), szFile))
            {
                FileSize += FileLength (szFile);
                wStat = MYFONT+27;

                if (iFontType == IF_TYPE1_TT)
                {
                    GetT1Install (hDlg, szLHS, szFile, szPfxFile);
    
                    // Get size of selected file
                    FileSize += FileLength (szPfxFile);
                    FileSize += FileLength (szFile);
    
                    wStat = MYFONT + 35;
                }
            }

            //  If last call for TrueType determination failed, try it again
            //  but this time call GDI to see if this is a native .TTF file
            //
            //  NOTE:  We can call GetFontResourceInfo() here because we
            //         have already made a call to InspectFontFile() which
            //         has done an AddFontResource() for this font.

            if (!bTrueType)
            {
                if (GetFontResourceInfoW (szPathName, &dwBufSize, &bTrueType, GFRI_ISTRUETYPE))
                {
                    wStat = bTrueType ? MYFONT+27 : MYFONT+28;
                }
                else
                    goto ReadFontFileError;
            }

            if (bTrueType && !TTEnabled())
                wStat = MYFONT+1;
        }
        else
        {
ReadFontFileError:
            // failure
            wStat = MYFONT+26;
        }

        lstrcpy (szOldFileName, szPathName);

        for (i = 0; i < nFonts; i++)
            SendMessage (hwndSample, LB_ADDSTRING, 0, MAKELONG(i, 0));

        break;


    default:
        wStat = MYFONT+25;
    }

ShowFontSelection:

    if (nNumFonts > 1)
    {
        szLHS[0] = 0;
    }
    else
    {
        LoadString (hModule, MYFONT + 13, szFile, CharSizeOf(szFile));
        wsprintf (szLHS, szFile, FileSize);
    }
    SetDlgItemText (hDlg, FONT_DISKSPACE, szLHS);

    LoadString (hModule, wStat, szFile, CharSizeOf(szFile));
    SetDlgItemText (hDlg, FONT_STATUS, szFile);

    //
    //  Only allow deletion of fonts if the "Fonts" registry key
    //  is NOT write-protected
    //

    if (!gfFontsProtected)
        EnableWindow (GetDlgItem (hDlg, DELFONT), nNumFonts>0);

    SendMessage (hwndSample, LB_SETCURSEL, 0, 0L);
    SendMessage (hwndSample, WM_SETREDRAW, TRUE, 0L);
    InvalidateRect (hwndSample, NULL, TRUE);
}


/////////////////////////////////////////////////////////////////////////////
//
// DelSharedFile
//
//   Return FALSE iff the user canceled the operation
//
/////////////////////////////////////////////////////////////////////////////

BOOL DelSharedFile (HWND hDlg, LPTSTR pszFontName, LPTSTR pszFile,
                    LPTSTR lpPathName, BOOL bCheckShared)
{
    LPTSTR   lpLastBS;
    TCHAR    szLocalSharedDir[PATHMAX];
    HANDLE   fh;

    //
    //  Find the system file and make sure we can delete it; if not in the
    //  system dir (and we care), then warn the user (with No as default)
    //  before deleting
    //

    if ((fh = MyOpenFontFile (pszFile, lpPathName, OF_WRITE)) == INVALID_HANDLE_VALUE)
        return(TRUE);
    MyCloseFile (fh);

    if (bCheckShared)
    {
//        CharToOem (szSharedDir, szLocalSharedDir);  replaced with lstrcpy

        lstrcpy (szLocalSharedDir, szSharedDir);

        *(BackslashTerm (szLocalSharedDir) - 1) = TEXT('\0');

        lpLastBS = _tcsrchr (lpPathName, TEXT('\\'));

        if (lpLastBS)
            *lpLastBS = TEXT('\0');

        if (_tcsicmp (szLocalSharedDir, lpPathName))
        {
            switch (MyMessageBox (hDlg, ERRORS+7, INITS+1,
                    MB_ICONEXCLAMATION|MB_YESNOCANCEL|MB_DEFBUTTON2, pszFontName))
            {
               case IDCANCEL:
                  return(FALSE);

               case IDYES:
                  break;

               default:
                  return(TRUE);
            }
        }
        *lpLastBS = TEXT('\\');
    }
    DeleteFontFile (pszFile);

    return(TRUE);
}


/////////////////////////////////////////////////////////////////////////////
//
// CtrlDeleteFont
//
//  Delete selected fonts.
//
/////////////////////////////////////////////////////////////////////////////

VOID CtrlDeleteFont (HWND hDlg)
{
    HWND    hListFonts;
    TCHAR   szLHS[81];
    TCHAR   szFile[PATHMAX];
    TCHAR   szTTFile[PATHMAX];
    TCHAR   szPathname[PATHMAX];
    LPTSTR  pszFileName;
    int    *pLocal;
    int     nNumFonts, i, j;
    int     iFontType;
    BOOL    bYesAll, bIsRemoved = FALSE;
    BOOL    bResult;


    SetDlgItemText (hDlg, IDOK, pszClose);

    hListFonts = GetDlgItem (hDlg, LBOX_INSTALLED);

    nNumFonts = (int)SendMessage (hListFonts, LB_GETSELCOUNT, 0, 0L);

    if (!(pLocal = (int *) LocalAlloc (LMEM_FIXED, nNumFonts * sizeof(int))))
        return;

    SendMessage (hListFonts, LB_GETSELITEMS, nNumFonts, (LONG)(LPTSTR)pLocal);

    SendDlgItemMessage (hDlg, FONT_SAMPLE, LB_RESETCONTENT, 0, 0L);

    PassedInspection (hLogFonts, szOldFileName);

    hLogFonts = 0;

    HourGlass(TRUE);


    for (i = 0, bYesAll = bDeleteFont = FALSE; i < nNumFonts; ++i)
    {
        SendMessage (hListFonts, LB_GETTEXT, pLocal[i], (LONG)(LPTSTR)szLHS);

        iFontType = SendMessage (hListFonts, LB_GETITEMDATA, pLocal[i], 0L);

        //
        //  Ask if we should remove this font (and delete it) unless the
        //  user has said yes to all.
        //

        if (!bYesAll)
        {
            HourGlass (FALSE);

            switch (DoDialogBoxParam (DLG_REMOVEFONT, hDlg, (DLGPROC)RemoveFontDlg,
                                      IDH_DLG_REMOVEFONT, (DWORD)szLHS))
            {
            case IDNO:
                  //
                  //  Note that we do not do HourGlass (TRUE), but that
                  //  should be harmless, since we are either going to come
                  //  right back here, or we are going to exit
                  //
                  continue;

            case IDD_YESALL:
                  bYesAll = TRUE;
            case IDYES:
                  break;

            default:
                  //
                  //  CANCEL and NOMEM (user already warned)
                  //
                  goto NoMoreFonts;
            }

            HourGlass(TRUE);
        }

        // yes, delete it

        if ((iFontType == IF_TYPE1) || (iFontType == IF_TYPE1_TT))
        {
            if (!DeleteT1Install (hDlg, szLHS, bDeleteFont))
            {
                //
                //  ERROR! Could not delete registry entry or files, message
                //  displayed in routine.
                //

                goto NoMoreFonts;
            }

            if (iFontType == IF_TYPE1)
            {
                //
                //  Remove from the listbox
                //
                //  We just want to make sure the listbox doesn't update
                //  until a  WM_PAINT message, so we don't get lots of
                //  flashes on a Yes to All
                //

                SendMessage (hListFonts, WM_SETREDRAW, FALSE, 0L);
                SendMessage (hListFonts, LB_DELETESTRING, pLocal[i], 0L);
                SendMessage (hListFonts, WM_SETREDRAW, TRUE, 0L);
                InvalidateRect (hListFonts, NULL, TRUE);

                for (j = i + 1; j < nNumFonts; ++j)
                    if (pLocal[j] > pLocal[i])
                        --pLocal[j];
        
                goto ContinueFontRemoval;
            }

            //
            //  else continue with removal of TrueType font
            //
        }


        GetProfileString (szFonts, szLHS, szNull, szFile, CharSizeOf(szFile));
        StripBlanks (szFile);

        // remove from win.ini

        WriteProfileString (szFonts, szLHS, 0L);

        // and the listbox
        //
        //  We just want to make sure the listbox doesn't update until a
        //  WM_PAINT message, so we don't get lots of flashes on a Yes to All
        //

        SendMessage (hListFonts, WM_SETREDRAW, FALSE, 0L);
        SendMessage (hListFonts, LB_DELETESTRING, pLocal[i], 0L);
        SendMessage (hListFonts, WM_SETREDRAW, TRUE, 0L);
        InvalidateRect (hListFonts, NULL, TRUE);

        for (j = i + 1; j < nNumFonts; ++j)
            if (pLocal[j] > pLocal[i])
                --pLocal[j];

        //
        //  Remove the font as listed
        //

        RemoveFontResource (szFile);

        //
        //  Check to see if the font was not really removed from the system.
        //

        if (pszFileName = _tcsrchr (szFile, TEXT('\\')))
            ++pszFileName;
        else
            pszFileName = szFile;

        //
        //  Check to see if the font was really removed from the internal GDI
        //  font database.  This lets me know if I can safely try to delete
        //  the file.  NOTE:  It will not be removed if the font is in use by
        //  another application.
        //

        j = sizeof(BOOL);

        bResult = GetFontResourceInfoW (szFile, &j, &bIsRemoved, GFRI_ISREMOVED);

        if (bResult && bIsRemoved)
        {
            //
            //  Try to get the name of the TrueType file from the resources
            //  of this file.  If it succeeds, then this is a TrueType
            //  companion file (i.e. a .FOT file) which references a .TTF file
            //

            if (GetTTFilename (szFile, sizeof(szTTFile), szTTFile))
            {
                DelSharedFile (hDlg, szLHS, szFile, szPathname, FALSE);
                lstrcpy (szFile, szTTFile);
            }
            else
                j = GetLastError ();
               
            //
            //  Now delete the font file(s) from disk if necessary
            //

            if (bDeleteFont)
                if (!DelSharedFile (hDlg, szLHS, szFile, szPathname, TRUE))
                      goto NoMoreFonts;
        }

#ifdef JAPAN // support Large TT font installation and Bitmap font support for small size TT

// If user push cancel button during this loop , we exit this without remove bitmap font
//  In case of DelSharedFile retun FALSE

    DeleteBitmapFont( hDlg , szFile , bDeleteFont );

#endif // JAPAN

ContinueFontRemoval:
        ;

    }
NoMoreFonts:

    HourGlass (FALSE);

    LocalFree ((HANDLE)pLocal);

    FontSelChange (hDlg);
    InvalidateRect (GetDlgItem (hDlg, LBOX_INSTALLED), NULL, TRUE);
    SendWinIniChange (szFonts);
}


/////////////////////////////////////////////////////////////////////////////
//
// FontDlg
//
//  Font applet main dialog procedure.
//
/////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY FontDlg (HWND hDlg, UINT message, DWORD wParam, LONG lParam)
{
    int temp;
    LPLOGFONT lpLogFont;
    HFONT hFont, hFontTemp;
    HDC hDC;
    TEXTMETRIC TM;
    HWND hWndTmp;
    LONG lRet;
    HKEY hkey;

    switch (message)
    {
    case WM_DROPFILES:
        FontsDropped(hDlg, (HANDLE) wParam);
        break;

    case WM_INITDIALOG:
        HourGlass(TRUE);

        DragAcceptFiles(hDlg, TRUE);

        //
        //  Save szSharedDir and set it locally based on security
        //

        lstrcpy (szSaveSharedDir, szSharedDir);

#ifdef SECURE
        /////////////////////////////////////////////////////////////////////
        //
        //  Get info on fonts key in registry - try to open it for
        //  writing.  Enable or Disable "Add" and  "Remove" buttons
        //  based on key privileges.
        //
        /////////////////////////////////////////////////////////////////////

        gfFontsProtected = FALSE;

        lRet = RegOpenKeyEx (HKEY_LOCAL_MACHINE,    // Root key
                             szFontsKey,            // Subkey to open
                             0L,                    // Reserved
                             KEY_READ | KEY_WRITE,  // SAM
                             &hkey);                // return handle

        if (lRet != ERROR_SUCCESS)
        {
            //
            //  "Fonts" key is write protected.
            //  Grey out "Add Fonts" and "Remove Fonts" buttons
            //

            EnableWindow (GetDlgItem (hDlg, ADDFONT), FALSE);
            EnableWindow (GetDlgItem (hDlg, DELFONT), FALSE);

            gfFontsProtected = TRUE;
        }
        else
        {
            //
            //  "Fonts" registry key is open
            //

            RegCloseKey (hkey);
        }

        //
        //  Setup the destination directory to point to where all
        //  fonts reside.  This is different in a "secure" system.
        //

        lstrcpy (szSharedDir, pszWinDir);
        BackslashTerm (szSharedDir);

        lRet = RegOpenKeyEx (HKEY_LOCAL_MACHINE,    // Root key
                             FONTPATHKEY,           // Subkey to open
                             0L,                    // Reserved
                             KEY_READ,              // SAM
                             &hkey);                // return handle

        if (lRet == ERROR_SUCCESS)
        {
            //
            //  The FontPath key exists; hence, this is a secure system.
            //  The "Fonts" directory %windir%\fonts is the default
            //  directory for installation.
            //

            RegCloseKey (hkey);

            lstrcat (szSharedDir, TEXT("fonts\\"));
        }
        else
        {
            //
            //  This is not a secure system, so the "Fonts" directory
            //  %windir%\system is the default directory for installation.
            //

            lstrcat (szSharedDir, TEXT("system\\"));
        }

        //
        //  Make an ANSI version of szSharedDir for t1instal call
        //

        if (WideCharToMultiByte (CP_ACP, 0, szSharedDir, -1,
                        szFontsDirA, PATHMAX, NULL, NULL) == FALSE)
        {
            //
            //  Report error adding font resource
            //

            MyMessageBox (hDlg, MYFONT+49, MYFONT+48,
                          MB_OK | MB_ICONEXCLAMATION,
                          (LPTSTR)szSharedDir);
            return (FALSE);
        }

#else  // SECURE

        //
        //  Now set it to Windows dir\System since all fonts now reside there
        //

        lstrcpy (szSharedDir, pszWinDir);
        BackslashTerm (szSharedDir);
        lstrcat (szSharedDir, TEXT("system\\"));

#endif  // SECURE

        temp = InitFontDlg (hDlg);

        HourGlass (FALSE);

        if (temp == -1 || hSetup)
        {
            PostMessage (hDlg, WM_COMMAND, ADDFONT, -1L);
        }

        //
        //  This is to draw all controls before the sample control
        //

        for (hWndTmp = GetWindow (hDlg, GW_CHILD); hWndTmp;
                            hWndTmp = GetWindow (hWndTmp, GW_HWNDNEXT))
            if (GetWindowLong (hWndTmp, GWL_ID) != FONT_SAMPLE)
                UpdateWindow (hWndTmp);
        break;

    case WM_ENTERIDLE:
        FilesToDescs();
        return FALSE;
        break;

    case WM_DRAWITEM:
#define lpdis ((LPDIS)lParam)
        if (lpdis->itemAction == ODA_DRAWENTIRE)
        {
            HDC hdc = lpdis->hDC;

            HourGlass(TRUE);

            lpLogFont = (LPLOGFONT) GlobalLock(hLogFonts);
            lpLogFont += lpdis->itemID;
            hFont = CreateFontIndirect(lpLogFont);

            if (hFont)
                hFontTemp = SelectObject(hdc, hFont);

            GetTextMetrics(hdc, &TM);

#ifdef DBCS
        // Load sample string for each character set.
            {
                switch (lpLogFont->lfCharSet) {
                case ANSI_CHARSET :
                case SYMBOL_CHARSET :
                    LoadString(
                        hModule,
                        ANSI_FONTSAMPLE,
                        szTest,
                        CharSizeOf(szTest)
                    );
                    break;
                case SHIFTJIS_CHARSET :
                    LoadString(
                        hModule,
                        SJ_FONTSAMPLE,
                        szTest,
                        CharSizeOf(szTest)
                    );
                    break;
                case HANGEUL_CHARSET :
                    LoadString(
                        hModule,
                        HANGEUL_FONTSAMPLE,
                        szTest,
                        CharSizeOf(szTest)
                    );
                    break;
                case OEM_CHARSET :
                    LoadString(
                        hModule,
                        OEM_FONTSAMPLE,
                        szTest,
                        CharSizeOf(szTest)
                    );
                    break;
                default :
                    LoadString(
                        hModule,
                        ANSI_FONTSAMPLE,
                        szTest,
                        CharSizeOf(szTest)
                    );
                    break;
                }
            }
#else
            // create sample string

            // pt size = (height) * 72 / pix_per_inch

            lstrcpy(szTest, TEXT("AaBbCcXxYyZz 123"));
#endif /* end of else-ifdef JAPAN */

            ExtTextOut(hdc, lpdis->rcItem.left,
                   lpdis->rcItem.top, ETO_OPAQUE,
                   &(lpdis->rcItem), szTest, lstrlen(szTest), NULL);

            if (hFont)
            {
                SelectObject(hdc, hFontTemp);
                DeleteObject(hFont);
            }
            GlobalUnlock(hLogFonts);

            HourGlass(FALSE);
        }
        break;

    case WM_MEASUREITEM:
        lpLogFont = (LPLOGFONT)GlobalLock(hLogFonts);
        lpLogFont += ((LPMIS)lParam)->itemID;

        hDC = GetDC(NULL);
        hFont = CreateFontIndirect(lpLogFont);
        if (hFont)
        {
            hFontTemp = SelectObject(hDC, hFont);
            GetTextMetrics(hDC, &TM);
            ((LPMIS)lParam)->itemHeight = TM.tmHeight;
        }
        else
            ((LPMIS)lParam)->itemHeight = 1;

        if (hFont)
        {
            SelectObject(hDC, hFontTemp);
            DeleteObject(hFont);
        }
        ReleaseDC(NULL, hDC);
        GlobalUnlock(hLogFonts);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDD_HELP:
            goto DoHelp;

        case LBOX_INSTALLED:
            if (HIWORD(wParam) == LBN_SELCHANGE)
            {
                HourGlass (TRUE);
                FontSelChange (hDlg);
                HourGlass (FALSE);
            }
            return(FALSE);
            break;

        case DELFONT:
            CtrlDeleteFont(hDlg);
            break;

        case ADDFONT:
            DoAddFont(hDlg);
            break;

        case FONT_TRUETYPE:
            DoDialogBoxParam (DLG_TRUETYPE, hDlg, (DLGPROC) TrueTypeDlg,
                              IDH_DLG_TRUETYPE, 0L);
            break;

        case PUSH_OK:
        case PUSH_CANCEL:
            PassedInspection(hLogFonts, szOldFileName);

            //  Restore szSharedDir to original entry value
            lstrcpy (szSharedDir, szSaveSharedDir);

            EndDialog(hDlg, 0L);
            break;
        }
        break;

    default:
        if (message == wHelpMessage || message == wBrowseMessage)
        {
DoHelp:
            CPHelp(hDlg);
            return TRUE;
        }
        else
            return(FALSE);
        break;
    }
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
// RemoveFontDlg
//
//  The following function verifies that the user wishes to delete a font
//  resource.  It also allows the user to have the font file deleted from
//  the disk.
//
/////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY RemoveFontDlg (HWND hDlg, UINT nMsg, DWORD wParam, LONG lParam)
{
    TCHAR szFormat[120];
    TCHAR szTemp[120];

    switch (nMsg)
    {

    case WM_INITDIALOG:

        //
        //  Set Dialog message text
        //

        LoadString (hModule, MYFONT + 18, szFormat, CharSizeOf(szFormat));
        wsprintf (szTemp, szFormat, (LPTSTR)lParam);
        SetDlgItemText (hDlg, FONT_REMOVEMSG, szTemp);
        EnableWindow (hDlg, TRUE);

        CheckDlgButton(hDlg, FONT_REMOVECHECK, bDeleteFont);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {

        case IDD_HELP:
            goto DoHelp;

        case IDYES:
        case IDD_YESALL:
            bDeleteFont = IsDlgButtonChecked (hDlg, FONT_REMOVECHECK);
        case IDNO:
        case IDCANCEL:
            EndDialog (hDlg, LOWORD(wParam));
            break;

        default:
            return FALSE;
        }
        break;

    default:
        if (nMsg == wHelpMessage)
        {
DoHelp:
            CPHelp (hDlg);
            return TRUE;
        }
        else
            return FALSE;
    }
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
// RemovePSFontDlg
//
//  The following function verifies that the user wishes to delete a font
//  resource and the companion PostScript font files.  It also allows the
//  user to have the font files deleted from the disk.
//
/////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY RemovePSFontDlg (HWND hDlg, UINT nMsg, DWORD wParam, LONG lParam)
{
    TCHAR szFormat[120];
    TCHAR szTemp[120];

    switch (nMsg)
    {

    case WM_INITDIALOG:

        //
        //  Set Dialog message text
        //

        LoadString (hModule, MYFONT + 18, szFormat, CharSizeOf(szFormat));
        wsprintf (szTemp, szFormat, (LPTSTR)lParam);
        SetDlgItemText (hDlg, FONT_REMOVEMSG, szTemp);
        EnableWindow (hDlg, TRUE);

        CheckDlgButton(hDlg, FONT_REMOVECHECK, bDeleteFont);
        CheckDlgButton(hDlg, FONT_REMOVE_PS, bDeletePS);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {

        case IDD_HELP:
            goto DoHelp;

        case IDYES:
        case IDD_YESALL:
            bDeleteFont = IsDlgButtonChecked (hDlg, FONT_REMOVECHECK);
            bDeletePS = IsDlgButtonChecked (hDlg, FONT_REMOVE_PS);
        case IDNO:
        case IDCANCEL:
            EndDialog (hDlg, LOWORD(wParam));
            break;

        default:
            return FALSE;
        }
        break;

    default:
        if (nMsg == wHelpMessage)
        {
DoHelp:
            CPHelp (hDlg);
            return TRUE;
        }
        else
            return FALSE;
    }
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// DoAddFont
//
//  Call the Common Dialogs OpenFile routine with our dialog template to
//  manage directories and controls for finding fonts.  The HOOK Proc for
//  this routine is located in font3.c.
//
/////////////////////////////////////////////////////////////////////////////

void DoAddFont(HWND hDlg)
{
    TCHAR szFilter[80];
    TCHAR szFileTemp[PATHMAX];
    OPENFILENAME OpenFileName;
    DWORD dwSave;
    static LPTSTR pFontDir = NULL;

    if (!pFontDir)
    {
        pFontDir = (LPTSTR) LocalAlloc (LPTR, ByteCountOf(PATHMAX + 1));
        GetWindowsDirectory (pFontDir, PATHMAX);
    }

    // If TT enabled load one filter, else load the other.  The default
    // is for TT to be ON in WIN3.1.

    if (GetProfileInt (szTrueType,szEnable,1))
        LoadString (hModule, IDS_ALLFONTSFILTER, szFilter, CharSizeOf(szFilter));
    else
        LoadString (hModule, IDS_NORMALFONTSFILTER, szFilter, CharSizeOf(szFilter));
    FixupNulls (szFilter);

    *szFileTemp = TEXT('\0');

    // Save context.
    dwSave = dwContext;
    dwContext = IDH_DLG_FONT2;

    OpenFileName.lStructSize = sizeof(OPENFILENAME);
    OpenFileName.hwndOwner = hDlg;
    OpenFileName.hInstance = hModule;
    OpenFileName.lpstrFilter = szFilter;
    OpenFileName.lpstrCustomFilter = NULL;
    OpenFileName.nMaxCustFilter = 0;
    OpenFileName.nFilterIndex = 1;
    OpenFileName.lpstrFile = szFileTemp;
    OpenFileName.nMaxFile = CharSizeOf(szFileTemp);
    OpenFileName.lpstrInitialDir = pFontDir ? (LPTSTR)pFontDir : NULL;
    OpenFileName.lpstrTitle = NULL;    // szCaption;
    OpenFileName.Flags = OFN_HIDEREADONLY | OFN_ENABLEHOOK | OFN_SHOWHELP |
                         OFN_ENABLETEMPLATE;
//    OpenFileName.lCustData = MAKELONG(hDlg, 0);
    OpenFileName.lCustData = (LONG) hDlg;
    OpenFileName.lpfnHook = FontHookProc;
    OpenFileName.lpTemplateName = (LPTSTR) MAKEINTRESOURCE(DLG_FONT2);
    OpenFileName.nFileOffset = 0;
    OpenFileName.nFileExtension = 0;
    OpenFileName.lpstrDefExt = NULL;
    OpenFileName.lpstrFileTitle = NULL;

    GetOpenFileName(&OpenFileName);
    dwContext = dwSave;

    // save the current dir so we can restore this
    if (pFontDir)
        GetCurrentDirectory (PATHMAX, pFontDir);

    // set the current dir back to windows so we don't hit the floppy
    GetSystemDirectory(szFileTemp, CharSizeOf(szFileTemp));
    SetCurrentDirectory(szFileTemp);
}

/////////////////////////////////////////////////////////////////////////////
//
// GetTTFilename
//
//  This function returns the name of the TrueType file associated with
//  the filename passed into the function.  The filename is return via
//  pszTTFilename parameter.  The Function returns TRUE if there was a
//  filename associated with it, FALSE otherwise.
//
/////////////////////////////////////////////////////////////////////////////

BOOL GetTTFilename (LPTSTR pszFile, int cb, LPTSTR pszTTFile)
{
    return GetFontResourceInfoW (pszFile, &cb, pszTTFile, GFRI_TTFILENAME);
}

