/* $Id: thread.c,v 1.1 2000/06/18 10:57:42 ea Exp $
 *
 */
#include <windows.h>
#include <errno.h>

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
