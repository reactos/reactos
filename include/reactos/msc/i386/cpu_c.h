#pragma once 

// not recognized by inline assembler:
// mov eax, cr4
#define mov_eax_cr4 _ASM _emit 0x0f _ASM _emit 0x20 _ASM _emit 0xe0
// sysexit
#define sysexit _ASM _emit 0x0f _ASM _emit 0x35

#define CpuCld() __asm cld
#define CpuStd() __asm std

void __wbinvd(void);
_INTRINSIC(__wbinvd, __wbinvd)
#define CpuWbinvd __wbinvd

#define CpuGetGdt_m(x) _ASM sgdt x
_INLINEF void CpuGetGdt(pvoid Descriptor) {_ASM_BEGIN
	mov eax, Descriptor
	sgdt [eax]
_ASM_END}

#define CpuSetGdt_m(x) _ASM lgdt x
_INLINEF void CpuSetGdt(pvoid Descriptor) {_ASM_BEGIN
	mov eax, Descriptor
	lgdt [eax]
_ASM_END}

#if 1	// select implementation
void __sidt(void *x);		// msc intrinsic
_INTRINSIC(__sidt, __sidt)
#define CpuGetIdt __sidt
#else
_INLINEF void CpuGetIdt(void *x) {_ASM_BEGIN
	mov eax, x
	sidt [eax]
_ASM_END}
#endif
#define CpuGetIdt_m(x) _ASM sidt x

#if 1	// select implementation
void __lidt(void *x);	// msc intrinic
_INTRINSIC(__lidt, __lidt)
#define CpuSetIdt __lidt
#else
_INLINEF void CpuSetIdt(void *x) {_ASM_BEGIN
	mov eax, x
	lidt [eax]
_ASM_END}
#endif
#define CpuSetIdt_m(x) _ASM lidt x

_INLINEF i16u CpuGetLdt(void) {_ASM_BEGIN
    sldt ax
_ASM_END}

_INLINEF void CpuSetLdt(IN i16u x) {_ASM_BEGIN
	lldt x
_ASM_END}

_INLINEF i16u CpuGetTr(void) {_ASM_BEGIN
    str ax
_ASM_END}

_INLINEF void CpuSetTr(IN i16u x) {_ASM_BEGIN
	ltr x
_ASM_END}

_INLINEF void CpuSetCr2(i32u x) {_ASM_BEGIN
    mov eax, x
    mov cr2, eax
_ASM_END}

_INLINEF i16u CpuGetSs(void) {_ASM_BEGIN
    mov ax, ss
_ASM_END}

_INLINEF void CpuSetSs(i16u x) {_ASM_BEGIN
	mov ax, x
	mov ss, ax
_ASM_END}

_INLINEF i16u CpuGetFs(void)  {_ASM_BEGIN
    mov ax, fs
_ASM_END}

_INLINEF void CpuSetFs(i16u x) {_ASM_BEGIN
	mov ax, x
	mov fs, ax
_ASM_END}

_INLINEF i16u CpuGetDs(void) {_ASM_BEGIN
	mov ax, ds
_ASM_END}

_INLINEF void CpuSetDs(i16u x) {_ASM_BEGIN
	mov ax, x
	mov ds, ax
_ASM_END}

_INLINEF i16u CpuGetEs(void) {_ASM_BEGIN
	mov ax, es;
_ASM_END}

_INLINEF void CpuSetEs(i16u x) {_ASM_BEGIN
	mov ax, x
	mov es, ax
_ASM_END}

_INLINEF i16u CpuGetGs(void) {_ASM_BEGIN
	mov ax, gs
_ASM_END}

_INLINEF void CpuSetGs(i16u x) {_ASM_BEGIN
	mov ax, x
    mov gs, ax
_ASM_END}

_INLINEF void CpuGetEbp(void) {_ASM_BEGIN
	mov eax, ebp
_ASM_END}
#define CpuGetEbp_m(x) _ASM mov x, ebp

_INLINEF void CpuSetEbp(i32 x) {_ASM_BEGIN
	mov ebp, x
_ASM_END}

#define CpuIret() _ASM iret

_INLINEF void CpuTrapReturn(IN iptru Stack) {_ASM_BEGIN
	mov esp, Stack
	popa
	ret
_ASM_END}

// FPU 
#define CpuFinit() _ASM finit
#define CpuFninit() _ASM fninit
#define CpuFnclex() _ASM fnclex
_INLINEF void CpuFstcw(void *x) {_ASM_BEGIN
	mov eax, x
	fstcw [eax]
_ASM_END}
#define CpuFstcw_m(x) _ASM fstcw x
_INLINEF void CpuFSetCw(i32 x) {_ASM_BEGIN
	fldcw x
_ASM_END}
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

