#ifndef __IMAGESOFT_PRECOMP_H
#define __IMAGESOFT_PRECOMP_H

//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h> /* GET_X/Y_LPARAM */
#include <stdio.h>
#include <tchar.h>
#include <commctrl.h>
#include "resource.h"

/* FIXME - add to headers !!! */
#ifndef SB_SIMPLEID
#define SB_SIMPLEID 0xFF
#endif
#ifndef RBBS_USECHEVRON
#define RBBS_USECHEVRON 0x200
#endif
#ifndef RBN_CHEVRONPUSHED
#define RBN_CHEVRONPUSHED (RBN_FIRST - 10)
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4100)
#endif

#define MAX_KEY_LENGTH 256
#define NUM_MAINTB_IMAGES 10
#define TB_BMP_WIDTH 16
#define TB_BMP_HEIGHT 16

#define TOOLS   0
#define COLORS  1
#define HISTORY 2

#define MONOCHROMEBITS  1
#define GREYSCALEBITS   8
#define PALLETEBITS     8
#define TRUECOLORBITS   24

#define PIXELS      0
#define CENTIMETERS 1
#define INCHES      2

#ifdef _MSC_VER
#pragma warning(disable : 4100)
#endif

/* generic definitions and forward declarations */
struct _MAIN_WND_INFO;
struct _EDIT_WND_INFO;
struct _FLT_WND;

typedef enum _MDI_EDITOR_TYPE {
    metUnknown = 0,
    metImageEditor,
} MDI_EDITOR_TYPE, *PMDI_EDITOR_TYPE;

/* about.c */
INT_PTR CALLBACK AboutDialogProc(HWND hDlg,
                                 UINT message,
                                 WPARAM wParam,
                                 LPARAM lParam);

/* imageprop.c */
typedef struct _IMAGE_PROP
{
    LPTSTR lpImageName;
    /* Canvas properties */
    USHORT Type;
    USHORT Unit;
    LONG Resolution;
    /* size of drawing area */
    LONG Width;
    LONG Height;
} IMAGE_PROP, *PIMAGE_PROP;

INT_PTR CALLBACK
ImagePropDialogProc(HWND hDlg,
                    UINT message,
                    WPARAM wParam,
                    LPARAM lParam);


/* imagesoft.c */
extern HINSTANCE hInstance;
extern HANDLE ProcessHeap;

/* imgedwnd.c */
typedef enum
{
    tSelect = 0,
    tMove,
    tLasso,
    tZoom,
    tMagicWand,
    tBrush,
    tEraser,
    tPencil,
    tColorPick,
    tStamp,
    tFill,
    tLine,
    tPolyline,
    tRectangle,
    tRoundRectangle,
    tPolygon,
    tElipse,
} TOOL;

typedef struct _OPEN_IMAGE_EDIT_INFO
{
    BOOL CreateNew;
    union
    {
        struct
        {
            LONG Width;
            LONG Height;
        } New;
        struct
        {
            LPTSTR lpImagePath;
        } Open;
    };
    LPTSTR lpImageName;
    USHORT Type;
    LONG Resolution;
} OPEN_IMAGE_EDIT_INFO, *POPEN_IMAGE_EDIT_INFO;

typedef struct _EDIT_WND_INFO
{
    MDI_EDITOR_TYPE MdiEditorType; /* Must be first member! */

    HWND hSelf;
    HBITMAP hBitmap;
    HDC hDCMem;
    struct _MAIN_WND_INFO *MainWnd;
    struct _EDIT_WND_INFO *Next;
    POINT ScrollPos;
    USHORT Zoom;
    DWORD Tool;

    POPEN_IMAGE_EDIT_INFO OpenInfo; /* Only valid during initialization */

    /* Canvas properties */
    USHORT Type;
    LONG Resolution;
    /* size of drawing area */
    LONG Width;
    LONG Height;

} EDIT_WND_INFO, *PEDIT_WND_INFO;


