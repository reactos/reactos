@ stub BluetoothAddressToString
@ stub BluetoothDisconnectDevice
@ stub BluetoothEnableDiscovery
@ stub BluetoothEnableIncomingConnections
@ stub BluetoothEnumerateInstalledServices
@ stub BluetoothEnumerateInstalledServicesEx
@ stub BluetoothEnumerateLocalServices
@ stub BluetoothFindBrowseGroupClose
@ stub BluetoothFindClassIdClose
@ stdcall BluetoothFindDeviceClose(ptr)
@ stub BluetoothFindFirstBrowseGroup
@ stub BluetoothFindFirstClassId
@ stdcall BluetoothFindFirstDevice(ptr ptr)
@ stub BluetoothFindFirstProfileDescriptor
@ stub BluetoothFindFirstProtocolDescriptorStack
@ stub BluetoothFindFirstProtocolEntry
@ stdcall BluetoothFindFirstRadio(ptr ptr)
@ stub BluetoothFindFirstService
@ stub BluetoothFindFirstServiceEx
@ stub BluetoothFindNextBrowseGroup
@ stub BluetoothFindNextClassId
@ stdcall BluetoothFindNextDevice(ptr ptr)
@ stub BluetoothFindNextProfileDescriptor
@ stub BluetoothFindNextProtocolDescriptorStack
@ stub BluetoothFindNextProtocolEntry
@ stdcall BluetoothFindNextRadio(ptr ptr)
@ stub BluetoothFindNextService
@ stub BluetoothFindProfileDescriptorClose
@ stub BluetoothFindProtocolDescriptorStackClose
@ stub BluetoothFindProtocolEntryClose
@ stdcall BluetoothFindRadioClose(ptr)
@ stub BluetoothFindServiceClose
@ stub BluetoothGATTAbortReliableWrite
@ stub BluetoothGATTBeginReliableWrite
@ stub BluetoothGATTEndReliableWrite
@ stub BluetoothGATTGetCharacteristicValue
@ stub BluetoothGATTGetCharacteristics
@ stub BluetoothGATTGetDescriptorValue
@ stub BluetoothGATTGetDescriptors
@ stub BluetoothGATTGetIncludedServices
@ stub BluetoothGATTGetServices
@ stub BluetoothGATTRegisterEvent
@ stub BluetoothGATTSetCharacteristicValue
@ stub BluetoothGATTSetDescriptorValue
@ stub BluetoothGATTUnregisterEvent
@ stub BluetoothGetDeviceInfo
@ stub BluetoothGetLocalServiceInfo
@ stdcall BluetoothGetRadioInfo(ptr ptr)
@ stub BluetoothGetServicePnpInstance
@ stub BluetoothIsConnectable
@ stub BluetoothIsDiscoverable
@ stub BluetoothIsVersionAvailable
@ stub BluetoothRegisterForAuthentication
@ stdcall BluetoothRegisterForAuthenticationEx(ptr ptr ptr ptr)
@ stub BluetoothRemoveDevice
@ stdcall BluetoothSdpEnumAttributes(ptr long ptr ptr)
@ stdcall BluetoothSdpGetAttributeValue(ptr long long ptr)
@ stdcall BluetoothSdpGetContainerElementData(ptr long ptr ptr)
@ stdcall BluetoothSdpGetElementData(ptr long ptr)
@ stub BluetoothSdpGetString
@ stub BluetoothSendAuthenticationResponse
@ stub BluetoothSendAuthenticationResponseEx
@ stub BluetoothSetLocalServiceInfo
@ stub BluetoothSetServiceState
@ stub BluetoothSetServiceStateEx
@ stdcall BluetoothUnregisterAuthentication(long)
@ stub BluetoothUpdateDeviceRecord
@ stub BthpCheckForUnsupportedGuid
@ stub BthpCleanupBRDeviceNode
@ stub BthpCleanupDeviceLocalServices
@ stub BthpCleanupDeviceRemoteServices
@ stub BthpCleanupLEDeviceNodes
@ stub BthpEnableAllServices
@ stub BthpEnableConnectableAndDiscoverable
@ stub BthpEnableRadioSoftware
@ stub BthpFindPnpInfo
@ stub BthpGATTCloseSession
@ stub BthpInnerRecord
@ stub BthpIsBluetoothServiceRunning
@ stub BthpIsConnectableByDefault
@ stub BthpIsDiscoverable
@ stub BthpIsDiscoverableByDefault
@ stub BthpIsRadioSoftwareEnabled
@ stub BthpIsTopOfServiceGroup
@ stub BthpMapStatusToErr
@ stub BthpNextRecord
@ stub BthpRegisterForAuthentication
@ stub BthpSetServiceState
@ stub BthpSetServiceStateEx
@ stub BthpTranspose16Bits
@ stub BthpTranspose32Bits
@ stub BthpTransposeAndExtendBytes
@ stdcall -private DllCanUnloadNow()
@ stub FindNextOpenVCOMPort
@ stub InstallIncomingComPort
@ stub ShouldForceAuthentication
