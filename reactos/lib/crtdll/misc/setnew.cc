#include <crtdll/stdlib.h>


typedef int (* new_handler_t)( size_t );

new_handler_t new_handler;

new_handler_t _set_new_handler(new_handler_t hnd)
{
        new_handler_t old = new_handler;
	
	new_handler = hnd;
        
        return old;
}

void operator delete(void* m)
{
	if ( m != NULL )
    		free( m );
}

void * operator new( unsigned int s )
{
	if ( s == 0 )
		s = 1;
        return malloc( s );
}
