//--------------------------------------------------------------------------
//
//   real0.s
//
//   Zeroth-level interrupt handling code for PowerPC Little-Endian.
//   This code must reside in real storage beginning at location 0.
//
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

        .file   "real0.s"

//--------------------------------------------------------------------------
//
//   Author:  Rick Simpson
//            IBM Thomas J. Watson Research Center
//            Yorktown Heights, NY
//            simpson@watson.ibm.com
//
//            Peter Johnston
//            IBM - Kirkland Programming Center
//            3600 Carillon Point
//            Kirkland, WA 98033
//            plj@vnet.ibm.com
//
//            Mark Mergen
//            IBM Thomas J. Watson Research Center
//            Yorktown Heights, NY  10598
//            mergen@watson.ibm.com
//
//            Pat Carr
//            RISC Software, Motorola SPS
//            Austin TX 78735
//            patcarr@pets.sps.mot.com
//
//            Ying Chan
//            RISC Software, Motorola SPS
//            Austin TX 78735
//            zulc@pets.sps.mot.com
//
//--------------------------------------------------------------------------
//
//   Fixed mapping of low storage,
//       from PowerPC Operating Environment Architecture
//
//      0x0000 - 0x00FF         (reserved)
//      0x0100 - 0x01FF      System Reset interrupt handler
//      0x0200 - 0x02FF      Machine Check interrupt handler
//      0x0300 - 0x03FF      Data Storage interrupt handler
//      0x0400 - 0x04FF      Instruction Storage interrupt handler
//      0x0500 - 0x05FF      External interrupt handler
//      0x0600 - 0x06FF      Alignment interrupt handler
//      0x0700 - 0x07FF      Program interrupt handler (a.k.a. program check)
//      0x0800 - 0x08FF      Floating Point Unavailable interrupt handler
//      0x0900 - 0x09FF      Decrementer interrupt handler
//      0x0A00 - 0x0BFF        (reserved)
//      0x0C00 - 0x0CFF      System Call interrupt handler
//      0x0D00 - 0x0DFF      Trace interrupt handler
//      0x0E00 - 0x0EFF      Floating Point Assist interrupt handler
//      0x0F00 - 0x0FFF      PMI handler
//
//   The next several handlers are specific to the 603:
//
//      0x1000 - 0x10FF      Instruction Translation Miss handler
//      0x1100 - 0x11FF      Data Store Translation Miss handler -- Load
//      0x1200 - 0x12FF      Data Store Translation Miss handler -- Store
//      0x1300 - 0x13FF      Instruction Address Breakpoint handler
//      0x1400 - 0x14FF      System Management Interrupt handler
//
//      0x1500 - 0x2FFF       (reserved)
//
//   This module is loaded into low-storage at real memory address 0.
//   System memory's address space begins at address 0x80000000 which
//   is mapped to real memory address 0 (see KiSystemInitialization()).
//   This module is designed to be entered in real mode and to switch
//   to virtual mode asap.  Code is compiled to run at VMA 0x80000000.
//
//--------------------------------------------------------------------------

#define mtbatl          mtibatl
#define mtbatu          mtibatu

#include "ksppc.h"

//  Symbolic names for SPRG registers

        .set    sprg.0, 0
        .set    sprg.1, 1
        .set    sprg.2, 2
        .set    sprg.3, 3

// Names for the four bits of a CR field

        .set    LT, 0
        .set    GT, 1
        .set    EQ, 2
        .set    OV, 3

// 601 special purpose register names
        .set    hid1,  1009

// special purpose register names (601, 603 and 604)
        .set    hid0,  1008
        .set    iabr,  1010

// special purpose register names (601, 604)
        .set    dabr,  1013

// 603 special purpose register names

        .set    dmiss,  976
        .set    imiss,  980
        .set    icmp,   981
        .set    rpa,    982

// 604 hid0 bits

        .set    h0_604_ice,     0x8000          // I-Cache Enable
        .set    h0_604_dce,     0x4000          // D-Cache Enable
        .set    h0_604_icl,     0x2000          // I-Cache Lock
        .set    h0_604_dcl,     0x1000          // D-Cache Lock
        .set    h0_604_icia,    0x0800          // I-Cache Invalidate All
        .set    h0_604_dcia,    0x0400          // D-Cache Invalidate All
        .set    h0_604_sse,     0x0080          // Super Scalar Enable
        .set    h0_604_bhte,    0x0004          // Branch History Table enable

        .set    h0_604_prefered, h0_604_ice+h0_604_dce+h0_604_sse+h0_604_bhte

// 613 hid0 bits

        .set    h0_613_ice,     0x8000          // I-Cache Enable
        .set    h0_613_dce,     0x4000          // D-Cache Enable
        .set    h0_613_icia,    0x0800          // I-Cache Invalidate All
        .set    h0_613_dcia,    0x0400          // D-Cache Invalidate All
        .set    h0_613_sge,     0x0080          // Store Gathering enable
	.set	h0_613_dcfa,	0x0040		// Data Cache Flush Assist
	.set	h0_613_btic,    0x0020		// Branch Target Instr enable
        .set    h0_613_bhte,    0x0004          // Branch History Table enable

//
// What we're really going to want in hid0 is
//
//	h0_613_ice+h0_613_dce+h0_613_sge+h0_613_dcfa+h0_613_btic+h0_613_bhte
//
// but these are untried features with currently unknown performance impact,
// so for now use
//
//	h0_613_ice+h0_613_dce++h0_613_btic+h0_613_bhte
//
        .set    h0_613_preferred, h0_613_ice+h0_613_dce+h0_613_btic+h0_613_bhte

// 620 hid0 bits

        .set    h0_620_ice,     0x8000          // I-Cache Enable
        .set    h0_620_dce,     0x4000          // D-Cache Enable
        .set    h0_620_icia,    0x0800          // I-Cache Invalidate All
        .set    h0_620_dcia,    0x0400          // D-Cache Invalidate All
        .set    h0_620_sse,     0x0080          // Super Scalar Enable
        .set    h0_620_bpm,     0x0020          // Dynamic branch prediction
        .set    h0_620_ifm,     0x0018          // Allow off-chip Instr. fetch

        .set    h0_620_prefered, h0_620_ice+h0_620_dce+h0_620_sse+h0_620_bpm+h0_620_ifm

// Known PPC versions:

        .set    PV601,    1
        .set    PV603,    3
        .set    PV604,    4
        .set    PV603p,   6       // 603e, Stretch
        .set    PV603pp,  7       // 603ev, Valiant
        .set    PV613,    8       // 613, aka Arthur
        .set    PV604p,   9       // 604+
        .set    PV620,   20

//--------------------------------------------------------------------------
//
//  Globally-used constants and variables
//
//--------------------------------------------------------------------------

// external variables

        .extern KdpOweBreakpoint
        .extern KeGdiFlushUserBatch
        .extern KeServiceDescriptorTable
        .extern KeTickCount
        .extern KiBreakPoints
        .extern PsWatchEnabled


// external procedures in ntoskrnl

        .extern ..DbgBreakPoint
        .extern ..KdSetOwedBreakpoints
        .extern ..KeBugCheck
        .extern ..KeBugCheckEx
        .extern ..KiDeliverApc
        .extern ..KiDispatchException
        .extern ..KiDispatchSoftwareIntDisabled
        .extern ..KiIdleLoop
        .extern ..KiInitializeKernel
        .extern ..MmAccessFault
        .extern ..PsConvertToGuiThread
        .extern ..PsWatchWorkingSet
        .extern ..RtlpRestoreContextRfiJump


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//
//    Beginning of fixed-storage area (real page 0)
//
//    Zeroth-Level Interrupt Handlers
//
//
//    These routines are located at hardware-mandated addresses.  Each
//    one is the target of a particular interrupt:  the hardware saves
//    the current instruction address in SRR0, the MSR (or most of it)
//    in SRR1, and branches to the start of one of these routines.
//
//    When entered, each of these routines is running with Instruction
//    Relocate OFF, Data Relocate OFF, and External Interrupts disabled.
//    It is the task of each Zeroth-Level Interrupt Handler to get back
//    into "relocate on" (both IR and DR) as soon as possible.  Turning
//    on IR is tricky because the kernel is not mapped V=R.  The "ZLIHs"
//    must be in Real Page 0, but Virtual Page 0 belongs to user space;
//    the kernel resides at Virtual address 0x80000000.  If the ZLIH
//    just turns on IR and DR, it will suddenly start executing code at
//    address 0x100 or so in user space (in supervisor state!).  On the
//    other hand, the ZLIH can't branch to the kernel at 0x80000000,
//    because that address doesn't even exist while IR is off.
//
//    The trick is to save the incoming SRR0 and SRR1, load up SRR0 and
//    SRR1 with a "new PSW" pointing to a First-Level Interrupt Handler
//    in the kernel's virtual space, and use "return from interrupt" to
//    both set IR (and DR) to 1 and branch to the proper virtual address
//    all in one go.
//
//    The code assembled here must reside at Real Address 0, and also in
//    the kernel's virtual space (presumably at 0x80000000, but that is
//    not required).
//
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

//
//  The following macros, zlih() and short_zlih(), generate the body of the
//  code for the Zeroth-Level Interrupt Handlers.
//
//  Each must be preceeded by an ".org" to the proper machine-mandated
//  address.  The ".org" can be followed by special fast-path interrupt
//  handling code, if appropriate, before the zlih() or short_zlih() is
//  coded.  (Only System Call and D/I Storage interrupts do this at present.)
//
//  zlih(code)
//  short_zlih(code)
//
//      code:   constant identifying the exception type
//
//  on Entry
//      MSR:    External interrupts disabled
//              Instruction Relocate OFF
//              Data Relocate OFF
//      SRR0:   Next instruction address at time of interrupt
//      SRR1:   MSR at time of interrupt
//
//  Exits to First-Level Interrupt Handler, with
//      MSR:    External interrupts disabled
//              Instruction Relocate ON
//              Data Relocate ON
//      GP registers:
//        r.2:  Constant identifying the interrupt type
//        r.3:  Saved SRR0 (interrupt address)
//        r.4:  Saved SRR1 (MSR value)
//        r.5:  -available-
//        r.11: -available-
//      In the PCR:
//        PcGprSave[0]:  Saved r.2
//        PcGprSave[1]:  Saved r.3
//        PcGprSave[2]:  Saved r.4
//        PcGprSave[3]:  Saved r.5
//        PcGprSave[5]:  Saved r.11
//
//      Nothing is left in the SPRG's

        .set    PCR_SAVE2, PcGprSave + 0
        .set    PCR_SAVE3, PcGprSave + 4
        .set    PCR_SAVE4, PcGprSave + 8
        .set    PCR_SAVE5, PcGprSave + 12
        .set    PCR_SAVE6, PcGprSave + 16
        .set    PCR_SAVE11, PcGprSave + 20

        .set    FLIH_MSR, 0x00013031            // ILE, FP, ME, IR, DR, LE bits in MSR
        .set    INT_ENA, MASK_SPR(MSR_EE,1)     // MSR External Interrupt Enable

//
// Note: propagate MSR[PM] bit into the MSR we load.
//
#if !DBG_STORE
#define zlih(code)                                              \
        mtsprg  sprg.2, r.5                                    ;\
        mfsprg  r.5, sprg.0                                    ;\
        stw     r.4, PCR_SAVE4 (r.5)                           ;\
        lwz     r.4, common_exception_entry.ptr - real0 (0)    ;\
        stw     r.3, PCR_SAVE3 (r.5)                           ;\
        mfsrr0  r.3                                            ;\
        mtsrr0  r.4                                            ;\
        mfsrr1  r.4                                            ;\
        stw     r.2, PCR_SAVE2 (r.5)                           ;\
        lis     r.2, FLIH_MSR >> 16                            ;\
        ori     r.2, r.2, FLIH_MSR & 0xFFFF                    ;\
        rlwimi	r.2, r.4, 0, MSR_PM, MSR_PM                    ;\
        mtsrr1  r.2                                            ;\
        stw     r.11, PCR_SAVE11 (r.5)                         ;\
        mfsprg  r.11, sprg.2                                   ;\
        li      r.2, code                                      ;\
        stw     r.11, PCR_SAVE5 (r.5)                          ;\
        rfi
#else
#define zlih(code)                                              \
        mtsprg  sprg.2, r.5                                    ;\
        mfsprg  r.5, sprg.0                                    ;\
        stw     r.4, PCR_SAVE4 (r.5)                           ;\
        stw     r.3, PCR_SAVE3 (r.5)                           ;\
        DBGSTORE_I_R(r3,r4,code)                               ;\
        lwz     r.4, common_exception_entry.ptr - real0 (0)    ;\
        mfsrr0  r.3                                            ;\
        mtsrr0  r.4                                            ;\
        mfsrr1  r.4                                            ;\
        stw     r.2, PCR_SAVE2 (r.5)                           ;\
        lis     r.2, FLIH_MSR >> 16                            ;\
        ori     r.2, r.2, FLIH_MSR & 0xFFFF                    ;\
        rlwimi	r.2, r.4, 0, MSR_PM, MSR_PM                    ;\
        mtsrr1  r.2                                            ;\
        stw     r.11, PCR_SAVE11 (r.5)                         ;\
        mfsprg  r.11, sprg.2                                   ;\
        li      r.2, code                                      ;\
        stw     r.11, PCR_SAVE5 (r.5)                          ;\
        rfi
#endif

#if !DBG_STORE
#define short_zlih(code)                                        \
        mtsprg  sprg.3, r.2                                    ;\
        li      r.2, code                                      ;\
        b       short_zlih_continue
#else
#define short_zlih(code)                                        \
        mtsprg  sprg.3, r.2                                    ;\
        mtsprg  sprg.2, r.3                                    ;\
        DBGSTORE_I_R(r2,r3,code)                               ;\
        mfsprg  r.3, sprg.2                                    ;\
        li      r.2, code                                      ;\
        b       short_zlih_continue
#endif

//--------------------------------------------------------------------------
//
// List of internal codes used to distinguish types of interrupts
// within real0.s, before converting to standard Windows NT
// "STATUS_..." code for KiDispatchException.
//
// These values are offsets into a branch table in common_exception_entry.
// That table MUST be updated if any entries here are added/deleted/changed.
//
        .set    CODE_MACHINE_CHECK,   0
        .set    CODE_EXTERNAL,        4
        .set    CODE_DECREMENTER,     8
        .set    CODE_STORAGE_ERROR,  12 // after dsi or isi tests
        .set    CODE_PAGE_FAULT,     16 // after dsi or isi hpt miss code
        .set    CODE_ALIGNMENT,      20
        .set    CODE_PROGRAM,        24
        .set    CODE_FP_UNAVAIL,     28
        .set    CODE_DIRECT_STORE,   32
        .set    CODE_SYSTEM_CALL,    36
        .set    CODE_TRACE,          40
        .set    CODE_FP_ASSIST,      44
        .set    CODE_RUN_MODE,       48
        .set    CODE_PANIC,          52
        .set    CODE_SYSTEM_MGMT,    56
        .set    CODE_DATA_BREAKPOINT,60
        .set    CODE_PMI,            64


//
// Code from here thru end_of_code_to_move is copied to low memory
// at system initialization.   This code is declared in the INIT
// section so the space can be used for other purposes after system
// initialization.
//
        .new_section INIT,"rcx6"        // force 64 byte alignment
                                        // for text in this module.
        .section INIT,"rcx6"
        .globl  real0
        .org    0
real0:
        .asciiz "PowerPC"

//-------------------------------------------------------------
//
//  Machine Check Interrupt
//
//  Machine check zeroth level interrupt handler is the same
//  as handlers using the macro EXCEPT that we don't reenable
//  machine check exceptions.
//
//-------------------------------------------------------------

        .org    0x200

        mtsprg  sprg.2, r.5
        mfsprg  r.5, sprg.0
        stw     r.4, PCR_SAVE4 (r.5)
#if !DBG_STORE
        lwz     r.4, common_exception_entry.ptr - real0 (0)
        stw     r.3, PCR_SAVE3 (r.5)
#else
        stw     r.3, PCR_SAVE3 (r.5)
        DBGSTORE_I_R(r3,r4,0x200)
        lwz     r.4, common_exception_entry.ptr - real0 (0)
#endif
        mfsrr0  r.3
        mtsrr0  r.4
        mfsrr1  r.4
        stw     r.2, PCR_SAVE2 (r.5)
        LWI(r.2,(FLIH_MSR&~MASK_SPR(MSR_ME,1))) // don't reenable machine check
        rlwimi	r.2, r.4, 0, MSR_PM, MSR_PM     // Preserve MSR[PM] bit
        mtsrr1  r.2
        stw     r.11, PCR_SAVE11 (r.5)
        mfsprg  r.11, sprg.2
        li      r.2,  CODE_MACHINE_CHECK
        stw     r.11, PCR_SAVE5 (r.5)
        rfi

//-------------------------------------------------------------
//
//  Data Storage Interrupt
//
//-------------------------------------------------------------

       .set     K_BASE,0x8000           // virtual address of kernel
       .set     SREG_INVAL,0x80         // software invalid sreg bit (really 0x00800000)
       .set     PTE_VALID,4             // software pte valid bit
       .set     HPT_LOCK,0x04fc         // real addr of hpt lock word
       .set	HPT_MASK,0x2100		// real addr of hpt masks for 64-bit
       .set     PTE_CHANGE, 0x0080      // TLB Change bit
       .set     PTE_COHERENCY, 0x0010   // Coherency required (WIMG(M)=1)
       .set     PTE_GUARDED, 0x8        // Guarded Storage (WIMG[G] == 1)

       .org     0x300

dsientry:
        mtsprg  sprg.2,r.1              // save gpr 1
        mfsprg  r.1,sprg.0              // get addr of processor ctl region
        stw     r.4,PcSiR4(r.1)         // save gpr 4
        stw     r.2,PcSiR2(r.1)         // save gpr 2
        INC_CTR(CTR_DSI,r1,r2)
        DBGSTORE_I_R(r2,r4,0x300)
        mfcr    r.4                     // save condition reg
        mfdsisr r.2                     // get data stg int status reg
        stw     r.0,PcSiR0(r.1)         // save gpr 0
        andis.  r.0,r.2,0x8cf0          // dsi other than page translation?
        mfdar   r.0                     // get failing addr in data addr reg
        rlwimi  r.0,r.2,7,0x00000001    // save st/l in low failing addr bit
        bne-    dsioth                  // branch if yes

        INC_CTR(CTR_DSI_HPT_MISS,r1,r2)

dsi10:  b       tpte			// goto test page table entry
dsi20:  b       tpte64


//-------------------------------------------------------------
//
//  Instruction Storage Interrupt
//
//-------------------------------------------------------------

       .org     0x400

isientry:
        mtsprg  sprg.2,r.1              // save gpr 1
        mfsprg  r.1,sprg.0              // get addr of processor ctl region
        stw     r.4,PcSiR4(r.1)         // save gpr 4
        stw     r.2,PcSiR2(r.1)         // save gpr 2
        INC_CTR(CTR_ISI,r1,r2)
        DBGSTORE_I_R(r2,r4,0x400)
        mfcr    r.4                     // save condition reg
        mfsrr1  r.2                     // get save/restore reg 1
        stw     r.0,PcSiR0(r.1)         // save gpr 0
        andis.  r.0,r.2,0x1820          // isi other than page translation?
        mfsrr0  r.0                     // get failing addr in sav/res reg 0
        bne     isioth                  // branch if yes

        INC_CTR(CTR_ISI_HPT_MISS,r1,r2)

isi10:  b       tpte			// goto test page table entry
isi20:  b       tpte64

       .org     0x4fc                   // HPT_LOCK in real0.s, miscasm.s
       .long    0                       // hash page table lock word

//-------------------------------------------------------------
//
//  External Interrupt
//
//-------------------------------------------------------------

        .org    0x500

        zlih(CODE_EXTERNAL)

//-------------------------------------------------------------
//
//  Alignment Interrupt
//
//-------------------------------------------------------------

        .org    0x600

        mtsprg  sprg.2,r.2              // save r2
        mtsprg  sprg.3,r.1              // save r1
        DBGSTORE_I_R(r1,r2,0x600)
        mfsprg  r.1,sprg.0              // get PCR addr
        mfdar   r.2                     // save DAR
        stw     r.2,PcSavedV0(r.1)      //   in PCR
        mfdsisr r.2                     // save DSISR
        stw     r.2,PcSavedV1(r.1)      //   in PCR
        mfsprg  r.2,sprg.2              // reload r2
        mfsprg  r.1,sprg.3              // reload r1
        zlih(CODE_ALIGNMENT)

//-------------------------------------------------------------
//
//  Program Interrupt
//
//-------------------------------------------------------------

        .org    0x700

        zlih(CODE_PROGRAM)


//
//      The following word contains the absolute address of
//      an instruction in the routine SwapContext.  It is
//      here so we can find it while we have almost no GPRs
//      available during the early stage of exception processing.
//
KepSwappingContextAddr:
        .extern KepSwappingContext
        .long   KepSwappingContext

//-------------------------------------------------------------
//
//  Floating Point Unavailable Interrupt
//
//-------------------------------------------------------------

        .org    0x800

//
//  For now, we don't attempt to lock the floating point unit.
//  If a floating point instruction is issued with FP unavailable,
//  it will interrupt to this location.  We turn on the FP availability
//  bit and resume execution.
//

        mtsprg  sprg.2, r.3
#if DBG_STORE
        mtsprg  sprg.3,r4
        DBGSTORE_I_R(r3,r4,0x800)
        mfsprg  r4,sprg.3
#endif
        mfsrr1  r.3
        ori     r.3, r.3, 0x2000
        mtsrr1  r.3
        mfsprg  r.3, sprg.2
        rfi


//      zlih(CODE_FP_UNAVAIL)

//-------------------------------------------------------------
//
//  Decrementer Interrupt
//
//-------------------------------------------------------------

        .org    0x900

        zlih(CODE_DECREMENTER)

//-------------------------------------------------------------
//
//  Direct Store Interrupt
//
//-------------------------------------------------------------

        .org    0xA00

        zlih(CODE_DIRECT_STORE)

//-------------------------------------------------------------
//
//  System Call Interrupt
//
//  Since System Call is really a "call", we need not preserve
//  volatile registers as the other interrupt handlers must.
//
//  Also, return from system call is to address in Link Register
//  so no need to save srr0 (exception address).
//
//  However, arguments are in r.3 thru r.10 so don't trash them.
//
//  Incoming value in r.2 (normally the TOC pointer) indicates
//  the system service being requested.
//
//-------------------------------------------------------------

        .org    0xC00

        DBGSTORE_I_R(r12,r11,0xc00)
        lwz     r.0, system_service_dispatch.ptr-real0(0)
        mfsrr1  r.12                          // save previous mode
        li      r.11, FLIH_MSR & 0xffff       // set low 16 bits of kernel mode
        rlwimi	r.11, r.12, 0, MSR_PM, MSR_PM // propagate MSR[PM]
        mtsrr1  r.11
        mtsrr0  r.0                           // set kernel entry address
        extrwi. r.11, r.12, 1, MSR_PR         // extract user mode
        rfi                                   // enter kernel


//-------------------------------------------------------------
//
//  Trace Interrupt
//
//-------------------------------------------------------------

        .org    0xD00

        zlih(CODE_TRACE)

//--------------------------------------------------------------------------
//
//  Floating Point Assist Zeroth-Level Interrupt Handler (optional; not 601)
//
//--------------------------------------------------------------------------

        .org    0xE00

        short_zlih(CODE_FP_ASSIST)


//--------------------------------------------------------------------------
//
//  PMI Interrupt (604)
//
//  N.B.  Some versions of the 604 do not turn off ENINT in MMCR0 when
//        signaling the PM interrupt.  Therefore interrupts must not be
//        enabled before the spot in the (external) PM interrupt handler
//        where ENINT is turned off.  This implies that one must not set
//        breakpoints or make calls to DbgPrint anywhere along the path
//        from here to the PM interrupt handler.
//
//--------------------------------------------------------------------------

        .org    0xF00

        short_zlih(CODE_PMI)

//--------------------------------------------------------------------------
//
//  Instruction Translation Miss (603 only)
//
//--------------------------------------------------------------------------

        .org    0x1000

        DBGSTORE_I_R(r1,r2,0x1000)
        mfsprg  r.1,sprg.0              // get physical address of PCR
        INC_CTR(CTR_ITLB_MISS,r1,r2)
        mfspr   r.0,imiss               // get faulting address
        lwz     r.2,PcPgDirRa(r.1)      // get process' PDE page
        mfsrin  r.3,r.0                 // get sreg of failing addr
        andis.  r.3,r.3,SREG_INVAL      // sreg invalid?
        bne     stgerr603               // branch if yes
        rlwimi  r.2,r.0,12,0x00000ffc   // calculate effective PDE address
        lwz     r.2,0(r.2)              // get effective PDE
        andi.   r.3,r.2,PTE_VALID       // check for valid PDE
        beq     pgf603                  // invalid --> can't just load TLB
        rlwinm  r.2,r.2,0,0xfffff000    // get real addr of page table page
        rlwimi  r.2,r.0,22,0x00000ffc   // calculate effective PTE address
        lwz     r.2,0(r.2)              // get effective PTE
        andi.   r.3,r.2,PTE_VALID       // check for valid PTE
        beq     pgf603                  // invalid --> can't just load TLB

        INC_CTR(CTR_ITLB_MISS_VALID_PTE,r1,r3)
        mtspr   rpa,r.2                 // present translation information
        mfsrr1  r.2
        tlbli   r.0                     // set translation for fault addr
        mtcrf   0x80,r.2                // restore CR0 at time of miss
        rfi

//--------------------------------------------------------------------------
//
//  Data Load Translation Miss (603 only)
//
//--------------------------------------------------------------------------

        .org    0x1100

        DBGSTORE_I_R(r1,r2,0x1100)
dtlbmiss:
        mfsprg  r.1,sprg.0              // get physical address of PCR
        INC_CTR(CTR_DTLB_MISS,r1,r2)
        mfspr   r.0,dmiss               // get faulting address
        lwz     r.2,PcPgDirRa(r.1)      // get process' PDE page
        mfsrin  r.3,r.0                 // get sreg of failing addr
        andis.  r.3,r.3,SREG_INVAL      // sreg invalid?
        bne     dstgerr603              // branch if yes
s15ok603:
        rlwimi  r.2,r.0,12,0x00000ffc   // calculate effective PDE address
        lwz     r.2,0(r.2)              // get effective PDE
        andi.   r.3,r.2,PTE_VALID       // check for valid PDE
        beq     pgf603                  // invalid --> can't just load TLB
        rlwinm  r.2,r.2,0,0xfffff000    // get real addr of page table page
        rlwimi  r.2,r.0,22,0x00000ffc   // calculate effective PTE address
        lwz     r.2,0(r.2)              // get effective PTE
        andi.   r.3,r.2,PTE_VALID       // check for valid PTE
        beq     pgf603                  // invalid --> can't just load TLB

//
// Blindly set the change bit in the PTE so the h/w won't feel obliged
// to interrupt to let us know a page has been written to.  Also, set
// the Coherency required bit (WIMG(M)=1) because it should be set.
//

tlbld603:
//
// 603e/ev Errata 19 work around.  The following instruction is modified
// at init time to include PTE_GUARDED if this is a 603e/ev.
//
        ori     r.2,r.2,PTE_CHANGE|PTE_COHERENCY
        INC_CTR(CTR_DTLB_MISS_VALID_PTE,r1,r3)
        mtspr   rpa,r.2                 // present translation information
        mfsrr1  r.2
        tlbld   r.0                     // set translation for fault addr
        mtcrf   0x80,r.2                // restore CR0 at time of miss
        rfi

//
//--------------------------------------------------------------------------
//
//  Data Store Translation Miss or Change Bit == 0 Exception (603 only)
//
//--------------------------------------------------------------------------

        .org    0x1200

        DBGSTORE_I_R(r1,r2,0x1200)
        b       dtlbmiss


//--------------------------------------------------------------------------
//
//  Instruction Address Breakpoint (603 only)
//
//--------------------------------------------------------------------------

        .org    0x1300

        zlih(CODE_RUN_MODE)

//--------------------------------------------------------------------------
//
//  System Management Interrupt (603 only) -- Power Management
//
//--------------------------------------------------------------------------

        .org    0x1400

        zlih(CODE_SYSTEM_MGMT)

//--------------------------------------------------------------------------
//
//  Run Mode Zeroth-Level Interrupt Handler (601 specific)
//
//--------------------------------------------------------------------------

        .org    0x2000

        zlih(CODE_RUN_MODE)


//
//--------------------------------------------------------------------------
//
//  HPT mask array for 64-bit memory management model
//
//--------------------------------------------------------------------------

        .org    0x2100

        .long   0x00000000
        .long   0x00000001
        .long   0x00000003
        .long   0x00000007
        .long   0x0000000f
        .long   0x0000001f
        .long   0x0000003f
        .long   0x0000007f
        .long   0x000000ff
        .long   0x000001ff
        .long   0x000003ff
        .long   0x000007ff
        .long   0x00000fff
        .long   0x00001fff
        .long   0x00003fff
        .long   0x00007fff
        .long   0x0000ffff
        .long   0x0001ffff
        .long   0x0003ffff
        .long   0x0007ffff
        .long   0x000fffff
        .long   0x001fffff
        .long   0x003fffff
        .long   0x007fffff
        .long   0x00ffffff
        .long   0x01ffffff
        .long   0x03ffffff
        .long   0x07ffffff
        .long   0x0fffffff
        .long   0x1fffffff
        .long   0x3fffffff
        .long   0x7fffffff
        .long   0xffffffff


//--------------------------------------------------------------------------
//
//  Reserved space from end of FLIHs to location 0x3000
//
//--------------------------------------------------------------------------

        .org    0x3000

//--------------------------------------------------------------------------
//
//  Address constants needed in low memory (ie memory which can
//  be addressed absolutely without difficulty).
//
//--------------------------------------------------------------------------

        .align  6                       // ensure cache line alignment

common_exception_entry.ptr:
        .long   common_exception_entry
system_service_dispatch.ptr:
        .long   system_service_dispatch
FpZero:
        .double 0                   // doubleword of 0's for clearing FP regs

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//
//  End fixed-storage area
//
//  Beyond this point nothing need appear at machine-dictated addresses
//
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
//
//  Continuation of Data Storage and Instruction Storage interrupts
//
//--------------------------------------------------------------------------

tpte:   stw     r.3,PcSiR3(r.1)         // save gpr 3
        stw     r.5,PcSiR5(r.1)         // save gpr 5
        lwz     r.2,PcPgDirRa(r.1)      // get real addr of page dir page
        mfsrin  r.1,r.0                 // get sreg of failing addr
        andis.  r.3,r.1,SREG_INVAL      // sreg invalid?
        bne     stgerr                  // branch if yes
s15ok:
        rlwimi  r.2,r.0,12,0x00000ffc   // insert pde index in pd page addr
        lwz     r.2,0(r.2)              // get page directory entry
        andi.   r.3,r.2,PTE_VALID       // pde valid?
        rlwinm  r.5,r.2,0,0xfffff000    // get real addr of page table page
        beq     pfault                  // branch if not
        rlwimi  r.5,r.0,22,0x00000ffc   // insert pte index in pt page addr
        lwz     r.2,0(r.5)              // get page table entry
        andi.   r.3,r.2,PTE_VALID       // pte valid?
        beq     pfault                  // branch if not
lpte:
        mtsprg  sprg.3,r.6              // save gpr 6
        rlwinm  r.3,r.0,20,0x0000ffff   // align failing vpi with vsid
        ori     r.2,r.2,0x190           // force RC and M bits in PTE
        xor     r.3,r.1,r.3             // hash - exclusive or vsid with vpi
        rlwimi  r.1,r.0,3,0x7e000000    // insert api into reg with vsid
        rlwinm  r.1,r.1,7,0xffffffbf    // align vsid,api as 1st word hpte
        mfsdr1  r.6                     // get storage description reg
        oris    r.1,r.1,0x8000          // set valid bit in 1st word hpte
        rlwinm  r.0,r.6,10,0x0007fc00   // align hpt mask with upper hash
        ori     r.0,r.0,0x03ff          // append lower one bits to mask
        and     r.0,r.0,r.3             // take hash modulo hpt size
        rlwinm  r.0,r.0,6,0x01ffffc0    // align hash as hpt group offset
#if !defined(NT_UP)
        li      r.3,HPT_LOCK            // get hpt lock address
getlk:  lwarx   r.5,0,r.3               // load and reserve lock word
        cmpwi   r.5,0                   // is lock available?
        mfsprg  r.5,sprg.0              // get processor ctl region addr
        bne-    getlk_spin              // loop if lock is unavailable
        stwcx.  r.5,0,r.3               // store conditional to lock word
        bne-    getlk_spin              // loop if lost reserve
        isync                           // context synchronize
#endif // NT_UP
        rlwinm  r.3,r.6,0,0xffff0000    // get real addr of hash page table
        or      r.3,r.0,r.3             // or with offset to get group addr
        INC_GRP_CTR_R(GRP_CTR_DSI_VALID_PTE,r3)
#if !defined(HPT_AS_TLB_RELOAD_BUFFER)
        ori     r.3,r.3,0x38            // point to last entry in group
        li      r.6,0                   // set no invalid hpte found
#endif
        b       thpte                   // goto test hash page table entry

#if !defined(NT_UP)
getlk_spin:
        lwz     r.5,0(r.3)
        cmpwi   r.5,0
        beq+    getlk
        b       getlk_spin
#endif

#if !defined(HPT_AS_TLB_RELOAD_BUFFER)
sirem:
        ori     r.6,r.3,0               // remember invalid hpte address
phpte:  andi.   r.0,r.3,0x003f          // tested all hptes in prim group?
        subi    r.3,r.3,8               // decrement to previous hpte
        beq     sinom                   // branch if yes
thpte:  lwz     r.0,4(r.3)              // get 1st(be) word of hpte
        andis.  r.5,r.0,0x8000          // hpte valid?
        beq     sirem                   // jump if no to remember
        cmplw   r.1,r.0                 // does hpte match search arg?
        bne     phpte                   // loop if no to previous hpte
#if 0
        lwz     r2,0x3900(0)
        addi    r2,r2,1
        stw     r2,0x3900(0)
#endif
        INC_GRP_CTR_R(GRP_CTR_DSI_FOUND,r3)
        b       skiphpte                // hpte already present -- nothing to do
#else
thpte:  lwz     r.0,4(r.3)              // get 1st(be) word of hpte
        andis.  r.5,r.0,0x8000          // hpte valid?
        beq     siinv                   // jump if no
        cmplw   r.1,r.0                 // does hpte match search arg?
        bne     sisto                   // jump if no
        INC_GRP_CTR_R(GRP_CTR_DSI_FOUND,r3)
        b       skiphpte                // hpte already present -- nothing to do
sisto:
        clrlwi  r.0,r.0,1               // turn off valid bit
        stw     r.0,4(r.3)              // invalidate 1st(be) wd victim hpte
        sync                            // ensure 1st word stored
siinv:
        stw     r.2,0(r.3)              // store pte as 2nd(be) wd hpte
        sync                            // ensure 2nd word stored
        stw     r.1,4(r.3)              // store vsid,api as 1st(be) wd hpte
#endif

#if !defined(HPT_AS_TLB_RELOAD_BUFFER)
sinom:
        cmplwi  r.6,0                   // was an invalid hpte found?
        beq     primov                  // branch if not
