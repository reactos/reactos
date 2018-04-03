
#ifndef _NTDLL_WINETEST_PRECOMP_H_
#define _NTDLL_WINETEST_PRECOMP_H_

#include <stdio.h>
#include <ntstatus.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include "ntdll_test.h"

#include <winuser.h>
#include <winnls.h>
#include <winioctl.h>

#endif /* !_NTDLL_WINETEST_PRECOMP_H_ */
