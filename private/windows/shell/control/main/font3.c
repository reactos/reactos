/** FILE: font3.c ********** Module Header ********************************
 *
 *  Control panel applet for Font configuration.  This file holds code for
 *  the items concerning fonts.
 *
 * History:
 *  12:30 on Tues  23 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *        Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
 *           Updated code to latest Win 3.1 sources
 *  04 April 1994  -by-  Steve Cathcart   [stevecat]
 *            Added support for PostScript Type 1 fonts
 *
 *  Copyright (C) 1990-1994 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                                Include files
//==========================================================================
// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// #include <fcntl.h>
// #include <io.h>
// #include <sys\types.h>
// #include <sys\stat.h>

// Application specific
#include "main.h"

#undef COLOR
#define CONST const
//typedef WCHAR *PWCHAR;

// Windows SDK
#ifdef JAPAN
#include <windows.h>
#endif // JAPAN
#include <wingdip.h>    // For private GDI entry point: GetFontResourceInfo()
#include <commdlg.h>
#include <dlgs.h>
#include <shellapi.h>

#include <lzexpand.h>

#include "fontdefs.h"

//==========================================================================
//                            Local Definitions
//==========================================================================
#define ATTRDIRLIST     0xC010  /* include directories and drives in listbox */
#define ATTRFILELIST    0x0000
#define MAXFILE         20
#define CBEXTMAX        20

#define OF_ERR_FNF 2

#ifdef JAPAN // support install font from *.inf file

#define OEMSETUPINF               TEXT("oemsetup.inf")
#define SETUPINF                  TEXT("setup.inf")

#define OEMSETUPINF_ID            1
#define SETUPINF_ID               2

#define INF_ID_MAX                SETUPINF_ID

#define DISKS_SECTION             TEXT("[disks]")
#define TRUETYPE_FONT_SECTION     TEXT("[TrueType fonts]")

#define OUTLINE_ID                TEXT(".outline")
#define BITMAP_ID                 TEXT(".bitmaps")

#define SOFTWARE_PREFIX TEXT("SOFTWARE\\")
#define SYS_PREFIX      TEXT("SYS:")
#define TT_REG_KEY      TEXT("Microsoft\\Windows NT\\CurrentVersion\\TrueType Bitmap support section\\Bitmap for ")
#define WIN_INI_KEY     TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\IniFileMapping\\win.ini")
#define BITMAP_FOR_KEY  TEXT("Bitmap for ")

#define TRUETYPE_WITH_INF         0xc000

#endif // JAPAN

typedef int (*NEXTFONT)(WORD, void *, LPTSTR, LPTSTR, LPTSTR);

struct tagDlgFontInfo
  {
    INT  iMagic;
    INT  iSize;
    HWND hListDesc;
    HWND hListFiles;
    HWND hDlg;
    LPOPENFILENAME lpOfn;
  } ;

#define DFI_MAGIC  (0x4b4c4243)                     //  KLBC
#define DFI_SIZE   (sizeof(struct tagDlgFontInfo))  //  Structure size

//==========================================================================
//                            External Declarations
//==========================================================================
/* data  */
extern SHORT nCurSel;
extern HWND  hDlgProgress;

extern BOOL  bCancelInstall;
extern TCHAR szFontsKey[];

// Registry location for installing PostScript printer font info

extern TCHAR szType1Key[];

/* functions */
extern void InitPSInstall ();
extern BOOL IsTrueType(LPBUFTYPE lpFile);

//==========================================================================
//                            Local Data Declarations
//==========================================================================
TCHAR szExtSave[CBEXTMAX+1] = TEXT("\0");


#ifdef JAPAN // support install font from *.inf file

BOOL bInstallingFontAtBackground = FALSE;

ATOM *pInfAtomKeyTable[3] = { NULL , NULL , NULL };

typedef struct
{
    ATOM AtomDisksNum;   // Disk Number atom;
    ATOM AtomDisksDir;   // Disk directory atom
    ATOM AtomDisksDesc;  // Disk Description atom;
    ATOM AtomDisksId;    // Disk Id atom;
} DISKS_INFO;

DISKS_INFO *pInfDisksInfo = NULL;

#endif // JAPAN

TCHAR szTTF[]      = TEXT(".TTF");
TCHAR szFON[]      = TEXT(".FON");        // Use this now.
TCHAR szPFM[]      = TEXT(".PFM");
TCHAR szPFB[]      = TEXT(".PFB");

TCHAR szPostScript[]    = TEXT("Type 1");


//==========================================================================
//                            Local Function Prototypes
//==========================================================================
void AddFonts (HWND hwndFontDlg, BOOL bInPlace,
                                        NEXTFONT lpfnNextFont, void *lpData);
VOID vAddDefaultExtension (TCHAR *, TCHAR *);
VOID ConvertExtension (LPTSTR pszFile, LPTSTR szExt);
BOOL IsNewExe (PBUFTYPE pFile);


//==========================================================================
//                                Functions
//==========================================================================

/////////////////////////////////////////////////////////////////////////////
//
// ConvertExtension
//
//  Find the extension on a file and change it to szExt.
//
/////////////////////////////////////////////////////////////////////////////

void ConvertExtension(LPTSTR lpszFile, LPTSTR lpszExt)
{
    LPTSTR lpch;

    if (!(lpch=_tcsrchr (lpszFile, TEXT('\\'))))
        lpch = lpszFile;

    if (lpch=_tcschr (lpch, TEXT('.')))
        *lpch = TEXT('\0');

    lstrcat (lpszFile, lpszExt);
}


/////////////////////////////////////////////////////////////////////////////
//
// IsCompressed
//
//  Leave this function as only ANSI because it just checks the header to
//  determine if it is a compress file or not.
//
/////////////////////////////////////////////////////////////////////////////

BOOL IsCompressed(LPTSTR szFile)
{
    static CHAR szCmpHdr[] = "SZDD\x88\xf0'3";

    BOOL     bRet = FALSE;
    HANDLE   fh;
    CHAR     buf[CharSizeOf(szCmpHdr)];


    if ((fh = OpenFileWithShare(szFile, NULL, OF_READ)) == INVALID_HANDLE_VALUE)
        return(FALSE);

    buf[CharSizeOf(buf)-1] = '\0';

    if (MyByteReadFile (fh, buf, CharSizeOf(buf)-1) && !lstrcmpA (buf, szCmpHdr))
        bRet = TRUE;

    MyCloseFile (fh);
    return (bRet);
}


/////////////////////////////////////////////////////////////////////////////
//
// ValidFontFile
//
// in:
//    lpszFile       file name to validate
// out:
//    lpszDesc       on succes name of TT file or description from exehdr
//    lpiFontType    set to a value based on Font type 1 == TT, 2 == Type1
//
// NOTE: Assumes that lpszDesc is of size DESCMAX
//
// returns:
//    TRUE success, FALSE failure
//
/////////////////////////////////////////////////////////////////////////////

BOOL ValidFontFile(LPTSTR lpszFile, LPTSTR lpszDesc, int *lpiFontType)
{
    BOOL    result;
    DWORD   dwBufSize;
    TCHAR   strbuf[PATHMAX];
    BUFTYPE File;
    BOOL    bTrueType = FALSE;

    *lpszDesc = (TCHAR) 0;
    *lpiFontType = NOT_TT_OR_T1;

    lstrcpy(File.name, lpszFile);

    if (IsTrueType(&File))
    {
        *lpiFontType = TRUETYPE_FONT;
        wsprintf(lpszDesc, TEXT("%s (%s)"), File.desc, szTrueType);
        return TRUE;
    }

    if (IsPSFont (NULL, lpszFile, lpszDesc, NULL, NULL, NULL, lpiFontType))
        return TRUE;

    result = FALSE;

    if (AddFontResource (lpszFile))
    {
        //  See if this is a TrueType font file

        dwBufSize = sizeof(BOOL);

        GetCurrentDirectory(PATHMAX, strbuf);
        BackslashTerm(strbuf);
        lstrcat(strbuf, lpszFile);

        result = GetFontResourceInfoW (strbuf,
                                       &dwBufSize,
                                       &bTrueType,
                                       GFRI_ISTRUETYPE);
        if (bTrueType)
            *lpiFontType = TRUETYPE_FONT;


        if (result)
        {
            dwBufSize = DESCMAX;

            result = GetFontResourceInfoW (strbuf,
                                           &dwBufSize,
                                           lpszDesc,
                                           GFRI_DESCRIPTION);

            if (result && bTrueType)
            {
                wsprintf(lpszDesc, TEXT("%s (%s)"), (LPTSTR)lpszDesc,
                                              (LPTSTR)szTrueType);
            }
        }
        RemoveFontResource (lpszFile);
    }
    return (result);
}

/////////////////////////////////////////////////////////////////////////////
//
// UniqueFilename
//
//   Guarantee a unique filename in a directory.  Do not overwrite existing
//   files.
//
/////////////////////////////////////////////////////////////////////////////

