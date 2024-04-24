/** @file
  CPUID leaf definitions.

  Provides defines for CPUID leaf indexes.  Data structures are provided for
  registers returned by a CPUID leaf that contain one or more bit fields.
  If a register returned is a single 32-bit value, then a data structure is
  not provided for that register.

  Copyright (c) 2017, Advanced Micro Devices. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
  AMD64 Architecture Programming Manual volume 2, March 2017, Sections 15.34

**/

#ifndef __AMD_CPUID_H__
#define __AMD_CPUID_H__

/**
CPUID Signature Information

@param   EAX  CPUID_SIGNATURE (0x00)

@retval  EAX  Returns the highest value the CPUID instruction recognizes for
              returning basic processor information. The value is returned is
              processor specific.
@retval  EBX  First 4 characters of a vendor identification string.
@retval  ECX  Last 4 characters of a vendor identification string.
@retval  EDX  Middle 4 characters of a vendor identification string.

**/

///
/// @{ CPUID signature values returned by AMD processors
///
#define CPUID_SIGNATURE_AUTHENTIC_AMD_EBX  SIGNATURE_32 ('A', 'u', 't', 'h')
#define CPUID_SIGNATURE_AUTHENTIC_AMD_EDX  SIGNATURE_32 ('e', 'n', 't', 'i')
#define CPUID_SIGNATURE_AUTHENTIC_AMD_ECX  SIGNATURE_32 ('c', 'A', 'M', 'D')
///
/// @}
///

/**
  CPUID Extended Processor Signature and Features

  @param   EAX  CPUID_EXTENDED_CPU_SIG (0x80000001)

  @retval  EAX  Extended Family, Model, Stepping Identifiers
                described by the type CPUID_AMD_EXTENDED_CPU_SIG_EAX.
  @retval  EBX  Brand Identifier
                described by the type CPUID_AMD_EXTENDED_CPU_SIG_EBX.
  @retval  ECX  Extended Feature Identifiers
                described by the type CPUID_AMD_EXTENDED_CPU_SIG_ECX.
  @retval  EDX  Extended Feature Identifiers
                described by the type CPUID_AMD_EXTENDED_CPU_SIG_EDX.
**/

