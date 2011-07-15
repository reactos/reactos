
#ifdef __i386__
#define __ASM_DEFINE_FUNC(name,suffix,code)

typedef unsigned int (__stdcall *__MSVC__MsiCustomActionEntryPoint)(unsigned int);

__inline unsigned int CUSTOMPROC_wrapper(__MSVC__MsiCustomActionEntryPoint proc, unsigned int handle)
{
#pragma message("warning: CUSTOMPROC_wrapper might not be correct")
    return proc(handle);
}

#endif
