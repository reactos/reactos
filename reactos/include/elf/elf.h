#ifndef _REACTOS_ELF_H_
#define _REACTOS_ELF_H_ 1

/* Machine-independent and word-size-independent definitions */
#include <elf/common.h>

/*
 * Word-size-dependent definitions. All ReactOS builds support all of them,
 * even if (obviously) code for the wrong architecture cannot be executed - the
 * files can still be used in machine-independent ways, e.g. as resource DLLs
 */
#include <elf/elf32.h>
#include <elf/elf64.h>

/* Machine-dependent definitions */
#include <elf/machine.h>

#endif

/* EOF */
