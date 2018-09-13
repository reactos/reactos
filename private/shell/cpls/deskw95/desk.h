#ifndef _MAIN_H
#define _MAIN_H

///////////////////////////////////////////////////////////////////////////////
//
// DESK.H
//
// Central header for 32-bit DESK.CPL
// precompiled (don't put small, volatile stuff in here)
//
///////////////////////////////////////////////////////////////////////////////

#define USECOMM
#define OEMRESOURCE
#define STRICT

#ifdef WIN32
#define INC_OLE2
#define CONST_VTABLE
#endif

#include <windows.h>
#include <windowsx.h>
#include <dlgs.h>
#include <cpl.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shsemip.h>
#include <shellp.h>

#ifndef RC_INVOKED
#include <prsht.h>
#endif

#define PATHMAX MAX_PATH

#endif