BOOL UniqueFilename (LPTSTR lpszDst, LPTSTR lpszSrc, LPTSTR lpszDir)
{
    TCHAR     szFullPath[PATHMAX];
    LPTSTR    lpszFile, lpszSrcExt, lpszDstExt;
    WORD      digit = 0;


    lstrcpy (szFullPath, lpszDir);

    lstrcpy (lpszFile = BackslashTerm(szFullPath), lpszSrc);

    if (!(lpszSrcExt = _tcschr(lpszSrc, TEXT('.'))))
        lpszSrcExt = szDot;

    if (OpenFileWithShare (szFullPath, NULL, OF_EXIST) == INVALID_HANDLE_VALUE)
        goto AllDone;

    if (!(lpszDstExt=_tcschr(lpszFile, TEXT('.'))))
        lpszDstExt = lpszFile + lstrlen(lpszFile);

    while (lpszDstExt-lpszFile < 7)
        *lpszDstExt++ = TEXT('_');

    do
    {
        TCHAR szTemp[8];

        wsprintf(szTemp, TEXT("%X"), digit++);
        if (digit++ > 0x4000)
            return (FALSE);
        lstrcpy (lpszFile + 8 - lstrlen(szTemp), szTemp);
        lstrcat (lpszFile, lpszSrcExt);
    }
    while (OpenFileWithShare (szFullPath, NULL, OF_EXIST) != INVALID_HANDLE_VALUE);

AllDone:
    lstrcpy (lpszDst, lpszFile);

    return(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
//
// AddFonts
//
// We now have a callback, so that this function is called before the
// browse dialog is dismissed, so that we don't have to do everything twice
//
//  This routine allows for installing the font files from their native
//  directory and either copying them to the FONTS dir, or leaving them
//  "InPlace".
//
/////////////////////////////////////////////////////////////////////////////

void AddFonts(HWND hwndFontDlg, BOOL bInPlace,
                                    NEXTFONT lpfnNextFont, void *lpData)
{
    TCHAR  szDesc[DESCMAX];
    TCHAR  szDir[PATHMAX];
    TCHAR  szTemp[PATHMAX];
    TCHAR  szTemp2[PATHMAX];
    TCHAR  szSrcName[PATHMAX];
    TCHAR  szDstName[PATHMAX];
    LPTSTR lpszDstDir;
    BOOL   bTrueType;
    BOOL   bDeletePSEntry;
    LPTSTR lpszDstName;
    BOOL   bThisOneInPlace;
    HWND   hListFonts;
    HWND   hAddFonts;
    WORD   wSrcDirLen, wCount;
    WORD   wMsg;
    int    rc;
    int    iFontType;
    int    ifType;
    int    iTotalFonts;
#ifdef JAPAN // support install font from *.inf
    int   InfId = 0 , PrevInfId = 0;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    TCHAR  szInfPath[PATHMAX];
    TUCHAR *pbInf = NULL;
    long  lFileSize;
#endif // JAPAN

    BOOL  bInShared = FALSE;
    BOOL  bFontsInstalled = FALSE;
    BOOL  bUpdateWinIni;
    struct tagDlgFontInfo *lpDlgFontInfo;

    LONG  lRet;
    HKEY  hkey;
    DWORD dwType;

#ifdef JAPAN
// We are installing font
    bInstallingFontAtBackground = TRUE;
#endif // JAPAN


    szDir[0] = TEXT('.');
    szDir[1] = TEXT('\\');
    szDir[2] = TEXT('\0');
    wSrcDirLen = 2;

    hListFonts = GetDlgItem (hwndFontDlg, LBOX_INSTALLED);
    SendMessage (hListFonts, LB_SETSEL, 0, -1L);

    BackslashTerm (szSharedDir);

    lpDlgFontInfo = (struct tagDlgFontInfo *) lpData;

    //
    //  Set local variables depending on who called AddFonts
    //

    if ((lpDlgFontInfo->iMagic == DFI_MAGIC) &&
        (lpDlgFontInfo->iSize == DFI_SIZE))
    {
        hAddFonts = lpDlgFontInfo->hDlg;

        //
        //  How many fonts do we have to install?
        //

        iTotalFonts = SendMessage (lpDlgFontInfo->hListDesc, LB_GETSELCOUNT, 0, 0);
    }
    else
    {
        HDROP *phdrop;

        phdrop = (HDROP *) lpData;

        hAddFonts = hwndFontDlg;

        //
        //  How many fonts do we have to install?
        //

        iTotalFonts = DragQueryFile (*phdrop, 0xffffffff, NULL, 0);
    }

    //
    //  Safeguards against causing Access violations in case bad
    //  values are returned.
    //

    if (!IsWindow (hAddFonts))
        hAddFonts = hwndFontDlg;

    if (!iTotalFonts)
        iTotalFonts = 1;

    bCancelInstall = FALSE;

    //
    //  Performance optimization - open the "fonts" registry key up
    //  early and keep it open so "Set" operations are must faster
    //  than using the "WriteProfileString" apis.
    //

    lRet = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                         szFontsKey,
                         0,
                         KEY_READ | KEY_WRITE,
                         &hkey);

    //
    //  I know "goto" statements are not 'cool' but because of the size
    //  of the install loops, it is actually easier to maintain this way.
    //

    if (lRet != ERROR_SUCCESS)
    {
        MyMessageBox (hwndFontDlg, MYFONT+50, MYFONT+20,
                      MB_OK | MB_ICONEXCLAMATION, (LPTSTR)NULL);
    
        return;
    }

    //
    //  Init Type1 font installation and Progress dialog
    //

    InitPSInstall ();
    InitProgress (hAddFonts);

    for (wCount=0; ; ++wCount)
    {
        //
        //  Check for user cancellation of installation
        //

        if (bCancelInstall)
            break;

        //
        //  Get next font to install
        //

        iFontType = (*lpfnNextFont)(wCount, lpData, szDstName,
                                         szSrcName, szDesc);

        bTrueType = (iFontType == TRUETYPE_FONT);

        bUpdateWinIni = TRUE;
        ifType = IF_OTHER;

        bDeletePSEntry = FALSE;

        //
        //  Check that a file name was given
        //

        if (!*szSrcName)
            break;

        //
        //  Update the overall progress dialog
        //

        UpdateProgress (iTotalFonts, wCount+1, wCount * 100 / iTotalFonts);

        //
        //  If they gave us a new dir, then check if it is remote
        //
        //  NOTE: The first call to NextFont routine returns the source
        //        directory in szDstName.  On subsequent calls this is NULL
        //        and our local szDir string holds the value.
        //

        if (*szDstName && (BackslashTerm (szDstName), lstrcmp (szDstName, szDir)))
        {
            lstrcpy (szDir, szDstName);
            wSrcDirLen = lstrlen (szDir);

            if (bInPlace)
            {
                switch (GetDriveType (szDir))
                {
                    case DRIVE_REMOTE:
                    case DRIVE_REMOVABLE:
                    case DRIVE_CDROM:
                    case DRIVE_RAMDISK:
                        if (MyMessageBox (hwndFontDlg, ERRORS+6, INITS+1,
                                          MB_YESNO|MB_ICONEXCLAMATION) != IDYES)
                            goto NoMoreFonts;
                }
            }
            bInShared = !_tcsicmp (szDir, szSharedDir);
        }

        CharUpper (szSrcName);
        lstrcpy (szDstName, szDir);
        lstrcat (szDstName, szSrcName);


        if (!*szDesc)
        {
            //
            //  Check if its a valid font file
            //

            if (!ValidFontFile(szDstName, szDesc, &iFontType))
            {
                bTrueType = (iFontType == TRUETYPE_FONT);

                //
                //  This is an invalid font
                //

                if (MyMessageBox(hwndFontDlg, bTrueType ? MYFONT+12 : MYFONT+11,
                            MYFONT+7, MB_OKCANCEL|MB_ICONEXCLAMATION,
                            (LPTSTR)(*szDesc?szDesc:szSrcName)) == IDCANCEL)
                        goto NoMoreFonts;
                goto NextSelection;
            }
        }

        if ((iFontType == TYPE1_FONT) || (iFontType == TYPE1_FONT_NC))
        {
            //
            //  szDstName has the full source file name
            //

            bThisOneInPlace = bInPlace || bInShared;

            if (bThisOneInPlace && !bInShared)
                lpszDstDir = szNull;
            else
                lpszDstDir = szSharedDir;

            //
            //  For installations involving the conversion of the Type1
            //  font to TrueType:
            //
            //         "szDstName" has the destination name of the
            //                     installed TrueType font file.
            //         "szDesc"    is munged to contain "(TrueType)".
            //

            switch (InstallT1Font (hAddFonts,
                                   hListFonts,       //  Installed fonts lbox
                                   !bThisOneInPlace, //  Copy TT file?
                                   bInShared,        //  Files in Shared Dir?
                                   szDstName,        //  PFM File & Directory
                                   szDesc))          //  Font description
            {
            case TYPE1_INSTALL_TT_AND_MPS:
                //
                //  The PS font was converted to TrueType and a matching
                //  PostScript font is ALREADY installed.
                //

                bDeletePSEntry = TRUE;

                //
                //  fall thru....

            case TYPE1_INSTALL_TT_AND_PS:
                //
                //  The PS font was converted to TrueType and the matching
                //  PostScript font was installed.
                //

                ifType = IF_TYPE1_TT;

                //
                //  fall thru....

            case TYPE1_INSTALL_TT_ONLY:
                //
                //
                //  The PS font was converted to TrueType and the matching
                //  PostScript font was NOT installed and a matching PS
                //  font was NOT found.
                //
                //  Setup variables to finish installation of converted
                //  TrueType font file.
                //
                //  NOTE:  In this case "ifType" already equals IF_OTHER
                //

                bUpdateWinIni =
                bTrueType = TRUE;

                //
                //  Reset install progress for TT version of font
                //

                ResetProgress ();
                Progress2 (0, szDesc);

                goto FinishTTInstall;


            case TYPE1_INSTALL_PS_AND_MTT:
                //
                //  The PostScript font was installed and we found a matching
                //  TrueType font that was already installed.  All Listbox
                //  manipulation was done in InstallT1Font routine.
                //
                //  fall thru....

            case TYPE1_INSTALL_PS_ONLY:
                //
                //  Only the PostScript font was installed.  All Listbox
                //  manipulation was done in InstallT1Font routine.
                //

                bUpdateWinIni = FALSE;
                goto FinishType1Install;

            case TYPE1_INSTALL_IDYES:
            case TYPE1_INSTALL_IDOK:
            case TYPE1_INSTALL_IDNO:
                //
                //  The font was not installed, but the User wanted to
                //  continue installation.  Continue installation with
                //  the next font.
                //
                //  The font was not installed due to an error somewhere
                //  and the User pressed OK in the MessageBox
                //
                //  OR
                //
                //  The User selected NO in the InstallPSDlg routine.
                //

                bUpdateWinIni = FALSE;
                goto NextSelection;

            case TYPE1_INSTALL_IDCANCEL:
                goto NoMoreFonts;

            default:
                //
                //  CANCEL and NOMEM (user already warned)
                //
                goto NoMoreFonts;
            }

            //
            //  On leaving this conditional many variables must be set up
            //  correctly to proceed with installation of a TrueType font.
            //
            //  szDesc     - fontname description for listbox display
            //  ifType     - itemdata to attach to TT lbox entry
            //  szSrcName  - filename of source font
            //  szDstName  - final destination filename
            //  lpszDstDir - points to destination path string
            //  bTrueType  - TRUE if Type1 file converted to TT
            //  bUpdateWinIni - FALSE if Type1 file not converted to TT
            //                  and used separatly to determine if [fonts]
            //                  section of win.ini (registry) should be
            //                  updated.
            //

        }

        //
        //  Start install progress for this font
        //

        ResetProgress ();
        Progress2 (0, szDesc);

#ifdef JAPAN // support install font from *.inf

    /*
        The routines , if the file is *.inf

        Jul.14.1993 -By- Hideyuki Nagase [hideyukn]
    */

        if( ( InfId = GetIdFontInf( szSrcName ) ) != 0 )
        {
            ATOM  Atom;
            BOOL  bRet;
            INT   iRet;
            int   ii;
            TUCHAR *pbStart , *pbEnd , *pbNow;
            TUCHAR szKey[DESCMAX] , szAtom[DESCMAX];

        /*
            Open and Read *.inf file into memory
        */
            if( PrevInfId != InfId )
            {
                if( hFile != INVALID_HANDLE_VALUE )
                    CloseInfFile( hFile , pbInf );

                hFile = OpenInfFile( szDstName , &pbInf , &lFileSize );
                if( hFile == INVALID_HANDLE_VALUE )
                {
                    InfId = 0;
                    goto NextSelection;
                }
            }
        /*
            Make a back up inf path
            We need path only , NO need filename
            This string should be termed with backslash
        */
            lstrcpy( szInfPath , szDstName );
            *( _tcsrchr( szInfPath , TEXT('\\') ) + 1 ) = TEXT('\0');
        /*
            Get Atom table Index
        */
            ii = 0x0fff & bTrueType;
        /*
            Get string that associated this atom
        */
            Atom = GetInfAtom( pInfAtomKeyTable[InfId] + ii , szAtom , DESCMAX );
            if( Atom == 0 )
                goto NextSelection;
        /*
            Prepare to Build font file
        */

        /*  Create outline data Section Name */

            szKey[0] = TEXT('[');
            szKey[1] = TEXT('\0');
            lstrcat( szKey , szAtom );
            lstrcat( szKey , OUTLINE_ID );
            lstrcat( szKey , TEXT("]") );

        /*  Setup disks infomation */

        /*
            if *.inf file is different from previous one, we have to re build
           disk info table
        */
            if( PrevInfId != InfId )
            {
                int         nDisks , ii;

            /* Free previous one */

                if( pInfDisksInfo != NULL )
                    FreeInfDisksTable( pInfDisksInfo );

            /* Search [disks] section */

                pbNow = GetInfSectionPointer( DISKS_SECTION , pbInf , lFileSize );

            /* Get the number of this section */

                nDisks = GetInfSectionMemberCount( pbNow );

            /* Allocate disk info table */

                pInfDisksInfo = AllocateInfDisksTable( nDisks );

                for( ii = 0 ; ii < nDisks ; ii ++ )
                {
                    TUCHAR szToken[DESCMAX];

                /* Get Disk Number */

                    GetInfLeftValue( pbNow , szToken , DESCMAX );

                    pInfDisksInfo[ii].AtomDisksNum = AddAtom( szToken );

                /* Get directory */

                    pbNow = GetInfRightValuePointer( pbNow );

                    pbNow = GetToken( pbNow , szToken , DESCMAX );

                    pInfDisksInfo[ii].AtomDisksDir = AddAtom( szToken );

                /* Get disks description */

                    pbNow = GetToken( pbNow , szToken , DESCMAX );

                    pInfDisksInfo[ii].AtomDisksDesc = AddAtom( szToken );

                /* Get disks id */

                    pbNow = GetToken( pbNow , szToken , DESCMAX );

                    pInfDisksInfo[ii].AtomDisksId = AddAtom( szToken );

                /* Move to Next Line */

                    pbNow = GetNextLineToken( pbNow );
                }
           }

        /*  *** Let's install outline data *** */

        /*
            Search Section
        */
            pbNow = GetInfSectionPointer( szKey , pbInf , lFileSize );
            if( pbNow == NULL )
                goto NextSelection;
        /*
            Check the member of this section ( outline section data member should be only 1 )
        */
            if( GetInfSectionMemberCount( pbNow ) != 1 )
                goto NextSelection;

        /*  Copy file name to szDstName */

            lstrcpy( szDstName , szSharedDir );
            lpszDstName = szDstName + lstrlen(szDstName);

        /*  Copy Left value ( left value presents file name after binding */

            GetInfLeftValue( pbNow , lpszDstName , PATHMAX - lstrlen(szDstName) );

        /*  szSrcName should be keep FontFile name ( without path ) */

            GetInfLeftValue( pbNow , szSrcName , PATHMAX );

        /*  Get right value */

            pbStart = GetInfRightValuePointer( pbNow );

        /*
            Install TrueType font
        */
            iRet = BuildAndOrCopyFont
                   (
                       hwndFontDlg , // Base windows handle
                       szDstName ,   // Distination Filename and Path
                       szInfPath ,   // Inf file directory ( Def.inst path )
                       pbStart       // File Build Info
                   );

            switch( iRet )
            {
            case TT_INSTALL_SUCCESS :
                goto StartBitmapInstall;

            case TT_INSTALL_ALREADY :
                goto StartBitmapInstall;
                // goto EndBitmapInstall;

            case TT_INSTALL_CANCEL :
            case TT_INSTALL_FAIL :
            default :
                goto NextSelection;
            }

        /* *** Let's install bitmap font data *** */

StartBitmapInstall:

        /*  Create bitmap data Section Name */

            szKey[0] = TEXT('[');
            szKey[1] = TEXT('\0');
            lstrcat( szKey , szAtom );
            lstrcat( szKey , BITMAP_ID );
            lstrcat( szKey , TEXT("]") );

        /*
           Search section
        */
            pbNow = GetInfSectionPointer( szKey , pbInf , lFileSize );

        /* If bitmap font section is not exit , we need to process following */
            if( pbNow == NULL )
                goto EndBitmapInstall;

        /* Install bitmap font */

            bRet = bInstallBitmapFont
                   (
                       hwndFontDlg , // Base window handle
                       szSrcName ,   // Truetype font file name
                       szSharedDir , // Distination Install path
                       szInfPath ,   // Inf file directory ( def. inst.path )
                       pbNow         // File Build Info
                   );

            if( bRet != TRUE )
            {
                TUCHAR szString[256];

                LoadString( hModule , MYFONT+31 , szString , CharSizeOf(szString) );
                if( IDYES
                     !=
                    MessageBox( hwndFontDlg , szString , NULL ,
                                MB_YESNO | MB_ICONEXCLAMATION )
                  )
                {
                    /* User cancel all font of this installation */
                    /* Delete font files */
                    DeleteFile (szDstName); /* Outline font */
                    DeleteBitmapFont (hwndFontDlg, szDstName, TRUE); /* Bitmap font */
                    goto NextSelection;
                }
                else
                {
                    /* User cancel only bitmap font of installation */
                    DeleteBitmapFont (hwndFontDlg, szDstName, TRUE); /* Bitmap font */
                }
            }

EndBitmapInstall:

            bInPlace = TRUE;
        }

#endif // JAPAN

        //////////////////////////////////////////////////////////////////////
        // Check if font is already loaded on the system
        //////////////////////////////////////////////////////////////////////

        //
        //  If the description string from the Font file, exactly matches
        //  that of one we already have in Win.Ini, the font is already
        //  installed in the system.
        //

        lRet =  RegQueryValueEx (hkey, szDesc, 0, &dwType, 0, 0);

        if (lRet == ERROR_SUCCESS)
        {
            //
            // "Font is already loaded"
            //
            int iTemp =  MyMessageBox (hwndFontDlg,
                                       MYFONT+21,
                                       INITS+1,
                                       MB_OKCANCEL | MB_ICONEXCLAMATION,
                                       (LPTSTR)szDesc);

            if (iTemp == IDCANCEL)
                goto NoMoreFonts;

            goto NextSelection;
        }

        //////////////////////////////////////////////////////////////////////
        // If USER asked us NOT to copy file to "fonts" directory AND it
        // is a compressed file, ask them if we should copy it anyway
        //////////////////////////////////////////////////////////////////////

        if (bThisOneInPlace = bInPlace || bInShared)
        {
            if (IsCompressed(szDstName))
            {
                if (bInPlace)
                {
                    //
                    // "Font is compressed"
                    //

                    switch (MyMessageBox(hwndFontDlg, ERRORS+9, INITS+1,
                            MB_YESNOCANCEL|MB_ICONEXCLAMATION, (LPTSTR)szDesc))
                    {
                    case IDCANCEL:
                        goto NoMoreFonts;

                    case IDNO:
                        goto NextSelection;

                    default:
                        //
                        //  This is IDYES or NOMEM
                        //
                        break;
                    }
                }

                bThisOneInPlace = FALSE;
            }
        }


#ifdef WIN31_WAY
#ifdef NOTTINPLACE
        if (bThisOneInPlace && (GetModuleHandle(szSrcName)>=(HANDLE)1 ||
                        (!bInShared && bTrueType)))
#else
        if (bThisOneInPlace && !bTrueType && !bInShared &&
                    GetModuleHandle(szSrcName)>(HANDLE)1)
#endif
#endif  //  WIN31_WAY

        //  NT way - supports TT directly, can't do a GetModuleHandle on TT
        //  or "compressed" or .fon files
        //
        //  if (bThisOneInPlace && !bTrueType && !bInShared)
        //

        if (bThisOneInPlace && (iFontType == NOT_TT_OR_T1) && !bInShared)
        {
            /* BUG: We still need to write win.ini line
             */
            if (bInShared)
                goto NextSelection;

            /* "Cannot use font in place; do you want to install in
             * the shared dir?"
             */
            bThisOneInPlace = FALSE;
        }

        //
        //  szDstName should already have the full source file name
        //

        if (bThisOneInPlace)
        {
            if (bTrueType && !bInShared)
            {
                lpszDstDir = szNull;
            }
            else
            {
                lstrcpy(szDstName, szSrcName);
                lpszDstDir = szDir;
            }
        }
        else
        {
            //////////////////////////////////////////////////////////////////
            // COPY files to "fonts" directory
            //////////////////////////////////////////////////////////////////

            LPTSTR pszFiles[2];

            /* Let LZ tell us what the name should have been
             */
            GetExpandedName (szDstName, szDstName);
            lstrcpy (szDstName, szDstName+wSrcDirLen);


            // [stevecat] Test code to FORCE the destination filename extension
            //  5/27/92   of TrueType files to .TTF for the COPY operation
            //
            // NOTE: This is because the LZ A algorithm does not correctly restore
            //       the trailing 'F' char for when the file is uncompressed.

            if (bTrueType)
                ConvertExtension(szDstName, szTTF);


            if (!(UniqueFilename (szDstName, szDstName, szSharedDir)))
            {
                if (MyMessageBox (hwndFontDlg, MYFONT+19, MYFONT+7,
                    MB_OKCANCEL | MB_ICONEXCLAMATION, (LPTSTR)szDesc) ==IDCANCEL)
                    goto NoMoreFonts;
                goto NextSelection;
            }
            lpszDstDir = szSharedDir;

            pszFiles[0] = szSrcName;
            pszFiles[1] = szDstName;

            lstrcpy (szDirOfSrc, szDir);

            //  Build Destination file pathname

            lstrcpy (szTemp2, szDstName);
            lstrcpy (szDstName, szSharedDir);

            pszFiles[1] = lstrcat (szDstName, szTemp2);

            //  Build Source file pathname

            lstrcpy (szTemp, szSrcName);
            lstrcpy (szSrcName, szDir);

            pszFiles[0] = lstrcat (szSrcName, szTemp);

            if ((rc = Copy (hwndFontDlg, pszFiles[0], pszFiles[1])) <= 0)
            {
                switch (rc)
                {
                    //  On these two return codes, the USER has effectively
                    //  "Cancelled" the copy operation for the fonts
                case COPY_CANCEL:
                case COPY_DRIVEOPEN:
                    goto NoMoreFonts;

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

                if (MyMessageBox (hwndFontDlg, wMsg, INITS+1,
                                  MB_OKCANCEL | MB_ICONEXCLAMATION,
                                  szDstName, szSrcName) == IDCANCEL)
                {
                    goto NoMoreFonts;
                }
                else
                {
                    goto NextSelection;
                }
            }

            //  Reset names to just bare Filenames

            lstrcpy (szSrcName, szTemp);
            lstrcpy (szDstName, szTemp2);
        }

        //////////////////////////////////////////////////////////////////////
        // If it is a TrueType font, generate a *.FOT file
        //////////////////////////////////////////////////////////////////////

FinishTTInstall:

        if (bTrueType)
        {
            HDC hDC;

            //
            //  Update install progress for this font - about half done
            //  since TrueType file has been copied, if necessary.
            //

            Progress2 (50, szDesc);

            hDC = GetDC (NULL);

            lstrcpy(szTemp, szSharedDir);
            ConvertExtension(szSrcName, szFOT);

            //  Special GDI patch ??? - may not be needed by NT WIN GDI

            if (*lpszDstDir)
                *(BackslashTerm (lpszDstDir)-1) = TEXT('\0');

            if (!UniqueFilename (lpszDstName=BackslashTerm(szTemp),
                                szSrcName, szSharedDir) ||
                                !CreateScalableFontResource (0, szTemp,
                                                             szDstName,
                                                             lpszDstDir))
            {
                ReleaseDC (NULL, hDC);

                //  Fix directory back with trailing backslash

                if (*lpszDstDir)
                    BackslashTerm (lpszDstDir);

                if (!bThisOneInPlace)
                {
                    lstrcpy (szTemp, szSharedDir);
                    lstrcpy (BackslashTerm (szTemp), szDstName);
                    DeleteFile (szTemp);
                }

                if (MyMessageBox (hwndFontDlg,
                                  TTEnabled() ? MYFONT+16 : MYFONT+22,
                                  INITS+1,
                                  MB_OKCANCEL | MB_ICONEXCLAMATION, szDesc)
                    == IDCANCEL)
                    goto NoMoreFonts;

                goto NextSelection;
            }

            //  Fix directory back with trailing backslash

            if (*lpszDstDir)
                BackslashTerm (lpszDstDir);

            ReleaseDC(NULL, hDC);
        }
        else
        {
            lpszDstName = szDstName;
            lstrcpy(szTemp, lpszDstDir);
            lstrcpy(BackslashTerm(szTemp), szDstName);
        }

        //////////////////////////////////////////////////////////////////////
        // Add the font resource
        //////////////////////////////////////////////////////////////////////

        if (!AddFontResource(szTemp))
        {
ErrorPath:
            //
            // Delete installed font file(s)
            //

            if (bTrueType || !bThisOneInPlace)
            {
                DeleteFile (szTemp);

                if (bTrueType && !bThisOneInPlace)
                {
                    lstrcpy (szTemp, szSharedDir);
                    lstrcpy (BackslashTerm (szTemp), szDstName);
                    DeleteFile (szTemp);
                }
            }

            //
            //  Report error adding font resource
            //

            if (MyMessageBox (hwndFontDlg, MYFONT+19, MYFONT+20,
                              MB_OKCANCEL | MB_ICONEXCLAMATION,
                              (LPTSTR)szDesc) == IDCANCEL)
                goto NoMoreFonts;

            goto NextSelection;
        }

        //////////////////////////////////////////////////////////////////////
        // Write string to win.ini and add font to Parent window listbox
        //////////////////////////////////////////////////////////////////////

        if (bUpdateWinIni)
        {
            TCHAR *pwszData = (bInShared || !bInPlace) ? lpszDstName : szTemp;

            lRet = RegSetValueEx (hkey, szDesc, 0, REG_SZ, (BYTE*) pwszData,
                                  (DWORD) (sizeof(TCHAR) * (lstrlen(pwszData) + 1)));

            if (lRet == ERROR_NOT_ENOUGH_MEMORY)
            {
                //
                //  Report registry out of memory error when adding
                //  fonts to system.  Tell user to increase registry
                //  quota using system applet and then try re-installing
                //  fonts later (after reboot).
                //
    
                MyMessageBox (hwndFontDlg, ERRORS+8, MYFONT+20,
                              MB_OK | MB_ICONEXCLAMATION,
                              (LPTSTR)NULL);
    
                goto NoMoreFonts;
            }
            else if (lRet != ERROR_SUCCESS)
            {
                //
                //  For any other error - like FONTS keys being
                //  write protected, remove font and report error
                //

                RemoveFontResource (szTemp);
                goto ErrorPath;
            }
        }

        //
        //  Update install progress for this font
        //

        Progress2 (100, szDesc);

        rc = SendMessage (hListFonts, LB_ADDSTRING, 0, (LONG)(LPTSTR)szDesc);

        //
        //  Attach font type to each listed font
        //

        SendMessage (hListFonts, LB_SETITEMDATA, rc, ifType);

        SendMessage (hListFonts, LB_SETSEL, 1, rc);

        //
        //  If a matching PostScript version of the font was found, delete
        //  the Listbox entry for it since we have just added a TrueType
        //  entry with type IF_TYPE1_TT for that font.
        //

        if (bDeletePSEntry)
        {
            //
            //  Change font description to have "(PostScript)" now
            //

            RemoveDecoration (szDesc, TRUE);

            wsprintf (szDesc, TEXT("%s (%s)"), szDesc, szPostScript);

            rc= SendMessage (hListFonts, LB_FINDSTRINGEXACT, (WPARAM) -1,
                                                         (LONG)szDesc);

            if (rc != LB_ERR)
                SendMessage (hListFonts, LB_DELETESTRING, rc, 0L);
        }

        UpdateWindow (hListFonts);

FinishType1Install:

        bFontsInstalled = TRUE;

NextSelection:

#ifdef JAPAN // support install font from *.inf

    /*  keep current *.inf id */

    PrevInfId = InfId;

#endif // JAPAN

        ;

    }

NoMoreFonts:
    RegCloseKey(hkey);

    //
    //  Update the overall progress dialog - show a 100% message
    //

    UpdateProgress (iTotalFonts, iTotalFonts, 100);

    Sleep (1000);

    TermProgress ();
    TermPSInstall ();

#ifdef JAPAN // support install font from *.inf

    /*  Close Inf file  */

    if (hFile != INVALID_HANDLE_VALUE )
        CloseInfFile( hFile , pbInf );

    /*  Delete Font Description atom */

    for (InfId = 0 ; InfId < INF_ID_MAX ; InfId ++ )
        FreeInfAtomTable( pInfAtomKeyTable[InfId] );

#endif // JAPAN

    if (bFontsInstalled)
    {
        SendWinIniChange (szFonts);
        SetDlgItemText (hwndFontDlg, IDOK, pszClose);
        nCurSel = -1;
        FontSelChange (hwndFontDlg);
    }
#ifdef JAPAN
    // We done install font
    bInstallingFontAtBackground = FALSE;
#endif // JAPAN

}


/////////////////////////////////////////////////////////////////////////////
//
// NextDropFont
//
//  drag drop file install.  with SHIFT key pressed we do in place.
//
/////////////////////////////////////////////////////////////////////////////

BOOL NextDropFont (WORD wCount, HANDLE *lphDrop,
                                    LPTSTR szDir, LPTSTR szFile, LPTSTR szDesc)
{
    LPTSTR lpTemp;

    /* Return no file if there is an error; always return no description
     */
    *szFile = *szDesc = TEXT('\0');

    if (!DragQueryFile (*lphDrop, wCount, szDir, PATHMAX))
        return(0);
    if (!(lpTemp = _tcsrchr (szDir, TEXT('\\'))))
        return(0);

    /* Get the file name, and terminate the dir name
     */
    ++lpTemp;
    lstrcpy (szFile, lpTemp);
    *lpTemp = TEXT('\0');

    /* The return value means nothing since there is no description
     */
    return(0);
}

/////////////////////////////////////////////////////////////////////////////
//
// FontsDropped
//
/////////////////////////////////////////////////////////////////////////////

void FontsDropped (HWND hwnd, HANDLE hDrop)
{
    AddFonts (hwnd, GetKeyState(VK_SHIFT)<0, (void *) NextDropFont, (void *) &hDrop);
    DragFinish (hDrop);
}


/////////////////////////////////////////////////////////////////////////////
//
// FilesToDescs
//
/////////////////////////////////////////////////////////////////////////////

int nFontsToGo = 0;
int nNumFonts;
HWND hWndAddFonts = NULL;
int nSelItem = -1;

VOID FilesToDescs(VOID)
{
    int  nItem;
    TCHAR szFileName[MAXFILE];
    TCHAR szDesc[DESCMAX];
//    TCHAR szTemp[DESCMAX];
    TCHAR szFontsRead[PATHMAX];
    int  iFontType;
    MSG  msg;
    HWND hListFiles, hListDesc, hStat, hwndAll;
#ifdef JAPAN // support install font from *.inf
    int  InfId = 0;

// If we are installing font in background,
// We have to avoid enter this routine

    if( bInstallingFontAtBackground )
        return;

#endif // JAPAN

    if (!hWndAddFonts || !nFontsToGo)
        return;

    hListFiles = GetDlgItem(hWndAddFonts, lst1);
    hListDesc  = GetDlgItem(hWndAddFonts, ctlLast+1);
    hStat      = GetDlgItem(hWndAddFonts, ctlLast+3);
    hwndAll    = GetDlgItem(hWndAddFonts, psh16);

    if (nFontsToGo < 0)
    {
        if (GetFocus()==hListDesc || GetFocus()==hwndAll)
            SendMessage(hWndAddFonts, WM_NEXTDLGCTL,
                               (DWORD) GetDlgItem(hWndAddFonts, lst2), 1L);

        EnableWindow(hListDesc, FALSE);
        EnableWindow(hwndAll, FALSE);
        SendMessage(hListDesc, LB_RESETCONTENT, 0, 0L);
        UpdateWindow(hListDesc);

        SendMessage(hStat, WM_SETTEXT, 0, (LONG)(LPTSTR)szNull);

        if (!(nNumFonts=nFontsToGo=(int)SendMessage(hListFiles, LB_GETCOUNT,
                                    0, 0L)))
            goto ThereAreNoFonts;

        SendMessage(hListDesc, WM_SETREDRAW, 0, 0L);
    }

    LoadString (hModule, MYFONT+23, szFontsRead, CharSizeOf(szFontsRead));
    /* We want to read at least one
     */
    goto ReadNext;

    for ( ; nFontsToGo; )
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
            return;

ReadNext:
        if (SendMessage(hListFiles, LB_GETTEXT, --nFontsToGo,
                                    (LONG)(LPTSTR)szFileName) == LB_ERR)
            continue;

        wsprintf(szDesc, szFontsRead,
                    (int)((long)(nNumFonts-nFontsToGo)*100/nNumFonts));
        SendMessage(hStat, WM_SETTEXT, 0, (LONG)(LPTSTR)szDesc);

#ifdef JAPAN // support Install font from *.inf

    /*
        The routines if the file is *.inf

        14.Jul.1993 -By- Hideyuki Nagase [hideyukn]
    */
    /*
        Check the file that is given by File List box is oemsetup.inf or setup.inf
    */
        if ( ( InfId = GetIdFontInf(szFileName) ) != 0 )
        {
            HANDLE hInfFile;
            BOOL   bRet;
            int    nFontCount , ii;
            long   lFileSize;
            unsigned short usStatus;
            TUCHAR *pbInf = NULL , *pbNow = NULL;
        /*
            Open *.Inf file and read it to memory
        */
            hInfFile = OpenInfFile( szFileName , &pbInf , &lFileSize );
            if( hInfFile == INVALID_HANDLE_VALUE )
                continue;
        /*
            Search TrueType font section
        */
            pbNow = GetInfSectionPointer( TRUETYPE_FONT_SECTION , pbInf , lFileSize );
            if( pbNow == NULL )
            {
                CloseInfFile( hInfFile , pbInf );
                continue;
            }
        /*
            Now bpNow point the top member in specified section
        */
        /*
            Set number of font in *.inf
        */
            nFontCount = GetInfSectionMemberCount( pbNow );
        /*
            Free old Inf Atoms
        */
            FreeInfAtomTable( pInfAtomKeyTable[InfId] );
            pInfAtomKeyTable[InfId] = NULL;
        /*
            Allocate Atom key table
        */
            pInfAtomKeyTable[InfId] = AllocateInfAtomTable( nFontCount );

            if( pInfAtomKeyTable == NULL ) continue;

        /* Read Description */

            for( ii = 0 ; ii < nFontCount ; ii ++ )
            {
            /*
                Is current pointer valid ?
            */
                if( pbNow == NULL )
                    break;
            /*
                Get Description string
            */
                bRet = GetInfDescription( pbNow , szDesc , DESCMAX );
                if( !bRet )
                    break;
            /*
                Send description string to List box
            */
                nItem = (int)SendMessage( hListDesc , LB_ADDSTRING , 0 , (LONG)(LPSTR)szDesc );
                if( nItem == LB_ERR )
                    continue;
            /*
                Send Item data to List box
            */
                usStatus = TRUETYPE_WITH_INF | ( 0x0fff & ii );

                SendMessage( hListDesc , LB_SETITEMDATA , nItem ,
                                       MAKELONG( nFontsToGo , usStatus ) );
            /*
                Keep key value ( left value ) into atom table
            */
                SetInfAtom( pInfAtomKeyTable[InfId] + ii , pbNow );
            /*
                Move to Next Line
            */
                pbNow = GetNextLineToken( pbNow );
            }
        /*
            Close Inf File
        */
            CloseInfFile( hInfFile , pbInf );
        }
         else
        {
#endif // JAPAN
        if (!ValidFontFile (szFileName, szDesc, &iFontType) || !szDesc[0])
            continue;

        if ((nItem = (int)SendMessage (hListDesc, LB_ADDSTRING, 0,
                                       (LONG)szDesc)) == LB_ERR)
            continue;

// [stevecat] What is this for?? Double reads of same file?  A hack to
//            cover-up double processing of same font?
//        if ((SendMessage(hListDesc, LB_GETTEXT, nItem-1, (LONG)(LPSTR)szTemp)
//                        !=LB_ERR && !strcmp(szTemp, szDesc)) ||
//            (SendMessage(hListDesc, LB_GETTEXT, nItem+1, (LONG)(LPSTR)szTemp)
//            !=LB_ERR && !strcmp(szTemp, szDesc)))
//            SendMessage(hListDesc, LB_DELETESTRING, nItem, 0L);

        SendMessage(hListDesc, LB_SETITEMDATA, nItem,
                                    MAKELONG(nFontsToGo, iFontType));
#ifdef JAPAN // support Install font from *.inf
        }
#endif // JAPAN
    }

    SendMessage(hStat, WM_SETTEXT, 0, (LONG)(LPTSTR)szNull);

    if ((int)SendMessage(hListDesc, LB_GETCOUNT, 0, 0L) > 0)
    {
        EnableWindow(hListDesc, TRUE);
        EnableWindow(hwndAll, TRUE);
    }
    else
    {
ThereAreNoFonts:
        LoadString (hModule, MYFONT+29, szFontsRead, CharSizeOf(szFontsRead));
        SendMessage (hListDesc, LB_ADDSTRING, 0, (LONG)szFontsRead);
    }

    SendMessage(hListDesc, WM_SETREDRAW, 1, 0L);
    InvalidateRect(hListDesc, NULL, TRUE);
}


/////////////////////////////////////////////////////////////////////////////
//
// NextDlgFont
//
/////////////////////////////////////////////////////////////////////////////

int NextDlgFont (WORD wCount, struct tagDlgFontInfo *lpDlgFontInfo,
                                LPTSTR szDir, LPTSTR szFile, LPTSTR szDesc)
{
    int nSel;
    DWORD dwSel;
    LPTSTR lpTemp;

    /* If we bug out early, then return with no file (meaning the end)
     */
    *szDir = *szFile = *szDesc = TEXT('\0');

    /* Check that some file is still selected
     */
    if (!(WORD)SendMessage(lpDlgFontInfo->hListDesc, LB_GETSELITEMS, 1,
      (LONG)(LPTSTR)&nSel))
        return(0);

    /* Fill in the directory if this is the first time through
     */
    if (wCount == 0)
    {
        lstrcpy(szDir, lpDlgFontInfo->lpOfn->lpstrFile);
        if (!(lpTemp = _tcsrchr(szDir, TEXT('\\'))))
            return(0);
        lpTemp[1] = TEXT('\0');
    }

    /* Unselect the first selected file, and get its desc, filename,
     * and return whether it is truetype
     */
    SendMessage (lpDlgFontInfo->hListDesc, LB_SETSEL, 0, (LONG)nSel);

    SendMessage (lpDlgFontInfo->hListDesc, LB_GETTEXT, nSel, (LONG)szDesc);

    dwSel = SendMessage (lpDlgFontInfo->hListDesc, LB_GETITEMDATA, nSel, 0L);

    SendMessage (lpDlgFontInfo->hListFiles, LB_GETTEXT, LOWORD(dwSel),
                                                                (LONG)szFile);
    return((int)HIWORD(dwSel));
}


/////////////////////////////////////////////////////////////////////////////
//
// FontHookProc
//
//  Hooks into common dialog to allow size of selected files to be displayed.
//
/////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY FontHookProc(HWND hDlg, UINT iMessage, WPARAM wParam, LONG lParam)
{
    HWND hListFiles, hListDesc;
    int nCount;

    switch (iMessage)
    {
    case WM_INITDIALOG:
        hWndAddFonts = hDlg;
        CheckDlgButton(hDlg, chx2, TRUE);
        nFontsToGo = -1;
        break;

    case WM_DESTROY:
        hWndAddFonts = NULL;
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ctlLast+1:
            switch (HIWORD(wParam))
            {
            case LBN_DBLCLK:
                PostMessage(hDlg, WM_COMMAND, MAKELONG(IDOK, BN_CLICKED),
                                            (LONG) GetDlgItem(hDlg, IDOK));
                break;

            case LBN_SELCHANGE:
DescToFiles:
                hListFiles = GetDlgItem(hDlg, lst1);
                hListDesc = GetDlgItem(hDlg, ctlLast+1);

                if (SendMessage(hListDesc, LB_GETSELCOUNT, 0, 0L) > 0)
                {
                    /* Just select the first file if there is any selection
                     */
                    SendMessage(hListDesc, LB_GETSELITEMS, 1,
                                                        (LONG)(LPTSTR)&nCount);
                    nCount = (int)SendMessage(hListDesc, LB_GETITEMDATA,
                                                                nCount, 0L);
                    SendMessage(hListFiles, LB_SETCURSEL, LOWORD(nCount), 0L);
                }
                else
                {
                    /* Select no files and clear the edit field
                     */
                    SendMessage(hListFiles, LB_SETCURSEL, (WPARAM)(LONG)-1, 0L);
                    SendDlgItemMessage(hDlg, edt1, WM_SETTEXT, 0,
                                                        (LONG)(LPTSTR)szNull);
                }

                SendMessage(hDlg, WM_COMMAND, MAKELONG(lst1, LBN_SELCHANGE),
                                                          (LONG) hListFiles);
            }
            break;

        case psh16:    // select all
            SendDlgItemMessage(hDlg, ctlLast+1, LB_SETSEL, TRUE, -1L);
            goto DescToFiles;

        case lst2:
            if (HIWORD(wParam) == LBN_DBLCLK)
                nFontsToGo = -1;
            break;

        case cmb2:
            switch (HIWORD(wParam))
            {
            case CBN_DROPDOWN:
                nSelItem = (int)SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0L);
                break;

            case CBN_CLOSEUP:
                if (nSelItem != (int)SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0L))
                    nFontsToGo = -1;
                nSelItem = -1;
                break;

            case CBN_SELCHANGE:
                if (nSelItem == -1)
                    nFontsToGo = -1;
                break;
            }
            break;

        case IDOK:
        case IDCANCEL:
        case IDABORT:
            nFontsToGo = -1;
            break;
        }
        break;

    default:
        if (iMessage == wBrowseDoneMessage)
        {
            HWND hFontDlg;
            struct tagDlgFontInfo DlgFontInfo;

            DlgFontInfo.lpOfn = (LPOPENFILENAME)lParam;
            EnableWindow (hFontDlg=DlgFontInfo.lpOfn->hwndOwner, TRUE);
            SetFocus (hFontDlg);
            DlgFontInfo.hListFiles = GetDlgItem (hDlg, lst1);
            DlgFontInfo.hListDesc = GetDlgItem (hDlg, ctlLast+1);
            DlgFontInfo.hDlg = hDlg;
            DlgFontInfo.iMagic = DFI_MAGIC;
            DlgFontInfo.iSize  = DFI_SIZE;

            //
            //  This is in NT 3.5 release, but causes a problem with
            //  returning focus back to Font dialog boxes if a user
            //  switches apps during installation.  So, now I no longer
            //  hide it.
            //
            //  ShowWindow(hDlg, SW_HIDE);

            //
            // force buttons to repaint
            //

            UpdateWindow (hFontDlg);

            AddFonts (hFontDlg, !IsDlgButtonChecked (hDlg, chx2),
                                (void *) NextDlgFont, (void *) &DlgFontInfo);
        }
        else if (iMessage == wHelpMessage)
        {
            CPHelp (hDlg);
            return TRUE;
        }

        break;
    }

    return FALSE;  // commdlg, do your thing
}


