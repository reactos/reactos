#ifndef _INLINE_NT_CURRENTTEB_H_
#define _INLINE_NT_CURRENTTEB_H_

#ifdef __GNUC__

#if defined(_M_IX86)
FORCEINLINE struct _TEB * NtCurrentTeb(void)
{
    struct _TEB *ret;

    __asm__ __volatile__ (
        "movl %%fs:0x18, %0\n"
        : "=r" (ret)
        : /* no inputs */
    );

    return ret;
}
#elif defined(_M_ARM)

//
// NT-ARM is not documented
//
#include <armddk.h>

#elif defined(_M_AMD64)
FORCEINLINE struct _TEB * NtCurrentTeb(VOID)
{
    return (struct _TEB *)__readgsqword(FIELD_OFFSET(NT_TIB, Self));
}
#elif defined(_M_PPC)
extern __inline__ struct _TEB * NtCurrentTeb(void)
{
    return __readfsdword_winnt(0x18);
}
#else
extern __inline__ struct _TEB * NtCurrentTeb(void)
{
    return __readfsdword_winnt(0x18);
}
#endif

#elif defined(__WATCOMC__)

extern PVOID GetCurrentFiber(void);
#pragma aux GetCurrentFiber = \
        "mov	eax, dword ptr fs:0x10" \
        value [eax] \
        modify [eax];

extern struct _TEB * NtCurrentTeb(void);
#pragma aux NtCurrentTeb = \
        "mov	eax, dword ptr fs:0x18" \
        value [eax] \
        modify [eax];

#elif defined(_MSC_VER)

#if (_MSC_FULL_VER >= 13012035)

__inline PVOID GetCurrentFiber(void) { return (PVOID)(ULONG_PTR)__readfsdword(0x10); }
__inline struct _TEB * NtCurrentTeb(void) { return (struct _TEB *)(ULONG_PTR)__readfsdword(0x18); }

#else

static __inline PVOID GetCurrentFiber(void)
{
    PVOID p;
	__asm mov eax, fs:[10h]
	__asm mov [p], eax
    return p;
}

static __inline struct _TEB * NtCurrentTeb(void)
{
    struct _TEB *p;
	__asm mov eax, fs:[18h]
	__asm mov [p], eax
    return p;
}

#endif /* _MSC_FULL_VER */

#endif /* __GNUC__/__WATCOMC__/_MSC_VER */

#endif//_INLINE_NT_CURRENTTEB_H_
