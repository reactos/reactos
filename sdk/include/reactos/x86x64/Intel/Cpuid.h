/** @file
  Intel CPUID leaf definitions.

  Provides defines for CPUID leaf indexes.  Data structures are provided for
  registers returned by a CPUID leaf that contain one or more bit fields.
  If a register returned is a single 32-bit value, then a data structure is
  not provided for that register.

  Copyright (c) 2015 - 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
  Intel(R) 64 and IA-32 Architectures Software Developer's Manual, Volume 2A,
  November 2018, CPUID instruction.
  Architecture Specification: Intel(R) Trust Domain Extensions Module, Chap 10.2
  344425-003US, August 2021

**/

#ifndef __INTEL_CPUID_H__
#define __INTEL_CPUID_H__

/**
  CPUID Signature Information

  @param   EAX  CPUID_SIGNATURE (0x00)

  @retval  EAX  Returns the highest value the CPUID instruction recognizes for
                returning basic processor information. The value is returned is
                processor specific.
  @retval  EBX  First 4 characters of a vendor identification string.
  @retval  ECX  Last 4 characters of a vendor identification string.
  @retval  EDX  Middle 4 characters of a vendor identification string.

  <b>Example usage</b>
  @code
  UINT32 Eax;
  UINT32 Ebx;
  UINT32 Ecx;
  UINT32 Edx;

  AsmCpuid (CPUID_SIGNATURE, &Eax, &Ebx, &Ecx, &Edx);
  @endcode
**/
#define CPUID_SIGNATURE  0x00

///
/// @{ CPUID signature values returned by Intel processors
///
#define CPUID_SIGNATURE_GENUINE_INTEL_EBX  SIGNATURE_32 ('G', 'e', 'n', 'u')
#define CPUID_SIGNATURE_GENUINE_INTEL_EDX  SIGNATURE_32 ('i', 'n', 'e', 'I')
#define CPUID_SIGNATURE_GENUINE_INTEL_ECX  SIGNATURE_32 ('n', 't', 'e', 'l')
///
/// @}
///

/**
  CPUID Version Information

  @param   EAX  CPUID_VERSION_INFO (0x01)

  @retval  EAX  Returns Model, Family, Stepping Information described by the
                type CPUID_VERSION_INFO_EAX.
  @retval  EBX  Returns Brand, Cache Line Size, and Initial APIC ID described by
                the type CPUID_VERSION_INFO_EBX.
  @retval  ECX  CPU Feature Information described by the type
                CPUID_VERSION_INFO_ECX.
  @retval  EDX  CPU Feature Information described by the type
                CPUID_VERSION_INFO_EDX.

  <b>Example usage</b>
  @code
  CPUID_VERSION_INFO_EAX  Eax;
  CPUID_VERSION_INFO_EBX  Ebx;
  CPUID_VERSION_INFO_ECX  Ecx;
  CPUID_VERSION_INFO_EDX  Edx;

  AsmCpuid (CPUID_VERSION_INFO, &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, &Edx.Uint32);
  @endcode
**/
#define CPUID_VERSION_INFO  0x01

