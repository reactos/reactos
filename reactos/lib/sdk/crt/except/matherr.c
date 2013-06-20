#include <precomp.h>

#define __USE_ISOC9X 1
#define __USE_ISOC99 1
#include <math.h>

#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

#ifndef HAVE_FINITE
#ifndef finite /* Could be a macro */
#ifdef isfinite
#define finite(x) isfinite(x)
#else
#define finite(x) (!isnan(x)) /* At least catch some cases */
#endif
#endif
#endif

#ifndef signbit
#define signbit(x) 0
#endif

typedef int (*MSVCRT_matherr_func)(struct _exception *);

static MSVCRT_matherr_func MSVCRT_default_matherr_func = NULL;

int CDECL _matherr(struct _exception *e)
{
  if (e)
    TRACE("(%p = %d, %s, %g %g %g)\n",e, e->type, e->name, e->arg1, e->arg2,
          e->retval);
  else
    TRACE("(null)\n");
  if (MSVCRT_default_matherr_func)
    return MSVCRT_default_matherr_func(e);
  ERR(":Unhandled math error!\n");
  return 0;
}

/*********************************************************************
 *		__setusermatherr (MSVCRT.@)
 */
void CDECL __setusermatherr(MSVCRT_matherr_func func)
{
  MSVCRT_default_matherr_func = func;
  TRACE(":new matherr handler %p\n", func);
}


#define _FPIEEE_RECORD void

/*
 * @unimplemented
 */
int _fpieee_flt(
        unsigned long exception_code,
        struct _EXCEPTION_POINTERS* ExceptionPointer,
        int (*handler)(_FPIEEE_RECORD*)
        )
{
    FIXME("Unimplemented!\n");
    return 0;
}
