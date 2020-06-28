@ stdcall MIDL_user_allocate(ptr)
@ stdcall MIDL_user_free(ptr)
@ stub CloseWZCDbLogSession
@ stdcall CreateEapcfgNode()
@ stdcall DestroyEapcfgNode(ptr)
@ stdcall DtlDestroyList(ptr ptr)
@ stdcall EapcfgNodeFromKey(ptr long)
@ stub EnumWZCDbLogRecords
@ stub FlushWZCDbLog
@ stub GetEncryptionForAdapter
@ stub GetModeForAdapter
@ stub GetSSIDForAdapter
@ stub GetSignalStrengthForAdapter
@ stub GetSpecificLogRecord
@ stub OpenWZCDbLogSession
@ stdcall ReadEapcfgList(long)
@ stdcall WZCDeleteIntfObj(ptr)
@ stdcall WZCEapolFreeState(ptr)
@ stdcall WZCEapolGetCustomAuthData(wstr wstr long long ptr ptr ptr)
@ stdcall WZCEapolGetInterfaceParams(wstr wstr long ptr ptr)
@ stub WZCEapolGetPMKCacheInfo
@ stdcall WZCEapolQueryState(wstr wstr ptr)
@ stdcall WZCEapolReAuthenticate(wstr wstr)
@ stdcall WZCEapolSetCustomAuthData(wstr wstr long long ptr ptr long)
@ stdcall WZCEapolSetInterfaceParams(wstr wstr long ptr ptr)
@ stdcall WZCEapolUIResponse(wstr ptr ptr int64 int64 int64 int64 int64 int64 int64 int64 int64 int64 int64 int64 int64 int64 int64 int64 int64 int64 int64 int64 int64 int64 int64 int64 int64 int64 int64 int64 int64 int64) # HACK: the 2nd parameter has very big real symbol name, since it uses the struct passed by value, so in order to avoid compilation problems, we added required amount of int64 parameters.
@ stdcall WZCEnumInterfaces(wstr ptr)
@ stub WZCGetAPIVersion
@ stdcall WZCGetEapUserInfo(wstr long long str ptr long)
@ stdcall WZCPassword2Key(ptr str)
@ stub WZCProviderCreateConnectionProperties
@ stub WZCProviderCreateUserProperties
@ stub WZCProviderCreateWirelessProfile
@ stub WZCProviderDeleteWirelessProfile
@ stdcall WZCQueryContext(wstr long ptr ptr)
@ stdcall WZCQueryInterface(wstr long ptr ptr)
@ stdcall WZCRefreshInterface(wstr long ptr ptr)
@ stdcall WZCSetContext(wstr long ptr ptr)
@ stdcall WZCSetInterface(wstr long ptr ptr)
