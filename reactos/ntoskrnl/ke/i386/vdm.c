/* $Id:$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/vdm.c
 * PURPOSE:         Virtual DOS machine support
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static UCHAR OrigIVT[1024];
static UCHAR OrigBDA[256];
/* static UCHAR OrigEBDA[]; */

extern VOID Ki386RetToV86Mode(PKV86M_REGISTERS InRegs,
			      PKV86M_REGISTERS OutRegs);

/* FUNCTIONS *****************************************************************/

VOID INIT_FUNCTION
NtEarlyInitVdm(VOID)
{
  /* GCC 3.4 warns if NULL is passed in parameter 2 to the standard function memcpy */
  PVOID start = (PVOID)0x0;

  /*
   * Save various BIOS data tables. At this point the lower 4MB memory
   * map is still active so we can just copy the data from low memory.
   */
  memcpy(OrigIVT, start, 1024);
  memcpy(OrigBDA, (PVOID)0x400, 256);
}

/*
 * @implemented
 */
NTSTATUS STDCALL NtVdmControl(ULONG ControlCode,
			      PVOID ControlData)
{
  switch (ControlCode)
  {
    case 0:
      memcpy(ControlData, OrigIVT, 1024);
      break;

    case 1:
      memcpy(ControlData, OrigBDA, 256);
      break;

    case 2:
    {
      KV86M_REGISTERS V86Registers;
      ULONG ret;

      ret = MmCopyFromCaller(&V86Registers,
                             ControlData,
                             sizeof(KV86M_REGISTERS));
      if(!NT_SUCCESS(ret)) return ret;

      /* FIXME: This should use ->VdmObjects */
      KeGetCurrentProcess()->Unused = 1;
      Ki386RetToV86Mode(&V86Registers, &V86Registers);
      
      /* FIXME: This should use ->VdmObjects */
      KeGetCurrentProcess()->Unused = 0;

      ret = MmCopyToCaller(ControlData,
                           &V86Registers,
                           sizeof(KV86M_REGISTERS));
      if(!NT_SUCCESS(ret)) return ret;

      break;
    }
  }
  return(STATUS_SUCCESS);
}

/* EOF */
