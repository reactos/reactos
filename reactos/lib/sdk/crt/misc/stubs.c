#include <precomp.h>

int __STRINGTOLD( long double *value, char **endptr, const char *str, int flags )
{
   FIXME("%p %p %s %x stub\n", value, endptr, str, flags );
   return 0;
}

/*********************************************************************
 *		$I10_OUTPUT (MSVCRT.@)
 * Function not really understood but needed to make the DLL work
 */
void MSVCRT_I10_OUTPUT(void)
{
  /* FIXME: This is probably data, not a function */
}

void __p__amblksiz(void)
{
   FIXME("__p__amblksiz stub\n");
}

void __fileinfo(void)
{
   FIXME("__fileinfo stub\n");
}

void stub(void)
{
   FIXME("stub\n");
}

/*********************************************************************
 *              _getmaxstdio (MSVCRT.@)
 */
int CDECL _getmaxstdio(void)
{
    FIXME("stub, always returns 512\n");
    return 512;
}

/*********************************************************************
*              _setmaxstdio_ (MSVCRT.@)
*/
int CDECL _setmaxstdio(int newmax)
{
    int res;
    if( newmax > 2048)
        res = -1;
    else
        res = newmax;
    FIXME("stub: setting new maximum for number of simultaneously open files not implemented,returning %d\n",res);
    return res;
}
