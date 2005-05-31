/******************************************************************************
 * arch-x86_32.h
 * 
 * Guest OS interface to x86 32-bit Xen.
 * 
 * Copyright (c) 2004, K A Fraser
 */

#ifndef __XEN_PUBLIC_ARCH_X86_32_H__
#define __XEN_PUBLIC_ARCH_X86_32_H__

#ifndef PACKED
/* GCC-specific way to pack structure definitions (no implicit padding). */
#define PACKED __attribute__ ((packed))
#endif

/*
 * Pointers and other address fields inside interface structures are padded to
 * 64 bits. This means that field alignments aren't different between 32- and
 * 64-bit architectures. 
 */
/* NB. Multi-level macro ensures __LINE__ is expanded before concatenation. */
#define __MEMORY_PADDING(_X) u32 __pad_ ## _X
#define _MEMORY_PADDING(_X)  __MEMORY_PADDING(_X)
#define MEMORY_PADDING       _MEMORY_PADDING(__LINE__)

/*
 * SEGMENT DESCRIPTOR TABLES
 */
/*
 * A number of GDT entries are reserved by Xen. These are not situated at the
 * start of the GDT because some stupid OSes export hard-coded selector values
 * in their ABI. These hard-coded values are always near the start of the GDT,
 * so Xen places itself out of the way, at the far end of the GDT.
 */
#define FIRST_RESERVED_GDT_PAGE  14
#define FIRST_RESERVED_GDT_BYTE  (FIRST_RESERVED_GDT_PAGE * 4096)
#define FIRST_RESERVED_GDT_ENTRY (FIRST_RESERVED_GDT_BYTE / 8)

/*
 * These flat segments are in the Xen-private section of every GDT. Since these
 * are also present in the initial GDT, many OSes will be able to avoid
 * installing their own GDT.
 */
#define FLAT_RING1_CS 0xe019    /* GDT index 259 */
#define FLAT_RING1_DS 0xe021    /* GDT index 260 */
#define FLAT_RING1_SS 0xe021    /* GDT index 260 */
#define FLAT_RING3_CS 0xe02b    /* GDT index 261 */
#define FLAT_RING3_DS 0xe033    /* GDT index 262 */
#define FLAT_RING3_SS 0xe033    /* GDT index 262 */

#define FLAT_KERNEL_CS FLAT_RING1_CS
#define FLAT_KERNEL_DS FLAT_RING1_DS
#define FLAT_KERNEL_SS FLAT_RING1_SS
#define FLAT_USER_CS    FLAT_RING3_CS
#define FLAT_USER_DS    FLAT_RING3_DS
#define FLAT_USER_SS    FLAT_RING3_SS

/* And the trap vector is... */
#define TRAP_INSTR "int $0x82"


/*
 * Virtual addresses beyond this are not modifiable by guest OSes. The 
 * machine->physical mapping table starts at this address, read-only.
 */
#ifdef CONFIG_X86_PAE
# define HYPERVISOR_VIRT_START (0xF5800000UL)
#else
# define HYPERVISOR_VIRT_START (0xFC000000UL)
#endif
#ifndef machine_to_phys_mapping
#define machine_to_phys_mapping ((u32 *)HYPERVISOR_VIRT_START)
#endif

#ifndef __ASSEMBLY__

/* NB. Both the following are 32 bits each. */
typedef unsigned long memory_t;   /* Full-sized pointer/address/memory-size. */
typedef unsigned long cpureg_t;   /* Full-sized register.                    */

/*
 * Send an array of these to HYPERVISOR_set_trap_table()
 */
#define TI_GET_DPL(_ti)      ((_ti)->flags & 3)
#define TI_GET_IF(_ti)       ((_ti)->flags & 4)
#define TI_SET_DPL(_ti,_dpl) ((_ti)->flags |= (_dpl))
#define TI_SET_IF(_ti,_if)   ((_ti)->flags |= ((!!(_if))<<2))
typedef struct {
    u8       vector;  /* 0: exception vector                              */
    u8       flags;   /* 1: 0-3: privilege level; 4: clear event enable?  */
    u16      cs;      /* 2: code selector                                 */
    memory_t address; /* 4: code address                                  */
} PACKED trap_info_t; /* 8 bytes */

typedef struct cpu_user_regs {
    u32 ebx;
    u32 ecx;
    u32 edx;
    u32 esi;
    u32 edi;
    u32 ebp;
    u32 eax;
    u16 error_code;    /* private */
    u16 entry_vector;  /* private */
    u32 eip;
    u16 cs;
    u8  saved_upcall_mask;
    u8  _pad0;
    u32 eflags;
    u32 esp;
    u16 ss, _pad1;
    u16 es, _pad2;
    u16 ds, _pad3;
    u16 fs, _pad4;
    u16 gs, _pad5;
} cpu_user_regs_t;

typedef u64 tsc_timestamp_t; /* RDTSC timestamp */

/*
 * The following is all CPU context. Note that the fpu_ctxt block is filled 
 * in by FXSAVE if the CPU has feature FXSR; otherwise FSAVE is used.
 */
typedef struct vcpu_guest_context {
#define VGCF_I387_VALID (1<<0)
#define VGCF_VMX_GUEST  (1<<1)
#define VGCF_IN_KERNEL  (1<<2)
    unsigned long flags;                    /* VGCF_* flags                 */
    cpu_user_regs_t user_regs;              /* User-level CPU registers     */
    struct { char x[512]; } fpu_ctxt        /* User-level FPU registers     */
    __attribute__((__aligned__(16)));       /* (needs 16-byte alignment)    */
    trap_info_t   trap_ctxt[256];           /* Virtual IDT                  */
    unsigned long ldt_base, ldt_ents;       /* LDT (linear address, # ents) */
    unsigned long gdt_frames[16], gdt_ents; /* GDT (machine frames, # ents) */
    unsigned long kernel_ss, kernel_sp;     /* Virtual TSS (only SS1/SP1)   */
    unsigned long pt_base;                  /* CR3 (pagetable base)         */
    unsigned long debugreg[8];              /* DB0-DB7 (debug registers)    */
    unsigned long event_callback_cs;        /* CS:EIP of event callback     */
    unsigned long event_callback_eip;
    unsigned long failsafe_callback_cs;     /* CS:EIP of failsafe callback  */
    unsigned long failsafe_callback_eip;
    unsigned long vm_assist;                /* VMASST_TYPE_* bitmap */
} vcpu_guest_context_t;

typedef struct {
    /* MFN of a table of MFNs that make up p2m table */
    u64 pfn_to_mfn_frame_list;
} arch_shared_info_t;

typedef struct {
} arch_vcpu_info_t;

#endif

#endif
