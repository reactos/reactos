/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS-on-ReactOS-16bit (aka. RoR16 or WoW16)
 * FILE:            subsystems/mvdm/wow16/user.c
 * PURPOSE:         16-bit USER stub module
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
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
