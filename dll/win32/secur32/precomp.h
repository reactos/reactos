/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/secur32/precomp.h
 * PURPOSE:         Security Library Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* SDK/DDK/NDK Headers. */

#include <assert.h>
#include <stdarg.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ntndk.h>
#include <lsass/lsass.h>
#define SECURITY_WIN32
#define _NO_KSECDD_IMPORT_
#include <ntsecapi.h>
#include <secext.h>
#include <security.h>
#include <ntsecpkg.h>
#include <sspi.h>

#include "lmcons.h"
#include "secur32_priv.h"
#include "thunks.h"


extern SecurityFunctionTableA securityFunctionTableA;
extern SecurityFunctionTableW securityFunctionTableW;

