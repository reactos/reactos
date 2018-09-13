#ifndef _MNDDLG_H
#define _MNDDLG_H

class CMapNetDriveMRU
{
public:
    CMapNetDriveMRU();
    
    BOOL IsValid() {return (NULL != m_hMRU);}

    BOOL FillCombo(HWND hwndCombo);

    BOOL AddString(LPCTSTR psz);

    ~CMapNetDriveMRU();

private:
    static const DWORD c_nMaxMRUItems;
    static const TCHAR c_szMRUSubkey[];
    
    HANDLE m_hMRU;

    static int CompareProc(LPCTSTR lpsz1, LPCTSTR lpsz2);
};

class CMapNetDrivePage: public CPropertyPage
{
public:
    CMapNetDrivePage(LPCONNECTDLGSTRUCT pConnectStruct, DWORD* pdwLastError): 
      m_pConnectStruct(pConnectStruct),
      m_pdwLastError(pdwLastError)
      {*m_pdwLastError = WN_SUCCESS; m_szDomainUser[0] = m_szPassword[0] = TEXT('\0');}

protected:
    // Message handlers
    INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh);
    BOOL OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem);
    BOOL OnDestroy(HWND hwnd);

    // Utility fn's
    void SetAdvancedLinkText(HWND hwnd);
    void EnableReconnect(HWND hwnd);
    BOOL ReadShowAdvanced();
    void WriteShowAdvanced(BOOL fShow);
    BOOL ReadReconnect();
    void WriteReconnect(BOOL fReconnect);
    void FillDriveBox(HWND hwnd);
    BOOL MapDrive(HWND hwnd);
    BOOL QueryDSForFolder(HWND hwndDlg, TCHAR* szFolderOut, DWORD cchFolderOut);
private:
    BOOL m_fAdvancedExpanded;
    LPCONNECTDLGSTRUCT m_pConnectStruct;
    DWORD* m_pdwLastError;

    // Hold results of the "connect as" dialog
    TCHAR m_szDomainUser[MAX_DOMAIN + MAX_USER + 2];
    TCHAR m_szPassword[MAX_PASSWORD + 1];

    // MRU list
    CMapNetDriveMRU m_MRU;
};

struct MapNetThreadData
{
    HWND hwnd;
    TCHAR szDomainUser[MAX_DOMAIN + MAX_USER + 2];
    TCHAR szPassword[MAX_PASSWORD + 1];
    TCHAR szPath[MAX_PATH + 1];
    TCHAR szDrive[3];
    BOOL fReconnect;
    HANDLE hEventCloseNow;
};

class CMapNetProgress: public CDialog
{
public:
    CMapNetProgress(MapNetThreadData* pdata, DWORD* pdwDevNum, DWORD* pdwLastError):
      m_pdata(pdata),
      m_pdwDevNum(pdwDevNum),
      m_pdwLastError(pdwLastError)
      {}

    ~CMapNetProgress()
    {if (m_hEventCloseNow != NULL) CloseHandle(m_hEventCloseNow);}

protected:
    // Message handlers
    INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    BOOL OnMapSuccess(HWND hwnd, DWORD dwDevNum, DWORD dwLastError);

    // Thread
    static DWORD WINAPI MapDriveThread(LPVOID pvoid);
    static BOOL MapDriveHelper(MapNetThreadData* pdata, DWORD* pdwDevNum, DWORD* pdwLastError);
    static BOOL ConfirmDisconnectDrive(HWND hWndDlg, LPCTSTR lpDrive, LPCTSTR lpShare, DWORD dwType);
    static BOOL ConfirmDisconnectOpenFiles(HWND hWndDlg);

private:
    // data
    MapNetThreadData* m_pdata;    

    DWORD* m_pdwDevNum;
    DWORD* m_pdwLastError;

    HANDLE m_hEventCloseNow;
};

class CConnectAsDlg: public CDialog
{
public:
    CConnectAsDlg(TCHAR* pszDomainUser, DWORD cchDomainUser, TCHAR* pszPassword, 
        DWORD cchPassword):
      m_pszDomainUser(pszDomainUser),
      m_cchDomainUser(cchDomainUser),
      m_pszPassword(pszPassword),
      m_cchPassword(cchPassword)
      {}

    INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

private:
    TCHAR* m_pszDomainUser;
    DWORD m_cchDomainUser;

    TCHAR* m_pszPassword;
    DWORD m_cchPassword;
};

#endif // !_MNDDLG_H
