#include <windows.h>


int _XcptFilter (
         DWORD ExceptionCode,
         struct _EXCEPTION_POINTERS *  ExceptionInfo 
        )
{
	return printf("Unhandled exception info\n");
//	return UnhandledExceptionFilter(ExceptionInfo);
}