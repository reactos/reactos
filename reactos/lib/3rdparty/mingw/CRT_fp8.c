/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

#include <_mingw.h>

extern void (* __MINGW_IMP_SYMBOL(_fpreset))(void);
void _fpreset (void);

void _fpreset (void)
{
  (* __MINGW_IMP_SYMBOL(_fpreset))();
}

void __attribute__ ((alias ("_fpreset"))) fpreset(void);
