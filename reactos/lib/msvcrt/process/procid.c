#include <msvcrti.h>


int _getpid (void)
{
   return (int)GetCurrentProcessId();
}

