@ stub BluetoothAddressToString
@ stub BluetoothAuthenticateDevice
@ stub BluetoothAuthenticateDeviceEx
@ stub BluetoothAuthenticateMultipleDevices
@ stub BluetoothAuthenticationAgent
@ stub BluetoothDisconnectDevice
@ stub BluetoothDisplayDeviceProperties
@ stub BluetoothEnableDiscovery
@ stub BluetoothEnableIncomingConnections
@ stub BluetoothEnumerateInstalledServices
@ stub BluetoothEnumerateInstalledServicesEx
@ stub BluetoothFindBrowseGroupClose
@ stub BluetoothFindClassIdClose
@ stdcall -import BluetoothFindDeviceClose(ptr)
@ stub BluetoothFindFirstBrowseGroup
@ stub BluetoothFindFirstClassId
@ stdcall -import BluetoothFindFirstDevice(ptr ptr)
@ stub BluetoothFindFirstProfileDescriptor
@ stub BluetoothFindFirstProtocolDescriptorStack
@ stub BluetoothFindFirstProtocolEntry
@ stdcall -import BluetoothFindFirstRadio(ptr ptr)
@ stub BluetoothFindFirstService
@ stub BluetoothFindFirstServiceEx
@ stub BluetoothFindNextBrowseGroup
@ stub BluetoothFindNextClassId
@ stdcall -import BluetoothFindNextDevice(ptr ptr)
@ stub BluetoothFindNextProfileDescriptor
@ stub BluetoothFindNextProtocolDescriptorStack
@ stub BluetoothFindNextProtocolEntry
@ stdcall -import BluetoothFindNextRadio(ptr ptr)
@ stub BluetoothFindNextService
@ stub BluetoothFindProfileDescriptorClose
@ stub BluetoothFindProtocolDescriptorStackClose
@ stub BluetoothFindProtocolEntryClose
@ stdcall -import BluetoothFindRadioClose(ptr)
@ stub BluetoothFindServiceClose
@ stub BluetoothGetDeviceInfo
@ stdcall -import BluetoothGetRadioInfo(ptr ptr)
@ stub BluetoothIsConnectable
@ stub BluetoothIsDiscoverable
@ stub BluetoothIsVersionAvailable
@ stub BluetoothMapClassOfDeviceToImageIndex
@ stub BluetoothMapClassOfDeviceToString
@ stub BluetoothRegisterForAuthentication
@ stdcall -import BluetoothRegisterForAuthenticationEx(ptr ptr ptr ptr)
@ stub BluetoothRemoveDevice
@ stdcall -import BluetoothSdpEnumAttributes(ptr long ptr ptr)
@ stdcall -import BluetoothSdpGetAttributeValue(ptr long long ptr)
@ stdcall -import BluetoothSdpGetContainerElementData(ptr long ptr ptr)
@ stdcall -import BluetoothSdpGetElementData(ptr long ptr)
@ stub BluetoothSdpGetString
@ stub BluetoothSelectDevices
@ stub BluetoothSelectDevicesFree
@ stub BluetoothSendAuthenticationResponse
@ stub BluetoothSendAuthenticationResponseEx
@ stub BluetoothSetLocalServiceInfo
@ stub BluetoothSetServiceState
@ stdcall -import BluetoothUnregisterAuthentication(long)
@ stub BluetoothUpdateDeviceRecord
@ stub BthpEnableAllServices
@ stub BthpFindPnpInfo
@ stub BthpMapStatusToErr
#@ stub CPlApplet
@ stdcall -private DllCanUnloadNow()
@ stub DllGetClassObject
