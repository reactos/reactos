/*
 * PROJECT:     ReactOS utilities
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        apps/utils/getfirefox/precomp.h
 * PURPOSE:     Precompiled header file
 * COPYRIGHT:   Copyright 2005 Ge van Geldorp (gvg@reactos.org)
 */

#define COBJMACROS
#define NTOS_MODE_USER
#define WIN32_NO_STATUS
#include <windows.h>
#include <ndk/ntndk.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <urlmon.h>

#include "resource.h"
