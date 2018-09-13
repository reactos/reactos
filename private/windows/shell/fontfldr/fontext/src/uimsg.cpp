///////////////////////////////////////////////////////////////////////////////
//
// uimsg.cpp
//      Explorer Font Folder extension routines.
//    Message box and status box routines.
//    These routines are all vUIPStatusXXX and iUIMsgXXX
//
//
// History:
//      31 May 95 SteveCat
//          Ported to Windows NT and Unicode, cleaned up
//
//
// NOTE/BUGS
//     $keywords: uimsg.cpp 1.3 22-Mar-94 1:26:04 PM$
//
//***************************************************************************
// $lgb$
// 1.0     7-Mar-94 eric Initial revision.
// 1.1     9-Mar-94 eric Added Mutex locks for GDI.
// 1.2    17-Mar-94 eric removed references to mutex.
// 1.3    22-Mar-94 eric Removed MFC toolbar code (it was already ifdef'd
//                       out)
// $lge$
//*************************************************************************** 
//
//  Copyright (C) 1992-1993 ElseWare Corporation.  All rights reserved.
//  Copyright (C) 1992-1995 Microsoft Corporation
//
///////////////////////////////////////////////////////////////////////////////

//==========================================================================
//                              Include files
//==========================================================================

#include "priv.h"
#include "globals.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include "resource.h"
#include "ui.h"
#include "dbutl.h"

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

//
//  Types
//

typedef TCHAR STATTEXT[ 128 ];
typedef TCHAR MSGTEXT [ 256 ];

//
//  Globals - these are shared throughout the system for
//            debugging purposes only.
//

BOOL  g_bTrace = FALSE;
BOOL  g_bDiag  = FALSE;

static BOOL  s_bIsStatClear = FALSE;
static BOOL  s_bStatPushed  = FALSE;

// static STATTEXT s_szStatTxtStack;

#if 1

#define STRING_BUF   256
// static TCHAR  s_szStatTxtStack[ STRING_BUF ];
static TCHAR  s_szMemDiag[ STRING_BUF ];     // Text for memory limit
static TCHAR  s_szMemCaption[ STRING_BUF ];  // Caption for memory limit message

#else

static CString  s_szStatTxtStack;
static CString  s_szMemDiag;            // Text for memory limit
static CString  s_szMemCaption;         // Caption for memory limit message

#endif

// -----------------------------------------------------------------------
// -------- Remove the Status stuff. It doesn' work in the Shell ---------
// -----------------------------------------------------------------------
#if 0 
// const int kStatusPane = 0;             // Which pane to put the text in.

//
//  Module-global routines
//

void FAR PASCAL vUIPStatusPushHelp( int Status, int Help )
{ vUIPStatusPush  (Status ); };


/***************************************************************************
 * FUNCTION: vUIPStatusClear
 *
 * PURPOSE:  If not already clear, re-clear the status line.
 *
 * RETURNS:  Nothing.
 ***************************************************************************/

void FAR PASCAL vUIPStatusClear( )
{
    if( !s_bIsStatClear )
    {
        pUIPStatusGetWindow( )->SendMessage( SB_SETTEXT, 0,
                                             (LONG)(LPTSTR)TEXT( "" ) );

        pUIPStatusGetWindow( )->UpdateWindow( );

        s_bIsStatClear = TRUE;
   }
}


/***************************************************************************
 * FUNCTION: vUIPStatusPush
 *
 * PURPOSE:  Save the current status message string and then display the
 *           new one.  When 'pop' is called we will restore the current
 *           message.
 *              The 'stack' has a depth of one message.
 *
 * RETURNS:  Nothing.
 ***************************************************************************/

void FAR PASCAL vUIPStatusPush( WORD wIDStr )
{
    //
    //  If don't already have something pushed on our tiny stack, get the
    //  current status text window contents and push that.
    //

    if( !s_bStatPushed )
    {
        s_szStatTxtStack = TEXT( "" );

        pUIPStatusGetWindow( )->SendMessage( SB_GETTEXT, 0,
                            (LONG)(LPTSTR)s_szStatTxtStack.GetBuffer( 255 ) );

        s_szStatTxtStack.ReleaseBuffer( );

        s_bStatPushed = TRUE;
    }

    //
    //  Regardless of whether we actually pushed, show the input text.
    //

    vUIPStatusShow( wIDStr );
}


