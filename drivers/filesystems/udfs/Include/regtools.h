////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////

#ifndef __MULTIENV_REG_TOOLS__H__
#define __MULTIENV_REG_TOOLS__H__

#include "check_env.h"

#ifndef WIN_32_MODE
#define HKEY  HANDLE
#endif //WIN_32_MODE

NTSTATUS
RegTGetKeyHandle(
    IN HKEY hRootKey,
    IN PCWSTR KeyName,
    OUT HKEY* hKey
    );

VOID
RegTCloseKeyHandle(
    IN HKEY hKey
    );

BOOLEAN
RegTGetDwordValue(
    IN HKEY hRootKey,
    IN PCWSTR RegistryPath,
    IN PCWSTR Name,
    IN PULONG pUlong
    );

BOOLEAN
RegTGetStringValue(
    IN HKEY hRootKey,
    IN PCWSTR RegistryPath,
    IN PCWSTR Name,
    IN PWCHAR pStr,
    IN ULONG MaxLen
    );

#endif //__MULTIENV_REG_TOOLS__H__
