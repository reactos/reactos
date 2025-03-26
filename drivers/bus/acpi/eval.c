#ifndef UNIT_TEST
#include "precomp.h"

#define NDEBUG
#include <debug.h>
#endif /* UNIT_TEST */

#define AcpiVerifyInBuffer(Stack, Length) \
    ((Stack)->Parameters.DeviceIoControl.InputBufferLength >= Length)

#define AcpiVerifyOutBuffer(Stack, Length) \
    ((Stack)->Parameters.DeviceIoControl.OutputBufferLength >= Length)

#define TAG_ACPI_PARAMETERS_LIST     'OpcA'
#define TAG_ACPI_PACKAGE_LIST        'PpcA'

/**
 * Null terminated ACPI name for the object.
 * For example, _DSM, LNKB, etc.
 */
#define ACPI_OBJECT_NAME_LENGTH      (4 + 1)

/**
 * Maximum number of nested package structures supported by the driver.
 * This should be enough to cover all the existing ACPI methods.
 */
#define ACPI_MAX_PACKAGE_DEPTH       5

/**
 * @brief Performs translation from the supplied object reference
 * into a string method argument.
 */
static
CODE_SEG("PAGE")
NTSTATUS
EvalConvertObjectReference(
    _Out_ PACPI_METHOD_ARGUMENT Argument,
    _In_ ACPI_OBJECT* Reference)
{
    ACPI_BUFFER OutName;
    ACPI_STATUS AcpiStatus;

    PAGED_CODE();

    Argument->Type = ACPI_METHOD_ARGUMENT_STRING;
    Argument->DataLength = ACPI_OBJECT_NAME_LENGTH;

    /* Convert the object handle to an ACPI name */
    OutName.Length = ACPI_OBJECT_NAME_LENGTH;
    OutName.Pointer = &Argument->Data[0];

    AcpiStatus = AcpiGetName(Reference->Reference.Handle, ACPI_SINGLE_NAME, &OutName);
    if (!ACPI_SUCCESS(AcpiStatus))
    {
        DPRINT1("AcpiGetName() failed on %p with status 0x%04lx\n",
                Reference->Reference.Handle,
                AcpiStatus);

        ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

/**
 * @brief Calculates the number of bytes needed for returned method argument
 * based on the type of an ACPI_OBJECT structure.
 */
static
CODE_SEG("PAGE")
NTSTATUS
EvalGetElementSize(
    _In_ ACPI_OBJECT* Obj,
    _In_ ULONG Depth,
    _Out_opt_ PULONG Count,
    _Out_ PULONG Size)
{
    ULONG TotalCount, TotalLength;
    NTSTATUS Status;

    PAGED_CODE();

    if (Depth >= ACPI_MAX_PACKAGE_DEPTH)
    {
        ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    switch (Obj->Type)
    {
        case ACPI_TYPE_INTEGER:
        {
            TotalLength = ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG));
            TotalCount = 1;
            break;
        }

        case ACPI_TYPE_STRING:
        {
            TotalLength = ACPI_METHOD_ARGUMENT_LENGTH(Obj->String.Length + sizeof(UCHAR));
            TotalCount = 1;
            break;
        }

        case ACPI_TYPE_BUFFER:
        {
            TotalLength = ACPI_METHOD_ARGUMENT_LENGTH(Obj->Buffer.Length);
            TotalCount = 1;
            break;
        }

        case ACPI_TYPE_PACKAGE:
        {
            ULONG i, TotalPackageLength;

            /* Get the size of the current packet */
            TotalPackageLength = 0;
            for (i = 0; i < Obj->Package.Count; i++)
            {
                ULONG ElementSize;

                Status = EvalGetElementSize(&Obj->Package.Elements[i],
                                            Depth + 1,
                                            NULL,
                                            &ElementSize);
                if (!NT_SUCCESS(Status))
                    return Status;

                TotalPackageLength += ElementSize;
            }

            /* Check if we need to wrap the list of elements into a package */
            if (Depth > 0)
            {
                TotalPackageLength = ACPI_METHOD_ARGUMENT_LENGTH(TotalPackageLength);
            }

            TotalLength = TotalPackageLength;
            TotalCount = Obj->Package.Count;
            break;
        }

        case ACPI_TYPE_LOCAL_REFERENCE:
        {
            TotalLength = ACPI_METHOD_ARGUMENT_LENGTH(ACPI_OBJECT_NAME_LENGTH);
            TotalCount = 1;
            break;
        }

        default:
        {
            DPRINT1("Unsupported element type %lu\n", Obj->Type);
            return STATUS_UNSUCCESSFUL;
        }
    }

    if (Count)
        *Count = TotalCount;

    *Size = TotalLength;

    return STATUS_SUCCESS;
}

/**
 * @brief Performs translation from the supplied ACPI_OBJECT structure into a method argument.
 */
static
CODE_SEG("PAGE")
NTSTATUS
EvalConvertEvaluationResults(
    _Out_ ACPI_METHOD_ARGUMENT* Argument,
    _In_ ULONG Depth,
    _In_ ACPI_OBJECT* Obj)
{
    ACPI_METHOD_ARGUMENT *Ptr;
    NTSTATUS Status;

    PAGED_CODE();

    if (Depth >= ACPI_MAX_PACKAGE_DEPTH)
    {
        ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    Ptr = Argument;
    switch (Obj->Type)
    {
        case ACPI_TYPE_INTEGER:
        {
            ACPI_METHOD_SET_ARGUMENT_INTEGER(Ptr, Obj->Integer.Value);
            break;
        }

        case ACPI_TYPE_STRING:
        {
            ACPI_METHOD_SET_ARGUMENT_STRING(Ptr, Obj->String.Pointer);
            break;
        }

        case ACPI_TYPE_BUFFER:
        {
            ACPI_METHOD_SET_ARGUMENT_BUFFER(Ptr, Obj->Buffer.Pointer, Obj->Buffer.Length);
            break;
        }

        case ACPI_TYPE_PACKAGE:
        {
            ULONG i;

            /* Check if we need to wrap the list of elements into a package */
            if (Depth > 0)
            {
                ULONG TotalPackageLength;

                /* Get the size of the current packet */
                TotalPackageLength = 0;
                for (i = 0; i < Obj->Package.Count; i++)
                {
                    ULONG ElementSize;

                    Status = EvalGetElementSize(&Obj->Package.Elements[i],
                                                Depth + 1,
                                                NULL,
                                                &ElementSize);
                    if (!NT_SUCCESS(Status))
                        return Status;

                    TotalPackageLength += ElementSize;
                }

                /* Start a new package */
                Argument->Type = ACPI_METHOD_ARGUMENT_PACKAGE;
                Argument->DataLength = TotalPackageLength;

                Ptr = (PACPI_METHOD_ARGUMENT)Ptr->Data;
            }

            for (i = 0; i < Obj->Package.Count; i++)
            {
                Status = EvalConvertEvaluationResults(Ptr, Depth + 1, &Obj->Package.Elements[i]);
                if (!NT_SUCCESS(Status))
                    return Status;

                Ptr = ACPI_METHOD_NEXT_ARGUMENT(Ptr);
            }
            break;
        }

        case ACPI_TYPE_LOCAL_REFERENCE:
        {
            Status = EvalConvertObjectReference(Ptr, Obj);
            if (!NT_SUCCESS(Status))
                return Status;
            break;
        }

        default:
        {
            DPRINT1("Unsupported element type %lu\n", Obj->Type);
            return STATUS_UNSUCCESSFUL;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * @brief Returns the number of sub-objects (elements) in a package.
 */
static
CODE_SEG("PAGE")
ULONG
EvalGetPackageCount(
    _In_ PACPI_METHOD_ARGUMENT Package,
    _In_ PACPI_METHOD_ARGUMENT PackageArgument,
    _In_ ULONG DataLength)
{
    ACPI_METHOD_ARGUMENT* Ptr;
    ULONG TotalLength = 0, TotalCount = 0;

    PAGED_CODE();

    /* Empty package */
    if (DataLength < ACPI_METHOD_ARGUMENT_LENGTH(0) || Package->Argument == 0)
        return 0;

    Ptr = PackageArgument;
    while (TotalLength < DataLength)
    {
        TotalLength += ACPI_METHOD_ARGUMENT_LENGTH_FROM_ARGUMENT(Ptr);
        TotalCount++;

        Ptr = ACPI_METHOD_NEXT_ARGUMENT(Ptr);
    }

    return TotalCount;
}

/**
 * @brief Performs translation from the supplied method argument into an ACPI_OBJECT structure.
 */
static
CODE_SEG("PAGE")
NTSTATUS
EvalConvertParameterObjects(
    _Out_ ACPI_OBJECT* Arg,
    _In_ ULONG Depth,
    _In_ PACPI_METHOD_ARGUMENT Argument,
    _In_ PIO_STACK_LOCATION IoStack,
    _In_ ULONG Offset)
{
    PAGED_CODE();

    if (Depth >= ACPI_MAX_PACKAGE_DEPTH)
    {
        ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    /* Validate that the method argument fits into the buffer */
    if (Depth > 0)
    {
        Offset += ACPI_METHOD_ARGUMENT_LENGTH_FROM_ARGUMENT(Argument);

        if (!AcpiVerifyInBuffer(IoStack, Offset))
        {
            DPRINT1("Argument buffer outside of argument bounds\n");
            return STATUS_ACPI_INVALID_ARGTYPE;
        }
    }

    switch (Argument->Type)
    {
        case ACPI_METHOD_ARGUMENT_INTEGER:
        {
            Arg->Type = ACPI_TYPE_INTEGER;
            Arg->Integer.Value = (ULONG64)Argument->Argument;
            break;
        }

        case ACPI_METHOD_ARGUMENT_STRING:
        {
            /*
             * FIXME: Add tests and remove this.
             * We should either default to an empty string, or reject the IOCTL.
             */
            ASSERT(Argument->DataLength >= sizeof(UCHAR));
            if (Argument->DataLength < sizeof(UCHAR))
            {
                return STATUS_NOT_IMPLEMENTED;
            }

            Arg->Type = ACPI_TYPE_STRING;
            Arg->String.Pointer = (PCHAR)&Argument->Data[0];
            Arg->String.Length = Argument->DataLength - sizeof(UCHAR);
            break;
        }

        case ACPI_METHOD_ARGUMENT_BUFFER:
        {
            Arg->Type = ACPI_TYPE_BUFFER;
            Arg->Buffer.Pointer = &Argument->Data[0];
            Arg->Buffer.Length = Argument->DataLength;
            break;
        }

        case ACPI_METHOD_ARGUMENT_PACKAGE:
        {
            ULONG i, PackageSize;
            NTSTATUS Status;
            PACPI_METHOD_ARGUMENT PackageArgument;

            Arg->Type = ACPI_TYPE_PACKAGE;

            Arg->Package.Count = EvalGetPackageCount(Argument,
                                                     (PACPI_METHOD_ARGUMENT)Argument->Data,
                                                     Argument->DataLength);
            /* Empty package, nothing more to convert */
            if (Arg->Package.Count == 0)
            {
                Arg->Package.Elements = NULL;
                break;
            }

            Status = RtlULongMult(Arg->Package.Count, sizeof(*Arg), &PackageSize);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Invalid package count 0x%lx\n", Arg->Package.Count);
                return STATUS_ACPI_INCORRECT_ARGUMENT_COUNT;
            }

            Arg->Package.Elements = ExAllocatePoolUninitialized(NonPagedPool,
                                                                PackageSize,
                                                                TAG_ACPI_PACKAGE_LIST);
            if (!Arg->Package.Elements)
                return STATUS_INSUFFICIENT_RESOURCES;

            PackageArgument = (PACPI_METHOD_ARGUMENT)Argument->Data;
            for (i = 0; i < Arg->Package.Count; i++)
            {
                Status = EvalConvertParameterObjects(&Arg->Package.Elements[i],
                                                     Depth + 1,
                                                     PackageArgument,
                                                     IoStack,
                                                     Offset);
                if (!NT_SUCCESS(Status))
                {
                    ExFreePoolWithTag(Arg->Package.Elements, TAG_ACPI_PACKAGE_LIST);
                    return Status;
                }

                PackageArgument = ACPI_METHOD_NEXT_ARGUMENT(PackageArgument);
            }
            break;
        }

        default:
        {
            DPRINT1("Unknown argument type %u\n", Argument->Type);
            return STATUS_UNSUCCESSFUL;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * @brief Creates a counted array of ACPI_OBJECTs from the given input buffer.
 */
static
CODE_SEG("PAGE")
NTSTATUS
EvalCreateParametersList(
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack,
    _In_ PACPI_EVAL_INPUT_BUFFER EvalInputBuffer,
    _Out_ ACPI_OBJECT_LIST* ParamList)
{
    ACPI_OBJECT* Arg;

    PAGED_CODE();

    if (!AcpiVerifyInBuffer(IoStack, RTL_SIZEOF_THROUGH_FIELD(ACPI_EVAL_INPUT_BUFFER, Signature)))
    {
        DPRINT1("Buffer too small\n");
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    switch (EvalInputBuffer->Signature)
    {
        case ACPI_EVAL_INPUT_BUFFER_SIGNATURE:
        {
            if (!AcpiVerifyInBuffer(IoStack, sizeof(*EvalInputBuffer)))
            {
                DPRINT1("Buffer too small\n");
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            ParamList->Count = 0;
            break;
        }

        case ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_SIGNATURE:
        {
            PACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER SimpleInt;

            if (!AcpiVerifyInBuffer(IoStack, sizeof(*SimpleInt)))
            {
                DPRINT1("Buffer too small\n");
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            Arg = ExAllocatePoolUninitialized(NonPagedPool, sizeof(*Arg), TAG_ACPI_PARAMETERS_LIST);
            if (!Arg)
                return STATUS_INSUFFICIENT_RESOURCES;

            ParamList->Count = 1;
            ParamList->Pointer = Arg;

            SimpleInt = Irp->AssociatedIrp.SystemBuffer;

            Arg->Type = ACPI_TYPE_INTEGER;
            Arg->Integer.Value = (ULONG64)SimpleInt->IntegerArgument;
            break;
        }

        case ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING_SIGNATURE:
        {
            PACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING SimpleStr;

            if (!AcpiVerifyInBuffer(IoStack, sizeof(*SimpleStr)))
            {
                DPRINT1("Buffer too small\n");
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            Arg = ExAllocatePoolUninitialized(NonPagedPool, sizeof(*Arg), TAG_ACPI_PARAMETERS_LIST);
            if (!Arg)
                return STATUS_INSUFFICIENT_RESOURCES;

            ParamList->Count = 1;
            ParamList->Pointer = Arg;

            SimpleStr = Irp->AssociatedIrp.SystemBuffer;

            Arg->Type = ACPI_TYPE_STRING;
            Arg->String.Pointer = (PCHAR)SimpleStr->String;
            Arg->String.Length = SimpleStr->StringLength;
            break;
        }

        case ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE:
        {
            PACPI_EVAL_INPUT_BUFFER_COMPLEX ComplexBuffer;
            PACPI_METHOD_ARGUMENT Argument;
            ULONG i, Length, Offset, ArgumentsSize;
            NTSTATUS Status;

            if (!AcpiVerifyInBuffer(IoStack, sizeof(*ComplexBuffer)))
            {
                DPRINT1("Buffer too small\n");
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            ComplexBuffer = Irp->AssociatedIrp.SystemBuffer;

            ParamList->Count = ComplexBuffer->ArgumentCount;
            if (ParamList->Count == 0)
            {
                DPRINT1("No arguments\n");
                return STATUS_ACPI_INCORRECT_ARGUMENT_COUNT;
            }

            Status = RtlULongMult(ParamList->Count, sizeof(*Arg), &ArgumentsSize);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Invalid argument count 0x%lx\n", ParamList->Count);
                return STATUS_ACPI_INCORRECT_ARGUMENT_COUNT;
            }

            Arg = ExAllocatePoolUninitialized(NonPagedPool,
                                              ArgumentsSize,
                                              TAG_ACPI_PARAMETERS_LIST);
            if (!Arg)
                return STATUS_INSUFFICIENT_RESOURCES;

            ParamList->Pointer = Arg;

            Argument = ComplexBuffer->Argument;
            Length = FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_COMPLEX, Argument);

            for (i = 0; i < ParamList->Count; i++)
            {
                Offset = Length;
                Length += ACPI_METHOD_ARGUMENT_LENGTH_FROM_ARGUMENT(Argument);

                if (!AcpiVerifyInBuffer(IoStack, Length))
                {
                    DPRINT1("Argument buffer outside of argument bounds\n");

                    ExFreePoolWithTag(ParamList->Pointer, TAG_ACPI_PARAMETERS_LIST);
                    return STATUS_ACPI_INVALID_ARGTYPE;
                }

                Status = EvalConvertParameterObjects(Arg, 0, Argument, IoStack, Offset);
                if (!NT_SUCCESS(Status))
                {
                    ExFreePoolWithTag(ParamList->Pointer, TAG_ACPI_PARAMETERS_LIST);
                    return Status;
                }

                Arg++;
                Argument = ACPI_METHOD_NEXT_ARGUMENT(Argument);
            }

            break;
        }

        default:
        {
            DPRINT1("Unsupported input buffer signature: 0x%lx\n", EvalInputBuffer->Signature);
            return STATUS_INVALID_PARAMETER_1;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * @brief Deallocates the memory for all sub-objects (elements) in a package.
 */
static
CODE_SEG("PAGE")
VOID
EvalFreeParameterArgument(
    _In_ ACPI_OBJECT* Arg,
    _In_ ULONG Depth)
{
    ULONG i;

    PAGED_CODE();

    if (Depth >= ACPI_MAX_PACKAGE_DEPTH)
    {
        ASSERT(FALSE);
        return;
    }

    if (Arg->Type == ACPI_TYPE_PACKAGE)
    {
        for (i = 0; i < Arg->Package.Count; i++)
        {
            EvalFreeParameterArgument(&Arg->Package.Elements[i], Depth + 1);
        }

        /* Check if the package isn't empty, and free it */
        if (Arg->Package.Elements)
            ExFreePoolWithTag(Arg->Package.Elements, TAG_ACPI_PACKAGE_LIST);
    }
}

/**
 * @brief Deallocates the given array of ACPI_OBJECTs.
 */
static
CODE_SEG("PAGE")
VOID
EvalFreeParametersList(
    _In_ ACPI_OBJECT_LIST* ParamList)
{
    ACPI_OBJECT* Arg;
    ULONG i;

    PAGED_CODE();

    Arg = ParamList->Pointer;
    for (i = 0; i < ParamList->Count; i++)
    {
        EvalFreeParameterArgument(Arg++, 0);
    }

    ExFreePoolWithTag(ParamList->Pointer, TAG_ACPI_PARAMETERS_LIST);
}

/**
 * @brief Converts the provided value of ACPI_STATUS to NTSTATUS return value.
 */
static
CODE_SEG("PAGE")
NTSTATUS
EvalAcpiStatusToNtStatus(
    _In_ ACPI_STATUS AcpiStatus)
{
    PAGED_CODE();

    if (ACPI_ENV_EXCEPTION(AcpiStatus))
    {
        switch (AcpiStatus)
        {
            case AE_NOT_FOUND:
            case AE_NOT_EXIST:
                return STATUS_OBJECT_NAME_NOT_FOUND;

            case AE_NO_MEMORY:
                return STATUS_INSUFFICIENT_RESOURCES;

            case AE_SUPPORT:
                return STATUS_ACPI_INCORRECT_ARGUMENT_COUNT;

            case AE_TIME:
                return STATUS_IO_TIMEOUT;

            case AE_NO_HARDWARE_RESPONSE:
                return STATUS_IO_DEVICE_ERROR;

            case AE_STACK_OVERFLOW:
                return STATUS_ACPI_STACK_OVERFLOW;

            default:
                break;
        }
    }

    if (ACPI_AML_EXCEPTION(AcpiStatus))
    {
        switch (AcpiStatus)
        {
            case AE_AML_UNINITIALIZED_ARG:
                return STATUS_ACPI_INCORRECT_ARGUMENT_COUNT;

            default:
                break;
        }
    }

    return STATUS_UNSUCCESSFUL;
}

/**
 * @brief Evaluates an ACPI namespace object.
 */
static
CODE_SEG("PAGE")
ACPI_STATUS
EvalEvaluateObject(
    _In_ PPDO_DEVICE_DATA DeviceData,
    _In_ PACPI_EVAL_INPUT_BUFFER EvalInputBuffer,
    _In_ ACPI_OBJECT_LIST* ParamList,
    _In_ ACPI_BUFFER* ReturnBuffer)
{
    CHAR MethodName[ACPI_OBJECT_NAME_LENGTH];

    PAGED_CODE();

    RtlCopyMemory(MethodName, EvalInputBuffer->MethodName, ACPI_OBJECT_NAME_LENGTH);
    MethodName[ACPI_OBJECT_NAME_LENGTH - 1] = ANSI_NULL;

    return AcpiEvaluateObject(DeviceData->AcpiHandle, MethodName, ParamList, ReturnBuffer);
}

/**
 * @brief Writes the results from the evaluation into the output IRP buffer.
 */
static
CODE_SEG("PAGE")
NTSTATUS
EvalCreateOutputArguments(
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack,
    _In_ ACPI_BUFFER* ReturnBuffer)
{
    ACPI_OBJECT* Obj;
    ULONG ExtraParamLength, OutputBufSize;
    PACPI_EVAL_OUTPUT_BUFFER OutputBuffer;
    NTSTATUS Status;
    ULONG Count;

    PAGED_CODE();

    /* If we didn't get anything back then we're done */
    if (!ReturnBuffer->Pointer || ReturnBuffer->Length == 0)
        return STATUS_SUCCESS;

    /* No output buffer is provided, we're done */
    if (IoStack->Parameters.DeviceIoControl.OutputBufferLength == 0)
        return STATUS_SUCCESS;

    if (!AcpiVerifyOutBuffer(IoStack, sizeof(*OutputBuffer)))
    {
        DPRINT1("Buffer too small\n");

        return STATUS_BUFFER_TOO_SMALL;
    }

    Obj = ReturnBuffer->Pointer;

    Status = EvalGetElementSize(Obj, 0, &Count, &ExtraParamLength);
    if (!NT_SUCCESS(Status))
        return Status;

    OutputBufSize = FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) + ExtraParamLength;

#ifdef UNIT_TEST
    OutputBuffer = Irp->OutputBuffer;
#else
    OutputBuffer = Irp->AssociatedIrp.SystemBuffer;
#endif
    OutputBuffer->Signature = ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE;
    OutputBuffer->Length = OutputBufSize;
    OutputBuffer->Count = Count;

    if (!AcpiVerifyOutBuffer(IoStack, OutputBufSize))
    {
        DPRINT("Buffer too small (%lu/%lu)\n",
               IoStack->Parameters.DeviceIoControl.OutputBufferLength,
               OutputBufSize);

        Irp->IoStatus.Information = OutputBufSize;
        return STATUS_BUFFER_OVERFLOW;
    }

    Status = EvalConvertEvaluationResults(OutputBuffer->Argument, 0, Obj);
    if (!NT_SUCCESS(Status))
        return Status;

    Irp->IoStatus.Information = OutputBufSize;
    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
Bus_PDO_EvalMethod(
    _In_ PPDO_DEVICE_DATA DeviceData,
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PACPI_EVAL_INPUT_BUFFER EvalInputBuffer;
    ACPI_OBJECT_LIST ParamList;
    ACPI_STATUS AcpiStatus;
    NTSTATUS Status;
    ACPI_BUFFER ReturnBuffer = { ACPI_ALLOCATE_BUFFER, NULL };

    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    EvalInputBuffer = Irp->AssociatedIrp.SystemBuffer;

    Status = EvalCreateParametersList(Irp, IoStack, EvalInputBuffer, &ParamList);
    if (!NT_SUCCESS(Status))
        return Status;

    AcpiStatus = EvalEvaluateObject(DeviceData, EvalInputBuffer, &ParamList, &ReturnBuffer);

    if (ParamList.Count != 0)
        EvalFreeParametersList(&ParamList);

    if (!ACPI_SUCCESS(AcpiStatus))
    {
        DPRINT("Query method '%.4s' failed on %p with status 0x%04lx\n",
               EvalInputBuffer->MethodName,
               DeviceData->AcpiHandle,
               AcpiStatus);

        return EvalAcpiStatusToNtStatus(AcpiStatus);
    }

    Status = EvalCreateOutputArguments(Irp, IoStack, &ReturnBuffer);

    if (ReturnBuffer.Pointer)
        AcpiOsFree(ReturnBuffer.Pointer);

    return Status;
}

#ifndef UNIT_TEST
CODE_SEG("PAGE")
VOID
NTAPI
Bus_PDO_EvalMethodWorker(
    _In_ PVOID Parameter)
{
    PEVAL_WORKITEM_DATA WorkItemData = Parameter;
    NTSTATUS Status;
    PIRP Irp;

    PAGED_CODE();

    Irp = WorkItemData->Irp;

    Status = Bus_PDO_EvalMethod(WorkItemData->DeviceData, Irp);

    ExFreePoolWithTag(WorkItemData, 'ipcA');

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}
#endif /* UNIT_TEST */
