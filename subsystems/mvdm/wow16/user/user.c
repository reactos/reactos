/*
 * PROJECT:     ReactOS-on-ReactOS-16bit (aka. RoR16 or WoW16)
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     16-bit USER stub module
 * COPYRIGHT:   Copyright 2017-2018 Hermes Belusca-Maito
 */

/* INCLUDES *******************************************************************/

/* PSDK Headers */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>

#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS ***********************************************************/

INT main(INT argc, const CHAR *argv[])
{
    OutputDebugStringA("USER.EXE: stub\n");
    return 0;
}
