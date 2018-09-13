#ifndef _pch_h
#define _pch_h

#include "windows.h"
#include "windowsx.h"
#include "winuserp.h"
#include "commctrl.h"
#include "shellapi.h"
#include "multimon.h"
#include "shlobj.h"
#include "shlobjp.h"
#include "shlwapi.h"
#include "shlwapip.h"
#include "atlbase.h"

#include "comctrlp.h"
#include "shsemip.h"
#include "shlapip.h"
#include "shellp.h"
#include "cfdefs.h"

#include "common.h"
#include "cmnquery.h"
#include "cmnquryp.h"

#include "resource.h"
#include "thunk.h"


//
// instance and core helpers
//

extern HINSTANCE g_hInstance; 
#define GLOBAL_HINSTANCE (g_hInstance)

STDAPI CCommonQuery_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);


//
// Magic debug flags
//

#define TRACE_CORE          0x00000001
#define TRACE_QUERY         0x00000002
#define TRACE_FORMS         0x00000004
#define TRACE_SCOPES        0x00000008
#define TRACE_FRAME         0x00000010
#define TRACE_FRAMEDLG      0x00000020
#define TRACE_PERSIST       0x00000040


//
// Conditionals used to build
//

#define HIDE_SEARCH_PANE    FALSE

#endif
