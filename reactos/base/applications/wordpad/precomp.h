#ifndef __WORDPAD_PRECOMP_H
#define __WORDPAD_PRECOMP_H

//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h> /* GET_X/Y_LPARAM */
#include <stdio.h>
#include <tchar.h>
#include <richedit.h>
#include <commctrl.h>
#include "resource.h"

/* FIXME - add to headers !!! */
#ifndef SB_SIMPLEID
#define SB_SIMPLEID 0xFF
#endif

#define MAX_KEY_LENGTH 256

#define DOC_TYPE_RICH_TEXT      0
#define DOC_TYPE_UNICODE_TEXT   1
#define DOC_TYPE_TEXT           2

#ifdef _MSC_VER
#pragma warning(disable : 4100)
#endif

/* generic definitions and forward declarations */
struct _MAIN_WND_INFO;
struct _EDIT_WND_INFO;

typedef enum _MDI_EDITOR_TYPE {
    metUnknown = 0,
    metImageEditor,
} MDI_EDITOR_TYPE, *PMDI_EDITOR_TYPE;


/* wordpad.c */
extern HINSTANCE hInstance;
extern HANDLE ProcessHeap;


/* editwnd.c */
typedef struct _OPEN_EDIT_INFO
{
    BOOL CreateNew;
    union
    {
        UINT DocType;       /* new */
        LPTSTR lpDocumentPath; /* open */
    };
    LPTSTR lpDocumentName;

} OPEN_EDIT_INFO, *POPEN_EDIT_INFO;

typedef struct _EDIT_WND_INFO
{
    MDI_EDITOR_TYPE MdiEditorType; /* Must be first member! */

    HWND hSelf;
    HWND hEdit;
    struct _MAIN_WND_INFO *MainWnd;
    struct _EDIT_WND_INFO *Next;
    POINT ScrollPos;
    USHORT Zoom;

    POPEN_EDIT_INFO OpenInfo; /* Only valid during initialization */

    LONG Width;
    LONG Height;

} EDIT_WND_INFO, *PEDIT_WND_INFO;

BOOL InitEditWindowImpl(VOID);
VOID UninitEditWindowImpl(VOID);
BOOL CreateEditWindow(struct _MAIN_WND_INFO *MainWnd,
                      POPEN_EDIT_INFO OpenInfo);
VOID SetEditorEnvironment(PEDIT_WND_INFO Info,
                          BOOL Setup);


/* mainwnd.c */
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

    /* Editors */
    PEDIT_WND_INFO ImageEditors;
    UINT ImagesCreated;
    PVOID ActiveEditor;

    /* status flags */
    UINT InMenuLoop : 1;
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



/* misc.c */
INT AllocAndLoadString(OUT LPTSTR *lpTarget,
                       IN HINSTANCE hInst,
                       IN UINT uID);

DWORD LoadAndFormatString(IN HINSTANCE hInstance,
                          IN UINT uID,
                          OUT LPTSTR *lpTarget,
                          ...);

BOOL StatusBarLoadAndFormatString(IN HWND hStatusBar,
                                  IN INT PartId,
                                  IN HINSTANCE hInstance,
                                  IN UINT uID,
                                  ...);

BOOL StatusBarLoadString(IN HWND hStatusBar,
                         IN INT PartId,
                         IN HINSTANCE hInstance,
                         IN UINT uID);

INT GetTextFromEdit(OUT LPTSTR lpString,
                    IN HWND hDlg,
                    IN UINT Res);

VOID GetError(DWORD err);


/* opensave.c */
VOID FileInitialize(HWND hwnd);
BOOL DoOpenFile(HWND hwnd, LPTSTR lpFileName, LPTSTR lpName);
BOOL DoSaveFile(HWND hwnd);

/* about.c */
INT_PTR CALLBACK
AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK
NewDocSelDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


#endif /* __WORDPAD_PRECOMP_H */
