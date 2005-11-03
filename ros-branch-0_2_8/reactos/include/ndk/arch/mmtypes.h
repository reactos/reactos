/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/arch/mmtypes.h
 * PURPOSE:         Architecture-specific definitions for Memory Manager Types
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _ARCH_MMTYPES_H
#define _ARCH_MMTYPES_H

#ifdef _M_IX86
#include <ndk/i386/mmtypes.h>
#else
#error "Unknown processor"
#endif

#endif
