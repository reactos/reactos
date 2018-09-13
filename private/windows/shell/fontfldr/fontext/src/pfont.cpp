///////////////////////////////////////////////////////////////////////////////
//
// pfont.cpp
//     Explorer Font Folder extension routines
//     Install Fonts dialog.
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

#include <direct.h>
#include <shlobjp.h>

#include "commdlg.h"
#include "resource.h"
#include "ui.h"
#include "fontman.h"
#include "cpanel.h"
#include "oeminf.h"

#include "lstrfns.h"

#include "t1.h"
#include "dbutl.h"
#include "fonthelp.h"

#include "fontcl.h"
#include "fontfile.h"

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

#define OF_ERR_FNF 2

#define  ID_BTN_COPYFILES  chx2
#define  ID_BTN_HELP       psh15
#define  ID_BTN_SELALL     psh16
#define  ID_LB_FONTFILES   lst1
#define  ID_LB_FONTDIRS    lst2
#define  ID_CB_FONTDISK    cmb2

#define MAX_FF_PROFILE_LEN    48

#define ffupper(c) ( (TCHAR) CharUpper( MAKEINTRESOURCE( c ) ) )

typedef  UINT (CALLBACK *HOOKER) (HWND, UINT, WPARAM, LPARAM);

#ifdef WINNT

#define IsDBCSLeadByte(x) (FALSE)

//
// Macros used to get current path from file open dialog.
// Used in FontHookProc.
// Borrowed from file open dlg code.
//
#define CHAR_BSLASH TEXT('\\')
#define CHAR_NULL   TEXT('\0')

#define DBL_BSLASH(sz) \
   (*(TCHAR *)(sz)       == CHAR_BSLASH) && \
   (*(TCHAR *)((sz) + 1) == CHAR_BSLASH)

#endif  //  WINNT


typedef struct
{  union {  DWORD ItemData;
      struct { WORD  nFileSlot;
            // BOOL  bTrueType; };   EMR: BOOL in win32 is 32 bits!
            // WORD  bTrueType; };   JSC: For NT, make it FontType for T1 support
               WORD  wFontType; };
   }; // end union
} AddITEMDATA;

//
//  WIN.INI sections
//

static TCHAR szINISTrueType[] = TEXT( "TrueType" );
static TCHAR szINISFonts[]    = TEXT( "fonts" );

//
//  WIN.INI keywords
//

static TCHAR szINIKEnable[] = TEXT( "TTEnable" );
static TCHAR szINIKOnly[]   = TEXT( "TTOnly" );

//
// Globals
//

TCHAR szDot[]  = TEXT( "." );

TCHAR szSetupDir[ PATHMAX ];  // For installing
TCHAR szDirOfSrc[ PATHMAX ];  // For installing
FullPathName_t s_szCurDir;    // Remember last directory for file open dialog.


UINT_PTR CALLBACK FontHookProc( HWND, UINT, WPARAM, LPARAM );


static VOID NEAR PASCAL ResetAtomInDescLB( HWND hLB );

BOOL NEAR PASCAL bAddSelFonts( LPTSTR lpszDir, BOOL bNoCopyJob );
BOOL NEAR PASCAL bFileFound( PTSTR  pszPath,  LPTSTR lpszFile );
BOOL NEAR PASCAL bIsCompressed( LPTSTR szFile );
BOOL NEAR PASCAL bFontInstalledNow( PTSTR  szLHS );
BOOL NEAR PASCAL bInstallFont( HWND hwndParent, LPTSTR lpszSrcPath,
                               BOOL bTrueType, PTSTR szLHS, int* iReply );
BOOL bInstallOEMFile( LPTSTR lpszDir, LPTSTR lpszDstName, LPTSTR lpszDesc,
                      WORD wFontType, WORD wCount );

BOOL NEAR PASCAL bTTEnabled( );

VOID NEAR PASCAL vEnsureInit( );
VOID NEAR PASCAL vPathOnSharedDir( LPTSTR lpszFile, LPTSTR lpszPath );
LPTSTR NEAR PASCAL lpNamePart( LPCTSTR lpszPath );

extern HWND ghwndFontDlg;

//
// We provide custom help text for only the following:
//
// 1. "Copy fonts to..." checkbox.
// 2. "Select All" push button.
// 3. "List of Fonts" listbox.
//
// All other requests for context-sensitive help text are forwarded
// to the "file open" common dialog for standard text.
//
const static DWORD rgHelpIDs[] =
{
    ID_BTN_COPYFILES, IDH_FONTS_ADD_COPY_TO_FONT_DIR,
    ID_BTN_SELALL,    IDH_FONTS_ADD_SELECTALL,
    ctlLast+1,        IDH_FONTS_ADD_FONTLIST,
    IDC_LIST_OF_FONTS,IDH_FONTS_ADD_FONTLIST,
    0,0
};


/*****************************************************************************
   AddFontsDialog - Our add-on to the commdlg font for listing the font names
*****************************************************************************/


class CWnd {
protected:
   CWnd( HWND hWnd = 0 ) : m_hWnd( hWnd ) {};

public:
   HWND  hWnd( ) { return m_hWnd; }

   void UpdateWindow( )
       { ::UpdateWindow( m_hWnd ); }

   BOOL EnableWindow( BOOL bEnable )
       { return ::EnableWindow( m_hWnd, bEnable ); }

   void SetRedraw( BOOL bRedraw = TRUE )
       { ::SendMessage( m_hWnd, WM_SETREDRAW, bRedraw, 0 ); }

   void InvalidateRect( LPCRECT lpRect, BOOL bErase )
       { ::InvalidateRect( m_hWnd, NULL, bErase ); }

   HWND GetDlgItem( int nID ) const
       { return ::GetDlgItem( m_hWnd, nID ); }


protected:
   HWND  m_hWnd;
};

class CListBox : public CWnd
{
public:
   CListBox( UINT id, HWND hParent ) : CWnd( ), m_id( id )
    {   m_hWnd = ::GetDlgItem( hParent, m_id );

        DEBUGMSG( (DM_TRACE1, TEXT( "CListBox: ctor" ) ) );

#ifdef _DEBUG
        if( !m_hWnd )
        {
            DEBUGMSG( (DM_ERROR, TEXT( "CListBox: No hWnd on id %d" ), id ) );
            // DEBUGBREAK;
        }
#endif

    }

   int GetCount( ) const
       { return (int)::SendMessage( m_hWnd, LB_GETCOUNT, 0, 0 ); }

   int GetCurSel( ) const
       { return (int)::SendMessage( m_hWnd, LB_GETCURSEL, 0, 0 ); }

   int GetSelItems( int nMaxItems, LPINT rgIndex ) const
       { return (int)::SendMessage( m_hWnd, LB_GETSELITEMS, nMaxItems, (LPARAM)rgIndex ); }

   int GetSelCount( ) const
       { return (int)::SendMessage( m_hWnd, LB_GETSELCOUNT, 0, 0 ); }

   int SetSel( int nIndex, BOOL bSelect = TRUE )
       { return (int)::SendMessage( m_hWnd, LB_SETSEL, bSelect, nIndex ); }

   int GetText( int nIndex, TCHAR FAR * lpszBuffer ) const
       { return (int)::SendMessage( m_hWnd, LB_GETTEXT, nIndex, (LPARAM)lpszBuffer ); }

   DWORD_PTR GetItemData( int nIndex ) const
       { return ::SendMessage( m_hWnd, LB_GETITEMDATA, nIndex, 0 ); }

   INT_PTR SetItemData( int nIndex, DWORD dwItemData )
       { return ::SendMessage( m_hWnd, LB_SETITEMDATA, nIndex, (LPARAM)dwItemData ); }

   void ResetContent( void )
       { ::SendMessage( m_hWnd, LB_RESETCONTENT, 0, 0 ); }

   int  FindStringExact( int nIndexStart, LPCTSTR lpszFind ) const
       { return (int)::SendMessage( m_hWnd, LB_FINDSTRINGEXACT, nIndexStart, (LPARAM)lpszFind ); }

   int  AddString( LPCTSTR lpszItem )
       { return (int)::SendMessage( m_hWnd, LB_ADDSTRING, 0, (LPARAM)lpszItem ); }

private:
   UINT m_id;
};


class CComboBox : public CWnd
{
public:
   CComboBox( UINT id, HWND hParent ) : CWnd( ), m_id( id )
      { m_hWnd = ::GetDlgItem( hParent, m_id );

        DEBUGMSG( (DM_TRACE1, TEXT( "CComboBox: ctor" ) ) );

        if( !m_hWnd )
        {
            DEBUGMSG( (DM_ERROR, TEXT( "CComboBox: No hWnd on id %d" ), id ) );
            // DEBUGBREAK;
        }
      }

   int GetCurSel( ) const
       { return (int)::SendMessage( m_hWnd, CB_GETCURSEL, 0, 0 ); }

private:
   UINT     m_id;
};


class AddFontsDialog : public CWnd // : public FastModalDialog
{
   public   :  // constructor only
                  AddFontsDialog ( );
                  ~AddFontsDialog( );
            void  vAddSelFonts   ( );
            void  vUpdatePctText ( );

            BOOL  bInitialize(void);

            BOOL  bAdded         ( ) {  return m_bAdded; };

            void  vStartFonts    ( ); //  {  m_nFontsToGo = -1; };

            BOOL  bStartState    ( ) {  return m_nFontsToGo == -1; };

            BOOL  bFontsDone     ( ) {  return m_nFontsToGo == 0;  };

            BOOL  bInitialFonts  ( ) { m_nFonts = pListBoxFiles()->GetCount();
                                       m_nFontsToGo = m_nFonts;
                                       return m_nFonts > 0; };

            void  vHoldComboSel  ( ) { m_nSelCombo = pGetCombo()->GetCurSel(); };

            void  vNewComboSel   ( ) { if( m_nSelCombo == -1 ) vStartFonts( );};

            void  vCloseCombo    ( ) { if( m_nSelCombo != pGetCombo( )->GetCurSel( ) )
                                           vStartFonts( );
                                       m_nSelCombo = -1; };

            CListBox * pListBoxDesc  ( )
                                 { return m_poListBoxDesc;};

            CListBox * pListBoxFiles( )
                                 { return m_poListBoxFiles; };

            //
            //  These were added to make up for no MFC
            //

            void EndDialog( int nRet ) { ::EndDialog( m_hWnd, nRet ); }

            void Attach( HWND hWnd )
                           { m_hWnd = hWnd;
                             m_poComboBox = new CComboBox( ID_CB_FONTDISK, hWnd );
                             m_poListBoxDesc = new CListBox( ID_LB_ADD, hWnd );
                             m_poListBoxFiles = new CListBox( ID_LB_FONTFILES, hWnd );}

            void Detach( ) {m_hWnd = 0;
                           if( m_poComboBox ) delete m_poComboBox;
                           if( m_poListBoxDesc ) delete m_poListBoxDesc;
                           if( m_poListBoxFiles ) delete m_poListBoxFiles; }

            void CheckDlgButton( UINT id, BOOL bCheck )
                                 { ::CheckDlgButton( m_hWnd, id, bCheck ); }

            void EndThread(void)
                { SetEvent(m_heventDestruction); }

            LONG AddRef(void);
            LONG Release(void);


   private :
            CComboBox * pGetCombo( ) { return m_poComboBox;};

   public   :  // fields
            CComboBox *    m_poComboBox;
            CListBox *     m_poListBoxFiles;
            CListBox *     m_poListBoxDesc;
            LPOPENFILENAME m_pOpen;
            BOOL           m_bAdded;
            int            m_nSelCombo;
            int            m_nFonts;
            int            m_nFontsToGo;
            HANDLE         m_hThread;
            DWORD          m_dwThreadId;
            HANDLE         m_heventDestruction; // Used to tell threads we're done.
            LONG           m_cRef;              // Instance reference counter.
};


/*****************************************************************************
   DBCS support.
*****************************************************************************/

#define TRUETYPE_SECTION      TEXT( "TrueType fonts" )
#define WIFEFONT_SECTION      TEXT( "Wife fonts" )
#define TRUETYPE_WITH_OEMINF  (WORD)0x8000
#define MAXFILE   MAX_PATH_LEN

static TCHAR szOEMSetup[] = TEXT( "oemsetup.inf" );

TCHAR szSetupInfPath[ PATHMAX ];

typedef struct tagADDFNT {
        CListBox * poListDesc;
        int     nIndex;
        int     which;
} ADDFNT, far *LPADDFNT;



/*************************************************************************
 * FUNCTION: CutOffWhite
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 *************************************************************************/

VOID NEAR PASCAL CutOffWhite( LPTSTR lpLine )
{
    TCHAR  szLineBuf[ 120 ];
    LPTSTR lpBuf = szLineBuf;
    LPTSTR lpch;


    for( lpch = lpLine; *lpch; lpch = CharNext( lpch ) )
    {
        if( *lpch==TEXT( ' ' ) || *lpch == TEXT( '\t' ) )
             continue;
        else
        {
            if( IsDBCSLeadByte( *lpch ) )
            {
                *lpBuf++ = *lpch;
                *lpBuf++ = *(lpch + 1);
            }
            else
                *lpBuf++ = *lpch;
        }
    }

    *lpBuf = TEXT( '\0' );

    lstrcpy( lpLine,szLineBuf );
}


