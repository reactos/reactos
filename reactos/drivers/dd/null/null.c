/* $Id: null.c,v 1.10 2002/09/08 10:22:05 chorns Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/null/null.c
 * PURPOSE:          NULL device driver
 * PROGRAMMER:       David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *              13/08/1998: Created
 *              29/04/2002: Fixed bugs, added zero-stream device
 */

/* INCLUDES */
#include <ddk/ntddk.h>
#include "null.h"

/* OBJECTS */
static const NULL_EXTENSION nxNull = NullBitBucket;
static const NULL_EXTENSION nxZero = NullZeroStream;

/* FUNCTIONS */
NTSTATUS STDCALL
NullDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
 PIO_STACK_LOCATION piosStack = IoGetCurrentIrpStackLocation(Irp);
 NTSTATUS nErrCode;
 
 nErrCode = STATUS_SUCCESS;

 switch(piosStack->MajorFunction)
 {
  /* opening and closing handles to the device */
  case IRP_MJ_CREATE:
  case IRP_MJ_CLOSE:
  {
   break;
  }

  /* write data */
  case IRP_MJ_WRITE:
  {
   switch(NULL_DEVICE_TYPE(DeviceObject))
   {
    case NullBitBucket:
     Irp->IoStatus.Information = piosStack->Parameters.Write.Length;
     break;

    case NullZeroStream:
    default:
     Irp->IoStatus.Information = 0;
     nErrCode = STATUS_NOT_IMPLEMENTED;
   }

   break;
  }

  /* read data */
  case IRP_MJ_READ:
  {
   switch(NULL_DEVICE_TYPE(DeviceObject))
   {
    case NullBitBucket:
     Irp->IoStatus.Information = 0;
     nErrCode = STATUS_END_OF_FILE;
     break;

    case NullZeroStream:
     RtlZeroMemory(Irp->AssociatedIrp.SystemBuffer, piosStack->Parameters.Read.Length);
     Irp->IoStatus.Information = piosStack->Parameters.Read.Length;
     break;

    default:
     Irp->IoStatus.Information = 0;
     nErrCode = STATUS_NOT_IMPLEMENTED;
   }

   break;
  }

  /* unsupported operations */
  default:
  {
   nErrCode = STATUS_NOT_IMPLEMENTED;
  }
 }

 Irp->IoStatus.Status = nErrCode;
 IoCompleteRequest(Irp, IO_NO_INCREMENT);

 return (nErrCode);
}

NTSTATUS STDCALL
NullUnload(PDRIVER_OBJECT DriverObject)
{
 return(STATUS_SUCCESS);
}

NTSTATUS STDCALL 
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
 PDEVICE_OBJECT pdoNullDevice;
 PDEVICE_OBJECT pdoZeroDevice;
 UNICODE_STRING wstrDeviceName;
 NTSTATUS nErrCode;

 /* register driver routines */
 DriverObject->MajorFunction[IRP_MJ_CLOSE] = NullDispatch;
 DriverObject->MajorFunction[IRP_MJ_CREATE] = NullDispatch;
 DriverObject->MajorFunction[IRP_MJ_WRITE] = NullDispatch;
 DriverObject->MajorFunction[IRP_MJ_READ] = NullDispatch;
 /* DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = NullDispatch; */
 DriverObject->DriverUnload = NullUnload;

 /* create null device */
 RtlInitUnicodeStringFromLiteral(&wstrDeviceName, L"\\Device\\Null");

 nErrCode = IoCreateDevice
 (
  DriverObject,
  sizeof(NULL_EXTENSION),
  &wstrDeviceName,
  FILE_DEVICE_NULL,
  0,
  FALSE,
  &pdoNullDevice
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  return (nErrCode);
 }

 pdoNullDevice->DeviceExtension = (PVOID)&nxNull;

 /* create zero device */
 RtlInitUnicodeStringFromLiteral(&wstrDeviceName, L"\\Device\\Zero");

 nErrCode = IoCreateDevice
 (
  DriverObject,
  sizeof(NULL_EXTENSION),
  &wstrDeviceName,
  FILE_DEVICE_NULL,
  FILE_READ_ONLY_DEVICE, /* zero device is read-only */
  FALSE,
  &pdoZeroDevice
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  IoDeleteDevice(pdoNullDevice);
  return (nErrCode);
 }

 pdoZeroDevice->DeviceExtension = (PVOID)&nxZero;
 pdoZeroDevice->Flags |= DO_BUFFERED_IO;

 return (nErrCode);
}

/* EOF */
