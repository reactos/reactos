
#ifndef _KERNEL32_WINETEST_PRECOMP_H_
#define _KERNEL32_WINETEST_PRECOMP_H_

#include <assert.h>
#include <stdio.h>

#define COBJMACROS

#include <ntstatus.h>
#define WIN32_NO_STATUS

#include <windows.h>
#include <wine/test.h>
#include <wine/winternl.h>
#include <winuser.h>
#include <winreg.h>
#include <wincon.h>
#include <winnls.h>
#include <winioctl.h>
#include <tlhelp32.h>

#endif /* !_KERNEL32_WINETEST_PRECOMP_H_ */