/***************************************************************************
 * FUNCTION: vUIPStatusPop
 *
 * PURPOSE:  Restore any previously 'push'ed status message
 *              The 'stack' has a depth of one message.
 *
 * RETURNS:  Nothing.
 ***************************************************************************/

void FAR PASCAL vUIPStatusPop( )
{
    if( s_bStatPushed )
    {
        pUIPStatusGetWindow( )->SendMessage( SB_SETTEXT, 0,
                            (LPARAM)(LPTSTR)s_szStatTxtStack.GetBuffer( 255 ) );

        s_szStatTxtStack.ReleaseBuffer( );

        pUIPStatusGetWindow( )->UpdateWindow( );

        s_bIsStatClear = FALSE;
        s_bStatPushed    = FALSE;
   }
}


/***************************************************************************
 * FUNCTION: vUIPStatusShow
 *
 * PURPOSE:  Format a string for the status box and show it.
 *
 * RETURNS:  Nothing.
 ***************************************************************************/

void FAR PASCAL vUIPStatusShow( WORD wIDStr, PTSTR wArg1, PTSTR wArg2,
                                                          PTSTR wArg3 )
{
    // CString    cMessage;

    TCHAR    cMessage[ STRING_BUF ];
    STATTEXT szStatLine;

    //
    //  No string ID means clear the box.
    //

    if( !wIDStr )
    {
        vUIPStatusClear( );
    }
    else if( !cMessage.LoadString( wIDStr ) )
    {
        //
        //  Load the message text - if this fails, clear the box
        //

        vUIPStatusClear( );
    }
    else if( _snprintf( szStatLine, sizeof( szStatLine ),
                        cMessage, wArg1, wArg2, wArg3 ) )
    {
        //
        // All okay, Format and display the message.
        //

        pUIPStatusGetWindow( )->SendMessage( SB_SETTEXT, 0,
                                             (LPARAM)(LPTSTR)szStatLine );

        pUIPStatusGetWindow( )->UpdateWindow( );
    
        s_bIsStatClear = FALSE;
   }
}
#endif

// -----------------------------------------------------------------------
// -------- Remove the Status stuff. It doesn' work in the Shell ---------
// -----------------------------------------------------------------------

//
//  Although these are used but once, we load now since they're for
//  out-of-memory diagnostics, and we probably won't be able to load
//  them when needed.
//

VOID FAR PASCAL vUIMsgInit( )
{
    // s_szMemDiag.LoadString ( IDS_MSG_NSFMEM );
    // s_szMemCaption.LoadString ( IDS_MSG_CAPTION );

    LoadString( g_hInst, IDS_MSG_NSFMEM, s_szMemDiag, ARRAYSIZE( s_szMemDiag ) );

    LoadString( g_hInst, IDS_MSG_CAPTION, s_szMemCaption, ARRAYSIZE( s_szMemDiag ) );
}

/***************************************************************************
 * FUNCTION: iUIMsgBox
 *
 * PURPOSE:  Format a string and show a message box per the caller's
 *              MB_ settings.  There are several cover routines that
 *              set this up so it may be called more simply (see the header)
 *
 * RETURNS:  The function returns the result of the message box, or zero on
 *              failure (the message box function also returns zero on failure)
 ***************************************************************************/

int FAR PASCAL iUIErrMemDlg(HWND hwndParent)
{
    MessageBox( hwndParent, s_szMemDiag, s_szMemCaption,
                MB_OK | MB_ICONHAND | MB_SYSTEMMODAL | MB_SETFOREGROUND );

    return -1;
}


