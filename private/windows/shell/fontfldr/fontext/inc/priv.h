#define _INC_OLE
#define CONST_VTABLE
#define DONT_WANT_SHELLDEBUG   1

#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlobjp.h>
#include <commctrl.h>
#include <ole2.h>

#include <tchar.h>

// define __FCN__ to enable the FileChangeNotify procession.
//
#define __FCN__

#ifdef _fstrcpy
#undef _fstrcpy
#endif
#ifdef _fstrcat
#undef _fstrcat
#endif
#ifdef _fstrlen
#undef _fstrlen
#endif

#define _fstrcpy lstrcpy
#define _fstrcat lstrcat
#define _fstrlen lstrlen

#pragma hdrstop





