_ONCE

// #define CPU_PREFER_NOINTRIN

extern i32u KeI386FxsrPresent;

// not recognized by inline assembler:
// mov eax, cr4
#define CpuMovEaxCr4 _ASM _emit 0x0f _ASM _emit 0x20 _ASM _emit 0xe0
// sysexit
#define CpuSysExit() _ASM _emit 0x0f _ASM _emit 0x35

// enable / disable int flag
#ifndef CPU_PREFER_NOINTRIN
void _enable(void);
__pragma(intrinsic(_enable))
#define CpuIntEnable _enable
void _disable(void);
__pragma(intrinsic(_disable))
#define CpuIntDisable _disable
#else
#define CpuIntEnable(void) __asm sti
#define CpuIntDisable(void) __asm cli
#endif

// set/clear direction flag
#define CpuStd() __asm std
#define CpuCld() __asm cld

// write back and invalidate caches
void __wbinvd(void);
_INTRINSIC(__wbinvd)
#define CpuWbinvd __wbinvd

// get/set Global Descriptor Table
_INLINEF void CpuGetGdt(void *Descriptor) {_ASM_BEGIN
	mov eax, Descriptor
	sgdt [eax]
	_ASM_END}
#define CpuGetGdt_m(x) _ASM sgdt x

_INLINEF void CpuSetGdt(pvoid Descriptor) {_ASM_BEGIN
	mov eax, Descriptor
	lgdt [eax]
	_ASM_END}
#define CpuSetGdt_m(x) _ASM lgdt x

// get/set Interrupt Descriptor Table
#ifndef CPU_PREFER_NOINTRIN
void __sidt(void *x);
_INTRINSIC(__sidt)
#define CpuGetIdt __sidt
void __lidt(void *x);	// msc intrinic
_INTRINSIC(__lidt)
#define CpuSetIdt __lidt
#else
_INLINEF void CpuGetIdt(void *x) {_ASM_BEGIN
	mov eax, x
	sidt [eax]
	_ASM_END}
_INLINEF void CpuSetIdt(void *x) {_ASM_BEGIN
	mov eax, x
	lidt [eax]
	_ASM_END}
#endif
#define CpuGetIdt_m(x) _ASM sidt x
#define CpuSetIdt_m(x) _ASM lidt x

// get/set Local Descriptor Table
_INLINEF i16u CpuGetLdt(void) {_ASM_BEGIN
    sldt ax
	_ASM_END}
#define CpuGetLdt_m(x) _ASM sldt x

_INLINEF void CpuSetLdt(IN i16u x) {_ASM_BEGIN
	lldt x
	_ASM_END}
#define CpuSetLdt_m(x) _ASM lldt x

// get/set Task Register
_INLINEF i16u CpuGetTr(void) {_ASM_BEGIN
    str ax
	_ASM_END}
#define CpuGetTr_m(x) _ASM str x

_INLINEF void CpuSetTr(IN i16u x) {_ASM_BEGIN
	ltr x
	_ASM_END}
#define CpuSetTr_m(x) _ASM ltr x

// get/set crx reg
_INLINEF void CpuGetCr2(i32u x) {_ASM_BEGIN
    mov eax, cr2
	_ASM_END}

_INLINEF void CpuSetCr2(i32u x) {_ASM_BEGIN
    mov eax, x
    mov cr2, eax
	_ASM_END}
#define CpuSetCr2_m(x) _ASM mov cr2, x

// get/set segment registers
_INLINEF i16u CpuGetCs(void) {_ASM_BEGIN
    mov ax, cs
	_ASM_END}
#define CpuGetCs_m(x) _ASM mov x, cs

_INLINEF i16u CpuGetSs(void) {_ASM_BEGIN
    mov ax, ss
	_ASM_END}
#define CpuGetSs_m(x) _ASM mov x, ss

_INLINEF void CpuSetSs(i16u x) {_ASM_BEGIN
	mov ax, x
	mov ss, ax
	_ASM_END}
#define CpuSetSs_m(x) _ASM mov ss, x

_INLINEF i16u CpuGetDs(void) {_ASM_BEGIN
    mov ax, ds
	_ASM_END}
#define CpuGetDs_m(x) _ASM mov x, ds

_INLINEF void CpuSetDs(i16u x) {_ASM_BEGIN
	mov ax, x
	mov ds, ax
	_ASM_END}
#define CpuSetDs_m(x) _ASM mov ds, x

_INLINEF i16u CpuGetEs(void) {_ASM_BEGIN
    mov ax, es
	_ASM_END}
#define CpuGetEs_m(x) _ASM mov x, es

_INLINEF void CpuSetEs(i16u x) {_ASM_BEGIN
	mov ax, x
	mov es, ax
	_ASM_END}
#define CpuSetEs_m(x) _ASM mov es, x

