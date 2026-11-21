/*
 * PROJECT:     ReactOS VirtIO Sound Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     VirtIO debug print glue
 * COPYRIGHT:   Copyright 2026 Ralf Lang <ralf.lang@ralf-lang.de>
 */

#include <kdebugprint.h>
#include <ntddk.h>

int virtioDebugLevel = 0;
int bDebugPrint = 1;
tDebugPrintFunc VirtioDebugPrintProc = (tDebugPrintFunc)DbgPrint;
