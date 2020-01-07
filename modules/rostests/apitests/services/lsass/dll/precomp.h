/*
 * PROJECT:     ReactOS lsass_apitest
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     precompiled header
 * COPYRIGHT:   Copyright 2020 Andreas Maier (staubim@quantentunnel.de)
 */

#ifndef _LSASS_APITEST_PRECOMP_H_
#define _LSASS_APITEST_PRECOMP_H_


#include <apitest.h>
#include <stdio.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winnt.h>
#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>

#include <sspi.h>
#include <ntsecapi.h>
#include <ntsecpkg.h>

#include <lsatest.h>
#include <lsamem.h>
#include <lsafntable.h>
#include <../shared/dbgutil.h>

#define NDEBUG
#include <debug.h>

#endif
