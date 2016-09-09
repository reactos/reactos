#include "precomp.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
Bus_PDO_EvalMethod(PPDO_DEVICE_DATA DeviceData,
                   PIRP Irp)
{
  ULONG Signature;
  NTSTATUS Status;
  ACPI_OBJECT_LIST ParamList;
  PACPI_EVAL_INPUT_BUFFER EvalInputBuff = Irp->AssociatedIrp.SystemBuffer;
  ACPI_BUFFER RetBuff = {ACPI_ALLOCATE_BUFFER, NULL};
  PACPI_EVAL_OUTPUT_BUFFER OutputBuf;
  PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
  ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER *SimpleInt;
  ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING *SimpleStr;

  if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG))
      return STATUS_INVALID_PARAMETER;

  Signature = *((PULONG)Irp->AssociatedIrp.SystemBuffer);

  switch (Signature)
  {
     case ACPI_EVAL_INPUT_BUFFER_SIGNATURE:
        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ACPI_EVAL_INPUT_BUFFER))
            return STATUS_INVALID_PARAMETER;

        ParamList.Count = 0;
        break;

     case ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_SIGNATURE:
        SimpleInt = Irp->AssociatedIrp.SystemBuffer;

        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER))
            return STATUS_INVALID_PARAMETER;

        ParamList.Count = 1;

        ParamList.Pointer = ExAllocatePoolWithTag(NonPagedPool, sizeof(ACPI_OBJECT), 'OpcA');
        if (!ParamList.Pointer) return STATUS_INSUFFICIENT_RESOURCES;

        ParamList.Pointer[0].Type = ACPI_TYPE_INTEGER;
        ParamList.Pointer[0].Integer.Value = SimpleInt->IntegerArgument;
        break;

     case ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING_SIGNATURE:
        SimpleStr = Irp->AssociatedIrp.SystemBuffer;

        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING))
            return STATUS_INVALID_PARAMETER;

        ParamList.Count = 1;

        ParamList.Pointer = ExAllocatePoolWithTag(NonPagedPool, sizeof(ACPI_OBJECT), 'OpcA');
        if (!ParamList.Pointer) return STATUS_INSUFFICIENT_RESOURCES;

        ParamList.Pointer[0].String.Pointer = (CHAR*)SimpleStr->String;
        ParamList.Pointer[0].String.Length = SimpleStr->StringLength;
        break;

     default:
        DPRINT1("Unsupported input buffer signature: %d\n", Signature);
        return STATUS_NOT_IMPLEMENTED;
  }

  Status = AcpiEvaluateObject(DeviceData->AcpiHandle,
                              (CHAR*)EvalInputBuff->MethodName,
                              &ParamList,
                              &RetBuff);

  if (ParamList.Count != 0)
      ExFreePoolWithTag(ParamList.Pointer, 'OpcA');

  if (ACPI_SUCCESS(Status))
  {
      ACPI_OBJECT *Obj = RetBuff.Pointer;
      ULONG ExtraParamLength;

      /* If we didn't get anything back then we're done */
      if (!RetBuff.Pointer || RetBuff.Length == 0)
          return STATUS_SUCCESS;

      switch (Obj->Type)
      {
          case ACPI_TYPE_INTEGER:
             ExtraParamLength = sizeof(ULONG);
             break;

          case ACPI_TYPE_STRING:
             ExtraParamLength = Obj->String.Length;
             break;

          case ACPI_TYPE_BUFFER:
             ExtraParamLength = Obj->Buffer.Length;
             break;

          case ACPI_TYPE_PACKAGE:
             DPRINT1("ACPI_TYPE_PACKAGE not supported yet!\n");
             return STATUS_UNSUCCESSFUL;

          default:
             ASSERT(FALSE);
             return STATUS_UNSUCCESSFUL;
      }

      /* Enough space for a ULONG is always included */
      if (ExtraParamLength >= sizeof(ULONG))
          ExtraParamLength -= sizeof(ULONG);
      else
          ExtraParamLength = 0;

      OutputBuf = ExAllocatePoolWithTag(NonPagedPool, sizeof(ACPI_EVAL_OUTPUT_BUFFER) +
                                               ExtraParamLength, 'BpcA');
      if (!OutputBuf) return STATUS_INSUFFICIENT_RESOURCES;

      OutputBuf->Signature = ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE;
      OutputBuf->Length = ExtraParamLength + sizeof(ACPI_METHOD_ARGUMENT);
      OutputBuf->Count = 1;

      switch (Obj->Type)
      {
          case ACPI_TYPE_INTEGER:
             ACPI_METHOD_SET_ARGUMENT_INTEGER(OutputBuf->Argument, Obj->Integer.Value);
             break;

          case ACPI_TYPE_STRING:
             ACPI_METHOD_SET_ARGUMENT_STRING(OutputBuf->Argument, Obj->String.Pointer);
             break;

          case ACPI_TYPE_BUFFER:
             ACPI_METHOD_SET_ARGUMENT_BUFFER(OutputBuf->Argument, Obj->Buffer.Pointer, Obj->Buffer.Length);
             break;

          case ACPI_TYPE_PACKAGE:
             DPRINT1("ACPI_TYPE_PACKAGE not supported yet!\n");
             return STATUS_UNSUCCESSFUL;

          default:
             ASSERT(FALSE);
             return STATUS_UNSUCCESSFUL;
      }

      if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(ACPI_EVAL_OUTPUT_BUFFER) +
                                                                  ExtraParamLength)
      {
          RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, OutputBuf, sizeof(ACPI_EVAL_OUTPUT_BUFFER) +
                                                                    ExtraParamLength);
          Irp->IoStatus.Information = sizeof(ACPI_EVAL_OUTPUT_BUFFER) + ExtraParamLength;
          ExFreePoolWithTag(OutputBuf, 'BpcA');
          return STATUS_SUCCESS;
      }
      else
      {
          ExFreePoolWithTag(OutputBuf, 'BpcA');
          return STATUS_BUFFER_TOO_SMALL;
      }
  }
  else
  {
      DPRINT1("Query method %s failed on %p\n", EvalInputBuff->MethodName, DeviceData->AcpiHandle);
      return STATUS_UNSUCCESSFUL; 
  }
}
