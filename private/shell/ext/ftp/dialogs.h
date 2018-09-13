/*****************************************************************************\
    FILE: Dialogs.h

    DESCRIPTION:
        This file exists to display dialogs needed during FTP operations.
\*****************************************************************************/
 
#ifndef _DIALOGS_H
#define _DIALOGS_H


#ifdef ADD_ABOUTBOX
HRESULT DisplayAboutBox(HWND hWnd);
#endif // ADD_ABOUTBOX
HRESULT BrowseForDir(HWND hwndParent, LPCTSTR pszTitle, LPCITEMIDLIST pidlDefaultSelect, LPITEMIDLIST * ppidlSelected);

/*****************************************************************************\
    Class: CDownloadDialog

    DESCRIPTION:
        Display the Downoad Dialog to select a directory to download into.
\*****************************************************************************/

class CDownloadDialog
{
public:
    CDownloadDialog();
    ~CDownloadDialog(void);

    // Public Member Functions
    HRESULT ShowDialog(HWND hwndOwner, LPTSTR pszDir, DWORD cchSize, DWORD * pdwDownloadType);

    static INT_PTR CALLBACK DownloadDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);

protected:
    // Private Member Variables
    HWND            m_hwnd; 
    LPTSTR          m_pszDir;
    DWORD           m_dwDownloadType;

    // Private Member Functions
    BOOL _DownloadDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
    BOOL _OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);
    BOOL _InitDialog(HWND hDlg);
    HRESULT _DownloadButton(HWND hDlg);
    void _BrowseButton(HWND hDlg);
};


#endif // _DIALOGS_H
