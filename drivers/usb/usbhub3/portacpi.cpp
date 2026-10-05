/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ACPI description of hub ports: _ADR, _DSM, _DSD, _UPC and _PLD
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"
#include "hubid.h"

#include <acpiioct.h>

#define NDEBUG
#include <debug.h>

/* Room for any method result this module reads */
#define HUB_ACPI_RESULT_BYTES       (sizeof(ACPI_EVAL_OUTPUT_BUFFER) + 1024)

/* "<device path>.XXXX" plus the NUL must fit the 256 byte method name */
#define HUB_ACPI_METHOD_SUFFIX      6U

/* _UPC connector type of a Type-C connector carrying USB 2 and SuperSpeed without a mux */
#define HUB_UPC_TYPE_C_NO_SWITCH    0x0A

/* _UPC: Connectable, connector type and two reserved values */
#define HUB_UPC_ELEMENTS            4

/* The first 16 bytes of _PLD are all the port keeps */
#define HUB_PLD_BYTES               16

/* USB _DSM {CE2EE385-00E6-48CB-9F05-2EDB927C4899}, revision 0, function 2: interconnect type */
static const GUID HubUsbDsmGuid =
    { 0xCE2EE385, 0x00E6, 0x48CB, { 0x9F, 0x05, 0x2E, 0xDB, 0x92, 0x7C, 0x48, 0x99 } };

#define HUB_DSM_REVISION            0
#define HUB_DSM_INTERCONNECT_TYPE   2

/* Function 5 returns 1 when U2 must stay off for the port */
#define HUB_DSM_DISABLE_U2          5

/* _DSD device properties {DAFFD814-6EBA-4D8C-8A91-BC9BBF4AA301} */
static const GUID HubDsdPropertiesGuid =
    { 0xDAFFD814, 0x6EBA, 0x4D8C, { 0x8A, 0x91, 0xBC, 0x9B, 0xBF, 0x4A, 0xA3, 0x01 } };

/* Port property naming the USB4 host router the port is tunneled through */
static const CHAR HubUsb4HostProperty[] = "usb4-host-interface";

/* A property entry is a package of the name and the value */
#define HUB_DSD_ENTRY_MIN_BYTES     16

/* Header, a 16 byte buffer, two integers and an empty package */
#define HUB_DSM_INPUT_BYTES         (FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX, Argument) + \
                                     ACPI_METHOD_ARGUMENT_LENGTH(sizeof(GUID)) +                \
                                     2 * ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG)) +           \
                                     ACPI_METHOD_ARGUMENT_LENGTH(0))

C_ASSERT(sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX) == 276);
C_ASSERT(HUB_DSM_INPUT_BYTES == 312);
C_ASSERT(sizeof(ACPI_ENUM_CHILDREN_INPUT_BUFFER) == 16);
C_ASSERT(sizeof(ACPI_ENUM_CHILDREN_OUTPUT_BUFFER) == 20);

/** Zeroed nonpaged WDF memory owned by the hub FDO. */
static
NTSTATUS
NTAPI
HubAcpiAllocate(
    _In_ HubFdo* Hub,
    _In_ size_t Bytes,
    _Out_ WDFMEMORY* Memory,
    _Outptr_result_bytebuffer_(Bytes) PVOID* Buffer)
{
    WDF_OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Hub->m_Device;

    Status = WdfMemoryCreate(&Attributes, NonPagedPool, HUB_TAG_HUB, Bytes, Memory, Buffer);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p ACPI buffer of %lu bytes failed 0x%lx\n", Hub, (ULONG)Bytes, Status);
        *Memory = NULL;
        *Buffer = NULL;
        return Status;
    }

    RtlZeroMemory(*Buffer, Bytes);
    return STATUS_SUCCESS;
}

static
VOID
NTAPI
HubAcpiFree(
    _In_opt_ WDFMEMORY Memory)
{
    if (Memory != NULL)
        WdfObjectDelete(Memory);
}

