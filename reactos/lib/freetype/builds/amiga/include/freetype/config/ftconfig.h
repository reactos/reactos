// TetiSoft: We must change FT_BASE_DEF and FT_EXPORT_DEF

//#define FT_BASE_DEF( x )  extern  x   // SAS/C wouldn't generate an XDEF
//#define FT_EXPORT_DEF( x )  extern  x // SAS/C wouldn't generate an XDEF
#undef FT_BASE_DEF
#define FT_BASE_DEF( x )  x
#undef FT_EXPORT_DEF
#define FT_EXPORT_DEF( x )  x

// TetiSoft: now include original file
#ifndef __MORPHOS__
#include "FT:include/freetype/config/ftconfig.h"
#else
// We must define that, it seems that
// lib/gcc-lib/ppc-morphos/2.95.3/include/syslimits.h is missing in 
// ppc-morphos-gcc-2.95.3-bin.tgz (gcc for 68k producing MorphOS PPC elf
// binaries from http://www.morphos.de)
#define _LIBC_LIMITS_H_
#include "/FT/include/freetype/config/ftconfig.h"
#endif