#ifdef JAPAN  // support install font from *.inf file

/*
    Support routines for font instollation from inf file

    Jul.14.1993 -By- Hideyuki Nagase [hideyukn]
*/

DISKS_INFO *
AllocateInfDisksTable
(
    int nCount
)
{
    DISKS_INFO *pDisks;

    pDisks = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT , sizeof( DISKS_INFO ) * ( nCount + 1 ) );

    return pDisks;
}

void
FreeInfDisksTable
(
    DISKS_INFO *pDisks
)
{
    DISKS_INFO *pDisksTop = pDisks;

    if( pDisks == NULL ) return;

    while( pDisks->AtomDisksNum != 0 )
    {
        DeleteAtom( pDisks->AtomDisksNum );
        DeleteAtom( pDisks->AtomDisksDir );
        DeleteAtom( pDisks->AtomDisksDesc );
        DeleteAtom( pDisks->AtomDisksId );
        pDisks ++;
    }

    LocalFree( pDisksTop );
}

INT
SearchInfDisksAtom
(
    DISKS_INFO *pDisks ,
    ATOM        AtomNum
)
{
    DISKS_INFO *pDisksTop = pDisks;
    INT        ii = 0;

    if( pDisks == NULL ) return( -1 );

    while( pDisks->AtomDisksNum != 0 )
    {
        if( pDisks->AtomDisksNum == AtomNum )
            return( ii );

        pDisks ++;
        ii ++;
    }
    return( -1 );
}