/* Synchronous internal IOCTL to the ACPI filter on the hub's PDO, if there is one */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubAcpiSend(
    _In_ HubFdo* Hub,
    _In_ ULONG IoControlCode,
    _In_ WDFMEMORY Input,
    _In_ WDFMEMORY Output,
    _Out_ PULONG_PTR Information)
{
    WDF_MEMORY_DESCRIPTOR InputDescriptor;
    WDF_MEMORY_DESCRIPTOR OutputDescriptor;

    WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&InputDescriptor, Input, NULL);
    WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&OutputDescriptor, Output, NULL);

    *Information = 0;
    return WdfIoTargetSendInternalIoctlSynchronously(WdfDeviceGetIoTarget(Hub->m_Device),
                                                     NULL,
                                                     IoControlCode,
                                                     &InputDescriptor,
                                                     &OutputDescriptor,
                                                     NULL,
                                                     Information);
}

/* METHOD EVALUATION *********************************************************/

/* STATUS_NO_SUCH_DEVICE may still come with a result; only _DSM looks at it */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubAcpiEvaluate(
    _In_ HubFdo* Hub,
    _In_ WDFMEMORY Input,
    _In_ WDFMEMORY Output,
    _In_ BOOLEAN CheckResultOnNoDevice)
{
    PACPI_EVAL_OUTPUT_BUFFER Result = (PACPI_EVAL_OUTPUT_BUFFER)WdfMemoryGetBuffer(Output, NULL);
    PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX Method = (PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX)WdfMemoryGetBuffer(Input, NULL);
    ULONG_PTR Information;
    NTSTATUS Status;

    Status = HubAcpiSend(Hub, IOCTL_ACPI_EVAL_METHOD_EX, Input, Output, &Information);
    if (!NT_SUCCESS(Status) && !(CheckResultOnNoDevice && Status == STATUS_NO_SUCH_DEVICE))
    {
        if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
            DPRINT1("Hub %p ACPI %s failed 0x%lx\n", Hub, Method->MethodName, Status);
        return Status;
    }

    if (Result->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE || Result->Count == 0)
    {
        DPRINT1("Hub %p ACPI %s returned no usable result\n", Hub, Method->MethodName);
        return STATUS_ACPI_INVALID_DATA;
    }

    return Status;
}

/** Fills the input header: "<Device>.<Method>" and the argument count. */
static
NTSTATUS
NTAPI
HubAcpiPrepareInput(
    _Out_ PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX Input,
    _In_ PCANSI_STRING Device,
    _In_reads_(4) PCSTR Method,
    _In_ ULONG ArgumentCount)
{
    if (Device->Length + HUB_ACPI_METHOD_SUFFIX > sizeof(Input->MethodName))
    {
        DPRINT1("ACPI device path %Z is too long for a method name\n", Device);
        return STATUS_INVALID_PARAMETER;
    }

    Input->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE_EX;
    Input->Size = sizeof(*Input);
    Input->ArgumentCount = ArgumentCount;

    RtlCopyMemory(Input->MethodName, Device->Buffer, Device->Length);
    Input->MethodName[Device->Length] = '.';
    RtlCopyMemory(&Input->MethodName[Device->Length + 1], Method, 4);
    Input->MethodName[Device->Length + 5] = ANSI_NULL;
    return STATUS_SUCCESS;
}

/** Evaluates a method without arguments into Output. */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubAcpiEvaluateMethod(
    _In_ HubFdo* Hub,
    _In_ PCANSI_STRING Device,
    _In_reads_(4) PCSTR Method,
    _In_ WDFMEMORY Output)
{
    PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX Input;
    WDFMEMORY InputMemory;
    NTSTATUS Status;

    Status = HubAcpiAllocate(Hub, sizeof(*Input), &InputMemory, (PVOID*)&Input);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = HubAcpiPrepareInput(Input, Device, Method, 0);
    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(WdfMemoryGetBuffer(Output, NULL), HUB_ACPI_RESULT_BYTES);
        Status = HubAcpiEvaluate(Hub, InputMemory, Output, FALSE);
    }

    HubAcpiFree(InputMemory);
    return Status;
}

