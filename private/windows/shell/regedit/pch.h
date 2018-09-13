/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       PCH.H
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        05 Mar 1994
*
*  Precompiled header for the Registry Editor.
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  05 Mar 1994 TCS Original implementation.
*  07 Apr 1994 TCS Moved the definitions associated to REGISTRY_ROOT to
*                  REGPORTE.H to be more easily shared by the real mode registry
*                  tool.
*
*******************************************************************************/

#ifndef _INC_PCH
#define _INC_PCH

#define STRICT
#define _INC_OLE
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <memory.h>
#include <regdef.h>
#include "regporte.h"
#include "regmisc.h"
#include "regdebug.h"

#define ARRAYSIZE(x)        (sizeof(x) / sizeof(x[0]))

#define IMAGEINDEX(x)                   ((x) - IDI_FIRSTIMAGE)

typedef struct _EDITVALUEPARAM {
    PTSTR pValueName;
    PBYTE pValueData;
    UINT cbValueData;
}   EDITVALUEPARAM, FAR *LPEDITVALUEPARAM;

//  Instance handle of this application.
extern HINSTANCE g_hInstance;

extern TCHAR g_NullString[];

//  TRUE if accelerator table should not be used, such as during a rename
//  operation.
extern BOOL g_fDisableAccelerators;

extern TCHAR g_KeyNameBuffer[MAXKEYNAME];
extern TCHAR g_ValueNameBuffer[MAXVALUENAME_LENGTH];
extern BYTE g_ValueDataBuffer[MAXDATA_LENGTH*sizeof(TCHAR)];

extern COLORREF g_clrWindow;
extern COLORREF g_clrWindowText;
extern COLORREF g_clrHighlight;
extern COLORREF g_clrHighlightText;

extern PTSTR g_pHelpFileName;

//  Association between the ASCII name and the handle of the registry key.
extern const REGISTRY_ROOT g_RegistryRoots[NUMBER_REGISTRY_ROOTS];

#endif // _INC_PCH
