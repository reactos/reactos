///////////////////////////////////////////////////////////////////////////////
//
// fontman.cpp
//      Explorer Font Folder extension routines.
//      Implementation for the class: CFontManager
//
//
// History:
//      31 May 95 SteveCat
//          Ported to Windows NT and Unicode, cleaned up
//
//
// NOTE/BUGS
//
//  Copyright (C) 1992-93 ElseWare Corporation.    All rights reserved.
//  Copyright (C) 1992-1995 Microsoft Corporation
//
///////////////////////////////////////////////////////////////////////////////

//==========================================================================
//                              Include files
//==========================================================================

#include "priv.h"
#include "globals.h"

#include <shlobjp.h>
#include <setupapi.h>

// #include "ole2.h"

#include "fontman.h"
#include "fontlist.h"
#include "fontcl.h"
#include "panmap.h"
#include "cpanel.h"
#include "strtab.h"

#include "dbutl.h"
#include "resource.h"

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

#if 1
#define ECS    EnterCriticalSection( &m_cs )
#define LCS    LeaveCriticalSection( &m_cs )
#else
#define ECS
#define LCS
#endif

#ifdef WINNT
static TCHAR s_szKey1[] = TEXT( "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts" );
static TCHAR s_szKeyFontDrivers[] = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Font Drivers");
#else
static TCHAR s_szKey1[] = TEXT( "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Fonts" );
#endif  // WINNT

static TCHAR s_szKey2[] = TEXT( "Display\\Fonts" );
static TCHAR s_szINISFonts[] = TEXT( "fonts" );


#if 1
/***************************************************************************
 * METHOD:  bKeyHasKey
 *
 * PURPOSE: Determine if the key exists in the registry.
 *
 * RETURNS: the number of bytes written into the buffer.
 *
 ***************************************************************************/

BOOL bKeyHasKey( HKEY          hk,
                 const TCHAR * pszKey,
                 TCHAR       * pszValue,
                 int           iValLen )
{
    DWORD  i;
    TCHAR  szKey[ 80 ];
    DWORD  dwKey,
           dwValue,
           dwKeyLen;
    DWORD  dwErr;
    int    iFound = 0;

    dwKeyLen = lstrlen( pszKey );

    i = 0;

    while( TRUE )
    {
        dwKey   = ARRAYSIZE( szKey );

        dwValue = iValLen * sizeof( TCHAR );

        dwErr = RegEnumValue( hk, i, szKey, &dwKey, NULL,
                              NULL, (LPBYTE)pszValue, &dwValue );

        if( dwErr == ERROR_NO_MORE_ITEMS )
            break;
        else if( dwErr == ERROR_SUCCESS )
        {
            //
            //  Null terminate it.
            //

            szKey[ dwKey ] = 0;

            //
            //  Check to see if this is the one we want.
            //

            if( dwKey == dwKeyLen )
            {
                if( ( iFound = !lstrcmpi( szKey, pszKey ) ) )
                    break;
            }
        }

        //
        //  Move on to the next one.
        //

        i++;
    }

    return (int)iFound;
}


BOOL bRegHasKey( const TCHAR * pszKey, TCHAR * pszValue = NULL, int iValLen = 0 );


BOOL bRegHasKey( const TCHAR * pszKey, TCHAR * pszValue, int iValLen )
{
    HKEY  hk;
    BOOL  bHasKey = FALSE;
    FullPathName_t szPath;

    if( !pszValue )
    {
        pszValue = szPath;
        iValLen  = ARRAYSIZE( szPath );
    }

    //
    //  Check standard 'fonts' registry list to see if font is
    //  already installed.
    //

    if( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE, s_szKey1, 0,
                                       KEY_READ, &hk ) )
    {
        bHasKey = bKeyHasKey( hk, pszKey, pszValue, iValLen );
        RegCloseKey( hk );
    }

#ifndef WINNT

    //
    // [stevecat] 7/10/95 NT does not yet support HKEY_CURRENT_CONFIG
    //                    and won't until Plug N Play fills in that
    //                    part of the registry.
    //

    if( !bHasKey )
    {
        if( ERROR_SUCCESS == RegOpenKeyEx( HKEY_CURRENT_CONFIG, s_szKey2,
                                           0, KEY_READ, &hk ) )
        {
            bHasKey = bKeyHasKey( hk, pszKey, pszValue, iValLen );
            RegCloseKey( hk );
        }
    }

#endif  //  WINNT

    //
    //  If we still don't have it, try from the WIN.INI file.
    //

    if( !bHasKey )
    {
        bHasKey = (BOOL) GetProfileString( s_szINISFonts, pszKey, TEXT( "" ),
                                           pszValue,
                                           iValLen );
    }

#ifdef LATER  // WINNT

    //
    //  Check 'Type 1' registry location to see if it is already installed.
    //

    if( !bHasKey )
    {
        if( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE, g_szType1Key,
                                           0, KEY_READ, &hk ) )
        {
            bHasKey = bKeyHasKey( hk, pszKey, pszValue, iValLen );

            RegCloseKey( hk );
        }
    }

#endif  //  LATER  WINNT

    return bHasKey;
}
#endif


/***************************************************************************
 * METHOD:
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 ***************************************************************************/

BOOL  WriteToRegistry( LPTSTR lpValue, LPTSTR lpData )
{
    HKEY  hk;
    LONG  lRet;


    if( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE, s_szKey1, 0,
                                       KEY_SET_VALUE, &hk ) )
    {
        if( lpData )
            lRet = RegSetValueEx( hk, lpValue, 0, REG_SZ,
                                  (const LPBYTE)lpData,
                                  (lstrlen( lpData ) + 1) * sizeof( TCHAR ) );
        else
        {
            lRet = RegDeleteValue( hk, lpValue );
        }

        RegCloseKey( hk );



#ifndef WINNT

    //
    // [stevecat] 7/10/95 NT does not yet support HKEY_CURRENT_CONFIG
    //                    and won't until Plug N Play fills in that
    //                    part of the registry.
    //

        //
        //  If we're deleting (lpData == 0 ), make sure the string is gone
        //  from the Win.ini and the other reg location.
        //

        if( !lpData && ( lRet != ERROR_SUCCESS ) )
        {
            if( ERROR_SUCCESS == RegOpenKeyEx( HKEY_CURRENT_CONFIG, s_szKey2,
                                               0, KEY_SET_VALUE, &hk ) )
            {
                lRet = RegDeleteValue( hk, lpValue );
                RegCloseKey( hk );
            }

        }

#endif  //  WINNT

        return( lRet == ERROR_SUCCESS );
    }

    return FALSE;
}


/***************************************************************************
 * METHOD:
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 ***************************************************************************/

CFontManager::CFontManager( )
   :  m_poFontList( 0 ),
      m_poTempList( 0 ),
      m_poPanMap( 0 ),
      m_bTriedOnce( FALSE ),
      m_bFamiliesNeverReset( TRUE ),
      m_poRemoveList( 0 ),
      m_hNotifyThread( 0 ),
      m_hReconcileThread( 0 ),
      m_hResetFamThread( 0 ),
      m_hMutexResetFamily( 0 ),
      m_hMutexInstallation( 0 ),
      m_hEventTerminateThreads(NULL)
{
    m_Notify.m_hWatch    = INVALID_HANDLE_VALUE;

    m_hMutexResetFamily = CreateMutex( NULL, FALSE, NULL );
    m_hEventResetFamily = CreateEvent( NULL, TRUE, FALSE, NULL );
    m_hMutexInstallation = CreateMutex( NULL, FALSE, NULL );
    m_hEventTerminateThreads  = CreateEvent(NULL, TRUE, FALSE, NULL);

    InitializeCriticalSection( &m_cs );
}


/***************************************************************************
 * METHOD:
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 ***************************************************************************/

CFontManager::~CFontManager( )
{
    //
    // Set the "terminate-all-threads" event.
    // Any threads in this module will recognize this event and 
    // immediately terminate their processing in a normal fashion.
    //
    // The order of these "SetEvent" calls is CRITICAL.  Must 
    // set the "terminate threads" event first.
    //
    if (NULL != m_hEventTerminateThreads)
    {
        SetEvent(m_hEventTerminateThreads);
    }
    if (NULL != m_hEventResetFamily)
    {
        SetEvent(m_hEventResetFamily);
    }        
    //
    // Wait for all active threads to terminate.
    //
    HANDLE rghThreads[3];
    int cThreads = 0;
    if (NULL != m_hNotifyThread)
        rghThreads[cThreads++] = m_hNotifyThread;
    if (NULL != m_hResetFamThread)
        rghThreads[cThreads++] = m_hResetFamThread;
    if (NULL != m_hReconcileThread)
        rghThreads[cThreads++] = m_hReconcileThread;

    DWORD dwWait = WaitForMultipleObjects(cThreads, rghThreads, TRUE, INFINITE);
        
    if( m_hNotifyThread )
    {
        CloseHandle( m_hNotifyThread );
    }
    if( m_hReconcileThread )
    {
        CloseHandle( m_hReconcileThread );
    }
    if( m_hResetFamThread )
    {
        CloseHandle( m_hResetFamThread );
    }

    ECS;
    if( m_Notify.m_hWatch != INVALID_HANDLE_VALUE )
    {
        if( !FindCloseChangeNotification( m_Notify.m_hWatch ) )
        {
            DEBUGMSG( (DM_ERROR, TEXT( "CFontManager:~CFontManager FindCloseChangeNotification FAILED - error = %d Handle = 0x%x" ), GetLastError( ), m_Notify.m_hWatch ) );
        }

        DEBUGMSG( (DM_TRACE2, TEXT( "CFontManager:~CFontManager FindCloseChangeNotification called" ) ) );
    }

    if( m_poFontList )
    {
        delete m_poFontList;
        m_poFontList = 0;
    }

    if( m_poPanMap )
    {
        m_poPanMap->Release( );
    }

    if( m_poTempList )
        delete m_poTempList;

    if( m_poRemoveList )
        delete m_poRemoveList;

    LCS;

    if (NULL != m_hMutexResetFamily)
        CloseHandle( m_hMutexResetFamily );

    if (NULL != m_hEventResetFamily)
        CloseHandle( m_hEventResetFamily );

    if (NULL != m_hMutexInstallation)
        CloseHandle( m_hMutexInstallation );

    if (NULL != m_hEventTerminateThreads)
        CloseHandle(m_hEventTerminateThreads);

    DeleteCriticalSection( &m_cs );
}