/* The Size field is the structure size, not the 312 bytes sent */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubAcpiEvaluateDsm(
    _In_ HubFdo* Hub,
    _In_ PCANSI_STRING Device,
    _In_ ULONG Function,
    _In_ WDFMEMORY Output)
{
    PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX Input;
    PACPI_METHOD_ARGUMENT Argument;
    WDFMEMORY InputMemory;
    NTSTATUS Status;

    Status = HubAcpiAllocate(Hub, HUB_DSM_INPUT_BYTES, &InputMemory, (PVOID*)&Input);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = HubAcpiPrepareInput(Input, Device, "_DSM", 4);
    if (!NT_SUCCESS(Status))
    {
        HubAcpiFree(InputMemory);
        return Status;
    }

    Argument = Input->Argument;
    Argument->Type = ACPI_METHOD_ARGUMENT_BUFFER;
    Argument->DataLength = sizeof(HubUsbDsmGuid);
    RtlCopyMemory(Argument->Data, &HubUsbDsmGuid, sizeof(HubUsbDsmGuid));

    Argument = ACPI_METHOD_NEXT_ARGUMENT(Argument);
    Argument->Type = ACPI_METHOD_ARGUMENT_INTEGER;
    Argument->DataLength = sizeof(ULONG);
    Argument->Argument = HUB_DSM_REVISION;

    Argument = ACPI_METHOD_NEXT_ARGUMENT(Argument);
    Argument->Type = ACPI_METHOD_ARGUMENT_INTEGER;
    Argument->DataLength = sizeof(ULONG);
    Argument->Argument = Function;

    Argument = ACPI_METHOD_NEXT_ARGUMENT(Argument);
    Argument->Type = ACPI_METHOD_ARGUMENT_PACKAGE;
    Argument->DataLength = 0;

    RtlZeroMemory(WdfMemoryGetBuffer(Output, NULL), HUB_ACPI_RESULT_BYTES);
    Status = HubAcpiEvaluate(Hub, InputMemory, Output, TRUE);

    HubAcpiFree(InputMemory);
    return Status;
}

/* CHILD ENUMERATION *********************************************************/

/* A 20 byte output only queries the size, returned in NumberOfChildren with STATUS_BUFFER_OVERFLOW */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubAcpiEnumChildren(
    _In_ HubFdo* Hub,
    _In_ WDFMEMORY Output,
    _In_ size_t OutputBytes)
{
    PACPI_ENUM_CHILDREN_OUTPUT_BUFFER Children = (PACPI_ENUM_CHILDREN_OUTPUT_BUFFER)WdfMemoryGetBuffer(Output, NULL);
    PACPI_ENUM_CHILDREN_INPUT_BUFFER Input;
    WDFMEMORY InputMemory;
    ULONG_PTR Information;
    NTSTATUS Status;

    Status = HubAcpiAllocate(Hub, sizeof(*Input), &InputMemory, (PVOID*)&Input);
    if (!NT_SUCCESS(Status))
        return Status;

    Input->Signature = ACPI_ENUM_CHILDREN_INPUT_BUFFER_SIGNATURE;
    Input->Flags = ENUM_CHILDREN_IMMEDIATE_ONLY;
    Input->NameLength = 0;

    Status = HubAcpiSend(Hub, IOCTL_ACPI_ENUM_CHILDREN, InputMemory, Output, &Information);
    HubAcpiFree(InputMemory);

    if (Status == STATUS_NOT_SUPPORTED)
        return Status;

    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
    {
        DPRINT1("Hub %p ACPI child enumeration failed 0x%lx\n", Hub, Status);
        return Status;
    }

    if (OutputBytes == sizeof(ACPI_ENUM_CHILDREN_OUTPUT_BUFFER))
    {
        if (Status != STATUS_BUFFER_OVERFLOW ||
            Children->Signature != ACPI_ENUM_CHILDREN_OUTPUT_BUFFER_SIGNATURE ||
            Children->NumberOfChildren < sizeof(ACPI_ENUM_CHILDREN_OUTPUT_BUFFER))
        {
            DPRINT1("Hub %p ACPI child enumeration size query is invalid 0x%lx\n", Hub, Status);
            return STATUS_ACPI_INVALID_DATA;
        }

        return STATUS_BUFFER_OVERFLOW;
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p ACPI child enumeration overflowed %lu bytes\n", Hub, (ULONG)OutputBytes);
        return Status;
    }

    if (Children->Signature != ACPI_ENUM_CHILDREN_OUTPUT_BUFFER_SIGNATURE ||
        Children->NumberOfChildren == 0 ||
        Information != OutputBytes)
    {
        DPRINT1("Hub %p ACPI child enumeration returned no children\n", Hub);
        return STATUS_ACPI_INVALID_DATA;
    }

    return STATUS_SUCCESS;
}

