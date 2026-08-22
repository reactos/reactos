/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Routines for x86 / x64 cache information gathering
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <x86x64/Cpuid.h>
#define NDEBUG
#include <debug.h>

/* DATA **********************************************************************/

typedef struct
{
    UCHAR Code;
    UCHAR Level : 4;
    UCHAR Type : 4;
    USHORT SizeinKB;
    UCHAR Associativity;
    UCHAR LineSize;
} INTEL_CACHE_DESCRIPTOR;

C_ASSERT(sizeof(INTEL_CACHE_DESCRIPTOR) == 6);

/*!
 * \brief Table of known Intel cache descriptors, as defined in the Intel SDM.
 *
 * Only data, instruction and unified caches are listed, TLBs and trace caches are ignored.
 *
 * \see "Intel® 64 and IA-32 Architectures Software Developer's Manual Volume 1: Basic Architecture" - "Table 21-12. Encoding of CPUID Leaf 2 Descriptors"
 *
 * \note For code 0x49 the L3 cache interpretation is used, which matches what Linux does:
 *     - https://github.com/torvalds/linux/blob/06cf61899d6498b33e4b7c87d99d5bd471ccc375/arch/x86/kernel/cpu/cpuid_0x2_table.c#L21
 */
static const INTEL_CACHE_DESCRIPTOR IntelCacheDescriptors[] =
{
    // 00H General Null descriptor, this byte contains no information.
    // 01H TLB Instruction TLB: 4 KByte pages, 4-way set associative, 32 entries.
    // 02H TLB Instruction TLB: 4 MByte pages, fully associative, 2 entries.
    // 03H TLB Data TLB: 4 KByte pages, 4-way set associative, 64 entries.
    // 04H TLB Data TLB: 4 MByte pages, 4-way set associative, 8 entries.
    // 05H TLB Data TLB1: 4 MByte pages, 4-way set associative, 32 entries.
    { 0x06, 1, CacheInstruction, 8, 4, 32, },// 06H Cache 1st-level instruction cache: 8 KBytes, 4-way set associative, 32 byte line size.
    { 0x08, 1, CacheInstruction, 16, 4, 32, }, // 08H Cache 1st-level instruction cache: 16 KBytes, 4-way set associative, 32 byte line size.
    { 0x09, 1, CacheInstruction, 32, 4, 64, }, // 09H Cache 1st-level instruction cache: 32KBytes, 4-way set associative, 64 byte line size.
    { 0x0A, 1, CacheData, 8, 2, 32, }, // 0AH Cache 1st-level data cache: 8 KBytes, 2-way set associative, 32 byte line size.
    // 0BH TLB Instruction TLB: 4 MByte pages, 4-way set associative, 4 entries.
    { 0x0C, 1, CacheData, 16, 4, 32, }, // 0CH Cache 1st-level data cache: 16 KBytes, 4-way set associative, 32 byte line size.
    { 0x0D, 1, CacheData, 16, 4, 64, }, // 0DH Cache 1st-level data cache: 16 KBytes, 4-way set associative, 64 byte line size.
    { 0x0E, 1, CacheData, 24, 6, 64, }, // 0EH Cache 1st-level data cache: 24 KBytes, 6-way set associative, 64 byte line size.
    { 0x1D, 2, CacheUnified, 128, 2, 64, }, // 1DH Cache 2nd-level cache: 128 KBytes, 2-way set associative, 64 byte line size.
    { 0x21, 2, CacheUnified, 256, 8, 64, }, // 21H Cache 2nd-level cache: 256 KBytes, 8-way set associative, 64 byte line size.
    { 0x22, 3, CacheUnified, 512, 4, 64, }, // 22H Cache 3rd-level cache: 512 KBytes, 4-way set associative, 64 byte line size, 2 lines per sector.
    { 0x23, 3, CacheUnified, 1024, 8, 64, }, // 23H Cache 3rd-level cache: 1 MBytes, 8-way set associative, 64 byte line size, 2 lines per sector.
    { 0x24, 2, CacheUnified, 1 * 1024, 16, 64, }, // 24H Cache 2nd-level cache: 1 MBytes, 16-way set associative, 64 byte line size.
    { 0x25, 3, CacheUnified, 2 * 1024, 8, 64, }, // 25H Cache 3rd-level cache: 2 MBytes, 8-way set associative, 64 byte line size, 2 lines per sector.
    { 0x29, 3, CacheUnified, 4 * 1024, 8, 64, }, // 29H Cache 3rd-level cache: 4 MBytes, 8-way set associative, 64 byte line size, 2 lines per sector.
    { 0x2C, 1, CacheData, 32, 8, 64, }, // 2CH Cache 1st-level data cache: 32 KBytes, 8-way set associative, 64 byte line size.
    { 0x30, 1, CacheInstruction, 32, 8, 64, }, // 30H Cache 1st-level instruction cache: 32 KBytes, 8-way set associative, 64 byte line size.
    // 40H Cache No 2nd-level cache or, if processor contains a valid 2nd-level cache, no 3rd-level cache.
    { 0x41, 2, CacheUnified, 128, 4, 32, }, // 41H Cache 2nd-level cache: 128 KBytes, 4-way set associative, 32 byte line size.
    { 0x42, 2, CacheUnified, 256, 4, 32, }, // 42H Cache 2nd-level cache: 256 KBytes, 4-way set associative, 32 byte line size.
    { 0x43, 2, CacheUnified, 512, 4, 32, }, // 43H Cache 2nd-level cache: 512 KBytes, 4-way set associative, 32 byte line size.
    { 0x44, 2, CacheUnified, 1 * 1024, 4, 32, }, // 44H Cache 2nd-level cache: 1 MByte, 4-way set associative, 32 byte line size.
    { 0x45, 2, CacheUnified, 2 * 1024, 4, 32, }, // 45H Cache 2nd-level cache: 2 MByte, 4-way set associative, 32 byte line size.
    { 0x46, 3, CacheUnified, 4 * 1024, 4, 64, }, // 46H Cache 3rd-level cache: 4 MByte, 4-way set associative, 64 byte line size.
    { 0x47, 3, CacheUnified, 8 * 1024, 8, 64, }, // 47H Cache 3rd-level cache: 8 MByte, 8-way set associative, 64 byte line size.
    { 0x48, 2, CacheUnified, 3 * 1024, 12, 64, }, // 48H Cache 2nd-level cache: 3MByte, 12-way set associative, 64 byte line size.
    { 0x49, 3, CacheUnified, 4 * 1024, 16, 64, }, // 49H Cache 3rd-level cache: 4MB, 16-way set associative, 64-byte line size (Intel Xeon processor MP, Family 0FH, Model 06H) 2nd-level cache: 4 MByte, 16-way set associative, 64 byte line size.
    { 0x4A, 3, CacheUnified, 6 * 1024, 12, 64, }, // 4AH Cache 3rd-level cache: 6MByte, 12-way set associative, 64 byte line size.
    { 0x4B, 3, CacheUnified, 8 * 1024, 16, 64, }, // 4BH Cache 3rd-level cache: 8MByte, 16-way set associative, 64 byte line size.
    { 0x4C, 3, CacheUnified, 12 * 1024, 12, 64, }, // 4CH Cache 3rd-level cache: 12MByte, 12-way set associative, 64 byte line size.
    { 0x4D, 3, CacheUnified, 16 * 1024, 16, 64, }, // 4DH Cache 3rd-level cache: 16MByte, 16-way set associative, 64 byte line size.
    { 0x4E, 2, CacheUnified, 6 * 1024, 24, 64, }, // 4EH Cache 2nd-level cache: 6MByte, 24-way set associative, 64 byte line size.
    // 4FH TLB Instruction TLB: 4 KByte pages, 32 entries.
    // 50H TLB Instruction TLB: 4 KByte and 2-MByte or 4-MByte pages, 64 entries.
    // 51H TLB Instruction TLB: 4 KByte and 2-MByte or 4-MByte pages, 128 entries.
    // 52H TLB Instruction TLB: 4 KByte and 2-MByte or 4-MByte pages, 256 entries.
    // 55H TLB Instruction TLB: 2-MByte or 4-MByte pages, fully associative, 7 entries.
    // 56H TLB Data TLB0: 4 MByte pages, 4-way set associative, 16 entries.
    // 57H TLB Data TLB0: 4 KByte pages, 4-way associative, 16 entries.
    // 59H TLB Data TLB0: 4 KByte pages, fully associative, 16 entries.
    // 5AH TLB Data TLB0: 2 MByte or 4 MByte pages, 4-way set associative, 32 entries.
    // 5BH TLB Data TLB: 4 KByte and 4 MByte pages, 64 entries.
    // 5CH TLB Data TLB: 4 KByte and 4 MByte pages,128 entries.
    // 5DH TLB Data TLB: 4 KByte and 4 MByte pages,256 entries.
    { 0x60, 1, CacheData, 16, 8, 64, }, // 60H Cache 1st-level data cache: 16 KByte, 8-way set associative, 64 byte line size.
    // 61H TLB Instruction TLB: 4 KByte pages, fully associative, 48 entries.
    // 63H TLB Data TLB: 2 MByte or 4 MByte pages, 4-way set associative, 32 entries and a separate array with 1 GByte pages, 4-way set associative, 4 entries.
    // 64H TLB Data TLB: 4 KByte pages, 4-way set associative, 512 entries.
    { 0x66, 1, CacheData, 8, 4, 64, }, // 66H Cache 1st-level data cache: 8 KByte, 4-way set associative, 64 byte line size.
    { 0x67, 1, CacheData, 16, 4, 64, }, // 67H Cache 1st-level data cache: 16 KByte, 4-way set associative, 64 byte line size.
    { 0x68, 1, CacheData, 32, 4, 64, }, // 68H Cache 1st-level data cache: 32 KByte, 4-way set associative, 64 byte line size.
    // 6AH Cache uTLB: 4 KByte pages, 8-way set associative, 64 entries.
    // 6BH Cache DTLB: 4 KByte pages, 8-way set associative, 256 entries.
    // 6CH Cache DTLB: 2M/4M pages, 8-way set associative, 128 entries.
    // 6DH Cache DTLB: 1 GByte pages, fully associative, 16 entries.
    // 70H Cache Trace cache: 12 K-μop, 8-way set associative.
    // 71H Cache Trace cache: 16 K-μop, 8-way set associative.
    // 72H Cache Trace cache: 32 K-μop, 8-way set associative.
    // 76H TLB Instruction TLB: 2M/4M pages, fully associative, 8 entries.
    { 0x78, 2, CacheUnified, 1 * 1024, 4, 64, }, // 78H Cache 2nd-level cache: 1 MByte, 4-way set associative, 64byte line size.
    { 0x79, 2, CacheUnified, 128, 8, 64, }, // 79H Cache 2nd-level cache: 128 KByte, 8-way set associative, 64 byte line size, 2 lines per sector.
    { 0x7A, 2, CacheUnified, 256, 8, 64, }, // 7AH Cache 2nd-level cache: 256 KByte, 8-way set associative, 64 byte line size, 2 lines per sector.
    { 0x7B, 2, CacheUnified, 512, 8, 64, }, // 7BH Cache 2nd-level cache: 512 KByte, 8-way set associative, 64 byte line size, 2 lines per sector.
    { 0x7C, 2, CacheUnified, 1 * 1024, 8, 64, }, // 7CH Cache 2nd-level cache: 1 MByte, 8-way set associative, 64 byte line size, 2 lines per sector.
    { 0x7D, 2, CacheUnified, 2 * 1024, 8, 64, }, // 7DH Cache 2nd-level cache: 2 MByte, 8-way set associative, 64 byte line size.
    { 0x7F, 2, CacheUnified, 512, 2, 64, }, // 7FH Cache 2nd-level cache: 512 KByte, 2-way set associative, 64-byte line size.
    { 0x80, 2, CacheUnified, 512, 8, 64, }, // 80H Cache 2nd-level cache: 512 KByte, 8-way set associative, 64-byte line size.
    { 0x82, 2, CacheUnified, 256, 8, 32, }, // 82H Cache 2nd-level cache: 256 KByte, 8-way set associative, 32 byte line size.
    { 0x83, 2, CacheUnified, 512, 8, 32, }, // 83H Cache 2nd-level cache: 512 KByte, 8-way set associative, 32 byte line size.
    { 0x84, 2, CacheUnified, 1 * 1024, 8, 32, }, // 84H Cache 2nd-level cache: 1 MByte, 8-way set associative, 32 byte line size.
    { 0x85, 2, CacheUnified, 2 * 1024, 8, 32, }, // 85H Cache 2nd-level cache: 2 MByte, 8-way set associative, 32 byte line size.
    { 0x86, 2, CacheUnified, 512, 4, 64, }, // 86H Cache 2nd-level cache: 512 KByte, 4-way set associative, 64 byte line size.
    { 0x87, 2, CacheUnified, 1 * 1024, 8, 64, }, // 87H Cache 2nd-level cache: 1 MByte, 8-way set associative, 64 byte line size.
    // A0H DTLB DTLB: 4k pages, fully associative, 32 entries.
    // B0H TLB Instruction TLB: 4 KByte pages, 4-way set associative, 128 entries.
    // B1H TLB Instruction TLB: 2M pages, 4-way, 8 entries or 4M pages, 4-way, 4 entries.
    // B2H TLB Instruction TLB: 4KByte pages, 4-way set associative, 64 entries.
    // B3H TLB Data TLB: 4 KByte pages, 4-way set associative, 128 entries.
    // B4H TLB Data TLB1: 4 KByte pages, 4-way associative, 256 entries.
    // B5H TLB Instruction TLB: 4KByte pages, 8-way set associative, 64 entries.
    // B6H TLB Instruction TLB: 4KByte pages, 8-way set associative, 128 entries.
    // BAH TLB Data TLB1: 4 KByte pages, 4-way associative, 64 entries.
    // C0H TLB Data TLB: 4 KByte and 4 MByte pages, 4-way associative, 8 entries.
    // C1H STLB Shared 2nd-Level TLB: 4 KByte/2MByte pages, 8-way associative, 1024 entries.
    // C2H DTLB DTLB: 2 MByte/4 MByte pages, 4-way associative, 16 entries.
    // C3H STLB Shared 2nd-Level TLB: 4 KByte /2 MByte pages, 6-way associative, 1536 entries. Also 1GBbyte pages, 4-way, 16 entries.
    // C4H DTLB DTLB: 2 MByte/4 MByte pages, 4-way associative, 32 entries.
    // CAH STLB Shared 2nd-Level TLB: 4 KByte pages, 4-way associative, 512 entries.
    { 0xD0, 3, CacheUnified, 512, 4, 64, }, // D0H Cache 3rd-level cache: 512 KByte, 4-way set associative, 64 byte line size.
    { 0xD1, 3, CacheUnified, 1 * 1024, 4, 64, }, // D1H Cache 3rd-level cache: 1 MByte, 4-way set associative, 64 byte line size.
    { 0xD2, 3, CacheUnified, 2 * 1024, 4, 64, }, // D2H Cache 3rd-level cache: 2 MByte, 4-way set associative, 64 byte line size.
    { 0xD6, 3, CacheUnified, 1 * 1024, 8, 64, }, // D6H Cache 3rd-level cache: 1 MByte, 8-way set associative, 64 byte line size.
    { 0xD7, 3, CacheUnified, 2 * 1024, 8, 64, }, // D7H Cache 3rd-level cache: 2 MByte, 8-way set associative, 64 byte line size.
    { 0xD8, 3, CacheUnified, 4 * 1024, 8, 64, }, // D8H Cache 3rd-level cache: 4 MByte, 8-way set associative, 64 byte line size.
    { 0xDC, 3, CacheUnified, 1 * 1024 + 512, 12, 64, }, // DCH Cache 3rd-level cache: 1.5 MByte, 12-way set associative, 64 byte line size.
    { 0xDD, 3, CacheUnified, 3 * 1024, 12, 64, }, // DDH Cache 3rd-level cache: 3 MByte, 12-way set associative, 64 byte line size.
    { 0xDE, 3, CacheUnified, 6 * 1024, 12, 64, }, // DEH Cache 3rd-level cache: 6 MByte, 12-way set associative, 64 byte line size.
    { 0xE2, 3, CacheUnified, 2 * 1024, 16, 64, }, // E2H Cache 3rd-level cache: 2 MByte, 16-way set associative, 64 byte line size.
    { 0xE3, 3, CacheUnified, 4 * 1024, 16, 64, }, // E3H Cache 3rd-level cache: 4 MByte, 16-way set associative, 64 byte line size.
    { 0xE4, 3, CacheUnified, 8 * 1024, 16, 64, }, // E4H Cache 3rd-level cache: 8 MByte, 16-way set associative, 64 byte line size.
    { 0xEA, 3, CacheUnified, 12 * 1024, 24, 64, }, // EAH Cache 3rd-level cache: 12MByte, 24-way set associative, 64 byte line size.
    { 0xEB, 3, CacheUnified, 18 * 1024, 24, 64, }, // EBH Cache 3rd-level cache: 18MByte, 24-way set associative, 64 byte line size.
    { 0xEC, 3, CacheUnified, 24 * 1024, 24, 64, }, // ECH Cache 3rd-level cache: 24MByte, 24-way set associative, 64 byte line size.
    // F0H Prefetch 64-Byte prefetching.
    // F1H Prefetch 128-Byte prefetching.
    // FEH General CPUID leaf 2 does not report TLB descriptor information; use CPUID leaf 18H to query TLB and other address translation parameters.
    // FFH General CPUID leaf 2 does not report cache descriptor information, use CPUID leaf 4 to query cache parameters.
};


