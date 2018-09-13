/* Unix specific function prototypes */

#include <mainwin.h>
#include "../shdocvw/unixstuff.h"
#include "shbrows2.h"
#include "shalias.h"

#define MAIL_ACTION_SEND 1
#define MAIL_ACTION_READ 2

#define UNIX_TITLE_SUFFIX TEXT("")

EXTERN_C MwPaintSpecialEOBorder( HWND hWnd, HDC hDC );

BOOL CheckForInvalidOptions( LPCTSTR inCmdLine );
void PrintIEHelp();
void PrintIEVersion();

// IE Thread Info stuff
void UnixStuffInit();
void StoreIEWindowInfo( HWND hwnd );
HWND GetIEWindowOnThread();

// Marshalling stuff

#define CoMarshalInterface CoMarshalInterfaceDummy

STDAPI CoMarshalInterfaceDummy( IStream *, REFIID, IUnknown *, DWORD, void *, DWORD );

struct THREADWINDOWINFO
{
    int cWindowCount;
    CShellBrowser2** rgpsb;
};

EXTERN_C THREADWINDOWINFO * InitializeThreadInfoStructs();
EXTERN_C HRESULT TranslateModelessAccelerator(MSG* msg, HWND hwnd);
EXTERN_C void FreeThreadInfoStructs();
EXTERN_C void AddFirstBrowserToList( CShellBrowser2 *psb );
EXTERN_C void RemoveBrowserFromList( CShellBrowser2 *psb );
EXTERN_C void IEFrameNewWindowSameThread(IETHREADPARAM* piei);
STDAPI_(BOOL) FileHasProperAssociation (LPCTSTR path);

BOOL IsNamedWindow(HWND hwnd, LPCTSTR pszClass);

LRESULT HandleCopyDataUnix(CShellBrowser2* psb, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#define DM_THREADWINDOWINFO          0

inline CShellBrowser2 * CheckAndForwardMessage( 
       THREADWINDOWINFO * lpThreadWindowInfo ,
       CShellBrowser2 * psb, 
       MSG msg
       )
{
    // get psb for msg.hwnd. (Note psb inside scope may != psb outside)
    if (lpThreadWindowInfo->cWindowCount)
    {
        CShellBrowser2* psb, *psbActive = NULL;
        int i;
        BOOL fFoundSB = FALSE, fDelayClosed = FALSE;
        for (i = 0; i < lpThreadWindowInfo->cWindowCount; i++)
        {
            psb = lpThreadWindowInfo->rgpsb[i];

            // Save the active window here so that  we don't have to 
            // loop again.
            if(psb->_fActivated) 
            {
                psbActive = psb;
            }

            if(psb->_fDelayedClose)
            {
                fDelayClosed = TRUE;
            }

            if (psb->_pbbd->_hwnd == msg.hwnd || IsChild(psb->_pbbd->_hwnd, msg.hwnd))
            {
                fFoundSB = TRUE;
                break;
            }
       }

#ifdef DEBUG
       if (!fFoundSB)
       {
           TraceMsg(DM_THREADWINDOWINFO, "IE_TP ThreadWindowInfo didnt find psb for hwnd = %X, reusing hwnd = %X", msg.hwnd, psb->_pbbd->_hwnd);
       }
#endif //DEBUG                            

       if( fDelayClosed || (i<lpThreadWindowInfo->cWindowCount) )
       {
           // Post WM_CLOSE messages for all the windows which are delayClosed.
           for (i = 0; i < lpThreadWindowInfo->cWindowCount; i++)
           {
               CShellBrowser2* psbDel = lpThreadWindowInfo->rgpsb[i];
               if( psbDel->_fDelayedClose )
               {
                   psbDel->_fDelayedClose = FALSE;
                   PostMessage( psbDel->_pbbd->_hwnd, WM_CLOSE, 0, 0 );
               }
           }
       }

       // Pass message to the current active window, because we fail to find the
       // appropriate parent. This happens in case of favorites window and msgs
       // get diverted to the last window on the array, which may be wrong.

       if( !fFoundSB && psbActive )
       {
           psb = psbActive;
       }
  
       return psb;
    }
    return NULL;
}

#undef KEYBOARDCUES