BOOL CreateImageEditWindow(struct _MAIN_WND_INFO *MainWnd,
                           POPEN_IMAGE_EDIT_INFO OpenInfo);
VOID SetImageEditorEnvironment(PEDIT_WND_INFO Info,
                               BOOL Setup);
BOOL InitImageEditWindowImpl(VOID);
VOID UninitImageEditWindowImpl(VOID);


/* tooldock.c */
typedef enum
{
    TOP_DOCK = 0,
    LEFT_DOCK,
    RIGHT_DOCK,
    BOTTOM_DOCK,
    NO_DOCK
} DOCK_POSITION;

typedef struct _DOCKBAR
{
    UINT BarId;
    LPCTSTR lpName;
    UINT DisplayTextId;
    DOCK_POSITION Position;
} DOCKBAR, *PDOCKBAR;

struct _TOOLBAR_DOCKS;

typedef BOOL (CALLBACK *PDOCKBAR_CREATECLIENT)(struct _TOOLBAR_DOCKS *TbDocks,
                                               const DOCKBAR *Dockbar,
                                               PVOID Context,
                                               HWND hParent,
                                               HWND *hwnd);
typedef BOOL (CALLBACK *PDOCKBAR_DESTROYCLIENT)(struct _TOOLBAR_DOCKS *TbDocks,
                                                const DOCKBAR *Dockbar,
                                                PVOID Context,
                                                HWND hwnd);
typedef BOOL (CALLBACK *PDOCKBAR_INSERTBAND)(struct _TOOLBAR_DOCKS *TbDocks,
                                             const DOCKBAR *Dockbar,
                                             PVOID Context,
                                             UINT *Index,
                                             LPREBARBANDINFO rbi);
typedef VOID (CALLBACK *PDOCKBAR_DOCKBAND)(struct _TOOLBAR_DOCKS *TbDocks,
                                           const DOCKBAR *Dockbar,
                                           PVOID Context,
                                           DOCK_POSITION DockFrom,
                                           DOCK_POSITION DockTo,
                                           LPREBARBANDINFO rbi);
typedef VOID (CALLBACK *PDOCKBAR_CHEVRONPUSHED)(struct _TOOLBAR_DOCKS *TbDocks,
                                                const DOCKBAR *Dockbar,
                                                PVOID Context,
                                                HWND hwndChild,
                                                LPNMREBARCHEVRON lpnm);

typedef struct _DOCKBAR_ITEM_CALLBACKS
{
    PDOCKBAR_CREATECLIENT CreateClient;
    PDOCKBAR_DESTROYCLIENT DestroyClient;
    PDOCKBAR_INSERTBAND InsertBand;
    PDOCKBAR_DOCKBAND DockBand;
    PDOCKBAR_CHEVRONPUSHED ChevronPushed;
} DOCKBAR_ITEM_CALLBACKS, *PDOCKBAR_ITEM_CALLBACKS;

typedef struct _DOCKBAR_ITEM
{
    struct _DOCKBAR_ITEM *Next;
    DOCKBAR DockBar;
    PVOID Context;
    HWND hWndTool;
    HWND hWndClient;
    DOCK_POSITION PrevDock;
    UINT PrevBandIndex;
    const DOCKBAR_ITEM_CALLBACKS *Callbacks;
} DOCKBAR_ITEM, *PDOCKBAR_ITEM;

typedef VOID (CALLBACK *PDOCKBAR_PARENTRESIZE)(PVOID Context,
                                               WORD cx,
                                               WORD cy);

#define DOCKS_COUNT 4
typedef struct _TOOLBAR_DOCKS
{
    HWND hParent;
    PVOID Context;
    HWND hRebar[DOCKS_COUNT];
    RECT rcRebar[DOCKS_COUNT];
    RECT rcClient;
    PDOCKBAR_ITEM Items;
    PDOCKBAR_PARENTRESIZE ParentResize;
    PDOCKBAR_ITEM Dragging;
    UINT DraggingBandId;
    TCHAR szTempText[255];
} TOOLBAR_DOCKS, *PTOOLBAR_DOCKS;

