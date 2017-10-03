#ifndef _GDIHV_H
#define _GDIHV_H

#include <tchar.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <commctrl.h>
#include <ndk/ntndk.h>
#include <psapi.h>

#include "gdi.h"
#include "mainwnd.h"
#include "proclist.h"
#include "handlelist.h"

#include "resource.h"

extern PGDI_TABLE_ENTRY GdiHandleTable;
extern HINSTANCE g_hInstance;

#endif //_GDIHV_H
