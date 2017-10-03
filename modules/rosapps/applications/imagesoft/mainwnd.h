
typedef struct _MENU_HINT
{
    WORD CmdId;
    UINT HintId;
} MENU_HINT, *PMENU_HINT;

typedef struct _MAIN_WND_INFO
{
    HWND hSelf;
    HWND hMdiClient;
    HWND hStatus;
    int nCmdShow;

    struct _FLT_WND *fltTools;
    struct _FLT_WND *fltColors;
    struct _FLT_WND *fltHistory;

    struct _TOOLBAR_DOCKS ToolDocks;

    /* Editors */
    PEDIT_WND_INFO ImageEditors;
    UINT ImagesCreated;

    PVOID ActiveEditor;

    /* status flags */
    BOOL InMenuLoop : 1;
} MAIN_WND_INFO, *PMAIN_WND_INFO;

BOOL InitMainWindowImpl(VOID);
VOID UninitMainWindowImpl(VOID);
HWND CreateMainWindow(LPCTSTR lpCaption,
                      int nCmdShow);
BOOL MainWndTranslateMDISysAccel(HWND hwnd,
                                 LPMSG lpMsg);
VOID MainWndSwitchEditorContext(PMAIN_WND_INFO Info,
                                HWND hDeactivate,
                                HWND hActivate);
MDI_EDITOR_TYPE MainWndGetCurrentEditor(PMAIN_WND_INFO MainWnd,
                                        PVOID *Info);
