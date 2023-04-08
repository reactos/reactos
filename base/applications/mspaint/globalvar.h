/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/globalvar.h
 * PURPOSE:     Declaring global variables for later initialization
 * PROGRAMMERS: Benedikt Freisen
 */

#pragma once

/* VARIABLES declared in main.cpp ***********************************/

extern BOOL askBeforeEnlarging;

extern POINT start;
extern POINT last;

extern HINSTANCE hProgInstance;

extern TCHAR filepathname[MAX_LONG_PATH];
extern BOOL isAFile;
extern BOOL imageSaved;

extern BOOL showGrid;

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

extern HWND hStatusBar;
extern float g_xDpi;
extern float g_yDpi;
extern INT fileSize;
extern SYSTEMTIME fileTime;

extern CFullscreenWindow fullscreenWindow;
extern CMiniatureWindow miniature;
extern CToolBox toolBoxContainer;
extern CToolSettingsWindow toolSettingsWindow;
extern CPaletteWindow paletteWindow;
extern CCanvasWindow canvasWindow;
extern CTextEditWindow textEditWindow;
