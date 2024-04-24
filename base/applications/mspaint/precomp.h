/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    The precompiled header
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 *             Copyright 2018 Stanislav Motylkov <x86corez@gmail.com>
 *             Copyright 2021-2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#ifdef NDEBUG
    #undef DBG
    #undef _DEBUG
#endif

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <tchar.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlpath.h>
#include <atlstr.h>
#include <atlwin.h>
#include <atltypes.h>
#include <windowsx.h>
#include <commdlg.h>
#include <commctrl.h>
#include <stdlib.h>
#define _USE_MATH_DEFINES /* for M_PI */
#include <math.h>
#include <shellapi.h>
#include <htmlhelp.h>
#include <strsafe.h>
#include <ui/CWaitCursor.h>

#include <debug.h>

/* CONSTANTS *******************************************************/

#define GRIP_SIZE   3
#define MIN_ZOOM    125
#define MAX_ZOOM    8000

#define MAX_LONG_PATH 512

#define WM_TOOLSMODELTOOLCHANGED         (WM_APP + 0)
#define WM_TOOLSMODELSETTINGSCHANGED     (WM_APP + 1)
#define WM_TOOLSMODELZOOMCHANGED         (WM_APP + 2)
#define WM_PALETTEMODELCOLORCHANGED      (WM_APP + 3)

enum HITTEST // hit
{
    HIT_NONE = 0, // Nothing hit or outside
    HIT_UPPER_LEFT,
    HIT_UPPER_CENTER,
    HIT_UPPER_RIGHT,
    HIT_MIDDLE_LEFT,
    HIT_MIDDLE_RIGHT,
    HIT_LOWER_LEFT,
    HIT_LOWER_CENTER,
    HIT_LOWER_RIGHT,
    HIT_BORDER,
    HIT_INNER,
};

/* COMMON FUNCTIONS *************************************************/

void ShowOutOfMemory(void);
BOOL nearlyEqualPoints(INT x0, INT y0, INT x1, INT y1);
BOOL OpenMailer(HWND hWnd, LPCWSTR pszPathName);
void getBoundaryOfPtStack(RECT& rcBoundary, INT cPoints, const POINT *pPoints);

#define DEG2RAD(degree) (((degree) * M_PI) / 180)
#define RAD2DEG(radian) ((LONG)(((radian) * 180) / M_PI))

/* This simplifies checking and unchecking menu items */
#define CHECKED_IF(bChecked) \
    ((bChecked) ? (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND))

/* This simplifies enabling or graying menu items */
#define ENABLED_IF(bEnabled) \
    ((bEnabled) ? (MF_ENABLED | MF_BYCOMMAND) : (MF_GRAYED | MF_BYCOMMAND))

template <typename T>
inline void Swap(T& a, T& b)
{
    T tmp = a;
    a = b;
    b = tmp;
}

/* LOCAL INCLUDES ***************************************************/

#include "resource.h"
#include "drawing.h"
#include "dib.h"
#include "fullscreen.h"
#include "history.h"
#include "miniature.h"
#include "palette.h"
#include "palettemodel.h"
#include "registry.h"
#include "selectionmodel.h"
#include "sizebox.h"
#include "canvas.h"
#include "textedit.h"
#include "toolbox.h"
#include "toolsettings.h"
#include "toolsmodel.h"
#include "main.h"
#include "dialogs.h"
#include "atlimagedx.h"

/* GLOBAL VARIABLES *************************************************/

extern HINSTANCE g_hinstExe;

extern WCHAR g_szFileName[MAX_LONG_PATH];
extern BOOL g_isAFile;
extern BOOL g_imageSaved;
extern BOOL g_showGrid;
extern BOOL g_askBeforeEnlarging;

extern CMainWindow mainWindow;

extern CMirrorRotateDialog mirrorRotateDialog;
extern CAttributesDialog attributesDialog;
extern CStretchSkewDialog stretchSkewDialog;
extern CFontsDialog fontsDialog;

extern RegistrySettings registrySettings;
extern ImageModel imageModel;
extern ToolsModel toolsModel;
extern SelectionModel selectionModel;
extern PaletteModel paletteModel;

extern HWND g_hStatusBar;
extern float g_xDpi;
extern float g_yDpi;
extern INT g_fileSize;
extern SYSTEMTIME g_fileTime;

extern CFullscreenWindow fullscreenWindow;
extern CMiniatureWindow miniature;
extern CToolBox toolBoxContainer;
extern CToolSettingsWindow toolSettingsWindow;
extern CPaletteWindow paletteWindow;
extern CCanvasWindow canvasWindow;
extern CTextEditWindow textEditWindow;
