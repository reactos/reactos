/*
 * PROJECT:         ReactOS Drivers
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         Kernel Security Support Provider Interface Driver
 *
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#define _NO_KSECDD_IMPORT_
#include <ntifs.h>

// 0x390004
#define IOCTL_KSEC_GEN_RANDOM \
    CTL_CODE(FILE_DEVICE_KSEC, 0x01, METHOD_BUFFERED, FILE_ANY_ACCESS)

NTSTATUS
NTAPI
KsecDdDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);


NTSTATUS
NTAPI
KsecGenRandom(
    PVOID Buffer,
    SIZE_T Length);

