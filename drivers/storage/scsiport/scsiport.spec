@ cdecl ScsiDebugPrint()
@ stdcall ScsiPortCompleteRequest(ptr long long long long)
@ stdcall ScsiPortConvertPhysicalAddressToUlong(long long)
@ stdcall -arch=i386 ScsiPortConvertUlongToPhysicalAddress(long) NTOSKRNL.RtlConvertUlongToLargeInteger
@ stdcall -arch=x86_64 ScsiPortConvertUlongToPhysicalAddress(long)
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
@ stdcall -arch=i386 ScsiPortReadPortBufferUchar(ptr ptr long) HAL.READ_PORT_BUFFER_UCHAR
@ stdcall -arch=i386 ScsiPortReadPortBufferUshort(ptr ptr long) HAL.READ_PORT_BUFFER_USHORT
@ stdcall -arch=i386 ScsiPortReadPortBufferUlong(ptr ptr long) HAL.READ_PORT_BUFFER_ULONG
@ stdcall -arch=i386 ScsiPortReadPortUchar(ptr) HAL.READ_PORT_UCHAR
@ stdcall -arch=i386 ScsiPortReadPortUshort(ptr) HAL.READ_PORT_USHORT
@ stdcall -arch=i386 ScsiPortReadPortUlong(ptr) HAL.READ_PORT_ULONG
@ stdcall -arch=i386 ScsiPortReadRegisterBufferUchar(ptr ptr long) NTOSKRNL.READ_REGISTER_BUFFER_UCHAR
@ stdcall -arch=i386 ScsiPortReadRegisterBufferUshort(ptr ptr long) NTOSKRNL.READ_REGISTER_BUFFER_USHORT
@ stdcall -arch=i386 ScsiPortReadRegisterBufferUlong(ptr ptr long) NTOSKRNL.READ_REGISTER_BUFFER_ULONG
@ stdcall -arch=i386 ScsiPortReadRegisterUchar(ptr) NTOSKRNL.READ_REGISTER_UCHAR
@ stdcall -arch=i386 ScsiPortReadRegisterUshort(ptr) NTOSKRNL.READ_REGISTER_USHORT
@ stdcall -arch=i386 ScsiPortReadRegisterUlong(ptr) NTOSKRNL.READ_REGISTER_ULONG
@ stdcall -arch=x86_64 ScsiPortReadPortBufferUchar(ptr ptr long)
@ stdcall -arch=x86_64 ScsiPortReadPortBufferUshort(ptr ptr long)
@ stdcall -arch=x86_64 ScsiPortReadPortBufferUlong(ptr ptr long)
@ stdcall -arch=x86_64 ScsiPortReadPortUchar(ptr)
@ stdcall -arch=x86_64 ScsiPortReadPortUshort(ptr)
@ stdcall -arch=x86_64 ScsiPortReadPortUlong(ptr)
@ stdcall -arch=x86_64 ScsiPortReadRegisterBufferUchar(ptr ptr long)
@ stdcall -arch=x86_64 ScsiPortReadRegisterBufferUshort(ptr ptr long)
@ stdcall -arch=x86_64 ScsiPortReadRegisterBufferUlong(ptr ptr long)
@ stdcall -arch=x86_64 ScsiPortReadRegisterUchar(ptr)
@ stdcall -arch=x86_64 ScsiPortReadRegisterUshort(ptr)
@ stdcall -arch=x86_64 ScsiPortReadRegisterUlong(ptr)
@ stdcall ScsiPortSetBusDataByOffset(ptr long long long ptr long long)
@ stdcall ScsiPortStallExecution(long) HAL.KeStallExecutionProcessor
@ stdcall ScsiPortValidateRange(ptr long long long long long long)
@ stdcall -arch=i386 ScsiPortWritePortBufferUchar(ptr ptr long) HAL.WRITE_PORT_BUFFER_UCHAR
@ stdcall -arch=i386 ScsiPortWritePortBufferUshort(ptr ptr long) HAL.WRITE_PORT_BUFFER_USHORT
@ stdcall -arch=i386 ScsiPortWritePortBufferUlong(ptr ptr long) HAL.WRITE_PORT_BUFFER_ULONG
@ stdcall -arch=i386 ScsiPortWritePortUchar(ptr long) HAL.WRITE_PORT_UCHAR
@ stdcall -arch=i386 ScsiPortWritePortUshort(ptr long) HAL.WRITE_PORT_USHORT
@ stdcall -arch=i386 ScsiPortWritePortUlong(ptr long) HAL.WRITE_PORT_ULONG
@ stdcall -arch=i386 ScsiPortWriteRegisterBufferUchar(ptr ptr long) NTOSKRNL.WRITE_REGISTER_BUFFER_UCHAR
@ stdcall -arch=i386 ScsiPortWriteRegisterBufferUshort(ptr ptr long) NTOSKRNL.WRITE_REGISTER_BUFFER_USHORT
@ stdcall -arch=i386 ScsiPortWriteRegisterBufferUlong(ptr ptr long) NTOSKRNL.WRITE_REGISTER_BUFFER_ULONG
@ stdcall -arch=i386 ScsiPortWriteRegisterUchar(ptr long) NTOSKRNL.WRITE_REGISTER_UCHAR
@ stdcall -arch=i386 ScsiPortWriteRegisterUshort(ptr long) NTOSKRNL.WRITE_REGISTER_USHORT
@ stdcall -arch=i386 ScsiPortWriteRegisterUlong(ptr long) NTOSKRNL.WRITE_REGISTER_ULONG
@ stdcall -arch=x86_64 ScsiPortWritePortBufferUchar(ptr ptr long)
@ stdcall -arch=x86_64 ScsiPortWritePortBufferUshort(ptr ptr long)
@ stdcall -arch=x86_64 ScsiPortWritePortBufferUlong(ptr ptr long)
@ stdcall -arch=x86_64 ScsiPortWritePortUchar(ptr long)
@ stdcall -arch=x86_64 ScsiPortWritePortUshort(ptr long)
@ stdcall -arch=x86_64 ScsiPortWritePortUlong(ptr long)
@ stdcall -arch=x86_64 ScsiPortWriteRegisterBufferUchar(ptr ptr long)
@ stdcall -arch=x86_64 ScsiPortWriteRegisterBufferUshort(ptr ptr long)
@ stdcall -arch=x86_64 ScsiPortWriteRegisterBufferUlong(ptr ptr long)
@ stdcall -arch=x86_64 ScsiPortWriteRegisterUchar(ptr long)
@ stdcall -arch=x86_64 ScsiPortWriteRegisterUshort(ptr long)
@ stdcall -arch=x86_64 ScsiPortWriteRegisterUlong(ptr long)
