//*************************************************************
//
//  Main header file for UserEnv project
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <windows.h>
#include <ole2.h>
#include <lm.h>
#include <ntregapi.h>
#define SECURITY_WIN32
#include <security.h>
#include <shlobj.h>

//
// Turn off shell debugging stuff so that it does not conflict
// with our debugging stuff.
//

#define DONT_WANT_SHELLDEBUG
#include <shlobjp.h>

#include <userenv.h>
#include <userenvp.h>
#include <ntdsapi.h>
#include <winldap.h>
#include <ntldap.h>
#include <dsgetdc.h>
#include <dsrole.h>
#include <accctrl.h>
#include <ntldap.h>
#define INCL_WINSOCK_API_TYPEDEFS 1
#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <dfsfsctl.h>
#include "globals.h"
#include "debug.h"
#include "dllload.h"
#include "profile.h"
#include "util.h"
#include "sid.h"
#include "events.h"
#include "copydir.h"
#include "resource.h"
#include "userdiff.h"
#include "policy.h"
#include "gpt.h"
#include "gpnotif.h"
#include "winbasep.h"
#include "shlobjp.h"

//
// Macros
//

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
