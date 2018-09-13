#ifndef __PRECOMPILED_HEADER_H
#define __PRECOMPILED_HEADER_H

#define _WIN32_WINNT 0x0500

#include <windows.h>
#include <windowsx.h>

#include <winuserp.h>
#include <commctrl.h>
#include <shlwapi.h>

#include <regstr.h>
#include <crtdbg.h>
#include <tchar.h>
#include <stdio.h>

#define ARRAYSIZE(a)   (sizeof(a) / sizeof((a)[0]))

extern HINSTANCE g_hInstDll;

#ifdef _DEBUG
#define VERIFY(xxx) _ASSERT(xxx)
#else
#define VERIFY(xxx) xxx
#endif

// This is the maxinum number of options allowed on any one preview page
#define MAX_DISTINCT_VALUES 30

// Helper function is defined in AccWiz.cpp
void LoadArrayFromStringTable(int nIdString, int *rgnValues, int *pnCountValues);

// This function is defined in AccWiz.cpp
int StringTableMessageBox(HWND hWnd, int nText, int nCaption, UINT uType);

// Functions from PCH.CPP
void GetNonClientMetrics(NONCLIENTMETRICS *pncm, LOGFONT *plfIcon);
BOOL IsCurrentFaceNamesDifferent();


#endif // __PRECOMPILED_HEADER_H