/**
  CPUID Version Information returned in EAX for CPUID leaf
  #CPUID_VERSION_INFO.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32    SteppingId       : 4; ///< [Bits   3:0] Stepping ID
    UINT32    Model            : 4; ///< [Bits   7:4] Model
    UINT32    FamilyId         : 4; ///< [Bits  11:8] Family
    UINT32    ProcessorType    : 2; ///< [Bits 13:12] Processor Type
    UINT32    Reserved1        : 2; ///< [Bits 15:14] Reserved
    UINT32    ExtendedModelId  : 4; ///< [Bits 19:16] Extended Model ID
    UINT32    ExtendedFamilyId : 8; ///< [Bits 27:20] Extended Family ID
    UINT32    Reserved2        : 4; ///< Reserved
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_VERSION_INFO_EAX;

///
/// @{ Define value for bit field CPUID_VERSION_INFO_EAX.ProcessorType
///
#define CPUID_VERSION_INFO_EAX_PROCESSOR_TYPE_ORIGINAL_OEM_PROCESSOR     0x00
#define CPUID_VERSION_INFO_EAX_PROCESSOR_TYPE_INTEL_OVERDRIVE_PROCESSOR  0x01
#define CPUID_VERSION_INFO_EAX_PROCESSOR_TYPE_DUAL_PROCESSOR             0x02
///
/// @}
///

/**
  CPUID Version Information returned in EBX for CPUID leaf
  #CPUID_VERSION_INFO.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 7:0] Provides an entry into a brand string table that contains
    /// brand strings for IA-32 processors.
    ///
    UINT32    BrandIndex    : 8;
    ///
    /// [Bits 15:8] Indicates the size of the cache line flushed by the CLFLUSH
    /// and CLFLUSHOPT instructions in 8-byte increments. This field was
    /// introduced in the Pentium 4 processor.
    ///
    UINT32    CacheLineSize : 8;
    ///
    /// [Bits 23:16] Maximum number of addressable IDs for logical processors
    /// in this physical package.
    ///
    /// @note
    /// The nearest power-of-2 integer that is not smaller than EBX[23:16] is
    /// the number of unique initial APICIDs reserved for addressing different
    /// logical processors in a physical package. This field is only valid if
    /// CPUID.1.EDX.HTT[bit 28]= 1.
    ///
    UINT32    MaximumAddressableIdsForLogicalProcessors : 8;
    ///
    /// [Bits 31:24] The 8-bit ID that is assigned to the local APIC on the
    /// processor during power up. This field was introduced in the Pentium 4
    /// processor.
    ///
    UINT32    InitialLocalApicId                        : 8;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_VERSION_INFO_EBX;

/**
  CPUID Version Information returned in ECX for CPUID leaf
  #CPUID_VERSION_INFO.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] Streaming SIMD Extensions 3 (SSE3).  A value of 1 indicates the
    /// processor supports this technology
    ///
    UINT32    SSE3                : 1;
    ///
    /// [Bit 1] A value of 1 indicates the processor supports the PCLMULQDQ
    /// instruction.  Carryless Multiplication
    ///
    UINT32    PCLMULQDQ           : 1;
    ///
    /// [Bit 2] 64-bit DS Area.  A value of 1 indicates the processor supports
    /// DS area using 64-bit layout.
    ///
    UINT32    DTES64              : 1;
    ///
    /// [Bit 3] MONITOR/MWAIT.  A value of 1 indicates the processor supports
    /// this feature.
    ///
    UINT32    MONITOR             : 1;
    ///
    /// [Bit 4] CPL Qualified Debug Store.  A value of 1 indicates the processor
    /// supports the extensions to the Debug Store feature to allow for branch
    /// message storage qualified by CPL
    ///
    UINT32    DS_CPL              : 1;
    ///
    /// [Bit 5] Virtual Machine Extensions.  A value of 1 indicates that the
    /// processor supports this technology.
    ///
    UINT32    VMX                 : 1;
    ///
    /// [Bit 6] Safer Mode Extensions. A value of 1 indicates that the processor
    /// supports this technology
    ///
    UINT32    SMX                 : 1;
    ///
    /// [Bit 7] Enhanced Intel SpeedStep(R) technology.  A value of 1 indicates
    /// that the processor supports this technology
    ///
    UINT32    EIST                : 1;
    ///
    /// [Bit 8] Thermal Monitor 2.  A value of 1 indicates whether the processor
    /// supports this technology
    ///
    UINT32    TM2                 : 1;
    ///
    /// [Bit 9] A value of 1 indicates the presence of the Supplemental Streaming
    /// SIMD Extensions 3 (SSSE3). A value of 0 indicates the instruction
    /// extensions are not present in the processor.
    ///
    UINT32    SSSE3               : 1;
    ///
    /// [Bit 10] L1 Context ID.  A value of 1 indicates the L1 data cache mode
    /// can be set to either adaptive mode or shared mode. A value of 0 indicates
    /// this feature is not supported. See definition of the IA32_MISC_ENABLE MSR
    /// Bit 24 (L1 Data Cache Context Mode) for details
    ///
    UINT32    CNXT_ID             : 1;
    ///
    /// [Bit 11] A value of 1 indicates the processor supports IA32_DEBUG_INTERFACE
    /// MSR for silicon debug
    ///
    UINT32    SDBG                : 1;
    ///
    /// [Bit 12] A value of 1 indicates the processor supports FMA (Fused Multiple
    ///  Add) extensions using YMM state.
    ///
    UINT32    FMA                 : 1;
    ///
    /// [Bit 13] CMPXCHG16B Available.  A value of 1 indicates that the feature
    /// is available.
    ///
    UINT32    CMPXCHG16B          : 1;
    ///
    /// [Bit 14] xTPR Update Control.  A value of 1 indicates that the processor
    /// supports changing IA32_MISC_ENABLE[Bit 23].
    ///
    UINT32    xTPR_Update_Control : 1;
    ///
    /// [Bit 15] Perfmon and Debug Capability:  A value of 1 indicates the
    /// processor supports the performance and debug feature indication MSR
    /// IA32_PERF_CAPABILITIES.
    ///
    UINT32    PDCM                : 1;
    UINT32    Reserved            : 1;
    ///
    /// [Bit 17] Process-context identifiers.  A value of 1 indicates that the
    /// processor supports PCIDs and that software may set CR4.PCIDE to 1.
    ///
    UINT32    PCID                : 1;
    ///
    /// [Bit 18] A value of 1 indicates the processor supports the ability to
    /// prefetch data from a memory mapped device.  Direct Cache Access.
    ///
    UINT32    DCA                 : 1;
    ///
    /// [Bit 19] A value of 1 indicates that the processor supports SSE4.1.
    ///
    UINT32    SSE4_1              : 1;
    ///
    /// [Bit 20] A value of 1 indicates that the processor supports SSE4.2.
    ///
    UINT32    SSE4_2              : 1;
    ///
    /// [Bit 21] A value of 1 indicates that the processor supports x2APIC
    /// feature.
    ///
    UINT32    x2APIC              : 1;
    ///
    /// [Bit 22] A value of 1 indicates that the processor supports MOVBE
    /// instruction.
    ///
    UINT32    MOVBE               : 1;
    ///
    /// [Bit 23] A value of 1 indicates that the processor supports the POPCNT
    /// instruction.
    ///
    UINT32    POPCNT              : 1;
    ///
    /// [Bit 24] A value of 1 indicates that the processor's local APIC timer
    /// supports one-shot operation using a TSC deadline value.
    ///
    UINT32    TSC_Deadline        : 1;
    ///
    /// [Bit 25] A value of 1 indicates that the processor supports the AESNI
    /// instruction extensions.
    ///
    UINT32    AESNI               : 1;
    ///
    /// [Bit 26] A value of 1 indicates that the processor supports the
    /// XSAVE/XRSTOR processor extended states feature, the XSETBV/XGETBV
    /// instructions, and XCR0.
    ///
    UINT32    XSAVE               : 1;
    ///
    /// [Bit 27] A value of 1 indicates that the OS has set CR4.OSXSAVE[Bit 18]
    /// to enable XSETBV/XGETBV instructions to access XCR0 and to support
    /// processor extended state management using XSAVE/XRSTOR.
    ///
    UINT32    OSXSAVE             : 1;
    ///
    /// [Bit 28] A value of 1 indicates the processor supports the AVX instruction
    /// extensions.
    ///
    UINT32    AVX                 : 1;
    ///
    /// [Bit 29] A value of 1 indicates that processor supports 16-bit
    /// floating-point conversion instructions.
    ///
    UINT32    F16C                : 1;
    ///
    /// [Bit 30] A value of 1 indicates that processor supports RDRAND instruction.
    ///
    UINT32    RDRAND              : 1;
    ///
    /// [Bit 31] A value of 1 indicates that processor is in Para-Virtualized.
    ///
    UINT32    ParaVirtualized     : 1;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_VERSION_INFO_ECX;

/**
  CPUID Version Information returned in EDX for CPUID leaf
  #CPUID_VERSION_INFO.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] Floating Point Unit On-Chip. The processor contains an x87 FPU.
    ///
    UINT32    FPU : 1;
    ///
    /// [Bit 1] Virtual 8086 Mode Enhancements.  Virtual 8086 mode enhancements,
    /// including CR4.VME for controlling the feature, CR4.PVI for protected
    /// mode virtual interrupts, software interrupt indirection, expansion of
    /// the TSS with the software indirection bitmap, and EFLAGS.VIF and
    /// EFLAGS.VIP flags.
    ///
    UINT32    VME : 1;
    ///
    /// [Bit 2] Debugging Extensions.  Support for I/O breakpoints, including
    /// CR4.DE for controlling the feature, and optional trapping of accesses to
    /// DR4 and DR5.
    ///
    UINT32    DE  : 1;
    ///
    /// [Bit 3] Page Size Extension.  Large pages of size 4 MByte are supported,
    /// including CR4.PSE for controlling the feature, the defined dirty bit in
    /// PDE (Page Directory Entries), optional reserved bit trapping in CR3,
    /// PDEs, and PTEs.
    ///
    UINT32    PSE : 1;
    ///
    /// [Bit 4] Time Stamp Counter.  The RDTSC instruction is supported,
    /// including CR4.TSD for controlling privilege.
    ///
    UINT32    TSC : 1;
    ///
    /// [Bit 5] Model Specific Registers RDMSR and WRMSR Instructions.  The
    /// RDMSR and WRMSR instructions are supported. Some of the MSRs are
    /// implementation dependent.
    ///
    UINT32    MSR : 1;
    ///
    /// [Bit 6] Physical Address Extension.  Physical addresses greater than 32
    /// bits are supported: extended page table entry formats, an extra level in
    /// the page translation tables is defined, 2-MByte pages are supported
    /// instead of 4 Mbyte pages if PAE bit is 1.
    ///
    UINT32    PAE : 1;
    ///
    /// [Bit 7] Machine Check Exception.  Exception 18 is defined for Machine
    /// Checks, including CR4.MCE for controlling the feature. This feature does
    /// not define the model-specific implementations of machine-check error
    /// logging, reporting, and processor shutdowns. Machine Check exception
    /// handlers may have to depend on processor version to do model specific
    /// processing of the exception, or test for the presence of the Machine
    /// Check feature.
    ///
    UINT32    MCE       : 1;
    ///
    /// [Bit 8] CMPXCHG8B Instruction.  The compare-and-exchange 8 bytes(64 bits)
    /// instruction is supported (implicitly locked and atomic).
    ///
    UINT32    CX8       : 1;
    ///
    /// [Bit 9] APIC On-Chip.  The processor contains an Advanced Programmable
    /// Interrupt Controller (APIC), responding to memory mapped commands in the
    /// physical address range FFFE0000H to FFFE0FFFH (by default - some
    /// processors permit the APIC to be relocated).
    ///
    UINT32    APIC      : 1;
    UINT32    Reserved1 : 1;
    ///
    /// [Bit 11] SYSENTER and SYSEXIT Instructions.  The SYSENTER and SYSEXIT
    /// and associated MSRs are supported.
    ///
    UINT32    SEP       : 1;
    ///
    /// [Bit 12] Memory Type Range Registers.  MTRRs are supported. The MTRRcap
    /// MSR contains feature bits that describe what memory types are supported,
    /// how many variable MTRRs are supported, and whether fixed MTRRs are
    /// supported.
    ///
    UINT32    MTRR      : 1;
    ///
    /// [Bit 13] Page Global Bit.  The global bit is supported in paging-structure
    /// entries that map a page, indicating TLB entries that are common to
    /// different processes and need not be flushed. The CR4.PGE bit controls
    /// this feature.
    ///
    UINT32    PGE       : 1;
    ///
    /// [Bit 14] Machine Check Architecture. A value of 1 indicates the Machine
    /// Check Architecture of reporting machine errors is supported. The MCG_CAP
    /// MSR contains feature bits describing how many banks of error reporting
    /// MSRs are supported.
    ///
    UINT32    MCA       : 1;
    ///
    /// [Bit 15] Conditional Move Instructions.  The conditional move instruction
    /// CMOV is supported. In addition, if x87 FPU is present as indicated by the
    /// CPUID.FPU feature bit, then the FCOMI and FCMOV instructions are supported.
    ///
    UINT32    CMOV      : 1;
    ///
    /// [Bit 16] Page Attribute Table.  Page Attribute Table is supported. This
    /// feature augments the Memory Type Range Registers (MTRRs), allowing an
    /// operating system to specify attributes of memory accessed through a
    /// linear address on a 4KB granularity.
    ///
    UINT32    PAT       : 1;
    ///
    /// [Bit 17] 36-Bit Page Size Extension.  4-MByte pages addressing physical
    /// memory beyond 4 GBytes are supported with 32-bit paging. This feature
    /// indicates that upper bits of the physical address of a 4-MByte page are
    /// encoded in bits 20:13 of the page-directory entry. Such physical
    /// addresses are limited by MAXPHYADDR and may be up to 40 bits in size.
    ///
    UINT32    PSE_36    : 1;
    ///
    /// [Bit 18] Processor Serial Number.  The processor supports the 96-bit
    /// processor identification number feature and the feature is enabled.
    ///
    UINT32    PSN       : 1;
    ///
    /// [Bit 19] CLFLUSH Instruction.  CLFLUSH Instruction is supported.
    ///
    UINT32    CLFSH     : 1;
    UINT32    Reserved2 : 1;
    ///
    /// [Bit 21] Debug Store.  The processor supports the ability to write debug
    /// information into a memory resident buffer.  This feature is used by the
    /// branch trace store (BTS) and precise event-based sampling (PEBS)
    /// facilities.
    ///
    UINT32    DS        : 1;
    ///
    /// [Bit 22] Thermal Monitor and Software Controlled Clock Facilities.  The
    /// processor implements internal MSRs that allow processor temperature to
    /// be monitored and processor performance to be modulated in predefined
    /// duty cycles under software control.
    ///
    UINT32    ACPI      : 1;
    ///
    /// [Bit 23] Intel MMX Technology.  The processor supports the Intel MMX
    /// technology.
    ///
    UINT32    MMX       : 1;
    ///
    /// [Bit 24] FXSAVE and FXRSTOR Instructions.  The FXSAVE and FXRSTOR
    /// instructions are supported for fast save and restore of the floating
    /// point context. Presence of this bit also indicates that CR4.OSFXSR is
    /// available for an operating system to indicate that it supports the
    /// FXSAVE and FXRSTOR instructions.
    ///
    UINT32    FXSR      : 1;
    ///
    /// [Bit 25] SSE.  The processor supports the SSE extensions.
    ///
    UINT32    SSE       : 1;
    ///
    /// [Bit 26] SSE2.  The processor supports the SSE2 extensions.
    ///
    UINT32    SSE2      : 1;
    ///
    /// [Bit 27] Self Snoop.  The processor supports the management of
    /// conflicting memory types by performing a snoop of its own cache
    /// structure for transactions issued to the bus.
    ///
    UINT32    SS        : 1;
    ///
    /// [Bit 28] Max APIC IDs reserved field is Valid.  A value of 0 for HTT
    /// indicates there is only a single logical processor in the package and
    /// software should assume only a single APIC ID is reserved. A value of 1
    /// for HTT indicates the value in CPUID.1.EBX[23:16] (the Maximum number of
    /// addressable IDs for logical processors in this package) is valid for the
    /// package.
    ///
    UINT32    HTT       : 1;
    ///
    /// [Bit 29] Thermal Monitor.  The processor implements the thermal monitor
    /// automatic thermal control circuitry (TCC).
    ///
    UINT32    TM        : 1;
    UINT32    Reserved3 : 1;
    ///
    /// [Bit 31] Pending Break Enable.  The processor supports the use of the
    /// FERR#/PBE# pin when the processor is in the stop-clock state (STPCLK# is
    /// asserted) to signal the processor that an interrupt is pending and that
    /// the processor should return to normal operation to handle the interrupt.
    /// Bit 10 (PBE enable) in the IA32_MISC_ENABLE MSR enables this capability.
    ///
    UINT32    PBE       : 1;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_VERSION_INFO_EDX;

/**
  CPUID Cache and TLB Information

  @param   EAX  CPUID_CACHE_INFO (0x02)

  @retval  EAX  Cache and TLB Information described by the type
                CPUID_CACHE_INFO_CACHE_TLB.
                CPUID_CACHE_INFO_CACHE_TLB.CacheDescriptor[0] always returns
                0x01 and must be ignored.  Only valid if
                CPUID_CACHE_INFO_CACHE_TLB.Bits.NotValid is clear.
  @retval  EBX  Cache and TLB Information described by the type
                CPUID_CACHE_INFO_CACHE_TLB.  Only valid if
                CPUID_CACHE_INFO_CACHE_TLB.Bits.NotValid is clear.
  @retval  ECX  Cache and TLB Information described by the type
                CPUID_CACHE_INFO_CACHE_TLB.  Only valid if
                CPUID_CACHE_INFO_CACHE_TLB.Bits.NotValid is clear.
  @retval  EDX  Cache and TLB Information described by the type
                CPUID_CACHE_INFO_CACHE_TLB.  Only valid if
                CPUID_CACHE_INFO_CACHE_TLB.Bits.NotValid is clear.

  <b>Example usage</b>
  @code
  CPUID_CACHE_INFO_CACHE_TLB  Eax;
  CPUID_CACHE_INFO_CACHE_TLB  Ebx;
  CPUID_CACHE_INFO_CACHE_TLB  Ecx;
  CPUID_CACHE_INFO_CACHE_TLB  Edx;

  AsmCpuid (CPUID_CACHE_INFO, &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, &Edx.Uint32);
  @endcode

  <b>Cache Descriptor values</b>
  <table>
  <tr><th>Value </th><th> Type    </th><th> Description </th></tr>
  <tr><td> 0x00 </td><td> General </td><td> Null descriptor, this byte contains no information</td></tr>
  <tr><td> 0x01 </td><td> TLB     </td><td> Instruction TLB: 4 KByte pages, 4-way set associative, 32 entries</td></tr>
  <tr><td> 0x02 </td><td> TLB     </td><td> Instruction TLB: 4 MByte pages, fully associative, 2 entries</td></tr>
  <tr><td> 0x03 </td><td> TLB     </td><td> Data TLB: 4 KByte pages, 4-way set associative, 64 entries</td></tr>
  <tr><td> 0x04 </td><td> TLB     </td><td> Data TLB: 4 MByte pages, 4-way set associative, 8 entries</td></tr>
  <tr><td> 0x05 </td><td> TLB     </td><td> Data TLB1: 4 MByte pages, 4-way set associative, 32 entries</td></tr>
  <tr><td> 0x06 </td><td> Cache   </td><td> 1st-level instruction cache: 8 KBytes, 4-way set associative,
                                            32 byte line size</td></tr>
  <tr><td> 0x08 </td><td> Cache   </td><td> 1st-level instruction cache: 16 KBytes, 4-way set associative,
                                            32 byte line size</td></tr>
  <tr><td> 0x09 </td><td> Cache   </td><td> 1st-level instruction cache: 32KBytes, 4-way set associative,
                                            64 byte line size</td></tr>
  <tr><td> 0x0A </td><td> Cache   </td><td> 1st-level data cache: 8 KBytes, 2-way set associative, 32 byte line size</td></tr>
  <tr><td> 0x0B </td><td> TLB     </td><td> Instruction TLB: 4 MByte pages, 4-way set associative, 4 entries</td></tr>
  <tr><td> 0x0C </td><td> Cache   </td><td> 1st-level data cache: 16 KBytes, 4-way set associative, 32 byte line size</td></tr>
  <tr><td> 0x0D </td><td> Cache   </td><td> 1st-level data cache: 16 KBytes, 4-way set associative, 64 byte line size</td></tr>
  <tr><td> 0x0E </td><td> Cache   </td><td> 1st-level data cache: 24 KBytes, 6-way set associative, 64 byte line size</td></tr>
  <tr><td> 0x1D </td><td> Cache   </td><td> 2nd-level cache: 128 KBytes, 2-way set associative, 64 byte line size</td></tr>
  <tr><td> 0x21 </td><td> Cache   </td><td> 2nd-level cache: 256 KBytes, 8-way set associative, 64 byte line size</td></tr>
  <tr><td> 0x22 </td><td> Cache   </td><td> 3rd-level cache: 512 KBytes, 4-way set associative, 64 byte line size,
                                            2 lines per sector</td></tr>
  <tr><td> 0x23 </td><td> Cache   </td><td> 3rd-level cache: 1 MBytes, 8-way set associative, 64 byte line size,
                                            2 lines per sector</td></tr>
  <tr><td> 0x24 </td><td> Cache   </td><td> 2nd-level cache: 1 MBytes, 16-way set associative, 64 byte line size</td></tr>
  <tr><td> 0x25 </td><td> Cache   </td><td> 3rd-level cache: 2 MBytes, 8-way set associative, 64 byte line size,
                                            2 lines per sector</td></tr>
  <tr><td> 0x29 </td><td> Cache   </td><td> 3rd-level cache: 4 MBytes, 8-way set associative, 64 byte line size,
                                            2 lines per sector</td></tr>
  <tr><td> 0x2C </td><td> Cache   </td><td> 1st-level data cache: 32 KBytes, 8-way set associative,
                                            64 byte line size</td></tr>
  <tr><td> 0x30 </td><td> Cache   </td><td> 1st-level instruction cache: 32 KBytes, 8-way set associative,
                                            64 byte line size</td></tr>
  <tr><td> 0x40 </td><td> Cache   </td><td> No 2nd-level cache or, if processor contains a valid 2nd-level cache,
                                            no 3rd-level cache</td></tr>
  <tr><td> 0x41 </td><td> Cache   </td><td> 2nd-level cache: 128 KBytes, 4-way set associative, 32 byte line size</td></tr>
  <tr><td> 0x42 </td><td> Cache   </td><td> 2nd-level cache: 256 KBytes, 4-way set associative, 32 byte line size</td></tr>
  <tr><td> 0x43 </td><td> Cache   </td><td> 2nd-level cache: 512 KBytes, 4-way set associative, 32 byte line size</td></tr>
  <tr><td> 0x44 </td><td> Cache   </td><td> 2nd-level cache: 1 MByte, 4-way set associative, 32 byte line size</td></tr>
  <tr><td> 0x45 </td><td> Cache   </td><td> 2nd-level cache: 2 MByte, 4-way set associative, 32 byte line size</td></tr>
  <tr><td> 0x46 </td><td> Cache   </td><td> 3rd-level cache: 4 MByte, 4-way set associative, 64 byte line size</td></tr>
  <tr><td> 0x47 </td><td> Cache   </td><td> 3rd-level cache: 8 MByte, 8-way set associative, 64 byte line size</td></tr>
  <tr><td> 0x48 </td><td> Cache   </td><td> 2nd-level cache: 3MByte, 12-way set associative, 64 byte line size</td></tr>
  <tr><td> 0x49 </td><td> Cache   </td><td> 3rd-level cache: 4MB, 16-way set associative, 64-byte line size
                                            (Intel Xeon processor MP, Family 0FH, Model 06H)<BR>
                                            2nd-level cache: 4 MByte, 16-way set associative, 64 byte line size</td></tr>
  <tr><td> 0x4A </td><td> Cache   </td><td> 3rd-level cache: 6MByte, 12-way set associative, 64 byte line size</td></tr>
  <tr><td> 0x4B </td><td> Cache   </td><td> 3rd-level cache: 8MByte, 16-way set associative, 64 byte line size</td></tr>
  <tr><td> 0x4C </td><td> Cache   </td><td> 3rd-level cache: 12MByte, 12-way set associative, 64 byte line size</td></tr>
  <tr><td> 0x4D </td><td> Cache   </td><td> 3rd-level cache: 16MByte, 16-way set associative, 64 byte line size</td></tr>
  <tr><td> 0x4E </td><td> Cache   </td><td> 2nd-level cache: 6MByte, 24-way set associative, 64 byte line size</td></tr>
  <tr><td> 0x4F </td><td> TLB     </td><td> Instruction TLB: 4 KByte pages, 32 entries</td></tr>
  <tr><td> 0x50 </td><td> TLB     </td><td> Instruction TLB: 4 KByte and 2-MByte or 4-MByte pages, 64 entries</td></tr>
  <tr><td> 0x51 </td><td> TLB     </td><td> Instruction TLB: 4 KByte and 2-MByte or 4-MByte pages, 128 entries</td></tr>
  <tr><td> 0x52 </td><td> TLB     </td><td> Instruction TLB: 4 KByte and 2-MByte or 4-MByte pages, 256 entries</td></tr>
  <tr><td> 0x55 </td><td> TLB     </td><td> Instruction TLB: 2-MByte or 4-MByte pages, fully associative, 7 entries</td></tr>
  <tr><td> 0x56 </td><td> TLB     </td><td> Data TLB0: 4 MByte pages, 4-way set associative, 16 entries</td></tr>
  <tr><td> 0x57 </td><td> TLB     </td><td> Data TLB0: 4 KByte pages, 4-way associative, 16 entries</td></tr>
  <tr><td> 0x59 </td><td> TLB     </td><td> Data TLB0: 4 KByte pages, fully associative, 16 entries</td></tr>
  <tr><td> 0x5A </td><td> TLB     </td><td> Data TLB0: 2 MByte or 4 MByte pages, 4-way set associative, 32 entries</td></tr>
  <tr><td> 0x5B </td><td> TLB     </td><td> Data TLB: 4 KByte and 4 MByte pages, 64 entries</td></tr>
  <tr><td> 0x5C </td><td> TLB     </td><td> Data TLB: 4 KByte and 4 MByte pages,128 entries</td></tr>
  <tr><td> 0x5D </td><td> TLB     </td><td> Data TLB: 4 KByte and 4 MByte pages,256 entries</td></tr>
  <tr><td> 0x60 </td><td> Cache   </td><td> 1st-level data cache: 16 KByte, 8-way set associative, 64 byte line size</td></tr>
  <tr><td> 0x61 </td><td> TLB     </td><td> Instruction TLB: 4 KByte pages, fully associative, 48 entries</td></tr>
  <tr><td> 0x63 </td><td> TLB     </td><td> Data TLB: 2 MByte or 4 MByte pages, 4-way set associative,
                                            32 entries and a separate array with 1 GByte pages, 4-way set associative,
                                            4 entries</td></tr>
  <tr><td> 0x64 </td><td> TLB     </td><td> Data TLB: 4 KByte pages, 4-way set associative, 512 entries</td></tr>
  <tr><td> 0x66 </td><td> Cache   </td><td> 1st-level data cache: 8 KByte, 4-way set associative, 64 byte line size</td></tr>
  <tr><td> 0x67 </td><td> Cache   </td><td> 1st-level data cache: 16 KByte, 4-way set associative, 64 byte line size</td></tr>
  <tr><td> 0x68 </td><td> Cache   </td><td> 1st-level data cache: 32 KByte, 4-way set associative, 64 byte line size</td></tr>
  <tr><td> 0x6A </td><td> Cache   </td><td> uTLB: 4 KByte pages, 8-way set associative, 64 entries</td></tr>
  <tr><td> 0x6B </td><td> Cache   </td><td> DTLB: 4 KByte pages, 8-way set associative, 256 entries</td></tr>
  <tr><td> 0x6C </td><td> Cache   </td><td> DTLB: 2M/4M pages, 8-way set associative, 128 entries</td></tr>
  <tr><td> 0x6D </td><td> Cache   </td><td> DTLB: 1 GByte pages, fully associative, 16 entries</td></tr>
  <tr><td> 0x70 </td><td> Cache   </td><td> Trace cache: 12 K-uop, 8-way set associative</td></tr>
  <tr><td> 0x71 </td><td> Cache   </td><td> Trace cache: 16 K-uop, 8-way set associative</td></tr>
  <tr><td> 0x72 </td><td> Cache   </td><td> Trace cache: 32 K-uop, 8-way set associative</td></tr>
  <tr><td> 0x76 </td><td> TLB     </td><td> Instruction TLB: 2M/4M pages, fully associative, 8 entries</td></tr>
  <tr><td> 0x78 </td><td> Cache   </td><td> 2nd-level cache: 1 MByte, 4-way set associative, 64byte line size</td></tr>
  <tr><td> 0x79 </td><td> Cache   </td><td> 2nd-level cache: 128 KByte, 8-way set associative, 64 byte line size,
                                            2 lines per sector</td></tr>
  <tr><td> 0x7A </td><td> Cache   </td><td> 2nd-level cache: 256 KByte, 8-way set associative, 64 byte line size,
                                            2 lines per sector</td></tr>
  <tr><td> 0x7B </td><td> Cache   </td><td> 2nd-level cache: 512 KByte, 8-way set associative, 64 byte line size,
                                            2 lines per sector</td></tr>
  <tr><td> 0x7C </td><td> Cache   </td><td> 2nd-level cache: 1 MByte, 8-way set associative, 64 byte line size,
                                            2 lines per sector</td></tr>
  <tr><td> 0x7D </td><td> Cache   </td><td> 2nd-level cache: 2 MByte, 8-way set associative, 64byte line size</td></tr>
  <tr><td> 0x7F </td><td> Cache   </td><td> 2nd-level cache: 512 KByte, 2-way set associative, 64-byte line size</td></tr>
  <tr><td> 0x80 </td><td> Cache   </td><td> 2nd-level cache: 512 KByte, 8-way set associative, 64-byte line size</td></tr>
  <tr><td> 0x82 </td><td> Cache   </td><td> 2nd-level cache: 256 KByte, 8-way set associative, 32 byte line size</td></tr>
  <tr><td> 0x83 </td><td> Cache   </td><td> 2nd-level cache: 512 KByte, 8-way set associative, 32 byte line size</td></tr>
  <tr><td> 0x84 </td><td> Cache   </td><td> 2nd-level cache: 1 MByte, 8-way set associative, 32 byte line size</td></tr>
  <tr><td> 0x85 </td><td> Cache   </td><td> 2nd-level cache: 2 MByte, 8-way set associative, 32 byte line size</td></tr>
  <tr><td> 0x86 </td><td> Cache   </td><td> 2nd-level cache: 512 KByte, 4-way set associative, 64 byte line size</td></tr>
  <tr><td> 0x87 </td><td> Cache   </td><td> 2nd-level cache: 1 MByte, 8-way set associative, 64 byte line size</td></tr>
  <tr><td> 0xA0 </td><td> DTLB    </td><td> DTLB: 4k pages, fully associative, 32 entries</td></tr>
  <tr><td> 0xB0 </td><td> TLB     </td><td> Instruction TLB: 4 KByte pages, 4-way set associative, 128 entries</td></tr>
  <tr><td> 0xB1 </td><td> TLB     </td><td> Instruction TLB: 2M pages, 4-way, 8 entries or 4M pages, 4-way, 4 entries</td></tr>
  <tr><td> 0xB2 </td><td> TLB     </td><td> Instruction TLB: 4KByte pages, 4-way set associative, 64 entries</td></tr>
  <tr><td> 0xB3 </td><td> TLB     </td><td> Data TLB: 4 KByte pages, 4-way set associative, 128 entries</td></tr>
  <tr><td> 0xB4 </td><td> TLB     </td><td> Data TLB1: 4 KByte pages, 4-way associative, 256 entries</td></tr>
  <tr><td> 0xB5 </td><td> TLB     </td><td> Instruction TLB: 4KByte pages, 8-way set associative, 64 entries</td></tr>
  <tr><td> 0xB6 </td><td> TLB     </td><td> Instruction TLB: 4KByte pages, 8-way set associative,
                                            128 entries</td></tr>
  <tr><td> 0xBA </td><td> TLB     </td><td> Data TLB1: 4 KByte pages, 4-way associative, 64 entries</td></tr>
  <tr><td> 0xC0 </td><td> TLB     </td><td> Data TLB: 4 KByte and 4 MByte pages, 4-way associative, 8 entries</td></tr>
  <tr><td> 0xC1 </td><td> STLB    </td><td> Shared 2nd-Level TLB: 4 KByte/2MByte pages, 8-way associative,
                                            1024 entries</td></tr>
  <tr><td> 0xC2 </td><td> DTLB    </td><td> DTLB: 4 KByte/2 MByte pages, 4-way associative, 16 entries</td></tr>
  <tr><td> 0xC3 </td><td> STLB    </td><td> Shared 2nd-Level TLB: 4 KByte /2 MByte pages, 6-way associative,
                                            1536 entries. Also 1GBbyte pages, 4-way, 16 entries.</td></tr>
  <tr><td> 0xC4 </td><td> DTLB    </td><td> DTLB: 2M/4M Byte pages, 4-way associative, 32 entries</td></tr>
  <tr><td> 0xCA </td><td> STLB    </td><td> Shared 2nd-Level TLB: 4 KByte pages, 4-way associative, 512 entries</td></tr>
  <tr><td> 0xD0 </td><td> Cache   </td><td> 3rd-level cache: 512 KByte, 4-way set associative, 64 byte line size</td></tr>
  <tr><td> 0xD1 </td><td> Cache   </td><td> 3rd-level cache: 1 MByte, 4-way set associative, 64 byte line size</td></tr>
  <tr><td> 0xD2 </td><td> Cache   </td><td> 3rd-level cache: 2 MByte, 4-way set associative, 64 byte line size</td></tr>
  <tr><td> 0xD6 </td><td> Cache   </td><td> 3rd-level cache: 1 MByte, 8-way set associative, 64 byte line size</td></tr>
  <tr><td> 0xD7 </td><td> Cache   </td><td> 3rd-level cache: 2 MByte, 8-way set associative, 64 byte line size</td></tr>
  <tr><td> 0xD8 </td><td> Cache   </td><td> 3rd-level cache: 4 MByte, 8-way set associative, 64 byte line size</td></tr>
  <tr><td> 0xDC </td><td> Cache   </td><td> 3rd-level cache: 1.5 MByte, 12-way set associative, 64 byte line size</td></tr>
  <tr><td> 0xDD </td><td> Cache   </td><td> 3rd-level cache: 3 MByte, 12-way set associative, 64 byte line size</td></tr>
  <tr><td> 0xDE </td><td> Cache   </td><td> 3rd-level cache: 6 MByte, 12-way set associative, 64 byte line size</td></tr>
  <tr><td> 0xE2 </td><td> Cache   </td><td> 3rd-level cache: 2 MByte, 16-way set associative, 64 byte line size</td></tr>
  <tr><td> 0xE3 </td><td> Cache   </td><td> 3rd-level cache: 4 MByte, 16-way set associative, 64 byte line size</td></tr>
  <tr><td> 0xE4 </td><td> Cache   </td><td> 3rd-level cache: 8 MByte, 16-way set associative, 64 byte line size</td></tr>
  <tr><td> 0xEA </td><td> Cache   </td><td> 3rd-level cache: 12MByte, 24-way set associative, 64 byte line size</td></tr>
  <tr><td> 0xEB </td><td> Cache   </td><td> 3rd-level cache: 18MByte, 24-way set associative, 64 byte line size</td></tr>
  <tr><td> 0xEC </td><td> Cache   </td><td> 3rd-level cache: 24MByte, 24-way set associative, 64 byte line size</td></tr>
  <tr><td> 0xF0 </td><td> Prefetch</td><td> 64-Byte prefetching</td></tr>
  <tr><td> 0xF1 </td><td> Prefetch</td><td> 128-Byte prefetching</td></tr>
  <tr><td> 0xFE </td><td> General </td><td> CPUID leaf 2 does not report TLB descriptor information; use CPUID
                                            leaf 18H to query TLB and other address translation parameters.</td></tr>
  <tr><td> 0xFF </td><td> General </td><td> CPUID leaf 2 does not report cache descriptor information,
                                            use CPUID leaf 4 to query cache parameters</td></tr>
  </table>
**/
#define CPUID_CACHE_INFO  0x02

