///////////////////////////////////////////////////////////////////////////////
//
// instfls.cpp
//      Explorer Font Folder extension routines
//    This file holds all the code for installing any kind of file.
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

#include "priv.h"
#include "globals.h"

#include "lstrfns.h"
#include "ui.h"
#include "cpanel.h"
#include "resource.h"

#include "dbutl.h"

#ifdef DEBUG
// Disk Space Check
//#define DISKCHK
#endif

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

/*****************************************************/
/******************** Globals ************************/
/*****************************************************/

extern FullPathName_t  s_szSharedDir;
extern TCHAR           szDirOfSrc[ PATHMAX ];  // For installing

LPTSTR    pszWinDir = s_szSharedDir;
LPTSTR    pszSysDir = s_szSharedDir;

UINT  s_wBrowseDoneMsg;

TCHAR szTestOpen[] = TEXT( "a:a" );
TCHAR szDisks[]    = TEXT( "Disks" );
TCHAR szOEMDisks[] = TEXT( "OEMDisks" );
TCHAR szNull[]     = TEXT( "" );

HWND  ghwndFontDlg;

/* extern */

TCHAR szDrv[ PATHMAX ];

extern TCHAR szSetupInfPath[];
extern TCHAR szSetupDir[];

#ifdef JAPAN
extern HWND ghWndPro;
#endif


/*****************************************************/
/******************** Defines ************************/
/*****************************************************/


#define RECOVERABLEERROR (VIF_SRCOLD | VIF_DIFFLANG | VIF_DIFFCODEPG | VIF_DIFFTYPE)
#define UNRECOVERABLEERROR (VIF_FILEINUSE | VIF_OUTOFSPACE | VIF_CANNOTCREATE | VIF_CANNOTDELETE | VIF_CANNOTRENAME | VIF_OUTOFMEMORY | VIF_CANNOTREADDST)
#define READONLY (1)


/*****************************************************/
/******************** Functions **********************/
/*****************************************************/


/* Fill in the lpName string with the name of the disk specified
 * in the lpDisk string.  This name is retrieved from the [disks]
 * or [oemdisks] section of setup.inf.
 * Returns: TRUE if name was found, FALSE otherwise
 * Assumes: lpName buffer is at least PATHMAX bytes
 */

BOOL NEAR PASCAL GetInstDiskName( LPTSTR lpDisk, LPTSTR lpName )
{
    if( GetPrivateProfileString( szDisks, lpDisk, szNull, lpName,
                                 PATHMAX, szSetupInfPath ) )
      return( TRUE );

    return( GetPrivateProfileString( szOEMDisks, lpDisk, szNull, lpName,
                                     PATHMAX, szSetupInfPath ) != 0 );
}


BOOL FAR PASCAL IsDriveReady( LPTSTR lpszPath )
{
    OFSTRUCT ofstruct;
    BOOL bReady;
//  MSG msg;


    szTestOpen[ 0 ] = lpszPath[ 0 ];
    szTestOpen[ 1 ] = lpszPath[ 1 ];

#ifdef WINNT

    bReady = MyOpenFile( szTestOpen[ 1 ] == TEXT( ':' )
                            ? szTestOpen
                            : szTestOpen + 2,
                            NULL,
                            OF_PARSE ) != (HANDLE) INVALID_HANDLE_VALUE;

    if( bReady )
        bReady = MyOpenFile( lpszPath, NULL, OF_EXIST )
                        != (HANDLE) INVALID_HANDLE_VALUE;

#else

    bReady = OpenFile( !IsDBCSLeadByte( szTestOpen[ 0 ] )
                        && szTestOpen[ 1 ] == TEXT( ':' )
                            ? szTestOpen
                            : szTestOpen + 2,
                            &ofstruct,
                            OF_PARSE ) != -1;

    if( bReady )
        bReady = OpenFile( lpszPath, &ofstruct, OF_EXIST ) != -1;

#endif  //  WINNT

#if 0
    //
    //  Repaint our window if necessary, and let everybody else do the same
    //

    while( PeekMessage( &msg, NULL, WM_PAINT, WM_PAINT, PM_REMOVE ) )
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
#endif

    return( bReady );
}


//
//  Hooks into common dialog to show only directories
//