//
// GetOrReleaseFontManager
//
// Used by both GetFontManager and ReleaseFontManager.  I have both functions
// calling into this single function so we can control the scope of the 
// single CFontManager ptr and reference counter to a single function.
//
// Here's the scoop:
// The font folder uses a single CFontManager object.  The original code
// created it in the extinit code, destroyed it on process detach 
// and accessed it through a global pointer.  While not the best way
// to manage a singleton, this worked for Win9x and NT4.  In NT5 fontext.dll
// now implements an icon handler.  Therefore, fontext.dll is ALWAYS
// loaded in explorer.exe and the global font manager wasn't being destroyed
// until logoff because process-detach code in explorer.exe is only invoked 
// at logoff.
// 
// I added reference counting and centralized the access to the singleton
// font manager so that it's created on demand and destroyed when the last
// client is finished with it.  I made the CFontManager ctor private to
// enforce the use of the GetFontManager API.
// I also added code so that the manager's threads are now shut down in 
// an orderly fashion.  The original implementation merely called 
// TerminateThread() in the font manager's dtor (bad).  
// 
// [brianau - 6/5/99]
//
extern CRITICAL_SECTION g_csFontManager; // defined in fontext.cpp

HRESULT GetOrReleaseFontManager(CFontManager **ppoFontManager, bool bGet)
{
    static CFontManager *pSingleton;
    static LONG cRef = 0;
    
    HRESULT hr = NOERROR;
    EnterCriticalSection(&g_csFontManager);
    if (bGet)
    {
        if (NULL == pSingleton)
        {
            //
            // No manager exists.  Create it.
            //
            pSingleton = new CFontManager();
            if (NULL != pSingleton)
            {
                if (!pSingleton->bInit())
                {
                    delete pSingleton;
                    pSingleton = NULL;
                }
            }
        }
        if (NULL != pSingleton)
        {
            *ppoFontManager = pSingleton;
            cRef++;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        *ppoFontManager = NULL;
        if (0 == (--cRef))
        {
            //
            // Last reference to manager.
            // Delete it.
            //
            delete pSingleton;
            pSingleton = NULL;
        }
    }
    LeaveCriticalSection(&g_csFontManager);
    return hr;
}

HRESULT GetFontManager(CFontManager **ppoFontManager)
{
    return GetOrReleaseFontManager(ppoFontManager, true);
}

void ReleaseFontManager(CFontManager **ppoFontManager)
{
    GetOrReleaseFontManager(ppoFontManager, false);
}


/***************************************************************************
 * METHOD:
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 ***************************************************************************/
BOOL CFontManager::bInit( )
{
    //
    // Load up the font list and request to receive file system change
    // notifications so we can react to new files added to the fonts directory.
    //
    DWORD idThread;
    if ( bLoadFontList() )
    {
         FullPathName_t szPath;

         GetFontsDirectory( szPath, ARRAYSIZE( szPath ) );

#ifdef WINNT
         //
         // Is a loadable Type1 font driver installed?
         // Result can be retrieved through CFontManager::Type1FontDriverInstalled().
         //
         CheckForType1FontDriver();
#endif

         m_Notify.m_hWatch    = FindFirstChangeNotification( szPath, 0, FILE_NOTIFY_CHANGE_FILE_NAME );

         DEBUGMSG( (DM_TRACE2, TEXT( "CFontManager:bInit FindFirstChangeNotification called" ) ) );

         if( m_Notify.m_hWatch != INVALID_HANDLE_VALUE )
         {
              //
              //  Launch a Background thread to keep an eye on it.
              //

              FindNextChangeNotification( m_Notify.m_hWatch );

              DEBUGMSG( (DM_TRACE2, TEXT( "CFontManager:bInit FindNextChangeNotification called - creating Notify thread" ) ) );
              DEBUGMSG( (DM_TRACE2, TEXT( "CFontManager:bInit ChangeNotification Handle = 0x%x" ), m_Notify.m_hWatch ) );
              
              InterlockedIncrement(&g_cRefThisDll);
              m_hNotifyThread = CreateThread( NULL,
                                       0,
                                      (LPTHREAD_START_ROUTINE)dwNotifyWatchProc,
                                      (LPVOID)this,
                                       0,                  // CREATE_NO_WINDOW,
                                       &idThread);
              if (NULL == m_hNotifyThread)
              {
                  InterlockedDecrement(&g_cRefThisDll);
              }
         }
         //
         //  Launch a background process to reconcile any new font files
         //  that have been dropped in the FONTS folder.
         //  Only do this if font list is valid.  Reconciliation requires
         //  that the font list exists.
         //
         vReconcileFolder( THREAD_PRIORITY_LOWEST );
    }

    //
    //  Start a thread that resets the font family information. This thread
    //  is activated by pulsing the m_hEventResetFamily handle. It runs at
    //  the lowest priority unless the main (UI) thread is waiting on it.
    //
    InterlockedIncrement(&g_cRefThisDll);
    m_hResetFamThread = CreateThread(
                                  NULL,
                                  0,
                                  (LPTHREAD_START_ROUTINE)dwResetFamilyFlags,
                                  (LPVOID)this,
                                  0, // CREATE_NO_WINDOW,
                                  &idThread);

    if( m_hResetFamThread )
    {
        SetThreadPriority( m_hResetFamThread, THREAD_PRIORITY_LOWEST );
    }
    else
    {
        InterlockedDecrement(&g_cRefThisDll);
    }

    //
    //  Even if the notify doesn't work, return OK.
    //

    return TRUE;
}


static DWORD dwReconcileThread(LPVOID pvParams)
{
    CFontManager* pFontManager = (CFontManager *)pvParams;
    if (NULL != pFontManager)
    {
        pFontManager->vDoReconcileFolder();
    }
    InterlockedDecrement(&g_cRefThisDll);
    return 0;
}


VOID CFontManager::vReconcileFolder( int iPriority )
{
    DWORD idThread;
    
    ECS;

    //
    //  If one is running reset its priority and return.
    //

    if( m_hReconcileThread )
    {
        SetThreadPriority( m_hReconcileThread, iPriority );
        LCS;
        return;
    }

    //
    //  Always do this in the background, if possible.
    //
    InterlockedIncrement(&g_cRefThisDll);
    m_hReconcileThread = CreateThread(
                                    NULL,
                                    0,
                                    (LPTHREAD_START_ROUTINE) dwReconcileThread,
                                    (LPVOID) this,
                                    0,              // CREATE_NO_WINDOW,
                                    &idThread);
    LCS;

    //
    //  At statup we want this to idle in the backgound. Most other times it
    //  runs at normal priority.
    //

    if( m_hReconcileThread )
    {
        SetThreadPriority( m_hReconcileThread, iPriority );
    }
    else
    {
        InterlockedDecrement(&g_cRefThisDll);
        vDoReconcileFolder( );
    }
}


BOOL bValidFOTFile( LPTSTR szFull, LPTSTR szLHS, BOOL *pbTrueType, LPDWORD lpdwStatus = NULL)
{
    LPCTSTR pszExt = PathFindExtension( szFull );

    //
    // Initialize status return.
    //
    if (NULL != lpdwStatus)
       *lpdwStatus = FVS_MAKE_CODE(FVS_INVALID_STATUS, FVS_FILE_UNK);


    if( !pszExt || lstrcmpi( pszExt, TEXT( ".fot" ) ) != 0 )
    {
        if (NULL != lpdwStatus)
            *lpdwStatus = FVS_MAKE_CODE(FVS_INVALID_FONTFILE, FVS_FILE_UNK);

        return( FALSE );
    }

    FontDesc_t szDesc;

    WORD wType;


    if( !::bCPValidFontFile( szFull, szDesc, &wType, TRUE, lpdwStatus ) )
    {
        return( FALSE );
    }

    *pbTrueType = TRUE;

    wsprintf( szLHS, c_szDescFormat, (LPTSTR) szDesc, c_szTrueType );

    return( TRUE );
}


VOID CFontManager::vDoReconcileFolder( )
{
    HANDLE            hSearch;
    WIN32_FIND_DATA   fData;
    FullPathName_t    szPath;
    FullPathName_t    szWD;
    BOOL              bAdded = FALSE;
    BOOL              bChangeNotifyRequired = FALSE;

#define WANT_DORECONCILEFOLDER
#ifdef  WANT_DORECONCILEFOLDER

    //
    // Load the list of hidden font file names from FONT.INF.
    // This initialization is done here on the background thread
    // so we don't steal cycles from the UI.
    //
    m_HiddenFontFilesList.Initialize();

    GetFontsDirectory( szPath, ARRAYSIZE( szPath ) );

    lstrcat( szPath, TEXT( "\\*.*" ) );

    //
    //  Process each file in the directory.
    //
    //  Reasons for getting here:
    //   - File was installed and the CFontFolder needs to be updated.
    //   - File was copied into Fonts folder, but not installed.
    //

    ///////////////////////////////////////////////////////////////////
    //
    // BUGBUG: [brianau]
    //
    // There was a condition in the original font folder where not all
    // fonts in a set of manually-added fonts would be installed by the
    // reconciliation process.  Given the following scenario and invariants:
    //
    // Scenario:
    //    Installing a large set of fonts manually with COPY command while
    //    installing a set of fonts through the font folder interface.
    //
    // Invariants:
    //    In response to a file sys change notification, the watch thread
    //    either starts the reconcile thread (if not active) or sets its
    //    priority if already active.
    //    The reconcile loop uses the FindFirstFile/FindNextFile functions
    //    to obtain names of files in the fonts directory.  FindNextFile
    //    returns information on the next file it encounters in the
    //    directory in alphabetical name order.
    //
    // The Problem:
    //    If a font file is being added to the fonts directory while the
    //    FindFirst/FindNext loop is active and it's name is lexically
    //    "less than" the file currently being returned by FindNextFile,
    //    it will be skipped by the FindFirst/FindNext processing and
    //    will not be installed.  Since the file sys notifications arrive
    //    in rapid succession while the reconcile loop is active,  the
    //    loop is never re-started (only thread priority is set).  Therefore,
    //    these missed fonts are never added.  However, they will be
    //    installed the next time the reconcile thread is started.
    //
    // Possible solutions:
    //    Replace the reconcile loop with a single installation function
    //    that is called each time a notification is received via
    //    ReadDirectoryChanges().  ReadDirectoryChangesW() is a new NT
    //    API similar to FindFirstChangeNotification/FindNextChangeNotification
    //    except it returns detailed information about the file that
    //    triggered the change notification.  This would eliminate the
    //    need for scanning the entire directory.
    //
    //    I added the existing "hack" of repeating the loop until no more
    //    fonts can be added.  It's not as clean as the ReadDirectoryChanges()
    //    fix but it works with the existing code.  Since we're on a
    //    background thread, the user won't notice the extra iterations.
    //
    //    I think we should replace this hack with the ReadDirectoryChanges
    //    solution when possible.
    ///////////////////////////////////////////////////////////////////
    do {
        bAdded = FALSE;

        hSearch = FindFirstFile( szPath, &fData );

        if( hSearch != INVALID_HANDLE_VALUE )
        {
            GetFontsDirectory( szWD, ARRAYSIZE( szWD ) );

            //
            //  We can't use Get/SetCurrent directory because we might
            //  hose the main UI thread. Most notably the Common File
            //  Dialog.
            //
            //  GetCurrentDirectory( ARRAYSIZE( szCD ), szCD );
            //  SetCurrentDirectory( szWD );
            //
            do
            {
                //
                // poSearchFontListFile needs a valid m_poFontList.
                //
                ASSERT(NULL != m_poFontList);

                //
                // Wait for mutex so we don't have multiple threads installing
                // fonts concurrently.  Aquire and release mutex for each font.
                // This will minimize blocking time for other waiting threads.
                // Since we're on a background thread, we don't mind waiting
                // a while for the mutex (5 seconds).
                //
                if ( MUTEXWAIT_SUCCESS == dwWaitForInstallationMutex(5000) )
                {
                    //
                    // See if the Font Manager knows about this font. If not,
                    // then install it.
                    //
                    if( fData.cFileName[ 0 ] != TEXT( '.' ) &&
                         !poSearchFontListFile( fData.cFileName ) &&
                         ShouldAutoInstallFile( fData.cFileName, fData.dwFileAttributes ) )
                    {

                        FullPathName_t szFull;
                        FontDesc_t     szLHS;
                        BOOL           bTrueType;
                        WORD           wType;

                        // GetFullPathName( fData.cFileName, ARRAYSIZE( szFull ), szFull, &lpTemp );

                        lstrcpy( szFull, szWD );

                        lpCPBackSlashTerm( szFull );

                        lstrcat( szFull, fData.cFileName );

                        //
                        //  Check to see if this is a valid font file.
                        //  Don't call CPDropInstall() 'cause we don't want any UI
                        //  coming up.
                        //
                        //  bCPDropInstall( m_poFontMan, szFull );
                        //

                        if( ::bCPValidFontFile( szFull, szLHS, &wType )
                               || bValidFOTFile( szFull, szLHS, &bTrueType ) )
                        {
                            //
                            //  Make sure it's not already in the registry, possibly
                            //  from another file
                            //

                            if(wType == TYPE1_FONT)
                            {
                                //
                                //  Check registry font entries under the
                                //  Type 1 Installer
                                //

                                if( !CheckT1Install( szLHS, NULL ) )
                                {
                                    FullPathName_t szPfbFile;

                                    if( IsPSFont( szFull, (LPTSTR) NULL, (LPTSTR) NULL,
                                                  szPfbFile, (BOOL *) NULL) )
                                    {
#ifdef WINNT
                                        if (Type1FontDriverInstalled())
                                        {
                                            TCHAR szType1FontResourceName[MAX_TYPE1_FONT_RESOURCE];

                                            if (BuildType1FontResourceName(
                                                    szFull,
                                                    szPfbFile,
                                                    szType1FontResourceName,
                                                    ARRAYSIZE(szType1FontResourceName)))
                                            {
                                                AddFontResource(szType1FontResourceName);
                                            }
                                        }
#endif // WINNT
                                        if( WriteType1RegistryEntry( NULL, szLHS, szFull,
                                                                     szPfbFile, TRUE ) )
                                        {
                                            bAdded = TRUE;
                                        }
                                    }
                                }
                            }
                            else if( !bRegHasKey( szLHS ) )
                            {
                                if( AddFontResource( fData.cFileName ) )
                                {
                                    if (WriteToRegistry( szLHS, fData.cFileName ))
                                        bAdded = TRUE;
                                    else
                                        RemoveFontResource(fData.cFileName);
                                }
                            }
                        }

                        //
                        //  TODO. Should we remove the file if it isn't a font file or
                        //  shouldn't be in this directory?
                        //
                        //  [stevecat] DO NOT delete extraneous files from this dir
                        //   on WINNT because we recognize .PFM files as the main
                        //   Type 1 file but the matching .PFB file may also be present
                        //   for use by the Postscript printer driver.
                        //
                    }
                    //
                    // Let some other thread install a font.
                    // WARNING:  Don't miss this call with an early return.
                    //           (break, goto, return)
                    //
                    bReleaseInstallationMutex();
               }
               else
               {
                    //
                    // I have yet to see this thread not get the mutex.
                    // But, just in case it doesn't, give up on installing
                    // this font.
                    // Note that we don't inform the user since this is a background
                    // thread that the user isn't aware of.
                    //
               }

           } while( FindNextFile( hSearch, &fData ) );

           // SetCurrentDirectory( szCD );

           FindClose( hSearch );
        }
        //
        // We need to post a font change notification if any fonts
        // have been added.
        //
        bChangeNotifyRequired = bChangeNotifyRequired || bAdded;
    } while (TRUE == bAdded);

#endif

    //
    //  For all practical purposes, we're done.
    //

    ECS;

    if( m_hReconcileThread )
    {
        CloseHandle( m_hReconcileThread );
        m_hReconcileThread = 0;
    }

    LCS;
    
    //
    // Destroy contents of hidden files list.
    // Folder reconciliation is the only time this table is used so
    // we don't need to keep the strings in memory when they're not
    // needed.
    // It will be re-created next time vDoReconcileFolder is called.
    //
    m_HiddenFontFilesList.Destroy();

    if( bChangeNotifyRequired )
        vCPWinIniFontChange( );
}


/***************************************************************************
 * METHOD:
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 ***************************************************************************/
VOID CFontManager::ProcessRegKey( HKEY hk, BOOL bCheckDup )
{

    DWORD          i;
    int            idx;
    FontDesc_t     szValue;
    DWORD          dwValue;
    FullPathName_t szData;
    DWORD          dwData;


    for( i = 0; ; ++i )
    {
        dwValue = sizeof( szValue );
        dwData  = sizeof( szData );

        LONG lRet = RegEnumValue( hk, i, szValue, &dwValue,
                                  NULL, NULL, (LPBYTE) szData, &dwData );

        if( lRet == ERROR_MORE_DATA )
        {
            //
            //  I guess I'm just going to skip this guy.
            //  It's mostly invalid anyway
            //

            continue;
        }
        else if (lRet != ERROR_SUCCESS)
        {
            //
            //  I assume this is ERROR_NO_MORE_ITEMS
            //

            break;
        }

        if( szValue[ 0 ] )
        {

            if( bCheckDup )
            {
                if( ( idx = iSearchFontListLHS( szValue ) ) >= 0 )
                {
                    if( m_poTempList )
                    {
                        CFontClass *poFont = m_poFontList->poDetach(idx);
                        m_poTempList->bAdd(poFont);
                        poFont->Release();  // Release from m_poFontList.
                    }


                    continue;
                }
            }

            poAddToList( szValue, szData );
        }
    }
}



#ifdef WINNT
/***************************************************************************
 * METHOD:  ProcessT1RegKey
 *
 * PURPOSE: Crack REG_MULTISZ Type registry value entry and look for dups
 *
 * RETURNS:
 *
 ***************************************************************************/

VOID CFontManager::ProcessT1RegKey( HKEY hk, BOOL bCheckDup )
{
    DWORD          i = 0;
    int            idx;
    TCHAR          szValue[ PATHMAX ];
    DWORD          dwValue = ARRAYSIZE( szValue );
    TCHAR          szData[ 2 * PATHMAX + 10 ];
    DWORD          dwData = sizeof( szData );
    FullPathName_t szPfmFile;
    FullPathName_t szPfbFile;


    while( ERROR_SUCCESS == RegEnumValue( hk, i, szValue, &dwValue, NULL,
                                          NULL, (LPBYTE)szData, &dwData ) )
    {
        if( szValue[ 0 ] )
        {
            if( bCheckDup )
            {
                if( ( idx = iSearchFontListLHS( szValue ) ) >= 0 )
                {
                    if( m_poTempList )
                    {
                        CFontClass *poFont = m_poFontList->poDetach(idx);
                        m_poTempList->bAdd(poFont);
                        poFont->Release();  // Release from m_poFontList.
                    }

                    goto MoveOn;
                }
            }

            //
            //  For Type 1 font entries, extract PFM and PFB font file
            //  names for storage in the class object.
            //

            if( ::ExtractT1Files( szData, szPfmFile, szPfbFile ) )
                poAddToList( szValue, szPfmFile, szPfbFile );
        }
MoveOn:
        //
        //  Move on to the next one.
        //

        dwValue = ARRAYSIZE( szValue );
        dwData  = sizeof( szData );
        i++;
    }
}

#endif  //  WINNT

/***************************************************************************
 * METHOD:  bRefresh
 *
 * PURPOSE: Re-read the win.ini and registry to determine if fonts have
 *          been added.
 *
 *          If we're checking for dups, move them to m_poTempList as we
 *          find them. Then move them back when we're all done.
 *
 * RETURNS:
 *
 ***************************************************************************/
BOOL CFontManager::bRefresh( BOOL bCheckDup )
{
    //
    //  Load the Font List.
    //

    static BOOL  s_bInRefresh = FALSE;
    TCHAR        szFonts[] = TEXT( "FONTS" );
    PTSTR        pszItem;                          // pointer into buffer
    PATHNAME     szPath;
    HANDLE       hLocalBuf;
    PTSTR        pLocalBuf, pEnd;
    DWORD        nCount;
    int          idx;

    //
    //  Don't reenter this puppy. If we're already in here (either this thread
    //  or another) the caller won't be able to get the list until it's
    //  available in a consistent state.
    //

    if( s_bInRefresh )
        return TRUE;

    s_bInRefresh = TRUE;

    //
    //  If we're checking for dups, then allocate the temp list to put them
    //  in. If we can't allocate one, just don't use it.
    //

    if( bCheckDup && !m_poTempList )
    {
        m_poTempList = new CFontList( m_poFontList->iCount( ) );

        if( !(m_poTempList && m_poTempList->bInit( ) ) )
            m_poTempList = 0;
    }

    //
    //  Process the WIN.INI file first.
    //

    nCount = GetSection( NULL, szFonts, &hLocalBuf );

    if( !hLocalBuf )
    {
        iUIErrMemDlg(NULL);
        s_bInRefresh = FALSE;

        return FALSE;
    }

    ECS;

    pLocalBuf = (PTSTR) LocalLock( hLocalBuf );

    pEnd = pLocalBuf + nCount;

    //
    //  Add all the fonts in the list, if they haven't been added already
    //

    for( pszItem = pLocalBuf; pszItem < pEnd; pszItem += lstrlen( pszItem )+1 )
    {
        if( !*pszItem )
            continue;

        if( bCheckDup )
        {
            if( ( idx = iSearchFontListLHS( pszItem ) ) >= 0 )
            {
                if( m_poTempList )
                {
                    CFontClass *poFont = m_poFontList->poDetach(idx);
                    m_poTempList->bAdd(poFont);
                    poFont->Release(); // Release from m_poFontList.
                }
                continue;
            }
        }

        GetProfileString( szFonts, pszItem, TEXT( "" ), szPath,
                          ARRAYSIZE( szPath ) );

        //
        //  there's a RHS here
        //

        if( *szPath )
        {
            poAddToList( pszItem, szPath, NULL );
        }
    }

    LocalUnlock( hLocalBuf );

    LocalFree  (hLocalBuf );

    //
    //  Now, process the entries in the Registry. There are two locations:
    //  one that holds the display fonts and one that holds the TT fonts.
    //

    HKEY     hk;


#ifndef WINNT

    //
    // [stevecat] 7/10/95 NT does not yet support HKEY_CURRENT_CONFIG
    //                    and won't until Plug N Play fills in that
    //                    part of the registry.
    //

    if( ERROR_SUCCESS == RegOpenKeyEx( HKEY_CURRENT_CONFIG, s_szKey2,
                                       0, KEY_READ, &hk ) )
    {
        ProcessRegKey( hk, bCheckDup );
        RegCloseKey( hk );
    }


    if( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE, s_szKey1, 0,
                                       KEY_READ, &hk ) )
    {
//
// [stevecat] Test code to remove font duplications in FontView list
//

#ifdef WINNT
        //
        //  Since WinNT performs .INI file mapping we must always
        //  set the "Check duplicates" flag or else we will get a
        //  double display of list view items.
        //

//
//  This doesn't seem to be working because the CheckDup code is seeing
//  the source dirs of the fonts as different if they come from Win.ini
//  verses the Registry.
//

        ProcessRegKey( hk, TRUE );

#else  //  WINNT

        ProcessRegKey( hk, bCheckDup );

#endif  //  WINNT

        RegCloseKey( hk );
    }

#endif  // ifndef WINNT


#ifdef WINNT
    //
    //  Process Type 1 fonts registry location
    //

    if( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE, g_szType1Key, 0,
                                       KEY_READ, &hk ) )
    {
        ProcessT1RegKey( hk, bCheckDup );
        RegCloseKey( hk );
    }
