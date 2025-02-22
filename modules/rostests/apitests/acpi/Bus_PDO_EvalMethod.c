/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Unit Tests for acpi!Bus_PDO_EvalMethod (IOCTL_ACPI_EVAL_METHOD handler)
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include <apitest.h>

#define WIN32_NO_STATUS
#include <ndk/rtlfuncs.h>

#define UNIT_TEST
#include <acpi.h>

#include <acpiioct.h>
#include <ntintsafe.h>
#include <initguid.h>

/* TEST DEFINITIONS ***********************************************************/

#define ok_eq_str_ex(entry, value, expected) \
    ok(!strcmp(value, expected), \
       "Line %lu: " #value " = \"%s\", expected \"%s\"\n", (entry)->Line, value, expected)

#define ok_eq_print_ex(entry, value, expected, spec) \
    ok((value) == (expected), \
       "Line %lu: " #value " = " spec ", expected " spec "\n", (entry)->Line, value, expected)

#define ok_not_print_ex(entry, value, expected, spec) \
    ok((value) != (expected), \
       "Line %lu: " #value " = " spec ", expected " spec "\n", (entry)->Line, value, expected)

#define ok_eq_hex_ex(entry, value, expected)       ok_eq_print_ex(entry, value, expected, "0x%08lx")
#define ok_eq_pointer_ex(entry, value, expected)   ok_eq_print_ex(entry, value, expected, "%p")
#define ok_eq_int_ex(entry, value, expected)       ok_eq_print_ex(entry, value, expected, "%d")
#define ok_eq_uint_ex(entry, value, expected)      ok_eq_print_ex(entry, value, expected, "%u")
#define ok_eq_ulong_ex(entry, value, expected)     ok_eq_print_ex(entry, value, expected, "%lu")
#define ok_eq_ulonglong_ex(entry, value, expected) ok_eq_print_ex(entry, value, expected, "%I64u")

#define ok_not_pointer_ex(entry, value, expected)  ok_not_print_ex(entry, value, expected, "%p")

typedef struct _EVAL_TEST_ENTRY
{
    ULONG Line;

    ULONG Flags;
#define STM_TEST_FLAG_INVALID_ARG_3_1              (1 << 0 )
#define STM_TEST_FLAG_INVALID_ARG_3_2              (1 << 1 )
#define STM_TEST_FLAG_INVALID_ARG_3_4              (1 << 2 )
#define STM_TEST_FLAG_INVALID_ARG_3_5              (1 << 3 )
#define STM_TEST_FLAG_INVALID_SIZE_1               (1 << 4 )
#define STM_TEST_FLAG_LARGE_ARG_BUFFER             (1 << 5 )
#define STM_TEST_FLAG_CHANGE_ARG_COUNT             (1 << 6 )
#define STM_TEST_FLAG_SUB_IN_BUFFER                (1 << 7 )
#define STM_TEST_FLAG_SUB_IRP_BUFFER               (1 << 8 )
#define STM_TEST_FLAG_SET_IN_BUFFER                (1 << 9 )
#define STM_TEST_FLAG_SET_IRP_BUFFER               (1 << 10)
#define STM_TEST_FLAG_BAD_ARG_TYPE                 (1 << 11)

#define GTM_TEST_FLAG_BAD_SIGNARUTE                (1 << 0)
#define GTM_TEST_FLAG_BUFFER_HAS_SIGNARUTE         (1 << 1)
#define GTM_TEST_FLAG_BUFFER_HAS_COUNT             (1 << 2)
#define GTM_TEST_FLAG_BUFFER_HAS_LENGTH            (1 << 3)
#define GTM_TEST_FLAG_ARG_HAS_BUFFER_TYPE          (1 << 4)
#define GTM_TEST_FLAG_ARG_HAS_DATA_LENGTH          (1 << 5)
#define GTM_TEST_FLAG_INC_OUT_BUFFER               (1 << 6)
#define GTM_TEST_FLAG_DEC_OUT_BUFFER               (1 << 7)
#define GTM_TEST_FLAG_SET_OUT_BUFFER               (1 << 8)

#define GTM_TEST_FLAG_METHOD_SUCCESS \
    (GTM_TEST_FLAG_BUFFER_HAS_SIGNARUTE | \
     GTM_TEST_FLAG_BUFFER_HAS_COUNT | GTM_TEST_FLAG_BUFFER_HAS_LENGTH | \
     GTM_TEST_FLAG_ARG_HAS_BUFFER_TYPE | GTM_TEST_FLAG_ARG_HAS_DATA_LENGTH)

#define DSM_TEST_FLAG_EMPTY_PACKAGE                (1 << 0)
#define DSM_TEST_FLAG_LARGE_SUB_PACKAGE_BUFFER     (1 << 1)

    NTSTATUS Status;
    ULONG Value;
} EVAL_TEST_ENTRY;

/* KERNEL DEFINITIONS (MOCK) **************************************************/

#define PAGED_CODE()
#define CODE_SEG(...)
#define DPRINT(...)  do { if (0) DbgPrint(__VA_ARGS__); } while (0)
#define DPRINT1(...) do { if (0) DbgPrint(__VA_ARGS__); } while (0)

typedef struct _IO_STACK_LOCATION
{
    union
    {
        struct
        {
            ULONG OutputBufferLength;
            ULONG InputBufferLength;
        } DeviceIoControl;
    } Parameters;
} IO_STACK_LOCATION, *PIO_STACK_LOCATION;

typedef struct _IRP
{
    union
    {
        PVOID SystemBuffer;
    } AssociatedIrp;
    IO_STATUS_BLOCK IoStatus;

    PVOID OutputBuffer;
    IO_STACK_LOCATION MyStack;
} IRP, *PIRP;

static LONG DrvpBlocksAllocated = 0;

#define ExAllocatePoolUninitialized ExAllocatePoolWithTag
#define NonPagedPool 1
static
PVOID
ExAllocatePoolWithTag(ULONG PoolType, SIZE_T NumberOfBytes, ULONG Tag)
{
    PULONG_PTR Mem;

    Mem = HeapAlloc(GetProcessHeap(), 0, NumberOfBytes + 2 * sizeof(PVOID));
    Mem[0] = NumberOfBytes;
    Mem[1] = Tag;

    ++DrvpBlocksAllocated;

    return (PVOID)(Mem + 2);
}

static
VOID
ExFreePoolWithTag(PVOID MemPtr, ULONG Tag)
{
    PULONG_PTR Mem = MemPtr;

    Mem -= 2;
    ok(Mem[1] == Tag, "Tag is %lx, expected %lx\n", Tag, Mem[1]);
    HeapFree(GetProcessHeap(), 0, Mem);

    --DrvpBlocksAllocated;
}

static
PIO_STACK_LOCATION
IoGetCurrentIrpStackLocation(PIRP Irp)
{
    return &Irp->MyStack;
}

/* ACPI DEFINITIONS ***********************************************************/

#include <pshpack1.h>
typedef struct _IDE_ACPI_TIMING_MODE_BLOCK
{
    struct
    {
        ULONG PioSpeed;
        ULONG DmaSpeed;
    } Drive[2];

    ULONG ModeFlags;
} IDE_ACPI_TIMING_MODE_BLOCK, *PIDE_ACPI_TIMING_MODE_BLOCK;
#include <poppack.h>

#define STM_ID_BLOCK_SIZE    512

/* ACPI DEFINITIONS (MOCK) ****************************************************/

#define FAKE_SB_NAMESPACE_ACPI_HANDLE    0xFF0F0001
#define FAKE_INTB_ACPI_HANDLE            0xFF0F0002
#define FAKE_INTC_ACPI_HANDLE            0xFF0F0003

typedef struct _PDO_DEVICE_DATA
{
    HANDLE AcpiHandle;
} PDO_DEVICE_DATA, *PPDO_DEVICE_DATA;

typedef struct _GTM_OBJECT_BUFFER
{
    ACPI_OBJECT Obj;
    IDE_ACPI_TIMING_MODE_BLOCK TimingMode;
} GTM_OBJECT_BUFFER, *PGTM_OBJECT_BUFFER;

typedef struct _BIF_OBJECT_BUFFER
{
    ACPI_OBJECT Obj;
    ACPI_OBJECT BatteryInformation[13];
    CHAR Buffer1[1 + 1];
    CHAR Buffer2[1 + 1];
    CHAR Buffer3[4 + 1];
    CHAR Buffer4[7 + 1];
} BIF_OBJECT_BUFFER, *PBIF_OBJECT_BUFFER;

typedef struct _PCL_OBJECT_BUFFER
{
    ACPI_OBJECT Obj;
} PCL_OBJECT_BUFFER, *PPCL_OBJECT_BUFFER;

typedef struct _PRT_OBJECT_BUFFER
{
    ACPI_OBJECT Obj;
    ACPI_OBJECT PackageContainer[2];
    ACPI_OBJECT Package1[4];
    ACPI_OBJECT Package2[4];
} PRT_OBJECT_BUFFER, *PPRT_OBJECT_BUFFER;

DEFINE_GUID(MY_DSM_GUID,
            0xB76E0B40, 0x3EC6, 0x4DBD, 0x8A, 0xCB, 0x8B, 0xCA, 0x65, 0xB8, 0xBC, 0x70);

static const ULONG DrvpBifIntegerFields[9] =
{
    0, 50000, 50000, 1, 10000, 100, 50, 1, 1
};

static const ULONG DrvpMyDsmIntegerFields[3] =
{
    0xAAAAAAAA, 0xBBBBBBBB, 0xDDDDDDDD
};

static const EVAL_TEST_ENTRY* DrvpEvalTestEntry;

void *
AcpiOsAllocate (
    ACPI_SIZE               Size)
{
    return ExAllocatePoolWithTag(NonPagedPool, Size, 'FFUB');
}

void
AcpiOsFree (
    void *                  Memory)
{
    ExFreePoolWithTag(Memory, 'FFUB');
}

ACPI_STATUS
AcpiGetName (
    ACPI_HANDLE             Handle,
    UINT32                  NameType,
    ACPI_BUFFER             *Buffer)
{
    /* We don't support anything else */
    ok(NameType == ACPI_SINGLE_NAME, "Unexpected call to %s\n", __FUNCTION__);

    /* Return a NULL-terminated string */
    if (Buffer->Length < (4 + 1))
        return AE_BUFFER_OVERFLOW;

    switch ((ULONG_PTR)Handle)
    {
        case FAKE_SB_NAMESPACE_ACPI_HANDLE:
            RtlCopyMemory(Buffer->Pointer, "_SB_", sizeof("_SB_"));
            break;

        case FAKE_INTB_ACPI_HANDLE:
            RtlCopyMemory(Buffer->Pointer, "LNKB", sizeof("LNKB"));
            break;

        case FAKE_INTC_ACPI_HANDLE:
            RtlCopyMemory(Buffer->Pointer, "LNKC", sizeof("LNKC"));
            break;

        default:
            return AE_BAD_PARAMETER;
    }

    return AE_OK;
}

ACPI_STATUS
AcpiEvaluateObject (
    ACPI_HANDLE             Object,
    ACPI_STRING             Pathname,
    ACPI_OBJECT_LIST        *ParameterObjects,
    ACPI_BUFFER             *ReturnObjectBuffer)
{
    UNREFERENCED_PARAMETER(Object);

    /* We don't support anything else */
    ok(ReturnObjectBuffer->Length == ACPI_ALLOCATE_BUFFER,
       "Unexpected call to %s\n", __FUNCTION__);

    if (strcmp(Pathname, "_STM") == 0)
    {
        ACPI_OBJECT* Arg;
        PIDE_ACPI_TIMING_MODE_BLOCK TimingMode;

        if (ParameterObjects->Count > 3)
            return AE_AML_UNINITIALIZED_ARG;

        if (ParameterObjects->Count != 3)
            return AE_OK;

        /* Argument 1 */
        {
            Arg = ParameterObjects->Pointer;

            ok_eq_uint_ex(DrvpEvalTestEntry, Arg->Type, ACPI_TYPE_BUFFER);
            ok_eq_uint_ex(DrvpEvalTestEntry, Arg->Buffer.Length, sizeof(*TimingMode));

            TimingMode = (PIDE_ACPI_TIMING_MODE_BLOCK)Arg->Buffer.Pointer;

            ok_eq_ulong_ex(DrvpEvalTestEntry, TimingMode->Drive[0].PioSpeed, 508LU);
            ok_eq_ulong_ex(DrvpEvalTestEntry, TimingMode->Drive[0].DmaSpeed, 120LU);
            ok_eq_ulong_ex(DrvpEvalTestEntry, TimingMode->Drive[1].PioSpeed, 240LU);
            ok_eq_ulong_ex(DrvpEvalTestEntry, TimingMode->Drive[1].DmaSpeed, 180LU);
            ok_eq_hex_ex(DrvpEvalTestEntry, TimingMode->ModeFlags, 0x10LU);
        }
        /* Argument 2 */
        {
            ++Arg;

            ok_eq_uint_ex(DrvpEvalTestEntry, Arg->Type, ACPI_TYPE_BUFFER);
            ok_eq_uint_ex(DrvpEvalTestEntry, Arg->Buffer.Length, STM_ID_BLOCK_SIZE);
        }
        /* Argument 3 */
        {
            ++Arg;

            ok_eq_uint_ex(DrvpEvalTestEntry, Arg->Type, ACPI_TYPE_BUFFER);
            ok_eq_uint_ex(DrvpEvalTestEntry, Arg->Buffer.Length, STM_ID_BLOCK_SIZE);
        }

        return AE_OK;
    }
    else if (strcmp(Pathname, "_GTM") == 0)
    {
        PGTM_OBJECT_BUFFER ReturnObject;
        PIDE_ACPI_TIMING_MODE_BLOCK TimingMode;

        ReturnObject = AcpiOsAllocate(sizeof(*ReturnObject));
        if (!ReturnObject)
            return AE_NO_MEMORY;

        /*
         * VPC 2007 output
         * AcpiGetHandle(NULL, "\\_SB.PCI0.IDE0.CHN1", &ObjHandle);
         * AcpiEvaluateObject(ObjHandle, "_GTM", NULL, &ReturnObj);
         */
        TimingMode = &ReturnObject->TimingMode;
        TimingMode->Drive[0].PioSpeed = 900;
        TimingMode->Drive[0].DmaSpeed = 900;
        TimingMode->Drive[1].PioSpeed = 120;
        TimingMode->Drive[1].DmaSpeed = 120;
        TimingMode->ModeFlags = 0x12;

        ReturnObject->Obj.Type = ACPI_TYPE_BUFFER;
        ReturnObject->Obj.Buffer.Length = sizeof(*TimingMode);
        ReturnObject->Obj.Buffer.Pointer = (PUCHAR)TimingMode;

        ReturnObjectBuffer->Pointer = ReturnObject;
        ReturnObjectBuffer->Length = sizeof(*ReturnObject);
        return AE_OK;
    }
    else if (strcmp(Pathname, "_BIF") == 0)
    {
        PBIF_OBJECT_BUFFER ReturnObject;
        ACPI_OBJECT* BatteryInformation;
        ULONG i;

        ReturnObject = AcpiOsAllocate(sizeof(*ReturnObject));
        if (!ReturnObject)
            return AE_NO_MEMORY;

        /*
         * Vbox 7.0 output
         * AcpiGetHandle(NULL, "\\_SB.PCI0.BAT0", &ObjHandle);
         * AcpiEvaluateObject(ObjHandle, "_BIF", NULL, &ReturnObj);
         */
        BatteryInformation = &ReturnObject->BatteryInformation[0];
        for (i = 0; i < RTL_NUMBER_OF(DrvpBifIntegerFields); ++i)
        {
            BatteryInformation[i].Integer.Type = ACPI_TYPE_INTEGER;
            BatteryInformation[i].Integer.Value = DrvpBifIntegerFields[i];
        }
        BatteryInformation[i].String.Type = ACPI_TYPE_STRING;
        BatteryInformation[i].String.Length = 1; /* Excluding the trailing null */
        BatteryInformation[i].String.Pointer = &ReturnObject->Buffer1[0];
        RtlCopyMemory(BatteryInformation[i].String.Pointer, "1", sizeof("1"));
        ++i;
        BatteryInformation[i].String.Type = ACPI_TYPE_STRING;
        BatteryInformation[i].String.Length = 1;
        BatteryInformation[i].String.Pointer = &ReturnObject->Buffer2[0];
        RtlCopyMemory(BatteryInformation[i].String.Pointer, "0", sizeof("0"));
        ++i;
        BatteryInformation[i].String.Type = ACPI_TYPE_STRING;
        BatteryInformation[i].String.Length = 4;
        BatteryInformation[i].String.Pointer = &ReturnObject->Buffer3[0];
        RtlCopyMemory(BatteryInformation[i].String.Pointer, "VBOX", sizeof("VBOX"));
        ++i;
        BatteryInformation[i].String.Type = ACPI_TYPE_STRING;
        BatteryInformation[i].String.Length = 7;
        BatteryInformation[i].String.Pointer = &ReturnObject->Buffer4[0];
        RtlCopyMemory(BatteryInformation[i].String.Pointer, "innotek", sizeof("innotek"));

        ReturnObject->Obj.Type = ACPI_TYPE_PACKAGE;
        ReturnObject->Obj.Package.Count = 13;
        ReturnObject->Obj.Package.Elements = BatteryInformation;

        ReturnObjectBuffer->Pointer = ReturnObject;
        ReturnObjectBuffer->Length = sizeof(*ReturnObject);
        return AE_OK;
    }
    else if (strcmp(Pathname, "_PCL") == 0)
    {
        PPCL_OBJECT_BUFFER ReturnObject;

        ReturnObject = AcpiOsAllocate(sizeof(*ReturnObject));
        if (!ReturnObject)
            return AE_NO_MEMORY;

        /*
         * Vbox 7.0 output
         * AcpiGetHandle(NULL, "\\_SB.PCI0.AC", &ObjHandle);
         * AcpiEvaluateObject(ObjHandle, "_PCL", NULL, &ReturnObj);
         */
        ReturnObject->Obj.Reference.Type = ACPI_TYPE_LOCAL_REFERENCE;
        ReturnObject->Obj.Reference.ActualType = ACPI_TYPE_DEVICE;
        ReturnObject->Obj.Reference.Handle = (ACPI_HANDLE)(ULONG_PTR)FAKE_SB_NAMESPACE_ACPI_HANDLE;

        ReturnObjectBuffer->Pointer = ReturnObject;
        ReturnObjectBuffer->Length = sizeof(*ReturnObject);
        return AE_OK;
    }
    else if (strcmp(Pathname, "_PRT") == 0)
    {
        PPRT_OBJECT_BUFFER ReturnObject;
        ULONG i;

        ReturnObject = AcpiOsAllocate(sizeof(*ReturnObject));
        if (!ReturnObject)
            return AE_NO_MEMORY;

        /*
         * Vbox 7.0 output
         * AcpiGetHandle(NULL, "\\_SB.PCI0", &ObjHandle);
         * AcpiEvaluateObject(ObjHandle, "_PRT", NULL, &ReturnObj);
         *
         * NOTE: To avoid similar copies of code executed and tested over and over again
         * we return 2 packages. The original method returns 120 packages.
         */
        ReturnObject->Obj.Type = ACPI_TYPE_PACKAGE;
        ReturnObject->Obj.Package.Count = 2;
        ReturnObject->Obj.Package.Elements = &ReturnObject->PackageContainer[0];

        i = 0;
        ReturnObject->PackageContainer[i].Type = ACPI_TYPE_PACKAGE;
        ReturnObject->PackageContainer[i].Package.Count = 4;
        ReturnObject->PackageContainer[i].Package.Elements = &ReturnObject->Package1[0];
        ++i;
        ReturnObject->PackageContainer[i].Type = ACPI_TYPE_PACKAGE;
        ReturnObject->PackageContainer[i].Package.Count = 4;
        ReturnObject->PackageContainer[i].Package.Elements = &ReturnObject->Package2[0];

        /* Package 1 */
        i = 0;
        ReturnObject->Package1[i].Integer.Type = ACPI_TYPE_INTEGER;
        ReturnObject->Package1[i].Integer.Value = 0x0002FFFF;
        ++i;
        ReturnObject->Package1[i].Integer.Type = ACPI_TYPE_INTEGER;
        ReturnObject->Package1[i].Integer.Value = 0x00000000;
        ++i;
        ReturnObject->Package1[i].Reference.Type = ACPI_TYPE_LOCAL_REFERENCE;
        ReturnObject->Package1[i].Reference.ActualType = ACPI_TYPE_DEVICE;
        ReturnObject->Package1[i].Reference.Handle = (ACPI_HANDLE)(ULONG_PTR)FAKE_INTB_ACPI_HANDLE;
        ++i;
        ReturnObject->Package1[i].Integer.Type = ACPI_TYPE_INTEGER;
        ReturnObject->Package1[i].Integer.Value = 0x00000000;

        /* Package 2 */
        i = 0;
        ReturnObject->Package2[i].Integer.Type = ACPI_TYPE_INTEGER;
        ReturnObject->Package2[i].Integer.Value = 0x0002FFFF;
        ++i;
        ReturnObject->Package2[i].Integer.Type = ACPI_TYPE_INTEGER;
        ReturnObject->Package2[i].Integer.Value = 0x00000001;
        ++i;
        ReturnObject->Package2[i].Reference.Type = ACPI_TYPE_LOCAL_REFERENCE;
        ReturnObject->Package2[i].Reference.ActualType = ACPI_TYPE_DEVICE;
        ReturnObject->Package2[i].Reference.Handle = (ACPI_HANDLE)(ULONG_PTR)FAKE_INTC_ACPI_HANDLE;
        ++i;
        ReturnObject->Package2[i].Integer.Type = ACPI_TYPE_INTEGER;
        ReturnObject->Package2[i].Integer.Value = 0x00000000;

        ReturnObjectBuffer->Pointer = ReturnObject;
        ReturnObjectBuffer->Length = sizeof(*ReturnObject);
        return AE_OK;
    }
    else if (strcmp(Pathname, "_DSM") == 0)
    {
        ACPI_OBJECT* Arg;

        /* Assumed object count per the spec */
        ok_eq_uint(ParameterObjects->Count, 4);

        if (ParameterObjects->Count != 4)
            return AE_AML_UNINITIALIZED_ARG;

        /* Argument 1 */
        {
            Arg = ParameterObjects->Pointer;

            ok_eq_uint_ex(DrvpEvalTestEntry, Arg->Type, ACPI_TYPE_BUFFER);
            ok_eq_uint_ex(DrvpEvalTestEntry, Arg->Buffer.Length, sizeof(GUID));
        }

        /* NOTE: This UUID doesn't exist, for testing purposes only */
        if (IsEqualGUID(Arg->Buffer.Pointer, &MY_DSM_GUID))
        {
            /* Argument 2 */
            {
                ++Arg;

                ok_eq_uint_ex(DrvpEvalTestEntry, Arg->Type, ACPI_TYPE_INTEGER);
                ok_eq_ulonglong_ex(DrvpEvalTestEntry, Arg->Integer.Value, 1ULL);
            }
            /* Argument 3 */
            {
                ++Arg;

                ok_eq_uint_ex(DrvpEvalTestEntry, Arg->Type, ACPI_TYPE_INTEGER);
                ok_eq_ulonglong_ex(DrvpEvalTestEntry, Arg->Integer.Value, 2ULL);
            }
            /* Argument 4 */
            {
                ++Arg;

                ok_eq_uint_ex(DrvpEvalTestEntry, Arg->Type, ACPI_TYPE_PACKAGE);

                if (DrvpEvalTestEntry->Flags & DSM_TEST_FLAG_EMPTY_PACKAGE)
                {
                    ok_eq_uint_ex(DrvpEvalTestEntry, Arg->Package.Count, 0);
                    ok_eq_pointer_ex(DrvpEvalTestEntry, Arg->Package.Elements, NULL);
                }
                else
                {
                    ACPI_OBJECT* PackageArg;
                    ACPI_OBJECT* PackageArg2;
                    ULONG i;

                    ok_eq_uint_ex(DrvpEvalTestEntry, Arg->Package.Count, 4);
                    ok_not_pointer_ex(DrvpEvalTestEntry, Arg->Package.Elements, NULL);

                    if (!Arg->Package.Elements)
                        return AE_AML_UNINITIALIZED_ARG;

                    /* Package 1 Arguments 1-2 */
                    PackageArg = Arg->Package.Elements;
                    for (i = 0; i < RTL_NUMBER_OF(DrvpMyDsmIntegerFields) - 1; i++)
                    {
                        ok_eq_uint_ex(DrvpEvalTestEntry, PackageArg->Type, ACPI_TYPE_INTEGER);
                        ok_eq_ulonglong_ex(DrvpEvalTestEntry,
                                           PackageArg->Integer.Value,
                                           (ULONG64)DrvpMyDsmIntegerFields[i]);

                        ++PackageArg;
                    }

                    /* Package 1 Argument 3 */
                    {
                        Arg = PackageArg;

                        ok_eq_uint_ex(DrvpEvalTestEntry, Arg->Type, ACPI_TYPE_PACKAGE);
                        ok_eq_uint_ex(DrvpEvalTestEntry, Arg->Package.Count, 1);

                        /* Package 2 Argument 1 */
                        PackageArg2 = Arg->Package.Elements;

                        ok_eq_uint_ex(DrvpEvalTestEntry, PackageArg2->Type, ACPI_TYPE_STRING);

                        /* Excluding the trailing null */
                        ok_eq_uint_ex(DrvpEvalTestEntry,
                                      PackageArg2->String.Length,
                                      (sizeof("1_TESTDATATESTDATA_2") - 1));
                        ok_eq_int_ex(DrvpEvalTestEntry,
                                     memcmp(PackageArg2->String.Pointer,
                                            "1_TESTDATATESTDATA_2",
                                            sizeof("1_TESTDATATESTDATA_2") - 1),
                                     0);
                    }
                    /* Package 1 Argument 4 */
                    {
                        ++PackageArg;

                        ok_eq_uint_ex(DrvpEvalTestEntry, PackageArg->Type, ACPI_TYPE_INTEGER);
                        ok_eq_ulonglong_ex(DrvpEvalTestEntry,
                                           PackageArg->Integer.Value,
                                           (ULONG64)DrvpMyDsmIntegerFields[2]);
                    }
                }
            }

            return AE_OK;
        }
    }

    return AE_NOT_FOUND;
}

#include "../../../../drivers/bus/acpi/eval.c"

/* GLOBALS ********************************************************************/

/* 2 ID blocks + timings + room for the test data */
#define STM_MAX_BUFFER_SIZE \
    (FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_COMPLEX, Argument) + \
     2 * STM_ID_BLOCK_SIZE + sizeof(IDE_ACPI_TIMING_MODE_BLOCK) + 0x100)

#define GTM_MAX_BUFFER_SIZE \
    (FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) + \
     ACPI_METHOD_ARGUMENT_LENGTH(sizeof(IDE_ACPI_TIMING_MODE_BLOCK)) + 0x50)

static const EVAL_TEST_ENTRY DrvpSmtTests[] =
{
    { __LINE__, 0, STATUS_SUCCESS },
    { __LINE__, STM_TEST_FLAG_INVALID_SIZE_1, STATUS_ACPI_INVALID_ARGTYPE },
    { __LINE__, STM_TEST_FLAG_LARGE_ARG_BUFFER, STATUS_ACPI_INVALID_ARGTYPE },
    { __LINE__, STM_TEST_FLAG_SUB_IN_BUFFER, STATUS_SUCCESS, 1 },
    { __LINE__, STM_TEST_FLAG_SUB_IN_BUFFER, STATUS_SUCCESS, 9 },
    { __LINE__, STM_TEST_FLAG_SUB_IRP_BUFFER, STATUS_SUCCESS, 1 },
    { __LINE__, STM_TEST_FLAG_SUB_IRP_BUFFER, STATUS_SUCCESS, 9 },
    { __LINE__, STM_TEST_FLAG_SET_IN_BUFFER, STATUS_SUCCESS, 0 },
    { __LINE__, STM_TEST_FLAG_SET_IRP_BUFFER, STATUS_INFO_LENGTH_MISMATCH, 0 },
    { __LINE__, STM_TEST_FLAG_SET_IRP_BUFFER, STATUS_INFO_LENGTH_MISMATCH,
                RTL_SIZEOF_THROUGH_FIELD(ACPI_EVAL_INPUT_BUFFER, Signature) - 2 },
    { __LINE__, STM_TEST_FLAG_SET_IRP_BUFFER, STATUS_INFO_LENGTH_MISMATCH,
                RTL_SIZEOF_THROUGH_FIELD(ACPI_EVAL_INPUT_BUFFER, Signature) - 1 },
    { __LINE__, STM_TEST_FLAG_SET_IRP_BUFFER, STATUS_INFO_LENGTH_MISMATCH,
                RTL_SIZEOF_THROUGH_FIELD(ACPI_EVAL_INPUT_BUFFER, Signature)  },
    { __LINE__, STM_TEST_FLAG_SET_IRP_BUFFER, STATUS_INFO_LENGTH_MISMATCH,
                sizeof(ACPI_EVAL_INPUT_BUFFER) - 2 },
    { __LINE__, STM_TEST_FLAG_SET_IRP_BUFFER, STATUS_INFO_LENGTH_MISMATCH,
                sizeof(ACPI_EVAL_INPUT_BUFFER) - 1 },
    { __LINE__, STM_TEST_FLAG_SET_IRP_BUFFER, STATUS_INSUFFICIENT_RESOURCES,
                sizeof(ACPI_EVAL_INPUT_BUFFER) },
    { __LINE__, STM_TEST_FLAG_SET_IRP_BUFFER, STATUS_INSUFFICIENT_RESOURCES,
                sizeof(ACPI_EVAL_INPUT_BUFFER) + 1 },
    { __LINE__, STM_TEST_FLAG_SET_IRP_BUFFER, STATUS_INSUFFICIENT_RESOURCES,
                sizeof(ACPI_EVAL_INPUT_BUFFER) + 2 },
    { __LINE__, STM_TEST_FLAG_BAD_ARG_TYPE, STATUS_SUCCESS, 0 },
    { __LINE__, STM_TEST_FLAG_CHANGE_ARG_COUNT, STATUS_ACPI_INCORRECT_ARGUMENT_COUNT, 0 },

#if 0
    /*
     * The return status depends on AML interpreter implementation
     * and testing it is not practical, keeping this for reference only.
     */
    { __LINE__, STM_TEST_FLAG_INVALID_ARG_3_1, STATUS_SUCCESS },
    { __LINE__, STM_TEST_FLAG_INVALID_ARG_3_2, STATUS_ACPI_INVALID_ARGTYPE },
    { __LINE__, STM_TEST_FLAG_INVALID_ARG_3_4, STATUS_SUCCESS },
    { __LINE__, STM_TEST_FLAG_INVALID_ARG_3_5, STATUS_SUCCESS },
    { __LINE__, STM_TEST_FLAG_CHANGE_ARG_COUNT, STATUS_ACPI_INCORRECT_ARGUMENT_COUNT, 30 },
    { __LINE__, STM_TEST_FLAG_CHANGE_ARG_COUNT, STATUS_ACPI_INCORRECT_ARGUMENT_COUNT, 2 },
#endif
};

static const EVAL_TEST_ENTRY DrvpGtmTests[] =
{
    { __LINE__, GTM_TEST_FLAG_METHOD_SUCCESS, STATUS_SUCCESS },
    { __LINE__, GTM_TEST_FLAG_METHOD_SUCCESS |
                GTM_TEST_FLAG_INC_OUT_BUFFER, STATUS_SUCCESS, 1 },
    { __LINE__, GTM_TEST_FLAG_METHOD_SUCCESS |
                GTM_TEST_FLAG_DEC_OUT_BUFFER, STATUS_BUFFER_OVERFLOW, 1 },
    { __LINE__, GTM_TEST_FLAG_SET_OUT_BUFFER, STATUS_SUCCESS, 0 },
    { __LINE__, GTM_TEST_FLAG_SET_OUT_BUFFER, STATUS_BUFFER_TOO_SMALL, 1 },
    { __LINE__, GTM_TEST_FLAG_SET_OUT_BUFFER, STATUS_BUFFER_TOO_SMALL,
                FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) - 1 },
    { __LINE__, GTM_TEST_FLAG_SET_OUT_BUFFER, STATUS_BUFFER_TOO_SMALL,
                FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) },
    { __LINE__, GTM_TEST_FLAG_SET_OUT_BUFFER, STATUS_BUFFER_TOO_SMALL,
                FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) + 1 },
    { __LINE__, GTM_TEST_FLAG_SET_OUT_BUFFER, STATUS_BUFFER_TOO_SMALL,
                sizeof(ACPI_EVAL_OUTPUT_BUFFER) - 1 },
    { __LINE__, GTM_TEST_FLAG_BUFFER_HAS_SIGNARUTE |
                GTM_TEST_FLAG_BUFFER_HAS_COUNT |
                GTM_TEST_FLAG_BUFFER_HAS_LENGTH |
                GTM_TEST_FLAG_SET_OUT_BUFFER,
                STATUS_BUFFER_OVERFLOW,
                sizeof(ACPI_EVAL_OUTPUT_BUFFER) },
    { __LINE__, GTM_TEST_FLAG_BUFFER_HAS_SIGNARUTE |
                GTM_TEST_FLAG_BUFFER_HAS_COUNT |
                GTM_TEST_FLAG_BUFFER_HAS_LENGTH |
                GTM_TEST_FLAG_SET_OUT_BUFFER,
                STATUS_BUFFER_OVERFLOW,
                sizeof(ACPI_EVAL_OUTPUT_BUFFER) + 1 },

    /* Pass an invalid signature */
    { __LINE__, GTM_TEST_FLAG_BAD_SIGNARUTE, STATUS_INVALID_PARAMETER_1 },
    { __LINE__, GTM_TEST_FLAG_BAD_SIGNARUTE | GTM_TEST_FLAG_SET_OUT_BUFFER,
                STATUS_INVALID_PARAMETER_1, 0 },
    { __LINE__, GTM_TEST_FLAG_BAD_SIGNARUTE | GTM_TEST_FLAG_SET_OUT_BUFFER,
                STATUS_BUFFER_TOO_SMALL,
                sizeof(ACPI_EVAL_OUTPUT_BUFFER) - 1 },
    { __LINE__, GTM_TEST_FLAG_BAD_SIGNARUTE | GTM_TEST_FLAG_SET_OUT_BUFFER,
                STATUS_INVALID_PARAMETER_1,
                sizeof(ACPI_EVAL_OUTPUT_BUFFER) },
};