/*************************************************************************
 * FUNCTION: StrNToAtom
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 *************************************************************************/

ATOM NEAR PASCAL StrNToAtom( LPTSTR lpStr, int n )
{
    TCHAR szAtom[ 80 ];

    //
    //  Take space for NULL
    //

    lstrcpyn( szAtom, lpStr, n+1 );

    //
    //  Null terminate the string
    //

    *(szAtom+n) = TEXT( '\0' );

    CutOffWhite( szAtom );

    return AddAtom( szAtom );
}


/*************************************************************************
 * FUNCTION: ResetAtomInDescLB
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 *************************************************************************/

VOID NEAR PASCAL ResetAtomInDescLB( HWND hLB )
{
    int   nCount;
    DWORD dwData;


    if( nCount = (int) SendMessage( hLB, LB_GETCOUNT, 0, 0L ) )
    {
        while( nCount > 0 )
        {
            nCount--;

            SendMessage( hLB, LB_GETITEMDATA, nCount, (LPARAM) (LPVOID) &dwData );

            //
            //  Atom handle must be C000H through FFFFH
            //

            if( HIWORD( dwData ) >= 0xC000 )
                DeleteAtom( HIWORD( dwData ) );
        }
    }
}


/*************************************************************************
 * FUNCTION: GetNextFontFromInf
 *
 * PURPOSE:
 *          Get font description from Inf scanline, and set it to LB.
 *          Also set ITEMDATA that includes correct index to file listbox,
 *          and 'tag' name string of inf file.
 *
 * RETURNS:
 *
 *************************************************************************/

#define WHICH_FNT_TT    0
#define WHICH_FNT_WIFE  1

WORD FAR PASCAL GetNextFontFromInf( LPTSTR lpszLine, LPADDFNT lpFnt )
{
    TCHAR   szDescName[ 80 ];
    LPTSTR  lpch,lpDscStart;
    WORD    wRet;
    int     nItem;
    ATOM    atmTagName;

    CListBox *  poListDesc = lpFnt->poListDesc;



    //
    //  Presume failure
    //

    wRet = (WORD)-1; /* Presume failure */

    if( lpch = StrChr( lpszLine, TEXT( '=' ) ) )
    {
        //
        //  Get tag string in 'WifeFont' section
        //

        atmTagName = StrNToAtom( lpszLine, (int)(lpch-lpszLine) );

        //
        //  Get description string from right side of '='.
        //  Setup Inf functions in CPSETUP ensure the string
        //  is formatted as key=value with no space between
        //  the key and the value strings.
        //

        lpDscStart = lpch + 1;
        lstrcpyn(szDescName, lpDscStart, ARRAYSIZE(szDescName));

        if( atmTagName && lpDscStart )
        {
           AddITEMDATA OurData;


           OurData.nFileSlot = (WORD)lpFnt->nIndex;
           OurData.wFontType = (lpFnt->which == WHICH_FNT_TT )
                                    ? (atmTagName & ~TRUETYPE_WITH_OEMINF )
                                    : atmTagName;

           if( poListDesc->FindStringExact( -1, szDescName ) == LB_ERR )
           {
                nItem = poListDesc->AddString( szDescName );

                if( nItem != LB_ERR )
                    poListDesc->SetItemData( nItem, OurData.ItemData );
                else
                {
                    DeleteAtom( atmTagName );

                    DEBUGMSG( (DM_ERROR, TEXT( "FONTEXT: Error adding string %s" ),
                              szDescName ) );
                }
            }
            else
            {
                DeleteAtom( atmTagName );

                DEBUGMSG( (DM_TRACE1,TEXT( "String %s already in list" ),
                          szDescName) );
            }

            //
            //  Doesn't matter whether already exists
            //

            wRet = NULL;
        }
        else if( atmTagName )
            DeleteAtom( atmTagName );
    }

    return wRet;
}


/*************************************************************************
 * FUNCTION: FindOemInList
 *
 * PURPOSE:
 *          Scan contents of listbox, checking if that is oemsetup.inf.
 *
 * RETURNS:
 *
 *************************************************************************/

BOOL NEAR PASCAL FindOemInList( CListBox * pListFiles,
                                int nFiles,
                                LPINT pIndex,
                                LPTSTR szInfFileName )
{
    int   i;
    TCHAR szFile[ MAXFILE ];


    DEBUGMSG( (DM_TRACE1,TEXT( "FONTEXT: FindOemInList" ) ) );

    for( i = 0; i < nFiles; i++ )
    {
        if( pListFiles->GetText( i, szFile ) != LB_ERR )
        {
            if( !lstrcmpi( szFile, szOEMSetup ) )
            {
                *pIndex = i;
                lstrcpy( szInfFileName, szFile );

                //
                //  found oemsetup.inf ... return index
                //

                return TRUE;
            }
        }
        else
            //
            //  fail
            //
            return FALSE;
    }

    //
    //  not found
    //

    return FALSE;
}


/*****************************************************************************
   Module-global variables
*****************************************************************************/

static AddFontsDialog*  s_pDlgAddFonts = NULL;
static UINT s_iLBSelChange = 0;



/*************************************************************************
 * FUNCTION: AddFontsDialog
 *
 * PURPOSE: class constructor
 *
 * RETURNS:
 *
 *************************************************************************/

AddFontsDialog::AddFontsDialog( )
   : CWnd( ),
      m_bAdded( FALSE ),
      m_poListBoxFiles( 0 ),
      m_poListBoxDesc( 0 ),
      m_hThread( NULL ),
      m_heventDestruction(NULL),
      m_cRef(0)
{
   /* vSetHelpID( IDH_DLG_FONT2 ); */
    AddRef();
}


/*************************************************************************
 * FUNCTION: ~AddFontsDialog
 *
 * PURPOSE: class destructor
 *
 * RETURNS:
 *
 *************************************************************************/

AddFontsDialog::~AddFontsDialog( )
{
    if (NULL != m_heventDestruction)
        CloseHandle(m_heventDestruction);

    if (NULL != m_hThread)
        CloseHandle(m_hThread);
}

//
// AddRef and Release
//
// These functions have the same meaning as in OLE (sort of).
// When the reference count drops to 0, the object is deleted. The return
// values for each carry the same reliability caveat as their OLE counterparts.
// Note that this only works for dynamically-created objects.  If Release() is
// called for an object not created with the C++ "new" operator, the call to
// "delete" will fault.
// Because two different threads access the dialog object through a
// statically-defined pointer.  The AddRef/Release mechanism works well for
// controlling the lifetime of this object and ensuring the object is available
// for each thread.
//
LONG AddFontsDialog::AddRef(void)
{
    LONG lReturn = m_cRef + 1;

    InterlockedIncrement(&m_cRef);

    DEBUGMSG((DM_TRACE1, TEXT("AddFontsDialog::AddRef %d -> %d"), lReturn-1, lReturn));

    return lReturn;
}


LONG AddFontsDialog::Release(void)
{
    LONG lReturn = m_cRef - 1;

    DEBUGMSG((DM_TRACE1, TEXT("AddFontsDialog::Release %d -> %d"), lReturn+1, lReturn));
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        DEBUGMSG((DM_TRACE1, TEXT("AddFontsDialog: Object deleted.")));
        lReturn = 0;
    }

    return lReturn;
}


/*************************************************************************
 * FUNCTION: AddFontsDialog::bInitialize
 *
 * PURPOSE:  Do any object initializations that may fail.
 *
 * RETURNS:  TRUE  = Object initialized.
 *           FALSE = Initialization failed.
 *
 *************************************************************************/

BOOL AddFontsDialog::bInitialize(void)
{
    //
    // If the destruction event object hasn't been created, create it.
    // This event object is used to tell the dialog's background thread
    // when to exit.
    //
    if (NULL == m_heventDestruction)
    {
        m_heventDestruction = CreateEvent(NULL,  // No security attrib.
                                          TRUE,  // Manual reset.
                                          FALSE, // Initially non-signaled.
                                          NULL); // Un-named.
    }

    return (NULL != m_heventDestruction);
}


/*************************************************************************
 * FUNCTION: dwThreadProc
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 *************************************************************************/

DWORD dwThreadProc( AddFontsDialog * poFD )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: BG thread running" ) ) );

    if (NULL != poFD)
    {
        poFD->AddRef();

        if(NULL == poFD->hWnd())
        {
            DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: BG thread window is null!!!" ) ) );
            DEBUGBREAK;
        }

        if (NULL != poFD->m_heventDestruction)
        {
            while( 1 )
            {
                //
                // Tell Dialog Proc to add more items to the dialog's font listbox.
                //
                PostMessage( poFD->hWnd(), WM_COMMAND, (WPARAM)IDM_IDLE, (LPARAM)0 );

                //
                // Wait max of 2 seconds for event to signal.
                // If signaled, exit loop and end thread proc.
                //
                if (WaitForSingleObject(poFD->m_heventDestruction, 2000) == WAIT_OBJECT_0)
                    break;
            }
        }
        poFD->Release();
    }

    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: BG thread completed." ) ) );

    return 0;
}


/*************************************************************************
 * FUNCTION: vStartFonts
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 *************************************************************************/

void  AddFontsDialog::vStartFonts( )
{

    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT:  ---------- vStartFonts-------" ) ) );
//   DEBUGBREAK;

    //
    //  Set the start state.
    //

    m_nFontsToGo = -1;

    //
    //  Create the background thread
    //

    if( !m_hThread )
    {
        m_hThread = CreateThread( NULL,
                                  0,
                                  (LPTHREAD_START_ROUTINE) dwThreadProc,
                                  (LPVOID) this,
                                  IDLE_PRIORITY_CLASS | CREATE_NO_WINDOW,
                                  &m_dwThreadId );
    }

#ifdef _DEBUG
    if( !m_hThread )
    {
        DEBUGMSG( (DM_ERROR, TEXT( "BG Thread not created" ) ) );
        DEBUGBREAK;
    }
#endif

}


/*************************************************************************
 * FUNCTION: vUpdatePctText
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 *************************************************************************/

void  AddFontsDialog::vUpdatePctText( )
{
    int    pct;
    TCHAR  szFontsRead[ 80 ];
    TCHAR  szTemp[ 80 ] = { TEXT( '\0' ) };


    if( m_nFontsToGo > 0 )
    {
       LoadString( g_hInst, IDSI_FMT_RETRIEVE, szFontsRead, 80 );

       pct = (int) ((long) ( m_nFonts - m_nFontsToGo ) * 100 / m_nFonts );

       wsprintf( szTemp, szFontsRead, pct );
    }

    SetDlgItemText( m_hWnd, ID_SS_PCT, szTemp );
}


BOOL bRemoteDrive( LPCTSTR szDir )
{
    if( szDir[ 0 ] == TEXT( '\\' ) && szDir[ 1 ] == TEXT( '\\' ) )
    {
        //
        //  This is a UNC name
        //

        return( TRUE );
    }

    if( IsDBCSLeadByte( szDir[ 0 ]) || szDir[ 1 ] != TEXT( ':' )
            || szDir[ 2 ] != TEXT( '\\' ) )
    {
        return( FALSE );
    }

#ifndef WIN32
    switch( GetDriveType( (szCurDir[ 0 ]-TEXT( 'A' ) ) & 31 ) )
#else
    TCHAR szRoot[ 4 ];

    lstrcpyn( szRoot, szDir, 4 );

    switch( GetDriveType( szRoot ) )
#endif
    {
    case DRIVE_REMOTE:
    case DRIVE_REMOVABLE:
    case DRIVE_CDROM:
        return( TRUE );

    default:
        break;
    }

    return( FALSE );
}


/*************************************************************************
 * FUNCTION: vAddSelFonts
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 *************************************************************************/

void AddFontsDialog :: vAddSelFonts( )
{
    FullPathName_t szCurDir;

    BOOL  bAddFonts  = TRUE;
    BOOL  bCopyFiles = IsDlgButtonChecked( m_hWnd, ID_BTN_COPYFILES );

#if 0
    CWnd* pFontDlg = CWnd::FromHandle( m_pOpen->hwndOwner );

    //
    // Let the main window live again, and give it focus
    //

    pFontDlg->EnableWindow( TRUE );
    pFontDlg->SetFocus( );
    ShowWindow( SW_HIDE );
    pFontDlg->UpdateWindow( );  // forces repaint of hidden area
#endif


    GetCurrentDirectory( ARRAYSIZE( szCurDir ), szCurDir );

    lpCPBackSlashTerm( szCurDir );

    //
    //  If we're not going to copy the font files, but they live on a remote
    //  disk for a removeable disk, we'd better make sure the user understands
    //  the implications
    //

    if( !bCopyFiles )
    {
        if( bRemoteDrive( szCurDir ) &&
            iUIMsgYesNoExclaim(m_hWnd, IDSI_MSG_COPYCONFIRM ) != IDYES )
        {
            goto done;
        }
    }

    //
    //  Save off the current directory. bAddSelFonts( ) may change it.
    //
    TCHAR  szCWD[ MAXFILE ];

    GetCurrentDirectory( ARRAYSIZE( szCWD ), szCWD );

    if( bAddSelFonts( szCurDir, !bCopyFiles ) )
        m_bAdded = TRUE;

    SetCurrentDirectory( szCWD );

    //
    //  From here, we dispose of the dialog appropriately
    //

done:
    if( m_bAdded )
    {
        ResetAtomInDescLB( s_pDlgAddFonts->pListBoxDesc()->hWnd() );

        FORWARD_WM_COMMAND( m_hWnd, IDABORT, 0, 0, PostMessage );
    }
    else
        ShowWindow( m_hWnd, SW_NORMAL );

    return;
}


