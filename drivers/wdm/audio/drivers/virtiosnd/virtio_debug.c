/*
 * PROJECT:     ReactOS VirtIO Sound Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     VirtIO debug print glue
 * COPYRIGHT:   Copyright 2026 Ralf Lang <ralf.lang@ralf-lang.de>
 */

/**
 * @file
 * @brief VirtIO debug print glue
 *
 * This file contains debug print functionality for the VirtIO sound driver.
 *
 * @copyright Copyright 2026 Ralf Lang <ralf.lang@ralf-lang.de>
 * @license GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 */

#include <kdebugprint.h>
#include <ntddk.h>

/**
 * @brief Debug level for VirtIO sound driver
 */
int virtioDebugLevel = 0;

/**
 * @brief Flag to enable/disable debug printing
 */
int bDebugPrint = 1;

/**
 * @brief Debug print function pointer
 */
tDebugPrintFunc VirtioDebugPrintProc = (tDebugPrintFunc)DbgPrint;