/* ONE PORT ******************************************************************/

/** First result argument when it is an integer. */
static
BOOLEAN
NTAPI
HubAcpiResultInteger(
    _In_ WDFMEMORY Output,
    _Out_ PULONG Value)
{
    PACPI_EVAL_OUTPUT_BUFFER Result = (PACPI_EVAL_OUTPUT_BUFFER)WdfMemoryGetBuffer(Output, NULL);

    *Value = 0;
    if (Result->Count == 0 || Result->Argument[0].Type != ACPI_METHOD_ARGUMENT_INTEGER)
        return FALSE;

    *Value = Result->Argument[0].Argument;
    return TRUE;
}

/* The interconnect type is only kept for diagnostics */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubAcpiReadInterconnect(
    _In_ HubFdo* Hub,
    _In_ HubPort* Port,
    _In_ PCANSI_STRING Device,
    _In_ WDFMEMORY Output)
{
    ULONG Value;

    if (!NT_SUCCESS(HubAcpiEvaluateDsm(Hub, Device, HUB_DSM_INTERCONNECT_TYPE, Output)))
        return;

    if (!HubAcpiResultInteger(Output, &Value))
    {
        DPRINT1("Port %u: _DSM interconnect type is not an integer (platform compliance)\n", Port->Number());
        return;
    }

    Port->m_InterconnectType = (USHORT)Value;
    if (Port->m_InterconnectType > 2)
        DPRINT1("Port %u: unknown interconnect type %u\n", Port->Number(), Port->m_InterconnectType);
    else if (Port->m_InterconnectType != 0)
        DPRINT("Port %u is %s\n", Port->Number(), (Port->m_InterconnectType == 1) ? "HSIC" : "SSIC");
}

/* Only a result of exactly 1 sets the flag; nothing clears it */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubAcpiReadNoU2(
    _In_ HubFdo* Hub,
    _In_ HubPort* Port,
    _In_ PCANSI_STRING Device,
    _In_ WDFMEMORY Output)
{
    ULONG Value;

    if (!NT_SUCCESS(HubAcpiEvaluateDsm(Hub, Device, HUB_DSM_DISABLE_U2, Output)))
        return;

    if (!HubAcpiResultInteger(Output, &Value))
    {
        DPRINT1("Port %u: _DSM U2 setting is not an integer (platform compliance)\n", Port->Number());
        return;
    }

    if (Value == 0)
        return;

    if (Value != 1)
    {
        DPRINT1("Port %u: unknown _DSM U2 setting %lu\n", Port->Number(), Value);
        return;
    }

    DPRINT("Port %u: platform keeps U2 off\n", Port->Number());
    Port->SetFlag(PortFlag::AcpiNoU2);
}