/**
  CPUID Cache and TLB Information returned in EAX, EBX, ECX, and EDX for CPUID
  leaf #CPUID_CACHE_INFO.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32    Reserved : 31;
    ///
    /// [Bit 31] If 0, then the cache descriptor bytes in the register are valid.
    /// if 1, then none of the cache descriptor bytes in the register are valid.
    ///
    UINT32    NotValid : 1;
  } Bits;
  ///
  /// Array of Cache and TLB descriptor bytes
  ///
  UINT8     CacheDescriptor[4];
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_CACHE_INFO_CACHE_TLB;

/**
  CPUID Processor Serial Number

  Processor serial number (PSN) is not supported in the Pentium 4 processor
  or later.  On all models, use the PSN flag (returned using CPUID) to check
  for PSN support before accessing the feature.

  @param   EAX  CPUID_SERIAL_NUMBER (0x03)

  @retval  EAX  Reserved.
  @retval  EBX  Reserved.
  @retval  ECX  Bits 31:0 of 96 bit processor serial number. (Available in
                Pentium III processor only; otherwise, the value in this
                register is reserved.)
  @retval  EDX  Bits 63:32 of 96 bit processor serial number. (Available in
                Pentium III processor only; otherwise, the value in this
                register is reserved.)

  <b>Example usage</b>
  @code
  UINT32  Ecx;
  UINT32  Edx;

  AsmCpuid (CPUID_SERIAL_NUMBER, NULL, NULL, &Ecx, &Edx);
  @endcode
**/
#define CPUID_SERIAL_NUMBER  0x03

/**
  CPUID Cache Parameters

  @param   EAX  CPUID_CACHE_PARAMS (0x04)
  @param   ECX  Cache Level.  Valid values start at 0.  Software can enumerate
                the deterministic cache parameters for each level of the cache
                hierarchy starting with an index value of 0, until the
                parameters report the value associated with the CacheType
                field in CPUID_CACHE_PARAMS_EAX is 0.

  @retval  EAX  Returns cache type information described by the type
                CPUID_CACHE_PARAMS_EAX.
  @retval  EBX  Returns cache line and associativity information described by
                the type CPUID_CACHE_PARAMS_EBX.
  @retval  ECX  Returns the number of sets in the cache.
  @retval  EDX  Returns cache WINVD/INVD behavior described by the type
                CPUID_CACHE_PARAMS_EDX.

  <b>Example usage</b>
  @code
  UINT32                  CacheLevel;
  CPUID_CACHE_PARAMS_EAX  Eax;
  CPUID_CACHE_PARAMS_EBX  Ebx;
  UINT32                  Ecx;
  CPUID_CACHE_PARAMS_EDX  Edx;

  CacheLevel = 0;
  do {
    AsmCpuidEx (
      CPUID_CACHE_PARAMS, CacheLevel,
      &Eax.Uint32, &Ebx.Uint32, &Ecx, &Edx.Uint32
      );
    CacheLevel++;
  } while (Eax.Bits.CacheType != CPUID_CACHE_PARAMS_CACHE_TYPE_NULL);
  @endcode
**/
#define CPUID_CACHE_PARAMS  0x04

/**
  CPUID Cache Parameters Information returned in EAX for CPUID leaf
  #CPUID_CACHE_PARAMS.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 4:0] Cache type field.  If #CPUID_CACHE_PARAMS_CACHE_TYPE_NULL,
    /// then there is no information for the requested cache level.
    ///
    UINT32    CacheType             : 5;
    ///
    /// [Bits 7:5] Cache level (Starts at 1).
    ///
    UINT32    CacheLevel            : 3;
    ///
    /// [Bit 8] Self Initializing cache level (does not need SW initialization).
    ///
    UINT32    SelfInitializingCache : 1;
    ///
    /// [Bit 9] Fully Associative cache.
    ///
    UINT32    FullyAssociativeCache : 1;
    ///
    /// [Bits 13:10] Reserved.
    ///
    UINT32    Reserved              : 4;
    ///
    /// [Bits 25:14] Maximum number of addressable IDs for logical processors
    /// sharing this cache.
    ///
    /// Add one to the return value to get the result.
    /// The nearest power-of-2 integer that is not smaller than (1 + EAX[25:14])
    /// is the number of unique initial APIC IDs reserved for addressing
    /// different logical processors sharing this cache.
    ///
    UINT32    MaximumAddressableIdsForLogicalProcessors : 12;
    ///
    /// [Bits 31:26] Maximum number of addressable IDs for processor cores in
    /// the physical package.
    ///
    /// The nearest power-of-2 integer that is not smaller than (1 + EAX[31:26])
    /// is the number of unique Core_IDs reserved for addressing different
    /// processor cores in a physical package. Core ID is a subset of bits of
    /// the initial APIC ID.
    /// The returned value is constant for valid initial values in ECX. Valid
    /// ECX values start from 0.
    ///
    UINT32    MaximumAddressableIdsForProcessorCores : 6;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_CACHE_PARAMS_EAX;

///
/// @{ Define value for bit field CPUID_CACHE_PARAMS_EAX.CacheType
///
#define CPUID_CACHE_PARAMS_CACHE_TYPE_NULL         0x00
#define CPUID_CACHE_PARAMS_CACHE_TYPE_DATA         0x01
#define CPUID_CACHE_PARAMS_CACHE_TYPE_INSTRUCTION  0x02
#define CPUID_CACHE_PARAMS_CACHE_TYPE_UNIFIED      0x03
///
/// @}
///

/**
  CPUID Cache Parameters Information returned in EBX for CPUID leaf
  #CPUID_CACHE_PARAMS.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 11:0] System Coherency Line Size.  Add one to the return value to
    /// get the result.
    ///
    UINT32    LineSize       : 12;
    ///
    /// [Bits 21:12] Physical Line Partitions.  Add one to the return value to
    /// get the result.
    ///
    UINT32    LinePartitions : 10;
    ///
    /// [Bits 31:22] Ways of associativity.  Add one to the return value to get
    /// the result.
    ///
    UINT32    Ways           : 10;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_CACHE_PARAMS_EBX;

/**
  CPUID Cache Parameters Information returned in EDX for CPUID leaf
  #CPUID_CACHE_PARAMS.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] Write-Back Invalidate/Invalidate.
    /// 0 = WBINVD/INVD from threads sharing this cache acts upon lower level
    /// caches for threads sharing this cache.
    /// 1 = WBINVD/INVD is not guaranteed to act upon lower level caches of
    /// non-originating threads sharing this cache.
    ///
    UINT32    Invalidate           : 1;
    ///
    /// [Bit 1] Cache Inclusiveness.
    /// 0 = Cache is not inclusive of lower cache levels.
    /// 1 = Cache is inclusive of lower cache levels.
    ///
    UINT32    CacheInclusiveness   : 1;
    ///
    /// [Bit 2] Complex Cache Indexing.
    /// 0 = Direct mapped cache.
    /// 1 = A complex function is used to index the cache, potentially using all
    /// address bits.
    ///
    UINT32    ComplexCacheIndexing : 1;
    UINT32    Reserved             : 29;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_CACHE_PARAMS_EDX;

/**
  CPUID MONITOR/MWAIT Information

  @param   EAX  CPUID_MONITOR_MWAIT (0x05)

  @retval  EAX  Smallest monitor-line size in bytes described by the type
                CPUID_MONITOR_MWAIT_EAX.
  @retval  EBX  Largest monitor-line size in bytes described by the type
                CPUID_MONITOR_MWAIT_EBX.
  @retval  ECX  Enumeration of Monitor-Mwait extensions support described by
                the type CPUID_MONITOR_MWAIT_ECX.
  @retval  EDX  Sub C-states supported described by the type
                CPUID_MONITOR_MWAIT_EDX.

  <b>Example usage</b>
  @code
  CPUID_MONITOR_MWAIT_EAX  Eax;
  CPUID_MONITOR_MWAIT_EBX  Ebx;
  CPUID_MONITOR_MWAIT_ECX  Ecx;
  CPUID_MONITOR_MWAIT_EDX  Edx;

  AsmCpuid (CPUID_MONITOR_MWAIT, &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, &Edx.Uint32);
  @endcode
**/
#define CPUID_MONITOR_MWAIT  0x05

/**
  CPUID MONITOR/MWAIT Information returned in EAX for CPUID leaf
  #CPUID_MONITOR_MWAIT.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 15:0] Smallest monitor-line size in bytes (default is processor's
    /// monitor granularity).
    ///
    UINT32    SmallestMonitorLineSize : 16;
    UINT32    Reserved                : 16;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_MONITOR_MWAIT_EAX;

/**
  CPUID MONITOR/MWAIT Information returned in EBX for CPUID leaf
  #CPUID_MONITOR_MWAIT.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 15:0] Largest monitor-line size in bytes (default is processor's
    /// monitor granularity).
    ///
    UINT32    LargestMonitorLineSize : 16;
    UINT32    Reserved               : 16;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_MONITOR_MWAIT_EBX;

/**
  CPUID MONITOR/MWAIT Information returned in ECX for CPUID leaf
  #CPUID_MONITOR_MWAIT.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] If 0, then only EAX and EBX are valid.  If 1, then EAX, EBX, ECX,
    /// and EDX are valid.
    ///
    UINT32    ExtensionsSupported : 1;
    ///
    /// [Bit 1] Supports treating interrupts as break-event for MWAIT, even when
    /// interrupts disabled.
    ///
    UINT32    InterruptAsBreak    : 1;
    UINT32    Reserved            : 30;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_MONITOR_MWAIT_ECX;

/**
  CPUID MONITOR/MWAIT Information returned in EDX for CPUID leaf
  #CPUID_MONITOR_MWAIT.

  @note
  The definition of C0 through C7 states for MWAIT extension are
  processor-specific C-states, not ACPI C-states.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 3:0] Number of C0 sub C-states supported using MWAIT.
    ///
    UINT32    C0States : 4;
    ///
    /// [Bits 7:4] Number of C1 sub C-states supported using MWAIT.
    ///
    UINT32    C1States : 4;
    ///
    /// [Bits 11:8] Number of C2 sub C-states supported using MWAIT.
    ///
    UINT32    C2States : 4;
    ///
    /// [Bits 15:12] Number of C3 sub C-states supported using MWAIT.
    ///
    UINT32    C3States : 4;
    ///
    /// [Bits 19:16] Number of C4 sub C-states supported using MWAIT.
    ///
    UINT32    C4States : 4;
    ///
    /// [Bits 23:20] Number of C5 sub C-states supported using MWAIT.
    ///
    UINT32    C5States : 4;
    ///
    /// [Bits 27:24] Number of C6 sub C-states supported using MWAIT.
    ///
    UINT32    C6States : 4;
    ///
    /// [Bits 31:28] Number of C7 sub C-states supported using MWAIT.
    ///
    UINT32    C7States : 4;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_MONITOR_MWAIT_EDX;

/**
  CPUID Thermal and Power Management

  @param   EAX  CPUID_THERMAL_POWER_MANAGEMENT (0x06)

  @retval  EAX  Thermal and power management features described by the type
                CPUID_THERMAL_POWER_MANAGEMENT_EAX.
  @retval  EBX  Number of Interrupt Thresholds in Digital Thermal Sensor
                described by the type CPUID_THERMAL_POWER_MANAGEMENT_EBX.
  @retval  ECX  Performance features described by the type
                CPUID_THERMAL_POWER_MANAGEMENT_ECX.
  @retval  EDX  Reserved.

  <b>Example usage</b>
  @code
  CPUID_THERMAL_POWER_MANAGEMENT_EAX  Eax;
  CPUID_THERMAL_POWER_MANAGEMENT_EBX  Ebx;
  CPUID_THERMAL_POWER_MANAGEMENT_ECX  Ecx;

  AsmCpuid (CPUID_THERMAL_POWER_MANAGEMENT, &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, NULL);
  @endcode
**/
#define CPUID_THERMAL_POWER_MANAGEMENT  0x06

/**
  CPUID Thermal and Power Management Information returned in EAX for CPUID leaf
  #CPUID_THERMAL_POWER_MANAGEMENT.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] Digital temperature sensor is supported if set.
    ///
    UINT32    DigitalTemperatureSensor               : 1;
    ///
    /// [Bit 1] Intel Turbo Boost Technology Available (see IA32_MISC_ENABLE[38]).
    ///
    UINT32    TurboBoostTechnology                   : 1;
    ///
    /// [Bit 2] APIC-Timer-always-running feature is supported if set.
    ///
    UINT32    ARAT                                   : 1;
    UINT32    Reserved1                              : 1;
    ///
    /// [Bit 4] Power limit notification controls are supported if set.
    ///
    UINT32    PLN                                    : 1;
    ///
    /// [Bit 5] Clock modulation duty cycle extension is supported if set.
    ///
    UINT32    ECMD                                   : 1;
    ///
    /// [Bit 6] Package thermal management is supported if set.
    ///
    UINT32    PTM                                    : 1;
    ///
    /// [Bit 7] HWP base registers (IA32_PM_ENABLE[Bit 0], IA32_HWP_CAPABILITIES,
    /// IA32_HWP_REQUEST, IA32_HWP_STATUS) are supported if set.
    ///
    UINT32    HWP                                    : 1;
    ///
    /// [Bit 8] IA32_HWP_INTERRUPT MSR is supported if set.
    ///
    UINT32    HWP_Notification                       : 1;
    ///
    /// [Bit 9] IA32_HWP_REQUEST[Bits 41:32] is supported if set.
    ///
    UINT32    HWP_Activity_Window                    : 1;
    ///
    /// [Bit 10] IA32_HWP_REQUEST[Bits 31:24] is supported if set.
    ///
    UINT32    HWP_Energy_Performance_Preference      : 1;
    ///
    /// [Bit 11] IA32_HWP_REQUEST_PKG MSR is supported if set.
    ///
    UINT32    HWP_Package_Level_Request              : 1;
    UINT32    Reserved2                              : 1;
    ///
    /// [Bit 13] HDC base registers IA32_PKG_HDC_CTL, IA32_PM_CTL1,
    /// IA32_THREAD_STALL MSRs are supported if set.
    ///
    UINT32    HDC                                    : 1;
    ///
    /// [Bit 14] Intel Turbo Boost Max Technology 3.0 available.
    ///
    UINT32    TurboBoostMaxTechnology30              : 1;
    ///
    /// [Bit 15] HWP Capabilities.
    /// Highest Performance change is supported if set.
    ///
    UINT32    HWPCapabilities                        : 1;
    ///
    /// [Bit 16] HWP PECI override is supported if set.
    ///
    UINT32    HWPPECIOverride                        : 1;
    ///
    /// [Bit 17] Flexible HWP is supported if set.
    ///
    UINT32    FlexibleHWP                            : 1;
    ///
    /// [Bit 18] Fast access mode for the IA32_HWP_REQUEST MSR is supported if set.
    ///
    UINT32    FastAccessMode                         : 1;
    UINT32    Reserved4                              : 1;
    ///
    /// [Bit 20] Ignoring Idle Logical Processor HWP request is supported if set.
    ///
    UINT32    IgnoringIdleLogicalProcessorHWPRequest : 1;
    UINT32    Reserved5                              : 11;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_THERMAL_POWER_MANAGEMENT_EAX;

/**
  CPUID Thermal and Power Management Information returned in EBX for CPUID leaf
  #CPUID_THERMAL_POWER_MANAGEMENT.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// {Bits 3:0] Number of Interrupt Thresholds in Digital Thermal Sensor.
    ///
    UINT32    InterruptThresholds : 4;
    UINT32    Reserved            : 28;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_THERMAL_POWER_MANAGEMENT_EBX;

/**
  CPUID Thermal and Power Management Information returned in ECX for CPUID leaf
  #CPUID_THERMAL_POWER_MANAGEMENT.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] Hardware Coordination Feedback Capability (Presence of IA32_MPERF
    /// and IA32_APERF). The capability to provide a measure of delivered
    /// processor performance (since last reset of the counters), as a percentage
    /// of the expected processor performance when running at the TSC frequency.
    ///
    UINT32    HardwareCoordinationFeedback : 1;
    UINT32    Reserved1                    : 2;
    ///
    /// [Bit 3] If this bit is set, then the processor supports performance-energy
    /// bias preference and the architectural MSR called IA32_ENERGY_PERF_BIAS
    /// (1B0H).
    ///
    UINT32    PerformanceEnergyBias        : 1;
    UINT32    Reserved2                    : 28;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_THERMAL_POWER_MANAGEMENT_ECX;

/**
  CPUID Structured Extended Feature Flags Enumeration

  @param   EAX  CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS (0x07)
  @param   ECX  CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_SUB_LEAF_INFO (0x00).

  @note
  If ECX contains an invalid sub-leaf index, EAX/EBX/ECX/EDX return 0.  Sub-leaf
  index n is invalid if n exceeds the value that sub-leaf 0 returns in EAX.

  @retval  EAX  The maximum input value for ECX to retrieve sub-leaf information.
  @retval  EBX  Structured Extended Feature Flags described by the type
                CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_EBX.
  @retval  ECX  Structured Extended Feature Flags described by the type
                CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_ECX.
  @retval  EDX  Reserved.

  <b>Example usage</b>
  @code
  UINT32                                       Eax;
  CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_EBX  Ebx;
  CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_ECX  Ecx;
  UINT32                                       SubLeaf;

  AsmCpuidEx (
    CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS,
    CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_SUB_LEAF_INFO,
    &Eax, NULL, NULL, NULL
    );
  for (SubLeaf = 0; SubLeaf <= Eax; SubLeaf++) {
    AsmCpuidEx (
      CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS,
      SubLeaf,
      NULL, &Ebx.Uint32, &Ecx.Uint32, NULL
      );
  }
  @endcode
**/
#define CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS  0x07

