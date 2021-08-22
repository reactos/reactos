/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ReactOS Extended RunOnce processing with UI.
 * COPYRIGHT:   Copyright 2021 He Yang <1160386205@qq.com>
 */

#pragma once

#define WIN32_NO_STATUS

#include <windows.h>
#include <winbase.h>
#include <windef.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <winreg.h>
#include <process.h>
#include <iernonce_undoc.h>

#include <atlbase.h>
#include <atlwin.h>

#include <cassert>
#include <cstdlib>

#include "registry.h"
#include "dialog.h"