/** TRUE when the argument header and its data end at or before End. */
static
BOOLEAN
NTAPI
HubAcpiArgumentFits(
    _In_ PACPI_METHOD_ARGUMENT Argument,
    _In_ PUCHAR End)
{
    PUCHAR Data = (PUCHAR)Argument + FIELD_OFFSET(ACPI_METHOD_ARGUMENT, Data);

    return Data <= End && Argument->DataLength <= (ULONG)(End - Data);
}

/**
 * @brief
 * Finds the value of the usb4-host-interface property in a _DSD result.
 */
static
NTSTATUS
NTAPI
HubAcpiFindUsb4Host(
    _In_ WDFMEMORY Output,
    _Outptr_result_maybenull_ PACPI_METHOD_ARGUMENT* Value)
{
    PACPI_EVAL_OUTPUT_BUFFER Result = (PACPI_EVAL_OUTPUT_BUFFER)WdfMemoryGetBuffer(Output, NULL);
    PUCHAR End = (PUCHAR)Result + min(Result->Length, (ULONG)HUB_ACPI_RESULT_BYTES);
    PACPI_METHOD_ARGUMENT Argument = Result->Argument;
    PACPI_METHOD_ARGUMENT Package = NULL;
    PACPI_METHOD_ARGUMENT Entry;
    PACPI_METHOD_ARGUMENT Name;
    PACPI_METHOD_ARGUMENT Next;
    PUCHAR PackageEnd;
    PUCHAR EntryEnd;
    ULONG Index;

    *Value = NULL;

    for (Index = 0;
         Index < Result->Count && HubAcpiArgumentFits(Argument, End);
         Index++, Argument = ACPI_METHOD_NEXT_ARGUMENT(Argument))
    {
        if ((Index & 1) != 0 ||
            Argument->Type != ACPI_METHOD_ARGUMENT_BUFFER ||
            Argument->DataLength != sizeof(GUID) ||
            !RtlEqualMemory(Argument->Data, &HubDsdPropertiesGuid, sizeof(GUID)))
        {
            continue;
        }

        Next = ACPI_METHOD_NEXT_ARGUMENT(Argument);
        if (HubAcpiArgumentFits(Next, End) && Next->Type == ACPI_METHOD_ARGUMENT_PACKAGE)
        {
            Package = Next;
            break;
        }
    }

    if (Package == NULL)
        return STATUS_UNSUCCESSFUL;

    PackageEnd = Package->Data + Package->DataLength;

    for (Entry = (PACPI_METHOD_ARGUMENT)Package->Data;
         HubAcpiArgumentFits(Entry, PackageEnd);
         Entry = ACPI_METHOD_NEXT_ARGUMENT(Entry))
    {
        if (Entry->Type != ACPI_METHOD_ARGUMENT_PACKAGE || Entry->DataLength < HUB_DSD_ENTRY_MIN_BYTES)
            continue;

        Name = (PACPI_METHOD_ARGUMENT)Entry->Data;
        EntryEnd = Entry->Data + Entry->DataLength;

        if (!HubAcpiArgumentFits(Name, EntryEnd) ||
            Name->Type != ACPI_METHOD_ARGUMENT_STRING ||
            Name->DataLength != sizeof(HubUsb4HostProperty) ||
            strncmp((PCSTR)Name->Data, HubUsb4HostProperty, sizeof(HubUsb4HostProperty) - 1) != 0)
        {
            continue;
        }

        /* A device reference in the ASL arrives as the path string of that device */
        Next = ACPI_METHOD_NEXT_ARGUMENT(Name);
        if (!HubAcpiArgumentFits(Next, EntryEnd))
            return STATUS_ACPI_FATAL;

        if (Next->Type != ACPI_METHOD_ARGUMENT_STRING)
            return STATUS_ACPI_INVALID_OBJTYPE;

        *Value = Next;
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

/* Widens the ASCII path into a string owned by the hub */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubAcpiCreateHostName(
    _In_ HubFdo* Hub,
    _In_ PACPI_METHOD_ARGUMENT Value,
    _Out_ WDFSTRING* Name)
{
    WDF_OBJECT_ATTRIBUTES Attributes;
    UNICODE_STRING Text;
    PWCHAR Wide;
    ULONG Chars = 0;
    ULONG Index;
    NTSTATUS Status;

    while (Chars < Value->DataLength && Value->Data[Chars] != ANSI_NULL)
        Chars++;

    if (Chars == 0 || Chars > MAXUSHORT / sizeof(WCHAR))
        return STATUS_ACPI_INVALID_DATA;

    Wide = (PWCHAR)ExAllocatePoolWithTag(PagedPool, Chars * sizeof(WCHAR), HUB_TAG_PORT);
    if (Wide == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    for (Index = 0; Index < Chars; Index++)
        Wide[Index] = (WCHAR)Value->Data[Index];

    Text.Buffer = Wide;
    Text.Length = (USHORT)(Chars * sizeof(WCHAR));
    Text.MaximumLength = Text.Length;

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Hub->m_Device;

    Status = WdfStringCreate(&Text, &Attributes, Name);

    ExFreePoolWithTag(Wide, HUB_TAG_PORT);
    return Status;
}

/**
 * @brief
 * Without a USB4 host name the port stays unbound; only a failed binding is an error.
 */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubAcpiBindUsb4Host(
    _In_ HubFdo* Hub,
    _In_ HubPort* Port,
    _In_ PCANSI_STRING Device,
    _In_ WDFMEMORY Output)
{
    PACPI_METHOD_ARGUMENT Value;
    WDFSTRING Name = Hub->m_Usb4HostName;
    NTSTATUS Status;

    if (Name == NULL)
    {
        if (!NT_SUCCESS(HubAcpiEvaluateMethod(Hub, Device, "_DSD", Output)))
            return STATUS_SUCCESS;

        Status = HubAcpiFindUsb4Host(Output, &Value);
        if (NT_SUCCESS(Status))
            Status = HubAcpiCreateHostName(Hub, Value, &Name);

        if (!NT_SUCCESS(Status))
        {
            if (Status != STATUS_UNSUCCESSFUL)
                DPRINT1("Port %u: _DSD USB4 host is unusable 0x%lx\n", Port->Number(), Status);
            return STATUS_SUCCESS;
        }
    }

    Status = HubUsb4BindPort(Hub, Port, Name);
    if (!NT_SUCCESS(Status))
        DPRINT1("Port %u not bound to its USB4 host 0x%lx\n", Port->Number(), Status);

    return Status;
}

/* Elements stored before an error, or past a short package, are left as they are */
static
NTSTATUS
NTAPI
HubAcpiStoreUpc(
    _In_ HubPort* Port,
    _In_ WDFMEMORY Output)
{
    PACPI_EVAL_OUTPUT_BUFFER Result = (PACPI_EVAL_OUTPUT_BUFFER)WdfMemoryGetBuffer(Output, NULL);
    PUCHAR End = (PUCHAR)Result + min(Result->Length, (ULONG)HUB_ACPI_RESULT_BYTES);
    PACPI_METHOD_ARGUMENT Element = Result->Argument;
    ULONG Index;

    if (Result->Count == 0)
        return STATUS_ACPI_INCORRECT_ARGUMENT_COUNT;

    for (Index = 0;
         Index < Result->Count &&
         (PUCHAR)Element + FIELD_OFFSET(ACPI_METHOD_ARGUMENT, Data) <= End &&
         (PUCHAR)ACPI_METHOD_NEXT_ARGUMENT(Element) <= End;
         Index++, Element = ACPI_METHOD_NEXT_ARGUMENT(Element))
    {
        if (Index >= HUB_UPC_ELEMENTS)
            return STATUS_ACPI_INCORRECT_ARGUMENT_COUNT;

        if (Element->Type != ACPI_METHOD_ARGUMENT_INTEGER)
            return STATUS_ACPI_INVALID_ARGTYPE;

        switch (Index)
        {
            case 0:
                Port->m_AcpiUpc.Connectable = (UCHAR)Element->Argument;
                break;

            case 1:
                Port->m_AcpiUpc.ConnectorType = (UCHAR)Element->Argument;
                break;

            case 2:
                Port->m_AcpiUpc.Reserved0 = Element->Argument;
                break;

            default:
                Port->m_AcpiUpc.Reserved1 = Element->Argument;
                break;
        }
    }

    return STATUS_SUCCESS;
}

/* "Type-C without switch" is never cleared once a _UPC reported it */
_IRQL_requires_(PASSIVE_LEVEL)
static
BOOLEAN
NTAPI
HubAcpiReadUpc(
    _In_ HubFdo* Hub,
    _In_ HubPort* Port,
    _In_ PCANSI_STRING Device,
    _In_ WDFMEMORY Output)
{
    NTSTATUS Status;

    Status = HubAcpiEvaluateMethod(Hub, Device, "_UPC", Output);
    if (NT_SUCCESS(Status))
        Status = HubAcpiStoreUpc(Port, Output);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Port %u: _UPC unusable: 0x%lx\n", Port->Number(), Status);
        return FALSE;
    }

    HubSetPortProperty(Port, PortProperty::AcpiUpcValid);

    if (Port->m_AcpiUpc.Connectable == 0)
        HubClearPortProperty(Port, PortProperty::Removable);
    else
        HubSetPortProperty(Port, PortProperty::Removable);

    if (Port->m_AcpiUpc.ConnectorType == HUB_UPC_TYPE_C_NO_SWITCH)
        HubSetPortProperty(Port, PortProperty::TypeCWithoutSwitch);

    return TRUE;
}

_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubAcpiReadPld(
    _In_ HubFdo* Hub,
    _In_ HubPort* Port,
    _In_ PCANSI_STRING Device,
    _In_ WDFMEMORY Output)
{
    PACPI_EVAL_OUTPUT_BUFFER Result = (PACPI_EVAL_OUTPUT_BUFFER)WdfMemoryGetBuffer(Output, NULL);
    NTSTATUS Status;

    Status = HubAcpiEvaluateMethod(Hub, Device, "_PLD", Output);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Port %u: _PLD failed: 0x%lx\n", Port->Number(), Status);
        return;
    }

    if (Result->Count == 0 ||
        Result->Argument[0].Type != ACPI_METHOD_ARGUMENT_BUFFER ||
        Result->Argument[0].DataLength < HUB_PLD_BYTES)
    {
        DPRINT1("Port %u: _PLD buffer unusable\n", Port->Number());
        return;
    }

    RtlCopyMemory(&Port->m_AcpiPld, Result->Argument[0].Data, HUB_PLD_BYTES);
    HubSetPortProperty(Port, PortProperty::AcpiPldValid);

    if (!Port->m_AcpiPld.UserVisible)
        HubClearPortProperty(Port, PortProperty::Removable);

    Hub->SetFlag(HubFlag::InAcpiNamespace);
}

/* The child's _ADR is the port number; fails only when binding fails */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubAcpiReadPort(
    _In_ HubFdo* Hub,
    _In_ PCANSI_STRING Device,
    _In_ WDFMEMORY Output)
{
    HubPort* Port;
    ULONG Address;
    USHORT PortNumber;
    NTSTATUS Status;

    if (!NT_SUCCESS(HubAcpiEvaluateMethod(Hub, Device, "_ADR", Output)) ||
        !HubAcpiResultInteger(Output, &Address))
    {
        return STATUS_SUCCESS;
    }

    PortNumber = (USHORT)Address;
    if (PortNumber == 0 || PortNumber > Hub->m_PortCount)
    {
        DPRINT1("Hub %p: ACPI port address %u out of range (platform compliance)\n", Hub, PortNumber);
        return STATUS_SUCCESS;
    }

    Port = Hub->FindPort(PortNumber);
    if (Port == NULL)
        return STATUS_SUCCESS;

    HubAcpiReadInterconnect(Hub, Port, Device, Output);

    if (Port->IsUsb30())
    {
        Status = HubAcpiBindUsb4Host(Hub, Port, Device, Output);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    HubAcpiReadNoU2(Hub, Port, Device, Output);

    if (!HubAcpiReadUpc(Hub, Port, Device, Output))
        return STATUS_SUCCESS;

    HubAcpiReadPld(Hub, Port, Device, Output);
    return STATUS_SUCCESS;
}

/*
 * A hub without ACPI nodes simply keeps its descriptor values. Only a port
 * that cannot be bound to its USB4 host router ends the walk with an error.
 */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubAcpiReadPortAttributes(
    _In_ HubFdo* Hub)
{
    PACPI_ENUM_CHILDREN_OUTPUT_BUFFER Children;
    PACPI_ENUM_CHILD Child;
    WDFMEMORY SizeMemory = NULL;
    WDFMEMORY ChildMemory = NULL;
    WDFMEMORY ResultMemory = NULL;
    PVOID Buffer;
    ANSI_STRING Device;
    ULONG ChildBytes;
    ULONG Index;
    NTSTATUS Status;
    NTSTATUS Result = STATUS_SUCCESS;

    PAGED_CODE();

    Status = HubAcpiAllocate(Hub, sizeof(*Children), &SizeMemory, (PVOID*)&Children);
    if (!NT_SUCCESS(Status))
        goto Done;

    Status = HubAcpiEnumChildren(Hub, SizeMemory, sizeof(*Children));
    if (Status != STATUS_BUFFER_OVERFLOW)
    {
        if (Status == STATUS_NOT_SUPPORTED)
            DPRINT("Hub %p has no ACPI namespace node\n", Hub);
        goto Done;
    }

    DPRINT("Hub %p has an ACPI namespace node\n", Hub);

    ChildBytes = Children->NumberOfChildren;
    Status = HubAcpiAllocate(Hub, ChildBytes, &ChildMemory, (PVOID*)&Children);
    if (!NT_SUCCESS(Status))
        goto Done;

    Status = HubAcpiEnumChildren(Hub, ChildMemory, ChildBytes);
    if (!NT_SUCCESS(Status))
        goto Done;

    Status = HubAcpiAllocate(Hub, HUB_ACPI_RESULT_BYTES, &ResultMemory, &Buffer);
    if (!NT_SUCCESS(Status))
        goto Done;

    /* The first child is the hub itself */
    Child = Children->Children;
    for (Index = 1; Index < Children->NumberOfChildren; Index++)
    {
        Child = ACPI_ENUM_CHILD_NEXT(Child);

        if ((PUCHAR)Child + FIELD_OFFSET(ACPI_ENUM_CHILD, Name) > (PUCHAR)Children + ChildBytes ||
            (PUCHAR)ACPI_ENUM_CHILD_NEXT(Child) > (PUCHAR)Children + ChildBytes)
        {
            break;
        }

        if (!(Child->Flags & ACPI_OBJECT_HAS_CHILDREN) || Child->NameLength == 0)
            continue;

        Device.Buffer = Child->Name;
        Device.Length = (USHORT)(Child->NameLength - 1);
        Device.MaximumLength = (USHORT)Child->NameLength;

        Result = HubAcpiReadPort(Hub, &Device, ResultMemory);
        if (!NT_SUCCESS(Result))
            break;
    }

Done:
    HubAcpiFree(ResultMemory);
    HubAcpiFree(ChildMemory);
    HubAcpiFree(SizeMemory);
    return Result;
}
