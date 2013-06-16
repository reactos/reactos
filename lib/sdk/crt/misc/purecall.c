
#include <precomp.h>

typedef void (__cdecl *MSVCRT_purecall_handler)(void);

static MSVCRT_purecall_handler purecall_handler = NULL;

/* _set_purecall_handler - not exported in native msvcrt */
MSVCRT_purecall_handler CDECL _set_purecall_handler(MSVCRT_purecall_handler function)
{
    MSVCRT_purecall_handler ret = purecall_handler;

    TRACE("(%p)\n", function);
    purecall_handler = function;
    return ret;
}

/*********************************************************************
 *		_purecall (MSVCRT.@)
 */
void CDECL _purecall(void)
{
  if(purecall_handler)
      purecall_handler();
  _amsg_exit( 25 );
}

