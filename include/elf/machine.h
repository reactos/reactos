#ifndef _REACTOS_ELF_MACHINE_H_
#define _REACTOS_ELF_MACHINE_H_ 1

#ifdef _M_IX86
#define _REACTOS_ELF_MACHINE_IS_TARGET
#include <elf/elf-i386.h>
#undef _REACTOS_ELF_MACHINE_IS_TARGET
#else
#error Unsupported target architecture
#endif

#endif
