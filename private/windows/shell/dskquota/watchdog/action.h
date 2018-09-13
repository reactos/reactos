#ifndef __ACTION_H
#define __ACTION_H
///////////////////////////////////////////////////////////////////////////////
/*  File: action.h

    Description: Declarations for classes to handle actions associated
        with user notifications (email, popup dialog etc).
        
            CAction
            CActionEmail
            CActionPopup

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////

#ifndef __MAPISEND_H
#   include "mapisend.h"
#endif


//
// Fwd declarations.  Don't need headers.
//
class CHistory;
class CStatisticsList;

class CAction
{
    public:
        CAction(VOID) { };
        virtual ~CAction(VOID) { };

        virtual HRESULT DoAction(CHistory& history) = 0;

    private:
        //
        // Prevent copy.
        //
        CAction(const CAction& rhs);
        CAction& operator = (const CAction& rhs);
};


class CActionEmail : public CAction
{
    public:
        CActionEmail(CMapiSession& MapiSession,
                     LPMAPIFOLDER pMapiFolder,
                     LPTSTR pszRecipientsTo,
                     LPTSTR pszRecipientsCc,
                     LPTSTR pszRecipientsBcc,
                     LPCTSTR pszSubject,
                     CMapiMessageBody& MsgBody);

        virtual ~CActionEmail(VOID);

        virtual HRESULT DoAction(CHistory& history);

    private:
        CMapiSession&   m_MapiSession;    // Reference to MAPI session object.
        CMapiRecipients m_MapiRecipients; // List of recipients for for message.
        CMapiMessage    m_MapiMsg;        // MAPI message we'll build and send.
        MAPI            m_Mapi;           // MAPI functions.

        //
        // Prevent copy.
        //
        CActionEmail(const CActionEmail& rhs);
        CActionEmail& operator = (const CActionEmail& rhs);
};

class CActionPopup : public CAction
{
    public:
        CActionPopup(CStatisticsList& stats);

        virtual ~CActionPopup(VOID);

        virtual HRESULT DoAction(CHistory& history);

    private:
        CStatisticsList& m_stats;
        HWND             m_hwnd;
        HINSTANCE        m_hmodCOMCTL32;
        HICON            m_hiconDialog;
        static UINT      m_idAutoCloseTimer;
        static UINT      m_uAutoCloseTimeout;

        HRESULT CreateAndRunPopup(
            HINSTANCE hInst,
            LPCTSTR pszDlgTemplate,
            HWND hwndParent);

        static BOOL CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        BOOL OnInitDialog(HWND hwnd);
        BOOL OnDestroy(HWND hwnd);
        BOOL OnNcDestroy(HWND hwnd);
        VOID InitializeList(HWND hwndList);

        //
        // Prevent copy.
        //
        CActionPopup(const CActionPopup& rhs);
        CActionPopup& operator = (const CActionPopup& rhs);
};

#endif //__ACTION_H

