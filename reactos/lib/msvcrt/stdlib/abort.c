#include <crtdll/stdlib.h>
#include <crtdll/stdio.h>
#include <crtdll/io.h>
#include <crtdll/signal.h>

char *msg ="Abort\n\r";

void abort()
{
	fflush(NULL);
	fcloseall();
	raise(SIGABRT);
	_write(stderr->_file, msg, sizeof(msg)-1);
	exit(3);
}

