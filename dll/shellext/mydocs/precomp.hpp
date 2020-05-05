/*
 * PROJECT:     mydocs
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     MyDocs implementation
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#define COBJMACROS
#define WIN32_NO_STATUS
#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atlstr.h>
#include <shlguid_undoc.h>
#include <shellapi.h>
#include <shellutils.h>
#include <strsafe.h>
#include <wine/debug.h>

#include "CMyDocsDropHandler.hpp"

#include "mydocs_version.h"
#include "resource.h"

extern LONG g_ModuleRefCnt;
