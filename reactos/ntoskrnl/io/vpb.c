/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/vpb.c
 * PURPOSE:         Volume Parameters Block managment
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/string.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS IoAttachVpb(PDEVICE_OBJECT DeviceObject)
{
   PVPB Vpb;
   
   Vpb = ExAllocatePool(NonPagedPool,sizeof(VPB));
   if (Vpb==NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   Vpb->Type = 0;
   Vpb->Size = sizeof(VPB) / sizeof(DWORD);
   Vpb->Flags = 0;
   Vpb->VolumeLabelLength = 0;
   Vpb->DeviceObject = NULL; 
   Vpb->RealDevice = DeviceObject;
   Vpb->SerialNumber = 0;
   Vpb->ReferenceCount = 0;
   RtlZeroMemory(Vpb->VolumeLabel,sizeof(WCHAR)*MAXIMUM_VOLUME_LABEL_LENGTH);
   
   DeviceObject->Vpb = Vpb;
}
