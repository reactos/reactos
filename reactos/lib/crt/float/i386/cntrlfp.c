/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <precomp.h>

#define X87_CW_IM   (1<<0)      /* Invalid operation mask */
#define X87_CW_DM   (1<<1)      /* Denormal operand mask */
#define X87_CW_ZM   (1<<2)      /* Zero divide mask */
#define X87_CW_OM   (1<<3)      /* Overflow mask */
#define X87_CW_UM   (1<<4)      /* Underflow mask */
#define X87_CW_PM   (1<<5)      /* Precision mask */

#define X87_CW_PC_MASK     (3<<8)   /* precision control mask */
#define X87_CW_PC24        (0<<8)   /* 24 bit precision */
#define X87_CW_PC53        (2<<8)   /* 53 bit precision */
#define X87_CW_PC64        (3<<8)   /* 64 bit precision */

#define X87_CW_RC_MASK     (3<<10)  /* rounding control mask */
#define X87_CW_RC_NEAREST  (0<<10)  /* round to nearest */
#define X87_CW_RC_DOWN     (1<<10)  /* round down */
#define X87_CW_RC_UP       (2<<10)  /* round up */
#define X87_CW_RC_ZERO     (3<<10)  /* round toward zero (chop) */

#define X87_CW_IC          (1<<12)  /* infinity control flag */

/*
 * @implemented
 */
unsigned int _controlfp(unsigned int unNew, unsigned int unMask)
{
  return _control87(unNew,unMask);
}

/*
 * @implemented
 */
unsigned int _control87(unsigned int unNew, unsigned int unMask)
{
  unsigned int FpuCw;
  unsigned int DummyCw = 0;

  /* get the controlword */
  asm volatile("fstcw %0\n\t" : "=m"(FpuCw));
  FpuCw &= 0x0000ffff;

  /* translate it into _control87 format */
  if (FpuCw & X87_CW_IM)
    DummyCw |= _EM_INVALID;
  if (FpuCw & X87_CW_DM)
    DummyCw |= _EM_DENORMAL;
  if (FpuCw & X87_CW_ZM)
    DummyCw |= _EM_ZERODIVIDE;
  if (FpuCw & X87_CW_OM)
    DummyCw |= _EM_OVERFLOW;
  if (FpuCw & X87_CW_UM)
    DummyCw |= _EM_UNDERFLOW;
  if (FpuCw & X87_CW_PM)
    DummyCw |= _EM_INEXACT;

  switch (FpuCw & X87_CW_PC_MASK)
  {
  case X87_CW_PC24:
    DummyCw |= _PC_24;
    break;
  case X87_CW_PC53:
    DummyCw |= _PC_53;
    break;
  case X87_CW_PC64:
    DummyCw |= _PC_64;
    break;
  }

  switch (FpuCw & X87_CW_RC_MASK)
  {
  case X87_CW_RC_NEAREST:
    DummyCw |= _RC_NEAR;
    break;
  case X87_CW_RC_DOWN:
    DummyCw |= _RC_DOWN;
    break;
  case X87_CW_RC_UP:
    DummyCw |= _RC_UP;
    break;
  case X87_CW_RC_ZERO:
    DummyCw |= _RC_CHOP;
    break;
  }

  /* unset (un)masked bits */
  DummyCw &= ~unMask;
  unNew &= unMask;

  /* set new bits */
  DummyCw |= unNew;

  /* translate back into x87 format
   * FIXME: translate infinity control!
   */
  FpuCw = 0;
  if (DummyCw & _EM_INVALID)
    FpuCw |= X87_CW_IM;
  if (DummyCw & _EM_DENORMAL)
    FpuCw |= X87_CW_DM;
  if (DummyCw & _EM_ZERODIVIDE)
    FpuCw |= X87_CW_ZM;
  if (DummyCw & _EM_OVERFLOW)
    FpuCw |= X87_CW_OM;
  if (DummyCw & _EM_UNDERFLOW)
    FpuCw |= X87_CW_UM;
  if (DummyCw & _EM_INEXACT)
    FpuCw |= X87_CW_PM;

  switch (DummyCw & _MCW_PC)
  {
  case _PC_24:
    FpuCw |= X87_CW_PC24;
    break;
  case _PC_53:
    FpuCw |= X87_CW_PC53;
    break;
  case _PC_64:
  default:
    FpuCw |= X87_CW_PC64;
    break;
  }

  switch (DummyCw & _MCW_RC)
  {
  case _RC_NEAR:
    FpuCw |= X87_CW_RC_NEAREST;
    break;
  case _RC_DOWN:
    FpuCw |= X87_CW_RC_DOWN;
    break;
  case _RC_UP:
    FpuCw |= X87_CW_RC_UP;
    break;
  case _RC_CHOP:
    FpuCw |= X87_CW_RC_ZERO;
    break;
  }

  /* set controlword */
  asm volatile("fldcw %0" : : "m"(FpuCw));

  return DummyCw;

#if 0 /* The follwing is the original code, broken I think! -blight */
register unsigned int __res;
#ifdef __GNUC__
__asm__ __volatile__ (
	"pushl	%%eax \n\t"		/* make room on stack */
	"fstcw	(%%esp) \n\t"
	"fwait \n\t"
	"popl	%%eax \n\t"
	"andl	$0xffff, %%eax	\n\t"   /* OK;  we have the old value ready */

	"movl	%1, %%ecx \n\t"
	"notl	%%ecx \n\t"
	"andl	%%eax, %%ecx \n\t"	/* the bits we want to keep */

	"movl	%2, %%edx \n\t"
	"andl	%1, %%edx \n\t"	/* the bits we want to change */

	"orl	%%ecx, %%edx\n\t"		/* the new value */
	"pushl	%%edx \n\t"
	"fldcw	(%%esp) \n\t"
	"popl	%%edx \n\t"

	:"=a" (__res):"r" (unNew),"r" (unMask): "dx", "cx");
#else
#endif /*__GNUC__*/
	return __res;
#endif
}
