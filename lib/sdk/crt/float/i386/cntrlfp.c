/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <precomp.h>
#include <float.h>

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

unsigned int CDECL _controlfp(unsigned int newval, unsigned int mask)
{
#ifdef __i386__
  return _control87( newval, mask & ~_EM_DENORMAL );
#else
  FIXME(":Not Implemented!\n");
  return 0;
#endif
}

/*********************************************************************
 *		_control87 (MSVCRT.@)
 */
unsigned int CDECL _control87(unsigned int newval, unsigned int mask)
{
#if defined(__GNUC__) && defined(__i386__)
  unsigned int fpword = 0;
  unsigned int flags = 0;

  TRACE("(%08x, %08x): Called\n", newval, mask);

  /* Get fp control word */
  __asm__ __volatile__( "fstcw %0" : "=m" (fpword) : );

  TRACE("Control word before : %08x\n", fpword);

  /* Convert into mask constants */
  if (fpword & 0x1)  flags |= _EM_INVALID;
  if (fpword & 0x2)  flags |= _EM_DENORMAL;
  if (fpword & 0x4)  flags |= _EM_ZERODIVIDE;
  if (fpword & 0x8)  flags |= _EM_OVERFLOW;
  if (fpword & 0x10) flags |= _EM_UNDERFLOW;
  if (fpword & 0x20) flags |= _EM_INEXACT;
  switch(fpword & 0xC00) {
  case 0xC00: flags |= _RC_UP|_RC_DOWN; break;
  case 0x800: flags |= _RC_UP; break;
  case 0x400: flags |= _RC_DOWN; break;
  }
  switch(fpword & 0x300) {
  case 0x0:   flags |= _PC_24; break;
  case 0x200: flags |= _PC_53; break;
  case 0x300: flags |= _PC_64; break;
  }
  if (fpword & 0x1000) flags |= _IC_AFFINE;

  /* Mask with parameters */
  flags = (flags & ~mask) | (newval & mask);

  /* Convert (masked) value back to fp word */
  fpword = 0;
  if (flags & _EM_INVALID)    fpword |= 0x1;
  if (flags & _EM_DENORMAL)   fpword |= 0x2;
  if (flags & _EM_ZERODIVIDE) fpword |= 0x4;
  if (flags & _EM_OVERFLOW)   fpword |= 0x8;
  if (flags & _EM_UNDERFLOW)  fpword |= 0x10;
  if (flags & _EM_INEXACT)    fpword |= 0x20;
  switch(flags & (_RC_UP | _RC_DOWN)) {
  case _RC_UP|_RC_DOWN: fpword |= 0xC00; break;
  case _RC_UP:          fpword |= 0x800; break;
  case _RC_DOWN:        fpword |= 0x400; break;
  }
  switch (flags & (_PC_24 | _PC_53)) {
  case _PC_64: fpword |= 0x300; break;
  case _PC_53: fpword |= 0x200; break;
  case _PC_24: fpword |= 0x0; break;
  }
  if (flags & _IC_AFFINE) fpword |= 0x1000;

  TRACE("Control word after  : %08x\n", fpword);

  /* Put fp control word */
  __asm__ __volatile__( "fldcw %0" : : "m" (fpword) );

  return flags;
#else
  FIXME(":Not Implemented!\n");
  return 0;
#endif
}
