/*
 * PROJECT:     ReactOS msutb.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Language Bar (Tipbar)
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include <stdlib.h>

#define WIN32_NO_STATUS
#define COBJMACROS
#define INITGUID

#include <windows.h>
#include <oleacc.h>
#include <imm.h>
#include <ddk/immdev.h>
#include <cguid.h>
#include <msctf.h>
#include <ctffunc.h>
#include <shlwapi.h>
#include <atlbase.h>
#include <atlcom.h>
#include <strsafe.h>
#undef STATUS_NO_MEMORY
#include <cicero/cicuif.h>

#include <wine/debug.h>

#include "resource.h"