UINT_PTR CALLBACK AddFileHookProc( HWND hDlg, UINT iMessage,
                               WPARAM wParam, LPARAM lParam )
{
    HWND hTemp;

    switch( iMessage )
    {
        case WM_INITDIALOG:
        {
            TCHAR szTemp[ 200 ];

            GetDlgItemText( ((LPOPENFILENAME)lParam)->hwndOwner, IDRETRY,
                            szTemp, ARRAYSIZE( szTemp ) );
            SetDlgItemText( hDlg, ctlLast+1, szTemp );

            goto PostMyMessage;
        }

        case WM_COMMAND:
            switch( wParam )
            {
                case lst2:
                case cmb2:
                case IDOK:
PostMyMessage:
                  PostMessage( hDlg, WM_COMMAND, ctlLast+2, 0L );
                  break;

                case pshHelp:
                    //
                    //  Enable this if a decision is made to add the help
                    //  information.
                    //
                    // WinHelp( hWnd, TEXT( "WINDOWS.HLP" ), HELP_CONTEXT,
                    //          IDH_WINDOWS_FONTS_BROWSE_31HELP );
                    //

                    return TRUE;
                    break;

                case ctlLast+2:
                    if( SendMessage( hTemp = GetDlgItem( hDlg, lst1 ),
                                     LB_GETCOUNT, 0, 0L ) )
                    {
                        SendMessage( hTemp, LB_SETCURSEL, 0, 0L );

                        SendMessage( hDlg, WM_COMMAND, lst1,
                                     MAKELONG( hTemp, LBN_SELCHANGE ) );
                        break;
                    }

                    SetDlgItemText( hDlg, edt1, szDrv );
                    break;
            }

            break;

        default:
            if( iMessage == s_wBrowseDoneMsg )
            {
                OFSTRUCT of;
                int fh;

                if( ( fh = LZOpenFile( szDrv, &of, OF_READ ) ) == -1 )
                {
                    iUIMsgExclaim(hDlg, IDSI_FMT_FILEFNF, (LPTSTR)szDrv );

                    //
                    //  return TRUE so commdlg does not exit
                    //
                    return( TRUE );
                }

                LZClose( fh );
            }
            break;
    }

    return FALSE;  // commdlg, do your thing
}


short nDisk=TEXT( 'A' );


VOID NEAR PASCAL FormatAddFilePrompt( LPTSTR szStr2 )
{
    TCHAR  szString[ 256 ], szStr3[ 200 ];
    LPTSTR pszStart, pszEnd;

    //
    //  Set the prompt to specify the disk
    //

    if( nDisk && GetInstDiskName( (LPTSTR)&nDisk, szStr3 )
          && (pszStart = StrChr( szStr3, TEXT( '"' ) ) )
          && (pszEnd = StrChr( ++pszStart, TEXT( '"' ) ) ) )
    {
        *pszEnd = TEXT( '\0' );

        LoadString( g_hInst, INSTALLIT, szString, ARRAYSIZE( szString ) );

        // wsprintf( szStr2, szString, (LPTSTR)pszStart, (LPTSTR)szDrv );

        LPTSTR args [ 2 ] = { pszStart, szDrv };

        FormatMessage( FORMAT_MESSAGE_FROM_STRING
                       | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       szString,
                       0,
                       0,
                       szStr2,
                       256,
                       (va_list *) args
                       );
    }
    else
    {
        LoadString( g_hInst, INSTALLIT + 1, szString, ARRAYSIZE( szString ) );
        wsprintf( szStr2, szString, (LPTSTR)szDrv );
    }
}