/* FUNCTIONS *****************************************************************/

/*!
 * \brief Finds the cache descriptor for the given descriptor value.
 *
 * \param[in] DescriptorCode - The cache descriptor code to find
 *
 * \return A pointer to the INTEL_CACHE_DESCRIPTOR structure if found, or NULL if not found.
 */
static
const INTEL_CACHE_DESCRIPTOR*
FindIntelCacheDescriptor(_In_ UCHAR DescriptorCode)
{
    ULONG Low = 0;
    ULONG High = RTL_NUMBER_OF(IntelCacheDescriptors);

    while (Low < High)
    {
        ULONG Mid = Low + (High - Low) / 2;
        const INTEL_CACHE_DESCRIPTOR* CacheDesc = &IntelCacheDescriptors[Mid];
        if (DescriptorCode < CacheDesc->Code)
        {
            High = Mid;
        }
        else if (DescriptorCode > CacheDesc->Code)
        {
            Low = Mid + 1;
        }
        else
        {
            return CacheDesc;
        }
    }

    return NULL;
}

/*!
 * \brief Retrieves cache information using CPUID leaf 2 for Intel processors.
 *
 * \param[out] CacheDescriptors - An array to receive the cache descriptors
 * \param[in] MaxCount - The maximum number of cache descriptors to fill
 *
 * \return The number of cache descriptors retrieved.
 * 
 * \See "Intel® 64 and IA-32 Architectures Software Developer's Manual Volume 1: Basic Architecture"
 *       - "Table 21-12. Encoding of CPUID Leaf 2 Descriptors"
 */
