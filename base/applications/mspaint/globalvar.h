/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Declaring global variables for later initialization
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 */

#pragma once

/* VARIABLES declared in main.cpp ***********************************/

extern BOOL g_askBeforeEnlarging;

extern POINT g_ptStart, g_ptEnd;

extern HINSTANCE g_hinstExe;

extern WCHAR g_szFileName[MAX_LONG_PATH];
extern BOOL g_isAFile;
extern BOOL g_imageSaved;
extern BOOL g_showGrid;

extern CMainWindow mainWindow;

/* VARIABLES declared in dialogs.cpp ********************************/

extern CMirrorRotateDialog mirrorRotateDialog;
extern CAttributesDialog attributesDialog;
extern CStretchSkewDialog stretchSkewDialog;
extern CFontsDialog fontsDialog;

/* VARIABLES declared in the other places ***************************/

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