static const EVAL_TEST_ENTRY DrvpBifTests[] =
{
    { __LINE__, 0, STATUS_SUCCESS },
};

static const EVAL_TEST_ENTRY DrvpPclTests[] =
{
    { __LINE__, 0, STATUS_SUCCESS },
};

static const EVAL_TEST_ENTRY DrvpPrtTests[] =
{
    { __LINE__, 0, STATUS_SUCCESS },
};

static const EVAL_TEST_ENTRY DrvpDsmTests[] =
{
    { __LINE__, 0, STATUS_SUCCESS },
    { __LINE__, DSM_TEST_FLAG_EMPTY_PACKAGE, STATUS_SUCCESS },
    { __LINE__, DSM_TEST_FLAG_LARGE_SUB_PACKAGE_BUFFER, STATUS_ACPI_INVALID_ARGTYPE },
};

/* FUNCTIONS ******************************************************************/

static
NTSTATUS
DrvCallAcpiDriver(
    _In_ PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_opt_ PACPI_EVAL_OUTPUT_BUFFER OutputBuffer,
    _In_ ULONG OutputBufferLength)
{
    PDO_DEVICE_DATA DeviceData;
    IRP Irp;

    DeviceData.AcpiHandle = NULL;

    Irp.AssociatedIrp.SystemBuffer = InputBuffer;
    Irp.OutputBuffer = OutputBuffer;

    Irp.MyStack.Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
    Irp.MyStack.Parameters.DeviceIoControl.OutputBufferLength = OutputBufferLength;

    return Bus_PDO_EvalMethod(&DeviceData, &Irp);
}

