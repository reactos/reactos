# @ stub DDMComputeLuid
# @ stub DDMFreeDialingParam
# @ stub DDMFreePhonebookContext
# @ stub DDMFreeRemoteEndpoint
# @ stub DDMGetAddressesFromPhonebook
# @ stub DDMGetEapInfo
# @ stub DDMGetEapUserIdentityW
# @ stub DDMGetPhoneBookContext
# @ stub DDMGetPhonebookInfo
# @ stub DDMGetProtocolStartParams
# @ stub DDMGetRasDialParams
# @ stub DDMGetRasDialingParams
# @ stub DDMGetTunnelEndpoints
# @ stub DDMRasPbkEntryCleanup
# @ stub DDMUpdateProtocolConfigInfoFromEntry
# @ stub DwCloneEntry
# @ stub DwEnumEntryDetails
# @ stub DwEnumEntryDetailsEx
# @ stub DwRasUninitialize
# @ stub GetAutoTriggerProfileInfo
# @ stub IsActiveAutoTriggerConnection
# @ stub IsActiveAutoTriggerConnectionEx
# @ stub RasAutoDialSharedConnection
@ stub RasAutodialAddressToNetwork
@ stub RasAutodialEntryToNetwork
@ stub RasClearConnectionStatistics
# @ stub RasClearLinkStatistics
# @ stub RasCompleteDialMachineCleanup
# @ stub RasConfigUserProxySettingsW
@ stdcall RasConnectionNotificationA(ptr long long)
@ stdcall RasConnectionNotificationW(ptr long long)
@ stdcall RasCreatePhonebookEntryA(long str)
@ stdcall RasCreatePhonebookEntryW(long wstr)
@ stdcall RasDeleteEntryA(str str)
@ stdcall RasDeleteEntryW(wstr wstr)
@ stdcall RasDeleteSubEntryA(str str long)
@ stdcall RasDeleteSubEntryW(wstr wstr long)
@ stdcall RasDialA(ptr str ptr long ptr ptr)
@ stdcall RasDialW(ptr wstr ptr long ptr ptr)
@ stub RasDialWow
@ stdcall RasEditPhonebookEntryA(long str str)
@ stdcall RasEditPhonebookEntryW(long wstr wstr)
@ stdcall RasEnumAutodialAddressesA(ptr ptr ptr)
@ stdcall RasEnumAutodialAddressesW(ptr ptr ptr)
@ stdcall RasEnumConnectionsA(ptr ptr ptr)
@ stdcall RasEnumConnectionsW(ptr ptr ptr)
@ stub RasEnumConnectionsWow
@ stdcall RasEnumDevicesA(ptr ptr ptr)
@ stdcall RasEnumDevicesW(ptr ptr ptr)
@ stdcall RasEnumEntriesA(str str ptr ptr ptr)
@ stdcall RasEnumEntriesW(wstr wstr ptr ptr ptr)
@ stub RasEnumEntriesWow
# @ stub RasFreeEapUserIdentityA
# @ stub RasFreeEapUserIdentityW
# @ stub RasFreeEntryAdvancedProperties
# @ stub RasGetAutoTriggerConnectStatus
@ stdcall RasGetAutodialAddressA(str ptr ptr ptr ptr)
@ stdcall RasGetAutodialAddressW(wstr ptr ptr ptr ptr)
@ stdcall RasGetAutodialEnableA(long ptr)
@ stdcall RasGetAutodialEnableW(long ptr)
@ stdcall RasGetAutodialParamA(long ptr ptr)
@ stdcall RasGetAutodialParamW(long ptr ptr)
@ stub RasGetConnectResponse
@ stdcall RasGetConnectStatusA(ptr ptr)
@ stdcall RasGetConnectStatusW(ptr ptr)
@ stub RasGetConnectStatusWow
# @ stub RasGetConnectionErrorStringW
@ stdcall RasGetConnectionStatistics(ptr ptr)
@ stub RasGetCountryInfoA
@ stub RasGetCountryInfoW
@ stub RasGetCredentialsA
@ stub RasGetCredentialsW
# @ stub RasGetCustomAuthDataA
# @ stub RasGetCustomAuthDataW
# @ stub RasGetEapUserDataA
# @ stub RasGetEapUserDataW
# @ stub RasGetEapUserIdentityA
# @ stub RasGetEapUserIdentityW
# @ stub RasGetEntryAdvancedProperties
@ stdcall RasGetEntryDialParamsA(str ptr ptr)
@ stdcall RasGetEntryDialParamsW(wstr ptr ptr)
# @ stub RasGetEntryHrasconnW
@ stdcall RasGetEntryPropertiesA(str str ptr ptr ptr ptr)
@ stdcall RasGetEntryPropertiesW(wstr wstr ptr ptr ptr ptr)
@ stdcall RasGetErrorStringA(long ptr long)
@ stdcall RasGetErrorStringW(long ptr long)
@ stub RasGetErrorStringWow
@ stub RasGetHport
@ stdcall RasGetLinkStatistics(ptr long ptr)
# @ stub RasGetNapStatus
# @ stub RasGetPCscf
# @ stub RasGetPbkPath
@ stdcall RasGetProjectionInfoA(ptr ptr ptr ptr)
# @ stub RasGetProjectionInfoEx
@ stdcall RasGetProjectionInfoW(ptr ptr ptr ptr)
@ stub RasGetSubEntryHandleA
@ stub RasGetSubEntryHandleW
@ stub RasGetSubEntryPropertiesA
@ stub RasGetSubEntryPropertiesW
# @ stub RasHandleTriggerConnDisconnect
@ stdcall RasHangUpA(long)
@ stdcall RasHangUpW(long)
@ stub RasHangUpWow
# @ stub RasInvokeEapUI
# @ stub RasIsPublicPhonebook
# @ stub RasIsSharedConnection
# @ stub RasNQMStateEnteredNotification
# @ stub RasQueryRedialOnLinkFailure
# @ stub RasQuerySharedAutoDial
# @ stub RasQuerySharedConnection
# @ stub RasRegisterEntryChangeNotification
@ stdcall RasRenameEntryA(str str str)
@ stdcall RasRenameEntryW(wstr wstr wstr)
# @ stub RasRestoreDefaultLegacyProxySettings
# @ stub RasScriptExecute
# @ stub RasScriptGetIpAddress
# @ stub RasScriptInit
# @ stub RasScriptReceive
# @ stub RasScriptSend
# @ stub RasScriptTerm
# @ stub RasSetAutoTriggerProfile
# @ stub RasSetAutoTriggerProfileEx
@ stdcall RasSetAutodialAddressA(str long ptr long long)
@ stdcall RasSetAutodialAddressW(wstr long ptr long long)
@ stdcall RasSetAutodialEnableA(long long)
@ stdcall RasSetAutodialEnableW(long long)
@ stdcall RasSetAutodialParamA(long ptr long)
@ stdcall RasSetAutodialParamW(long ptr long)
@ stdcall RasSetCredentialsA(str str ptr long)
@ stdcall RasSetCredentialsW(wstr wstr ptr long)
@ stdcall RasSetCustomAuthDataA(str str ptr long)
@ stdcall RasSetCustomAuthDataW(wstr wstr ptr long)
# @ stub RasSetEapUserDataA
# @ stub RasSetEapUserDataAEx
# @ stub RasSetEapUserDataW
# @ stub RasSetEapUserDataWEx
# @ stub RasSetEntryAdvancedProperties
@ stdcall RasSetEntryDialParamsA(str ptr long)
@ stdcall RasSetEntryDialParamsW(wstr ptr long)
@ stdcall RasSetEntryPropertiesA(str str ptr long ptr long)
@ stdcall RasSetEntryPropertiesW(wstr wstr ptr long ptr long)
@ stub RasSetOldPassword
# @ stub RasSetPerConnectionProxy
# @ stub RasSetSharedAutoDial
@ stdcall RasSetSubEntryPropertiesA(str str long ptr long ptr long)
@ stdcall RasSetSubEntryPropertiesW(wstr wstr long ptr long ptr long)
# @ stub RasTriggerConnection
# @ stub RasTriggerConnectionEx
# @ stub RasTriggerDisconnection
# @ stub RasTriggerDisconnectionEx
# @ stub RasUnregisterEntryChangeNotification
# @ stub RasUpdateConnection
@ stdcall RasValidateEntryNameA(str str)
@ stdcall RasValidateEntryNameW(wstr wstr)
# stub @ RasWriteSharedPbkOptions
# stub @ UnInitializeRAS

