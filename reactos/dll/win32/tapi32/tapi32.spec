@ stub GetTapi16CallbackMsg
@ stub LAddrParamsInited
@ stub LOpenDialAsst
@ stub LocWizardDlgProc
@ stub MMCAddProvider
@ stub MMCConfigProvider
@ stub MMCGetAvailableProviders
@ stub MMCGetDeviceFlags
@ stub MMCGetLineInfo
@ stub MMCGetLineStatus
@ stub MMCGetPhoneInfo
@ stub MMCGetPhoneStatus
@ stub MMCGetProviderList
@ stub MMCGetServerConfig
@ stub MMCInitialize
@ stub MMCRemoveProvider
@ stub MMCSetLineInfo
@ stub MMCSetPhoneInfo
@ stub MMCSetServerConfig
@ stub MMCShutdown
@ stub NonAsyncEventThread
@ stub TAPIWndProc
@ stub TUISPIDLLCallback
@ stub internalConfig
@ stub internalCreateDefLocation
@ stub internalNewLocationW
@ stub internalPerformance
@ stub internalRemoveLocation
@ stub internalRenameLocationW
@ stdcall lineAccept(long str long)
@ stdcall lineAddProvider(str long ptr) lineAddProviderA
@ stdcall lineAddProviderA(str long ptr)
@ stdcall lineAddToConference(long long)
@ stub lineAgentSpecific
@ stdcall lineAnswer(long str long)
@ stdcall lineBlindTransfer(long str long) lineBlindTransferA
@ stdcall lineBlindTransferA(long str long)
@ stub lineBlindTransferW
@ stdcall lineClose(long)
@ stdcall lineCompleteCall(long ptr long long)
@ stdcall lineCompleteTransfer(long long ptr long)
@ stdcall lineConfigDialog(long long str) lineConfigDialogA
@ stdcall lineConfigDialogA(long long str)
@ stdcall lineConfigDialogEdit(long long str ptr long ptr) lineConfigDialogEditA
@ stdcall lineConfigDialogEditA(long long str ptr long ptr)
@ stub lineConfigDialogEditW
@ stub lineConfigDialogW
@ stdcall lineConfigProvider(long long)
@ stub lineCreateAgentA
@ stub lineCreateAgentSessionA
@ stub lineCreateAgentSessionW
@ stub lineCreateAgentW
@ stdcall lineDeallocateCall(long)
@ stdcall lineDevSpecific(long long long ptr long)
@ stdcall lineDevSpecificFeature(long long ptr long)
@ stdcall lineDial(long str long) lineDialA
@ stdcall lineDialA(long str long)
@ stub lineDialW
@ stdcall lineDrop(long str long)
@ stdcall lineForward(long long long ptr long ptr ptr) lineForwardA
@ stdcall lineForwardA(long long long ptr long ptr ptr)
@ stub lineForwardW
@ stdcall lineGatherDigits(long long str long str long long) lineGatherDigitsA
@ stdcall lineGatherDigitsA(long long str long str long long)
@ stub lineGatherDigitsW
@ stdcall lineGenerateDigits(long long str long) lineGenerateDigitsA
@ stdcall lineGenerateDigitsA(long long str long)
@ stub lineGenerateDigitsW
@ stdcall lineGenerateTone(long long long long ptr)
@ stdcall lineGetAddressCaps(long long long long long ptr) lineGetAddressCapsA
@ stdcall lineGetAddressCapsA(long long long long long ptr)
@ stub lineGetAddressCapsW
@ stdcall lineGetAddressID(long ptr long str long) lineGetAddressIDA
@ stdcall lineGetAddressIDA(long ptr long str long)
@ stub lineGetAddressIDW
@ stdcall lineGetAddressStatus(long long ptr) lineGetAddressStatusA
@ stdcall lineGetAddressStatusA(long long ptr)
@ stub lineGetAddressStatusW
@ stub lineGetAgentActivityListA
@ stub lineGetAgentActivityListW
@ stub lineGetAgentCapsA
@ stub lineGetAgentCapsW
@ stub lineGetAgentGroupListA
@ stub lineGetAgentGroupListW
@ stub lineGetAgentInfo
@ stub lineGetAgentSessionInfo
@ stub lineGetAgentSessionList
@ stub lineGetAgentStatusA
@ stub lineGetAgentStatusW
@ stdcall lineGetAppPriority(str long ptr long ptr ptr) lineGetAppPriorityA
@ stdcall lineGetAppPriorityA(str long ptr long ptr ptr)
@ stub lineGetAppPriorityW
@ stdcall lineGetCallInfo(long ptr) lineGetCallInfoA
@ stdcall lineGetCallInfoA(long ptr)
@ stub lineGetCallInfoW
@ stdcall lineGetCallStatus(long ptr)
@ stdcall lineGetConfRelatedCalls(long ptr)
@ stdcall lineGetCountry(long long ptr) lineGetCountryA
@ stdcall lineGetCountryA(long long ptr)
@ stub lineGetCountryW
@ stdcall lineGetDevCaps(long long long long ptr) lineGetDevCapsA
@ stdcall lineGetDevCapsA(long long long long ptr)
@ stub lineGetDevCapsW
@ stdcall lineGetDevConfig(long ptr str) lineGetDevConfigA
@ stdcall lineGetDevConfigA(long ptr str)
@ stub lineGetDevConfigW
@ stub lineGetGroupListA
@ stub lineGetGroupListW
@ stdcall lineGetID(long long long long ptr str) lineGetIDA
@ stdcall lineGetIDA(long long long long ptr str)
@ stub lineGetIDW
@ stdcall lineGetIcon(long str ptr) lineGetIconA
@ stdcall lineGetIconA(long str ptr)
@ stub lineGetIconW
@ stdcall lineGetLineDevStatus(long ptr) lineGetLineDevStatusA
@ stdcall lineGetLineDevStatusA(long ptr)
@ stub lineGetLineDevStatusW
@ stub lineGetMessage
@ stdcall lineGetNewCalls(long long long ptr)
@ stdcall lineGetNumRings(long long ptr)
@ stdcall lineGetProviderList(long ptr) lineGetProviderListA
@ stdcall lineGetProviderListA(long ptr)
@ stub lineGetProviderListW
@ stub lineGetProxyStatus
@ stub lineGetQueueInfo
@ stub lineGetQueueInfoA
@ stub lineGetQueueInfoW
@ stdcall lineGetRequest(long long ptr) lineGetRequestA
@ stdcall lineGetRequestA(long long ptr)
@ stub lineGetRequestW
@ stdcall lineGetStatusMessages(long ptr ptr)
@ stdcall lineGetTranslateCaps(long long ptr) lineGetTranslateCapsA
@ stdcall lineGetTranslateCapsA(long long ptr)
@ stub lineGetTranslateCapsW
@ stdcall lineHandoff(long str long) lineHandoffA
@ stdcall lineHandoffA(long str long)
@ stub lineHandoffW
@ stdcall lineHold(long)
@ stdcall lineInitialize(ptr long ptr str ptr)
@ stdcall lineInitializeExA(ptr long ptr str ptr ptr ptr)
@ stub lineInitializeExW
@ stdcall lineMakeCall(long ptr str long ptr) lineMakeCallA
@ stdcall lineMakeCallA(long ptr str long ptr)
@ stub lineMakeCallW
@ stdcall lineMonitorDigits(long long)
@ stdcall lineMonitorMedia(long long)
@ stdcall lineMonitorTones(long ptr long)
@ stdcall lineNegotiateAPIVersion(long long long long ptr ptr)
@ stdcall lineNegotiateExtVersion(long long long long long ptr)
@ stdcall lineOpen(long long ptr long long long long long ptr) lineOpenA
@ stdcall lineOpenA(long long ptr long long long long long ptr)
@ stub lineOpenW
@ stdcall linePark(long long str ptr) lineParkA
@ stdcall lineParkA(long long str ptr)
@ stub lineParkW
@ stdcall linePickup(long long ptr str str) linePickupA
@ stdcall linePickupA(long long ptr str str)
@ stub linePickupW
@ stdcall linePrepareAddToConference(long ptr ptr) linePrepareAddToConferenceA
@ stdcall linePrepareAddToConferenceA(long ptr ptr)
@ stub linePrepareAddToConferenceW
@ stub lineProxyMessage
@ stub lineProxyResponse
@ stdcall lineRedirect(long str long) lineRedirectA
@ stdcall lineRedirectA(long str long)
@ stub lineRedirectW
@ stdcall lineRegisterRequestRecipient(long long long long)
@ stdcall lineReleaseUserUserInfo(long)
@ stdcall lineRemoveFromConference(long)
@ stdcall lineRemoveProvider(long long)
@ stdcall lineSecureCall(long)
@ stdcall lineSendUserUserInfo(long str long)
@ stub lineSetAgentActivity
@ stub lineSetAgentGroup
@ stub lineSetAgentMeasurementPeriod
@ stub lineSetAgentSessionState
@ stub lineSetAgentState
@ stub lineSetAgentStateEx
@ stdcall lineSetAppPriority(str long ptr long str long) lineSetAppPriorityA
@ stdcall lineSetAppPriorityA(str long ptr long str long)
@ stub lineSetAppPriorityW
@ stdcall lineSetAppSpecific(long long)
@ stub lineSetCallData
@ stdcall lineSetCallParams(long long long long ptr)
@ stdcall lineSetCallPrivilege(long long)
@ stub lineSetCallQualityOfService
@ stub lineSetCallTreatment
@ stdcall lineSetCurrentLocation(long long)
@ stdcall lineSetDevConfig(long ptr long str) lineSetDevConfigA
@ stdcall lineSetDevConfigA(long ptr long str)
@ stub lineSetDevConfigW
@ stub lineSetLineDevStatus
@ stdcall lineSetMediaControl(long long long long ptr long ptr long ptr long ptr long)
@ stdcall lineSetMediaMode(long long)
@ stdcall lineSetNumRings(long long long)
@ stub lineSetQueueMeasurementPeriod
@ stdcall lineSetStatusMessages(long long long)
@ stdcall lineSetTerminal(long long long long long long long)
@ stdcall lineSetTollList(long long str long) lineSetTollListA
@ stdcall lineSetTollListA(long long str long)
@ stub lineSetTollListW
@ stdcall lineSetupConference(long long ptr ptr long ptr) lineSetupConferenceA
@ stdcall lineSetupConferenceA(long long ptr ptr long ptr)
@ stub lineSetupConferenceW
@ stdcall lineSetupTransfer(long ptr ptr) lineSetupTransferA
@ stdcall lineSetupTransferA(long ptr ptr)
@ stub lineSetupTransferW
@ stdcall lineShutdown(long)
@ stdcall lineSwapHold(long long)
@ stdcall lineTranslateAddress(long long long str long long ptr) lineTranslateAddressA
@ stdcall lineTranslateAddressA(long long long str long long ptr)
@ stub lineTranslateAddressW
@ stdcall lineTranslateDialog(long long long long str) lineTranslateDialogA
@ stdcall lineTranslateDialogA(long long long long str)
@ stub lineTranslateDialogW
@ stdcall lineUncompleteCall(long long)
@ stdcall lineUnhold(long)
@ stdcall lineUnpark(long long ptr str) lineUnparkA
@ stdcall lineUnparkA(long long ptr str)
@ stub lineUnparkW
@ stdcall phoneClose(long)
@ stdcall phoneConfigDialog(long long str) phoneConfigDialogA
@ stdcall phoneConfigDialogA(long long str)
@ stub phoneConfigDialogW
@ stdcall phoneDevSpecific(long ptr long)
@ stdcall phoneGetButtonInfo(long long ptr) phoneGetButtonInfoA
@ stdcall phoneGetButtonInfoA(long long ptr)
@ stub phoneGetButtonInfoW
@ stdcall phoneGetData(long long ptr long)
@ stdcall phoneGetDevCaps(long long long long ptr) phoneGetDevCapsA
@ stdcall phoneGetDevCapsA(long long long long ptr)
@ stub phoneGetDevCapsW
@ stdcall phoneGetDisplay(long ptr)
@ stdcall phoneGetGain(long long ptr)
@ stdcall phoneGetHookSwitch(long ptr)
@ stdcall phoneGetID(long ptr str) phoneGetIDA
@ stdcall phoneGetIDA(long ptr str)
@ stub phoneGetIDW
@ stdcall phoneGetIcon(long str ptr) phoneGetIconA
@ stdcall phoneGetIconA(long str ptr)
@ stub phoneGetIconW
@ stdcall phoneGetLamp(long long ptr)
@ stub phoneGetMessage
@ stdcall phoneGetRing(long ptr ptr)
@ stdcall phoneGetStatus(long ptr) phoneGetStatusA
@ stdcall phoneGetStatusA(long ptr)
@ stdcall phoneGetStatusMessages(long ptr ptr ptr)
@ stub phoneGetStatusW
@ stdcall phoneGetVolume(long long ptr)
@ stdcall phoneInitialize(ptr long ptr str ptr)
@ stub phoneInitializeExA
@ stub phoneInitializeExW
@ stdcall phoneNegotiateAPIVersion(long long long long ptr ptr)
@ stdcall phoneNegotiateExtVersion(long long long long long ptr)
@ stdcall phoneOpen(long long ptr long long long long)
@ stdcall phoneSetButtonInfo(long long ptr) phoneSetButtonInfoA
@ stdcall phoneSetButtonInfoA(long long ptr)
@ stub phoneSetButtonInfoW
@ stdcall phoneSetData(long long ptr long)
@ stdcall phoneSetDisplay(long long long str long)
@ stdcall phoneSetGain(long long long)
@ stdcall phoneSetHookSwitch(long long long)
@ stdcall phoneSetLamp(long long long)
@ stdcall phoneSetRing(long long long)
@ stdcall phoneSetStatusMessages(long long long long)
@ stdcall phoneSetVolume(long long long)
@ stdcall phoneShutdown(long)
@ stdcall tapiGetLocationInfo(str str) tapiGetLocationInfoA
@ stdcall tapiGetLocationInfoA(str str)
@ stub tapiGetLocationInfoW
@ stub tapiRequestDrop
@ stdcall tapiRequestMakeCall(str str str str) tapiRequestMakeCallA
@ stdcall tapiRequestMakeCallA(str str str str)
@ stub tapiRequestMakeCallW
@ stub tapiRequestMediaCall
@ stub tapiRequestMediaCallA
@ stub tapiRequestMediaCallW