///
/// CPUID Structured Extended Feature Flags Enumeration sub-leaf
///
#define CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_SUB_LEAF_INFO  0x00

/**
  CPUID Structured Extended Feature Flags Enumeration in EBX for CPUID leaf
  #CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS sub leaf
  #CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_SUB_LEAF_INFO.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] Supports RDFSBASE/RDGSBASE/WRFSBASE/WRGSBASE if 1.
    ///
    UINT32    FSGSBASE              : 1;
    ///
    /// [Bit 1] IA32_TSC_ADJUST MSR is supported if 1.
    ///
    UINT32    IA32_TSC_ADJUST       : 1;
    ///
    /// [Bit 2] Intel SGX is supported if 1. See section 37.7 "DISCOVERING SUPPORT
    /// FOR INTEL(R) SGX AND ENABLING ENCLAVE INSTRUCTIONS".
    ///
    UINT32    SGX                   : 1;
    ///
    /// [Bit 3] If 1 indicates the processor supports the first group of advanced
    /// bit manipulation extensions (ANDN, BEXTR, BLSI, BLSMSK, BLSR, TZCNT)
    ///
    UINT32    BMI1                  : 1;
    ///
    /// [Bit 4] Hardware Lock Elision
    ///
    UINT32    HLE                   : 1;
    ///
    /// [Bit 5] If 1 indicates the processor supports AVX2 instruction extensions.
    ///
    UINT32    AVX2                  : 1;
    ///
    /// [Bit 6] x87 FPU Data Pointer updated only on x87 exceptions if 1.
    ///
    UINT32    FDP_EXCPTN_ONLY       : 1;
    ///
    /// [Bit 7] Supports Supervisor-Mode Execution Prevention if 1.
    ///
    UINT32    SMEP                  : 1;
    ///
    /// [Bit 8] If 1 indicates the processor supports the second group of
    /// advanced bit manipulation extensions (BZHI, MULX, PDEP, PEXT, RORX,
    /// SARX, SHLX, SHRX)
    ///
    UINT32    BMI2                  : 1;
    ///
    /// [Bit 9] Supports Enhanced REP MOVSB/STOSB if 1.
    ///
    UINT32    EnhancedRepMovsbStosb : 1;
    ///
    /// [Bit 10] If 1, supports INVPCID instruction for system software that
    /// manages process-context identifiers.
    ///
    UINT32    INVPCID               : 1;
    ///
    /// [Bit 11] Restricted Transactional Memory
    ///
    UINT32    RTM                   : 1;
    ///
    /// [Bit 12] Supports Intel(R) Resource Director Technology (Intel(R) RDT)
    /// Monitoring capability if 1.
    ///
    UINT32    RDT_M                 : 1;
    ///
    /// [Bit 13] Deprecates FPU CS and FPU DS values if 1.
    ///
    UINT32    DeprecateFpuCsDs      : 1;
    ///
    /// [Bit 14] Supports Intel(R) Memory Protection Extensions if 1.
    ///
    UINT32    MPX                   : 1;
    ///
    /// [Bit 15] Supports Intel(R) Resource Director Technology (Intel(R) RDT)
    /// Allocation capability if 1.
    ///
    UINT32    RDT_A                 : 1;
    ///
    /// [Bit 16] AVX512F.
    ///
    UINT32    AVX512F               : 1;
    ///
    /// [Bit 17] AVX512DQ.
    ///
    UINT32    AVX512DQ              : 1;
    ///
    /// [Bit 18] If 1 indicates the processor supports the RDSEED instruction.
    ///
    UINT32    RDSEED                : 1;
    ///
    /// [Bit 19] If 1 indicates the processor supports the ADCX and ADOX
    /// instructions.
    ///
    UINT32    ADX                   : 1;
    ///
    /// [Bit 20] Supports Supervisor-Mode Access Prevention (and the CLAC/STAC
    /// instructions) if 1.
    ///
    UINT32    SMAP                  : 1;
    ///
    /// [Bit 21] AVX512_IFMA.
    ///
    UINT32    AVX512_IFMA           : 1;
    UINT32    Reserved6             : 1;
    ///
    /// [Bit 23] If 1 indicates the processor supports the CLFLUSHOPT instruction.
    ///
    UINT32    CLFLUSHOPT            : 1;
    ///
    /// [Bit 24] If 1 indicates the processor supports the CLWB instruction.
    ///
    UINT32    CLWB                  : 1;
    ///
    /// [Bit 25] If 1 indicates the processor supports the Intel Processor Trace
    /// extensions.
    ///
    UINT32    IntelProcessorTrace   : 1;
    ///
    /// [Bit 26] AVX512PF. (Intel Xeon Phi only.).
    ///
    UINT32    AVX512PF              : 1;
    ///
    /// [Bit 27] AVX512ER. (Intel Xeon Phi only.).
    ///
    UINT32    AVX512ER              : 1;
    ///
    /// [Bit 28] AVX512CD.
    ///
    UINT32    AVX512CD              : 1;
    ///
    /// [Bit 29] Supports Intel(R) Secure Hash Algorithm Extensions (Intel(R)
    /// SHA Extensions) if 1.
    ///
    UINT32    SHA                   : 1;
    ///
    /// [Bit 30] AVX512BW.
    ///
    UINT32    AVX512BW              : 1;
    ///
    /// [Bit 31] AVX512VL.
    ///
    UINT32    AVX512VL              : 1;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_EBX;

/**
  CPUID Structured Extended Feature Flags Enumeration in ECX for CPUID leaf
  #CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS sub leaf
  #CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_SUB_LEAF_INFO.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] If 1 indicates the processor supports the PREFETCHWT1 instruction.
    /// (Intel Xeon Phi only.)
    ///
    UINT32    PREFETCHWT1      : 1;
    ///
    /// [Bit 1] AVX512_VBMI.
    ///
    UINT32    AVX512_VBMI      : 1;
    ///
    /// [Bit 2] Supports user-mode instruction prevention if 1.
    ///
    UINT32    UMIP             : 1;
    ///
    /// [Bit 3] Supports protection keys for user-mode pages if 1.
    ///
    UINT32    PKU              : 1;
    ///
    /// [Bit 4] If 1, OS has set CR4.PKE to enable protection keys (and the
    /// RDPKRU/WRPKRU instructions).
    ///
    UINT32    OSPKE            : 1;
    UINT32    Reserved8        : 8;
    ///
    /// [Bit 13] If 1, the following MSRs are supported: IA32_TME_CAPABILITY, IA32_TME_ACTIVATE,
    /// IA32_TME_EXCLUDE_MASK, and IA32_TME_EXCLUDE_BASE.
    ///
    UINT32    TME_EN           : 1;
    ///
    /// [Bits 14] AVX512_VPOPCNTDQ. (Intel Xeon Phi only.).
    ///
    UINT32    AVX512_VPOPCNTDQ : 1;
    UINT32    Reserved7        : 1;
    ///
    /// [Bits 16] Supports 5-level paging if 1.
    ///
    UINT32    FiveLevelPage    : 1;
    ///
    /// [Bits 21:17] The value of MAWAU used by the BNDLDX and BNDSTX instructions
    /// in 64-bit mode.
    ///
    UINT32    MAWAU            : 5;
    ///
    /// [Bit 22] RDPID and IA32_TSC_AUX are available if 1.
    ///
    UINT32    RDPID            : 1;
    UINT32    Reserved3        : 7;
    ///
    /// [Bit 30] Supports SGX Launch Configuration if 1.
    ///
    UINT32    SGX_LC           : 1;
    UINT32    Reserved4        : 1;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_ECX;

/**
  CPUID Structured Extended Feature Flags Enumeration in EDX for CPUID leaf
  #CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS sub leaf
  #CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_SUB_LEAF_INFO.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 1:0] Reserved.
    ///
    UINT32    Reserved1                               : 2;
    ///
    /// [Bit 2] AVX512_4VNNIW. (Intel Xeon Phi only.)
    ///
    UINT32    AVX512_4VNNIW                           : 1;
    ///
    /// [Bit 3] AVX512_4FMAPS. (Intel Xeon Phi only.)
    ///
    UINT32    AVX512_4FMAPS                           : 1;
    ///
    /// [Bit 14:4] Reserved.
    ///
    UINT32    Reserved4                               : 11;
    ///
    /// [Bit 15] Hybrid. If 1, the processor is identified as a hybrid part.
    ///
    UINT32    Hybrid                                  : 1;
    ///
    /// [Bit 25:16] Reserved.
    ///
    UINT32    Reserved5                               : 10;
    ///
    /// [Bit 26] Enumerates support for indirect branch restricted speculation
    /// (IBRS) and the indirect branch pre-dictor barrier (IBPB). Processors
    /// that set this bit support the IA32_SPEC_CTRL MSR and the IA32_PRED_CMD
    /// MSR. They allow software to set IA32_SPEC_CTRL[0] (IBRS) and
    /// IA32_PRED_CMD[0] (IBPB).
    ///
    UINT32    EnumeratesSupportForIBRSAndIBPB         : 1;
    ///
    /// [Bit 27] Enumerates support for single thread indirect branch
    /// predictors (STIBP). Processors that set this bit support the
    /// IA32_SPEC_CTRL MSR. They allow software to set IA32_SPEC_CTRL[1]
    /// (STIBP).
    ///
    UINT32    EnumeratesSupportForSTIBP               : 1;
    ///
    /// [Bit 28] Enumerates support for L1D_FLUSH. Processors that set this bit
    /// support the IA32_FLUSH_CMD MSR. They allow software to set
    /// IA32_FLUSH_CMD[0] (L1D_FLUSH).
    ///
    UINT32    EnumeratesSupportForL1D_FLUSH           : 1;
    ///
    /// [Bit 29] Enumerates support for the IA32_ARCH_CAPABILITIES MSR.
    ///
    UINT32    EnumeratesSupportForCapability          : 1;
    ///
    /// [Bit 30] Enumerates support for the IA32_CORE_CAPABILITIES MSR.
    ///
    UINT32    EnumeratesSupportForCoreCapabilitiesMsr : 1;
    ///
    /// [Bit 31] Enumerates support for Speculative Store Bypass Disable (SSBD).
    /// Processors that set this bit sup-port the IA32_SPEC_CTRL MSR. They allow
    /// software to set IA32_SPEC_CTRL[2] (SSBD).
    ///
    UINT32    EnumeratesSupportForSSBD                : 1;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_EDX;

/**
  CPUID Direct Cache Access Information

  @param   EAX  CPUID_DIRECT_CACHE_ACCESS_INFO (0x09)

  @retval  EAX  Value of bits [31:0] of IA32_PLATFORM_DCA_CAP MSR (address 1F8H).
  @retval  EBX  Reserved.
  @retval  ECX  Reserved.
  @retval  EDX  Reserved.

  <b>Example usage</b>
  @code
  UINT32  Eax;

  AsmCpuid (CPUID_DIRECT_CACHE_ACCESS_INFO, &Eax, NULL, NULL, NULL);
  @endcode
**/
#define CPUID_DIRECT_CACHE_ACCESS_INFO  0x09

/**
  CPUID Architectural Performance Monitoring

  @param   EAX  CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING (0x0A)

  @retval  EAX  Architectural Performance Monitoring information described by
                the type CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING_EAX.
  @retval  EBX  Architectural Performance Monitoring information described by
                the type CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING_EBX.
  @retval  ECX  Reserved.
  @retval  EDX  Architectural Performance Monitoring information described by
                the type CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING_EDX.

  <b>Example usage</b>
  @code
  CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING_EAX  Eax;
  CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING_EBX  Ebx;
  CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING_EDX  Edx;

  AsmCpuid (CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING, &Eax.Uint32, &Ebx.Uint32, NULL, &Edx.Uint32);
  @endcode
**/
#define CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING  0x0A

/**
  CPUID Architectural Performance Monitoring EAX for CPUID leaf
  #CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 7:0] Version ID of architectural performance monitoring.
    ///
    UINT32    ArchPerfMonVerID : 8;
    ///
    /// [Bits 15:8] Number of general-purpose performance monitoring counter
    /// per logical processor.
    ///
    /// IA32_PERFEVTSELx MSRs start at address 186H and occupy a contiguous
    /// block of MSR address space. Each performance event select register is
    /// paired with a corresponding performance counter in the 0C1H address
    /// block.
    ///
    UINT32    PerformanceMonitorCounters : 8;
    ///
    /// [Bits 23:16] Bit width of general-purpose, performance monitoring counter.
    ///
    /// The bit width of an IA32_PMCx MSR. This the number of valid bits for
    /// read operation. On write operations, the lower-order 32 bits of the MSR
    /// may be written with any value, and the high-order bits are sign-extended
    /// from the value of bit 31.
    ///
    UINT32    PerformanceMonitorCounterWidth : 8;
    ///
    /// [Bits 31:24] Length of EBX bit vector to enumerate architectural
    /// performance monitoring events.
    ///
    UINT32    EbxBitVectorLength             : 8;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING_EAX;

/**
  CPUID Architectural Performance Monitoring EBX for CPUID leaf
  #CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] Core cycle event not available if 1.
    ///
    UINT32    UnhaltedCoreCycles         : 1;
    ///
    /// [Bit 1] Instruction retired event not available if 1.
    ///
    UINT32    InstructionsRetired        : 1;
    ///
    /// [Bit 2] Reference cycles event not available if 1.
    ///
    UINT32    UnhaltedReferenceCycles    : 1;
    ///
    /// [Bit 3] Last-level cache reference event not available if 1.
    ///
    UINT32    LastLevelCacheReferences   : 1;
    ///
    /// [Bit 4] Last-level cache misses event not available if 1.
    ///
    UINT32    LastLevelCacheMisses       : 1;
    ///
    /// [Bit 5] Branch instruction retired event not available if 1.
    ///
    UINT32    BranchInstructionsRetired  : 1;
    ///
    /// [Bit 6] Branch mispredict retired event not available if 1.
    ///
    UINT32    AllBranchMispredictRetired : 1;
    UINT32    Reserved                   : 25;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING_EBX;

/**
  CPUID Architectural Performance Monitoring EDX for CPUID leaf
  #CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 4:0] Number of fixed-function performance counters
    /// (if Version ID > 1).
    ///
    UINT32    FixedFunctionPerformanceCounters     : 5;
    ///
    /// [Bits 12:5] Bit width of fixed-function performance counters
    /// (if Version ID > 1).
    ///
    UINT32    FixedFunctionPerformanceCounterWidth : 8;
    UINT32    Reserved1                            : 2;
    ///
    /// [Bits 15] AnyThread deprecation.
    ///
    UINT32    AnyThreadDeprecation                 : 1;
    UINT32    Reserved2                            : 16;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING_EDX;

/**
  CPUID Extended Topology Information

  @note
  CPUID leaf 1FH is a preferred superset to leaf 0BH. Intel recommends first
  checking for the existence of Leaf 1FH before using leaf 0BH.
  Most of Leaf 0BH output depends on the initial value in ECX.  The EDX output
  of leaf 0BH is always valid and does not vary with input value in ECX.  Output
  value in ECX[7:0] always equals input value in ECX[7:0].
  Sub-leaf index 0 enumerates SMT level. Each subsequent higher sub-leaf index
  enumerates a higher-level topological entity in hierarchical order.
  For sub-leaves that return an invalid level-type of 0 in ECX[15:8]; EAX and
  EBX will return 0.
  If an input value n in ECX returns the invalid level-type of 0 in ECX[15:8],
  other input values with ECX > n also return 0 in ECX[15:8].

  @param   EAX  CPUID_EXTENDED_TOPOLOGY (0x0B)
  @param   ECX  Level number

  @retval  EAX  Extended topology information described by the type
                CPUID_EXTENDED_TOPOLOGY_EAX.
  @retval  EBX  Extended topology information described by the type
                CPUID_EXTENDED_TOPOLOGY_EBX.
  @retval  ECX  Extended topology information described by the type
                CPUID_EXTENDED_TOPOLOGY_ECX.
  @retval  EDX  x2APIC ID the current logical processor.

  <b>Example usage</b>
  @code
  CPUID_EXTENDED_TOPOLOGY_EAX  Eax;
  CPUID_EXTENDED_TOPOLOGY_EBX  Ebx;
  CPUID_EXTENDED_TOPOLOGY_ECX  Ecx;
  UINT32                       Edx;
  UINT32                       LevelNumber;

  LevelNumber = 0;
  do {
    AsmCpuidEx (
      CPUID_EXTENDED_TOPOLOGY, LevelNumber,
      &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, &Edx
      );
    LevelNumber++;
  } while (Eax.Bits.ApicIdShift != 0);
  @endcode
**/
#define CPUID_EXTENDED_TOPOLOGY  0x0B

/**
  CPUID Extended Topology Information EAX for CPUID leaf #CPUID_EXTENDED_TOPOLOGY.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 4:0] Number of bits to shift right on x2APIC ID to get a unique
    /// topology ID of the next level type.  All logical processors with the
    /// same next level ID share current level.
    ///
    /// @note
    /// Software should use this field (EAX[4:0]) to enumerate processor
    /// topology of the system.
    ///
    UINT32    ApicIdShift : 5;
    UINT32    Reserved    : 27;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_EXTENDED_TOPOLOGY_EAX;

/**
  CPUID Extended Topology Information EBX for CPUID leaf #CPUID_EXTENDED_TOPOLOGY.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 15:0] Number of logical processors at this level type. The number
    /// reflects configuration as shipped by Intel.
    ///
    /// @note
    /// Software must not use EBX[15:0] to enumerate processor topology of the
    /// system. This value in this field (EBX[15:0]) is only intended for
    /// display/diagnostic purposes. The actual number of logical processors
    /// available to BIOS/OS/Applications may be different from the value of
    /// EBX[15:0], depending on software and platform hardware configurations.
    ///
    UINT32    LogicalProcessors : 16;
    UINT32    Reserved          : 16;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_EXTENDED_TOPOLOGY_EBX;

/**
  CPUID Extended Topology Information ECX for CPUID leaf #CPUID_EXTENDED_TOPOLOGY.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 7:0] Level number. Same value in ECX input.
    ///
    UINT32    LevelNumber : 8;
    ///
    /// [Bits 15:8] Level type.
    ///
    /// @note
    /// The value of the "level type" field is not related to level numbers in
    /// any way, higher "level type" values do not mean higher levels.
    ///
    UINT32    LevelType   : 8;
    UINT32    Reserved    : 16;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_EXTENDED_TOPOLOGY_ECX;

///
/// @{ Define value for CPUID_EXTENDED_TOPOLOGY_ECX.LevelType
///
#define   CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_INVALID  0x00
#define   CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_SMT      0x01
#define   CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_CORE     0x02
///
/// @}
///

/**
  CPUID Extended State Information

  @param   EAX  CPUID_EXTENDED_STATE (0x0D)
  @param   ECX  CPUID_EXTENDED_STATE_MAIN_LEAF (0x00).
                CPUID_EXTENDED_STATE_SUB_LEAF (0x01).
                CPUID_EXTENDED_STATE_SIZE_OFFSET (0x02).
                Sub leafs 2..n based on supported bits in XCR0 or IA32_XSS_MSR.
**/
#define CPUID_EXTENDED_STATE  0x0D

