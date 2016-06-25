
#ifdef __i386__
#define __ASM_DEFINE_FUNC(name,suffix,code)

typedef unsigned int (__stdcall *__MSVC__MsiCustomActionEntryPoint)(unsigned int);

static
__declspec(naked)
unsigned int
__cdecl
CUSTOMPROC_wrapper(__MSVC__MsiCustomActionEntryPoint proc, unsigned int handle)
{
    __asm
    {
        push ebp
        mov ebp, esp
        sub esp, 4
        push dword ptr [ebp + 12]
        mov eax, dword ptr [ebp + 8]
        call eax
        leave
        ret
    }
}

#endif