static
VOID
DrvEvaluateStmObject(
    _In_ const EVAL_TEST_ENTRY* TestEntry,
    _In_ PIDE_ACPI_TIMING_MODE_BLOCK TimingMode,
    _In_ PUCHAR IdBlock,
    _In_ PACPI_EVAL_INPUT_BUFFER_COMPLEX InputBuffer)
{
    PACPI_METHOD_ARGUMENT Argument, Argument2, Argument3;
    NTSTATUS Status;
    ULONG InputBufferSize, IrpBufferSize;

    InputBufferSize = FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_COMPLEX, Argument) +
                      ACPI_METHOD_ARGUMENT_LENGTH(sizeof(*TimingMode)) +
                      ACPI_METHOD_ARGUMENT_LENGTH(STM_ID_BLOCK_SIZE) +
                      ACPI_METHOD_ARGUMENT_LENGTH(STM_ID_BLOCK_SIZE);

    if (TestEntry->Flags & STM_TEST_FLAG_INVALID_SIZE_1)
    {
        InputBufferSize -= ACPI_METHOD_ARGUMENT_LENGTH(STM_ID_BLOCK_SIZE) +
                           ACPI_METHOD_ARGUMENT_LENGTH(STM_ID_BLOCK_SIZE);
    }

    InputBuffer->MethodNameAsUlong = 'MTS_'; // _STM
    InputBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
    if (TestEntry->Flags & STM_TEST_FLAG_SUB_IN_BUFFER)
    {
        InputBuffer->Size = InputBufferSize - TestEntry->Value;
    }
    else if (TestEntry->Flags & STM_TEST_FLAG_SET_IN_BUFFER)
    {
        InputBuffer->Size = TestEntry->Value;
    }
    else
    {
        InputBuffer->Size = InputBufferSize;
    }

    if (TestEntry->Flags & STM_TEST_FLAG_CHANGE_ARG_COUNT)
    {
        InputBuffer->ArgumentCount = TestEntry->Value;
    }
    else
    {
        InputBuffer->ArgumentCount = 3;
    }

    /* Argument 1: The channel timing information block */
    Argument = InputBuffer->Argument;
    ACPI_METHOD_SET_ARGUMENT_BUFFER(Argument, TimingMode, sizeof(*TimingMode));

    /* Argument 2: The ATA drive ID block */
    Argument2 = ACPI_METHOD_NEXT_ARGUMENT(Argument);
    ACPI_METHOD_SET_ARGUMENT_BUFFER(Argument2, IdBlock, STM_ID_BLOCK_SIZE);

    /* Argument 3: The ATA drive ID block */
    Argument3 = ACPI_METHOD_NEXT_ARGUMENT(Argument2);
    ACPI_METHOD_SET_ARGUMENT_BUFFER(Argument3, IdBlock, STM_ID_BLOCK_SIZE);

    if (TestEntry->Flags & STM_TEST_FLAG_BAD_ARG_TYPE)
    {
        Argument3->Type = 0xFFFF;
    }

    if (TestEntry->Flags & STM_TEST_FLAG_LARGE_ARG_BUFFER)
    {
        Argument2->DataLength = STM_ID_BLOCK_SIZE * 2;
        Argument3->DataLength = STM_ID_BLOCK_SIZE * 2;
    }

    if (TestEntry->Flags & STM_TEST_FLAG_INVALID_ARG_3_1)
    {
        ACPI_METHOD_SET_ARGUMENT_STRING(Argument3, IdBlock);
    }
    else if (TestEntry->Flags & STM_TEST_FLAG_INVALID_ARG_3_2)
    {
        ACPI_METHOD_SET_ARGUMENT_INTEGER(Argument3, 0xDEADBEEF);
    }
    else if (TestEntry->Flags & STM_TEST_FLAG_INVALID_ARG_3_4)
    {
        Argument3->DataLength += 5;
    }
    else if (TestEntry->Flags & STM_TEST_FLAG_INVALID_ARG_3_5)
    {
        Argument3->DataLength -= 5;
    }

    if (TestEntry->Flags & STM_TEST_FLAG_SUB_IRP_BUFFER)
    {
        IrpBufferSize = InputBufferSize - TestEntry->Value;
    }
    else if (TestEntry->Flags & STM_TEST_FLAG_SET_IRP_BUFFER)
    {
        IrpBufferSize = TestEntry->Value;
    }
    else
    {
        IrpBufferSize = InputBufferSize;
    }

    /* Evaluate the _STM method */
    DrvpEvalTestEntry = TestEntry;
    Status = DrvCallAcpiDriver(InputBuffer, IrpBufferSize, NULL, 0);

    ok_eq_hex_ex(TestEntry, Status, TestEntry->Status);
}

