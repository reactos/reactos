@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stub DockPattern_SetDockPosition
@ stub ExpandCollapsePattern_Collapse
@ stub ExpandCollapsePattern_Expand
@ stub GridPattern_GetItem
#@ stub IgnoreLeaksInCurrentlyTrackedMemory
@ stub InvokePattern_Invoke
@ stub ItemContainerPattern_FindItemByProperty
@ stub LegacyIAccessiblePattern_DoDefaultAction
@ stub LegacyIAccessiblePattern_GetIAccessible
@ stub LegacyIAccessiblePattern_Select
@ stub LegacyIAccessiblePattern_SetValue
@ stub MultipleViewPattern_GetViewName
@ stub MultipleViewPattern_SetCurrentView
#@ stub PostTestCheckForLeaks
@ stub RangeValuePattern_SetValue
@ stub ScrollItemPattern_ScrollIntoView
@ stub ScrollPattern_Scroll
@ stub ScrollPattern_SetScrollPercent
@ stub SelectionItemPattern_AddToSelection
@ stub SelectionItemPattern_RemoveFromSelection
@ stub SelectionItemPattern_Select
@ stub SynchronizedInputPattern_Cancel
@ stub SynchronizedInputPattern_StartListening
@ stub TextPattern_GetSelection
@ stub TextPattern_GetVisibleRanges
@ stub TextPattern_RangeFromChild
@ stub TextPattern_RangeFromPoint
@ stub TextPattern_get_DocumentRange
@ stub TextPattern_get_SupportedTextSelection
@ stub TextRange_AddToSelection
@ stub TextRange_Clone
@ stub TextRange_Compare
@ stub TextRange_CompareEndpoints
@ stub TextRange_ExpandToEnclosingUnit
@ stub TextRange_FindAttribute
@ stub TextRange_FindText
@ stub TextRange_GetAttributeValue
@ stub TextRange_GetBoundingRectangles
@ stub TextRange_GetChildren
@ stub TextRange_GetEnclosingElement
@ stub TextRange_GetText
@ stub TextRange_Move
@ stub TextRange_MoveEndpointByRange
@ stub TextRange_MoveEndpointByUnit
@ stub TextRange_RemoveFromSelection
@ stub TextRange_ScrollIntoView
@ stub TextRange_Select
@ stub TogglePattern_Toggle
@ stub TransformPattern_Move
@ stub TransformPattern_Resize
@ stub TransformPattern_Rotate
@ stdcall UiaAddEvent(ptr long ptr long ptr long ptr ptr)
@ stdcall UiaClientsAreListening()
#@ stub UiaDisconnectAllProviders
@ stdcall UiaDisconnectProvider(ptr)
@ stdcall UiaEventAddWindow(ptr long)
@ stdcall UiaEventRemoveWindow(ptr long)
@ stdcall UiaFind(ptr ptr ptr ptr ptr ptr)
@ stub UiaGetErrorDescription
@ stub UiaGetPatternProvider
@ stdcall UiaGetPropertyValue(ptr long ptr)
@ stdcall UiaGetReservedMixedAttributeValue(ptr)
@ stdcall UiaGetReservedNotSupportedValue(ptr)
@ stdcall UiaGetRootNode(ptr)
@ stdcall UiaGetRuntimeId(ptr ptr)
@ stdcall UiaGetUpdatedCache(ptr ptr long ptr ptr ptr)
@ stub UiaHPatternObjectFromVariant
@ stub UiaHTextRangeFromVariant
@ stdcall UiaHUiaNodeFromVariant(ptr ptr)
@ stdcall UiaHasServerSideProvider(long)
@ stdcall UiaHostProviderFromHwnd(long ptr)
#@ stub UiaIAccessibleFromProvider
@ stdcall UiaLookupId(long ptr)
@ stdcall UiaNavigate(ptr long ptr ptr ptr ptr)
@ stdcall UiaNodeFromFocus(ptr ptr ptr)
@ stdcall UiaNodeFromHandle(long ptr)
@ stub UiaNodeFromPoint
@ stdcall UiaNodeFromProvider(ptr ptr)
@ stdcall UiaNodeRelease(ptr)
@ stub UiaPatternRelease
#@ stub UiaProviderForNonClient
@ stdcall UiaProviderFromIAccessible(ptr long long ptr)
@ stdcall UiaRaiseAsyncContentLoadedEvent(ptr long double)
@ stdcall UiaRaiseAutomationEvent(ptr long)
@ stdcall UiaRaiseAutomationPropertyChangedEvent(ptr long int128 int128)
@ stdcall UiaRaiseChangesEvent(ptr long ptr)
@ stdcall UiaRaiseNotificationEvent(ptr long long wstr wstr)
@ stdcall UiaRaiseStructureChangedEvent(ptr long ptr long)
@ stdcall UiaRaiseTextEditTextChangedEvent(ptr long ptr)
@ stdcall UiaRegisterProviderCallback(ptr)
@ stdcall UiaRemoveEvent(ptr)
@ stdcall UiaReturnRawElementProvider(long long long ptr)
@ stub UiaSetFocus
@ stub UiaTextRangeRelease
#@ stub UpdateErrorLoggingCallback
@ stub ValuePattern_SetValue
@ stub VirtualizedItemPattern_Realize
@ stub WindowPattern_Close
@ stub WindowPattern_SetWindowVisualState
@ stub WindowPattern_WaitForInputIdle