/**
  CPUID Extended State Information Main Leaf

  @param   EAX  CPUID_EXTENDED_STATE (0x0D)
  @param   ECX  CPUID_EXTENDED_STATE_MAIN_LEAF (0x00)

  @retval  EAX  Reports the supported bits of the lower 32 bits of XCR0. XCR0[n]
                can be set to 1 only if EAX[n] is 1.  The format of the extended
                state main leaf is described by the type
                CPUID_EXTENDED_STATE_MAIN_LEAF_EAX.
  @retval  EBX  Maximum size (bytes, from the beginning of the XSAVE/XRSTOR save
                area) required by enabled features in XCR0. May be different than
                ECX if some features at the end of the XSAVE save area are not
                enabled.
  @retval  ECX  Maximum size (bytes, from the beginning of the XSAVE/XRSTOR save
                area) of the XSAVE/XRSTOR save area required by all supported
                features in the processor, i.e., all the valid bit fields in XCR0.
  @retval  EDX  Reports the supported bits of the upper 32 bits of XCR0.
                XCR0[n+32] can be set to 1 only if EDX[n] is 1.

  <b>Example usage</b>
  @code
  CPUID_EXTENDED_STATE_MAIN_LEAF_EAX  Eax;
  UINT32                              Ebx;
  UINT32                              Ecx;
  UINT32                              Edx;

  AsmCpuidEx (
    CPUID_EXTENDED_STATE, CPUID_EXTENDED_STATE_MAIN_LEAF,
    &Eax.Uint32, &Ebx, &Ecx, &Edx
    );
  @endcode
**/
#define CPUID_EXTENDED_STATE_MAIN_LEAF  0x00

/**
  CPUID Extended State Information EAX for CPUID leaf #CPUID_EXTENDED_STATE,
  sub-leaf #CPUID_EXTENDED_STATE_MAIN_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] x87 state.
    ///
    UINT32    x87        : 1;
    ///
    /// [Bit 1] SSE state.
    ///
    UINT32    SSE        : 1;
    ///
    /// [Bit 2] AVX state.
    ///
    UINT32    AVX        : 1;
    ///
    /// [Bits 4:3] MPX state.
    ///
    UINT32    MPX        : 2;
    ///
    /// [Bits 7:5] AVX-512 state.
    ///
    UINT32    AVX_512    : 3;
    ///
    /// [Bit 8] Used for IA32_XSS.
    ///
    UINT32    IA32_XSS   : 1;
    ///
    /// [Bit 9] PKRU state.
    ///
    UINT32    PKRU       : 1;
    UINT32    Reserved1  : 3;
    ///
    /// [Bit 13] Used for IA32_XSS, part 2.
    ///
    UINT32    IA32_XSS_2 : 1;
    UINT32    Reserved2  : 18;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_EXTENDED_STATE_MAIN_LEAF_EAX;

/**
  CPUID Extended State Information Sub Leaf

  @param   EAX  CPUID_EXTENDED_STATE (0x0D)
  @param   ECX  CPUID_EXTENDED_STATE_SUB_LEAF (0x01)

  @retval  EAX  The format of the extended state sub-leaf is described by the
                type CPUID_EXTENDED_STATE_SUB_LEAF_EAX.
  @retval  EBX  The size in bytes of the XSAVE area containing all states
                enabled by XCRO | IA32_XSS.
  @retval  ECX  The format of the extended state sub-leaf is described by the
                type CPUID_EXTENDED_STATE_SUB_LEAF_ECX.
  @retval  EDX  Reports the supported bits of the upper 32 bits of the
                IA32_XSS MSR. IA32_XSS[n+32] can be set to 1 only if EDX[n] is 1.

  <b>Example usage</b>
  @code
  CPUID_EXTENDED_STATE_SUB_LEAF_EAX  Eax;
  UINT32                             Ebx;
  CPUID_EXTENDED_STATE_SUB_LEAF_ECX  Ecx;
  UINT32                             Edx;

  AsmCpuidEx (
    CPUID_EXTENDED_STATE, CPUID_EXTENDED_STATE_SUB_LEAF,
    &Eax.Uint32, &Ebx, &Ecx.Uint32, &Edx
    );
  @endcode
**/
#define CPUID_EXTENDED_STATE_SUB_LEAF  0x01

/**
  CPUID Extended State Information EAX for CPUID leaf #CPUID_EXTENDED_STATE,
  sub-leaf #CPUID_EXTENDED_STATE_SUB_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] XSAVEOPT is available.
    ///
    UINT32    XSAVEOPT : 1;
    ///
    /// [Bit 1] Supports XSAVEC and the compacted form of XRSTOR if set.
    ///
    UINT32    XSAVEC   : 1;
    ///
    /// [Bit 2] Supports XGETBV with ECX = 1 if set.
    ///
    UINT32    XGETBV   : 1;
    ///
    /// [Bit 3] Supports XSAVES/XRSTORS and IA32_XSS if set.
    ///
    UINT32    XSAVES   : 1;
    UINT32    Reserved : 28;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_EXTENDED_STATE_SUB_LEAF_EAX;

/**
  CPUID Extended State Information ECX for CPUID leaf #CPUID_EXTENDED_STATE,
  sub-leaf #CPUID_EXTENDED_STATE_SUB_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 7:0] Used for XCR0.
    ///
    UINT32    XCR0      : 1;
    ///
    /// [Bit 8] PT STate.
    ///
    UINT32    PT        : 1;
    ///
    /// [Bit 9] Used for XCR0.
    ///
    UINT32    XCR0_1    : 1;
    UINT32    Reserved1 : 3;
    ///
    /// [Bit 13] HWP state.
    ///
    UINT32    HWPState  : 1;
    UINT32    Reserved8 : 18;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_EXTENDED_STATE_SUB_LEAF_ECX;

/**
  CPUID Extended State Information Size and Offset Sub Leaf

  @note
  Leaf 0DH output depends on the initial value in ECX.
  Each sub-leaf index (starting at position 2) is supported if it corresponds to
  a supported bit in either the XCR0 register or the IA32_XSS MSR.
  If ECX contains an invalid sub-leaf index, EAX/EBX/ECX/EDX return 0. Sub-leaf
  n (0 <= n <= 31) is invalid if sub-leaf 0 returns 0 in EAX[n] and sub-leaf 1
  returns 0 in ECX[n]. Sub-leaf n (32 <= n <= 63) is invalid if sub-leaf 0
  returns 0 in EDX[n-32] and sub-leaf 1 returns 0 in EDX[n-32].

  @param   EAX  CPUID_EXTENDED_STATE (0x0D)
  @param   ECX  CPUID_EXTENDED_STATE_SIZE_OFFSET (0x02).  Sub leafs 2..n based
                on supported bits in XCR0 or IA32_XSS_MSR.

  @retval  EAX  The size in bytes (from the offset specified in EBX) of the save
                area for an extended state feature associated with a valid
                sub-leaf index, n.
  @retval  EBX  The offset in bytes of this extended state component's save area
                from the beginning of the XSAVE/XRSTOR area.  This field reports
                0 if the sub-leaf index, n, does not map to a valid bit in the
                XCR0 register.
  @retval  ECX  The format of the extended state components's save area as
                described by the type CPUID_EXTENDED_STATE_SIZE_OFFSET_ECX.
                This field reports 0 if the sub-leaf index, n, is invalid.
  @retval  EDX  This field reports 0 if the sub-leaf index, n, is invalid;
                otherwise it is reserved.

  <b>Example usage</b>
  @code
  UINT32                                Eax;
  UINT32                                Ebx;
  CPUID_EXTENDED_STATE_SIZE_OFFSET_ECX  Ecx;
  UINT32                                Edx;
  UINTN                                 SubLeaf;

  for (SubLeaf = CPUID_EXTENDED_STATE_SIZE_OFFSET; SubLeaf < 32; SubLeaf++) {
    AsmCpuidEx (
      CPUID_EXTENDED_STATE, SubLeaf,
      &Eax, &Ebx, &Ecx.Uint32, &Edx
      );
  }
  @endcode
**/
#define CPUID_EXTENDED_STATE_SIZE_OFFSET  0x02

/**
  CPUID Extended State Information ECX for CPUID leaf #CPUID_EXTENDED_STATE,
  sub-leaf #CPUID_EXTENDED_STATE_SIZE_OFFSET.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] Is set if the bit n (corresponding to the sub-leaf index) is
    /// supported in the IA32_XSS MSR; it is clear if bit n is instead supported
    /// in XCR0.
    ///
    UINT32    XSS       : 1;
    ///
    /// [Bit 1] is set if, when the compacted format of an XSAVE area is used,
    /// this extended state component located on the next 64-byte boundary
    /// following the preceding state component (otherwise, it is located
    /// immediately following the preceding state component).
    ///
    UINT32    Compacted : 1;
    UINT32    Reserved  : 30;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_EXTENDED_STATE_SIZE_OFFSET_ECX;

/**
  CPUID Intel Resource Director Technology (Intel RDT) Monitoring Information

  @param   EAX  CPUID_INTEL_RDT_MONITORING (0x0F)
  @param   ECX  CPUID_INTEL_RDT_MONITORING_ENUMERATION_SUB_LEAF (0x00).
                CPUID_INTEL_RDT_MONITORING_L3_CACHE_SUB_LEAF (0x01).

**/
#define CPUID_INTEL_RDT_MONITORING  0x0F

/**
  CPUID Intel Resource Director Technology (Intel RDT) Monitoring Information
  Enumeration Sub-leaf

  @param   EAX  CPUID_INTEL_RDT_MONITORING (0x0F)
  @param   ECX  CPUID_INTEL_RDT_MONITORING_ENUMERATION_SUB_LEAF (0x00)

  @retval  EAX  Reserved.
  @retval  EBX  Maximum range (zero-based) of RMID within this physical
                processor of all types.
  @retval  ECX  Reserved.
  @retval  EDX  L3 Cache Intel RDT Monitoring Information Enumeration described by
                the type CPUID_INTEL_RDT_MONITORING_ENUMERATION_SUB_LEAF_EDX.

  <b>Example usage</b>
  @code
  UINT32                                                  Ebx;
  CPUID_INTEL_RDT_MONITORING_ENUMERATION_SUB_LEAF_EDX     Edx;

  AsmCpuidEx (
    CPUID_INTEL_RDT_MONITORING, CPUID_INTEL_RDT_MONITORING_ENUMERATION_SUB_LEAF,
    NULL, &Ebx, NULL, &Edx.Uint32
    );
  @endcode
**/
#define CPUID_INTEL_RDT_MONITORING_ENUMERATION_SUB_LEAF  0x00

/**
  CPUID Intel RDT Monitoring Information EDX for CPUID leaf
  #CPUID_INTEL_RDT_MONITORING, sub-leaf
  #CPUID_INTEL_RDT_MONITORING_ENUMERATION_SUB_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32    Reserved1    : 1;
    ///
    /// [Bit 1] Supports L3 Cache Intel RDT Monitoring if 1.
    ///
    UINT32    L3CacheRDT_M : 1;
    UINT32    Reserved2    : 30;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_INTEL_RDT_MONITORING_ENUMERATION_SUB_LEAF_EDX;

/**
  CPUID L3 Cache Intel RDT Monitoring Capability Enumeration Sub-leaf

  @param   EAX  CPUID_INTEL_RDT_MONITORING (0x0F)
  @param   ECX  CPUID_INTEL_RDT_MONITORING_L3_CACHE_SUB_LEAF (0x01)

  @retval  EAX  Reserved.
  @retval  EBX  Conversion factor from reported IA32_QM_CTR value to occupancy metric (bytes).
  @retval  ECX  Maximum range (zero-based) of RMID of this resource type.
  @retval  EDX  L3 Cache Intel RDT Monitoring Capability information described by the
                type CPUID_INTEL_RDT_MONITORING_L3_CACHE_SUB_LEAF_EDX.

  <b>Example usage</b>
  @code
  UINT32                                            Ebx;
  UINT32                                            Ecx;
  CPUID_INTEL_RDT_MONITORING_L3_CACHE_SUB_LEAF_EDX  Edx;

  AsmCpuidEx (
    CPUID_INTEL_RDT_MONITORING, CPUID_INTEL_RDT_MONITORING_L3_CACHE_SUB_LEAF,
    NULL, &Ebx, &Ecx, &Edx.Uint32
    );
  @endcode
**/
#define CPUID_INTEL_RDT_MONITORING_L3_CACHE_SUB_LEAF  0x01

/**
  CPUID L3 Cache Intel RDT Monitoring Capability Information EDX for CPUID leaf
  #CPUID_INTEL_RDT_MONITORING, sub-leaf
  #CPUID_INTEL_RDT_MONITORING_L3_CACHE_SUB_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] Supports L3 occupancy monitoring if 1.
    ///
    UINT32    L3CacheOccupancyMonitoring      : 1;
    ///
    /// [Bit 1] Supports L3 Total Bandwidth monitoring if 1.
    ///
    UINT32    L3CacheTotalBandwidthMonitoring : 1;
    ///
    /// [Bit 2] Supports L3 Local Bandwidth monitoring if 1.
    ///
    UINT32    L3CacheLocalBandwidthMonitoring : 1;
    UINT32    Reserved                        : 29;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_INTEL_RDT_MONITORING_L3_CACHE_SUB_LEAF_EDX;

/**
  CPUID Intel Resource Director Technology (Intel RDT) Allocation Information

  @param   EAX  CPUID_INTEL_RDT_ALLOCATION (0x10).
  @param   ECX  CPUID_INTEL_RDT_ALLOCATION_ENUMERATION_SUB_LEAF (0x00).
                CPUID_INTEL_RDT_ALLOCATION_L3_CACHE_SUB_LEAF (0x01).
                CPUID_INTEL_RDT_ALLOCATION_L2_CACHE_SUB_LEAF (0x02).
**/
#define CPUID_INTEL_RDT_ALLOCATION  0x10

/**
  Intel Resource Director Technology (Intel RDT) Allocation Enumeration Sub-leaf

  @param   EAX  CPUID_INTEL_RDT_ALLOCATION (0x10)
  @param   ECX  CPUID_INTEL_RDT_ALLOCATION_ENUMERATION_SUB_LEAF (0x00).

  @retval  EAX  Reserved.
  @retval  EBX  L3 and L2 Cache Allocation Technology information described by
                the type CPUID_INTEL_RDT_ALLOCATION_ENUMERATION_SUB_LEAF_EBX.
  @retval  ECX  Reserved.
  @retval  EDX  Reserved.

  <b>Example usage</b>
  @code
  CPUID_INTEL_RDT_ALLOCATION_ENUMERATION_SUB_LEAF_EBX  Ebx;

  AsmCpuidEx (
    CPUID_INTEL_RDT_ALLOCATION, CPUID_INTEL_RDT_ALLOCATION_ENUMERATION_SUB_LEAF,
    NULL, &Ebx.Uint32, NULL, NULL
    );
  @endcode
**/
#define CPUID_INTEL_RDT_ALLOCATION_ENUMERATION_SUB_LEAF  0x00

/**
  CPUID L3 and L2 Cache Allocation Support Information EBX for CPUID leaf
  #CPUID_INTEL_RDT_ALLOCATION, sub-leaf
  #CPUID_INTEL_RDT_ALLOCATION_ENUMERATION_SUB_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32    Reserved1         : 1;
    ///
    /// [Bit 1] Supports L3 Cache Allocation Technology if 1.
    ///
    UINT32    L3CacheAllocation : 1;
    ///
    /// [Bit 2] Supports L2 Cache Allocation Technology if 1.
    ///
    UINT32    L2CacheAllocation : 1;
    ///
    /// [Bit 3] Supports Memory Bandwidth Allocation if 1.
    ///
    UINT32    MemoryBandwidth   : 1;
    UINT32    Reserved3         : 28;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_INTEL_RDT_ALLOCATION_ENUMERATION_SUB_LEAF_EBX;

/**
  L3 Cache Allocation Technology Enumeration Sub-leaf

  @param   EAX  CPUID_INTEL_RDT_ALLOCATION (0x10)
  @param   ECX  CPUID_INTEL_RDT_ALLOCATION_L3_CACHE_SUB_LEAF (0x01)

  @retval  EAX  RESID L3 Cache Allocation Technology information described by
                the type CPUID_INTEL_RDT_ALLOCATION_L3_CACHE_SUB_LEAF_EAX.
  @retval  EBX  Bit-granular map of isolation/contention of allocation units.
  @retval  ECX  RESID L3 Cache Allocation Technology information described by
                the type CPUID_INTEL_RDT_ALLOCATION_L3_CACHE_SUB_LEAF_ECX.
  @retval  EDX  RESID L3 Cache Allocation Technology information described by
                the type CPUID_INTEL_RDT_ALLOCATION_L3_CACHE_SUB_LEAF_EDX.

  <b>Example usage</b>
  @code
  CPUID_INTEL_RDT_ALLOCATION_L3_CACHE_SUB_LEAF_EAX  Eax;
  UINT32                                            Ebx;
  CPUID_INTEL_RDT_ALLOCATION_L3_CACHE_SUB_LEAF_ECX  Ecx;
  CPUID_INTEL_RDT_ALLOCATION_L3_CACHE_SUB_LEAF_EDX  Edx;

  AsmCpuidEx (
    CPUID_INTEL_RDT_ALLOCATION, CPUID_INTEL_RDT_ALLOCATION_L3_CACHE_SUB_LEAF,
    &Eax.Uint32, &Ebx, &Ecx.Uint32, &Edx.Uint32
    );
  @endcode
**/
#define CPUID_INTEL_RDT_ALLOCATION_L3_CACHE_SUB_LEAF  0x01