ATOM *
AllocateInfAtomTable
(
    int nCount
)
{
    ATOM *pAtom;

    pAtom = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT , sizeof( ATOM ) * ( nCount + 1 ) );

    return( pAtom );
}

void
FreeInfAtomTable
(
    ATOM *pAtom
)
{
    if( pAtom == NULL ) return;

    while( *pAtom != 0 )
    {
        DeleteAtom( *pAtom );
        pAtom ++;
    }
}

int
GetIdFontInf
(
    TUCHAR *szFileName
)
{
    if( szFileName == NULL ) return 0;

    if( lstrcmpi( szFileName , OEMSETUPINF ) == 0 )
        return OEMSETUPINF_ID;

    if( lstrcmpi( szFileName , SETUPINF ) == 0 )
        return SETUPINF_ID;

    return 0;
}

HANDLE
OpenInfFile
(
    TUCHAR *szFileName ,
    TUCHAR **ppbInfData ,
    long   *plFileSize
)
{
    HANDLE hInfFile;
    long  lFileSize;
    TUCHAR *pbInf;
    TUCHAR aszInfPath[ MAX_PATH ];
/*
    Get the directory where oemsetup.inf is in
*/
    if( _tcschr( szFileName , TEXT('\\') ) == NULL )
    {
        GetCurrentDirectory( MAX_PATH , aszInfPath );
        BackslashTerm( aszInfPath );
        lstrcat( aszInfPath , szFileName );
    }
     else
    {
        lstrcpy( aszInfPath , szFileName );
    }
/*
    Open it
*/
    hInfFile = MyOpenFile( aszInfPath , NULL, OF_READ );
    if( hInfFile == INVALID_HANDLE_VALUE )
        return INVALID_HANDLE_VALUE;
/*
    Get file size
*/
/*  Move to EOF */
    lFileSize = MyFileSeek( hInfFile , 0L , 2 );
/*  Back to top */
    MyFileSeek( hInfFile , 0L , 0 );
/*
    Allocate memory to keep inf file data
*/
    pbInf = LocalAlloc( LMEM_FIXED , ByteCountOf(lFileSize + 2) );
    if( pbInf == NULL )
    {
        MyCloseFile( hInfFile );
        return INVALID_HANDLE_VALUE;
    }
/*
    Read file into memory
*/
    MyAnsiReadFile( hInfFile , CP_OEMCP, pbInf , lFileSize );

    *(pbInf + lFileSize + 1) = TEXT('\n');
    *(pbInf + lFileSize + 2) = TEXT('\0');
/*
    Prepare to return
*/
    MyCloseFile( hInfFile );

    *ppbInfData = pbInf;
    *plFileSize = lFileSize;

    return( hInfFile );
}

