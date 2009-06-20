;ScsiPortNotification // vista64
@ stdcall VideoPortAcquireDeviceLock(ptr)
@ stdcall VideoPortAcquireSpinLock(ptr ptr ptr)
@ stdcall VideoPortAcquireSpinLockAtDpcLevel(ptr ptr)
@ stdcall VideoPortAllocateBuffer(ptr long ptr)
@ stdcall VideoPortAllocateCommonBuffer(ptr ptr long ptr long ptr)
@ stdcall VideoPortAllocateContiguousMemory(ptr long long long)
@ stdcall VideoPortAllocatePool(ptr long long long)
@ stdcall VideoPortAssociateEventsWithDmaHandle(ptr ptr ptr ptr)
@ stdcall VideoPortCheckForDeviceExistence(ptr long long long long long long)
;VideoPortCheckForDeviceExistence  there's a second dummy export with the same name on ms videoprt
@ stdcall VideoPortClearEvent(ptr ptr)
@ stdcall VideoPortCompareMemory(ptr ptr long) NTOSKRNL.RtlCompareMemory
@ stdcall VideoPortCompleteDma(ptr ptr ptr long)
@ stdcall VideoPortCreateEvent(ptr long ptr ptr)
@ stdcall VideoPortCreateSecondaryDisplay(ptr ptr long)
@ stdcall VideoPortCreateSpinLock(ptr ptr)
@ stdcall VideoPortDDCMonitorHelper(ptr ptr ptr long)
;VideoPortDbgReportComplete // vista64
;VideoPortDbgReportCreate // vista64
;VideoPortDbgReportSecondaryData // vista64
@ cdecl VideoPortDebugPrint(long ptr)
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
@ fastcall VideoPortInterlockedDecrement(ptr) NTOSKRNL.InterlockedDecrement
@ fastcall VideoPortInterlockedExchange(ptr long) NTOSKRNL.InterlockedExchange
@ fastcall VideoPortInterlockedIncrement(ptr) NTOSKRNL.InterlockedIncrement
@ stdcall VideoPortLockBuffer(ptr ptr long long)
;VideoPortIsNoVesa // 2003 and later
@ stdcall VideoPortLockPages(ptr ptr ptr ptr long)
@ stdcall VideoPortLogError(ptr ptr long long)
@ stdcall VideoPortMapBankedMemory(ptr long long ptr ptr ptr long long ptr ptr)
@ stdcall VideoPortMapDmaMemory(ptr ptr double ptr ptr ptr ptr ptr)
@ stdcall VideoPortMapMemory(ptr long long ptr ptr ptr)
@ stdcall VideoPortMoveMemory(ptr ptr long) NTOSKRNL.RtlMoveMemory
;VideoPortNotification
@ stdcall VideoPortPutDmaAdapter(ptr ptr)
@ stdcall VideoPortQueryPerformanceCounter(ptr ptr)
@ stdcall VideoPortQueryServices(ptr long ptr)
@ stdcall VideoPortQuerySystemTime(ptr) NTOSKRNL.KeQuerySystemTime
@ stdcall VideoPortQueueDpc(ptr ptr ptr)
@ stdcall VideoPortReadPortBufferUchar(ptr ptr long) HAL.READ_PORT_BUFFER_UCHAR
@ stdcall VideoPortReadPortBufferUlong(ptr ptr long) HAL.READ_PORT_BUFFER_ULONG
@ stdcall VideoPortReadPortBufferUshort(ptr ptr long) HAL.READ_PORT_BUFFER_USHORT
@ stdcall VideoPortReadPortUchar(ptr) HAL.READ_PORT_UCHAR
@ stdcall VideoPortReadPortUlong(ptr) HAL.READ_PORT_ULONG
@ stdcall VideoPortReadPortUshort(ptr) HAL.READ_PORT_USHORT
@ stdcall VideoPortReadRegisterBufferUchar(ptr ptr long) NTOSKRNL.READ_REGISTER_BUFFER_UCHAR
@ stdcall VideoPortReadRegisterBufferUlong(ptr ptr long) NTOSKRNL.READ_REGISTER_BUFFER_ULONG
@ stdcall VideoPortReadRegisterBufferUshort(ptr ptr long) NTOSKRNL.READ_REGISTER_BUFFER_USHORT
@ stdcall VideoPortReadRegisterUchar(ptr) NTOSKRNL.READ_REGISTER_UCHAR
@ stdcall VideoPortReadRegisterUlong(ptr) NTOSKRNL.READ_REGISTER_ULONG
@ stdcall VideoPortReadRegisterUshort(ptr) NTOSKRNL.READ_REGISTER_USHORT
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
@ stdcall VideoPortWritePortBufferUchar(ptr ptr long) HAL.WRITE_PORT_BUFFER_UCHAR
@ stdcall VideoPortWritePortBufferUlong(ptr ptr long) HAL.WRITE_PORT_BUFFER_ULONG
@ stdcall VideoPortWritePortBufferUshort(ptr ptr long) HAL.WRITE_PORT_BUFFER_USHORT
@ stdcall VideoPortWritePortUchar(ptr long) HAL.WRITE_PORT_UCHAR
@ stdcall VideoPortWritePortUlong(ptr long) HAL.WRITE_PORT_ULONG
@ stdcall VideoPortWritePortUshort(ptr long) HAL.WRITE_PORT_USHORT
@ stdcall VideoPortWriteRegisterBufferUchar(ptr ptr long) NTOSKRNL.WRITE_REGISTER_BUFFER_UCHAR
@ stdcall VideoPortWriteRegisterBufferUlong(ptr ptr long) NTOSKRNL.WRITE_REGISTER_BUFFER_ULONG
@ stdcall VideoPortWriteRegisterBufferUshort(ptr ptr long) NTOSKRNL.WRITE_REGISTER_BUFFER_USHORT
@ stdcall VideoPortWriteRegisterUchar(ptr long) NTOSKRNL.WRITE_REGISTER_UCHAR
@ stdcall VideoPortWriteRegisterUlong(ptr long) NTOSKRNL.WRITE_REGISTER_ULONG
@ stdcall VideoPortWriteRegisterUshort(ptr long) NTOSKRNL.WRITE_REGISTER_USHORT
@ stdcall VideoPortZeroDeviceMemory(ptr long) NTOSKRNL.RtlZeroMemory
@ stdcall VideoPortZeroMemory(ptr long) NTOSKRNL.RtlZeroMemory
@ stdcall VpNotifyEaData(ptr ptr)
