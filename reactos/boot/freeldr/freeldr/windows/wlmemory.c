/*
 * PROJECT:         EFI Windows Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            freeldr/winldr/wlmemory.c
 * PURPOSE:         Memory related routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <freeldr.h>

#include <ndk/asm.h>

#define NDEBUG
#include <debug.h>

// This is needed because headers define wrong one for ReactOS
#undef KIP0PCRADDRESS
#define KIP0PCRADDRESS                      0xffdff000

#define HYPER_SPACE_ENTRY       0x300

/* GLOBALS ***************************************************************/

/* FUNCTIONS **************************************************************/

BOOLEAN
WinLdrTurnOnPaging(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                   ULONG PcrBasePage,
                   ULONG TssBasePage,
                   PVOID GdtIdt)
{
	return FALSE;
}
