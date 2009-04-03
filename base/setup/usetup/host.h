#ifdef __REACTOS__

#include "native/host_native.h"
#define HOST_InitConsole NATIVE_InitConsole
#define HOST_InitMemory NATIVE_InitMemory
#define HOST_CreateFileSystemList NATIVE_CreateFileSystemList

#else

#include "win32/host_win32.h"
#define HOST_InitConsole WIN32_InitConsole
#define HOST_InitMemory WIN32_InitMemory
#define HOST_CreateFileSystemList WIN32_CreateFileSystemList

#endif

BOOLEAN
HOST_InitConsole(
	VOID);

BOOLEAN
HOST_InitMemory(
	VOID);

BOOLEAN
HOST_CreateFileSystemList(
	IN PFILE_SYSTEM_LIST List);

BOOLEAN
HOST_FormatPartition(
	IN PFILE_SYSTEM_ITEM FileSystem,
	IN PCUNICODE_STRING DriveRoot,
	IN PFMIFSCALLBACK Callback);
