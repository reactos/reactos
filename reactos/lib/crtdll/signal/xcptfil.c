#include <windows.h>


int _XcptFilter (
         DWORD ExceptionCode,
         struct _EXCEPTION_POINTERS *  ExceptionInfo 
        )
{
	return UnhandledExceptionFilter(ExceptionInfo);
}