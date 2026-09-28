/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ACPI IOCTLs on PDOs and filter DOs
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <acpiioct.h>
#include <uacpi/tables.h>
#include <uacpi/acpi.h>
#include <debug.h>

/* Codes and signatures the WDK hides below their NTDDI floor */
#ifndef IOCTL_ACPI_GET_DEVICE_INFORMATION
#define IOCTL_ACPI_GET_DEVICE_INFORMATION \
    CTL_CODE(FILE_DEVICE_ACPI, 10, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#endif

#ifndef IOCTL_ACPI_GET_DEVICE_SPECIFIC_DATA
#define IOCTL_ACPI_GET_DEVICE_SPECIFIC_DATA \
    CTL_CODE(FILE_DEVICE_ACPI, 14, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#endif

#ifndef IOCTL_ACPI_EVAL_METHOD_V2
#define IOCTL_ACPI_EVAL_METHOD_V2 \
    CTL_CODE(FILE_DEVICE_ACPI, 15, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#endif

#ifndef IOCTL_ACPI_ASYNC_EVAL_METHOD_V2
#define IOCTL_ACPI_ASYNC_EVAL_METHOD_V2 \
    CTL_CODE(FILE_DEVICE_ACPI, 16, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#endif

#ifndef IOCTL_ACPI_EVAL_METHOD_V2_EX
#define IOCTL_ACPI_EVAL_METHOD_V2_EX \
    CTL_CODE(FILE_DEVICE_ACPI, 17, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#endif

#ifndef IOCTL_ACPI_ASYNC_EVAL_METHOD_V2_EX
#define IOCTL_ACPI_ASYNC_EVAL_METHOD_V2_EX \
    CTL_CODE(FILE_DEVICE_ACPI, 18, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#endif

#ifndef IOCTL_ACPI_GET_DEVICE_INFORMATION_SIGNATURE
#define IOCTL_ACPI_GET_DEVICE_INFORMATION_SIGNATURE 'JieA'
#endif

#ifndef IOCTL_ACPI_GET_DEVICE_SPECIFIC_DATA_SIGNATURE
#define IOCTL_ACPI_GET_DEVICE_SPECIFIC_DATA_SIGNATURE 'HieA'
#endif

/* FILE_DEVICE_ACPI functions without a public contract */
#define UACPINT_ACPI_CTL_CODE(Function) \
    CTL_CODE(FILE_DEVICE_ACPI, Function, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define UACPINT_IOCTL_REGISTER_OPREGION         UACPINT_ACPI_CTL_CODE(2)
#define UACPINT_IOCTL_UNREGISTER_OPREGION       UACPINT_ACPI_CTL_CODE(3)
#define UACPINT_IOCTL_QUERY_BIOS_NAME           UACPINT_ACPI_CTL_CODE(9)
#define UACPINT_IOCTL_TRANSLATE_BIOS_RESOURCES  UACPINT_ACPI_CTL_CODE(11)
#define UACPINT_IOCTL_REGISTER_FIRMWARE_LOCK    UACPINT_ACPI_CTL_CODE(12)
#define UACPINT_IOCTL_UNREGISTER_FIRMWARE_LOCK  UACPINT_ACPI_CTL_CODE(13)

/* Processor object information, same code and layout from Vista to Windows 10 */
#define UACPINT_IOCTL_GET_PROCESSOR_OBJ_INFO    0x00294180u

/* Package nesting limit in both directions */
#define UACPINT_EVAL_MAX_DEPTH                  8

/* Room for an absolute _EX method path */
#define UACPINT_EVAL_PATH_LENGTH                260

/* A namespace name is four characters and the NUL */
#define UACPINT_NODE_NAME_LENGTH                5

/* ENUM_CHILDREN records are packed, the caller does not round the stride */
#define UACPINT_ENUM_CHILD_RECORD_LENGTH \
    ((ULONG)(FIELD_OFFSET(ACPI_ENUM_CHILD, Name) + UACPINT_NODE_NAME_LENGTH))

/*
 * Pended evaluation, used when an eval IOCTL arrives above PASSIVE_LEVEL
 */
typedef struct _UACPINT_EVAL_WORK
{
    PIO_WORKITEM          WorkItem;
    uacpi_namespace_node *Node;
    CHAR                  Name[8];
    PIRP                  Irp;
} UACPINT_EVAL_WORK, *PUACPINT_EVAL_WORK;

/*
 * Processor object information reply
 */
typedef struct _UACPINT_PROCESSOR_OBJ_INFO
{
    ULONG ProcessorId;
    ULONG BlockAddress;
    UCHAR BlockLength;
    ULONG ApicId;          ///< only when the caller has room for it
} UACPINT_PROCESSOR_OBJ_INFO, *PUACPINT_PROCESSOR_OBJ_INFO;

C_ASSERT(FIELD_OFFSET(UACPINT_PROCESSOR_OBJ_INFO, ApicId) == 12);
C_ASSERT(sizeof(UACPINT_PROCESSOR_OBJ_INFO) == 16);

static
NTSTATUS
NTAPI
UacpiNtArgumentsToObjects(
    _In_reads_bytes_(BytesAvailable) PACPI_METHOD_ARGUMENT First,
    _In_ ULONG Count,
    _In_ ULONG BytesAvailable,
    _In_ ULONG Depth,
    _Out_ uacpi_object_array *Array);

static
VOID
NTAPI
UacpiNtCopyNodeName(
    _In_ uacpi_namespace_node *Node,
    _Out_writes_(UACPINT_NODE_NAME_LENGTH) PCHAR Name)
{
    uacpi_object_name ObjectName = uacpi_namespace_node_name(Node);

    RtlCopyMemory(Name, ObjectName.text, sizeof(ObjectName.text));
    Name[4] = ANSI_NULL;
}

static
NTSTATUS
NTAPI
UacpiNtAllocateObjectArray(
    _Out_ uacpi_object_array *Array,
    _In_ ULONG Count)
{
    Array->count = 0;
    Array->objects = ExAllocatePoolWithTag(NonPagedPool,
                                           Count * sizeof(*Array->objects),
                                           UACPINT_POOL_TAG);
    if (!Array->objects)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(Array->objects, Count * sizeof(*Array->objects));
    Array->count = Count;
    return STATUS_SUCCESS;
}

static
VOID
NTAPI
UacpiNtFreeObjectArray(
    _Inout_ uacpi_object_array *Array)
{
    uacpi_size i;

    if (!Array->objects)
        return;

    for (i = 0; i < Array->count; i++)
    {
        if (Array->objects[i])
            uacpi_object_unref(Array->objects[i]);
    }

    ExFreePoolWithTag(Array->objects, UACPINT_POOL_TAG);
    Array->objects = NULL;
    Array->count = 0;
}

/* Counts the arguments packed back to back in a package argument */
static
NTSTATUS
NTAPI
UacpiNtCountArguments(
    _In_reads_bytes_(Length) PACPI_METHOD_ARGUMENT First,
    _In_ ULONG Length,
    _Out_ PULONG Count)
{
    PACPI_METHOD_ARGUMENT Argument = First;
    ULONG ArgumentLength;

    *Count = 0;
    while (Length >= (ULONG)FIELD_OFFSET(ACPI_METHOD_ARGUMENT, Data))
    {
        ArgumentLength = ACPI_METHOD_ARGUMENT_LENGTH(Argument->DataLength);
        if (ArgumentLength > Length)
            return STATUS_INVALID_PARAMETER;

        (*Count)++;
        Length -= ArgumentLength;
        Argument = ACPI_METHOD_NEXT_ARGUMENT(Argument);
    }

    return STATUS_SUCCESS;
}

/* One wire argument to one uACPI object, Consumed is its wire size */
static
NTSTATUS
NTAPI
UacpiNtArgumentToObject(
    _In_reads_bytes_(BytesAvailable) PACPI_METHOD_ARGUMENT Argument,
    _In_ ULONG BytesAvailable,
    _In_ ULONG Depth,
    _Out_ uacpi_object **Object,
    _Out_ PULONG Consumed)
{
    uacpi_object_array Nested;
    uacpi_data_view View;
    ULONG ArgumentLength;
    ULONG NestedCount;
    NTSTATUS Status;

    *Object = NULL;
    *Consumed = 0;

    if (BytesAvailable < (ULONG)FIELD_OFFSET(ACPI_METHOD_ARGUMENT, Data))
        return STATUS_INVALID_PARAMETER;

    ArgumentLength = ACPI_METHOD_ARGUMENT_LENGTH(Argument->DataLength);
    if (ArgumentLength > BytesAvailable)
        return STATUS_INVALID_PARAMETER;

    *Consumed = ArgumentLength;

    switch (Argument->Type)
    {
        case ACPI_METHOD_ARGUMENT_INTEGER:
            *Object = uacpi_object_create_integer(Argument->Argument);
            break;

        /* DataLength counts the NUL */
        case ACPI_METHOD_ARGUMENT_STRING:
            if (Argument->DataLength == 0 || Argument->Data[Argument->DataLength - 1] != '\0')
                return STATUS_INVALID_PARAMETER;
            *Object = uacpi_object_create_cstring((const uacpi_char *)Argument->Data);
            break;

        case ACPI_METHOD_ARGUMENT_BUFFER:
            View.bytes = (uacpi_u8 *)Argument->Data;
            View.length = Argument->DataLength;
            *Object = uacpi_object_create_buffer(View);
            break;

        case ACPI_METHOD_ARGUMENT_PACKAGE:
            if (Depth >= UACPINT_EVAL_MAX_DEPTH)
                return STATUS_INVALID_PARAMETER;

            Status = UacpiNtCountArguments((PACPI_METHOD_ARGUMENT)Argument->Data,
                                           Argument->DataLength,
                                           &NestedCount);
            if (!NT_SUCCESS(Status))
                return Status;

            Status = UacpiNtArgumentsToObjects((PACPI_METHOD_ARGUMENT)Argument->Data,
                                               NestedCount,
                                               Argument->DataLength,
                                               Depth + 1,
                                               &Nested);
            if (!NT_SUCCESS(Status))
                return Status;

            /* The package takes its own references */
            *Object = uacpi_object_create_package(Nested);
            UacpiNtFreeObjectArray(&Nested);
            break;

        default:
            return STATUS_INVALID_PARAMETER;
    }

    return *Object ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
}

static
NTSTATUS
NTAPI
UacpiNtArgumentsToObjects(
    _In_reads_bytes_(BytesAvailable) PACPI_METHOD_ARGUMENT First,
    _In_ ULONG Count,
    _In_ ULONG BytesAvailable,
    _In_ ULONG Depth,
    _Out_ uacpi_object_array *Array)
{
    PACPI_METHOD_ARGUMENT Argument = First;
    ULONG Consumed;
    NTSTATUS Status;
    ULONG i;

    Array->objects = NULL;
    Array->count = 0;

    if (Count == 0)
        return STATUS_SUCCESS;

    /* Every argument takes at least a header and a ULONG */
    if (Count > BytesAvailable / ACPI_METHOD_ARGUMENT_LENGTH(0))
        return STATUS_INVALID_PARAMETER;

    Status = UacpiNtAllocateObjectArray(Array, Count);
    if (!NT_SUCCESS(Status))
        return Status;

    for (i = 0; i < Count; i++)
    {
        Status = UacpiNtArgumentToObject(Argument, BytesAvailable, Depth, &Array->objects[i], &Consumed);
        if (!NT_SUCCESS(Status))
        {
            UacpiNtFreeObjectArray(Array);
            return Status;
        }

        BytesAvailable -= Consumed;
        Argument = ACPI_METHOD_NEXT_ARGUMENT(Argument);
    }

    return STATUS_SUCCESS;
}

/* Wire size of an object once serialized */
static
NTSTATUS
NTAPI
UacpiNtObjectWireSize(
    _In_ uacpi_object *Object,
    _In_ ULONG Depth,
    _Out_ PULONG Size)
{
    uacpi_object_array Package;
    uacpi_data_view View;
    ULONG ElementSize;
    NTSTATUS Status;
    ULONG Total;
    uacpi_size i;

    *Size = 0;

    /* DataLength is a USHORT, anything bigger has no wire form */
    switch (uacpi_object_get_type(Object))
    {
        case UACPI_OBJECT_INTEGER:
            *Size = ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG));
            return STATUS_SUCCESS;

        /* The view excludes the NUL, the wire string carries it */
        case UACPI_OBJECT_STRING:
            if (uacpi_unlikely_error(uacpi_object_get_string(Object, &View)))
                return STATUS_UNSUCCESSFUL;
            if (View.length + 1 > MAXUSHORT)
                return STATUS_ACPI_INVALID_DATA;
            *Size = ACPI_METHOD_ARGUMENT_LENGTH((ULONG)View.length + 1);
            return STATUS_SUCCESS;

        case UACPI_OBJECT_BUFFER:
            if (uacpi_unlikely_error(uacpi_object_get_buffer(Object, &View)))
                return STATUS_UNSUCCESSFUL;
            if (View.length > MAXUSHORT)
                return STATUS_ACPI_INVALID_DATA;
            *Size = ACPI_METHOD_ARGUMENT_LENGTH((ULONG)View.length);
            return STATUS_SUCCESS;

        case UACPI_OBJECT_PACKAGE:
            if (Depth >= UACPINT_EVAL_MAX_DEPTH)
                return STATUS_INVALID_PARAMETER;
            if (uacpi_unlikely_error(uacpi_object_get_package(Object, &Package)))
                return STATUS_UNSUCCESSFUL;

            Total = 0;
            for (i = 0; i < Package.count; i++)
            {
                Status = UacpiNtObjectWireSize(Package.objects[i], Depth + 1, &ElementSize);
                if (!NT_SUCCESS(Status))
                    return Status;
                Total += ElementSize;
            }

            if (Total > MAXUSHORT)
                return STATUS_ACPI_INVALID_DATA;
            *Size = ACPI_METHOD_ARGUMENT_LENGTH(Total);
            return STATUS_SUCCESS;

        /* Fields, mutexes and the like */
        default:
            return STATUS_ACPI_INVALID_DATA;
    }
}

/* Serializes Object, which UacpiNtObjectWireSize already sized and validated */
static
NTSTATUS
NTAPI
UacpiNtObjectToArgument(
    _In_ uacpi_object *Object,
    _Out_ PACPI_METHOD_ARGUMENT Argument,
    _Out_ PULONG Written)
{
    PACPI_METHOD_ARGUMENT Nested;
    uacpi_object_array Package;
    uacpi_data_view View;
    uacpi_u64 Value = 0;
    ULONG ElementSize;
    NTSTATUS Status;
    ULONG Total;
    uacpi_size i;

    *Written = 0;

    switch (uacpi_object_get_type(Object))
    {
        /* Version 1 wire integers are 32 bits wide */
        case UACPI_OBJECT_INTEGER:
            (VOID)uacpi_object_get_integer(Object, &Value);
            Argument->Type = ACPI_METHOD_ARGUMENT_INTEGER;
            Argument->DataLength = sizeof(ULONG);
            Argument->Argument = (ULONG)Value;
            break;

        case UACPI_OBJECT_STRING:
            if (uacpi_unlikely_error(uacpi_object_get_string(Object, &View)))
                return STATUS_UNSUCCESSFUL;
            Argument->Type = ACPI_METHOD_ARGUMENT_STRING;
            Argument->DataLength = (USHORT)(View.length + 1);
            RtlCopyMemory(Argument->Data, View.const_bytes, View.length);
            Argument->Data[View.length] = '\0';
            break;

        case UACPI_OBJECT_BUFFER:
            if (uacpi_unlikely_error(uacpi_object_get_buffer(Object, &View)))
                return STATUS_UNSUCCESSFUL;
            Argument->Type = ACPI_METHOD_ARGUMENT_BUFFER;
            Argument->DataLength = (USHORT)View.length;
            RtlCopyMemory(Argument->Data, View.const_bytes, View.length);
            break;

        case UACPI_OBJECT_PACKAGE:
            if (uacpi_unlikely_error(uacpi_object_get_package(Object, &Package)))
                return STATUS_UNSUCCESSFUL;

            Total = 0;
            Nested = (PACPI_METHOD_ARGUMENT)Argument->Data;
            for (i = 0; i < Package.count; i++)
            {
                Status = UacpiNtObjectToArgument(Package.objects[i], Nested, &ElementSize);
                if (!NT_SUCCESS(Status))
                    return Status;

                Total += ElementSize;
                Nested = (PACPI_METHOD_ARGUMENT)((PUCHAR)Nested + ElementSize);
            }

            Argument->Type = ACPI_METHOD_ARGUMENT_PACKAGE;
            Argument->DataLength = (USHORT)Total;
            break;

        default:
            return STATUS_ACPI_INVALID_DATA;
    }

    *Written = ACPI_METHOD_ARGUMENT_LENGTH(Argument->DataLength);
    return STATUS_SUCCESS;
}

/* StringLength counts the NUL and must fit in what follows the header */
static
BOOLEAN
NTAPI
UacpiNtStringArgumentValid(
    _In_ ULONG Available,
    _In_ ULONG StringLength,
    _In_reads_bytes_(StringLength) const UCHAR *String)
{
    return StringLength != 0 &&
           StringLength <= Available &&
           String[StringLength - 1] == '\0';
}

/* Wraps a single created object into Args */
static
NTSTATUS
NTAPI
UacpiNtSingleArgument(
    _In_opt_ uacpi_object *Object,
    _Out_ uacpi_object_array *Args)
{
    NTSTATUS Status;

    Status = UacpiNtAllocateObjectArray(Args, 1);
    if (!NT_SUCCESS(Status))
    {
        if (Object)
            uacpi_object_unref(Object);
        return Status;
    }

    Args->objects[0] = Object;
    return Object ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
}

/* Decodes an eval input buffer into a method path and its arguments */
static
NTSTATUS
NTAPI
UacpiNtDecodeEvalInput(
    _In_reads_bytes_(InputLength) PVOID Input,
    _In_ ULONG InputLength,
    _Out_writes_z_(UACPINT_EVAL_PATH_LENGTH) PCHAR Path,
    _Inout_ uacpi_object_array *Args)
{
    PACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_EX IntegerEx;
    PACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING_EX StringEx;
    PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX ComplexEx;
    PACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER Integer;
    PACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING String;
    PACPI_EVAL_INPUT_BUFFER_COMPLEX Complex;
    PACPI_EVAL_INPUT_BUFFER_EX PlainEx;
    PACPI_EVAL_INPUT_BUFFER Plain;
    ULONG Offset;

    Path[0] = ANSI_NULL;

    if (InputLength < sizeof(ULONG))
        return STATUS_INVALID_PARAMETER;

    /* The original names are four characters, the _EX ones are NUL terminated paths */
    switch (*(ULONG UNALIGNED *)Input)
    {
        case ACPI_EVAL_INPUT_BUFFER_SIGNATURE:
            Plain = Input;
            if (InputLength < sizeof(*Plain))
                return STATUS_INVALID_PARAMETER;
            RtlCopyMemory(Path, Plain->MethodName, sizeof(Plain->MethodName));
            Path[sizeof(Plain->MethodName)] = ANSI_NULL;
            return STATUS_SUCCESS;

        case ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_SIGNATURE:
            Integer = Input;
            if (InputLength < sizeof(*Integer))
                return STATUS_INVALID_PARAMETER;
            RtlCopyMemory(Path, Integer->MethodName, sizeof(Integer->MethodName));
            Path[sizeof(Integer->MethodName)] = ANSI_NULL;
            return UacpiNtSingleArgument(uacpi_object_create_integer(Integer->IntegerArgument), Args);

        case ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING_SIGNATURE:
            String = Input;
            Offset = FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING, String);
            if (InputLength < Offset ||
                !UacpiNtStringArgumentValid(InputLength - Offset, String->StringLength, String->String))
            {
                return STATUS_INVALID_PARAMETER;
            }
            RtlCopyMemory(Path, String->MethodName, sizeof(String->MethodName));
            Path[sizeof(String->MethodName)] = ANSI_NULL;
            return UacpiNtSingleArgument(uacpi_object_create_cstring((const uacpi_char *)String->String), Args);

        case ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE:
            Complex = Input;
            Offset = FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_COMPLEX, Argument);
            if (InputLength < Offset || InputLength < Complex->Size)
                return STATUS_INVALID_PARAMETER;
            RtlCopyMemory(Path, Complex->MethodName, sizeof(Complex->MethodName));
            Path[sizeof(Complex->MethodName)] = ANSI_NULL;
            return UacpiNtArgumentsToObjects((PACPI_METHOD_ARGUMENT)Complex->Argument,
                                             Complex->ArgumentCount,
                                             InputLength - Offset,
                                             0,
                                             Args);

        case ACPI_EVAL_INPUT_BUFFER_SIGNATURE_EX:
            PlainEx = Input;
            if (InputLength < sizeof(*PlainEx))
                return STATUS_INVALID_PARAMETER;
            RtlStringCbCopyNA(Path, UACPINT_EVAL_PATH_LENGTH, PlainEx->MethodName, sizeof(PlainEx->MethodName));
            return STATUS_SUCCESS;

        case ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_SIGNATURE_EX:
            IntegerEx = Input;
            if (InputLength < sizeof(*IntegerEx))
                return STATUS_INVALID_PARAMETER;
            RtlStringCbCopyNA(Path, UACPINT_EVAL_PATH_LENGTH, IntegerEx->MethodName, sizeof(IntegerEx->MethodName));
            return UacpiNtSingleArgument(uacpi_object_create_integer(IntegerEx->IntegerArgument), Args);

        case ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING_SIGNATURE_EX:
            StringEx = Input;
            Offset = FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING_EX, String);
            if (InputLength < Offset ||
                !UacpiNtStringArgumentValid(InputLength - Offset, StringEx->StringLength, StringEx->String))
            {
                return STATUS_INVALID_PARAMETER;
            }
            RtlStringCbCopyNA(Path, UACPINT_EVAL_PATH_LENGTH, StringEx->MethodName, sizeof(StringEx->MethodName));
            return UacpiNtSingleArgument(uacpi_object_create_cstring((const uacpi_char *)StringEx->String), Args);

        case ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE_EX:
            ComplexEx = Input;
            Offset = FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX, Argument);
            if (InputLength < Offset || InputLength < ComplexEx->Size)
                return STATUS_INVALID_PARAMETER;
            RtlStringCbCopyNA(Path, UACPINT_EVAL_PATH_LENGTH, ComplexEx->MethodName, sizeof(ComplexEx->MethodName));
            return UacpiNtArgumentsToObjects((PACPI_METHOD_ARGUMENT)ComplexEx->Argument,
                                             ComplexEx->ArgumentCount,
                                             InputLength - Offset,
                                             0,
                                             Args);

        default:
            return STATUS_INVALID_PARAMETER;
    }
}

/* Serializes a method result, a top level package becomes one argument per element */
static
NTSTATUS
NTAPI
UacpiNtWriteEvalOutput(
    _In_ uacpi_object *Result,
    _Out_writes_bytes_opt_(OutputLength) PVOID Output,
    _In_ ULONG OutputLength,
    _Out_ PULONG_PTR Information)
{
    PACPI_EVAL_OUTPUT_BUFFER OutputBuffer = Output;
    PACPI_METHOD_ARGUMENT Argument;
    uacpi_object_array Package;
    uacpi_object **Objects;
    ULONG Required;
    NTSTATUS Status;
    ULONG Depth;
    ULONG Count;
    ULONG Size;
    ULONG i;

    *Information = 0;

    if (uacpi_object_get_type(Result) == UACPI_OBJECT_PACKAGE &&
        uacpi_likely_success(uacpi_object_get_package(Result, &Package)))
    {
        Objects = Package.objects;
        Count = (ULONG)Package.count;
        Depth = 1;
    }
    else
    {
        Objects = &Result;
        Count = 1;
        Depth = 0;
    }

    Required = FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument);
    for (i = 0; i < Count; i++)
    {
        Status = UacpiNtObjectWireSize(Objects[i], Depth, &Size);
        if (!NT_SUCCESS(Status))
            return Status;
        Required += Size;
    }

    if (!OutputBuffer || OutputLength < (ULONG)FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument))
        return STATUS_BUFFER_TOO_SMALL;

    OutputBuffer->Signature = ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE;
    OutputBuffer->Length = Required;

    /* Two pass callers size their buffer from Length */
    if (OutputLength < Required)
    {
        OutputBuffer->Count = 0;
        *Information = FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument);
        return STATUS_BUFFER_OVERFLOW;
    }

    OutputBuffer->Count = Count;
    Argument = (PACPI_METHOD_ARGUMENT)OutputBuffer->Argument;
    for (i = 0; i < Count; i++)
    {
        Status = UacpiNtObjectToArgument(Objects[i], Argument, &Size);
        if (!NT_SUCCESS(Status))
            return Status;
        Argument = (PACPI_METHOD_ARGUMENT)((PUCHAR)Argument + Size);
    }

    *Information = Required;
    return STATUS_SUCCESS;
}