static
ULONG
GetCacheInfoIntelLeaf2(
    _Out_cap_(MaxCount) PCACHE_DESCRIPTOR CacheDescriptors,
    _In_ ULONG MaxCount)
{
    CPUID_CACHE_INFO_REGS CacheInfoRegs;
    ULONG Passes, Pass, Reg, Byte;
    ULONG Count = 0;

    /* Query the cache info. The number of passes is stored in EAX[7..0] */
    __cpuid(CacheInfoRegs.AsInt32, CPUID_CACHE_INFO);
    Passes = CacheInfoRegs.Eax.CacheDescriptor[0];

    /* Now loop through each pass */
    for (Pass = 0; Pass < Passes; Pass++)
    {
        /* Query the information for the next pass */
        __cpuid(CacheInfoRegs.AsInt32, CPUID_CACHE_INFO);

        /* Loop through each register in the cache info */
        for (Reg = 0; Reg < ARRAYSIZE(CacheInfoRegs.Regs); Reg++)
        {
            const CPUID_CACHE_INFO_CACHE_TLB *CpuidReg = &CacheInfoRegs.Regs[Reg];

            if (CpuidReg->Bits.NotValid)
                continue;

            /* Loop through each byte in the register. For EAX we need to skip the first. */
            for (Byte = (Reg == 0) ? 1 : 0; Byte < 4; Byte++)
            {
                const INTEL_CACHE_DESCRIPTOR* IntelCacheDesc;
                UCHAR DescriptorCode = CpuidReg->CacheDescriptor[Byte];
                if (DescriptorCode == 0)
                    continue;

                IntelCacheDesc = FindIntelCacheDescriptor(DescriptorCode);
                if (IntelCacheDesc != NULL)
                {
                    /* Make sure there is enough space in the array */
                    if (Count >= MaxCount)
                    {
                        return Count;
                    }

                    CacheDescriptors[Count].Level = IntelCacheDesc->Level;
                    CacheDescriptors[Count].Associativity = IntelCacheDesc->Associativity;
                    CacheDescriptors[Count].LineSize = IntelCacheDesc->LineSize;
                    CacheDescriptors[Count].Size = IntelCacheDesc->SizeinKB * 1024;
                    CacheDescriptors[Count].Type = IntelCacheDesc->Type;
                    Count++;
                }
            }
        }
    }

    return Count;
}

