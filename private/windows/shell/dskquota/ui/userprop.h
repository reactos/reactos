#ifndef __DSKQUOTA_USER_PROPSHEET_H
#define __DSKQUOTA_USER_PROPSHEET_H
///////////////////////////////////////////////////////////////////////////////
/*  File: userprop.h

    Description: Provides declarations for quota user property page.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
    06/25/98    Replaced AddUserPropSheet with AddUserDialog.        BrianAu
                Now that we're getting user info from the DS
                object picker, the prop sheet idea doesn't work
                so well.  A std dialog is better.
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _INC_DSKQUOTA_H
#   include "dskquota.h"
#endif
#ifndef _INC_DSKQUOTA_USER_H
#   include "user.h"
#endif
#ifndef _INC_DSKQUOTA_UNDO_H
#   include "undo.h"
#endif
#ifndef _INC_DSKQUOTA_DETAILS_H
#   include "details.h"    // For LVSelection
#endif
#ifndef __OBJSEL_H_
#   include <objsel.h>
#endif

#include "resource.h"

//
// User property page.
//
class UserPropSheet
{
    private:
        enum { iICON_USER_SINGLE,
               iICON_USER_MULTIPLE,
               cUSER_ICONS };

        enum { iICON_STATUS_OK,
               iICON_STATUS_OVER_THRESHOLD,
               iICON_STATUS_OVER_LIMIT,
               cSTATUS_ICONS };

        //
        // Prevent copying.
        //
        UserPropSheet(const UserPropSheet&);
        void operator = (const UserPropSheet&);

        LONGLONG           m_cVolumeMaxBytes;
        LONGLONG           m_llQuotaUsed;
        LONGLONG           m_llQuotaLimit;
        LONGLONG           m_llQuotaThreshold;
        int                m_idCtlNextFocus;
        PDISKQUOTA_CONTROL m_pQuotaControl;
        UndoList&          m_UndoList;
        LVSelection&       m_LVSelection;
        HWND               m_hWndParent;
        CVolumeID          m_idVolume;
        CString            m_strPageTitle;
        BOOL               m_bIsDirty;
        BOOL               m_bHomogeneousSelection;      // All selected, same limit/threshold.
        HICON              m_hIconUser[cUSER_ICONS];     // 0=Single, 1=Multi-user.
        HICON              m_hIconStatus[cSTATUS_ICONS]; // 0=OK,1=Warn,2=Error
        XBytes            *m_pxbQuotaLimit;
        XBytes            *m_pxbQuotaThreshold;

        static INT_PTR OnInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam);
        INT_PTR OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);
        INT_PTR OnNotify(HWND hDlg, WPARAM wParam, LPARAM lParam);
        INT_PTR OnHelp(HWND hDlg, WPARAM wParam, LPARAM lParam);
        INT_PTR OnContextMenu(HWND hwndItem, int xPos, int yPos);

        //
        // PSN_xxxx handlers.
        //
        INT_PTR OnSheetNotifyApply(HWND hDlg, WPARAM wParam, LPARAM lParam);
        INT_PTR OnSheetNotifyKillActive(HWND hDlg, WPARAM wParam, LPARAM lParam);
        INT_PTR OnSheetNotifySetActive(HWND hDlg, WPARAM wParam, LPARAM lParam);

        //
        // EN_xxxx handlers.
        //
        INT_PTR OnEditNotifyUpdate(HWND hDlg, WPARAM wParam, LPARAM lParam);
        INT_PTR OnEditNotifyKillFocus(HWND hDlg, WPARAM wParam, LPARAM lParam);

        //
        // CBN_xxxx handlers.
        //
        INT_PTR OnComboNotifySelChange(HWND hDlg, WPARAM wParam, LPARAM lParam);

        HRESULT UpdateControls(HWND hDlg) const;
        HRESULT InitializeControls(HWND hDlg);
        HRESULT RefreshCachedUserQuotaInfo(VOID);
        HRESULT ApplySettings(HWND hDlg, bool bUndo = true);
        HRESULT RefreshCachedQuotaInfo(VOID);

        VOID UpdateSpaceUsed(HWND hDlg, LONGLONG iUsed, LONGLONG iLimit, INT cUsers);
        VOID UpdateUserName(HWND hDlg, PDISKQUOTA_USER pUser);
        VOID UpdateUserName(HWND hDlg, INT cUsers);
        VOID UpdateUserStatusIcon(HWND hDlg, LONGLONG iUsed, LONGLONG iThreshold, LONGLONG iLimit);

        INT QueryUserIcon(HWND hDlg) const;
        INT QueryUserStatusIcon(HWND hDlg) const;

    public:
        //
        // Prop sheet for editing users.
        //
        UserPropSheet(PDISKQUOTA_CONTROL pQuotaControl,
                      const CVolumeID& idVolume,
                      HWND hWndParent,
                      LVSelection& LVSelection,
                      UndoList& UndoList);

        ~UserPropSheet(VOID);

        HRESULT Run(VOID);

        //
        // Dialog Proc callback.
        //
        static INT_PTR APIENTRY DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
};

#endif // __DSKQUOTA_USER_PROPSHEET_H

