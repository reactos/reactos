#include <windows.h>
#include <excpt.h>


EXCEPTION_DISPOSITION
_except_handler2(
struct _EXCEPTION_RECORD *ExceptionRecord,
void *Frame,
struct _CONTEXT *ContextRecord,
void *DispatcherContext)
{
	printf("exception handler\n");
}