/**
  CPUID Extended Processor Signature and Features EAX for CPUID leaf
  #CPUID_EXTENDED_CPU_SIG.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 3:0] Stepping.
    ///
    UINT32    Stepping   : 4;
    ///
    /// [Bits 7:4] Base Model.
    ///
    UINT32    BaseModel  : 4;
    ///
    /// [Bits 11:8] Base Family.
    ///
    UINT32    BaseFamily : 4;
    ///
    /// [Bit 15:12] Reserved.
    ///
    UINT32    Reserved1  : 4;
    ///
    /// [Bits 19:16] Extended Model.
    ///
    UINT32    ExtModel   : 4;
    ///
    /// [Bits 27:20] Extended Family.
    ///
    UINT32    ExtFamily  : 8;
    ///
    /// [Bit 31:28] Reserved.
    ///
    UINT32    Reserved2  : 4;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_AMD_EXTENDED_CPU_SIG_EAX;

/**
  CPUID Extended Processor Signature and Features EBX for CPUID leaf
  #CPUID_EXTENDED_CPU_SIG.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 27:0] Reserved.
    ///
    UINT32    Reserved : 28;
    ///
    /// [Bit 31:28] Package Type.
    ///
    UINT32    PkgType  : 4;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_AMD_EXTENDED_CPU_SIG_EBX;

/**
  CPUID Extended Processor Signature and Features ECX for CPUID leaf
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
    UINT32    LAHF_SAHF               : 1;
    ///
    /// [Bit 1] Core multi-processing legacy mode.
    ///
    UINT32    CmpLegacy               : 1;
    ///
    /// [Bit 2] Secure Virtual Mode feature.
    ///
    UINT32    SVM                     : 1;
    ///
    /// [Bit 3] Extended APIC register space.
    ///
    UINT32    ExtApicSpace            : 1;
    ///
    /// [Bit 4] LOCK MOV CR0 means MOV CR8.
    ///
    UINT32    AltMovCr8               : 1;
    ///
    /// [Bit 5] LZCNT instruction support.
    ///
    UINT32    LZCNT                   : 1;
    ///
    /// [Bit 6] SSE4A instruction support.
    ///
    UINT32    SSE4A                   : 1;
    ///
    /// [Bit 7] Misaligned SSE Mode.
    ///
    UINT32    MisAlignSse             : 1;
    ///
    /// [Bit 8] ThreeDNow Prefetch instructions.
    ///
    UINT32    PREFETCHW               : 1;
    ///
    /// [Bit 9] OS Visible Work-around support.
    ///
    UINT32    OSVW                    : 1;
    ///
    /// [Bit 10] Instruction Based Sampling.
    ///
    UINT32    IBS                     : 1;
    ///
    /// [Bit 11] Extended Operation Support.
    ///
    UINT32    XOP                     : 1;
    ///
    /// [Bit 12] SKINIT and STGI support.
    ///
    UINT32    SKINIT                  : 1;
    ///
    /// [Bit 13] Watchdog Timer support.
    ///
    UINT32    WDT                     : 1;
    ///
    /// [Bit 14] Reserved.
    ///
    UINT32    Reserved1               : 1;
    ///
    /// [Bit 15] Lightweight Profiling support.
    ///
    UINT32    LWP                     : 1;
    ///
    /// [Bit 16] 4-Operand FMA instruction support.
    ///
    UINT32    FMA4                    : 1;
    ///
    /// [Bit 17] Translation Cache Extension.
    ///
    UINT32    TCE                     : 1;
    ///
    /// [Bit 21:18] Reserved.
    ///
    UINT32    Reserved2               : 4;
    ///
    /// [Bit 22] Topology Extensions support.
    ///
    UINT32    TopologyExtensions      : 1;
    ///
    /// [Bit 23] Core Performance Counter Extensions.
    ///
    UINT32    PerfCtrExtCore          : 1;
    ///
    /// [Bit 25:24] Reserved.
    ///
    UINT32    Reserved3               : 2;
    ///
    /// [Bit 26] Data Breakpoint Extension.
    ///
    UINT32    DataBreakpointExtension : 1;
    ///
    /// [Bit 27] Performance Time-Stamp Counter.
    ///
    UINT32    PerfTsc                 : 1;
    ///
    /// [Bit 28] L3 Performance Counter Extensions.
    ///
    UINT32    PerfCtrExtL3            : 1;
    ///
    /// [Bit 29] MWAITX and MONITORX capability.
    ///
    UINT32    MwaitExtended           : 1;
    ///
    /// [Bit 31:30] Reserved.
    ///
    UINT32    Reserved4               : 2;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_AMD_EXTENDED_CPU_SIG_ECX;

/**
  CPUID Extended Processor Signature and Features EDX for CPUID leaf
  #CPUID_EXTENDED_CPU_SIG.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] x87 floating point unit on-chip.
    ///
    UINT32    FPU            : 1;
    ///
    /// [Bit 1] Virtual-mode enhancements.
    ///
    UINT32    VME            : 1;
    ///
    /// [Bit 2] Debugging extensions, IO breakpoints, CR4.DE.
    ///
    UINT32    DE             : 1;
    ///
    /// [Bit 3] Page-size extensions (4 MB pages).
    ///
    UINT32    PSE            : 1;
    ///
    /// [Bit 4] Time stamp counter, RDTSC/RDTSCP instructions, CR4.TSD.
    ///
    UINT32    TSC            : 1;
    ///
    /// [Bit 5] MSRs, with RDMSR and WRMSR instructions.
    ///
    UINT32    MSR            : 1;
    ///
    /// [Bit 6] Physical-address extensions (PAE).
    ///
    UINT32    PAE            : 1;
    ///
    /// [Bit 7] Machine check exception, CR4.MCE.
    ///
    UINT32    MCE            : 1;
    ///
    /// [Bit 8] CMPXCHG8B instruction.
    ///
    UINT32    CMPXCHG8B      : 1;
    ///
    /// [Bit 9] APIC exists and is enabled.
    ///
    UINT32    APIC           : 1;
    ///
    /// [Bit 10] Reserved.
    ///
    UINT32    Reserved1      : 1;
    ///
    /// [Bit 11] SYSCALL and SYSRET instructions.
    ///
    UINT32    SYSCALL_SYSRET : 1;
    ///
    /// [Bit 12] Memory-type range registers.
    ///
    UINT32    MTRR           : 1;
    ///
    /// [Bit 13] Page global extension, CR4.PGE.
    ///
    UINT32    PGE            : 1;
    ///
    /// [Bit 14] Machine check architecture, MCG_CAP.
    ///
    UINT32    MCA            : 1;
    ///
    /// [Bit 15] Conditional move instructions, CMOV, FCOMI, FCMOV.
    ///
    UINT32    CMOV           : 1;
    ///
    /// [Bit 16] Page attribute table.
    ///
    UINT32    PAT            : 1;
    ///
    /// [Bit 17] Page-size extensions.
    ///
    UINT32    PSE36          : 1;
    ///
    /// [Bit 19:18] Reserved.
    ///
    UINT32    Reserved2      : 2;
    ///
    /// [Bit 20] No-execute page protection.
    ///
    UINT32    NX             : 1;
    ///
    /// [Bit 21] Reserved.
    ///
    UINT32    Reserved3      : 1;
    ///
    /// [Bit 22] AMD Extensions to MMX instructions.
    ///
    UINT32    MmxExt         : 1;
    ///
    /// [Bit 23] MMX instructions.
    ///
    UINT32    MMX            : 1;
    ///
    /// [Bit 24] FXSAVE and FXRSTOR instructions.
    ///
    UINT32    FFSR           : 1;
    ///
    /// [Bit 25] FXSAVE and FXRSTOR instruction optimizations.
    ///
    UINT32    FFXSR          : 1;
    ///
    /// [Bit 26] 1-GByte large page support.
    ///
    UINT32    Page1GB        : 1;
    ///
    /// [Bit 27] RDTSCP instructions.
    ///
    UINT32    RDTSCP         : 1;
    ///
    /// [Bit 28] Reserved.
    ///
    UINT32    Reserved4      : 1;
    ///
    /// [Bit 29] Long Mode.
    ///
    UINT32    LM             : 1;
    ///
    /// [Bit 30] 3DNow! instructions.
    ///
    UINT32    ThreeDNow      : 1;
    ///
    /// [Bit 31] AMD Extensions to 3DNow! instructions.
    ///
    UINT32    ThreeDNowExt   : 1;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_AMD_EXTENDED_CPU_SIG_EDX;

/**
CPUID Linear Physical Address Size

@param   EAX  CPUID_VIR_PHY_ADDRESS_SIZE (0x80000008)

@retval  EAX  Linear/Physical Address Size described by the type
              CPUID_AMD_VIR_PHY_ADDRESS_SIZE_EAX.
@retval  EBX  Linear/Physical Address Size described by the type
              CPUID_AMD_VIR_PHY_ADDRESS_SIZE_EBX.
@retval  ECX  Linear/Physical Address Size described by the type
              CPUID_AMD_VIR_PHY_ADDRESS_SIZE_ECX.
@retval  EDX  Reserved.
**/

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
    /// [Bits 7:0] Maximum physical byte address size in bits.
    ///
    UINT32    PhysicalAddressBits : 8;
    ///
    /// [Bits 15:8] Maximum linear byte address size in bits.
    ///
    UINT32    LinearAddressBits   : 8;
    ///
    /// [Bits 23:16] Maximum guest physical byte address size in bits.
    ///
    UINT32    GuestPhysAddrSize   : 8;
    ///
    /// [Bit 31:24] Reserved.
    ///
    UINT32    Reserved            : 8;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_AMD_VIR_PHY_ADDRESS_SIZE_EAX;

