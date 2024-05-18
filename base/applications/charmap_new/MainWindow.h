#pragma once
#include "GridView.h"

class CCharMapWindow
{
    HWND m_hMainWnd;
    HWND m_hStatusBar;
    int m_CmdShow;
    HMODULE m_hRichEd;

    CGridView *m_GridView;

public:
    CCharMapWindow(void);
    ~CCharMapWindow(void);

    bool Create(
        _In_ HINSTANCE hInst,
        _In_ int nCmdShow
        );

private:
    static INT_PTR CALLBACK DialogProc(
        _In_ HWND   hwndDlg,
        _In_ UINT   uMsg,
        _In_ WPARAM wParam,
        _In_ LPARAM lParam
        );

    bool Initialize(
        _In_z_ LPCTSTR lpCaption,
        _In_ int nCmdShow
        );

    int Run();
    void Uninitialize(void);

    BOOL OnCreate(
        _In_ HWND hwnd
        );

    BOOL OnDestroy(void);
    BOOL OnSize(void);

    BOOL OnNotify(
        _In_ LPARAM lParam
        );

    BOOL OnContext(
        _In_ LPARAM lParam
        );

    BOOL OnCommand(
        _In_ WPARAM wParam,
        LPARAM lParam
        );

    bool CreateStatusBar(void);

    bool StatusBarLoadString(
        _In_ HWND hStatusBar,
        _In_ INT PartId,
        _In_ HINSTANCE hInstance,
        _In_ UINT uID
        );

    void UpdateStatusBar(
        _In_ bool InMenuLoop
        );

    static int CALLBACK
    EnumDisplayFont(
        ENUMLOGFONTEXW *lpelfe,
        NEWTEXTMETRICEXW *lpntme,
        DWORD FontType,
        LPARAM lParam
        );

    bool CreateFontComboBox(
        );

    bool ChangeMapFont(
        );
};