extern BOOL bRegHasKey( const TCHAR * szKey, TCHAR * szVal = 0, int iValLen = 0 );


/*************************************************************************
 * FUNCTION: bFontInstalledNow
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 *************************************************************************/

BOOL NEAR PASCAL bFontInstalledNow( PTSTR szLHS )
{
    return  bRegHasKey( szLHS );
}


/***************************************************************************
 * FUNCTION: vEnsureInit
 *
 * PURPOSE:  Make sure our basic module-wide initialization is complete
 *
 * RETURNS:  none
 ***************************************************************************/

VOID NEAR PASCAL vEnsureInit( )
{
    static BOOL s_bInited = FALSE;


    if( s_bInited )
        return;

//
// [stevecat] This is done at vCPPanelInit( ) time now.
//
//    ::GetFontsDirectory( s_szSharedDir, ARRAYSIZE( s_szSharedDir ) );
//
//    lpCPBackSlashTerm( s_szSharedDir );

    s_bInited = TRUE;
}


/***************************************************************************
 * FUNCTION: lpNamePart
 *
 * PURPOSE:  Return pointer to filename only part of path
 *
 * RETURNS:  Pointer to filename
 ***************************************************************************/

LPTSTR NEAR PASCAL lpNamePart( LPCTSTR lpszPath )
{
    LPCTSTR lpCh = StrRChr( lpszPath, NULL, TEXT( '\\' ) );

    if( lpCh )
        lpCh++;
    else
        lpCh = lpszPath;

    return (LPTSTR) lpCh;
}


/***************************************************************************
 * FUNCTION: vPathOnSharedDir
 *
 * PURPOSE:  Make a full path name out of the input file name and on the
 *           shared directory
 *
 * RETURNS:  none
 ***************************************************************************/

VOID NEAR PASCAL vPathOnSharedDir( LPTSTR lpszFileOnly, LPTSTR lpszPath )
{
    lstrcpy( lpszPath, s_szSharedDir );
    lstrcat( lpszPath, lpszFileOnly );
}


/***************************************************************************
 * FUNCTION:   bTTEnabled
 *
 * PURPOSE:    Determine if TrueType is enabled.  GetRasterizerCaps is
 *             supposed to tell us, but it can give eroneous results.  For
 *             instance, if SETUP.INF wasn't there when windows started (or
 *             Language is commented out of WIN.INI), the return status is
 *             bad - why???
 *             One guess - it appears that the Flags field is filled in
 *             properly regardless, but that the Language field is what's
 *             actually returned as the status, and Language defaults to 0.
 *             We'll finesse this by comfirming the size field in the
 *             structure matches what we think it should be, and only believe
 *             the flags in that  case.
 *
 * RETURNS:    BOOL - True if TrueType is enabled
 ***************************************************************************/

BOOL NEAR PASCAL bTTEnabled( )
{
    RASTERIZER_STATUS info;


    GetRasterizerCaps( &info, sizeof( info ) );

    return( info.wFlags & TT_ENABLED ) && ( info.nSize == sizeof( info ) );
}


/***************************************************************************
 * FUNCTION:   vHashToNulls
 *
 * PURPOSE:    Substitute nulls for all the hash (TEXT( '#' )) characters in the input
 *             string - we use this to help with strings that need embedded
 *             nulls but also need to be stored in resource files.
 *
 * RETURNS:    none
 ***************************************************************************/

VOID NEAR PASCAL vHashToNulls( LPTSTR lp )
{
    while( *lp )
        if( *lp == TEXT( '#' ) )
            *lp++ = NULL;
        else
            lp = CharNext( lp );
}


#ifdef WINNT

/////////////////////////////////////////////////////////////////////////////
//
// bIsCompressed
//
//  Leave this function as only ANSI because it just checks the header to
//  determine if it is a compress file or not.
//
/////////////////////////////////////////////////////////////////////////////

BOOL bIsCompressed( LPTSTR szFile )
{
    static CHAR szCmpHdr[] = "SZDD\x88\xf0'3";

    BOOL     bRet = FALSE;
    HANDLE   fh;
    CHAR     buf[ ARRAYSIZE( szCmpHdr ) ];


    if( ( fh = wCPOpenFileWithShare( szFile, NULL, OF_READ ) )
              == (HANDLE) INVALID_HANDLE_VALUE )
        return(FALSE);

    buf[ ARRAYSIZE( buf ) - 1 ] = '\0';

    if( MyByteReadFile( fh, buf, ARRAYSIZE( buf ) - 1 )
           && !lstrcmpA( buf, szCmpHdr ) )
        bRet = TRUE;

    MyCloseFile( fh );

    return( bRet );
}

#else

/***************************************************************************
 * FUNCTION:   bIsCompressed
 *
 * PURPOSE:    Determine if the input file is compressed by peaking into
 *             the header and looking for our magic string.
 *
 * RETURNS:    BOOL - True if compressed
 ***************************************************************************/

BOOL NEAR PASCAL bIsCompressed( LPTSTR szFile )
{
    static   char szMagic[] = "SZDD\x88\xf0'3";
    const    int iLast      = sizeof( szMagic ) - 1;
    char     buf[ sizeof( szMagic ) ];
    int      fh;
    OFSTRUCT of;
    BOOL     bRet = FALSE;

    if( ( fh = wCPOpenFileWithShare( szFile, &of, OF_READ ) ) != HFILE_ERROR )
    {
        buf[ iLast ] = '\0';

        if( _lread( fh, buf, iLast ) == iLast )
            if( lstrcmp( buf, szMagic ) == 0 )
                bRet = TRUE;

        _lclose( fh );
    }
    return( bRet );
}

#endif  //  WINNT


/***************************************************************************
 * FUNCTION:   vConvertExtension
 *
 * PURPOSE:    Take the input file name and replace its extension with that
 *             input
 *
 * RETURNS:    none
 ***************************************************************************/

VOID NEAR PASCAL vConvertExtension( LPTSTR lpszFile, LPTSTR lpszExt )
{
    LPTSTR lpCh;

    //
    //  Remove any extension
    //

    if( lpCh = StrChr( lpNamePart( lpszFile ), TEXT( '.' ) ) )
        *lpCh = TEXT( '\0' );

    //
    //  Old name updated now.
    //

    lstrcat( lpszFile, lpszExt );
}


#ifdef WINNT

/***************************************************************************
 * FUNCTION:   bFileFound
 *
 * PURPOSE:    Check for existance of the given file - we really want it
 *             not to exist.
 *
 * RETURNS:    BOOL - True if file exists
 ***************************************************************************/

BOOL PASCAL bFileFound( LPTSTR pszFullPath, LPTSTR lpszFileOnly )
{
    if( wCPOpenFileWithShare( pszFullPath, NULL, OF_EXIST )
            != (HANDLE) INVALID_HANDLE_VALUE )
        return TRUE;
    else
        return GetModuleHandle( lpszFileOnly ) != NULL;
}

#else

/***************************************************************************
 * FUNCTION:   bFileFound
 *
 * PURPOSE:    Check for existance of the given file - we really want it
 *             not to exist.
 *
 * RETURNS:    BOOL - True if file exists
 ***************************************************************************/

BOOL NEAR PASCAL bFileFound( PTSTR pszFullPath, LPTSTR lpszFileOnly )
{
    OFSTRUCT of;


    if( wCPOpenFileWithShare( pszFullPath, &of, OF_EXIST ) != (WORD)HFILE_ERROR )
        return TRUE;

#ifndef WIN32

    // EMR: The OFSTRUCT isn't being updated with the error code on OF_EXIST test.

    else if( of.nErrCode != OF_ERR_FNF )
        return TRUE;
#endif
    else
        return GetModuleHandle( lpszFileOnly ) != NULL;
}

#endif  //  WINNT


/////////////////////////////////////////////////////////////////////////////
//
// UniqueFilename
//
//   Guarantee a unique filename in a directory.  Do not overwrite existing
//   files.
//
/////////////////////////////////////////////////////////////////////////////

BOOL bUniqueFilename( LPTSTR lpszDst, LPTSTR lpszSrc, LPTSTR lpszDir )
{
    TCHAR   szFullPath[PATHMAX];
    LPTSTR  lpszFile, lpszSrcExt, lpszDstExt;
    WORD    digit = 0;


    lstrcpy( szFullPath, lpszDir );

    lstrcpy( lpszFile = lpCPBackSlashTerm( szFullPath ), lpszSrc );

    if( !(lpszSrcExt = _tcschr( lpszSrc, TEXT( '.' ) ) ) )
        lpszSrcExt = szDot;


    if( wCPOpenFileWithShare( szFullPath, NULL, OF_EXIST ) == INVALID_HANDLE_VALUE )
        goto AllDone;

    if( !(lpszDstExt = _tcschr( lpszFile, TEXT( '.' ) ) ) )
        lpszDstExt = lpszFile + lstrlen( lpszFile );

    while( lpszDstExt - lpszFile < 7 )
        *lpszDstExt++ = TEXT( '_' );

    do
    {
        TCHAR szTemp[ 8 ];

        wsprintf( szTemp, TEXT( "%X" ), digit++ );

        if( digit++ > 0x4000 )
            return( FALSE );

        lstrcpy( lpszFile + 8 - lstrlen( szTemp ), szTemp );
        lstrcat( lpszFile, lpszSrcExt );
    }
    while( wCPOpenFileWithShare( szFullPath, NULL, OF_EXIST ) != INVALID_HANDLE_VALUE );

AllDone:
    lstrcpy( lpszDst, lpszFile );

    return( TRUE );
}


/***************************************************************************
 * FUNCTION: bUniqueOnSharedDir
 *
 * PURPOSE:  Given the source filename, lpszSRc and the directory on which
 *           it resides, lpszDir, make a unique filename by sticking letters
 *           on the end of the name until we get a good one.
 *
 * RETURNS:  BOOL - success of attempt
 ***************************************************************************/

BOOL NEAR PASCAL bUniqueOnSharedDir( LPTSTR lpszUniq, LPTSTR lpszSrc )
{
    TCHAR           szOrigExt[ 5 ];     // Hold input file extension
    FullPathName_t szFullPath;          // Working space for unique name
    LPTSTR          lpszFileOnly;       // Points withing szFullPath
    LPTSTR          lpCh;


    //
    //  Make the full file name out of the input directory and file names.
    //  Hold pointer to where file portion begins
    //

    vPathOnSharedDir( lpszSrc, szFullPath );

    lpszFileOnly = lpNamePart( szFullPath );

    //
    //  Check the full file for existance  - if we couldn't find it, good -
    //  that's what we were shooting for.  Otherwise, make a unique name
    //

    if( bFileFound( szFullPath, lpszFileOnly ) )
    {
        //
        //  Original file not unique
        //

        //
        //  Now we're going to work on making the fake file name.  To make
        //  it easier we're going to force the name length to be at least
        //  7 characters.  We're going to mess with the name in our local
        //  scratch space, to that's where we set the pointers.  We also
        //  hold our extension so that we can shove it on as we iterate thru
        //  the name guesses.
        //

        if( lpCh = StrChr( lpszFileOnly, TEXT( '.' ) ) )
        {
            lstrcpy( szOrigExt, lpCh );

            //
            //  Chop name at extension point.
            //

            *lpCh = 0;
        }
        else
            lstrcpy( szOrigExt, TEXT( "." ) );

        while( lstrlen( lpszFileOnly ) < 7 )
            lstrcat( lpszFileOnly, TEXT( "_" ) );

        //
        //  Now we're going to try to make the names.  We'll loop through
        //  hex digits, building file names with the digit stuck in the last
        //  spot, followed by our extension.
        //

        WORD digit = 0;
        TCHAR szTemp[ 8 ];


        do
        {
            wsprintf( szTemp, TEXT( "%X" ), digit++ );

            if( digit++ > 0x4000 )
                //
                //  Give up at some point
                //

                return FALSE;

            lstrcpy( lpszFileOnly + 8 - lstrlen( szTemp ), szTemp );

            lstrcat( lpszFileOnly, szOrigExt );

        } while( bFileFound( szFullPath, lpszFileOnly ) );

    }  // Original file not unique

    //
    //  We now have a unique name, copy it to the output space
    //

    lstrcpy( lpszUniq, lpszFileOnly );

    return TRUE;
}


/***************************************************************************
 * FUNCTION: IGetExpandedName
 *
 * PURPOSE:  get the expanded name, but fill in the common extensions if
 *           it's not imbedded in the compressed file.
 *
 *  Some compressed dudes don't have the missing last character imbedded
 *  in the file.  if this is the case, we check to see if it's in the known
 *  extension list above.  If it is, we change the name ourselves
 *
 * RETURNS:  INT - same as LZ functions
 ***************************************************************************/