/* Runs an eval IOCTL, all of them are METHOD_BUFFERED with one SystemBuffer both ways */
static
NTSTATUS
NTAPI
UacpiNtEvaluate(
    _In_opt_ uacpi_namespace_node *Node,
    _In_z_ const char *TraceName,
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    PVOID Buffer = Irp->AssociatedIrp.SystemBuffer;
    uacpi_object_array Args = { 0 };
    CHAR Path[UACPINT_EVAL_PATH_LENGTH];
    uacpi_object *Result = NULL;
    ULONG_PTR Information = 0;
    uacpi_status UacpiStatus;
    NTSTATUS Status;
    SIZE_T Length;

    if (!Node || !GlobalAcpiFdo || !GlobalAcpiFdo->InterpreterReady)
    {
        Status = STATUS_DEVICE_NOT_READY;
    }
    else
    {
        Status = UacpiNtDecodeEvalInput(Buffer,
                                        IoStack->Parameters.DeviceIoControl.InputBufferLength,
                                        Path,
                                        &Args);
    }

    if (NT_SUCCESS(Status))
    {
        /* Four character names may be padded with spaces */
        Length = strlen(Path);
        while (Length > 0 && Path[Length - 1] == ' ')
            Path[--Length] = ANSI_NULL;

        DPRINT("EVAL %s on %s\n", Path, TraceName);
        UacpiStatus = uacpi_eval(Node, Path, Args.count ? &Args : NULL, &Result);
        if (uacpi_unlikely_error(UacpiStatus))
        {
            if (UacpiStatus == UACPI_STATUS_NOT_FOUND)
            {
                DPRINT("EVAL %s on %s: not found\n", Path, TraceName);
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
            }
            else
            {
                DPRINT1("EVAL %s on %s failed: %s\n", Path, TraceName, uacpi_status_to_string(UacpiStatus));
                Status = STATUS_UNSUCCESSFUL;
            }
        }
        else if (Result)
        {
            Status = UacpiNtWriteEvalOutput(Result,
                                            Buffer,
                                            IoStack->Parameters.DeviceIoControl.OutputBufferLength,
                                            &Information);
        }
    }

    if (Result)
        uacpi_object_unref(Result);
    UacpiNtFreeObjectArray(&Args);

    Irp->IoStatus.Information = Information;
    return Status;
}

