// Dummy C++ file
// FIXME: to be deleted before merge

#define NONAMELESSUNION // C++ forbids nameless struct/union

#include <win32k.h>

#define NDEBUG
#include <debug.h>

void dummy(void)
{
    PsGetCurrentThreadWin32Thread();
}