static
VOID
DrvTestComplexBuffer(VOID)
{
    IDE_ACPI_TIMING_MODE_BLOCK TimingMode;
    UCHAR IdBlock[STM_ID_BLOCK_SIZE];
    ULONG i;
    UCHAR Buffer[STM_MAX_BUFFER_SIZE];
    PACPI_EVAL_INPUT_BUFFER_COMPLEX InputBuffer = (PACPI_EVAL_INPUT_BUFFER_COMPLEX)Buffer;

    /* Initialize method arguments */
    RtlZeroMemory(IdBlock, sizeof(IdBlock));
    TimingMode.Drive[0].PioSpeed = 508;
    TimingMode.Drive[0].DmaSpeed = 120;
    TimingMode.Drive[1].PioSpeed = 240;
    TimingMode.Drive[1].DmaSpeed = 180;
    TimingMode.ModeFlags = 0x10;

    for (i = 0; i < RTL_NUMBER_OF(DrvpSmtTests); ++i)
    {
        DrvEvaluateStmObject(&DrvpSmtTests[i], &TimingMode, IdBlock, InputBuffer);
    }
}

static
VOID
DrvEvaluateGtmObject(
    _In_ const EVAL_TEST_ENTRY* TestEntry,
    _In_ PACPI_EVAL_OUTPUT_BUFFER OutputBuffer)
{
    ACPI_EVAL_INPUT_BUFFER InputBuffer;
    ULONG OutputBufferSize;
    NTSTATUS Status;
    PACPI_METHOD_ARGUMENT Argument;
    ULONG Signature, Count, Length;
    USHORT Type, DataLength;
    PIDE_ACPI_TIMING_MODE_BLOCK TimingMode;

    InputBuffer.MethodNameAsUlong = 'MTG_'; // _GTM
    if (TestEntry->Flags & GTM_TEST_FLAG_BAD_SIGNARUTE)
        InputBuffer.Signature = 'BAD0';
    else
        InputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;

    OutputBufferSize = FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) +
                       ACPI_METHOD_ARGUMENT_LENGTH(sizeof(*TimingMode));

    if (TestEntry->Flags & GTM_TEST_FLAG_INC_OUT_BUFFER)
    {
        OutputBufferSize += TestEntry->Value;
    }
    else if (TestEntry->Flags & GTM_TEST_FLAG_DEC_OUT_BUFFER)
    {
        OutputBufferSize -= TestEntry->Value;
    }
    else if (TestEntry->Flags & GTM_TEST_FLAG_SET_OUT_BUFFER)
    {
        OutputBufferSize = TestEntry->Value;
    }

    /* Evaluate the _GTM method */
    Status = DrvCallAcpiDriver(&InputBuffer, sizeof(InputBuffer), OutputBuffer, OutputBufferSize);

    ok_eq_hex_ex(TestEntry, Status, TestEntry->Status);

    if (TestEntry->Flags & GTM_TEST_FLAG_BUFFER_HAS_SIGNARUTE)
        Signature = ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE;
    else
        Signature = 0;
    ok_eq_hex_ex(TestEntry, OutputBuffer->Signature, Signature);

    if (TestEntry->Flags & GTM_TEST_FLAG_BUFFER_HAS_COUNT)
        Count = 1;
    else
        Count = 0;
    ok_eq_ulong_ex(TestEntry, OutputBuffer->Count, Count);

    if (TestEntry->Flags & GTM_TEST_FLAG_BUFFER_HAS_LENGTH)
    {
        Length = FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) +
                 ACPI_METHOD_ARGUMENT_LENGTH(sizeof(*TimingMode));
    }
    else
    {
        Length = 0;
    }
    ok_eq_ulong_ex(TestEntry, OutputBuffer->Length, Length);

    Argument = OutputBuffer->Argument;
    if (TestEntry->Flags & GTM_TEST_FLAG_ARG_HAS_BUFFER_TYPE)
        Type = ACPI_METHOD_ARGUMENT_BUFFER;
    else
        Type = ACPI_METHOD_ARGUMENT_INTEGER;
    ok_eq_uint_ex(TestEntry, Argument->Type, Type);

    if (TestEntry->Flags & GTM_TEST_FLAG_ARG_HAS_DATA_LENGTH)
        DataLength = sizeof(ACPI_EVAL_OUTPUT_BUFFER);
    else
        DataLength = 0;
    ok_eq_uint_ex(TestEntry, Argument->DataLength, DataLength);

    if ((TestEntry->Flags & GTM_TEST_FLAG_ARG_HAS_BUFFER_TYPE) && NT_SUCCESS(TestEntry->Status))
    {
        TimingMode = (PIDE_ACPI_TIMING_MODE_BLOCK)Argument->Data;

        ok_eq_ulong_ex(TestEntry, TimingMode->Drive[0].PioSpeed, 900LU);
        ok_eq_ulong_ex(TestEntry, TimingMode->Drive[0].DmaSpeed, 900LU);
        ok_eq_ulong_ex(TestEntry, TimingMode->Drive[1].PioSpeed, 120LU);
        ok_eq_ulong_ex(TestEntry, TimingMode->Drive[1].DmaSpeed, 120LU);
        ok_eq_hex_ex(TestEntry, TimingMode->ModeFlags, 0x12LU);
    }
}

