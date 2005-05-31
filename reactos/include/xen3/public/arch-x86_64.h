/******************************************************************************
 * arch-x86_64.h
 * 
 * Guest OS interface to x86 64-bit Xen.
 * 
 * Copyright (c) 2004, K A Fraser
 */

#ifndef __XEN_PUBLIC_ARCH_X86_64_H__
#define __XEN_PUBLIC_ARCH_X86_64_H__

#ifndef PACKED
/* GCC-specific way to pack structure definitions (no implicit padding). */
#define PACKED __attribute__ ((packed))
#endif

/* Pointers are naturally 64 bits in this architecture; no padding needed. */
#define _MEMORY_PADDING(_X)
#define MEMORY_PADDING 

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
 * 64-bit segment selectors
 * These flat segments are in the Xen-private section of every GDT. Since these
 * are also present in the initial GDT, many OSes will be able to avoid
 * installing their own GDT.
 */

#define FLAT_RING3_CS32 0xe023  /* GDT index 260 */
#define FLAT_RING3_CS64 0xe033  /* GDT index 261 */
#define FLAT_RING3_DS32 0xe02b  /* GDT index 262 */
#define FLAT_RING3_DS64 0x0000  /* NULL selector */
#define FLAT_RING3_SS32 0xe02b  /* GDT index 262 */
#define FLAT_RING3_SS64 0xe02b  /* GDT index 262 */

#define FLAT_KERNEL_DS64 FLAT_RING3_DS64
#define FLAT_KERNEL_DS32 FLAT_RING3_DS32
#define FLAT_KERNEL_DS   FLAT_KERNEL_DS64
#define FLAT_KERNEL_CS64 FLAT_RING3_CS64
#define FLAT_KERNEL_CS32 FLAT_RING3_CS32
#define FLAT_KERNEL_CS   FLAT_KERNEL_CS64
#define FLAT_KERNEL_SS64 FLAT_RING3_SS64
#define FLAT_KERNEL_SS32 FLAT_RING3_SS32
#define FLAT_KERNEL_SS   FLAT_KERNEL_SS64

#define FLAT_USER_DS64 FLAT_RING3_DS64
#define FLAT_USER_DS32 FLAT_RING3_DS32
#define FLAT_USER_DS   FLAT_USER_DS64
#define FLAT_USER_CS64 FLAT_RING3_CS64
#define FLAT_USER_CS32 FLAT_RING3_CS32
#define FLAT_USER_CS   FLAT_USER_CS64
#define FLAT_USER_SS64 FLAT_RING3_SS64
#define FLAT_USER_SS32 FLAT_RING3_SS32
#define FLAT_USER_SS   FLAT_USER_SS64

/* And the trap vector is... */
#define TRAP_INSTR "syscall"

#ifndef HYPERVISOR_VIRT_START
#define HYPERVISOR_VIRT_START (0xFFFF800000000000UL)
#define HYPERVISOR_VIRT_END   (0xFFFF880000000000UL)
#endif

#ifndef __ASSEMBLY__

/* The machine->physical mapping table starts at this address, read-only. */
#ifndef machine_to_phys_mapping
#define machine_to_phys_mapping ((u32 *)HYPERVISOR_VIRT_START)
#endif

/*
 * int HYPERVISOR_set_segment_base(unsigned int which, unsigned long base)
 *  @which == SEGBASE_*  ;  @base == 64-bit base address
 * Returns 0 on success.
 */
#define SEGBASE_FS          0
#define SEGBASE_GS_USER     1
#define SEGBASE_GS_KERNEL   2
#define SEGBASE_GS_USER_SEL 3 /* Set user %gs specified in base[15:0] */

/*
 * int HYPERVISOR_switch_to_user(void)
 * All arguments are on the kernel stack, in the following format.
 * Never returns if successful. Current kernel context is lost.
 * If flags contains VGCF_IN_SYSCALL:
 *   Restore RAX, RIP, RFLAGS, RSP. 
 *   Discard R11, RCX, CS, SS.
 * Otherwise:
 *   Restore RAX, R11, RCX, CS:RIP, RFLAGS, SS:RSP.
 * All other registers are saved on hypercall entry and restored to user.
 */
/* Guest exited in SYSCALL context? Return to guest with SYSRET? */
#define VGCF_IN_SYSCALL (1<<8)
struct switch_to_user {
    /* Top of stack (%rsp at point of hypercall). */
    u64 rax, r11, rcx, flags, rip, cs, rflags, rsp, ss;
    /* Bottom of switch_to_user stack frame. */
} PACKED;

/* NB. Both the following are 64 bits each. */
typedef unsigned long memory_t;   /* Full-sized pointer/address/memory-size. */
typedef unsigned long cpureg_t;   /* Full-sized register.                    */

/*
 * Send an array of these to HYPERVISOR_set_trap_table().
 * N.B. As in x86/32 mode, the privilege level specifies which modes may enter
 * a trap via a software interrupt. Since rings 1 and 2 are unavailable, we
 * allocate privilege levels as follows:
 *  Level == 0: Noone may enter
 *  Level == 1: Kernel may enter
 *  Level == 2: Kernel may enter
 *  Level == 3: Everyone may enter
 */
#define TI_GET_DPL(_ti)      ((_ti)->flags & 3)
#define TI_GET_IF(_ti)       ((_ti)->flags & 4)
#define TI_SET_DPL(_ti,_dpl) ((_ti)->flags |= (_dpl))
#define TI_SET_IF(_ti,_if)   ((_ti)->flags |= ((!!(_if))<<2))
typedef struct {
    u8       vector;  /* 0: exception vector                              */
    u8       flags;   /* 1: 0-3: privilege level; 4: clear event enable?  */
    u16      cs;      /* 2: code selector                                 */
    u32      __pad;   /* 4 */
    memory_t address; /* 8: code address                                  */
} PACKED trap_info_t; /* 16 bytes */

typedef struct cpu_user_regs {
    u64 r15;
    u64 r14;
    u64 r13;
    u64 r12;
    union { u64 rbp, ebp; };
    union { u64 rbx, ebx; };
    u64 r11;
    u64 r10;
    u64 r9;
    u64 r8;
    union { u64 rax, eax; };
    union { u64 rcx, ecx; };
    union { u64 rdx, edx; };
    union { u64 rsi, esi; };
    union { u64 rdi, edi; };
    u32 error_code;    /* private */
    u32 entry_vector;  /* private */
    union { u64 rip, eip; };
    u16 cs;
    u8  saved_upcall_mask;
    u8  _pad0[5];
    union { u64 rflags, eflags; };
    union { u64 rsp, esp; };
    u16 ss, _pad1[3];
    u16 es, _pad2[3];
    u16 ds, _pad3[3];
    u16 fs, _pad4[3]; /* Non-zero => takes precedence over fs_base.      */
    u16 gs, _pad5[3]; /* Non-zero => takes precedence over gs_base_user. */
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
    unsigned long event_callback_eip;
    unsigned long failsafe_callback_eip;
    unsigned long syscall_callback_eip;
    unsigned long vm_assist;                /* VMASST_TYPE_* bitmap */
    /* Segment base addresses. */
    u64           fs_base;
    u64           gs_base_kernel;
    u64           gs_base_user;
} vcpu_guest_context_t;

typedef struct {
    /* MFN of a table of MFNs that make up p2m table */
    u64 pfn_to_mfn_frame_list;
} arch_shared_info_t;

typedef struct {
} arch_vcpu_info_t;

#endif /* !__ASSEMBLY__ */

#endif