TCHAR *c_aKnownExtensions[] = {
    TEXT( "ttf" ),
    TEXT( "fon" ),
};


DWORD IGetExpandedName( LPTSTR lpszSrc, LPTSTR lpszDest, UINT cchDest )
{
    LPTSTR lpszDestExt;

    CFontFile file;
    DWORD dwReturn = file.GetExpandedName(lpszSrc, lpszDest, cchDest);

    lpszDestExt = PathFindExtension( lpszDest );

    if( lpszDestExt && *lpszDestExt )
    {
        lpszDestExt++;

        //
        //  is it missing the last character?
        //  assumes that if the uncompressed extension was 2 characters,
        //  it's missing one.
        //

        if( lstrlen( lpszDestExt ) == 2 )
        {
            int i;

            for( i = 0; i < ARRAYSIZE( c_aKnownExtensions ); i++ )
            {
                if( !StrCmpNI( lpszDestExt, c_aKnownExtensions[ i ], 2 ) )
                {
                    //
                    //  matches!  Take it the corresponding full extension
                    //

                    lstrcpy( lpszDestExt, c_aKnownExtensions[ i ]);

                    break;
                }
            }
        }

        //
        //  this preserves the long file name because
        //  getexpandedname always returns the short name
        //

        if( lstrlen( lpszDestExt ) <= 3 )
        {
            TCHAR szExt[ 4 ];

            //
            //  save away the extension
            //

            lstrcpy( szExt, lpszDestExt );

            //
            //  restore the long name
            //

            lstrcpy( lpszDest, lpszSrc );

            lpszDest = PathFindExtension( lpszDest );

            //
            //  blast back the new extension
            //

            if( lpszDest && *lpszDest )
            {
                lpszDest++;
                lstrcpy( lpszDest, szExt );
            }
        }
    }

    return dwReturn;

}


/***************************************************************************
 * FUNCTION:   bAddSelFonts
 *
 * PURPOSE:    Install all the fonts that are currently selected in the
 *             add dialog
 *
 * RETURNS:    BOOL - True if any fonts have been installed - not necessarily
 *             all that were requested
 ***************************************************************************/

BOOL NEAR PASCAL bAddSelFonts( LPTSTR lpszInDir,
                               BOOL   bNoCopyJob )
{
    FontDesc_t     szLHS;
    FullPathName_t szTruePath;
    FullPathName_t szSelPath;
    FullPathName_t szFontPath;
    FullPathName_t szInDirCopy;
    FILENAME       szDstFile;
    FILENAME       szSelFile;     // Filename from listbox (but Uppercase )
    BOOL           bTrueType;
    BOOL           bNoCopyFile;
    int            nSelSlot;
    AddITEMDATA    OurData;
    int            iReply = 0;
    BOOL           bOnSharedDir    = FALSE;
    BOOL           bFontsInstalled = FALSE;
    CListBox *     pListFiles      = s_pDlgAddFonts->pListBoxFiles( );
    CListBox *     pListDesc       = s_pDlgAddFonts->pListBoxDesc( );
    WaitCursor     cWaiter;          // Starts and stops busy cursor
    WORD           wCount = 0;
    int            iTotalFonts, i = 0;
    //
    // Create "saved" versions of bNoCopyFile and bOnSharedDir
    // so that the original values are used for call to InstallT1Font( )
    // in the "for each file" loop.
    // Code following the call to InstallT1Font modifies bNoCopyFile and
    // bOnSharedDir so that they are incorrect on subsequent calls to
    // InstallT1Font.
    //
    BOOL bNoCopyFileSaved  = FALSE;
    BOOL bOnSharedDirSaved = FALSE;

    BOOL bOwnInstallationMutex = FALSE;
    HWND hwndProgress = NULL;
    CFontManager *poFontManager = NULL;

    //
    //  Determine if the files are already in the shared directory
    //  (which is where they're headed).
    //

    bOnSharedDirSaved = bOnSharedDir = (lstrcmpi( lpszInDir, s_szSharedDir ) == 0);

    bNoCopyFileSaved = bNoCopyFile = (bNoCopyJob || bOnSharedDir);


    iTotalFonts = pListDesc->GetSelCount( );

    if (!iTotalFonts)
        iTotalFonts = 1;

    //
    //  Init Type1 font installation and Progress dialog
    //

    InitPSInstall( );
    hwndProgress = InitProgress( pListDesc->hWnd() );

    //
    //  We're going to loop until we can't get any more fonts from the
    //  the selection list of the description list box
    //

    while(pListDesc->GetSelItems( 1, &nSelSlot ) )
    {
        if (InstallCancelled())
            goto OperationCancelled;

        if (SUCCEEDED(GetFontManager(&poFontManager)))
        {
            //
            // Must own installation mutex to install font.
            //
            INT iUserResponse  = IDRETRY;
            DWORD dwWaitResult = CFontManager::MUTEXWAIT_SUCCESS;

            while( IDRETRY == iUserResponse &&
                   (dwWaitResult = poFontManager->dwWaitForInstallationMutex()) != CFontManager::MUTEXWAIT_SUCCESS )
            {
                if ( CFontManager::MUTEXWAIT_WMQUIT != dwWaitResult )
                    iUserResponse = iUIMsgRetryCancelExclaim(hwndProgress, IDS_INSTALL_MUTEX_WAIT_FAILED, NULL);
                else
                {
                    //
                    // Cancel if thread received WM_QUIT while waiting for mutex.
                    //
                    iUserResponse = IDCANCEL;
                }
            }
            ReleaseFontManager(&poFontManager);

            //
            // If user chose to cancel or we got a WM_QUIT msg, cancel the installation.
            //
            if ( IDCANCEL == iUserResponse )
                goto OperationCancelled;

            bOwnInstallationMutex = TRUE;
        }

        //
        //  While selected desc
        //

        //
        //  Assume we're continuing.
        //

        iReply = 0;

        //
        //  Pull out a selected font, marking as unselected (so we don't grab
        //  again), and get the font name string.
        //

        pListDesc->SetSel( nSelSlot, FALSE );

        pListDesc->GetText( nSelSlot, szLHS );

        vUIPStatusShow( IDS_FMT_FONTINS, szLHS );

        //
        //  If the current selected font is already installed, don't reinstall
        //  until the user de-installs it.  Inform the user, and drop to
        //  decision handler.
        //

        if( bFontInstalledNow( szLHS ) )
        {
            UINT uMB = (pListDesc->GetSelCount( ) )
                                ? (MB_OKCANCEL | MB_ICONEXCLAMATION )
                                : MB_OK | MB_ICONEXCLAMATION;

            iReply = iUIMsgBox( hwndProgress, IDSI_FMT_ISINSTALLED, IDS_MSG_CAPTION,
                                uMB, szLHS );
            goto ReplyPoint;
        }

        //
        //  Now we can get the corresponding font file name from the files
        //  list box (since we can get its slot).  Force to uppercase for
        //  safety.
        //

        OurData.ItemData = (DWORD)pListDesc->GetItemData( nSelSlot );

        pListFiles->GetText( OurData.nFileSlot, szSelFile );

        bTrueType = (OurData.wFontType == TRUETYPE_FONT);

        //
        //  Update the overall progress dialog
        //

        UpdateProgress (iTotalFonts, i + 1, i * 100 / iTotalFonts);

        i++;

        //
        //  Build the complete selected file path name by appending to
        //  the input directory string
        //

        lstrcpy( szSelPath, lpszInDir );

        lstrcat( szSelPath, szSelFile );

        //
        //  Save a copy of the input directory to be used from here on.
        //

        lstrcpy( szInDirCopy, lpszInDir );

        BOOL    bUpdateWinIni;
        int     ifType;


        if( (OurData.wFontType == TYPE1_FONT)
           || (OurData.wFontType == TYPE1_FONT_NC) )
        {

            bNoCopyFile  = bNoCopyFileSaved;
            bOnSharedDir = bOnSharedDirSaved;

            //
            //  szSelPath has the full source file name
            //
            //  For installations involving the conversion of the Type1
            //  font to TrueType:
            //
            //         "szSelPath" has the destination name of the
            //                     installed TrueType font file.
            //         "szLHS"     is munged to contain "(TrueType)".
            //

            switch( ::InstallT1Font( hwndProgress,
                                     !bNoCopyFile,      //  Copy TT file?
                                     !bNoCopyFile,      //  Copy PFM/PFB files?
                                     bOnSharedDir,      //  Files in Shared Dir?
                                     szSelPath,         //  IN:  PFM File & Dir
                                                        //  OUT: TTF File & Dir
                                     szLHS ) )          //  IN & OUT: Font desc
            {
            case TYPE1_INSTALL_TT_AND_MPS:
                //
                //  The PS font was converted to TrueType and a matching
                //  PostScript font is ALREADY installed.
                //
                // bDeletePSEntry = TRUE;
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

                goto FinishTTInstall;


            case TYPE1_INSTALL_PS_AND_MTT:
                //
                //  The PostScript font was installed and we found a matching
                //  TrueType font that was already installed.
                //
                //  fall thru....

            case TYPE1_INSTALL_PS_ONLY:
                //
                //  Only the PostScript font was installed.
                //

                bUpdateWinIni = FALSE;
                bFontsInstalled = TRUE;

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
            default:
                //
                //  CANCEL and NOMEM (user already warned)
                //
                goto OperationCancelled;
            }

            //
            //  On leaving this conditional many variables must be set up
            //  correctly to proceed with installation of a TrueType font.
            //
            //  szLHS         - fontname description for listbox display
            //  ifType        - itemdata to attach to TT lbox entry
            //  szSelPath     - filename of source font
            //  bTrueType     - TRUE if Type1 file converted to TT
            //  bUpdateWinIni - FALSE if Type1 file not converted to TT
            //                  and used separatly to determine if [fonts]
            //                  section of win.ini (registry) should be
            //                  updated.
            //

FinishTTInstall:

            //
            //  Determine if TTF file to install is in 'fonts' dir
            //

            lstrcpy( szFontPath, szSelPath );


            LPTSTR lpCh = StrRChr( szFontPath, NULL, TEXT( '\\' ) );

            if( lpCh )
            {
                lpCh++;
                *lpCh = TEXT( '\0' );
            }

            bOnSharedDir = lstrcmpi( szFontPath, s_szSharedDir ) == 0;
        }

        //
        //  Start install progress for this font
        //

        ResetProgress( );
        Progress2( 0, szLHS );

        //
        //  Reading OEMSETUP.INF for WIFE/DBCS TT.
        //  if the description is from .inf file, get necessary information
        //  from oemsetup.inf file, based on the information, merge splited
        //  files into single file if exist, and for WIFE font, install font
        //  driver if necessary.
        //

        if( OurData.wFontType > (0xC000 & ~TRUETYPE_WITH_OEMINF ) )
        {
            //
            //  Got a font with oemsetup.inf.
            //

            DEBUGMSG( (DM_TRACE1, TEXT( "Calling bInstallOEMFile %s" ),
                       szSelPath ) );

            // DEBUGBREAK;

            if( !bInstallOEMFile( szInDirCopy, szSelPath, szLHS,
                                  OurData.wFontType, wCount++ ) )
                goto NextSelection;

            SetCurrentDirectory( lpszInDir );

            DEBUGMSG( (DM_TRACE1, TEXT( "--- After bInstallOEMFile() --- " ) ) );
            DEBUGMSG( (DM_TRACE1, TEXT( "lpszInDir: %s" ) , szInDirCopy) );
            DEBUGMSG( (DM_TRACE1, TEXT( "szSelPath: %s" ) , szSelPath) );
            // DEBUGBREAK;

            bOnSharedDir = TRUE;

            //
            //  Use the newly created file as the one to be installed.
            //

            lstrcpy( szSelFile, lpNamePart( szSelPath ) );

            lstrcpy( szInDirCopy, szSelPath );

            *(StrRChr( szInDirCopy, NULL, TEXT( '\\' ) ) + 1 ) = 0;

        }

        //
        //  Check if its a valid font file, telling the user the bad news if not
        //

        DWORD dwStatus;
        if( !::bCPValidFontFile( szSelPath, NULL, NULL, FALSE, &dwStatus ) )
        {
            //
            // Display message box informing user about invalid font and why
            // it is invalid.  If user selects Cancel, font installation
            // is aborted.
            //
            iReply = iUIMsgBoxInvalidFont(hwndProgress, szSelPath, szLHS, dwStatus);
            goto ReplyPoint;
        }

        //
        //  A tricky case here - if the file is compressed, it cannot be used
        //  without decompressing, which makes it hard if we're not copying the
        //  file.  Give the user the option to copy this single file, even if
        //  the job is a non-copy job.  An exception: if we've determined that
        //  the job is non-copy (because the source and destination are the
        //  same), but the user's marked it as copy, we'll do an in-place copy
        //  without telling the user what we did.
        //

        bNoCopyFile = bNoCopyJob || bOnSharedDir;

        if( bNoCopyFile && bIsCompressed( szSelPath ) )
        {
            if( bNoCopyJob )
            {
                iReply = iUIMsgYesNoExclaim(hwndProgress, IDSI_FMT_COMPRFILE, szLHS );

                if( iReply != IDYES )
                    goto ReplyPoint;
            }
            bNoCopyFile = FALSE;
        }

#ifdef WINNT

        if( bNoCopyFile && (OurData.wFontType == NOT_TT_OR_T1)
            && !bOnSharedDir )
            bNoCopyFile = FALSE;

#else

        if( bNoCopyFile && !bTrueType && !bOnSharedDir
             && ( GetModuleHandle( szSelFile ) != NULL ) )
            bNoCopyFile = FALSE;

#endif  //  WINNT

        //
        //  If we're not copying the file, just make sure the font
        //  file path is copied to szFontPath so the font can be installed
        //  in the call to bInstallFont( ).
        //

        if( bNoCopyFile )
        {
            lstrcpy(szFontPath, szSelPath);
        }
        else
        {
            //
            //  The file name might be from a compressed file, so we use LZ to
            //  get the true complete path. From this, we re-extract the name
            //  portion, which we'll use from here on as the file name.
            //
            //  If GetExpandedName() fails, try to use the original path name.
            //

            if( ERROR_SUCCESS != IGetExpandedName( szSelPath, szTruePath, ARRAYSIZE(szSelPath)))
                lstrcpy( szTruePath, szSelPath );

            lstrcpy( szDstFile, lpNamePart( szTruePath ) );

            //
            //  Use this true file name to make a unique path name on the
            //  shared directory
            //

            if( !(bUniqueOnSharedDir( szDstFile, szDstFile ) ) )
            {
                iReply = iUIMsgOkCancelExclaim(hwndProgress, IDSI_FMT_BADINSTALL,
                                                IDSI_CAP_NOCREATE, szLHS );
                goto ReplyPoint;
            }

            //
            //  Finally, we're ready to install the file.  Note that we start
            //  with the original file name and directory. Our destination is
            //  the one we've constructed, on the shared directory.
            //

            if( bCPInstallFile( hwndProgress, szInDirCopy, szSelFile, szDstFile ) )
                vPathOnSharedDir( szDstFile, szFontPath );
            else
                goto ReplyPoint;

            Progress2( 50, szLHS );
        }

        //
        //  Install the font (as opposed to the file), if successful we note
        //  that at least one font's been installed.  If there is a problem,
        //  we need to clean up whatever we did before this attempt - most
        //  notably installing above.
        //
        if( bInstallFont(hwndProgress, szFontPath, bTrueType, szLHS, &iReply ) )
            bFontsInstalled = TRUE;
        else if( !bNoCopyFile )
            vCPDeleteFromSharedDir( szDstFile );

        //
        //  If we copied a file that was in the fonts directory, then delete
        //  the source. This will happen in the case of multi-floppy installs.
        //

        if( !bNoCopyFile && bOnSharedDir )
            vCPDeleteFromSharedDir( szSelPath );

        Progress2( 100, szLHS );

        //
        //  Here's where we jump on any diagnostics.  If the user wanted us
        //  to cancel, we return immediately.
        //

ReplyPoint:
        if( iReply == IDCANCEL )
            goto OperationCancelled;


FinishType1Install:

NextSelection:

        if (SUCCEEDED(GetFontManager(&poFontManager)))
        {
            poFontManager->bReleaseInstallationMutex();
            bOwnInstallationMutex = FALSE;
            ReleaseFontManager(&poFontManager);
        }
    }  // While selected desc

    //
    //  Update the overall progress dialog - show a 100% message
    //

    UpdateProgress( iTotalFonts, iTotalFonts, 100 );

    Sleep( 1000 );

//
// Don't update progress indicator if user cancelled out of operation.
//
OperationCancelled:

    TermProgress( );
    TermPSInstall( );

    if (SUCCEEDED(GetFontManager(&poFontManager)))
    {
        if (bOwnInstallationMutex)
        {
            poFontManager->bReleaseInstallationMutex();
        }
        ReleaseFontManager(&poFontManager);
    }

    return bFontsInstalled;
}