int FAR PASCAL iUIMsgBox( HWND hwndParent, WORD wIDStr, WORD wCAPStr, UINT uiMBFlags,
                          LPCTSTR wArg1, LPCTSTR wArg2, LPCTSTR wArg3, LPCTSTR wArg4 )
{
    // CString    cCaption;
    // CString    cMessage;

    TCHAR   cCaption[ STRING_BUF ];
    TCHAR   cMessage[ STRING_BUF ];
    MSGTEXT szMessage;
    int     iResult = 0;

    //
    //  Load the string and the message caption.    Then format the string,
    //  being careful about length, and show the (modal) message box.
    //

    if( wIDStr == IDS_MSG_NSFMEM )
        return iUIErrMemDlg(hwndParent);

    if( LoadString( g_hInst, wIDStr, cMessage, ARRAYSIZE( cMessage ) ) )
    {
        if( !LoadString( g_hInst, wCAPStr, cCaption, ARRAYSIZE( cCaption ) ) )
            cCaption[ 0 ] = 0;

        //
        //  If we have more than one string we have to format it for
        //

        LPCTSTR  args[ 4 ] = { wArg1, wArg2, wArg3, wArg4 };

        iResult = (int)FormatMessage( FORMAT_MESSAGE_FROM_STRING
                                      | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                      cMessage,
                                      0,
                                      0,
                                      szMessage,
                                      ARRAYSIZE( szMessage ),
                                      (va_list *) args
                                      );
        if(  iResult )
        {
            iResult = MessageBox( hwndParent, szMessage, cCaption,
                                  MB_TASKMODAL | MB_SETFOREGROUND | uiMBFlags );

            if( iResult == -1 )
                return iUIErrMemDlg(hwndParent);
        }
    }

    return iResult;
}


int FAR PASCAL iUIMsgBoxWithCaption(HWND hwndParent, WORD wIDStr, WORD wCaption )
{ return iUIMsgBox(hwndParent, wIDStr, wCaption, MB_OKCANCEL | MB_ICONEXCLAMATION ); };


int FAR PASCAL iUIMsgOkCancelExclaim(HWND hwndParent, WORD wIDStr, WORD wIdCap, LPCTSTR wArg )
{ return iUIMsgBox(hwndParent, wIDStr, wIdCap, MB_OKCANCEL | MB_ICONEXCLAMATION, wArg );};


int FAR PASCAL iUIMsgRetryCancelExclaim(HWND hwndParent, WORD wIDStr, LPCTSTR wArg )
{ return iUIMsgBox(hwndParent, wIDStr, IDS_MSG_CAPTION, MB_RETRYCANCEL | MB_ICONEXCLAMATION, wArg );};


int FAR PASCAL iUIMsgYesNoExclaim(HWND hwndParent, WORD wIDStr, WORD wIdCap, LPCTSTR wArg )
{ return iUIMsgBox(hwndParent, wIDStr, wIdCap, MB_YESNO | MB_ICONEXCLAMATION, wArg );};


int FAR PASCAL iUIMsgYesNoExclaim(HWND hwndParent, WORD wIDStr, LPCTSTR wArg )
{ return iUIMsgBox(hwndParent, wIDStr, IDS_MSG_CAPTION, MB_YESNO | MB_ICONEXCLAMATION, wArg );};


int FAR PASCAL iUIMsgExclaim(HWND hwndParent, WORD wIDStr, LPCTSTR wArg )
{ return iUIMsgBox(hwndParent, wIDStr, IDS_MSG_CAPTION, MB_OK | MB_ICONEXCLAMATION, wArg );};


int FAR PASCAL iUIMsgBox(HWND hwndParent, WORD wIDStr, LPCTSTR wArg )
{ return iUIMsgBox(hwndParent, wIDStr, IDS_MSG_CAPTION, MB_OK | MB_ICONHAND, wArg ); };


int FAR PASCAL iUIMsgInfo(HWND hwndParent, WORD wIDStr, LPCTSTR wArg )
{ return iUIMsgBox(hwndParent, wIDStr, IDS_MSG_CAPTION, MB_OK | MB_ICONINFORMATION, wArg ); };




