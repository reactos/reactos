#include "precomp.h"

#define NDEBUG
#include <debug.h>

static
NTSTATUS
GetPackageSize(ACPI_OBJECT *Package,
               PULONG Count,
               PULONG Size)
{
    ULONG Length, RawLength, TotalLength;
    UINT32 i;

    TotalLength = 0;
    for (i = 0; i < Package->Package.Count; i++)
    {
        switch (Package->Package.Elements[i].Type)
        {
            case ACPI_TYPE_INTEGER:
                Length = sizeof(ACPI_METHOD_ARGUMENT);
                DPRINT("Integer %lu -> %lu: %lu\n", sizeof(ULONG), Length, Package->Package.Elements[i].Integer.Value);
                TotalLength += Length;
                break;

            case ACPI_TYPE_STRING:
                RawLength = Package->Package.Elements[i].String.Length + 1;
                Length = sizeof(ACPI_METHOD_ARGUMENT);
                if (RawLength > sizeof(ULONG))
                    Length += RawLength - sizeof(ULONG);
                DPRINT("String %lu -> %lu: '%s'\n", RawLength, Length, Package->Package.Elements[i].String.Pointer);
                TotalLength += Length;
                break;

            default:
                DPRINT1("Unsupported element type %lu\n", Package->Package.Elements[i].Type);
                return STATUS_UNSUCCESSFUL;
        }
    }

    *Count = Package->Package.Count;
    *Size = TotalLength;

    return STATUS_SUCCESS;
}


static
NTSTATUS
ConvertPackageArguments(ACPI_METHOD_ARGUMENT *Argument,
                        ACPI_OBJECT *Package)
{
    ACPI_METHOD_ARGUMENT *Ptr;
    UINT32 i;

    Ptr = Argument;
    for (i = 0; i < Package->Package.Count; i++)
    {
        switch (Package->Package.Elements[i].Type)
        {
            case ACPI_TYPE_INTEGER:
                DPRINT("Integer %lu\n", sizeof(ACPI_METHOD_ARGUMENT));
                ACPI_METHOD_SET_ARGUMENT_INTEGER(Ptr, Package->Package.Elements[i].Integer.Value);
                break;

            case ACPI_TYPE_STRING:
                DPRINT("String %lu\n", Package->Package.Elements[i].String.Length);
                ACPI_METHOD_SET_ARGUMENT_STRING(Ptr, Package->Package.Elements[i].String.Pointer);
                break;

            default:
                DPRINT1("Unsupported element type %lu\n", Package->Package.Elements[i].Type);
                return STATUS_UNSUCCESSFUL;
        }

        Ptr = ACPI_METHOD_NEXT_ARGUMENT(Ptr);
    }

    return STATUS_SUCCESS;
}


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
  CHAR MethodName[5];

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

  RtlCopyMemory(MethodName,
                EvalInputBuff->MethodName,
                sizeof(EvalInputBuff->MethodName));
  MethodName[4] = ANSI_NULL;
  Status = AcpiEvaluateObject(DeviceData->AcpiHandle,
                              MethodName,
                              &ParamList,
                              &RetBuff);

  if (ParamList.Count != 0)
      ExFreePoolWithTag(ParamList.Pointer, 'OpcA');

  if (ACPI_SUCCESS(Status))
  {
      ACPI_OBJECT *Obj = RetBuff.Pointer;
      ULONG ExtraParamLength = 0;
      ULONG Count = 1;

      /* If we didn't get anything back then we're done */
      if (!RetBuff.Pointer || RetBuff.Length == 0)
          return STATUS_SUCCESS;

      switch (Obj->Type)
      {
          case ACPI_TYPE_INTEGER:
             ExtraParamLength = sizeof(ACPI_METHOD_ARGUMENT);
             break;

          case ACPI_TYPE_STRING:
             ExtraParamLength = sizeof(ACPI_METHOD_ARGUMENT);
             if (Obj->String.Length + 1 > sizeof(ULONG))
                 ExtraParamLength += Obj->String.Length + 1 - sizeof(ULONG);
             break;

          case ACPI_TYPE_BUFFER:
             ExtraParamLength = sizeof(ACPI_METHOD_ARGUMENT);
             if (Obj->Buffer.Length > sizeof(ULONG))
                 ExtraParamLength += Obj->Buffer.Length + 1 - sizeof(ULONG);
             break;

          case ACPI_TYPE_PACKAGE:
             Status = GetPackageSize(Obj, &Count, &ExtraParamLength);
             if (!NT_SUCCESS(Status))
                 return Status;
             break;

          default:
             ASSERT(FALSE);
             return STATUS_UNSUCCESSFUL;
      }

      DPRINT("ExtraParamLength %lu\n", ExtraParamLength);
      OutputBuf = ExAllocatePoolWithTag(NonPagedPool,
                                        sizeof(ACPI_EVAL_OUTPUT_BUFFER) - sizeof(ACPI_METHOD_ARGUMENT) + ExtraParamLength,
                                        'BpcA');
      if (!OutputBuf)
          return STATUS_INSUFFICIENT_RESOURCES;

      OutputBuf->Signature = ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE;
      OutputBuf->Length = ExtraParamLength;
      OutputBuf->Count = Count;

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
             Status = ConvertPackageArguments(OutputBuf->Argument, Obj);
             if (!NT_SUCCESS(Status))
                 return Status;
             break;

          default:
             ASSERT(FALSE);
             return STATUS_UNSUCCESSFUL;
      }

      if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(ACPI_EVAL_OUTPUT_BUFFER) - sizeof(ACPI_METHOD_ARGUMENT) +
                                                                  ExtraParamLength)
      {
          RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, OutputBuf, sizeof(ACPI_EVAL_OUTPUT_BUFFER) - sizeof(ACPI_METHOD_ARGUMENT) +
                                                                    ExtraParamLength);
          Irp->IoStatus.Information = sizeof(ACPI_EVAL_OUTPUT_BUFFER) - sizeof(ACPI_METHOD_ARGUMENT) + ExtraParamLength;
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
      DPRINT1("Query method %4s failed on %p\n", EvalInputBuff->MethodName, DeviceData->AcpiHandle);
      return STATUS_UNSUCCESSFUL;
  }
}
