#ifndef __INCLUDE_INTERNAL_HAL_H
#define __INCLUDE_INTERNAL_HAL_H

#ifdef i386
#include <internal/i386/hal.h>
#else
#error "Unknown processor"
#endif

BOOLEAN STDCALL HalInitSystem (ULONG			BootPhase,
			       PLOADER_PARAMETER_BLOCK	LoaderBlock);

#endif
