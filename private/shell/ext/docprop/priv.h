//--------------------------------------------------------------
// common user interface routines
//
//
//--------------------------------------------------------------

#ifndef _OLE_PROP_H_
#define _OLE_PROP_H_

#define STRICT
#define CONST_VTABLE

#include "nocrt.h"

#include <windows.h>
#include <commdlg.h>
#include <dlgs.h>       // commdlg IDs
#include <shellapi.h>
#include <commctrl.h>
#include <windowsx.h>
#include <shlobj.h>
#include <malloc.h>

#undef Assert
#include "debug.h"
#include "resource.h"

#include "offglue.h"
#include "plex.h"
#include "extdef.h"
#include "offcapi.h"
#include "proptype.h"
#include "propmisc.h"
#include "debug.h"
#include "internal.h"
#include "strings.h"
#include "propvar.h"


STDAPI_(void) DllAddRef();
STDAPI_(void) DllRelease();

extern HANDLE g_hmodThisDll;

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

#define VERBOSE

#ifndef DEBUG
#ifdef VERBOSE
#undef VERBOSE
#endif
#endif

#ifdef VERBOSE
#define MESSAGE(a) {OutputDebugString(a TEXT("\r\n"));}
#else
#define MESSAGE(a)
#endif

#endif
