#pragma once
#include "DeviceView.h"

typedef struct _MENU_HINT
{
    WORD CmdId;
    UINT HintId;
} MENU_HINT, *PMENU_HINT;

class CDeviceManager
{
    CAtlStringW m_szMainWndClass;
    CDeviceView *m_DeviceView;
    HWND m_hMainWnd;
    HWND m_hStatusBar;
    HWND m_hToolBar;
    HMENU m_hMenu;
    HMENU m_hActionMenu;
    int m_CmdShow;
    bool m_RefreshPending;

public:
    CDeviceManager(void);
    ~CDeviceManager(void);

    bool Create(
        _In_ HWND hWndParent,
        _In_ HINSTANCE hInst,
        _In_opt_z_ LPCWSTR lpMachineName,
        _In_ int nCmdShow
        );

private:
    static LRESULT CALLBACK MainWndProc(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam
        );

    bool Initialize(
        _In_z_ LPCTSTR lpCaption,
        _In_ int nCmdShow
        );

    int Run();
    void Uninitialize(void);

    LRESULT OnCreate(
        _In_ HWND hwnd
        );

    LRESULT OnDestroy(void);
    LRESULT OnSize(void);

    LRESULT OnNotify(
        _In_ LPARAM lParam
        );

    LRESULT OnContext(
        _In_ LPARAM lParam
        );

    LRESULT OnCommand(
        _In_ WPARAM wParam,
        LPARAM lParam
        );

    bool CreateToolBar(void);
    bool CreateStatusBar(void);

    void UpdateToolbar(void);

    bool StatusBarLoadString(
        _In_ HWND hStatusBar,
        _In_ INT PartId,
        _In_ HINSTANCE hInstance,
        _In_ UINT uID
        );

    void UpdateStatusBar(
        _In_ bool InMenuLoop
        );

    bool MainWndMenuHint(
        _In_ WORD CmdId,
        _In_ const MENU_HINT *HintArray,
        _In_ DWORD HintsCount,
        _In_ UINT DefHintId
        );

    bool RefreshView(
        _In_ ViewType Type
        );
};

