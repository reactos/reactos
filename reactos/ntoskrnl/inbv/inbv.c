/* $Id: inbv.c,v 1.1 2003/08/11 18:50:12 chorns Exp $
 *
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/inbv/inbv.c
 * PURPOSE:        Boot video support
 * PROGRAMMER:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *  12-07-2003 CSH Created
 */

/* INCLUDES ******************************************************************/

#include <roskrnl.h>
#include <ntos/bootvid.h>

#define NDEBUG
#include <internal/debug.h>


/* GLOBALS *******************************************************************/

/* DATA **********************************************************************/

/* FUNCTIONS *****************************************************************/

VOID
STDCALL
InbvAcquireDisplayOwnership(VOID)
{
}


BOOLEAN
STDCALL
InbvCheckDisplayOwnership(VOID)
{
  return FALSE;
}


BOOLEAN
STDCALL
InbvDisplayString(IN PCHAR String)
{
  return FALSE;
}


VOID
STDCALL
InbvEnableBootDriver(IN BOOLEAN Enable)
{
  if (Enable)
    {
      VidInitialize();
    }
  else
    {
      VidCleanUp();
    }
}

BOOLEAN
STDCALL
InbvEnableDisplayString(IN BOOLEAN Enable)
{
  return FALSE;
}


VOID
STDCALL
InbvInstallDisplayStringFilter(IN PVOID Unknown)
{
}


BOOLEAN
STDCALL
InbvIsBootDriverInstalled(VOID)
{
  return VidIsBootDriverInstalled();
}


VOID
STDCALL
InbvNotifyDisplayOwnershipLost(IN PVOID Callback)
{
}


BOOLEAN
STDCALL
InbvResetDisplay(VOID)
{
  return VidResetDisplay();
}


VOID
STDCALL
InbvSetScrollRegion(IN ULONG Left,
  IN ULONG Top,
  IN ULONG Width,
  IN ULONG Height)
{
}


VOID
STDCALL
InbvSetTextColor(IN ULONG Color)
{
}


VOID
STDCALL
InbvSolidColorFill(IN ULONG Left,
  IN ULONG Top,
  IN ULONG Width,
  IN ULONG Height,
  IN ULONG Color)
{
}