static
VOID
NTAPI
UacpiNtEvalWorker(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context)
{
    PUACPINT_EVAL_WORK Work = Context;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(DeviceObject);

    Status = UacpiNtEvaluate(Work->Node, Work->Name, Work->Irp);
    UacpiNtCompleteIrp(Work->Irp, Status, Work->Irp->IoStatus.Information);

    IoFreeWorkItem(Work->WorkItem);
    ExFreePoolWithTag(Work, UACPINT_POOL_TAG);
}

/* AML only runs at PASSIVE_LEVEL, so a work item takes the IRP */
static
NTSTATUS
NTAPI
UacpiNtQueueEvaluation(
    _In_ uacpi_namespace_node *Node,
    _In_z_ const char *Name,
    _In_ PDEVICE_OBJECT Self,
    _Inout_ PIRP Irp)
{
    PUACPINT_EVAL_WORK Work;

    Work = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Work), UACPINT_POOL_TAG);
    if (!Work)
        return UacpiNtCompleteIrp(Irp, STATUS_INSUFFICIENT_RESOURCES, 0);

    Work->WorkItem = IoAllocateWorkItem(Self);
    if (!Work->WorkItem)
    {
        ExFreePoolWithTag(Work, UACPINT_POOL_TAG);
        return UacpiNtCompleteIrp(Irp, STATUS_INSUFFICIENT_RESOURCES, 0);
    }

    Work->Node = Node;
    RtlStringCbCopyA(Work->Name, sizeof(Work->Name), Name);
    Work->Irp = Irp;

    IoMarkIrpPending(Irp);
    IoQueueWorkItem(Work->WorkItem, UacpiNtEvalWorker, DelayedWorkQueue, Work);
    return STATUS_PENDING;
}

