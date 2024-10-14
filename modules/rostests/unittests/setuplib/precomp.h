/*
 * PROJECT:     ReactOS Setup Library - Unit-tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Precompiled header
 * COPYRIGHT:   Copyright 2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

#include <apitest.h>

/* C Headers */
#include <stdio.h>

/* PSDK/NDK */
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>

#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>

#include <setuplib.h>

/* EOF */