///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: vUIMsgBoxInvalidFont
//
// DESCRIP:  Displays a simple message box for errors encountered through
//           the following font folder functions:
//
//              IsPSFont
//              bCPValidType1Font
//              bIsValidFontFile
//              bIsTrueType
//
//           Font validation occurs in many places in the font folder.
//           This function was added to consolidate validation error reporting
//           and to ensure consistency when reporting these types of errors.
//
// ARGUMENTS:
//           pszFontFile
//              Name of font file being validated.
//
//           pszFontDesc
//              Descriptive name of font being validated.
//
//           dwType1Code
//              Code returned from one of the validation functions.
//              See IsPSFont for details.
//
//           uStyle
//              Message box style.  Defaults to MB_OKCANCEL | MB_ICONEXCLAMATION
//
///////////////////////////////////////////////////////////////////////////////
//
// Map font validation status codes to message box string resources.
//
static const struct str_id_map{
    BYTE code;          // Status portion of status code.
    DWORD idStr;        // Message format string resource id.
}StrIdMap[] = {
    { FVS_INVALID_FONTFILE,   IDS_FMT_FVS_INVFONTFILE },
    { FVS_BAD_VERSION,        IDS_FMT_FVS_BADVERSION  },
    { FVS_FILE_BUILD_ERR,     IDS_FMT_FVS_FILECREATE  },
    { FVS_FILE_EXISTS,        IDS_FMT_FVS_FILEEXISTS  },
    { FVS_FILE_OPEN_ERR,      IDS_FMT_FVS_FILEOPEN    },
    { FVS_FILE_CREATE_ERR,    IDS_FMT_FVS_FILECREATE  },
    { FVS_FILE_IO_ERR,        IDS_FMT_FVS_FILEIO      },
    { FVS_INVALID_ARG,        IDS_FMT_FVS_INTERNAL    },
    { FVS_EXCEPTION,          IDS_FMT_FVS_INTERNAL    },
    { FVS_INSUFFICIENT_BUF,   IDS_FMT_FVS_INTERNAL    },
    { FVS_MEM_ALLOC_ERR,      IDS_FMT_FVS_INTERNAL    },
    { FVS_INVALID_STATUS,     IDS_FMT_FVS_INTERNAL    }};

//
// Map font validation status codes to file name extension strings.
//
static const struct file_ext_map{
   BYTE file;        // File portion of status code.
   LPTSTR pext;      // File name extension string.
}FileExtMap[] = {
   { FVS_FILE_INF, TEXT(".INF") },
   { FVS_FILE_AFM, TEXT(".AFM") },
   { FVS_FILE_PFB, TEXT(".PFB") },
   { FVS_FILE_PFM, TEXT(".PFM") },
   { FVS_FILE_TTF, TEXT(".TTF") },
   { FVS_FILE_FOT, TEXT(".FOT") }};


