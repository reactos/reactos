//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef _FXFORWARD_HPP_
#define _FXFORWARD_HPP_

typedef struct _FX_DRIVER_GLOBALS *PFX_DRIVER_GLOBALS;

struct FxAutoIrp;
class  FxCallback;
class  FxCallbackLock;
class  FxCallbackMutexLock;
class  FxCallbackSpinLock;
class  FxChildList;
class  FxCmResList;
class  FxCollection;
struct FxCollectionInternal;
class  FxCommonBuffer;
struct FxContextHeader;
class  FxDevice;
class  FxDeviceBase;
struct FxDeviceDescriptionEntry;
class  FxDeviceInterface;
struct FxDeviceText;
struct FxCxDeviceInfo;
class  FxDefaultIrpHandler;
class  FxDisposeList;
class  FxDmaEnabler;
class  FxDmaTransactionBase;
class  FxDmaPacketTransaction;
class  FxDmaScatterGatherTransaction;
class  FxDriver;
class  FxFileObject;
struct FxFileObjectInfo;
struct FxGlobalsStump;
class  FxInterrupt;
struct FxIoQueueNode;
class  FxIoQueue;
class  FxIoResList;
class  FxIoResReqList;
class  FxIoTarget;
class  FxIoTargetSelf;
class  FxIrp;
struct FxIrpPreprocessInfo;
struct FxIrpDynamicDispatchInfo;
class  FxIrpQueue;
class  FxLock;
class  FxLookasideList;
class  FxLookasideListFromPool;
class  FxMemoryBuffer;
class  FxMemoryBufferFromLookaside;
class  FxMemoryBufferFromPool;
class  FxMemoryBufferFromPoolLookaside;
class  FxMemoryBufferPreallocated;
class  FxMemoryObject;
class  FxNonPagedObject;
class  FxNPagedLookasideList;
class  FxNPagedLookasideListFromPool;
class  FxObject;
class  FxPackage;
class  FxPagedLookasideListFromPool;
class  FxPagedObject;
class  FxPkgFdo;
class  FxPkgGeneral;
class  FxPkgIo;
class  FxPkgPdo;
class  FxPkgPnp;
struct FxPnpMachine;
struct FxPnpStateCallback;
struct FxPowerMachine;
struct FxPostProcessInfo;
class  FxPowerIdleMachine;
struct FxPowerPolicyMachine;
struct FxPowerPolicyStateCallback;
struct FxPowerStateCallback;
struct FxQueryInterface;
class  FxRequest;
class  FxRequestBase;
struct FxRequestBuffer;
struct FxRequestContext;
class  FxRequestFromLookaside;
class  FxRequestMemory;
struct FxRequestOutputBuffer;
struct FxRequestSystemBuffer;
class  FxRelatedDevice;
class  FxRelatedDeviceList;
class  FxResourceCm;
class  FxResourceIo;
class  FxSelfManagedIoMachine;
class  FxSpinLock;
class  FxString;
struct FxStump;
class  FxSyncRequest;
class  FxSystemWorkItem;
class  FxSystemThread;
class  FxTagTracker;
class  FxTimer;
struct FxTraceInfo;
class  FxTransactionedList;
struct FxTransactionedEntry;
class  FxUsbDevice;
struct FxUsbIdleInfo;
class  FxUsbInterface;
class  FxUsbPipe;
struct FxUsbPipeContinuousReader;
class  FxVerifierLock;
struct FxWatchdog;
class  FxWaitLock;
class  FxWmiProvider;
class  FxWmiInstance;
class  FxWmiInstanceExternal;
class  FxWmiInstanceInternal;
struct FxWmiInstanceInternalCallbacks;
class  FxWmiIrpHandler;
class  FxWorkItem;

class  IFxHasCallbacks;
class  IFxMemory;

enum FxObjectType;
enum FxWmiInstanceAction;
enum FxDriverObjectUmFlags : USHORT;

PVOID
FxObjectHandleAlloc(
    __in        PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in        POOL_TYPE PoolType,
    __in        size_t Size,
    __in        ULONG Tag,
    __in_opt    PWDF_OBJECT_ATTRIBUTES Attributes,
    __in        USHORT ExtraSize,
    __in        FxObjectType ObjectType
    );

#if (FX_CORE_MODE==FX_CORE_USER_MODE)
#include "FxForwardUm.hpp"
#endif

#endif //  _FXFORWARD_HPP_
