@ stub RasAutodialAddressToNetwork
@ stub RasAutodialEntryToNetwork
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
@ stdcall RasEnumEntriesW(str str ptr ptr ptr)
@ stub RasEnumEntriesWow
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
@ stub RasGetCountryInfoA
@ stub RasGetCountryInfoW
@ stub RasGetCredentialsA
@ stub RasGetCredentialsW
@ stdcall RasGetEntryDialParamsA(str ptr ptr)
@ stdcall RasGetEntryDialParamsW(wstr ptr ptr)
@ stdcall RasGetEntryPropertiesA(str str ptr ptr ptr ptr)
@ stdcall RasGetEntryPropertiesW(wstr wstr ptr ptr ptr ptr)
@ stdcall RasGetErrorStringA(long ptr long)
@ stdcall RasGetErrorStringW(long ptr long)
@ stub RasGetErrorStringWow
@ stub RasGetHport
@ stdcall RasGetProjectionInfoA(ptr ptr ptr ptr)
@ stdcall RasGetProjectionInfoW(ptr ptr ptr ptr)
@ stub RasGetSubEntryHandleA
@ stub RasGetSubEntryHandleW
@ stub RasGetSubEntryPropertiesA
@ stub RasGetSubEntryPropertiesW
@ stdcall RasHangUpA(long)
@ stdcall RasHangUpW(long)
@ stub RasHangUpWow
@ stdcall RasRenameEntryA(str str str)
@ stdcall RasRenameEntryW(wstr wstr wstr)
@ stdcall RasSetAutodialAddressA(str long ptr long long)
@ stdcall RasSetAutodialAddressW(wstr long ptr long long)
@ stdcall RasSetAutodialEnableA(long long)
@ stdcall RasSetAutodialEnableW(long long)
@ stdcall RasSetAutodialParamA(long ptr long)
@ stdcall RasSetAutodialParamW(long ptr long)
@ stub RasSetCredentialsA
@ stub RasSetCredentialsW
@ stdcall RasSetEntryDialParamsA(str ptr long)
@ stdcall RasSetEntryDialParamsW(wstr ptr long)
@ stdcall RasSetEntryPropertiesA(str str ptr long ptr long)
@ stdcall RasSetEntryPropertiesW(wstr wstr ptr long ptr long)
@ stub RasSetOldPassword
@ stdcall RasSetSubEntryPropertiesA(str str long ptr long ptr long)
@ stdcall RasSetSubEntryPropertiesW(wstr wstr long ptr long ptr long)
@ stdcall RasValidateEntryNameA(str str)
@ stdcall RasValidateEntryNameW(wstr wstr)

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
