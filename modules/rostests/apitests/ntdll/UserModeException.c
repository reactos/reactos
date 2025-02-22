/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for user mode exceptions
 * COPYRIGHT:   Copyright 2021 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"
#include <pseh/pseh2.h>

typedef enum _CPU_VENDOR
{
    CPU_VENDOR_INTEL,
    CPU_VENDOR_AMD,
    CPU_VENDOR_CYRIX,
    CPU_VENDOR_VIA,
    CPU_VENDOR_TRANSMETA,
    CPU_VENDOR_UNKNOWN
} CPU_VENDOR;

CPU_VENDOR g_CpuVendor;
ULONG g_CpuFeatures;

#define CPU_FEATURE_VMX 0x01
#define CPU_FEATURE_HV 0x02

typedef void (*PFUNC)(void);

#define FL_INVALID 0
#define FL_AMD 0x01
#define FL_INTEL 0x02
#define FL_CYRIX 0x04
#define FL_VIA 0x08
#define FL_TM 0x10
#define FL_ANY (FL_AMD | FL_INTEL | FL_CYRIX | FL_VIA | FL_TM)

#define FL_VMX 0x20
#define FL_HV 0x40

#define FL_PRIV 0x100
#define FL_ACC 0x200
#define FL_INVLS 0x400

typedef struct _TEST_ENTRY
{
    ULONG Line;
    UCHAR InstructionBytes[64];
    ULONG ExpectedAddressOffset;
    ULONG Flags;
} TEST_ENTRY, *PTEST_ENTRY;

static
CPU_VENDOR
DetermineCpuVendor(void)
{
    INT CpuInfo[4];
    union
    {
        INT ShuffledInts[3];
        CHAR VendorString[13];
    } Vendor;

    __cpuid(CpuInfo, 0);
    Vendor.ShuffledInts[0] = CpuInfo[1];
    Vendor.ShuffledInts[1] = CpuInfo[3];
    Vendor.ShuffledInts[2] = CpuInfo[2];
    Vendor.VendorString[12] = 0;
    trace("Vendor: %s\n", Vendor.VendorString);

    if (strcmp(Vendor.VendorString, "GenuineIntel") == 0)
    {
        return CPU_VENDOR_INTEL;
    }
    else if (strcmp(Vendor.VendorString, "AuthenticAMD") == 0)
    {
        return CPU_VENDOR_AMD;
    }
    else if (strcmp(Vendor.VendorString, "CyrixInstead") == 0)
    {
        return CPU_VENDOR_CYRIX;
    }
    else if (strcmp(Vendor.VendorString, "CentaurHauls") == 0)
    {
        return CPU_VENDOR_VIA;
    }
    else if (strcmp(Vendor.VendorString, "GenuineTMx86") == 0)
    {
        return CPU_VENDOR_TRANSMETA;
    }
    else
    {
        return CPU_VENDOR_UNKNOWN;
    }
}

static
void
DetermineCpuFeatures(void)
{
    INT CpuInfo[4];
    ULONG Features = 0;

    g_CpuVendor = DetermineCpuVendor();

    __cpuid(CpuInfo, 1);
    if (CpuInfo[2] & (1 << 5)) Features |= CPU_FEATURE_VMX;
    if (CpuInfo[2] & (1 << 31)) Features |= CPU_FEATURE_HV;
    trace("CPUID 1: 0x%x, 0x%x, 0x%x, 0x%x\n", CpuInfo[0], CpuInfo[1], CpuInfo[2], CpuInfo[3]);

    g_CpuFeatures = Features;
}

