///////////////////////////////////////////////////////////////////////////////
//
// t1.cpp
//      Explorer Font Folder extension routines
//
//
// History:
//      31 May 95 SteveCat
//          Ported to Windows NT and Unicode, cleaned up
//
//
// NOTE/BUGS
//
//  Copyright (C) 1992-1995 Microsoft Corporation
//
///////////////////////////////////////////////////////////////////////////////

//==========================================================================
//                              Include files
//==========================================================================
// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

// Application specific

#undef IN
#include "t1instal.h"

#define CONST const

#include "priv.h"
#include "globals.h"
#include "fontcl.h"
#include "resource.h"
#include "ui.h"
#include "cpanel.h"
#include "fontman.h"


extern FullPathName_t  s_szSharedDir;

TCHAR c_szDescFormat[] = TEXT( "%s (%s)" );
TCHAR c_szPostScript[] = TEXT( "Type 1" );

TCHAR szFonts[] = TEXT( "fonts" );

char  g_szFontsDirA[ PATHMAX ];           //  ANSI string!

TCHAR m_szMsgBuf[ PATHMAX ];

TCHAR szTTF[ ] = TEXT(".TTF");
TCHAR szFON[ ] = TEXT(".FON");
TCHAR szPFM[ ] = TEXT(".PFM");
TCHAR szPFB[ ] = TEXT(".PFB");

typedef const void (__stdcall *PROGRESSPROC)( short, void *);

#undef T1_SUPPORT

BOOL RegisterProgressClass( void );
VOID UnRegisterProgressClass( void );

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if defined(T1_SUPPORT)


#include "t1.h"
#include "cpanel.h"  // for bUniqueOnSharedDir()


typedef short (__stdcall *CONVERTTYPEFACEPROC) ( char *, char *, char *, PROGRESSPROC, void *);
typedef BOOL  (__stdcall *CHECKTYPE1PROC)( char *, DWORD, char *, DWORD, char *, DWORD, char *, BOOL *);
typedef short (__stdcall *CHECKCOPYRIGHTPROC)( char *, DWORD, char *);

CONVERTTYPEFACEPROC lpConvertTypefaceA = 0;
CHECKTYPE1PROC      lpCheckType1A = 0;
CHECKCOPYRIGHTPROC  lpCheckCopyrightA = 0;



// --------------------------------------------------------------------
// The Following functions support Type 1 to TrueType conversion.
// --------------------------------------------------------------------

BOOL bInitType1( )
{
    static BOOL bTriedOne = FALSE;
    static HINSTANCE g_hType1;

    if( !g_hType1 && !bTriedOne )
    {
        g_hType1 = LoadLibrary( TEXT( "T1INSTAL.DLL" ) );
        bTriedOne = TRUE;

        if( g_hType1 )
        {
            lpConvertTypefaceA = (CONVERTTYPEFACEPROC) GetProcAddress( g_hType1, "ConvertTypefaceA" );
            lpCheckType1A = (CHECKTYPE1PROC) GetProcAddress( g_hType1, "CheckType1A" );
            lpCheckCopyrightA = (CHECKCOPYRIGHTPROC) GetProcAddress( g_hType1, "CheckCopyrightA" );

            if( !lpConvertTypefaceA || !lpCheckType1A || !lpCheckCopyrightA )
                g_hType1 = FALSE;
        }
    }

    return( g_hType1 != 0 );
}


BOOL NEAR PASCAL bIsType1( LPSTR        lpFile,
                           FontDesc_t * lpDesc,
                           LPT1_INFO    lpInfo )
{
    BOOL bRet = FALSE;
    T1_INFO  t1_info;

    if( bInitType1( ) )
    {
        if( !lpInfo )
            lpInfo = &t1_info;

        bRet = (*lpCheckType1A)( lpFile,
                                 sizeof( FontDesc_t ), (char *)lpDesc,
                                 sizeof( lpInfo->pfm ), (char *)&lpInfo->pfm,
                                 sizeof( lpInfo->pfb ), (char *)&lpInfo->pfb,
                                 &lpInfo->bCreatedPFM );
    }

    return bRet;
}


BOOL NEAR PASCAL bConvertT1( LPT1_INFO lpInfo )
{
    BOOL    bRet = FALSE;
    char  szVendor[ 128 ];

    if( bInitType1( ) )
    {
        short nRet = (*lpCheckCopyrightA)( lpInfo->pfb, sizeof( szVendor ), szVendor );

        if( nRet >= 0 )
        {
            //
            //  Get a TT file name to convert it to and convert it.
            //

            if( bUniqueOnSharedDir( lpInfo->ttf, "CvtT1.ttf" ) )
            {
                bRet = (*lpConvertTypefaceA)( lpInfo->pfb, lpInfo->pfm,
                                              lpInfo->ttf, 0, 0 );
            }
        }
    }

    return bRet;
}

#endif   // T1_SUPPORT



#ifdef WINNT

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
#define PROGRESSCOLOR_BAR         6
#define PROGRESSCOLOR_TEXT        7

#define CCOLORS                   8

#define CHAR_BACKSLASH  TEXT( '\\' )
#define CHAR_COLON      TEXT( ':' )
#define CHAR_NULL       TEXT( '\0' )
#define CHAR_TRUE       TEXT( 'T' )
#define CHAR_FALSE      TEXT( 'F' )

//==========================================================================
//                       External Declarations
//==========================================================================

extern HWND  hLBoxInstalled;

//==========================================================================
//                       Local Data Declarations
//==========================================================================

BOOL bYesAll_PS = FALSE;        //  Use global state for all PS fonts
BOOL bConvertPS = TRUE;         //  Convert Type1 files to TT
BOOL bInstallPS = TRUE;         //  Install PS files
BOOL bCopyPS    = TRUE;         //  Copy PS files to Windows dir

BOOL bCancelInstall = FALSE;    // Global installation cancel

TCHAR szTrue[]  = TEXT( "T" );
TCHAR szFalse[] = TEXT( "F" );
TCHAR szHash[]  = TEXT( "#" );

BOOL bProgMsgDisplayed;         //  Used by Progress to avoid msg flicker
BOOL bProg2MsgDisplayed;        //  Used by Progress2 to avoid msg flicker

HWND hDlgProgress = NULL;

//
//  Used to determine Foreground/Backgnd colors for progress bar control
//  Global values are set at RegisterClass time
//

DWORD   rgbFG;
DWORD   rgbBG;

//  Registry location for installing PostScript printer font info

TCHAR g_szType1Key[] = TEXT( "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Type 1 Installer\\Type 1 Fonts" );

//
//  Array of default colors, matching the order of PROGRESSCOLOR_* values.
//

DWORD rgColorPro[ CCOLORS ] = {
                         COLOR_BTNFACE,             //  PROGRESSCOLOR_FACE
                         COLOR_BTNTEXT,             //  PROGRESSCOLOR_ARROW
                         COLOR_BTNSHADOW,           //  PROGRESSCOLOR_SHADOW
                         COLOR_BTNHIGHLIGHT,        //  PROGRESSCOLOR_HIGHLIGHT
                         COLOR_WINDOWFRAME,         //  PROGRESSCOLOR_FRAME
                         COLOR_WINDOW,              //  PROGRESSCOLOR_WINDOW
                         COLOR_ACTIVECAPTION,       //  PROGRESSCOLOR_BAR
                         COLOR_CAPTIONTEXT          //  PROGRESSCOLOR_TEXT
                         };

