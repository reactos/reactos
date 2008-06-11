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

