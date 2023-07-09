
typedef
struct COPY_SYMBOLS_DATA_STRUCT {
    HWND m_hwndDlgParent;
    char m_szSrcPath[_MAX_PATH * 2];
    char m_szDestPath[_MAX_PATH * 2];

    long m_lStopCopying;

    COPY_SYMBOLS_DATA_STRUCT() {
        m_hwndDlgParent = NULL;
        ZeroMemory(m_szSrcPath, sizeof(m_szSrcPath));
        ZeroMemory(m_szDestPath, sizeof(m_szDestPath));
        m_lStopCopying = FALSE;
    }
} * PCOPY_SYMBOLS_DATA_STRUCT;


INT_PTR CALLBACK Copying_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