static
VOID
DrvTestInputBuffer(VOID)
{
    UCHAR Buffer[GTM_MAX_BUFFER_SIZE];
    ULONG i;
    PACPI_EVAL_OUTPUT_BUFFER OutputBuffer = (PACPI_EVAL_OUTPUT_BUFFER)Buffer;

    for (i = 0; i < RTL_NUMBER_OF(DrvpGtmTests); ++i)
    {
        RtlZeroMemory(Buffer, sizeof(Buffer));

        DrvEvaluateGtmObject(&DrvpGtmTests[i], OutputBuffer);
    }
}

static
VOID
DrvEvaluateBifObject(
    _In_ const EVAL_TEST_ENTRY* TestEntry,
    _In_ PACPI_EVAL_OUTPUT_BUFFER OutputBuffer)
{
    ACPI_EVAL_INPUT_BUFFER InputBuffer;
    ULONG i, OutputBufferSize;
    NTSTATUS Status;
    PACPI_METHOD_ARGUMENT Argument;

    InputBuffer.MethodNameAsUlong = 'FIB_'; // _BIF
    InputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;

    OutputBufferSize = FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) +
                       ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG)) * 9 +
                       ACPI_METHOD_ARGUMENT_LENGTH(sizeof("1")) +
                       ACPI_METHOD_ARGUMENT_LENGTH(sizeof("0")) +
                       ACPI_METHOD_ARGUMENT_LENGTH(sizeof("VBOX")) +
                       ACPI_METHOD_ARGUMENT_LENGTH(sizeof("innotek"));

    /* Evaluate the _BIF method */
    Status = DrvCallAcpiDriver(&InputBuffer, sizeof(InputBuffer), OutputBuffer, OutputBufferSize);

    ok_eq_hex_ex(TestEntry, Status, TestEntry->Status);
    ok_eq_hex_ex(TestEntry, OutputBuffer->Signature, (ULONG)ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE);
    ok_eq_ulong_ex(TestEntry, OutputBuffer->Count, 13LU);
    ok_eq_ulong_ex(TestEntry, OutputBuffer->Length, OutputBufferSize);

    /* Arguments 1-9 */
    Argument = OutputBuffer->Argument;
    for (i = 0; i < RTL_NUMBER_OF(DrvpBifIntegerFields); ++i)
    {
        ok_eq_uint_ex(TestEntry, Argument->Type, ACPI_METHOD_ARGUMENT_INTEGER);
        ok_eq_uint_ex(TestEntry, Argument->DataLength, sizeof(ULONG));
        ok_eq_ulong_ex(TestEntry, Argument->Argument, DrvpBifIntegerFields[i]);

        Argument = ACPI_METHOD_NEXT_ARGUMENT(Argument);
    }
    /* Argument 10 */
    {
        ok_eq_uint_ex(TestEntry, Argument->Type, ACPI_METHOD_ARGUMENT_STRING);
        ok_eq_uint_ex(TestEntry, Argument->DataLength, sizeof("1")); // Including the trailing null
        ok_eq_str_ex(TestEntry, (PCSTR)Argument->Data, "1");
    }
    /* Argument 11 */
    {
        Argument = ACPI_METHOD_NEXT_ARGUMENT(Argument);

        ok_eq_uint_ex(TestEntry, Argument->Type, ACPI_METHOD_ARGUMENT_STRING);
        ok_eq_uint_ex(TestEntry, Argument->DataLength, sizeof("0"));
        ok_eq_str_ex(TestEntry, (PCSTR)Argument->Data, "0");
    }
    /* Argument 12 */
    {
        Argument = ACPI_METHOD_NEXT_ARGUMENT(Argument);

        ok_eq_uint_ex(TestEntry, Argument->Type, ACPI_METHOD_ARGUMENT_STRING);
        ok_eq_uint_ex(TestEntry, Argument->DataLength, sizeof("VBOX"));
        ok_eq_str_ex(TestEntry, (PCSTR)Argument->Data, "VBOX");
    }
    /* Argument 13 */
    {
        Argument = ACPI_METHOD_NEXT_ARGUMENT(Argument);

        ok_eq_uint_ex(TestEntry, Argument->Type, ACPI_METHOD_ARGUMENT_STRING);
        ok_eq_uint_ex(TestEntry, Argument->DataLength, sizeof("innotek"));
        ok_eq_str_ex(TestEntry, (PCSTR)Argument->Data, "innotek");
    }
}