shpte:  stw     r.2,0(r.6)              // store pte as 2nd(be) wd hpte
        sync                            // ensure 2nd word stored
        stw     r.1,4(r.6)              // store vsid,api as 1st(be) wd hpte
#endif
skiphpte:

#if !defined(NT_UP)
        li      r.0,0                   // get a zero value
        sync                            // ensure all previous stores done
        stw     r.0,HPT_LOCK(0)         // store zero in hpt lock word
#endif // NT_UP
        mfsprg  r.6,sprg.3              // reload saved gpr 6
        mfsprg  r.1,sprg.0              // get addr of processor ctl region
        mtcrf   0xff,r.4                // reload condition reg
        lwz     r.5,PcSiR5(r.1)         // reload saved gpr 5
        lwz     r.4,PcSiR4(r.1)         // reload saved gpr 4
        lwz     r.3,PcSiR3(r.1)         // reload saved gpr 3
        lwz     r.2,PcSiR2(r.1)         // reload saved gpr 2
        lwz     r.0,PcSiR0(r.1)         // reload saved gpr 0
        mfsprg  r.1,sprg.2              // reload saved gpr 1
        rfi                             // return from interrupt

#if !defined(HPT_AS_TLB_RELOAD_BUFFER)
primov: mfdec   r.5                     // get decrementer
        addi    r.6,r.3,8               // recompute primary hpt group addr
        INC_GRP_CTR_R(GRP_CTR_DSI_FULL,r6)
        rlwimi  r.6,r.5,28,0x00000038   // choose 1 of 8 hptes as victim
#if !defined(PRESERVE_HPTE_CONTENTS)
        li      r.0,0
#else
        lwz     r.0,4(r.6)              // get 1st(be) word of victim hpte
        clrlwi  r.0,r.0,1               // turn off valid bit
#endif
        stw     r.0,4(r.6)              // invalidate 1st(be) wd victim hpte
        sync                            // ensure 1st word stored
        b       shpte
#endif

dsioth: rlwinm  r.2,r.2,16,0x0000ffff   // rotate dsisr bits into low half
        cmplwi  r.2,0x0a00              // dsi from protect against store?
        beq+    sfault                  // branch if yes
        andi.   r.2,r.2,0x40            // check for data store bkp
        bne     dsbkp                   // branch if data store bkp
        b       isioth                  // goto join other error processing
stgerr:
        clrrwi  r.3,r.0,PAGE_SHIFT      // get page address of fault
        mfsprg  r.2,sprg.0              // get phys addr of processor ctl region
        cmpwi   r.3,0xffffd000          // is fault in PCR page?
#if !COLLECT_PAGING_DATA
        ori     r.2, r.2, 1             // user readonly
        beq     lpte                    // branch if yes
#else
        bne     stgerr_not_pcr
        INC_CTR(CTR_PCR,r2,r3)
        ori     r.2, r.2, 1             // user readonly
        b       lpte
stgerr_not_pcr:
#endif
        clrrwi  r.2, r.2, 1             // get PCR address back
        lwz     r.2, PcPcrPage2(r.2)    // get phys addr of PCR2
        cmpwi   r.3,0xffffe000          // is fault in PCR2 page?
#if !COLLECT_PAGING_DATA
        ori     r.2, r.2, 1             // user readonly
        beq     lpte                    // branch if yes
#else
        bne     stgerr_not_pcr2
        mfsprg  r.2,sprg.0              // get phys addr of processor ctl region
        INC_CTR(CTR_PCR2,r2,r3)
        lwz     r.2, PcPcrPage2(r.2)    // get phys addr of PCR2
        ori     r.2, r.2, 1             // user readonly
        b       lpte
stgerr_not_pcr2:
#endif
        mfsprg  r.2,sprg.0              // get phys addr of PCR
        rlwinm  r.3, r.3, 4, 0xf        // Check sreg #
        lwz     r.2, PcPgDirRa(r.2)     // Page Directory addr
        cmpwi   r.3, 0xf                // sreg 15
        beq     s15ok

report_stgerr:
        mfsprg  r.1,sprg.0              // get addr of processor ctl region
        lwz     r.3,PcSiR3(r.1)         // reload saved gpr 3
        lwz     r.5,PcSiR5(r.1)         // reload saved gpr 5
isioth: stw     r.0,PcBadVaddr(r.1)     // save failing addr and st/l bit
        mtcrf   0xff,r.4                // reload condition reg
        INC_CTR (CTR_STORAGE_ERROR,r1,r4)
        lwz     r.4,PcSiR4(r.1)         // reload saved gpr 4
        lwz     r.2,PcSiR2(r.1)         // reload saved gpr 2
        lwz     r.0,PcSiR0(r.1)         // reload saved gpr 0
        mfsprg  r.1,sprg.2              // reload saved gpr 1
        short_zlih(CODE_STORAGE_ERROR)

pfault: mfsprg  r.1,sprg.0              // get addr of processor ctl region
        lwz     r.3,PcSiR3(r.1)         // reload saved gpr 3
        lwz     r.5,PcSiR5(r.1)         // reload saved gpr 5
sfault: stw     r.0,PcBadVaddr(r.1)     // save failing addr and st/l bit
        mtcrf   0xff,r.4                // reload condition reg
        INC_CTR (CTR_PAGE_FAULT,r1,r4)
        lwz     r.4,PcSiR4(r.1)         // reload saved gpr 4
        lwz     r.2,PcSiR2(r.1)         // reload saved gpr 2
        lwz     r.0,PcSiR0(r.1)         // reload saved gpr 0
        mfsprg  r.1,sprg.2              // reload saved gpr 1
        short_zlih(CODE_PAGE_FAULT)

dsbkp:  stw     r.0,PcBadVaddr(r.1)     // save failing addr and st/l bit
        mtcrf   0xff,r.4                // reload condition reg
        lwz     r.4,PcSiR4(r.1)         // reload saved gpr 4
        lwz     r.2,PcSiR2(r.1)         // reload saved gpr 2
        lwz     r.0,PcSiR0(r.1)         // reload saved gpr 0
        mfsprg  r.1,sprg.2              // reload saved gpr 1
        short_zlih(CODE_DATA_BREAKPOINT)


//--------------------------------------------------------------------------
//
//  Continuation of 64-bit memory management implementation
//
//--------------------------------------------------------------------------

tpte64: stw     r.3,PcSiR3(r.1)         // save gpr 3
        stw     r.5,PcSiR5(r.1)         // save gpr 5
        lwz     r.2,PcPgDirRa(r.1)      // get real addr of page dir page
        mfsrin  r.1,r.0                 // get sreg of failing addr
        andis.  r.3,r.1,SREG_INVAL      // sreg invalid?
        bne     stgerr64                // branch if yes
s15ok64:
        rlwimi  r.2,r.0,12,0x00000ffc   // insert pde index in pd page addr
        lwz     r.2,0(r.2)              // get page directory entry
        andi.   r.3,r.2,PTE_VALID       // pde valid?
        rlwinm  r.5,r.2,0,0xfffff000    // get real addr of page table page
        beq     pfault                  // branch if not
        rlwimi  r.5,r.0,22,0x00000ffc   // insert pte index in pt page addr
        lwz     r.2,0(r.5)              // get page table entry
        andi.   r.3,r.2,PTE_VALID       // pte valid?
        beq     pfault                  // branch if not
lpte64:
        mtsprg  sprg.3,r.6              // save gpr 6
        rlwinm  r.3,r.0,20,0x0000ffff   // align failing vpi with vsid
        ori     r.2,r.2,0x190           // force RC and M bits in PTE
        xor     r.3,r.1,r.3             // hash - exclusive or vsid with vpi
	mfsdr1	r.6			// get storage descriptor reg
        li      r.5,1                   // construct valid PTE part 1
        rldimi  r.5,r.1,12,28           // insert VSID from SR into PTE
        rlwimi  r.5,r.0,16,20,24        // insert API from virtual addr arg
        ori     r.1,r.5,0		// stash in r.1
        rlwinm	r.5,r.6,2,25,29  	// extract htabsize and multiply by 4
	addi	r.5,r.5,HPT_MASK	// add real addr of hpt mask array base
	lwz	r.0,0(r.5)		// have hptmask in r.0
        rlwinm  r.0,r.0,11,0x001ff800   // align hpt mask with upper hash
        ori     r.0,r.0,0x07ff          // append lower one bits to mask
        and     r.0,r.0,r.3             // take hash modulo hpt size
        rlwinm  r.0,r.0,7,0xffffff80    // align hash as hpt group offset

#if !defined(NT_UP)
        li      r.3,HPT_LOCK            // get hpt lock address
getlk64:lwarx   r.5,0,r.3               // load and reserve lock word
        and.    r.5,r.5,r.5             // is lock available?
        mfsprg  r.5,sprg.0              // get processor ctl region addr
        bne-    getlk64_spin            // loop if lock is unavailable
        stwcx.  r.5,0,r.3               // store conditional to lock word
        bne-    getlk64_spin            // loop if lost reserve
        isync                           // context synchronize
#endif // NT_UP
        rldicr  r.3,r.6,0,45            // get real addr of hash page table
        or      r.3,r.0,r.3             // or with offset to get group addr
        INC_GRP_CTR_R(GRP_CTR_DSI_VALID_PTE,r3)
        ori     r.3,r.3,0x70            // point to last entry in group
        li      r.6,0                   // set no invalid hpte found
        b       thpte64                 // goto test hash page table entry

#if !defined(NT_UP)
getlk64_spin:
	lwz	r.5, 0(r.3)
	cmpwi	r.5, 0
	beq+	getlk64
	b	getlk64_spin
#endif

si64rem:
        mr      r.6,r.3                 // remember invalid hpte address
phpte64:andi.   r.0,r.3,0x007f          // tested all hptes in prim group?
        subi    r.3,r.3,16              // decrement to previous hpte
        beq-    si64nom                 // branch if yes
thpte64:ld      r.0,0(r.3)              // get 1st(be) word of hpte
        andi.   r.5,r.0,0x1             // hpte valid?
        beq     si64rem                 // jump if no to remember
        cmplw   r.1,r.0                 // does hpte match search arg?
        bne     phpte64                 // loop if no to previous hpte
        INC_GRP_CTR_R(GRP_CTR_DSI_FOUND,r3)
        b       skiphpte64              // hpte already present -- nothing to do

si64nom:
        cmplwi  r.6,0                   // was an invalid hpte found?
        beq     primov64                // branch if not
shpte64:std     r.2,8(r.6)              // store pte as 2nd(be) wd hpte
        sync                            // ensure 2nd word stored
        std     r.1,0(r.6)              // store vsid,api as 1st(be) wd hpte
skiphpte64:

#if !defined(NT_UP)
        li      r.0,0                   // get a zero value
        sync                            // ensure all previous stores done
        stw     r.0,HPT_LOCK(0)         // store zero in hpt lock word
#endif // NT_UP
        mfsprg  r.6,sprg.3              // reload saved gpr 6
        mfsprg  r.1,sprg.0              // get addr of processor ctl region
        mtcrf   0xff,r.4                // reload condition reg
        lwz     r.5,PcSiR5(r.1)         // reload saved gpr 5
        lwz     r.4,PcSiR4(r.1)         // reload saved gpr 4
        lwz     r.3,PcSiR3(r.1)         // reload saved gpr 3
        lwz     r.2,PcSiR2(r.1)         // reload saved gpr 2
        lwz     r.0,PcSiR0(r.1)         // reload saved gpr 0
        mfsprg  r.1,sprg.2              // reload saved gpr 1
        rfi                             // return from interrupt

primov64:
        mfdec   r.5                     // get decrementer
        addi    r.6,r.3,16              // recompute primary hpt group addr
        INC_GRP_CTR_R(GRP_CTR_DSI_FULL,r6)
        rlwimi  r.6,r.5,27,0x00000070   // choose 1 of 8 hptes as victim
        ld      r.0,0(r.6)              // get 1st(be) word of victim hpte
        clrrdi  r.0,r.0,1               // turn off valid bit
        std     r.0,0(r.6)              // invalidate 1st(be) wd victim hpte
        sync                            // ensure 1st word stored
        b       shpte64
stgerr64:
        clrrwi  r.3,r.0,PAGE_SHIFT      // get page address of fault
        mfsprg  r.2,sprg.0              // get phys addr of processor ctl region
        cmpwi   r.3,0xffffd000          // is fault in PCR page?
#if !COLLECT_PAGING_DATA
        ori     r.2, r.2, 1             // user readonly
        beq     lpte64                  // branch if yes
#else
        bne     stgerr64_not_pcr
        INC_CTR(CTR_PCR,r2,r3)
        ori     r.2, r.2, 1             // user readonly
        b       lpte64
stgerr64_not_pcr:
#endif
        clrrwi  r.2, r.2, 1             // get PCR address back
        lwz     r.2, PcPcrPage2(r.2)    // get phys addr of PCR2
        cmpwi   r.3,0xffffe000          // is fault in PCR2 page?
#if !COLLECT_PAGING_DATA
        ori     r.2, r.2, 1             // user readonly
        beq     lpte64                  // branch if yes
#else
        bne     stgerr64_not_pcr2
        mfsprg  r.2,sprg.0              // get phys addr of processor ctl region
        INC_CTR(CTR_PCR2,r2,r3)
        lwz     r.2, PcPcrPage2(r.2)    // get phys addr of PCR2
        ori     r.2, r.2, 1             // user readonly
        b       lpte64
stgerr64_not_pcr2:
#endif
        mfsprg  r.2,sprg.0              // get phys addr of PCR
        rlwinm  r.3, r.3, 4, 0xf        // Check sreg #
        lwz     r.2, PcPgDirRa(r.2)     // Page Directory addr
        cmpwi   r.3, 0xf                // sreg 15
        beq     s15ok64
        b       report_stgerr

//--------------------------------------------------------------------------
//
//  Continuation of Translation Miss interrupts (603)
//
//--------------------------------------------------------------------------

pgf603:
        INC_CTR (CTR_PAGE_FAULT,r1,r3)
        mfsrr1  r.2
        rlwimi  r.0,r.2,16,0x00000001   // stuff in S/L bit
        stw     r.0,PcBadVaddr(r.1)     // save fault address and st/l bit
        mtcrf   0x80,r.2                // restore CR0
        mfmsr   r.2                     // turn off use of temporary regs
        rlwinm  r.2,r.2,0x0,0xfffdffff  // clear bit 14, MSR[TGPR]
        mtmsr   r.2                     // now have access to "real" GPRs
        isync
        short_zlih(CODE_PAGE_FAULT)

dstgerr603:
        clrrwi  r.3,r.0,PAGE_SHIFT      // get page address of fault
        ori     r.2,r.1,0               // copy PCR physical address
        cmpwi   r.3,0xffffd000          // is fault in PCR page?
        ori     r.2, r.2, 1             // user readonly
        beq     tlbld603                // branch if yes
        lwz     r.2, PcPcrPage2(r.1)    // get phys addr of PCR2
        cmpwi   r.3,0xffffe000          // is fault in PCR2 page?
        ori     r.2, r.2, 1             // user readonly
        beq     tlbld603                // branch if yes
        lwz     r.2, PcPgDirRa(r.1)     // Page Directory addr
        rlwinm  r.3, r.3, 4, 0xf        // Check sreg #
        cmpwi   r.3, 0xf                // sreg 15
        beq     s15ok603

stgerr603:
        INC_CTR (CTR_STORAGE_ERROR,r1,r3)
        mfsrr1  r.2
        rlwimi  r.0,r.2,16,0x00000001   // stuff S/L bit in fault address
        stw     r.0,PcBadVaddr(r.1)     // save fault address and st/l bit
        mtcrf   0x80,r.2                // restore CR0
        mfmsr   r.2                     // turn off use of temporary regs
        rlwinm  r.2,r.2,0x0,0xfffdffff  // clear bit 14, MSR[TGPR]
        mtmsr   r.2                     // now have access to "real" GPRs
        isync
        short_zlih(CODE_STORAGE_ERROR)


//--------------------------------------------------------------------------
//
//  Short Zero Level Interrupt Continue (short_zlih_continue)
//
//  Branched-to by zhort_zlih() macro, with:
//      MSR:    External interrupts disabled
//              Instruction Relocate OFF
//              Data Relocate OFF
//      SRR0:   Next instruction address at time of interrupt
//      SRR1:   MSR at time of interrupt
//      SPRG3:  Saved r.2
//      r.2:    Code number indicating type of interrupt
//
//  Exits to common_exception_entry, with
//      MSR:    External interrupts disabled
//              Instruction Relocate ON
//              Data Relocate ON
//      GP registers:
//        r.2:  Constant identifying the exception type
//        r.3:  Saved SRR0 (interrupt address)
//        r.4:  Saved SRR1 (MSR value)
//        r.5:  -available-
//        r.11: -available-
//      In the PCR:
//        PcGprSave[0]:  Saved r.2
//        PcGprSave[1]:  Saved r.3
//        PcGprSave[2]:  Saved r.4
//        PcGprSave[3]:  Saved r.5
//        PcGprSave[5]:  Saved r.11
//
//      Nothing is left in the SPRG's
//
//--------------------------------------------------------------------------

short_zlih_continue:

        mtsprg  sprg.2, r.5                     // stash r.5 temporarily
        mfsprg  r.5, sprg.0                     // r.5 -> KiPcr (real address)
        stw     r.4, PCR_SAVE4 (r.5)            // save r.4 in PCR
        lwz     r.4, common_exception_entry.ptr - real0 (0) // load virt addr of common code **TEMP**
        stw     r.3, PCR_SAVE3 (r.5)            // save r.3 in PCR
        mfsrr0  r.3                             // save SRR0 (interrupt addr) in r.3
        mtsrr0  r.4                             // set branch address into SRR0
        mfsrr1  r.4                             // save SRR1 (MSR) in r.4
        stw     r.11, PCR_SAVE11 (r.5)          // save r.11 in PCR
        lis     r.11, FLIH_MSR >> 16            // load new value for
        ori     r.11, r.11, FLIH_MSR & 0xFFFF   //   MSR
        rlwimi  r.11, r.4, 0, MSR_PM, MSR_PM    // propagate MSR[PM]
        mtsrr1  r.11                            // set new MSR value into SRR1
        mfsprg  r.11, sprg.2                    // fetch stashed r.5 value
        stw     r.11, PCR_SAVE5 (r.5)           // save r.5 in PCR
        mfsprg  r.11, sprg.3                    // fetch stashed r.2 value
        stw     r.11, PCR_SAVE2 (r.5)           // save r.2 in PCR
        rfi                                     // turn on address translation,
                                                // branch to common code
//--------------------------------------------------------------------------
//
//  End of low memory code.  Code from the start of this module to here
//  must all be relocated together if relocation is required.   Switch
//  code section to .text as remaining code in this module must exist
//  for the life of the system.
//
//--------------------------------------------------------------------------
end_of_code_to_move:

        .org    0x3800

        .org    0x4000                  // gen warning if above overflows.

//
// Code from here thru Kseg0CodeEnd is copied to KSEG0 at system
// initialization.  This code is declared in the INIT section so the
// space can be used for other purposes after system initialization.
//

Kseg0CodeStart:

        .align  6

StartProcessor:

        ori     r.31, r.3, 0            // save address of LPB

StartProcessor.LoadKiStartProcessorAddress:

        lis     r.30, 0                 // load address of KiStartProcessor
        ori     r.30, r.30, 0           //  (actual address filled in at
                                        //  init time by processor 0)
        mfmsr   r.11                    // get current state
        rlwinm  r.11, r.11, 0, ~INT_ENA // clear interrupt enable
        mtmsr   r.11                    // disable interrupts
        rlwinm  r.11, r.11, 0, ~(MASK_SPR(MSR_IR,1)|MASK_SPR(MSR_DR,1))
        mtsrr1  r.11                    // desired initial state
        mtsrr0  r.30                    // desired return address
        rfi                             // switch to real mode and jump
                                        //  to KiStartProcessor

        .align  6

KiPriorityExitRfi:
        mtsrr0  r.4                     // set target address
        mtsrr1  r.5                     // set target state
        lwz     r.4, PCR_SAVE4 (r.6)    // reload r.4, 5 and 6 from PCR
        lwz     r.5, PCR_SAVE5 (r.6)
        lwz     r.6, PCR_SAVE6 (r.6)
        rfi                             // resume thread

        .align  6

KiServiceExitKernelRfi:
        mtsrr0  r.11                    // move caller's IAR to SRR0
        mtsrr1  r.10                    // move caller's MSR to SRR1
        rfi                             // return from interrupt

        .align  6

KiServiceExitUserRfi:
        mtsrr0  r.11                    // move caller's IAR to SRR0
        mtsrr1  r.10                    // move caller's MSR to SRR1
        li      r.9, 0                  // clear the last few
        li      r.12, 0
        li      r.11, 0
        li      r.10, 0                 //   volatile GP regs
        rfi                             // return from interrupt

        .align  6

RtlpRestoreContextRfi:
        mtsrr0  r.7                     // set target address
        mtsrr1  r.3                     // set target state
        lwz     r.3, PCR_SAVE4 (r.8)    // reload r.3, 7 and 8 from PCR
        lwz     r.7, PCR_SAVE5 (r.8)
        lwz     r.8, PCR_SAVE6 (r.8)
        rfi                             // resume thread

//++
//
// VOID
// KiFlushSingleTb (
//    IN BOOLEAN Invalid,
//    IN PVOID Virtual
//    )
//
// Routine Description:
//
//    This function flushes a single entry from the translation buffer.
//
// Arguments:
//
//    Invalid (r3) - Supplies a boolean variable that determines the reason
//       that the TB entry is being flushed.
//
//    Virtual (r4) - Supplies the virtual address of the entry that is to
//       be flushed from the translation buffer.
//
// Return Value:
//
//    None.
//
//--

        .align  6

FlushSingleTb:

        INC_CTR2(CTR_FLUSH_SINGLE,r5,r6)
flst0:  b       flnhpt                  // default to no hpt

        mfsrin  r.5,r.4                 // get sreg of virtual addr arg
        rlwinm  r.6,r.4,20,0x0000ffff   // align arg vpi with vsid
        xor     r.6,r.5,r.6             // hash - exclusive or vsid with vpi
        rlwimi  r.5,r.4,3,0x7e000000    // insert api into reg with vsid
        rlwinm  r.5,r.5,7,0xffffffbf    // align vsid,api as 1st word hpte
        oris    r.5,r.5,0x8000          // set valid bit in 1st word hpte
        mfsdr1  r.7                     // get storage description reg
        rlwinm  r.8,r.7,10,0x0007fc00   // align hpt mask with upper hash
        ori     r.8,r.8,0x03ff          // append lower one bits to mask
        and     r.6,r.8,r.6             // take hash modulo hpt size
        rlwinm  r.6,r.6,6,0x01ffffc0    // align hash as hpt group offset
        rlwinm  r.7,r.7,0,0xffff0000    // get real addr of hash page table
        oris    r.7,r.7,K_BASE          // or with kernel virtual address
        or      r.6,r.7,r.6             // or with offset to get group addr
        INC_GRP_CTR(GRP_CTR_FLUSH_SINGLE,r6,r9,r10)

#if !defined(NT_UP)

        li      r.9,HPT_LOCK            // get hpt lock real address
        oris    r.9,r.9,K_BASE          // or with kernel virtual address

        DISABLE_INTERRUPTS(r.10,r.11)   // disable ints while lock held

flglk:  lwarx   r.7,0,r.9               // load and reserve lock word
        cmpwi   r.7,0                   // is lock available?
        mfsprg  r.7,sprg.0              // get processor ctl region addr
        bne-    flglkw                  // loop if lock is unavailable
        stwcx.  r.7,0,r.9               // store conditional to lock word
        bne-    flglkw                  // loop if lost reserve
        isync                           // context synchronize

#endif // NT_UP

        b       fltst                   // goto test hash page table entry

#if !defined(NT_UP)
flglkw:
        ENABLE_INTERRUPTS(r.10)
flglkws:
        lwz     r.7,0(r.9)
        cmpwi   r.7,0
        bne-    flglkws
        mtmsr   r.11
	cror	0,0,0			// N.B. 603e/ev Errata 15
        b       flglk
#endif

#if !defined(HPT_AS_TLB_RELOAD_BUFFER)
flnxt:  addi    r.6,r.6,8               // increment to next hpte
        andi.   r.7,r.6,0x003f          // tested all hptes in prim group?
        beq     flinv                   // branch if yes
fltst:  lwz     r.7,4(r.6)              // get 1st(be) word of hpte
        cmplw   r.5,r.7                 // does hpte match search arg?
        bne     flnxt                   // loop if no to next hpte
#else
fltst:  lwz     r.7,4(r.6)              // get 1st(be) word of hpte
        cmplw   r.5,r.7                 // does hpte match search arg?
        bne     flinv                   // loop if no to next hpte
#endif
        INC_GRP_CTR(GRP_CTR_FLUSH_SINGLE_FOUND,r6,r5,r12)
#if defined(PRESERVE_HPTE_CONTENTS)
        clrlwi  r.8,r.7,1               // turn off valid bit
#endif
        stw     r.8,4(r.6)              // invalidate 1st(be) wd match hpte
        sync                            // ensure 1st word stored
flinv:  tlbie   r.4                     // invalidate tlb entry
        sync                            // ensure invalidate done

#if !defined(NT_UP)

flst1:  tlbsync                         // ensure broadcasts done
flst2:  sync                            // ensure tlbsync done

        li      r.7,0                   // get a zero value
        stw     r.7,0(r.9)              // store zero in hpt lock word

        ENABLE_INTERRUPTS(r.10)         // restore interrupt status

#endif // NT_UP

        blr                             // return

flnhpt: tlbie   r.4                     // invalidate tlb entry
        sync                            // ensure invalidate done

        blr                             // return


//++
//
// VOID
// KiFlushSingleTb64 (
//    IN BOOLEAN Invalid,
//    IN PVOID Virtual
//    )
//
// Routine Description:
//
//    This function is the analog of KiFlushSingleTb
//    for processors using the 64-bit memory management model.
//
// Arguments:
//
//    Invalid (r3) - Supplies a boolean variable that determines the reason
//       that the TB entry is being flushed.
//
//    Virtual (r4) - Supplies the virtual address of the entry that is to
//       be flushed from the translation buffer.
//
// Return Value:
//
//    None.
//
//--
        .align  6

FlushSingleTb64:

        INC_CTR2(CTR_FLUSH_SINGLE,r5,r6)

	mfsrin	r.8,r.4			// get sreg of virtual addr arg
	rlwinm	r.6,r.4,20,0x0000ffff	// align arg vpi with vsid
	xor	r.6,r.8,r.6		// hash -- exclusive or vsid with vpi
	mfsdr1	r.7			// get storage description reg
	li	r.5,1			// construct valid PTE part 1
	rldimi	r.5,r.8,12,28		// insert VSID from SR into PTE
	rlwimi	r.5,r.4,16,20,24	// insert API from virtual addr arg
	rlwinm	r.8,r.7,2,25,29		// extract htabsize and multiply by 4
	li	r.9,HPT_MASK		// real address of HPT masks array
	oris	r.9,r.9,K_BASE		// convert to kernel virtual
	lwzx	r.8,r.9,r.8		// get hpt mask in use
	rlwinm	r.8,r.8,11,0x001ff800	// align hpt mask with upper hash
	ori	r.8,r.8,0x7ff		// append lower one bits to mask
	and	r.6,r.8,r.6		// take hash modulo hpt size
	rlwinm	r.6,r.6,7,0xffffff80	// align hash as hpt group offset
	rldicr	r.7,r.7,0,45		// get real base addr of hpt
	oris	r.7,r.7,K_BASE		// convert to kernel virtual addr
	or	r.6,r.7,r.6		// or with offset to get group addr
	INC_GRP_CTR(GRP_CTR_FLUSH_SINGLE,r6,r9,r10)

#if !defined(NT_UP)

        li      r.9,HPT_LOCK            // get hpt lock real address
        oris    r.9,r.9,K_BASE          // or with kernel virtual address

        DISABLE_INTERRUPTS(r.10,r.11)   // disable ints while lock held

flglk64:
	lwarx   r.7,0,r.9               // load and reserve lock word
        cmpwi   r.7,0                   // is lock available?
        mfsprg  r.7,sprg.0              // get processor ctl region addr
        bne-    flglk64w                // loop if lock is unavailable
        stwcx.  r.7,0,r.9               // store conditional to lock word
        bne-    flglk64w                // loop if lost reserve
        isync                           // context synchronize

#endif // NT_UP

        b       fltst64                 // goto test hash page table entry

#if !defined(NT_UP)
flglk64w:
        ENABLE_INTERRUPTS(r.10)
flglk64ws:
        lwz     r.7,0(r.9)
        cmpwi   r.7,0
        bne-    flglk64ws
        mtmsr   r.11
        b       flglk64
#endif

#if !defined(HPT_AS_TLB_RELOAD_BUFFER)
flnxt64:
	addi    r.6,r.6,16              // increment to next hpte
        andi.   r.7,r.6,0x007f          // tested all hptes in prim group?
        beq     flinv64                 // branch if yes
fltst64:
	ld      r.7,0(r.6)              // get 1st(be) word of hpte
        cmpld   r.5,r.7                 // does hpte match search arg?
        bne     flnxt64                 // loop if no to next hpte
#else
fltst64:
	ld      r.7,0(r.6)              // get 1st(be) word of hpte
        cmpld   r.5,r.7                 // does hpte match search arg?
        bne     flinv64                 // loop if no to next hpte
#endif
        INC_GRP_CTR(GRP_CTR_FLUSH_SINGLE_FOUND,r6,r5,r12)
#if defined(PRESERVE_HPTE_CONTENTS)
        clrrdi  r.8,r.7,1               // turn off valid bit
#else
	li	r.8,0			// which turns off bit 63 (valid bit)
#endif
        std     r.8,0(r.6)              // invalidate 1st(be) wd match hpte
        sync                            // ensure 1st word stored
flinv64:
	tlbie   r.4                     // invalidate tlb entry
        sync                            // ensure invalidate done

#if !defined(NT_UP)

	tlbsync                         // ensure broadcasts done
	sync                            // ensure tlbsync done

        li      r.7,0                   // get a zero value
        stw     r.7,0(r.9)              // store zero in hpt lock word

        ENABLE_INTERRUPTS(r.10)         // restore interrupt status

#endif // NT_UP

	blr				// return

//++
//
// VOID
// KeFillEntryTb (
//    IN HARDWARE_PTE Pte[],
//    IN PVOID Virtual,
//    IN BOOLEAN Invalid
//    )
//
// Routine Description:
//
//    This function fills a translation buffer entry. If the entry is already
//    in the translation buffer, then the entry is overwritten. Otherwise, a
//    random entry is overwritten.
//
// Arguments:
//
//    Pte (r3) - Supplies a pointer to the page table entry that is to be
//       written into the TB.
//
//    Virtual (r4) - Supplies the virtual address of the entry that is to
//       be filled in the translation buffer.
//
//    Invalid (r5) - Supplies a boolean value that determines whether the
//       TB entry should be invalidated.
//
// Return Value:
//
//    None.
//
//--

        .align  6

FillEntryTb:

        INC_CTR2(CTR_FILL_ENTRY,r6,r7)
fiet0:  b       finhpt                  // default to no hpt

        lwz     r.3,0(r.3)              // get page table entry
        mfsrin  r.5,r.4                 // get sreg of virtual addr arg
        ori     r.3,r.3,0x0190          // set CR and M

#if DBG

        andi.   r.6,r.3,PTE_VALID       // pte valid?
        bne     fiptev                  // branch if yes
        twi     31,0,KERNEL_BREAKPOINT  // break into kernel debugger
fiptev:

#endif

        rlwinm  r.6,r.4,20,0x0000ffff   // align arg vpi with vsid
        xor     r.6,r.5,r.6             // hash - exclusive or vsid with vpi
        rlwimi  r.5,r.4,3,0x7e000000    // insert api into reg with vsid
        rlwinm  r.5,r.5,7,0xffffffbf    // align vsid,api as 1st word hpte
        oris    r.5,r.5,0x8000          // set valid bit in 1st word hpte
        mfsdr1  r.7                     // get storage description reg
        rlwinm  r.8,r.7,10,0x0007fc00   // align hpt mask with upper hash
        ori     r.8,r.8,0x03ff          // append lower one bits to mask
        and     r.6,r.8,r.6             // take hash modulo hpt size
        rlwinm  r.6,r.6,6,0x01ffffc0    // align hash as hpt group offset
        rlwinm  r.7,r.7,0,0xffff0000    // get real addr of hash page table
        oris    r.7,r.7,K_BASE          // or with kernel virtual address
        or      r.6,r.7,r.6             // or with offset to get group addr
#if !defined(HPT_AS_TLB_RELOAD_BUFFER)
        ori     r.6,r.6,0x38            // point to last entry in group
        li      r.8,0                   // set no invalid hpte found
#endif
        INC_GRP_CTR(GRP_CTR_FILL_ENTRY,r6,r9,r10)

        DISABLE_INTERRUPTS(r.10,r.11)   // disable interrupts

#if !defined(NT_UP)

        li      r.9,HPT_LOCK            // get hpt lock real address
        oris    r.9,r.9,K_BASE          // or with kernel virtual address
figlk:
        lwarx   r.7,0,r.9               // load and reserve lock word
        cmpwi   r.7,0                   // is lock available?
        mfsprg  r.7,sprg.0              // get processor ctl region addr
        bne-    figlkw                  // loop if lock is unavailable
        stwcx.  r.7,0,r.9               // store conditional to lock word
        bne-    figlkw                  // loop if lost reserve
        isync                           // context synchronize

#endif // NT_UP

        b       fitst                   // goto test hash page table entry

#if !defined(NT_UP)
figlkw:
        ENABLE_INTERRUPTS(r.10)
figlkws:
        lwz     r.7,0(r.9)
        cmpwi   r.7,0
        bne-    figlkws
        mtmsr   r.11
	cror	0,0,0			// N.B. 603e/ev Errata 15
        b       figlk
#endif

#if !defined(HPT_AS_TLB_RELOAD_BUFFER)
firem:
        ori     r.8,r.6,0               // remember invalid hpte address
fiprv:
        andi.   r.7,r.6,0x003f          // tested all hptes in prim group?
        subi    r.6,r.6,8               // decrement to previous hpte
        beq     finom                   // branch if yes
fitst:
        lwz     r.7,4(r.6)              // get 1st(be) word of hpte
        andis.  r.11,r.7,0x8000         // hpte valid?
        beq     firem                   // jump if no to remember
        cmplw   r.5,r.7                 // does hpte match search arg?
        bne     fiprv                   // loop if no to previous hpte
        INC_GRP_CTR(GRP_CTR_FILL_ENTRY_FOUND,r6,r5,r7)
        stw     r.3,0(r.6)              // store pte 2nd(be) wd match hpte
        sync                            // ensure 2nd word stored
#else
fitst:
        lwz     r.7,4(r.6)              // get 1st(be) word of hpte
        andis.  r.11,r.7,0x8000         // hpte valid?
        beq     fiinv                   // jump if no
        clrlwi  r.7,r.7,1               // turn off valid bit
        stw     r.7,4(r.6)              // invalidate hpte
        sync                            // ensure update done
fiinv:
        stw     r.3,0(r.6)              // store pte as 2nd(be) wd hpte
        sync                            // ensure 2nd word stored
        stw     r.5,4(r.6)              // store vsid,api as 1st(be) wd hpte
