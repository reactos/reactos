#ifndef __SETTINGS_FOLDER_DLL_H
#define __SETTINGS_FOLDER_DLL_H


#ifdef DM_ASSERT
# undef DM_ASSERT
#endif
#ifdef DM_WARNING
# undef DM_WARNING
#endif
#ifdef DM_ERROR
# undef DM_ERROR
#endif
#ifdef DM_TRACE
# undef DM_TRACE
#endif

#include "except.h"
#include "alloc.h"
#include "debug.h"
#include "utils.h"
#include "autolock.h"
#include "dynarray.h"
#include "gdiobj.h"
#include "resource.h"
#include "comobj.h"
#include "setfldx.h"
#include "msg.h"

#include "folder.h"


#ifndef ARRAYSIZE
#   define ARRAYSIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

//
// Declaration for callback used to launch tray property sheet in explorer.
//
typedef VOID (WINAPI *PTRAYPROPSHEETCALLBACK)(DWORD nStartPage);

#include "globals.h"

#endif // __SETTINGS_FOLDER_DLL_H

