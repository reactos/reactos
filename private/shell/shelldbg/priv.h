#ifndef _PRIV_H_
#define _PRIV_H_

// This stuff must run on Win95
#define _WIN32_WINDOWS      0x0400

#ifndef WINVER
#define WINVER              0x0400
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <debug.h>
#include <shlwapi.h>
#include <ccstock.h>
#include <shlguid.h>
#include <shdguid.h>
#include <shguidp.h>
#include <shlobj.h>
#include "dbutil.h"
#include "qistub.h"

#ifndef SIZEOF
#define SIZEOF(a)       sizeof(a)
#endif

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

// dump flags
#define DF_DEBUGQI      0x00000001

// trace flags
#define TF_SHDLIFE      0x00000001

#define TF_QISTUB   TF_SHDLIFE

#endif // _PRIV_H