#endif
fiexi:
        tlbie   r.4                     // invalidate tlb entry
        sync                            // ensure invalidate done

#if !defined(NT_UP)

fiet1:  tlbsync                         // ensure broadcasts done
fiet2:  sync                            // ensure tlbsync done

        li      r.7,0                   // get a zero value
        stw     r.7,0(r.9)              // store zero in hpt lock word

#endif // NT_UP

        ENABLE_INTERRUPTS(r.10)         // enable interrupts

        blr                             // return

#if !defined(HPT_AS_TLB_RELOAD_BUFFER)
finom:
        cmplwi  r.8,0                   // was an invalid hpte found?
        beq     fipov                   // branch if not
fisto:
        stw     r.3,0(r.8)              // store pte as 2nd(be) wd hpte
        sync                            // ensure 2nd word stored
        stw     r.5,4(r.8)              // store vsid,api as 1st(be) wd hpte
        b       fiexi

fipov:
        mfdec   r.7                     // get decrementer
        addi    r.8,r.6,8               // recompute primary hpt group addr
        rlwimi  r.8,r.7,28,0x00000038   // choose 1 of 8 hptes as victim
        INC_GRP_CTR(GRP_CTR_FILL_ENTRY_FULL,r8,r7,r12)
#if !defined(PRESERVE_HPTE_CONTENTS)
        li      r.6,0
#else
        lwz     r.6,4(r.8)              // get 1st(be) word of victim hpte
        clrlwi  r.6,r.6,1               // turn off valid bit
#endif
        stw     r.6,4(r.8)              // invalidate 1st(be) wd victim hpte
        sync                            // ensure 1st word stored
        b       fisto                   // goto store new hpte
#endif

finhpt:
        andi.   r.6,r.5,1               // is Invalid == TRUE?
        beqlr                           // return if Invalid == FALSE
        tlbie   r.4                     // invalidate tlb entry
        sync                            // ensure invalidate done

        blr                             // return


//++
//
// VOID
// KeFillEntryTb64 (
//    IN HARDWARE_PTE Pte[],
//    IN PVOID Virtual,
//    IN BOOLEAN Invalid
//    )
//
// Routine Description:
//
//    This function is the analog to KeFillEntryTb
//    for processors which use the 64-bit memory management model.
//
// Arguments:
//
//    Pte (r3) - Supplies a pointer to the page table entry that is to be
//       written into the TB.
//
//    Virtual (r4) - Supplies the virtual address of the entry that is to
//       be filled in the translation buffer.
//
//    Invalid (r5) - Supplies a boolean value that determines whether the
//       TB entry should be invalidated.
//
// Return Value:
//
//    None.
//
//--

	.align	6

FillEntryTb64:

        INC_CTR2(CTR_FILL_ENTRY,r6,r7)

	lwz     r.3,0(r.3)              // get page table entry
        mfsrin  r.8,r.4                 // get sreg of virtual addr arg
        ori     r.3,r.3,0x0190          // set CR and M

#if DBG

        andi.   r.6,r.3,PTE_VALID       // pte valid?
        bne     fiptev64                // branch if yes
        twi     31,0,KERNEL_BREAKPOINT  // break into kernel debugger
fiptev64:

#endif

	rlwinm	r.6,r.4,20,0x0000ffff	// align arg vpi with vsid
	xor	r.6,r.8,r.6		// hash -- exclusive or vsid with vpi
	mfsdr1	r.7			// get storage description reg
	li	r.5,1			// construct valid PTE part 1
	rldimi	r.5,r.8,12,28		// insert VSID from SR into PTE
	rlwimi	r.5,r.4,16,20,24	// insert API from virtual addr arg
	rlwinm	r.8,r.7,2,25,29		// extract htabsize and multiply by 4
	li	r.9,HPT_MASK		// real address of HPT masks array
	oris	r.9,r.9,K_BASE		// convert to kernel virtual
	lwzx	r.8,r.9,r.8		// get hpt mask in use
	rlwinm	r.8,r.8,11,0x001ff800	// align hpt mask with upper hash
	ori	r.8,r.8,0x7ff		// append lower one bits to mask
	and	r.6,r.8,r.6		// take hash modulo hpt size
	rlwinm	r.6,r.6,7,0xffffff80	// align hash as hpt group offset
	rldicr	r.7,r.7,0,45		// get real base addr of hpt
	oris	r.7,r.7,K_BASE		// convert to kernel virtual addr
	or	r.6,r.7,r.6		// or with offset to get group addr

#if !defined(HPT_AS_TLB_RELOAD_BUFFER)
	ori	r.6,r.6,0x70		// point to last entry in group
	li	r.8,0			// set no invalid hpte found
#endif
	INC_GRP_CTR(GRP_CTR_FILL_ENTRY,r6,r9,r10)

	DISABLE_INTERRUPTS(r.10,r.11)	// disable interrupts

#if !defined(NT_UP)

        li      r.9,HPT_LOCK            // get hpt lock real address
        oris    r.9,r.9,K_BASE          // or with kernel virtual address
figlk64:
        lwarx   r.7,0,r.9               // load and reserve lock word
        cmpwi   r.7,0                   // is lock available?
        mfsprg  r.7,sprg.0              // get processor ctl region addr
        bne-    figlk64w                // loop if lock is unavailable
        stwcx.  r.7,0,r.9               // store conditional to lock word
        bne-    figlk64w                // loop if lost reserve
        isync                           // context synchronize

#endif // NT_UP

        b       fitst64                 // goto test hash page table entry

#if !defined(NT_UP)
figlk64w:
        ENABLE_INTERRUPTS(r.10)
figlk64ws:
        lwz     r.7,0(r.9)
        cmpwi   r.7,0
        bne-    figlk64ws
        mtmsr   r.11
        b       figlk64
#endif

#if !defined(HPT_AS_TLB_RELOAD_BUFFER)
firem64:
        ori     r.8,r.6,0               // remember invalid hpte address
fiprv64:
        andi.   r.7,r.6,0x007f          // tested all hptes in prim group?
        subi    r.6,r.6,16              // decrement to previous hpte
        beq     finom64                 // branch if yes
fitst64:
        ld      r.7,0(r.6)              // get 1st(be) word of hpte
        andi.   r.11,r.7,0x0001         // hpte valid?
        beq     firem64                 // loop if no to remember
        cmpld   r.5,r.7                 // does hpte match search arg?
        bne     fiprv64                 // loop if no to next hpte
        INC_GRP_CTR(GRP_CTR_FILL_ENTRY_FOUND,r6,r5,r7)
        std     r.3,8(r.6)              // store pte 2nd(be) wd match hpte
        sync                            // ensure 2st word stored
#else
fitst64:
        ld      r.7,0(r.6)              // get 1st(be) word of hpte
        andi.   r.11,r.7,0x0001         // hpte valid?
        beq     fiinv64                 // loop if no to remember
        clrrdi  r.7,r,7,1               // turn off valid bit
        std     r.7,0(r.6)              // invalidate hpte
        sync                            // ensure update done
fiinv64:
        std     r.3,8(r.6)              // store pte 2nd(be) word of hpte
        sync                            // ensure 2nd word stored
        std     r.5,0(r.6)              // store pte 1st(be) word of hpte
#endif

fiexi64:
        tlbie   r.4                     // invalidate tlb entry
        sync                            // ensure invalidate done

#if !defined(NT_UP)

        tlbsync                         // ensure broadcasts done
        sync                            // ensure tlbsync done

        li      r.7,0                   // get a zero value
        stw     r.7,0(r.9)              // store zero in hpt lock word

#endif // NT_UP

        ENABLE_INTERRUPTS(r.10)         // enable interrupts

	blr				// return

#if !defined(HPT_AS_TLB_RELOAD_BUFFER)
finom64:
        cmplwi  r.8,0                   // was an invalid hpte found?
        beq     fipov64                 // branch if not
fisto64:
        std     r.3,8(r.8)              // store pte as 2nd(be) wd hpte
        sync                            // ensure 2nd word stored
        std     r.5,0(r.8)              // store vsid,api as 1st(be) wd hpte
        b       fiexi64

fipov64:
        mfdec   r.7                     // get decrementer
        addi    r.8,r.6,16              // recompute primary hpt group addr
        rlwimi  r.8,r.7,29,0x00000070   // choose 1 of 8 hptes as victim
        INC_GRP_CTR(GRP_CTR_FILL_ENTRY_FULL,r8,r7,r12)
#if !defined(PRESERVE_HPTE_CONTENTS)
	li	r.7,0
#else
        ld      r.6,0(r.8)              // get 1st(be) word of victim hpte
        clrrdi  r.7,r.6,1               // turn off valid bit
#endif
        stw     r.7,0(r.8)              // invalidate 1st(be) wd victim hpte
        sync                            // ensure 1st word stored
        b       fisto64                 // goto store new hpte
#endif


//++
//
// VOID
// KeFlushCurrentTb (
//    )
//
// Routine Description:
//
//    This function flushes the entire translation buffer.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//--

        .align  6

FlushCurrentTb:

        INC_CTR2(CTR_FLUSH_CURRENT,r6,r7)
flct0:  li      r.6, 128                // default to MAX num congruence
                                        // classes.

#if !defined(NT_UP)

flct1:  b       felk                    // default to no hpt.

#else

flct1:  b       feloop                  // default to no hpt.

#endif

fehpt:  mfsdr1  r.5                     // get storage description reg
        addi    r.0,r.5,1               // add one to hpt mask
        rlwinm  r.0,r.0,10,0x000ffc00   // align as number of hpt groups
        rlwinm  r.5,r.5,0,0xffff0000    // get real addr of hash page table
        mtctr   r.0                     // put number groups in count reg
        oris    r.5,r.5,K_BASE          // or with kernel virtual address

#if DBG

        lbz     r.3,KiPcr+PcDcacheMode(r.0) // get dcache mode information

#endif

#if !defined(NT_UP)

felk:   li      r.9,HPT_LOCK            // get hpt lock real address
        oris    r.9,r.9,K_BASE          // or with kernel virtual address

        DISABLE_INTERRUPTS(r.10,r.11)   // disable ints while lock held

feglk:  lwarx   r.7,0,r.9               // load and reserve lock word
        cmpwi   r.7,0                   // is lock available?
        li      r.7,-1                  // lock value - hpt clear
        bne     feglkw                  // jif lock is unavailable (wait)
feglks: stwcx.  r.7,0,r.9               // store conditional to lock word
        bne-    feglkw                  // loop if lost reserve
        isync                           // context synchronize
flct2:  b       feloop                  // default to no hpt

#endif // NT_UP

//
// Zero the Hashed Page Table
//

        li      r.0, 0
fenxg:
#if !defined(HPT_AS_TLB_RELOAD_BUFFER)
#if !defined(PRESERVE_HPTE_CONTENTS)
        stw     r.0,4(r.5)              // invalidate 1st(be) wd 1st hpte
        stw     r.0,12(r.5)             // invalidate 1st(be) wd 2nd hpte
        stw     r.0,20(r.5)             // invalidate 1st(be) wd 3rd hpte
        stw     r.0,28(r.5)             // invalidate 1st(be) wd 4th hpte
        stw     r.0,36(r.5)             // invalidate 1st(be) wd 5th hpte
        stw     r.0,44(r.5)             // invalidate 1st(be) wd 6th hpte
        stw     r.0,52(r.5)             // invalidate 1st(be) wd 7th hpte
        stw     r.0,60(r.5)             // invalidate 1st(be) wd 8th hpte
#else
        lwz     r.0,4(r.5)              // get 1st(be) wd 1st hpte
        lwz     r.7,12(r.5)             // get 1st(be) wd 2nd hpte
        lwz     r.8,20(r.5)             // get 1st(be) wd 3rd hpte
        lwz     r.11,28(r.5)            // get 1st(be) wd 4th hpte
        clrlwi  r.0,r.0,1               // turn off valid bit
        clrlwi  r.7,r.7,1               // turn off valid bit
        clrlwi  r.8,r.8,1               // turn off valid bit
        clrlwi  r.11,r.11,1             // turn off valid bit
        stw     r.0,4(r.5)              // invalidate 1st(be) wd 1st hpte
        stw     r.7,12(r.5)             // invalidate 1st(be) wd 2nd hpte
        stw     r.8,20(r.5)             // invalidate 1st(be) wd 3rd hpte
        stw     r.11,28(r.5)            // invalidate 1st(be) wd 4th hpte
        lwz     r.0,36(r.5)             // get 1st(be) wd 5th hpte
        lwz     r.7,44(r.5)             // get 1st(be) wd 6th hpte
        lwz     r.8,52(r.5)             // get 1st(be) wd 7th hpte
        lwz     r.11,60(r.5)            // get 1st(be) wd 8th hpte
        clrlwi  r.0,r.0,1               // turn off valid bit
        clrlwi  r.7,r.7,1               // turn off valid bit
        clrlwi  r.8,r.8,1               // turn off valid bit
        clrlwi  r.11,r.11,1             // turn off valid bit
        stw     r.0,36(r.5)             // invalidate 1st(be) wd 5th hpte
        stw     r.7,44(r.5)             // invalidate 1st(be) wd 6th hpte
        stw     r.8,52(r.5)             // invalidate 1st(be) wd 7th hpte
        stw     r.11,60(r.5)            // invalidate 1st(be) wd 8th hpte
#endif
#else
#if defined(PRESERVE_HPTE_CONTENTS)
        lwz     r.0,4(r.5)              // get 1st(be) wd 1st hpte
        clrlwi  r.0,r.0,1               // turn off valid bit
#endif
        stw     r.0,4(r.5)              // invalidate 1st(be) wd 1st hpte
#endif
        addi    r.5,r.5,64              // increment to next hpt group addr
        bdnz    fenxg                   // loop through all groups
        sync                            // ensure all stores done

//
// Invalidate all TLB entries
//

feloop: mtctr   r.6                     // put number classes in count reg
fenxt:  tlbie   r.6                     // invalidate tlb congruence class
        addi    r.6,r.6,4096            // increment to next class address
        bdnz    fenxt                   // loop through all classes
        sync                            // ensure all invalidates done

#if !defined(NT_UP)

flct3:  tlbsync                         // ensure broadcasts done
flct4:  sync                            // ensure tlbsync done

        li      r.7,0                   // get a zero value
        stw     r.7,0(r.9)              // store zero in hpt lock word

        ENABLE_INTERRUPTS(r.10)         // restore previous interrupt state

        blr                             // return

//
// We come here if the hpt lock is held by another processor
// when we first attempt to take it.  If the lock value is < 0
// then the lock is held by a processor that is clearing the entire
// hpt (other processor is in KeFlushCurrentTb).  If this happens
// then all we need do is waitfor the lock to become available (we
// don't actually need to take it) and return.
//

feglkw:
        crmove  4,0                     // move -ve bit to cr.1
        ENABLE_INTERRUPTS(r.10)
fegwt:  lwz     r.7,0(r.9)              // load lock word
        cmpwi   r.7,0                   // is lock available?
        bne-    fegwt                   // if not available, try again
        li      r.7,-1                  // lock value - hpt clear
        bt      4,feglk_done            // jif already cleared
        mtmsr   r.11
	cror	0,0,0			// N.B. 603e/ev Errata 15
        b       feglk

feglk_done:
        sync                            // wait till tlb catches up

#endif // NT_UP

        blr                             // return


//++
//
// VOID
// KeFlushCurrentTb64 (
//    )
//
// Routine Description:
//
//    This function flushes the entire translation buffer,
//    for processors which use the 64-bit memory management model.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//--

	.align	6

FlushCurrentTb64:
	INC_CTR2(CTR_FLUSH_CURRENT,r6,r7)

	mfsdr1	r.7			// get storage description reg
	li	r.8,HPT_MASK		// real addr of HPT masks array
	oris	r.8,r.8,K_BASE		// convert to kernel virtual addr
	rlwinm	r.5,r.7,2,25,29		// extract htabsize and multiply by 4
	lwzx	r.5,r.8,r.5		// get mask in use
	li	r.0,0			// calculate number of groups in hpt
	ori	r.0,r.0,0x8000		// least number of groups possible
	slw	r.0,r.0,r.5		// actual number of groups in hpt
	rldicr	r.5,r.7,0,45		// get real base addr of hpt
	mtctr	r.0			// put number of groups in count reg
	oris	r.5,r.5,K_BASE		// kernel virtual addr of hpt

#if !defined(NT_UP)

felk64:
	li      r.9,HPT_LOCK            // get hpt lock real address
        oris    r.9,r.9,K_BASE          // or with kernel virtual address

        DISABLE_INTERRUPTS(r.10,r.11)   // disable ints while lock held

feglk64:
	lwarx   r.7,0,r.9               // load and reserve lock word
        cmpwi   r.7,0                   // is lock available?
        li      r.7,-1                  // lock value - hpt clear
        bne     feglk64w                // jif lock is unavailable (wait)
feglk64s:
	stwcx.  r.7,0,r.9               // store conditional to lock word
        bne-    feglk64w                // loop if lost reserve
        isync                           // context synchronize

#endif // NT_UP

//
// Zero the Hashed Page Table
//

        li      r.0, 0
fenxg64:
#if !defined(HPT_AS_TLB_RELOAD_BUFFER)
#if !defined(PRESERVE_HPTE_CONTENTS)
        std     r.0,   0(r.5)           // invalidate 1st(be) wd 1st hpte
        std     r.0,  16(r.5)           // invalidate 1st(be) wd 2nd hpte
        std     r.0,  32(r.5)           // invalidate 1st(be) wd 3rd hpte
        std     r.0,  48(r.5)           // invalidate 1st(be) wd 4th hpte
        std     r.0,  64(r.5)           // invalidate 1st(be) wd 5th hpte
        std     r.0,  80(r.5)           // invalidate 1st(be) wd 6th hpte
        std     r.0,  96(r.5)           // invalidate 1st(be) wd 7th hpte
        std     r.0, 112(r.5)           // invalidate 1st(be) wd 8th hpte
#else
        ld      r.0,   0(r.5)           // get 1st(be) wd 1st hpte
        ld      r.7,  16(r.5)           // get 1st(be) wd 1st hpte
        ld      r.8,  32(r.5)           // get 1st(be) wd 1st hpte
        ld      r.11, 48(r.5)           // get 1st(be) wd 1st hpte
        clrrdi  r.0,  r.0,  1           // turn off valid bit
        clrrdi  r.7,  r.7,  1           // turn off valid bit
        clrrdi  r.8,  r.8,  1           // turn off valid bit
        clrrdi  r.11, r.11, 1           // turn off valid bit
        std     r.0,   0(r.5)           // invalidate 1st(be) wd 1st hpte
        std     r.7,  16(r.5)           // invalidate 1st(be) wd 1st hpte
        std     r.8,  32(r.5)           // invalidate 1st(be) wd 1st hpte
        std     r.11, 48(r.5)           // invalidate 1st(be) wd 1st hpte
//  Second cache line
        ld      r.0,  64(r.5)           // get 1st(be) wd 1st hpte
        ld      r.7,  80(r.5)           // get 1st(be) wd 1st hpte
        ld      r.8,  96(r.5)           // get 1st(be) wd 1st hpte
        ld      r.11,112(r.5)           // get 1st(be) wd 1st hpte
        clrrdi  r.0,  r.0,  1           // turn off valid bit
        clrrdi  r.7,  r.7,  1           // turn off valid bit
        clrrdi  r.8,  r.8,  1           // turn off valid bit
        clrrdi  r.11, r.11, 1           // turn off valid bit
        std     r.0,  64(r.5)           // invalidate 1st(be) wd 1st hpte
        std     r.7,  80(r.5)           // invalidate 1st(be) wd 1st hpte
        std     r.8,  96(r.5)           // invalidate 1st(be) wd 1st hpte
        std     r.11,112(r.5)           // invalidate 1st(be) wd 1st hpte
#endif
#else
#if defined(PRESERVE_HPTE_CONTENTS)
        ld      r.0,   0(r.5)           // get 1st(be) wd 1st hpte
        clrrdi  r.0, r.0, 1             // turn off valid bit
#endif
        std     r.0,   0(r.5)           // invalidate 1st(be) wd 1st hpte
#endif
        addi    r.5,r.5,128             // increment to next hpt group addr
        bdnz    fenxg64                 // loop through all groups
        sync                            // ensure all stores done
        b       feloop

//
// We come here if the hpt lock is held by another processor
// when we first attempt to take it.  If the lock value is < 0
// then the lock is held by a processor that is clearing the entire
// hpt (other processor is in KeFlushCurrentTb).  If this happens
// then all we need do is waitfor the lock to become available (we
// don't actually need to take it) and return.
//

#if !defined(NT_UP)

feglk64w:
        crmove  4,0                     // move -ve bit to cr.1
        ENABLE_INTERRUPTS(r.10)
fegwt64:
	lwz     r.7,0(r.9)              // load lock word
        cmpwi   r.7,0                   // is lock available?
        bne-    fegwt64                 // if not available, try again
        li      r.7,-1                  // lock value - hpt clear
        bt      4,feglk64_done          // jif already cleared
        mtmsr   r.11
        cror    cr.7,cr.7,cr.7
        b       feglk64

feglk64_done:
        sync                            // wait till tlb catches up

#endif // NT_UP

	blr				// return

Kseg0CodeEnd:

//--------------------------------------------------------------------------
//
//  KiSystemStartup()
//
//    This is the system entry point for processor 0.  It will copy the
//    exception vectors to low real memory, re-initialize memory mapping
//    for KSEG0 (via BAT0), call the routine to initialize the kernel and
//    fall thru into the idle loop.
//
//  Arguments:
//
//      r3 - supplies address of loader parameter block.
//
//  Return values:
//
//      None.  There is no return from this function.
//
//  Remarks:
//
//      ntoskrnl is assumed to be in memory covered by KSEG0.
//
//--------------------------------------------------------------------------
        FN_TABLE(KiSystemBegin,0,0)

        DUMMY_ENTRY_S(KiSystemBegin,INIT)

        stwu    r.sp, -(STK_MIN_FRAME+8) (r.sp) // This code is never
        mflr    r.0                             // executed, it is here
        stw     r.0,  -(STK_MIN_FRAME+8) (r.sp) // so the unwinder can
                                                // have a good time.

        PROLOGUE_END(KiSystemBegin)

        ALTERNATE_ENTRY_S(KiSystemStartup,INIT)

        ori     r.31, r.3, 0                    // save address of LPB

//
// Disable translation prior to initializing the BAT registers.
//

        bl      ki.real                         // disable address translation
c0start:                                        // this address in r.30


//
// Move the kernel trap handlers.  They need to start at physical
// memory address zero.
//
// The trap handlers do NOT use relative branches outside of the
// area bounded by the labels real0 thru end_of_code_to_move.
//

        li      r.8, -4                         // target address - 4
        li      r.9, (end_of_code_to_move-real0+3)/4 // num words to move
        mtctr   r.9
        subi    r.7, r.30, c0start-real0+4      // source address - 4

//
// Get physical address of ntoskrnl's TOC
//

        lwz     r.toc, rm_toc_pointer-c0start(r.30) // get v address of toc
        rlwinm  r.toc, r.toc, 0, 0x7fffffff     // cvt to phys addr

//
// Set the bit mask identifying the available breakpoint registers.
//   601 = 1 data, 1 instr.
//   603 = 1 instr.
//   604 = 1 data, 1 instr.
//   613 = 1 data, 1 instr.
//   620 = 1 data (instr. too coarse).
//   All others = 0 breakpoint registers.
//

        mfpvr   r.17                    // get processor type & rev
        rlwinm  r.17, r.17, 16, 0xffff  // isolate processor type
        lis     r.4,0x0100              // 0 = data addr bkpt register
                                        // 1 = instruction addr bkpt register
        cmpwi   r.17, PV603             // 603?
        cmpwi   cr.1, r.17, PV603p      // 603+?
        cmpwi   cr.7, r.17, PV603pp     // 603++?
        li      r.6, 4                  // Offset for 603 branch
        beq     setbkp                  // jif we are on a 603
        beq     cr.1,setbkp             // jif we are on a 603+
        beq     cr.7,setbkp             // jif we are on a 603++
        oris    r.4, r.4, 0x1000        // Add data bkpt
        li      r.6, 0                  // Assume 601 branches (default)
        cmpwi   r.17, PV601
        beq     setbkp                  // jif 601
        li      r.6, 8                  // Offset for 604 branch
        cmpwi   r.17, PV604
        cmpwi   cr.1, r.17, PV604p
        beq     setbkp                  // jif 604
        beq     cr.1,setbkp             // jif 604+
        cmpwi   r.17, PV613
        beq     setbkp                  // jif 613
        li      r.6, 12                 // Offset for 620 branch
        lis     r.4, 0x1000             // Data only (instr too coarse)
        cmpwi   r.17, PV620
        beq     setbkp                  // jif 620
        li      r.4, 0                  // Not a known chip. No DRs
        li      r.6, 16                 // Offset for not supported branch

setbkp: lwz     r.5, [toc]KiBreakPoints(r.toc)
        rlwinm  r.5, r.5, 0, 0x7fffffff // get phys address of KiBreakPoints
        stw     r.4, 0(r.5)
        cmpwi   r.6, 0
        lwz     r.28, addr_common_exception_entry-c0start(r.30)
        rlwinm  r.28, r.28, 0, 0x7fffffff // base addr = common_exception_entry
        beq     kicopytraps             // 601 branch is the default

//
// Processor is not a 601, change each of the Debug Register branch
// tables for the appropriate processor.
//

        la      r.5, BranchDr1-common_exception_entry(r.28) // 1st
        lhzx    r.4, r.5, r.6           // Load appropriate branch instruction
        add     r.4, r.4, r.6           // Modify branch relative to table
        sth     r.4, 0(r.5)             // Replace 601 branch

        la      r.5, BranchDr2-common_exception_entry(r.28) // 2nd
        lhzx    r.4, r.5, r.6           // Load appropriate branch instruction
        add     r.4, r.4, r.6           // Modify branch relative to table
        sth     r.4, 0(r.5)             // Replace 601 branch

        la      r.5, BranchDr3-common_exception_entry(r.28) // 3rd
        lhzx    r.4, r.5, r.6           // Load appropriate branch instruction
        add     r.4, r.4, r.6           // Modify branch relative to table
        sth     r.4, 0(r.5)             // Replace 601 branch

        la      r.5, BranchDr4-common_exception_entry(r.28) // 4th
        lhzx    r.4, r.5, r.6           // Load appropriate branch instruction
        add     r.4, r.4, r.6           // Modify branch relative to table
        sth     r.4, 0(r.5)             // Replace 601 branch

kicopytraps:

        lwzu    r.10, 4(r.7)                    // copy trap handler to low
        stwu    r.10, 4(r.8)                    // memory (word by word)
        bdnz    kicopytraps

//
// 603e/ev Errata 19 work around.
//
// r.17 has the (shifted) PVR in it.  If this is a 603e/603ev, turn on guarded
// storage for data TLB entries by modifying the TLB miss handler.
//

        cmpwi   r.17, PV603p                    // 603e?
        beq     do_errata_19_tlb                // modify instruction if so
	cmpwi   r.17, PV603pp                   // 603ev?
	bne     skip_errata_19_tlb              // skip modification if not
do_errata_19_tlb:
        lwz     r.10, tlbld603-real0(0)         // load TLB handler instruction
        ori     r.10, r.10, PTE_GUARDED         // turn on guarded storage
        stw     r.10, tlbld603-real0(0)         // store TLB handler instruction
skip_errata_19_tlb:

//
// Force sdr1 (address of HPT) to 0 until HPT allocated.  This is necessary
// because some HALs use sdr1 to get an address in KSEG0 to use for cache
// flushing.  Setting it to 0 ensures that the calculated address will be
// in KSEG0.
//

        li      r.3, 0
        mtsdr1  r.3

//
// Move the code that needs to be in KSEG0.
//

        rlwinm  r.3, r.31, 0, 0x7fffffff // get real address of LPB
        lwz     r.8, LpbKernelKseg0PagesDescriptor(r.3)
        clrlwi  r.8, r.8, 1
        lwz     r.8, MadBasePage(r.8)
        slwi    r.8, r.8, PAGE_SHIFT
        ori     r.6, r.8, 0             // save KSEG0 code address
        subi    r.8, r.8, 4
        li      r.9, (Kseg0CodeEnd-Kseg0CodeStart+3) >> 2 // num words to move
        mtctr   r.9
        subi    r.7, r.30, c0start-Kseg0CodeStart+4       // source address - 4
kicopykseg0:
        lwzu    r.10, 4(r.7)                    // copy trap handler to low
        stwu    r.10, 4(r.8)                    // memory (word by word)
        bdnz    kicopykseg0

//
// Fix the branches into KSEG0 code.  These are built as relative
// branches with an offset from Kseg0CodeStart, so we need to add the
// offset from the instruction to the base of the KSGE0 code to the
// offset in the instruction.
//

        la      r.7, KiPriorityExitRfiJump1-common_exception_entry(r.28)
        lwz     r.4, 0(r.7)
        rlwinm  r.5, r.4, 0, 0x03fffffc
        add     r.5, r.5, r.6
        sub     r.5, r.5, r.7
        rlwimi  r.4, r.5, 0, 0x03fffffc
        stw     r.4, 0(r.7)

        la      r.7, KiPriorityExitRfiJump2-common_exception_entry(r.28)
        lwz     r.4, 0(r.7)
        rlwinm  r.5, r.4, 0, 0x03fffffc
        add     r.5, r.5, r.6
        sub     r.5, r.5, r.7
        rlwimi  r.4, r.5, 0, 0x03fffffc
        stw     r.4, 0(r.7)

        la      r.7, KiServiceExitKernelRfiJump-common_exception_entry(r.28)
        lwz     r.4, 0(r.7)
        rlwinm  r.5, r.4, 0, 0x03fffffc
        add     r.5, r.5, r.6
        sub     r.5, r.5, r.7
        rlwimi  r.4, r.5, 0, 0x03fffffc
        stw     r.4, 0(r.7)

        la      r.7, KiServiceExitUserRfiJump-common_exception_entry(r.28)
        lwz     r.4, 0(r.7)
        rlwinm  r.5, r.4, 0, 0x03fffffc
        add     r.5, r.5, r.6
        sub     r.5, r.5, r.7
        rlwimi  r.4, r.5, 0, 0x03fffffc
        stw     r.4, 0(r.7)

        lwz     r.7, addr_RtlpRestoreContextRfiJump-c0start(r.30)
        clrlwi  r.7, r.7, 1
        lwz     r.4, 0(r.7)
        li      r.5, RtlpRestoreContextRfi-Kseg0CodeStart
        add     r.5, r.5, r.6
        sub     r.5, r.5, r.7
        rlwimi  r.4, r.5, 0, 0x03fffffc
        stw     r.4, 0(r.7)

//
// Change the function descriptor for KiStartProcessor so that it points
// to Kseg0Code.StartProcessor.  Fix up the instructions in
// Kseg0Code.StartProcessor that load the address of the real
// KiStartProcessor.
//
// N.B. The only reference to KiStartProcessor is in KeStartAllProcessors,
//      which references through the function descriptor.  There are no
//      direct calls to KiStartProcessor.
//

        lwz     r.5, [toc]KiStartProcessor(r.toc)
        clrlwi  r.5, r.5, 1
        addi    r.4, r.6, StartProcessor-Kseg0CodeStart
        oris    r.4, r.4, K_BASE
        stw     r.4, 0(r.5)

        la      r.7, cnstart-c0start(r.30)
        addi    r.4, r.6, StartProcessor.LoadKiStartProcessorAddress-Kseg0CodeStart
        lwz     r.5, 0(r.4)
        rlwimi  r.5, r.7, 16, 0xffff
        stw     r.5, 0(r.4)
        lwz     r.5, 4(r.4)
        rlwimi  r.5, r.7, 0, 0xffff
        stw     r.5, 4(r.4)

//
// Fix up the KiFlushSingleTb, KeFillEntryTb, and KeFlushCurrentTb functions.
//
// The first thing in KeFlushCurrentTb is a load immediate of the
// number of TLB congruence classes for this procesor.  It is set
// to a default of 128, we adjust that to the correct value now.
//

        lhz     r.4, LpbNumberCongruenceClasses(r.3)
        lwz     r.7, flct0-Kseg0CodeStart(r.6) // load current li instruction
        rlwimi  r.7, r.4, 0, 0x0000ffff // merge number congruence classes
                                        // into li instruction
        stw     r.7, flct0-Kseg0CodeStart(r.6) // replace instruction

        lis     r.7, 0x6000             // r.7 now contains a no-op

//
// Determine memory management model: 32-bit (default) or 64-bit.
//
        lbz     r.5, LpbMemoryManagementModel(r.3)
        cmpwi   r.5, 64                 // 64-bit support?
        beq     ikms64                  // jif 64-bit support needed

//
// Is this a 601?  If so, the upper 16 bits of the PVR would contain
// 0x0001 and we don't care about the lower 16 bits.  By checking to
// see if any of the upmost bits are non-zero we can determine if it
// is (or is not) a 601.  If if is not a 601, the result of the follow-
// ing struction (in cr.0) will be "not equal".  If this processor is
// a 601 we remove the tlbsync/sync sequences in KiFlushSingleTb,
// KeFillEntryTb and KeFlushCurrentTb.
//

#if !defined(NT_UP)

        cmpwi   r.17, PV601
        bne     ikms10                  // jif not a 601

        stw     r.7, flst1-Kseg0CodeStart(r.6) // nop KiFlushSingleTb tlbsync
        stw     r.7, flst2-Kseg0CodeStart(r.6) //                     sync

        stw     r.7, fiet1-Kseg0CodeStart(r.6) // nop KeFillEntryTb tlbsync
        stw     r.7, fiet2-Kseg0CodeStart(r.6) //                   sync

        stw     r.7, flct3-Kseg0CodeStart(r.6) // nop KeFlushCurrentTb tlbsync
        stw     r.7, flct4-Kseg0CodeStart(r.6) //                      sync

ikms10:

#endif

//
// Fix KiFlushSingleTb, KeFillEntryTb and KeFlushCurrentTb HPT usage
//

        lhz     r.5, LpbHashedPageTableSize(r.3)
        cmpwi   r.5, 0                  // does this processor use a HPT?
        beq     ikms20                  // if processor does not use an
                                        // HPT, leave branches alone

        stw     r.7, flst0-Kseg0CodeStart(r.6) // allow fall-thru to HPT case.
        stw     r.7, fiet0-Kseg0CodeStart(r.6) // allow fall-thru to HPT case.
        stw     r.7, flct1-Kseg0CodeStart(r.6) // no-op branch around hpt clear

#if !defined(NT_UP)
        stw     r.7, flct2-Kseg0CodeStart(r.6) // no-op branch around hpt clear
#endif

ikms20:

