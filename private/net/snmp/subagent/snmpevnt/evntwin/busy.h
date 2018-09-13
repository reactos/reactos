//****************************************************************************
//
//  Copyright (c) 1994, Microsoft Corporation
//
//  File:  BUSY.H
//
//  The classes defined here are:
//
//     CBusy    Utility class to indicate to the user that the app is busy.
//              When an instance is constructed, it changes the cursor to the
//              wait cursor.  If a parent window is provided it will receive a
//              message to set the text of its status indicator to a specified
//              string.  This is useful for windows that have a status bar or
//              some other textual indication of status.  When the instance is
//              destructed, the cursor is restored and the parent window is
//              sent a message to reset its status indicator.
//
//  History:
//
//      Scott V. Walker, SEA    6/30/94    Created.
//
//****************************************************************************
#ifndef _BUSY_H_
#define _BUSY_H_

//****************************************************************************
//
//  Messages sent to parent window.
//
//----------------------------------------------------------------------------
//
//  WM_BUSY_GETTEXT
//
//  This message is sent by the CBusy to retrieve the current status indicator
//  text.  The CBusy will restore this text when it destructs.
//
//  wparam = nLength;           // Length of buffer.
//  lparam = (LPARAM)pStr;      // Pointer to buffer to copy data into.
//
//----------------------------------------------------------------------------
//
//  WM_BUSY_SETTEXT
//
//  This message is sent by the CBusy to inform the window to set its status
//  indicator to the given string.
//
//  wparam is unused.
//  lparam = (LPARAM)pStr;      // Pointer to buffer containing status text.
//
//  Return value = n/a.
//
//****************************************************************************

#define WM_BUSY_GETTEXT     (WM_USER + 0x75)
#define WM_BUSY_SETTEXT     (WM_USER + 0x76)

//****************************************************************************
//
//  CLASS:  CBusy
//
//  When you construct a CBusy, you have the option of specifying a parent
//  window and a string ID.  If these are provided, The CBusy will send
//  WM_BUSY_GETTEXT and WM_BUSY_SETTEXT messages to the window during
//  construction and destruction.  The parent window can respond to these
//  messages by modifying a text status indicator (such as a status bar) to
//  display the specified string.  Use these by constructing a local instance
//  at the top of a function.  When the function goes out of scope (no matter
//  where the return is encountered), the instance will be destructed, causing
//  the busy indications (cursor and text) to be restored.
//
//----------------------------------------------------------------------------
//
//  CBusy::CBusy
//
//  Constructor.  When an instance is constructed, it sets the cursor to the
//  wait cursor and optionally notifies a specified window to change its
//  status indicator.
//
//  Parameters:
//      CWnd *pParentWnd    Optional parent window.  If provided, the CBusy
//                          sends WM_BUSY_GETTEXT and WM_BUSY_SETTEXT
//                          messages to the given window.
//      const char *pszText Optional string.  If provided (and if a parent
//                          window is specified), the CBusy passes it in the
//                          WM_BUSY_SETTEXT message to the parent window.  If
//                          not provided, the parent window is sent an empty
//                          string.
//
//      If parameter 2 is a UINT, CBusy will treat it as a string ID and do a
//      LoadString. 
//
//****************************************************************************

class CBusy : public CObject
{

private:

    CWnd *m_pParentWnd;
    HCURSOR m_hOldCursor;
    CString m_sOldText;

private:

    void SetBusy(CWnd *pParentWnd, LPCTSTR pszText);

public:

    CBusy(CWnd *pParentWnd, LPCTSTR pszText);
    CBusy(CWnd *pParentWnd, UINT nID);
    CBusy(CWnd *pParentWnd);
    CBusy();
    ~CBusy();
};

#endif // _BUSY_H_