#endif  //  WINNT

    //
    //  If we put some things in m_poTempList, put them back in the main list.
    //

    if( m_poTempList )
    {
        int iCount = m_poTempList->iCount( );
        int i;

        for( i = iCount - 1; i >= 0; i-- )
        {
            CFontClass *poFont = m_poTempList->poDetach(i);
            m_poFontList->bAdd(poFont);
            poFont->Release(); // Release from m_poTempList.
        }
    }

    LCS;

    //
    //  Reset the family connections.
    //

    vResetFamilyFlags( );

    s_bInRefresh = FALSE;

    return TRUE;
}


/*************************************************************************
 * METHOD:  vToBeRemoved
 *
 * PURPOSE: Set the TBR list to poList. The font manager owns the list at
 *          this point. If there is already a TBR list, then this one is
 *          merged with the current one and deleted.
 *
 * RETURNS: Nothing.
 *************************************************************************/

void CFontManager::vToBeRemoved( CFontList * poList )
{
    int   iCount;
    int   i;

    CFontClass * poFont;

    //
    //  Quick check for valid pointer.
    //
    if( !poList )
       return;

    ECS;

    //
    //  Make sure the objects aren't in the main list. Merge into the current
    //  list, if there is one, simultaneously.
    //

    iCount = poList->iCount( );

    if( !iCount )
       goto done;

    for( i = 0; i < iCount; i++ )
    {
        poFont = poList->poObjectAt( i );

        if (NULL != m_poFontList->poDetach( poFont ))
            poFont->Release();

        if( m_poRemoveList )
            m_poRemoveList->bAdd( poFont );
    }

    if( !m_poRemoveList )
        m_poRemoveList = poList;
    else
        delete poList;

done:
    LCS;
}