TEST_ENTRY TestEntries[] =
{
    /* Some invalid instruction encodings */
    { __LINE__, { 0x0F, 0x0B, 0xC3 }, 0, FL_INVALID },
    { __LINE__, { 0x0F, 0x38, 0x0C, 0xC3 }, 0, FL_INVALID },
#ifdef _M_AMD64
    { __LINE__, { 0x06, 0xC3 }, 0, FL_INVALID },
#endif

    /* Privileged instructions */
    { __LINE__, { 0xF4, 0xC3 }, 0, FL_ANY | FL_PRIV }, // HLT
    { __LINE__, { 0xFA, 0xC3 }, 0, FL_ANY | FL_PRIV }, // CLI
    { __LINE__, { 0xFB, 0xC3 }, 0, FL_ANY | FL_PRIV }, // STI
    { __LINE__, { 0x0F, 0x06, 0xC3 }, 0, FL_ANY | FL_PRIV }, // CLTS
#ifdef _M_AMD64
    { __LINE__, { 0x0F, 0x07, 0xC3 }, 0, FL_ANY | FL_PRIV }, // SYSRET
#endif
    { __LINE__, { 0x0F, 0x08, 0xC3 }, 0, FL_ANY | FL_PRIV }, // INVD
    { __LINE__, { 0x0F, 0x09, 0xC3 }, 0, FL_ANY | FL_PRIV }, // WBINVD
    { __LINE__, { 0x0F, 0x20, 0xC3 }, 0, FL_ANY | FL_PRIV }, // MOV CR, XXX
    { __LINE__, { 0x0F, 0x21, 0xC3 }, 0, FL_ANY | FL_PRIV }, // MOV DR, XXX
    { __LINE__, { 0x0F, 0x22, 0xC3 }, 0, FL_ANY | FL_PRIV }, // MOV XXX, CR
    { __LINE__, { 0x0F, 0x23, 0xC3 }, 0, FL_ANY | FL_PRIV }, // MOV YYY, DR
    { __LINE__, { 0x0F, 0x30, 0xC3 }, 0, FL_ANY | FL_PRIV }, // WRMSR
    { __LINE__, { 0x0F, 0x32, 0xC3 }, 0, FL_ANY | FL_PRIV }, // RDMSR
    { __LINE__, { 0x0F, 0x33, 0xC3 }, 0, FL_ANY | FL_PRIV }, // RDPMC
    { __LINE__, { 0x0F, 0x35, 0xC3 }, 0, FL_ANY | FL_PRIV }, // SYSEXIT
    { __LINE__, { 0x0F, 0x78, 0xC8, 0xC3 }, 0, FL_INTEL | FL_HV | FL_ACC }, // VMREAD EAX, ECX
    { __LINE__, { 0x0F, 0x79, 0xC1, 0xC3 }, 0, FL_INTEL | FL_HV | FL_ACC }, // VMWRITE EAX, ECX
    { __LINE__, { 0x0F, 0x00, 0x10, 0xC3 }, 0, FL_ANY | FL_PRIV },  // LLDT WORD PTR [EAX]
    { __LINE__, { 0x0F, 0x00, 0x50, 0x00, 0xC3 }, 0, FL_ANY | FL_PRIV },  // LLDT WORD PTR [EAX + 0x00]
    { __LINE__, { 0x0F, 0x00, 0x18, 0xC3 }, 0, FL_ANY | FL_PRIV },  // LTR WORD PTR [EAX]
    { __LINE__, { 0x0F, 0x00, 0x58, 0x00, 0xC3 }, 0, FL_ANY | FL_PRIV },  // LTR WORD PTR [EAX + 0x00]
    { __LINE__, { 0x0F, 0x01, 0xC1, 0xC3 }, 0, FL_INTEL | FL_HV },  // VMCALL
    { __LINE__, { 0x0F, 0x01, 0xC2, 0xC3 }, 0, FL_INTEL | FL_HV | FL_ACC },  // VMLAUNCH
    { __LINE__, { 0x0F, 0x01, 0xC3, 0xC3 }, 0, FL_INTEL | FL_HV | FL_ACC },  // VMRESUME
    { __LINE__, { 0x0F, 0x01, 0xC4, 0xC3 }, 0, FL_INTEL | FL_HV | FL_ACC },  // VMXOFF
    { __LINE__, { 0x0F, 0x01, 0xC8, 0xC3 }, 0, FL_INVALID },  // MONITOR
    { __LINE__, { 0x0F, 0x01, 0xC9, 0xC3 }, 0, FL_INVALID },  // MWAIT
    // { __LINE__, { 0x0F, 0x01, 0xD1, 0xC3 }, 0, FL_ANY | FL_PRIV },  // XSETBV FIXME: privileged or access violation?
#ifdef _M_AMD64
    { __LINE__, { 0x0F, 0x01, 0xF8, 0xC3 }, 0, FL_ANY | FL_PRIV },  // SWAPGS
#endif
    { __LINE__, { 0x0F, 0x01, 0x10, 0xC3 }, 0, FL_ANY | FL_PRIV },  // LGDT [EAX]
    { __LINE__, { 0x0F, 0x01, 0x18, 0xC3 }, 0, FL_ANY | FL_PRIV },  // LIDT [EAX]
    { __LINE__, { 0x0F, 0x01, 0x30, 0xC3 }, 0, FL_ANY | FL_PRIV },  // LMSW [EAX]
#ifdef _M_AMD64 // Gives access violation on Test WHS for some reason
    { __LINE__, { 0x0F, 0x01, 0x38, 0xC3 }, 0, FL_ANY | FL_PRIV },  // INVLPG [EAX]
#endif
    { __LINE__, { 0x66, 0x0F, 0x38, 0x80, 0x01, 0xC3 }, 0, FL_INTEL | FL_HV | FL_ACC }, // INVEPT EAX,OWORD PTR [ECX]
    { __LINE__, { 0x66, 0x0F, 0x38, 0x81, 0x01, 0xC3 }, 0, FL_INTEL | FL_HV | FL_ACC }, // INVVPID EAX,OWORD PTR [ECX]
    { __LINE__, { 0x0F, 0xC7, 0x31, 0xC3 }, 0, FL_INTEL | FL_HV | FL_ACC }, // VMPTRLD QWORD PTR [ECX]
    { __LINE__, { 0x66, 0x0F, 0xC7, 0x31, 0xC3 }, 0, FL_INTEL | FL_HV | FL_ACC }, // VMCLEAR QWORD PTR [ECX]
    { __LINE__, { 0xF3, 0x0F, 0xC7, 0x31, 0xC3 }, 0, FL_INVALID }, // VMXON QWORD PTR [ECX]
    { __LINE__, { 0x0F, 0xC7, 0xE0, 0xC3 }, 0, FL_INVALID }, // VMPTRST

    /* Test prefixes */
    { __LINE__, { 0x26, 0xF4, 0xC3 }, 0, FL_ANY | FL_PRIV }, // ES HLT
    { __LINE__, { 0x2E, 0xF4, 0xC3 }, 0, FL_ANY | FL_PRIV }, // CS: HLT
    { __LINE__, { 0x36, 0xF4, 0xC3 }, 0, FL_ANY | FL_PRIV }, // SS: HLT
    { __LINE__, { 0x3E, 0xF4, 0xC3 }, 0, FL_ANY | FL_PRIV }, // DS: HLT
    { __LINE__, { 0x64, 0xF4, 0xC3 }, 0, FL_ANY | FL_PRIV }, // FS: HLT
    { __LINE__, { 0x65, 0xF4, 0xC3 }, 0, FL_ANY | FL_PRIV }, // GS: HLT
    { __LINE__, { 0x66, 0xF4, 0xC3 }, 0, FL_ANY | FL_PRIV }, // DATA HLT
    { __LINE__, { 0x67, 0xF4, 0xC3 }, 0, FL_ANY | FL_PRIV }, // ADDR HLT
    { __LINE__, { 0xF0, 0xF4, 0xC3 }, 0, FL_ANY | FL_INVLS | FL_PRIV }, // LOCK HLT
    { __LINE__, { 0xF2, 0xF4, 0xC3 }, 0, FL_ANY | FL_PRIV }, // REP HLT
    { __LINE__, { 0xF3, 0xF4, 0xC3 }, 0, FL_ANY | FL_PRIV }, // REPZ HLT
    { __LINE__, { 0x9B, 0xF4, 0xC3 }, 1, FL_ANY | FL_PRIV }, // WAIT // not a prefix
#ifdef _M_AMD64
    { __LINE__, { 0x40, 0xF4, 0xC3 }, 0, FL_ANY | FL_PRIV }, // REX HLT
#endif
    { __LINE__, { 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0xC3 }, 0, FL_ANY }, // This one is OK
#ifdef _M_AMD64
    { __LINE__, { 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0xC3 }, 0, FL_ANY | FL_ACC }, // Too many prefixes
#else
    { __LINE__, { 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0xC3 }, 0, FL_INVALID }, // Too many prefixes
#endif
    { __LINE__, { 0xF0, 0x90, 0xC3 }, 0, FL_INVLS }, // LOCK NOP
    { __LINE__, { 0xF0, 0x83, 0x0C, 0x24, 0x00, 0xC3 }, 0, FL_ANY }, // LOCK OR DWORD PTR [ESP], 0x0 
    { __LINE__, { 0x3E, 0x66, 0x67, 0xF0, 0xF3, 0xF4, 0xC3 }, 0, FL_ANY | FL_INVLS | FL_PRIV }, // DS: DATA ADDR LOCK REPZ HLT
#ifdef _M_AMD64
    /* Check non-canonical address access (causes a #GP) */
    { __LINE__, { 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0xC3 }, 0, FL_ANY | FL_ACC }, //  MOV AL, [0x1000000000000000]
#endif

};

