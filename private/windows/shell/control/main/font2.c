/** FILE: font2.c ********** Module Header ********************************
 *
 *  Control panel applet for Font configuration.  This file holds code for
 *  the items concerning fonts.
 *
 * History:
 *  12:30 on Tues  23 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *           Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
 *           Updated code to latest Win 3.1 sources
 *  04 April 1994  -by-  Steve Cathcart   [stevecat]
 *            Added support for PostScript Type 1 fonts
 *
 *  Copyright (C) 1990-1994 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                         Include files
//==========================================================================
// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Application specific
#include "main.h"

#undef IN
#include "t1instal.h"

#undef COLOR
#define CONST const
//typedef WCHAR *PWCHAR;

// Windows SDK - private
#include <wingdip.h>       // For private GDI entry point: GetFontResourceInfo()
//==========================================================================
//                        Local Definitions
//==========================================================================

#define GWL_PROGRESS    0
#define SET_PROGRESS    WM_USER

//  Progress Control color indices
#define PROGRESSCOLOR_FACE        0
#define PROGRESSCOLOR_ARROW       1
#define PROGRESSCOLOR_SHADOW      2
#define PROGRESSCOLOR_HIGHLIGHT   3
#define PROGRESSCOLOR_FRAME       4
#define PROGRESSCOLOR_WINDOW      5

#define CCOLORS                  6

#define CHAR_BACKSLASH  TEXT('\\')
#define CHAR_COLON      TEXT(':')
#define CHAR_NULL       TEXT('\0')
#define CHAR_TRUE       TEXT('T')
#define CHAR_FALSE      TEXT('F')

//==========================================================================
//                       External Declarations
//==========================================================================

extern TCHAR szTTF[];
extern TCHAR szFON[];
extern TCHAR szPFM[];
extern TCHAR szPFB[];
extern TCHAR szPostScript[];
extern HWND  hLBoxInstalled;

//==========================================================================
//                       Local Data Declarations
//==========================================================================

BOOL bYesAll_PS = FALSE;        //  Use global state for all PS fonts
BOOL bConvertPS = TRUE;         //  Convert Type1 files to TT
BOOL bInstallPS = TRUE;         //  Install PS files
BOOL bCopyPS    = TRUE;         //  Copy PS files to Windows dir

BOOL bCancelInstall = FALSE;    // Global installation cancel

TCHAR szTrue[]  = TEXT("T");
TCHAR szFalse[] = TEXT("F");
TCHAR szHash[]  = TEXT("#");

BOOL bProgMsgDisplayed;         //  Used by Progress to avoid msg flicker
BOOL bProg2MsgDisplayed;        //  Used by Progress2 to avoid msg flicker

HWND hDlgProgress = NULL;

//
//  Used to determine Foreground/Backgnd colors for progress bar control
//  Global values are set at RegisterClass time
//

DWORD   rgbFG;
DWORD   rgbBG;

// Registry location for installing PostScript printer font info

TCHAR szType1Key[] = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Type 1 Installer\\Type 1 Fonts");

//Array of default colors, matching the order of PROGRESSCOLOR_* values.
DWORD rgColorPro[CCOLORS]={
                         COLOR_BTNFACE,             //  PROGRESSCOLOR_FACE
                         COLOR_BTNTEXT,             //  PROGRESSCOLOR_ARROW
                         COLOR_BTNSHADOW,           //  PROGRESSCOLOR_SHADOW
                         COLOR_BTNHIGHLIGHT,        //  PROGRESSCOLOR_HIGHLIGHT
                         COLOR_WINDOWFRAME,         //  PROGRESSCOLOR_FRAME
                         COLOR_WINDOW               //  PROGRESSCOLOR_WINDOW
                         };

typedef struct _T1_INSTALL_OPTIONS
{
    BOOL        bMatchingTT;
    BOOL        bOnlyPSInstalled;
    int         iFontType;
    LPTSTR      szDesc;
} T1_INSTALL_OPTIONS, *PT1_INSTALL_OPTIONS;

//
//  Linked-list structure used for copyright Vendors
//

typedef struct _psvendor
{
    struct _psvendor *pNext;
    LPTSTR pszCopyright;            //  Copyright string
    int    iResponse;               //  User's response to YES/NO MsgBox
} PSVENDOR;

PSVENDOR *pFirstVendor = NULL;      //  ptr to linked list for PS vendors

//==========================================================================
//                      Local Function Prototypes
//==========================================================================

BOOL CheckT1Install (LPTSTR pszDesc, LPTSTR pszData);
BOOL CheckTTInstall (LPTSTR szDesc);
void Draw3DRect (HDC hDC, HBRUSH hBrushFace, HPEN hPenFrame, HPEN hPenHigh,
                 HPEN hPenShadow, int x1, int y1, int x2, int y2);
BOOL APIENTRY InstallPSDlg (HWND hDlg, UINT nMsg, DWORD wParam, LONG lParam);
void Progress (short PercentDone, void* UniqueValue);
LRESULT APIENTRY ProgressBarCtlProc(HWND hTest, UINT message, WPARAM wParam, LONG lParam);
BOOL APIENTRY ProgressDlg (HWND hDlg, UINT nMsg, DWORD wParam, LONG lParam);
LONG ProgressPaint (HWND hWnd, DWORD dwProgress);

BOOL WriteType1RegistryEntry (HWND hDlg, LPTSTR szDesc, LPTSTR szPfmName, LPTSTR szPfbName);

//==========================================================================
//                           Functions
//==========================================================================

/////////////////////////////////////////////////////////////////////////////
//
// InspectFontFile
//
//  This routine is called to set up all fonts from the FON file passed.
//  The handle returned is a handle to global memory containing LOGFONT
//  structures, the number of structures contained is returned in the
//  integer pointed to by the second parameter.
//
//  in:
//       szFontFile font file name
//  out:
//       pNumFonts
//
//  NOTE: there must be matching pairs of InspectFontFile () and
//        PassedInspection () on each font so module counts don't
//        get out of sync.
//
/////////////////////////////////////////////////////////////////////////////

HANDLE InspectFontFile (LPTSTR szFontFile, int *pNumFonts)
{
  HANDLE   hlpLOGFONT;
  int      nFonts;
  DWORD    dwBufSize;
  LPTSTR   lpFontBuffer;
  BOOL     bStatus;


    nFonts = 0;

    if (MyOpenFile (szFontFile, NULL, OF_EXIST) != INVALID_HANDLE_VALUE)
    {
        if (nFonts = AddFontResource (szFontFile))
        {
            dwBufSize = (DWORD) (nFonts * sizeof (LOGFONT));
            if (hlpLOGFONT = GlobalAlloc (GMEM_MOVEABLE, dwBufSize))
            {
                lpFontBuffer = GlobalLock(hlpLOGFONT);

                bStatus = GetFontResourceInfoW (szFontFile,
                                                &dwBufSize,
                                                lpFontBuffer,
                                                GFRI_LOGFONTS);

                GlobalUnlock(hlpLOGFONT);
                if (bStatus == 0)
                {
                    GlobalFree (hlpLOGFONT);
                    nFonts = 0;
                    // DbgPrint ("CPanel.Fonts::InspectFontFile - GetFontResourceInfo(LOGFONTS) failed!\n");
                }
            }
        }
    }
    return ((*pNumFonts = nFonts) ? hlpLOGFONT : 0);
}


/////////////////////////////////////////////////////////////////////////////
//
// PassedInspection
//
//  This function takes a handle to memory containing the information
//  obtained through a call to InspectFontFile, as well as the filename
//  where the information was retrieved.  If the handle is null,
//  InspectFontFile did not increase the reference count and as such the
//  count should not be decreased.
//
/////////////////////////////////////////////////////////////////////////////

HANDLE PassedInspection (HANDLE hLogicalFont, LPTSTR szFileName)
{
    if (hLogicalFont)
    {
        GlobalFree (hLogicalFont);
        RemoveFontResource (szFileName);
    }
    return (hLogicalFont);
}

/////////////////////////////////////////////////////////////////////////////
//
// AddBackslash
//
//  Add a Backslash character to the end of a path, if necessary.
//
/////////////////////////////////////////////////////////////////////////////

void AddBackslash (LPTSTR pszFile)
{
    LPTSTR psz;

    if (*CharPrev(pszFile, psz = pszFile + lstrlen (pszFile)) != TEXT('\\'))
    {
        *psz     = TEXT('\\');
        *(psz+1) = TEXT('\0');
    }
}


/////////////////////////////////////////////////////////////////////////////
//
// StripFilespec
//
//   Remove the filespec portion from a path (including the backslash).
//
/////////////////////////////////////////////////////////////////////////////

VOID StripFilespec (LPTSTR lpszPath)
{
   LPTSTR     p;

   p = lpszPath + lstrlen(lpszPath);

   while ((*p != CHAR_BACKSLASH) && (*p != CHAR_COLON) && (p != lpszPath))
      p--;

   if (*p == CHAR_COLON)
      p++;

   //
   // Don't strip backslash from root directory entry.
   //

   if (p != lpszPath) {
      if ((*p == CHAR_BACKSLASH) && (*(p-1) == CHAR_COLON))
         p++;
   }

   *p = CHAR_NULL;
}


/////////////////////////////////////////////////////////////////////////////
//
// StripPath
//
//   Extract only the filespec portion from a path.
//
/////////////////////////////////////////////////////////////////////////////

VOID StripPath (LPTSTR lpszPath)
{
  LPTSTR     p;

  p = lpszPath + lstrlen(lpszPath);

  while ((*p != CHAR_BACKSLASH) && (*p != CHAR_COLON) && (p != lpszPath))
      p--;

  if (p != lpszPath)
      p++;

  if (p != lpszPath)
      lstrcpy(lpszPath, p);
}


/////////////////////////////////////////////////////////////////////////////
//
// AddVendorCopyright
//
//  Add a PostScript Vendor's Copyright and User response to "MAYBE" list. 
//  This linked-list is used to keep track of a user's prior response to
//  message about converting this vendor's fonts to TrueType.  If a vendor
//  is not in the registry, we cannot automatically assume that the font
//  can be converted.  We must present the User with a message, asking them
//  to get permission from the vendor before converting the font to TrueType.
//
//  However, we do allow them to continue the installation and convert the
//  font to TrueType by selecting the YES button on the Message box.  This
//  routine keeps track of each vendor and the User's response for that
//  vendor.  This way we do not continually ask them about the same vendor
//  during installation of a large number of fonts.
//
//  (Insert item into linked list)
//
//
/////////////////////////////////////////////////////////////////////////////

BOOL AddVendorCopyright (LPTSTR pszCopyright, int iResponse)
{
    PSVENDOR *pVendor;          //  temp pointer to linked list


    //
    //  Make the new PSVENDOR node and add it to the linked list.
    //

    if (pFirstVendor)
    {
        pVendor = (PSVENDOR *) AllocMem (sizeof(PSVENDOR));

        if (pVendor)
        {
            pVendor->pNext = pFirstVendor;
            pFirstVendor = pVendor;
        }
        else
            return FALSE;
    }
    else        // First time thru
    {
        pFirstVendor = (PSVENDOR *) AllocMem (sizeof(PSVENDOR));

        if (pFirstVendor)
            pFirstVendor->pNext = NULL;
        else 
            return FALSE;
    }

    //
    //  Save User response and Copyright string
    //

    pFirstVendor->iResponse = iResponse;

    pFirstVendor->pszCopyright = AllocStr (pszCopyright);

    //
    //  Return success.
    //

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// CheckVendorCopyright
//
//  Check if a Vendor Copyright is already in the "MAYBE" linked-list and
//  and return User response if it is found.
//
//  Returns:
//          IDYES  - User wants to convert typeface anyway
//          IDNO   - User does not want to convert typeface
//          -1     - Entry not found
//
/////////////////////////////////////////////////////////////////////////////

int CheckVendorCopyright (LPTSTR pszCopyright)
{
    PSVENDOR *pVendor;          // temp pointer to linked list

    //
    //  Traverse the list, testing each node for matching copyright string
    //

    pVendor = pFirstVendor;

    while (pVendor)
    {
        if (!lstrcmpi (pVendor->pszCopyright, pszCopyright))
            return (pVendor->iResponse);

        pVendor = pVendor->pNext;
    }

    //
    //  "Did not find matching copyright" return
    //

    return (-1);
}


/////////////////////////////////////////////////////////////////////////////
//
// IsPSFont
//
//  Check validity of font file passed in and get paths to .pfm/.pfb
//  files, determine if it can be converted to TT.
//
// in:
//    lpszPfm        .pfm file name to validate
// out:
//    lpszDesc       on succes Font Name of Type1
//    lpszPfm        on succes the path to .pfm file
//    lpszPfb        on succes the path to .pfb file
//    pbCreatedPFM   on succes whether a PFM file was created or not
//    lpiFontType    set to a value based on Font type 1 == TT, 2 == Type1
//
// NOTE: Assumes that lpszPfm and lpszPfb are of size PATHMAX & lpszDesc is
//       of size DESCMAX
//
// returns:
//    TRUE success, FALSE failure
//
/////////////////////////////////////////////////////////////////////////////

BOOL IsPSFont (HWND   hDlg,             //  if NULL, HWND_DESKTOP used
               LPTSTR lpszKey,
               LPTSTR lpszDesc,         //  Optional
               LPTSTR lpszPfm,          //  Optional
               LPTSTR lpszPfb,          //  Optional
               BOOL  *pbCreatedPFM,     //  Optional
               int   *lpiFontType)
{
    BOOL    bRet = FALSE;
    TCHAR   strbuf[PATHMAX];
    TCHAR   szCopyright[PATHMAX];
    BOOL    bPFM;
    int     iResponse;
    HWND    hwndParent;

    //
    //  ANSI buffers for use with ANSI only API's
    //

    char    *desc, Descbuf[PATHMAX];
    char    Keybuf[PATHMAX];
    char    *pfb, Pfbbuf[PATHMAX];
    char    *pfm, Pfmbuf[PATHMAX];
    char    Vendorbuf[PATHMAX];
    DWORD   iDesc, iPfb, iPfm;


    if (!lpiFontType)
        return bRet;

    if (lpszDesc)
        *lpszDesc = (TCHAR) 0;

    desc = Descbuf;
    iDesc = PATHMAX;

    pfb = Pfbbuf;
    iPfb = PATHMAX;

    if (lpszPfm)
    {
        pfm = Pfmbuf;
        iPfm = PATHMAX;
    }
    else
    {
        pfm = NULL;
        iPfm = 0;
    }

    if (pbCreatedPFM)
        *pbCreatedPFM = FALSE;

    *lpiFontType = NOT_TT_OR_T1;

    WideCharToMultiByte (CP_ACP, 0, lpszKey, -1, Keybuf, PATHMAX, NULL, NULL);

    //
    //  The CheckType1A routine accepts either a .INF or .PFM file name as
    //  the Keybuf (i.e. Key file)  input parameter.  If the input is a .INF
    //  file, a .PFM file will be created in the SYSTEM directory if (and
    //  only if) the .PFM file name parameter is non-NULL.  Otherwise, it
    //  will just check to see if a valid .INF, .AFM and .PFB file exist for
    //  the font.
    //
    //  The bPFM BOOL value is an output parameter that tells me if the routine
    //  created a .PFM file from the .INF/.AFM file for this font.  If the
    //  pfm input parameter is non-NULL, it will always receive the proper
    //  path for the .PFM file.
    //  

    if (CheckType1A (Keybuf, iDesc, desc, iPfm, pfm, iPfb, pfb, &bPFM, szFontsDirA))
    {
        if (pbCreatedPFM)
            *pbCreatedPFM = bPFM;
        //
        //  Check convertability of this font from Type1 to TrueType.
        //
        //  Returns: SUCCESS, FAILURE, MAYBE
        //

        switch (CheckCopyrightA (Pfbbuf, PATHMAX, Vendorbuf))
        {
        case FAILURE:
            *lpiFontType = TYPE1_FONT_NC;

            //
            //  Put up a message box stating that this Type1 font vendor
            //  does not allow us to Convert their fonts to TT.  This will
            //  let the user know that it is not Microsoft's fault that the
            //  font is not converted to TT, but the vendor's fault.
            //
            //  NOTE:  This is only done if the User has NOT selected the
            //         YesToAll_PS install option.  Otherwise it will be very
            //         annoying to see message repeated over and over.
            //
            //  HACK:  The lpszPfb arg is used in the conditional to determine
            //         when the message box should be displayed.  If the
            //         IsPSFont routine is called from ValidFontFile routine,
            //         this arg will be NULL.  If this routine is called from
            //         InstallPSFont routine it will be non-NULL.  It is only
            //         during actual installation that we want the message
            //         displayed.
            //

            if (!bYesAll_PS && lpszPfb)
            {
                //
                // Convert ANSI buffers to UNICODE for our use
                //
    
                MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, Vendorbuf, -1,
                                     szCopyright, PATHMAX);

                MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, Descbuf, -1,
                                     strbuf, PATHMAX);
    

                hwndParent = (hDlg == NULL) ? HWND_DESKTOP : hDlg;

                MyMessageBox (hwndParent, MYFONT+14, INITS+1,
                              MB_OK | MB_ICONEXCLAMATION,
                              (LPTSTR) szCopyright,
                              (LPTSTR) strbuf);
            }
            break;

        case SUCCESS:
            *lpiFontType = TYPE1_FONT;
            break;

        case MAYBE:
            //
            //  Check font copyright and ask for user response if necessary
            //

            //
            // Convert ANSI buffers to UNICODE for our use
            //

            MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, Vendorbuf, -1, szCopyright, PATHMAX);
            MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, Descbuf, -1, strbuf, PATHMAX);

            switch (CheckVendorCopyright (szCopyright))
            {
            case IDYES:
                *lpiFontType = TYPE1_FONT;
                break;

            case IDNO:
                *lpiFontType = TYPE1_FONT_NC;
                break;

            case -1:
            default:
                *lpiFontType = TYPE1_FONT;

            //  HACK:  The lpszPfb arg is used in the conditional to determine
            //         when the message box should be displayed.  If the
            //         IsPSFont routine is called from ValidFontFile routine,
            //         this arg will be NULL.  If this routine is called from
            //         InstallPSFont routine it will be non-NULL.  It is only
            //         during actual installation that we want the message
            //         displayed.
            //
                if (lpszPfb)
                {
                    hwndParent = (hDlg == NULL) ? HWND_DESKTOP : hDlg;
    
                    iResponse = MyMessageBox (hwndParent, MYFONT+41, INITS+1,
                                              MB_YESNO | MB_ICONEXCLAMATION
                                              | MB_DEFBUTTON2,
                                              (LPTSTR) strbuf,
                                              (LPTSTR) szCopyright);
    
                    AddVendorCopyright (szCopyright, iResponse);
    
                    *lpiFontType = (iResponse == IDYES) ? TYPE1_FONT : TYPE1_FONT_NC;
                }
                break;
            }
            break;

        default:
            //
            //  ERROR! from routine - assume worst case
            //

            *lpiFontType = TYPE1_FONT_NC;
            break;
        }

        //
        //  Return Font description
        //

        if (lpszDesc)
        {
            // Convert Descbuf to UNICODE since IsType1 is ANSI
            MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, Descbuf, -1, strbuf, PATHMAX);

            wsprintf(lpszDesc, TEXT("%s (%s)"), strbuf, szPostScript);
        }

        //
        //  Return PFM file name
        //

        if (lpszPfm)
        {
            // Return PFM file name - convert to UNICODE since IsType1 is ANSI
            MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, Pfmbuf, -1, lpszPfm, PATHMAX);
        }

        //
        //  Return PFB file name
        //

        if (lpszPfb)
        {
            // Return PFB file name - convert to UNICODE since IsType1 is ANSI
            MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, Pfbbuf, -1, lpszPfb, PATHMAX);
        }

        bRet = TRUE;
    }
    return bRet;

}


/////////////////////////////////////////////////////////////////////////////
//
// InitPSInstall
//
//  Initialize PostScript install routine global variables.
//
/////////////////////////////////////////////////////////////////////////////

void InitPSInstall ()
{
    //
    //  Initialize linked list variables for "MAYBE" copyright vendor list
    //
    
    pFirstVendor = NULL;

    //
    //  Other installation globals
    //

    bYesAll_PS = FALSE;
    bConvertPS = TRUE;
    bInstallPS = TRUE;
    bCopyPS    = TRUE;
    
    return;
}


/////////////////////////////////////////////////////////////////////////////
//
// TermPSInstall
//
//  Initialize PostScript install routine global variables.
//
/////////////////////////////////////////////////////////////////////////////

void TermPSInstall ()
{
    PSVENDOR *pVendor;

    //
    //  Traverse the list, freeing list memory and strings.
    //

    pVendor = pFirstVendor;

    while (pVendor)
    {
        pFirstVendor = pVendor;
        pVendor = pVendor->pNext;

        if (pFirstVendor->pszCopyright)
            FreeStr ((LPVOID) pFirstVendor->pszCopyright);

        FreeMem ((LPVOID) pFirstVendor, sizeof(PSVENDOR));
    }

    //
    //  Reset global to be safe
    //

    pFirstVendor = NULL;

    return;
}


/////////////////////////////////////////////////////////////////////////////
//
// InstallT1Font
//
//  Install PostScript Type1 font, possibly converting the Type1 font to a
//  TrueType font in the process.  Write registry entries so the PostScript
//  printer driver can find these files either in their original source
//  directory or locally in the 'shared' or system directory.
//
/////////////////////////////////////////////////////////////////////////////

int InstallT1Font (HWND   hDlg,
                   HWND   hListFonts,           //  Installed fonts listbox
                   BOOL   bCopyTTFile,          //  Copy TT file?
                   BOOL   bInSharedDir,         //  Files in Shared Directory?
                   LPTSTR szKeyName,            //  IN: PFM/INF Source File name & dir
                                                //  OUT: Destination file name    
                   LPTSTR szDesc)               //  INOUT: Font description

{
    int    iFontType;                           //  Enumerated Font type
    int    rc, iRet;
    WORD   wMsg;
    BOOL   bCreatedPfm = FALSE;

    TCHAR  szTemp[PATHMAX];
    TCHAR  szTemp2[PATHMAX];
    TCHAR  szPfbName[PATHMAX];
    TCHAR  szPfmName[PATHMAX];
    TCHAR  szSrcDir[PATHMAX];
    TCHAR  szDstName[PATHMAX];
    TCHAR  szTTFName[PATHMAX];
    TCHAR  *pszArg1, *pszArg2;

    T1_INSTALL_OPTIONS t1ops;

    //
    //  ASCII Buffers for use in ASCII-only api calls
    //

    char  pfb[PATHMAX];
    char  pfm[PATHMAX];
    char  ttf[PATHMAX];


    if (!IsPSFont (hDlg, szKeyName, NULL, szPfmName, szPfbName,
                    &bCreatedPfm, &iFontType))
    {
        MyMessageBox (hDlg, ERRORS+1, INITS+1,
                      MB_OK | MB_ICONEXCLAMATION,
                      (LPTSTR) szDesc);

        iRet = TYPE1_INSTALL_IDNO;
        goto InstallPSFailure;
    }

    t1ops.szDesc = szDesc;
    t1ops.iFontType = iFontType;

    //
    //  Keep a copy of source directory
    //

    lstrcpy (szSrcDir, szKeyName);
    StripFilespec (szSrcDir);
    AddBackslash (szSrcDir);

    //////////////////////////////////////////////////////////////////////
    // Check if font is already loaded on the system
    //////////////////////////////////////////////////////////////////////

    t1ops.bOnlyPSInstalled = FALSE;
    t1ops.bMatchingTT      = FALSE;

    //
    //  Check both Type1 & Fonts registry location for prior font installation
    //
    if (CheckT1Install (szDesc, NULL))
    {
        if (CheckTTInstall (szDesc))
        {
            //
            // "Font is already loaded"
            //

            iRet = MyMessageBox (hDlg, MYFONT+21, INITS+1,
                                 MB_OKCANCEL|MB_ICONEXCLAMATION,
                                 (LPTSTR)szDesc);
            goto InstallPSFailure;
        }
        else
            t1ops.bOnlyPSInstalled = TRUE;
    }
    else if (CheckTTInstall (szDesc))
    {
        t1ops.bMatchingTT = TRUE;

        if (!bYesAll_PS)
        {
            //
            // "The TrueType version of this font is already installed."
            //

            switch (MyMessageBox(hDlg, MYFONT+17, INITS+1,
                    MB_YESNOCANCEL|MB_ICONEXCLAMATION, (LPTSTR)szDesc))
            {
            case IDYES:
                break;

            case IDNO:
                iRet = TYPE1_INSTALL_IDNO;
                goto InstallPSFailure;

            case IDCANCEL:
            default:
                iRet = TYPE1_INSTALL_IDCANCEL;
                goto InstallPSFailure;
            }
        }
    }

    //
    //  The global state of
    //
    //      bConvertPS  -  Convert Type1 files to TT
    //      bInstallPS  -  Install PS files
    //
    //  is only effective for the last time the "Install Type 1 fonts"
    //  dialog was displayed.  Check the state of these globals against
    //  what we know about the current font to determine if the dialog
    //  should be redisplayed.
    //
    //  5/31/94 [stevecat] DO NOT redisplay the dialog after "YesToAll"
    //  selected once.  Instead, display messages about 'exceptions' to
    //  their initial choices and give user the option to continue
    //  installation.
    //

    if (bYesAll_PS)
    {
        //
        //  If the PS version of this font is already installed AND the
        //  global bInstall == TRUE, then the globals are out-of-sync
        //  with this font.  Let user know and continue installation.
        //

        if (t1ops.bOnlyPSInstalled && bInstallPS)
        {
            //
            // "The Type 1 version of this font is already installed."
            //

            switch (MyMessageBox(hDlg, MYFONT+15, INITS+1,
                    MB_YESNOCANCEL|MB_ICONEXCLAMATION, (LPTSTR)szDesc))
            {
            case IDYES:
                break;

            case IDNO:
                iRet = TYPE1_INSTALL_IDNO;
                goto InstallPSFailure;

            case IDCANCEL:
            default:
                iRet = TYPE1_INSTALL_IDCANCEL;
                goto InstallPSFailure;
            }
        }

        //
        //  If the matching TT font is already installed AND the global
        //  bConvertPS == TRUE, then the globals are out-of-sync with
        //  this font.  Let the user know and continue installation.
        //

        if (t1ops.bMatchingTT && bConvertPS)
        {
            //
            // "The TrueType version of this font is already installed."
            //

            switch (MyMessageBox(hDlg, MYFONT+17, INITS+1,
                    MB_YESNOCANCEL|MB_ICONEXCLAMATION, (LPTSTR)szDesc))
            {
            case IDYES:
                break;

            case IDNO:
                iRet = TYPE1_INSTALL_IDNO;
                goto InstallPSFailure;

            case IDCANCEL:
            default:
                iRet = TYPE1_INSTALL_IDCANCEL;
                goto InstallPSFailure;
            }
        }
    }

    //
    //  Get user options for PostScript font installation:
    //      - TT conversion
    //      - Type1 installation
    //      - Copying Type1 files
    //
    //  State returned in globals:
    //
    //      bConvertPS  -  Convert Type1 files to TT
    //      bInstallPS  -  Install PS files
    //      bCopyPS     -  Copy PS files to Windows\System dir
    

    if (!bYesAll_PS)
    {
        HourGlass (FALSE);

    
        switch (DoDialogBoxParam(DLG_INSTALL_PS, hDlg, (DLGPROC)InstallPSDlg,
                                 IDH_DLG_INSTALL_PS, (LPARAM) &t1ops))
        {
        case IDNO:
            //
            //  Note that we do not do HourGlass (TRUE), but that
            //  should be harmless, since we are either going to come
            //  right back here, or we are going to exit
            //
            return IDNO;
    
        case IDD_YESALL:
            bYesAll_PS = TRUE;

            //
            //  Fall thru...
            //

        case IDYES:
            //
            //  Give a warning here about installing from a non-local
            //  directory and not copying files, as necessary.
            //

            if (bInstallPS && !bCopyPS)
            {
                lstrcpy (szTemp, szPfbName);
                StripFilespec (szTemp);
                AddBackslash (szTemp);

                switch (GetDriveType (szTemp))
                {
                    case DRIVE_REMOTE:
                    case DRIVE_REMOVABLE:
                    case DRIVE_CDROM:
                    case DRIVE_RAMDISK:
                        if (MyMessageBox (hDlg, ERRORS+6, INITS+1,
                                          MB_YESNO|MB_ICONEXCLAMATION) != IDYES)
                        {
                            iRet = TYPE1_INSTALL_IDCANCEL;
                            goto InstallPSFailure;
                        }
                }
            }
            break;
    
        default:
            // CANCEL and NOMEM (user already warned)
            iRet = TYPE1_INSTALL_IDCANCEL;
            goto InstallPSFailure;
        }
    
        HourGlass (TRUE);
    }

    //
    // szDstName should already have the full source file name
    //
    //  Only convert the Type1 font to TT if:
    //
    //  a) The user asked us to do it;
    //  b) The font can be converted, AND;
    //  c) There is not a matching TT font already installed
    //

    if (bConvertPS && (iFontType != TYPE1_FONT_NC) && !t1ops.bMatchingTT)
    {
        //////////////////////////////////////////////////////////////////
        // Convert Type1 files to TrueType
        //
        // Copy converted TT file, if necessary, to "fonts" directory
        //
        // NOTE: We are using the ConvertTypeface api to do the copying
        //       and it is an ASCII only api.
        //////////////////////////////////////////////////////////////////

        //
        //  Create destination name with .ttf
        //

        lstrcpy (szTemp, szPfmName);
        StripPath (szTemp);
        ConvertExtension (szTemp, szTTF);

        //
        //  Build destination file pathname based on bCopyTTFile
        //

        if (bCopyTTFile || bInSharedDir)
        {
            //
            //  Copy file to local directory
            //

            lstrcpy (szDstName, szSharedDir);
        }
        else
        {
            //
            //  Create converted file in source directory
            //

            lstrcpy (szDstName, szSrcDir);
        }

        //
        //  Check new filename for uniqueness
        //

        if (!(UniqueFilename (szTemp, szTemp, szDstName)))
        {
            iRet = MyMessageBox (hDlg, MYFONT+19, MYFONT+7,
                                 MB_OKCANCEL | MB_ICONEXCLAMATION,
                                 (LPTSTR)szDesc);
            goto InstallPSFailure;
        }

        lstrcat (szDstName, szTemp);

        //
        //  Save destination filename for return to caller
        //

        if (bCopyTTFile || bInSharedDir)
            lstrcpy (szTTFName, szTemp);
        else
            lstrcpy (szTTFName, szDstName);

        //
        //  We will convert and copy the Type1 font in the same api
        //

        WideCharToMultiByte (CP_ACP, 0, szPfbName, -1, pfb,
                                PATHMAX, NULL, NULL);

        WideCharToMultiByte (CP_ACP, 0, szPfmName, -1, pfm,
                                PATHMAX, NULL, NULL);

        WideCharToMultiByte (CP_ACP, 0, szDstName, -1, ttf,
                                PATHMAX, NULL, NULL);

        ResetProgress ();

        //
        //  Remove "PostScript" postfix string from description
        //

        RemoveDecoration (szDesc, TRUE);

        if ((rc = (int) ConvertTypefaceA (pfb, pfm, ttf,
                                          Progress,
                                          (void *) szDesc)) < 0)
        {
            pszArg1 = szPfmName;
            pszArg2 = szPfbName;

            switch (rc)
            {
            case ARGSTACK:
            case TTSTACK:
            case NOMEM:
                wMsg = ERRORS+11;
                break;

            case NOMETRICS:
            case BADMETRICS:
            case UNSUPPORTEDFORMAT:
                //
                //  Something is wrong with the .pfm metrics file
                //
                pszArg1 = szDstName;
                pszArg2 = szPfmName;
                wMsg = MYFONT+43;
                break;

            case BADT1HYBRID:
            case BADT1HEADER:
            case BADCHARSTRING:
            case NOCOPYRIGHT:
                //
                //  Bad .pfb input file - format, or corruption
                //
                pszArg1 = szDstName;
                pszArg2 = szPfbName;
                wMsg = MYFONT+44;
                break;

            case BADINPUTFILE:
                //
                //  Bad input file names, or formats or file errors
                //  or file read errors
                //
                pszArg1 = szDstName;
                pszArg2 = szPfbName;
                wMsg = MYFONT+45;
                break;

            case BADOUTPUTFILE:
                //
                //  No diskspace for copy, read-only share, etc.
                //
                pszArg1 = szDstName;
                wMsg = MYFONT+46;
                break;

            default:
                //
                //  Cannot convert szDesc to TrueType - general failure
                //
                pszArg1 = szDstName;
                pszArg2 = szDesc;
                wMsg = MYFONT + 47;
                break;
            }


            iRet = MyMessageBox (hDlg, wMsg, INITS+1,
                                 MB_OKCANCEL | MB_ICONEXCLAMATION,
                                 pszArg1, pszArg2, szPfmName);
            goto InstallPSFailure;
        }

        //
        //  Change font description to have "TrueType" now
        //

        wsprintf(szDesc, TEXT("%s (%s)"), szDesc, szTrueType);
    }

    iRet = TYPE1_INSTALL_IDNO;

    if (bInstallPS && !t1ops.bOnlyPSInstalled)
    {
        //
        //  Remove "PostScript" postfix string from description
        //

        lstrcpy (szTemp2, szDesc);
        RemoveDecoration (szTemp2, TRUE);

        //
        //  Now reset per font install progress
        //

        ResetProgress ();
        Progress2 (0, szTemp2);


        //
        //  Only copy the files if the User asked us to AND they are NOT
        //  already in the Shared directory.
        //

        if (bCopyPS && !bInSharedDir)
        {
            //
            //  Copy file progress
            //

            Progress2 (10, szTemp2);

            /////////////////////////////////////////////////////////////////
            // COPY files to "system" directory
            /////////////////////////////////////////////////////////////////

            //
            //  For .inf/.afm file install::  Check .pfm pathname to see if
            //  it is the same as the destination file pathname we built.
            //  Make this check before we test/create a UniqueFilename.
            //

            //  Build Destination file pathname for .PFM file
    
            lstrcpy (szTemp, szPfmName);
            StripPath (szTemp);

            lstrcpy (szDstName, szSharedDir);
            lstrcat (szDstName, szTemp);

            //
            //  Check to see if the .pfm file already exists in the "system"
            //  directory.  If it does, then just copy the .pfb over.
            //

            if (!lstrcmpi (szPfmName, szDstName))
                goto CopyPfbFile;


            if (!(UniqueFilename (szTemp, szTemp, szSharedDir)))
            {

                iRet = MyMessageBox (hDlg, MYFONT+19, MYFONT+7,
                                     MB_OKCANCEL | MB_ICONEXCLAMATION,
                                     (LPTSTR)szDesc);

                goto InstallPSFailure;
            }
    
            lstrcpy (szDstName, szSharedDir);
            lstrcat (szDstName, szTemp);
    
            if ((rc = Copy (hDlg, szPfmName, szDstName)) <= 0)
            {
                switch (rc)
                {
                    //  On these two return codes, the USER has effectively
                    //  "Cancelled" the copy operation for the fonts
                case COPY_CANCEL:
                case COPY_DRIVEOPEN:
                    return IDCANCEL;
    
                case COPY_SELF:
                    wMsg = ERRORS+10;
                    break;
    
                case COPY_NOCREATE:
                    wMsg = ERRORS+13;
                    break;
    
                case COPY_NODISKSPACE:
                    wMsg = ERRORS+12;
                    break;
    
                case COPY_NOMEMORY:
                    wMsg = ERRORS+11;
                    break;
    
                default:
                    wMsg = ERRORS+14;
                    break;
                }

                iRet = MyMessageBox (hDlg, wMsg, INITS+1,
                                     MB_OKCANCEL | MB_ICONEXCLAMATION,
                                     szDstName, szPfmName);
                goto InstallPSFailure;
            }
    
CopyPfbFile:

            //
            //  Copying pfm file was small portion of install
            //

            Progress2 (30, szTemp2);

            //  Build Destination file pathname for .PFB file
    
            lstrcpy (szTemp, szPfbName);
            StripPath (szTemp);

            if (!(UniqueFilename (szTemp, szTemp, szSharedDir)))
            {
                iRet = MyMessageBox (hDlg, MYFONT+19, MYFONT+7,
                                     MB_OKCANCEL | MB_ICONEXCLAMATION,
                                     (LPTSTR)szDesc);
                goto InstallPSFailure;
            }
    
            lstrcpy (szDstName, szSharedDir);
            lstrcat (szDstName, szTemp);
    
            if ((rc = Copy (hDlg, szPfbName, szDstName)) <= 0)
            {
                switch (rc)
                {
                    //  On these two return codes, the USER has effectively
                    //  "Cancelled" the copy operation for the fonts
                case COPY_CANCEL:
                case COPY_DRIVEOPEN:
                    return IDCANCEL;
    
                case COPY_SELF:
                    wMsg = ERRORS+10;
                    break;
    
                case COPY_NOCREATE:
                    wMsg = ERRORS+13;
                    break;
    
                case COPY_NODISKSPACE:
                    wMsg = ERRORS+12;
                    break;
    
                case COPY_NOMEMORY:
                    wMsg = ERRORS+11;
                    break;
    
                default:
                    wMsg = ERRORS+14;
                    break;
                }

                iRet = MyMessageBox (hDlg, wMsg, INITS+1,
                                     MB_OKCANCEL | MB_ICONEXCLAMATION,
                                     szDstName, szPfbName);
                goto InstallPSFailure;
            }
        }

        //
        //  Copying pfb file was large portion of install
        //

        Progress2 (85, szTemp2);

        //
        //  Write registry entry to "install" font for use by the
        //  PostScript driver, but only after successfully copying
        //  files (if copy was necessary).
        //

        iRet = WriteType1RegistryEntry (hDlg, szDesc, szPfmName, szPfbName);

        //
        //  Final registry write completes install, except for listbox munging.
        //  Note that TrueType file install is handled separately.
        //
    
        Progress2 (100, szTemp2);
    }

    //
    //  Determine correct return code based on installation options and
    //  current installed state of font.
    //

    if (bConvertPS && (iFontType != TYPE1_FONT_NC))
    {
        //
        //  Handle the special case of when the matching TTF font is already
        //  installed.
        //
        if (t1ops.bMatchingTT)
            goto Type1InstallCheck;


        lstrcpy (szKeyName, szTTFName);

        if (t1ops.bOnlyPSInstalled)
        {
            //
            //  There is already a Lbox entry for the PS version of this font
            //  that needs to be deleted because the TT installation will
            //  add a new Lbox entry.
            //

            iRet = TYPE1_INSTALL_TT_AND_MPS;

            //
            //  Funnel all exits thru 1 point to check for Install Cancellation
            //

            goto MasterExit;
        }
        else if (bInstallPS)
        {
            iRet = (iRet == IDOK) ? TYPE1_INSTALL_TT_AND_PS : TYPE1_INSTALL_TT_ONLY;

            //
            //  Funnel all exits thru 1 point to check for Install Cancellation
            //

            goto MasterExit;
        }
        else
        {
            iRet = TYPE1_INSTALL_TT_ONLY;
            goto CheckPfmDeletion;
        }
    }


Type1InstallCheck:

    if (bInstallPS)
    {
        if (iRet != IDOK)
        {
            iRet = TYPE1_INSTALL_IDNO;
            goto InstallPSFailure;
        }

        iRet = TYPE1_INSTALL_IDNO;

        if (t1ops.bMatchingTT)
        {
            //
            //  If we previously found the Matching TT font for this Type1
            //  font and installed the PostScript font in this session, set
            //  the font type for the Matching TT font to IF_TYPE1_TT.
            //
            //  Also, do not add a new entry in listbox for the Type1 font.
            //
            //  Find matching  "xxxxx (TrueType)" entry and change its
            //  ItemData to IF_TYPE1_TT
            //
            //  Change font description to have "(TrueType)" now
            //
    
            RemoveDecoration (szDesc, TRUE);

            wsprintf (szDesc, TEXT("%s (%s)"), szDesc, szTrueType);
    
            rc= SendMessage (hListFonts, LB_FINDSTRINGEXACT, (WPARAM) -1,
                                                         (LONG)szDesc);
    
            if (rc != LB_ERR)
            {
                SendMessage (hListFonts, LB_SETITEMDATA, rc, (LONG) IF_TYPE1_TT);
    
                SendMessage (hListFonts, LB_SETSEL, 1, rc);
    
                UpdateWindow (hListFonts);
    
                iRet = TYPE1_INSTALL_PS_AND_MTT;
            }
        }
        else
        {
            rc = SendMessage (hListFonts, LB_ADDSTRING, 0, (LONG)(LPTSTR)szDesc);
        
            //
            //  Attach font type to each listed font
            //
        
            if (rc != LB_ERR)
            {
                SendMessage (hListFonts, LB_SETITEMDATA, rc, IF_TYPE1);
        
                SendMessage (hListFonts, LB_SETSEL, 1, rc);
        
                UpdateWindow (hListFonts);

                iRet = TYPE1_INSTALL_PS_ONLY;
            }
        }
    }

    if (!bInstallPS)
        goto CheckPfmDeletion;

    //
    //  Funnel all exits thru 1 point to check for Install Cancellation
    //

    goto MasterExit;

    /////////////////////////////////////////////////////////////////////////
    //
    //  Install failure exit AND Delete extraneously created PFM file
    //
    //  NOTE:  For the installation scenario where we based installation on
    //         the .INF/.AFM file and the Type1 font was NOT installed, we
    //         need to delete the .FPM file that was created by the
    //         CheckType1A routine in the IsPSFont routine.
    //
    /////////////////////////////////////////////////////////////////////////

InstallPSFailure:

CheckPfmDeletion:

    if (bCreatedPfm)
        DeleteFile (szPfmName);

MasterExit:

    return (bCancelInstall ? IDCANCEL : iRet);
}


/////////////////////////////////////////////////////////////////////////////
//
// InstallPSDlg
//
//  This dialog proc manages the Install PostScript font dialog which allows
//  the user several options for the installation, including converting the
//  file to a TrueType font.
//
//  Globals munged:
//
//      bConvertPS    -   Convert Type1 files to TT
//      bInstallPS    -   Install PS files
//      bCopyPS       -   Copy PS files to Windows dir
//
/////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY InstallPSDlg (HWND hDlg, UINT nMsg, DWORD wParam, LONG lParam)
{
    TCHAR  szFormat[PATHMAX];
    TCHAR  szTemp[PATHMAX];
    TCHAR  szTemp2[PATHMAX];
    int    iButtonChecked;
    int    wMsg;

    static HWND hwndActive = NULL;

    T1_INSTALL_OPTIONS *pt1ops;


    switch (nMsg)
    {

    case WM_INITDIALOG:

        pt1ops = (PT1_INSTALL_OPTIONS) lParam;

        //
        //  Remove all "PostScript" or "TrueType" postfix to font name
        //

        lstrcpy (szTemp2, (LPTSTR)pt1ops->szDesc);

        RemoveDecoration (szTemp2, FALSE);

        LoadString (hModule, MYFONT + 34, szFormat, CharSizeOf(szFormat));
        wsprintf (szTemp, szFormat, szTemp2);

        SetWindowLong (hDlg, DWL_USER, lParam);

        SetDlgItemText (hDlg, FONT_INSTALLMSG, szTemp);
        EnableWindow (hDlg, TRUE);

        if (pt1ops->bOnlyPSInstalled && pt1ops->bMatchingTT)
        {
            //
            // ERROR!  Both of these options should not be set at
            //         this point.  It means that the font is
            //         already installed.  This should have been
            //         handled before calling this dialog.
            //

            wMsg = MYFONT+21;
InstallError:

            MyMessageBox (hDlg,
                          wMsg,
                          INITS+1,
                          MB_OK | MB_ICONEXCLAMATION,
                          pt1ops->szDesc);

            EndDialog (hDlg, IDNO);
            break;
        }

        if ((pt1ops->iFontType == TYPE1_FONT_NC) && pt1ops->bOnlyPSInstalled)
        {
            //
            //  ERROR! This case is when I have detected only the PS
            //         version of font installed, and the font CANNOT
            //         be converted to TT for some reason.
            //

            wMsg = MYFONT+37;
            goto InstallError;
        }

        /////////////////////////////////////////////////////////////////////
        //
        //  Setup user options depending on install state of font and
        //  convertibility to TT of T1 font and on previous user choices.
        //
        /////////////////////////////////////////////////////////////////////


        if ((pt1ops->iFontType == TYPE1_FONT) && (!pt1ops->bMatchingTT))
        {
            //
            //  This one can be converted
            //
            CheckDlgButton(hDlg, FONT_CONVERT_PS, bConvertPS);
        }
        else
        {
            //
            //  Do not allow conversion to TT because, either the font
            //  type is TYPE1_FONT_NC (i.e. it cannot be converted) OR
            //  the TT version of font is already installed.
            //
           
            CheckDlgButton(hDlg, FONT_CONVERT_PS, FALSE);
            EnableWindow (GetDlgItem (hDlg, FONT_CONVERT_PS), FALSE);
        }

        if (pt1ops->bOnlyPSInstalled)
        {
            //
            //  If the PostScript version of this font is already
            //  installed, then we gray out the options to re-install
            //  the PostScript version of font, but continue to allow
            //  the User to convert it to TT.
            //

            CheckDlgButton(hDlg, FONT_INSTALL_PS, 0);

            EnableWindow (GetDlgItem (hDlg, FONT_INSTALL_PS), FALSE);

            CheckDlgButton(hDlg, FONT_COPY_PS, 0);

            EnableWindow (GetDlgItem (hDlg, FONT_COPY_PS), FALSE);
        }
        else
        {
            //
            //  PostScript version of font is not installed.  Set
            //  state of "INSTALL" and "COPY" checkboxes based on
            //  global state of "INSTALL"
            //

            CheckDlgButton(hDlg, FONT_INSTALL_PS, bInstallPS);

            CheckDlgButton(hDlg, FONT_COPY_PS, bCopyPS);

            EnableWindow (GetDlgItem (hDlg, FONT_COPY_PS), bInstallPS);

        }

        //
        //  Save the modeless dlg window handle for reactivation
        //

        hwndActive = GetActiveWindow ();
        break;


    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case FONT_INSTALL_PS:
            if (HIWORD(wParam) != BN_CLICKED)
                break;

            //
            // Get state of "INSTALL" checkbox
            //

            iButtonChecked = IsDlgButtonChecked (hDlg, LOWORD(wParam));

            //
            // A disabled checkbox is same as "No Install" selection
            //

            if (iButtonChecked != 1)
                iButtonChecked = 0;

            //
            //  Enable or disable "COPY" control based on state of
            //  "INSTALL" checkbox.  Also, initialize it.
            //

            EnableWindow (GetDlgItem (hDlg, FONT_COPY_PS), iButtonChecked);

            if (iButtonChecked)
                CheckDlgButton(hDlg, FONT_COPY_PS, bCopyPS);

            break;


        case IDD_HELP:
            goto DoHelp;

        case IDYES:
        case IDD_YESALL:
            bConvertPS =
            bInstallPS = FALSE;

            if (IsDlgButtonChecked (hDlg, FONT_CONVERT_PS) == 1)
                bConvertPS = TRUE;

            if (IsDlgButtonChecked (hDlg, FONT_INSTALL_PS) == 1)
                bInstallPS = TRUE;

            //
            //  This is checked twice because it could be disabled,
            //  in which case we leave the previous state alone.
            //

            if (IsDlgButtonChecked (hDlg, FONT_COPY_PS) == 1)
                bCopyPS = TRUE;

            if (IsDlgButtonChecked (hDlg, FONT_COPY_PS) == 0)
                bCopyPS = FALSE;

            //
            //  Fall thru...
            //
            
        case IDNO:
        case IDCANCEL:
            //
            //  Reset the active window to "Install Font Progress" modeless dlg
            //

            if (hwndActive)
            {
                SetActiveWindow (hwndActive);
                hwndActive = NULL;
            }

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
// RemoveDecoration
//
//  Deletes the "(TrueType)" or "(PostScript)" postfix string from the end
//  of a font name.  Optionally it will also remove the trailing space.
//
//  NOTE:  This function modifies the string passed into the function.
//
/////////////////////////////////////////////////////////////////////////////

void RemoveDecoration (LPTSTR pszDesc, BOOL bDeleteTrailingSpace)
{
    LPTSTR lpch;

    //
    //  Remove any postfix strings like "(PostScript)" or "(TrueType)"
    //

    if (lpch = _tcschr (pszDesc, TEXT('(')))
    {
        //
        //  End string at <space> before "("
        //
        if (bDeleteTrailingSpace)
            lpch--;

        *lpch = CHAR_NULL;
    }

    return ;
}

/////////////////////////////////////////////////////////////////////////////
//
// CheckT1Install
//
//  Checks the Type1 fonts location in registry to see if this font is or
//  has been previously installed as a "PostScript" font.  Optionally, it
//  will return the data for the "szDesc" value if it finds a matching entry.
//
//  Assumes "szData" buffer is of least size T1_MAX_DATA.
//
/////////////////////////////////////////////////////////////////////////////

BOOL CheckT1Install (LPTSTR pszDesc, LPTSTR pszData)
{
    TCHAR  szTemp[PATHMAX];
    DWORD  dwSize;
    DWORD  dwType;
    HKEY   hkey;
    BOOL   bRet = FALSE;


    hkey = NULL;

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,        // Root key
                      szType1Key,                // Subkey to open
                      0L,                        // Reserved
                      KEY_READ,                  // SAM
                      &hkey)                     // return handle
            == ERROR_SUCCESS)
    {
        //
        //  Remove any postfix strings like "PostScript" or "TrueType"
        //

        lstrcpy (szTemp, pszDesc);

        RemoveDecoration (szTemp, TRUE);

        dwSize =  pszData ? T1_MAX_DATA * sizeof(TCHAR) : 0;

        if (RegQueryValueEx (hkey, szTemp, NULL, &dwType,
                             (LPBYTE) pszData, &dwSize)
                ==  ERROR_SUCCESS)
        {
            bRet = (dwType == REG_MULTI_SZ);
        }
        else
        {
            bRet = FALSE;
        }

        RegCloseKey (hkey);
    }

    return bRet;
}


/////////////////////////////////////////////////////////////////////////////
//
// AddSystemPath
//
//  Add "System" path to a naked file name, if no path currently exists
//  on the filename.  Assumes pszFile buffer is at least PATHMAX chars in
//  length.
//
/////////////////////////////////////////////////////////////////////////////

void AddSystemPath (LPTSTR pszFile)
{
    TCHAR  szPath[PATHMAX];

    //
    //  Add "system" path, if no path present on file
    //

    lstrcpy (szPath, pszFile);

    StripFilespec (szPath);

    if (szPath[0] == CHAR_NULL)
    {
        lstrcpy (szPath, szSharedDir);
        AddBackslash (szPath);
        lstrcat (szPath, pszFile);
        lstrcpy (pszFile, szPath);
    }

    return ;
}

/////////////////////////////////////////////////////////////////////////////
//
// ExtractT1Files
//
//  Extracts file names from a REG_MULTI_SZ (multi-string) array that is
//  passed into this routine.  The output strings are expected to be at
//  least PATHMAX in size.  A "" (NULL string) indicates that a filename
//  string was not present.  This should only happen for the PFB filename
//  argument.
//
/////////////////////////////////////////////////////////////////////////////

BOOL ExtractT1Files (LPTSTR pszMulti, LPTSTR pszPfmFile, LPTSTR pszPfbFile)
{
    LPTSTR pszPfm;
    LPTSTR pszPfb;

    if (!pszMulti)
        return FALSE;

    if ((pszMulti[0] != CHAR_TRUE) && (pszMulti[0] != CHAR_FALSE))
        return FALSE;

    //
    //  .Pfm file should always be present
    //

    pszPfm = pszMulti + lstrlen(pszMulti) + 1;

    lstrcpy (pszPfmFile, pszPfm);

    //
    //  Add "system" path, if no path present on files
    //

    AddSystemPath (pszPfmFile);

    //
    //  Check to see if .pfb filename is present
    //

    if (pszMulti[0] == CHAR_TRUE)
    {
        pszPfb = pszPfm + lstrlen(pszPfm) + 1;
        lstrcpy (pszPfbFile, pszPfb);

        //
        //  Add "system" path, if no path present on files
        //
    
        AddSystemPath (pszPfbFile);
    }
    else
    {
        pszPfbFile[0] = CHAR_NULL;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
// DeleteT1Install
//
//  Deletes a Type1 entry from the registry and optionally the files pointed
//  to in the data strings.
//
/////////////////////////////////////////////////////////////////////////////

BOOL DeleteT1Install (HWND hDlg, LPTSTR pszDesc, BOOL bDeleteFiles)
{
    TCHAR  szTemp[PATHMAX];
    TCHAR  szTemp2[T1_MAX_DATA];
    TCHAR  szPfmFile[PATHMAX];
    TCHAR  szPfbFile[PATHMAX];
    TCHAR  szPath[PATHMAX];
    DWORD  dwSize;
    DWORD  dwType;
    HKEY   hkey;
    BOOL   bRet = FALSE;

    hkey = NULL;

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,        // Root key
                      szType1Key,                // Subkey to open
                      0L,                        // Reserved
                      (KEY_READ | KEY_WRITE),    // SAM
                      &hkey)                     // return handle
            == ERROR_SUCCESS)
    {
        //
        //  Remove any postfix strings like "PostScript" or "TrueType"
        //

        lstrcpy (szTemp, pszDesc);

        RemoveDecoration (szTemp, TRUE);

        if (bDeleteFiles)
        {
            dwSize = sizeof(szTemp2);
    
            if (RegQueryValueEx (hkey, szTemp, NULL, &dwType,
                                 (LPBYTE) szTemp2, &dwSize)
                    ==  ERROR_SUCCESS)
            {
                if (ExtractT1Files (szTemp2, szPfmFile, szPfbFile))
                {
                    //
                    //  Delete the files
                    //

                    if (DelSharedFile (hDlg, szTemp, szPfbFile, szPath, TRUE))
                        DelSharedFile (hDlg, szTemp, szPfmFile, szPath, FALSE);
                }
                else
                {
                    //  ERROR! Cannot get file names from string
                    goto RemoveT1Error;
                }
            }
            else
            {
RemoveT1Error:
                MyMessageBox (hDlg, MYFONT+4, INITS+1,
                              MB_ICONEXCLAMATION|MB_OKCANCEL, szTemp);

                bRet = FALSE;
            }
        }

        if (RegDeleteValue (hkey, szTemp) != ERROR_SUCCESS)
        {
            //
            //  ERROR! Put up message box
            //

            MyMessageBox (hDlg, MYFONT+4, INITS+1,
                          MB_ICONEXCLAMATION|MB_OKCANCEL, szTemp);

            bRet = FALSE;
        }
        else
        {
            bRet = TRUE;
        }

        RegCloseKey (hkey);
    }

    return bRet;
}


/////////////////////////////////////////////////////////////////////////////
//
// GetT1Install
//
//  Gets a Type1 entry information from the registry into the files pointed
//  to in the data strings.
//
/////////////////////////////////////////////////////////////////////////////

BOOL GetT1Install (HWND hDlg, LPTSTR pszDesc, LPTSTR pszPfmFile, LPTSTR pszPfbFile)
{
    TCHAR  szTemp2[T1_MAX_DATA];
    BOOL   bRet = FALSE;


    if (CheckT1Install (pszDesc, szTemp2))
    {
        bRet = ExtractT1Files (szTemp2, pszPfmFile, pszPfbFile);
    }

    return bRet;
}


/////////////////////////////////////////////////////////////////////////////
//
// CheckTTInstall
//
//  Check FONTS location in registry to see if this font has already
//  been installed.
//
/////////////////////////////////////////////////////////////////////////////

BOOL CheckTTInstall (LPTSTR szDesc)
{
    TCHAR szTemp[PATHMAX];
    TCHAR szTemp2[PATHMAX];

    //
    //  Change description string to have TrueType instead of
    //  PostScript and then check if it is already installed.
    //

    lstrcpy (szTemp, szDesc);

    RemoveDecoration (szTemp, FALSE);

    wsprintf(szTemp, TEXT("%s(%s)"), szTemp, szTrueType);

    if (GetProfileString(szFonts, szTemp, szNull, szTemp2, CharSizeOf(szTemp2)))
        return TRUE;

    return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
//
// WriteType1RegistryEntry
//
//  Create registry entry for this PostScript font by writing the path of
//  both the .PFM and .PFB files.
//
//  NOTE:  Checks global "bCopyPS" to determine if files have been copied
//         to the local shared directory.  In that case, the path info is
//         stripped from the file names passed into routine.
//
/////////////////////////////////////////////////////////////////////////////

int WriteType1RegistryEntry (HWND hDlg,
                             LPTSTR szDesc,         // Font name description
                             LPTSTR szPfmName,      // .PFM filename
                             LPTSTR szPfbName)      // .PFB filename
{
    TCHAR  szTemp[2*PATHMAX+6];
    TCHAR  szTemp2[PATHMAX];
    TCHAR  szClass[PATHMAX];
    DWORD  dwSize;
    DWORD  dwDisposition;
    HKEY   hkey = NULL;

    //
    //  Must have a Font description to store information in registry
    //

    if (!szDesc || !szPfmName)
        return TYPE1_INSTALL_IDNO;

    //
    //  Try to create the key if it does not exist or open existing key.
    //
    
    if (RegCreateKeyEx (HKEY_LOCAL_MACHINE,        // Root key
                        szType1Key,                // Subkey to open/create
                        0L,                        // Reserved
                        szClass,                   // Class string
                        0L,                        // Options
                        KEY_WRITE,                 // SAM
                        NULL,                      // ptr to Security struct
                        &hkey,                     // return handle
                        &dwDisposition)            // return disposition
            == ERROR_SUCCESS)
    {
        //
        //  Create REG_MULTI_SZ string to save in registry
        //
        //  X <null> [path]zzzz.pfm <null> [path]xxxxx.pfb <null><null>
        //
        //  Where X == T(rue) if .pfb file present
        //

        lstrcpy (szTemp, szPfbName ? szTrue : szFalse);
        lstrcat (szTemp, szHash);

        if (bCopyPS)
            StripPath (szPfmName);

        lstrcat (szTemp, szPfmName);
        lstrcat (szTemp, szHash);

        if (szPfbName)
        {
            if (bCopyPS)
                StripPath (szPfbName);

            lstrcat (szTemp, szPfbName);
            lstrcat (szTemp, szHash);
        }

        lstrcat (szTemp, szHash);

        dwSize = ByteCountOf(lstrlen(szTemp));

        //
        //  Now convert string to multi-string
        //

        FixupNulls (szTemp);

        //
        //  Create Registry Value name to store info under by
        //  removing any postfix strings like "PostScript" or
        //  "TrueType" from Font description string.
        //

        lstrcpy (szTemp2, szDesc);

        RemoveDecoration (szTemp2, TRUE);

        if (RegSetValueEx (hkey, szTemp2, 0L, REG_MULTI_SZ,
                            (LPBYTE) szTemp, dwSize)
                != ERROR_SUCCESS)
        {
            goto WriteRegError;
        }

        RegCloseKey (hkey);
    }
    else
    {
WriteRegError:

        //
        //  Put up a message box error stating that the USER does
        //  not have the permission necessary to install type1
        //  fonts.
        //

        if (hkey)
            RegCloseKey (hkey);

        return (MyMessageBox (hDlg, MYFONT+38, INITS+1,
                              MB_OKCANCEL | MB_ICONEXCLAMATION,
                              (LPTSTR)szDesc,
                              (LPTSTR)szType1Key));
    }

    return TYPE1_INSTALL_IDOK;
}


/////////////////////////////////////////////////////////////////////////////
//
// EnumType1Fonts
//
//  List all of the Type 1 fonts installed in the registry, check for
//  TrueType installation and put Postscript-only installed fonts in
//  the "Installed Fonts" list box.
//
//  This routine assumes that the TrueType descriptions are already
//  displayed in the "Installed Fonts" listbox.
//
/////////////////////////////////////////////////////////////////////////////

BOOL EnumType1Fonts (HWND hLBox)
{
    int    i, j;
    DWORD  dwSize;
    DWORD  dwPathMax;
    DWORD  dwType;
    HKEY   hkey;
    LPTSTR pszDesc;
    LPTSTR pszFontName;
    BOOL   bRet = FALSE;


    //////////////////////////////////////////////////////////////////////
    //  Get some storage
    //////////////////////////////////////////////////////////////////////

    pszFontName =
    pszDesc     = NULL;

    dwPathMax = PATHMAX * sizeof(TCHAR);

    pszFontName = (LPTSTR) AllocMem (dwPathMax);

    pszDesc = (LPTSTR) AllocMem (dwPathMax);

    if (!pszDesc || !pszFontName)
        goto EnumType1Exit;


    //////////////////////////////////////////////////////////////////////
    //  Get list of installed Type 1 fonts
    //////////////////////////////////////////////////////////////////////

    hkey = NULL;

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,       // Root key
                      szType1Key,               // Subkey to open
                      0L,                       // Reserved
                      KEY_READ,                 // SAM
                      &hkey)                    // return handle
            == ERROR_SUCCESS)
    {
        dwSize = dwPathMax;
        i = 0;

        while (RegEnumValue (hkey, i++, pszFontName, &dwSize, NULL,
                             &dwType, (LPBYTE) NULL, NULL)
                    == ERROR_SUCCESS)
        {
            if (dwType != REG_MULTI_SZ)
                continue;

            wsprintf(pszDesc, TEXT("%s (%s)"), pszFontName, szPostScript);

            //
            //  Check to see if TrueType version is already installed
            //

            if (CheckTTInstall (pszDesc))
            {
                //
                //  Find matching  "xxxxx (TrueType)" entry and change its
                //  ItemData to IF_TYPE1_TT
                //

                wsprintf(pszDesc, TEXT("%s (%s)"), pszFontName, szTrueType);

                j = SendMessage (hLBox, LB_FINDSTRINGEXACT, (WPARAM) -1,
                                                             (LONG)pszDesc);

                if (j != LB_ERR)
                    SendMessage (hLBox, LB_SETITEMDATA, j, (LONG) IF_TYPE1_TT);

                // else
                    // ERROR! We should have found a matching LB entry for this
                    //        font based on TT name.
            }
            else
            {
                //
                //  Put Font name string in ListBox
                // 

                j = SendMessage (hLBox, LB_ADDSTRING, 0, (LONG)pszDesc);

                if (j != LB_ERR)
                    SendMessage (hLBox, LB_SETITEMDATA, j, (LONG) IF_TYPE1);
                // else
                    // ERROR! We found an installed Type1 font but cannot show
                    //        it in listbox because of USER error.
            }

            dwSize = dwPathMax;
        }

        bRet = TRUE;

        RegCloseKey (hkey);
    }

EnumType1Exit:

    if (pszDesc)
        FreeMem (pszDesc, dwPathMax);

    if (pszFontName)
        FreeMem (pszFontName, dwPathMax);

    return bRet;
}


/////////////////////////////////////////////////////////////////////////////
//
// InitProgress
//
//  Create and initialize the Progress dialog.  Initial state is visible.
//
/////////////////////////////////////////////////////////////////////////////

BOOL InitProgress (HWND hwnd)
{
    if (hDlgProgress)
        return TRUE;

    hDlgProgress = CreateDialog (hModule, MAKEINTRESOURCE(DLG_PROGRESS),
                                 hwnd, (DLGPROC)ProgressDlg);

    return (hDlgProgress != NULL);
}


/////////////////////////////////////////////////////////////////////////////
//
// TermProgress
//
//  Remove and cleanup after the Progress dialog.
//
/////////////////////////////////////////////////////////////////////////////

void TermProgress ()
{
    if (hDlgProgress)
        DestroyWindow (hDlgProgress);

    hDlgProgress = NULL;

    return;
}


/////////////////////////////////////////////////////////////////////////////
//
// cpProgressYield
//
//  Allow other messages including Dialog messages for Modeless dialog to be
//  processed while we are converting Type1 files to TrueType.
//
//  Since the font conversion is done on a single thread (in order to keep it
//  synchronous with installation of all fonts) we need to provide a mechanism
//  that will allow a user to Cancel out of the operation and also allow
//  window messages, like WM_PAINT, to be processed by other Window Procedures.
//
/////////////////////////////////////////////////////////////////////////////

VOID cpProgressYield()
{
    MSG msg;

    while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
    {
//        if (!hDlgProgress || !IsDialogMessage (hDlgProgress, &msg))
        if (!IsDialogMessage (hDlgProgress, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// UpdateProgress
//
//   Set the overall progress control in Progress Dialog, along with a
//   message describing installation progress.
//
/////////////////////////////////////////////////////////////////////////////

void UpdateProgress (int iTotalCount, int iFontInstalling, int iProgress)
{
    TCHAR szTemp[120];

    wsprintf (szTemp, szGenErr, iFontInstalling, iTotalCount);
    SetDlgItemText (hDlgProgress, ID_INSTALLMSG, szTemp);

	SendDlgItemMessage (hDlgProgress, ID_OVERALL, SET_PROGRESS,
                        (int) iProgress, 0L);

    //
    //  Process outstanding messages
    //

    cpProgressYield();
}


/////////////////////////////////////////////////////////////////////////////
//
// ResetProgress
//
//   Clear the progress bar control and reset message to NULL
//
/////////////////////////////////////////////////////////////////////////////

void ResetProgress ()
{
    SetDlgItemText (hDlgProgress, ID_PROGRESSMSG, szNull);

	SendDlgItemMessage(hDlgProgress, ID_BAR, SET_PROGRESS, (int) 0, 0L);

    bProgMsgDisplayed = FALSE;
    bProg2MsgDisplayed = FALSE;

    //
    //  Process outstanding messages
    //

    cpProgressYield();
}


/////////////////////////////////////////////////////////////////////////////
//
// Progress
//
//   Progress function for ConvertTypefaceA - Adobe Type1 to TrueType font
//   file converter.  Put up progress in converting font and message
//   describing font being converted.
//
/////////////////////////////////////////////////////////////////////////////

void Progress (short PercentDone, void* UniqueValue)
{
    TCHAR szTemp[120];
    TCHAR szTemp2[120];
    DWORD err = GetLastError(); // save whatever t1instal may have set

    //
    //  UniqueValue is a pointer to the string name of the file being
    //  converted.  Only put this message up if not previously displayed.
    //

    if (!bProgMsgDisplayed)
    {
        LoadString (hModule, MYFONT + 32, szTemp2, CharSizeOf(szTemp2));
        wsprintf (szTemp, szTemp2, (LPTSTR) UniqueValue);
        SetDlgItemText (hDlgProgress, ID_PROGRESSMSG, szTemp);
        bProgMsgDisplayed = TRUE;
    }

	SendDlgItemMessage (hDlgProgress, ID_BAR, SET_PROGRESS,
                         (int) PercentDone, 0L);

    //
    //  Process outstanding messages
    //

    cpProgressYield();

    //
    // reset last error to whatever t1instal set it to:
    //

    SetLastError(err);
}


/////////////////////////////////////////////////////////////////////////////
//
// Progress2
//
//   Progress function for updating progress dialog controls on a per font
//   install basis.
//
/////////////////////////////////////////////////////////////////////////////

void Progress2 (int PercentDone, LPTSTR pszDesc)
{
    TCHAR szTemp[PATHMAX];
    TCHAR szTemp2[240];
    
    //
    //  szDesc is a pointer to the string name of the file being installed.
    //  Only put this message up if not previously displayed.

    if (!bProg2MsgDisplayed)
    {
        LoadString (hModule, MYFONT + 40, szTemp2, CharSizeOf(szTemp2));
        wsprintf (szTemp, szTemp2, pszDesc);
        SetDlgItemText (hDlgProgress, ID_PROGRESSMSG, szTemp);
        bProg2MsgDisplayed = TRUE;
    }

	SendDlgItemMessage (hDlgProgress, ID_BAR, SET_PROGRESS, (int) PercentDone, 0L);

    //
    //  Process outstanding messages
    //

    cpProgressYield();
}


/////////////////////////////////////////////////////////////////////////////
//
// ProgressDlg
//
//  Display progress messages to user based on progress in converting
//  font files to TrueType
//
/////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY ProgressDlg (HWND hDlg, UINT nMsg, DWORD wParam, LONG lParam)
{

    switch (nMsg)
    {

    case WM_INITDIALOG:
        CentreWindow (hDlg);

        //
        //  Load in Progress messages
        //

        LoadString (hModule, MYFONT + 39, szGenErr, CharSizeOf(szGenErr));

        EnableWindow (hDlg, TRUE);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {

        case IDOK:
        case IDCANCEL:
            bCancelInstall = (LOWORD(wParam) == IDCANCEL);

            EndDialog (hDlg, LOWORD(wParam));
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


/////////////////////////////////////////////////////////////////////////////
//
// ProgressBarCtlProc
//
//  Window Procedure for the Progress Bar custom control.  Handles all
//  messages like WM_PAINT just as a normal application window would.
//
/////////////////////////////////////////////////////////////////////////////

LRESULT APIENTRY ProgressBarCtlProc(HWND hWnd, UINT message, WPARAM wParam, LONG lParam)
{
    DWORD   dwProgress;

    dwProgress = (DWORD) GetWindowLong (hWnd, GWL_PROGRESS);

    switch (message)
    {
    case WM_CREATE:
        dwProgress = 0;
        SetWindowLong (hWnd, GWL_PROGRESS, (LONG) dwProgress);
        break;

    case SET_PROGRESS:
        SetWindowLong (hWnd, GWL_PROGRESS, (LONG) wParam);
        InvalidateRect(hWnd, NULL, FALSE);
        UpdateWindow(hWnd);
        break;


    case WM_ENABLE:
        //  Force a repaint since the control will look different.
        InvalidateRect (hWnd, NULL, TRUE);
        UpdateWindow (hWnd);
        break;


    case WM_PAINT:
        return ProgressPaint (hWnd, dwProgress);


    default:
        return (DefWindowProc (hWnd, message, wParam, lParam));
        break;
    }
    return(0L);
}


/////////////////////////////////////////////////////////////////////////////
//
// RegisterProgressClass
//
//
/////////////////////////////////////////////////////////////////////////////

BOOL RegisterProgressClass (HANDLE hModule)
{
    WNDCLASS wcTest;

    wcTest.lpszClassName = TEXT("cpProgress");
    wcTest.hInstance     = hModule;
    wcTest.lpfnWndProc   = ProgressBarCtlProc;
    wcTest.hCursor       = LoadCursor(NULL, IDC_WAIT);
    wcTest.hIcon         = NULL;
    wcTest.lpszMenuName  = NULL;
    wcTest.hbrBackground = (HBRUSH) (rgColorPro[PROGRESSCOLOR_WINDOW]);
    wcTest.style         = CS_HREDRAW | CS_VREDRAW;
    wcTest.cbClsExtra    = 0;
    wcTest.cbWndExtra    = sizeof(DWORD);

    //
    //  Set Bar color to Blue and text color to white
    //

    rgbBG = RGB(  0,   0, 255);
    rgbFG = RGB(255, 255, 255);

    return (RegisterClass((LPWNDCLASS) &wcTest));
}


/////////////////////////////////////////////////////////////////////////////
//
// UnRegisterProgressClass
//
//
/////////////////////////////////////////////////////////////////////////////

VOID UnRegisterProgressClass (HANDLE hModule)
{
    UnregisterClass(TEXT("cpProgress"), hModule);
}


/////////////////////////////////////////////////////////////////////////////
//
// ProgressPaint
//
// Description:
//
//  Handles all WM_PAINT messages for the control and paints
//  the control for the progress state.
//
// Parameters:
//  hWnd            HWND Handle to the control.
//  dwProgress      DWORD Progress amount - between 1 and 100
//
// Return Value:
//  LONG            0L.
//
//
//  This is an alternate way to do the progress bar in the control.  Instead
//  of drawing a rectangle, it uses ExtTextOut to draw the opagueing rect
//  based on the percentage complete.  Clever.
//
/////////////////////////////////////////////////////////////////////////////

LONG ProgressPaint (HWND hWnd, DWORD dwProgress)
{
    PAINTSTRUCT ps;
    HDC         hDC;
    TCHAR   szTemp[20];
    int     dx, dy, len;
    RECT    rc1, rc2;
    SIZE    Size;


    hDC = BeginPaint (hWnd, &ps);

    GetClientRect (hWnd, &rc1);
    FrameRect (hDC, &rc1, GetStockObject(BLACK_BRUSH));
    InflateRect (&rc1, -2, -2);
    rc2 = rc1;

    dx = rc1.right;
    dy = rc1.bottom;

    rc1.right = rc2.left = (dwProgress * dx / 100) + 1;

    //
    //  Boundary condition testing
    //

    if (rc2.left > rc2.right)
        rc2.left = rc2.right;

    len = wsprintf (szTemp, TEXT("%3d%%"), dwProgress);
    GetTextExtentPoint32 (hDC, szTemp, len, &Size);

    SetBkColor (hDC, rgbBG);
    SetTextColor (hDC, rgbFG);
    ExtTextOut (hDC, (dx-Size.cx)/2, (dy-Size.cy)/2,
                ETO_OPAQUE | ETO_CLIPPED, &rc1, szTemp, len, NULL);

    SetBkColor (hDC, rgbFG);
    SetTextColor (hDC, rgbBG);
    ExtTextOut (hDC, (dx-Size.cx)/2, (dy-Size.cy)/2,
                ETO_OPAQUE | ETO_CLIPPED, &rc2, szTemp, len, NULL);

    EndPaint (hWnd, &ps);
    return 0L;

}

#ifdef LATER
/////////////////////////////////////////////////////////////////////////////
//
// ProgressPaint
//
// Description:
//
//  Handles all WM_PAINT messages for the control and paints
//  the control for the progress state.
//
// Parameters:
//  hWnd            HWND Handle to the control.
//  dwProgress      DWORD Progress amount - between 1 and 100
//
// Return Value:
//  LONG            0L.
//
/////////////////////////////////////////////////////////////////////////////

LONG ProgressPaint (HWND hWnd, DWORD dwProgress)
{
    PAINTSTRUCT ps;
    LPRECT      lpRect;
    RECT        rect;
//    RECT        rc2;
    HDC         hDC;
    COLORREF    rgCr[CCOLORS];
    HPEN        rgHPen[CCOLORS];
    int         iColor, len;
    TCHAR       szTemp[20];
    SIZE        Size;

    HBRUSH      hBrushFace;
    HBRUSH      hBrushCtl;

    int         x1, x2, y1, y2;

    if (dwProgress > 100)
        return 0L;

    lpRect = &rect;

    hDC = BeginPaint (hWnd, &ps);

    //
    //  Get colors that we'll need.  We do not want to cache these
    //  items since we may our top-level parent window may have
    //  received a WM_WININICHANGE message at which time the control
    //  is repainted.  Since this control never sees that message,
    //  we cannot assume that colors will remain the same throughout
    //  the life of the control.
    //

    for (iColor = 0; iColor < CCOLORS; iColor++)
    {
        rgCr[iColor] = GetSysColor (rgColorPro[iColor]);

        rgHPen[iColor] = CreatePen (PS_SOLID, 1, rgCr[iColor]);
    }

    //
    //  Draw outer frame of Control and adjust size
    //

    GetClientRect (hWnd, lpRect);
    FrameRect(hDC, lpRect, GetStockObject (BLACK_BRUSH));

    hBrushFace  = CreateSolidBrush (rgCr[PROGRESSCOLOR_FACE]);

    InflateRect (lpRect, -1, -1);

#ifdef OLD
    //
    //  Draw the face color and the outer frame
    //

    SelectObject (hDC, hBrushFace);
    SelectObject (hDC, rgHPen[PROGRESSCOLOR_FRAME]);
#endif  // OLD

    //
    //  Draw 3D Progress bar within the control
    //

    Draw3DRect (hDC, hBrushFace, rgHPen[PROGRESSCOLOR_FRAME],
                rgHPen[PROGRESSCOLOR_HIGHLIGHT],
                rgHPen[PROGRESSCOLOR_SHADOW],
                lpRect->left, lpRect->top,
                lpRect->right * dwProgress / 100, lpRect->bottom);

    //
    //  Now draw the rest of the control rectangle in Window color
    //  (this will erase any text from last call)
    //

    x1 = (lpRect->right * dwProgress / 100)+ 1;

    if (x1 > lpRect->right)
        x1= lpRect->right;

    y1 = lpRect->top + 1;
    x2 = lpRect->right - 1;
    y2 = lpRect->bottom - 1;

    hBrushCtl = CreateSolidBrush (rgCr[PROGRESSCOLOR_WINDOW]);
    SelectObject (hDC, hBrushCtl);
    SelectObject (hDC, rgHPen[PROGRESSCOLOR_WINDOW]);

    Rectangle (hDC, x1, y1, x2, y2);

    //
    //  Now draw text % centered with-in the rectangle
    //

    len = wsprintf (szTemp, TEXT("%3d%%"), dwProgress);
    GetTextExtentPoint32 (hDC, szTemp, len, &Size);

#ifdef OLD
    rc2.left   = (lpRect->right - Size.cx) / 2;
    rc2.top    = (lpRect->bottom - Size.cy) / 2;
    rc2.right  = rc2.left + Size.cx;
    rc2.bottom = rc2.top + Size.cy;
#endif  // OLD

    SetBkMode (hDC, TRANSPARENT);
    SetTextColor (hDC, GetSysColor(COLOR_BTNTEXT));
    ExtTextOut (hDC, (lpRect->right-Size.cx)/2, (lpRect->bottom-Size.cy)/2,
                0L, lpRect, szTemp, len, NULL);

    //
    //  Clean up
    //

    EndPaint(hWnd, &ps);

    DeleteObject (hBrushFace);
    DeleteObject (hBrushCtl);

    for (iColor = 0; iColor < CCOLORS; iColor++)
    {
        if (rgHPen[iColor])
            DeleteObject (rgHPen[iColor]);
    }

    return 0L;
}


/////////////////////////////////////////////////////////////////////////////
//
// Draw3DRect
//
// Description:
//  Draws the 3D button look within a given rectangle.  This rectangle
//  is assumed to be bounded by a one pixel black border, so everything
//  is bumped in by one.
//
// Parameters:
//  hDC         DC to draw to.
//  hBrushFace  HBRUSH rectangle fill color brush.
//  hPenFrame   HPEN rectangle frame color pen.
//  hPenHigh    HPEN highlight color pen.
//  hPenShadow  HPEN shadow color pen.
//  x1          int Upper left corner x.
//  y1          int Upper left corner y.
//  x2          int Lower right corner x.
//  y2          int Lower right corner y.
//
//  NOTE:  hBrushFace and hPenFrame are usually the same color, only one
//         is a Pen used to draw the rectangle frame and the brush color
//         is used to fill the rectangle.  Then the highlight and shadow
//         pens are used to create the 3D effect.
//
// Return Value:
//  void
//
/////////////////////////////////////////////////////////////////////////////

void Draw3DRect (HDC hDC, HBRUSH hBrushFace, HPEN hPenFrame,
                 HPEN hPenHigh, HPEN hPenShadow,
                 int x1, int y1, int x2, int y2)
{
    HPEN   hPenOrg;
    RECT   rect;

    if (x1 < 0)
        x1 = 0;

    if (x2 < 1)
        x2 = 1;

    if (y1 < 1)
        y1 = 1;

    if (y2 < 0)
        y2 = 0;

    if (!(x1 < x2))
        x1 = x2;

    if (!(y1 < y2))
        y1 = y2;

    //
    //  Draw the face color and the rectangle frame
    //

    SelectObject (hDC, hBrushFace);
    hPenOrg = SelectObject (hDC, hPenFrame);

    Rectangle (hDC, x1, y1, x2, y2);

    //
    //  Shrink the rectangle to account for borders.
    //

    x1+=1;
    x2-=1;
    y1+=1;
    y2-=1;

    if (x2 < 1)
        x2 = 1;

    if (y2 < 1)
        y2 = 1;

    if (!(x1 < x2))
        x1 = x2;

    if (!(y1 < y2))
        y1 = y2;

    SelectObject (hDC, hPenShadow);

    //
    //  Lowest shadow line.
    //

    MoveToEx (hDC, x1, y2, NULL);
    LineTo (hDC, x2, y2);
    LineTo (hDC, x2, y1-1);

    //
    //  Upper shadow line.
    //

    MoveToEx (hDC, x1+1, y2-1, NULL);
    LineTo (hDC, x2-1, y2-1);
    LineTo (hDC, x2-1, y1);

    SelectObject (hDC, hPenHigh);

    //
    //  Upper highlight line.
    //

    MoveToEx (hDC, x1, y2-1, NULL);
    LineTo (hDC, x1, y1);
    LineTo (hDC, x2, y1);

    if (hPenOrg)
        SelectObject (hDC, hPenOrg);

    return;
}


//  If using the ExtTextOut method for progress ctl, these globals are needed
DWORD   rgbFG;
DWORD   rgbBG;

/////////////////////////////////////////////////////////////////////////////
//
// ProgressPaint2
//
// Description:
//
//  Handles all WM_PAINT messages for the control and paints
//  the control for the progress state.
//
// Parameters:
//  hWnd            HWND Handle to the control.
//  dwProgress      DWORD Progress amount - between 1 and 100
//
// Return Value:
//  LONG            0L.
//
//
//  This is an alternate way to do the progress bar in the control.  Instead
//  of drawing a rectangle, it uses ExtTextOut to draw the opagueing rect
//  based on the percentage complete.  Clever.
//
/////////////////////////////////////////////////////////////////////////////

LONG ProgressPaint2 (HWND hWnd, DWORD dwProgress)
{
    PAINTSTRUCT ps;
    HDC         hDC;
    TCHAR   szTemp[20];
    int     dx, dy, len;
    RECT    rc1, rc2;
    SIZE    Size;
    DWORD   rgbFG;
    DWORD   rgbBG;


    rgbBG = RGB(  0,   0, 255);
    rgbFG = RGB(255, 255, 255);

    hDC = BeginPaint (hWnd, &ps);

    GetClientRect (hWnd, &rc1);
    FrameRect (hDC, &rc1, GetStockObject(BLACK_BRUSH));
    InflateRect (&rc1, -1, -1);
    rc2 = rc1;

    dx = rc1.right;
    dy = rc1.bottom;

    rc1.right = rc2.left = (dwProgress * dx / 100) + 1;

    len = wsprintf (szTemp, TEXT("%3d%%"), dwProgress);
    GetTextExtentPoint32 (hDC, szTemp, len, &Size);

    SetBkColor (hDC, rgbBG);
    SetTextColor (hDC, rgbFG);
    ExtTextOut (hDC, (dx-Size.cx)/2, (dy-Size.cy)/2,
                ETO_OPAQUE | ETO_CLIPPED, &rc1, szTemp, len, NULL);

    SetBkColor (hDC, rgbFG);
    SetTextColor (hDC, rgbBG);
    ExtTextOut (hDC, (dx-Size.cx)/2, (dy-Size.cy)/2,
                ETO_OPAQUE | ETO_CLIPPED, &rc2, szTemp, len, NULL);

    EndPaint (hWnd, &ps);
    return 0L;

}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#endif  // LATER



