
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windowsx.h>
#include <prsht.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shsemip.h>
#include <stdlib.h>
#include <shlobjp.h>
#include <shellp.h>
#include <string.h>
#include <htmlhelp.h>
#include <stdlib.h>
#include <tchar.h>
#include <string.h>

#include <initguid.h>
#include <help.h>
#include <commctrl.h>

#include <winuser.h>
#include <winuserp.h>

#include "..\..\common\deskcplext.h"
#include "..\..\common\propsext.h"
#include "..\..\common\deskcmmn.h"

#include "resource.h"

//
// Avoid bringing in C runtime code for NO reason
//
#if defined(__cplusplus)
inline void * __cdecl operator new(size_t size) { return (void *)LocalAlloc(LPTR, size); }
inline void __cdecl operator delete(void *ptr) { LocalFree(ptr); }
extern "C" inline __cdecl _purecall(void) { return 0; }
#endif  // __cplusplus
