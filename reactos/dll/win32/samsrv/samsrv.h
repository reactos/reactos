/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Security Account Manager (LSA) Server
 * FILE:            reactos/dll/win32/samsrv/samsrv.h
 * PURPOSE:         Common header file
 *
 * PROGRAMMERS:     Eric Kohl
 */

#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/umtypes.h>

#include <samsrv/samsrv.h>

#include "sam_s.h"

#include <wine/debug.h>