void Test_SingleInstruction(
    PVOID RwxMemory,
    PTEST_ENTRY TestEntry)
{
    PEXCEPTION_POINTERS ExcPtrs;
    EXCEPTION_RECORD ExceptionRecord;
    NTSTATUS ExpectedStatus, Status = STATUS_SUCCESS;
    PFUNC Func = (PFUNC)RwxMemory;
    ULONG Flags;

    RtlZeroMemory(&ExceptionRecord, sizeof(ExceptionRecord));

    RtlCopyMemory(RwxMemory,
                  TestEntry->InstructionBytes,
                  sizeof(TestEntry->InstructionBytes));

    _SEH2_TRY
    {
        Func();
    }
    _SEH2_EXCEPT(ExcPtrs = _SEH2_GetExceptionInformation(),
                 ExceptionRecord = *ExcPtrs->ExceptionRecord,
                 EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    Flags = TestEntry->Flags;
#ifdef _M_IX86
    if (Flags & FL_INVLS)
    {
        ExpectedStatus = STATUS_INVALID_LOCK_SEQUENCE;
    }
    else
#endif
    if (((g_CpuVendor == CPU_VENDOR_INTEL) && !(Flags & FL_INTEL)) ||
        ((g_CpuVendor == CPU_VENDOR_AMD) && !(Flags & FL_AMD)) ||
        ((g_CpuVendor == CPU_VENDOR_CYRIX) && !(Flags & FL_CYRIX)) ||
        ((g_CpuVendor == CPU_VENDOR_VIA) && !(Flags & FL_VIA)))
    {
        ExpectedStatus = STATUS_ILLEGAL_INSTRUCTION;
    }
    else if (((Flags & FL_VMX) && !(g_CpuFeatures & CPU_FEATURE_VMX)) ||
             ((Flags & FL_HV) && !(g_CpuFeatures & CPU_FEATURE_HV)))
    {
        ExpectedStatus = STATUS_ILLEGAL_INSTRUCTION;
    }
    else if (Flags & FL_PRIV)
    {
        ExpectedStatus = STATUS_PRIVILEGED_INSTRUCTION;
    }
    else if (Flags & FL_ACC)
    {
        ExpectedStatus = STATUS_ACCESS_VIOLATION;
    }
    else
    {
        ExpectedStatus = STATUS_SUCCESS;
    }

    ok_hex_(__FILE__, TestEntry->Line, Status, ExpectedStatus);
    ok_hex_(__FILE__, TestEntry->Line, ExceptionRecord.ExceptionCode, Status);
    ok_hex_(__FILE__, TestEntry->Line, ExceptionRecord.ExceptionFlags, 0);
    ok_ptr_(__FILE__, TestEntry->Line, ExceptionRecord.ExceptionRecord, NULL);

    if (Status == STATUS_SUCCESS)
    {
        ok_ptr_(__FILE__, TestEntry->Line, ExceptionRecord.ExceptionAddress, NULL);
    }
    else
    {
        ok_ptr_(__FILE__, TestEntry->Line, ExceptionRecord.ExceptionAddress, (PUCHAR)Func + TestEntry->ExpectedAddressOffset);
    }

    if (Status == STATUS_ACCESS_VIOLATION)
    {
        ok_dec_(__FILE__, TestEntry->Line, ExceptionRecord.NumberParameters, 2);
        ok_size_t_(__FILE__, TestEntry->Line, ExceptionRecord.ExceptionInformation[0], 0);
        ok_size_t_(__FILE__, TestEntry->Line, ExceptionRecord.ExceptionInformation[1], (LONG_PTR)-1);
    }
    else
    {
        ok_dec_(__FILE__, TestEntry->Line, ExceptionRecord.NumberParameters, 0);
#if 0 // FIXME: These are inconsistent between Windows versions (simply uninitialized?)
        ok_size_t_(__FILE__, TestEntry->Line, ExceptionRecord.ExceptionInformation[0], 0);
        ok_size_t_(__FILE__, TestEntry->Line, ExceptionRecord.ExceptionInformation[1], 0);
#endif
    }
}

static
void
Test_InstructionFaults(void)
{
    PVOID RwxMemory;
    ULONG i;

    /* Allocate a page of RWX memory */
    RwxMemory = VirtualAlloc(NULL,
                             PAGE_SIZE,
                             MEM_RESERVE | MEM_COMMIT,
                             PAGE_EXECUTE_READWRITE);
    ok(RwxMemory != NULL, "Failed to allocate RWX memory!\n");
    if (RwxMemory == NULL)
    {
        return;
    }

    for (i = 0; i < ARRAYSIZE(TestEntries); i++)
    {
        Test_SingleInstruction(RwxMemory, &TestEntries[i]);
    }

    /* Clean up */
    VirtualFree(RwxMemory, 0, MEM_RELEASE);
}

START_TEST(UserModeException)
{
    DetermineCpuFeatures();

    Test_InstructionFaults();
}
