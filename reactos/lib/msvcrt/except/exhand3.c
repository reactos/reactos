#include <msvcrti.h>


EXCEPTION_DISPOSITION
_except_handler3(
struct _EXCEPTION_RECORD *ExceptionRecord,
void *Frame,
struct _CONTEXT *ContextRecord,
void *DispatcherContext)
{
	printf("_except_handler3()\n");
}
