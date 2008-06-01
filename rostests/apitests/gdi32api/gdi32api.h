#ifndef _GDITEST_H
#define _GDITEST_H

#define WIN32_NO_STATUS
#include <windows.h>
#include <windows.h>
#include <ndk/ntndk.h>
#include <wingdi.h>
#include <winddi.h>
#include <d3dnthal.h>
#include <prntfont.h>

/* Public Win32K Headers */
#include <win32k/ntgdityp.h>
#include <ntgdi.h>
#include <win32k/ntgdihdl.h>

#include "../apitest.h"
#include "gdi.h"

extern HINSTANCE g_hInstance;
extern PGDI_TABLE_ENTRY GdiHandleTable;

#endif /* _GDITEST_H */

/* EOF */
