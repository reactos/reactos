/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/freeldr/freeldr/arch/arm/entry.c
 * PURPOSE:         Implements the entry point for ARM machines (see also boot.S)
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES ***************************************************************/

#include <freeldr.h>
#include <debug.h>

/* FUNCTIONS **************************************************************/

VOID
RealEntryPoint(VOID)
{
    BootMain("");
}

VOID
FrLdrBugCheckWithMessage(
    ULONG BugCode,
    PCHAR File,
    ULONG Line,
    PSTR Format,
    ...)
{

}