//
// Modify the branch instructions at KiFlushSingleTb, KeFillEntryTb, and
// KeFlushCurrentTb so that they point to the real routines in KSEG0.
//
// Also, modify the function descriptors for KiFlushSingleTb,
// KeFillEntryTb, and KeFlushCurrentTb so that they point to the real
// routines in KSEG0.
//

        la      r.7, FlushSingleTbJump-common_exception_entry(r.28)
        lwz     r.4, 0(r.7)
        rlwinm  r.5, r.4, 0, 0x03fffffc
        add     r.5, r.5, r.6
        sub     r.5, r.5, r.7
        rlwimi  r.4, r.5, 0, 0x03fffffc
        stw     r.4, 0(r.7)

        la      r.7, FillEntryTbJump-common_exception_entry(r.28)
        lwz     r.4, 0(r.7)
        rlwinm  r.5, r.4, 0, 0x03fffffc
        add     r.5, r.5, r.6
        sub     r.5, r.5, r.7
        rlwimi  r.4, r.5, 0, 0x03fffffc
        stw     r.4, 0(r.7)

        la      r.7, FlushCurrentTbJump-common_exception_entry(r.28)
        lwz     r.4, 0(r.7)
        rlwinm  r.5, r.4, 0, 0x03fffffc
        add     r.5, r.5, r.6
        sub     r.5, r.5, r.7
        rlwimi  r.4, r.5, 0, 0x03fffffc
        stw     r.4, 0(r.7)

        oris    r.5, r.6, K_BASE

        lwz     r.7, [toc]KiFlushSingleTb(r.toc)
        clrlwi  r.7, r.7, 1
        la      r.4, FlushSingleTb-Kseg0CodeStart(r.5)
        stw     r.4, 0(r.7)

        lwz     r.7, [toc]KeFillEntryTb(r.toc)
        clrlwi  r.7, r.7, 1
        la      r.4, FillEntryTb-Kseg0CodeStart(r.5)
        stw     r.4, 0(r.7)

        lwz     r.7, [toc]KeFlushCurrentTb(r.toc)
        clrlwi  r.7, r.7, 1
        la      r.4, FlushCurrentTb-Kseg0CodeStart(r.5)
        stw     r.4, 0(r.7)

ikms30:

//
// KeZeroPage defaults to using an FP store loop which is faster than
// a dcbz loop on 603 class processors.  If this is not one of those
// processors, replace the branch at kzp.repl with an ISYNC instruction.
//
// N.B. The caches will get flushed (explicitly) before KeZeroPage is
// ever used so we don't worry about it here.
//

        cmpwi   cr.0, r.17, PV603
        cmpwi   cr.1, r.17, PV603p
        cmpwi   cr.7, r.17, PV603pp
        beq     cr.0, kzp.adjust.end    // jif 603
        beq     cr.1, kzp.adjust.end    // jif 603e
        beq     cr.7, kzp.adjust.end    // jif 603ev

        lis     r.12, 0x4c00            // generate an isync instruction
        ori     r.12, r.12, 0x012c      // 4c00012c.
        stw     r.12, kzp.repl-common_exception_entry(r.28)

kzp.adjust.end:

//
// Set address of Processor Control Region for Kernel (KiPcr).
//

        lwz     r.12, LpbPcrPage(r.3)   // Get PCR page number
        slwi    r.12, r.12, PAGE_SHIFT  // convert to real byte address
        mtsprg  sprg.0, r.12            // Real addr of KiPcr in SPRG 0
        oris    r.11, r.12, K_BASE      // Virt addr of KiPcr in kernel
        mtsprg  sprg.1, r.11            // virtual space in SPRG 1.

//
// Initialize first process PD address, PDEs for PD and hyper PT.
//

        lwz     r.1, LpbPdrPage(r.3)    // pnum of PD,hyPT left by OS Ldr
        slwi    r.1, r.1, PAGE_SHIFT    // make it a real address
        stw     r.1, PcPgDirRa(r.12)    // store in PCR for HPT misses
        b       set_segment_registers

//---------------------------------------------------------------------------
//
// Set up for 64 bit memory management model
//
//---------------------------------------------------------------------------

ikms64:

//
// Modify the branch instructions at KiFlushSingleTb, KeFillEntryTb,
// KeFlushCurrentTb so that they point to the real routines in KSEG0.
//
// Also, modify the function descriptors for KiFlushSingleTb,
// KeFillEntryTb, and KeFlushCurrentTb so that they point to the real
// routines in KSEG0.
//
// Finally, fix up the DSI and ISI code to use correct "tail".
//

        la      r.7, FlushSingleTbJump-common_exception_entry(r.28)
        lwz     r.4, 0(r.7)
        rlwinm  r.5, r.4, 0, 0x03fffffc
        add     r.5, r.5, r.6
        addi    r.5, r.5, FlushSingleTb64-FlushSingleTb
        sub     r.5, r.5, r.7
        rlwimi  r.4, r.5, 0, 0x03fffffc
        stw     r.4, 0(r.7)

        la      r.7, FillEntryTbJump-common_exception_entry(r.28)
        lwz     r.4, 0(r.7)
        rlwinm  r.5, r.4, 0, 0x03fffffc
        add     r.5, r.5, r.6
        addi    r.5, r.5, FillEntryTb64-FillEntryTb
        sub     r.5, r.5, r.7
        rlwimi  r.4, r.5, 0, 0x03fffffc
        stw     r.4, 0(r.7)

        la      r.7, FlushCurrentTbJump-common_exception_entry(r.28)
        lwz     r.4, 0(r.7)
        rlwinm  r.5, r.4, 0, 0x03fffffc
        add     r.5, r.5, r.6
        addi    r.5, r.5, FlushCurrentTb64-FlushCurrentTb
        sub     r.5, r.5, r.7
        rlwimi  r.4, r.5, 0, 0x03fffffc
        stw     r.4, 0(r.7)

        oris    r.5, r.6, K_BASE

        lwz     r.7, [toc]KiFlushSingleTb(r.toc)
        clrlwi  r.7, r.7, 1
        la      r.4, FlushSingleTb64-Kseg0CodeStart(r.5)
        stw     r.4, 0(r.7)

        lwz     r.7, [toc]KeFillEntryTb(r.toc)
        clrlwi  r.7, r.7, 1
        la      r.4, FillEntryTb64-Kseg0CodeStart(r.5)
        stw     r.4, 0(r.7)

        lwz     r.7, [toc]KeFlushCurrentTb(r.toc)
        clrlwi  r.7, r.7, 1
        la      r.4, FlushCurrentTb64-Kseg0CodeStart(r.5)
        stw     r.4, 0(r.7)
//
// Set up branch to tpte64 instead of tpte -- done this way in consideration
// of cache line size
//
// First handle DSI
//
        li      r.7, 0x300
        lwz     r.4, dsi20-dsientry(r.7)
        addi    r.4, r.4, dsi20-dsi10
        stw     r.4, dsi10-dsientry(r.7)

// Now handle ISI

        li      r.7, 0x400
        lwz     r.4, isi20-isientry(r.7)
        addi    r.4, r.4, isi20-isi10
        stw     r.4, isi10-isientry(r.7)

        b       ikms30


//--------------------------------------------------------------------------
//
//  KiStartProcessor()
//
//    This is the system entry point for processors other than processor 0.
//    It will re-initialize memory mapping for KSEG0 (via BAT0), call the
//    routine to initialize the kernel and fall thru into the idle loop.
//
//  Arguments:
//
//      r3 - supplies address of loader parameter block.
//      r4 - supplies the per-processor virtual address of the PCR (not 0xffffd000)
//
//  Return values:
//
//      None.  There is no return from this function.
//
//  Remarks:
//
//      ntoskrnl is assumed to be in memory covered by KSEG0.
//
//--------------------------------------------------------------------------

        ALTERNATE_ENTRY_S(KiStartProcessor,INIT)

cnstart:                                        // this address in r.30

        lwz     r.toc, rm_toc_pointer-cnstart(r.30)// get kernel toc pointer
        rlwinm  r.toc, r.toc, 0, 0x7fffffff     // cvt to phys addr

//
//  Set address of Processor Control Region for Kernel (KiPcr)
//

        rlwinm  r.3, r.31, 0, 0x7fffffff// get real address of LPB
        lwz     r.12, LpbPcrPage(r.3)   // Get PCR page number
        slwi    r.12, r.12, PAGE_SHIFT  // convert to real byte address
        mtsprg  sprg.0, r.12            // physical addr of KiPcr in SPRG 0
        mtsprg  sprg.1, r.4             // virtual  addr of KiPcr in SPRG 1.

//
// Initialize PageDirectory address in this PCR.
//

        lwz     r.1, LpbPdrPage(r.3)    // pnum of PD,hyPT left by OS Ldr
        slwi    r.1, r.1, PAGE_SHIFT    // make it a real address
        stw     r.1, PcPgDirRa(r.12)    // store in PCR for HPT misses

//
// The following code is executed at startup on ALL processors.
//

set_segment_registers:

//
// Set the Storage Descriptor Register (address of the Hashed Page
// Table) for this processor.
//
// Note: This implementation requires the HPT be allocated at an
// address < 4GB on 64 bit PowerPC implementations.  As it is
// (currently) required that the HPT be addressable in KSEG0
// this should not be of significant concern.
//

        lhz     r.10, LpbHashedPageTableSize(r.3)
        slwi    r.10, r.10, PAGE_SHIFT
        subi    r.10, r.10, 1
        lwz     r.1, LpbHashedPageTable(r.3)
        rlwimi  r.1, r.10, 16, 0x1ff
        mtsdr1  r.1

//
// Invalidate most segment registers.
//

        lis     r.0, SREG_INVAL                 // invalid segment register value
        mtsr    0,   r.0
        mtsr    1,   r.0
        mtsr    2,   r.0
        mtsr    3,   r.0
        mtsr    4,   r.0
        mtsr    5,   r.0
        mtsr    6,   r.0
        mtsr    7,   r.0
        mtsr    11,  r.0
        mtsr    15,  r.0                        // temp set 15 invalid

//
// Initialize segment register 12.  Assume initial PID = 0.
//

        li      r.10, 12                // T=0, Ks,Kp=0, VSID=PID,12
        mtsr    12, r.10

//
// Initialize the global segment registers 8, 9, 10, 13, 14
//

        li      r.10, 8
        oris    r.10, r.10, 0x2000      // T=0 Ks=0 Kp=1 VSID=14
        mtsr    8, r.10
        li      r.10, 9                 // T=0 Ks,Kp=0 VSID=9
        mtsr    9, r.10
        li      r.10, 10                // T=0 Ks,Kp=0 VSID=10
        mtsr    10, r.10
        li      r.10, 13                // T=0 Ks,Kp=0 VSID=13
        mtsr    13, r.10
        li      r.10, 14
        oris    r.10, r.10, 0x2000      // T=0 Ks=0 Kp=1 VSID=14
        mtsr    14, r.10

//
//  Set BAT0 so we have block address translation for the low part of KSEG0
//
//  Virtual address layout is as follows
//
//  4GB         ---------------------------------       FFFFFFFF
//              |       Non Paged Pool          |
//              |                               |
//              - - - - - - - - - - - - - - - - -
//              |       Paged Pool              |
//              |                               |       90000000 *
//           -- ---------------------------------
//           |  |                               |
//      BAT0 |  |       Kernel and HAL          |
//           |  |                               |       80000000 **
//  2GB      -- ---------------------------------
//              |                               |
//              |                               |
//              |                               |
//              |       User Space              |
//              |                               |
//              |                               |
//              |                               |
//              |                               |
//              |                               |
//              |                               |
//              |                               |       0
//  0GB         ---------------------------------
//
//      *    On MIPS this is C0000000, however, we can cover only
//           256MB with a BAT and why not allocate that space to
//           the paged pool?
//      **   Mapped to physical address 0.
//
//  WARNING: 601 BAT registers are incompatible with other 60x
//           BAT registers.
//
//  Set BAT0 to virtual 0x80000000, physical 0 for max size.
//  (max size = 8MB for 601, 256MB for other 60x processors).
//  BAT effective page size = 128KB, so, BEPI = 2^31 / 2^17
//  = 2^(31-17) = 2^14 = 16K entries,.....  but this field is
//  left justified, ie left shift 17 bits to position in reg ...
//  in other words, just take the base address and slam into BLPI
//  without adjusting.
//
//  Set the following control bits
//
//  W   Write Thru              0
//  I   Inhibit Caching         0
//  M   Memory Coherency        1
//
//  want key = 0 for supervisor mode, 1 for user mode, so
//  Ks = 0
//  Ku = 1
//  want all access in supervisor mode, none in user mode, so
//  PP = 0b00
//

        lwz     r.11, LpbKseg0Top(r.3) // Get VA of byte above KSEG0
        stw     r.11, PcKseg0Top(r.12)  // Copy into PCR

        mfpvr   r.6                     // check processor type
        rlwinm  r.6,r.6,16,16,31        // extract processor version
        cmpwi   r.6,PV601
        bne     bat_not_601

        li      r.10, 0b0010100         // WIM | Ks | Ku | PP
        oris    r.10, r.10, K_BASE      // set BLPI

//
//  Set BSM all 1s and Valid bit.
//  PBN (Physical Block Number) = 0.
//

        clrlwi  r.11, r.11, 1                   // get size of KSEG0 (turn off 0x80000000)
        subi    r.11, r.11, 1                   // convert to mask
        rlwinm  r.11, r.11, 32-17, 0x3f         // mask >> 17 == block length mask
        ori     r.11, r.11, 0x40                // set V

        mtbatl  0, r.11         // set BAT0
        mtbatu  0, r.10

//
// Clear Valid bit in BATs 1, 2 and 3
//

        li      r.0, 0
        mtbatl  1, r.0
        mtbatl  2, r.0
        mtbatl  3, r.0

        b       skip_bat_not_601

bat_not_601:

//
// Clear Valid bits in ALL BATs prior to setting any of them.
//

        li      r.0, 0
        mtdbatu  0, r.0
        mtdbatl  0, r.0
        mtdbatu  1, r.0
        mtdbatl  1, r.0
        mtdbatu  2, r.0
        mtdbatl  2, r.0
        mtdbatu  3, r.0
        mtdbatl  3, r.0

        mtibatu  0, r.0
        mtibatl  0, r.0
        mtibatu  1, r.0
        mtibatl  1, r.0
        mtibatu  2, r.0
        mtibatl  2, r.0
        mtibatu  3, r.0
        mtibatl  3, r.0

        isync

//
// Set BAT0 to cover KSEG0.
//
// Set Block Effective Page Index (ie effective address) to 2GB,
// BL to 8MB, Valid Supervisor mode, not Valid Problem mode.
//

        clrlwi  r.11, r.11, 1                   // get size of KSEG0 (turn off 0x80000000)
        subi    r.11, r.11, 1                   // convert to mask
        rlwinm  r.11, r.11, 32-15, 0x1ffc       // mask >> 17 << 2 == block length mask
        ori     r.11, r.11, 2                   // set Vs (Vp is off)
        oris    r.11, r.11, K_BASE              // set BEPI (0x80000000)

//
//  BRPN (Block Real Page Number) = 0.  PP for Supervisor
//  read/write.
//
//                      ------- W - Write thru       0
//                      |------ I - Inhibit Cache    0
//                      ||----- M - Memory Coherency REQUIRED
//                      |||---- G - Guard bit        0
//                      ||||--- Reserved
//                      |||||
        li      r.10, 0b0010010 // BRPN | WIMG | PP

        mtibatl 0, r.10         // set IBAT0
        mtibatu 0, r.11

//
// 603e/ev Errata 19 work around.
//
// r.6 has the (shifted) PVR in it.  If this is a 603e/603ev, turn on guarded
// storage for the DBAT.  It is illegal to set G=1 for the IBAT.
//

        cmpwi   r.6, PV603p                     // 603e?
        beq     do_errata_19_bat                // modify instruction if so
	cmpwi   r.6, PV603pp                    // 603ev?
	bne     skip_errata_19_bat              // skip modification if not
do_errata_19_bat:
        ori     r.10, r.10, PTE_GUARDED		// turn on guarded storage
skip_errata_19_bat:

        mtdbatl 0, r.10         // set DBAT0
        mtdbatu 0, r.11


skip_bat_not_601:

//
//  Initialize the Processor Control Region (PCR).
//

        li      r.11, PCR_MINOR_VERSION         // set minor version number
        sth     r.11, PcMinorVersion(r.12)
        li      r.11, PCR_MAJOR_VERSION         // set major version number
        sth     r.11, PcMajorVersion(r.12)

#if DBG

        lhz     r.11, LpbIcacheMode(r.3)        // get,set cache modes
        stb     r.11, PcIcacheMode(r.12)
        rlwinm  r.11, r.11, 0x18, 0x18, 0x1f
        stb     r.11, PcDcacheMode(r.12)

#endif

        lwz     r.11, LpbPcrPage2(r.3)          // Get PCR2 page number
        slwi    r.11, r.11, PAGE_SHIFT          // convert to real byte address
        stw     r.11, PcPcrPage2(r.12)          // Store in PCR

//
//  Initialize the addresses of various data structures that are
//  referenced from the exception and interrupt handling code.
//
//  N.B. The panic stack is a separate stack that is used when
//       the current kernel stack overlfows.
//
//  N.B. The interrupt stack is a separate stack and is used to
//       process all interrupts that run at IRQL 3 and above.
//

        lwz     r.11, LpbPrcb(r.3)              // set processor block address
        stw     r.11, PcPrcb(r.12)
        lwz     r.1,  LpbKernelStack(r.3)       // set initial stack address
        stw     r.1,  PcInitialStack(r.12)
        lwz     r.11, LpbPanicStack(r.3)        // set panic stack address
        stw     r.11, PcPanicStack(r.12)
        lwz     r.11, LpbInterruptStack(r.3)    // set interrupt stack address
        stw     r.11, PcInterruptStack(r.12)
        lwz     r.11, LpbThread(r.3)            // set current thread address
        stw     r.11, PcCurrentThread(r.12)

//
// Get the first level data and instruction cache values from the loader
// parameter block and move them into the PCR.
//

        lwz     r.23,  LpbFirstLevelDcacheSize(r.3)
        lwz     r.24,  LpbFirstLevelDcacheFillSize(r.3)
        lwz     r.25,  LpbFirstLevelIcacheSize(r.3)
        lwz     r.26,  LpbFirstLevelIcacheFillSize(r.3)

        stw     r.23,  PcFirstLevelDcacheSize(r.12)
        addi    r.23,  r.24, -1
        stw     r.24,  PcFirstLevelDcacheFillSize(r.12)
        stw     r.24,  PcDcacheFillSize(r.12)
        stw     r.23,  PcDcacheAlignment(r.12)

        addi    r.23,  r.26, -1
        stw     r.25,  PcFirstLevelIcacheSize(r.12)
        stw     r.26,  PcFirstLevelIcacheFillSize(r.12)
        stw     r.26,  PcIcacheFillSize(r.12)
        stw     r.23,  PcIcacheAlignment(r.12)

//
// Set the second level data and instruction cache fill size and size.
//

        lwz     r.23, LpbSecondLevelDcacheSize(r.3)
        lwz     r.24, LpbSecondLevelDcacheFillSize(r.3)
        lwz     r.25, LpbSecondLevelIcacheSize(r.3)
        lwz     r.26, LpbSecondLevelIcacheFillSize(r.3)

        stw     r.23, PcSecondLevelDcacheSize(r.12)
        stw     r.24, PcSecondLevelDcacheFillSize(r.12)
        stw     r.25, PcSecondLevelIcacheSize(r.12)
        stw     r.26, PcSecondLevelIcacheFillSize(r.12)

//
//  Set current IRQL to highest value
//

        li      r.11, HIGH_LEVEL
        stb     r.11, PcCurrentIrql(r.12)

//
// Compute address of Loader Parameter Block into r.8 where it will
// remain for the call to KiInitializeKernel.
//

        oris    r.8, r.3, K_BASE                // LoaderBlock |= KSEG0_BASE

//
//  Get processor into mapped mode
//

        bl      ki.virtual

//  **** PROCESSOR NOW IN VIRTUAL MODE ****

//  For the remainder of this module, register usage is compliant with the
//  standard linkage conventions for little-endian PowerPC.
//

//
//  Get virtual address of ntoskrnl's TOC
//

        oris    r.toc, r.toc, K_BASE            // TOC is in KSEG0

//
//  Buy stack frame
//

        subi    r.1, r.1,  STK_MIN_FRAME+8
        li      r.13, 0                         // zero back chain and friends
        stw     r.13, 0(r.1)                    // initialize teb
        stw     r.13, 4(r.1)
        stw     r.13, 8(r.1)

//
//  Setup arguments and call kernel initialization procedure
//
//  KiInitializeKernel(
//                      IdleProcess,
//                      IdleThread,
//                      IdleStack,
//                      Prcb,
//                      CpuNumber,
//                      LoaderParameterBlock
//                    )
//

        lwz     r.3,  LpbProcess(r.8)           // get idle process address
        lwz     r.4,  LpbThread(r.8)            // get idle thread address
        lwz     r.5,  LpbKernelStack(r.8)       // get idle thread stack address
        lwz     r.6,  LpbPrcb(r.8)              // get processor block address
        lbz     r.7,  PbNumber(r.6)             // get processor number

//
// Set segment register 15 to a unique vsid for this processor.  This vsid
// also has the SREG_INVAL bit set so a dmiss or dtlb miss to any address
// will be shunted to the storage error path rather than filling the entry
// from the "shared" NT page tables.
//

        lis     r.0, SREG_INVAL|(SREG_INVAL>>1) // special marker for segment f
        or      r.0, r.0, r.7                   // VSID = 0b11,procnum
        oris    r.0, r.0, 0x2000                // T=0 Ks=0 Kp=1
        mtsr    15,  r.0
        isync

        bl      ..KiInitializeKernel

        bl      ..KiIdleLoop

        DUMMY_EXIT (KiSystemBegin)

//--------------------------------------------------------------------------
//
//  ki.virtual  switch kernel from unmapped instructions and data to
//              mapped.
//
//              Kernel is loaded into memory at real address 0,
//              virtual address 0x80000000.
//
//              On exit, MSR_IR and MSR_DR must be set and the return
//              address must have been adjusted such that execution
//              continues at the virtual address equivalent to the real
//              address in LR on entry.
//
//              The change of state and transfer are accomplished atomically
//              by setting the target address and state for return from
//              interrupt then using rfi to put these changes into effect.
//
//  Entry Requirements:
//              Processor executing in supervisor state.
//
//  Returns to next instruction in caller.
//      MSR:    Instruction Relocate ON
//              Data Relocate ON
//
//--------------------------------------------------------------------------

ki.virtual:
        mflr    r.0                     // save return address

#if DBG

//
// This section of code determines the caching mode for the kernel.
// Based on processor type and values in the PCR, either set HID0 to
// turn off the caches (603/604), or do nothing (601).
//
        mfpvr   r.11                    // get processor type
        lhz     r.12, PcIcacheMode(r.12) // get I/D caching information
        srwi    r.11, r.11, 16          // extract processor version
        cmpli   cr.6, 0, r.11, PV601    // cr.6 -> is this a 601?
        rlwinm  r.4, r.12, 0x0, 0x18, 0x1f      // r.4  -> i-cache mode
        rlwinm  r.5, r.12, 0x18, 0x18, 0x1f     // r.5  -> d-cache mode
        beq-    cr.6, cache_done         // branch if on 601,  nothing to do

//
// Set cache bits for HID0 on 603/604 family chips and 620
//
        cmpli   cr.6, 0, r.4, 0         // cr.6 -> any bits set for icache?
        addi    r.6, r.0, 0             // r.6 -> 0
        ori     r.6, r.6, 0xc000        // r.6 -> 0xc000 (I/D caches on)
        beq+    cr.6, cache_d           // branch if no bits set for icache
        xori    r.6, r.6, 0x8000        // r.6 -> 0x4000 (I cache off)

cache_d:
        cmpli   cr.6, 0, r.5, 0         // cr.6 -> any bits set for dcache?
        beq+    cr.6, cache_done        // branch if no bits set
        xori    r.6, r.6, 0x4000        // r.6 -> 0x[80]000 (turn off dcache)

//
// At this point r.6 has the bits to or into the register that we are
// going to place in HID0.  Possible values are:
//          0xc000 : I cache ON,  D cache ON
//          0x8000 : I cache ON,  D cache OFF
//          0x4000 : I cache OFF, D cache ON
//          0x0000 : I cache OFF, D cache OFF
//
// N.B. r.6 is NOT set for 601
//
cache_done:
#endif

//
// Acquire the HPT lock to ensure that tlbie/tlbsync is done on
// only one processor at a time.
//

#if !defined(NT_UP)
        li      r.10,HPT_LOCK           // get hpt lock address
kv.getlk:
        lwarx   r.11,0,r.10             // load and reserve lock word
        cmpwi   r.11,0                  // is lock available?
        mfsprg  r.11,sprg.0             // get processor ctl region addr
        bne-    kv.getlk_spin           // loop if lock is unavailable
        stwcx.  r.11,0,r.10             // store conditional to lock word
        bne-    kv.getlk_spin           // loop if lost reserve
        isync                           // context synchronize
        b       kv.getlk_got
kv.getlk_spin:
        lwz     r.11,0(r.10)
        cmpwi   r.11,0
        beq+    kv.getlk
        b       kv.getlk_spin
kv.getlk_got:
#endif // NT_UP

//
// before switching, flush L1 cache just to be sure everything that was
// loaded is really in memory.  We flush the data cache and invalidate
// the instruction cache.  Currently the largest D cache is the unified
// I/D cache on the 601 at 32KB.  We flush the D-Cache by loading 256KB
// worth of data.  This is more than can be contained in the largest
// anticipated cache.
//
// The HAL can't be used for this function yet because mapping isn't
// enabled.
//
// N.B. We do the cache flushing/invalidation in blocks of 32 bytes
// which is the smallest PowerPC cache block size.
//

        li      r.11, 256*1024/32       // amount to load, in blocks
        mtctr   r.11
        li      r.10, -32               // start address - sizeof cache block

//
// Touch 256 K bytes
//

lcache:
        lbzu    r.11, 32(r.10)
        bdnz    lcache

//
// Invalidate the TLB.  We just outright invalidate 256 congruence classes,
// the largest known number is currently the 601 which has 128.   256 will
// hopefully allow for expansion.
//

        li      r.7,256                 // default num congruence classes
        mtctr   r.7
tlbi:   tlbie   r.7                     // invalidate entry
        addi    r.7,r.7, 0x1000         // bump to next page
        bdnz    tlbi

        sync                            // wait tlbie completion

//
// Depending on processor type, invalidate the instruction cache and
// enable both instruction and data caches.
//

        mfpvr   r.12                    // check processor type
        srwi    r.12, r.12, 16          // extract processor version
        cmpwi   cr.1, r.12, PV601       // is this a 601?
        cmpwi   cr.6, r.12, PV603       // is this a 603?
        cmpwi   cr.7, r.12, PV603p      // perhaps a 603+?
        cmpwi   cr.0, r.12, PV603pp     // perhaps a 603++?
        beq     cr.1, go_virtual        // branch is 601

//
// not a 601, on MP systems, wait for tlb propagation.
//

#if !defined(NT_UP)

        tlbsync
        sync

#endif

        beq     cr.6, caches_603        // branch if 603
        beq     cr.7, caches_603        // branch if 603+
        beq     cr.0, caches_603        // branch if 603++
        cmpwi   cr.6, r.12, PV604       // is this a 604?
        cmpwi   cr.7, r.12, PV604p      // is this a 604+?
        beq     cr.6, caches_604        // branch if 604
        beq     cr.7, caches_604        // branch if 604+
	cmpwi	cr.0, r.12, PV613	// is this a 613?
	cmpwi	cr.7, r.12, PV620	// is this a 620?
	beq	cr.0, caches_613	// branch if 613
	beq	cr.7, caches_620	// branch if 620
        b       caches_unknown          // branch to handle unknown cases

//  THIS IS 603 SPECIFIC ...

caches_603:

        mfspr   r.12, hid0              // get hid0
        rlwinm  r.12, r.12, 0, 0x0fff   // clear ICE, DCE, ILOCK, DLOCK
        ori     r.11, r.12, 0xc00       // flash instruction and data caches
        mtspr   hid0, r.11              // set ICFI and DCFI
        mtspr   hid0, r.12              // clear ICFI and DCFI
#if !DBG
        ori     r.12, r.12, 0xc000      // enable instruction and data caches
#else
        or      r.12, r.12, r.6         // set established cache inhibit bits
        sync
        isync
#endif
        mtspr   hid0, r.12
        b       go_virtual              // switch modes

//  THIS IS 604/604+ SPECIFIC ...

caches_604:

        mfspr   r.12,hid0               // get hid0
#if DBG
        rlwinm  r.12,r.12,0x0,0x12,0xf  // clear cache enables
        ori     r.12,r.12,h0_604_sse+h0_604_bhte+h0_604_icia+h0_604_dcia
        or      r.12,r.12,r.6           // or in cache modes
        sync
        isync
#else
        ori     r.12,r.12,h0_604_prefered// enable all the things we want
        ori     r.12,r.12,h0_604_icia+h0_604_dcia // and invalidate both
                                        // caches.  604 clears ICIA and DCIA
                                        // automatically.
#endif
        mtspr   hid0,r.12


        b       go_virtual              // fall thru to switch modes

//  THIS IS 613 SPECIFIC ...

caches_613:

        mfspr   r.12,hid0               // get hid0
#if DBG
        rlwinm  r.12,r.12,0x0,0x12,0xf  // clear cache enables
        ori     r.12,r.12,h0_613_sge+h0_613_btic+h0_613_bhte+h0_613_icia+h0_613_dcia
        or      r.12,r.12,r.6           // or in cache modes
        sync
        isync
#else
        ori     r.12,r.12,h0_613_preferred // enable all the things we want
        ori     r.12,r.12,h0_613_icia+h0_613_dcia // and invalidate both
                                        // caches.  613 clears ICIA and DCIA
                                        // automatically.
#endif
        mtspr   hid0,r.12
        b       go_virtual              // fall thru to switch modes

//  THIS IS 620 SPECIFIC ...

caches_620:

        mfspr   r.12,hid0               // get hid0

#if DBG
        rlwinm  r.12,r.12,0x0,0x12,0xf  // clear cache enables
        ori     r.12,r.12,h0_620_sse+h0_620_bpm+h0_620_ifm
        ori     r.12,r.12,h0_620_icia+h0_620_dcia
        or      r.12,r.12,r.6           // or in cache modes

#else
        ori     r.12,r.12,h0_620_prefered// enable all the things we want
        ori     r.12,r.12,h0_620_icia+h0_620_dcia // and invalidate both
                                        // caches.  620 clears ICIA and DCIA
                                        // automatically.
#endif
        sync
        isync
        mtspr   hid0,r.12
        sync
        isync
        b       go_virtual              // fall thru to switch modes

//
// Ensure the interrupt/exception vectors are not stale in the I-Cache
// by invalidating all cache lines in the region that was copied to low
// memory.
//
// Note that as soon as we are able to do so, we should use the hal to
// do a full, machine dependent, I-Cache invalidate.
//

caches_unknown:

        li      r.11, 0                 // target address
        li      r.10, (end_of_code_to_move-real0+31)/32 // num blocks to inval
        mtctr   r.10

invalidate_icache:
        icbi    0, r.11
        addi    r.11, r.11, 32
        bdnz    invalidate_icache

//
// Now invalidate all code in *this* module, some of which may have
// been modified during initialization.
//

        bl      invalidate_real0
invalidate_real0:
        mflr    r.10                    // address of invalidate_real0 to r.10
        li      r.12, (invalidate_real0-end_of_code_to_move)/32
        mtctr   r.12                    // number if blocks to invalidate
        subi    r.10, r.10, invalidate_real0-end_of_code_to_move

invalidate_real0_loop:
        icbi    0, 10
        addi    r.10, r.10, 32
        bdnz    invalidate_real0_loop

go_virtual:

#if !defined(NT_UP)
        li      r.10,0                  // get a zero value
        sync                            // ensure all previous stores done
        stw     r.10,HPT_LOCK(0)        // store zero in hpt lock word
#endif // NT_UP

//
// Done.  Now get system into virtual mode.  (return address is in r.0)
//
// N.B.  rfi does not load the MSR ILE bit from SRR1, so we need to turn
//       ILE on explicitly before the rfi.
//

        mfmsr   r.10                    // get current MSR
        rlwimi  r.10, r.10, 0, MASK_SPR(MSR_ILE,1) // turn on ILE
        mtmsr   r.10                    // set new MSR

        LWI(r.10, FLIH_MSR)             // initialize machine state
        oris    r.0, r.0, K_BASE        // set top bit of address
        mtsrr0  r.0                     // set rfi target address
        mtsrr1  r.10                    // set rfi target machine state
        rfi                             // return


//--------------------------------------------------------------------------
//
//  ki.real     switch kernel from mapped instructions and data to
//              unmapped.
//
//              On exit, MSR_IR and MSR_DR will be clear and the return
//              address adjusted such that execution continues at the
//              real address equivalent to the address in lr on entry.
//              Interrupts are disabled by clearing bit MSR_EE.
//
//              Note: this depends on the return address being in the
//              range covered by KSEG0.
//
//              The change of state and transfer are accomplished atomically
//              by setting the target address and state for return from
//              interrupt then using rfi to put these changes into effect.
//
//  Entry Requirements:
//              Processor executing in supervisor state.
//
//  Returns to next instruction in caller.
//      MSR:    Instruction Relocate OFF
//              Data Relocate OFF
//              Interrupts Disabled
//      r.30    return address (in real mode)
//
//--------------------------------------------------------------------------

ki.real:

        mflr    r.30
        mfmsr   r.8                             // get current state
        rlwinm  r.8, r.8, 0, ~INT_ENA           // clear interrupt enable
        mtmsr   r.8                             // disable interrupts
        rlwinm  r.8, r.8, 0, ~(MASK_SPR(MSR_IR,1)|MASK_SPR(MSR_DR,1))
        mtsrr1  r.8                             // desired initial state
        rlwinm  r.30, r.30, 0, 0x7fffffff       // physical return addrress
        mtsrr0  r.30
        rfi                                     // return

//--------------------------------------------------------------------------
//
//  Address of toc and common_exception_entry, available to init code.
//

rm_toc_pointer:
        .long   .toc                    // address of kernel toc
addr_common_exception_entry:
        .long   common_exception_entry
addr_RtlpRestoreContextRfiJump:
        .long   ..RtlpRestoreContextRfiJump

//--------------------------------------------------------------------------
//
//  Remaining code in this module exists for the life of the system.
//
//--------------------------------------------------------------------------

        .new_section .text,"rcx6"       // force 64 byte alignment
                                        // for text in this module.
        .text

        .align  2
toc_pointer:
        .long   .toc                    // address of kernel toc

//--------------------------------------------------------------------------
//
//  common_exception_entry
//
//  This is the common entry point into kernel for most exceptions/
//  interrupts.  The processor is running with instruction and data
//  relocation enabled when control reaches here.
//
//  on Entry:
//      MSR:    External interrupts disabled
//              Instruction Relocate ON
//              Data Relocate ON
//      GP registers:
//        r.2:  Constant identifying the exception type
//        r.3:  Saved SRR0 (interrupt address)
//        r.4:  Saved SRR1 (MSR value)
//        r.5:  -available-
//        r.11: -available-
//      In the PCR:
//        PcGprSave[0]:  Saved r.2
//        PcGprSave[1]:  Saved r.3
//        PcGprSave[2]:  Saved r.4
//        PcGprSave[3]:  Saved r.5
//        PcGprSave[5]:  Saved r.11
//
//      All other registers still have their contents as of the time
//      of interrupt
//
// Our stack frame header must contain space for 16 words of arguments, the
// maximum that can be specified on a system call.  Stack frame header struct
// defines space for 8 such words.
//
// We'll build a structure on the stack like this:
//
//      low addr  |                    |
//                |                    |
//             /  |--------------------| <-r.1 at point we call
//            |   | Stack Frame Header |   KiDispatchException
//            |   | (back chain, misc. |
//            |   | stuff, 16 words of |
//            |   | parameter space)   |
//           /    |--------------------|
//                | Trap Frame         |
//    STACK_DELTA | (volatile state)   |
//                |                 <------ includes ExceptionRecord, imbedded within
//           \    |--------------------|
//            |   | Exception Frame    |
//            |   | (non-volatile      |
//            |   | state)             |
//            |   |                    |
//            |   |--------------------|
//            |   | Slack space,       |
//            |   |  skipped over to   |
//            |   |  avoid stepping on |
//            |   |  data used by leaf |
//            |   |  routines          |
//             \  |--------------------| <-r.1 at point of interrupt, if interrupted
//                |                    |   kernel code, or base of kernel stack if
//      high addr |                    |   interrupted user code