void
CloseInfFile
(
    HANDLE hInfFile ,
    TUCHAR *pbInfData
)
{
    UNREFERENCED_PARAMETER( hInfFile );

    if( pbInfData != NULL ) LocalFree( pbInfData );
}

TUCHAR *
SkipWhite
(
    TUCHAR *lpch
)
{
    if( lpch == NULL ) return( NULL );

    for ( ; ; lpch=CharNext(lpch))
    {
        switch (*lpch)
        {
        case TEXT(' '):
        case TEXT('\t'):
        case TEXT('\r'):
        case TEXT('\n'):
            break;

        default:
            return(lpch);
        }
    }
}

TUCHAR *
BackWhite
(
    TUCHAR *lpch ,
    TUCHAR *lpchTop
)
{
    if( lpch == NULL ) return( NULL );

    for ( ; ; lpch=CharPrev(lpchTop,lpch))
    {
        switch (*lpch)
        {
        case TEXT(' '):
        case TEXT('\t'):
        case TEXT('\r'):
        case TEXT('\n'):
            break;

        default:
            return(lpch);
        }
    }
}

BOOL
IsSection
(
    TUCHAR *lpch
)
{
    if( lpch == NULL ) return FALSE;

    if( *lpch == TEXT('[') ) return TRUE;

    return FALSE;
}

