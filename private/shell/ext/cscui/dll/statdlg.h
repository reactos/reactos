//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       statdlg.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCUI_STATDLG_H
#define _INC_CSCUI_STATDLG_H


#ifndef _CSCST_H_
#   include "cscst.h"
#endif

class CStatusDlg
{
    public:
        enum Modes { MODE_NORMAL = 0,
                     MODE_AUTOSYNC
                   };

        static int Create(HWND hwndParent, LPCTSTR pszText, eSysTrayState eState, Modes mode = MODE_NORMAL);

        static LPTSTR StrDup(LPCTSTR psz);

    private:
        CStatusDlg(HINSTANCE hInstance, LPCTSTR pszText, eSysTrayState eState, Modes mode = MODE_NORMAL);

        ~CStatusDlg(void);

        int Run(HWND hwndParent);

        void Destroy(void);

        enum { iIMAGELIST_ICON_SERVER = 0,
               iIMAGELIST_ICON_SERVER_OFFLINE,
               iIMAGELIST_ICON_SERVER_BACK,
               iIMAGELIST_ICON_SERVER_DIRTY
             };

        //
        // An entry in the listview.
        //
        class LVEntry
        {
            public:
                LVEntry(HINSTANCE hInstance, LPCTSTR pszServer, bool bConnectable);
                ~LVEntry(void);

                LPCTSTR Server(void) const
                    { return m_pszServer; }

                void GetStatusText(LPTSTR pszStatus, int cchStatus) const;

                void GetInfoText(LPTSTR pszInfo, int cchInfo) const;

                int GetImageIndex(void) const;

                bool AddShare(LPCTSTR pszShare, const CSCSHARESTATS& s, DWORD dwCscStatus);

                void GetStats(CSCSHARESTATS *ps, DWORD *pdwCscStatus) const;

                int GetShareCount(void) const;

                LPCTSTR GetShareName(int iShare) const;

                bool IsModified(void) const
                {
                    return (0 < m_stats.cModified);
                }

                bool IsConnected(void) const
                {
                    return (0 != (FLAG_CSC_SHARE_STATUS_CONNECTED & m_dwCscStatus));
                }

                bool IsOffline(void) const
                {
                    return (0 != (FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP & m_dwCscStatus));
                }

                bool IsConnectable(void) const
                {
                    return m_bConnectable;
                }
                //
                // Structure describing the display info for one listview entry.
                //
                struct DispInfo
                {
                    UINT idsStatusText;
                    UINT idsInfoText;
                    int  iImage;
                };

            private:
                //
                // Disp Info flags used to control what's displayed for the
                // entry in the listview.
                //
                enum { DIF_MODIFIED  = 0x00000001,
                       DIF_AVAILABLE = 0x00000002,
                       DIF_ONLINE    = 0x00000004 };

                HINSTANCE      m_hInstance;
                CSCSHARESTATS  m_stats;
                DWORD          m_dwCscStatus;
                LPTSTR         m_pszServer;
                HDPA           m_hdpaShares;
                mutable int    m_iDispInfo;    // Index into s_rgDispInfo[]
                bool           m_bConnectable;

                static const TCHAR s_szBlank[];
                static const DispInfo s_rgDispInfo[];

                int GetDispInfoIndex(void) const;

                //
                // Prevent copy.
                //
                LVEntry(const LVEntry& rhs);
                LVEntry& operator = (const LVEntry& rhs);
        };

        enum { iLVSUBITEM_SERVER = 0,
               iLVSUBITEM_STATUS,
               iLVSUBITEM_INFO
             };

        HINSTANCE     m_hInstance;
        HWND          m_hwndDlg;
        HWND          m_hwndLV;
        HIMAGELIST    m_himl;
        LPTSTR        m_pszText;
        Modes         m_mode;           // MODE_NORMAL etc.
        eSysTrayState m_eSysTrayState;
        int           m_cyExpanded;
        int           m_iLastColSorted;
        bool          m_bExpanded;
        bool          m_bSortAscending;
        
        BOOL OnInitDialog(WPARAM wParam, LPARAM lParam);
        BOOL OnCommand(WPARAM wParam, LPARAM lParam);
        BOOL OnNotify(WPARAM wParam, LPARAM lParam);
        BOOL OnDestroy(void);
        void OnLVN_GetDispInfo(LV_DISPINFO *plvdi);
        void OnLVN_ColumnClick(NM_LISTVIEW *pnmlv);

        void CreateListColumns(void);
        HIMAGELIST CreateImageList(void);
        void FillListView(void);
        void DestroyLVEntries(void);
        LVEntry *CreateLVEntry(LPCTSTR pszServer, bool bConnectable);
        LVEntry *FindLVEntry(LPCTSTR pszServer);
        bool ShouldIncludeLVEntry(const CStatusDlg::LVEntry& entry);
        bool ShouldCheckLVEntry(const CStatusDlg::LVEntry& entry);
        void PrepListForDisplay(void);
        void EnableListviewCheckboxes(bool bEnable);

        static int CALLBACK CompareLVItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
        static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        void ExpandDialog(bool bExpand);
        void UpdateDetailsBtnTitle(void);
        void RememberUsersDialogSizePref(bool bExpanded);
        bool UserLikesDialogExpanded(void);
        HRESULT SynchronizeServers(void);
        HRESULT BuildFilenameList(CscFilenameList *pfnl);
};

BOOL TransitionShareOnline(LPCTSTR pszShare, BOOL  bShareIsAlive, BOOL  bCheckSpeed, DWORD dwPathSpeed);
HRESULT ReconnectServers(CscFilenameList *pfnl, BOOL bCheckForOpenFiles, BOOL bCheckSpeed);


#endif // _INC_CSCUI_STATDLG_H

