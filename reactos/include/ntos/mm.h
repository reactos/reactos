/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/mm.h
 * PURPOSE:      Memory managment declarations used by all the parts of the 
 *               system
 * PROGRAMMER:   David Welch <welch@cwcom.net>
 * UPDATE HISTORY: 
 *               27/06/00: Created
 */


#ifndef __INCLUDE_MM_H
#define __INCLUDE_MM_H

#define SEC_COMMIT	(134217728)
#define SEC_IMAGE	(16777216)
#define SEC_NOCACHE	(268435456)
#define SEC_RESERVE	(67108864)
#define PAGE_READONLY	(2)
#define PAGE_READWRITE	(4)
#define PAGE_WRITECOPY	(8)
#define PAGE_EXECUTE	(16)
#define PAGE_EXECUTE_READ	(32)
#define PAGE_EXECUTE_READWRITE	(64)
#define PAGE_EXECUTE_WRITECOPY	(128)
#define PAGE_GUARD	(256)
#define PAGE_NOACCESS	(1)
#define PAGE_NOCACHE	(512)
#define MEM_COMMIT	(4096)
#define MEM_FREE	(65536)
#define MEM_RESERVE	(8192)
#define MEM_IMAGE	(16777216)
#define MEM_MAPPED	(262144)
#define MEM_PRIVATE	(131072)
#define MEM_DECOMMIT	(16384)
#define MEM_RELEASE	(32768)
#define MEM_TOP_DOWN	(1048576)
#define EXCEPTION_GUARD_PAGE	(0x80000001L)
#define SECTION_EXTEND_SIZE	(0x10)
#define SECTION_MAP_EXECUTE	(0x8)
#define SECTION_MAP_READ	(0x4)
#define SECTION_MAP_WRITE	(0x2)
#define SECTION_QUERY		(0x1)
#define SECTION_ALL_ACCESS	(0xf001fL)

typedef struct _MEMORY_BASIC_INFORMATION {
  PVOID BaseAddress;
  PVOID AllocationBase;
  DWORD AllocationProtect;
  DWORD RegionSize;
  DWORD State;
  DWORD Protect;
  DWORD Type;
} MEMORY_BASIC_INFORMATION, *PMEMORY_BASIC_INFORMATION;

#define FILE_MAP_ALL_ACCESS	(0xf001fL)
#define FILE_MAP_READ	(4)
#define FILE_MAP_WRITE	(2)
#define FILE_MAP_COPY	(1)


#endif /* __INCLUDE_MM_H */
