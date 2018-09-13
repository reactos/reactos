
#include "stdafx.h"

#include <stdarg.h>

#include "pbrush.h"
#include "pbrusfrm.h"
#include "pbrusvw.h"
#include "minifwnd.h"
#include "cmpmsg.h"

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

#include "memtrace.h"

//
//
// CmpCenterParent retrieves a window to which a modal dialog should
// be centered (relative center).
//
// NOTE: The return value may be temporary!
//
CWnd* CmpCenterParent()
    {
    CWnd* pPopupWnd = AfxGetMainWnd();;

    ASSERT(pPopupWnd != NULL);

    if (pPopupWnd->IsKindOf(RUNTIME_CLASS(CMiniFrmWnd)))
        {
        // don't center relative to mini-frame windows
        CWnd* pParentWnd = pPopupWnd->GetParent();

        // instead use parent or main window...
        if (pParentWnd != NULL)
            pPopupWnd = pParentWnd;
        else
            pPopupWnd = theApp.m_pMainWnd;
        }

    return pPopupWnd;
    }

//
// composer message box, same interface as windows, but you give
// string id's not strings
//
// example: CmpMessageBox( IDS_OUTOFMEMORY, IDS_ERROR, MB_OK );
//
int CmpMessageBox(  WORD    wTextStringID,      // string id of text
                    WORD    wCaptionID,         // string id of caption
                    UINT    nType )             // same as message box
    {
    TCHAR FAR*   lpText;
    TCHAR FAR*   lpCaption;
    CString     sText, sCaption;

    if( wCaptionID == CMPNOSTRING )
        lpCaption = NULL;
    else
        {
        VERIFY( sCaption.LoadString( wCaptionID ) );

        lpCaption = (TCHAR FAR*)(const TCHAR *)sCaption;
        }

    if( wTextStringID == CMPNOSTRING )
        lpText = TEXT("");
    else
        {
        VERIFY( sText.LoadString( wTextStringID ) );

        lpText = (TCHAR FAR*)(const TCHAR *)sText;
        }

    CWnd *pcWnd = AfxGetMainWnd();

    if (pcWnd != NULL)
        {
        return  pcWnd->MessageBox(lpText, lpCaption, nType | MB_TASKMODAL);
        }
    else
        {
        return  ::MessageBox(NULL, lpText, lpCaption,nType | MB_TASKMODAL);
        }
    }

int CmpMessageBoxString( CString&   s,
                         WORD       wCaptionID,
                         UINT       nType )
    {
    TCHAR FAR*   lpCaption;
    CString     sText, sCaption;

    if( wCaptionID == CMPNOSTRING )
        lpCaption = NULL;
    else
        {
        VERIFY( sCaption.LoadString( wCaptionID ) );

        lpCaption = (TCHAR FAR*)(const TCHAR *)sCaption;
        }

    CWnd *pcWnd = AfxGetMainWnd();

    if (pcWnd != NULL)
        {
        return  pcWnd->MessageBox((const TCHAR *)s, lpCaption,nType | MB_TASKMODAL);
        }
    else
        {
        return  ::MessageBox(NULL, (const TCHAR *)s, lpCaption,nType | MB_TASKMODAL);
        }
    }

int CmpMessageBox2(  WORD    wTextStringID,
                     WORD    wCaptionID,
                     UINT    nType,
                     LPCTSTR szParam1,
                     LPCTSTR szParam2 )
    {
    TCHAR FAR*   lpText;
    TCHAR FAR*   lpCaption;
    CString     sText, sCaption;

    if( wCaptionID == CMPNOSTRING )
        lpCaption = NULL;
    else
        {
        VERIFY( sCaption.LoadString( wCaptionID ) );

        lpCaption = (TCHAR FAR*)(const TCHAR *)sCaption;
        }

    if( wTextStringID == CMPNOSTRING )
        lpText = TEXT("");
    else
        {
        AfxFormatString2( sText, wTextStringID, szParam1, szParam2);

        lpText = (TCHAR FAR*)(const TCHAR *)sText;
        }

    CWnd *pcWnd = AfxGetMainWnd();

    if (pcWnd != NULL)
        {
        return  pcWnd->MessageBox(lpText, lpCaption, nType | MB_TASKMODAL);
        }
    else
        {
        return  ::MessageBox(NULL, lpText, lpCaption,nType | MB_TASKMODAL);
        }
    }

//
// composer message box, combines wsprintf, you continue to
// use string ids
//
// example:
//
// CmpMessageBoxPrintf( IDS_CANTOPEN, IDS_ERROR, MB_OK, lpszFileName );
//

#define nLocalBuf 512

extern "C" int CDECL
    CmpMessageBoxPrintf(WORD    wTextStringID,  // string id of text (format)
                        WORD    wCaptionID,     // string id of caption
                        UINT    nType,          // same as message box
                        ... )                   // wsprintf arguments
    {
    TCHAR FAR*   lpText;
    TCHAR FAR*   lpCaption;
    CString     sText, sCaption;
    int         nBuf;
    TCHAR        szBuffer[nLocalBuf];

    va_list args;
    va_start( args, nType );

    if( wCaptionID == CMPNOSTRING )
        lpCaption = NULL;
    else
        {
        VERIFY( sCaption.LoadString( wCaptionID ) );

        lpCaption = (TCHAR FAR*)(const TCHAR *)sCaption;
        }

    if( wTextStringID == CMPNOSTRING )
        lpText = TEXT("");
    else
        {
        VERIFY( sText.LoadString( wTextStringID ) );

        lpText = (TCHAR FAR*)(const TCHAR *)sText;
        }

    nBuf = wvsprintf( szBuffer, lpText, args );

    ASSERT( nBuf < nLocalBuf );
    CWnd *pcWnd = AfxGetMainWnd();

    if (pcWnd != NULL)
        {
        return  pcWnd->MessageBox(szBuffer, lpCaption,nType | MB_TASKMODAL);
        }
    else
        {
        return  ::MessageBox(NULL, szBuffer, lpCaption,nType | MB_TASKMODAL);
        }
    }
