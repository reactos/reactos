/*
 * PROJECT:     ReactOS PathCch Library - Unit-tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Precompiled header
 * COPYRIGHT:   Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

#include <apitest.h>

/* C Headers */
#include <stdio.h>

/* PSDK Headers */
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>

#include <strsafe.h>
#include <pathcch.h>

/* EOF */
