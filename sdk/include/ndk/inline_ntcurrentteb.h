#ifndef _INLINE_NT_CURRENTTEB_H_
#define _INLINE_NT_CURRENTTEB_H_

#if defined(_M_IX86)
FORCEINLINE struct _TEB * NtCurrentTeb(void)
{
    return (struct _TEB *)__readfsdword(0x18);
}
#elif defined(_M_ARM)
FORCEINLINE struct _TEB * NtCurrentTeb(void)
{
    __debugbreak();
    return (struct _TEB *)0;
}
#elif defined(_M_AMD64)
FORCEINLINE struct _TEB * NtCurrentTeb(void)
{
    return (struct _TEB *)__readgsqword(FIELD_OFFSET(NT_TIB, Self));
}
#elif defined(_M_PPC)
FORCEINLINE struct _TEB * NtCurrentTeb(void)
{
    return (struct _TEB *)__readfsdword_winnt(0x18);
}
#else
#error Unsupported architecture
#endif

#endif//_INLINE_NT_CURRENTTEB_H_