static
VOID
DrvTestPackageReturnValueAndStringData(VOID)
{
    UCHAR Buffer[0x100];
    ULONG i;
    PACPI_EVAL_OUTPUT_BUFFER OutputBuffer = (PACPI_EVAL_OUTPUT_BUFFER)Buffer;

    for (i = 0; i < RTL_NUMBER_OF(DrvpBifTests); ++i)
    {
        RtlZeroMemory(Buffer, sizeof(Buffer));

        DrvEvaluateBifObject(&DrvpBifTests[i], OutputBuffer);
    }
}

static
VOID
DrvEvaluatePclObject(
    _In_ const EVAL_TEST_ENTRY* TestEntry,
    _In_ PACPI_EVAL_OUTPUT_BUFFER OutputBuffer)
{
    ACPI_EVAL_INPUT_BUFFER InputBuffer;
    ULONG OutputBufferSize;
    NTSTATUS Status;
    PACPI_METHOD_ARGUMENT Argument;

    InputBuffer.MethodNameAsUlong = 'LCP_'; // _PCL
    InputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;

    OutputBufferSize = FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) +
                       ACPI_METHOD_ARGUMENT_LENGTH(sizeof("ABCD")); // ACPI name for the object

    /* Evaluate the _PCL method */
    Status = DrvCallAcpiDriver(&InputBuffer, sizeof(InputBuffer), OutputBuffer, OutputBufferSize);

    ok_eq_hex_ex(TestEntry, Status, TestEntry->Status);
    ok_eq_hex_ex(TestEntry, OutputBuffer->Signature, (ULONG)ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE);
    ok_eq_ulong_ex(TestEntry, OutputBuffer->Count, 1LU);
    ok_eq_ulong_ex(TestEntry, OutputBuffer->Length, OutputBufferSize);

    Argument = OutputBuffer->Argument;

    ok_eq_uint_ex(TestEntry, Argument->Type, ACPI_METHOD_ARGUMENT_STRING);
    ok_eq_uint_ex(TestEntry, Argument->DataLength, sizeof("ABCD"));
    ok_eq_str_ex(TestEntry, (PCSTR)Argument->Data, "_SB_");
}