/*!
 * \brief Retrieves cache information for legacy AMD processors.
 *
 * \param[out] CacheDescriptors - An array to receive cache descriptors
 * \param[in] MaxCount - The maximum number of cache descriptors to fill
 *
 * \return The number of cache descriptors filled
 *
 * \see
 * - https://docs.amd.com/v/u/en-US/24594_3.38_APM_Vol3 (E.4.4 Function 8000_0005h—L1 Cache and TLB Information
 *   and E.4.5 Function 8000_0006h—L2 Cache and TLB and L3 Cache Information)
 * - https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/56255_OSRR.pdf
 */
static
ULONG
GetCacheInfoAmdLegacy(
    _Out_cap_(MaxCount) PCACHE_DESCRIPTOR CacheDescriptors,
    _In_ ULONG MaxCount)
{
    CPUID_L1_TLB_INFO_REGS L1TlbInfo;
    CPUID_AMD_EXTENDED_CACHE_INFO_REGS CacheInfoRegs;
    ULONG Count = 0;
    static const UCHAR AssocMap[16] =
    {
        0, 1, 2, 0, 4, 0, 8, 0, 16, 0, 32, 48, 64, 96, 128, 0xFF
    };

    __cpuid(L1TlbInfo.AsInt32, CPUID_L1_TLB_INFO);

    /* L1 data cache */
    if ((L1TlbInfo.Amd.Ecx.Bits.L1DcSize != 0) && (Count < MaxCount))
    {
        CacheDescriptors[Count].Level = 1;
        CacheDescriptors[Count].Type = CacheData;
        CacheDescriptors[Count].Size = L1TlbInfo.Amd.Ecx.Bits.L1DcSize * 1024;
        CacheDescriptors[Count].Associativity = L1TlbInfo.Amd.Ecx.Bits.L1DcAssoc;
        CacheDescriptors[Count].LineSize = L1TlbInfo.Amd.Ecx.Bits.L1DcLineSize;
        Count++;
    }

    /* L1 instruction cache */
    if ((L1TlbInfo.Amd.Edx.Bits.L1IcSize != 0) && (Count < MaxCount))
    {
        CacheDescriptors[Count].Level = 1;
        CacheDescriptors[Count].Type = CacheInstruction;
        CacheDescriptors[Count].Size = L1TlbInfo.Amd.Edx.Bits.L1IcSize * 1024;
        CacheDescriptors[Count].Associativity = L1TlbInfo.Amd.Edx.Bits.L1IcAssoc;
        CacheDescriptors[Count].LineSize = L1TlbInfo.Amd.Edx.Bits.L1IcLineSize;
        Count++;
    }

    __cpuid(CacheInfoRegs.AsInt32, CPUID_EXTENDED_CACHE_INFO);

    /* L2 unified cache (Assoc 9 means look at leaf 0x8000001D) */
    if ((CacheInfoRegs.Ecx.Bits.L2Size != 0) && (CacheInfoRegs.Ecx.Bits.L2Assoc != 9) && (Count < MaxCount))
    {
        CacheDescriptors[Count].Level = 2;
        CacheDescriptors[Count].Type = CacheUnified;
        CacheDescriptors[Count].Size = CacheInfoRegs.Ecx.Bits.L2Size * 1024;
        CacheDescriptors[Count].Associativity = AssocMap[CacheInfoRegs.Ecx.Bits.L2Assoc];
        CacheDescriptors[Count].LineSize = CacheInfoRegs.Ecx.Bits.L2LineSize;
        Count++;
    }

    /* L3 unified cache (Assoc 9 means look at leaf 0x8000001D) */
    if ((CacheInfoRegs.Edx.Bits.L3Size != 0) && (CacheInfoRegs.Edx.Bits.L3Assoc != 9) && (Count < MaxCount))
    {
        CacheDescriptors[Count].Level = 3;
        CacheDescriptors[Count].Type = CacheUnified;
        CacheDescriptors[Count].Size = CacheInfoRegs.Edx.Bits.L3Size * 512 * 1024;
        CacheDescriptors[Count].Associativity = AssocMap[CacheInfoRegs.Edx.Bits.L3Assoc];
        CacheDescriptors[Count].LineSize = CacheInfoRegs.Edx.Bits.L3LineSize;
        Count++;
    }

    return Count;
}

