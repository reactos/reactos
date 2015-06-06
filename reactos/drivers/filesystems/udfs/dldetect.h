////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////
/*

Module Name:

    DLDetect.h

Abstract:

    This file contains all defines and protos related to DeadLock Detector.

Environment:

    NT Kernel Mode
*/

#ifndef _DL_DETECT_H_
#define _DL_DETECT_H_


#define DLDAllocatePool(size)   MyAllocatePool__(NonPagedPool,size)
#define DLDFreePool(addr)       MyFreePool__((addr))

#define DLDGetCurrentResourceThread() \
    ((ERESOURCE_THREAD)PsGetCurrentThread())

#ifndef ResourceOwnedExclusive
#define ResourceOwnedExclusive 0x80
#endif
#define ResourceDisableBoost   0x08


VOID DLDInit(ULONG MaxThrdCount);


VOID DLDAcquireExclusive(PERESOURCE Resource,       
                ULONG BugCheckId,
                ULONG Line);

VOID DLDAcquireShared(PERESOURCE Resource,       
                      ULONG BugCheckId,
                      ULONG Line,
                      BOOLEAN WaitForExclusive);

VOID DLDAcquireSharedStarveExclusive(PERESOURCE Resource,       
                      ULONG BugCheckId,
                      ULONG Line);

VOID DLDUnblock(PERESOURCE Resource);


VOID DLDFree(VOID);

typedef struct _THREAD_STRUCT {
    ERESOURCE_THREAD    ThreadId;
    PERESOURCE          WaitingResource;
    ULONG               BugCheckId;
    ULONG               Line;
} THREAD_STRUCT, *PTHREAD_STRUCT;


typedef struct _THREAD_REC_BLOCK {
    PTHREAD_STRUCT      Thread;
    PERESOURCE          HoldingResource;
} THREAD_REC_BLOCK, *PTHREAD_REC_BLOCK;

#endif // _DL_DETECT_H_