/**
  CPUID Linear Physical Address Size EBX for CPUID leaf
  #CPUID_VIR_PHY_ADDRESS_SIZE.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 0] Clear Zero Instruction.
    ///
    UINT32    CLZERO     : 1;
    ///
    /// [Bits 1] Instructions retired count support.
    ///
    UINT32    IRPerf     : 1;
    ///
    /// [Bits 2] Restore error pointers for XSave instructions.
    ///
    UINT32    XSaveErPtr : 1;
    ///
    /// [Bit 31:3] Reserved.
    ///
    UINT32    Reserved   : 29;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_AMD_VIR_PHY_ADDRESS_SIZE_EBX;

/**
  CPUID Linear Physical Address Size ECX for CPUID leaf
  #CPUID_VIR_PHY_ADDRESS_SIZE.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 7:0] Number of threads - 1.
    ///
    UINT32    NC               : 8;
    ///
    /// [Bit 11:8] Reserved.
    ///
    UINT32    Reserved1        : 4;
    ///
    /// [Bits 15:12] APIC ID size.
    ///
    UINT32    ApicIdCoreIdSize : 4;
    ///
    /// [Bits 17:16] Performance time-stamp counter size.
    ///
    UINT32    PerfTscSize      : 2;
    ///
    /// [Bit 31:18] Reserved.
    ///
    UINT32    Reserved2        : 14;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_AMD_VIR_PHY_ADDRESS_SIZE_ECX;

/**
  CPUID AMD Processor Topology

  @param   EAX  CPUID_AMD_PROCESSOR_TOPOLOGY (0x8000001E)

  @retval  EAX  Extended APIC ID described by the type
                CPUID_AMD_PROCESSOR_TOPOLOGY_EAX.
  @retval  EBX  Core Identifiers described by the type
                CPUID_AMD_PROCESSOR_TOPOLOGY_EBX.
  @retval  ECX  Node Identifiers described by the type
                CPUID_AMD_PROCESSOR_TOPOLOGY_ECX.
  @retval  EDX  Reserved.
**/
#define CPUID_AMD_PROCESSOR_TOPOLOGY  0x8000001E