/**
  CPUID L3 Cache Allocation Technology Information EAX for CPUID leaf
  #CPUID_INTEL_RDT_ALLOCATION, sub-leaf
  #CPUID_INTEL_RDT_ALLOCATION_L3_CACHE_SUB_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 4:0] Length of the capacity bit mask for the corresponding ResID
    /// using minus-one notation.
    ///
    UINT32    CapacityLength : 5;
    UINT32    Reserved       : 27;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_INTEL_RDT_ALLOCATION_L3_CACHE_SUB_LEAF_EAX;

/**
  CPUID L3 Cache Allocation Technology Information ECX for CPUID leaf
  #CPUID_INTEL_RDT_ALLOCATION, sub-leaf
  #CPUID_INTEL_RDT_ALLOCATION_L3_CACHE_SUB_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32    Reserved3              : 2;
    ///
    /// [Bit 2] Code and Data Prioritization Technology supported if 1.
    ///
    UINT32    CodeDataPrioritization : 1;
    UINT32    Reserved2              : 29;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_INTEL_RDT_ALLOCATION_L3_CACHE_SUB_LEAF_ECX;

/**
  CPUID L3 Cache Allocation Technology Information EDX for CPUID leaf
  #CPUID_INTEL_RDT_ALLOCATION, sub-leaf
  #CPUID_INTEL_RDT_ALLOCATION_L3_CACHE_SUB_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 15:0] Highest COS number supported for this ResID.
    ///
    UINT32    HighestCosNumber : 16;
    UINT32    Reserved         : 16;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_INTEL_RDT_ALLOCATION_L3_CACHE_SUB_LEAF_EDX;

/**
  L2 Cache Allocation Technology Enumeration Sub-leaf

  @param   EAX  CPUID_INTEL_RDT_ALLOCATION (0x10)
  @param   ECX  CPUID_INTEL_RDT_ALLOCATION_L2_CACHE_SUB_LEAF (0x02)

  @retval  EAX  RESID L2 Cache Allocation Technology information described by
                the type CPUID_INTEL_RDT_ALLOCATION_L2_CACHE_SUB_LEAF_EAX.
  @retval  EBX  Bit-granular map of isolation/contention of allocation units.
  @retval  ECX  Reserved.
  @retval  EDX  RESID L2 Cache Allocation Technology information described by
                the type CPUID_INTEL_RDT_ALLOCATION_L2_CACHE_SUB_LEAF_EDX.

  <b>Example usage</b>
  @code
  CPUID_INTEL_RDT_ALLOCATION_L2_CACHE_SUB_LEAF_EAX  Eax;
  UINT32                                            Ebx;
  CPUID_INTEL_RDT_ALLOCATION_L2_CACHE_SUB_LEAF_EDX  Edx;

  AsmCpuidEx (
    CPUID_INTEL_RDT_ALLOCATION, CPUID_INTEL_RDT_ALLOCATION_L2_CACHE_SUB_LEAF,
    &Eax.Uint32, &Ebx, NULL, &Edx.Uint32
    );
  @endcode
**/
#define CPUID_INTEL_RDT_ALLOCATION_L2_CACHE_SUB_LEAF  0x02

/**
  CPUID L2 Cache Allocation Technology Information EAX for CPUID leaf
  #CPUID_INTEL_RDT_ALLOCATION, sub-leaf
  #CPUID_INTEL_RDT_ALLOCATION_L2_CACHE_SUB_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 4:0] Length of the capacity bit mask for the corresponding ResID
    /// using minus-one notation.
    ///
    UINT32    CapacityLength : 5;
    UINT32    Reserved       : 27;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_INTEL_RDT_ALLOCATION_L2_CACHE_SUB_LEAF_EAX;

/**
  CPUID L2 Cache Allocation Technology Information EDX for CPUID leaf
  #CPUID_INTEL_RDT_ALLOCATION, sub-leaf
  #CPUID_INTEL_RDT_ALLOCATION_L2_CACHE_SUB_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 15:0] Highest COS number supported for this ResID.
    ///
    UINT32    HighestCosNumber : 16;
    UINT32    Reserved         : 16;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_INTEL_RDT_ALLOCATION_L2_CACHE_SUB_LEAF_EDX;

/**
  Memory Bandwidth Allocation Enumeration Sub-leaf

  @param   EAX  CPUID_INTEL_RDT_ALLOCATION (0x10)
  @param   ECX  CPUID_INTEL_RDT_ALLOCATION_MEMORY_BANDWIDTH_SUB_LEAF (0x03)

  @retval  EAX  RESID memory bandwidth Allocation Technology information
                described by the type
                CPUID_INTEL_RDT_ALLOCATION_MEMORY_BANDWIDTH_SUB_LEAF_EAX.
  @retval  EBX  Reserved.
  @retval  ECX  RESID memory bandwidth Allocation Technology information
                described by the type
                CPUID_INTEL_RDT_ALLOCATION_MEMORY_BANDWIDTH_SUB_LEAF_ECX.
  @retval  EDX  RESID memory bandwidth Allocation Technology information
                described by the type
                CPUID_INTEL_RDT_ALLOCATION_MEMORY_BANDWIDTH_SUB_LEAF_EDX.

  <b>Example usage</b>
  @code
  CPUID_INTEL_RDT_ALLOCATION_MEMORY_BANDWIDTH_SUB_LEAF_EAX  Eax;
  UINT32                                                    Ebx;
  CPUID_INTEL_RDT_ALLOCATION_MEMORY_BANDWIDTH_SUB_LEAF_ECX  Ecx;
  CPUID_INTEL_RDT_ALLOCATION_MEMORY_BANDWIDTH_SUB_LEAF_EDX  Edx;


  AsmCpuidEx (
    CPUID_INTEL_RDT_ALLOCATION, CPUID_INTEL_RDT_ALLOCATION_MEMORY_BANDWIDTH_SUB_LEAF,
    &Eax.Uint32, &Ebx, NULL, &Edx.Uint32
    );
  @endcode
**/
#define CPUID_INTEL_RDT_ALLOCATION_MEMORY_BANDWIDTH_SUB_LEAF  0x03

/**
  CPUID memory bandwidth Allocation Technology Information EAX for CPUID leaf
  #CPUID_INTEL_RDT_ALLOCATION, sub-leaf
  #CPUID_INTEL_RDT_ALLOCATION_MEMORY_BANDWIDTH_SUB_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 11:0] Reports the maximum MBA throttling value supported for
    /// the corresponding ResID using minus-one notation.
    ///
    UINT32    MaximumMBAThrottling : 12;
    UINT32    Reserved             : 20;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_INTEL_RDT_ALLOCATION_MEMORY_BANDWIDTH_SUB_LEAF_EAX;

/**
  CPUID memory bandwidth Allocation Technology Information ECX for CPUID leaf
  #CPUID_INTEL_RDT_ALLOCATION, sub-leaf
  #CPUID_INTEL_RDT_ALLOCATION_MEMORY_BANDWIDTH_SUB_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 1:0] Reserved.
    ///
    UINT32    Reserved1 : 2;
    ///
    /// [Bits 3] Reports whether the response of the delay values is linear.
    ///
    UINT32    Liner     : 1;
    UINT32    Reserved2 : 29;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_INTEL_RDT_ALLOCATION_MEMORY_BANDWIDTH_SUB_LEAF_ECX;

/**
  CPUID memory bandwidth Allocation Technology Information EDX for CPUID leaf
  #CPUID_INTEL_RDT_ALLOCATION, sub-leaf
  #CPUID_INTEL_RDT_ALLOCATION_MEMORY_BANDWIDTH_SUB_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 15:0] Highest COS number supported for this ResID.
    ///
    UINT32    HighestCosNumber : 16;
    UINT32    Reserved         : 16;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_INTEL_RDT_ALLOCATION_MEMORY_BANDWIDTH_SUB_LEAF_EDX;

/**
  Intel SGX resource capability and configuration.
  See Section 37.7.2 "Intel(R) SGX Resource Enumeration Leaves".

  If CPUID.(EAX=07H, ECX=0H):EBX.SGX = 1, the processor also supports querying
  CPUID with EAX=12H on Intel SGX resource capability and configuration.

  @param   EAX  CPUID_INTEL_SGX (0x12)
  @param   ECX  CPUID_INTEL_SGX_CAPABILITIES_0_SUB_LEAF (0x00).
                CPUID_INTEL_SGX_CAPABILITIES_1_SUB_LEAF (0x01).
                CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF (0x02).
                Sub leafs 2..n based on the sub-leaf-type encoding (returned in EAX[3:0])
                until the sub-leaf type is invalid.

**/
#define CPUID_INTEL_SGX  0x12

/**
  Sub-Leaf 0 Enumeration of Intel SGX Capabilities.
  Enumerates Intel SGX capability, including enclave instruction opcode support.

  @param   EAX  CPUID_INTEL_SGX (0x12)
  @param   ECX  CPUID_INTEL_SGX_CAPABILITIES_0_SUB_LEAF (0x00)

  @retval  EAX  The format of Sub-Leaf 0 Enumeration of Intel SGX Capabilities is
                described by the type CPUID_INTEL_SGX_CAPABILITIES_0_SUB_LEAF_EAX.
  @retval  EBX  MISCSELECT: Reports the bit vector of supported extended features
                that can be written to the MISC region of the SSA.
  @retval  ECX  Reserved.
  @retval  EDX  The format of Sub-Leaf 0 Enumeration of Intel SGX Capabilities is
                described by the type CPUID_INTEL_SGX_CAPABILITIES_0_SUB_LEAF_EDX.

  <b>Example usage</b>
  @code
  CPUID_INTEL_SGX_CAPABILITIES_0_SUB_LEAF_EAX  Eax;
  UINT32                                       Ebx;
  CPUID_INTEL_SGX_CAPABILITIES_0_SUB_LEAF_EDX  Edx;

  AsmCpuidEx (
    CPUID_INTEL_SGX, CPUID_INTEL_SGX_CAPABILITIES_0_SUB_LEAF,
    &Eax.Uint32, &Ebx, NULL, &Edx.Uint32
    );
  @endcode
**/
#define CPUID_INTEL_SGX_CAPABILITIES_0_SUB_LEAF  0x00

/**
  Sub-Leaf 0 Enumeration of Intel SGX Capabilities EAX for CPUID leaf #CPUID_INTEL_SGX,
  sub-leaf #CPUID_INTEL_SGX_CAPABILITIES_0_SUB_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] If 1, indicates leaf functions of SGX1 instruction are supported.
    ///
    UINT32    SGX1      : 1;
    ///
    /// [Bit 1] If 1, indicates leaf functions of SGX2 instruction are supported.
    ///
    UINT32    SGX2      : 1;
    UINT32    Reserved1 : 3;
    ///
    /// [Bit 5] If 1, indicates Intel SGX supports ENCLV instruction leaves
    /// EINCVIRTCHILD, EDECVIRTCHILD, and ESETCONTEXT.
    ///
    UINT32    ENCLV     : 1;
    ///
    /// [Bit 6] If 1, indicates Intel SGX supports ENCLS instruction leaves ETRACKC,
    /// ERDINFO, ELDBC, and ELDUC.
    ///
    UINT32    ENCLS     : 1;
    UINT32    Reserved2 : 25;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_INTEL_SGX_CAPABILITIES_0_SUB_LEAF_EAX;

/**
  Sub-Leaf 0 Enumeration of Intel SGX Capabilities EDX for CPUID leaf #CPUID_INTEL_SGX,
  sub-leaf #CPUID_INTEL_SGX_CAPABILITIES_0_SUB_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 7:0] The maximum supported enclave size is 2^(EDX[7:0]) bytes
    /// when not in 64-bit mode.
    ///
    UINT32    MaxEnclaveSize_Not64 : 8;
    ///
    /// [Bit 15:8] The maximum supported enclave size is 2^(EDX[15:8]) bytes
    /// when operating in 64-bit mode.
    ///
    UINT32    MaxEnclaveSize_64    : 8;
    UINT32    Reserved             : 16;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_INTEL_SGX_CAPABILITIES_0_SUB_LEAF_EDX;

/**
  Sub-Leaf 1 Enumeration of Intel SGX Capabilities.
  Enumerates Intel SGX capability of processor state configuration and enclave
  configuration in the SECS structure.

  @param   EAX  CPUID_INTEL_SGX (0x12)
  @param   ECX  CPUID_INTEL_SGX_CAPABILITIES_1_SUB_LEAF (0x01)

  @retval  EAX  Report the valid bits of SECS.ATTRIBUTES[31:0] that software can
                set with ECREATE. SECS.ATTRIBUTES[n] can be set to 1 using ECREATE
                only if EAX[n] is 1, where n < 32.
  @retval  EBX  Report the valid bits of SECS.ATTRIBUTES[63:32] that software can
                set with ECREATE. SECS.ATTRIBUTES[n+32] can be set to 1 using ECREATE
                only if EBX[n] is 1, where n < 32.
  @retval  ECX  Report the valid bits of SECS.ATTRIBUTES[95:64] that software can
                set with ECREATE. SECS.ATTRIBUTES[n+64] can be set to 1 using ECREATE
                only if ECX[n] is 1, where n < 32.
  @retval  EDX  Report the valid bits of SECS.ATTRIBUTES[127:96] that software can
                set with ECREATE. SECS.ATTRIBUTES[n+96] can be set to 1 using ECREATE
                only if EDX[n] is 1, where n < 32.

  <b>Example usage</b>
  @code
  UINT32  Eax;
  UINT32  Ebx;
  UINT32  Ecx;
  UINT32  Edx;

  AsmCpuidEx (
    CPUID_INTEL_SGX, CPUID_INTEL_SGX_CAPABILITIES_1_SUB_LEAF,
    &Eax, &Ebx, &Ecx, &Edx
    );
  @endcode
**/
#define CPUID_INTEL_SGX_CAPABILITIES_1_SUB_LEAF  0x01

/**
  Sub-Leaf Index 2 or Higher Enumeration of Intel SGX Resources.
  Enumerates available EPC resources.

  @param   EAX  CPUID_INTEL_SGX (0x12)
  @param   ECX  CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF (0x02)

  @retval  EAX  The format of Sub-Leaf Index 2 or Higher Enumeration of Intel SGX
                Resources is described by the type
                CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF_EAX.
  @retval  EBX  The format of Sub-Leaf Index 2 or Higher Enumeration of Intel SGX
                Resources is described by the type
                CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF_EBX.
  @retval  EDX  The format of Sub-Leaf Index 2 or Higher Enumeration of Intel SGX
                Resources is described by the type
                CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF_ECX.
  @retval  EDX  The format of Sub-Leaf Index 2 or Higher Enumeration of Intel SGX
                Resources is described by the type
                CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF_EDX.

  <b>Example usage</b>
  @code
  CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF_EAX  Eax;
  CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF_EBX  Ebx;
  CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF_ECX  Ecx;
  CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF_EDX  Edx;

  AsmCpuidEx (
    CPUID_INTEL_SGX, CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF,
    &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, &Edx.Uint32
    );
  @endcode
**/
#define CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF  0x02

/**
  Sub-Leaf Index 2 or Higher Enumeration of Intel SGX Resources EAX for CPUID
  leaf #CPUID_INTEL_SGX, sub-leaf #CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 3:0] Sub-leaf-type encoding.
    /// 0000b: This sub-leaf is invalid, EBX:EAX and EDX:ECX report 0.
    /// 0001b: This sub-leaf provides information on the Enclave Page Cache (EPC)
    ///        in EBX:EAX and EDX:ECX.
    /// All other encoding are reserved.
    ///
    UINT32    SubLeafType            : 4;
    UINT32    Reserved               : 8;
    ///
    /// [Bit 31:12] If EAX[3:0] = 0001b, these are bits 31:12 of the physical address of
    /// the base of the EPC section.
    ///
    UINT32    LowAddressOfEpcSection : 20;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF_EAX;

/**
  Sub-Leaf Index 2 or Higher Enumeration of Intel SGX Resources EBX for CPUID
  leaf #CPUID_INTEL_SGX, sub-leaf #CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 19:0] If EAX[3:0] = 0001b, these are bits 51:32 of the physical address of
    /// the base of the EPC section.
    ///
    UINT32    HighAddressOfEpcSection : 20;
    UINT32    Reserved                : 12;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF_EBX;

/**
  Sub-Leaf Index 2 or Higher Enumeration of Intel SGX Resources ECX for CPUID
  leaf #CPUID_INTEL_SGX, sub-leaf #CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 3:0] The EPC section encoding.
    /// 0000b: Not valid.
    /// 0001b: The EPC section is confidentiality, integrity and replay protected.
    /// All other encoding are reserved.
    ///
    UINT32    EpcSection          : 4;
    UINT32    Reserved            : 8;
    ///
    /// [Bit 31:12] If EAX[3:0] = 0001b, these are bits 31:12 of the size of the
    /// corresponding EPC section within the Processor Reserved Memory.
    ///
    UINT32    LowSizeOfEpcSection : 20;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF_ECX;

/**
  Sub-Leaf Index 2 or Higher Enumeration of Intel SGX Resources EDX for CPUID
  leaf #CPUID_INTEL_SGX, sub-leaf #CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 19:0] If EAX[3:0] = 0001b, these are bits 51:32 of the size of the
    /// corresponding EPC section within the Processor Reserved Memory.
    ///
    UINT32    HighSizeOfEpcSection : 20;
    UINT32    Reserved             : 12;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_INTEL_SGX_CAPABILITIES_RESOURCES_SUB_LEAF_EDX;

/**
  CPUID Intel Processor Trace Information

  @param   EAX  CPUID_INTEL_PROCESSOR_TRACE (0x14)
  @param   ECX  CPUID_INTEL_PROCESSOR_TRACE_MAIN_LEAF (0x00).
                CPUID_INTEL_PROCESSOR_TRACE_SUB_LEAF (0x01).

**/
#define CPUID_INTEL_PROCESSOR_TRACE  0x14

/**
  CPUID Intel Processor Trace Information Main Leaf

  @param   EAX  CPUID_INTEL_PROCEDSSOR_TRACE (0x14)
  @param   ECX  CPUID_INTEL_PROCEDSSOR_TRACE_MAIN_LEAF (0x00)

  @retval  EAX  Reports the maximum sub-leaf supported in leaf 14H.
  @retval  EBX  Returns Intel processor trace information described by the
                type CPUID_INTEL_PROCESSOR_TRACE_MAIN_LEAF_EBX.
  @retval  ECX  Returns Intel processor trace information described by the
                type CPUID_INTEL_PROCESSOR_TRACE_MAIN_LEAF_ECX.
  @retval  EDX  Reserved.

  <b>Example usage</b>
  @code
  UINT32                                     Eax;
  CPUID_INTEL_PROCESSOR_TRACE_MAIN_LEAF_EBX  Ebx;
  CPUID_INTEL_PROCESSOR_TRACE_MAIN_LEAF_ECX  Ecx;

  AsmCpuidEx (
    CPUID_INTEL_PROCESSOR_TRACE, CPUID_INTEL_PROCESSOR_TRACE_MAIN_LEAF,
    &Eax, &Ebx.Uint32, &Ecx.Uint32, NULL
    );
  @endcode
**/
#define CPUID_INTEL_PROCESSOR_TRACE_MAIN_LEAF  0x00

