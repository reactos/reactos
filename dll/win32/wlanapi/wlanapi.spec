@ stdcall WlanAllocateMemory (long)
@ stdcall WlanCloseHandle (ptr ptr)
@ stdcall WlanConnect (ptr ptr ptr ptr)
@ stdcall WlanDeleteProfile (ptr ptr ptr ptr)
@ stdcall WlanDisconnect (ptr ptr ptr)
@ stdcall WlanEnumInterfaces (ptr ptr ptr)
@ stub WlanExtractPsdIEDataList
@ stdcall WlanFreeMemory (ptr)
@ stdcall WlanGetAvailableNetworkList (ptr ptr long ptr ptr)
@ stub WlanGetFilterList
@ stdcall WlanGetInterfaceCapability (ptr ptr ptr ptr)
@ stub WlanGetNetworkBssList
@ stdcall WlanGetProfile (ptr ptr ptr ptr ptr long ptr)
@ stdcall WlanGetProfileCustomUserData (ptr ptr ptr ptr ptr ptr)
@ stdcall WlanGetProfileList (ptr ptr ptr ptr)
@ stub WlanGetSecuritySettings
@ stdcall WlanIhvControl (ptr ptr ptr long ptr long ptr ptr)
@ stdcall WlanOpenHandle (long ptr ptr ptr)
@ stub WlanQueryAutoConfigParameter
@ stdcall WlanQueryInterface (ptr ptr ptr ptr ptr ptr ptr)
@ stdcall WlanReasonCodeToString (long long ptr ptr)
@ stdcall WlanRegisterNotification (ptr long long ptr ptr ptr ptr)
@ stdcall WlanRenameProfile (ptr ptr ptr ptr ptr)
@ stub WlanSaveTemporaryProfile
@ stdcall WlanScan (ptr ptr ptr ptr ptr)
@ stub WlanSetAutoConfigParameter
@ stub WlanSetFilterList
@ stub WlanSetInterface
@ stdcall WlanSetProfile (ptr ptr long ptr ptr long ptr ptr)
@ stdcall WlanSetProfileCustomUserData (ptr ptr ptr long ptr ptr)
@ stub WlanSetProfileEapUserData
@ stub WlanSetProfileEapXmlUserData
@ stdcall WlanSetProfileList (ptr ptr long ptr ptr)
@ stub WlanSetProfilePosition
@ stub WlanSetPsdIEDataList
@ stdcall WlanSetSecuritySettings (ptr long ptr)
