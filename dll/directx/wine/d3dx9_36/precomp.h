
#pragma once

#include <wine/config.h>
#include <wine/port.h>

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <usp10.h>

#include <wine/winternl.h>

#include <wine/list.h>
#include <wine/unicode.h>

#include "d3dx9_private.h"
#include "txc_dxtn.h"

#include <dxfile.h>
#include <rmxfguid.h>

#include <d3dcommon.h>
#include <d3dcompiler.h>

#include <ole2.h>
#include <wincodec.h>
