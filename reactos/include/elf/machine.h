#ifndef _REACTOS_ELF_MACHINE_H_
#define _REACTOS_ELF_MACHINE_H_ 1

#ifdef _M_IX86
#include <elf/elf-i386.h>
#else
#error Unsupported target architecture
#endif

#endif