500 stub	RnaEngineRequest
501 stub	DialEngineRequest
502 stub	SuprvRequest
503 stub	DialInMessage
504 stub	RnaEnumConnEntries
505 stub	RnaGetConnEntry
506 stub	RnaFreeConnEntry
507 stub	RnaSaveConnEntry
508 stub	RnaDeleteConnEntry
509 stub	RnaRenameConnEntry
510 stub	RnaValidateEntryName
511 stub	RnaEnumDevices
512 stub	RnaGetDeviceInfo
513 stub	RnaGetDefaultDevConfig
514 stub	RnaBuildDevConfig
515 stub	RnaDevConfigDlg
516 stub	RnaFreeDevConfig
517 stub	RnaActivateEngine
518 stub	RnaDeactivateEngine
519 stub	SuprvEnumAccessInfo
520 stub	SuprvGetAccessInfo
521 stub	SuprvSetAccessInfo
522 stub	SuprvGetAdminConfig
523 stub	SuprvInitialize
524 stub	SuprvDeInitialize
525 stub	RnaUIDial
526 stub	RnaImplicitDial
527 stub	RasDial16
528 stub	RnaSMMInfoDialog
529 stub	RnaEnumerateMacNames
530 stub	RnaEnumCountryInfo
531 stub	RnaGetAreaCodeList
532 stub	RnaFindDriver
533 stub	RnaInstallDriver
534 stub	RnaGetDialSettings
535 stub	RnaSetDialSettings
536 stub	RnaGetIPInfo
537 stub	RnaSetIPInfo

560 stub	RnaCloseMac
561 stub	RnaComplete
562 stub	RnaGetDevicePort
563 stub	RnaGetUserProfile
564 stub	RnaOpenMac
565 stub	RnaSessInitialize
566 stub	RnaStartCallback
567 stub	RnaTerminate
568 stub	RnaUICallbackDialog
569 stub	RnaUIUsernamePassword