//
// This stack frame format is defined a KEXCEPTION_STACK_FRAME in ppc.h.
//

#define DeliverApcSaveTrap      0xc     // Save trap frame address in reserved
                                        // (offset 12) Stack Frame Header
                                        // location for possible call out

                .text                           // resume .text section

//
// An Exception Record is embedded within the Trap Frame
//
        .set    ER_BASE, TF_BASE + TrExceptionRecord
        .set    DR_BASE, PbProcessorState + PsSpecialRegisters

//--------------------------------------------------------------------------
//  The following is never executed, it is provided to allow virtual
//  unwind to restore register state prior to an exception occuring.
//  This is a common prologue for the various exception handlers.

        FN_TABLE(KiCommonExceptionEntry,0,3)

        DUMMY_ENTRY(KiCommonExceptionEntry)

        stwu    r.sp, -STACK_DELTA (r.sp)
        stw     r.0, TrGpr0 + TF_BASE (r.sp)
        mflr    r.0
        stw     r.0, TrLr + TF_BASE (r.sp)
        mflr    r.0
        stw     r.0, EfLr (r.sp)
        mfcr    r.0
        stw     r.0, EfCr (r.sp)

        stw     r.2, TrGpr2 + TF_BASE(r.sp)
        stw     r.3, TrGpr3 + TF_BASE(r.sp)
        stw     r.4, TrGpr4 + TF_BASE(r.sp)
        stw     r.5, TrGpr5 + TF_BASE(r.sp)
        stw     r.6, TrGpr6 + TF_BASE(r.sp)
        stw     r.7, TrGpr7 + TF_BASE(r.sp)
        stw     r.8, TrGpr8 + TF_BASE(r.sp)
        stw     r.9, TrGpr9 + TF_BASE(r.sp)
        stw     r.10, TrGpr10 + TF_BASE(r.sp)
        stw     r.11, TrGpr11 + TF_BASE(r.sp)
        stw     r.12, TrGpr12 + TF_BASE(r.sp)

        mfctr   r.6                             //   Fixed Point Exception
        mfxer   r.7                             //   registers

        stfd    f.0, TrFpr0 + TF_BASE(r.sp)     // save volatile FPRs
        lis     r.12, K_BASE                    // base addr of KSEG0
        stfd    f.1, TrFpr1 + TF_BASE(r.sp)
        lfd     f.1, FpZero-real0(r.12)         // get FP 0.0
        stfd    f.2, TrFpr2 + TF_BASE(r.sp)
        mffs    f.0                             // get Floating Point Status
                                                //  and Control Register (FPSCR)
        stfd    f.3, TrFpr3 + TF_BASE(r.sp)
        stfd    f.4, TrFpr4 + TF_BASE(r.sp)
        stfd    f.5, TrFpr5 + TF_BASE(r.sp)
        stfd    f.6, TrFpr6 + TF_BASE(r.sp)
        stfd    f.7, TrFpr7 + TF_BASE(r.sp)
        stfd    f.8, TrFpr8 + TF_BASE(r.sp)
        stfd    f.9, TrFpr9 + TF_BASE(r.sp)
        stfd    f.10, TrFpr10 + TF_BASE(r.sp)
        stfd    f.11, TrFpr11 + TF_BASE(r.sp)
        stfd    f.12, TrFpr12 + TF_BASE(r.sp)
        stfd    f.13, TrFpr13 + TF_BASE(r.sp)

        stw     r.6, TrCtr + TF_BASE(r.sp)      // save Count register
        stw     r.7, TrXer + TF_BASE(r.sp)      // save Fixed Point Exception rg
        stfd    f.0, TrFpscr + TF_BASE(r.sp)    // save FPSCR register.
        mtfsf   0xff, f.1                       // clear FPSCR

//      \PROLOGUE_END(KiCommonExceptionEntry)
        .set KiCommonExceptionEntry.body, $+1   // so the debugger can see
                                                // difference between this
                                                // and normal prologues.

        .align  6                               // ensure the following is
                                                // cache block aligned (for
                                                // performance) (cache line
                                                // for 601)
common_exception_entry:
        mfcr    r.5                             // save CR at time of interrupt
        stw     r.6, KiPcr+PCR_SAVE6(r.0)       // save r.6 in PCR

// Code here and below frequently needs to test whether the previous mode
// was "kernel" or "user".  We isolate the PR (problem state, i.e., user mode)
// bit from the previous MSR into Condition Reg bit 19 (in cr.4), where it will
// stay.  Subsequently we can just branch-if-true (for user mode) or
// branch-if-false (for kernel mode) on CR bit WAS_USER_MODE.

        .set    WAS_USER_MODE, 19               // CR bit number

        rlwinm  r.6, r.4, 32+MSR_PR-WAS_USER_MODE, MASK_SPR(WAS_USER_MODE,1)
        mtcrf   0b00001000, r.6                 // PR to cr.4 WAS_USER_MODE
        lwz     r.6, KiPcr+PcInitialStack(r.0)  // kernel stack addr for thread
        bt      WAS_USER_MODE, cee.20           // branch if was in user state

// Get stack lower bound.

        lwz     r.11, KiPcr+PcStackLimit(r.0)   // get current stack limit

// Processor was in supervisor state.  We'll add our stack frame to the stack
// whose address is still in r.1 from the point of interrupt.  First, make sure
// that the stack address is valid.

        cmplw   cr.1, r.sp, r.6                 // test for underflow, into cr.1

// Make sure that stack hasn't overflowed, underflowed, or become misaligned

        subi    r.6, r.sp, STACK_DELTA          // allocate stack frame; ptr into r.6

        cmplw   cr.2, r.6, r.11                 // test for overflow, into cr.2
        andi.   r.11, r.sp, 7                   // test r.sp for 8-byte align into cr.0
        bgt-    cr.1, cee.10                    // branch if stack has underflowed
        bne-    cr.0, cee.10                    // branch if stack is misaligned
        bge+    cr.2, cee.30                    // branch if stack has not overflowed

// stack overflow/underflow/misalign

cee.10:
//      Allow for the possibility that we're actually changing from
//      one thread's stack to another,... this is a two instruction
//      window during which interrupts are disabled but might be that
//      someone is single stepping thru that code ... The address of
//      the second instruction is 'global' for the benefit of this
//      test.  If that is what happened, we must actually execute
//      the second instruction so that the correct stack is in use
//      because we would fail the stack check if any other exception
//      occurs while we are in this state and also because the old
//      stack may not be mapped any longer.
//
//      That instruction is a
//
//      ori     r.sp, r.22, 0
//
//      (we can't check this because the instruction has been replaced
//      by a breakpoint).
//
//      We KNOW that r.22 contains what we should use as a stack pointer!!
//
//      Available registers
//      r.6  contains what we had hoped would be the new stack address,
//      r.11

        li      r.6, KepSwappingContextAddr-real0// dereference pointer to
        oris    r.6, r.6, K_BASE                 // word containing addr of
        lwz     r.6, 0(r.6)                      // KepSwappingContext
        cmplw   r.6, r.3
        bne     cee.15                          // jif that wasn't it.

//      Ok, that seems to be the problem, do the stack switch and try
//      to validate again.  (can't branch up as if we fail, we'll be
//      in a loop).

        lwz     r.11, KiPcr+PcStackLimit(r.0)   // get current stack limit
        lwz     r.6, KiPcr+PcInitialStack(r.0)  // kernel stack addr for thread
        cmplw   cr.1, r.22, r.6                 // test for underflow, into cr.1
        subi    r.6, r.22, STACK_DELTA          // allocate stack frame; ptr into r.6
        cmplw   cr.2, r.6, r.11                 // test for overflow, into cr.2
        andi.   r.11, r.22, 7                   // test r.22 for 8-byte align into cr.0
        bgt-    cr.1, cee.15                    // branch if stack has underflowed
        bne-    cr.0, cee.15                    // branch if stack is misaligned
        bge+    cr.2, cee.30                    // branch if stack has not overflowed

//
//      It really is a problem (underflow, overflow or misalignment).
//
cee.15:
        lwz     r.11, KiPcr+PcStackLimit(r.0)   // refetch old stack limit
        lwz     r.6, KiPcr+PcPanicStack(r.0)    // switch to panic stack
        lwz     r.2, KiPcr+PcInitialStack(r.0)  // refetch old initial stack
        stw     r.11, TF_BASE-8-STACK_DELTA_NEWSTK(r.6) // save old StackLimit as
                                                //  if it was 15th parameter
        subi    r.11, r.6, KERNEL_STACK_SIZE    // compute stack limit
        stw     r.6, KiPcr+PcInitialStack(r.0)  // so we don't repeat ourselves
                                                //  ie, avoid overflowing because
                                                //  we went to the panic stack
        stw     r.11, KiPcr+PcStackLimit(r.0)   // set stack limit
        subi    r.6, r.6, STACK_DELTA_NEWSTK    // allocate stack frame
        stw     r.2, TF_BASE-4(r.6)             // save old InitialStack as
                                                //  if it was 16th parameter
        li      r.2, CODE_PANIC                 // set exception cause to panic
        b       cee.30                          // process exception

// Previous state was user mode
//
// Segment registers 9, 10, 12 and 13 need to be setup for kernel mode.
// In user mode they are set to zero as no access is allowed to these
// segments, and there are no combinations of Ks Kp and PP that allow
// kernel both read-only and read/write pages that are user no-access.

cee.20:
        mfsr    r.6, 0                          // get PID from SR0
        ori     r.6, r.6, 12                    // T=0 Ks,Kp=0 VSID=pgdir,12
        mtsr    12, r.6
        li      r.6, 9                         // T=0 Ks,Kp=0 VSID=9
        mtsr    9, r.6
        li      r.6, 10                         // T=0 Ks,Kp=0 VSID=10
        mtsr    10, r.6
        li      r.6, 13                         // T=0 Ks,Kp=0 VSID=13
        mtsr    13, r.6
        isync                                   // context synchronize

        lbz     r.6,  KiPcr+PcDebugActive(r.0)
        cmpwi   cr.1, r.6,  0                   // Hardware debug register set?

        lwz     r.6, KiPcr+PcInitialStack(r.0)  // kernel stack addr for thread
        subi    r.6, r.6, STACK_DELTA_NEWSTK    // allocate stack frame

// Stack address in r.6
        beq+    cr.1, cee.30                    // jif no debug registers set

// Yuck! There aren't any registers but this is the best place.
//       Save r.3 and reload it after debug register processing.
        stw     r.3, TrIar + TF_BASE (r.6)      // we need some registers
        stw     r.4, TrMsr + TF_BASE (r.6)      // save SRR1  (MSR)
        stw     r.5, TrCr + TF_BASE (r.6)       // save Condition Register
        stw     r.7, TrGpr7 + TF_BASE (r.6)
        stw     r.8, TrGpr8 + TF_BASE (r.6)
        li      r.3, 0                          // Initialize Dr7
        lwz     r.5, KiPcr+PcPrcb(r.0)          // get processor block address
        lwz     r.4, DR_BASE + SrKernelDr7(r.5) // Kernel DR set?
        rlwinm  r.4, r.4, 0, 0xFF
        cmpwi   cr.7, r.4, 0
        stw     r.3, TrDr7 + TF_BASE(r.6)       // No DRs set
        lwz     r.7, DR_BASE + SrKernelDr0(r.5) // Get kernel IABR
        lwz     r.8, DR_BASE + SrKernelDr1(r.5) // Get kernel DABR
        ori     r.7, r.7, 0x3                   // Sanitize IABR (Dr0)
        ori     r.8, r.8, 0x4                   // Sanitize DABR (Dr1)

//
// WARNING: Don't rearrange this branch table. The first branch is overlayed
// with the correct branch instruction (modified) based on the processor
// during system initialization. The correct order is 601, 603, 604, skip.
//
BranchDr1:
        b       cee.21                          // 601
        b       cee.23                          // 603
        b       cee.24                          // 604/613 - common path with 601
	b	cee.220				// 620
        b       cee.27                          // unknown

cee.21:                                         // 601 SPECIFIC
        li      r.3, 0x0080                     // Normal run mode
        rlwinm  r.7, r.7, 0, 0xfffffffc         // Sanitize IABR (Dr0)
        rlwinm  r.8, r.8, 0, 0xfffffff8         // Sanitize DABR (Dr1)
        bne     cr.7, cee.24                    // Leave hid1 set for full cmp
        mtspr   hid1, r.3

cee.24:
        mfspr   r.3, iabr                       // Load the IABR (Dr0)
        rlwinm. r.3, r.3, 0, 0xfffffffc         // IABR(DR0) set?
        li      r.4, 0                          // Initialize Dr7
        stw     r.3, TrDr0 + TF_BASE(r.6)
        mfspr   r.3, dabr                       // Load the DABR (Dr1)
        beq     noiabr.1                        // jiff Dr0 not set
        li      r.4, 0x1                        // Set LE0 in Dr7

noiabr.1:
        rlwimi  r.4, r.3, 19, 11, 11            // Interchange R/W1 bits
        rlwimi  r.4, r.3, 21, 10, 10            // and move to Dr7
        rlwinm. r.3, r.3, 0, 0xfffffff8         // Sanitize Dr1
        stw     r.3, TrDr1 + TF_BASE(r.6)       // Store Dr1 in trap frame
        beq     nodabr.1                        // jiff Dr1 not set
        ori     r.4, r.4, 0x4                   // Set LE1 in Dr7

nodabr.1:
        ori     r.4, r.4, 0x100                 // Set LE bit in Dr7
        stw     r.4, TrDr7 + TF_BASE(r.6)
        li      r.4, 0
        beq     cr.7, nokdr.1
        lwz     r.3, DR_BASE + SrKernelDr7(r.5)
        rlwinm. r.4, r.3, 0, 0x0000000c         // LE1/GE1 set?
        beq     nodr1.1                         // jiff Dr1 not set
        rlwimi  r.8, r.3, 13, 30, 30            // Interchange R/W1 bits
        rlwimi  r.8, r.3, 11, 31, 31
        mtspr   dabr, r.8

nodr1.1:
        rlwinm. r.3, r.3, 0, 0x00000003         // LE0/GE0 set?
        beq     cee.27
        mtspr   iabr, r.7
        isync
        b       cee.27

cee.23:                                         // 603 SPECIFIC
        mfspr   r.3, iabr                       // Load the IABR (Dr0)
        rlwinm. r.3, r.3, 0, 0xfffffffc         // Sanitize Dr0
        li      r.4, 0x101                      // Initialize Dr7
        stw     r.3, TrDr0 + TF_BASE(r.6)
        stw     r.4, TrDr7 + TF_BASE(r.6)
        li      r.4, 0
        beq     cr.7, nokdr.2                   // jif no kernel DR set
        rlwinm  r.7, r.7, 0, 0xfffffffc         // Sanitize IABR
        ori     r.7, r.7, 0x2
        mtspr   iabr, r.7
        b       cee.27

cee.220:					// 620 SPECIFIC
	mfspr	r.3, dabr			// Load the DABR (Dr1)
	b	noiabr.1

nokdr.2:
        mtspr   iabr, r.4
        b       cee.27

nokdr.1:
        mtspr   dabr, r.4
        mtspr   iabr, r.4
        isync

cee.27:
        lwz     r.8, TrGpr8 + TF_BASE (r.6)     // Reload all the registers
        lwz     r.7, TrGpr7 + TF_BASE (r.6)     // the debug code clobbered
        lwz     r.5, TrCr + TF_BASE (r.6)
        lwz     r.4, TrMsr + TF_BASE (r.6)
        lwz     r.3, TrIar + TF_BASE (r.6)

cee.30:

// Save Trap Frame (volatile registers)

        stw     r.3, TrIar + TF_BASE (r.6)      // save SRR0  (IAR) in Trap Frame
        stw     r.3, ErExceptionAddress + ER_BASE (r.6) //   and in Exception Record
        stw     r.3, EfLr (r.6)                         //   and for unwind
        stw     r.4, TrMsr + TF_BASE (r.6)      // save SRR1  (MSR)
        stw     r.5, TrCr + TF_BASE (r.6)       // save Condition Register
        stw     r.5, EfCr (r.6)                 //   and for unwind

        stw     r.0, TrGpr0 + TF_BASE (r.6)     // save volatile GPRs, fetching some
        stw     r.sp, TrGpr1 + TF_BASE (r.6)    //   of them from their temporary save
        lwz     r.0, KiPcr+PCR_SAVE2 (r.0)      //   area in the PCR
        lwz     r.5, KiPcr+PCR_SAVE3 (r.0)
        stw     r.0, TrGpr2 + TF_BASE (r.6)
        stw     r.5, TrGpr3 + TF_BASE (r.6)
        lwz     r.0, KiPcr+PCR_SAVE4 (r.0)
        lwz     r.5, KiPcr+PCR_SAVE5 (r.0)
        stw     r.0, TrGpr4 + TF_BASE (r.6)
        stw     r.5, TrGpr5 + TF_BASE (r.6)
        li      r.5, 0                          // Init DR6 (DR status) to zero
        lwz     r.0, KiPcr+PCR_SAVE6 (r.0)
        stw     r.7, TrGpr7 + TF_BASE (r.6)
        lbz     r.7, KiPcr+PcCurrentIrql (r.0)
        stw     r.0, TrGpr6 + TF_BASE (r.6)
        lwz     r.0, KiPcr+PCR_SAVE11 (r.0)
        stw     r.8, TrGpr8 + TF_BASE (r.6)
        stw     r.9, TrGpr9 + TF_BASE (r.6)
        stw     r.10, TrGpr10 + TF_BASE (r.6)
        stw     r.0, TrGpr11 + TF_BASE (r.6)
        stw     r.12, TrGpr12 + TF_BASE (r.6)
        stb     r.7, TrOldIrql + TF_BASE (r.6)  // save current Irql in tf
        stw     r.5, TrDr6 + TF_BASE (r.6)

// We've pulled everything out of the PCR that needs saving.
// Set up r.1 as our stack pointer so that we can take another interrupt now if need be.

        stw     r.sp, CrBackChain (r.6)         // set chain to previous stack frame
        ori     r.sp, r.6, 0                    // move new stack frame pointer to r.sp

// Save rest of trap frame

        cmpwi   r2, CODE_DECREMENTER            // does this interrupt save volatile FPRs?

        mflr    r.5                             // get Link, Count, and
        mfctr   r.6                             //   Fixed Point Exception
        mfxer   r.7                             //   registers

        ble     skip_float                      // if le, don't save volatile FPRs
        stfd    f.0, TrFpr0 + TF_BASE (r.sp)    // save volatile FPRs
        lis     r.12, K_BASE                    // base address of KSEG0
        stfd    f.1, TrFpr1 + TF_BASE (r.sp)
        lfd     f.1, FpZero-real0(r.12)         // get FP 0.0
        stfd    f.2, TrFpr2 + TF_BASE (r.sp)
        stfd    f.3, TrFpr3 + TF_BASE (r.sp)
        stfd    f.4, TrFpr4 + TF_BASE (r.sp)
        stfd    f.5, TrFpr5 + TF_BASE (r.sp)
        mffs    f.0                             // get Floating Point Status
                                                //   and Control Register (FPSCR)
        stfd    f.6, TrFpr6 + TF_BASE (r.sp)
        stfd    f.7, TrFpr7 + TF_BASE (r.sp)
        stfd    f.8, TrFpr8 + TF_BASE (r.sp)
        stfd    f.9, TrFpr9 + TF_BASE (r.sp)
        stfd    f.10, TrFpr10 + TF_BASE (r.sp)
        stfd    f.11, TrFpr11 + TF_BASE (r.sp)
        stfd    f.12, TrFpr12 + TF_BASE (r.sp)
        stfd    f.13, TrFpr13 + TF_BASE (r.sp)
        stfd    f.0, TrFpscr + TF_BASE (r.sp)
        mtfsf   0xff, f.1

skip_float:

        stw     r.5, TrLr + TF_BASE (r.sp)      // save Link,
        stw     r.6, TrCtr + TF_BASE (r.sp)     //      Count,
        stw     r.7, TrXer + TF_BASE (r.sp)     //      Fixed Point Exception

// End of save Trap Frame

// Volatile registers (r.0 thru r.12) are now available for use.
//      r.2     holds code number identifying interrupt
//      r.3     holds interrupt SRR0
//      r.4     holds interrupt SRR1

//-----------------------------------------------------------------
//
// Perform processing specific to the type of PowerPC interrupt,
// including deciding just what Windows NT exception code ("STATUS_...")
// value should be associated with this interrupt/exception.
//
// The internal code number in r.2 is an index into the following branch table.
//

        bl      next
next:
        mflr    r.12
        la      r.12, branch_table - next (r.12)

//      leave r.12 pointing to branch_table; used as base for data addressing
//      in code below

        add     r.2, r.12, r.2
        mtlr    r.2
        lwz     r.2, toc_pointer - branch_table (r.12) // load the kernel's TOC address
        blr     // thru branch table

// Note that no "validation" of the code is done; this table must match
// the set of CODE_... values defined above, or all bets are off.

branch_table:
        b       process_machine_check           // 0
        b       process_external                // 4
        b       process_decrementer             // 8
        b       process_storage_error           // 12
        b       process_page_fault              // 16
        b       process_alignment               // 20
        b       process_program                 // 24
        b       process_fp_unavail              // 28
        b       process_direct_store            // 32
        b       process_system_call             // 36
        b       process_trace                   // 40
        b       process_fp_assist               // 44
        b       process_run_mode                // 48
        b       process_panic                   // 52
// Added for PPC603:
        b       process_system_management       // 56
// 601, 604 data address breakpoint
        b       process_data_breakpoint         // 60
// 604 Performance Monitor Interupt
        b       process_pmi                     // 64

        DUMMY_EXIT(KiCommonExceptionEntry)

// Register contents at this point are:
//      r.0          -scratch-
//      r.1 (r.sp)   Our stack pointer
//      r.2 (r.toc)  Kernel's TOC pointer
//      r.3          SRR0 (interrupt address)
//      r.4          SRR1 (MSR at time of interrup)
//      r.5--r.11    -scratch-
//      r.12         Address of branch_table, above
//      r.13--r.31   Non-volatile state, STILL UNSAVED

// This follows standard linkage conventions, with SRR0 and SRR1 as the
// two parameters of a call.

//-----------------------------------------------------------------
//
// Storage Error and Page Fault after DS and IS interrupts --
//
//   The code immediately below is never executed.  It is present to
//   allow for unwinding the stack through process_storage_error or
//   process_page_fault, for exception handling.

        FN_TABLE(KiStorageFaultDispatch,0,0)

        DUMMY_ENTRY (KiStorageFaultDispatch)

        b       common_exception_entry  // use common prologue for unwind

        PROLOGUE_END (KiStorageFaultDispatch)

process_storage_error:
        lwz     r.4,KiPcr+PcBadVaddr(r.0) // get failing addr, st/l bit
        rlwinm  r.3,r.4,0,0x00000001    // isolate st/l indicator
        stw     r.3,ErExceptionInformation+ER_BASE(r.1) // st/l to er
        stw     r.4,ErExceptionInformation+4+ER_BASE(r.1) // fail to er
        LWI    (r.3,STATUS_ACCESS_VIOLATION) // access violation status
        li      r.4,2                   // there are 2 er parms
        b       seter                   // goto setup rest of er

process_page_fault:
        mfmsr   r.0
        rlwinm  r.5,r.4,18,0x00000001   // 3rd arg - processor mode
        lwz     r.4,KiPcr+PcBadVaddr(r.0) // 2nd arg - failing addr
        rlwinm  r.3,r.4,0,0x00000001    // 1st arg - st/l indicator
        stw     r.3,ErExceptionInformation+ER_BASE(r.1) // st/l to er
        stw     r.4,ErExceptionInformation+4+ER_BASE(r.1) // fail to er
        ori     r.0,r.0,INT_ENA
        mtmsr   r.0                     // enable interrupts
	cror	0,0,0			// N.B. 603e/ev Errata 15
        bl      ..MmAccessFault         // call mem mgmt fault subr

//
// Check if working set watch is enabled.
//

        lwz     r.5,[toc]PsWatchEnabled(r.2) // get &working set watch enable
        cmpwi   r.3,STATUS_SUCCESS      // mem mgmt handled fault?
        lbz     r.5,0(r5)               // get working set watch enable flag
        blt     xcptn                   // branch if fault not handled
        cmpwi   r.5,0                   // watch enabled?
        lwz     r.5,ErExceptionInformation+4+ER_BASE(r.1)// set bad addr
        beq     owdbkp                  // jif watch disabled

        lwz     r.4,TrIar+TF_BASE(r.1)  // set failing PC
        bl      ..PsWatchWorkingSet     // record working set information

//
// Check if the debugger has any owed breakpoints.
//

owdbkp:
        lwz     r.4,[toc]KdpOweBreakpoint(r.2)
        lbz     r.4,0(r.4)              // get owed breakpoint flag
        cmpwi   r.4,0
        beq     KiAlternateExit         // jif no owed breakpoints

        bl      ..KdSetOwedBreakpoints  // call insrt breakpts subr
        b       KiAlternateExit         // goto resume thread

xcptn:  LWI    (r.0,(STATUS_IN_PAGE_ERROR|0x10000000)) // was code for
        cmplw   r.3,r.0                 //   irql too high returned?
        beq     irqlhi                  // branch if yes
        li      r.4,2                   // assume 2 er parms
        LWI    (r.0,STATUS_ACCESS_VIOLATION) // was it
        cmplw   r.3,r.0                 //   access violation?
        beq     seter                   // branch if yes
        LWI    (r.0,STATUS_GUARD_PAGE_VIOLATION) // was it
        cmplw   r.3,r.0                 //   guard page violation?
        beq     seter                   // branch if yes
        LWI    (r.0,STATUS_STACK_OVERFLOW) // was it
        cmplw   r.3,r.0                 //   stack overflow?
        beq     seter                   // branch if yes
        stw     r.3,ErExceptionInformation+8+ER_BASE(r.1) // stat to er
        LWI    (r.3,STATUS_IN_PAGE_ERROR) // use in page error status
        li      r.4,3                   // now there are 3 er parms

seter:  stw     r.3,ErExceptionCode+ER_BASE(r.1) // set er xcptn code
        stw     r.4,ErNumberParameters+ER_BASE(r.1) // set er num parms
        li      r.0,0                   // zero
        stw     r.0,ErExceptionFlags+ER_BASE(r.1) //   er flags
        stw     r.0,ErExceptionRecord+ER_BASE(r.1) //   er record ptr
        b       exception_dispatch      // goto dispatch exception

irqlhi: li      r.3,IRQL_NOT_LESS_OR_EQUAL // get irql too high code
        lwz     r.4,ErExceptionInformation+4+ER_BASE(r.1) // fail addr
        lbz     r.5,KiPcr+PcCurrentIrql(r.0) // get current irql from pcr
        lwz     r.6,ErExceptionInformation+ER_BASE(r.1) // st/l indic
        lwz     r.7,ErExceptionAddress+ER_BASE(r.1) // get int addr
        bl      ..KeBugCheckEx         // call bug check subroutine
        b       $

        DUMMY_EXIT(KiStorageFaultDispatch)

//-----------------------------------------------------------------
//
// Alignment interrupt --
//      Must be for data.  It's not possible to cause the machine
//      to branch to an address that isn't a multiple of 4, hence
//      there is no "misaligned instruction" exception

//      This array of bits, indexed by the 7-bit "index" value from
//      the DSISR, indicates whether the offending instruction is
//      a load or a store.  "1" indicates store.  See alignem.c for
//      an indication of how the 7-bit index value maps to the set
//      of load/store instructions.
//
//  The code immediately below is never executed.  It is present to allow
//  for unwinding the stack through process_alignment, for exception
//  handling.

        FN_TABLE(KiAlignmentFaultDispatch,0,0)

        DUMMY_ENTRY (KiAlignmentFaultDispatch)

        b       common_exception_entry  // use common prologue for unwind

        PROLOGUE_END (KiAlignmentFaultDispatch)

        .align  2
al_table:
        .long   0x44624460, 0x40004000, 0x60440402, 0x44624460

process_alignment:
        lwz     r.4, KiPcr+PcSavedV1 (r.0)      // get DSISR (align status)
        lwz     r.0, KiPcr+PcSavedV0 (r.0)      // get DAR (offending address)

        rlwinm  r.5, r.4, 19, 0b1100            // isolate table word number
        la      r.6, al_table - branch_table (r.12) // load address of table
        lwzx    r.6, r.6, r.5                   // load word from table
        rlwinm  r.5, r.4, 22, 0x1f              // isolate bit number within word
        rlwnm   r.6, r.6, r.5, 0x1              // isolate load/store bit
        cmpwi   r.0, 0                          // test for user vs. system address

//      put load/store indicator, DAR, and DSISR in exception record

        stw     r.6, ErExceptionInformation + ER_BASE (r.sp)
        stw     r.0, ErExceptionInformation + 4 + ER_BASE (r.sp)
        stw     r.4, ErExceptionInformation + 8 + ER_BASE (r.sp)

        crand   0, cr.0 + LT, WAS_USER_MODE     // test for system addr AND user mode
        LWI    (r.0, STATUS_DATATYPE_MISALIGNMENT) // load most probable status value
        bf+     0, al_1                         // branch if not an access violation
        LWI    (r.0, STATUS_ACCESS_VIOLATION)   // load access viol. status value
al_1:
        stw     r.0, ErExceptionCode + ER_BASE (r.sp) // store status value in excep. record
        li      r.0, 0
        stw     r.0, ErExceptionFlags + ER_BASE (r.sp)
        stw     r.0, ErExceptionRecord + ER_BASE (r.sp)
        li      r.0, 3
        stw     r.0, ErNumberParameters + ER_BASE (r.sp)
        b       exception_dispatch

        DUMMY_EXIT (KiAlignmentFaultDispatch)

//-----------------------------------------------------------------
//
// Program interrupt --
//      SRR0 contains the failing instruction address
//      SRR1 contains bits indicating the reason for interrupt
//        Floating-point enabled exception                 (SRR1 bit 11)
//        Illegal instruction                              (SRR1 bit 12)
//        Privileged instruction executed in problem state (SRR1 bit 13)
//        Trap instruction                                 (SRR1 bit 14)
//      In addition, SRR1 bit 15 is set if this is an
//        IMPRECISE floating point interrupt
//
//      On entry to this code, the interrupt-time SRR0 value is in
//        r.3, and the SRR1 value is in r.4
//
//  The code immediately below is never executed.  It is present to allow
//  for unwinding the stack through process_program, for exception
//  handling.

        FN_TABLE(KiProgramFaultDispatch,0,0)

        DUMMY_ENTRY (KiProgramFaultDispatch)

        b       common_exception_entry  // use common prologue for unwind

        PROLOGUE_END (KiProgramFaultDispatch)

//      This code is rather poorly scheduled, but that doesn't matter.
//      None of this is performance-critical.

        .set    PR_FPE, 0       // locations of the indicator bits (above)
        .set    PR_ILL, 1       //   after moving them to CR field 0
        .set    PR_PRIV, 2
        .set    PR_TRAP, 3

        .set    TO_BKP,   0b11111               // trap BREAKPOINT
        .set    TO_DIV0,  0b00110               // trap Integer DIV by zero
        .set    TO_DIV0U, 0b00111               // trap unconditional DIV by 0

process_program:
        rlwinm  r.0, r.4, 11, 0xF0000000        // move the 4 bits to high end
        mtcrf   0x80, r.0                       //   insert them into bits 0..3 of CR
        bt      PR_ILL, pr_2                    // branch if illegal instruction

        li      r.0, 0                          // fill in common info in
        stw     r.0, ErExceptionFlags + ER_BASE (r.sp) //   exception record
        stw     r.0, ErExceptionRecord + ER_BASE (r.sp)
        lwz     r.4, 0 (r.3)                    // pick up the instruction itself
        li      r.0, 1
        stw     r.0, ErNumberParameters + ER_BASE (r.sp) // show 1 parameter
        stw     r.4, ErExceptionInformation + ER_BASE (r.sp) // save instr as 1st "info" word

        bt      PR_FPE, pr_1                    // branch if float exception
        bt      PR_PRIV, pr_3                   // branch if privileged instruction
                                                // fall thru if trap instruction

// Trap instruction.
//      If the instruction has 0b11111 as the trap condition field,
//      then it's a "breakpoint".  Otherwise it's an array bounds
//      violation.
//      The instruction itself is in r.4 at this point.

        rlwinm  r.5, r.4, 11, 0b11111           // isolate the "TO" field
        cmpwi   r.5, TO_BKP                     // breakpoint?
        LWI    (r.0, STATUS_BREAKPOINT)         // assume breakpoint status
        beq     pr_0                            // branch if correct

//
// Integer divide by zero is implemented as a trap.  The TO equal bit is set
// and the immediate field must be zero.  To differentiate from other possible
// uses of trap on zero, the logically less than bit is also set. (in a comp
// against zero this will NEVER cause the trap so is useful just as a flag).
// The compiler may also set the logically greater than bit if this in an
// unconditional divide by zero.  In the following check, "or" in the logically
// greater than bit then check that both the TO field is TO_DIV0U AND the
// immediate field is zero.
//
        ori     r.5, r.5, TO_DIV0^TO_DIV0U      // |= "logically greater than"
        rlwimi  r.5, r.4, 16, 0xffff0000        // |= immediate field ( << 16)
        cmpwi   r.5, TO_DIV0U
        beq     pr_0
        LWI    (r.0, STATUS_ARRAY_BOUNDS_EXCEEDED) // assume bounds check trap

pr_0:
        stw     r.0, ErExceptionCode + ER_BASE (r.sp) // store proper status in excep. record
        b       exception_dispatch

// Floating-point enabled exception.
//      Pass all thru under the code "FLOAT_STACK_CHECK" at this point;
//      subdivide them further later.

pr_1:
        LWI    (r.0, STATUS_FLOAT_STACK_CHECK)
        stw     r.0, ErExceptionCode + ER_BASE (r.sp)
        b       exception_dispatch

// Illegal instruction.

pr_2:

#if 0

//
// Save the contents of the HPT group for the failing address.
//

        mfsrin  r.5,r.3                 // get sreg of virtual addr arg
        stw     r.5, 28 + ErExceptionInformation + ER_BASE (r.sp)
        rlwinm  r.6,r.3,20,0x0000ffff   // align arg vpi with vsid
        xor     r.6,r.5,r.6             // hash - exclusive or vsid with vpi
        rlwimi  r.5,r.3,3,0x7e000000    // insert api into reg with vsid
        rlwinm  r.5,r.5,7,0xffffffbf    // align vsid,api as 1st word hpte
        stw     r.5, 32 + ErExceptionInformation + ER_BASE (r.sp)
        mfsdr1  r.7                     // get storage description reg
        rlwinm  r.8,r.7,10,0x0007fc00   // align hpt mask with upper hash
        ori     r.8,r.8,0x03ff          // append lower one bits to mask
        and     r.6,r.8,r.6             // take hash modulo hpt size
        rlwinm  r.6,r.6,6,0x01ffffc0    // align hash as hpt group offset
        rlwinm  r.7,r.7,0,0xffff0000    // get real addr of hash page table
        oris    r.7,r.7,K_BASE          // or with kernel virtual address
        or      r.6,r.7,r.6             // or with offset to get group addr
        stw     r.6, 36 + ErExceptionInformation + ER_BASE (r.sp)
        li      r7,0x37fc
        oris    r7,r7,K_BASE
        subi    r6,r6,4
        li      r8,16
        mfctr   r0
        mtctr   r8
