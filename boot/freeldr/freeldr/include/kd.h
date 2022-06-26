/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Kernel debugger header file for the FreeLoader
 * COPYRIGHT:   Copyright 2001-2003 Brian Palmer <brianp@sginet.com>
 *              Copyright 2022 Hervé Poussineau <hpoussin@reactos.org>
 */

#pragma once

#include <windbgkd.h>
#include <kddll.h>
#include <kdnetextensibility.h>

VOID DebugPrintChar(UCHAR Character);