/* Only the instance name is reported, a short buffer gets the size and OVERFLOW */
static
NTSTATUS
NTAPI
UacpiNtGetDeviceInformation(
    _In_opt_ uacpi_namespace_node *Node,
    _In_z_ const char *TraceName,
    _In_ PIRP Irp,
    _Out_ PULONG_PTR Information)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    PACPI_DEVICE_INFORMATION_OUTPUT_BUFFER Info = Irp->AssociatedIrp.SystemBuffer;
    ULONG OutputLength = IoStack->Parameters.DeviceIoControl.OutputBufferLength;
    CHAR Instance[UACPINT_NODE_NAME_LENGTH];
    ULONG Needed;

    *Information = 0;

    if (!Node)
        return STATUS_INVALID_DEVICE_REQUEST;

    UacpiNtCopyNodeName(Node, Instance);

    Needed = sizeof(*Info) + sizeof(Instance);
    if (OutputLength < Needed)
    {
        /* Only report the size when Signature and Size fit */
        if (OutputLength >= RTL_SIZEOF_THROUGH_FIELD(ACPI_DEVICE_INFORMATION_OUTPUT_BUFFER, Size))
        {
            Info->Size = (USHORT)Needed;
            *Information = RTL_SIZEOF_THROUGH_FIELD(ACPI_DEVICE_INFORMATION_OUTPUT_BUFFER, Size);
        }

        DPRINT("%s: device information needs %lu bytes, got %lu\n", TraceName, Needed, OutputLength);
        return STATUS_BUFFER_OVERFLOW;
    }

    RtlZeroMemory(Info, Needed);
    Info->Signature = IOCTL_ACPI_GET_DEVICE_INFORMATION_SIGNATURE;
    Info->Size = (USHORT)Needed;
    Info->Revision = 1;
    Info->InstanceIdOffset = (USHORT)sizeof(*Info);
    Info->InstanceIdLength = (USHORT)sizeof(Instance);
    RtlCopyMemory((PUCHAR)Info + Info->InstanceIdOffset, Instance, sizeof(Instance));

    *Information = Needed;
    DPRINT("%s: device information instance '%s'\n", TraceName, Instance);
    return STATUS_SUCCESS;
}

