///////////////////////////////////////////////////
//
// main.h
//				main.cpp's lumber room :)
///////////////////////////////////////////////////

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#include <package.h>
#include "resource.h"

/* Some Variables */

int selected, splitter_pos = 50;

pTree tree;
HMENU hPopup;
HACCEL hHotKeys;
HWND hTBar, hTree, hEdit, hStatus;
HTREEITEM nodes [MAXNODES];

/* Window Callbacks */

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK StatusProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK OptionsProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

/* Prototypes */

void Help (void);

int AddItem (int id, const char* name, int parent, int icon);
int SetText (const char* text);
int SetStatus (int status1, int status2, WCHAR* text);
int Ask (const WCHAR* message);

/* Toolbar Releated */

#define TBSTYLE_FLAT 2048

// This is the struct where the toolbar is defined
extern const TBBUTTON Buttons [];