/*************************************************************************
 * METHOD:  bCheckTBR( )
 *
 * PURPOSE: Check the To Be Removed list. Any files that no longer exist
 *          will be uninstalled.
 *
 * RETURNS: TRUE if something was removed.
 *************************************************************************/
BOOL  CFontManager::bCheckTBR( )
{
    int            iCount,
                   i;
    FullPathName_t szPath;
    BOOL           bRet = FALSE;
    CFontClass * poFont;

    //
    //  Quick return.
    //

    if( !m_poRemoveList )
        return bRet;

    ECS;

    //
    //  Walk the list and check for files that no longer exist and remove them.
    //

    iCount = m_poRemoveList->iCount( );
 
    for( i = iCount - 1; i >= 0; i-- )
    {
        poFont = m_poRemoveList->poObjectAt( i );

        poFont->bGetFQName( szPath, ARRAYSIZE( szPath ) );

        if( GetFileAttributes( szPath ) == 0xffffffff )
        {
            m_poRemoveList->poDetach( i );

            //
            //  Make sure it is no longer in the registry.
            //

            poFont->bRFR( );

            vDeleteFont( poFont, FALSE );

            bRet = TRUE;
       }
    }

    //
    //  If there's nothing left in here, delete the list.
    //

    if(  !m_poRemoveList->iCount( ) )
    {
        delete m_poRemoveList;
        m_poRemoveList = 0;
    }

    LCS;

    //
    //  Notify everyone that the font world has changed.
    //

    if( !m_poRemoveList )
        vCPWinIniFontChange( );

    return bRet;
}


/*************************************************************************
 * METHOD:  vUndoTBR( )
 *
 * PURPOSE: Undo the To Be Removed list. This usually happens when something
 *          has gone wrong with a delete operation.
 *
 * RETURNS: Nothing.
 *************************************************************************/

void CFontManager::vUndoTBR( )
{
    int   iCount;
    int   i;
    CFontClass *   poFont;

    //
    //  Try once more and quick return.
    //

    bCheckTBR( );

    if( !m_poRemoveList )
       return;

    ECS;

    //
    //  Put anything in the list back into the main list.
    //

    if( m_poRemoveList )
    {
        iCount = m_poRemoveList->iCount( );

        for( i = (iCount-1); i >= 0; i-- )
        {
            poFont = m_poRemoveList->poObjectAt( i );

            //
            //  If we can add the item back into GDI, do so
            //

            if( poFont->bAFR( ) )
            {
                m_poRemoveList->poDetach( i );
                m_poFontList->bAdd( poFont );
            }
        }

        //
        //  Delete the list.
        //

        if( !m_poRemoveList->iCount( ) )
        {
            delete m_poRemoveList;
            m_poRemoveList = 0;
        }
    }

    LCS;

    //
    //  Notify everyone that the font world has changed.
    //

    if( !m_poRemoveList )
        vCPWinIniFontChange( );

}


/*************************************************************************
 * METHOD:  vVerifyList( )
 *
 * PURPOSE: Validate the entries in the list.
 *
 * RETURNS: Nothing.
 *************************************************************************/

void CFontManager::vVerifyList( )
{
    int            iCount,
                   i;
    CFontList    * poList = 0;
    FullPathName_t szPath;
    CFontClass   * poFont;

    //
    //  Quick return;
    //

    if( !m_poFontList )
        return;

    ECS;

    //
    //  Walk the list and any files that don't reference valid files.
    //

    iCount = m_poFontList->iCount( );

    for( i = iCount - 1; i >= 0; i--)
    {
        poFont = m_poFontList->poObjectAt( i );

        //
        //  Only look at something that is in the FONTS folder or on the
        //  same drive( TODO ) as the Windows directory.
        //

        if( poFont->bOnSysDir( ) )
        {
            poFont->bGetFQName( szPath, ARRAYSIZE( szPath ) );

            if( GetFileAttributes( szPath ) == 0xffffffff )
            {
                //
                //  Allocate the list if necessary.
                //

                if( !poList )
                {
                    poList = new CFontList( 50 );
                    if (poList)
                    {
                        if (!poList->bInit())
                        {
                            delete poList;
                            poList = NULL;
                        }
                    }
                }
                if (!poList)
                    break;

                poList->bAdd( poFont );
                poFont = m_poFontList->poDetach( i );
                if (NULL != poFont)
                    poFont->Release();
            }
        }
    }

    //
    //  Set the list up to be removed. This will happen in a background
    //  thread.
    //

    if( poList )
       vToBeRemoved( poList );

    LCS;
}


/***************************************************************************
 * METHOD:
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 ***************************************************************************/

void CFontManager::vGetFamily( CFontClass * lpFontRec, CFontList * poList )
{
    int   iCount = m_poFontList->iCount( );
    WORD  wFam   = lpFontRec->wGetFamIndex( );

    while( iCount--)
    {
       lpFontRec = m_poFontList->poObjectAt( iCount );

       if( lpFontRec && ( wFam == lpFontRec->wGetFamIndex( ) ) )
          poList->bAdd( lpFontRec );
    }
}


/***************************************************************************
 * METHOD:
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 ***************************************************************************/

BOOL CFontManager::bLoadFontList( )
{
    BOOL  bRet = FALSE;

    ECS;

    if( m_poFontList )
    {
        bRet = TRUE;
        goto done;
    }

    //
    //  Allocate a list for 220 fonts (with the default bucket size) and
    //  64 directory entries.
    //

    m_poFontList = new CFontList( 220 );

    if( !m_poFontList )
    {
        goto done;
    }

    //
    //  Initialize them. The bInit() function will delete them if it fails.
    //

    if( !m_poFontList->bInit( ) )
    {
        m_poFontList = 0;
        goto done;
    }

    bRet = bRefresh( FALSE );

    //
    //  Verify the list.
    //

    vVerifyList( );

done:
    LCS;

    return bRet;
}


/***************************************************************************
 * METHOD:
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 ***************************************************************************/

