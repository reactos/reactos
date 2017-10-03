#pragma once

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <winuser.h>
#include <rpc.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <reactos/dll/devmgr/devmgr.h>
#include <cpl.h>

#define NDEBUG
#include <debug.h>

#define MAX_STR_SIZE 255

extern HINSTANCE hApplet;

/* EOF */
