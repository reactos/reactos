#include <precomp.h>

static BOOL n_format_enabled = TRUE;

int CDECL _get_printf_count_output( void )
{
    return n_format_enabled ? 1 : 0;
}

int CDECL _set_printf_count_output( int enable )
{
    BOOL old = n_format_enabled;
    n_format_enabled = (enable ? TRUE : FALSE);
    return old ? 1 : 0;
}

int __STRINGTOLD( long double *value, char **endptr, const char *str, int flags )
{
   FIXME("%p %p %s %x stub\n", value, endptr, str, flags );
   return 0;
}

void __fileinfo(void)
{
   FIXME("__fileinfo stub\n");
}

void stub(void)
{
   FIXME("stub\n");
}

unsigned int _get_output_format(void)
{
   return 0;
}

