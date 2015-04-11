

#ifdef _MSC_VER

    /* Globals */
    GBLS    AreaName
    GBLS    FuncName
    GBLS    PrologName
    GBLS    FuncEndName
AreaName    SETS    "|.text|"

    MACRO
    TEXTAREA
    AREA |.text|,ALIGN=2,CODE,READONLY
AreaName    SETS    "|.text|"
    MEND

    MACRO
    NESTED_ENTRY    $Name
FuncName    SETS    "$Name"
PrologName  SETS    "$Name":CC:"_Prolog"
FuncEndName SETS    "$Name":CC:"_end"
    //AREA |.pdata|,ALIGN=2,PDATA
    ALIGN 2
    EXPORT $FuncName [FUNC]
$FuncName
    ROUT
    MEND

    MACRO
    PROLOG_END
$PrologName
    MEND

    MACRO
    ENTRY_END $Name
$FuncEndName
    MEND

	MACRO
	LEAF_ENTRY $Name
FuncName    SETS    "$Name"
PrologName  SETS	"Invalid Prolog"
FuncEndName SETS    "$Name":CC:"_end"
	ALIGN 2
	EXPORT $FuncName [FUNC]
$FuncName
	ROUT
	MEND

    MACRO
    LEAF_END $Name
$FuncEndName
    MEND

    MACRO
    TRAP_PROLOG $Abort
        fixme
    MEND

    MACRO
    SYSCALL_PROLOG $Abort
        fixme
    MEND

    MACRO
    TRAP_EPILOG $SystemCall
        fixme
    MEND

    #define CR 13
    #define LF 10
    #define NUL 0

    #define ASCII dcb

    MACRO
    UNIMPLEMENTED $Name
    MEND

#else

/* Compatibility define */
#define EQU .equ

.macro IMPORT Name
    /* Ignore */
.endm

.macro TEXTAREA
    .section .text, "rx"
    .align 2
.endm

.macro NESTED_ENTRY Name
FuncName    .equ    &Name
PrologName  .equ    &Name&_Prolog
FuncEndName .equ    &Name&_end
    .global &FuncName
    .align 2
    .func &FuncName
    &FuncName:
.endm

.macro PROLOG_END
    \PrologName:
.endm

.macro ENTRY_END Name
    &FuncEndName:
   .endfunc
.endm

.macro TRAP_PROLOG Abort
    //
    // Fixup lr
    //
.if \Abort
    sub lr, lr, #8
.else
    sub lr, lr, #4
.endif

    // Save the bottom 4 registers
    stmdb sp, {r0-r3}

    // Save the abort lr, sp, spsr, cpsr
    mov r0, lr
    mov r1, sp
    mrs r2, cpsr
    mrs r3, spsr

    // Switch to SVC mode
    bic r2, r2, #CPSR_MODES
    orr r2, r2, #CPSR_SVC_MODE
    msr cpsr_c, r2

    // Save the SVC sp before we modify it
    mov r2, sp

    // Make space for the trap frame
    sub sp, sp, #TrapFrameLength

    // Save abt32 state
    str r0, [sp, #TrPc]
    str lr, [sp, #TrSvcLr]
    str r2, [sp, #TrSvcSp]

    // Restore the saved SPSR
    msr spsr_all, r3

    // Restore our 4 registers
    ldmdb r1, {r0-r3}

    // Build trap frame
    // FIXME: Change to stmdb later
    str r0, [sp, #TrR0]
    str r1, [sp, #TrR1]
    str r2, [sp, #TrR2]
    str r3, [sp, #TrR3]
    str r4, [sp, #TrR4]
    str r5, [sp, #TrR5]
    str r6, [sp, #TrR6]
    str r7, [sp, #TrR7]
    str r8, [sp, #TrR8]
    str r9, [sp, #TrR9]
    str r10, [sp, #TrR10]
    str r11, [sp, #TrR11]
    str r12, [sp, #TrR12]
    mov r12, sp
    add r12, r12, #TrUserSp
    stm r12, {sp, lr}^
    mrs r0, spsr_all
    str r0, [sp, #TrSpsr]
    ldr r0, =0xBADB0D00
    str r0, [sp, #TrDbgArgMark]
.endm

.macro SYSCALL_PROLOG
    // Make space for the trap frame
    sub sp, sp, #TrapFrameLength

    // Build trap frame
    // FIXME: Change to stmdb later
    str r0, [sp, #TrR0]
    str r1, [sp, #TrR1]
    str r2, [sp, #TrR2]
    str r3, [sp, #TrR3]
    str r4, [sp, #TrR4]
    str r5, [sp, #TrR5]
    str r6, [sp, #TrR6]
    str r7, [sp, #TrR7]
    str r8, [sp, #TrR8]
    str r9, [sp, #TrR9]
    str r10, [sp, #TrR10]
    str r11, [sp, #TrR11]
    str r12, [sp, #TrR12]
    mov r12, sp
    add r12, r12, #TrUserSp
    stm r12, {sp, lr}^
    str sp, [sp, #TrSvcSp]
    str lr, [sp, #TrPc]
    mrs r0, spsr_all
    str r0, [sp, #TrSpsr]
    ldr r0, =0xBADB0D00
    str r0, [sp, #TrDbgArgMark]
.endm

.macro TRAP_EPILOG SystemCall
    // ASSERT(TrapFrame->DbgArgMark == 0xBADB0D00)
    ldr r0, [sp, #TrDbgArgMark]
    ldr r1, =0xBADB0D00
    cmp r0, r1
    bne 1f

    // Get the SPSR and restore it
    ldr r0, [sp, #TrSpsr]
    msr spsr_all, r0

    // Restore the registers
    // FIXME: Use LDMIA later
    mov r0, sp
    add r0, r0, #TrUserSp
    ldm r0, {sp, lr}^
    ldr r0, [sp, #TrR0]
    ldr r1, [sp, #TrR1]
    ldr r2, [sp, #TrR2]
    ldr r3, [sp, #TrR3]
    ldr r4, [sp, #TrR4]
    ldr r5, [sp, #TrR5]
    ldr r6, [sp, #TrR6]
    ldr r7, [sp, #TrR7]
    ldr r8, [sp, #TrR8]
    ldr r9, [sp, #TrR9]
    ldr r10, [sp, #TrR10]
    ldr r11, [sp, #TrR11]
    ldr r12, [sp, #TrR12]

    // Restore program execution state
.if \SystemCall
    ldr lr, [sp, #TrPc]
    add sp, sp, #TrapFrameLength
    movs pc, lr
.else
    add sp, sp, #TrSvcSp
    ldmia sp, {sp, lr, pc}^
.endif
1:
    b .
.endm

#endif


