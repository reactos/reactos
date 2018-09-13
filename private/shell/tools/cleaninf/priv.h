#ifndef _PRIV_H_
#define _PRIV_H_

#include <windows.h>
#include <windowsx.h>

#include <ccstock.h>
#include <debug.h>

#ifdef __cplusplus
#include <shstr.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include "resource.h"

#include "parse.h"

#ifdef __cplusplus
extern "C" {
#endif

// Helper functions
// (Don't link to shlwapi.dll so this is a stand-alone tool)
//
void PathUnquoteSpaces(LPTSTR lpsz);
BOOL StrTrim(LPSTR  pszTrimMe, LPCSTR pszTrimChars);


// Trace and Dump flags
#define BF_ONOPEN           0x00000010


// Parse file flags
#define PFF_WHITESPACE      0x00000001
#define PFF_INF             0x00000002
#define PFF_HTML            0x00000004
#define PFF_JS              0x00000008
#define PFF_HTC             0x00000010


#ifdef __cplusplus
}
#endif

#endif // _PRIV_H_