static
VOID
DrvTestReferenceReturnValue(VOID)
{
    UCHAR Buffer[0x100];
    ULONG i;
    PACPI_EVAL_OUTPUT_BUFFER OutputBuffer = (PACPI_EVAL_OUTPUT_BUFFER)Buffer;

    for (i = 0; i < RTL_NUMBER_OF(DrvpPclTests); ++i)
    {
        RtlZeroMemory(Buffer, sizeof(Buffer));

        DrvEvaluatePclObject(&DrvpPclTests[i], OutputBuffer);
    }
}

static
VOID
DrvEvaluatePrtObject(
    _In_ const EVAL_TEST_ENTRY* TestEntry,
    _In_ PACPI_EVAL_OUTPUT_BUFFER OutputBuffer)
{
    ACPI_EVAL_INPUT_BUFFER InputBuffer;
    ULONG PackageNum, ArgNum, OutputBufferSize;
    NTSTATUS Status;
    PACPI_METHOD_ARGUMENT Argument, PackageArgument;

    InputBuffer.MethodNameAsUlong = 'TRP_'; // _PRT
    InputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;

#define PRT_PACKAGE_ENTRY_SIZE \
    (ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG)) * 3 + \
     ACPI_METHOD_ARGUMENT_LENGTH(sizeof("LNKB")))

    OutputBufferSize = FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) +
                       ACPI_METHOD_ARGUMENT_LENGTH(PRT_PACKAGE_ENTRY_SIZE) * 2;

    /* Evaluate the _PRT method */
    Status = DrvCallAcpiDriver(&InputBuffer, sizeof(InputBuffer), OutputBuffer, OutputBufferSize);

    ok_eq_hex_ex(TestEntry, Status, TestEntry->Status);
    ok_eq_hex_ex(TestEntry, OutputBuffer->Signature, (ULONG)ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE);
    ok_eq_ulong_ex(TestEntry, OutputBuffer->Count, 2LU);
    ok_eq_ulong_ex(TestEntry, OutputBuffer->Length, OutputBufferSize);

    Argument = OutputBuffer->Argument;
    for (PackageNum = 0; PackageNum < 2; PackageNum++)
    {
        ok_eq_uint_ex(TestEntry, Argument->Type, ACPI_METHOD_ARGUMENT_PACKAGE);
        ok_eq_uint_ex(TestEntry, Argument->DataLength, (USHORT)PRT_PACKAGE_ENTRY_SIZE);

        PackageArgument = (PACPI_METHOD_ARGUMENT)Argument->Data;
        for (ArgNum = 0; ArgNum < 4; ArgNum++)
        {
            if (ArgNum != 2)
            {
                ULONG ExpectedValue;

                ok_eq_uint_ex(TestEntry, PackageArgument->Type, ACPI_METHOD_ARGUMENT_INTEGER);
                ok_eq_uint_ex(TestEntry, PackageArgument->DataLength, sizeof(ULONG));

                if (ArgNum == 0)
                {
                    ExpectedValue = 0x0002FFFF;
                }
                else
                {
                    if ((PackageNum == 1) && (ArgNum == 1))
                        ExpectedValue = 0x00000001;
                    else
                        ExpectedValue = 0x00000000;
                }
                ok_eq_ulong_ex(TestEntry, PackageArgument->Argument, ExpectedValue);
            }
            else
            {
                ok_eq_uint_ex(TestEntry, PackageArgument->Type, ACPI_METHOD_ARGUMENT_STRING);
                ok_eq_uint_ex(TestEntry, PackageArgument->DataLength, sizeof("ABCD"));
                ok_eq_str_ex(TestEntry, (PCSTR)PackageArgument->Data,
                             (PackageNum == 0) ? "LNKB" : "LNKC");
            }

            PackageArgument = ACPI_METHOD_NEXT_ARGUMENT(PackageArgument);
        }

        Argument = ACPI_METHOD_NEXT_ARGUMENT(Argument);
    }
}