/**
  CPUID Intel Processor Trace EBX for CPUID leaf #CPUID_INTEL_PROCESSOR_TRACE,
  sub-leaf #CPUID_INTEL_PROCESSOR_TRACE_MAIN_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] If 1, indicates that IA32_RTIT_CTL.CR3Filter can be set to 1,
    /// and that IA32_RTIT_CR3_MATCH MSR can be accessed.
    ///
    UINT32    Cr3Filter            : 1;
    ///
    /// [Bit 1] If 1, indicates support of Configurable PSB and Cycle-Accurate
    /// Mode.
    ///
    UINT32    ConfigurablePsb      : 1;
    ///
    /// [Bit 2] If 1, indicates support of IP Filtering, TraceStop filtering,
    /// and preservation of Intel PT MSRs across warm reset.
    ///
    UINT32    IpTraceStopFiltering : 1;
    ///
    /// [Bit 3] If 1, indicates support of MTC timing packet and suppression of
    /// COFI-based packets.
    ///
    UINT32    Mtc                  : 1;
    ///
    /// [Bit 4] If 1, indicates support of PTWRITE. Writes can set
    /// IA32_RTIT_CTL[12] (PTWEn) and IA32_RTIT_CTL[5] (FUPonPTW), and PTWRITE
    /// can generate packets.
    ///
    UINT32    PTWrite              : 1;
    ///
    /// [Bit 5] If 1, indicates support of Power Event Trace. Writes can set
    /// IA32_RTIT_CTL[4] (PwrEvtEn), enabling Power Event Trace packet
    /// generation.
    ///
    UINT32    PowerEventTrace      : 1;
    UINT32    Reserved             : 26;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_INTEL_PROCESSOR_TRACE_MAIN_LEAF_EBX;

/**
  CPUID Intel Processor Trace ECX for CPUID leaf #CPUID_INTEL_PROCESSOR_TRACE,
  sub-leaf #CPUID_INTEL_PROCESSOR_TRACE_MAIN_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] If 1, Tracing can be enabled with IA32_RTIT_CTL.ToPA = 1, hence
    /// utilizing the ToPA output scheme; IA32_RTIT_OUTPUT_BASE and
    /// IA32_RTIT_OUTPUT_MASK_PTRS MSRs can be accessed.
    ///
    UINT32    RTIT                    : 1;
    ///
    /// [Bit 1] If 1, ToPA tables can hold any number of output entries, up to
    /// the maximum allowed by the MaskOrTableOffset field of
    /// IA32_RTIT_OUTPUT_MASK_PTRS.
    ///
    UINT32    ToPA                    : 1;
    ///
    /// [Bit 2] If 1, indicates support of Single-Range Output scheme.
    ///
    UINT32    SingleRangeOutput       : 1;
    ///
    /// [Bit 3] If 1, indicates support of output to Trace Transport subsystem.
    ///
    UINT32    TraceTransportSubsystem : 1;
    UINT32    Reserved                : 27;
    ///
    /// [Bit 31] If 1, generated packets which contain IP payloads have LIP
    /// values, which include the CS base component.
    ///
    UINT32    LIP                     : 1;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_INTEL_PROCESSOR_TRACE_MAIN_LEAF_ECX;

/**
  CPUID Intel Processor Trace Information Sub-leaf

  @param   EAX  CPUID_INTEL_PROCEDSSOR_TRACE (0x14)
  @param   ECX  CPUID_INTEL_PROCESSOR_TRACE_SUB_LEAF (0x01)

  @retval  EAX  Returns Intel processor trace information described by the
                type CPUID_INTEL_PROCESSOR_TRACE_SUB_LEAF_EAX.
  @retval  EBX  Returns Intel processor trace information described by the
                type CPUID_INTEL_PROCESSOR_TRACE_SUB_LEAF_EBX.
  @retval  ECX  Reserved.
  @retval  EDX  Reserved.

  <b>Example usage</b>
  @code
  UINT32                                    MaximumSubLeaf;
  UINT32                                    SubLeaf;
  CPUID_INTEL_PROCESSOR_TRACE_SUB_LEAF_EAX  Eax;
  CPUID_INTEL_PROCESSOR_TRACE_SUB_LEAF_EBX  Ebx;

  AsmCpuidEx (
    CPUID_INTEL_PROCESSOR_TRACE, CPUID_INTEL_PROCESSOR_TRACE_MAIN_LEAF,
    &MaximumSubLeaf, NULL, NULL, NULL
    );

  for (SubLeaf = CPUID_INTEL_PROCESSOR_TRACE_SUB_LEAF; SubLeaf <= MaximumSubLeaf; SubLeaf++) {
    AsmCpuidEx (
      CPUID_INTEL_PROCESSOR_TRACE, SubLeaf,
      &Eax.Uint32, &Ebx.Uint32, NULL, NULL
      );
  }
  @endcode
**/
#define CPUID_INTEL_PROCESSOR_TRACE_SUB_LEAF  0x01

/**
  CPUID Intel Processor Trace EAX for CPUID leaf #CPUID_INTEL_PROCESSOR_TRACE,
  sub-leaf #CPUID_INTEL_PROCESSOR_TRACE_SUB_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 2:0] Number of configurable Address Ranges for filtering.
    ///
    UINT32    ConfigurableAddressRanges : 3;
    UINT32    Reserved                  : 13;
    ///
    /// [Bits 31:16] Bitmap of supported MTC period encodings
    ///
    UINT32    MtcPeriodEncodings        : 16;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_INTEL_PROCESSOR_TRACE_SUB_LEAF_EAX;

/**
  CPUID Intel Processor Trace EBX for CPUID leaf #CPUID_INTEL_PROCESSOR_TRACE,
  sub-leaf #CPUID_INTEL_PROCESSOR_TRACE_SUB_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 15:0] Bitmap of supported Cycle Threshold value encodings.
    ///
    UINT32    CycleThresholdEncodings : 16;
    ///
    /// [Bits 31:16] Bitmap of supported Configurable PSB frequency encodings.
    ///
    UINT32    PsbFrequencyEncodings   : 16;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_INTEL_PROCESSOR_TRACE_SUB_LEAF_EBX;

/**
  CPUID Time Stamp Counter and Nominal Core Crystal Clock Information

  @note
  If EBX[31:0] is 0, the TSC/"core crystal clock" ratio is not enumerated.
  EBX[31:0]/EAX[31:0] indicates the ratio of the TSC frequency and the core
  crystal clock frequency.
  If ECX is 0, the nominal core crystal clock frequency is not enumerated.
  "TSC frequency" = "core crystal clock frequency" * EBX/EAX.
  The core crystal clock may differ from the reference clock, bus clock, or core
  clock frequencies.

  @param   EAX  CPUID_TIME_STAMP_COUNTER (0x15)

  @retval  EAX  An unsigned integer which is the denominator of the
                TSC/"core crystal clock" ratio
  @retval  EBX  An unsigned integer which is the numerator of the
                TSC/"core crystal clock" ratio.
  @retval  ECX  An unsigned integer which is the nominal frequency
                of the core crystal clock in Hz.
  @retval  EDX  Reserved.

  <b>Example usage</b>
  @code
  UINT32  Eax;
  UINT32  Ebx;
  UINT32  Ecx;

  AsmCpuid (CPUID_TIME_STAMP_COUNTER, &Eax, &Ebx, &Ecx, NULL);
  @endcode
**/
#define CPUID_TIME_STAMP_COUNTER  0x15

/**
  CPUID Processor Frequency Information

  @note
  Data is returned from this interface in accordance with the processor's
  specification and does not reflect actual values. Suitable use of this data
  includes the display of processor information in like manner to the processor
  brand string and for determining the appropriate range to use when displaying
  processor information e.g. frequency history graphs. The returned information
  should not be used for any other purpose as the returned information does not
  accurately correlate to information / counters returned by other processor
  interfaces.  While a processor may support the Processor Frequency Information
  leaf, fields that return a value of zero are not supported.

  @param   EAX  CPUID_TIME_STAMP_COUNTER (0x16)

  @retval  EAX  Returns processor base frequency information described by the
                type CPUID_PROCESSOR_FREQUENCY_EAX.
  @retval  EBX  Returns maximum frequency information described by the type
                CPUID_PROCESSOR_FREQUENCY_EBX.
  @retval  ECX  Returns bus frequency information described by the type
                CPUID_PROCESSOR_FREQUENCY_ECX.
  @retval  EDX  Reserved.

  <b>Example usage</b>
  @code
  CPUID_PROCESSOR_FREQUENCY_EAX  Eax;
  CPUID_PROCESSOR_FREQUENCY_EBX  Ebx;
  CPUID_PROCESSOR_FREQUENCY_ECX  Ecx;

  AsmCpuid (CPUID_PROCESSOR_FREQUENCY, &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, NULL);
  @endcode
**/
#define CPUID_PROCESSOR_FREQUENCY  0x16

/**
  CPUID Processor Frequency Information EAX for CPUID leaf
  #CPUID_PROCESSOR_FREQUENCY.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 15:0] Processor Base Frequency (in MHz).
    ///
    UINT32    ProcessorBaseFrequency : 16;
    UINT32    Reserved               : 16;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_PROCESSOR_FREQUENCY_EAX;

/**
  CPUID Processor Frequency Information EBX for CPUID leaf
  #CPUID_PROCESSOR_FREQUENCY.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 15:0] Maximum Frequency (in MHz).
    ///
    UINT32    MaximumFrequency : 16;
    UINT32    Reserved         : 16;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_PROCESSOR_FREQUENCY_EBX;

/**
  CPUID Processor Frequency Information ECX for CPUID leaf
  #CPUID_PROCESSOR_FREQUENCY.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 15:0] Bus (Reference) Frequency (in MHz).
    ///
    UINT32    BusFrequency : 16;
    UINT32    Reserved     : 16;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_PROCESSOR_FREQUENCY_ECX;

/**
  CPUID SoC Vendor Information

  @param   EAX  CPUID_SOC_VENDOR (0x17)
  @param   ECX  CPUID_SOC_VENDOR_MAIN_LEAF (0x00)
                CPUID_SOC_VENDOR_BRAND_STRING1 (0x01)
                CPUID_SOC_VENDOR_BRAND_STRING1 (0x02)
                CPUID_SOC_VENDOR_BRAND_STRING1 (0x03)

  @note
  Leaf 17H output depends on the initial value in ECX.  SOC Vendor Brand String
  is a UTF-8 encoded string padded with trailing bytes of 00H.  The complete SOC
  Vendor Brand String is constructed by concatenating in ascending order of
  EAX:EBX:ECX:EDX and from the sub-leaf 1 fragment towards sub-leaf 3.

**/
#define CPUID_SOC_VENDOR  0x17

/**
  CPUID SoC Vendor Information

  @param   EAX  CPUID_SOC_VENDOR (0x17)
  @param   ECX  CPUID_SOC_VENDOR_MAIN_LEAF (0x00)

  @retval  EAX  MaxSOCID_Index. Reports the maximum input value of supported
                sub-leaf in leaf 17H.
  @retval  EBX  Returns SoC Vendor information described by the type
                CPUID_SOC_VENDOR_MAIN_LEAF_EBX.
  @retval  ECX  Project ID. A unique number an SOC vendor assigns to its SOC
                projects.
  @retval  EDX  Stepping ID. A unique number within an SOC project that an SOC
                vendor assigns.

  <b>Example usage</b>
  @code
  UINT32                          Eax;
  CPUID_SOC_VENDOR_MAIN_LEAF_EBX  Ebx;
  UINT32                          Ecx;
  UINT32                          Edx;

  AsmCpuidEx (
    CPUID_SOC_VENDOR, CPUID_SOC_VENDOR_MAIN_LEAF,
    &Eax, &Ebx.Uint32, &Ecx, &Edx
    );
  @endcode
**/
#define CPUID_SOC_VENDOR_MAIN_LEAF  0x00

/**
  CPUID SoC Vendor Information EBX for CPUID leaf #CPUID_SOC_VENDOR sub-leaf
  #CPUID_SOC_VENDOR_MAIN_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 15:0] SOC Vendor ID.
    ///
    UINT32    SocVendorId    : 16;
    ///
    /// [Bit 16] If 1, the SOC Vendor ID field is assigned via an industry
    /// standard enumeration scheme. Otherwise, the SOC Vendor ID field is
    /// assigned by Intel.
    ///
    UINT32    IsVendorScheme : 1;
    UINT32    Reserved       : 15;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_SOC_VENDOR_MAIN_LEAF_EBX;

/**
  CPUID SoC Vendor Information

  @param   EAX  CPUID_SOC_VENDOR (0x17)
  @param   ECX  CPUID_SOC_VENDOR_BRAND_STRING1 (0x01)

  @retval  EAX  SOC Vendor Brand String. UTF-8 encoded string of type
                CPUID_SOC_VENDOR_BRAND_STRING_DATA.
  @retval  EBX  SOC Vendor Brand String. UTF-8 encoded string of type
                CPUID_SOC_VENDOR_BRAND_STRING_DATA.
  @retval  ECX  SOC Vendor Brand String. UTF-8 encoded string of type
                CPUID_SOC_VENDOR_BRAND_STRING_DATA.
  @retval  EDX  SOC Vendor Brand String. UTF-8 encoded string of type
                CPUID_SOC_VENDOR_BRAND_STRING_DATA.

  <b>Example usage</b>
  @code
  CPUID_SOC_VENDOR_BRAND_STRING_DATA  Eax;
  CPUID_SOC_VENDOR_BRAND_STRING_DATA  Ebx;
  CPUID_SOC_VENDOR_BRAND_STRING_DATA  Ecx;
  CPUID_SOC_VENDOR_BRAND_STRING_DATA  Edx;

  AsmCpuidEx (
    CPUID_SOC_VENDOR, CPUID_SOC_VENDOR_BRAND_STRING1,
    &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, &Edx.Uint32
    );
  @endcode
**/
#define CPUID_SOC_VENDOR_BRAND_STRING1  0x01

