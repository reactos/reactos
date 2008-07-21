@ cdecl ScsiDebugPrint()
@ stdcall ScsiPortCompleteRequest(ptr long long long long)
@ stdcall ScsiPortConvertPhysicalAddressToUlong(long long)
@ stdcall ScsiPortConvertUlongToPhysicalAddress(long) NTOSKRNL.RtlConvertUlongToLargeInteger
@ stdcall ScsiPortFlushDma(ptr)
@ stdcall ScsiPortFreeDeviceBase(ptr ptr)
@ stdcall ScsiPortGetBusData(ptr long long long ptr long)
@ stdcall ScsiPortGetDeviceBase(ptr long long long long long long)
@ stdcall ScsiPortGetLogicalUnit(ptr long long long)
@ stdcall ScsiPortGetPhysicalAddress(ptr ptr ptr long)
@ stdcall ScsiPortGetSrb(ptr long long long long)
@ stdcall ScsiPortGetUncachedExtension(ptr ptr long)
@ stdcall ScsiPortGetVirtualAddress(ptr long long)
@ stdcall ScsiPortInitialize(ptr ptr ptr ptr)
@ stdcall ScsiPortIoMapTransfer(ptr ptr long long)
@ stdcall ScsiPortLogError(ptr ptr long long long long long)
@ stdcall ScsiPortMoveMemory(ptr ptr long)
@ cdecl ScsiPortNotification()
@ stdcall ScsiPortReadPortBufferUchar(ptr ptr long) HAL.READ_PORT_BUFFER_UCHAR
@ stdcall ScsiPortReadPortBufferUshort(ptr ptr long) HAL.READ_PORT_BUFFER_USHORT
@ stdcall ScsiPortReadPortBufferUlong(ptr ptr long) HAL.READ_PORT_BUFFER_ULONG
@ stdcall ScsiPortReadPortUchar(ptr) HAL.READ_PORT_UCHAR
@ stdcall ScsiPortReadPortUshort(ptr) HAL.READ_PORT_USHORT
@ stdcall ScsiPortReadPortUlong(ptr) HAL.READ_PORT_ULONG
@ stdcall ScsiPortReadRegisterBufferUchar(ptr ptr long) NTOSKRNL.READ_REGISTER_BUFFER_UCHAR
@ stdcall ScsiPortReadRegisterBufferUshort(ptr ptr long) NTOSKRNL.READ_REGISTER_BUFFER_USHORT
@ stdcall ScsiPortReadRegisterBufferUlong(ptr ptr long) NTOSKRNL.READ_REGISTER_BUFFER_ULONG
@ stdcall ScsiPortReadRegisterUchar(ptr) NTOSKRNL.READ_REGISTER_UCHAR
@ stdcall ScsiPortReadRegisterUshort(ptr) NTOSKRNL.READ_REGISTER_USHORT
@ stdcall ScsiPortReadRegisterUlong(ptr) NTOSKRNL.READ_REGISTER_ULONG
@ stdcall ScsiPortSetBusDataByOffset(ptr long long long ptr long long)
@ stdcall ScsiPortStallExecution(long) HAL.KeStallExecutionProcessor
@ stdcall ScsiPortValidateRange(ptr long long long long long long)
@ stdcall ScsiPortWritePortBufferUchar(ptr ptr long) HAL.WRITE_PORT_BUFFER_UCHAR
@ stdcall ScsiPortWritePortBufferUshort(ptr ptr long) HAL.WRITE_PORT_BUFFER_USHORT
@ stdcall ScsiPortWritePortBufferUlong(ptr ptr long) HAL.WRITE_PORT_BUFFER_ULONG
@ stdcall ScsiPortWritePortUchar(ptr long) HAL.WRITE_PORT_UCHAR
@ stdcall ScsiPortWritePortUshort(ptr long) HAL.WRITE_PORT_USHORT
@ stdcall ScsiPortWritePortUlong(ptr long) HAL.WRITE_PORT_ULONG
@ stdcall ScsiPortWriteRegisterBufferUchar(ptr ptr long) NTOSKRNL.WRITE_REGISTER_BUFFER_UCHAR
@ stdcall ScsiPortWriteRegisterBufferUshort(ptr ptr long) NTOSKRNL.WRITE_REGISTER_BUFFER_USHORT
@ stdcall ScsiPortWriteRegisterBufferUlong(ptr ptr long) NTOSKRNL.WRITE_REGISTER_BUFFER_ULONG
@ stdcall ScsiPortWriteRegisterUchar(ptr long) NTOSKRNL.WRITE_REGISTER_UCHAR
@ stdcall ScsiPortWriteRegisterUshort(ptr long) NTOSKRNL.WRITE_REGISTER_USHORT
@ stdcall ScsiPortWriteRegisterUlong(ptr long) NTOSKRNL.WRITE_REGISTER_ULONG
