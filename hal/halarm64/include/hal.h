/*
 * PROJECT:     ReactOS ARM64 HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Precompiled header
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

/* C Headers */
#include <stdio.h>

/* WDK HAL Compilation hack */
#include <excpt.h>
#include <ntdef.h>
#undef NTSYSAPI
#define NTSYSAPI __declspec(dllimport)

/* IFS/DDK/NDK Headers */
#include <ntifs.h>
#include <arc/arc.h>
#include <ndk/asm.h>
#include <ndk/halfuncs.h>
#include <ndk/inbvfuncs.h>
#include <ndk/iofuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/rtlfuncs.h>

/* Internal shared ACPI header */
#include <drivers/acpi/acpi.h>

/* Internal kernel headers */
#include <internal/arm64/ke.h>
#include <internal/arm64/mm.h>

#define TAG_HAL ' laH'

/* Internal HAL Headers, the DMA support is shared with halx86 */
#include <haldma.h>
#include "halp.h"