/**
  CPUID SoC Vendor Brand String for CPUID leafs #CPUID_SOC_VENDOR_BRAND_STRING1,
  #CPUID_SOC_VENDOR_BRAND_STRING2, and #CPUID_SOC_VENDOR_BRAND_STRING3.
**/
typedef union {
  ///
  /// 4 UTF-8 characters of Soc Vendor Brand String
  ///
  CHAR8     BrandString[4];
  ///
  /// All fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_SOC_VENDOR_BRAND_STRING_DATA;

/**
  CPUID SoC Vendor Information

  @param   EAX  CPUID_SOC_VENDOR (0x17)
  @param   ECX  CPUID_SOC_VENDOR_BRAND_STRING2 (0x02)

  @retval  EAX  SOC Vendor Brand String. UTF-8 encoded string of type
                CPUID_SOC_VENDOR_BRAND_STRING_DATA.
  @retval  EBX  SOC Vendor Brand String. UTF-8 encoded string of type
                CPUID_SOC_VENDOR_BRAND_STRING_DATA.
  @retval  ECX  SOC Vendor Brand String. UTF-8 encoded string of type
                CPUID_SOC_VENDOR_BRAND_STRING_DATA.
  @retval  EDX  SOC Vendor Brand String. UTF-8 encoded string of type
                CPUID_SOC_VENDOR_BRAND_STRING_DATA.

  <b>Example usage</b>
  @code
  CPUID_SOC_VENDOR_BRAND_STRING_DATA  Eax;
  CPUID_SOC_VENDOR_BRAND_STRING_DATA  Ebx;
  CPUID_SOC_VENDOR_BRAND_STRING_DATA  Ecx;
  CPUID_SOC_VENDOR_BRAND_STRING_DATA  Edx;

  AsmCpuidEx (
    CPUID_SOC_VENDOR, CPUID_SOC_VENDOR_BRAND_STRING2,
    &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, &Edx.Uint32
    );
  @endcode
**/
#define CPUID_SOC_VENDOR_BRAND_STRING2  0x02

/**
  CPUID SoC Vendor Information

  @param   EAX  CPUID_SOC_VENDOR (0x17)
  @param   ECX  CPUID_SOC_VENDOR_BRAND_STRING3 (0x03)

  @retval  EAX  SOC Vendor Brand String. UTF-8 encoded string of type
                CPUID_SOC_VENDOR_BRAND_STRING_DATA.
  @retval  EBX  SOC Vendor Brand String. UTF-8 encoded string of type
                CPUID_SOC_VENDOR_BRAND_STRING_DATA.
  @retval  ECX  SOC Vendor Brand String. UTF-8 encoded string of type
                CPUID_SOC_VENDOR_BRAND_STRING_DATA.
  @retval  EDX  SOC Vendor Brand String. UTF-8 encoded string of type
                CPUID_SOC_VENDOR_BRAND_STRING_DATA.

  <b>Example usage</b>
  @code
  CPUID_SOC_VENDOR_BRAND_STRING_DATA  Eax;
  CPUID_SOC_VENDOR_BRAND_STRING_DATA  Ebx;
  CPUID_SOC_VENDOR_BRAND_STRING_DATA  Ecx;
  CPUID_SOC_VENDOR_BRAND_STRING_DATA  Edx;

  AsmCpuidEx (
    CPUID_SOC_VENDOR, CPUID_SOC_VENDOR_BRAND_STRING3,
    &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, &Edx.Uint32
    );
  @endcode
**/
#define CPUID_SOC_VENDOR_BRAND_STRING3  0x03

/**
  CPUID Deterministic Address Translation Parameters

  @note
  Each sub-leaf enumerates a different address translation structure.
  If ECX contains an invalid sub-leaf index, EAX/EBX/ECX/EDX return 0. Sub-leaf
  index n is invalid if n exceeds the value that sub-leaf 0 returns in EAX. A
  sub-leaf index is also invalid if EDX[4:0] returns 0.
  Valid sub-leaves do not need to be contiguous or in any particular order. A
  valid sub-leaf may be in a higher input ECX value than an invalid sub-leaf or
  than a valid sub-leaf of a higher or lower-level structure.
  * Some unified TLBs will allow a single TLB entry to satisfy data read/write
  and instruction fetches. Others will require separate entries (e.g., one
  loaded on data read/write and another loaded on an instruction fetch).
  Please see the Intel 64 and IA-32 Architectures Optimization Reference Manual
  for details of a particular product.
  ** Add one to the return value to get the result.

  @param   EAX  CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS (0x18)
  @param   ECX  CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS_MAIN_LEAF (0x00)
                CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS_SUB_LEAF  (0x*)

**/
#define CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS  0x18

/**
  CPUID Deterministic Address Translation Parameters

  @param   EAX  CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS (0x18)
  @param   ECX  CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS_MAIN_LEAF (0x00)

  @retval  EAX  Reports the maximum input value of supported sub-leaf in leaf 18H.
  @retval  EBX  Returns Deterministic Address Translation Parameters described by
                the type CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS_EBX.
  @retval  ECX  Number of Sets.
  @retval  EDX  Returns Deterministic Address Translation Parameters described by
                the type CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS_EDX.

  <b>Example usage</b>
  @code
  UINT32                                                  Eax;
  CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS_EBX  Ebx;
  UINT32                                                  Ecx;
  CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS_EDX  Edx;

  AsmCpuidEx (
    CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS,
    CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS_MAIN_LEAF,
    &Eax, &Ebx.Uint32, &Ecx, &Edx.Uint32
    );
  @endcode
**/
#define CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS_MAIN_LEAF  0x00

/**
  CPUID Deterministic Address Translation Parameters EBX for CPUID leafs.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 0] 4K page size entries supported by this structure.
    ///
    UINT32    Page4K       : 1;
    ///
    /// [Bits 1] 2MB page size entries supported by this structure.
    ///
    UINT32    Page2M       : 1;
    ///
    /// [Bits 2] 4MB page size entries supported by this structure.
    ///
    UINT32    Page4M       : 1;
    ///
    /// [Bits 3] 1 GB page size entries supported by this structure.
    ///
    UINT32    Page1G       : 1;
    ///
    /// [Bits 7:4] Reserved.
    ///
    UINT32    Reserved1    : 4;
    ///
    /// [Bits 10:8] Partitioning (0: Soft partitioning between the logical
    /// processors sharing this structure)
    ///
    UINT32    Partitioning : 3;
    ///
    /// [Bits 15:11] Reserved.
    ///
    UINT32    Reserved2    : 5;
    ///
    /// [Bits 31:16] W = Ways of associativity.
    ///
    UINT32    Way          : 16;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS_EBX;

/**
  CPUID Deterministic Address Translation Parameters EDX for CPUID leafs.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 4:0] Translation cache type field.
    ///
    UINT32    TranslationCacheType  : 5;
    ///
    /// [Bits 7:5] Translation cache level (starts at 1).
    ///
    UINT32    TranslationCacheLevel : 3;
    ///
    /// [Bits 8] Fully associative structure.
    ///
    UINT32    FullyAssociative      : 1;
    ///
    /// [Bits 13:9] Reserved.
    ///
    UINT32    Reserved1             : 5;
    ///
    /// [Bits 25:14] Maximum number of addressable IDs for logical
    /// processors sharing this translation cache.
    ///
    UINT32    MaximumNum            : 12;
    ///
    /// [Bits 31:26] Reserved.
    ///
    UINT32    Reserved2             : 6;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS_EDX;

///
/// @{ Define value for CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS_EDX.TranslationCacheType
///
#define   CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS_TRANSLATION_CACHE_TYPE_INVALID          0x00
#define   CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS_TRANSLATION_CACHE_TYPE_DATA_TLB         0x01
#define   CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS_TRANSLATION_CACHE_TYPE_INSTRUCTION_TLB  0x02
#define   CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS_TRANSLATION_CACHE_TYPE_UNIFIED_TLB      0x03
///
/// @}
///

/**
  CPUID Hybrid Information Enumeration Leaf

  @param   EAX  CPUID_HYBRID_INFORMATION (0x1A)
  @param   ECX  CPUID_HYBRID_INFORMATION_MAIN_LEAF (0x00).

  @retval  EAX  Enumerates the native model ID and core type described
                by the type CPUID_NATIVE_MODEL_ID_AND_CORE_TYPE_EAX
  @retval  EBX  Reserved.
  @retval  ECX  Reserved.
  @retval  EDX  Reserved.

  <b>Example usage</b>
  @code
  CPUID_NATIVE_MODEL_ID_AND_CORE_TYPE_EAX          Eax;

  AsmCpuidEx (
    CPUID_HYBRID_INFORMATION,
    CPUID_HYBRID_INFORMATION_MAIN_LEAF,
    &Eax, NULL, NULL, NULL
    );
  @endcode

**/
#define CPUID_HYBRID_INFORMATION  0x1A

///
/// CPUID Hybrid Information Enumeration main leaf
///
#define CPUID_HYBRID_INFORMATION_MAIN_LEAF  0x00

/**
  CPUID Hybrid Information EAX for CPUID leaf #CPUID_HYBRID_INFORMATION,
  main leaf #CPUID_HYBRID_INFORMATION_MAIN_LEAF.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 23:0] Native model ID of the core.
    ///
    /// The core-type and native mode ID can be used to uniquely identify
    /// the microarchitecture of the core.This native model ID is not unique
    /// across core types, and not related to the model ID reported in CPUID
    /// leaf 01H, and does not identify the SOC.
    ///
    UINT32    NativeModelId : 24;
    ///
    /// [Bit 31:24] Core type
    ///
    UINT32    CoreType      : 8;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_NATIVE_MODEL_ID_AND_CORE_TYPE_EAX;

///
/// @{ Define value for CPUID_NATIVE_MODEL_ID_AND_CORE_TYPE_EAX.CoreType
///
#define   CPUID_CORE_TYPE_INTEL_ATOM  0x20
#define   CPUID_CORE_TYPE_INTEL_CORE  0x40
///
/// @}
///

/**
  CPUID V2 Extended Topology Enumeration Leaf

  @note
  CPUID leaf 1FH is a preferred superset to leaf 0BH. Intel recommends first checking
  for the existence of Leaf 1FH and using this if available.
  Most of Leaf 1FH output depends on the initial value in ECX. The EDX output of leaf
  1FH is always valid and does not vary with input value in ECX. Output value in ECX[7:0]
  always equals input value in ECX[7:0]. Sub-leaf index 0 enumerates SMT level. Each
  subsequent higher sub-leaf index enumerates a higher-level topological entity in
  hierarchical order. For sub-leaves that return an invalid level-type of 0 in ECX[15:8];
  EAX and EBX will return 0. If an input value n in ECX returns the invalid level-type of
  0 in ECX[15:8], other input values with ECX > n also return 0 in ECX[15:8].

  Software should use this field (EAX[4:0]) to enumerate processor topology of the system.
  Software must not use EBX[15:0] to enumerate processor topology of the system. This value
  in this field (EBX[15:0]) is only intended for display/diagnostic purposes. The actual
  number of logical processors available to BIOS/OS/Applications may be different from the
  value of EBX[15:0], depending on software and platform hardware configurations.

  @param   EAX  CPUID_V2_EXTENDED_TOPOLOGY                        (0x1F)
  @param   ECX  Level number

**/
#define CPUID_V2_EXTENDED_TOPOLOGY  0x1F

///
/// @{ Define value for CPUID_EXTENDED_TOPOLOGY_ECX.LevelType
/// The value of the "level type" field is not related to level numbers in
/// any way, higher "level type" values do not mean higher levels.
///
#define   CPUID_V2_EXTENDED_TOPOLOGY_LEVEL_TYPE_MODULE  0x03
#define   CPUID_V2_EXTENDED_TOPOLOGY_LEVEL_TYPE_TILE    0x04
#define   CPUID_V2_EXTENDED_TOPOLOGY_LEVEL_TYPE_DIE     0x05
///
/// @}
///

/**
  CPUID Guest TD Run Time Environment Enumeration Leaf

  @note
  Guest software can be designed to run either as a TD, as a legacy virtual machine,
  or directly on the CPU, based on enumeration of its run-time environment.
  CPUID leaf 21H emulation is done by the Intel TDX module. Sub-leaf 0 returns the values
  shown below. Other sub-leaves return 0 in EAX/EBX/ECX/EDX.
    EAX: 0x00000000
    EBX: 0x65746E49 "Inte"
    ECX: 0x20202020 "    "
    EDX: 0x5844546C "lTDX"

  @param   EAX  CPUID_GUESTTD_RUNTIME_ENVIRONMENT                        (0x21)
  @param   ECX  Level number

**/
#define CPUID_GUESTTD_RUNTIME_ENVIRONMENT  0x21

///
/// @{ CPUID Guest TD signature values returned by Intel processors
///
#define CPUID_GUESTTD_SIGNATURE_GENUINE_INTEL_EBX  SIGNATURE_32 ('I', 'n', 't', 'e')
#define CPUID_GUESTTD_SIGNATURE_GENUINE_INTEL_ECX  SIGNATURE_32 (' ', ' ', ' ', ' ')
#define CPUID_GUESTTD_SIGNATURE_GENUINE_INTEL_EDX  SIGNATURE_32 ('l', 'T', 'D', 'X')
///
/// @}
///

/**
  CPUID Extended Function

  @param   EAX  CPUID_EXTENDED_FUNCTION (0x80000000)

  @retval  EAX  Maximum Input Value for Extended Function CPUID Information.
  @retval  EBX  Reserved.
  @retval  ECX  Reserved.
  @retval  EDX  Reserved.

  <b>Example usage</b>
  @code
  UINT32  Eax;

  AsmCpuid (CPUID_EXTENDED_FUNCTION, &Eax, NULL, NULL, NULL);
  @endcode
**/
#define CPUID_EXTENDED_FUNCTION  0x80000000

/**
  CPUID Extended Processor Signature and Feature Bits

  @param   EAX  CPUID_EXTENDED_CPU_SIG (0x80000001)

  @retval  EAX  CPUID_EXTENDED_CPU_SIG.
  @retval  EBX  Reserved.
  @retval  ECX  Extended Processor Signature and Feature Bits information
                described by the type CPUID_EXTENDED_CPU_SIG_ECX.
  @retval  EDX  Extended Processor Signature and Feature Bits information
                described by the type CPUID_EXTENDED_CPU_SIG_EDX.

  <b>Example usage</b>
  @code
  UINT32                      Eax;
  CPUID_EXTENDED_CPU_SIG_ECX  Ecx;
  CPUID_EXTENDED_CPU_SIG_EDX  Edx;

  AsmCpuid (CPUID_EXTENDED_CPU_SIG, &Eax, NULL, &Ecx.Uint32, &Edx.Uint32);
  @endcode
**/
#define CPUID_EXTENDED_CPU_SIG  0x80000001

/**
  CPUID Extended Processor Signature and Feature Bits ECX for CPUID leaf
  #CPUID_EXTENDED_CPU_SIG.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] LAHF/SAHF available in 64-bit mode.
    ///
    UINT32    LAHF_SAHF : 1;
    UINT32    Reserved1 : 4;
    ///
    /// [Bit 5] LZCNT.
    ///
    UINT32    LZCNT     : 1;
    UINT32    Reserved2 : 2;
    ///
    /// [Bit 8] PREFETCHW.
    ///
    UINT32    PREFETCHW : 1;
    UINT32    Reserved3 : 23;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_EXTENDED_CPU_SIG_ECX;

/**
  CPUID Extended Processor Signature and Feature Bits EDX for CPUID leaf
  #CPUID_EXTENDED_CPU_SIG.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32    Reserved1      : 11;
    ///
    /// [Bit 11] SYSCALL/SYSRET available in 64-bit mode.
    ///
    UINT32    SYSCALL_SYSRET : 1;
    UINT32    Reserved2      : 8;
    ///
    /// [Bit 20] Execute Disable Bit available.
    ///
    UINT32    NX             : 1;
    UINT32    Reserved3      : 5;
    ///
    /// [Bit 26] 1-GByte pages are available if 1.
    ///
    UINT32    Page1GB        : 1;
    ///
    /// [Bit 27] RDTSCP and IA32_TSC_AUX are available if 1.
    ///
    UINT32    RDTSCP         : 1;
    UINT32    Reserved4      : 1;
    ///
    /// [Bit 29] Intel(R) 64 Architecture available if 1.
    ///
    UINT32    LM             : 1;
    UINT32    Reserved5      : 2;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_EXTENDED_CPU_SIG_EDX;

/**
  CPUID Processor Brand String

  @param   EAX  CPUID_BRAND_STRING1 (0x80000002)

  @retval  EAX  Processor Brand String in type CPUID_BRAND_STRING_DATA.
  @retval  EBX  Processor Brand String Continued in type CPUID_BRAND_STRING_DATA.
  @retval  ECX  Processor Brand String Continued in type CPUID_BRAND_STRING_DATA.
  @retval  EDX  Processor Brand String Continued in type CPUID_BRAND_STRING_DATA.

  <b>Example usage</b>
  @code
  CPUID_BRAND_STRING_DATA  Eax;
  CPUID_BRAND_STRING_DATA  Ebx;
  CPUID_BRAND_STRING_DATA  Ecx;
  CPUID_BRAND_STRING_DATA  Edx;

  AsmCpuid (CPUID_BRAND_STRING1, &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, &Edx.Uint32);
  @endcode
**/
#define CPUID_BRAND_STRING1  0x80000002

/**
  CPUID Processor Brand String for CPUID leafs #CPUID_BRAND_STRING1,
  #CPUID_BRAND_STRING2, and #CPUID_BRAND_STRING3.
**/
typedef union {
  ///
  /// 4 ASCII characters of Processor Brand String
  ///
  CHAR8     BrandString[4];
  ///
  /// All fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_BRAND_STRING_DATA;

/**
  CPUID Processor Brand String

  @param   EAX  CPUID_BRAND_STRING2 (0x80000003)

  @retval  EAX  Processor Brand String Continued in type CPUID_BRAND_STRING_DATA.
  @retval  EBX  Processor Brand String Continued in type CPUID_BRAND_STRING_DATA.
  @retval  ECX  Processor Brand String Continued in type CPUID_BRAND_STRING_DATA.
  @retval  EDX  Processor Brand String Continued in type CPUID_BRAND_STRING_DATA.

  <b>Example usage</b>
  @code
  CPUID_BRAND_STRING_DATA  Eax;
  CPUID_BRAND_STRING_DATA  Ebx;
  CPUID_BRAND_STRING_DATA  Ecx;
  CPUID_BRAND_STRING_DATA  Edx;

  AsmCpuid (CPUID_BRAND_STRING2, &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, &Edx.Uint32);
  @endcode
**/
#define CPUID_BRAND_STRING2  0x80000003

/**
  CPUID Processor Brand String

  @param   EAX  CPUID_BRAND_STRING3 (0x80000004)

  @retval  EAX  Processor Brand String Continued in type CPUID_BRAND_STRING_DATA.
  @retval  EBX  Processor Brand String Continued in type CPUID_BRAND_STRING_DATA.
  @retval  ECX  Processor Brand String Continued in type CPUID_BRAND_STRING_DATA.
  @retval  EDX  Processor Brand String Continued in type CPUID_BRAND_STRING_DATA.

  <b>Example usage</b>
  @code
  CPUID_BRAND_STRING_DATA  Eax;
  CPUID_BRAND_STRING_DATA  Ebx;
  CPUID_BRAND_STRING_DATA  Ecx;
  CPUID_BRAND_STRING_DATA  Edx;

  AsmCpuid (CPUID_BRAND_STRING3, &Eax.Uint32, &Ebx.Uint32, &Ecx.Uint32, &Edx.Uint32);
  @endcode
**/
#define CPUID_BRAND_STRING3  0x80000004

/**
  CPUID Extended Cache information

  @param   EAX  CPUID_EXTENDED_CACHE_INFO (0x80000006)

  @retval  EAX  Reserved.
  @retval  EBX  Reserved.
  @retval  ECX  Extended cache information described by the type
                CPUID_EXTENDED_CACHE_INFO_ECX.
  @retval  EDX  Reserved.

  <b>Example usage</b>
  @code
  CPUID_EXTENDED_CACHE_INFO_ECX  Ecx;

  AsmCpuid (CPUID_EXTENDED_CACHE_INFO, NULL, NULL, &Ecx.Uint32, NULL);
  @endcode
**/
#define CPUID_EXTENDED_CACHE_INFO  0x80000006

/**
  CPUID Extended Cache information ECX for CPUID leaf #CPUID_EXTENDED_CACHE_INFO.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 7:0] Cache line size in bytes.
    ///
    UINT32    CacheLineSize   : 8;
    UINT32    Reserved        : 4;
    ///
    /// [Bits 15:12] L2 Associativity field.  Supported values are in the range
    /// #CPUID_EXTENDED_CACHE_INFO_ECX_L2_ASSOCIATIVITY_DISABLED to
    /// #CPUID_EXTENDED_CACHE_INFO_ECX_L2_ASSOCIATIVITY_FULL
    ///
    UINT32    L2Associativity : 4;
    ///
    /// [Bits 31:16] Cache size in 1K units.
    ///
    UINT32    CacheSize       : 16;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_EXTENDED_CACHE_INFO_ECX;

///
/// @{ Define value for bit field CPUID_EXTENDED_CACHE_INFO_ECX.L2Associativity
///
#define CPUID_EXTENDED_CACHE_INFO_ECX_L2_ASSOCIATIVITY_DISABLED       0x00
#define CPUID_EXTENDED_CACHE_INFO_ECX_L2_ASSOCIATIVITY_DIRECT_MAPPED  0x01
#define CPUID_EXTENDED_CACHE_INFO_ECX_L2_ASSOCIATIVITY_2_WAY          0x02
#define CPUID_EXTENDED_CACHE_INFO_ECX_L2_ASSOCIATIVITY_4_WAY          0x04
#define CPUID_EXTENDED_CACHE_INFO_ECX_L2_ASSOCIATIVITY_8_WAY          0x06
#define CPUID_EXTENDED_CACHE_INFO_ECX_L2_ASSOCIATIVITY_16_WAY         0x08
#define CPUID_EXTENDED_CACHE_INFO_ECX_L2_ASSOCIATIVITY_32_WAY         0x0A
#define CPUID_EXTENDED_CACHE_INFO_ECX_L2_ASSOCIATIVITY_48_WAY         0x0B
#define CPUID_EXTENDED_CACHE_INFO_ECX_L2_ASSOCIATIVITY_64_WAY         0x0C
#define CPUID_EXTENDED_CACHE_INFO_ECX_L2_ASSOCIATIVITY_96_WAY         0x0D
#define CPUID_EXTENDED_CACHE_INFO_ECX_L2_ASSOCIATIVITY_128_WAY        0x0E
#define CPUID_EXTENDED_CACHE_INFO_ECX_L2_ASSOCIATIVITY_FULL           0x0F
///
/// @}
///

/**
  CPUID Extended Time Stamp Counter information

  @param   EAX  CPUID_EXTENDED_TIME_STAMP_COUNTER (0x80000007)

  @retval  EAX  Reserved.
  @retval  EBX  Reserved.
  @retval  ECX  Reserved.
  @retval  EDX  Extended time stamp counter (TSC) information described by the
                type CPUID_EXTENDED_TIME_STAMP_COUNTER_EDX.

  <b>Example usage</b>
  @code
  CPUID_EXTENDED_TIME_STAMP_COUNTER_EDX  Edx;

  AsmCpuid (CPUID_EXTENDED_TIME_STAMP_COUNTER, NULL, NULL, NULL, &Edx.Uint32);
  @endcode
**/
#define CPUID_EXTENDED_TIME_STAMP_COUNTER  0x80000007

/**
  CPUID Extended Time Stamp Counter information EDX for CPUID leaf
  #CPUID_EXTENDED_TIME_STAMP_COUNTER.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32    Reserved1    : 8;
    ///
    /// [Bit 8] Invariant TSC available if 1.
    ///
    UINT32    InvariantTsc : 1;
    UINT32    Reserved2    : 23;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_EXTENDED_TIME_STAMP_COUNTER_EDX;

/**
  CPUID Linear Physical Address Size

  @param   EAX  CPUID_VIR_PHY_ADDRESS_SIZE (0x80000008)

  @retval  EAX  Linear/Physical Address Size described by the type
                CPUID_VIR_PHY_ADDRESS_SIZE_EAX.
  @retval  EBX  Reserved.
  @retval  ECX  Reserved.
  @retval  EDX  Reserved.

  <b>Example usage</b>
  @code
  CPUID_VIR_PHY_ADDRESS_SIZE_EAX  Eax;

  AsmCpuid (CPUID_VIR_PHY_ADDRESS_SIZE, &Eax.Uint32, NULL, NULL, NULL);
  @endcode
**/
#define CPUID_VIR_PHY_ADDRESS_SIZE  0x80000008

/**
  CPUID Linear Physical Address Size EAX for CPUID leaf
  #CPUID_VIR_PHY_ADDRESS_SIZE.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 7:0] Number of physical address bits.
    ///
    /// @note
    /// If CPUID.80000008H:EAX[7:0] is supported, the maximum physical address
    /// number supported should come from this field.
    ///
    UINT32    PhysicalAddressBits : 8;
    ///
    /// [Bits 15:8] Number of linear address bits.
    ///
    UINT32    LinearAddressBits   : 8;
    UINT32    Reserved            : 16;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_VIR_PHY_ADDRESS_SIZE_EAX;

#endif