static
VOID
DrvTestNestedPackageReturnValue(VOID)
{
    UCHAR Buffer[0x100];
    ULONG i;
    PACPI_EVAL_OUTPUT_BUFFER OutputBuffer = (PACPI_EVAL_OUTPUT_BUFFER)Buffer;

    for (i = 0; i < RTL_NUMBER_OF(DrvpPclTests); ++i)
    {
        RtlZeroMemory(Buffer, sizeof(Buffer));

        DrvEvaluatePrtObject(&DrvpPrtTests[i], OutputBuffer);
    }
}

static
VOID
DrvEvaluateDsmObject(
    _In_ const EVAL_TEST_ENTRY* TestEntry)
{
#define MY_DSM_SUB_PACKAGE_ENTRY_SIZE \
    (ACPI_METHOD_ARGUMENT_LENGTH(sizeof("1_TESTDATATESTDATA_2")))

#define MY_DSM_PACKAGE_ENTRY_SIZE \
    (ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG)) * 3 + \
     ACPI_METHOD_ARGUMENT_LENGTH(MY_DSM_SUB_PACKAGE_ENTRY_SIZE))

#define MY_DSM_BUFFER_SIZE \
    (FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_COMPLEX, Argument) + \
     ACPI_METHOD_ARGUMENT_LENGTH(sizeof(GUID)) + \
     ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG)) + \
     ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG)) + \
     ACPI_METHOD_ARGUMENT_LENGTH(MY_DSM_PACKAGE_ENTRY_SIZE))

    UCHAR Buffer[MY_DSM_BUFFER_SIZE];
    ULONG InputSize;
    NTSTATUS Status;
    PACPI_METHOD_ARGUMENT Argument, PackageArgument, PackageArgument2;
    PACPI_EVAL_INPUT_BUFFER_COMPLEX InputBuffer = (PACPI_EVAL_INPUT_BUFFER_COMPLEX)Buffer;

    RtlZeroMemory(Buffer, sizeof(Buffer));

    InputSize = MY_DSM_BUFFER_SIZE;
    if (TestEntry->Flags & DSM_TEST_FLAG_EMPTY_PACKAGE)
    {
        InputSize -= ACPI_METHOD_ARGUMENT_LENGTH(MY_DSM_PACKAGE_ENTRY_SIZE);
        InputSize += ACPI_METHOD_ARGUMENT_LENGTH(ACPI_METHOD_ARGUMENT_LENGTH(0));
    }

    InputBuffer->MethodNameAsUlong = 'MSD_'; // _DSM
    InputBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
    InputBuffer->ArgumentCount = 4;
    InputBuffer->Size = InputSize;

    /* Argument 1: The UUID */
    Argument = InputBuffer->Argument;
    ACPI_METHOD_SET_ARGUMENT_BUFFER(Argument, &MY_DSM_GUID, sizeof(GUID));

    /* Argument 2: The Revision ID */
    Argument = ACPI_METHOD_NEXT_ARGUMENT(Argument);
    ACPI_METHOD_SET_ARGUMENT_INTEGER(Argument, 1);

    /* Argument 3: The Function Index */
    Argument = ACPI_METHOD_NEXT_ARGUMENT(Argument);
    ACPI_METHOD_SET_ARGUMENT_INTEGER(Argument, 2);

    /* Argument 4: The device-specific package */
    Argument = ACPI_METHOD_NEXT_ARGUMENT(Argument);
    Argument->Type = ACPI_METHOD_ARGUMENT_PACKAGE;
    if (TestEntry->Flags & DSM_TEST_FLAG_EMPTY_PACKAGE)
    {
        /* Empty package */
        Argument->DataLength = ACPI_METHOD_ARGUMENT_LENGTH(0);
        Argument->Argument = 0;
    }
    else
    {
        Argument->DataLength = MY_DSM_PACKAGE_ENTRY_SIZE;

        /* Package 1 Argument 1: Some test data */
        PackageArgument = (PACPI_METHOD_ARGUMENT)Argument->Data;
        ACPI_METHOD_SET_ARGUMENT_INTEGER(PackageArgument, DrvpMyDsmIntegerFields[0]);

        /* Package 1 Argument 2: Some test data */
        PackageArgument = ACPI_METHOD_NEXT_ARGUMENT(PackageArgument);
        ACPI_METHOD_SET_ARGUMENT_INTEGER(PackageArgument, DrvpMyDsmIntegerFields[1]);

        /* Package 1 Argument 3: Start a new subpackage */
        PackageArgument = ACPI_METHOD_NEXT_ARGUMENT(PackageArgument);
        PackageArgument->Type = ACPI_METHOD_ARGUMENT_PACKAGE;
        PackageArgument->DataLength = MY_DSM_SUB_PACKAGE_ENTRY_SIZE;

        /* Package 2 Argument 1: Some test data */
        PackageArgument2 = (PACPI_METHOD_ARGUMENT)PackageArgument->Data;
        ACPI_METHOD_SET_ARGUMENT_STRING(PackageArgument2, "1_TESTDATATESTDATA_2");

        if (TestEntry->Flags & DSM_TEST_FLAG_LARGE_SUB_PACKAGE_BUFFER)
        {
            PackageArgument2->DataLength = 32768;
        }
        else
        {
            /* Package 1 Argument 4: Some test data */
            PackageArgument = ACPI_METHOD_NEXT_ARGUMENT(PackageArgument);
            ACPI_METHOD_SET_ARGUMENT_INTEGER(PackageArgument, DrvpMyDsmIntegerFields[2]);
        }
    }

    /* Evaluate the _DSM method */
    DrvpEvalTestEntry = TestEntry;
    Status = DrvCallAcpiDriver(InputBuffer, InputSize, NULL, 0);

    ok_eq_hex_ex(TestEntry, Status, TestEntry->Status);
}

static
VOID
DrvTestPackageInputValue(VOID)
{
    ULONG i;

    for (i = 0; i < RTL_NUMBER_OF(DrvpDsmTests); ++i)
    {
        DrvEvaluateDsmObject(&DrvpDsmTests[i]);
    }
}

static
VOID
DrvTestUnknownMethod(VOID)
{
    NTSTATUS Status;
    ACPI_EVAL_INPUT_BUFFER InputBuffer;

    InputBuffer.MethodNameAsUlong = 'FFF_'; // _FFF
    InputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;

    /* Try to evaluate some unsupported control method */
    Status = DrvCallAcpiDriver(&InputBuffer, sizeof(InputBuffer), NULL, 0);

    ok_eq_hex(Status, STATUS_OBJECT_NAME_NOT_FOUND);
}

START_TEST(Bus_PDO_EvalMethod)
{
    DrvTestComplexBuffer();
    DrvTestInputBuffer();
    DrvTestPackageReturnValueAndStringData();
    DrvTestReferenceReturnValue();
    DrvTestNestedPackageReturnValue();
    DrvTestPackageInputValue();
    DrvTestUnknownMethod();

    ok(DrvpBlocksAllocated == 0, "Leaking memory %ld blocks\n", DrvpBlocksAllocated);
}
