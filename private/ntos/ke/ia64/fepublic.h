/******************************
Intel Confidential
******************************/

#ifndef _EM_PUBLIC_H
#define _EM_PUBLIC_H

// MACH
// #define IN_KERNEL

// MACH
#define LITTLE_ENDIAN

#ifndef INLINE
#define INLINE
#endif

#if !(defined(BIG_ENDIAN) || defined(LITTLE_ENDIAN))
    #error Endianness not established; define BIG_ENDIAN or LITTLE_ENDIAN
#endif


/************************/
/* System Include Files */
/************************/
 
#include <stdio.h>
#include <memory.h>
#ifndef unix
#include <setjmp.h>
#endif
 
/*********************/
/* Type Declarations */
/*********************/

#include "fetypes.h"
#include "festate.h"
#include "fesupprt.h"
#include "fehelper.h"
#include "feproto.h"

#define U64_lsh(val, bits)  ((bits) > 63 ? 0 : (EM_uint64_t)(val) << (bits))
#define fp_U64_rsh(val, bits)  ((bits) > 63 ? 0 : (EM_uint64_t)(val) >> (bits))
#define LL_SSHR(val, bits)  ((EM_uint64_t)(val) >> ((bits) > 63 ? 63 : (bits)))

#endif
