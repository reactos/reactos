/*
 * Copyright 2013 Mislav Blažević
 * Copyright 2015,2016 Mark Jansen
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

#ifndef SDBPAPI_H
#define SDBPAPI_H

#ifdef __cplusplus
extern "C" {
#endif

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
void WINAPI SdbpFlush(PDB db);
DWORD SdbpStrlen(LPCWSTR string);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // SDBPAPI_H
