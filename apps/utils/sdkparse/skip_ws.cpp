// skip_ws.cpp

#include <string.h>

#include "skip_ws.h"

const char* ws = " \t\r\n\v";

char* skip_ws ( char* p )
{
	return p + strspn ( p, ws );
}

const char* skip_ws ( const char* p )
{
	return p + strspn ( p, ws );
}
