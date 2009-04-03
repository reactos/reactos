/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

extern void (*_imp___fpreset)(void) ;
void _fpreset (void)
{ (*_imp___fpreset)(); }

void __attribute__ ((alias ("_fpreset"))) fpreset(void);
