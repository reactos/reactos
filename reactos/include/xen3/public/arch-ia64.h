/******************************************************************************
 * arch-ia64/hypervisor-if.h
 * 
 * Guest OS interface to IA64 Xen.
 */

#ifndef __HYPERVISOR_IF_IA64_H__
#define __HYPERVISOR_IF_IA64_H__

// "packed" generates awful code
#define PACKED

/* Pointers are naturally 64 bits in this architecture; no padding needed. */
#define _MEMORY_PADDING(_X)
#define MEMORY_PADDING 

#ifndef __ASSEMBLY__

/* NB. Both the following are 64 bits each. */
typedef unsigned long memory_t;   /* Full-sized pointer/address/memory-size. */
typedef unsigned long cpureg_t;   /* Full-sized register.                    */

typedef struct
{
} PACKED execution_context_t;

/*
 * NB. This may become a 64-bit count with no shift. If this happens then the 
 * structure size will still be 8 bytes, so no other alignments will change.
 */
typedef struct {
    u32  tsc_bits;      /* 0: 32 bits read from the CPU's TSC. */
    u32  tsc_bitshift;  /* 4: 'tsc_bits' uses N:N+31 of TSC.   */
} PACKED tsc_timestamp_t; /* 8 bytes */

#include <asm/tlb.h>	/* TR_ENTRY */

typedef struct {
	unsigned long ipsr;
	unsigned long iip;
	unsigned long ifs;
	unsigned long precover_ifs;
	unsigned long isr;
	unsigned long ifa;
	unsigned long iipa;
	unsigned long iim;
	unsigned long unat;  // not sure if this is needed until NaT arch is done
	unsigned long tpr;
	unsigned long iha;
	unsigned long itir;
	unsigned long itv;
	unsigned long pmv;
	unsigned long cmcv;
	unsigned long pta;
	int interrupt_collection_enabled; // virtual psr.ic
	int interrupt_delivery_enabled; // virtual psr.i
	int pending_interruption;
	int incomplete_regframe;	// see SDM vol2 6.8
	unsigned long delivery_mask[4];
	int metaphysical_mode;	// 1 = use metaphys mapping, 0 = use virtual
	int banknum;	// 0 or 1, which virtual register bank is active
	unsigned long bank0_regs[16]; // bank0 regs (r16-r31) when bank1 active
	unsigned long bank1_regs[16]; // bank1 regs (r16-r31) when bank0 active
	unsigned long rrs[8];	// region registers
	unsigned long krs[8];	// kernel registers
	unsigned long pkrs[8]; // protection key registers
	// FIXME:  These shouldn't be here as they can be overwritten by guests
	// and validation at TLB miss time would be too expensive.
	TR_ENTRY itrs[NITRS];
	TR_ENTRY dtrs[NDTRS];
	TR_ENTRY itlb;
	TR_ENTRY dtlb;
	unsigned long itlb_pte;
	unsigned long dtlb_pte;
	unsigned long irr[4];
	unsigned long insvc[4];
	unsigned long iva;
	unsigned long dcr;
	unsigned long itc;
	unsigned long domain_itm;
	unsigned long domain_timer_interval;
	unsigned long xen_itm;
	unsigned long xen_timer_interval;
//} PACKED arch_shared_info_t;
} arch_vcpu_info_t;		// DON'T PACK 

typedef struct {
} arch_shared_info_t;		// DON'T PACK 

/*
 * The following is all CPU context. Note that the i387_ctxt block is filled 
 * in by FXSAVE if the CPU has feature FXSR; otherwise FSAVE is used.
 */
typedef struct {
    //unsigned long flags;
} PACKED full_execution_context_t;

#endif /* !__ASSEMBLY__ */

#endif /* __HYPERVISOR_IF_IA64_H__ */