int iUIMsgBoxInvalidFont(HWND hwndParent, LPCTSTR pszFontFile, LPCTSTR pszFontDesc, 
                         DWORD dwStatus, UINT uStyle)
{
    TCHAR szCannotInstall[STRING_BUF];               // Message prefix.
    TCHAR szFileName[MAX_PATH + 1];                  // Local file name copy.
    TCHAR szNulString[]  = TEXT("");                 // Output when no arg used.
    LPTSTR pszArgs[2]    = { NULL, NULL };           // Message inserts.
    LPTSTR pszFileExt    = NULL;                     // Ptr to ext part of file name.
    DWORD dwMsgId        = IDS_FMT_FVS_INTERNAL;     // Message string resource id.
    UINT cchLoaded       = 0;                        // LoadString status.
    int i                = 0;                        // General loop counter.
    const DWORD dwStatusCode = FVS_STATUS(dwStatus); // Status part of code.
    const DWORD dwStatusFile = FVS_FILE(dwStatus);   // File part of code.

    ASSERT(NULL != pszFontFile);

    //
    // Check to see if status value was properly set.
    // This check relies on status codes being initialized
    // to FVS_INVALID_STATUS.
    //
    ASSERT(dwStatusCode != FVS_INVALID_STATUS);

    //
    // We don't display a message if the status is SUCCESS.
    //
    if (dwStatusCode == FVS_SUCCESS)
    {
       ASSERT(FALSE);  // Complain to developer.
       return 0;
    }

    //
    // Format the common prefix for all messages.
    //
    if ((pszFontDesc != NULL) && (pszFontDesc[0] != TEXT('\0')))
    {
        TCHAR szFmtPrefix[STRING_BUF];

        //
        // Description string is provided and is not blank.
        // Prefix is "Unable to install the "<font desc>" font."
        //
        if ((cchLoaded = LoadString(g_hInst,
                       IDS_FMT_FVS_PREFIX,
                       szFmtPrefix,
                       ARRAYSIZE(szFmtPrefix))) > 0)
        {
            //
            // WARNING: This argument array assumes that there is only ONE
            //          replaceable argument in the string IDS_FMT_FVS_PREFIX.
            //          If this resource is modified to include more embedded
            //          values, this arg array must be extended as well.
            //
            LPCTSTR FmtMsgArgs[] = { pszFontDesc };

            cchLoaded = FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                                      FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                      szFmtPrefix,
                                      0,
                                      0,
                                      szCannotInstall,    
                                      ARRAYSIZE(szCannotInstall),
                                      (va_list *)FmtMsgArgs);
        }
    }
    else
    {
        //
        // Description string is not provided or is blank.
        // Prefix is "Unable to install the font."
        //
        cchLoaded = LoadString(g_hInst,
                       IDSI_CAP_NOINSTALL,
                       szCannotInstall,
                       ARRAYSIZE(szCannotInstall));
    }

    //
    // Verify prefix string is loaded and formatted.
    //
    if (0 == cchLoaded)
    {
        //
        // Resource not found/loaded.
        //
        szCannotInstall[0] = TEXT('\0');  // Make sure we're terminated.
        ASSERT(FALSE);                    // Complain during development.
    }


    lstrcpy(szFileName, pszFontFile);    // Don't want to alter source string.

    int nMapSize = ARRAYSIZE(StrIdMap);
    for (i = 0; i < nMapSize; i++)
    {
        if (StrIdMap[i].code == dwStatusCode)
        {
            dwMsgId = StrIdMap[i].idStr;
            break;
        }
    }

    pszFileExt = NULL;
    if (FVS_FILE_UNK != dwStatusFile)
    {
        nMapSize = ARRAYSIZE(FileExtMap);
        for (i = 0; i < nMapSize; i++)
        {
            if (FileExtMap[i].file == dwStatusFile)
            {
                pszFileExt = FileExtMap[i].pext;
                break;
            }
        }
    }
         
    //
    // Replace the file extension if a file type was specified in status code.
    //
    if (NULL != pszFileExt)
    {
        LPTSTR pchPeriod = StrRChr(szFileName, NULL, TEXT('.'));
        if (NULL != pchPeriod)
           lstrcpy(pchPeriod, pszFileExt);
    }

    //
    // Set up the required arguments for the message format strings.
    //
    pszArgs[0] = szCannotInstall;  // All msgs use this prefix.

    switch(dwMsgId)
    {
        //
        // These don't include an embedded file name.
        //
        case IDS_FMT_FVS_FILEIO:
        case IDS_FMT_FVS_INTERNAL:
           pszArgs[1] = NULL;
           break;

        //
        // By default, each message includes a file name.
        //
        default:
           pszArgs[1] = szFileName;
           break;
    }
              

    //
    // Modify very long path names so that they fit into the message box.
    // They are formatted as "c:\dir1\dir2\dir3\...\dir8\filename.ext"
    // DrawTextEx isn't drawing on anything.  Only it's formatting capabilities are
    // being used. The DT_CALCRECT flag prevents drawing.
    //
    HWND hWnd       = GetDesktopWindow();
    HDC  hDC        = GetDC(hWnd);
    LONG iBaseUnits = GetDialogBaseUnits();
    RECT rc;
    const int MAX_PATH_DISPLAY_WD = 60; // Max characters to display in path name.
    const int MAX_PATH_DISPLAY_HT =  1; // Path name is 1 character high.

    rc.left   = 0;
    rc.top    = 0;
    rc.right  = MAX_PATH_DISPLAY_WD * LOWORD(iBaseUnits);
    rc.bottom = MAX_PATH_DISPLAY_HT * HIWORD(iBaseUnits);

    DrawTextEx(hDC, szFileName, ARRAYSIZE(szFileName), &rc,
                                DT_CALCRECT | DT_PATH_ELLIPSIS | DT_MODIFYSTRING, NULL);
    ReleaseDC(hWnd, hDC);

    //
    // Display message using standard Type 1 installer msg box.
    // Note that iUIMsgBox wants 16-bit resource ID's.
    //
    return iUIMsgBox(hwndParent, (WORD)dwMsgId, IDS_MSG_CAPTION, uStyle,
                     (pszArgs[0] ? pszArgs[0] : szNulString),
                     (pszArgs[1] ? pszArgs[1] : szNulString));
}