BOOL
IsInfMember
(
    TUCHAR *lpch
)
{
    TUCHAR *LineEnd;

    LineEnd = _tcschr( lpch , TEXT('\n') );

    if( LineEnd == NULL ) return( FALSE );

    while( lpch <= LineEnd )
        if( *lpch++ == TEXT('=') ) return( TRUE );

    return( FALSE );
}

TUCHAR *
GoNextLine
(
    TUCHAR *lpch
)
{
    return( _tcschr( lpch , TEXT('\n') ) );
}

TUCHAR *
GetNextLineToken
(
    TUCHAR *lpch
)
{
    lpch = GoNextLine( lpch );

    if( lpch != NULL )
        lpch = SkipWhite( lpch );

     return lpch;
}

TUCHAR *
GetInfRightValuePointer
(
    TUCHAR *lpch
)
{
    lpch = _tcschr( lpch , TEXT('=') );

    lpch ++;

    return( SkipWhite( lpch ) );
}

TUCHAR *
GetInfLeftValue
(
    TUCHAR *lpch ,
    TUCHAR *szBuffer ,
    int     limit
)
{
    TUCHAR *lpchTop = lpch;

    lpch = _tcschr( lpch , TEXT('=') );

    lpch --;

    lpch = BackWhite( lpch , lpchTop );

    while( lpchTop <= lpch && --limit )
        *szBuffer ++ = *lpchTop ++;

    *szBuffer = TEXT('\0');

    return( lpch );
}