/***************************************************************************
 * FUNCTION:   bInstallFont
 *
 * PURPOSE:    Install the font specified by the inputs.  The reply parameter
 *             specifies how, in the event of failure, the user wishes to
 *             proceeed.
 *
 * RETURNS:    BOOL - success of attempt.
 ***************************************************************************/

BOOL NEAR PASCAL bInstallFont( HWND hwndParent,
                               LPTSTR lpszSrcPath,
                               BOOL   bTrueType,
                               PTSTR  szLHS,
                               int*   iReply )
{
    LPTSTR          lpszResource;
    FullPathName_t  szFullPath;
    FullPathName_t  szFontsDir;
    LPTSTR          lpszName;
    BOOL            bSuccess = FALSE;
    BOOL            bInFontsDir = FALSE;


    //
    //  Determine if this file is in the FONTS directory.
    //

    lstrcpy( szFullPath, lpszSrcPath );

    lpszName = lpNamePart( szFullPath );

    if( lpszName == szFullPath )
    {
        bInFontsDir = TRUE;
    }
    else
    {
        *(lpszName-1) = 0;

        GetFontsDirectory( szFontsDir, ARRAYSIZE( szFontsDir ) );

        if( !lstrcmpi( szFontsDir, szFullPath ) )
        {
           bInFontsDir = TRUE;
        }
    }

    //
    //  If it is a TrueType font, the input file will be the TTF.
    //  Generate the corresponding *.FOT file
    //

    if( bInFontsDir )
        lpszResource = lpszName;
    else
        lpszResource = lpszSrcPath;


    //
    //  Add the font resource, and then add the font record to our list.
    //  If these both succeed, we've finally reached the ultimate return point.
    //

    if( AddFontResource( lpszResource ) )
    {
        CFontManager *poFontManager;
        if (SUCCEEDED(GetFontManager(&poFontManager)))
        {
            if(poFontManager->poAddToList(szLHS, lpszResource, NULL) != NULL )
            {
                // WriteProfileString( szINISFonts, szLHS, lpszResource );
                WriteToRegistry( szLHS, lpszResource );
                bSuccess = TRUE;
            }
            ReleaseFontManager(&poFontManager);
            return bSuccess;
        }
        else
        {
            //
            //  Clear if we couldn't add
            //
            RemoveFontResource( lpszResource );
        }
    }
#ifdef _DEBUG
    else
        DEBUGMSG( (DM_ERROR, TEXT( "AddFontResource failed on %s" ),
                   lpszResource ) );
#endif

    //
    //  If we've failed in the final stages, report to the user.  We also
    //  need to clean up any file we've created.
    //

    if( bInFontsDir )
        vCPDeleteFromSharedDir( lpszSrcPath );

    *iReply = iUIMsgOkCancelExclaim(hwndParent, IDSI_FMT_BADINSTALL, IDSI_CAP_NOINSTALL,
                                     szLHS );

    return bSuccess;
}


BOOL HitTestDlgItem(int x, int y, HWND hwndItem)
{
    const POINT pt = { x, y };
    RECT rc;
    GetWindowRect(hwndItem, &rc);
    return PtInRect(&rc, pt);
}


/*************************************************************************
 * FUNCTION: FontHookProc
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 *************************************************************************/
UINT_PTR CALLBACK FontHookProc( HWND hWnd,
                                UINT iMessage,
                                WPARAM wParam,
                                LPARAM lParam )
{
    switch( iMessage )
    {

    case WM_INITDIALOG:
        DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: ------------ WM_INITDIALOG " ) ) );

        s_pDlgAddFonts->Attach( hWnd );

        s_pDlgAddFonts->m_pOpen = (LPOPENFILENAME) lParam;

        s_pDlgAddFonts->CheckDlgButton( ID_BTN_COPYFILES, TRUE );

        s_pDlgAddFonts->vStartFonts( );

        SetFocus( s_pDlgAddFonts->GetDlgItem( IDOK ) );
        break;

    case WM_DESTROY:
        s_pDlgAddFonts->Detach( );
        break;

    case WM_HELP:
        if (IsWindowEnabled(hWnd))
        {
            LPHELPINFO lphi = (LPHELPINFO)lParam;
            if (HELPINFO_WINDOW == lphi->iContextType)
            {
                for (int i = 0; 0 != rgHelpIDs[i]; i += 2)
                {
                    if (lphi->iCtrlId == (int)rgHelpIDs[i])
                    {
                        //
                        // Only display custom help when necessary.
                        // Otherwise, use standard "file open dlg" help.
                        //
                        WinHelp( (HWND)lphi->hItemHandle,
                                 NULL,
                                 HELP_WM_HELP,
                                 (DWORD_PTR)(LPVOID)rgHelpIDs);

                        return TRUE;
                    }
                }
            }
        }
        break;

    case WM_CONTEXTMENU:
         {
            const x = GET_X_LPARAM(lParam);
            const y = GET_Y_LPARAM(lParam);
            for (int i = 0; 0 != rgHelpIDs[i]; i += 2)
            {
                HWND hwndItem = GetDlgItem(hWnd, rgHelpIDs[i]);
                //
                // This hit test shouldn't be required.  For some reason
                // wParam is the HWND of the dialog whenever the user 
                // right-clicks on some of our template controls.  I can't
                // figure it out but the hit test adjusts for the problem.
                // [brianau - 6/8/99]
                // 
                if ((HWND)wParam == hwndItem || HitTestDlgItem(x, y, hwndItem))
                {
                    //
                    // Only display custom help when necessary.
                    // Otherwise, use standard "file open dlg" help.
                    //
                    WinHelp( (HWND)wParam,
                              NULL,
                              HELP_CONTEXTMENU,
                              (DWORD_PTR)(LPVOID)rgHelpIDs);
                              
                    return TRUE;
                }                 
            }
        }
        break;

    case WM_COMMAND:
        switch( GET_WM_COMMAND_ID( wParam, lParam ) )
        {
        //
        //  command switch
        //
        case IDM_IDLE:
            vCPFilesToDescs( );
            break;

        case ID_BTN_SELALL:
            //
            //  select all
            //
            s_pDlgAddFonts->pListBoxDesc()->SetSel( -1, TRUE );
            break;

        case ID_BTN_HELP:
            WinHelp( hWnd, TEXT( "WINDOWS.HLP>PROC4" ), HELP_CONTEXT,
                     IDH_WINDOWS_FONTS_ADDNEW_31HELP );
            break;

        case ID_LB_FONTDIRS:
            if( GET_WM_COMMAND_CMD( wParam, lParam ) == LBN_DBLCLK )
                s_pDlgAddFonts->vStartFonts( );
            break;

        case IDOK:
            if( s_pDlgAddFonts->pListBoxDesc()->GetSelCount() > 0 )
                s_pDlgAddFonts->vAddSelFonts( );
            else
                s_pDlgAddFonts->vStartFonts();

            break;

        case IDCANCEL:
        case IDABORT:
            ResetAtomInDescLB( s_pDlgAddFonts->pListBoxDesc()->hWnd() );
            s_pDlgAddFonts->EndDialog( 0 );
            break;

        case ID_LB_ADD:
            // if( HIWORD( lParam ) == LBN_DBLCLK )

            if( GET_WM_COMMAND_CMD( wParam,lParam ) == LBN_DBLCLK )
                s_pDlgAddFonts->vAddSelFonts( );
            break;

        case ID_CB_FONTDISK:
            switch( GET_WM_COMMAND_CMD( wParam, lParam ) )
            {
            //
            //  Switch on combo parameter
            //

            case CBN_DROPDOWN:
                s_pDlgAddFonts->vHoldComboSel();
                break;

            case CBN_CLOSEUP:
                s_pDlgAddFonts->vCloseCombo( );
                break;

            case CBN_SELCHANGE:
                s_pDlgAddFonts->vNewComboSel( );
                break;
           }  // Switch on combo parameter
           break;

        } // command switch
        break;

    default:
        if( iMessage == s_iLBSelChange )
        {
            switch( wParam )
            {
            case ID_CB_FONTDISK:
                switch( HIWORD( lParam ) )
                {
                case CD_LBSELCHANGE:
                    //
                    //  This catches the DriveNotReady case
                    //  This code is hit once before WM_INITDIALOG is handled.
                    //  The check for a valid hWnd prevents a DEBUGBREAK in
                    //  dwThreadProc.
                    //

                    if (NULL != s_pDlgAddFonts->hWnd())
                        s_pDlgAddFonts->vStartFonts( );
                    break;
                }
#ifdef WINNT
                //
                // Fall through...
                // We want to capture current directory list selection
                // if either directory or drive changes.
                //
            case ID_LB_FONTDIRS:
               if (HIWORD(lParam) == CD_LBSELCHANGE)
               {
                  int cch     = 0;            // Index into s_szCurDir.
                  int iDirNew = 0;            // Id of directory item open in listbox.
                  BOOL bBufOverflow = FALSE;  // Buffer overflow indicator.

                  //
                  // Build current path selected in directory list box.
                  // We save this path in s_szCurDir so that if the FileOpen dialog is closed and
                  // re-opened, it will start navigating where it last left off.
                  // This path-building code was taken from the common dialog module fileopen.c
                  // The buffer overflow protection was added.
                  //
                  iDirNew = (DWORD)SendMessage( GetDlgItem(hWnd, ID_LB_FONTDIRS), LB_GETCURSEL, 0, 0L );
                  cch = (int)SendMessage(GetDlgItem(hWnd, ID_LB_FONTDIRS), LB_GETTEXT, 0, (LPARAM)(LPTSTR)s_szCurDir);

                  if (DBL_BSLASH(s_szCurDir))
                  {
                      lstrcat(s_szCurDir, TEXT("\\"));
                      cch++;
                  }

                  for (int idir = 1; !bBufOverflow && idir <= iDirNew; ++idir)
                  {
                      TCHAR szTemp[MAX_PATH + 1]; // Temp buf for directory name.
                      int n = 0;                  // Chars in directory name.

                      n = (int)SendDlgItemMessage(
                                    hWnd,
                                    ID_LB_FONTDIRS,
                                    LB_GETTEXT,
                                    (WPARAM)idir,
                                    (LPARAM)szTemp );

                      //
                      // Check if this directory name will overflow s_szCurDir.
                      //
                      if (cch + n < ARRAYSIZE(s_szCurDir))
                      {
                          //
                          // We have enough space for this directory name.
                          // Append it to s_szCurDir, advance the buffer index and
                          // append a backslash.
                          //
                          lstrcpy(&s_szCurDir[cch], szTemp);
                          cch += n;
                          s_szCurDir[cch++] = CHAR_BSLASH;
                      }
                      else
                          bBufOverflow = TRUE;  // This will terminate the loop.
                                                // s_szCurDir will still contain
                                                // a valid path.  It will just
                                                // be shy 1 or more directories.
                  }

                  //
                  // All done.  Terminate it.
                  // Note that this wipes out the final trailing backslash.
                  //
                  if (iDirNew)
                  {
                      s_szCurDir[cch - 1] = CHAR_NULL;
                  }
               }
#endif
               break;
            }
         }
         break;
    } // message switch

    //
    //  commdlg, do your thing
    //

    return FALSE;
}


