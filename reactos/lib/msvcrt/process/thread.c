/* $Id: thread.c,v 1.4 2002/09/08 10:22:54 chorns Exp $
 *
 */
#include <windows.h>
#include <msvcrt/errno.h>
#include <msvcrt/process.h>

unsigned long
_beginthread (
	void ( __cdecl	* start_address ) (void *),
	unsigned	stack_size,
	void		* arglist
	)
{
	errno = ENOSYS;
	return (unsigned long) -1;
}


unsigned long
_beginthreadex (
	void			* security,
	unsigned		stack_size,
	unsigned ( __stdcall	* start_address ) (void *),
	void			* arglist,
	unsigned		initflag,
	unsigned		* thrdaddr
	)
{
	errno = ENOSYS;
	return (unsigned long) -1;
}


void _endthread (void)
{
}


void _endthreadex (unsigned retval)
{
}


/* EOF */
