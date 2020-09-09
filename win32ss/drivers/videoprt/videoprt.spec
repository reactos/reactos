@ stdcall VideoPortAcquireDeviceLock(ptr)
@ stdcall VideoPortAcquireSpinLock(ptr ptr ptr)
@ stdcall VideoPortAcquireSpinLockAtDpcLevel(ptr ptr)
@ stdcall VideoPortAllocateBuffer(ptr long ptr)
@ stdcall VideoPortAllocateCommonBuffer(ptr ptr long ptr long ptr)
@ stdcall VideoPortAllocateContiguousMemory(ptr long long long)
@ stdcall VideoPortAllocatePool(ptr long long long)
@ stdcall VideoPortAssociateEventsWithDmaHandle(ptr ptr ptr ptr)
@ stdcall VideoPortCheckForDeviceExistence(ptr long long long long long long)

;; Starting NT 5.1, the following function was introduced for a mysterious reason
;; (it differs from the previous one only by its name).
@ stdcall VideoPortCheckForDeviceExistance(ptr long long long long long long) VideoPortCheckForDeviceExistence

@ stdcall VideoPortClearEvent(ptr ptr)
@ stdcall VideoPortCompareMemory(ptr ptr long) NTOSKRNL.RtlCompareMemory
@ stdcall VideoPortCompleteDma(ptr ptr ptr long)
@ stdcall VideoPortCreateEvent(ptr long ptr ptr)
@ stdcall VideoPortCreateSecondaryDisplay(ptr ptr long)
@ stdcall VideoPortCreateSpinLock(ptr ptr)
@ stdcall VideoPortDDCMonitorHelper(ptr ptr ptr long)
@ varargs VideoPortDebugPrint(long str)
@ stdcall VideoPortDeleteEvent(ptr ptr)
@ stdcall VideoPortDeleteSpinLock(ptr ptr)
@ stdcall VideoPortDisableInterrupt(ptr)
@ stdcall VideoPortDoDma(ptr ptr long)
@ stdcall VideoPortEnableInterrupt(ptr)
@ stdcall VideoPortEnumerateChildren(ptr ptr)
@ stdcall VideoPortFlushRegistry(ptr)
@ stdcall VideoPortFreeCommonBuffer(ptr long ptr long long long)
@ stdcall VideoPortFreeDeviceBase(ptr ptr)
@ stdcall VideoPortFreePool(ptr ptr)
@ stdcall VideoPortGetAccessRanges(ptr long ptr long ptr ptr ptr ptr)
@ stdcall VideoPortGetAgpServices(ptr ptr)
@ stdcall VideoPortGetAssociatedDeviceExtension(ptr)
@ stdcall VideoPortGetAssociatedDeviceID(ptr)
@ stdcall VideoPortGetBusData(ptr long long ptr long long)
@ stdcall VideoPortGetBytesUsed(ptr ptr)
@ stdcall VideoPortGetCommonBuffer(ptr long long ptr ptr long)
@ stdcall VideoPortGetCurrentIrql()
@ stdcall VideoPortGetDeviceBase(ptr long long long long)
@ stdcall VideoPortGetDeviceData(ptr long ptr ptr)
@ stdcall VideoPortGetDmaAdapter(ptr ptr)
@ stdcall VideoPortGetDmaContext(ptr ptr)
@ stdcall VideoPortGetMdl(ptr ptr)
@ stdcall VideoPortGetRegistryParameters(ptr wstr long ptr ptr)
@ stdcall VideoPortGetRomImage(ptr ptr long long)
@ stdcall VideoPortGetVersion(ptr ptr)
@ stdcall VideoPortGetVgaStatus(ptr ptr)
@ stdcall VideoPortInitialize(ptr ptr ptr ptr)
@ stdcall VideoPortInt10(ptr ptr)
;;@ fastcall -arch=i386,arm VideoPortInterlockedDecrement(ptr) NTOSKRNL.InterlockedDecrement
;;@ fastcall -arch=x86_64 VideoPortInterlockedDecrement(ptr)
@ fastcall VideoPortInterlockedDecrement(ptr)
;;@ fastcall -arch=i386,arm VideoPortInterlockedExchange(ptr long) NTOSKRNL.InterlockedExchange
;;@ fastcall -arch=x86_64 VideoPortInterlockedExchange(ptr long)
@ fastcall VideoPortInterlockedExchange(ptr long)
;;@ fastcall -arch=i386,arm VideoPortInterlockedIncrement(ptr) NTOSKRNL.InterlockedIncrement
;;@ fastcall -arch=x86_64 VideoPortInterlockedIncrement(ptr)
@ fastcall VideoPortInterlockedIncrement(ptr)
@ stdcall VideoPortIsNoVesa()
@ stdcall VideoPortLockBuffer(ptr ptr long long)
@ stdcall VideoPortLockPages(ptr ptr ptr ptr long)
@ stdcall VideoPortLogError(ptr ptr long long)
@ stdcall VideoPortMapBankedMemory(ptr long long ptr ptr ptr long long ptr ptr)
@ stdcall VideoPortMapDmaMemory(ptr ptr double ptr ptr ptr ptr ptr)
@ stdcall VideoPortMapMemory(ptr long long ptr ptr ptr)
@ stdcall VideoPortMoveMemory(ptr ptr long) NTOSKRNL.RtlMoveMemory
@ stdcall VideoPortPutDmaAdapter(ptr ptr)
@ stdcall VideoPortQueryPerformanceCounter(ptr ptr)
@ stdcall VideoPortQueryServices(ptr long ptr)
@ stdcall -arch=i386,arm VideoPortQuerySystemTime(ptr) NTOSKRNL.KeQuerySystemTime
@ stdcall -arch=x86_64 VideoPortQuerySystemTime(ptr)
@ stdcall VideoPortQueueDpc(ptr ptr ptr)
@ stdcall -arch=i386,arm VideoPortReadPortBufferUchar(ptr ptr long) HAL.READ_PORT_BUFFER_UCHAR
@ stdcall -arch=x86_64 VideoPortReadPortBufferUchar(ptr ptr long)
@ stdcall -arch=i386,arm VideoPortReadPortBufferUlong(ptr ptr long) HAL.READ_PORT_BUFFER_ULONG
@ stdcall -arch=x86_64 VideoPortReadPortBufferUlong(ptr ptr long)
@ stdcall -arch=i386,arm VideoPortReadPortBufferUshort(ptr ptr long) HAL.READ_PORT_BUFFER_USHORT
@ stdcall -arch=x86_64 VideoPortReadPortBufferUshort(ptr ptr long)
@ stdcall -arch=i386,arm VideoPortReadPortUchar(ptr) HAL.READ_PORT_UCHAR
@ stdcall -arch=x86_64 VideoPortReadPortUchar(ptr)
@ stdcall -arch=i386,arm VideoPortReadPortUlong(ptr) HAL.READ_PORT_ULONG
@ stdcall -arch=x86_64 VideoPortReadPortUlong(ptr)
@ stdcall -arch=i386,arm VideoPortReadPortUshort(ptr) HAL.READ_PORT_USHORT
@ stdcall -arch=x86_64 VideoPortReadPortUshort(ptr)
@ stdcall -arch=i386,arm VideoPortReadRegisterBufferUchar(ptr ptr long) NTOSKRNL.READ_REGISTER_BUFFER_UCHAR
@ stdcall -arch=x86_64 VideoPortReadRegisterBufferUchar(ptr ptr long)
@ stdcall -arch=i386,arm VideoPortReadRegisterBufferUlong(ptr ptr long) NTOSKRNL.READ_REGISTER_BUFFER_ULONG
@ stdcall -arch=x86_64 VideoPortReadRegisterBufferUlong(ptr ptr long)
@ stdcall -arch=i386,arm VideoPortReadRegisterBufferUshort(ptr ptr long) NTOSKRNL.READ_REGISTER_BUFFER_USHORT
@ stdcall -arch=x86_64 VideoPortReadRegisterBufferUshort(ptr ptr long)
@ stdcall -arch=i386,arm VideoPortReadRegisterUchar(ptr) NTOSKRNL.READ_REGISTER_UCHAR
@ stdcall -arch=x86_64 VideoPortReadRegisterUchar(ptr)
@ stdcall -arch=i386,arm VideoPortReadRegisterUlong(ptr) NTOSKRNL.READ_REGISTER_ULONG
@ stdcall -arch=x86_64 VideoPortReadRegisterUlong(ptr)
@ stdcall -arch=i386,arm VideoPortReadRegisterUshort(ptr) NTOSKRNL.READ_REGISTER_USHORT
@ stdcall -arch=x86_64 VideoPortReadRegisterUshort(ptr)
@ stdcall VideoPortReadStateEvent(ptr ptr)
@ stdcall VideoPortRegisterBugcheckCallback(ptr long ptr long)
@ stdcall VideoPortReleaseBuffer(ptr ptr)
@ stdcall VideoPortReleaseCommonBuffer(ptr ptr long long long ptr long)
@ stdcall VideoPortReleaseDeviceLock(ptr)
@ stdcall VideoPortReleaseSpinLock(ptr ptr long)
@ stdcall VideoPortReleaseSpinLockFromDpcLevel(ptr ptr)
@ stdcall VideoPortScanRom(ptr ptr long ptr)
@ stdcall VideoPortSetBusData(ptr long long ptr long long)
@ stdcall VideoPortSetBytesUsed(ptr ptr long)
@ stdcall VideoPortSetDmaContext(ptr ptr ptr)
@ stdcall VideoPortSetEvent(ptr ptr)
@ stdcall VideoPortSetRegistryParameters(ptr wstr ptr long)
@ stdcall VideoPortSetTrappedEmulatorPorts(ptr long ptr)
@ stdcall VideoPortSignalDmaComplete(ptr ptr)
@ stdcall VideoPortStallExecution(ptr) HAL.KeStallExecutionProcessor
@ stdcall VideoPortStartDma(ptr ptr ptr long ptr ptr ptr long)
@ stdcall VideoPortStartTimer(ptr)
@ stdcall VideoPortStopTimer(ptr)
@ stdcall VideoPortSynchronizeExecution(ptr long ptr ptr)
@ stdcall VideoPortUnlockBuffer(ptr ptr)
@ stdcall VideoPortUnlockPages(ptr ptr)
@ stdcall VideoPortUnmapDmaMemory(ptr ptr ptr ptr)
@ stdcall VideoPortUnmapMemory(ptr ptr ptr)
@ stdcall VideoPortVerifyAccessRanges(ptr long ptr)
@ stdcall VideoPortWaitForSingleObject(ptr ptr ptr)
@ stdcall -arch=i386,arm VideoPortWritePortBufferUchar(ptr ptr long) HAL.WRITE_PORT_BUFFER_UCHAR
@ stdcall -arch=x86_64 VideoPortWritePortBufferUchar(ptr ptr long)
@ stdcall -arch=i386,arm VideoPortWritePortBufferUlong(ptr ptr long) HAL.WRITE_PORT_BUFFER_ULONG
@ stdcall -arch=x86_64 VideoPortWritePortBufferUlong(ptr ptr long)
@ stdcall -arch=i386,arm VideoPortWritePortBufferUshort(ptr ptr long) HAL.WRITE_PORT_BUFFER_USHORT
@ stdcall -arch=x86_64 VideoPortWritePortBufferUshort(ptr ptr long)
@ stdcall -arch=i386,arm VideoPortWritePortUchar(ptr long) HAL.WRITE_PORT_UCHAR
@ stdcall -arch=x86_64 VideoPortWritePortUchar(ptr long)
@ stdcall -arch=i386,arm VideoPortWritePortUlong(ptr long) HAL.WRITE_PORT_ULONG
@ stdcall -arch=x86_64 VideoPortWritePortUlong(ptr long)
@ stdcall -arch=i386,arm VideoPortWritePortUshort(ptr long) HAL.WRITE_PORT_USHORT
@ stdcall -arch=x86_64 VideoPortWritePortUshort(ptr long)
@ stdcall -arch=i386,arm VideoPortWriteRegisterBufferUchar(ptr ptr long) NTOSKRNL.WRITE_REGISTER_BUFFER_UCHAR
@ stdcall -arch=x86_64 VideoPortWriteRegisterBufferUchar(ptr ptr long)
@ stdcall -arch=i386,arm VideoPortWriteRegisterBufferUlong(ptr ptr long) NTOSKRNL.WRITE_REGISTER_BUFFER_ULONG
@ stdcall -arch=x86_64 VideoPortWriteRegisterBufferUlong(ptr ptr long)
@ stdcall -arch=i386,arm VideoPortWriteRegisterBufferUshort(ptr ptr long) NTOSKRNL.WRITE_REGISTER_BUFFER_USHORT
@ stdcall -arch=x86_64 VideoPortWriteRegisterBufferUshort(ptr ptr long)
@ stdcall -arch=i386,arm VideoPortWriteRegisterUchar(ptr long) NTOSKRNL.WRITE_REGISTER_UCHAR
@ stdcall -arch=x86_64 VideoPortWriteRegisterUchar(ptr long)
@ stdcall -arch=i386,arm VideoPortWriteRegisterUlong(ptr long) NTOSKRNL.WRITE_REGISTER_ULONG
@ stdcall -arch=x86_64 VideoPortWriteRegisterUlong(ptr long)
@ stdcall -arch=i386,arm VideoPortWriteRegisterUshort(ptr long) NTOSKRNL.WRITE_REGISTER_USHORT
@ stdcall -arch=x86_64 VideoPortWriteRegisterUshort(ptr long)
@ stdcall VideoPortZeroDeviceMemory(ptr long) NTOSKRNL.RtlZeroMemory
@ stdcall VideoPortZeroMemory(ptr long) NTOSKRNL.RtlZeroMemory
@ stdcall VpNotifyEaData(ptr ptr)
@ stdcall WdDdiWatchdogDpcCallback(ptr ptr ptr ptr)
