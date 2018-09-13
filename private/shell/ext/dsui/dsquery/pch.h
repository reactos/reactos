#ifndef _pch_h
#define _pch_h

#include "windows.h"
#include "windowsx.h"
#include "winspool.h"
#include "commctrl.h"
#include "commdlg.h"
#include "shellapi.h"
#include "multimon.h"
#include "shlobj.h"
#include "shlobjp.h"
#include "shlwapi.h"
#include "shlwapip.h"
#include "atlbase.h"

#include "winuserp.h"
#include "comctrlp.h"
#include "shsemip.h"
#include "shlapip.h"
#include "shellp.h"
#include "htmlhelp.h"
#include "cfdefs.h"

#include "activeds.h"
#include "iadsp.h"

#include "dsclient.h"
#include "dsclintp.h"
#include "cmnquery.h"
#include "cmnquryp.h"
#include "dsquery.h"
#include "dsqueryp.h"

#include "common.h"
#include "resource.h"
#include "dialogs.h"
#include "iids.h"
#include "cstrings.h"

#include "query.h"
#include "forms.h"
#include "io.h"
#include "helpids.h"

//
// Magic debug flags
//

#define TRACE_CORE          0x00000001
#define TRACE_HANDLER       0x00000002
#define TRACE_FORMS         0x00000004
#define TRACE_SCOPES        0x00000008
#define TRACE_UI            0x00000010
#define TRACE_VIEW          0x00000020
#define TRACE_QUERYTHREAD   0x00000040
#define TRACE_DATAOBJ       0x00000080
#define TRACE_CACHE         0x00000100
#define TRACE_MENU          0x00000200
#define TRACE_IO            0x00000400
#define TRACE_VIEWMENU      0x00000800
#define TRACE_PWELL         0x00001000
#define TRACE_FIELDCHOOSER  0x00002000
#define TRACE_PBAG          0x00004000

#define TRACE_ALWAYS        0xffffffff          // use with caution

//
// constants
//

#define DSQUERY_HELPFILE   TEXT("dsclient.hlp")
#define DS_POLICY          TEXT("Software\\Policies\\Microsoft\\Windows\\Directory UI")

extern HINSTANCE g_hInstance; 
#define GLOBAL_HINSTANCE (g_hInstance)

STDAPI CDsFind_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI CPropertyBag_CreateInstance(REFIID riid, void **ppv);


#endif
