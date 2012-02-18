#define WIN32_NO_STATUS
#include <windows.h>
#include <windowsx.h>

#define NTOS_MODE_USER
#include <ndk/cmfuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/rtlfuncs.h>

#include <setupapi.h>
#include <commctrl.h>
#include <stdio.h>
#include <io.h>
#include <tchar.h>
#include <stdlib.h>
#include <samlib/samlib.h>
#include <syssetup/syssetup.h>
#include <userenv.h>
#include <shlobj.h>
#include <objidl.h>
#include <shlwapi.h>
#include <string.h>
#include <pseh/pseh2.h>
#include <time.h>
#include <ntsecapi.h>

#include "globals.h"
#include "resource.h"
