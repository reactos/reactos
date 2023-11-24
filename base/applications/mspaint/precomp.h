/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    The precompiled header
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
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

#include "resource.h"
#include "common.h"
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