loadloop1:
        lwzu    r8,4(r6)
        stwu    r8,4(r7)
        bdnz    loadloop1
        mtctr   r0
        subi    r6,r6,60

//
// Turn the data cache off.
//

        mfspr   r9, 1008
        ori     r7, r9, 0x4000
        mtspr   1008, r7
        sync

//
// Dump the HPT group again.
//

        li      r7,0x383c
        oris    r7,r7,K_BASE
        subi    r6,r6,4
        li      r8,16
        mfctr   r0
        mtctr   r8
loadloop2:
        lwzu    r8,4(r6)
        stwu    r8,4(r7)
        bdnz    loadloop2
        mtctr   r0
        subi    r6,r6,60

//
// Get the instruction word from memory.
//

        lwz     r4, 0(r3)
        stw     r.4, 4 + ErExceptionInformation + ER_BASE (r.sp)

//
// Dump the HPT group again.
//

        li      r7,0x387c
        oris    r7,r7,K_BASE
        subi    r6,r6,4
        li      r8,16
        mfctr   r0
        mtctr   r8
loadloop3:
        lwzu    r8,4(r6)
        stwu    r8,4(r7)
        bdnz    loadloop3
        mtctr   r0
        subi    r6,r6,60

//
// Turn the data cache on.
//

        mtspr   1008, r9
        sync

#endif

        li      r.0, 0                          // fill in common info in
        stw     r.0, ErExceptionFlags + ER_BASE (r.sp) //   exception record
        stw     r.0, ErExceptionRecord + ER_BASE (r.sp)
        lwz     r.4, 0 (r.3)                    // pick up the instruction itself
        li      r.0, 1
        stw     r.0, ErNumberParameters + ER_BASE (r.sp) // show 1 parameter
        stw     r.4, ErExceptionInformation + ER_BASE (r.sp) // save instr as 1st "info" word

        LWI    (r.0, STATUS_ILLEGAL_INSTRUCTION)
        stw     r.0, ErExceptionCode + ER_BASE (r.sp)

#if 0

//
// The following is normally left out,... but it can be useful when
// debugging coherency problems so I've left the code around for future
// use.  plj
//

//
// Dump the HPT group again.
//

        li      r7,0x38bc
        oris    r7,r7,K_BASE
        subi    r6,r6,4
        li      r8,16
        mfctr   r0
        mtctr   r8
loadloop4:
        lwzu    r8,4(r6)
        stwu    r8,4(r7)
        bdnz    loadloop4
        mtctr   r0
        subi    r6,r6,60

//
// Look for a matching entry (valid or invalid) in the HPT for the instruction address.
//

nexthpte:
        lwz     r.7,4(r.6)              // get 1st(be) word of hpte
        lwz     r.8,0(r.6)              // get 2nd(be) word of hpte
        clrlwi  r.11,r.7,1              // mask off valid bit
        cmplw   r.5,r.11                // does hpte match search arg?
        beq     found                   // break if eq
        addi    r.6,r.6,8               // increment to next hpte
        andi.   r.7,r.6,0x003f          // tested all hptes in prim group?
        bne     nexthpte                // loop if not
        li      r.6,0                   // no match found
        li      r.7,0
        li      r.8,0
found:
        stw     r.6, 16 + ErExceptionInformation + ER_BASE (r.sp)
        stw     r.7, 20 + ErExceptionInformation + ER_BASE (r.sp)
        stw     r.8, 24 + ErExceptionInformation + ER_BASE (r.sp)

//
// Go get the instruction from memory (the load we just did got it from
// the data cache).  Try to do this by "invalidating" the data cache block
// containing the instruction.
//

        dcbf    0, r.3                          // invalidate d cache block
        sync
        lwz     r.4, 0(r.3)                     // refetch the instruction
        stw     r.4, 8 + ErExceptionInformation + ER_BASE (r.sp)

//
// Now be even harsher about getting the instruction from memory.
//

        dcbf    0, r.3                          // invalidate d cache block
        sync
        tlbie   r.3                             // invalidate TLB entry
        sync
        lwz     r.4, 0(r.3)                     // refetch the instruction
        li      r.0, 10                         // bump parameter count
        stw     r.0, ErNumberParameters + ER_BASE (r.sp)
        stw     r.4, 12 + ErExceptionInformation + ER_BASE (r.sp)

//
// Write to the FirePower scratch pad register, in case they're watching
// with a logic analyzer.
//

        LWI     (r.4,0xb0900024)
        stw     r.3, 0(r.4)
        sync

        //twi     31,0,0x16

#endif

        b       exception_dispatch

// Privileged instruction executed in user mode (problem state).

pr_3:
        LWI    (r.0, STATUS_PRIVILEGED_INSTRUCTION)
        stw     r.0, ErExceptionCode + ER_BASE (r.sp)
        b       exception_dispatch

        DUMMY_EXIT (KiProgramFaultDispatch)

        FN_TABLE(KiDataAddressBreakpointDispatch,0,0)

        DUMMY_ENTRY(KiDataAddressBreakpointDispatch)

        b       common_exception_entry  // use common prologue for unwind

        PROLOGUE_END(KiDataAddressBreakpointDispatch)
process_data_breakpoint:
        li      r.4, 2                          // Dr1 Breakpoint register
        LWI    (r.0, STATUS_SINGLE_STEP)        // assume breakpoint status
        stw     r.0, ErExceptionCode + ER_BASE (r.sp)
        stw     r.4, TrDr6 + TF_BASE(r.sp) // Save data breakpoint address
        li      r.0, 1
        stw     r.0, ErNumberParameters + ER_BASE (r.sp) // show 1 parameter
        li      r.0, 0
        stw     r.0, ErExceptionInformation + ER_BASE (r.sp) // parm = 0
        b       exception_dispatch

        DUMMY_EXIT(KiDataAddressBreakpointDispatch)

//-----------------------------------------------------------------
//
// Floating Point Unavailable interrupt --
//
//  The code immediately below is never executed.  It is present to allow
//  for unwinding the stack through process_fp_unavail, for exception
//  handling.

        FN_TABLE(KiFloatingPointUnavailableDispatch,0,0)

        DUMMY_ENTRY (KiFloatingPointUnavailableDispatch)

        b       common_exception_entry  // use common prologue for unwind

        PROLOGUE_END (KiFloatingPointUnavailableDispatch)

process_fp_unavail:
        li      r.3, TRAP_CAUSE_UNKNOWN
        bl      ..KeBugCheck
        b       $

        DUMMY_EXIT (KiFloatingPointUnavailableDispatch)

//-----------------------------------------------------------------
//
// Machine Check, Decrementer Interrupt and External Interrupt are bundled
// into somewhat common code as all three are handled by the HAL.
//
//  The code immediately below is never executed.  It is present to allow
//  for unwinding the stack through process_machine_check, process_decrementer
//  and process_external for exception handling.

        .struct 0
        .space  StackFrameHeaderLength
IntTOC: .space  4       // saved TOC
IntOIS: .space  4       // saved On Interrupt Stack indicator
IntIRQL:.space  1       // saved IRQL
        .align  3       // 8 byte align
IntFrame:

        FN_TABLE (KiInterruptException,0,0)

        DUMMY_ENTRY (KiInterruptException)
        b       common_exception_entry  // use common prologue for unwind
        stwu    r.sp, -IntFrame(r.sp)

        PROLOGUE_END (KiInterruptException)

//-----------------------------------------------------------------
//
// Machine Check --
//      Machine check is treated just like an interrupt, we let
//      the HAL handle it.
//      Load offset to PCR->InterruptRoutine[MACHINE_CHECK_VECTOR]
//      and branch to KiInterruptException to handle dispatch.
//
process_machine_check:
        li      r.3, PcInterruptRoutine + IrMachineCheckVector
        b       KiInterruptException10

//-----------------------------------------------------------------
//
// Performance Monitor --
//	The 604 (and follow-ons) Performance Monitor interrupt is
//	handled like an external interrupt.  Some PMI agent registers
//	to handle the interrupt.  Load offset to PMI handler and
//	branch to KiInterruptException to handle.
//
// N.B.  Some versions of the 604 do not turn off ENINT in MMCR0 when
//       signaling the PM interrupt.  Therefore interrupts must not be
//       enabled before the spot in the (external) PM interrupt handler
//       where ENINT is turned off.  This implies that one must not set
//       breakpoints or make calls to DbgPrint anywhere along the path
//       from here to the PM interrupt handler.
//
process_pmi:
        li      r.3, PcInterruptRoutine + IrPmiVector
        b       KiInterruptException10


//-----------------------------------------------------------------
//
// Decrementer interrupt --
//      Load offset to PCR->InterruptRoutine[DECREMENT_VECTOR]
//      and branch to KiInterruptException to handle dispatch.
//
process_decrementer:
        li      r.3, PcInterruptRoutine + IrDecrementVector
        b       KiInterruptException10

//-----------------------------------------------------------------
//
// External (I/O) interrupt --
//      Load offset to PCR->InterruptRoutine[EXTERNAL_VECTOR]
//      and fall thru to KiInterruptException to handle dispatch.
//

process_external:
        li      r.3, PcInterruptRoutine + IrDeviceVector
//      b       KiInterruptException10

//-----------------------------------------------------------------
//
//  KiInterruptException
//
//      This code switches to the interrupt stack (if not already
//      on the interrupt stack) and dispatches the appropriate handler
//      for the interrupt.
//
//      On return from the handler, we switch back to the previous
//      stack and check for and run dpc interrupts if IRQL is below
//      DISPATCH_LEVEL.
//
//      On entry r.3  contains the offset into the PCR of the address
//                    of the handler.
//               r.sp current stack pointer
//               Interrupts are disabled.
//
//      Calls the handler with
//               r.3    Address of Interrupt Object
//               r.4    Address of Service Context
//               r.5    Address of Trap Frame
//
//      Exits to KiAlternateExit.

KiInterruptException10:
        lwz     r.7, KiPcr(r.3)                 // get address of fn descr
        lwz     r.8, KiPcr+PcPrcb(r.0)          // get processor block address
        lwz     r.12,KiPcr+PcOnInterruptStack(r.0) // get stack indicator
        stw     r.sp,KiPcr+PcOnInterruptStack(r.0) // set new stack indicator
        addi    r.5, r.sp, TF_BASE              // set 3rd parm = &Trap Frame
        lwz     r.6, 0(r.7)                     // get addr of entry point
        lwz     r.9, PbInterruptCount(r.8)      // get current interrupt count
        cmpwi   r.12,0                          // check already on intrpt stk
        subi    r.3, r.7, InDispatchCode        // compute addr of Intr Object
        lwz     r.4, InServiceContext(r.3)      // get addr of Service Context
        mtlr    r.6                             // service proc entry point.
        li      r.6, -IntFrame                  // size of stack frame
        bne     kie20                           // jif already on interrupt stk
        lwz     r.10,KiPcr+PcInitialStack(r.0)  // get old initial stack addr
        lwz     r.11,KiPcr+PcStackLimit(r.0)    // get old stack limit
        lbz     r.0, KiPcr+PcCurrentIrql(r.0)   // get current IRQL
        lwz     r.6, KiPcr+PcInterruptStack(r.0) // get interrupt stack
        stw     r.10,KiPcr+PcSavedInitialStack(r.0) // save old initial stack addr
        stw     r.11,KiPcr+PcSavedStackLimit(r.0) // save old stack limit
        subi    r.11, r.6, KERNEL_STACK_SIZE    // compute new stack limit
        cmpwi   r.0, DISPATCH_LEVEL             // IRQL >= DISPATCH_LEVEL ?
        stw     r.6, KiPcr+PcInitialStack(r.0)  // set new initial stack
        stw     r.11, KiPcr+PcStackLimit(r.0)   // set new stack limit
        subi    r.6, r.6, IntFrame
        stb     r.0, IntIRQL(r.6)               // save old IRQL (on new stack)
        sub     r.6, r.6, r.sp                  // diff needed for new sp
        bge     kie20                           // jif IRQL >= DISPATCH_LEVEL

//
// IRQL is below DISPATCH_LEVEL, raise to DISPATCH_LEVEL to avoid context
// switch while on the interrupt stack.
//

        li      r.0, DISPATCH_LEVEL             // raise IRQL
        stb     r.0, KiPcr+PcCurrentIrql(r.0)
kie20:  stwux   r.sp, r.sp, r.6                 // buy stack frame
        stw     r.toc, IntTOC(r.sp)             // save our toc
        stw     r.12,  IntOIS(r.sp)             // save On Int Stk indicator
        lwz     r.toc, 4(r.7)                   // get callee's toc
        addi    r.9, r.9, 1                     // increment interrupt count
        stw     r.9, PbInterruptCount(r.8)
        blrl                                    // call service proc

//
// Back from the service proc.  If we switched stacks on the way in we
// need to switch back.
//

        mfmsr   r.7
        lwz     r.9, IntOIS(r.sp)               // get saved stack indicator
        lwz     r.toc, IntTOC(r.sp)             // restore our toc
        cmpwi   r.9, 0                          // if eq, must switch stacks
        rlwinm  r.7,r.7,0,~INT_ENA
        mtmsr   r.7                             // disable interrupts
	cror	0,0,0				// N.B. 603e/ev Errata 15
        lbz     r.6, IntIRQL(r.sp)              // get previous IRQL
        stw     r.9, KiPcr+PcOnInterruptStack(r.0) // restore stack indicator
        lwz     r.sp, 0(r.sp)                   // switch stacks back
        la      r.3, TF_BASE(r.sp)              // compute trap frame address
        bne     KiPriorityExit                  // jif staying on interrupt stk

        lwz     r.8, KiPcr+PcSavedInitialStack(r.0) // get old initial stack
        lwz     r.9, KiPcr+PcSavedStackLimit(r.0) // get old stack limit
        cmpwi   cr.3, r.6, APC_LEVEL            // check current IRQL
        stw     r.8, KiPcr+PcInitialStack(r.0)  // restore thread initial stack
        stw     r.9, KiPcr+PcStackLimit(r.0)  // restore thread stack limit

//
// If previous IRQL is below DISPATCH_LEVEL, restore current IRQL to its
// correct value, check for pending DPC interrupts and deliver them now.
//
// N.B. We used cr.3 for the comparison against APC_LEVEL.  This cr
// is non-volatile but we know we are going to exit thru a path that
// will restore it so we're bending the rules a little.  We use it
// so that it will be intact after calling KiDispatchSoftwareInterrupt.
//

        bgt     cr.3, KiPriorityExit            // exit interrupt state

        blt     cr.3, kie25                     // below APC_LEVEL, no matter
                                                // what, we need to go the
                                                // interrupt enable path

//
// We are at APC level, if no DPC pending, get out the fast way without
// enabling interrupts.
//

        lbz     r.5, KiPcr+PcDispatchInterrupt(r.0) // is there a DPC int pending?
        cmpwi   r.5, 0                          // s/w int pending?
        bne     kie25
        stb     r.6, KiPcr+PcCurrentIrql(r.0)   // set correct IRQL
        b       KiPriorityExit

//
// The only reason to enable interrupts before exiting interrupt state
// is to run pending DPCs and APCs.  In the following, we enable interrupts
// BEFORE resetting current IRQL to its correct value which is below
// DISPATCH_LEVEL.  This (and the following sync) should cause any pending
// interrupts to be taken immediately with IRQL set to DISPATCH_LEVEL.
// When exiting from the second (nested) interrupt we will not enable
// interrupts early (because previous IRQL is DISPATCH_LEVEL) so we
// will not run out of stack if we are being swamped with interrupts.
// This guarantees the maximum number of interrupt contexts on the
// kernel stack at any point is 2.  (The first which is below DISPATCH
// and the second which is taken at DISPATCH LEVEL).
//
// plj programming note: If the mtmsr/sync combo isn't enough to force
// pending interrupts at precisely the right time switch, to an rfi
// sequence.  Currently all PowerPC implementations will take a pending
// interrupt prior to execution of the first instruction pointed to
// by srr0 if interrupts are enabled as a result of the rfi.  (processors
// include 601, 601+, 603, 603+, 604, 604+ and 620).
//
// Solution to overflowing stack for devices that interrupt at a high rate.
// Close the window to dispatching software interrupts and don't enable
// until we determine there is a pending software interrupt. Return
// from dispatching software interrupts disabled until we determine that
// we are resuming to user mode and at an IRQL < APC_LEVEL. The only time
// this should occur is when we are no longer running a nested interrupt.


kie25:
        lhz     r.5, KiPcr+PcSoftwareInterrupt(r.0) // is there a s/w int pending?
        stb     r.6, KiPcr+PcCurrentIrql(r.0)   // restore previous IRQL
        cmpwi   cr.1, r.5, 0
        bne     cr.1, kie30                     // if ne, s/w int pending

        lwz     r.8, TF_BASE+TrMsr(r.sp)        // load saved MSR value
        extrwi. r.8, r.8, 1, MSR_PR             // see if resuming user mode
        beq     KiPriorityExit                  // jif kernel mode
        beq     cr.3, KiPriorityExit            // skip user mode APC check
                                                // if at APC level.
        lwz     r.6, KiPcr+PcCurrentThread(r.0) // get address of current thread
        stb     r.5, ThAlerted(r.6)             // clear kernel mode alerted
        lbz     r.6, ThApcState+AsUserApcPending(r.6)
        cmpwi   r.6, 0                          // user mode APC pending?
        beq     KiPriorityExit                  // if eq, none pending

kie30:

//
// Either a software interrupt is pending, or the current thread has a
// user mode APC pending.  We need to save the volatile floating point
// state before we can proceed.
//

        stfd    f.0, TrFpr0 + TF_BASE(r.sp)     // save volatile FPRs
        stfd    f.1, TrFpr1 + TF_BASE(r.sp)
        stfd    f.2, TrFpr2 + TF_BASE(r.sp)
        stfd    f.3, TrFpr3 + TF_BASE(r.sp)
        stfd    f.4, TrFpr4 + TF_BASE(r.sp)
        stfd    f.5, TrFpr5 + TF_BASE(r.sp)
        mffs    f.0                             // get Floating Point Status
                                                //   and Control Register (FPSCR)
        stfd    f.6, TrFpr6 + TF_BASE(r.sp)
        stfd    f.7, TrFpr7 + TF_BASE(r.sp)
        stfd    f.8, TrFpr8 + TF_BASE(r.sp)
        stfd    f.9, TrFpr9 + TF_BASE(r.sp)
        stfd    f.10, TrFpr10 + TF_BASE(r.sp)
        stfd    f.11, TrFpr11 + TF_BASE(r.sp)
        stfd    f.12, TrFpr12 + TF_BASE(r.sp)
        stfd    f.13, TrFpr13 + TF_BASE(r.sp)
        stfd    f.0, TrFpscr + TF_BASE(r.sp)

        beq     cr.1, kie40                     // if eq, no s/w int pending

//
// Software interrupt pending.  Dispatch it.
//

        stwu    r.sp, -IntFrame(r.sp)           // buy stack frame
        li      r.3, 0                          // Tell dispatch routines to
                                                //  not enable interrupts when
                                                //  returning to IRQL 0.
        ori     r.7,r.7,INT_ENA                 // Set up for interrupts to be enabled (r7 is
                                                //  input arg to KiDispatchSoftwareIntDisabled)
        bl      ..KiDispatchSoftwareIntDisabled // run pending DPCs and
                                                // if applicable APCs.
        addi    r.sp, r.sp, IntFrame            // return stack frame
        la      r.3, TF_BASE(r.sp)              // compute trap frame address


//
// Having dispatched the software interrupt, we now need to check whether
// the current thread has a user mode APC pending.
//

        lwz     r.8, TF_BASE+TrMsr(r.sp)        // load saved MSR value
        extrwi. r.8, r.8, 1, MSR_PR             // see if resuming user mode
        beq     ae.restore                      // jif kernel mode
        beq     cr.3, ae.restore                // skip user mode APC check
                                                // if at APC level.
        li      r.5, 0
        lwz     r.6, KiPcr+PcCurrentThread(r.0) // get address of current thread
        stb     r.5, ThAlerted(r.6)             // clear kernel mode alerted
        lbz     r.6, ThApcState+AsUserApcPending(r.6)
        cmpwi   r.6, 0                          // user mode APC pending?
        beq     ae.restore                      // if eq, none pending
kie40:
        mfmsr   r.7
        ori     r.7,r.7,INT_ENA
        mtmsr   r.7                             // enable interrupts
        sync                                    // flush pipeline

//
// A user mode APC is pending.  Branch to common code to deliver it.
//

        b       ae.apc_deliver                  // join common code

        DUMMY_EXIT (KiInterruptException)

//-----------------------------------------------------------------
//
// Direct-Store Error interrupt --
//      Treat this as a bus error.
//
//  The code immediately below is never executed.  It is present to allow
//  for unwinding the stack through process_direct_store, for exception
//  handling.

        FN_TABLE(KiDirectStoreFaultDispatch,0,0)

        DUMMY_ENTRY (KiDirectStoreFaultDispatch)

        b       common_exception_entry  // use common prologue for unwind

        PROLOGUE_END (KiDirectStoreFaultDispatch)

process_direct_store:
        li      r.3, DATA_BUS_ERROR     // should define a PPC specific
                                        // code, for now borrow from MIPS.
        bl      ..KeBugCheck
        b       $

        DUMMY_EXIT (KiDirectStoreFaultDispatch)

//-----------------------------------------------------------------
//
// Trace exception --
//
//  The code immediately below is never executed.  It is present to allow
//  for unwinding the stack through process_trace, for exception handling.

        FN_TABLE(KiTraceExceptionDispatch,0,0)

        DUMMY_ENTRY (KiTraceExceptionDispatch)

        b       common_exception_entry  // use common prologue for unwind

        PROLOGUE_END (KiTraceExceptionDispatch)

process_trace:
        li      r.3, TRAP_CAUSE_UNKNOWN // should define a PPC specific
                                        // code, for now borrow from MIPS.
        bl      ..KeBugCheck
        b       $

        DUMMY_EXIT (KiTraceExceptionDispatch)

//-----------------------------------------------------------------
//
// Floating-Point Assist interrupt --
//
//  The code immediately below is never executed.  It is present to allow
//  for unwinding the stack through process_fp_assist, for exception
//  handling.

        FN_TABLE(KiFpAssistExceptionDispatch,0,0)

        DUMMY_ENTRY (KiFpAssistExceptionDispatch)

        b       common_exception_entry  // use common prologue for unwind

        PROLOGUE_END (KiFpAssistExceptionDispatch)

process_fp_assist:
        li      r.3, TRAP_CAUSE_UNKNOWN // should define a PPC specific
                                        // code, for now borrow from MIPS.
        bl      ..KeBugCheck
        b       $

        DUMMY_EXIT (KiFpAssistExceptionDispatch)


//-----------------------------------------------------------------
//
// System Management interrupt --
//  This is the power management handler for the PowerPC 603.
//  THIS CODE WILL CHANGE.
//
//  The code immediately below is never executed.  It is present to allow
//  for unwinding the stack through process_system_management, for exception
//  handling.

        FN_TABLE(KiSystemManagementExceptionDispatch,0,0)

        DUMMY_ENTRY (KiSystemManagementExceptionDispatch)

        b       common_exception_entry  // use common prologue for unwind

        PROLOGUE_END (KiSystemManagementExceptionDispatch)

process_system_management:
        li      r.3, TRAP_CAUSE_UNKNOWN // will need PowerMgmt specific code
        bl      ..KeBugCheck
        b       $

        DUMMY_EXIT (KiSystemManagementExceptionDispatch)



//-----------------------------------------------------------------
//
// Run-Mode interrupt --
//
//  The code immediately below is never executed.  It is present to allow
//  for unwinding the stack through process_run_mode, for exception
//  handling.

        FN_TABLE(KiRunModeExceptionDispatch,0,0)

        DUMMY_ENTRY (KiRunModeExceptionDispatch)

        b       common_exception_entry  // use common prologue for unwind

        PROLOGUE_END (KiRunModeExceptionDispatch)

process_run_mode:
        LWI    (r.0, STATUS_SINGLE_STEP)   // assume breakpoint status
        stw     r.0, ErExceptionCode + ER_BASE (r.sp)
        li      r.0, 1                     // Dr0 breakpoint register
        stw     r.0, TrDr6 + TF_BASE(r.sp) // Save instr. breakpoint address
        stw     r.0, ErNumberParameters + ER_BASE (r.sp) // show 1 parameter
        li      r.0, 0
        stw     r.0, ErExceptionInformation + ER_BASE (r.sp) // parm = 0
        b       exception_dispatch

        DUMMY_EXIT (KiRunModeExceptionDispatch)

//-----------------------------------------------------------------
//
// We've panic'd, call KeBugCheck
//-----------------------------------------------------------------
//
//  The code immediately below is never executed.  It is present to allow
//  for unwinding the stack through process_panic, for exception
//  handling.

        FN_TABLE(KiStackOvflDispatch,0,0)

        DUMMY_ENTRY (KiStackOvflDispatch)

        b       common_exception_entry  // use common prologue for unwind

        PROLOGUE_END (KiStackOvflDispatch)

process_panic:
        li      r.3, PANIC_STACK_SWITCH
        bl      ..KeBugCheck
        b       $

        DUMMY_EXIT (KiStackOvflDispatch)

//-----------------------------------------------------------------
//
// System Call entry via common_exception_entry ???  Can't happen.
//
//  The code immediately below is never executed.  It is present to allow
//  for unwinding the stack through process_run_mode, for exception
//  handling.

        FN_TABLE(KiSystemCallEntryError,0,0)

        DUMMY_ENTRY (KiSystemCallEntryError)

        b       common_exception_entry  // use common prologue for unwind

        PROLOGUE_END (KiSystemCallEntryError)

process_system_call:
        li      r.3, TRAP_CAUSE_UNKNOWN // should define a PPC specific
                                        // code, for now borrow from MIPS.
        bl      ..KeBugCheck
        b       $

        DUMMY_EXIT (KiSystemCallEntryError)

//-----------------------------------------------------------------
//
//  We are about to call KiDispatchException, as follows:
//
//      KiDispatchException (& Exception Record,               [r.3]
//                           & Exception Frame,                [r.4]
//                           & Trap Frame,                     [r.5]
//                           Previous Mode (0=kernel, 1=user), [r.6]
//                           First Chance flag (1));           [r.7]
//
//  The code immediately below is never executed.  It is present to allow
//  for unwinding the stack through exception_dispatch, for exception
//  handling.

        FN_TABLE(KiExceptionDispatch,0,0)

        DUMMY_ENTRY (KiExceptionDispatch)

        b       common_exception_entry  // use common prologue for unwind

        stw     r.13, ExGpr13 + EF_BASE(r.sp)  // save non-volatile GPRs
        stw     r.14, ExGpr14 + EF_BASE(r.sp)
        stw     r.15, ExGpr15 + EF_BASE(r.sp)
        stw     r.16, ExGpr16 + EF_BASE(r.sp)
        stw     r.17, ExGpr17 + EF_BASE(r.sp)
        stw     r.18, ExGpr18 + EF_BASE(r.sp)
        stw     r.19, ExGpr19 + EF_BASE(r.sp)
        stw     r.20, ExGpr20 + EF_BASE(r.sp)
        stw     r.21, ExGpr21 + EF_BASE(r.sp)
        stw     r.22, ExGpr22 + EF_BASE(r.sp)
        stw     r.23, ExGpr23 + EF_BASE(r.sp)
        stw     r.24, ExGpr24 + EF_BASE(r.sp)
        stw     r.25, ExGpr25 + EF_BASE(r.sp)
        stw     r.26, ExGpr26 + EF_BASE(r.sp)
        stw     r.27, ExGpr27 + EF_BASE(r.sp)
        stw     r.28, ExGpr28 + EF_BASE(r.sp)
        stw     r.29, ExGpr29 + EF_BASE(r.sp)
        stw     r.30, ExGpr30 + EF_BASE(r.sp)
        stw     r.31, ExGpr31 + EF_BASE(r.sp)

        stfd    f.14, ExFpr14 + EF_BASE(r.sp)  // save non-volatile FPRs
        stfd    f.15, ExFpr15 + EF_BASE(r.sp)
        stfd    f.16, ExFpr16 + EF_BASE(r.sp)
        stfd    f.17, ExFpr17 + EF_BASE(r.sp)
        stfd    f.18, ExFpr18 + EF_BASE(r.sp)
        stfd    f.19, ExFpr19 + EF_BASE(r.sp)
        stfd    f.20, ExFpr20 + EF_BASE(r.sp)
        stfd    f.21, ExFpr21 + EF_BASE(r.sp)
        stfd    f.22, ExFpr22 + EF_BASE(r.sp)
        stfd    f.23, ExFpr23 + EF_BASE(r.sp)
        stfd    f.24, ExFpr24 + EF_BASE(r.sp)
        stfd    f.25, ExFpr25 + EF_BASE(r.sp)
        stfd    f.26, ExFpr26 + EF_BASE(r.sp)
        stfd    f.27, ExFpr27 + EF_BASE(r.sp)
        stfd    f.28, ExFpr28 + EF_BASE(r.sp)
        stfd    f.29, ExFpr29 + EF_BASE(r.sp)
        stfd    f.30, ExFpr30 + EF_BASE(r.sp)
        stfd    f.31, ExFpr31 + EF_BASE(r.sp)

        PROLOGUE_END (KiExceptionDispatch)

exception_dispatch:
        mfmsr   r.0
        ori     r.0,r.0,INT_ENA
        mtmsr   r.0                             // enable interrupts
        sync


//  The first argument (r.3) to KiDispatchException is the address
//  of the Exception Record.

        la      r.3, ER_BASE (r.sp)

//  The second argument (r.4) is the address of the Exception Frame.

//-------------------------------------------------------------------
//
//  Generate Exception Frame (save the non-volatile state)

        la      r.4, EF_BASE (r.sp)             // address of Exception Frame

        stw     r.13, ExGpr13 (r.4)             // save non-volatile GPRs
        stw     r.14, ExGpr14 (r.4)
        stw     r.15, ExGpr15 (r.4)
        stw     r.16, ExGpr16 (r.4)
        stw     r.17, ExGpr17 (r.4)
        stw     r.18, ExGpr18 (r.4)
        stw     r.19, ExGpr19 (r.4)
        stw     r.20, ExGpr20 (r.4)
        stw     r.21, ExGpr21 (r.4)
        stw     r.22, ExGpr22 (r.4)
        stw     r.23, ExGpr23 (r.4)
        stw     r.24, ExGpr24 (r.4)
        stw     r.25, ExGpr25 (r.4)
        stw     r.26, ExGpr26 (r.4)
        stw     r.27, ExGpr27 (r.4)
        stw     r.28, ExGpr28 (r.4)
        stw     r.29, ExGpr29 (r.4)
        stw     r.30, ExGpr30 (r.4)
        stw     r.31, ExGpr31 (r.4)

        stfd    f.14, ExFpr14 (r.4)             // save non-volatile FPRs
        stfd    f.15, ExFpr15 (r.4)
        stfd    f.16, ExFpr16 (r.4)
        stfd    f.17, ExFpr17 (r.4)
        stfd    f.18, ExFpr18 (r.4)
        stfd    f.19, ExFpr19 (r.4)
        stfd    f.20, ExFpr20 (r.4)
        stfd    f.21, ExFpr21 (r.4)
        stfd    f.22, ExFpr22 (r.4)
        stfd    f.23, ExFpr23 (r.4)
        stfd    f.24, ExFpr24 (r.4)
        stfd    f.25, ExFpr25 (r.4)
        stfd    f.26, ExFpr26 (r.4)
        stfd    f.27, ExFpr27 (r.4)
        stfd    f.28, ExFpr28 (r.4)
        stfd    f.29, ExFpr29 (r.4)
        stfd    f.30, ExFpr30 (r.4)
        stfd    f.31, ExFpr31 (r.4)

//  End of Exception Frame
//
//-------------------------------------------------------------------

//  The third argument (r.5) to KiDispatch Exception is the address
//  of the Trap Frame.

        la      r.5, TF_BASE (r.sp)

//  The fourth argument (r.6) is the previous mode:  0 for kernel mode,
//  1 for user mode.  We have this value in bit WAS_USER_MODE of the CR.

        mfcr    r.6
        rlwinm  r.6, r.6, 32+WAS_USER_MODE-31, 1

//  The fifth argument (r.7) is the "first chance" flag.

        li      r.7, 1                          // First Chance = TRUE

//  Call KiDispatchException(
//      &ExceptionRecord,
//      &Exception Frame,
//      &Trap Frame,
//      Previous Mode,
//      First Chance = TRUE)
//
        bl      ..KiDispatchException

//  Load registers required by KiExceptionExit:
//      r.3 points to Exception Frame
//      r.4 points to Trap Frame

        la      r.3, EF_BASE (r.sp)
        la      r.4, TF_BASE (r.sp)

        b       ..KiExceptionExit

//  Fall thru ...

        DUMMY_EXIT (KiExceptionDispatch)

//--------------------------------------------------------------------------
//
//  KiCommonFakeMillicode() -- This code is never executed.  It is provided
//                      to allow virtual unwind to restore register state
//                      prior to an exception.
//
// This is fake register save millicode "called" during a fake prologue.
// The reverse execution of the fake prologue establishes r.12 to point
// to the Exception Frame.
//
//  Arguments:
//
//      r.12 -- address of Exception Frame to restore from.
//
//  Return values:
//
//      Only non-volatile registers are restored by the virtual unwinder.
//
//--------------------------------------------------------------------------

        FN_TABLE(KiCommonFakeMillicode, 0, 1)

        DUMMY_ENTRY(KiCommonFakeMillicode)

        PROLOGUE_END(KiCommonFakeMillicode)

        stfd     f.14, ExFpr14 (r.12)             // restore non-volatile FPRs
        stfd     f.15, ExFpr15 (r.12)
        stfd     f.16, ExFpr16 (r.12)
        stfd     f.17, ExFpr17 (r.12)
        stfd     f.18, ExFpr18 (r.12)
        stfd     f.19, ExFpr19 (r.12)
        stfd     f.20, ExFpr20 (r.12)
        stfd     f.21, ExFpr21 (r.12)
        stfd     f.22, ExFpr22 (r.12)
        stfd     f.23, ExFpr23 (r.12)
        stfd     f.24, ExFpr24 (r.12)
        stfd     f.25, ExFpr25 (r.12)
        stfd     f.26, ExFpr26 (r.12)
        stfd     f.27, ExFpr27 (r.12)
        stfd     f.28, ExFpr28 (r.12)
        stfd     f.29, ExFpr29 (r.12)
        stfd     f.30, ExFpr30 (r.12)
        stfd     f.31, ExFpr31 (r.12)

        stw     r.14, ExGpr14 (r.12)             // restore non-volatile GPRs
        stw     r.15, ExGpr15 (r.12)
        stw     r.16, ExGpr16 (r.12)
        stw     r.17, ExGpr17 (r.12)
        stw     r.18, ExGpr18 (r.12)
        stw     r.19, ExGpr19 (r.12)
        stw     r.20, ExGpr20 (r.12)
        stw     r.21, ExGpr21 (r.12)
        stw     r.22, ExGpr22 (r.12)
        stw     r.23, ExGpr23 (r.12)
        stw     r.24, ExGpr24 (r.12)
        stw     r.25, ExGpr25 (r.12)
        stw     r.26, ExGpr26 (r.12)
        stw     r.27, ExGpr27 (r.12)
        stw     r.28, ExGpr28 (r.12)
        stw     r.29, ExGpr29 (r.12)
        stw     r.30, ExGpr30 (r.12)
        stw     r.31, ExGpr31 (r.12)
        blr

        DUMMY_EXIT(KiCommonFakeMillicode)

