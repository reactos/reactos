/*
    This file was added by Andrew Greenwood. So, if it messes anything up,
    you know who to call...
*/

#ifndef __INCLUDE_NTOS_KEFUNCS_H
#define __INCLUDE_NTOS_KEFUNCS_H

#define KEBUGCHECK(a) DbgPrint("KeBugCheck at %s:%i\n",__FILE__,__LINE__), KeBugCheck(a)
#define KEBUGCHECKEX(a,b,c,d,e) DbgPrint("KeBugCheckEx at %s:%i\n",__FILE__,__LINE__), KeBugCheckEx(a,b,c,d,e)


#ifdef __NTOSKRNL__
extern CHAR EXPORTED KeNumberProcessors;
extern LOADER_PARAMETER_BLOCK EXPORTED KeLoaderBlock;
extern ULONG EXPORTED KeDcacheFlushCount;
extern ULONG EXPORTED KeIcacheFlushCount;
extern KAFFINITY EXPORTED KeActiveProcessors;
extern ULONG EXPORTED KiDmaIoCoherency; /* RISC Architectures only */
extern ULONG EXPORTED KeMaximumIncrement;
extern ULONG EXPORTED KeMinimumIncrement;
#else
extern CHAR IMPORTED KeNumberProcessors;
extern KAFFINITY IMPORTED KeActiveProcessors;
extern LOADER_PARAMETER_BLOCK IMPORTED KeLoaderBlock;
extern ULONG EXPORTED KeDcacheFlushCount;
extern ULONG EXPORTED KeIcacheFlushCount;
extern ULONG IMPORTED KiDmaIoCoherency; /* RISC Architectures only */
extern ULONG IMPORTED KeMaximumIncrement;
extern ULONG IMPORTED KeMinimumIncrement;
#endif


VOID STDCALL KeFlushWriteBuffer (VOID);

#endif /* __INCLUDE_NTOS_KEFUNCS_H */
