/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    emulx86.h

Abstract:

    This is the private include file for the x86 emulated opcode component.
    Much of this file is a duplicate of .../ntos/vdm/i386/vdm.inc
    with some parts from i386.h

Author:

    Charles Spirakis (intel) 1 Feb 1996

Revision History:

--*/
//
// Prefix Flags
// 
// Copied from .../ntos/vdm/i386/vdm.inc
// The bottom byte originally corresponded to the number of prefixes seen
// which is effectively the length of the instruction...
//
#if	REALLY
#define PREFIX_ES               0x00000100
#define PREFIX_CS               0x00000200
#define PREFIX_SS               0x00000400
#define PREFIX_DS               0x00000800
#define PREFIX_FS               0x00001000
#define PREFIX_GS               0x00002000
#define PREFIX_OPER32           0x00004000
#define PREFIX_ADDR32           0x00008000
#define PREFIX_LOCK             0x00010000
#define PREFIX_REPNE            0x00020000
#define PREFIX_REP              0x00040000
#define PREFIX_SEG_ALL          0x00003f00


//
// Reginfo structure
//
// Similar to .../ntos/vdm/i386/vdm.inc
//

typedef struct _REGINFO {
	ULONG RiSegSs;
	ULONG RiEsp;
	ULONG RiEFlags;
	ULONG RiSegCs;
	ULONG RiEip;
	PKIA32_FRAME RiTrapFrame;
	ULONG RiCsLimit;
	ULONG RiCsBase;
	ULONG RiSsLimit;
	ULONG RiSsBase;
	ULONG RiPrefixFlags;
	// ULONG RiOperand; 		// Not needed when convered to C code
	//
	// These are newly added fields
	//
	ULONG RiInstLength;
	PUCHAR RiLinearAddr;
	UCHAR RiOpcode;
	KXDESCRIPTOR RiCsDescriptor;
	KXDESCRIPTOR RiSsDescriptor;
} REGINFO, *PREGINFO;

#define BOP_OPCODE	0xc4c4

#define DPMISTACK_EXCEPTION_OFFSET 0x1000

//
// And the offset if we include the windows "undocumented feature"
//
#define DPMISTACK_OFFSET	(DPMISTACK_EXCEPTION_OFFSET - 0x20)

#ifdef	NEEDED

//
// Common STYPE's that are checked
//

#define STYPE_DATA	            0x12
#define STYPE_CODE	            0x18
#define STYPE_EXECWRITE_MASK	0x1a

// Because the descriptors are stored in registers and are stored
// in an unscrambled format, Need to define shifts and masks
// to make it easy to get to them

// base is 32 bits
#define UNSCRAM_BASE_OFFSET	0
#define UNSCRAM_BASE_MASK	0x0ffffffff
#define UNSCRAM_GET_BASE(x) ((PUCHAR) ((x) & UNSCRAM_BASE_MASK))

// limit is 20 bits
#define UNSCRAM_LIMIT_OFFSET	32
#define UNSCRAM_LIMIT_MASK	0x0fffff
#define UNSCRAM_GET_LIMIT(x) (((x) >> UNSCRAM_LIMIT_OFFSET) & UNSCRAM_LIMIT_MASK)

// type (in the NT world) includes ths S bit, so
// is really 5 bits
#define UNSCRAM_STYPE_OFFSET	52
#define UNSCRAM_STYPE_MASK	0x1f
#define UNSCRAM_GET_STYPE(x) (((x) >> UNSCRAM_STYPE_OFFSET) & UNSCRAM_STYPE_MASK)

// dpl is 2 bits
#define UNSCRAM_DPL_OFFSET	57
#define UNSCRAM_DPL_MASK	0x3
#define UNSCRAM_GET_DPL(x) (((x) >> UNSCRAM_DPL_OFFSET) & UNSCRAM_DPL_MASK)

// The Present bit is 1 bit
#define UNSCRAM_PRESENT_OFFSET	59
#define UNSCRAM_PRESENT_MASK	0x1
#define UNSCRAM_GET_PRESENT(x) (((x) >> UNSCRAM_PRESENT_OFFSET) & UNSCRAM_PRESENT_MASK)

// The Big bit is 1 bit
#define UNSCRAM_BIG_OFFSET	62
#define UNSCRAM_BIG_MASK	0x1
#define UNSCRAM_GET_BIG(x) (((x) >> UNSCRAM_BIG_OFFSET) & UNSCRAM_BIG_MASK)

// The Granularity is 1 bit
#define UNSCRAM_GRAN_OFFSET	63
#define UNSCRAM_GRAN_MASK	0x1
#define UNSCRAM_GET_GRAN(x) (((x) >> UNSCRAM_GRAN_OFFSET) & UNSCRAM_GRAN_MASK)

// All of the flags fit into 12 bits
#define UNSCRAM_FLAGS_OFFSET	52
#define UNSCRAM_FLAGS_MASK		0xfff
#define UNSCRAM_GET_FLAGS(x) (((x) >> UNSCRAM_FLAGS_OFFSET) & UNSCRAM_FLAGS_MASK)

#define UNSCRAM_EXPAND_IS_DOWN(x)	((((x) >> UNSCRAM_STYPE_OFFSET) & 0x1c) == 0x14)
#define UNSCRAM_GRAN_IS_LARGE(x)	((x) & (UNSCRAM_GRAN_MASK << UNSCRAM_GRAN_OFFSET))
#define UNSCRAM_IS_PRESENT(x)	((x) & (UNSCRAM_PRESENT_MASK << UNSCRAM_PRESENT_OFFSET))
#define UNSCRAM_ACCESS_BIG(x)	((x) & (UNSCRAM_BIG_MASK << UNSCRAM_BIG_OFFSET))

#endif	// NEEDED

// From i386.h
//
// If kernel mode, then
//      let caller specify Carry, Parity, AuxCarry, Zero, Sign, Trap,
//      Direction, Overflow, Interrupt, AlignCheck.
//
// If user mode, then
//      let caller specify Carry, Parity, AuxCarry, Zero, Sign, Trap,
//      Direction, Overflow, AlignCheck.
//      force Interrupts on.
//
// Since there isn't any 386 code running in kernal mode, don't need
// the full sanitize with mode, and change name to make sure we
// don't accidentally use the original one (let the compiler catch the error)
//

#define SANITIZE_FLAGS_IA32(eFlags) (\
        ((((eFlags) & EFLAGS_V86_MASK) && KeIA32VdmIoplAllowed) ? \
        (((eFlags) & KeIA32EFlagsAndMaskV86) | KeIA32EFlagsOrMaskV86) : \
        ((EFLAGS_INTERRUPT_MASK) | ((eFlags) & EFLAGS_USER_SANITIZE)))

extern ULONG KeIA32EFlagsAndMaskV86;
extern ULONG KeIA32EFlagsOrMaskV86;
extern BOOLEAN KeIA32VdmIoplAllowed;
extern ULONG KeIA32VirtualIntExtensions;

#define UNSCRAM_LIMIT_OFFSET	32
#endif
