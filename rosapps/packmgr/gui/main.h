///////////////////////////////////////////////////
//
// main.h
//				main.cpp's lumber room :)
///////////////////////////////////////////////////

#include <windows.h>
#include <ntos/keyboard.h>
#include <commctrl.h>
#include <iostream>
#include <fstream>

#include <package.hpp>
#include "resource.h"

/* Some Variables */ 

int selected, splitter_pos = 50;

pTree tree;
HMENU hPopup;
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
TBBUTTON Buttons [] = 
{
	{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
	
	{0, 1, TBSTATE_INDETERMINATE, TBSTYLE_BUTTON, 0L, 0}, // No Action
	{1, 2, TBSTATE_INDETERMINATE, TBSTYLE_BUTTON, 0L, 0}, // Install
	{2, 3, TBSTATE_INDETERMINATE, TBSTYLE_BUTTON, 0L, 0}, // Install from source
	{3, 4, TBSTATE_INDETERMINATE, TBSTYLE_BUTTON, 0L, 0}, // Update
	{4, 5, TBSTATE_INDETERMINATE, TBSTYLE_BUTTON, 0L, 0}, // Unistall

	{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
	{5, 6, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}, // DoIt (tm)
	{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},

	{6, 7, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}, // Help
	{7, 8, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}, // Options

	{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
}; 