//--------------------------------------------------------------------------
//
//  KiExceptionExit() -- Control is transferred to this routine to exit
//                       from an exception.  The state contained in the
//                       specified Trap Frame and Exception Frame is
//                       reloaded, and execution is resumed.
//
//  Note: This transfer of control occurs from
//
//      1. a fall thru from the above code
//      2. an exit from the continue system service
//      3. an exit from the raise exception system service
//      4. an exit into user mode from thread startup.
//
//  Arguments:
//
//      r.1 -- a valid stack pointer
//      r.2 -- kernel's TOC pointer
//      r.3 -- address of Exception Frame
//      r.4 -- address of Trap Frame
//
//  Return values:
//
//      There is no return from this routine.
//
//--------------------------------------------------------------------------

        FN_TABLE(KiExceptionExit_, 0, 0)

        DUMMY_ENTRY(KiExceptionExit_)

//  The following is never executed, it is provided to allow virtual
//  unwind to restore register state prior to an exception occuring.

        rfi                                     // tell unwinder to update establisher
                                                //   frame address using sp

        stw     r.sp, TrGpr1 (r.sp)             // Load r.1
        stw     r.12, TrGpr12 (r.sp)            // Load r.12
        bl      ..KiCommonFakeMillicode         // Restore the Exception Frame

        stw     r.0, TrGpr0 (r.sp)
        mflr    r.0                     // Sets only Lr
        stw     r.0, TrLr (r.sp)
        mflr    r.0                     // Sets Iar and Lr
        stw     r.0, TrIar (r.sp)
        mfcr    r.0
        stw     r.0, TrCr (r.sp)

        stw     r.2, TrGpr2 (r.sp)
        stw     r.3, TrGpr3 (r.sp)
        stw     r.4, TrGpr4 (r.sp)
        stw     r.5, TrGpr5 (r.sp)
        stw     r.6, TrGpr6 (r.sp)
        stw     r.7, TrGpr7 (r.sp)
        stw     r.8, TrGpr8 (r.sp)
        stw     r.9, TrGpr9 (r.sp)
        stw     r.10, TrGpr10 (r.sp)
        stw     r.11, TrGpr11 (r.sp)

        mfctr   r.6                   //   Fixed Point Exception
        mfxer   r.7                   //   registers

        stfd    f.0, TrFpr0 (r.sp)    // save volatile FPRs
        stfd    f.1, TrFpr1 (r.sp)
        stfd    f.2, TrFpr2 (r.sp)
        stfd    f.3, TrFpr3 (r.sp)
        stfd    f.4, TrFpr4 (r.sp)
        stfd    f.5, TrFpr5 (r.sp)
        stfd    f.6, TrFpr6 (r.sp)
        stfd    f.7, TrFpr7 (r.sp)
        stfd    f.8, TrFpr8 (r.sp)
        stfd    f.9, TrFpr9 (r.sp)
        stfd    f.10, TrFpr10 (r.sp)
        stfd    f.11, TrFpr11 (r.sp)
        stfd    f.12, TrFpr12 (r.sp)
        stfd    f.13, TrFpr13 (r.sp)
        mffs    f.0                   // get Floating Point Status
                                      //   and Control Register (FPSCR)

        stw     r.6, TrCtr (r.sp)     //      Count,
        stw     r.7, TrXer (r.sp)     //      Fixed Point Exception,
        stfd    f.0, TrFpscr (r.sp)   //  and FPSCR registers.

        stw     r.sp, 12 (r.sp)       // Load the Trap Frame
        stw     r.12, 4 (r.sp)        // Load the Exception Frame

        PROLOGUE_END(KiExceptionExit_)

        .align  6                               // cache line align

        ALTERNATE_ENTRY(KiExceptionExit)

        stw     r.3, 4(r.sp)                    // store r.3 + r.4 for use
        stw     r.4, 12(r.sp)                   // by virtual unwinder

        lbz     r.8, KiPcr+PcCurrentIrql(r.0)   // check if an APC could be
        cmplwi  r.8, APC_LEVEL                  // delivered now.
        bge     ee.apc_skip
ee.apc_recheck:
        lwz     r.15, TrMsr(r.4)                // load saved MSR value
        lbz     r.7, KiPcr+PcApcInterrupt(r.0)
        extrwi  r.15, r.15, 1, MSR_PR           // extract problem state bit
        or.     r.6, r.7, r.15                  // user mode || intr pending
        beq     ee.apc_skip                     // jif neither
        addic.  r.7, r.7, -1
        beq     ee.apc_intr                     // apc interrupt

//      no interrupt pending but going to user mode, check for user mode apc
//      pending.

        lwz     r.6, KiPcr+PcCurrentThread(r.0) // get address of current thread
        li      r.7, 0
        stb     r.7, ThAlerted(r.6)             // clear kernel mode alerted
        lbz     r.6, ThApcState+AsUserApcPending(r.6)
        cmplwi  r.6, 0
        beq     ee.apc_skip                     // jif none pending
        b       ee.apc_deliver

ee.apc_intr:
        stb     r.7, KiPcr+PcApcInterrupt(r.0)
ee.apc_deliver:
        lwz     r.6, KiPcr+PcPrcb(r.0)          // get address of PRCB
        ori     r.16, r.3, 0                    // save incoming Exception
        ori     r.14, r.4, 0                    // and Trap Frame addreses
        lwz     r.7, PbApcBypassCount(r.6)      // get APC bypass count

        li      r.5, APC_LEVEL                  // raise Irql to APC_LEVEL
        stb     r.5, KiPcr+PcCurrentIrql(r.0)

// Call to KiDeliverApc requires three parameters:
//   r.3  Previous Mode
//   r.4  addr of Exception Frame
//   r.5  addr of Trap Frame

        addi    r.7, r.7, 1                     // increment APC bypass count
        ori     r.5, r.4, 0                     // trap frame addr to r.5
        ori     r.4, r.3, 0                     // exception frame addr to r.4
        ori     r.3, r.15, 0                    // previous mode
        stw     r.7, PbApcBypassCount(r.6)      // store new APC bypass count
        bl      ..KiDeliverApc                  // process pending apc

        li      r.5, 0                          // lower Irql to < APC_LEVEL
        stb     r.5, KiPcr+PcCurrentIrql(r.0)
        ori     r.3, r.16, 0                    // restore saved frame
        ori     r.4, r.14, 0                    //   pointers
        b       ee.apc_recheck                  // check again

ee.apc_skip:

// Restore state from Exception Frame

        lfd     f.14, ExFpr14 (r.3)             // restore non-volatile FPRs
        lfd     f.15, ExFpr15 (r.3)
        lfd     f.16, ExFpr16 (r.3)
        lfd     f.17, ExFpr17 (r.3)
        lfd     f.18, ExFpr18 (r.3)
        lfd     f.19, ExFpr19 (r.3)
        lfd     f.20, ExFpr20 (r.3)
        lfd     f.21, ExFpr21 (r.3)
        lfd     f.22, ExFpr22 (r.3)
        lfd     f.23, ExFpr23 (r.3)
        lfd     f.24, ExFpr24 (r.3)
        lfd     f.25, ExFpr25 (r.3)
        lfd     f.26, ExFpr26 (r.3)
        lfd     f.27, ExFpr27 (r.3)
        lfd     f.28, ExFpr28 (r.3)
        lfd     f.29, ExFpr29 (r.3)
        lfd     f.30, ExFpr30 (r.3)
        lfd     f.31, ExFpr31 (r.3)

        lwz     r.14, ExGpr14 (r.3)             // restore non-volatile GPRs
        lwz     r.15, ExGpr15 (r.3)
        lwz     r.16, ExGpr16 (r.3)
        lwz     r.17, ExGpr17 (r.3)
        lwz     r.18, ExGpr18 (r.3)
        lwz     r.19, ExGpr19 (r.3)
        lwz     r.20, ExGpr20 (r.3)
        lwz     r.21, ExGpr21 (r.3)
        lwz     r.22, ExGpr22 (r.3)
        lwz     r.23, ExGpr23 (r.3)
        lwz     r.24, ExGpr24 (r.3)
        lwz     r.25, ExGpr25 (r.3)
        lwz     r.26, ExGpr26 (r.3)
        lwz     r.27, ExGpr27 (r.3)
        lwz     r.28, ExGpr28 (r.3)
        lwz     r.29, ExGpr29 (r.3)
        lwz     r.30, ExGpr30 (r.3)
        lwz     r.31, ExGpr31 (r.3)

        ori     r.3, r.4, 0                     // now r.3 points to Trap Frame

        b       ae.restore2                     // we already checked for apcs

        DUMMY_EXIT(KiExceptionExit_)

// ----------------------------------------------------------------------
//
// Entry here is only from other routines in real0.s
// On entry, r.1 (r.sp) points to stack frame containing Trap Frame
//    and space for Exception Frame.
// Non-volatile state is in regs, not in Exception Frame.
// Trap Frame and Exception Frame are addressed via stack pointer.
//

        FN_TABLE(KiAlternateExit_, 0, 0)

        DUMMY_ENTRY(KiAlternateExit_)

//  The following is never executed, it is provided to allow virtual
//  unwind to restore register state prior to an exception occuring.

        rfi                                     // tell unwinder to update establisher
                                                //   frame address using sp

        stw     r.sp, TrGpr1 + TF_BASE (r.sp)   // Load r.1
        stw     r.0, TrGpr0 + TF_BASE (r.sp)
        mflr    r.0                     // Sets only Lr
        stw     r.0, TrLr + TF_BASE (r.sp)
        mflr    r.0                     // Sets Iar and Lr
        stw     r.0, TrIar + TF_BASE (r.sp)
        mfcr    r.0
        stw     r.0, TrCr + TF_BASE (r.sp)

        stw     r.2, TrGpr2 + TF_BASE (r.sp)
        stw     r.3, TrGpr3 + TF_BASE (r.sp)
        stw     r.4, TrGpr4 + TF_BASE (r.sp)
        stw     r.5, TrGpr5 + TF_BASE (r.sp)
        stw     r.6, TrGpr6 + TF_BASE (r.sp)
        stw     r.7, TrGpr7 + TF_BASE (r.sp)
        stw     r.8, TrGpr8 + TF_BASE (r.sp)
        stw     r.9, TrGpr9 + TF_BASE (r.sp)
        stw     r.10, TrGpr10 + TF_BASE (r.sp)
        stw     r.11, TrGpr11 + TF_BASE (r.sp)
        stw     r.12, TrGpr12 + TF_BASE (r.sp)

        mfctr   r.6                   //   Fixed Point Exception
        mfxer   r.7                   //   registers

        stfd    f.0, TrFpr0 + TF_BASE (r.sp)    // save volatile FPRs
        stfd    f.1, TrFpr1 + TF_BASE (r.sp)
        stfd    f.2, TrFpr2 + TF_BASE (r.sp)
        stfd    f.3, TrFpr3 + TF_BASE (r.sp)
        stfd    f.4, TrFpr4 + TF_BASE (r.sp)
        stfd    f.5, TrFpr5 + TF_BASE (r.sp)
        stfd    f.6, TrFpr6 + TF_BASE (r.sp)
        stfd    f.7, TrFpr7 + TF_BASE (r.sp)
        stfd    f.8, TrFpr8 + TF_BASE (r.sp)
        stfd    f.9, TrFpr9 + TF_BASE (r.sp)
        stfd    f.10, TrFpr10 + TF_BASE (r.sp)
        stfd    f.11, TrFpr11 + TF_BASE (r.sp)
        stfd    f.12, TrFpr12 + TF_BASE (r.sp)
        stfd    f.13, TrFpr13 + TF_BASE (r.sp)
        mffs    f.0                   // get Floating Point Status
                                      //   and Control Register (FPSCR)

        stw     r.6, TrCtr + TF_BASE (r.sp)     //      Count,
        stw     r.7, TrXer + TF_BASE (r.sp)     //      Fixed Point Exception,
        stfd    f.0, TrFpscr + TF_BASE (r.sp)   //  and FPSCR registers.

        PROLOGUE_END(KiAlternateExit_)

        .align  6                               // cache line align

KiAlternateExit:

        lbz     r.8, KiPcr+PcCurrentIrql(r.0)   // level which could be
        cmplwi  r.8, APC_LEVEL                  // delivered now.
        bge     ae.restore
        lwz     r.8, TF_BASE+TrMsr(r.sp)        // load saved MSR value
        lbz     r.7, KiPcr+PcApcInterrupt(r.0)
        extrwi  r.8, r.8, 1, MSR_PR             // extract problem state bit
        or.     r.6, r.7, r.8                   // user mode || intr pending
        beq     ae.restore                      // jif neither
        addic.  r.7, r.7, -1
        beq     ae.apc_intr                     // apc interrupt

//      no interrupt pending but going to user mode, check for user mode apc
//      pending.

        lwz     r.6, KiPcr+PcCurrentThread(r.0) // get address of current thread
        li      r.7, 0
        stb     r.7, ThAlerted(r.6)             // clear kernel mode alerted
        lbz     r.6, ThApcState+AsUserApcPending(r.6)
        cmplwi  r.6, 0
        beq     ae.restore                      // jif none pending
        b       ae.apc_deliver

ae.apc_intr:
        cmplwi  r.8, 0                          // check previous mode
        stb     r.7, KiPcr+PcApcInterrupt(r.0)  // clear pending intr flag
        beq     ae.apc_kernel                   // if previous mode == kernel
ae.apc_deliver:

// Call KiDeliverApc() for pending APC, previous mode == user.  Before doing
// so, we must store the non-volatile state into the Exception Frame, for
// KiDeliverApc() takes Trap Frame and Exception Frame as input.

        la      r.4, EF_BASE (r.sp)             // addr of Exception Frame
        stw     r.13, ExGpr13 (r.4)             // store non-volatile GPRs
        stw     r.14, ExGpr14 (r.4)
        stw     r.15, ExGpr15 (r.4)
        stw     r.16, ExGpr16 (r.4)
        stw     r.17, ExGpr17 (r.4)
        stw     r.18, ExGpr18 (r.4)
        stw     r.19, ExGpr19 (r.4)
        stw     r.20, ExGpr20 (r.4)
        stw     r.21, ExGpr21 (r.4)
        stw     r.22, ExGpr22 (r.4)
        stw     r.23, ExGpr23 (r.4)
        stw     r.24, ExGpr24 (r.4)
        stw     r.25, ExGpr25 (r.4)
        stw     r.26, ExGpr26 (r.4)
        stw     r.27, ExGpr27 (r.4)
        stw     r.28, ExGpr28 (r.4)
        stw     r.29, ExGpr29 (r.4)
        stw     r.30, ExGpr30 (r.4)
        stw     r.31, ExGpr31 (r.4)

        stfd    f.14, ExFpr14 (r.4)             // save non-volatile FPRs
        stfd    f.15, ExFpr15 (r.4)
        stfd    f.16, ExFpr16 (r.4)
        stfd    f.17, ExFpr17 (r.4)
        stfd    f.18, ExFpr18 (r.4)
        stfd    f.19, ExFpr19 (r.4)
        stfd    f.20, ExFpr20 (r.4)
        stfd    f.21, ExFpr21 (r.4)
        stfd    f.22, ExFpr22 (r.4)
        stfd    f.23, ExFpr23 (r.4)
        stfd    f.24, ExFpr24 (r.4)
        stfd    f.25, ExFpr25 (r.4)
        stfd    f.26, ExFpr26 (r.4)
        stfd    f.27, ExFpr27 (r.4)
        stfd    f.28, ExFpr28 (r.4)
        stfd    f.29, ExFpr29 (r.4)
        stfd    f.30, ExFpr30 (r.4)
        stfd    f.31, ExFpr31 (r.4)

// The call to KiDeliverApc requires three parameters:
//   r.3  Previous Mode
//   r.4  addr of Exception Frame
//   r.5  addr of Trap Frame

ae.apc_kernel:
        li      r.3, APC_LEVEL                  // raise Irql
        stb     r.3, KiPcr+PcCurrentIrql(r.0)
ae.apc_again:
        lwz     r.6, KiPcr+PcPrcb(r.0)          // get address of PRCB
        la      r.5, TF_BASE (r.sp)             // r.5 = addr of trap frame
        lwz     r.3, TrMsr(r.5)                 // load saved MSR value
        la      r.4, EF_BASE (r.sp)             // r.4 = addr of except. frame
        lwz     r.7, PbApcBypassCount(r.6)      // get APC bypass count
        extrwi  r.3, r.3, 1, MSR_PR             // r.3 = previous mode
        addi    r.7, r.7, 1                     // increment APC bypass count
        stw     r.7, PbApcBypassCount(r.6)      // store new APC bypass count
        bl      ..KiDeliverApc                  // process pending apc

        lbz     r.7, KiPcr+PcApcInterrupt(r.0)
        addic.  r.7, r.7, -1
        bne     ae.apc_done                     // none pending, continue
        stb     r.7, KiPcr+PcApcInterrupt(r.0)
        b       ae.apc_again

ae.apc_done:
        li      r.3, 0                          // lower Irql < APC_LEVEL
        stb     r.3, KiPcr+PcCurrentIrql(r.0)

ae.restore:
        la      r.3, TF_BASE (r.sp)             // addr of Trap Frame

ae.restore2:

// Restore state from Trap Frame and control information

        DUMMY_EXIT(KiAlternateExit_)

        lfd     f.13, TrFpscr (r.3)             // get FP status
        lfd     f.0,  TrFpr0 (r.3)              // restore volatile FPRs
        lfd     f.1,  TrFpr1 (r.3)
        lfd     f.2,  TrFpr2 (r.3)
        lfd     f.3,  TrFpr3 (r.3)
        lfd     f.4,  TrFpr4 (r.3)
        lfd     f.5,  TrFpr5 (r.3)
        lfd     f.6,  TrFpr6 (r.3)
        lfd     f.7,  TrFpr7 (r.3)
        lfd     f.8,  TrFpr8 (r.3)
        lfd     f.9,  TrFpr9 (r.3)
        lfd     f.10, TrFpr10 (r.3)
        mtfsf   0xff, f.13                      // move FP status to FPSCR
        lfd     f.11, TrFpr11 (r.3)
        lfd     f.12, TrFpr12 (r.3)
        lfd     f.13, TrFpr13 (r.3)             // restore f.13

KiPriorityExit:

        mfmsr   r.10                            // get current MSR
        lwz     r.9,  TrCtr (r.3)               // get XER, LR, CTR
        lwz     r.8,  TrLr (r.3)
        lwz     r.7,  TrXer (r.3)
        lwz     r.5,  TrMsr (r.3)               // get resume MSR
        rlwinm  r.10, r.10, 0, ~INT_ENA         // turn off EE bit
        lwz     r.4,  TrGpr4 (r.3)              // get GPRs 4, 5 and 6 and
        lwz     r.0,  TrGpr5 (r.3)              // save them in the PCR
        lwz     r.6,  TrGpr6 (r.3)
        li      r.11, TrMsr                     // offset to MSR in TF
        lwz     r.2,  TrGpr2 (r.3)
        mtctr   r.9                             // restore XER, LR, CTR
        mtlr    r.8
        lwz     r.9,  TrGpr9 (r.3)
        mtxer   r.7
        lwz     r.8,  TrGpr8 (r.3)
        lwz     r.7,  TrGpr7 (r.3)

//
// WARNING:  Cannot tolerate page fault or interrupt.  TLB/HPT miss ok.
//

        mtmsr   r.10                            // disable interrupts
	cror	0,0,0				// N.B. 603e/ev Errata 15
        stwcx.  r.5, r.11, r.3                  // clear outstanding reserves.
        lwz     r.1,  TrGpr1 (r.3)              // restore stack pointer
        extrwi. r.11, r.5, 1, MSR_PR            // test resume PR bit

        stw     r.4, KiPcr+PCR_SAVE4 (r.0)      // save r.4, 5, 6 in PCR
        stw     r.0, KiPcr+PCR_SAVE5 (r.0)
        stw     r.6, KiPcr+PCR_SAVE6 (r.0)
        lwz     r.6, TrCr (r.3)                 // get resume CR
        lwz     r.4, TrIar (r.3)                // get resume IAR

        lwz     r.12, TrGpr12 (r.3)

//
// This should not be needed,.... but it is.
// Why?  plj 6/1/95.
//

        lwz     r.13, KiPcr+PcTeb (r.0)

        bne     kee.usermode                    // jif resuming user mode

        lwz     r.11, TrGpr11 (r.3)
        lwz     r.0, TrGpr0 (r.3)
        mtcrf   0xff, r.6                       // restore CR

        lwz     r.10, TrGpr10 (r.3)
        lwz     r.3,  TrGpr3  (r.3)

//
// WARNING:  Cannot tolerate TLB/HPT miss from mtsrr0 thru rfi.  On an MP
//           system, the TLB/HPT could be flushed by another processor, so
//           we use the BAT0 address of the PCR.
//

        mfsprg  r.6, sprg.1                     // get BAT0 addr of PCR
KiPriorityExitRfiJump1:
        b       $+(KiPriorityExitRfi-Kseg0CodeStart)

kee.usermode:

        lwz     r.4, KiPcr+PcCurrentThread(r.0) // get address of current thread
        lbz     r.0, ThDebugActive(r.4)         // Debug only but has to be here

        cmpwi   cr.1, r.0, 0                    // Hardware debug register set?
        bne-    cr.1, ke.debug                  // jif debug registers set

ke.09:
        lwz     r.4,  TrIar (r.3)               // get resume IAR (again)
        lwz     r.10, TrGpr10 (r.3)
        lwz     r.11, TrGpr11 (r.3)
        lwz     r.0,  TrGpr0  (r.3)
        mtcrf   0xff,r.6                        // restore CR

        lwz     r.3,  TrGpr3  (r.3)

//
// WARNING:  The following removes access to the system paged pool
//           address space.  The kernel stack is no longer addressable.
//

        lis     r.6, SREG_INVAL                 // invalidate segment registers

        mtsr    9, r.6                          // 9, 10, 12, 13
        mtsr    10, r.6
        mtsr    12, r.6
        mtsr    13, r.6

//
// WARNING:  Cannot tolerate TLB/HPT miss from mtsrr0 thru rfi.  On an MP
//           system, the TLB/HPT could be flushed by another processor, so
//           we use the BAT0 address of the PCR.
//

        mfsprg  r.6, sprg.1                     // get BAT0 addr of PCR
KiPriorityExitRfiJump2:
        b       $+(KiPriorityExitRfi-Kseg0CodeStart)

//
// The following code is out of line for efficiency.   It is only
// executed when we are resuming to user mode in a thread that has
// h/w breakpoints set.
//
// Registers 0, 4, 10 and 11 are available.
//

ke.debug:

        lwz     r.4, TrDr1 (r.3)                // Get kernel DABR
        lwz     r.11, TrDr7 (r.3)
        lwz     r.10, TrDr0 (r.3)                // Get kernel IABR
        rlwinm  r.4, r.4, 0, 0xfffffff8         // Sanitize DABR (Dr1)
        ori     r.10, r.10, 0x3                   // Sanitize IABR (Dr0) 604
        ori     r.4, r.4, 0x4                   // Sanitize DABR       604
//
// WARNING: Don't rearrange this branch table. The first branch is overlayed
// with the correct branch instruction (modified) based on the processor
// during system initialization. The correct order is 601, 603, 604/613, 620, skip.
//
BranchDr2:
        b       ke.601                          // 601
        b       ke.603                          // 603
        b       ke.604                          // 604/613
        b       ke.604                          // 620 -- common with 604/613
        b       ke.09                           // unknown

ke.601:                                         // 601 SPECIFIC
        lis     r.0, 0x6080                     // Full cmp., trace mode except.
        rlwinm  r.10, r.10, 0, 0xfffffffc       // Sanitize IABR (Dr0)
        rlwinm  r.4,  r.4,  0, 0xfffffff8       // Sanitize DABR (Dr0) undo 604
        mtspr   hid1, r.0

ke.604:
        rlwinm. r.0, r.11, 0, 0x0000000c        // LE1/GE1 set?
        beq     kedr1.1                         // jiff Dr1 not set
        rlwimi  r.4, r.11, 13, 30, 30           // Interchange R/W1 bits
        rlwimi  r.4, r.11, 11, 31, 31
        mtspr   dabr, r.4

kedr1.1:
        rlwinm. r.11, r.11, 0, 0x00000003       // LE0/GE0 set?
        beq     ke.09
        mtspr   iabr, r.10
        isync
        b       ke.09

ke.603:                                         // 603 SPECIFIC
        rlwinm  r.10, r.10, 0, 0xfffffffc       // Sanitize IABR
        ori     r.10, r.10, 0x2
        mtspr   iabr, r.10
        b       ke.09

//-----------------------------------------------------------------
//
// System Call interrupt -- system_service_dispatch
//
//   This is the kernel entry point for System Service calls.
//
//   The processor is running with instruction and data relocation
//   enabled when control reaches here.
//
//   Invocation of a "system service" routine
//
//   Calls to the system service "stubs" (Zw<whatever>, Nt<whatever>) are
//   always call-thru-function-descriptor, like this:
//
//      Calling procedure:
//         get addr of descriptor
//         save TOC pointer, if not already saved
//         load entry point addr from TOC,     (gets addr of ..ZwSysCallInstr)
//            move to LR
//         load callee's TOC addr from TOC     (gets system service code)
//         branch-and-link via LR
//
//      ..ZwSysCallInstr:
//         <system call> instr
//         return
//
//   The function descriptors for the system services are specially built.
//   All of them point to the same entry point in the first word,
//   ..ZwSysCallInstr.  Instead of a TOC address, the system call code is
//   in the second word.
//
//
//   on Entry:
//      MSR:    External interrupts disabled
//              Instruction Relocate ON
//              Data Relocate ON
//      GP registers:
//        r.0:  Address of entry point
//        r.2:  System Service number
//        r.3:  thru r.10 system call paramaters
//        r.12: Previous mode (saved srr1)
//
//        cr.0 eq set if previous mode was kernel
//
//      Available registers:
//        r.0
//
//      All other registers still have their contents as of the time
//      of interrupt
//
//  Our stack frame header must contain space for 16 words of arguments, the
//  maximum that can be specified on a system call.  Stack frame header struct
//  defines space for 8 such words.
//
//  We'll build a structure on the stack like this:
//
//      low addr  |                    |
//                |                    |
//             /  |--------------------| <-r.1 at point we call
//            |   | Stack Frame Header |   KiDispatchException
//            |   | (back chain, misc. |
//            |   | stuff, 16 words of |
//            |   | parameter space)   |
//           /    |--------------------|
//                | Trap Frame         |
//    STACK_DELTA | (volatile state)   |
//                |                 <------ includes ExceptionRecord, imbedded within
//           \    |--------------------|
//            |   | Exception Frame    |   Exception frame only if previous
//            |   | (non-volatile      |   mode == User mode
//            |   | state)             |
//            |   |                    |
//             \  |--------------------| <-r.1 at point of interrupt, if interrupted
//                |                    |   kernel code, or base of kernel stack if
//      high addr |                    |   interrupted user code

//
//--------------------------------------------------------------------------
//  The following is never executed, it is provided to allow virtual
//  unwind to restore register state prior to an exception occuring.
//  This is a common prologue for the various exception handlers.

        FN_TABLE(KiSystemServiceDispatch,0,0)

        DUMMY_ENTRY(KiSystemServiceDispatch)

        stwu    r.sp, -STACK_DELTA (r.sp)
        mflr    r.0
        stw     r.0, TrLr + TF_BASE (r.sp)

        PROLOGUE_END(KiSystemServiceDispatch)

        .align  6                               // ensure the following is
                                                // cache block aligned (for
                                                // performance) (cache line
                                                // for 601)
system_service_dispatch:

//
// We need another register, trash r.13 which contains the Teb pointer
// and reload it later. (r.13 might not be TEB if was kernel mode).
//

        lwz     r.11, KiPcr+PcInitialStack(r.0) // kernel stack addr for thread
        lwz     r.13, KiPcr+PcCurrentThread(r.0)// get current thread addr
        beq     ssd.20                          // branch if was in kernel state

//
// Previous state was user mode
//
// Segment registers 9, 10, 12, and 13 need to be setup for kernel mode.
// In user mode they are set to zero as no access is allowed to these
// segments, and there are no combinations of Ks Kp and PP that allow
// kernel both read-only and read/write pages that are user no-access.
//

        mfsr    r.0, 0                          // get PID from SR0

        ori     r.0, r.0, 12                    // T=0 Ks,Kp=0 VSID=pgdir,12
        mtsr    12, r.0
        li      r.0, 9                          // T=0 Ks,Kp=0 VSID=9
        mtsr    9, r.0
        li      r.0, 10                         // T=0 Ks,Kp=0 VSID=10
        mtsr    10, r.0
        li      r.0, 13                         // T=0 Ks,Kp=0 VSID=13
        mtsr    13, r.0
        isync                                   // context synchronize

//
// Allocate stack frame and save old stack pointer and other volatile
// registers in trap frame (needed if user mode APC needs to be run
// on exit).
//

        lbz     r.0,  ThDebugActive(r.13)       // get h/w debug flag
        stw     r.sp, TrGpr1  + TF_BASE - USER_SYS_CALL_FRAME(r.11)
        stw     r.sp, CrBackChain       - USER_SYS_CALL_FRAME(r.11)
        stw     r.2,  TrGpr2  + TF_BASE - USER_SYS_CALL_FRAME(r.11)
        subi    r.sp, r.11, USER_SYS_CALL_FRAME
        stw     r.3,  TrGpr3  + TF_BASE - USER_SYS_CALL_FRAME(r.11)
        stw     r.4,  TrGpr4  + TF_BASE(r.sp)
        stw     r.5,  TrGpr5  + TF_BASE(r.sp)
        stw     r.6,  TrGpr6  + TF_BASE(r.sp)
        cmpwi   cr.1, r.0,  0                   // Hardware debug register set?
        stw     r.7,  TrGpr7  + TF_BASE(r.sp)
        stw     r.8,  TrGpr8  + TF_BASE(r.sp)
        stw     r.9,  TrGpr9  + TF_BASE(r.sp)
        stw     r.10, TrGpr10 + TF_BASE(r.sp)

        bne-    cr.1, ssd.dbg_regs              // jif debug registers set
        b       ssd.30                          // join common code

//
// Processor was in supervisor state.  We'll add our stack frame to the stack
// whose address is still in r.1 from the point of interrupt.
//
// Test stack (r.sp) for 8 byte alignment, overflow or underflow.
//

ssd.20:

        andi.   r.0, r.sp, 7                    // 8-byte align into cr.0
        lwz     r.0, KiPcr+PcStackLimit(r.0)    // get current stack limit
        cmplw   cr.1, r.sp, r.11                // underflow, into cr.1
        subi    r.11, r.sp, KERN_SYS_CALL_FRAME // allocate stack frame; ptr into r.11
        cmplw   cr.5, r.11, r.0                 // test for overflow, into cr.2
        bgt-    cr.1, ssd.stk_err               // jif stack has underflowed
        bne-    cr.0, ssd.stk_err               // jif stack is misaligned
        blt-    cr.5, ssd.stk_err               // jif stack has overflowed

//
// Stack looks ok, use it.  First, save the old stack pointer in the
// back chain.  Also, we need an additional scratch register, save r.10
// now (already done in user mode case).
//

        stw     r.sp, CrBackChain(r.11)         // save old stack pointer
        stw     r.10,TrGpr10+TF_BASE(r.11)      // save r.10
        ori     r.sp, r.11, 0                   // set new stack pointer

//
// The following code is common to both user mode and kernel mode entry.
// Stack address in r.sp, Save Trap Frame volatile registers.
//

ssd.30:

        mflr    r.0                             // get return address
        mffs    f.0                             // get FPSCR
        lbz     r.11, KiPcr+PcCurrentIrql(r.0)  // get old (current) irql
        stw     r.12, TrMsr  + TF_BASE(r.sp)    // save SRR1  (MSR)
        stw     r.0,  TrIar  + TF_BASE(r.sp)    // save return addr in TrapFrame
        stw     r.0,  TrLr   + TF_BASE(r.sp)    // save Link register
        stfd    f.0,  TrFpscr + TF_BASE(r.sp)   // save FPSCR
        stb     r.11, TrOldIrql + TF_BASE(r.sp) // save current Irql in tf

//
// Use the service code as an index into the thread's service table, and call
// the routine indicated.
//
// At this point-
//
// r.0  (scratch)
// r.1  contains the current stack pointer.
// r.2  contains the service code (still).
// r.3 - r.10 are untouched (r.10 saved in trap frame)
// r.11 is available
// r.12 contains the old MSR value
// r.13 contains the current thread address
// r.14 - r.31 are untouched
//
// Warning, don't enable interrupts until you (at least) don't care what's
// in r.13
//

        lbz     r.0, ThPreviousMode(r.13)       // get old previous mode
        lwz     r.11, ThTrapFrame(r.13)         // get current trap frame address
        extrwi  r.12, r.12, 1, MSR_PR           // extract user mode bit
        stb     r.12, ThPreviousMode(r.13)      // set new previous mode
        stb     r.0, TrPreviousMode+TF_BASE(r.sp) // save old previous mode
        stw     r.11, TrTrapFrame+TF_BASE(r.sp) // save current trap frame address

        la      r.0, TF_BASE(r.sp)              // get trap frame address
        lwz     r.12, ThServiceTable(r.13)      // get service descriptor table address
        stw     r.0, ThTrapFrame(r.13)          // store trap frame address

#if DBG
        lbz     r.0, ThKernelApcDisable(r.13)   // get current APC disable count
        stb     r.0, TrSavedKernelApcDisable+TF_BASE(r.sp) // save APC disable count
        lbz     r.13, ThApcStateIndex(r.13)     // get current APC state index
        stb     r.13, TrSavedApcStateIndex+TF_BASE(r.sp) // save APC state index
#endif

        mfmsr   r.0                             // fetch the current MSR
        lwz     r.13, KiPcr+PcTeb(r.0)          // restore Teb (ok enable ints)
        ori     r.0, r.0, INT_ENA               // enable interrupts
        mtmsr   r.0                             // external interrupts enabled
	cror	0,0,0				// N.B. 603e/ev Errata 15

SystemServiceRepeat:

        rlwinm  r.10, r.2, 32-SERVICE_TABLE_SHIFT, SERVICE_TABLE_MASK
        add     r.12, r.12, r.10                // compute service descriptor address
        cmpwi   cr.7, r.10, SERVICE_TABLE_TEST  // is this a GUI service?  (checking
                                                //  service descriptor offset)
        lwz     r.11, SdLimit(r.12)             // get service number limit
        rlwinm  r.0, r.2, 0, SERVICE_NUMBER_MASK // isolate service table offset
        cmplw   r.0, r.11                       // check service number against limit
        bge     ssd.convert_to_gui              // jump if service number too high

        slwi    r.0, r.0, 2                     // compute system service offset value