INT_PTR CALLBACK AddFileDlg( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    RECT rc;

    switch( message )
    {
    case WM_INITDIALOG:
        GetWindowRect( hDlg,&rc );

        SetWindowPos( hDlg,NULL,
                (GetSystemMetrics( SM_CXSCREEN ) - (rc.right - rc.left) ) / 2,
                (GetSystemMetrics( SM_CYSCREEN ) - (rc.bottom - rc.top) ) / 3,
                0, 0, SWP_NOSIZE | SWP_NOACTIVATE );

        SetDlgItemText( hDlg, IDRETRY,  (LPTSTR)lParam );

        SendDlgItemMessage( hDlg, COLOR_SAVE, EM_LIMITTEXT, PATHMAX - 20, 0L );

        SetDlgItemText( hDlg, COLOR_SAVE, szDirOfSrc );

        SendDlgItemMessage( hDlg, COLOR_SAVE, EM_SETSEL, 0, 0x7FFF0000 );

        if (g_bDBCS)
        {
            if( ghwndFontDlg )
            {
                TCHAR szString[ 64 ];

                LoadString( g_hInst, INSTALLIT + 2, szString, ARRAYSIZE( szString ) );

                SetWindowText( hDlg, szString );
            }
        }
        return( TRUE );

    case WM_COMMAND:
        switch( wParam )
        {
        case IDOK:
            GetDlgItemText( hDlg, COLOR_SAVE,  szDirOfSrc, PATHMAX );

            vCPStripBlanks( szDirOfSrc );

            lpCPBackSlashTerm( szDirOfSrc );

        case IDCANCEL:
            EndDialog( hDlg, wParam == IDOK );

            return( TRUE );

        case IDD_BROWSE:
            {
                OPENFILENAME OpenFileName;
                TCHAR szPath[ PATHMAX ];
                TCHAR szFilter[ 20 ];

                // DWORD dwSave;

                int temp;
                LPTSTR lpTemp;

                szFilter[ 0 ] = TEXT( 'a' );
                szFilter[ 1 ] = TEXT( '\0' );

                lstrcpy( szFilter+2, szDrv );

                if( !(lpTemp = StrChr( szFilter+2, TEXT( '.' ) ) ) )
                   lpTemp = szFilter+2+lstrlen( szFilter+2 );

                lstrcpy( lpTemp, TEXT( ".*" ) );

                *szPath = TEXT( '\0' );

                GetDlgItemText( hDlg, COLOR_SAVE, szDirOfSrc, PATHMAX );

                //
                //  Save context. TODO: fix. EMR
                //  dwSave = dwContext;
                //  dwContext = IDH_DLG_BROWSE;
                //

                OpenFileName.lStructSize = sizeof( OPENFILENAME );
                OpenFileName.hwndOwner = hDlg;
                OpenFileName.hInstance = g_hInst;
                OpenFileName.lpstrFilter = szFilter;
                OpenFileName.lpstrCustomFilter = NULL;
                OpenFileName.nMaxCustFilter = 0;
                OpenFileName.nFilterIndex = 1;
                OpenFileName.lpstrFile = (LPTSTR) szPath;
                OpenFileName.nMaxFile = ARRAYSIZE( szPath );
                OpenFileName.lpstrInitialDir = (LPTSTR) szDirOfSrc;
                OpenFileName.lpstrTitle = NULL;
                OpenFileName.Flags = OFN_HIDEREADONLY | OFN_ENABLEHOOK |
                                     OFN_ENABLETEMPLATE |
                                     /* OFN_SHOWHELP | */ OFN_NOCHANGEDIR;
                OpenFileName.lCustData = MAKELONG( hDlg, 0 );
                OpenFileName.lpfnHook = AddFileHookProc;
                OpenFileName.lpTemplateName =(LPTSTR)MAKEINTRESOURCE( DLG_BROWSE );
                OpenFileName.nFileOffset = 0;
                OpenFileName.nFileExtension = 0;
                OpenFileName.lpstrDefExt = NULL;
                OpenFileName.lpstrFileTitle = NULL;

                temp = GetOpenFileName( &OpenFileName );

                //
                //  Restore context.
                //  TODO: FIX  -EMR
                //  dwContext = dwSave;
                //

                //
                //  force buttons to repaint
                //

                UpdateWindow( hDlg );

                if( temp )
                {
                    szPath[ OpenFileName.nFileOffset ] = TEXT( '\0' );

                    SetDlgItemText( hDlg, COLOR_SAVE, szPath );
                }
#ifdef DEBUG
                else
                {
                    wsprintf( szPath, TEXT( "Commdlg error = 0x%04x" ),
                              temp = LOWORD( CommDlgExtendedError( ) ) );

                    if( temp )
                        MessageBox( hDlg, szPath, TEXT( "Control" ),
                                   MB_SETFOREGROUND|MB_OK|MB_ICONINFORMATION );
                }
#endif

                break;
            }

#if 0 // EMR TODO fix
        case IDD_HELP:
              goto DoHelp;
#endif
        }
        break;

    default:

#if 0    // EMR: TODO
        if( message == wHelpMessage )
        {
DoHelp:
            CPHelp( hDlg );
            return TRUE;
        }
        else
#endif
            return FALSE;

    }

    //
    //  Didn't process a message
    //

    return( FALSE );
}


/* This copies a string up to a given char (not including the char)
 * into another string, up to a maximum number of chars
 * Notice that wMax includes the terminating NULL, while StrCpyN
 * does not
 */