VOID TbdInitializeDocks(PTOOLBAR_DOCKS TbDocks,
                        HWND hWndParent,
                        PVOID Context,
                        PDOCKBAR_PARENTRESIZE ParentResizeProc);
INT TbdAdjustUpdateClientRect(PTOOLBAR_DOCKS TbDocks,
                              PRECT rcClient);
HDWP TbdDeferDocks(HDWP hWinPosInfo,
                   PTOOLBAR_DOCKS TbDocks);
BOOL TbdAddToolbar(PTOOLBAR_DOCKS TbDocks,
                   const DOCKBAR *Dockbar,
                   PVOID Context,
                   const DOCKBAR_ITEM_CALLBACKS *DockbarCallbacks);
BOOL TbdDockBarIdFromClientWindow(PTOOLBAR_DOCKS TbDocks,
                                  HWND hWndClient,
                                  UINT *Id);
BOOL TbdHandleNotifications(PTOOLBAR_DOCKS TbDocks,
                            LPNMHDR pnmh,
                            LRESULT *Result);
VOID TbdHandleEnabling(PTOOLBAR_DOCKS TbDocks,
                       HWND hWnd,
                       BOOL Enable);
VOID TbdHandleActivation(PTOOLBAR_DOCKS TbDocks,
                         HWND hWnd,
                         WPARAM *wParam,
                         LPARAM *lParam);
VOID TbdShowFloatingToolbars(PTOOLBAR_DOCKS TbDocks,
                             BOOL Show);
BOOL TbdInitImpl(VOID);
VOID TbdUninitImpl(VOID);

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

    struct _FLT_WND *fltTools;
    struct _FLT_WND *fltColors;
    struct _FLT_WND *fltHistory;

    TOOLBAR_DOCKS ToolDocks;

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

BOOL ToolbarDeleteControlSpace(HWND hWndToolbar,
                               const TBBUTTON *ptbButton);

typedef VOID (*ToolbarChangeControlCallback)(HWND hWndToolbar,
                                             HWND hWndControl,
                                             BOOL Vert);
VOID ToolbarUpdateControlSpaces(HWND hWndToolbar,
                                ToolbarChangeControlCallback ChangeCallback);

BOOL ToolbarInsertSpaceForControl(HWND hWndToolbar,
                                  HWND hWndControl,
                                  INT Index,
                                  INT iCmd,
                                  BOOL HideVertical);

HIMAGELIST InitImageList(UINT NumButtons,
                         UINT StartResource);

/* opensave.c */
VOID FileInitialize(HWND hwnd);
BOOL DoOpenFile(HWND hwnd,
                LPTSTR lpFileName,
                LPTSTR lpName);
BOOL DoSaveFile(HWND hwnd);

/* floattoolbar.c */
typedef struct _FLT_WND
{
    HWND hSelf;
    LPTSTR lpName;
    INT x;
    INT y;
    INT Width;
    INT Height;
    BOOL bOpaque;
} FLT_WND, *PFLT_WND;

BOOL FloatToolbarCreateToolsGui(PMAIN_WND_INFO Info);
BOOL FloatToolbarCreateColorsGui(PMAIN_WND_INFO Info);
BOOL FloatToolbarCreateHistoryGui(PMAIN_WND_INFO Info);
BOOL InitFloatWndClass(VOID);
VOID UninitFloatWndImpl(VOID);
BOOL ShowHideWindow(HWND hwnd);

/* font.c */
VOID FillFontStyleComboList(HWND hwndCombo);
VOID FillFontSizeComboList(HWND hwndCombo);

/* custcombo.c */
VOID MakeFlatCombo(HWND hwndCombo);

#endif /* __IMAGESOFT_PRECOMP_H */