/* There is no _DSD support, so every property is reported missing */
static
NTSTATUS
NTAPI
UacpiNtGetDeviceSpecificData(
    _In_opt_ uacpi_namespace_node *Node,
    _In_z_ const char *TraceName,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    PACPI_GET_DEVICE_SPECIFIC_DATA Request = Irp->AssociatedIrp.SystemBuffer;

    if (!Node)
        return STATUS_INVALID_DEVICE_REQUEST;

    if (IoStack->Parameters.DeviceIoControl.InputBufferLength <
        (ULONG)FIELD_OFFSET(ACPI_GET_DEVICE_SPECIFIC_DATA, PropertyName))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!Request || Request->Signature != IOCTL_ACPI_GET_DEVICE_SPECIFIC_DATA_SIGNATURE)
        return STATUS_INVALID_PARAMETER;

    DPRINT("%s: device specific data requested, no _DSD support\n", TraceName);
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

/* Absolute namespace path as a NUL terminated WCHAR string */
static
NTSTATUS
NTAPI
UacpiNtQueryBiosName(
    _In_opt_ uacpi_namespace_node *Node,
    _In_z_ const char *TraceName,
    _In_ PIRP Irp,
    _Out_ PULONG_PTR Information)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    const uacpi_char *AbsolutePath;
    UNICODE_STRING WidePath;
    ANSI_STRING AnsiPath;
    NTSTATUS Status;
    USHORT Required;

    *Information = 0;

    if (!Node)
        return STATUS_INVALID_DEVICE_REQUEST;

    AbsolutePath = uacpi_namespace_node_generate_absolute_path(Node);
    if (!AbsolutePath)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlInitAnsiString(&AnsiPath, AbsolutePath);

    /* The size already counts the terminator */
    Required = (USHORT)RtlAnsiStringToUnicodeSize(&AnsiPath);
    if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < Required)
    {
        uacpi_free_absolute_path(AbsolutePath);
        return STATUS_BUFFER_TOO_SMALL;
    }

    WidePath.Buffer = Irp->AssociatedIrp.SystemBuffer;
    WidePath.Length = 0;
    WidePath.MaximumLength = Required;

    Status = RtlAnsiStringToUnicodeString(&WidePath, &AnsiPath, FALSE);
    uacpi_free_absolute_path(AbsolutePath);
    if (!NT_SUCCESS(Status))
        return Status;

    *Information = WidePath.Length + sizeof(WCHAR);
    DPRINT("%s: BIOS name %wZ\n", TraceName, &WidePath);
    return STATUS_SUCCESS;
}

