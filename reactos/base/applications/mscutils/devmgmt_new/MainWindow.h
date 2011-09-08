#pragma once
class CMainWindow
{
    CAtlString m_szMainWndClass;
    HWND m_hMainWnd;
    int m_CmdShow;

private:
    static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    BOOL StatusBarLoadString(HWND hStatusBar, INT PartId, HINSTANCE hInstance, UINT uID);

public:
    CMainWindow(void);
    ~CMainWindow(void);

    BOOL Initialize(LPCTSTR lpCaption, int nCmdShow);
    INT Run();
    VOID Uninitialize();
};

