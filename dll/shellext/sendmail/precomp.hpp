/*
 * PROJECT:     sendmail
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     DeskLink implementation
 * COPYRIGHT:   Copyright 2019 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
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

#include "CDeskLinkDropHandler.hpp"

#include "sendmail_version.h"
#include "resource.h"

extern LONG g_ModuleRefCnt;

HRESULT
CreateShellLink(
    LPCWSTR pszLinkPath,
    LPCWSTR pszTargetPath OPTIONAL,
    LPCITEMIDLIST pidlTarget OPTIONAL,
    LPCWSTR pszArg OPTIONAL,
    LPCWSTR pszDir OPTIONAL,
    LPCWSTR pszIconPath OPTIONAL,
    INT iIconNr OPTIONAL,
    LPCWSTR pszComment OPTIONAL);
