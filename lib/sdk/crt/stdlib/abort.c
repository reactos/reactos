/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/??????
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>
#include <signal.h>

char *msg ="Abort\n\r";

/*
 * @implemented
 */
void abort()
{
	fflush(NULL);
	_fcloseall();
	raise(SIGABRT);
	_write(stderr->_file, msg, sizeof(msg)-1);
	exit(3);
}

