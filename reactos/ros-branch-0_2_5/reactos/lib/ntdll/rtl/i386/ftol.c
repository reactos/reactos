/* $Id: ftol.c,v 1.1 2003/04/23 22:58:02 gvg Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security manager
 * FILE:              lib/ntdll/rtl/i386/ftol.c
 * PROGRAMER:         Ge van Geldorp (ge@gse.nl)
 * REVISION HISTORY:  2003/04/24 Created
 */

/*
 * This routine is called by MSVC-generated code to convert from floating point
 * to integer representation. The floating point number to be converted is
 * on the top of the floating point stack.
 */
long long __cdecl _ftol(void)
{
  unsigned short cw_orig;
  unsigned short cw_round_chop;
  long long ll;

  /* Set "round towards zero" mode */
  __asm__("fstcw %0\n\t" : "=m" (cw_orig));
  __asm__("fwait\n\t");
  cw_round_chop = cw_orig | 0x0c00;
  __asm__("fldcw %0\n\t" : : "m" (cw_round_chop));

  /* Do the actual conversion */
  __asm__("fistpq %0\n\t" : "=m" (ll) );

  /* And restore the rounding mode */
  __asm__("fldcw %0\n\t" : : "m" (cw_orig));

  return ll;
}
