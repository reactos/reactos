///////////////////////////////////////////////////////////////////////////////
//
// fsnotify.cpp
//      Explorer Font Folder extension routines
//     Routines to watch the Fonts directory and handle change notifications.
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

#if defined(__FCN__)

#include "fsnotify.h"
#include "fontman.h"

#include "dbutl.h"

#ifdef DEBUG
int iCount = 0;
#endif  // DEBUG

#define ARRAYSIZE(a)  (sizeof(a) / sizeof((a)[0]))

//------------------------------------------------------------------------
// FUNCTION:   dwNotifyWatchProc
//
// PURPOSE:    Watch a directory and notify the CFontManager when something
//             has changed.
//------------------------------------------------------------------------
DWORD dwNotifyWatchProc( LPVOID pvParams )
{
    DWORD dwRet;
    BOOL  bFileChange = FALSE;
    BOOL bDone = FALSE;
    CFontManager *pFontManager = (CFontManager *)pvParams;

    DEBUGMSG( (DM_TRACE2, TEXT( "dwNotifyWatchProc called" ) ) );
    
    if (NULL == pFontManager)
        return (DWORD)-1;

    HANDLE rgHandles[] = { pFontManager->m_hEventTerminateThreads,
                           pFontManager->m_Notify.m_hWatch };
    while(!bDone)
    {
        //
        //  Wait for the FONTS folder to change. If we time out, then attempt
        //  to undo any deletions that might be occuring.
        //                      
        dwRet = WaitForMultipleObjects(ARRAYSIZE(rgHandles), 
                                       rgHandles, 
                                       FALSE, 
                                       1500);

        if (WAIT_TIMEOUT == dwRet)
        {
            // DEBUGMSG( (DM_TRACE2, TEXT( "dwNotifyWatchProc: main loop - Timeout from WaitForSingleObject - iteration %d" ), ++iCount ) );

            if( !pFontManager->bCheckTBR( ) )
                pFontManager->vUndoTBR( );
        
            //
            //  Go through the fonts directory and make sure it's in a
            //  stable condition.
            //

            if( bFileChange )
            {
                bFileChange = FALSE;
                pFontManager->vReconcileFolder( THREAD_PRIORITY_NORMAL );
            }
        }
        else switch(dwRet - WAIT_OBJECT_0)
        {
            case 0:
                //
                // "Terminate-threads" event was set.  Time to go home.
                //
                bDone = true;
                break;
                
            case 1:
                //
                //  Things be happenin'. We could call bCheckTBR() at this
                //  point, but we might as well wait for a time out and do
                //  it all at once. Doing nothing just causes us to wait 
                //  another 1.5 secs; i.e. reset the timeout.
                //
                bFileChange = TRUE;

                //
                //  Since an event came in, reset the Change Notification to
                //  watch it some more.  This call should NOT be done during
                //  the TIMEOUT because it causes another change packet under
                //  WinNT and an undesireable race condition under Win 95.
                //  (Note:  The extra change packets under WinNT are allocated
                //   out of Non-paged pool memory and excessive requests can
                //   use up the processes non-paged pool memory quota, and
                //   then the fun really begins with the app.)  [stevecat]
                //
        
                if( !FindNextChangeNotification(pFontManager->m_Notify.m_hWatch))
                {
                    DEBUGMSG( (DM_ERROR, TEXT( "dwNotifyWatchProc: FindNextChangeNotification FAILED - error = %d" ), GetLastError( ) ) );
                }
        
                DEBUGMSG( (DM_TRACE2, TEXT( "dwNotifyWatchProc: FindNextChangeNotification called - handle = 0x%x" ), pFontManager->m_Notify.m_hWatch));
        
                //
                // Wait 'til next 1.5 second timeout to do anything.
                //
                break;
                
            default:
                    break;
        }
    }
    InterlockedDecrement(&g_cRefThisDll);    
    return 0;
}

#endif   // __FCN__ 
