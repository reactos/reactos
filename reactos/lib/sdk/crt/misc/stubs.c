#include <precomp.h>

int __STRINGTOLD( long double *value, char **endptr, const char *str, int flags )
{
   FIXME("%p %p %s %x partial stub\n", value, endptr, str, flags );
   *value = strtold(str,endptr);
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

void __fileinfo(void)
{
   FIXME("__fileinfo stub\n");
}

void stub(void)
{
   FIXME("stub\n");
}

/*********************************************************************
 *              _daylight (MSVCRT.@)
 */
int MSVCRT___daylight = 0;

/*********************************************************************
 *              __p_daylight (MSVCRT.@)
 */
int * CDECL MSVCRT___p__daylight(void)
{
    return &MSVCRT___daylight;
}
