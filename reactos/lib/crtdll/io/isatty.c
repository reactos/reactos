#include <io.h>

#undef isatty

int isatty( int handle )
{
	return (handle & 3);
}
