/* $Id: thread.c,v 1.7 2003/07/11 17:25:32 royce Exp $
 *
 */
#include <windows.h>
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