CFontClass * CFontManager::poAddToList( LPTSTR lpDesc,      //  Font desc
                                        LPTSTR lpPath,      //  Primary font file
                                        LPTSTR lpCompFile ) //  Companion font file
{
    BOOL         bSuccess = FALSE;
    CFontClass * poFont   = new CFontClass;


    if( !poFont )
       return 0;

    if( bSuccess = poFont->bInit( lpDesc, lpPath, lpCompFile ) )
    {
        ECS;

        bSuccess = m_poFontList->bAdd( poFont );

        LCS;

        if( !bSuccess )
            delete poFont;
    }

    if(  !bSuccess )
    {
        poFont = NULL;
    }

    return poFont;
}


/***************************************************************************
 * METHOD:
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 ***************************************************************************/

CFontList * CFontManager::poLockFontList( )
{
    if( bLoadFontList( ) )
    {
       DEBUGMSG( (DM_MESSAGE_TRACE2, TEXT( "CFontManager: EnterCriticalSection in poLockFontList()  " ) ) );

       ECS;

       return m_poFontList;
    }

    return 0;
}


void CFontManager::vReleaseFontList( )
{
    LCS;
    DEBUGMSG( (DM_MESSAGE_TRACE2, TEXT( "CFontManager: LeaveCriticalSection in vReleaseFontList()  " ) ) );
}


/***************************************************************************
 * METHOD:
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 ***************************************************************************/

int CFontManager::GetSection( LPTSTR lpFile, LPTSTR lpSection, LPHANDLE hSection )
{
    ULONG  nCount;
    ULONG  nSize;
    HANDLE hLocal, hTemp;
    TCHAR *pszSect;


    if( !(hLocal = LocalAlloc( LMEM_MOVEABLE, nSize = 8192 ) ) )
        return( 0 );

    //
    //  Now that a buffer exists, Enumerate all LHS of the section.  If the
    //  buffer overflows, reallocate it and try again.
    //

    do
    {
        pszSect = (PTSTR) LocalLock( hLocal );

        if( lpFile )
            nCount = GetPrivateProfileString( lpSection, NULL, TEXT( "" ),
                                              pszSect, nSize / sizeof( TCHAR ),
                                              lpFile );
        else
            nCount = GetProfileString( lpSection, NULL, TEXT( "" ), pszSect,
                                       nSize / sizeof( TCHAR ) );

        LocalUnlock( hLocal );

        if( nCount != ( nSize / sizeof( TCHAR ) ) - 2 )
            break;

        nSize += 4096;

        if( !(hLocal = LocalReAlloc( hTemp = hLocal, nSize, LMEM_MOVEABLE ) ) )
        {
            LocalFree( hTemp );
            return( 0 );
        }
    } while( 1 ) ;

    *hSection = hLocal;

    return( nCount );
}


/***************************************************************************
 * METHOD:  GetFontsDirectory
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 ***************************************************************************/

