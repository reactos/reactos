@ stdcall USBD_Debug_GetHeap(long long long long)
@ stdcall USBD_Debug_RetHeap(ptr long long)
@ stdcall USBD_Debug_LogEntry(ptr ptr ptr ptr)
;; stdcall USBD_AllocateDeviceName
@ stdcall USBD_CalculateUsbBandwidth(long long long)
;; stdcall USBD_CompleteRequest
@ stdcall USBD_CreateConfigurationRequest(ptr ptr)
@ stdcall -arch=i386 _USBD_CreateConfigurationRequestEx@8(ptr ptr) USBD_CreateConfigurationRequestEx
@ stdcall USBD_CreateConfigurationRequestEx(ptr ptr)
;; stdcall USBD_CreateDevice
;; stdcall USBD_Dispatch
;; stdcall USBD_FreeDeviceMutex
;; stdcall USBD_FreeDeviceName
;; stdcall USBD_GetDeviceInformation
@ stdcall USBD_GetInterfaceLength(ptr ptr)
@ stdcall USBD_GetPdoRegistryParameter(ptr ptr long ptr long)
;; stdcall USBD_GetSuspendPowerState
@ stdcall USBD_GetUSBDIVersion(ptr)
;; stdcall USBD_InitializeDevice
;; stdcall USBD_MakePdoName
@ stdcall USBD_ParseConfigurationDescriptor(ptr long long)
@ stdcall -arch=i386 _USBD_ParseConfigurationDescriptorEx@28(ptr ptr long long long long long) USBD_ParseConfigurationDescriptorEx
@ stdcall USBD_ParseConfigurationDescriptorEx(ptr ptr long long long long long)
@ stdcall -arch=i386 _USBD_ParseDescriptors@16(ptr long ptr long) USBD_ParseDescriptors
@ stdcall USBD_ParseDescriptors(ptr long ptr long)
@ stdcall USBD_QueryBusTime(ptr ptr)
;; stdcall USBD_RegisterHcDeviceCapabilities
;; stdcall USBD_RegisterHcFilter
;; stdcall USBD_RegisterHostController
;; stdcall USBD_RemoveDevice
;; stdcall USBD_RestoreDevice
;; stdcall USBD_SetSuspendPowerState
;; stdcall USBD_WaitDeviceMutex
