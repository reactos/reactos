#ifndef __NTOSKRNL_INCLUDE_INTERNAL_ARCH_MM_H
#define __NTOSKRNL_INCLUDE_INTERNAL_ARCH_MM_H

#ifdef i386
#include <internal/i386/mm.h>
#else
#error "Unknown processor"
#endif

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_ARCH_MM_H */
