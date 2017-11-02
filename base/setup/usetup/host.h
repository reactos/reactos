#ifdef __REACTOS__

#include "native/host_native.h"
#define HOST_InitConsole NATIVE_InitConsole
#define HOST_InitMemory NATIVE_InitMemory

#else

#include "win32/host_win32.h"
#define HOST_InitConsole WIN32_InitConsole
#define HOST_InitMemory WIN32_InitMemory

#endif

BOOLEAN
HOST_InitConsole(VOID);

BOOLEAN
HOST_InitMemory(VOID);