/* USB hubs skip ports that do not report ACPI_OBJECT_HAS_CHILDREN */
static
BOOLEAN
NTAPI
UacpiNtNodeHasChildren(
    _In_ uacpi_namespace_node *Node)
{
    uacpi_namespace_node *Child = UACPI_NULL;

    return uacpi_likely_success(uacpi_namespace_node_next(Node, &Child)) && Child != UACPI_NULL;
}

/* Immediate children only, by their four character names */
static
NTSTATUS
NTAPI
UacpiNtEnumChildren(
    _In_opt_ uacpi_namespace_node *Node,
    _In_z_ const char *TraceName,
    _In_ PIRP Irp,
    _Out_ PULONG_PTR Information)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG OutputLength = IoStack->Parameters.DeviceIoControl.OutputBufferLength;
    PACPI_ENUM_CHILDREN_INPUT_BUFFER Request = Irp->AssociatedIrp.SystemBuffer;
    PACPI_ENUM_CHILDREN_OUTPUT_BUFFER Output = Irp->AssociatedIrp.SystemBuffer;
    CHAR Name[UACPINT_NODE_NAME_LENGTH];
    uacpi_namespace_node *Child;
    PACPI_ENUM_CHILD Entry;
    ULONG Needed;
    ULONG Count;

    *Information = 0;

    if (!Node)
        return STATUS_INVALID_DEVICE_REQUEST;

    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG) * 2)
        return STATUS_INVALID_PARAMETER;

    if (!Request || Request->Signature != ACPI_ENUM_CHILDREN_INPUT_BUFFER_SIGNATURE)
        return STATUS_INVALID_PARAMETER;

    Count = 0;
    Child = NULL;
    while (uacpi_likely_success(uacpi_namespace_node_next(Node, &Child)) && Child)
        Count++;

    Needed = FIELD_OFFSET(ACPI_ENUM_CHILDREN_OUTPUT_BUFFER, Children) +
             Count * UACPINT_ENUM_CHILD_RECORD_LENGTH;

    if (OutputLength < Needed)
    {
        /* The required size goes in NumberOfChildren when the header fits */
        if (OutputLength < sizeof(*Output))
        {
            DPRINT("%s: enum children buffer of %lu bytes is too small\n", TraceName, OutputLength);
            return STATUS_BUFFER_TOO_SMALL;
        }

        RtlZeroMemory(Output, OutputLength);
        Output->Signature = ACPI_ENUM_CHILDREN_OUTPUT_BUFFER_SIGNATURE;
        Output->NumberOfChildren = Needed;
        *Information = sizeof(*Output);

        DPRINT("%s: %lu children need %lu bytes, got %lu\n", TraceName, Count, Needed, OutputLength);
        return STATUS_BUFFER_OVERFLOW;
    }

    RtlZeroMemory(Output, Needed);
    Output->Signature = ACPI_ENUM_CHILDREN_OUTPUT_BUFFER_SIGNATURE;
    Output->NumberOfChildren = Count;

    Entry = Output->Children;
    Child = NULL;
    while (uacpi_likely_success(uacpi_namespace_node_next(Node, &Child)) && Child)
    {
        UacpiNtCopyNodeName(Child, Name);
        Entry->Flags = UacpiNtNodeHasChildren(Child) ? ACPI_OBJECT_HAS_CHILDREN : 0;
        Entry->NameLength = sizeof(Name);
        RtlCopyMemory(Entry->Name, Name, sizeof(Name));

        /* Same stride as the caller's ACPI_ENUM_CHILD_NEXT */
        Entry = ACPI_ENUM_CHILD_NEXT(Entry);
    }

    *Information = (ULONG_PTR)((PUCHAR)Entry - (PUCHAR)Output);
    DPRINT("%s: %lu children, %lu bytes\n", TraceName, Count, (ULONG)*Information);
    return STATUS_SUCCESS;
}

