#ifdef _MSC_VER
#pragma warning ( disable : 4786 ) // identifier was truncated to '255' characters in the debug information
#endif//_MSC_VER

//#include <stdlib.h> // mingw proves it's insanity once again
#include "mingw.h"

MingwBackend::MingwBackend(Project& project)
	: Backend(project)
{
}
