#include "host_native.h"

BOOLEAN
NATIVE_InitConsole(
	VOID)
{
	return (BOOLEAN)AllocConsole();
}