/* uACPI owns the FACS protocol, so the global lock goes straight to it */
static
NTSTATUS
NTAPI
UacpiNtManipulateGlobalLock(
    _In_ BOOLEAN Acquire,
    _In_z_ const char *TraceName,
    _In_ PIRP Irp)
{
    static uacpi_u32 Sequence;
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    uacpi_status UacpiStatus;

    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG))
        return STATUS_INVALID_PARAMETER;

    if (!Irp->AssociatedIrp.SystemBuffer)
        return STATUS_INVALID_PARAMETER;

    if (Acquire)
        UacpiStatus = uacpi_acquire_global_lock(0xFFFF, &Sequence);
    else
        UacpiStatus = uacpi_release_global_lock(Sequence);

    DPRINT("%s: global lock %s returned %d\n", TraceName, Acquire ? "acquire" : "release", (int)UacpiStatus);
    return uacpi_unlikely_error(UacpiStatus) ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS;
}

/* ACPI IOCTLs shared by PDOs and filters, TRUE when the IRP was completed or pended */
static
BOOLEAN
NTAPI
UacpiNtAcpiDeviceControl(
    _In_opt_ uacpi_namespace_node *Node,
    _In_z_ const char *Name,
    _In_ PDEVICE_OBJECT Self,
    _Inout_ PIRP Irp,
    _Out_ PNTSTATUS Disposition)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG Code = IoStack->Parameters.DeviceIoControl.IoControlCode;
    ULONG_PTR Information = 0;
    NTSTATUS Status;

    switch (Code)
    {
        case IOCTL_ACPI_GET_DEVICE_INFORMATION:
            Status = UacpiNtGetDeviceInformation(Node, Name, Irp, &Information);
            *Disposition = UacpiNtCompleteIrp(Irp, Status, Information);
            return TRUE;

        case IOCTL_ACPI_GET_DEVICE_SPECIFIC_DATA:
            Status = UacpiNtGetDeviceSpecificData(Node, Name, Irp);
            *Disposition = UacpiNtCompleteIrp(Irp, Status, 0);
            return TRUE;

        case UACPINT_IOCTL_QUERY_BIOS_NAME:
            Status = UacpiNtQueryBiosName(Node, Name, Irp, &Information);
            *Disposition = UacpiNtCompleteIrp(Irp, Status, Information);
            return TRUE;

        case UACPINT_IOCTL_REGISTER_OPREGION:
        case UACPINT_IOCTL_UNREGISTER_OPREGION:
        case UACPINT_IOCTL_TRANSLATE_BIOS_RESOURCES:
        case UACPINT_IOCTL_REGISTER_FIRMWARE_LOCK:
        case UACPINT_IOCTL_UNREGISTER_FIRMWARE_LOCK:
            DPRINT1("%s: ACPI IOCTL function %lu is not implemented\n", Name, (Code >> 2) & 0xFFF);
            *Disposition = UacpiNtCompleteIrp(Irp, STATUS_NOT_SUPPORTED, 0);
            return TRUE;

        case IOCTL_ACPI_ENUM_CHILDREN:
            Status = UacpiNtEnumChildren(Node, Name, Irp, &Information);
            *Disposition = UacpiNtCompleteIrp(Irp, Status, Information);
            return TRUE;

        case IOCTL_ACPI_ACQUIRE_GLOBAL_LOCK:
            Status = UacpiNtManipulateGlobalLock(TRUE, Name, Irp);
            *Disposition = UacpiNtCompleteIrp(Irp, Status, 0);
            return TRUE;

        case IOCTL_ACPI_RELEASE_GLOBAL_LOCK:
            Status = UacpiNtManipulateGlobalLock(FALSE, Name, Irp);
            *Disposition = UacpiNtCompleteIrp(Irp, Status, 0);
            return TRUE;

        /* The version 2 codes share the parser, unknown input signatures are rejected */
        case IOCTL_ACPI_EVAL_METHOD:
        case IOCTL_ACPI_EVAL_METHOD_EX:
        case IOCTL_ACPI_ASYNC_EVAL_METHOD:
        case IOCTL_ACPI_ASYNC_EVAL_METHOD_EX:
        case IOCTL_ACPI_EVAL_METHOD_V2:
        case IOCTL_ACPI_EVAL_METHOD_V2_EX:
        case IOCTL_ACPI_ASYNC_EVAL_METHOD_V2:
        case IOCTL_ACPI_ASYNC_EVAL_METHOD_V2_EX:
            if (KeGetCurrentIrql() == PASSIVE_LEVEL)
            {
                Status = UacpiNtEvaluate(Node, Name, Irp);
                *Disposition = UacpiNtCompleteIrp(Irp, Status, Irp->IoStatus.Information);
            }
            else
            {
                *Disposition = UacpiNtQueueEvaluation(Node, Name, Self, Irp);
            }
            return TRUE;

        default:
            return FALSE;
    }
}