LPTSTR FAR PASCAL CpyToChr( LPTSTR lpDest, LPTSTR lpSrc, TCHAR cChr, int iMax )
{
    LPTSTR lpch;
    int    len;

    lpch = StrChr( lpSrc, cChr );

    if( lpch )
        len = (int)(lpch - lpSrc);
    else
        len = lstrlen( lpSrc );

    iMax--;

    if( len > iMax )
        len = iMax;

//     StrCpyN( lpDest, lpSrc, len );

    lstrcpyn( lpDest, lpSrc, len + 1 );

    return lpSrc + lstrlen( lpDest );
}


/* Parse a string like '5:hppcl.drv,' into the disk and the driver
 * *nDsk gets the 5, and pszDriver gets "hppcl.drv"
 * It is assumed that the ONE byte before the ':' identifies
 * the disk, and is in '0'-'9' or 'A'-'Z'
 */

VOID FAR PASCAL GetDiskAndFile( LPTSTR pszInf,
                                short /* int */ FAR *nDsk,
                                LPTSTR pszDriver,
                                WORD wSize )
{
    LPTSTR pszTmp;

    //
    //  Determine the disk on which to find the file; note if a comma comes
    //  before a colon, there is no disk specified
    //

    if( !(pszTmp = StrChr( pszInf+1, TEXT( ':' ) ) )
        || StrRChr( pszInf+1, pszTmp, TEXT( ',' ) ) )
    {
        *nDsk = 0;
    }
    else
    {
        pszInf = pszTmp + 1;
        *nDsk  = *(pszTmp - 1 );
    }

    //
    //  Get the driver name and terminate at the TEXT( ',' )
    //

    CpyToChr( pszDriver, pszInf, TEXT( ',' ), wSize );
}


/* This attempts to set the attributes of a file; dx is set to 0
 * if the call was successful, -1 otherwise.  ax gets the attributes,
 * or the DOS error.
 */

DWORD NEAR PASCAL GetSetFileAttr( LPTSTR lpFileName, DWORD dwAttr )
{
    if( dwAttr != 0xffffffff )
        SetFileAttributes( lpFileName, dwAttr );

    return GetFileAttributes( lpFileName );
}

#ifdef DISKCHK

BOOL NEAR PASCAL DebugGetDiskSpace( )
{
    WORD wAvailCluster;
    WORD wBytePerSector;
    WORD wSectorPerCluster;
    LONG lFreeSize;
    unsigned int iRet;
    TCHAR szDev[ 80 ];
    TCHAR szSysDir[ 128 ];
    int nDrive;

    GetSystemDirectory( (LPTSTR) szSysDir, 127 );
    nDrive = (int)( (unsigned int)szSysDir[ 0 ] - (unsigned int)TEXT( 'A' )
                                                + (unsigned int)1 );

_asm{
    mov dl,nDrive
    mov ah,36h
    int 21h

    cmp ax,0FFFFh
    je DFend

    mov wAvailCluster,bx
    mov wBytePerSector,ax
    mov wSectorPerCluster,cx

DFend:
    mov iRet,ax
}

    if( iRet!=-1 )
    {
        lFreeSize = (LONG)wAvailCluster * (LONG)wBytePerSector * (LONG)wSectorPerCluster;

        wsprintf( (LPTSTR)szDev,TEXT( "DiskFreeSize is %ld\r\n" ),lFreeSize );

        OutputDebugString( (LPTSTR)szDev );
    }
    else
        OutputDebugString( (LPTSTR)TEXT( "Get Free Size Error!\r\n" ) );
}

#else

#define DebugGetDiskSpace( ) NULL

#endif

//
// Returns: Number of files installed.
//          0xFFFFFFFF = Operation aborted by user.
//