int CFontManager::GetFontsDirectory( LPTSTR lpDir, int iLen )
{
    return ::GetFontsDirectory( lpDir, iLen );
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION: CFontManager::dwWaitForInstallationMutex
//
//  DESCRIP: Block until installation mutex is available.
//           Thread input messages are handled during wait.
//
//ARGUMENTS: dwTimeout
//              Number of milliseconds to wait for mutex.
//              The default is 2,000 ( 2 seconds ).
//
//  RETURNS: MUTEXWAIT_SUCCESS  = Obtained mutex either via release or abandonment.
//           MUTEXWAIT_WMQUIT   = Received a WM_QUIT message while waiting.
//                                  Caller should automatically cancel operation.
//           MUTEXWAIT_TIMEOUT  = Wait timed out.
//                                  Caller can retry or cancel operation.
//           MUTEXWAIT_FAILED   = Wait failure.
//                                  Shouldn't happen.  Caller should retry/cancel.
//////////////////////////////////////////////////////////////////////////////
DWORD CFontManager::dwWaitForInstallationMutex(DWORD dwTimeout)
{
    DWORD dwWaitResult = 0;                 // Wait result.
    DWORD dwResult     = MUTEXWAIT_SUCCESS; // Return code.

    if (NULL != m_hMutexInstallation)
    {
        //
        // Repeat this loop until one of the following occurs:
        //    1. We aquire the installation mutex.
        //    2. Mutex is abandoned by owner thread.
        //    3. Mutex wait times out.
        //    4. Mutex wait results in error.
        //    5. Receive a WM_QUIT message while waiting.
        //
        do
        {
            //
            // Note:  Don't handle posted messages.  The folder posts an IDM_IDLE message
            //        to the font install dialog every 2 seconds for filling in the font
            //        name list.  This message will satisfy the wait and screw up the
            //        timeout processing.  The (~QS_POSTMESSAGE) prevents this.
            //
            dwWaitResult = MsgWaitForMultipleObjects(1,
                                                     &m_hMutexInstallation,
                                                     FALSE,
                                                     dwTimeout,
                                                     QS_ALLINPUT & (~QS_POSTMESSAGE));
            switch(dwWaitResult)
            {
                case WAIT_OBJECT_0 + 1:
                {
                    MSG msg ;
                    //
                    // Allow blocked thread to respond to sent messages.
                    //
                    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                    {
                        if ( WM_QUIT != msg.message )
                        {
                            TranslateMessage(&msg);
                            DispatchMessage(&msg);
                        }
                        else
                        {
                            dwResult     = MUTEXWAIT_WMQUIT;
                            dwWaitResult = WAIT_FAILED;
                        }
                    }
                    break;
                }

                case WAIT_OBJECT_0:
                case WAIT_ABANDONED_0:
                    DEBUGMSG((DM_TRACE1, TEXT("Thread 0x%08x: HAS install mutex."), GetCurrentThread()));
                    dwResult = MUTEXWAIT_SUCCESS;
                    break;

                case WAIT_TIMEOUT:
                    DEBUGMSG((DM_TRACE1, TEXT("Thread 0x%08X: TIMEOUT waiting for install mutex."), GetCurrentThread()));
                    dwResult = MUTEXWAIT_TIMEOUT;
                    break;

                case WAIT_FAILED:
                default:
                    dwResult = MUTEXWAIT_FAILED;
                    break;
            }
        } while( (WAIT_OBJECT_0 + 1) == dwWaitResult );
    }
    return dwResult;
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: CFontManager::bReleaseInstallationMutex
//
//  DESCRIP: Release ownership of the installation mutex.
//
//  RETURNS: TRUE  = Thread owned the mutex and it was released.
//           FALSE = Thread did not own the mutex or it couldn't be released.
//////////////////////////////////////////////////////////////////////////////
BOOL CFontManager::bReleaseInstallationMutex(void)
{
    BOOL bStatus = FALSE;

    if (NULL != m_hMutexInstallation)
    {
        DEBUGMSG((DM_TRACE1, TEXT("Thread 0x%08x: RELEASED install mutex."), GetCurrentThread()));
        bStatus = ReleaseMutex(m_hMutexInstallation);
    }

    return bStatus;
}


///////////////////////////////////////////////////////////////////////////////
//  NOTE: [brianau]
//
//  The following functions iSuspendNotify() and iResumeNotify() have been
//  superceded by dwWaitForInstallationMutex() and bReleaseInstallationMutex().
//  The original intent of the Suspend and Resume functions was to prevent
//  problems between the main folder thread and the folder reconciliation thread
//  installing fonts concurrently.  However, merely suspending the file system
//  notification watch thread is not sufficient to prevent concurrency problems.
//  Because of file-scope and global data used in the font folder and Type 1
//  installer code, the installation code path must be treated like a critical
//  section and guarded by a thread synchronization object such as a Mutex.
//  The functions dwWaitForInstallationMutex() and bReleaseInstallationMutex()
//  provide ownership control of this synchronization object.
//
//  The original code has been retained for documentation purposes.
//
///////////////////////////////////////////////////////////////////////////////
#if 0

int CFontManager::iSuspendNotify( )
{
    DEBUGMSG( (DM_MESSAGE_TRACE2, TEXT( "CFontManager: iSuspendNotify called" ) ) );

    if( !m_iSuspendNotify )
    {
        DEBUGMSG( (DM_MESSAGE_TRACE2, TEXT( "CFontManager: iSuspendNotify thread being suspended" ) ) );
        SuspendThread( m_hNotifyThread );
    }

    m_iSuspendNotify++;

    return m_iSuspendNotify;
}


int CFontManager::iResumeNotify( )
{
    DEBUGMSG( (DM_MESSAGE_TRACE2, TEXT( "CFontManager: iResumeNotify called" ) ) );

    if( m_iSuspendNotify )
        m_iSuspendNotify--;

    if( !m_iSuspendNotify )
    {
        DEBUGMSG( (DM_MESSAGE_TRACE2, TEXT( "CFontManager: iResumeNotify thread being resumed" ) ) );
        ResumeThread( m_hNotifyThread );
    }

    return m_iSuspendNotify;
}

#endif

//
// Wait until the family-reset mutex is available.  Also returns
// if the "terminate-threads" event is set, meaning that it's time
// to shut down the folder.
// Returns:
//    TRUE  = Obtained the mutex.  OK to proceed.
//    FALSE = Terminate-threads was signaled.  We don't necessarily
//            own the mutex.  Don't proceed.  Return asap.
//
BOOL CFontManager::bWaitOnFamilyReset( )
{
    //
    //  Set the thread to Highest priority until we get the mutex,
    //  which means the thread is done.
    //
    DWORD dwWait;
    BOOL bResult = FALSE;
    BOOL bDone = FALSE;
    HANDLE rgHandles[] = { m_hEventTerminateThreads,
                           m_hMutexResetFamily };
                           
    if( m_hResetFamThread )
        SetThreadPriority( m_hResetFamThread, THREAD_PRIORITY_HIGHEST );

    //
    // This is called on the UI thread.  Must be able to handle
    // sent thread messages.
    //
    do
    {
        dwWait = MsgWaitForMultipleObjects(ARRAYSIZE(rgHandles),
                                           rgHandles,
                                           FALSE,
                                           INFINITE,
                                           QS_ALLINPUT & (~QS_POSTMESSAGE));
                                           
        if (WAIT_OBJECT_0 + ARRAYSIZE(rgHandles) == dwWait)
        {
            MSG msg ;
            //
            // Allow blocked thread to respond to sent messages.
            //
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if ( WM_QUIT != msg.message )
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                else
                {
                    dwWait = 0; // Rcvd WM_QUIT. Exit loop.
                }
            }
        }
        else if (0 == (dwWait - WAIT_OBJECT_0))
        {
            //
            // Terminate-threads event was set.
            //
            bDone = TRUE;
        }
        else if (1 == (dwWait - WAIT_OBJECT_0))
        {
            //
            // Got the mutex.
            //
            if( m_hResetFamThread )
                SetThreadPriority( m_hResetFamThread, THREAD_PRIORITY_LOWEST );
            
            bResult = TRUE;
            bDone   = TRUE;
        }            
    }
    while(!bDone);
    //
    // Always release the mutex before returning.
    // Even if the wait was satisfied by the "terminate" event this
    // will ensure we don't hold the mutex.  If we don't own it this call
    // will harmlessly fail.
    //
    ReleaseMutex( m_hMutexResetFamily );
    
    return bResult;
}


/***************************************************************************
 * METHOD:  dwResetFamilyFlags
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 ***************************************************************************/

DWORD dwResetFamilyFlags(LPVOID pvParams)
{
    CFontManager *pFontManager = (CFontManager *)pvParams;

    if (NULL != pFontManager)
    {
        BOOL bDone = FALSE;
        HANDLE rghObj[] = { pFontManager->m_hMutexResetFamily,
                            pFontManager->m_hEventResetFamily };

        while(!bDone)
        {
            //
            //  Wait for the FONTS folder to change. If we time out, then attempt
            //  to undo any deletions that might be occuring.
            //  Note that the CFontManager dtor will set both the reset-family
            //  event and the terminate-threads event.  This way we'll be sure
            //  to terminate this thread when it's time to shut down.
            //                      
            WaitForMultipleObjects(ARRAYSIZE(rghObj), 
                                   rghObj, 
                                   TRUE, 
                                   INFINITE);

            ResetEvent(pFontManager->m_hEventResetFamily);
            //
            // We got the OK to reset the family flags.  Now let's check
            // the "terminate-threads" event.  If it's set then it's time to
            // go home.
            //
            if (WAIT_OBJECT_0 == WaitForSingleObject(pFontManager->m_hEventTerminateThreads, 0))
            {
                bDone = true;
            }
            else
            {
                pFontManager->vDoResetFamilyFlags( );
            }
            //
            //  Release the mutex. The event was already reset by the
            //  PulseEvent
            //
            ReleaseMutex(pFontManager->m_hMutexResetFamily );
        }
    }

    InterlockedDecrement(&g_cRefThisDll);
    return 0;
}


/***************************************************************************
 * METHOD:  vResetFamilyFlags
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 ***************************************************************************/

void CFontManager::vResetFamilyFlags( )
{
    SetEvent( m_hEventResetFamily );
}


/***************************************************************************
 * METHOD:  vDoResetFamilyFlags
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 ***************************************************************************/

void CFontManager::vDoResetFamilyFlags( )
{
    /* static */ WORD   s_wIdx = 0;

    CFontClass * poFont;
    CFontClass * poFont2;

    if( !m_poFontList )
        return;


    ECS;

restart:
    int iCount = m_poFontList->iCount( );

    //
    //  It would be nice to walk the list and only set the values that aren't
    //  set to IDX_NULL. However, this doesn't work if the main font for
    //  a family is deleted.
    //

    for( int i = 0; i < iCount; i++ )
    {
        m_poFontList->poObjectAt( i )->vSetFamIndex( IDX_NULL );
    }

    //
    //  Release for a sec.
    //

    LCS;

    Sleep( 0 );

    ECS;

    iCount = m_poFontList->iCount( );

    for( i = 0; i < iCount; i++)
    {
        poFont = m_poFontList->poObjectAt( i );

        if( poFont->wGetFamIndex( ) == IDX_NULL )
        {
            //
            //  Set the index and get the name.
            //

            poFont->vSetFamIndex( s_wIdx );
            poFont->vSetFamilyFont( );

            //
            //  Everything up to here already has an index.
            //

            for( int j = i + 1; j < iCount; j++ )
            {
                poFont2 = m_poFontList->poObjectAt( j );

                if( poFont2->bSameFamily( poFont ) )
                {
                    poFont2->vSetFamIndex( s_wIdx );
                    poFont2->vSetNoFamilyFont( );

#ifdef WINNT
                    //
                    //  A Type1 cannot supercede as "head of the family".
                    //  If two non-Type1 fonts are competing for
                    //  "head of the family", the one with the "more regular"
                    //  style wins.  Lesser style values are "more regular".
                    //  This could be one boolean expression but I think the
                    //  nested "ifs" are more readable.
                    //
                    if ( !poFont2->bType1() )
                    {
                        if ( poFont->bType1() ||
                            (poFont2->dwStyle() < poFont->dwStyle()) )
                        {
                            poFont2->vSetFamilyFont( );
                            poFont->vSetNoFamilyFont( );

                            //
                            //  Use the new one as the main family font.
                            //
                            poFont = poFont2;
                        }
                    }
#else
                    //
                    // A smaller style value indicates a more Regular style.
                    //
                    if ( poFont2->dwStyle( ) < poFont->dwStyle( ) )
                    {
                        poFont2->vSetFamilyFont( );
                        poFont->vSetNoFamilyFont( );

                        //
                        //  Use the new one as the main family font.
                        //

                        poFont = poFont2;
                    }
#endif // WINNT
                }
            }

            s_wIdx++;

        }

        //
        //  If the main thread isn't waiting for us. Let go of the
        //  Critical_Section for a moment.
        //

        LCS;

        Sleep( 0 );

        ECS;

        //
        //  If the list has changed while we were gone, start over. We could
        //  just recurse and exit, but the combination of ECS/LCS and stack
        //  could mess us up.
        //

        if( iCount != m_poFontList->iCount( ) )
        {
            goto restart;
        }
    }

    m_bFamiliesNeverReset = FALSE;

    LCS;
}


/***************************************************************************
 * METHOD:  iSearchFontListFile
 *
 * PURPOSE: Search the list of fonts to see if the file is represented.
 *
 * RETURNS: the index or (-1)
 *
 ***************************************************************************/

int   CFontManager::iSearchFontListFile( PTSTR pszFile )
{
    //
    //  This function assumes the file is in the Fonts directory.
    //  This shortcoming will be rectified if necessary.
    //

    CFontClass* poFont = 0;
    int iCount = poLockFontList()->iCount();
    int i;
    int iRet = (-1);

    if( pszFile == NULL ) return NULL;

    for( i = 0; i < iCount; i++ )
    {
        poFont = m_poFontList->poObjectAt( i );

        if( poFont->bSameFileName( pszFile ) )
        {
            iRet = i;
            break;
        }
    }

    vReleaseFontList( );

    return iRet;
}


/***************************************************************************
 * METHOD:  poSearchFontListFile
 *
 * PURPOSE: Search the list of fonts to see if the file is represented.
 *
 * RETURNS: The object or NULL
 *
 ***************************************************************************/

CFontClass * CFontManager::poSearchFontListFile( PTSTR pszFile )
{
    return m_poFontList->poObjectAt( iSearchFontListFile( pszFile ) );
}


/***************************************************************************
 * METHOD:  ShouldAutoInstallFile
 *
 * PURPOSE: TRUE  = Install this file from reconciliation thread.
 *          FALSE = Don't install this file on reconciliation thread.
 *
 ***************************************************************************/

//
// Extensions of font files that should be excluded from auto-installation.
// This list includes AFM and INF because the reconciliation thread doesn't
// know how to build a PFM from an AFM/INF pair.  That function also displays
// a UI which we also don't want on the reconciliation thread.  To install
// a Type1 font on the reconciliation thread, the PFM and PFB files have to
// be copied to the fonts folder.
//
LPCTSTR c_pszExcludeThese[] = {TEXT("TTE"),
                               TEXT("AFM"),
                               TEXT("INF"),
                               NULL};


BOOL CFontManager::ShouldAutoInstallFile( PTSTR pszFile, DWORD dwAttribs )
{
    LPTSTR pszExt = NULL;

    //
    // If the file is hidden, don't auto install it.
    //
    if( dwAttribs & FILE_ATTRIBUTE_HIDDEN || m_HiddenFontFilesList.Exists(pszFile))
        return FALSE;

    pszExt = PathFindExtension( pszFile );

    if( pszExt && *pszExt )
    {
        pszExt++;

        for (UINT i = 0; NULL != c_pszExcludeThese[i]; i++)
        {
            if (0 == lstrcmpi(c_pszExcludeThese[i], pszExt))
            {
                //
                // If the file's extension is in the list of excluded
                // extensions, don't install it.
                //
                return FALSE;
            }
        }
    }

    return TRUE;
}


/***************************************************************************
 * METHOD:  iSearchFontListLHS
 *
 * PURPOSE: Search the list of fonts for the description.
 *
 * RETURNS: the font object or NULL.
 *
 ***************************************************************************/

int  CFontManager::iSearchFontListLHS( PTSTR pszLHS )
{
    CFontClass* poFont = 0;
    int iCount = poLockFontList()->iCount();
    int i;
    int iRet = (-1);


    if( pszLHS == NULL ) return NULL;

    for( i = 0; i < iCount; i++ )
    {
        poFont = m_poFontList->poObjectAt( i );

        if( poFont->bSameDesc( pszLHS ) )
        {
            iRet = i;
            break;
        }
    }

    vReleaseFontList( );

    return iRet;
}


/***************************************************************************
 * METHOD:  poSearchFontListLHS
 *
 * PURPOSE: Search the list of fonts for the description.
 *
 * RETURNS: the font object or NULL.
 *
 ***************************************************************************/

CFontClass * CFontManager::poSearchFontListLHS( PTSTR pszLHS )
{
    return m_poFontList->poObjectAt( iSearchFontListLHS( pszLHS ) );
}


/***************************************************************************
 * FUNCTION: iSearchFontList
 *
 * PURPOSE:  Search the FONTLIST for a face name.
 *
 * RETURNS:  index of item, or (-1)
 ***************************************************************************/

int CFontManager::iSearchFontList( PTSTR pszTarget, BOOL bExact, int iType )
{
    CFontClass* poFont = 0;
    int iCount;
    int i;

    if( pszTarget == NULL ) return( -1 );

    ECS;

    iCount = m_poFontList->iCount();

    for( i = 0; i < iCount; i++ ) {
       poFont = m_poFontList->poObjectAt( i );
       if( poFont->bSameName( pszTarget ) ) {
          LCS;
          return i;
       }
    }

    //
    //  If we didn't find a name, and bExact == FALSE, then look for anything
    //  with an overlap.
    //

    if( !bExact )
    {
        for( i = 0; i < iCount; i++ )
        {
            poFont = m_poFontList->poObjectAt( i );

            if( poFont->bNameOverlap( pszTarget ) )
            {
                if( ( iType == kSearchTT ) && !(poFont->bTrueType( ) || poFont->bOpenType( )))
                    continue;

                if( ( iType == kSearchNotTT ) && (poFont->bTrueType( ) || poFont->bOpenType( )))
                    continue;

                LCS;

                return i;
            }
        }
    }

    LCS;

    //
    //  No Match.
    //

    return( -1 );
}


/***************************************************************************
 * FUNCTION: lpDBSearchFontList
 *
 * PURPOSE:  Search the FONTLIST for a face name.
 *
 * RETURNS:  FontClass* if found, NULL if not.
 ***************************************************************************/

CFontClass* CFontManager::poSearchFontList( PTSTR pszTarget, BOOL bExact, int iType )
{
    return m_poFontList->poObjectAt( iSearchFontList( pszTarget, bExact, iType ) );
}


#if 0
/***************************************************************************
 * METHOD:  eFont
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 ***************************************************************************/
typedef struct {
    HDC   hDC;
    CFontManager * poFontMan;
    WORD  wFamIdx;
    CFontList * poSrcList;
    CFontList * poDstList;
} LENUMFAM;


static int CALLBACK eFont( LPLOGFONT    lpLogFont,
                           LPTEXTMETRIC lpTextMetric,
                           int          iFontType,
                           LPARAM       lFontEnum )
{
    LENUMFAM * lef = (LENUMFAM *) lFontEnum;
    ENUMLOGFONT FAR * lpEnum = (ENUMLOGFONT FAR *) lpLogFont;
    CFontManager * poFontMan = lef->poFontMan;
    CFontClass * poFont ;

    if( iFontType == TRUETYPE_FONTTYPE )
    {
        poFont = poFontMan->poSearchFontList( (PTSTR)lpEnum->elfFullName );

//      DEBUGMSG( (DM_TRACE1, TEXT( "eFont: Couldn't find find %s" ), lpEnum->elfFullName ) );

        if( !poFont )
        {
            //
            //  We didn't get the font based on full name. Try combinining the
            //  full name and the style.
            //

            TCHAR szCombine[ LF_FULLFACESIZE + LF_FACESIZE + 1 ];

            strcpy( szCombine, (LPTSTR) lpEnum->elfFullName );
            strcat( szCombine, TEXT( " " ) );
            strcat( szCombine, (LPTSTR) lpEnum->elfStyle );

            poFont = poFontMan->poSearchFontList( szCombine );

            if( !poFont )
            {
                DEBUGMSG( (DM_TRACE1, TEXT( "eFont: Couldn't find find %s" ),
                          szCombine ) );
            }
        }
    }
    else
    {
        //
        //  In order to handle WIN.INI entries of the form:
        //    "name 8,10,12 = fontfile"
        //  We look at partial overlapping names. Exclude TT files. If
        //  the WIN.INI has been hand-edited, you're out of luck.
        //

        poFont = poFontMan->poSearchFontList( (PTSTR) lpLogFont->lfFaceName,
                                              FALSE, kSearchNotTT );

    }

    if( poFont )
    {
        poFont->vSetFamName( lpLogFont->lfFaceName );
        poFont->vSetFamIndex( lef->wFamIdx );

        poFont->m_wWeight    = (WORD) lpLogFont->lfWeight;
        poFont->m_fSymbol    = (lpLogFont->lfCharSet == SYMBOL_CHARSET );
        poFont->m_fItalic    = (lpLogFont->lfItalic != 0);
        poFont->m_fUnderline = (lpLogFont->lfUnderline != 0);
        poFont->m_fStrikeout = (lpLogFont->lfStrikeOut != 0);

        //
        //  Move this into the temp list. It will be moved back when we're
        //  done enumerating.
        //

        if( lef->poDstList && lef->poDstList->bAdd( poFont ) )
            lef->poSrcList->poDetach( poFont );
    }

    return 1;
}


/***************************************************************************
 * METHOD:  eAllFamilies
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 ***************************************************************************/

static int CALLBACK eAllFamilies( LPLOGFONT     lpLogFont,
                                  LPTEXTMETRIC  lpTextMetric,
                                  int           iFontType,
                                  LPARAM        lEnumFam )
{
    LENUMFAM * lef = (LENUMFAM *)lEnumFam;
    int iSearchType = ( (iFontType == TRUETYPE_FONTTYPE) ? kSearchTT : kSearchNotTT );


    //
    //  Try to set a font as the primary font for this family..
    //

    CFontClass * poFont = lef->poFontMan->poSearchFontList(
                                                        lpLogFont->lfFaceName,
                                                        FALSE, iSearchType );

    if( poFont )
        poFont->vSetFamilyFont( );

    //
    //  Enumerate all the fonts within this family.
    //

    EnumFontFamilies( lef->hDC,
                      lpLogFont->lfFaceName,
                      (FONTENUMPROC) eFont,
                      (LPARAM) lEnumFam );

    //
    //  Increment the family index counter.
    //

    lef->wFamIdx++;

    return 1;
}


/***************************************************************************
 * METHOD:  bLoadFamList
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 ***************************************************************************/
BOOL CFontManager::bLoadFamList( )
{
    HWND  hWnd = GetDesktopWindow( );

    if( hWnd )
    {
        HDC   hDC = GetWindowDC( hWnd );

        if( hDC )
        {
            LENUMFAM lef;

            ECS;

            lef.hDC       = hDC;
            lef.poFontMan = this;
            lef.wFamIdx   = 1;
            lef.poSrcList = m_poFontList;
            lef.poDstList = m_poTempList;

            EnumFontFamilies( hDC, NULL, (FONTENUMPROC) eAllFamilies,
                              (LPARAM) &lef );

            ReleaseDC( hWnd, hDC );

            //
            //  If anything was moved into the temp list, move it back.
            //

            if( m_poTempList )
            {
                int iCount = m_poTempList->iCount();
                int i;

                for( i = iCount - 1; i >= 0; i-- )
                    m_poFontList->bAdd( m_poTempList->poDetach( i ) );
            }

            LCS;

            return TRUE;
        }
    }

    return FALSE;
}
#endif


/***************************************************************************
 * METHOD:  vDeleteFontList
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 ***************************************************************************/

void CFontManager::vDeleteFontList( CFontList * poList, BOOL bDelete )
{
    //
    //  Build up NULL delimited, NULL-terminated buffer to hand to
    //  SHFileOperations(). Any files that are links (not in the fonts
    //  directory) are not part of this -- we just remove their reference
    //  in WIN.INI
    //

    int            iCount = poList->iCount();
    FullPathName_t szPath;
    int            iBufSize = 1; // 1 for double-nul terminator.
    CFontClass *   poFont;
    int            i;


    if( !bDelete )
        goto uninstall;

    //
    //  Count the amount of memory we need. While we're at it, remove
    //  the font from GDI
    //

    for( i = 0; i < iCount; i++ )
    {
        poFont = poList->poObjectAt( i );

        if( poFont->bGetFileToDel( szPath ) )
        {
            iBufSize += lstrlen( szPath ) + 1;

            //
            // Add length of PFB file path if this font has an associated PFB
            // and that PFB is located in the fonts directory.  We don't
            // want to delete a PFB that isn't under our control.
            // Note that bGetPFB() returns FALSE if the font isn't Type1.
            //
            if (poFont->bGetPFB(szPath, ARRAYSIZE(szPath)) &&
                bFileIsInFontsDirectory(szPath))
            {
                iBufSize += lstrlen(szPath) + 1;
            }

            //
            //  If this doesn't work, we'll pick it up below because the
            //  delete failed.
            //

            poFont->bRFR( );
        }
    }

    //
    //  If all the entries were links, then there is no buffer.
    //

    if( 1 < iBufSize )
    {
        LPTSTR  lpBuf = new TCHAR[ iBufSize ];

        if( !lpBuf )
            return;

        //
        //  Fill it in.
        //

        LPTSTR lpCur = lpBuf;

        for( i = 0; i < iCount; i++ )
        {
            poFont = poList->poObjectAt( i );

            if( poFont->bGetFileToDel( lpCur ) )
            {
                lpCur += ( lstrlen( lpCur ) + 1 );

                //
                // Add path to the PFB file if there is one and if
                // that PFB is located in the fonts directory.  We don't
                // want to delete a PFB that isn't under our control.
                // Note that bGetPFB() returns FALSE if the font isn't Type1.
                //
                if (poFont->bGetPFB(szPath, ARRAYSIZE(szPath)) &&
                    bFileIsInFontsDirectory(szPath))
                {
                    lstrcpyn(lpCur, szPath, iBufSize - (size_t)(lpCur - lpBuf));
                    lpCur += (lstrlen(lpCur) + 1);
                }
            }
        }

        *lpCur = 0;

        //
        //  Do the delete.
        //

        SHFILEOPSTRUCT sFileOp =
        {
            NULL,
            FO_DELETE,
            lpBuf,
            NULL,
            FOF_ALLOWUNDO | FOF_NOCONFIRMATION,
            0,
            0
        } ;

        int iRet = SHFileOperation( &sFileOp );

        //
        //  If the operation was cancelled, determined what was done and what
        //  wasn't.
        //

        if( iRet || sFileOp.fAnyOperationsAborted )
        {
            //
            // Walk the list and determine if the file is there or not.
            //

            for( i = iCount - 1; i >= 0; i-- )
            {
               poFont = poList->poObjectAt( i );

               if( poFont->bOnSysDir( ) )
               {
                    poFont->vGetDirFN( szPath );

                    //
                    //  If the file exists then the operation didn't succeed.
                    //  Remove it from the list and AddFontResource.
                    //

                    if( GetFileAttributes( szPath ) != 0xffffffff )
                    {
                        poList->poDetach( i );
                        poFont->bAFR( );
                        poFont->Release();
                    }
                }
            }
        }
    }  // End of if( iBufSize )

uninstall:

    //
    //  Remove the fonts from the main list.
    //

    iCount = poList->iCount( );

    for( i = 0; i < iCount; i++)
    {
        poFont = poList->poObjectAt( i );
        vDeleteFont( poFont, FALSE );
    }

    //
    //  If there was something deleted, then notify apps.
    //

    if( iCount )
        vCPWinIniFontChange( );
}


/***************************************************************************
 * METHOD:  vDeleteFont
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 ***************************************************************************/

void CFontManager::vDeleteFont( CFontClass * lpFontRec, BOOL bRemoveFile )
{
    FontDesc_t     szLHS;

#if 0
    FullPathName_t szFile;

    //
    //  First, remove the font resource itself
    //  TODO. We need to return an error code. The font may be in use.
    //

    lpFontRec->bGetFQName( szFile, ARRAYSIZE( szFile ) );

    DWORD dwError;

    if( !RemoveFontResource( szFile ) )
    {
        TCHAR szFN[ MAX_PATH_LEN ];

        dwError = GetLastError( );

        lpFontRec->vGetFileName( szFN );

        if( !RemoveFontResource( szFN ) )
        {
            return;
        }
    }
#else

#ifdef WINNT

    if( lpFontRec->bType1( ) )
    {
        //
        //  Remove registry entries (files should have been deleted
        //  before reaching this point - in vDeleteFontList above).
        //
        //

        lpFontRec->vGetDesc( szLHS );

        DeleteT1Install( NULL, szLHS, bRemoveFile );

        goto RemoveRecord;
    }

#endif  //  WINNT


    if( !lpFontRec->bRFR( ) )
        return;

#endif

    //  Remove the entry from WIN.INI or the registry -- whereever it
    //  resides.

    lpFontRec->vGetDesc( szLHS );

    WriteProfileString( s_szINISFonts, szLHS, 0L );

    WriteToRegistry( szLHS, NULL );
    
    //
    //  Now, if we're talking about a FOT file for truetype guys, we always
    //  remove the FOT file. The corresponding TTF file is removed based on
    //  the user request. Stick the TTF file in the 'normal' file slot, the
    //  one whose deletion is conditional
    //

#if 0

    if( lpFontRec->bFOTFile( ) )
    {
        OpenFile( szFile, &ofstruct, OF_DELETE );

        lpFontRec->vGetTTFDirFN( szFile );
    }

#endif

RemoveRecord:

    ECS;

    //
    //  Remove the record from the list.
    //

    if( !m_poFontList->bDelete( lpFontRec ) )
        lpFontRec->Release();

    LCS;
}


/***************************************************************************
 * METHOD:  vDeleteFontFamily
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 ***************************************************************************/

void CFontManager::vDeleteFontFamily( CFontClass * lpFontRec, BOOL bRemoveFile )
{
    int   iCount = m_poFontList->iCount( );
    WORD  wFam = lpFontRec->wGetFamIndex( );

    while( iCount--)
    {
       lpFontRec = m_poFontList->poObjectAt( iCount );

       if( lpFontRec && ( wFam == lpFontRec->wGetFamIndex( ) ) )
          vDeleteFont( lpFontRec, bRemoveFile );
    }
}


/***************************************************************************
 * METHOD:  iCompare
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 ***************************************************************************/

int CFontManager::iCompare( CFontClass * pFont1,
                            CFontClass * pFont2,
                            CFontClass * pOrigin )
{
   USHORT   nDiff1,
            nDiff2;

   // DEBUGMSG( (DM_TRACE1,TEXT( "FontMan: iCompare" ) ) );

   nDiff1 = nDiff( pOrigin, pFont1 );
   nDiff2 = nDiff( pOrigin, pFont2 );

   return(  ( (int)(ULONG) nDiff1 ) - ((int)(ULONG) nDiff2 ) );
}


/***************************************************************************
 * METHOD:  nDiff
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 ***************************************************************************/

USHORT CFontManager::nDiff( CFontClass * pFont1, CFontClass * pFont2 )
{
   IPANOSEMapper * m_poMap;
   USHORT   nRet = (USHORT)(-1);

   // DEBUGMSG( (DM_TRACE1,TEXT( "nDiff        " ) ) );

   if( SUCCEEDED( GetPanMapper( &m_poMap ) ) ) {
      BYTE * lpPan1 = pFont1->lpBasePANOSE( );
      BYTE * lpPan2 = pFont2->lpBasePANOSE( );

      nRet = m_poMap->unPANMatchFonts( lpPan1,
                        PANOSE_LEN,
                        lpPan2,
                        PANOSE_LEN,
                        *lpPan1 );

      m_poMap->Release( );
   }

   return nRet;
}

// -----------------------------------------------------------------------
// GetPanoseClass
//
// Return the class of the Panose Mapper. If we expose this in the
// registery, we can just alter this function and everything still
// works correctly.
//
//------------------------------------------------------------------------

CLSID GetPanoseClass( )
{
   return CLSID_PANOSEMapper;
}

/***************************************************************************
 * METHOD:  GetPanMapper
 *
 * PURPOSE:
 *
 * RETURNS:
 *
 ***************************************************************************/
HRESULT CFontManager::GetPanMapper( IPANOSEMapper ** ppMapper )
{
    HRESULT   hr = ResultFromScode( E_NOINTERFACE );


    *ppMapper = NULL;

    ECS;

    if( !m_poPanMap && !m_bTriedOnce )
    {
        m_bTriedOnce = TRUE;

        DEBUGMSG( (DM_TRACE1,TEXT( "GetPanMapper calling GetPanoseClass()" ) ) );

        CLSID clsid = GetPanoseClass( );

        DEBUGMSG( (DM_TRACE1,TEXT( "GetPanMapper calling CoCreateInstance" ) ) );

        DEBUGMSG( (DM_TRACE1, TEXT( "Initializing OLE" ) ) );

#if 0
        hr = CoInitialize( NULL );

        if( FAILED( hr ) )
        {
            LCS;
            return( hr );
        }
#endif

        DEBUGMSG( (DM_TRACE1, TEXT( "Calling CoCreateInstance" ) ) );
#if 0
        hr = CoCreateInstance( clsid,
                               NULL,
                               (DWORD) CLSCTX_INPROC_SERVER,
                               IID_IPANOSEMapper,
                               (LPVOID *) &m_poPanMap );
#endif
        hr = SHCoCreateInstance( NULL,
                                 &clsid,
                                 NULL,
                                 IID_IPANOSEMapper,
                                 (LPVOID *) &m_poPanMap );

        if( FAILED( hr ) )
        {
#if 0
            CoUninitialize( );
#endif
            DEBUGMSG( (DM_ERROR, TEXT( "FONTEXT: CFontMan::GetPan() Failed  %d" ),
                        hr) );

            // DEBUGBREAK;
        }

        //
        //  We have the mapper. Relax the threshold so we
        //  can get values for sorting.
        //

        else
            m_poPanMap->vPANRelaxThreshold( );
    }

    //
    //  AddRef for the caller. (This will make the count > 1 )
    //  We Release( ) on delete.
    //

    if( m_poPanMap )
    {
        // DEBUGMSG( (DM_TRACE1, TEXT( "GetPanMapper calling m_poPanMap->AddRef()" ) ) );

        m_poPanMap->AddRef( );

        *ppMapper = m_poPanMap;
        hr = NOERROR;
    }

    LCS;

    return hr;
}


//
// Build a hash table of font file names contained in the
// "HiddenFontFiles" section of %windir%\FONT.INF.  This identifies
// those font files that are to be always hidden, and therefore
// excluded from installation via the folder reconciliation thread.
//
DWORD CFontManager::HiddenFilesList::Initialize(void)
{
    DWORD dwNamesLoaded = 0;

    //
    // If already initialized, destroy current contents.
    //
    if (IsInitialized())
        Destroy();

    //
    // Initialize hash table with 101 buckets and
    // make it case insensitive.
    // There are currently 140 entries in the HiddenFontFiles
    // section of FONT.INF.
    //
    if (StringTable::Initialize(101,       // Hash bucket count.
                                FALSE,     // Case insensitive.
                                FALSE))    // No duplicates.
    {
        HANDLE hInf = INVALID_HANDLE_VALUE;

        hInf = SetupOpenInfFile(TEXT("FONT.INF"), NULL, INF_STYLE_WIN4, NULL);

        if (INVALID_HANDLE_VALUE != hInf)
        {
            INFCONTEXT Context;

            if(SetupFindFirstLine(hInf, TEXT("HiddenFontFiles"), NULL, &Context))
            {
                TCHAR szFileName[MAX_PATH];
                DWORD dwReqdSize = 0;

                do
                {
                    if(SetupGetStringField(&Context,
                                           0,
                                           szFileName,
                                           ARRAYSIZE(szFileName),
                                           &dwReqdSize))
                    {
                        if (Add(szFileName))
                            dwNamesLoaded++;
                    }
                } while(SetupFindNextLine(&Context, &Context));
            }
            SetupCloseInfFile(hInf);
        }
    }
    return dwNamesLoaded;
}


#ifdef WINNT
//
// Determine if a Type1 font driver is loaded.
// This code was taken from \ntuser\client\fntsweep.c
// Originally written by BodinD.
//
BOOL CFontManager::CheckForType1FontDriver(void)
{
    LONG       lRet;
    WCHAR      awcClass[MAX_PATH] = L"";
    DWORD      cwcClassName = MAX_PATH;
    DWORD      cSubKeys;
    DWORD      cjMaxSubKey;
    DWORD      cwcMaxClass;
    DWORD      cValues = 0;
    DWORD      cwcMaxValueName;
    DWORD      cjMaxValueData;
    DWORD      cjSecurityDescriptor;

    HKEY       hkey = NULL;
    FILETIME   ftLastWriteTime;

    BOOL bRet = FALSE;

    // open the font drivers key and check if there are any entries, if so
    // return true. If that is the case we will call AddFontResourceW on
    // Type 1 fonts at boot time, right after user had logged on
    // PostScript printer drivers are not initialized at this time yet,
    // it is safe to do it at this time.

    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,        // Root key
                         s_szKeyFontDrivers,        // Subkey to open
                         0L,                        // Reserved
                         KEY_READ,                  // SAM
                         &hkey);                    // return handle

    if (lRet == ERROR_SUCCESS)
    {
        // get the number of entries in the [Fonts] section

        lRet = RegQueryInfoKeyW(
                   hkey,
                   awcClass,              // "" on return
                   &cwcClassName,         // 0 on return
                   NULL,
                   &cSubKeys,             // 0 on return
                   &cjMaxSubKey,          // 0 on return
                   &cwcMaxClass,          // 0 on return
                   &cValues,              // == cExternalDrivers
                   &cwcMaxValueName,      // longest value name
                   &cjMaxValueData,       // longest value data
                   &cjSecurityDescriptor, // security descriptor,
                   &ftLastWriteTime
                   );

        if ((lRet == ERROR_SUCCESS) && cValues)
        {
            bRet = TRUE;
        }

        RegCloseKey(hkey);
    }
    return (m_bType1FontDriverInstalled = bRet);
}

#endif // WINNT
