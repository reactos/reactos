//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       ispudlg.hxx
//
//  Contents:   Microsoft Internet Security Office Helper
//
//  History:    14-Aug-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef ISPUDLG_HXX
#define ISPUDLG_HXX

class ISPUdlg_
{
    public:
            ISPUdlg_(HWND hWndParent, HINSTANCE hInst, DWORD dwDialogId);
            virtual ~ISPUdlg_(void);

            HRESULT         Invoke(void);

            void            ShowError(HWND hWnd, DWORD dwStringId, DWORD dwTitleId);

            HWND            MyWindow(void) { return(m_hWndMe); }
            HWND            ParentWindow(void) { return(m_hWndParent); }

            void            StartShowProcessing(DWORD dwDialogId, DWORD dwTextControlId, DWORD dwStringId);
            void            ChangeShowProcessing(DWORD dwTextControlId, DWORD dwStirngId);
            void            EndShowProcessing(void);

            virtual BOOL    OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    protected:
            HRESULT     m_hrResult;

            virtual BOOL    OnCommand(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) { return(FALSE); }
            virtual BOOL    OnInitDialog(HWND hWnd, WPARAM wParam, LPARAM lParam) { return(FALSE); }
            virtual BOOL    OnOK(HWND hWnd);
            virtual BOOL    OnCancel(HWND hWnd);
            virtual BOOL    OnHelp(HWND hWnd, WPARAM wParam, LPARAM lParam) { return(FALSE); }

            virtual void    Center(HWND hWnd2Center = NULL);

            void            SetItemText(DWORD dwControlId, WCHAR *pwszText);
            BOOL            GetItemText(DWORD dwControlId, WCHAR **ppwszText);

    private:
            HWND        m_hWndMe;
            HWND        m_hWndParent;
            HWND        m_hDlgProcessing;
            HINSTANCE   m_hInst;
            DWORD       m_dwDialogId;

};

#endif // ISPUDLG_HXX

