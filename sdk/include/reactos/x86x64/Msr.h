/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Provides CPUID structure definitions
 * COPYRIGHT:   Copyright 2023 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define SIZE_2KB 2048

#include "Intel/ArchitecturalMsr.h"

typedef struct
{
    UINT32 Reserved0 : 2; // [1:0] Reserved
    UINT32 InterruptWindowExiting : 1; // [2] Interrupt window exiting
    UINT32 UseTSCOffsetting : 1; // [3] Use TSC offsetting
    UINT32 Reserved4 : 3; // [6..4] Reserved
    UINT32 HLTExiting : 1; // [7] HLT exiting
    UINT32 Reserved8 : 1; // [8] Reserved
    UINT32 INVLPG_Exiting : 1; // [9] INVLPG exiting
    UINT32 MWAIT_Exiting : 1; // [10] MWAIT exiting
    UINT32 RDPMC_Exiting : 1; // [11] RDPMC exiting
    UINT32 RDTSC_Exiting : 1; // [12] RDTSC exiting
    UINT32 CR3_Load_Exiting : 1; // [15] CR3 load exiting
    UINT32 CR3_Store_Exiting : 1; // [16] CR3 store exiting
    UINT32 ActivateTertiaryControls : 1; // [17] Activate tertiary controls
    UINT32 CR8_Load_Exiting : 1; // [19] CR8 load exiting
    UINT32 CR8_Store_Exiting : 1; // [20] CR8 store exiting
    UINT32 Use_TPR_Shadow : 1; // [21] Use TPR shadow
    UINT32 NMI_Window_Exiting : 1; // [22] NMI window exiting
    UINT32 MOV_DR_Exiting : 1; // [23] MOV DR exiting
    UINT32 Unconditional_IO_Exiting : 1; // [24] Unconditional I/O exiting
    UINT32 Use_IO_Bitmaps : 1; // [25] Use I/O bitmaps 
    UINT32 Monitor_Trap_Flag : 1; // [27] Monitor trap flag
    UINT32 Use_MSR_Bitmap : 1; // [28] Use MSR bitmap
    UINT32 MONITOR_Exiting : 1; // [29] MONITOR exiting
    UINT32 PAUSE_Exiting : 1; // [30] PAUSE exiting
    UINT32 ActivateSecondaryControls : 1; // [31] Activate secondary controls
} VMX_PROCBASED_CTRLS;

typedef struct
{
    UINT32 VirtualizeApicAccesses : 1; // [0] Virtualize APIC accesses 
    UINT32 EPT : 1; // [1] Enable EPT
    UINT32 DescriptorTable_Exiting : 1; // [2] Descriptor-table exiting
    UINT32 RDTSCP : 1; // [3] Enable RDTSCP
    UINT32 Virtualize_x2APIC : 1; // [4] Virtualize x2APIC mode
    UINT32 VPID : 1; // [5] Enable VPID
    UINT32 WBINVD_Exiting : 1; // [6] WBINVD exiting
    UINT32 UnrestrictedGuest : 1; // [7] Unrestricted guest
    UINT32 APIC_Virtualization : 1; // [8] APIC-register virtualization
    UINT32 VirtualInterruptDelivery : 1; // [9] Virtual-interrupt delivery
    UINT32 PAUSE_Loop_Exiting : 1; // [10] PAUSE-loop exiting
    UINT32 RDRAND_Exiting : 1; // [11] RDRAND exiting
    UINT32 INVPCID : 1; // [12] Enable INVPCID
    UINT32 VM_Functions : 1; // [13] Enable VM functions
    UINT32 VMCS_Shadowing : 1; //  [14] VMCS shadowing
    UINT32 ENCLS_Exiting : 1; // [15] Enable ENCLS exiting
    UINT32 RDSEED_Exiting : 1; // [16] RDSEED exiting
    UINT32 PML : 1; // [17] Enable PML
    UINT32 EPT_Violation : 1; // [18] EPT-violation #VE
    UINT32 Conceal_VMX_from_PT : 1; // [19] Conceal VMX from PT
    UINT32 XSAVES : 1; // [20] Enable XSAVES / XRSTORS
    UINT32 PASID_Translation : 1; // [21] PASID translation
    UINT32 ModeBasedExecutionControl : 1; // [22] Mode-based execute control for EPT
    UINT32 SubPageWritePerm : 1; // [23] Sub-page write permissions for EPT
    UINT32 GuestPhysicalAddr : 1; // [24] Intel PT uses guest physical addresses
    UINT32 TSC_Scaling : 1; // [25] Use TSC scaling
    UINT32 User_Wait : 1; // [26] Enable user wait and pause
    UINT32 PCONFIG : 1; // [27] Enable PCONFIG
    UINT32 ENCLV_Exiting : 1; // [28] Enable ENCLV exiting
    UINT32 VMM_Bus_Lock_Detection : 1; // [30] VMM bus-lock detection
    UINT32 InstructionTimeout : 1; // [31] Instruction timeout
} VMX_PROCBASED_CTLS2;

typedef union
{
    struct
    {
        VMX_PROCBASED_CTRLS Allowed0;
        VMX_PROCBASED_CTRLS Allowed1;
    } Bits;

    UINT64    Uint64;
} MSR_IA32_VMX_PROCBASED_CTLS_REGISTER;


typedef union
{
    struct
    {
        VMX_PROCBASED_CTLS2 Allowed0;
        VMX_PROCBASED_CTLS2 Allowed1;
    } Bits;

    UINT64    Uint64;
} MSR_IA32_VMX_PROCBASED_CTLS2_REGISTER;
