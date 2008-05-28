#include <precomp.h>

#define NDEBUG
#include <internal/debug.h>

int __STRINGTOLD( long double *value, char **endptr, const char *str, int flags )
{
   DPRINT1("%p %p %s %x stub\n", value, endptr, str, flags );
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
   DPRINT1("__p__amblksiz stub\n");
}

void __fileinfo(void)
{
   DPRINT1("__fileinfo stub\n");
}

void stub(void)
{
   DPRINT1("stub\n");
}

/*********************************************************************
 *              _getmaxstdio (MSVCRT.@)
 */
int CDECL _getmaxstdio(void)
{
    DPRINT1("stub, always returns 512\n");
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
    DPRINT1("stub: setting new maximum for number of simultaneously open files not implemented,returning %d\n",res);
    return res;
}
