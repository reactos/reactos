#include <windows.h>
#include <crtdll/process.h>
#include <crtdll/errno.h>
#include <crtdll/internal/file.h>


unsigned long
	_beginthread	(void (*pfuncStart)(void *),
			 unsigned unStackSize, void* pArgList)
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
void	_endthread(void)
{
	//fixme ExitThread
	//ExitThread(0);
	for(;;);
}