/***************************************************************************
 * FUNCTION:   vCPDeleteFromSharedDir
 *
 * PURPOSE:    Delete the input file from the shared directory - a cleanup
 *             function for our installation attempts
 *
 * RETURNS:    None
 ***************************************************************************/

VOID FAR PASCAL vCPDeleteFromSharedDir( LPTSTR lpszFileOnly )
{
    FullPathName_t szTempPath;


    vPathOnSharedDir( lpNamePart( lpszFileOnly ), szTempPath );

    DeleteFile( szTempPath );
}


/***************************************************************************
 * FUNCTION:   vCPFilesToDescs
 *
 * PURPOSE:    We're during idle here, so try to convert at least one item
 *             from the selected file list to the description list (which is
 *             the one the user can see)
 *
 * RETURNS:    None
 ***************************************************************************/

VOID FAR PASCAL vCPFilesToDescs( )
{
    TCHAR          szNoFonts[ 80 ];
    BOOL           bSomeDesc;
    int            nDescSlot;
    AddITEMDATA    OurData;
    FullPathName_t szFilePath;
    FontDesc_t     szDesc;
    CListBox*      pListDesc;
    CListBox*      pListFiles;
    MSG            msg;


    if( !s_pDlgAddFonts || !s_pDlgAddFonts->m_nFontsToGo )
        return;

    pListFiles = s_pDlgAddFonts->pListBoxFiles( );
    pListDesc  = s_pDlgAddFonts->pListBoxDesc( );

    if( s_pDlgAddFonts->bStartState( ) )
    {
        //
        //  Reset the atoms that are in here.
        //

        ResetAtomInDescLB( pListDesc->hWnd( ) );

        //
        //  Make sure our focus isn't off in some weird place - force it
        //  to our directory list
        //

        HWND hFocus = ::GetFocus( );

        int iFocusID;


        if( hFocus != NULL )
            iFocusID = ::GetDlgCtrlID( hFocus );
        else
            iFocusID = ID_LB_ADD;

        if( ( iFocusID == (ID_LB_ADD) ) || (iFocusID == (ID_SS_PCT) ) )
        {
            ::SendMessage( s_pDlgAddFonts->hWnd( ), WM_NEXTDLGCTL,
            (WPARAM)GetDlgItem( s_pDlgAddFonts->hWnd( ), ID_LB_FONTDIRS ), 1L );
        }

        pListDesc->ResetContent( );
        pListDesc->UpdateWindow( );

        s_pDlgAddFonts->vUpdatePctText( );

        if( !s_pDlgAddFonts->bInitialFonts( ) )
        {
            bSomeDesc = FALSE;
            goto Done;
        }

        pListDesc->SetRedraw( FALSE );

        //
        //  DBCS. The first time through, look for oemsetup.inf
        //

        {
        int        nFileIndex;
        WORD       wRet;
        ADDFNT     stData;
        TCHAR      szInfFile[ MAXFILE ];
        FontDesc_t szTemp;

        if( FindOemInList( pListFiles, s_pDlgAddFonts->m_nFontsToGo,
                           &nFileIndex, szInfFile ) )
        {
            //
            //  save original path to setup.inf
            //

            lstrcpy( szTemp, szSetupInfPath );

            //
            //  get dir of oemsetup.inf
            //

            GetCurrentDirectory( ARRAYSIZE( szSetupInfPath ), szSetupInfPath );

            lpCPBackSlashTerm( szSetupInfPath );
            lstrcat( szSetupInfPath, szInfFile );

            stData.poListDesc = pListDesc;
            stData.nIndex     = nFileIndex;
            stData.which      = WHICH_FNT_WIFE;

            if( ( wRet = ReadSetupInfCB( szSetupInfPath, WIFEFONT_SECTION,
                    (LPSETUPINFPROC) GetNextFontFromInf, &stData ) ) != NULL )
            {
                //
                //  didn't reach to the end of section
                //

                if( wRet == INSTALL+14 )
                    //
                    //  didn't find the section
                    //
                    goto ScanTTInf;
            }
            else
            {
ScanTTInf:
                stData.which = WHICH_FNT_TT;

                wRet = ReadSetupInfCB( szSetupInfPath, TRUETYPE_SECTION,
                                       (LPSETUPINFPROC) GetNextFontFromInf,
                                       &stData );
            }

            //
            //  reset setupinf path global
            //

            lstrcpy( szSetupInfPath,szTemp );

            if( wRet && wRet != INSTALL+14 )
            {
                //
                //  Found the section, but invalid format
                //

                bSomeDesc = FALSE;
                goto Done;
            }
        }
        } // End of DBCS section.
    }

    //
    //  We want to read at least one
    //

    goto ReadNext;

    for(  ; s_pDlgAddFonts->m_nFontsToGo; )
    {
        if( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )
            return;

ReadNext:

        s_pDlgAddFonts->m_nFontsToGo--;
        s_pDlgAddFonts->vUpdatePctText( );

        OurData.nFileSlot = (WORD)s_pDlgAddFonts->m_nFontsToGo;

        if( pListFiles->GetText( OurData.nFileSlot, szFilePath ) == LB_ERR )
            continue;

        WORD  wType;

        DEBUGMSG( (DM_TRACE1, TEXT( "Checking file: %s" ), szFilePath ) );

        if( !::bCPValidFontFile( szFilePath, szDesc, &wType ) )
        {
            DEBUGMSG( (DM_TRACE1, TEXT( "......Invalid" ) ) );
            continue;
        }

        DEBUGMSG( (DM_TRACE1, TEXT( "......Valid.   Desc: %s" ), szDesc) );

        OurData.wFontType = wType;

        //
        //  See if there's already an entry for this font name - if so, don't
        //  add it again.  If there isn't, go ahead and add, setting our
        //  item data block.
        //

        if( pListDesc->FindStringExact( -1, szDesc ) == LB_ERR )
        {
            nDescSlot = pListDesc->AddString( szDesc );

            if( nDescSlot != LB_ERR )
                pListDesc->SetItemData( nDescSlot, OurData.ItemData );
            else
            {
                DEBUGMSG( (DM_ERROR, TEXT( "FONTEXT: Error adding string %s" ),
                           szDesc ) );
                // DEBUGBREAK;
            }
        }
        else
            DEBUGMSG( (DM_TRACE1,TEXT( "String %s already in list" ), szDesc ) );

    }

    s_pDlgAddFonts->vUpdatePctText( );

    bSomeDesc = (pListDesc->GetCount( ) > 0 );

    DEBUGMSG( (DM_TRACE1,TEXT( "Count in ListDesc: %d" ), pListDesc->GetCount( ) ) );
//  DEBUGBREAK;

Done:

    if( !bSomeDesc )
    {
        //
        //  cszNoFonts.LoadString( IDSI_MSG_NOFONTS );
        //

        LoadString( g_hInst, IDSI_MSG_NOFONTS, szNoFonts, ARRAYSIZE( szNoFonts ) );

        pListDesc->AddString( szNoFonts );
    }

    //
    //  Get everything back to the correct state before exiting.
    //  We can select all if we've got at least one item in the description
    //  list box.  Likewise, we can select from the list box itself.
    //  Re-enable redrawing of the box and invalidate to force a redraw.
    //

    // s_pDlgAddFonts->GetDlgItem( ID_BTN_SELALL )->EnableWindow( bSomeDesc );

    ::EnableWindow( s_pDlgAddFonts->GetDlgItem( ID_BTN_SELALL ), bSomeDesc );

    pListDesc->EnableWindow( bSomeDesc );

    pListDesc->SetRedraw( TRUE );

    pListDesc->InvalidateRect( NULL, TRUE );

    pListDesc->UpdateWindow( );
}


/*************************************************************************
 * FUNCTION: CPDropInstall
 *
 * PURPOSE:
 *          iCount - Number of fonts left to install, NOT counting this one
 *
 *
 * RETURNS:
 *
 *************************************************************************/

