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

#ifndef __USE_W32API

#define SEC_BASED       (0x00200000)
#define SEC_NO_CHANGE   (0x00400000)
#define SEC_IMAGE       (0x01000000)
#define SEC_VLM         (0x02000000)
#define SEC_RESERVE     (0x04000000)
#define SEC_COMMIT      (0x08000000)
#define SEC_NOCACHE     (0x10000000)
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

#define FILE_MAP_ALL_ACCESS	(0xf001fL)
#define FILE_MAP_READ	(4)
#define FILE_MAP_WRITE	(2)
#define FILE_MAP_COPY	(1)
#else /* __USE_W32API */

#include <ddk/ntifs.h>

#endif /* __USE_W32API */

#define PAGE_ROUND_UP(x) ( (((ULONG)x)%PAGE_SIZE) ? ((((ULONG)x)&(~(PAGE_SIZE-1)))+PAGE_SIZE) : ((ULONG)x) )
#define PAGE_ROUND_DOWN(x) (((ULONG)x)&(~(PAGE_SIZE-1)))

#endif /* __INCLUDE_MM_H */