/**
  CPUID AMD Processor Topology EAX for CPUID leaf
  #CPUID_AMD_PROCESSOR_TOPOLOGY.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 31:0] Extended APIC Id.
    ///
    UINT32    ExtendedApicId;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_AMD_PROCESSOR_TOPOLOGY_EAX;

/**
  CPUID AMD Processor Topology EBX for CPUID leaf
  #CPUID_AMD_PROCESSOR_TOPOLOGY.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 7:0] Core Id.
    ///
    UINT32    CoreId         : 8;
    ///
    /// [Bits 15:8] Threads per core.
    ///
    UINT32    ThreadsPerCore : 8;
    ///
    /// [Bit 31:16] Reserved.
    ///
    UINT32    Reserved       : 16;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_AMD_PROCESSOR_TOPOLOGY_EBX;

/**
  CPUID AMD Processor Topology ECX for CPUID leaf
  #CPUID_AMD_PROCESSOR_TOPOLOGY.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 7:0] Node Id.
    ///
    UINT32    NodeId            : 8;
    ///
    /// [Bits 10:8] Nodes per processor.
    ///
    UINT32    NodesPerProcessor : 3;
    ///
    /// [Bit 31:11] Reserved.
    ///
    UINT32    Reserved          : 21;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_AMD_PROCESSOR_TOPOLOGY_ECX;

/**
  CPUID Memory Encryption Information

  @param   EAX  CPUID_MEMORY_ENCRYPTION_INFO (0x8000001F)

  @retval  EAX  Returns the memory encryption feature support status.
  @retval  EBX  If memory encryption feature is present then return
                the page table bit number used to enable memory encryption support
                and reducing of physical address space in bits.
  @retval  ECX  Returns number of encrypted guest supported simultaneously.
  @retval  EDX  Returns minimum SEV enabled and SEV disabled ASID.

  <b>Example usage</b>
  @code
  UINT32 Eax;
  UINT32 Ebx;
  UINT32 Ecx;
  UINT32 Edx;

  AsmCpuid (CPUID_MEMORY_ENCRYPTION_INFO, &Eax, &Ebx, &Ecx, &Edx);
  @endcode
**/

#define CPUID_MEMORY_ENCRYPTION_INFO  0x8000001F

/**
  CPUID Memory Encryption support information EAX for CPUID leaf
  #CPUID_MEMORY_ENCRYPTION_INFO.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] Secure Memory Encryption (Sme) Support
    ///
    UINT32    SmeBit          : 1;

    ///
    /// [Bit 1] Secure Encrypted Virtualization (Sev) Support
    ///
    UINT32    SevBit          : 1;

    ///
    /// [Bit 2] Page flush MSR support
    ///
    UINT32    PageFlushMsrBit : 1;

    ///
    /// [Bit 3] Encrypted state support
    ///
    UINT32    SevEsBit        : 1;

    ///
    /// [Bit 31:4] Reserved
    ///
    UINT32    ReservedBits    : 28;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_MEMORY_ENCRYPTION_INFO_EAX;

/**
  CPUID Memory Encryption support information EBX for CPUID leaf
  #CPUID_MEMORY_ENCRYPTION_INFO.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 5:0] Page table bit number used to enable memory encryption
    ///
    UINT32    PtePosBits      : 6;

    ///
    /// [Bit 11:6] Reduction of system physical address space bits when
    ///  memory encryption is enabled
    ///
    UINT32    ReducedPhysBits : 5;

    ///
    /// [Bit 31:12] Reserved
    ///
    UINT32    ReservedBits    : 21;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_MEMORY_ENCRYPTION_INFO_EBX;

/**
  CPUID Memory Encryption support information ECX for CPUID leaf
  #CPUID_MEMORY_ENCRYPTION_INFO.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 31:0] Number of encrypted guest supported simultaneously
    ///
    UINT32    NumGuests;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_MEMORY_ENCRYPTION_INFO_ECX;

/**
  CPUID Memory Encryption support information EDX for CPUID leaf
  #CPUID_MEMORY_ENCRYPTION_INFO.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 31:0] Minimum SEV enabled, SEV-ES disabled ASID
    ///
    UINT32    MinAsid;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32    Uint32;
} CPUID_MEMORY_ENCRYPTION_INFO_EDX;

#endif