/*!
 * \brief Retrieves cache information for modern Intel/AMD processors.
 *
 * \param[out] CacheDescriptors - An array to receive cache descriptors
 * \param[in] MaxCount - The maximum number of cache descriptors to fill
 * \param[in] CpuIdLeaf - The CPUID leaf to use (0x04 for Intel, 0x8000001D for AMD)
 *
 * \return The number of cache descriptors filled
 */
static
ULONG
KiGetCacheInfoModern(
    _Out_cap_(MaxCount) PCACHE_DESCRIPTOR CacheDescriptors,
    _In_ ULONG MaxCount,
    _In_ ULONG CpuIdLeaf)
{
    CPUID_CACHE_PARAMS_REGS Regs;
    ULONG Subleaf;
    ULONG Count;

    Count = 0;

    /* Iterate through subleaves, bail out at 32 or when all descriptors are filled */
    for (Subleaf = 0; (Subleaf < 32) && (Count < MaxCount); Subleaf++)
    {
        __cpuidex(Regs.AsInt32, CpuIdLeaf, Subleaf);

        switch (Regs.Eax.Bits.CacheType)
        {
            case CPUID_CACHE_PARAMS_CACHE_TYPE_DATA:
                CacheDescriptors[Count].Type = CacheData;
                break;

            case CPUID_CACHE_PARAMS_CACHE_TYPE_INSTRUCTION:
                CacheDescriptors[Count].Type = CacheInstruction;
                break;

            case CPUID_CACHE_PARAMS_CACHE_TYPE_UNIFIED:
                CacheDescriptors[Count].Type = CacheUnified;
                break;

            case CPUID_CACHE_PARAMS_CACHE_TYPE_NULL:
                return Count;

            default:
                continue;
        }

        if (Regs.Eax.Bits.FullyAssociativeCache)
        {
            CacheDescriptors[Count].Associativity = 0xFF;
        }
        else
        {
            ULONG Ways = Regs.Ebx.Bits.Ways + 1;
            CacheDescriptors[Count].Associativity = (Ways > 0xFE) ? 0xFE : (UCHAR)Ways;
        }

        CacheDescriptors[Count].Level = (UCHAR)Regs.Eax.Bits.CacheLevel;
        CacheDescriptors[Count].LineSize = (USHORT)(Regs.Ebx.Bits.LineSize + 1);
        CacheDescriptors[Count].Size = (Regs.Ebx.Bits.Ways + 1) *
                                       (Regs.Ebx.Bits.LinePartitions + 1) *
                                       (Regs.Ebx.Bits.LineSize + 1) *
                                       (Regs.NumberOfSets + 1);
        Count++;
    }

    return Count;
}

