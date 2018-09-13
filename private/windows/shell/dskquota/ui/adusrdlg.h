#ifndef __DSKQUOTA_ADDUSER_DIALOG_H
#define __DSKQUOTA_ADDUSER_DIALOG_H
///////////////////////////////////////////////////////////////////////////////
/*  File: adusrdlg.h

    Description: Provides declarations for the "Add User" dialog.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/15/98    Initial creation.                                    BrianAu
                Separated code from userprop.h
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

class AddUserDialog
{
    public:
        //
        // Prop sheet for editing users.
        //
        AddUserDialog(PDISKQUOTA_CONTROL pQuotaControl,
                      const CVolumeID& idVolume,
                      HINSTANCE hInstance,
                      HWND hwndParent,
                      HWND hwndDetailsLV,
                      UndoList& UndoList);

        virtual ~AddUserDialog(VOID);

        HRESULT Run(VOID);

    private:
        enum { iICON_USER_SINGLE, iICON_USER_MULTIPLE, cUSER_ICONS };

        LONGLONG           m_cVolumeMaxBytes;
        LONGLONG           m_llQuotaLimit;
        LONGLONG           m_llQuotaThreshold;
        PDISKQUOTA_CONTROL m_pQuotaControl;
        UndoList&          m_UndoList;
        HINSTANCE          m_hInstance;
        HWND               m_hwndParent;
        HWND               m_hwndDetailsLV;
        DS_SELECTION_LIST *m_pSelectionList;
        CLIPFORMAT         m_cfSelectionList;
        CVolumeID          m_idVolume;
        HICON              m_hIconUser[cUSER_ICONS];     // 0=Single, 1=Multi-user.
        XBytes            *m_pxbQuotaLimit;
        XBytes            *m_pxbQuotaThreshold;

        INT_PTR OnInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam);
        INT_PTR OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);
        INT_PTR OnHelp(HWND hDlg, WPARAM wParam, LPARAM lParam);
        INT_PTR OnContextMenu(HWND hwndItem, int xPos, int yPos);
        INT_PTR OnEditNotifyUpdate(HWND hDlg, WPARAM wParam, LPARAM lParam);
        INT_PTR OnComboNotifySelChange(HWND hDlg, WPARAM wParam, LPARAM lParam);
        INT_PTR OnOk(HWND hDlg, WPARAM wParam, LPARAM lParam);
        HRESULT ApplySettings(HWND hDlg, bool bUndo = true);
        HRESULT BrowseForUsers(HWND hwndParent, IDataObject **ppdtobj);

        LPCWSTR GetDsSelUserName(const DS_SELECTION& sel);
        HRESULT GetDsSelUserSid(const DS_SELECTION& sel, LPBYTE pbSid, int cbSid);
        HRESULT HexCharsToByte(LPTSTR pszByte, LPBYTE pb);

        static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

        //
        // Prevent copy.
        //
        AddUserDialog(const AddUserDialog&);
        void operator = (const AddUserDialog&);
};


#endif // __DSKQUOTA_ADDUSER_DIALOG_H