TUCHAR *
GetToken
(
    TUCHAR *lpchTop ,
    TUCHAR *lpBuffer ,
    int     iBufferSize
)
{
    TUCHAR *lpch;

    lpch = SkipWhite( lpchTop );

    while( iBufferSize )
    {
        switch( *lpch )
        {
//      case TEXT(' ')  :
        case TEXT(':')  :
        case TEXT('=')  :
        case TEXT(',')  :
        case TEXT('\t') :
        case TEXT('\r') :
        case TEXT('\n') :
            *lpBuffer = TEXT('\0');
            lpch++;
            return( lpch );

        case TEXT('"')  :
            ++lpch;
            while( *lpch != TEXT('"') && *lpch != TEXT('\n') &&
                   *lpch != TEXT('\r') && *lpch != TEXT('\0') && --iBufferSize )
            {
                *lpBuffer ++ = *lpch ++;
            }
            lpch ++;
            break;
        default :
            if( *lpch != TEXT(' ') )
            {
                *lpBuffer ++ = *lpch;
                --iBufferSize;
            }
            lpch ++;
        }
    }
}

TUCHAR *
GetInfSectionPointer
(
    TUCHAR *SectionName ,
    TUCHAR *InfData ,
    long    lInfDataSize
)
{
    TUCHAR *SectionFind;
    TUCHAR *SectionTop;

    SectionFind = _tcsstr( InfData , SectionName );

    if( SectionFind == NULL ) return NULL;

    SectionTop = GetNextLineToken( SectionFind );

    return SectionTop;
}

int
GetInfSectionMemberCount
(
    TUCHAR *SectionTopMember
)
{
    int nCount = 0;
    TUCHAR *lpch = SectionTopMember;

    while( lpch != NULL && IsInfMember( lpch ) )
    {
        nCount ++;
        lpch = GetNextLineToken( lpch );
    }

    return( nCount );
}

BOOL
GetInfDescription
(
    TUCHAR *pbNow ,
    TUCHAR *szBuffer ,
    long    limit
)
{
    pbNow = GetInfRightValuePointer( pbNow );

    if( *pbNow != TEXT('"') ) return FALSE;

    GetToken( pbNow , szBuffer , limit );

    return TRUE;
}

void
SetInfAtom
(
    ATOM *pInfAtom ,
    TUCHAR *ValueTop
)
{
    TUCHAR szKey[DESCMAX];

    GetToken( ValueTop , szKey , DESCMAX );

    *pInfAtom = AddAtom( szKey );
}

ATOM
GetInfAtom
(
    ATOM *pInfAtom ,
    TUCHAR *szBuffer ,
    int  iBufferSize
)
{
    return( GetAtomName( *pInfAtom , szBuffer , iBufferSize ) );
}

BOOL
GetInstallFontPath
(
    HWND    hDlg ,
    TUCHAR *DiskNum ,
    TUCHAR *szTempFile ,
    TUCHAR *szFileName
)
{
    TUCHAR szText[256];
    TUCHAR szDesc[DESCMAX];
    ATOM AtomNum;
    int  ii;

    AtomNum = FindAtom( DiskNum );

    ii = SearchInfDisksAtom( pInfDisksInfo , AtomNum );

    if( ii == -1 ) return ( FALSE );

    GetAtomName( pInfDisksInfo[ii].AtomDisksDesc , szDesc , DESCMAX );

/* Propably szTempFile pointed file name, we need only path */

    *( _tcsrchr( szTempFile , TEXT('\\') ) ) = TEXT('\0');

    return( OpenInstallFontDialog( hDlg , szDesc , szFileName , szTempFile ) );
}

#define TT_INSTALL_SUCCESS 0x01
#define TT_INSTALL_FAIL    0x02
#define TT_INSTALL_ALREADY 0x03
#define TT_INSTALL_CANCEL  0x04

