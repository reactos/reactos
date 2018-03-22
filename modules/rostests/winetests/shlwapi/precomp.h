
#ifndef _SHLWAPI_WINETEST_PRECOMP_H_
#define _SHLWAPI_WINETEST_PRECOMP_H_

#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define COBJMACROS
#define CONST_VTABLE

#include <wine/test.h>

#include <winnls.h>
#include <winreg.h>
#include <shlwapi.h>
#include <shlguid.h>
#include <shobjidl.h>
#include <ole2.h>
#include <wininet.h>

#endif /* !_SHLWAPI_WINETEST_PRECOMP_H_ */
