@ stdcall -private DllInitialize(long)
@ stdcall -private DllUnload()

; Adapters (adapter.c)
@ stdcall PcAddAdapterDevice(ptr ptr ptr long long)
@ stdcall PcInitializeAdapterDriver(ptr ptr ptr)

; Factories
@ stdcall PcNewDmaChannel(ptr ptr long ptr ptr)
@ stdcall PcNewInterruptSync(ptr ptr ptr long long)
@ stdcall PcNewMiniport(ptr ptr)
@ stdcall PcNewPort(ptr ptr)
@ stdcall PcNewRegistryKey(ptr ptr long long ptr ptr ptr long ptr)
@ stdcall PcNewResourceList(ptr ptr long ptr ptr)
@ stdcall PcNewResourceSublist(ptr ptr long ptr long)
@ stdcall PcNewServiceGroup(ptr ptr)

; Digital Rights Management (drm.c)
@ stdcall PcAddContentHandlers(long ptr long)
@ stdcall PcCreateContentMixed(ptr long ptr)
@ stdcall PcDestroyContent(long)
@ stdcall PcForwardContentToDeviceObject(long ptr ptr)
@ stdcall PcForwardContentToFileObject(long ptr)
@ stdcall PcForwardContentToInterface(long ptr long)
@ stdcall PcGetContentRights(long ptr)

; IRP Helpers
@ stdcall PcCompleteIrp(ptr ptr long)
@ stdcall PcDispatchIrp(ptr ptr)
@ stdcall PcForwardIrpSynchronous(ptr ptr)

; Misc
@ stdcall PcGetTimeInterval(long long)
@ stdcall PcRegisterSubdevice(ptr wstr ptr)

; Physical Connections
@ stdcall PcRegisterPhysicalConnection(ptr ptr long ptr long)
@ stdcall PcRegisterPhysicalConnectionFromExternal(ptr ptr long ptr long)
@ stdcall PcRegisterPhysicalConnectionToExternal(ptr ptr long ptr long)

; Power Management
@ stdcall PcRegisterAdapterPowerManagement(ptr ptr)
@ stdcall PcRequestNewPowerState(ptr long)
@ stdcall PcUnregisterAdapterPowerManagement(ptr)

; Properties
@ stdcall PcCompletePendingPropertyRequest(ptr long)
@ stdcall PcGetDeviceProperty(ptr long long ptr ptr)

; Timeouts
@ stdcall PcRegisterIoTimeout(ptr ptr ptr)
@ stdcall PcUnregisterIoTimeout(ptr ptr ptr)
