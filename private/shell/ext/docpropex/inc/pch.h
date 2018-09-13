//-------------------------------------------------------------------------//
// pch.h : Precompiled header
//-------------------------------------------------------------------------//

#if !defined(__PCH_H__)
#define __PCH_H__

#define STRICT
#define _WIN32_WINNT 0x0500
#define _ATL_APARTMENT_THREADED

#ifdef DBG
#define _ENABLE_TRACING
#endif

#include <atlbase.h>
extern CComModule _Module;
#define HINST_RESDLL    _Module.GetResourceInstance()
#define HINST_THISDLL   _Module.GetModuleInstance()
#include <atlcom.h>
#include <atlctl.h>

#include <shlobj.h>
#include <shlobjp.h>    // OCHOST
#include <shlwapi.h>
#include <shlwapip.h>
#include <commctrl.h>
#include <comctrlp.h>
#include "ptdebug.h"
#include "ptutil.h"

#endif // __PCH_H__
