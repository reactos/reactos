#ifndef MUP_H
#define MUP_H

#include <ddk/ntifs.h>


#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))



typedef struct
{
  ULONG Dummy;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION, VCB, *PVCB;



/* create.c */

NTSTATUS STDCALL
MupCreate(PDEVICE_OBJECT DeviceObject,
	  PIRP Irp);



#endif /* MUP_H */
