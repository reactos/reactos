_ONCE

/*	platf2.h
	portability definitions
	20091216 jcatena
	This shouldn't be necessary if code uses plaf.h, cpu.h and intrin.h macros instead of hardwired compiler specific directives
*/

#if 1

#if defined(_MSC_VER)
#if !defined(__cplusplus) 
#define inline _INLINE
#endif
#endif

#define __inline__ _INLINE
// #define FORCEINLINE _INLINEF
// #define DECLSPEC_NORETURN _NORETURN
// #define FASTCALL _FASTCALL
#define __attribute__(packed)
#define __builtin_expect(x, v) _EXPECT(x, y)

#define Ke386ClearDirectionFlag CpuCld
#define Ke386GetGlobalDescriptorTable CpuGetGdt
#define Ke386SetGlobalDescriptorTable CpuSetGdt
#define Ke386GetLocalDescriptorTable CpuGetLdt
#define Ke386SetLocalDescriptorTable CpuSetLdt
#define Ke386GetTr CpuGetTr
#define Ke386SetTr CpuSetTr
#define Ke386GetCs CpuGetCs
#define Ke386GetSs CpuGetSs
#define Ke386SetSs CpuSetSs
#define Ke386GetDs CpuGetDs
#define Ke386SetDs CpuSetDs
#define Ke386GetEs CpuGetEs
#define Ke386SetEs CpuSetEs
#define Ke386GetFs CpuGetFs
#define Ke386SetFs CpuSetFs
#define Ke386GetGs CpuGetGs
#define Ke386SetGs CpuSetGs
#define Ke386SetCr2 CpuSetCr2
#define Ke386FnInit CpuFninit
#define Ke386FnSave CpuFnsave
#define Ke386FxSave CpuFxsave
#define Ke386FxStore CpuFxrstor
#define Ke386SaveFpuState CpuFsaveu
#define Ke386LoadFpuState CpuFrstoru


// _INTRINSIC(_BitScanForward)
#define BitScanForward _BitScanForward
// _INTRINSIC(_BitScanReverse)
#define BitScanReverse _BitScanReverse

#endif	// #if 1