int FAR PASCAL CPDropInstall( HWND hwndParent,
                              LPTSTR lpszInPath,
                              DWORD  dwEffect ,
                              LPTSTR lpszDestName,
                              int    iCount )
{
    FullPathName_t szTruePath;
    FullPathName_t szFontPath;
    FullPathName_t szSourceDir;
    FILENAME       szInFile;
    FILENAME       szDstFile;
    FontDesc_t     szLHS;
    LPTSTR         lpCh;
    int            iReply;
    WORD           wType;
    BOOL           bTrueType;
    int            iReturn = CPDI_FAIL;
    BOOL           bNoCopyFile;
    UINT           uMB = ( (iCount > 0) ? (MB_OKCANCEL | MB_ICONEXCLAMATION)
                                      : MB_OK | MB_ICONEXCLAMATION );
    DWORD          dwStatus = FVS_MAKE_CODE(FVS_INVALID_STATUS, FVS_FILE_UNK);

    static BOOL s_bInit = FALSE;
    static int  s_iTotal = 1;

    BOOL bOwnInstallationMutex = FALSE;
    HWND hwndProgress = NULL;
    CFontManager *poFontManager = NULL;

    vEnsureInit( );

    //
    //  Init Type1 font installation and Progress dialog
    //

    if( !s_bInit )
    {
        InitPSInstall( );
        hwndProgress = InitProgress( hwndParent );

        s_iTotal = ( iCount > 0 ) ? ( iCount + 1 ) : 1;

        s_bInit = TRUE;


    }

    GetFontManager(&poFontManager);
    
    //
    // Must own installation mutex to install font.
    //
    if ( NULL != poFontManager )
    {
        INT iUserResponse  = IDRETRY;
        DWORD dwWaitResult = CFontManager::MUTEXWAIT_SUCCESS;

        while( IDRETRY == iUserResponse &&
               (dwWaitResult = poFontManager->dwWaitForInstallationMutex()) != CFontManager::MUTEXWAIT_SUCCESS )
        {
            if ( CFontManager::MUTEXWAIT_WMQUIT != dwWaitResult )
                iUserResponse = iUIMsgRetryCancelExclaim(hwndProgress, IDS_INSTALL_MUTEX_WAIT_FAILED, NULL);
            else
            {
                //
                // Cancel if thread received a WM_QUIT message while waiting.
                //
                iUserResponse = IDCANCEL;
            }
        }

        //
        // If user chose to cancel or we got a WM_QUIT msg, cancel the installation.
        //
        if ( IDCANCEL == iUserResponse )
        {
            iReturn = CPDI_CANCEL;
            goto done;
        }

        bOwnInstallationMutex = TRUE;
    }


    //
    // Update the font number in the progress dialog.
    // Leave the % complete unchanged.
    //
    UpdateProgress( s_iTotal, s_iTotal - iCount,
                  (s_iTotal - iCount - 1) * 100 / s_iTotal );


    //
    //  If this is a type1 font, then convert it and install the resulting
    //  TrueType file.
    //

    // BGK - Add copy/nocopy w/compress here

    bNoCopyFile = (dwEffect == DROPEFFECT_LINK );

    if( !::bCPValidFontFile( lpszInPath, szLHS, &wType, FALSE, &dwStatus ) )
    {
        //
        // Display message informing user that font file is invalid and why.
        // Abort the installation of this font if user pressed Cancel.
        //
        lstrcpy( szFontPath, lpszInPath );
        if (iUIMsgBoxInvalidFont(hwndProgress, szFontPath, szLHS, dwStatus) == IDCANCEL)
            iReturn = CPDI_CANCEL;
    }
    else if( bFontInstalledNow( szLHS ) )
    {
        if( iUIMsgBox(hwndProgress, IDSI_FMT_ISINSTALLED, IDS_MSG_CAPTION, uMB, szLHS )
                     == IDCANCEL )
        {
            iReturn = CPDI_CANCEL;
        }

    }
    else
    {
        bTrueType = (wType == TRUETYPE_FONT);

        vUIPStatusShow( IDS_FMT_FONTINS, szLHS );

        BOOL    bUpdateWinIni;
        int     ifType;


        if( (wType == TYPE1_FONT) || (wType == TYPE1_FONT_NC) )
        {
            //
            //  For installations involving the conversion of the Type1
            //  font to TrueType:
            //
            //         "lpszInPath" has the destination name of the
            //                      installed TrueType font file.
            //         "szLHS"      is munged to contain "(TrueType)".
            //

            switch( ::InstallT1Font( hwndProgress,
                                     !bNoCopyFile,    //  Copy TT file?
                                     TRUE,            //  Copy PFM/PFB files?
                                     FALSE,           //  Files in Shared Dir?
                                     lpszInPath,      //  IN:  PFM File & Dir
                                                      //  OUT: TTF File & Dir
                                     szLHS ) )        //  IN & OUT: Font desc
            {
            case TYPE1_INSTALL_TT_AND_MPS:
                //
                //  The PS font was converted to TrueType and a matching
                //  PostScript font is ALREADY installed.
                //
                // bDeletePSEntry = TRUE;
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

                iReturn = CPDI_SUCCESS;

                goto FinishTTInstall;


            case TYPE1_INSTALL_PS_AND_MTT:
                //
                //  The PostScript font was installed and we found a matching
                //  TrueType font that was already installed.
                //
                //  fall thru....

            case TYPE1_INSTALL_PS_ONLY:
                //
                //  Only the PostScript font was installed.
                //

                bUpdateWinIni = FALSE;
                iReturn = CPDI_SUCCESS;

                goto done;

            case TYPE1_INSTALL_IDYES:
            case TYPE1_INSTALL_IDOK:

                bUpdateWinIni = FALSE;
                iReturn = CPDI_SUCCESS;
                goto done;

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
                iReturn = CPDI_FAIL;
                goto done;

            case TYPE1_INSTALL_IDCANCEL:
            default:
                iReturn = CPDI_CANCEL;
                goto done;
            }

            //
            //  On leaving this conditional many variables must be set up
            //  correctly to proceed with installation of a TrueType font.
            //
            //  szLHS         - fontname description for listbox display
            //  ifType        - itemdata to attach to TT lbox entry
            //  lpszInPath    - filename of source font
            //  bTrueType     - TRUE if Type1 file converted to TT
            //

FinishTTInstall:

            //
            //  Determine if TTF file to install is in 'fonts' dir
            //

            lstrcpy( szFontPath, lpszInPath );


            LPTSTR lpCh = StrRChr( szFontPath, NULL, TEXT( '\\' ) );

            if( lpCh )
            {
                lpCh++;
                *lpCh = TEXT( '\0' );
            }

            bNoCopyFile = lstrcmpi( szFontPath, s_szSharedDir ) == 0;
        }

        //
        //  Start install progress for this font
        //

        ResetProgress( );
        Progress2( 0, szLHS );

        //
        //  If the file is compressed, then do a copy.
        //

        if( bIsCompressed( lpszInPath ) )
        {
            dwEffect = DROPEFFECT_COPY;
            bNoCopyFile = FALSE;
        }

        if( bNoCopyFile )
        {
            //
            //  If we're not copying the file, just make sure the font
            //  file path is copied to szFontPath so the font can be installed
            //  in the call to bInstallFont( ).
            //
            lstrcpy(szFontPath, lpszInPath);
        }
        else
        {
            //
            //  Before monkeying around with the name strings, grab the source
            //  directory, including the terminating slash.  Also hold the file
            //  portion.
            //

            //
            //  Copy in name only
            //

            lstrcpy( szInFile, lpNamePart( lpszInPath ) );

            lstrcpy( szSourceDir, lpszInPath );

            //
            //  Get past any path
            //

            lpCh  = StrRChr( szSourceDir, NULL, TEXT( '\\' ) );

            lpCh++;
            *lpCh = 0;

            //
            //  Let LZ tell us what the name should have been
            //

            if( ERROR_SUCCESS != IGetExpandedName( lpszInPath, szTruePath, PATHMAX ))
            {
                //
                //  GetExpanded failed. This usually means we can't get at
                //  the file for some reason.
                //

                iUIMsgOkCancelExclaim(hwndProgress, IDSI_FMT_BADINSTALL,
                                       IDSI_CAP_NOCREATE, szLHS );
                goto done;
            }

            //
            //  Now we're going to work on making the new file name - it's
            //  file only, and we'll tweak for uniqueness
            //

            if( lpszDestName && *lpszDestName )
            {
                lstrcpy( szDstFile, lpszDestName );
            }
            else
            {
                //
                //  Copy in name only
                //

                lstrcpy( szDstFile, lpNamePart( szTruePath ) );
            }

            if( !(bUniqueOnSharedDir( szDstFile, szDstFile ) ) )
            {
                iUIMsgOkCancelExclaim(hwndProgress, IDSI_FMT_BADINSTALL,
                                       IDSI_CAP_NOCREATE, szLHS );
                goto done;
            }

            //
            //  Ready to install the file
            //

            if( iReturn = (bCPInstallFile( hwndProgress, szSourceDir, szInFile, szDstFile )
                                        ? CPDI_SUCCESS : CPDI_FAIL ) )
            {
                vPathOnSharedDir( szDstFile, szFontPath );
            }
            else
                goto done;

            Progress2( 50, szLHS );

        }

        if( bInstallFont( hwndProgress, szFontPath, bTrueType, szLHS, &iReply ) )
        {
            iReturn = CPDI_SUCCESS;

            //
            //  Attempt to remove the source file, if the operation was
            //  a MOVE.
            //
            //  EXCEPTION: If we're doing a Type1 font installation, the name
            //  in lpszInPath buffer is the path to the matching TrueType font
            //  file created. If this is the case, do not delete it.
            //
            if( (wType != TYPE1_FONT) && (wType != TYPE1_FONT_NC) &&
                (dwEffect == DROPEFFECT_MOVE) )
            {
                //
                // SHFileOperation requires that file list be double-nul
                // terminated.
                //
                *(lpszInPath + lstrlen(lpszInPath) + 1) = TEXT('\0');

                SHFILEOPSTRUCT sFileOp = { NULL,
                                           FO_DELETE,
                                           lpszInPath,
                                           NULL,
                                           FOF_SILENT | FOF_NOCONFIRMATION,
                                           0,
                                           0
                                          };

                SHFileOperation( &sFileOp );
            }

            Progress2( 100, szLHS );
        }
        else if( !bNoCopyFile )
            vCPDeleteFromSharedDir( szDstFile );
    }

done:

    //
    //  Update the overall progress dialog
    //  Only update if user didn't cancel the operation.
    //
    if (CPDI_CANCEL != iReturn)
    {
        UpdateProgress( s_iTotal, s_iTotal - iCount,
                        (s_iTotal - iCount) * 100 / s_iTotal );
    }

    //
    //  If no more fonts coming down, properly terminate Progress
    //  dialog and reset local statics.  Also, give user time to
    //  to see this on a fast system (1 sec. delay).
    //
    if (InstallCancelled())
        iReturn = CPDI_CANCEL;

    if( s_bInit && (iCount == 0 || iReturn == CPDI_CANCEL) )
    {
        Sleep( 1000 );

        TermProgress( );
        TermPSInstall( );

        s_iTotal =  1;

        s_bInit = FALSE;
    }

    //
    // Release the installation mutex if we own it.
    //
    if ( NULL != poFontManager)
    {
        if (bOwnInstallationMutex)
            poFontManager->bReleaseInstallationMutex();

        ReleaseFontManager(&poFontManager);
    }
    return iReturn;
}


/*************************************************************************
 * FUNCTION: bCPAddFonts
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 *************************************************************************/

BOOL FAR PASCAL bCPAddFonts( HWND ma )
{
    FullPathName_t  szWinDir;
    FullPathName_t  szFileTemp = { TEXT( '\0' ) };
    TCHAR           cFilter[ 80 ];
    TCHAR           szFilter[ 80 ];

    //
    //  Current directory we're using
    //  Moved s_szCurDir to file-scope so that it can be updated through
    //  FontHookProc. [brianau]
    //
    //  static FullPathName_t s_szCurDir;

    static BOOL           s_bFirst = TRUE;

    //
    //  This code is NOT REENTRANT!!!!.
    //  Make sure we don't.
    //

    if( s_pDlgAddFonts )
        return FALSE;

    // DEBUGBREAK;

    vEnsureInit( );

    //
    //  We start the current directory to be the windows directory - it will
    //  later remain as where the user last set it.
    //

    if( s_bFirst )
    {
        s_bFirst = FALSE;
        GetWindowsDirectory( s_szCurDir, ARRAYSIZE( s_szCurDir ) );
    }

    //
    //  Nothing added yet!
    //

    BOOL bFontsAdded = FALSE;

    //
    //  We need to set the font filter.  If TrueType is enabled, we use a
    //  filter that includes these files.  Otherwise, a simpler filter is
    //  used.  The filter has embedded nulls, which are stored in the
    //  resource file as hashes
    //

    if( GetProfileInt( szINISTrueType, szINIKEnable,1 ) )

       LoadString( g_hInst, IDS_MSG_ALLFILTER, cFilter, ARRAYSIZE( cFilter ) );

    else

       LoadString( g_hInst, IDS_MSG_NORMALFILTER, cFilter, ARRAYSIZE( cFilter ) );


    lstrcpy( szFilter, cFilter );

    vHashToNulls( szFilter );

    //
    //  Now we'll use the common open-file dialog to present the user with
    //  some choices on fonts to add
    //

    static OPENFILENAME OpenFileName;

    memset( &OpenFileName, 0, sizeof( OpenFileName ) );

    OpenFileName.lStructSize    = sizeof( OPENFILENAME );
    OpenFileName.hwndOwner      = ma;
    OpenFileName.hInstance      = g_hInst;
    OpenFileName.lpstrFilter    = szFilter;
    OpenFileName.nFilterIndex   = 1;
    OpenFileName.lpstrFile      = szFileTemp;
    OpenFileName.nMaxFile       = ARRAYSIZE( szFileTemp );
    OpenFileName.lpstrInitialDir= s_szCurDir;

    OpenFileName.Flags          = OFN_HIDEREADONLY   | OFN_ENABLEHOOK |
                                  OFN_ENABLETEMPLATE;
    OpenFileName.lpTemplateName = MAKEINTRESOURCE( ID_DLG_FONT2 );
    OpenFileName.lpfnHook       = FontHookProc;

    //
    //  This is our companion struture, which we handle independently
    //

    s_pDlgAddFonts = new AddFontsDialog;

    if(NULL == s_pDlgAddFonts)
    {
        DEBUGMSG( (DM_ERROR, TEXT( "AddFontsDialog not created." ) ) );

        // DEBUGBREAK;
        // BUGBUG: Way low on memory. MessageBox?
        return FALSE;
    }

    if (!s_pDlgAddFonts->bInitialize())
    {
        DEBUGMSG( (DM_ERROR, TEXT( "AddFontsDialog initialization failed." ) ) );
        s_pDlgAddFonts->Release();
        s_pDlgAddFonts = NULL;

        return FALSE;
    }

    s_iLBSelChange = RegisterWindowMessage( LBSELCHSTRING );

//
//  Suspension of the file system notify thread is no longer required.
//  It has been superceded by the installation mutex in CFontManager.
//  See comment in header of CFontManager::iSuspendNotify() for details.
//
//    if( poFontMan )
//        poFontMan->iSuspendNotify( );

    GetOpenFileName( &OpenFileName );

//    if( poFontMan )
//        poFontMan->iResumeNotify( );

    bFontsAdded = s_pDlgAddFonts->bAdded( );

    s_pDlgAddFonts->EndThread(); // Stop the IDM_IDLE thread.
    s_pDlgAddFonts->Release();   // Decr ref count.
    s_pDlgAddFonts = NULL;       // Static ptr no longer used.

    //
    //  save the current dir so we can restore this
    //  Modified so current directory is saved in FontHookProc.
    //
    //  GetCurrentDirectory( ARRAYSIZE( s_szCurDir ), s_szCurDir );

    //
    //  set the current dir back to windows so we don't hit the floppy
    //

    GetWindowsDirectory( szWinDir, ARRAYSIZE( szWinDir ) );

    SetCurrentDirectory( szWinDir );

    return bFontsAdded > 0;
}


/*************************************************************************
 * FUNCTION: CopyTTOutlineWithInf
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 *************************************************************************/

