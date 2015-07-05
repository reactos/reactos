#pragma once
#include "DeviceView.h"

typedef struct _MENU_HINT
{
    WORD CmdId;
    UINT HintId;
} MENU_HINT, *PMENU_HINT;

class CMainWindow
{
    CAtlStringW m_szMainWndClass;
    CDeviceView *m_DeviceView;
    HWND m_hMainWnd;
    HWND m_hStatusBar;
    HWND m_hToolBar;
    HIMAGELIST m_ToolbarhImageList;
    HMENU m_hMenu;
    HMENU m_hActionMenu;
    int m_CmdShow;

public:
    CMainWindow(void);
    ~CMainWindow(void);

    bool Initialize(LPCTSTR lpCaption, int nCmdShow);
    int Run();
    void Uninitialize();

private:
    static LRESULT CALLBACK MainWndProc(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam
        );

    LRESULT OnCreate(HWND hwnd);
    LRESULT OnDestroy();
    LRESULT OnSize();
    LRESULT OnNotify(LPARAM lParam);
    LRESULT OnContext(LPARAM lParam);
    LRESULT OnCommand(WPARAM wParam, LPARAM lParam);

    bool CreateToolBar();
    bool CreateStatusBar();

    void UpdateToolbar(
        _In_ LPTV_ITEMW TvItem
        );

    bool StatusBarLoadString(
        HWND hStatusBar,
        INT PartId,
        HINSTANCE hInstance,
        UINT uID
        );

    void UpdateStatusBar(
        _In_ bool InMenuLoop
        );

    bool MainWndMenuHint(
        WORD CmdId,
        const MENU_HINT *HintArray,
        DWORD HintsCount,
        UINT DefHintId
        );

    bool RefreshView(
        ViewType Type
        );
};