_INLINEF i16u CpuGetFs(void)  {_ASM_BEGIN
    mov ax, fs
	_ASM_END}
#define CpuGetFs_m(x) _ASM mov x, fs

_INLINEF void CpuSetFs(i16u x) {_ASM_BEGIN
	mov ax, x
	mov fs, ax
	_ASM_END}
#define CpuSetFs_m(x) _ASM mov fs, x

_INLINEF i16u CpuGetGs(void)  {_ASM_BEGIN
    mov ax, gs
	_ASM_END}
#define CpuGetGs_m(x) _ASM mov x, gs

_INLINEF void CpuSetGs(i16u x) {_ASM_BEGIN
	mov ax, x
	mov gs, ax
	_ASM_END}
#define CpuSetGs_m(x) _ASM mov gs, x

// get/set registers
_INLINEF void CpuGetEsp(void) {_ASM_BEGIN
	mov eax, esp
	_ASM_END}
#define CpuGetEsp_m(x) _ASM mov x, esp

_INLINEF void CpuSetEsp(i32u x) {_ASM_BEGIN
	mov esp, x
	_ASM_END}
#define CpuSetEsp_m(x) _ASM mov esp, x

_INLINEF void CpuGetEbp(void) {_ASM_BEGIN
	mov eax, ebp
	_ASM_END}
#define CpuGetEbp_m(x) _ASM mov x, ebp

_INLINEF void CpuSetEbp(i32u x) {_ASM_BEGIN
	mov ebp, x
	_ASM_END}
#define CpuSetEbp_m(x) _ASM mov ebp, x

// iret
_INLINEF _NORETURN void CpuIret(void) {_ASM iretd}

// far jump 16:32
_INLINEF _NORETURN void CpuJmpf(i32u seg_, i32u offs) {
	_ASM_BEGIN
	push seg_
	push offs
	retf
	_ASM_END}

// epilogs / special returns
_INLINEF void CpuTrapReturn(IN pvoid Stack) {_ASM_BEGIN
	mov esp, Stack
	popa
	ret
	_ASM_END}

// fpu init
#define CpuFinit() _ASM finit
#define CpuFninit() _ASM fninit

// fpu clear exceptions
#define CpuFclex() _ASM fclex
#define CpuFnclex() _ASM fnclex

// fpu get/store status word (16 bits)
_INLINEF void CpuFGetSw(i16u *x) {_ASM_BEGIN
	mov eax, x
	fstsw [eax]
	_ASM_END}
#define CpuFGetSw_m(x) _ASM fstsw x

_INLINEF void CpuFnGetSw(i16u *x) {_ASM_BEGIN
	mov eax, x
	fnstsw [eax]
	_ASM_END}
#define CpuFnGetSw_m(x) _ASM fnstsw x

// fpu set/load control word
_INLINEF void CpuFSetCw(i16u x) {_ASM_BEGIN
	fldcw x
	_ASM_END}
#define CpuFSetCw_m _ASM fldcw x

// fpu state save/restore
_INLINEF void CpuFsave(void *x) {_ASM_BEGIN
	mov eax, x
	fsave [eax]
	_ASM_END}
#define CpuFsave_m(x) _ASM fsave x

_INLINEF void CpuFnsave(void *x) {_ASM_BEGIN
	mov eax, x
	fnsave [eax]
	_ASM_END}
#define CpuFnsave_m(x) _ASM fnsave x

_INLINEF void CpuFrstor(void *x) {_ASM_BEGIN
	mov eax, x
    frstor [eax]
	_ASM_END}
#define CpuFrstor_m(x) _ASM frstor x

_INLINEF void CpuFxsave(void *x) {_ASM_BEGIN
	mov eax, x
	fxsave [eax]
	_ASM_END}
#define CpuFxsave_m(x) _ASM fxsave x

_INLINEF void CpuFxrstor(void *x) {_ASM_BEGIN
	mov eax, x
	fxrstor [eax]
	_ASM_END}
#define CpuFxrstor(x) _ASM fxrstor x

_INLINEF void CpuFsaveu(void *x) {_ASM_BEGIN
	mov eax, x
	cmp KeI386FxsrPresent, 0
	jnz fx
	fnsave [eax]
	jmp r
fx:	fxsave [eax]
r:	
	_ASM_END}

_INLINEF void CpuFrstoru(void *x) {_ASM_BEGIN
	mov eax, x
	cmp KeI386FxsrPresent, 0
	jnz fx
	frstor [eax]
	jmp r
fx:	fxrstor [eax]
r:	
	_ASM_END}

// fpu vars
#define FPU_DOUBLE(var) \
	double var; \
	_ASM fstp var \
	_ASM fwait
#define FPU_DOUBLES(var1,var2) \
	double var1,var2; \
	_ASM fstp var1 \
	_ASM fwait \
	_ASM fstp var2 \
	_ASM fwait


