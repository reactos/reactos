/*
 * Copyright (c) 2001
 *	Politecnico di Torino.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the Politecnico
 * di Torino, and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef __memory_t
#define __memory_t

#define		uint8	UCHAR
#define		int8	CHAR
#define		uint16	USHORT
#define		int16	SHORT
#define		uint32	ULONG
#define		int32	LONG
#define		uint64	ULONGLONG
#define		int64	LONGLONG

/*memory type*/
typedef struct __MEM_TYPE
{
	uint8 *buffer;
	uint32 size;
}  MEM_TYPE, *PMEM_TYPE;

#define LONG_AT(base,offset) (*(int32*)((uint8*)base+(uint32)offset))

#define ULONG_AT(base,offset) (*(uint32*)((uint8*)base+(uint32)offset))

#define SHORT_AT(base,offset) (*(int16*)((uint8*)base+(uint32)offset))

#define USHORT_AT(base,offset) (*(uint16*)((uint8*)base+(uint32)offset))

#ifdef __GNUC__

int32 SW_LONG_AT(void *b, uint32 c);
uint32 SW_ULONG_AT(void *b, uint32 c);
int16 SW_SHORT_AT(void *b, uint32 os);
uint16 SW_USHORT_AT(void *b, uint32 os);
VOID SW_ULONG_ASSIGN(void *dst, uint32 src);

#else /* __GNUC__ */

__inline int32 SW_LONG_AT(void *b, uint32 c)
{
	return	((int32)*((uint8 *)b+c)<<24|
		 (int32)*((uint8 *)b+c+1)<<16|
		 (int32)*((uint8 *)b+c+2)<<8|
		 (int32)*((uint8 *)b+c+3)<<0);
}

__inline uint32 SW_ULONG_AT(void *b, uint32 c)
{
	return	((uint32)*((uint8 *)b+c)<<24|
		 (uint32)*((uint8 *)b+c+1)<<16|
		 (uint32)*((uint8 *)b+c+2)<<8|
		 (uint32)*((uint8 *)b+c+3)<<0);
}

__inline int16 SW_SHORT_AT(void *b, uint32 os)
{
	return ((int16)
		((int16)*((uint8 *)b+os+0)<<8|
		 (int16)*((uint8 *)b+os+1)<<0));
}

__inline uint16 SW_USHORT_AT(void *b, uint32 os)
{
	return ((uint16)
		((uint16)*((uint8 *)b+os+0)<<8|
		 (uint16)*((uint8 *)b+os+1)<<0));
}

__inline VOID SW_ULONG_ASSIGN(void *dst, uint32 src)
{
	*((uint8*)dst+0)=*((uint8*)&src+3);
	*((uint8*)dst+1)=*((uint8*)&src+2);
	*((uint8*)dst+2)=*((uint8*)&src+1);
	*((uint8*)dst+3)=*((uint8*)&src+0);

}

#endif /* __GNUC__ */

#ifdef WIN_NT_DRIVER

#define ALLOCATE_MEMORY(dest,type,amount) \
	  (dest)=ExAllocatePool(NonPagedPool,sizeof(type)*(amount));
#define ALLOCATE_ZERO_MEMORY(dest,type,amount) \
	{ \
		(dest)=ExAllocatePool(NonPagedPool,sizeof(type)*(amount)); \
		if ((dest)!=NULL) \
			RtlZeroMemory((dest),sizeof(type)*(amount)); \
	}	

#define FREE_MEMORY(dest) ExFreePool(dest);
#define ZERO_MEMORY(dest,amount) RtlZeroMemory(dest,amount);
#define COPY_MEMORY(dest,src,amount) RtlCopyMemory(dest,src,amount);

#endif /*WIN_NT_DRIVER*/


#endif 

