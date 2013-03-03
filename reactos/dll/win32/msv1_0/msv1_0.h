/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/msv1_0/msv1_0.h
 * PURPOSE:     Common header file
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#define NTOS_MODE_USER
#include <ndk/cmfuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/lpctypes.h>
#include <ndk/lpcfuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/setypes.h>

#include <sspi.h>
#include <ntsecapi.h>
#include <ntsecpkg.h>

#include <wine/debug.h>

/* EOF */
