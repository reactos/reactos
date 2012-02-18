#include <precomp.h>

typedef void (CDECL *_INITTERMFUN)(void);
typedef int (CDECL *_INITTERM_E_FN)(void);


/*********************************************************************
 *		_initterm (MSVCRT.@)
 */
void CDECL _initterm(_INITTERMFUN *start,_INITTERMFUN *end)
{
  _INITTERMFUN* current = start;

  TRACE("(%p,%p)\n",start,end);
  while (current<end)
  {
    if (*current)
    {
      TRACE("Call init function %p\n",*current);
      (**current)();
      TRACE("returned\n");
    }
    current++;
  }
}

/*********************************************************************
 *  _initterm_e (MSVCRT.@)
 *
 * call an array of application initialization functions and report the return value
 */
int CDECL _initterm_e(_INITTERM_E_FN *table, _INITTERM_E_FN *end)
{
    int res = 0;

    TRACE("(%p, %p)\n", table, end);

    while (!res && table < end) {
        if (*table) {
            TRACE("calling %p\n", **table);
            res = (**table)();
            if (res)
                TRACE("function %p failed: 0x%x\n", *table, res);

        }
        table++;
    }
    return res;
}