typedef struct _T1_INSTALL_OPTIONS
{
    BOOL        bMatchingTT;
    BOOL        bOnlyPSInstalled;
    LPTSTR      szDesc;
    WORD        wFontType;
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

BOOL CheckTTInstall( LPTSTR szDesc );
void Draw3DRect( HDC hDC, HBRUSH hBrushFace, HPEN hPenFrame, HPEN hPenHigh,
                 HPEN hPenShadow, int x1, int y1, int x2, int y2 );
INT_PTR APIENTRY InstallPSDlg( HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam );
void STDCALL  Progress( short PercentDone, void* UniqueValue );
LRESULT APIENTRY ProgressBarCtlProc( HWND hTest, UINT message, WPARAM wParam, LPARAM lParam );
INT_PTR APIENTRY ProgressDlg( HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam );
INT_PTR APIENTRY CopyrightNotifyDlgProc( HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam );
LONG ProgressPaint( HWND hWnd, DWORD dwProgress );


//==========================================================================
//                           Functions
//==========================================================================

/////////////////////////////////////////////////////////////////////////////
//
// StripFilespec
//
//   Remove the filespec portion from a path (including the backslash).
//
/////////////////////////////////////////////////////////////////////////////

VOID StripFilespec( LPTSTR lpszPath )
{
   LPTSTR     p;

   p = lpszPath + lstrlen( lpszPath );

   while( ( *p != CHAR_BACKSLASH )  && ( *p != CHAR_COLON )  && ( p != lpszPath ) )
      p--;

   if( *p == CHAR_COLON )
      p++;

   //
   //  Don't strip backslash from root directory entry.
   //

   if( p != lpszPath )
   {
      if( ( *p == CHAR_BACKSLASH )  && ( *( p - 1 )  == CHAR_COLON ) )
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

VOID StripPath( LPTSTR lpszPath )
{
  LPTSTR     p;


  p = lpszPath + lstrlen( lpszPath );

  while( ( *p != CHAR_BACKSLASH )  && ( *p != CHAR_COLON )  && ( p != lpszPath ) )
      p--;

  if( p != lpszPath )
      p++;

  if( p != lpszPath )
      lstrcpy( lpszPath, p );
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
//  (Insert item into linked list )
//
//
/////////////////////////////////////////////////////////////////////////////

BOOL AddVendorCopyright( LPTSTR pszCopyright, int iResponse )
{
    PSVENDOR *pVendor;          //  temp pointer to linked list


    //
    //  Make the new PSVENDOR node and add it to the linked list.
    //

    if( pFirstVendor )
    {
        pVendor = (PSVENDOR *) AllocMem( sizeof( PSVENDOR ) );

        if( pVendor )
        {
            pVendor->pNext = pFirstVendor;
            pFirstVendor = pVendor;
        }
        else
            return FALSE;
    }
    else        // First time thru
    {
        pFirstVendor = (PSVENDOR *) AllocMem( sizeof( PSVENDOR ) );

        if( pFirstVendor )
            pFirstVendor->pNext = NULL;
        else
            return FALSE;
    }

    //
    //  Save User response and Copyright string
    //

    pFirstVendor->iResponse = iResponse;

    pFirstVendor->pszCopyright = AllocStr( pszCopyright );

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

int CheckVendorCopyright( LPTSTR pszCopyright )
{
    PSVENDOR *pVendor;          // temp pointer to linked list

    //
    //  Traverse the list, testing each node for matching copyright string
    //

    pVendor = pFirstVendor;

    while( pVendor )
    {
        if( !lstrcmpi( pVendor->pszCopyright, pszCopyright ) )
            return( pVendor->iResponse );

        pVendor = pVendor->pNext;
    }

    //
    //  "Did not find matching copyright" return
    //

    return( -1 );
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: CopyrightNotifyDlgProc
//
// DESCRIP:  Display the dialog informing the user about possible
//           Type1 font copyright problems.
//
// ARGS:     lParam is the address of an array of text string pointers.
//           Element 0 is the name of the font.
//           Element 1 is the name of the vendor.
//
///////////////////////////////////////////////////////////////////////////////
enum arg_nums{ARG_FONTNAME = 0, ARG_VENDORNAME};
INT_PTR APIENTRY CopyrightNotifyDlgProc( HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam )
{

    switch( nMsg )
    {
        case WM_INITDIALOG:
        {
            LPCTSTR *lpaszTextItems   = (LPCTSTR *)lParam;
            LPCTSTR lpszVendorName    = NULL;
            TCHAR szUnknownVendor[80] = { TEXT('\0') };

            ASSERT(NULL != lpaszTextItems);
            ASSERT(NULL != lpaszTextItems[ARG_FONTNAME]);
            ASSERT(NULL != lpaszTextItems[ARG_VENDORNAME]);

            //
            // Set font name string.
            //
            SetWindowText(GetDlgItem(hDlg, IDC_COPYRIGHT_FONTNAME),
                                           lpaszTextItems[ARG_FONTNAME]);

            //
            // Set vendor name string.  If name provided is blank, use a default
            // string of "Unknown Vendor Name" from string table.
            //
            if (TEXT('\0') == *lpaszTextItems[ARG_VENDORNAME])
            {
                UINT cchLoaded = 0;
                cchLoaded = LoadString(g_hInst, IDSI_UNKNOWN_VENDOR,
                                       szUnknownVendor, ARRAYSIZE(szUnknownVendor));

                ASSERT(cchLoaded > 0); // Complain if someone removed resource string.

                lpszVendorName = szUnknownVendor;
            }
            else
                lpszVendorName = lpaszTextItems[ARG_VENDORNAME];

            SetWindowText(GetDlgItem(hDlg, IDC_COPYRIGHT_VENDORNAME), lpszVendorName),
            CentreWindow( hDlg );
            break;
        }

    case WM_COMMAND:
        switch( LOWORD( wParam ) )
        {
            case IDYES:
            case IDNO:
                //
                // Dialog Proc must return IDYES or IDNO when a button
                // is selected.  This is the value stored in the Vendor Copyright
                // list.
                //
                EndDialog(hDlg, LOWORD(wParam));
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

//
// OkToConvertType1ToTrueType
//
// This function checks the authorization for converting a Type1
// font to it's TrueType equivalent.  Authorization information is stored
// in the registry under the section ...\Type 1 Installer\Copyrights.
//
// If authorization is GRANTED, the function returns TRUE and the font may be
// converted to TrueType.
//
// If authorization is explicitly DENIED and the user has not selected
// "Yes to All" in the Type1 options dialog, a message box is displayed
// informing the user of the denial and the function returns FALSE
//
// If no authorization information is found in the registry for this vendor,
// a dialog box is displayed warning the user about possible copyright
// violations.  The user can answer Yes to convert the font anyway or No
// to skip the conversion.  This response is stored in memory on a per-vendor
// basis so that the user won't have to respond to this same question if
// a font from this vendor is encountered again.
//
BOOL OkToConvertType1ToTrueType(LPCTSTR pszFontDesc, LPCTSTR pszPFB, HWND hwndParent)
{
    char    szCopyrightA[MAX_PATH]; // For string from Type1 (ANSI) file.
#ifdef UNICODE
    WCHAR   szCopyrightW[MAX_PATH]; // For display in UNICODE build.
    char    szPfbA[MAX_PATH];       // For arg to CheckCopyrightA (ANSI).
#endif
    LPTSTR  pszCopyright = NULL;    // Ptr to Copyright string (A or W).
    LPSTR   pszPfbA      = NULL;    // Ptr to ANSI PFB filename string.
    DWORD   dwStatus     = 0;       // Temp result variable.
    BOOL    bResult      = FALSE;   // Function return value.

    //
    //  Check convertability of this font from Type1 to TrueType.
    //
    //  Returns: SUCCESS, FAILURE, MAYBE
    //
#ifdef UNICODE
    WideCharToMultiByte(CP_ACP, 0, pszPFB, -1, szPfbA, ARRAYSIZE(szPfbA), NULL, NULL);
    pszPfbA = szPfbA;
#else
    pszPfbA = pszPFB;
#endif

    dwStatus = CheckCopyrightA(pszPfbA, ARRAYSIZE(szCopyrightA), szCopyrightA);
    if (SUCCESS == dwStatus)
    {
        bResult = TRUE;
    }
    else
    {

#ifdef UNICODE
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szCopyrightA, -1,
                                         szCopyrightW, ARRAYSIZE(szCopyrightW));
        pszCopyright = szCopyrightW;
#else
        pszCopyright = szCopyrightA;
#endif

        switch(dwStatus)
        {

            case FAILURE:

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
                if (!bYesAll_PS)
                {

                    iUIMsgBox( hwndParent,
                               MYFONT + 2, IDS_MSG_CAPTION,
                               MB_OK | MB_ICONEXCLAMATION,
                               (LPTSTR)  pszCopyright,
                               (LPTSTR)  pszFontDesc );
                }
                break;

            case MAYBE:
                //
                //  Check font copyright and ask for user response if necessary
                //
                switch(CheckVendorCopyright(pszCopyright))
                {
                    case IDYES:
                        //
                        // User previously responded "Yes" for converting fonts
                        // from this vendor.  Automatic approval.
                        //
                        bResult = TRUE;
                        break;

                    case IDNO:
                        //
                        // User previously responded "No" for converting fonts
                        // from this vendor.  Automatic denial.
                        //
                        bResult = FALSE;
                        break;

                    case -1:
                    default:
                    {
                        //
                        // No previous record of having asked user about this vendor.
                        //
                        INT iResponse = IDNO;

                        //
                        // Warn user about possible copyright problems.
                        // Ask if they want to convert to TrueType anyway.
                        //
                        LPCTSTR lpszDlgTextItems[] = {pszFontDesc, pszCopyright};

                        iResponse = (INT)DialogBoxParam(g_hInst,
                                                   MAKEINTRESOURCE(DLG_COPYRIGHT_NOTIFY),
                                                   hwndParent ? hwndParent : HWND_DESKTOP,
                                                   CopyrightNotifyDlgProc,
                                                   (LPARAM)lpszDlgTextItems);
                        //
                        // Remember this response for this vendor.
                        //
                        AddVendorCopyright(pszCopyright, iResponse);

                        //
                        // Translate user response to a T/F return value.
                        //
                        bResult = (iResponse == IDYES);

                        break;
                    }
                }
                break;

            default:
                //
                //  ERROR! from routine - assume worst case
                //
                break;
        }
    }
    return bResult;
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
//                   If valid pointer on entry...
//                   *bpCreatedPFM == TRUE means check for existing PFM.
//                                    FALSE means don't check for PFM.
//    lpdwStatus     Address of caller status variable.  Use this value
//                   to determine cause of FALSE return value.
//
// NOTE: Assumes that lpszPfm and lpszPfb are of size PATHMAX & lpszDesc is
//       of size DESCMAX
//
// returns:
//    TRUE success, FALSE failure
//
// The following table lists the possible font verification status codes
// returned in *lpdwStatus along with a brief description of the cause for each.
// See fvscodes.h for numeric code value details.
//
//
//          FVS_SUCCESS
//          FVS_FILE_OPEN_ERR
//          FVS_BUILD_ERR
//          FVS_FILE_EXISTS
//          FVS_INSUFFICIENT_BUF
//          FVS_INVALID_FONTFILE
//          FVS_BAD_VERSION
//          FVS_FILE_IO_ERR
//          FVS_EXCEPTION
//
/////////////////////////////////////////////////////////////////////////////

BOOL IsPSFont( LPTSTR lpszKey,
               LPTSTR lpszDesc,         //  Optional
               LPTSTR lpszPfm,          //  Optional
               LPTSTR lpszPfb,          //  Optional
               BOOL  *pbCreatedPFM,     //  Optional
               LPDWORD lpdwStatus )     //  May be NULL
{
    BOOL    bRet = FALSE;
    TCHAR   strbuf[ PATHMAX ] ;
    BOOL    bPFM;

    //
    //  ANSI buffers for use with ANSI only API's
    //

    char    *desc, Descbuf[ PATHMAX ] ;
    char    Keybuf[ PATHMAX ] ;
    char    *pfb, Pfbbuf[ PATHMAX ] ;
    char    *pfm, Pfmbuf[ PATHMAX ] ;
    DWORD   iDesc, iPfb, iPfm;
    DWORD   dwStatus = FVS_MAKE_CODE(FVS_SUCCESS, FVS_FILE_UNK);

    //
    // Initialize status return.
    //
    if (NULL != lpdwStatus)
        *lpdwStatus = FVS_MAKE_CODE(FVS_INVALID_STATUS, FVS_FILE_UNK);


    if( lpszDesc )
        *lpszDesc = (TCHAR)  0;

    desc = Descbuf;
    iDesc = PATHMAX;

    pfb = Pfbbuf;
    iPfb = PATHMAX;

    if( lpszPfm )
    {
        pfm = Pfmbuf;
        iPfm = PATHMAX;
    }
    else
    {
        pfm = NULL;
        iPfm = 0;
    }

    if( pbCreatedPFM )
    {
        bPFM = *pbCreatedPFM;  // Caller says if CheckType1WithStatusA checks for dup PFM
        *pbCreatedPFM = FALSE;
    }
    else
        bPFM = TRUE;  // By default, CheckType1WithStatusA should check for dup PFM.

    WideCharToMultiByte( CP_ACP, 0, lpszKey, -1, Keybuf, PATHMAX, NULL, NULL );

    //
    //  The CheckType1WithStatusA routine accepts either a .INF or .PFM file name as
    //  the Keybuf( i.e. Key file )   input parameter.  If the input is a .INF
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
    //  The bPFM flag is used to tell CheckType1WithStatusA if it should check for an
    //  existing PFM file.  If invoked as part of a drag-drop install, we
    //  don't want to do this check.  If from install dialog, do the check.
    //

    dwStatus = ::CheckType1WithStatusA( Keybuf, iDesc, desc, iPfm, pfm, iPfb, pfb, &bPFM, g_szFontsDirA );

    if( FVS_STATUS(dwStatus) == FVS_SUCCESS)
    {
        if( pbCreatedPFM )
            *pbCreatedPFM = bPFM;

        //
        //  Return Font description
        //

        if( lpszDesc )
        {
#ifdef UNICODE
            //
            //  Convert Descbuf to UNICODE since Type1 files are ANSI
            //
            MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, Descbuf, -1, strbuf, PATHMAX );
            vCPStripBlanks(strbuf);
            wsprintf(lpszDesc, c_szDescFormat, strbuf, c_szPostScript );
#else
            vCPStripBlanks(Descbuf);
            wsprintf(lpszDesc, c_szDescFormat, Descbuf, c_szPostScript );
#endif
        }

        //
        //  Return PFM file name
        //

        if( lpszPfm )
        {
            //
            //  Return PFM file name - convert to UNICODE since Type1 files are ANSI
            //
#ifdef UNICODE
            MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, Pfmbuf, -1, lpszPfm, PATHMAX );
#else
            lstrcpy(lpszPfm, Pfmbuf);
#endif
        }

        //
        //  Return PFB file name
        //

        if( lpszPfb )
        {
            //
            //  Return PFB file name - convert to UNICODE since Type1 files are ANSI
            //
#ifdef UNICODE
            MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, Pfbbuf, -1, lpszPfb, PATHMAX );
#else
            lstrcpy(lpszPfb, Pfbbuf);
#endif
        }

        bRet = TRUE;
    }

    //
    // Return status if user wants it.
    //
    if (NULL != lpdwStatus)
       *lpdwStatus = dwStatus;

    return bRet;
}


/////////////////////////////////////////////////////////////////////////////
//
// InitPSInstall
//
//  Initialize PostScript install routine global variables.
//
/////////////////////////////////////////////////////////////////////////////

void InitPSInstall( )
{
    CFontManager *poFontManager;
    if (SUCCEEDED(GetFontManager(&poFontManager)))
    {
        //
        //  Initialize linked list variables for "MAYBE" copyright vendor list
        //
        pFirstVendor = NULL;

        //
        //  Other installation globals
        //

        bYesAll_PS = FALSE;

        //
        // If the native ATM driver is installed, we NEVER convert from Type1 to
        // TrueType.
        //
        bConvertPS = !poFontManager->Type1FontDriverInstalled();

        bInstallPS = TRUE;
        bCopyPS    = TRUE;
        ReleaseFontManager(&poFontManager);
    }        

    return;
}


/////////////////////////////////////////////////////////////////////////////
//
// TermPSInstall
//
//  Initialize PostScript install routine global variables.
//
/////////////////////////////////////////////////////////////////////////////

void TermPSInstall( )
{

    PSVENDOR *pVendor;

    //
    //  Traverse the list, freeing list memory and strings.
    //

    pVendor = pFirstVendor;

    while( pVendor )
    {
        pFirstVendor = pVendor;
        pVendor = pVendor->pNext;

        if( pFirstVendor->pszCopyright )
            FreeStr( pFirstVendor->pszCopyright );

        FreeMem( (LPVOID)  pFirstVendor, sizeof( PSVENDOR ) );
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

int InstallT1Font( HWND   hwndParent,
                   BOOL   bCopyTTFile,          //  Copy TT file?
                   BOOL   bCopyType1Files,      //  Copy PFM/PFB to font folder?
                   BOOL   bInSharedDir,         //  Files in Shared Directory?
                   LPTSTR szKeyName,            //  IN: PFM/INF Source File name & dir
                                                //  OUT: Destination file name
                   LPTSTR szDesc )              //  INOUT: Font description

{
    WORD   wFontType = NOT_TT_OR_T1;            //  Enumerated Font type
    int    rc, iRet;
    WORD   wMsg;
    BOOL   bCreatedPfm = FALSE;  // F = IsPSFont doesn't check for existing PFM

    TCHAR  szTemp[    PATHMAX ] ;
    TCHAR  szTemp2[   PATHMAX ] ;
    TCHAR  szPfbName[ PATHMAX ] ;
    TCHAR  szPfmName[ PATHMAX ] ;
    TCHAR  szSrcDir[  PATHMAX ] ;
    TCHAR  szDstName[ PATHMAX ] ;
    TCHAR  szTTFName[ PATHMAX ] ;
    TCHAR  *pszArg1, *pszArg2;

    T1_INSTALL_OPTIONS t1ops;

    //
    //  ASCII Buffers for use in ASCII-only api calls
    //

    char  pfb[ PATHMAX ] ;
    char  pfm[ PATHMAX ] ;
    char  ttf[ PATHMAX ] ;

    DWORD dwStatus = FVS_MAKE_CODE(FVS_SUCCESS, FVS_FILE_UNK);


    //////////////////////////////////////////////////////////////////////
    //
    //  Check if font is already loaded on the system
    //
    //////////////////////////////////////////////////////////////////////

    t1ops.bOnlyPSInstalled = FALSE;
    t1ops.bMatchingTT      = FALSE;

    //
    //  Check both Type1 & Fonts registry location for prior font installation
    //

    if( CheckT1Install( szDesc, NULL ) )
    {
        //
        // "Font is already loaded"
        //
        iRet = iUIMsgOkCancelExclaim( hwndParent,
                                      MYFONT + 5,
                                      IDS_MSG_CAPTION,
                                      (LPTSTR)szDesc);
        //
        // Return without deleting the PFM file.
        // Because this font is a duplicate, the PFM
        // already existed.
        //
        goto MasterExit;
    }
    else if( CheckTTInstall( szDesc ) )
    {
        t1ops.bMatchingTT = TRUE;

        if( !bYesAll_PS )
        {
            //
            // "The TrueType version of this font is already installed."
            //

            switch( iUIMsgBox( hwndParent,
                               MYFONT + 4, IDS_MSG_CAPTION,
                               MB_YESNOCANCEL | MB_ICONEXCLAMATION,
                               (LPTSTR) szDesc ) )
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
    // BUGBUG [brianau]
    // If we're installing from a .INF/.AFM pair, this function automatically
    // creates a new .PFM file in the fonts directory, even if the .PFM file
    // already exists.  This could cause a possible file mismatch between an
    // existing .PFB and the new .PFM.
    //
    if(::IsPSFont( szKeyName, (LPTSTR) NULL, szPfmName, szPfbName,
                     &bCreatedPfm, &dwStatus ))
    {
        CFontManager *poFontManager;
        if (SUCCEEDED(GetFontManager(&poFontManager)))
        {
            wFontType = TYPE1_FONT;

            lstrcpyn(szTemp, szDesc, ARRAYSIZE(szTemp));
            RemoveDecoration(szTemp, TRUE);
            if (poFontManager->Type1FontDriverInstalled() ||
                !OkToConvertType1ToTrueType(szTemp,
                                            szPfbName,
                                            hwndParent))
            {
                wFontType = TYPE1_FONT_NC;
            }
            ReleaseFontManager(&poFontManager);
        }
    }
    else
    {
        if (iUIMsgBoxInvalidFont(hwndParent, szKeyName, szDesc, dwStatus) == IDCANCEL)
           iRet = TYPE1_INSTALL_IDCANCEL;
        else
           iRet = TYPE1_INSTALL_IDNO;
        goto InstallPSFailure;
    }

    t1ops.szDesc = szDesc;
    t1ops.wFontType = wFontType;

    //
    //  Keep a copy of source directory
    //

    lstrcpy( szSrcDir, szKeyName );

    StripFilespec( szSrcDir );

    lpCPBackSlashTerm( szSrcDir );
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

    if( bYesAll_PS )
    {
        //
        //  If the PS version of this font is already installed AND the
        //  global bInstall == TRUE, then the globals are out-of-sync
        //  with this font.  Let user know and continue installation.
        //

        if( t1ops.bOnlyPSInstalled && bInstallPS )
        {
            //
            // "The Type 1 version of this font is already installed."
            //

            switch( iUIMsgBox( hwndParent,
                               MYFONT + 3, IDS_MSG_CAPTION,
                               MB_YESNOCANCEL | MB_ICONEXCLAMATION,
                               (LPTSTR) szDesc ) )
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

        if( t1ops.bMatchingTT && bConvertPS )
        {
            //
            // "The TrueType version of this font is already installed."
            //

            switch( iUIMsgBox( hwndParent,
                               MYFONT + 4, IDS_MSG_CAPTION,
                               MB_YESNOCANCEL | MB_ICONEXCLAMATION,
                               (LPTSTR) szDesc ) )
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
    //

    //
    // The following section of commented-out code prevents the Type1 font
    // installation dialog from being displayed.  Since we're removing
    // the requirement for Type1-to-TrueType conversion from NT5, this
    // dialog is no longer necessary.  Instead of taking out all of the code
    // related to Type1-TrueType conversion, I've merely disabled this dialog
    // and automatically set the values of bConvertPS, bInstallPS and bCopyPS.
    // To do this right, the font folder needs to be re-written from scratch.
    // It's a real mess and is very fragile so I don't want to introduce subtle
    // errors or spend a lot of time partially re-writing only some of it.
    // Note that simply removing this preprocessor directive will NOT re-enable
    // TrueType to Type1 conversion.  There are other minor changes in the code
    // that support the deactivation of this feature. [brianau 5/15/97]
    //
#ifdef ENABLE_TRUETYPE_TO_TYPE1_CONVERSION

    if( !bYesAll_PS )
    {
        //
        //  Set normal cursor for new dialog
        //

        HCURSOR hCurs = SetCursor( LoadCursor( NULL, IDC_ARROW ) );


        switch( DoDialogBoxParam( DLG_INSTALL_PS, hDlgProgress, InstallPSDlg,
                                  IDH_DLG_INSTALL_PS, (LPARAM)  &t1ops ) )
        {
        case IDNO:
            //
            //  Note that we do not do HourGlass( TRUE ) , but that
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

            if( bInstallPS && !bCopyPS )
            {
                lstrcpy( szTemp, szPfbName );
                StripFilespec( szTemp );
                lpCPBackSlashTerm( szTemp );

                switch( GetDriveType( szTemp ) )
                {
                    case DRIVE_REMOTE:
                    case DRIVE_REMOVABLE:
                    case DRIVE_CDROM:
                    case DRIVE_RAMDISK:
                        switch( iUIMsgBox( IDSI_MSG_COPYCONFIRM, IDS_MSG_CAPTION,
                                       MB_YESNOCANCEL | MB_ICONEXCLAMATION))
                        {
                            case IDNO:
                               iRet = TYPE1_INSTALL_IDNO;
                               goto InstallPSFailure;
                            case IDCANCEL:
                               iRet = TYPE1_INSTALL_IDCANCEL;
                               goto InstallPSFailure;
                            default:
                               break;
                        }
                        break;
                }
            }
            break;

        default:
            //
            // CANCEL and NOMEM( user already warned )
            //

            iRet = TYPE1_INSTALL_IDCANCEL;
            goto InstallPSFailure;
        }

        //
        //  Reset previous cursor
        //

        SetCursor( hCurs );
    }
#endif // ENABLE_TRUETYPE_TO_TYPE1_CONVERSION

    //
    // These values were originally set in the Type1 installation dialog.
    // Now that this dialog has been disabled, we hard code the values.
    // [brianau 5/15/97]
    //
    bInstallPS = TRUE;             // Always install the font.
    bConvertPS = FALSE;            // Never convert Type1 to TrueType.
    bCopyPS    = bCopyType1Files;

    //
    // szDstName should already have the full source file name
    //
    //  Only convert the Type1 font to TT if:
    //
    //  a)  The user asked us to do it;
    //  b)  The font can be converted, AND;
    //  c)  There is not a matching TT font already installed
    //
    if( bConvertPS && ( wFontType != TYPE1_FONT_NC )  && !t1ops.bMatchingTT )
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

        lstrcpy( szTemp, szPfmName );
        StripPath( szTemp );
        vConvertExtension( szTemp, szTTF );

        //
        //  Build destination file pathname based on bCopyTTFile
        //

        if( bCopyTTFile || bInSharedDir )
        {
            //
            //  Copy file to local directory
            //

            lstrcpy( szDstName, s_szSharedDir );
        }
        else
        {
            //
            //  Create converted file in source directory
            //

            lstrcpy( szDstName, szSrcDir );
        }

        //
        //  Check new filename for uniqueness
        //

        if( !(bUniqueFilename( szTemp, szTemp, szDstName ) ) )
        {
            iRet = iUIMsgOkCancelExclaim( hwndParent, IDSI_FMT_BADINSTALL,
                                          IDSI_CAP_NOCREATE, szDesc );
            goto InstallPSFailure;
        }

        lstrcat( szDstName, szTemp );

        //
        //  Save destination filename for return to caller
        //

#if 1
        lstrcpy( szTTFName, szDstName );
#else
        if( bCopyTTFile || bInSharedDir )
            lstrcpy( szTTFName, szTemp );
        else
            lstrcpy( szTTFName, szDstName );
#endif

        //
        //  We will convert and copy the Type1 font in the same api
        //

        WideCharToMultiByte( CP_ACP, 0, szPfbName, -1, pfb,
                                PATHMAX, NULL, NULL );

        WideCharToMultiByte( CP_ACP, 0, szPfmName, -1, pfm,
                                PATHMAX, NULL, NULL );

        WideCharToMultiByte( CP_ACP, 0, szDstName, -1, ttf,
                                PATHMAX, NULL, NULL );

        ResetProgress( );

        //
        //  Remove "PostScript" postfix string from description
        //

        RemoveDecoration( szDesc, TRUE );

        if( (rc = (int) ::ConvertTypefaceA( pfb, pfm, ttf,
                                            (PROGRESSPROC) Progress,
                                            (void *) szDesc ) ) < 0 )
        {
            pszArg1 = szPfmName;
            pszArg2 = szPfbName;

            switch( rc )
            {
            case ARGSTACK:
            case TTSTACK:
            case NOMEM:
                wMsg = INSTALL3;
                break;

            case NOMETRICS:
            case BADMETRICS:
            case UNSUPPORTEDFORMAT:
                //
                //  Something is wrong with the .pfm metrics file
                //

                pszArg1 = szDstName;
                pszArg2 = szPfmName;
                wMsg = MYFONT + 13;
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
                wMsg = MYFONT + 14;
                break;

            case BADINPUTFILE:
                //
                //  Bad input file names, or formats or file errors
                //  or file read errors
                //

                pszArg1 = szDstName;
                pszArg2 = szPfbName;
                wMsg = MYFONT + 15;
                break;

            case BADOUTPUTFILE:
                //
                //  No diskspace for copy, read-only share, etc.
                //

                pszArg1 = szDstName;
                wMsg = MYFONT + 16;
                break;

            default:
                //
                //  Cannot convert szDesc to TrueType - general failure
                //

                pszArg1 = szDstName;
                pszArg2 = szDesc;
                wMsg = MYFONT + 17;
                break;
            }

            iRet = iUIMsgBox( hwndParent, wMsg, IDS_MSG_CAPTION,
                              MB_OKCANCEL | MB_ICONEXCLAMATION,
                              pszArg1, pszArg2, szPfmName );

            goto InstallPSFailure;
        }

        //
        //  Change font description to have "TrueType" now
        //

        wsprintf( szDesc, c_szDescFormat, szDesc, c_szTrueType );
    }

    iRet = TYPE1_INSTALL_IDNO;

    if( bInstallPS && !t1ops.bOnlyPSInstalled )
    {
        //
        //  Remove "PostScript" postfix string from description
        //

        lstrcpy( szTemp2, szDesc );
        RemoveDecoration( szTemp2, TRUE );

        //
        //  Now reset per font install progress
        //

        ResetProgress( );
        Progress2( 0, szTemp2 );


        //
        //  Only copy the files if the User asked us to AND they are NOT
        //  already in the Shared directory.
        //

        if( bCopyPS && !bInSharedDir )
        {
            //
            //  Copy file progress
            //

            Progress2( 10, szTemp2 );

            /////////////////////////////////////////////////////////////////
            // COPY files to "fonts" directory
            /////////////////////////////////////////////////////////////////

            //
            //  For .inf/.afm file install::  Check .pfm pathname to see if
            //  it is the same as the destination file pathname we built.
            //  Make this check before we test/create a UniqueFilename.
            //

            //  Build Destination file pathname for .PFM file

            lstrcpy( szDstName, s_szSharedDir );
            lstrcat( szDstName, lpNamePart( szPfmName ) );

            //
            //  Check to see if the .pfm file already exists in the "fonts"
            //  directory.  If it does, then just copy the .pfb over.
            //

            if( !lstrcmpi( szPfmName, szDstName ) )
                goto CopyPfbFile;

            //
            //  Setup args for bCPInstallFile call
            //

            StripPath( szDstName );
            StripPath( szPfmName );

            if( !(bUniqueOnSharedDir( szDstName, szDstName ) ) )
            {
                iRet = iUIMsgOkCancelExclaim( hwndParent, IDSI_FMT_BADINSTALL,
                                              IDSI_CAP_NOCREATE, szDesc );
                goto InstallPSFailure;
            }

            if( !bCPInstallFile( hwndParent, szSrcDir, szPfmName, szDstName ) )
                goto InstallPSFailure;

CopyPfbFile:

            //
            //  Copying pfm file was small portion of install
            //

            Progress2( 30, szTemp2 );

            //
            //  Setup and copy .PFB file
            //
            //  Setup args for bCPInstallFile call
            //

            lstrcpy(szSrcDir, szPfbName);  // Prepare src directory name.
            StripFilespec( szSrcDir );
            lpCPBackSlashTerm( szSrcDir );

            StripPath( szPfbName );
            lstrcpy( szDstName, szPfbName );

            if( !(bUniqueOnSharedDir( szDstName, szDstName ) ) )
            {
                iRet = iUIMsgOkCancelExclaim( hwndParent, IDSI_FMT_BADINSTALL,
                                              IDSI_CAP_NOCREATE, szDesc );
                goto InstallPSFailure;
            }

            if( !bCPInstallFile( hwndParent, szSrcDir, szPfbName, szDstName ) )
                goto InstallPSFailure;
        }

        //
        //  Copying pfb file was large portion of install
        //

        Progress2( 85, szTemp2 );

        //
        //  Write registry entry to "install" font for use by the
        //  PostScript driver, but only after successfully copying
        //  files (if copy was necessary).
        //
        //  NOTE:  This routine will strip the path off of the filenames
        //         if they are in the Fonts dir.
        //

        iRet = WriteType1RegistryEntry( hwndParent, szDesc, szPfmName, szPfbName, bCopyPS );

        //
        // If the Type1 font driver is installed,
        // add the Type1 font resource to GDI.
        //
        {
            CFontManager *poFontManager;
            if (SUCCEEDED(GetFontManager(&poFontManager)))
            {
                if (poFontManager->Type1FontDriverInstalled())
                {
                    TCHAR szType1FontResourceName[MAX_TYPE1_FONT_RESOURCE];

                    if (BuildType1FontResourceName(szPfmName,
                                                   szPfbName,
                                                   szType1FontResourceName,
                                                   ARRAYSIZE(szType1FontResourceName)))
                    {
                        AddFontResource(szType1FontResourceName);
                    }
                }
                ReleaseFontManager(&poFontManager);
            }
        }

        //
        //  No need to add to internal font list here.  It is added in the calling
        //  code (bAddSelFonts or CPDropInstall).
        //

        //
        //  Final registry write completes install, except for listbox munging.
        //  Note that TrueType file install is handled separately.
        //

        Progress2( 100, szTemp2 );
    }

    //
    //  Determine correct return code based on installation options and
    //  current installed state of font.
    //

    if( bConvertPS && ( wFontType != TYPE1_FONT_NC ) )
    {
        //
        //  Handle the special case of when the matching TTF font is already
        //  installed.
        //

        if( t1ops.bMatchingTT )
            goto Type1InstallCheck;


        lstrcpy( szKeyName, szTTFName );

        if( t1ops.bOnlyPSInstalled )
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
        else if( bInstallPS )
        {
            iRet = ( iRet == IDOK )  ? TYPE1_INSTALL_TT_AND_PS : TYPE1_INSTALL_TT_ONLY;

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

#ifdef LATER
//
//  Since fonts are displayed in an Explorer (Fonts) Folder, there is not an
//  initial display listbox of "Installed" fonts to check against.  These
//  checks must be made against the registry list of installed fonts.
//
//  Also, I need to force a "Refresh" of the Folder view after installation
//  is complete of either each font or all selected fonts.   This might already
//  be being done somewhere.
//

    if( bInstallPS )
    {
        if( iRet != IDOK )
        {
            iRet = TYPE1_INSTALL_IDNO;
            goto InstallPSFailure;
        }

        iRet = TYPE1_INSTALL_IDNO;

        if( t1ops.bMatchingTT )
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
            //  Change font description to have "(TrueType ) " now
            //

            RemoveDecoration( szDesc, TRUE );

            wsprintf( szDesc, c_szDescFormat, szDesc, c_szTrueType );

            rc= SendMessage( hListFonts, LB_FINDSTRINGEXACT, (WPARAM )  -1,
                                                         (LONG ) szDesc );

            if( rc != LB_ERR )
            {
                SendMessage( hListFonts, LB_SETITEMDATA, rc, (LONG) IF_TYPE1_TT );

                SendMessage( hListFonts, LB_SETSEL, 1, rc );

                UpdateWindow( hListFonts );

                iRet = TYPE1_INSTALL_PS_AND_MTT;
            }
        }
        else
        {
            rc = SendMessage( hListFonts, LB_ADDSTRING, 0, (LONG)(LPTSTR) szDesc );

            //
            //  Attach font type to each listed font
            //

            if( rc != LB_ERR )
            {
                SendMessage( hListFonts, LB_SETITEMDATA, rc, IF_TYPE1 );

                SendMessage( hListFonts, LB_SETSEL, 1, rc );

                UpdateWindow( hListFonts );

                iRet = TYPE1_INSTALL_PS_ONLY;
            }
        }
    }
#else  //  LATER

    if( bInstallPS )
    {
        if( iRet != IDOK )
        {
            iRet = TYPE1_INSTALL_IDNO;
            goto InstallPSFailure;
        }

        if( t1ops.bMatchingTT )
        {
            iRet = TYPE1_INSTALL_PS_AND_MTT;
        }
        else
        {
            iRet = TYPE1_INSTALL_PS_ONLY;
        }
    }

#endif  //  LATER

    if( !bInstallPS )
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
    //         CheckType1WithStatusA routine in the IsPSFont routine.
    //
    /////////////////////////////////////////////////////////////////////////

InstallPSFailure:

CheckPfmDeletion:

    if( bCreatedPfm )
        DeleteFile( szPfmName );

MasterExit:

    return( InstallCancelled() ? IDCANCEL : iRet );
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

INT_PTR APIENTRY InstallPSDlg( HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam )
{
    TCHAR  szFormat[ PATHMAX ] ;
    TCHAR  szTemp[ PATHMAX ] ;
    TCHAR  szTemp2[ PATHMAX ] ;
    int    iButtonChecked;
    WORD   wMsg;

    static HWND hwndActive = NULL;

    T1_INSTALL_OPTIONS *pt1ops;


    switch( nMsg )
    {

    case WM_INITDIALOG:

        pt1ops = (PT1_INSTALL_OPTIONS)  lParam;

        //
        //  Remove all "PostScript" or "TrueType" postfix to font name
        //

        lstrcpy( szTemp2, (LPTSTR) pt1ops->szDesc );

        RemoveDecoration( szTemp2, FALSE );

        {
            LPCTSTR args[] = { szTemp2 };

            LoadString( g_hInst, MYFONT + 7, szFormat, ARRAYSIZE( szFormat ) );
            FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                          FORMAT_MESSAGE_ARGUMENT_ARRAY,
                          szFormat,
                          0,
                          0,
                          szTemp,
                          ARRAYSIZE(szTemp),
                          (va_list *)args);
        }

        SetWindowLongPtr( hDlg, GWL_PROGRESS, lParam );

        SetDlgItemText( hDlg, FONT_INSTALLMSG, szTemp );
        EnableWindow( hDlg, TRUE );

        //
        // If the native ATM driver is installed, the "Convert to TrueType" checkbox
        // is always disabled.
        //
        {
            CFontManager *poFontManager;
            if (SUCCEEDED(GetFontManager(&poFontManager)))
            {
                if (poFontManager->Type1FontDriverInstalled())
                    EnableWindow(GetDlgItem(hDlg, FONT_CONVERT_PS), FALSE);
                ReleaseFontManager(&poFontManager);
            }
        }

        if( pt1ops->bOnlyPSInstalled && pt1ops->bMatchingTT )
        {
            //
            // ERROR!  Both of these options should not be set at
            //         this point.  It means that the font is
            //         already installed.  This should have been
            //         handled before calling this dialog.
            //

            wMsg = MYFONT + 5;
InstallError:

            iUIMsgExclaim( hDlg, wMsg, pt1ops->szDesc );

            EndDialog( hDlg, IDNO );
            break;
        }

        if( (pt1ops->wFontType == TYPE1_FONT_NC )  && pt1ops->bOnlyPSInstalled )
        {
            //
            //  ERROR! This case is when I have detected only the PS
            //         version of font installed, and the font CANNOT
            //         be converted to TT for some reason.
            //

            wMsg = MYFONT + 8;
            goto InstallError;
        }

        /////////////////////////////////////////////////////////////////////
        //
        //  Setup user options depending on install state of font and
        //  convertibility to TT of T1 font and on previous user choices.
        //
        /////////////////////////////////////////////////////////////////////


        if( (pt1ops->wFontType == TYPE1_FONT )  && (!pt1ops->bMatchingTT ) )
        {
            //
            //  This one can be converted
            //

            CheckDlgButton( hDlg, FONT_CONVERT_PS, bConvertPS );
        }
        else
        {
            //
            //  Do not allow conversion to TT because, either the font
            //  type is TYPE1_FONT_NC( i.e. it cannot be converted )  OR
            //  the TT version of font is already installed.
            //

            CheckDlgButton( hDlg, FONT_CONVERT_PS, FALSE );
            EnableWindow( GetDlgItem( hDlg, FONT_CONVERT_PS ) , FALSE );
        }

        if( pt1ops->bOnlyPSInstalled )
        {
            //
            //  If the PostScript version of this font is already
            //  installed, then we gray out the options to re-install
            //  the PostScript version of font, but continue to allow
            //  the User to convert it to TT.
            //

            CheckDlgButton( hDlg, FONT_INSTALL_PS, 0 );

            EnableWindow( GetDlgItem( hDlg, FONT_INSTALL_PS ) , FALSE );

            CheckDlgButton( hDlg, FONT_COPY_PS, 0 );

            EnableWindow( GetDlgItem( hDlg, FONT_COPY_PS ) , FALSE );
        }
        else
        {
            //
            //  PostScript version of font is not installed.  Set
            //  state of "INSTALL" and "COPY" checkboxes based on
            //  global state of "INSTALL"
            //

            CheckDlgButton( hDlg, FONT_INSTALL_PS, bInstallPS );

            CheckDlgButton( hDlg, FONT_COPY_PS, bCopyPS );

            EnableWindow( GetDlgItem( hDlg, FONT_COPY_PS ) , bInstallPS );

        }

        //
        //  Save the modeless dlg window handle for reactivation
        //

        hwndActive = GetActiveWindow( );
        break;


    case WM_COMMAND:
        switch( LOWORD( wParam ) )
        {
        case FONT_INSTALL_PS:
            if( HIWORD( wParam )  != BN_CLICKED )
                break;

            //
            // Get state of "INSTALL" checkbox
            //

            iButtonChecked = IsDlgButtonChecked( hDlg, LOWORD( wParam ) );

            //
            // A disabled checkbox is same as "No Install" selection
            //

            if( iButtonChecked != 1 )
                iButtonChecked = 0;

            //
            //  Enable or disable "COPY" control based on state of
            //  "INSTALL" checkbox.  Also, initialize it.
            //

            EnableWindow( GetDlgItem( hDlg, FONT_COPY_PS ) , iButtonChecked );

            if( iButtonChecked )
                CheckDlgButton( hDlg, FONT_COPY_PS, bCopyPS );

            break;


        case IDD_HELP:
            goto DoHelp;

        case IDYES:
        case IDD_YESALL:
            bConvertPS =
            bInstallPS = FALSE;

            if( IsDlgButtonChecked( hDlg, FONT_CONVERT_PS )  == 1 )
                bConvertPS = TRUE;

            if( IsDlgButtonChecked( hDlg, FONT_INSTALL_PS )  == 1 )
                bInstallPS = TRUE;

            //
            //  This is checked twice because it could be disabled,
            //  in which case we leave the previous state alone.
            //

            if( IsDlgButtonChecked( hDlg, FONT_COPY_PS )  == 1 )
                bCopyPS = TRUE;

            if( IsDlgButtonChecked( hDlg, FONT_COPY_PS )  == 0 )
                bCopyPS = FALSE;

            //
            //  Fall thru...
            //

        case IDNO:
        case IDCANCEL:
            //
            //  Reset the active window to "Install Font Progress" modeless dlg
            //

            if( hwndActive )
            {
                SetActiveWindow( hwndActive );
                hwndActive = NULL;
            }

            EndDialog( hDlg, LOWORD( wParam ) );
            break;

        default:
            return FALSE;
        }
        break;

    default:
DoHelp:

#ifdef PS_HELP
//
// FIXFIX [stevecat] Enable help
        if( nMsg == wHelpMessage )
        {
DoHelp:
            CPHelp( hDlg );
            return TRUE;
        }
        else
#endif  //  PS_HELP

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

void RemoveDecoration( LPTSTR pszDesc, BOOL bDeleteTrailingSpace )
{
    LPTSTR lpch;

    //
    //  Remove any postfix strings like "(PostScript)" or "(TrueType)"
    //

    if( lpch = _tcschr( pszDesc, TEXT('(') ) )
    {
        //
        //  End string at <space> before "("
        //

        if( bDeleteTrailingSpace )
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
//  will return the data for the "szData" value if it finds a matching entry.
//
//  Assumes "szData" buffer is of least size T1_MAX_DATA.
//
/////////////////////////////////////////////////////////////////////////////

BOOL CheckT1Install( LPTSTR pszDesc, LPTSTR pszData )
{
    TCHAR  szTemp[ PATHMAX ] ;
    DWORD  dwSize;
    DWORD  dwType;
    HKEY   hkey;
    BOOL   bRet = FALSE;


    hkey = NULL;

    if( RegOpenKeyEx( HKEY_LOCAL_MACHINE,        // Root key
                      g_szType1Key,              // Subkey to open
                      0L,                        // Reserved
                      KEY_READ,                  // SAM
                      &hkey )                    // return handle
            == ERROR_SUCCESS )
    {
        //
        //  Remove any postfix strings like "PostScript" or "TrueType"
        //

        lstrcpy( szTemp, pszDesc );

        RemoveDecoration( szTemp, TRUE );

        dwSize = pszData ? T1_MAX_DATA * sizeof( TCHAR ) : 0;

        if( RegQueryValueEx( hkey, szTemp, NULL, &dwType,
                             (LPBYTE)  pszData, &dwSize )
                ==  ERROR_SUCCESS )
        {
            bRet = ( dwType == REG_MULTI_SZ );
        }
        else
        {
            bRet = FALSE;
        }

        RegCloseKey( hkey );
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

void AddSystemPath( LPTSTR pszFile )
{
    TCHAR  szPath[ PATHMAX ] ;

    //
    //  Add "system" path, if no path present on file
    //

    lstrcpy( szPath, pszFile );

    StripFilespec( szPath );

    if( szPath[ 0 ]  == CHAR_NULL )
    {
        lstrcpy( szPath, s_szSharedDir );
        lpCPBackSlashTerm( szPath );
        lstrcat( szPath, pszFile );
        lstrcpy( pszFile, szPath );
    }

    return ;
}

/////////////////////////////////////////////////////////////////////////////
//
// ExtractT1Files
//
//  Extracts file names from a REG_MULTI_SZ (multi-string)  array that is
//  passed into this routine.  The output strings are expected to be at
//  least PATHMAX in size.  A "" (NULL string)  indicates that a filename
//  string was not present.  This should only happen for the PFB filename
//  argument.
//
/////////////////////////////////////////////////////////////////////////////

BOOL ExtractT1Files( LPTSTR pszMulti, LPTSTR pszPfmFile, LPTSTR pszPfbFile )
{
    LPTSTR pszPfm;
    LPTSTR pszPfb;

    if( !pszMulti )
        return FALSE;

    if( ( pszMulti[ 0 ]  != CHAR_TRUE )  && ( pszMulti[ 0 ]  != CHAR_FALSE ) )
        return FALSE;

    //
    //  .Pfm file should always be present
    //

    pszPfm = pszMulti + lstrlen( pszMulti ) + 1;

    lstrcpy( pszPfmFile, pszPfm );

    //
    //  Add "system" path, if no path present on files
    //

    AddSystemPath( pszPfmFile );

    //
    //  Check to see if .pfb filename is present
    //

    if( pszMulti[ 0 ]  == CHAR_TRUE )
    {
        pszPfb = pszPfm + lstrlen( pszPfm )  + 1;
        lstrcpy( pszPfbFile, pszPfb );

        //
        //  Add "system" path, if no path present on files
        //

        AddSystemPath( pszPfbFile );
    }
    else
    {
        pszPfbFile[ 0 ]  = CHAR_NULL;
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

BOOL DeleteT1Install( HWND hwndParent, LPTSTR pszDesc, BOOL bDeleteFiles )
{
    TCHAR  szTemp[ PATHMAX ] ;
    TCHAR  szTemp2[ T1_MAX_DATA ] ;
    TCHAR  szPfmFile[ PATHMAX ] ;
    TCHAR  szPfbFile[ PATHMAX ] ;
    TCHAR  szPath[ PATHMAX ] ;
    DWORD  dwSize;
    DWORD  dwType;
    HKEY   hkey;
    BOOL   bRet = FALSE;

    hkey = NULL;

    if( RegOpenKeyEx( HKEY_LOCAL_MACHINE,        // Root key
                      g_szType1Key,              // Subkey to open
                      0L,                        // Reserved
                      (KEY_READ | KEY_WRITE),    // SAM
                      &hkey )                    // return handle
            == ERROR_SUCCESS )
    {
        //
        //  Remove any postfix strings like "PostScript" or "TrueType"
        //

        lstrcpy( szTemp, pszDesc );

        RemoveDecoration( szTemp, TRUE );

        if( bDeleteFiles )
        {
            dwSize = sizeof( szTemp2 );

            if( RegQueryValueEx( hkey, szTemp, NULL, &dwType,
                                 (LPBYTE )  szTemp2, &dwSize )
                    ==  ERROR_SUCCESS )
            {
                if( ExtractT1Files( szTemp2, szPfmFile, szPfbFile ) )
                {
                    //
                    //  Delete the files
                    //
//                    if( DelSharedFile( hDlg, szTemp, szPfbFile, szPath, TRUE ) )
//                        DelSharedFile( hDlg, szTemp, szPfmFile, szPath, FALSE );
//
                    vCPDeleteFromSharedDir( szPfbFile );
                    vCPDeleteFromSharedDir( szPfmFile );
                }
                else
                {
                    //  ERROR! Cannot get file names from string
                    goto RemoveT1Error;
                }
            }
            else
            {
                goto RemoveT1Error;
            }
        }

        if( RegDeleteValue( hkey, szTemp )  != ERROR_SUCCESS )
        {
RemoveT1Error:

            //
            //  ERROR! Put up message box
            //

            iUIMsgOkCancelExclaim( hwndParent,
                                   MYFONT + 1,
                                   IDS_MSG_CAPTION,
                                   (LPTSTR ) szTemp );

            bRet = FALSE;
        }
        else
        {
            bRet = TRUE;
        }

        RegCloseKey( hkey );
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

BOOL GetT1Install( LPTSTR pszDesc, LPTSTR pszPfmFile, LPTSTR pszPfbFile )
{
    TCHAR  szTemp2[ T1_MAX_DATA ] ;
    BOOL   bRet = FALSE;


    if( CheckT1Install( pszDesc, szTemp2 ) )
    {
        bRet = ExtractT1Files( szTemp2, pszPfmFile, pszPfbFile );
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

BOOL CheckTTInstall( LPTSTR szDesc )
{
    TCHAR szTemp[ PATHMAX ] ;
    TCHAR szTemp2[ PATHMAX ] ;

    //
    //  Change description string to have TrueType instead of
    //  PostScript and then check if it is already installed.
    //

    lstrcpy( szTemp, szDesc );

    RemoveDecoration( szTemp, TRUE );

    wsprintf( szTemp, c_szDescFormat, szTemp, c_szTrueType );

    if( GetProfileString( szFonts, szTemp, szNull, szTemp2, ARRAYSIZE( szTemp2 ) ) )
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

int WriteType1RegistryEntry( HWND hwndParent,
                             LPTSTR szDesc,         // Font name description
                             LPTSTR szPfmName,      // .PFM filename
                             LPTSTR szPfbName,      // .PFB filename
                             BOOL   bInFontsDir )   // Files in fonts dir?
{
    TCHAR  szTemp[ 2*PATHMAX+6 ] ;
    TCHAR  szTemp2[ PATHMAX ] ;
    TCHAR  szClass[ PATHMAX ] ;
    DWORD  dwSize;
    DWORD  dwDisposition;
    HKEY   hkey = NULL;

    //
    //  Must have a Font description to store information in registry
    //

    if( !szDesc || !szPfmName )
        return TYPE1_INSTALL_IDNO;

    //
    //  Try to create the key if it does not exist or open existing key.
    //

    if( RegCreateKeyEx( HKEY_LOCAL_MACHINE,        // Root key
                        g_szType1Key,              // Subkey to open/create
                        0L,                        // Reserved
                        szClass,                   // Class string
                        0L,                        // Options
                        KEY_WRITE,                 // SAM
                        NULL,                      // ptr to Security struct
                        &hkey,                     // return handle
                        &dwDisposition )           // return disposition
            == ERROR_SUCCESS )
    {
        //
        //  Create REG_MULTI_SZ string to save in registry
        //
        //  X <null> [path]zzzz.pfm <null> [path]xxxxx.pfb <null><null>
        //
        //  Where X == T(rue )  if .pfb file present
        //

        lstrcpy( szTemp, szPfbName ? szTrue : szFalse );
        lstrcat( szTemp, szHash );

        if( bInFontsDir )
            StripPath( szPfmName );

        lstrcat( szTemp, szPfmName );
        lstrcat( szTemp, szHash );

        if( szPfbName )
        {
            if( bInFontsDir )
                StripPath( szPfbName );

            lstrcat( szTemp, szPfbName );
            lstrcat( szTemp, szHash );
        }

        lstrcat( szTemp, szHash );

        dwSize = lstrlen( szTemp ) * sizeof( TCHAR );

        //
        //  Now convert string to multi-string
        //

        vHashToNulls( szTemp );

        //
        //  Create Registry Value name to store info under by
        //  removing any postfix strings like "Type 1" or
        //  "TrueType" from Font description string.
        //

        lstrcpy( szTemp2, szDesc );

        RemoveDecoration( szTemp2, TRUE );

        if( RegSetValueEx( hkey, szTemp2, 0L, REG_MULTI_SZ,
                            (LPBYTE) szTemp, dwSize )
                != ERROR_SUCCESS )
        {
            goto WriteRegError;
        }

        RegCloseKey( hkey );
    }
    else
    {
WriteRegError:

        //
        //  Put up a message box error stating that the USER does
        //  not have the permission necessary to install type1
        //  fonts.
        //

        if( hkey )
            RegCloseKey( hkey );

        return( iUIMsgBox( hwndParent,
                           MYFONT + 9, IDS_MSG_CAPTION,
                           MB_OKCANCEL | MB_ICONEXCLAMATION,
                           (LPTSTR) szDesc,
                           (LPTSTR) g_szType1Key ) );
    }

    return TYPE1_INSTALL_IDOK;
}


#ifdef NOTUSED

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

BOOL EnumType1Fonts( HWND hLBox )
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

    dwPathMax = PATHMAX * sizeof( TCHAR );

    pszFontName = (LPTSTR) AllocMem( dwPathMax );

    pszDesc = (LPTSTR) AllocMem( dwPathMax );

    if( !pszDesc || !pszFontName )
        goto EnumType1Exit;


    //////////////////////////////////////////////////////////////////////
    //  Get list of installed Type 1 fonts
    //////////////////////////////////////////////////////////////////////

    hkey = NULL;

    if( RegOpenKeyEx( HKEY_LOCAL_MACHINE,       // Root key
                      g_szType1Key,             // Subkey to open
                      0L,                       // Reserved
                      KEY_READ,                 // SAM
                      &hkey )                   // return handle
            == ERROR_SUCCESS )
    {
        dwSize = dwPathMax;
        i = 0;

        while( RegEnumValue( hkey, i++, pszFontName, &dwSize, NULL,
                             &dwType, (LPBYTE)  NULL, NULL )
                    == ERROR_SUCCESS )
        {
            if( dwType != REG_MULTI_SZ )
                continue;

            wsprintf( pszDesc, c_szDescFormat, pszFontName, c_szPostScript );

            //
            //  Check to see if TrueType version is already installed
            //

            if( CheckTTInstall( pszDesc ) )
            {
                //
                //  Find matching  "xxxxx (TrueType)" entry and change its
                //  ItemData to IF_TYPE1_TT
                //

                wsprintf( pszDesc, c_szDescFormat, pszFontName, c_szTrueType );

                j = SendMessage( hLBox, LB_FINDSTRINGEXACT, (WPARAM)  -1,
                                                             (LONG) pszDesc );

                if( j != LB_ERR )
                    SendMessage( hLBox, LB_SETITEMDATA, j, (LONG)  IF_TYPE1_TT );

                // else
                    // ERROR! We should have found a matching LB entry for this
                    //        font based on TT name.
            }
            else
            {
                //
                //  Put Font name string in ListBox
                //

                j = SendMessage( hLBox, LB_ADDSTRING, 0, (LONG) pszDesc );

                if( j != LB_ERR )
                    SendMessage( hLBox, LB_SETITEMDATA, j, (LONG)  IF_TYPE1 );
                // else
                    // ERROR! We found an installed Type1 font but cannot show
                    //        it in listbox because of USER error.
            }

            dwSize = dwPathMax;
        }

        bRet = TRUE;

        RegCloseKey( hkey );
    }

EnumType1Exit:

    if( pszDesc )
        FreeMem( pszDesc, dwPathMax );

    if( pszFontName )
        FreeMem( pszFontName, dwPathMax );

    return bRet;
}

#endif  //  NOTUSED


/////////////////////////////////////////////////////////////////////////////
//
// InitProgress
//
//  Create and initialize the Progress dialog.  Initial state is visible.
//
/////////////////////////////////////////////////////////////////////////////

HWND InitProgress( HWND hwnd )
{
    if( NULL == hDlgProgress )
    {
        RegisterProgressClass( );

        hDlgProgress = CreateDialog( g_hInst, MAKEINTRESOURCE( DLG_PROGRESS ) ,
                                     hwnd ? hwnd :HWND_DESKTOP,
                                     ProgressDlg );
    }

    return hDlgProgress;
}


/////////////////////////////////////////////////////////////////////////////
//
// TermProgress
//
//  Remove and cleanup after the Progress dialog.
//
/////////////////////////////////////////////////////////////////////////////

void TermProgress( )
{
    if( hDlgProgress )
    {
        DestroyWindow( hDlgProgress );
        UnRegisterProgressClass( );
    }

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
//  Since the font conversion is done on a single thread( in order to keep it
//  synchronous with installation of all fonts )  we need to provide a mechanism
//  that will allow a user to Cancel out of the operation and also allow
//  window messages, like WM_PAINT, to be processed by other Window Procedures.
//
/////////////////////////////////////////////////////////////////////////////

VOID cpProgressYield( )
{
    MSG msg;

    while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
    {
//        if( !hDlgProgress || !IsDialogMessage( hDlgProgress, &msg ) )
        if( !IsDialogMessage( hDlgProgress, &msg ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
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

void UpdateProgress( int iTotalCount, int iFontInstalling, int iProgress )
{
    TCHAR szTemp[ 120 ] ;

    wsprintf( szTemp, m_szMsgBuf, iFontInstalling, iTotalCount );

    SetDlgItemText( hDlgProgress, ID_INSTALLMSG, szTemp );

	SendDlgItemMessage( hDlgProgress, ID_OVERALL, SET_PROGRESS,
                        (int) iProgress, 0L );

    //
    //  Process outstanding messages
    //

    cpProgressYield( );
}


/////////////////////////////////////////////////////////////////////////////
//
// ResetProgress
//
//   Clear the progress bar control and reset message to NULL
//
/////////////////////////////////////////////////////////////////////////////

void ResetProgress(  )
{
    SetDlgItemText( hDlgProgress, ID_PROGRESSMSG, szNull );

    SendDlgItemMessage( hDlgProgress, ID_BAR, SET_PROGRESS, (int) 0, 0L );

    bProgMsgDisplayed = FALSE;

    bProg2MsgDisplayed = FALSE;

    //
    //  Process outstanding messages
    //

    cpProgressYield( );
}


BOOL InstallCancelled(void)
{
    return bCancelInstall;
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

void STDCALL Progress( short PercentDone, void* UniqueValue )
{
    TCHAR szTemp[ 120 ] ;
    TCHAR szTemp2[ 120 ] ;
    DWORD err = GetLastError( ); // save whatever t1instal may have set

    //
    //  UniqueValue is a pointer to the string name of the file being
    //  converted.  Only put this message up if not previously displayed.
    //

    if( !bProgMsgDisplayed )
    {
        LPCTSTR args[] = { (LPCTSTR)UniqueValue };

        LoadString( g_hInst, MYFONT + 6, szTemp2, ARRAYSIZE( szTemp2 ) );
        FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                      FORMAT_MESSAGE_ARGUMENT_ARRAY,
                      szTemp2,
                      0,
                      0,
                      szTemp,
                      ARRAYSIZE(szTemp),
                      (va_list *)args);

        SetDlgItemText( hDlgProgress, ID_PROGRESSMSG, szTemp );

        bProgMsgDisplayed = TRUE;
    }

	SendDlgItemMessage( hDlgProgress, ID_BAR, SET_PROGRESS,
                        (int) PercentDone, 0L );

    //
    //  Process outstanding messages
    //

    cpProgressYield( );

    //
    //  reset last error to whatever t1instal set it to:
    //

    SetLastError( err );
}


/////////////////////////////////////////////////////////////////////////////
//
// Progress2
//
//   Progress function for updating progress dialog controls on a per font
//   install basis.
//
/////////////////////////////////////////////////////////////////////////////

void Progress2( int PercentDone, LPTSTR pszDesc )
{
    TCHAR szTemp[ PATHMAX ] ;
    TCHAR szTemp2[ 240 ] ;

    //
    //  szDesc is a pointer to the string name of the file being installed.
    //  Only put this message up if not previously displayed.

    if( !bProg2MsgDisplayed )
    {
        LPCTSTR args[] = { pszDesc };

        LoadString( g_hInst, MYFONT + 11, szTemp2, ARRAYSIZE( szTemp2 ) );
        FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                      FORMAT_MESSAGE_ARGUMENT_ARRAY,
                      szTemp2,
                      0,
                      0,
                      szTemp,
                      ARRAYSIZE(szTemp),
                      (va_list *)args);

        SetDlgItemText( hDlgProgress, ID_PROGRESSMSG, szTemp );

        bProg2MsgDisplayed = TRUE;
    }

	SendDlgItemMessage( hDlgProgress, ID_BAR, SET_PROGRESS, (int) PercentDone, 0L );

    //
    //  Process outstanding messages
    //

    cpProgressYield( );
}


/////////////////////////////////////////////////////////////////////////////
//
// ProgressDlg
//
//  Display progress messages to user based on progress in converting
//  font files to TrueType
//
/////////////////////////////////////////////////////////////////////////////

INT_PTR APIENTRY ProgressDlg( HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam )
{

    switch( nMsg )
    {

    case WM_INITDIALOG:
        CentreWindow( hDlg );

        //
        //  Load in Progress messages
        //

        LoadString( g_hInst, MYFONT + 10, m_szMsgBuf, ARRAYSIZE( m_szMsgBuf ) );

        EnableWindow( hDlg, TRUE );
        bCancelInstall = FALSE;
        break;

    case WM_COMMAND:
        switch( LOWORD( wParam ) )
        {

        case IDOK:
        case IDCANCEL:
            bCancelInstall = ( LOWORD( wParam )  == IDCANCEL );
            //
            // Dialog is destroyed programmatically after font is installed.
            // See TermProgress( )
            //
            break;

        default:
            return FALSE;
        }
        break;

    case WM_DESTROY:
        bCancelInstall = FALSE;
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

LRESULT APIENTRY ProgressBarCtlProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    DWORD dwProgress;

    dwProgress = (DWORD)GetWindowLong( hWnd, GWL_PROGRESS );

    switch( message )
    {
    case WM_CREATE:
        dwProgress = 0;

        SetWindowLong( hWnd, GWL_PROGRESS, (LONG)dwProgress );

        break;

    case SET_PROGRESS:
        SetWindowLong( hWnd, GWL_PROGRESS, (LONG) wParam );

        InvalidateRect( hWnd, NULL, FALSE );

        UpdateWindow( hWnd );

        break;


    case WM_ENABLE:
        //
        //  Force a repaint since the control will look different.
        //

        InvalidateRect( hWnd, NULL, TRUE );

        UpdateWindow( hWnd );

        break;


    case WM_PAINT:
        return ProgressPaint( hWnd, dwProgress );


    default:
        return( DefWindowProc( hWnd, message, wParam, lParam ) );

        break;
    }
    return( 0L );
}


/////////////////////////////////////////////////////////////////////////////
//
// RegisterProgressClass
//
//
/////////////////////////////////////////////////////////////////////////////

BOOL RegisterProgressClass( void )
{
    WNDCLASS wcTest;

    wcTest.lpszClassName = TEXT( "cpProgress" );
    wcTest.hInstance     = (HINSTANCE) g_hInst;
    wcTest.lpfnWndProc   = ProgressBarCtlProc;
    wcTest.hCursor       = LoadCursor( NULL, IDC_WAIT );
    wcTest.hIcon         = NULL;
    wcTest.lpszMenuName  = NULL;
    wcTest.hbrBackground = (HBRUSH) ( rgColorPro[ PROGRESSCOLOR_WINDOW ] );
    wcTest.style         = CS_HREDRAW | CS_VREDRAW;
    wcTest.cbClsExtra    = 0;
    wcTest.cbWndExtra    = sizeof( DWORD );

    //
    //  Set Bar color to Blue and text color to white
//
// [stevecat]  Let's make these follow the window title bar color and text
//             color.  Just make the USER calls to get these colors.  This
//             will make it look better with different color schemes and the
//             Theme packs.
//
    //

//    rgbBG = RGB(   0,   0, 255 );
//    rgbFG = RGB( 255, 255, 255 );

    rgbBG = GetSysColor( rgColorPro[ PROGRESSCOLOR_BAR ] );
    rgbFG = GetSysColor( rgColorPro[ PROGRESSCOLOR_TEXT ] );

    return( RegisterClass( (LPWNDCLASS) &wcTest ) );
}


/////////////////////////////////////////////////////////////////////////////
//
// UnRegisterProgressClass
//
//
/////////////////////////////////////////////////////////////////////////////

VOID UnRegisterProgressClass( void )
{
    UnregisterClass( TEXT( "cpProgress" ), (HINSTANCE) g_hInst );
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

LONG ProgressPaint( HWND hWnd, DWORD dwProgress )
{
    PAINTSTRUCT ps;
    HDC         hDC;
    TCHAR       szTemp[ 20 ] ;
    int         dx, dy, len;
    RECT        rc1, rc2;
    SIZE        Size;


    hDC = BeginPaint( hWnd, &ps );

    GetClientRect( hWnd, &rc1 );

    FrameRect( hDC, &rc1, (HBRUSH) GetStockObject( BLACK_BRUSH ) );

    InflateRect( &rc1, -1, -1 );

    rc2 = rc1;

    dx = rc1.right;
    dy = rc1.bottom;

    if( dwProgress == 100 )
        rc1.right = rc2.left = dx;
    else
        rc1.right = rc2.left = ( dwProgress * dx / 100 ) + 1;

    //
    //  Boundary condition testing
    //

    if( rc2.left > rc2.right )
        rc2.left = rc2.right;

    len = wsprintf( szTemp, TEXT( "%3d%%" ), dwProgress );

    GetTextExtentPoint32( hDC, szTemp, len, &Size );

    SetBkColor( hDC, rgbBG );
    SetTextColor( hDC, rgbFG );

    ExtTextOut( hDC, ( dx - Size.cx ) / 2, ( dy - Size.cy ) / 2,
                ETO_OPAQUE | ETO_CLIPPED, &rc1, szTemp, len, NULL );

    SetBkColor( hDC, rgbFG );
    SetTextColor( hDC, rgbBG );

    ExtTextOut( hDC, ( dx - Size.cx ) / 2, ( dy - Size.cy ) / 2,
                ETO_OPAQUE | ETO_CLIPPED, &rc2, szTemp, len, NULL );

    EndPaint( hWnd, &ps );

    return 0L;

}

//
// Creates a resource name string suitable for calling AddFontResource for a
// Type1 font.  The resulting resource string is in the following format:
//
//      <path to pfm>|<path to pfb>
//
// Returns: TRUE  = String was created.
//          FALSE = Caller passed a NULL pointer or destination buffer is too small.
//
BOOL BuildType1FontResourceName(LPCTSTR pszPfm, LPCTSTR pszPfb, LPTSTR pszDest, DWORD cchDest)
{
    BOOL bResult = FALSE;

    if (NULL != pszDest && pszPfm != NULL && pszPfb != NULL)
    {
        *pszDest = TEXT('\0');

        //
        // Make sure dest buffer has room for both paths plus separator and term nul.
        //
        if ((lstrlen(pszPfm) + lstrlen(pszPfb) + 2) < (INT)cchDest)
        {
            wsprintf(pszDest, TEXT("%s|%s"), pszPfm, pszPfb);
            bResult = TRUE;
        }
    }
    return bResult;
}

#endif  //  WINNT
