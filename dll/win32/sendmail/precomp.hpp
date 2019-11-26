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

extern HINSTANCE sendmail_hInstance;

HRESULT
CreateShellLink(
    LPCWSTR pszLinkPath,
    LPCWSTR pszCmd,
    LPCWSTR pszArg OPTIONAL,
    LPCWSTR pszDir OPTIONAL,
    LPCWSTR pszIconPath OPTIONAL,
    INT iIconNr OPTIONAL,
    LPCWSTR pszComment OPTIONAL);
