/* $Id: thread.c,v 1.8 2004/08/15 17:34:27 chorns Exp $
 *
 */

#include "precomp.h"
#include <msvcrt/errno.h>
#include <msvcrt/process.h>
#include <msvcrt/internal/file.h>


/*
 * @implemented
 */
unsigned long _beginthread(
    void (*pfuncStart)(void*),
	unsigned unStackSize,
    void* pArgList)
{
	DWORD  ThreadId;
 	HANDLE hThread;
	if (  pfuncStart == NULL )
		__set_errno(EINVAL);

	hThread = CreateThread( NULL,unStackSize,(LPTHREAD_START_ROUTINE)pfuncStart,pArgList,0, &ThreadId);
	if (hThread == NULL ) {
		__set_errno(EAGAIN);
		return -1;
	}
	return (unsigned long)hThread;
}

/*
 * @unimplemented
 */
void	_endthread(void)
{
	//fixme ExitThread
	//ExitThread(0);
	for(;;);
}