/*!
 * \brief Normalizes the cache descriptors by removing duplicates and sorting them.
 *
 * \param[in,out] CacheDescriptors - An array of cache descriptors
 * \param[in] CacheCount - The number of cache descriptors
 * \param[in] MaxCount - The maximum number of cache descriptors to fill
 *
 * \return The number of valid cache descriptors after normalization
 */
static
ULONG
NormalizeCacheDescriptors(
    _Inout_updates_(MaxCount) PCACHE_DESCRIPTOR CacheDescriptors,
    _In_ ULONG CacheCount,
    _In_ ULONG MaxCount)
{
    ULONG l, r, i;
    ULONG NewCount = 0;

    /* Sort the array */
    for (l = 0; l < CacheCount; l++)
    {
        for (r = l + 1; r < CacheCount; r++)
        {
            /* Sort order: 1. Level (ascending), 2. Type (enum value, descending) */
            if ((CacheDescriptors[l].Level > CacheDescriptors[r].Level) ||
                ((CacheDescriptors[l].Level == CacheDescriptors[r].Level) &&
                 (CacheDescriptors[l].Type < CacheDescriptors[r].Type)))
            {
                CACHE_DESCRIPTOR Temp = CacheDescriptors[l];
                CacheDescriptors[l] = CacheDescriptors[r];
                CacheDescriptors[r] = Temp;
            }
        }
    }

    /* Remove duplicates */
    if (CacheCount > 0)
    {
        CacheDescriptors[0] = CacheDescriptors[0];
        NewCount = 1;
        for (i = 1; i < CacheCount; i++)
        {
            if ((CacheDescriptors[i].Level != CacheDescriptors[i - 1].Level) ||
                (CacheDescriptors[i].Type != CacheDescriptors[i - 1].Type))
            {
                CacheDescriptors[NewCount] = CacheDescriptors[i];
                NewCount++;
            }
        }
    }

    /* Clear stray entries */
    RtlZeroMemory(&CacheDescriptors[NewCount],
                  sizeof(CACHE_DESCRIPTOR) * (MaxCount - NewCount));

    return NewCount;
}

