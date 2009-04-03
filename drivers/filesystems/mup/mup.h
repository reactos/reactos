#ifndef MUP_H
#define MUP_H

#ifndef TAG
#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#endif

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

#endif /* MUP_H */
