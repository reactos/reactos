/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Kernel
 * PURPOSE:     PCMCIA Bus Driver
 * PROGRAMMERS: Cameron Gutman (cameron.gutman@reactos.org)
 */

#include <pcmcia.h>

//#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
PcmciaFdoPlugPlay(PPCMCIA_FDO_EXTENSION FdoExt,
                  PIRP Irp)
{
    UNREFERENCED_PARAMETER(FdoExt);

    UNIMPLEMENTED;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_NOT_SUPPORTED;
}
