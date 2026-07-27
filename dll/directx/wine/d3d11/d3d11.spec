@ stdcall D3D11CoreCreateDevice(ptr ptr long ptr long ptr)
@ stub D3D11CoreCreateLayeredDevice
@ stub D3D11CoreGetLayeredDeviceSize
@ stdcall D3D11CoreRegisterLayers()
@ stdcall D3D11CreateDevice(ptr long ptr long ptr long long ptr ptr ptr)
@ stdcall D3D11CreateDeviceAndSwapChain(ptr long ptr long ptr long long ptr ptr ptr ptr ptr)
@ stdcall D3D11On12CreateDevice(ptr long ptr long ptr long long ptr ptr ptr)
@ stdcall -version=0x600+ D3DKMTCheckVidPnExclusiveOwnership(ptr) gdi32.D3DKMTCheckVidPnExclusiveOwnership # __REACTOS__: gdi32 only exports D3DKMT* for NT6+
@ stdcall -version=0x600+ D3DKMTCloseAdapter(ptr) gdi32.D3DKMTCloseAdapter
@ stub D3DKMTCreateAllocation
@ stub D3DKMTCreateContext
@ stdcall -version=0x600+ D3DKMTCreateDevice(ptr) gdi32.D3DKMTCreateDevice
@ stub D3DKMTCreateSynchronizationObject
@ stub D3DKMTDestroyAllocation
@ stub D3DKMTDestroyContext
@ stdcall -version=0x600+ D3DKMTDestroyDevice(ptr) gdi32.D3DKMTDestroyDevice
@ stub D3DKMTDestroySynchronizationObject
@ stub D3DKMTEscape
@ stub D3DKMTGetContextSchedulingPriority
@ stub D3DKMTGetDeviceState
@ stub D3DKMTGetDisplayModeList
@ stub D3DKMTGetMultisampleMethodList
@ stub D3DKMTGetRuntimeData
@ stub D3DKMTGetSharedPrimaryHandle
@ stub D3DKMTLock
@ stdcall -version=0x600+ D3DKMTOpenAdapterFromGdiDisplayName(ptr) gdi32.D3DKMTOpenAdapterFromGdiDisplayName
@ stub D3DKMTOpenAdapterFromHdc
@ stub D3DKMTOpenResource
@ stub D3DKMTPresent
@ stdcall -version=0x600+ D3DKMTQueryAdapterInfo(ptr) gdi32.D3DKMTQueryAdapterInfo
@ stub D3DKMTQueryAllocationResidency
@ stub D3DKMTQueryResourceInfo
@ stub D3DKMTRender
@ stub D3DKMTSetAllocationPriority
@ stub D3DKMTSetContextSchedulingPriority
@ stub D3DKMTSetDisplayMode
@ stub D3DKMTSetDisplayPrivateDriverFormat
@ stub D3DKMTSetGammaRamp
@ stdcall -version=0x600+ D3DKMTSetVidPnSourceOwner(ptr) gdi32.D3DKMTSetVidPnSourceOwner
@ stub D3DKMTSignalSynchronizationObject
@ stub D3DKMTUnlock
@ stub D3DKMTWaitForSynchronizationObject
@ stub D3DKMTWaitForVerticalBlankEvent
@ stub OpenAdapter10
@ stub OpenAdapter10_2
