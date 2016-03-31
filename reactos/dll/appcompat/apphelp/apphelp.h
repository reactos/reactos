/*
 * Copyright 2013 Mislav Blažević
 * Copyright 2015 Mark Jansen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef APPHELP_H
#define APPHELP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _SHIM_LOG_LEVEL {
    SHIM_ERR = 1,
    SHIM_WARN = 2,
    SHIM_INFO = 3,
}SHIM_LOG_LEVEL;

/* apphelp.c */
BOOL WINAPIV ShimDbgPrint(SHIM_LOG_LEVEL Level, PCSTR FunctionName, PCSTR Format, ...);
extern ULONG g_ShimDebugLevel;

#define SHIM_ERR(fmt, ...)  do { if (g_ShimDebugLevel) ShimDbgPrint(SHIM_ERR, __FUNCTION__, fmt, ##__VA_ARGS__ ); } while (0)
#define SHIM_WARN(fmt, ...)  do { if (g_ShimDebugLevel) ShimDbgPrint(SHIM_WARN, __FUNCTION__, fmt, ##__VA_ARGS__ ); } while (0)
#define SHIM_INFO(fmt, ...)  do { if (g_ShimDebugLevel) ShimDbgPrint(SHIM_INFO, __FUNCTION__, fmt, ##__VA_ARGS__ ); } while (0)


/* sdbapi.c */
void SdbpHeapInit(void);
void SdbpHeapDeinit(void);
#if SDBAPI_DEBUG_ALLOC

LPVOID SdbpAlloc(SIZE_T size, int line, const char* file);
LPVOID SdbpReAlloc(LPVOID mem, SIZE_T size, int line, const char* file);
void SdbpFree(LPVOID mem, int line, const char* file);

#define SdbAlloc(size) SdbpAlloc(size, __LINE__, __FILE__)
#define SdbReAlloc(mem, size) SdbpReAlloc(mem, size, __LINE__, __FILE__)
#define SdbFree(mem) SdbpFree(mem, __LINE__, __FILE__)

#else

LPVOID SdbpAlloc(SIZE_T size);
LPVOID SdbpReAlloc(LPVOID mem, SIZE_T size);
void SdbpFree(LPVOID mem);

#define SdbAlloc(size) SdbpAlloc(size)
#define SdbReAlloc(mem, size) SdbpReAlloc(mem, size)
#define SdbFree(mem) SdbpFree(mem)

#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // APPHELP_H
