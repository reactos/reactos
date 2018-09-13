
//////////////////////////////////////////////////////////////////////////
//
//                                CMPMSG.H
//
//  Owner:     HenryB
//  Created:   Dec 9, 1991
//  Revision:
//
//////////////////////////////////////////////////////////////////////////

//
// Composer Message Box routines
//

#define CMPNOSTRING     (WORD)(-1)              // empty/no string

// CmpCenterParent retrieves a window to which a modal dialog should
// be centered (relative center).
//
// NOTE: The return value may be temporary!
//
CWnd* CmpCenterParent();

//
// composer message box, same interface as windows, but you give
// string id's not strings
//
// example: CmpMessageBox( IDS_OUTOFMEMORY, IDS_ERROR, MB_OK );
//
int CmpMessageBox(  WORD    wTextStringID,      // string id of text
                    WORD    wCaptionID,         // string id of caption
                    UINT    nType );            // same as message box

//
// composer message box wrapper for parameterized strings
//
// example: CmpMessageBox2( IDS_NOCONVERT, IDS_ERROR, MB_OK, lpszFrom, lpszInto );
//
int CmpMessageBox2(  WORD    wTextStringID,     // string id of text
                     WORD    wCaptionID,        // string id of caption
                     UINT    nType,             // same as message box
                     LPCTSTR szParam1,           // string for %1 param
                     LPCTSTR szParam2 );         // string for %2 param

//
// composer message box, combines wsprintf, you continue to
// use string ids
//
// example:
//
// CmpMessageBoxPrintf( IDS_CANTOPEN, IDS_ERROR, MB_OK, lpszFileName );
//

extern "C" int CDECL
    CmpMessageBoxPrintf(WORD    wTextStrinID,   // string id of text (format)
                        WORD    wCaptionID,     // string id of caption
                        UINT    nType,          // same as message box
                        ... );                  // wsprintf arguments


int CmpMessageBoxString( CString&   s,
                         WORD       wCaptionID,
                         UINT       nType );