INT
BuildAndOrCopyFont
(
    HWND    hDlg ,
    TUCHAR *szDstName ,   // Distination Filename and Path
    TUCHAR *szInfPath ,   // Inf file path
    TUCHAR *pbStart ,     // File Build Info
)
{
    HANDLE    hFile , hFileMaster;
    TUCHAR    szTempFile[ MAX_PATH ];
    int       ii;

/*
    At first, Check we have builded file , or Not.
*/

    hFileMaster = MyOpenFile( szDstName , NULL, OF_READ );

/*  If exist , we return with TRUE */

    if( hFileMaster != INVALID_HANDLE_VALUE )
    {
        MyCloseFile( hFileMaster );
        return( TT_INSTALL_ALREADY );
    }

/*  Create Distination File */

    hFileMaster = MyOpenFile( szDstName , NULL, OF_CREATE );

    if( hFileMaster == INVALID_HANDLE_VALUE )
        return( TT_INSTALL_FAIL );

/*  We Build up distination file path according to information in inf */

/*  Set default install path */

    lstrcpy( szTempFile , szInfPath );

/*  Build File */

    for( ; ; )
    {
        TUCHAR DisksId[DESCMAX];
        TUCHAR DisksFile[MAX_PATH];
        TUCHAR *ExpandedData;
        long   lLzFileSize;

        if( pbStart == NULL || *pbStart == TEXT('\n') || *pbStart == TEXT('\r') )
            break;

    /* Get Disks Id and Source filename */

        pbStart = GetToken( pbStart , DisksId , DESCMAX );

        pbStart = GetToken( pbStart , DisksFile , MAX_PATH );

    /* Build install file path */

        lstrcat( szTempFile , DisksFile );

    /* open it */

        while( ( hFile = MyOpenFile( szTempFile , NULL, OF_READ ) ) == INVALID_HANDLE_VALUE )
        {
            if( GetInstallFontPath( hDlg , DisksId , szTempFile , DisksFile ) == FALSE )
            {
                MyCloseFile( hFileMaster );
                DeleteFile( szDstName );
                return( TT_INSTALL_CANCEL );
            }

            if( szTempFile[lstrlen(szTempFile) - 1] != TEXT('\\') )
            {
                lstrcat( szTempFile , TEXT("\\") );
            }
            lstrcat( szTempFile , DisksFile );
        /* Update background rect in install dlg of insert disk dlg */
            UpdateWindow( GetParent( hDlg ) );
            UpdateWindow( hDlg );
            HourGlass( TRUE );
        }

    /* Get file size */

        lLzFileSize = MyFileSeek( hFile , 0 , 2 );
        MyFileSeek( hFile , 0 , 0 );

    /* Allocate memory to keep file data */

        ExpandedData = LocalAlloc( LMEM_FIXED , ByteCountOf(lLzFileSize) );

        if( ExpandedData == NULL ) return( TT_INSTALL_FAIL );

    /* Read the data and write down it to file */

        MyByteReadFile( hFile , (LPVOID) ExpandedData , lLzFileSize );
        MyByteWriteFile( hFileMaster , (LPVOID) ExpandedData , lLzFileSize );

        LocalFree( ExpandedData );
        MyCloseFile( hFile );

    /*  Cut file name , we need only path nexttime */

        *( _tcsrchr( szTempFile , TEXT('\\') ) + 1 ) = TEXT('\0');
    }

    MyCloseFile( hFileMaster );

    return( TT_INSTALL_SUCCESS );
}

BOOL
bInstallBitmapFont
(
    HWND     hDlg ,
    TUCHAR *szTTFont ,
    TUCHAR *szDstPath ,
    TUCHAR *szInfPath ,
    TUCHAR *pbSectionTop
)
{
    HKEY hKey , hKeyMap;
    INT  nBitmapFont;
    INT  iRet;
    DWORD dwRet;
    TUCHAR *pbInf = pbSectionTop;
    TUCHAR *pbNow;
    TUCHAR szKey[ MAX_PATH ];
    TUCHAR szSrcFont[ MAX_PATH ];
    TUCHAR szDstFont[ MAX_PATH ];
    TUCHAR szToken[ MAX_PATH ];
    TUCHAR char szData[ MAX_PATH ];

    lstrcpy( szSrcFont , szInfPath );
    lstrcpy( szDstFont , szDstPath );

/*
    Compute the member count in this bitmap section
*/

    nBitmapFont = GetInfSectionMemberCount( pbSectionTop );

/*
    At least, Bitmap font section has to have 2 member.
     One font and "Mode=" description
*/

    if( nBitmapFont < 2 )
        return( FALSE );

/* Create Ini file mapping */

    lstrcpy( szKey , BITMAP_FOR_KEY );
    lstrcat( szKey , szTTFont );

    lstrcpy( szData , SYS_PREFIX );
    lstrcat( szData , TT_REG_KEY );
    lstrcat( szData , szTTFont );

    RegOpenKeyEx( HKEY_LOCAL_MACHINE ,
                  WIN_INI_KEY ,
                  0 ,
                  KEY_ALL_ACCESS ,
                  &hKeyMap
                );

    RegSetValueEx( hKeyMap ,
                   szKey ,
                   0 ,
                   REG_SZ ,
                   szData ,
                   ByteCountOf(lstrlen(szData) + 1)
                 );

    RegCloseKey( hKeyMap );

/* Create key */

    lstrcpy( szKey , SOFTWARE_PREFIX );
    lstrcat( szKey , TT_REG_KEY );
    lstrcat( szKey , szTTFont );

    RegCreateKeyEx( HKEY_LOCAL_MACHINE ,
                    szKey ,
                    0 ,
                    NULL ,
                    0 ,
                    KEY_ALL_ACCESS ,
                    NULL ,
                    &hKey ,
                    &dwRet
                  );

/* Install bitmap font */

    while( nBitmapFont-- )
    {
        pbNow = pbInf;

    /* Get distination font file name */

        pbNow = GetInfLeftValue( pbNow , szToken , MAX_PATH );

        pbNow = GetInfRightValuePointer( pbNow );

    /* Check the key is "Mode" ? */

        if( lstrcmpi( szToken , TEXT("Mode") ) == 0 )
        {
            pbNow = GetToken( pbNow , szData , MAX_PATH );

        /* Write Mode information into registry */

            RegSetValueEx( hKey ,
                           szToken ,
                           0 ,
                           REG_SZ ,
                           szData ,
                           ByteCountOf(lstrlen(szData) + 1)
                         );

        /* Move to Next Line for next loop */

            pbInf = GetNextLineToken( pbInf );

            continue;
        }

    /* Build path */

        lstrcat( szDstFont , szToken );

    /* Get font size */

        pbNow = GetToken( pbNow , szData , MAX_PATH );

    /* Now , pbNow point the information of Build font file */

        iRet = BuildAndOrCopyFont
               (
                   hDlg ,
                   szDstFont ,
                   szSrcFont ,
                   pbNow
               );

        switch( iRet )
        {
        case TT_INSTALL_SUCCESS :
        case TT_INSTALL_ALREADY :
        {

        /* Write registry */

            RegSetValueEx( hKey ,
                           szData ,
                           0 ,
                           REG_SZ ,
                           szToken ,
                           ByteCountOf(lstrlen(szToken) + 1)
                         );

            break;
        }
        case TT_INSTALL_FAIL :
        case TT_INSTALL_CANCEL :
            goto ErrorExit;
        }

    /* Init DstPath for Next loop */

        *( _tcsrchr( szDstFont , TEXT('\\') ) + 1 ) = TEXT('\0');

    /* Move to Pointer for Next loop */

        pbInf = GetNextLineToken( pbInf );
    }

    RegCloseKey( hKey );
    return( TRUE );

ErrorExit:

    RegCloseKey( hKey );
    return( FALSE );

}

VOID
DeleteBitmapFont
(
    HWND hDlg ,
    TUCHAR *szPathAndName ,
    BOOL bErase
)
{
    HKEY     hKey;
    LONG     lRet;
    TUCHAR *szNow;
    TUCHAR  szPath[ MAX_PATH ];
    TUCHAR  szFile[ MAX_PATH ];
    TUCHAR  szKey[ MAX_PATH ];
    TUCHAR  szValue[ MAX_PATH ];
    TUCHAR  szData[ MAX_PATH ];
    DWORD dwValue , dwData , dwType;
    DWORD dwIndex;

    lstrcpy( szPath , szPathAndName );

    szNow = _tcsrchr( szPath , TEXT('\\') );
    if( szNow == NULL ) return;

/* keep TT font file name */

    lstrcpy( szFile , szNow + 1 );

/* Cut file name */

    *( szNow + 1 ) = TEXT('\0');

/* We have to remove the data in registry and delete file */

/* *** delete mapping data *** */

/* Create value name */

   lstrcpy( szValue , BITMAP_FOR_KEY );
   lstrcat( szValue , szFile );

   RegOpenKeyEx( HKEY_LOCAL_MACHINE ,
                 WIN_INI_KEY ,
                 0 ,
                 KEY_ALL_ACCESS ,
                 &hKey
               );

/* If this key is not exist, bitmap font was not installed , yet */

   if (hKey != ERROR_SUCCESS)
       return;

   RegDeleteValue( hKey ,
                   szValue
                 );

   RegCloseKey( hKey );

/* *** delete bitmap support data *** */

/* Create key name */

   lstrcpy( szKey , SOFTWARE_PREFIX );
   lstrcat( szKey , TT_REG_KEY );
   lstrcat( szKey , szFile );

   RegOpenKeyEx( HKEY_LOCAL_MACHINE ,
                 szKey ,
                 0 ,
                 KEY_ALL_ACCESS ,
                 &hKey
               );

/* If this key does not exist, bitmap font was not installed yet */

   if (hKey != ERROR_SUCCESS)
       return;

/* Init for loop */

   dwIndex = 0;

/* Enum Registry value */

   dwValue = MAX_PATH;
   dwData = MAX_PATH;

   lRet = RegEnumValue( hKey ,
                        dwIndex ,
                        szValue ,
                        &dwValue ,
                        0 ,
                        &dwType ,
                        szData ,
                        &dwData
                      );

   while( lRet == ERROR_SUCCESS )
   {

   /* Check this value is Mode or not */

       if( bErase && lstrcmpi( szValue , TEXT("Mode") ) != 0 )
       {

       /* if it not "Mode" , Delete bitmap font file */

       /* Build path */

          lstrcat( szPath , szData );

       /* Delete file */

          DeleteFile( szPath );

       /* Cut file name from path */

          *( _tcsrchr( szPath , TEXT('\\') ) + 1 ) = TEXT('\0');
       }

       dwIndex ++;

       dwValue = MAX_PATH;
       dwData = MAX_PATH;

       lRet = RegEnumValue( hKey ,
                            dwIndex ,
                            szValue ,
                            &dwValue ,
                            0 ,
                            &dwType ,
                            szData ,
                            &dwData
                          );
    }

    RegCloseKey( hKey );

/* *** Delete registey key *** */

    RegDeleteKey( HKEY_LOCAL_MACHINE ,
                  szKey
                );

}

#endif // JAPAN