BOOL NEAR PASCAL CopyTTOutlineWithInf( HWND hwndFontDlg,
                                       LPTSTR pszInfSection,
                                       LPTSTR szDesc,
                                       LPTSTR szSrc,
                                       LPTSTR szDst )
{
    TCHAR szTemp[ PATHMAX ];
    TCHAR szDstName[ PATHMAX ];
    BOOL bInstalled = FALSE;
    LPTSTR lpTemp ;
    LPTSTR pszFiles[ 30 ];
    UINT   nFiles;
    DWORD  dwInstallFilesResult = 0;

#ifdef PROGRESS
    TCHAR szStatus[ 64 ];
#endif

    DEBUGMSG( (DM_TRACE1, TEXT( "CopyTTOutlineWithInf()" ) ) );
    DEBUGMSG( (DM_TRACE1, TEXT( "\tszDesc: %s" ), szDesc) );
    DEBUGMSG( (DM_TRACE1, TEXT( "\tszSrc: %s" ), szSrc) );
    DEBUGMSG( (DM_TRACE1, TEXT( "\tszDst: %s" ), szDst) );
    // DEBUGBREAK;

    //
    //  Set global window handle. used in InstallFiles to determine title of
    //  AddFile dialog.
    //

    ghwndFontDlg = hwndFontDlg; // s_pDlgAddFonts->hWnd( );

    //
    //  Get destination filename from line .
    //

    if( lpTemp = StrChr( pszInfSection, TEXT( '=' ) ) )
    {
        *lpTemp = TEXT( '\0' );

        //
        //  Got left of TEXT( '=' ).
        //

        lstrcpy( szSrc, pszInfSection );

        CutOffWhite( szSrc );

        DEBUGMSG( (DM_TRACE1, TEXT( "szSrc after CutOffWhite %s" ), szSrc) );

        pszInfSection = lpTemp + 1;
    }
    else
    {
        //
        //  Bad format inf file.
        //

        DEBUGMSG( (DM_TRACE1, TEXT( "Bad Format inf file: %s" ), pszInfSection) );

        bInstalled = FALSE;

        goto NoMoreFiles;
    }

    //
    //  Assume that existing check has been already done in AddFonts( ).
    //

    //
    //  Right side of TEXT( '=' ) can be shirink without any space.
    //

    CutOffWhite( pszInfSection );

    DEBUGMSG( (DM_TRACE1, TEXT( "pSection after CutOffWhite: %s" ), pszInfSection) );

    //
    //  Build up params for InstallFiles. Now we have pLine as x:name,y:name..
    //

    for(  nFiles = 0, lpTemp = pszInfSection; nFiles < 30; )
    {
        pszFiles[ nFiles ] = lpTemp;

        DEBUGMSG( (DM_TRACE1, TEXT( "File %d: %s" ), nFiles, lpTemp) );
        // DEBUGBREAK;

        nFiles++;

        //
        //  Null terminate each file name string
        //

        if( lpTemp = StrChr( lpTemp+1,TEXT( ',' ) ) )
        {
            *lpTemp ++ = TEXT( '\0' );
        }
        else
            //
            //  Reach end of line.
            //

            break;
    }

#ifdef PROGRESS
    if( hSetup && nFiles )
    {
        ProClear( NULL );

        LoadString( hInst, IDS_WAITCOPYFONT, szStatus, ARRAYSIZE( szStatus ) );

        ProSetText( ID_STATUS1, szStatus );

        LoadString( hInst, IDS_COPYING, szStatus, ARRAYSIZE( szStatus ) );

        ProPrintf( ID_STATUS2, szStatus, (LPTSTR) szDesc );

        LoadString( hInst, IDS_FILE, szStatus, ARRAYSIZE( szStatus ) );

        ProPrintf( ID_STATUS3, szStatus, (LPTSTR) szSrc );

        ProSetBarRange( nFiles );

        ProSetBarPos( 0 );

        if ((dwInstallFilesResult = InstallFiles(hwndFontDlg, pszFiles, nFiles,
                                        SuFontCopyStatus, IFF_CHECKINI)) != nFiles)
        {
            goto NoMoreFiles;
        }
        else
        {
            ProSetBarPos( 100 );
        }
    }
    else
#endif
    if ((dwInstallFilesResult = InstallFiles(hwndFontDlg, pszFiles, nFiles,
                                             NULL, IFF_CHECKINI)) != nFiles)
    {
        goto NoMoreFiles;
    }

    lstrcpy( szDstName, s_szSharedDir );

    lpCPBackSlashTerm( szDstName );

    lstrcat( szDstName, szSrc );


    //
    //  On success, return the place we installed the file.
    //

    lstrcpy( szDst, szDstName );

    //
    //  If source file was splited into multiple files, then we build up
    //  single destination file.
    //

    if(  nFiles  )
    {
        short nDisk;

        GetDiskAndFile( pszFiles[ 0 ], &nDisk, szTemp, ARRAYSIZE( szTemp ) );

        //
        //  Even when nFiles == 1, if the a source file name (we just copied)
        //  is different from the one of destination, we must copy it to
        //  actual destination file.
        //

        if(  lstrcmpi( szSrc, szTemp )  )

#ifdef PROGRESS
        {
            if( hSetup )
            {
                LoadString( hInst, IDS_CAT, szStatus, ARRAYSIZE( szStatus ) );
                ProSetText( ID_STATUS3, szStatus );
            }
            bInstalled = fnAppendSplitFiles( pszFiles, szDstName, nFiles );
        }
#else
        bInstalled = fnAppendSplitFiles( pszFiles, szDstName, nFiles );
#endif

    }

NoMoreFiles:

    //
    // If user aborted file installation, set return value so callers will
    // know this.  Callers look for ~01 value.
    //
    if ((DWORD)(-1) == dwInstallFilesResult)
        bInstalled = ~01;

    //
    //  If we didn't create the final dest file, make sure it is deleted.
    //

    if( !bInstalled )
    {
        vCPDeleteFromSharedDir( szDstName );
    }

    ghwndFontDlg = NULL;

    return bInstalled;
}

/*************************************************************************
 * FUNCTION: CopyTTFontWithInf
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 *************************************************************************/

BOOL NEAR PASCAL CopyTTFontWithInf( HWND hwndFontDlg,
                                    LPTSTR szProfile,
                                    LPTSTR szDesc,
                                    LPTSTR szSrc,
                                    LPTSTR szDst )
{
    TCHAR   szSect[ MAX_FF_PROFILE_LEN+14 ];
    LPTSTR  pszInfSection = NULL;
    LPTSTR  lpch;
    BOOL    bRet = FALSE;

    lstrcpy( szSect, szProfile );

    //
    //  'outline' section
    //

    lstrcpy( (lpch = szSect + lstrlen( szProfile ) ),
              TEXT( ".outline" ) /* szTTInfOutline */);

    DEBUGMSG( (DM_TRACE1,TEXT( "CopyTTFontWithInf" ) ) );

    DEBUGMSG( (DM_TRACE1,TEXT( "\tszProfile: %s" ), szProfile ) );

    DEBUGMSG( (DM_TRACE1,TEXT( "\tszSect: %s" ), szSect ) );
    // DEBUGBREAK;

    ReadSetupInfSection(szSetupInfPath, szSect, &pszInfSection);

    if (NULL != pszInfSection)
    {
        if ((bRet = CopyTTOutlineWithInf(hwndFontDlg, pszInfSection, szDesc, szSrc, szDst)))
        {
            if (~01 == bRet)
            {
                //
                // User abort.
                //
                DEBUGMSG( (DM_ERROR, TEXT( "CopyTTFontWithInf: Return UserAbort!" ) ) );
            }
        }
        else
        {
            //
            //  Fail at installing outline font.
            //
            DEBUGMSG( (DM_ERROR, TEXT( "CopyTTFontWithInf: Error CopyTTOutlineWithInf" ) ) );
        }
        LocalFree(pszInfSection);
    }
    else
    {
        //
        //  Maybe .inf error.
        //
        DEBUGMSG( (DM_ERROR, TEXT( "CopyTTFontWithInf: Error ReadSetUpInf" ) ) );
    }

    return bRet;
}

/*************************************************************************
 * FUNCTION: bInstallOEMFile
 *
 *   LPTSTR lpszDir,      // the directory where this thing is.
 *   LPTSTR lpszDstName,  // Full path to oemsetup.inf on entry
 *   LPTSTR lpszDesc,     // Description of font.
 *   WORD   wFontType,
 *   WORD  wCount )
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 *************************************************************************/
BOOL bInstallOEMFile( LPTSTR lpszDir,
                      LPTSTR lpszDstName,
                      LPTSTR lpszDesc,
                      WORD   wFontType,
                      WORD  wCount )
{
    FullPathName_t szSrcName;
    TCHAR          szTag[ 80 ];

    static FullPathName_t  szOemInfPath;

    TCHAR   szTemp[ PATHMAX ];
    HANDLE  hSection = NULL;


    DEBUGMSG( (DM_TRACE1, TEXT( "bInstallOEMFile( %s, %s )" ), lpszDstName,
                                                               lpszDesc) );
    //
    //  DEBUGBREAK;
    //  copy .inf file into fonts directory
    //
    // Remember the source directory.
    //

    lstrcpy( szSrcName, lpszDstName );

    if(  wCount == 0 )
    {
        //
        //  Assume all of description are from same .inf when first one is.
        //

        if( !CopyNewOEMInfFile( lpszDstName ) )
        {
            //
            //  TODO. ui message of some sort.
            //
            return FALSE;
        }

        //
        //  lpszDestName now has the new location of the oemsetup.inf file.
        //

        lstrcpy( szOemInfPath, lpszDstName );

        DEBUGMSG( (DM_TRACE1,TEXT( "szOemInfPath: %s " ), szOemInfPath ) );

        //
        //  Let InstallFiles() to prompt correct directory.
        //

        lstrcpy( szSetupDir, lpszDir );
    }

    //
    //  Build oemsetup.inf path..
    //

    lstrcpy( szTemp, szSetupInfPath );

    lstrcpy( szSetupInfPath,szOemInfPath );

    if( wFontType > 0xC000 )
    {
        DEBUGMSG( (DM_TRACE1,TEXT( "Can't do a WIFE font, yet" ) ) );
        return FALSE;

        //
        // In this case, bTrueType is atom for tag string of wifefont.
        //

        if( GetAtomName( wFontType, szTag, ARRAYSIZE( szTag ) - 1 ) )
        {
#if 0 // EMR. We don't do WIFE, yet.

            //
            //  Build driver section string..
            //

            lstrcpy( FdDesc,szTag );

            lstrcat( FdDesc,szWifeInfDrivers );

            if( !(hSection = ReadSetupInf( FdDesc ) ) )
            {
               goto InfError;
            }

            if( !AddFontDrvFromInf( hwndFontDlg,hSection ) )
                  goto InfError;

            hSection = LocalFree( hSection );

            //
            //  Build fonts section string..
            //

            lstrcpy( FfDesc,szTag );

            lstrcat( FfDesc,szWifeInfFonts );

            if( !(hSection = ReadSetupInf( FfDesc ) ) )
            {
                  goto InfError;
            }

            if( !CopyWifeFontWithInf( hwndFontDlg, hSection,
                                      NULL, szSrcName, szDstName ) )
                  goto InfError;

            hSection = LocalFree( hSection );

            //
            //  We have already this in system directory.
            //

            bInShared = TRUE;

            //
            //  Actually this is not TrueType.
            //

            bTrueType = FALSE;
#endif
        }
        else
        {
            //
            // Bad condition..maybe mem error or something like that
            //
InfError:
            if( hSection )
               LocalFree( hSection );

            //
            //  Restore setup.inf path.
            //

            lstrcpy( szSetupInfPath, szTemp );

            DEBUGMSG( (DM_TRACE1, TEXT( "Error in OEM install" ) ) );
            // DEBUGBREAK;

#if 0
            - TODO: Need an error message here.

            MyMessageBox( hwndFontDlg, INSTALL+14, INITS+1,
                          MB_OK|MB_ICONEXCLAMATION ) ;
#endif
            return FALSE;
        }
    }
    else if( GetAtomName( wFontType | TRUETYPE_WITH_OEMINF,
                          szTag, ARRAYSIZE( szTag ) - 1 ) )
    {
        BOOL bRet;

        //
        //  When 1 < bTrueType < C000, it also must be an atom for tag string,
        // but it lost 'which bit' when the value was set into listbox.
        // Treat this case as TRUETYPE.
        //

        lstrcpy( szDirOfSrc, lpszDir );

        DEBUGMSG( (DM_TRACE1, TEXT( "Calling CopyTTFontWithInf()." ) ) );
        // DEBUGBREAK;

        if( !(bRet = CopyTTFontWithInf( s_pDlgAddFonts->hWnd(), szTag,
                                        lpszDesc, szSrcName, lpszDstName ) ) )
             goto InfError;

        //
        //  Check User Abort.
        //

        if( bRet == ~01 )
            return FALSE;

        // lstrcpy( lpszDir,lpszDstName );

    }

    //
    //  Restore setup.inf path.
    //

    lstrcpy( szSetupInfPath,szTemp );


    return TRUE;
}
