#include <msvcrt/stdlib.h>
#include <msvcrt/stdio.h>
#include <msvcrt/io.h>
#include <msvcrt/signal.h>

char *msg ="Abort\n\r";

/*
 * @implemented
 */
void abort()
{
	fflush(NULL);
	fcloseall();
	raise(SIGABRT);
	_write(stderr->_file, msg, sizeof(msg)-1);
	exit(3);
}

