/*
 * PROJECT:     ReactOS Application compatibility module
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Shim engine private functions
 * COPYRIGHT:   Copyright 2013 Mislav Blažević
 *              Copyright 2015-2017 Mark Jansen (mark.jansen@reactos.org)
 */

#ifndef SDBPAPI_H
#define SDBPAPI_H

#ifdef __cplusplus
extern "C" {
#endif

void SdbpHeapInit(void);
void SdbpHeapDeinit(void);

#if SDBAPI_DEBUG_ALLOC

LPVOID SdbpAlloc(SIZE_T size, int line, const char* file);
LPVOID SdbpReAlloc(LPVOID mem, SIZE_T size, SIZE_T oldSize, int line, const char* file);
void SdbpFree(LPVOID mem, int line, const char* file);

#define SdbAlloc(size) SdbpAlloc(size, __LINE__, __FILE__)
#define SdbReAlloc(mem, size, oldSize) SdbpReAlloc(mem, size, oldSize, __LINE__, __FILE__)
#define SdbFree(mem) SdbpFree(mem, __LINE__, __FILE__)

#else

LPVOID SdbpAlloc(SIZE_T size);
LPVOID SdbpReAlloc(LPVOID mem, SIZE_T size, SIZE_T oldSize);
void SdbpFree(LPVOID mem);

#define SdbAlloc(size) SdbpAlloc(size)
#define SdbReAlloc(mem, size, oldSize) SdbpReAlloc(mem, size, oldSize)
#define SdbFree(mem) SdbpFree(mem)

#endif

#if !defined(SDBWRITE_HOSTTOOL)
typedef struct tagMEMMAPPED {
    HANDLE file;
    HANDLE section;
    PBYTE view;
    SIZE_T size;
    SIZE_T mapped_size;
} MEMMAPPED, *PMEMMAPPED;

BOOL WINAPI SdbpOpenMemMappedFile(LPCWSTR path, PMEMMAPPED mapping);
void WINAPI SdbpCloseMemMappedFile(PMEMMAPPED mapping);
#endif


PDB WINAPI SdbpCreate(LPCWSTR path, PATH_TYPE type, BOOL write);
void WINAPI SdbpFlush(PDB pdb);
DWORD SdbpStrlen(PCWSTR string);
DWORD SdbpStrsize(PCWSTR string);

BOOL WINAPI SdbpCheckTagType(TAG tag, WORD type);
BOOL WINAPI SdbpCheckTagIDType(PDB pdb, TAGID tagid, WORD type);

#ifndef WINAPIV
#define WINAPIV
#endif

typedef enum _SHIM_LOG_LEVEL {
    SHIM_ERR = 1,
    SHIM_WARN = 2,
    SHIM_INFO = 3,
} SHIM_LOG_LEVEL;

BOOL WINAPIV ShimDbgPrint(SHIM_LOG_LEVEL Level, PCSTR FunctionName, PCSTR Format, ...);
extern ULONG g_ShimDebugLevel;

#define SHIM_ERR(fmt, ...)  do { if (g_ShimDebugLevel) ShimDbgPrint(SHIM_ERR, __FUNCTION__, fmt, ##__VA_ARGS__ ); } while (0)
#define SHIM_WARN(fmt, ...)  do { if (g_ShimDebugLevel) ShimDbgPrint(SHIM_WARN, __FUNCTION__, fmt, ##__VA_ARGS__ ); } while (0)
#define SHIM_INFO(fmt, ...)  do { if (g_ShimDebugLevel) ShimDbgPrint(SHIM_INFO, __FUNCTION__, fmt, ##__VA_ARGS__ ); } while (0)

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SDBPAPI_H
