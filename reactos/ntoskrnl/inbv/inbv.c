/* $Id: inbv.c,v 1.7 2004/05/07 05:12:10 royce Exp $
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
#include <ddk/ntbootvid.h>
#include <rosrtl/string.h>

#define NDEBUG
#include <internal/debug.h>


/* GLOBALS *******************************************************************/

/* DATA **********************************************************************/

static HANDLE BootVidDevice = NULL;
static BOOL BootVidDriverInstalled = FALSE;
static NTBOOTVID_FUNCTION_TABLE BootVidFunctionTable;

/* FUNCTIONS *****************************************************************/

NTSTATUS
STATIC
InbvCheckBootVid(VOID)
{
  IO_STATUS_BLOCK Iosb;

  if (BootVidDevice == NULL)
    {
      NTSTATUS Status;
      OBJECT_ATTRIBUTES ObjectAttributes;
      UNICODE_STRING BootVidName;

      RtlRosInitUnicodeStringFromLiteral(&BootVidName, L"\\Device\\BootVid");
      InitializeObjectAttributes(&ObjectAttributes,
				 &BootVidName,
				 0,
				 NULL,
				 NULL);
      Status = ZwOpenFile(&BootVidDevice,
			  FILE_ALL_ACCESS,
			  &ObjectAttributes,
			  &Iosb,
			  0,
			  0);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}
    }
  return(STATUS_SUCCESS);
}

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

BOOLEAN
STDCALL
InbvResetDisplayParameters(ULONG SizeX, ULONG SizeY)
{
  return(InbvResetDisplay());
}

VOID
STDCALL INIT_FUNCTION
InbvEnableBootDriver(IN BOOLEAN Enable)
{
  NTSTATUS Status;
  IO_STATUS_BLOCK Iosb;

  Status = InbvCheckBootVid();
  if (!NT_SUCCESS(Status))
    {
      return;
    }

  if (Enable)
    {
      /* Notify the hal we will acquire the display. */
      HalAcquireDisplayOwnership(InbvResetDisplayParameters);

      Status = NtDeviceIoControlFile(BootVidDevice,
				     NULL,
				     NULL,
				     NULL,
				     &Iosb,
				     IOCTL_BOOTVID_INITIALIZE,
				     NULL,
				     0,
				     &BootVidFunctionTable,
				     sizeof(BootVidFunctionTable));
      if (!NT_SUCCESS(Status))
	{
	  KEBUGCHECK(0);
	}
      BootVidDriverInstalled = TRUE;
      CHECKPOINT;
    }
  else
    {
      Status = NtDeviceIoControlFile(BootVidDevice,
				     NULL,
				     NULL,
				     NULL,
				     &Iosb,
				     IOCTL_BOOTVID_CLEANUP,
				     NULL,
				     0,
				     NULL,
				     0);
      if (!NT_SUCCESS(Status))
	{
	  KEBUGCHECK(0);
	}
      BootVidDriverInstalled = FALSE;
      /* Notify the hal we have released the display. */
      HalReleaseDisplayOwnership();

      NtClose(BootVidDevice);
      BootVidDevice = NULL;
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
  return(BootVidDriverInstalled);
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
  if (!BootVidDriverInstalled)
    {
      return(FALSE);
    }
  return(BootVidFunctionTable.ResetDisplay());
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
