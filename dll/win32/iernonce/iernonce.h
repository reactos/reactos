/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ReactOS Extended RunOnce processing with UI.
 * COPYRIGHT:   Copyright 2021 He Yang <1160386205@qq.com>
 */

#pragma once

#include <cassert>
#include <cstdlib>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <iernonce_undoc.h>

#include <atlbase.h>
#include <atlwin.h>

#include "registry.h"
#include "dialog.h"
