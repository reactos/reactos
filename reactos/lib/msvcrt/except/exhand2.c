#include <msvcrti.h>


EXCEPTION_DISPOSITION
_except_handler2(
struct _EXCEPTION_RECORD *ExceptionRecord,
void *Frame,
struct _CONTEXT *ContextRecord,
void *DispatcherContext)
{
	printf("_except_handler2()\n");
}
