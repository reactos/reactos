#ifndef _MUP_PCH_
#define _MUP_PCH_

#include <wdm.h>

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))

typedef struct
{
  ULONG Dummy;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION, VCB, *PVCB;

/* create.c */
DRIVER_DISPATCH MupCreate;
NTSTATUS NTAPI
MupCreate(PDEVICE_OBJECT DeviceObject,
	  PIRP Irp);

/* mup.c */
NTSTATUS NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
	    PUNICODE_STRING RegistryPath);

#endif /* _MUP_PCH_ */