DWORD FAR PASCAL InstallFiles( HWND hwnd,
                               LPTSTR FAR *pszFiles,
                               int nCount,
                               INSTALL_PROC lpfnNewFile,
                               WORD wIFFlags )
{
    SHFILEOPSTRUCT fop;
    FullPathName_t szWinDir;
    TCHAR          szTmpFile[MAX_PATH];
    DWORD          dwInstalledCount = 0;
    TCHAR          szFile[MAX_PATH];
    int            i;
    int            nPass;
    BOOL           bFileExists = FALSE;
    int            iSHFileOpResult = 0;
    UINT OldErrorMode;

    //
    //  Initialize. Set the fop struct to copy the files into
    //  the fonts directory.
    //

    if( !GetFontsDirectory( szWinDir, ARRAYSIZE( szWinDir ) ) )
        goto backout;

    //
    // SHFileOperation requires that the source and destination file
    // lists be double-nul terminated.
    //
    szWinDir[lstrlen(szWinDir) + 1] = TEXT('\0');

    memset( &fop, 0, sizeof( fop ) );

    fop.hwnd   = hwnd;
    fop.wFunc  = FO_COPY;
    fop.pTo    = szWinDir;
    fop.fFlags = FOF_NOCONFIRMATION;

    for( i = 0; i < nCount; i++)
    {
        //
        // Which disk and file are we on?
        //

        GetDiskAndFile( pszFiles[ i ], &nDisk, szFile, ARRAYSIZE( szFile ) );

        vCPStripBlanks( szFile );

        if( !nDisk )
        {
            LPTSTR pszEnd;

            if( !GetInstDiskName( (LPTSTR) &nDisk, szDirOfSrc )
                 || !(pszEnd = StrChr( szDirOfSrc, TEXT( ',' ) ) ) )
                goto backout;

            *pszEnd = 0;
            vCPStripBlanks( szDirOfSrc );

#if 0
            //
            //  TEXT( "." ) is special, and means there is no default dir.
            //

            if( *(WORD *)szDirOfSrc == TEXT( '.' ) )
            {
                //
                //  TODO. What do we do with this?
                //
                continue;
            }
#endif
        }

        CharUpper( szFile );

        *szTmpFile = 0;

        //
        //  Need to check DriveReady before attempting to install the file.
        //

        nPass = 0;

        do
        {
            OldErrorMode = SetErrorMode( 1 );

            lstrcpy( szTmpFile, szDirOfSrc );

            lpCPBackSlashTerm( szTmpFile );

            lstrcat( szTmpFile, szFile );

            bFileExists = IsDriveReady( szTmpFile );

            SetErrorMode( OldErrorMode );

            if( !bFileExists )
            {
                BOOL bUserPressedOk = FALSE;

                GetDiskAndFile( pszFiles[ i ], &nDisk,szDrv,80 );

                CharUpper( szDrv );

                //
                //  Query the user for the disk. This has to succeed or
                //  we bail.
                //

                FormatAddFilePrompt( szTmpFile );

                //
                //  EMR TODO fix help id.
                //

                bUserPressedOk = DoDialogBoxParam( DLG_INSTALL,
                                        hwnd,
                                        AddFileDlg,
                                        0              /* IDH_DLG_INSERT_DISK */,
                                        (LPARAM) (LPTSTR) szTmpFile );
                if( !bUserPressedOk )
                {
                    //
                    // User pressed "Cancel"
                    //
                    dwInstalledCount = (DWORD)-1;
                    goto backout;
                }
            }
        } while( !bFileExists  );

        //
        //  Copy the file
        //

        memset( szTmpFile, 0, sizeof( szTmpFile ) );

        lstrcpy( szTmpFile, szDirOfSrc );

        lpCPBackSlashTerm( szTmpFile );

        lstrcat( szTmpFile, szFile );

        //
        // SHFileOperation requires that the source and destination file
        // lists be double-nul terminated.
        //
        szTmpFile[lstrlen(szTmpFile) + 1] = TEXT('\0');
        fop.pFrom = szTmpFile;

        if( ( iSHFileOpResult = SHFileOperation( &fop ) ) || fop.fAnyOperationsAborted )
        {
            //
            // If operation was aborted or cancelled.
            //
            if( fop.fAnyOperationsAborted ||
              ( iSHFileOpResult == 0x75 /* DE_OPCANCELLED */) )
            {
                dwInstalledCount = (DWORD)-1;
            }
            goto backout;
        }
        else
        {
            dwInstalledCount++;  // Success!
        }
    }

backout:

    //
    //  If we cancelled, remove any of the files that we may have installed.
    //

    if( (DWORD)(-1) == dwInstalledCount)
    {
        for( int j = 0; j <= i; j++)
        {
            //
            // Which disk and file are we on?
            //

            GetDiskAndFile( pszFiles[ j ], &nDisk, szFile, ARRAYSIZE( szFile ) );
            vCPStripBlanks( szFile );

            vCPDeleteFromSharedDir( szFile );
       }
    }

    return dwInstalledCount;
}