#if DBG
        lwz     r.11, SdCount(r.12)             // get service count table address
        cmpwi   r.11, 0                         // does table exist?
        beq     ssd.100                         // if not, skip update
        lwzux   r.2, r.11, r.0                  // get count and addr of count
        addi    r.2, r.2, 1                     // increment service count
        stw     r.2, 0(r.11)                    // store new count
ssd.100:
#endif

        lwz     r.11, SdBase(r.12)              // get service table address
        lwz     r.2, -4(r.11)                   // get toc for this service table
        lwzx    r.11, r.11, r.0                 // get address of service routine

//
// If the system service is a GUI service and the GDI user batch queue is
// not empty, then call the appropriate service to flush the user batch.
//

        bne     cr.7, ssd.115                   // if ne, not GUI system service

        lwz     r.10, TeGdiBatchCount(r.13)     // get number of batched GDI calls
        stw     r.11, TrGpr11+TF_BASE(r.sp)     // save service routine address
        cmpwi   r.10, 0                         // check number of batched GDI calls

        stw     r.3, TrGpr3+TF_BASE(r.sp)       // save arguments (r10 already saved)
        beq     ssd.115                         // if eq, no batched calls
        bl      ssd.113                         // get a base address to use to load toc
ssd.113:
        stw     r.2, TrGpr2+TF_BASE(r.sp)       // save service table TOC
        mflr    r.2                             // get &ssd.113
        stw     r.4, TrGpr4+TF_BASE(r.sp)
        lwz     r.2, toc_pointer-ssd.113(r.2)   // load toc address
        stw     r.5, TrGpr5+TF_BASE(r.sp)
        lwz     r.2, [toc]KeGdiFlushUserBatch(r.2) // get address of flush routine
        stw     r.6, TrGpr6+TF_BASE(r.sp)
        lwz     r.2, 0(r.2)                     // get address of descriptor
        stw     r.7, TrGpr7+TF_BASE(r.sp)
        lwz     r.3, 0(r.2)                     // get address of flush routine
        stw     r.8, TrGpr8+TF_BASE(r.sp)
        lwz     r.2, 4(r.2)                     // get TOC for flush routine
        mtctr   r.3
        stw     r.9, TrGpr9+TF_BASE(r.sp)
        stw     r.0, TrGpr0+TF_BASE(r.sp)       // save locals in r0 and r12
        stw     r.12, TrGpr12+TF_BASE(r.sp)
        bctrl                                   // call GDI user batch flush routine

        lwz     r.11,TrGpr11+TF_BASE(r.sp)      // restore service routine address

        lwz     r.3,TrGpr3+TF_BASE(r.sp)        // restore arguments (except r10)
        lwz     r.4,TrGpr4+TF_BASE(r.sp)
        lwz     r.5,TrGpr5+TF_BASE(r.sp)
        lwz     r.6,TrGpr6+TF_BASE(r.sp)
        lwz     r.7,TrGpr7+TF_BASE(r.sp)
        lwz     r.8,TrGpr8+TF_BASE(r.sp)
        lwz     r.9,TrGpr9+TF_BASE(r.sp)

        lwz     r.0,TrGpr0+TF_BASE(r.sp)        // restore locals in r0 and r12
        lwz     r.12,TrGpr12+TF_BASE(r.sp)

        lwz     r.2,TrGpr2+TF_BASE(r.sp)        // restore service table TOC

ssd.115:

//
// Low-order bit of service table entry indicates presence of in-memory
// arguments.  Up to 8 args are passed in GPRs; any additional are passed
// in memory in the caller's stack frame.
//
// note: we put the entry in the link register anyway, the bottom two bits
//       are ignored as a branch address.
//

        mtlr    r.11                            // set service routine address
        andi.   r.11, r.11, 1                   // low-order bit set?
        lwz     r.10,TrGpr10+TF_BASE(r.sp)      // restore r10
        beq+    ssd.150                         // jif no in-memory arguments

//
// Capture arguments passed in memory in caller's stack frame, to ensure that
// caller does not modify them after they are probed and, in kernel mode,
// because a trap frame has been allocated on the stack.
//
// For PowerPC, space for all the passed arguments is allocated in the caller's
// stack frame.  The first 8 words are actually passed in GPRs, but space is
// allocated for them in case the called routine needs to take the address of
// any of the arguments.  Arguments past the 8th word are passed in storage.
//
// The "in-memory arguments" flag means that at least 9 words of parameters
// are passed on this call, the first 8 being in GPRs 3 through 10.  Since
// we will call the system call target using our own stack frame, we must
// copy our caller's in-memory arguments into our frame so that our callee
// can find them.
//
// It is thought that the loop below, using 0-cycle branch-on-count, will be
// faster on average than a straight branch-free copy of 8 words, but only
// time will tell.

        srwi    r.0, r.0, 2                     // compute argument count offset value
        lwz     r.11, SdNumber(r.12)            // get pointer to argument count table
        lwz     r.12, 0(r.sp)                   // load caller's stack pointer
        lbzx    r.0, r.11, r.0                  // load count of bytes to copy
        addi    r.11, r.sp, CrParameter7        // point to target, less 1 word
        srwi    r.0, r.0, 2                     // compute count of words to copy
        mtctr   r.0                             // move count to CTR
        addi    r.12, r.12, CrParameter7        // point to source, less 1 word
ssd.120:
        lwzu    r.0, 4(r.12)                    // copy one word from source to
        stwu    r.0, 4(r.11)                    //   target, updating pointers
        bdnz    ssd.120                         // decrement count, jif non-zero

//
// Call the system service.
//
// Note that the non-volatile state is still in GPRs 13..31 and FPRs 14..31;
// it is the called service routine's responsibility to preserve it.

ssd.150:

// In keeping with the assembler code in mips/x4trap.s, we put the
// Trap Frame address in r.12 so that NtContinue() can find it, just
// in case it happens to be the target.

        la      r.12, TF_BASE(r.sp)             // mips code passes this in s8

        blrl                                    // call the target

//----------------------------------------------------------
//
// Exit point for system calls
//
//----------------------------------------------------------

        ALTERNATE_ENTRY (KiSystemServiceExit)

//
// Increment count of system calls and see if an APC interrupt should be
// generated now.
//
// Restore old trap frame address from the current trap frame.
//

        lwz     r.4, KiPcr+PcPrcb(r.0)          // get processor block address
        la      r.12, TF_BASE(r.sp)             // get trap frame address
        lwz     r.6, KiPcr+PcCurrentThread(r.0) // get current thread address
        lbz     r.8, KiPcr+PcCurrentIrql(r.0)   // get current IRQL
        lwz     r.5, PbSystemCalls(r.4)         // get count of system calls
        lwz     r.9, TrTrapFrame(r.12)          // get old trap frame address
        lwz     r.10, TrMsr(r.12)               // load saved MSR value
        addi    r.5, r.5, 1                     // bump count of system calls
        stw     r.9, ThTrapFrame(r.6)           // restore old trap frame address
        stw     r.5, PbSystemCalls(r.4)         // store new count of system calls

//
// KiServiceExit is an alternate entry referenced by KiCallUserMode in callout.s.
// On entry:
//      r6  -- current thread
//      r8  -- contains current IRQL
//      r10 -- contains saved MSR
//      r12 -- points to trap frame
//
// NOTE: r.sp CANNOT BE USED FROM THIS POINT ON, except on paths that cannot
// have come from KiCallUserMode.  This is because the stack pointer is
// different when this is a normal system service than when this is a user
// mode callout.
//

        ALTERNATE_ENTRY (KiServiceExit)

#if DBG
        lbz     r.5, ThKernelApcDisable(r.6)    // get current APC disable count
        lbz     r.7, ThApcStateIndex(r.6)       // get current APC state index
        lbz     r.4, TrSavedKernelApcDisable(r.12) // get previous APC disable count
        lbz     r.9, TrSavedApcStateIndex(r.12) // get previous APC state index
        xor     r.4, r.4, r.5                   // compare APC disable count
        xor     r.7, r.9, r.7                   // compare APC state index
        or.     r.7, r.7, r.4                   // merge comparison value
        bne     ssd.badapc                      // if ne, invalid state or count
#endif

        cmplwi  r.8, APC_LEVEL                  // APC deliverable?
        bge     ssd.190                         // not at APC level, continue
        lbz     r.7, KiPcr+PcApcInterrupt(r.0)
        extrwi  r.8, r.10, 1, MSR_PR            // extract problem state bit
        or.     r.0, r.7, r.8                   // user mode || intr pending
        beq+    ssd.190                         // jif neither
        addic.  r.7, r.7, -1
        beq     ssd.160                         // apc interrupt

//      no interrupt pending but going to user mode, check for user mode apc
//      pending.

        li      r.7, 0
        stb     r.7, ThAlerted(r.6)             // clear kernel mode alerted
        lbz     r.6, ThApcState+AsUserApcPending(r.6)
        cmplwi  r.6, 0
        beq+    ssd.190                         // none pending, continue

        la      r.4, EF_BASE - TF_BASE (r.12)   // addr of Exception Frame
        b       ssd.170

ssd.170ep:
        la      r.4, EF_BASE (r.sp)             // addr of Exception Frame
        la      r.12, TF_BASE (r.sp)            // addr of Trap Frame
        b       ssd.170

ssd.160:
        cmplwi  r.8, 0                          // check previous mode
        stb     r.7, KiPcr+PcApcInterrupt(r.0)  // clear pending intr flag
        la      r.4, EF_BASE - TF_BASE (r.12)   // addr of Exception Frame
        beq     ssd.180                         // if previous mode == kernel
ssd.170:

// Call KiDeliverApc() for pending APC, previous mode == user.  Before doing
// so, we must store the non-volatile state into the Exception Frame, for
// KiDeliverApc() takes Trap Frame and Exception Frame as input.

        stw     r.13, ExGpr13 (r.4)             // store non-volatile GPRs
        stw     r.14, ExGpr14 (r.4)
        stw     r.15, ExGpr15 (r.4)
        stw     r.16, ExGpr16 (r.4)
        stw     r.17, ExGpr17 (r.4)
        stw     r.18, ExGpr18 (r.4)
        stw     r.19, ExGpr19 (r.4)
        stw     r.20, ExGpr20 (r.4)
        stw     r.21, ExGpr21 (r.4)
        stw     r.22, ExGpr22 (r.4)
        stw     r.23, ExGpr23 (r.4)
        stw     r.24, ExGpr24 (r.4)
        stw     r.25, ExGpr25 (r.4)
        stw     r.26, ExGpr26 (r.4)
        stw     r.27, ExGpr27 (r.4)
        stw     r.28, ExGpr28 (r.4)
        stw     r.29, ExGpr29 (r.4)
        stw     r.30, ExGpr30 (r.4)
        stw     r.31, ExGpr31 (r.4)

        stfd    f.14, ExFpr14 (r.4)             // save non-volatile FPRs
        stfd    f.15, ExFpr15 (r.4)
        stfd    f.16, ExFpr16 (r.4)
        stfd    f.17, ExFpr17 (r.4)
        stfd    f.18, ExFpr18 (r.4)
        stfd    f.19, ExFpr19 (r.4)
        stfd    f.20, ExFpr20 (r.4)
        stfd    f.21, ExFpr21 (r.4)
        stfd    f.22, ExFpr22 (r.4)
        stfd    f.23, ExFpr23 (r.4)
        stfd    f.24, ExFpr24 (r.4)
        stfd    f.25, ExFpr25 (r.4)
        stfd    f.26, ExFpr26 (r.4)
        stfd    f.27, ExFpr27 (r.4)
        stfd    f.28, ExFpr28 (r.4)
        stfd    f.29, ExFpr29 (r.4)
        stfd    f.30, ExFpr30 (r.4)
        stfd    f.31, ExFpr31 (r.4)

// Also, clear volatile state within the trap frame that will be restored
// by NtContinue when the APC completes that has not already been set to
// reasonable values.  (ie what wasn't saved on entry).

        li      r.0, 0
        stw     r.0, TrGpr11(r.12)
        stw     r.0, TrGpr12(r.12)
        stw     r.0, TrGpr0 (r.12)
        stw     r.0, TrXer  (r.12)

// Call to KiDeliverApc requires three parameters:
//   r.3  Previous Mode
//   r.4  addr of Exception Frame
//   r.5  addr of Trap Frame

ssd.180:
        bl      ssd.185                         // get a base address to
ssd.185:                                        // use to load kernel toc
        lwz     r.6, KiPcr+PcPrcb(r.0)          // get address of PRCB
        li      r.5, APC_LEVEL                  // raise Irql
        stw     r.3, TrGpr3(r.12)               // save sys call return value
        lwz     r.0, TrMsr (r.12)               // load user's MSR value from trap frame
        stb     r.5, KiPcr+PcCurrentIrql(r.0)
        ori     r.5, r.12, 0                    // r.5 <- trap frame addr
        lwz     r.7, PbApcBypassCount(r.6)      // get APC bypass count
        mflr    r.2                             // get &ssd.185
        extrwi  r.3, r.0, 1, MSR_PR             // r.3 <- prev. state
        addi    r.7, r.7, 1                     // increment APC bypass count

        stw     r.12, DeliverApcSaveTrap(r.sp)  // save trap frame addr
        lwz     r.2, toc_pointer-ssd.185(r.2)   // load toc address
        stw     r.7, PbApcBypassCount(r.6)      // store new APC bypass count
        bl      ..KiDeliverApc                  // process pending apc
        lwz     r.12, DeliverApcSaveTrap(r.sp)  // restore trap frame addr

        li      r.8, 0
        stb     r.8, KiPcr+PcCurrentIrql(r.0)   // restore old IRQL
        lwz     r.10, TrMsr(r.12)               // get caller's MSR value
        lwz     r.3, TrGpr3(r.12)               // restore sys call result
ssd.190:

//
// Return to the caller, in the proper mode.
//
// As this is like an ordinary call, we need not restore the volatile
// registers.  We must preserve r.3, the possible return value from
// the system service routine.
//
// If we are returning to user state, we zero the rest of the volatile
// state to prevent unauthorized viewing of left-over information.
//
// The non-volatile state has been preserved by our callee, as for all calls.
//
// We get the caller's stack frame pointer out of the back chain field
// in our stack frame header, not by incrementing our stack pointer, because
// the caller's stack may be user while ours is known to be kernel.
//
// We must reload these:
//    caller's TOC pointer (r.2)
//    caller's stack pointer (r.1)
//    caller's instruction address
//
// We already have
//    caller's MSR value (r.10)
//
// We can use (and must clear) these:
//    r.0
//    r.4 - r.9, r.11, r.12
//    f.0 - f.13
//    XER
//    CR

        lfd     f.0, TrFpscr(r.12)              // get saved FPSCR
        lwz     r.7, KiPcr+PcCurrentThread(r.0) // get current thread address
        extrwi. r.0, r.10, 1, MSR_PR            // see if resuming user mode
        lbz     r.6, TrPreviousMode(r.12)       // get old previous mode
        mfmsr   r.8                             // fetch the current MSR value
        lwz     r.11,  TrIar(r.12)              // get caller's resume address
        rlwinm  r.8, r.8, 0, ~(INT_ENA)         // clear int enable
        stb     r.6, ThPreviousMode(r.7)        // restore old previous mode
        mtfsf   0xff, f.0                       // restore FPSCR
        mtmsr   r.8                             // disable interrupts
	cror	0,0,0				// N.B. 603e/ev Errata 15
        bne     ssd.200                         // branch if resuming user mode

// Resuming kernel mode -- don't bother to clear the volatile state


        lwz     r.sp, 0(r.sp)                   // reload caller's stack pointer

//
// WARNING:  Cannot tolerate a TLB/HPT miss from here thru rfi.
//

KiServiceExitKernelRfiJump:
        b       $+(KiServiceExitKernelRfi-Kseg0CodeStart)

// Resuming user mode -- clear the volatile state

ssd.200:
        lbz     r.6, ThDebugActive(r.7)
        lis     r.5, K_BASE                     // base address of KSEG0
        lfd     f.0, FpZero-real0(r.5)          // load FP 0.0
        li      r.0, 0                          // clear a GP reg
        lwz     r.toc, TrGpr2(r.12)             // reload caller's TOC pointer
        cmpwi   cr.1, r.6, 0                    // Hardware debug register set?
        lwz     r.8, TrLr(r.12)                 // get saved LR
        mtxer   r.0                             // clear the XER
        bne     cr.1, ssd.220                   // jif no debug registers set

ssd.210:
        fmr     f.1, f.0                        // clear remaining volatile FPRs
        lwz     r.5, TrGpr5(r.12)
        mtctr   r.0                             // clear the CTR
        fmr     f.2, f.0
        lwz     r.6, TrGpr6(r.12)
        lis     r.9, SREG_INVAL                 // invalid segment reg value
        fmr     f.3, f.0
        lwz     r.4, TrGpr4(r.12)
        fmr     f.4, f.0
        lwz     r.sp, TrGpr1(r.12)              // reload caller's stack pointer
        fmr     f.5, f.0
        li      r.7, 0
        fmr     f.6, f.0
        fmr     f.7, f.0
        fmr     f.8, f.0
        mtlr    r.8                             // restore saved LR
        fmr     f.9, f.0
        li      r.8, 0
        fmr     f.10, f.0
        mtsr    9, r.9                          // invalidate sregs 9, 10, 12, 13
        fmr     f.11, f.0
        mtsr    10, r.9
        fmr     f.12, f.0
        mtsr    12, r.9
        fmr     f.13, f.0
        mtsr    13, r.9

//
// WARNING:  Cannot tolerate a TLB/HPT miss from here thru rfi.
//

KiServiceExitUserRfiJump:
        b       $+(KiServiceExitUserRfi-Kseg0CodeStart)






//
// We get here from above if we are going to user mode and the h/w debug
// registers are being used.   This is where we renable them for this
// processor.  The code is out of line on the theory that it isn't used
// all that often.
//

ssd.220:
        lwz     r.4, TrDr1 (r.12)               // Get kernel DABR
        lwz     r.5, TrDr7 (r.12)
        lwz     r.6, TrDr0 (r.12)               // Get kernel IABR
        rlwinm  r.4, r.4, 0, 0xfffffff8         // Sanitize DABR (Dr1)
        ori     r.6, r.6, 0x3                   // Sanitize IABR (Dr0) 604
        ori     r.4, r.4, 0x4                   // Sanitize DABR       604
//
// WARNING: Don't rearrange this branch table. The first branch is overlayed
// with the correct branch instruction (modified) based on the processor
// during system initialization. The correct order is 601, 603, 604, skip.
//
BranchDr4:
        b       ssd.201                         // 601
        b       ssd.203                         // 603
        b       ssd.204                         // 604/613
        b       ssd.204                         // 620 -- common with 604/613
        b       ssd.210                         // unknown

ssd.201:                                        // 601 SPECIFIC
        lis     r.9, 0x6080                     // Full cmp., trace mode except.
        rlwinm  r.4, r.4, 0, 0xfffffff8         // Sanitize DABR (Dr1)
        rlwinm  r.6, r.6, 0, 0xfffffffc         // Sanitize IABR (Dr0)
        mtspr   hid1, r.9

ssd.204:
        rlwinm. r.9, r.5, 0, 0x0000000c         // LE1/GE1 set?
        beq     ssddr11.1                       // jiff Dr1 not set
        rlwimi  r.4, r.5, 13, 30, 30            // Interchange R/W1 bits
        rlwimi  r.4, r.5, 11, 31, 31
        mtspr   dabr, r.4

ssddr11.1:
        rlwinm. r.5, r.5, 0, 0x00000003         // LE0/GE0 set?
        beq     ssd.210
        mtspr   iabr, r.6
        isync
        b       ssd.210

ssd.203:                                        // 603 SPECIFIC
        rlwinm  r.6, r.6, 0, 0xfffffffc         // Sanitize IABR
        ori     r.6, r.6, 0x2
        mtspr   iabr, r.6
        b       ssd.210





        .align  5

ssd.convert_to_gui:

//
// The specified system service number is not within range. Attempt to
// convert the thread to a GUI thread if specified system service is
// not a base service and the thread has not already been converted to
// a GUI thread.
//
// N.B. The argument registers r3-r10 and the system service number in r2
//      must be preserved if an attempt is made to convert the thread to
//      a GUI thread.
//
// At this point:
//
// r.0  contains masked service number (scratch)
// r.1  contains the current stack pointer
// r.2  contains the service code
// r.3 - r.9 are untouched
// r.10 contains the offset into the service descriptor table
// r.11 contains the service number limit for the r.12 table (scratch)
// r.12 contains the service descriptor address
// r.13 contains the current TEB address
// r.14 - r.31 are untouched
// cr.7 is the result of comparing r.10 with 0 (if eq, service is a base service)
//
// On return to SystemServiceRepeat:
//
// r.0  is undefined
// r.1  contains the current stack pointer
// r.2  contains the service code
// r.3 - r.9 are untouched
// r.10 is undefined
// r.11 is undefined
// r.12 contains the service descriptor table address (ThWin32Thread)
// r.13 contains the current TEB address
// r.14 - r.31 are untouched
//

        bne     cr.7, ssd.inv_service           // if ne, not GUI system service

        stw     r.2,TrFpr0+TF_BASE(r.sp)        // save system service number
        stw     r.3,TrGpr3+TF_BASE(r.sp)        // save argument registers (except r10)
        stw     r.4,TrGpr4+TF_BASE(r.sp)
        stw     r.5,TrGpr5+TF_BASE(r.sp)
        stw     r.6,TrGpr6+TF_BASE(r.sp)
        stw     r.7,TrGpr7+TF_BASE(r.sp)
        stw     r.8,TrGpr8+TF_BASE(r.sp)
        stw     r.9,TrGpr9+TF_BASE(r.sp)
        bl      ssd.221                         // load system toc address
ssd.221:                                        //
        mflr    r.2                             //
        lwz     r.2, toc_pointer-ssd.221(r.2)   //
        bl      ..PsConvertToGuiThread          // attempt to convert to GUI thread
        ori     r.11,r.3, 0                     // save completion status

        la      r.0, TF_BASE(r.sp)              // get trap frame address
        stw     r.0, ThTrapFrame(r.13)          // store trap frame address

        lwz     r.2,TrFpr0+TF_BASE(r.sp)        // restore system service number
        lwz     r.3,TrGpr3+TF_BASE(r.sp)        // restore argument registers (except r10)
        lwz     r.4,TrGpr4+TF_BASE(r.sp)
        lwz     r.5,TrGpr5+TF_BASE(r.sp)
        lwz     r.6,TrGpr6+TF_BASE(r.sp)
        lwz     r.7,TrGpr7+TF_BASE(r.sp)
        lwz     r.8,TrGpr8+TF_BASE(r.sp)
        lwz     r.9,TrGpr9+TF_BASE(r.sp)

        lwz     r.12,KiPcr+PcCurrentThread(r.0) // get current thread address
        lwz     r.12,ThServiceTable(r.12)       // get service dispatcher table address
        cmpwi   r.11,0                          // did conversion work?
        beq     SystemServiceRepeat             // if yes, retry

//
// Invalid system service code number found in r.2
//

ssd.inv_service:
        LWI     (r.3, STATUS_INVALID_SYSTEM_SERVICE)
        b       ..KiSystemServiceExit

#if DBG

ssd.badapc:

//
//
// An attempt is being made to exit a system service while kernel APCs are
// disabled, or while attached to another process and the previous mode is
// not kernel.
//
//    r5 - Supplies the APC disable count.
//    r6 - Supplies the APC state index.
//

        mflr    r.0                             // save LR
        bl      ssd.badapc.1                    // get a base address to
ssd.badapc.1:                                   // use to load kernel toc
        mflr    r.2                             // get &ssd.badapc.1
        lwz     r.2, toc_pointer-ssd.badapc.1(r.2) // load toc address
        mtlr    r.0                             // restore LR
        li      r.3, SYSTEM_EXIT_OWNED_MUTEX    // set bug check code
        li      r.4, 0                          // mutex levels have been removed
        bl      ..KeBugCheckEx                  // call bug check routine
        b       $

#endif

//
// stack overflow/underflow/misalign on system call
//
// We need to convert this system call into a code panic trap, saving
// state information such that common_exception_entry's handler can
// deal with it.  We have already trashed a certain amount of info
// but what still exists we will set up in the manner common_exception_
// entry's handler expects.

ssd.stk_err:
        stw     r.2, KiPcr+PCR_SAVE2(r.0)           // save service code
        stw     r.3, KiPcr+PCR_SAVE3(r.0)           // save gprs 3 thru 6
        stw     r.4, KiPcr+PCR_SAVE4(r.0)
        stw     r.5, KiPcr+PCR_SAVE5(r.0)
        stw     r.6, KiPcr+PCR_SAVE6(r.0)
        mfcr    r.5                             // preserved CR
        mflr    r.3                             // fake up srr0
        ori     r.4, r.12, 0                    // srr1
        lis     r.12, 0xdead                    // mark those we already
        stw     r.12, KiPcr+PCR_SAVE11(r.0)     //  lost
        ori     r.13, r.12, 0

        lwz     r.6, KiPcr+PcPanicStack(r.0)    // switch to panic stack
        li      r.2, CODE_PANIC                 // set exception cause to panic
        subi    r.11, r.6, KERNEL_STACK_SIZE    // compute stack limit
        stw     r.6, KiPcr+PcInitialStack(r.0)  // so we don't repeat ourselves
                                                //  ie, avoid overflowing because
                                                //  we went to the panic stack.
        stw     r.11, KiPcr+PcStackLimit(r.0)   // set stack limit
        subi    r.6, r.6, STACK_DELTA_NEWSTK    // allocate stack frame
        b       cee.30                          // process exception

//
// The following code is used to clear the hardware debug registers in
// the event that we have just come from user mode and they are set.
//
// This code is out of line because it is expected to be executed
// infrequently.
//

ssd.dbg_regs:

        li      r.3, 0                          // Initialize DR7
        lwz     r.5, KiPcr+PcPrcb(r.0)          // get processor block address
        lwz     r.4, DR_BASE + SrKernelDr7(r.5) // Kernel DR set?
        rlwinm  r.4, r.4, 0, 0xFF
        cmpwi   cr.7, r.4, 0
        stw     r.3, TrDr7 + TF_BASE(r.sp)      // No DRs set
        stw     r.3, TrDr6 + TF_BASE(r.sp)      // Not a DR breakpoint
        lwz     r.7, DR_BASE + SrKernelDr0(r.5) // Get kernel IABR
        lwz     r.8, DR_BASE + SrKernelDr1(r.5) // Get kernel DABR
        ori     r.7, r.7, 0x3                   // Sanitize IABR (Dr0)
        ori     r.8, r.8, 0x4                   // Sanitize DABR (Dr1)

//
// WARNING: Don't rearrange this branch table. The first branch is overlayed
// with the correct branch instruction (modified) based on the processor
// during system initialization. The correct order is 601, 603, 604/613, 620, skip.
//
BranchDr3:
        b       ssd.dbg_10                      // 601
        b       ssd.dbg_30                      // 603
        b       ssd.dbg_20                      // 604/613
        b       ssd.dbg_40                      // 620
        b       ssd.30                          // unknown - back into mainline

ssd.dbg_10:                                     // 601 SPECIFIC
        li      r.3, 0x0080                     // Normal run mode
        rlwinm  r.7, r.7, 0, 0xfffffffc         // Sanitize IABR (Dr0)
        rlwinm  r.8, r.8, 0, 0xfffffff8         // Sanitize DABR (Dr1)
        bne     cr.7, ssd.dbg_20                // Leave hid1 set for full cmp
        mtspr   hid1, r.3

ssd.dbg_20:                                     // 601/604 SPECIFIC
        mfspr   r.3, iabr                       // Load the IABR (Dr0)
        rlwinm. r.3, r.3, 0, 0xfffffffc         // IABR(DR0) set?
        li      r.4, 0                          // Initialize Dr7
        stw     r.3, TrDr0 + TF_BASE(r.sp)
        mfspr   r.3, dabr                       // Load the DABR (Dr1)
        beq     ssiabr.1                        // jiff Dr0 not set
        li      r.4, 0x1                        // Set LE0 in Dr7

ssiabr.1:
        rlwimi  r.4, r.3, 19, 11, 11            // Interchange R/W1 bits
        rlwimi  r.4, r.3, 21, 10, 10            // and move to Dr7
        rlwinm. r.3, r.3, 0, 0xfffffff8         // Sanitize Dr1
        stw     r.3, TrDr1 + TF_BASE(r.sp)      // Store Dr1 in trap frame
        beq     ssdabr.1                        // jiff Dr1 not set
        ori     r.4, r.4, 0x4                   // Set LE1 in Dr7

ssdabr.1:
        ori     r.4, r.4, 0x100                 // Set LE bit in Dr7
        stw     r.4, TrDr7 + TF_BASE(r.sp)
        li      r.4, 0
        beq     cr.7, sskdr.1                   // jif no kernel DR set
        lwz     r.3, DR_BASE + SrKernelDr7(r.5)
        rlwinm. r.4, r.3, 0, 0x0000000c         // LE1/GE1 set?
        beq     ssdr1.1                         // jiff Dr1 not set
        rlwimi  r.8, r.3, 13, 30, 30            // Interchange R/W1 bits
        rlwimi  r.8, r.3, 11, 31, 31
        mtspr   dabr, r.8

ssdr1.1:
        rlwinm. r.3, r.3, 0, 0x00000003         // LE0/GE0 set?
        beq     ssd.dbg_90
        mtspr   iabr, r.7
        isync
        b       ssd.dbg_90

ssd.dbg_30:                                     // 603 SPECIFIC
        mfspr   r.3, iabr                       // Load the IABR (Dr0)
        rlwinm. r.3, r.3, 0, 0xfffffffc         // Sanitize Dr0
        li      r.4, 0x101                      // Initialize Dr7
        stw     r.3, TrDr0 + TF_BASE(r.sp)
        stw     r.4, TrDr7 + TF_BASE(r.sp)
        li      r.4, 0
        beq     cr.7, sskdr.2                   // jif no kernel DR set
        rlwinm  r.7, r.7, 0, 0xfffffffc         // Sanitize IABR
        ori     r.7, r.7, 0x2
        mtspr   iabr, r.7
        b       ssd.dbg_90

ssd.dbg_40:                                     // 620 SPECIFIC
        mfspr   r.3, dabr                       // Load the DABR (Dr1)
        b       ssiabr.1


sskdr.2:
        mtspr   iabr, r.4
        b       ssd.dbg_90

sskdr.1:
        mtspr   dabr, r.4
        mtspr   iabr, r.4
        isync

ssd.dbg_90:
        lwz     r.8, TrGpr8 + TF_BASE(r.sp)     // reload registers that
        lwz     r.7, TrGpr7 + TF_BASE(r.sp)     //   we clobbered
        lwz     r.5, TrGpr5 + TF_BASE (r.sp)    //   including cr.0
        lwz     r.4, TrGpr4 + TF_BASE (r.sp)    // set cr.0 again
        lwz     r.3, TrGpr3 + TF_BASE (r.sp)
        cmpwi   r.2, 0
        b       ssd.30                          // go back to main line

        DUMMY_EXIT(KiSystemServiceDispatch)

//
// Define the size of a cache block.  This is 32 bytes on all currently
// supported processors, it is 64 bytes on some of the newer processors
// including 620.
//

#define PAGE_SIZE       (1 << PAGE_SHIFT)
#define BLOCK_SZ        32


//++
//
//  VOID
//  KeZeroPage(
//      ULONG PageFrame
//      );
//
//  Routine Description:
//
//    Zero a page of memory using the fastest means possible.
//
//  Arguments:
//
//    PageFrame         Page Number (in physical memory) of the page to
//                      be zeroed.
//
//  Return Value:
//
//    None.
//
// BUGBUG
//
// There are a number of 603 errata related to the dcbz instruction.
// It turns out that using dcbz on 603 class processors is also slow
// (601/604 are considerably faster).  So, rather than applying h/w
// workarounds to the 603 problems, we use a simple loop.
//
//
//--

        .align  5
        LEAF_ENTRY(KeZeroPage)

        mfmsr   r.9                     // get current MSR value
        li      r.6, PAGE_SIZE/BLOCK_SZ // number of blocks to zero
        li      r.7, 0                  // starting offset
        mtctr   r.6                     // set iteration count
        rlwinm  r.8, r.9, 0, 0xffff7fff // disable interrupts
        rlwinm  r.8, r.8, 0, 0xffffffef // disable data translation
        mtmsr   r.8                     // disable ints and data xlate
	cror	0,0,0			// N.B. 603e/ev Errata 15
        slwi    r.3, r.3, PAGE_SHIFT    // chg page number to phys address

//
// If the processor is NOT a member of the 603 family (603, 603e, 603ev)
// the following instruction will have been replaced at init time with an
// ISYNC instruction which is required to stop the processor from looking
// ahead and doing address translation while we're turning it off.
//
// N.B. We use a dbnz because the 603 code does one less iteration (see
// below).
//

kzp.repl:
        bdnz+   KeZeroPage603           // use 603 code unless this inst
                                        // has been replaced with an isync.

zero_block:
        dcbz    r.3, r.7
        addi    r.7, r.7, BLOCK_SZ
        bdnz    zero_block

        mtmsr   r.9                     // restore old interrupt and xlate
                                        // settings
        isync

        ALTERNATE_EXIT(KeZeroPage)

//
// The following code is used for 603 class machines.  The loop is
// unrolled but part of the first and last blocks are done seperately
// in order to init f.1 which is used for the actual code.
//
// N.B. On a 601 or 604, the loop is faster if it is NOT unrolled.

KeZeroPage603:
        isync                           // allow no look ahead

        stw     r.7, 0(r.3)             // zero first 8 bytes and reload
        stw     r.7, 4(r.3)             // into an FP reg for rest of loop.
        li      r.5, 32                 // size of cache block
        lfd     f.1, 0(r.3)

zero_block603:
        dcbtst  r.5, r.3                // touch 32 bytes ahead
        stfd    f.1, 8(r.3)
        stfd    f.1,16(r.3)
        stfd    f.1,24(r.3)
        stfdu   f.1,32(r.3)
        bdnz    zero_block603

//
// Three 8 byte blocks to go.
//

        stfdu   f.1, 8(r.3)
        stfdu   f.1, 8(r.3)
        stfdu   f.1, 8(r.3)

        mtmsr   r.9                     // restore old interrupt and xlate
                                        // settings
        isync

        LEAF_EXIT(KeZeroPage)

//
// The code for this routine really exists at label FlushSingleTb in this module.
//

        LEAF_ENTRY(KiFlushSingleTb)
FlushSingleTbJump:
        b       $+(FlushSingleTb-Kseg0CodeStart)
        LEAF_EXIT(KiFlushSingleTb)

//
// The code for this routine really exists at label FillEntryTb in this module.
//

        LEAF_ENTRY(KeFillEntryTb)
FillEntryTbJump:
        b       $+(FillEntryTb-Kseg0CodeStart)
        LEAF_EXIT(KeFillEntryTb)

//
// The code for this routine really exists at label FillEntryTb in this module.
//

        LEAF_ENTRY(KeFlushCurrentTb)
FlushCurrentTbJump:
        b       $+(FlushCurrentTb-Kseg0CodeStart)
        LEAF_EXIT(KeFlushCurrentTb)
