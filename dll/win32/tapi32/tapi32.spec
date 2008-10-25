@ stdcall lineAccept(long str long)
@ stdcall lineAddProvider(str long ptr) lineAddProviderA
@ stdcall lineAddProviderA(str long ptr)
@ stdcall lineAddToConference(long long)
@ stdcall lineAnswer(long str long)
@ stdcall lineBlindTransfer(long str long) lineBlindTransferA
@ stdcall lineBlindTransferA(long str long)
@ stdcall lineClose(long)
@ stdcall lineCompleteCall(long ptr long long)
@ stdcall lineCompleteTransfer(long long ptr long)
@ stdcall lineConfigDialog(long long str) lineConfigDialogA
@ stdcall lineConfigDialogA(long long str)
@ stdcall lineConfigDialogEdit(long long str ptr long ptr) lineConfigDialogEditA
@ stdcall lineConfigDialogEditA(long long str ptr long ptr)
@ stdcall lineConfigProvider(long long)
@ stdcall lineDeallocateCall(long)
@ stdcall lineDevSpecific(long long long ptr long)
@ stdcall lineDevSpecificFeature(long long ptr long)
@ stdcall lineDial(long str long) lineDialA
@ stdcall lineDialA(long str long)
@ stdcall lineDrop(long str long)
@ stdcall lineForward(long long long ptr long ptr ptr) lineForwardA
@ stdcall lineForwardA(long long long ptr long ptr ptr)
@ stdcall lineGatherDigits(long long str long str long long) lineGatherDigitsA
@ stdcall lineGatherDigitsA(long long str long str long long)
@ stdcall lineGenerateDigits(long long str long) lineGenerateDigitsA
@ stdcall lineGenerateDigitsA(long long str long)
@ stdcall lineGenerateTone(long long long long ptr)
@ stdcall lineGetAddressCaps(long long long long long ptr) lineGetAddressCapsA
@ stdcall lineGetAddressCapsA(long long long long long ptr)
@ stdcall lineGetAddressID(long ptr long str long) lineGetAddressIDA
@ stdcall lineGetAddressIDA(long ptr long str long)
@ stdcall lineGetAddressStatus(long long ptr) lineGetAddressStatusA
@ stdcall lineGetAddressStatusA(long long ptr)
@ stdcall lineGetAppPriority(str long ptr long ptr ptr) lineGetAppPriorityA
@ stdcall lineGetAppPriorityA(str long ptr long ptr ptr)
@ stdcall lineGetCallInfo(long ptr) lineGetCallInfoA
@ stdcall lineGetCallInfoA(long ptr)
@ stdcall lineGetCallStatus(long ptr)
@ stdcall lineGetConfRelatedCalls(long ptr)
@ stdcall lineGetCountry(long long ptr) lineGetCountryA
@ stdcall lineGetCountryA(long long ptr)
@ stdcall lineGetDevCaps(long long long long ptr) lineGetDevCapsA
@ stdcall lineGetDevCapsA(long long long long ptr)
@ stdcall lineGetDevConfig(long ptr str) lineGetDevConfigA
@ stdcall lineGetDevConfigA(long ptr str)
@ stdcall lineGetID(long long long long ptr str) lineGetIDA
@ stdcall lineGetIDA(long long long long ptr str)
@ stdcall lineGetIcon(long str ptr) lineGetIconA
@ stdcall lineGetIconA(long str ptr)
@ stdcall lineGetLineDevStatus(long ptr) lineGetLineDevStatusA
@ stdcall lineGetLineDevStatusA(long ptr)
@ stdcall lineGetNewCalls(long long long ptr)
@ stdcall lineGetNumRings(long long ptr)
@ stdcall lineGetProviderList(long ptr) lineGetProviderListA
@ stdcall lineGetProviderListA(long ptr)
@ stdcall lineGetRequest(long long ptr) lineGetRequestA
@ stdcall lineGetRequestA(long long ptr)
@ stdcall lineGetStatusMessages(long ptr ptr)
@ stdcall lineGetTranslateCaps(long long ptr) lineGetTranslateCapsA
@ stdcall lineGetTranslateCapsA(long long ptr)
@ stdcall lineHandoff(long str long) lineHandoffA
@ stdcall lineHandoffA(long str long)
@ stdcall lineHold(long)
@ stdcall lineInitialize(ptr long ptr str ptr)
@ stdcall lineInitializeExA(ptr long ptr str ptr ptr ptr)
@ stdcall lineMakeCall(long ptr str long ptr) lineMakeCallA
@ stdcall lineMakeCallA(long ptr str long ptr)
@ stdcall lineMonitorDigits(long long)
@ stdcall lineMonitorMedia(long long)
@ stdcall lineMonitorTones(long ptr long)
@ stdcall lineNegotiateAPIVersion(long long long long ptr ptr)
@ stdcall lineNegotiateExtVersion(long long long long long ptr)
@ stdcall lineOpen(long long ptr long long long long long ptr) lineOpenA
@ stdcall lineOpenA(long long ptr long long long long long ptr)
@ stdcall linePark(long long str ptr) lineParkA
@ stdcall lineParkA(long long str ptr)
@ stdcall linePickup(long long ptr str str) linePickupA
@ stdcall linePickupA(long long ptr str str)
@ stdcall linePrepareAddToConference(long ptr ptr) linePrepareAddToConferenceA
@ stdcall linePrepareAddToConferenceA(long ptr ptr)
@ stdcall lineRedirect(long str long) lineRedirectA
@ stdcall lineRedirectA(long str long)
@ stdcall lineRegisterRequestRecipient(long long long long)
@ stdcall lineReleaseUserUserInfo(long)
@ stdcall lineRemoveFromConference(long)
@ stdcall lineRemoveProvider(long long)
@ stdcall lineSecureCall(long)
@ stdcall lineSendUserUserInfo(long str long)
@ stdcall lineSetAppPriority(str long ptr long str long) lineSetAppPriorityA
@ stdcall lineSetAppPriorityA(str long ptr long str long)
@ stdcall lineSetAppSpecific(long long)
@ stdcall lineSetCallParams(long long long long ptr)
@ stdcall lineSetCallPrivilege(long long)
@ stdcall lineSetCurrentLocation(long long)
@ stdcall lineSetDevConfig(long ptr long str) lineSetDevConfigA
@ stdcall lineSetDevConfigA(long ptr long str)
@ stdcall lineSetMediaControl(long long long long ptr long ptr long ptr long ptr long)
@ stdcall lineSetMediaMode(long long)
@ stdcall lineSetNumRings(long long long)
@ stdcall lineSetStatusMessages(long long long)
@ stdcall lineSetTerminal(long long long long long long long)
@ stdcall lineSetTollList(long long str long) lineSetTollListA
@ stdcall lineSetTollListA(long long str long)
@ stdcall lineSetupConference(long long ptr ptr long ptr) lineSetupConferenceA
@ stdcall lineSetupConferenceA(long long ptr ptr long ptr)
@ stdcall lineSetupTransfer(long ptr ptr) lineSetupTransferA
@ stdcall lineSetupTransferA(long ptr ptr)
@ stdcall lineShutdown(long)
@ stdcall lineSwapHold(long long)
@ stdcall lineTranslateAddress(long long long str long long ptr) lineTranslateAddressA
@ stdcall lineTranslateAddressA(long long long str long long ptr)
@ stdcall lineTranslateDialog(long long long long str) lineTranslateDialogA
@ stdcall lineTranslateDialogA(long long long long str)
@ stdcall lineUncompleteCall(long long)
@ stdcall lineUnhold(long)
@ stdcall lineUnpark(long long ptr str) lineUnparkA
@ stdcall lineUnparkA(long long ptr str)
@ stdcall phoneClose(long)
@ stdcall phoneConfigDialog(long long str) phoneConfigDialogA
@ stdcall phoneConfigDialogA(long long str)
@ stdcall phoneDevSpecific(long ptr long)
@ stdcall phoneGetButtonInfo(long long ptr) phoneGetButtonInfoA
@ stdcall phoneGetButtonInfoA(long long ptr)
@ stdcall phoneGetData(long long ptr long)
@ stdcall phoneGetDevCaps(long long long long ptr) phoneGetDevCapsA
@ stdcall phoneGetDevCapsA(long long long long ptr)
@ stdcall phoneGetDisplay(long ptr)
@ stdcall phoneGetGain(long long ptr)
@ stdcall phoneGetHookSwitch(long ptr)
@ stdcall phoneGetID(long ptr str) phoneGetIDA
@ stdcall phoneGetIDA(long ptr str)
@ stdcall phoneGetIcon(long str ptr) phoneGetIconA
@ stdcall phoneGetIconA(long str ptr)
@ stdcall phoneGetLamp(long long ptr)
@ stdcall phoneGetRing(long ptr ptr)
@ stdcall phoneGetStatus(long ptr) phoneGetStatusA
@ stdcall phoneGetStatusA(long ptr)
@ stdcall phoneGetStatusMessages(long ptr ptr ptr)
@ stdcall phoneGetVolume(long long ptr)
@ stdcall phoneInitialize(ptr long ptr str ptr)
@ stdcall phoneNegotiateAPIVersion(long long long long ptr ptr)
@ stdcall phoneNegotiateExtVersion(long long long long long ptr)
@ stdcall phoneOpen(long long ptr long long long long)
@ stdcall phoneSetButtonInfo(long long ptr) phoneSetButtonInfoA
@ stdcall phoneSetButtonInfoA(long long ptr)
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
@ stub    tapiRequestDrop
@ stdcall tapiRequestMakeCall(str str str str) tapiRequestMakeCallA
@ stdcall tapiRequestMakeCallA(str str str str)
@ stub    tapiRequestMediaCall