/*!
* \brief Retrieves cache descriptors for the current CPU.
*
* \param[out] CacheDescriptors - An array to receive the cache descriptors
* \param[in] MaxCount - The maximum number of cache descriptors to fill
* \param[in] Vendor - The CPU vendor
* 
* \return The number of cache descriptors retrieved
*/
ULONG
NTAPI
KiGetCpuCacheDescriptors(
    _Out_writes_(MaxCount) PCACHE_DESCRIPTOR CacheDescriptors,
    _In_ ULONG MaxCount,
    _In_ CPU_VENDORS Vendor)
{
    CPUID_SIGNATURE_REGS Signature;
    CPUID_EXTENDED_FUNCTION_REGS ExtendedFunction;
    ULONG Count = 0;

    /* Get the maximum leaf and maximum extended leaf */
    __cpuid(&Signature.AsInt32[0], CPUID_SIGNATURE);
    __cpuid(ExtendedFunction.AsInt32, CPUID_EXTENDED_FUNCTION);

    /* Distinguish by vendor: AMD (and Hygon) have their own custom cpuid leaves. */
    if (Vendor == CPU_AMD)
    {
        if (ExtendedFunction.MaxLeaf >= CPUID_CACHE_PARAMS_AMD)
        {
            Count = KiGetCacheInfoModern(CacheDescriptors, MaxCount, CPUID_CACHE_PARAMS_AMD);
        }

        if ((Count == 0) && (ExtendedFunction.MaxLeaf >= CPUID_EXTENDED_CACHE_INFO))
        {
            Count = GetCacheInfoAmdLegacy(CacheDescriptors, MaxCount);
        }
    }
    else
    {
        if (Signature.MaxLeaf >= CPUID_CACHE_PARAMS)
        {
            Count = KiGetCacheInfoModern(CacheDescriptors, MaxCount, CPUID_CACHE_PARAMS);
        }

        if ((Count == 0) && (Signature.MaxLeaf >= CPUID_CACHE_INFO))
        {
            Count = GetCacheInfoIntelLeaf2(CacheDescriptors, MaxCount);
        }
    }

    /* Normalize the descriptors */
    Count = NormalizeCacheDescriptors(CacheDescriptors, Count, MaxCount);

    return Count;
}