/* APIC ID of the MADT entry whose ACPI processor UID matches ProcessorId */
static
BOOLEAN
NTAPI
UacpiNtFindInitialApicId(
    _In_ uacpi_u8 ProcessorId,
    _Out_ PULONG ApicId)
{
    struct acpi_madt_x2apic *X2Apic;
    struct acpi_madt_lapic *LocalApic;
    struct acpi_entry_hdr *Header;
    struct acpi_madt *Madt;
    BOOLEAN Found = FALSE;
    uacpi_table Table;
    PUCHAR Entry;
    PUCHAR End;

    if (uacpi_unlikely_error(uacpi_table_find_by_signature(ACPI_MADT_SIGNATURE, &Table)))
        return FALSE;

    if (!Table.ptr)
        return FALSE;

    Madt = Table.ptr;
    Entry = (PUCHAR)Madt->entries;
    End = (PUCHAR)Madt + Madt->hdr.length;

    while (!Found && Entry + sizeof(*Header) <= End)
    {
        Header = (struct acpi_entry_hdr *)Entry;
        if (Header->length < sizeof(*Header) || Entry + Header->length > End)
            break;

        if (Header->type == ACPI_MADT_ENTRY_TYPE_LAPIC && Header->length >= sizeof(*LocalApic))
        {
            LocalApic = (struct acpi_madt_lapic *)Entry;
            if (LocalApic->uid == ProcessorId)
            {
                *ApicId = LocalApic->id;
                Found = TRUE;
            }
        }
        else if (Header->type == ACPI_MADT_ENTRY_TYPE_LOCAL_X2APIC && Header->length >= sizeof(*X2Apic))
        {
            X2Apic = (struct acpi_madt_x2apic *)Entry;
            if (X2Apic->uid == ProcessorId)
            {
                *ApicId = X2Apic->id;
                Found = TRUE;
            }
        }

        Entry += Header->length;
    }

    uacpi_table_unref(&Table);
    return Found;
}

/* The CPU driver issues this at START to learn which processor it is */
static
BOOLEAN
NTAPI
UacpiNtProcessorObjInfo(
    _In_ PUACPINT_PDO Pdo,
    _Inout_ PIRP Irp,
    _Out_ PNTSTATUS Disposition)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG OutputLength = IoStack->Parameters.DeviceIoControl.OutputBufferLength;
    PUACPINT_PROCESSOR_OBJ_INFO Info = Irp->AssociatedIrp.SystemBuffer;
    uacpi_processor_info ProcessorInfo;
    BOOLEAN HaveInfo = FALSE;
    ULONG ShortLength;
    ULONG ApicId;

    if (IoStack->Parameters.DeviceIoControl.IoControlCode != UACPINT_IOCTL_GET_PROCESSOR_OBJ_INFO ||
        !Pdo->IsProcessor)
    {
        return FALSE;
    }

    /* acpi.sys only answers kernel mode callers */
    if (Irp->RequestorMode != KernelMode)
    {
        *Disposition = UacpiNtCompleteIrp(Irp, STATUS_INVALID_DEVICE_REQUEST, 0);
        return TRUE;
    }

    /* Everything up to the APIC ID is required */
    ShortLength = FIELD_OFFSET(UACPINT_PROCESSOR_OBJ_INFO, ApicId);
    if (OutputLength < ShortLength || !Info)
    {
        *Disposition = UacpiNtCompleteIrp(Irp, STATUS_INFO_LENGTH_MISMATCH, 0);
        return TRUE;
    }

    RtlZeroMemory(Info, (OutputLength >= sizeof(*Info)) ? sizeof(*Info) : ShortLength);

    if (Pdo->Node && UacpiNtGetProcessorInfo(Pdo->Node, &ProcessorInfo))
    {
        Info->ProcessorId = ProcessorInfo.id;
        Info->BlockAddress = ProcessorInfo.block_address;
        Info->BlockLength = ProcessorInfo.block_length;
        HaveInfo = TRUE;
    }

    if (OutputLength < sizeof(*Info))
    {
        *Disposition = UacpiNtCompleteIrp(Irp, STATUS_SUCCESS, ShortLength);
        return TRUE;
    }

    ApicId = 0xFFFFFFFF;
    if (HaveInfo)
        UacpiNtFindInitialApicId((uacpi_u8)Info->ProcessorId, &ApicId);
    Info->ApicId = ApicId;

    *Disposition = UacpiNtCompleteIrp(Irp, STATUS_SUCCESS, sizeof(*Info));
    return TRUE;
}

NTSTATUS
NTAPI
UacpiNtPdoDeviceControl(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp)
{
    NTSTATUS Disposition;
    BOOLEAN Handled;

    if (Pdo->IsProcessor && UacpiNtProcessorObjInfo(Pdo, Irp, &Disposition))
        return Disposition;

    /* Device class IOCTLs go ahead of the evaluation path */
    if (Pdo->ButtonCaps != 0)
    {
        Disposition = UacpiNtButtonDeviceControl(Pdo, Irp, &Handled);
        if (Handled)
            return Disposition;
    }

    if (Pdo->Ec)
    {
        Disposition = UacpiNtEcDeviceControl(Pdo, Irp, &Handled);
        if (Handled)
            return Disposition;
    }

    if (Pdo->IsThermalZone)
    {
        Disposition = UacpiNtThermalDeviceControl(Pdo, Irp, &Handled);
        if (Handled)
            return Disposition;
    }

    if (UacpiNtAcpiDeviceControl(Pdo->Node, Pdo->Name, Pdo->Shared.Self, Irp, &Disposition))
        return Disposition;

    /* Bottom of the stack, nobody else can answer it */
    DPRINT("%s: IOCTL 0x%08lx is not supported\n",
           Pdo->Name,
           IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.IoControlCode);
    return UacpiNtCompleteIrp(Irp, STATUS_NOT_SUPPORTED, 0);
}

/* ACPI IOCTLs are served off the filter's node, the rest go down the stack */
NTSTATUS
NTAPI
UacpiNtFilterDeviceControl(
    _In_ PUACPINT_FLT Filter,
    _In_ PIRP Irp)
{
    CHAR Name[8];
    NTSTATUS Disposition;

    if (Filter->Node)
    {
        UacpiNtCopyNodeName(Filter->Node, Name);
        if (UacpiNtAcpiDeviceControl(Filter->Node, Name, Filter->Shared.Self, Irp, &Disposition))
            return Disposition;
    }

    return UacpiNtForwardAndForget(Filter->LowerDevice, Irp);
}
