#include <io.h>



int _isatty( int handle )
{
	if ( handle < 5 )
		return 1;
	return 0;
}
