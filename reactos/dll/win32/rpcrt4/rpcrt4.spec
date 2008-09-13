@ stdcall CreateProxyFromTypeInfo(ptr ptr ptr ptr ptr)
@ stub CreateStubFromTypeInfo
@ stdcall CStdStubBuffer_AddRef(ptr)
@ stdcall CStdStubBuffer_Connect(ptr ptr)
@ stdcall CStdStubBuffer_CountRefs(ptr)
@ stdcall CStdStubBuffer_DebugServerQueryInterface(ptr ptr)
@ stdcall CStdStubBuffer_DebugServerRelease(ptr ptr)
@ stdcall CStdStubBuffer_Disconnect(ptr)
@ stdcall CStdStubBuffer_Invoke(ptr ptr ptr)
@ stdcall CStdStubBuffer_IsIIDSupported(ptr ptr)
@ stdcall CStdStubBuffer_QueryInterface(ptr ptr ptr)
@ stub CreateServerInterfaceFromStub # wxp
@ stdcall DceErrorInqTextA (long ptr)
@ stdcall DceErrorInqTextW (long ptr)
@ stdcall -private DllRegisterServer()
@ stub GlobalMutexClearExternal
@ stub GlobalMutexRequestExternal
@ stdcall IUnknown_AddRef_Proxy(ptr)
@ stdcall IUnknown_QueryInterface_Proxy(ptr ptr ptr)
@ stdcall IUnknown_Release_Proxy(ptr)
@ stdcall I_RpcAbortAsyncCall(ptr long) I_RpcAsyncAbortCall
@ stdcall I_RpcAllocate(long)
@ stdcall I_RpcAsyncAbortCall(ptr long)
@ stdcall I_RpcAsyncSetHandle(ptr ptr)
@ stub I_RpcBCacheAllocate
@ stub I_RpcBCacheFree
@ stub I_RpcBindingCopy
@ stub I_RpcBindingInqConnId
@ stub I_RpcBindingInqDynamicEndPoint
@ stub I_RpcBindingInqDynamicEndPointA
@ stub I_RpcBindingInqDynamicEndPointW
@ stub I_RpcBindingInqLocalClientPID # wxp
@ stub I_RpcBindingInqSecurityContext
@ stdcall I_RpcBindingInqTransportType(ptr ptr)
@ stub I_RpcBindingInqWireIdForSnego
@ stub I_RpcBindingIsClientLocal
# 9x version of I_RpcBindingSetAsync has 3 arguments, not 2
@ stdcall I_RpcBindingSetAsync(ptr ptr)
@ stub I_RpcBindingToStaticStringBindingW
@ stub I_RpcClearMutex
@ stub I_RpcConnectionInqSockBuffSize
@ stub I_RpcConnectionSetSockBuffSize
@ stub I_RpcDeleteMutex
@ stub I_RpcEnableWmiTrace # wxp
@ stdcall I_RpcExceptionFilter(long)
@ stdcall I_RpcFree(ptr)
@ stdcall I_RpcFreeBuffer(ptr)
@ stub I_RpcFreePipeBuffer
@ stdcall I_RpcGetBuffer(ptr)
@ stub I_RpcGetBufferWithObject
@ stdcall I_RpcGetCurrentCallHandle()
@ stub I_RpcGetExtendedError
@ stub I_RpcIfInqTransferSyntaxes
@ stub I_RpcLogEvent
@ stdcall I_RpcMapWin32Status(long)
@ stub I_RpcNegotiateTransferSyntax # wxp
@ stub I_RpcNsBindingSetEntryName
@ stub I_RpcNsBindingSetEntryNameA
@ stub I_RpcNsBindingSetEntryNameW
@ stub I_RpcNsInterfaceExported
@ stub I_RpcNsInterfaceUnexported
@ stub I_RpcParseSecurity
@ stub I_RpcPauseExecution
@ stub I_RpcProxyNewConnection # wxp
@ stub I_RpcReallocPipeBuffer
@ stdcall I_RpcReceive(ptr)
@ stub I_RpcRequestMutex
@ stdcall I_RpcSend(ptr)
@ stdcall I_RpcSendReceive(ptr)
@ stub I_RpcServerAllocateIpPort
@ stub I_RpcServerInqAddressChangeFn
@ stub I_RpcServerInqLocalConnAddress # wxp
@ stub I_RpcServerInqTransportType
@ stub I_RpcServerRegisterForwardFunction
@ stub I_RpcServerSetAddressChangeFn
@ stdcall I_RpcServerStartListening(ptr) # win9x
@ stdcall I_RpcServerStopListening() # win9x
@ stub I_RpcServerUseProtseq2A
@ stub I_RpcServerUseProtseq2W
@ stub I_RpcServerUseProtseqEp2A
@ stub I_RpcServerUseProtseqEp2W
@ stub I_RpcSetAsyncHandle
@ stub I_RpcSsDontSerializeContext
@ stub I_RpcSystemFunction001 # wxp (oh, brother!)
@ stub I_RpcTransConnectionAllocatePacket
@ stub I_RpcTransConnectionFreePacket
@ stub I_RpcTransConnectionReallocPacket
@ stub I_RpcTransDatagramAllocate2
@ stub I_RpcTransDatagramAllocate
@ stub I_RpcTransDatagramFree
@ stub I_RpcTransGetThreadEvent
@ stub I_RpcTransIoCancelled
@ stub I_RpcTransServerNewConnection
@ stub I_RpcTurnOnEEInfoPropagation # wxp
@ stdcall I_RpcWindowProc(ptr long long long) # win9x
@ stub I_UuidCreate
@ stub MIDL_wchar_strcpy
@ stub MIDL_wchar_strlen
@ stub MesBufferHandleReset
@ stdcall MesDecodeBufferHandleCreate(ptr long ptr)
@ stdcall MesDecodeIncrementalHandleCreate(ptr ptr ptr)
@ stub MesEncodeDynBufferHandleCreate
@ stdcall MesEncodeFixedBufferHandleCreate(ptr long ptr ptr)
@ stdcall MesEncodeIncrementalHandleCreate(ptr ptr ptr ptr)
@ stdcall MesHandleFree(ptr)
@ stdcall MesIncrementalHandleReset(ptr ptr ptr ptr ptr long)
@ stub MesInqProcEncodingId
@ stdcall NDRCContextBinding(ptr)
@ stdcall NDRCContextMarshall(ptr ptr)
@ stdcall NDRCContextUnmarshall(ptr ptr ptr long)
@ stdcall NDRSContextMarshall2(ptr ptr ptr ptr ptr long)
@ stdcall NDRSContextMarshall(ptr ptr ptr)
@ stdcall NDRSContextMarshallEx(ptr ptr ptr ptr)
@ stdcall NDRSContextUnmarshall2(ptr ptr ptr ptr long)
@ stdcall NDRSContextUnmarshall(ptr ptr)
@ stdcall NDRSContextUnmarshallEx(ptr ptr ptr)
@ stub NDRcopy
@ stdcall NdrAllocate(ptr long)
@ stub NdrAsyncClientCall
@ stub NdrAsyncServerCall
@ stdcall NdrByteCountPointerBufferSize(ptr ptr ptr)
@ stdcall NdrByteCountPointerFree(ptr ptr ptr)
@ stdcall NdrByteCountPointerMarshall(ptr ptr ptr)
@ stdcall NdrByteCountPointerUnmarshall(ptr ptr ptr long)
@ stdcall NdrCStdStubBuffer2_Release(ptr ptr)
@ stdcall NdrCStdStubBuffer_Release(ptr ptr)
@ stdcall NdrClearOutParameters(ptr ptr ptr)
@ varargs NdrClientCall2(ptr ptr)
@ varargs NdrClientCall(ptr ptr) NdrClientCall2
@ stdcall NdrClientContextMarshall(ptr ptr long)
@ stdcall NdrClientContextUnmarshall(ptr ptr ptr)
@ stub NdrClientInitialize
@ stdcall NdrClientInitializeNew(ptr ptr ptr long)
@ stdcall NdrComplexArrayBufferSize(ptr ptr ptr)
@ stdcall NdrComplexArrayFree(ptr ptr ptr)
@ stdcall NdrComplexArrayMarshall(ptr ptr ptr)
@ stdcall NdrComplexArrayMemorySize(ptr ptr)
@ stdcall NdrComplexArrayUnmarshall(ptr ptr ptr long)
@ stdcall NdrComplexStructBufferSize(ptr ptr ptr)
@ stdcall NdrComplexStructFree(ptr ptr ptr)
@ stdcall NdrComplexStructMarshall(ptr ptr ptr)
@ stdcall NdrComplexStructMemorySize(ptr ptr)
@ stdcall NdrComplexStructUnmarshall(ptr ptr ptr long)
@ stdcall NdrConformantArrayBufferSize(ptr ptr ptr)
@ stdcall NdrConformantArrayFree(ptr ptr ptr)
@ stdcall NdrConformantArrayMarshall(ptr ptr ptr)
@ stdcall NdrConformantArrayMemorySize(ptr ptr)
@ stdcall NdrConformantArrayUnmarshall(ptr ptr ptr long)
@ stdcall NdrConformantStringBufferSize(ptr ptr ptr)
@ stdcall NdrConformantStringMarshall(ptr ptr ptr)
@ stdcall NdrConformantStringMemorySize(ptr ptr)
@ stdcall NdrConformantStringUnmarshall(ptr ptr ptr long)
@ stdcall NdrConformantStructBufferSize(ptr ptr ptr)
@ stdcall NdrConformantStructFree(ptr ptr ptr)
@ stdcall NdrConformantStructMarshall(ptr ptr ptr)
@ stdcall NdrConformantStructMemorySize(ptr ptr)
@ stdcall NdrConformantStructUnmarshall(ptr ptr ptr long)
@ stdcall NdrConformantVaryingArrayBufferSize(ptr ptr ptr)
@ stdcall NdrConformantVaryingArrayFree(ptr ptr ptr)
@ stdcall NdrConformantVaryingArrayMarshall(ptr ptr ptr)
@ stdcall NdrConformantVaryingArrayMemorySize(ptr ptr)
@ stdcall NdrConformantVaryingArrayUnmarshall(ptr ptr ptr long)
@ stdcall NdrConformantVaryingStructBufferSize(ptr ptr ptr)
@ stdcall NdrConformantVaryingStructFree(ptr ptr ptr)
@ stdcall NdrConformantVaryingStructMarshall(ptr ptr ptr)
@ stdcall NdrConformantVaryingStructMemorySize(ptr ptr)
@ stdcall NdrConformantVaryingStructUnmarshall(ptr ptr ptr long)
@ stdcall NdrContextHandleInitialize(ptr ptr)
@ stdcall NdrContextHandleSize(ptr ptr ptr)
@ stdcall NdrConvert2(ptr ptr long)
@ stdcall NdrConvert(ptr ptr)
@ stdcall NdrCorrelationFree(ptr)
@ stdcall NdrCorrelationInitialize(ptr ptr long long)
@ stdcall NdrCorrelationPass(ptr)
@ stub NdrDcomAsyncClientCall
@ stub NdrDcomAsyncStubCall
@ stdcall NdrDllCanUnloadNow(ptr)
@ stdcall NdrDllGetClassObject(ptr ptr ptr ptr ptr ptr)
@ stdcall NdrDllRegisterProxy(long ptr ptr)
@ stdcall NdrDllUnregisterProxy(long ptr ptr)
@ stdcall NdrEncapsulatedUnionBufferSize(ptr ptr ptr)
@ stdcall NdrEncapsulatedUnionFree(ptr ptr ptr)
@ stdcall NdrEncapsulatedUnionMarshall(ptr ptr ptr)
@ stdcall NdrEncapsulatedUnionMemorySize(ptr ptr)
@ stdcall NdrEncapsulatedUnionUnmarshall(ptr ptr ptr long)
@ stdcall NdrFixedArrayBufferSize(ptr ptr ptr)
@ stdcall NdrFixedArrayFree(ptr ptr ptr)
@ stdcall NdrFixedArrayMarshall(ptr ptr ptr)
@ stdcall NdrFixedArrayMemorySize(ptr ptr)
@ stdcall NdrFixedArrayUnmarshall(ptr ptr ptr long)
@ stdcall NdrFreeBuffer(ptr)
@ stdcall NdrFullPointerFree(ptr ptr)
@ stdcall NdrFullPointerInsertRefId(ptr long ptr)
@ stdcall NdrFullPointerQueryPointer(ptr ptr long ptr)
@ stdcall NdrFullPointerQueryRefId(ptr long long ptr)
@ stdcall NdrFullPointerXlatFree(ptr)
@ stdcall NdrFullPointerXlatInit(long long) 
@ stdcall NdrGetBuffer(ptr long ptr)
@ stub NdrGetDcomProtocolVersion
@ stub NdrGetPartialBuffer
@ stub NdrGetPipeBuffer
@ stub NdrGetSimpleTypeBufferAlignment # wxp
@ stub NdrGetSimpleTypeBufferSize # wxp
@ stub NdrGetSimpleTypeMemorySize # wxp
@ stub NdrGetTypeFlags # wxp
@ stub NdrGetUserMarshallInfo
@ stub NdrHardStructBufferSize #(ptr ptr ptr)
@ stub NdrHardStructFree #(ptr ptr ptr)
@ stub NdrHardStructMarshall #(ptr ptr ptr)
@ stub NdrHardStructMemorySize #(ptr ptr)
@ stub NdrHardStructUnmarshall #(ptr ptr ptr long)
@ stdcall NdrInterfacePointerBufferSize(ptr ptr ptr)
@ stdcall NdrInterfacePointerFree(ptr ptr ptr)
@ stdcall NdrInterfacePointerMarshall(ptr ptr ptr)
@ stdcall NdrInterfacePointerMemorySize(ptr ptr)
@ stdcall NdrInterfacePointerUnmarshall(ptr ptr ptr long)
@ stub NdrIsAppDoneWithPipes
@ stdcall NdrMapCommAndFaultStatus(ptr ptr ptr long)
@ stub NdrMarkNextActivePipe
@ stub NdrMesProcEncodeDecode2
@ varargs NdrMesProcEncodeDecode(ptr ptr ptr)
@ stub NdrMesSimpleTypeAlignSize
@ stub NdrMesSimpleTypeDecode
@ stub NdrMesSimpleTypeEncode
@ stub NdrMesTypeAlignSize2
@ stub NdrMesTypeAlignSize
@ stub NdrMesTypeDecode2
@ stub NdrMesTypeDecode
@ stub NdrMesTypeEncode2
@ stub NdrMesTypeEncode
@ stub NdrMesTypeFree2
@ stdcall NdrNonConformantStringBufferSize(ptr ptr ptr)
@ stdcall NdrNonConformantStringMarshall(ptr ptr ptr)
@ stdcall NdrNonConformantStringMemorySize(ptr ptr)
@ stdcall NdrNonConformantStringUnmarshall(ptr ptr ptr long)
@ stdcall NdrNonEncapsulatedUnionBufferSize(ptr ptr ptr)
@ stdcall NdrNonEncapsulatedUnionFree(ptr ptr ptr)
@ stdcall NdrNonEncapsulatedUnionMarshall(ptr ptr ptr)
@ stdcall NdrNonEncapsulatedUnionMemorySize(ptr ptr)
@ stdcall NdrNonEncapsulatedUnionUnmarshall(ptr ptr ptr long)
@ stub NdrNsGetBuffer
@ stub NdrNsSendReceive
@ stdcall NdrOleAllocate(long)
@ stdcall NdrOleFree(ptr)
@ stub NdrOutInit # wxp
@ stub NdrPartialIgnoreClientBufferSize # wxp
@ stub NdrPartialIgnoreClientMarshall # wxp
@ stub NdrPartialIgnoreServerInitialize # wxp
@ stub NdrPartialIgnoreServerUnmarshall # wxp
@ stub NdrPipePull
@ stub NdrPipePush
@ stub NdrPipeSendReceive
@ stub NdrPipesDone
@ stub NdrPipesInitialize
@ stdcall NdrPointerBufferSize(ptr ptr ptr)
@ stdcall NdrPointerFree(ptr ptr ptr)
@ stdcall NdrPointerMarshall(ptr ptr ptr)
@ stdcall NdrPointerMemorySize(ptr ptr)
@ stdcall NdrPointerUnmarshall(ptr ptr ptr long)
@ stdcall NdrProxyErrorHandler(long)
@ stdcall NdrProxyFreeBuffer(ptr ptr)
@ stdcall NdrProxyGetBuffer(ptr ptr)
@ stdcall NdrProxyInitialize(ptr ptr ptr ptr long)
@ stdcall NdrProxySendReceive(ptr ptr)
@ stdcall NdrRangeUnmarshall(ptr ptr ptr long)
@ stub NdrRpcSmClientAllocate
@ stub NdrRpcSmClientFree
@ stub NdrRpcSmSetClientToOsf
@ stub NdrRpcSsDefaultAllocate
@ stub NdrRpcSsDefaultFree
@ stub NdrRpcSsDisableAllocate
@ stub NdrRpcSsEnableAllocate
@ stdcall NdrSendReceive(ptr ptr)
@ stdcall NdrServerCall2(ptr)
@ stub NdrServerCall
@ stdcall NdrServerContextMarshall(ptr ptr long)
@ stdcall NdrServerContextNewMarshall(ptr ptr ptr ptr) # wxp
@ stdcall NdrServerContextNewUnmarshall(ptr ptr) # wxp
@ stdcall NdrServerContextUnmarshall(ptr)
@ stub NdrServerInitialize
@ stub NdrServerInitializeMarshall
@ stdcall NdrServerInitializeNew(ptr ptr ptr)
@ stub NdrServerInitializePartial # wxp
@ stub NdrServerInitializeUnmarshall
@ stub NdrServerMarshall
@ stub NdrServerUnmarshall
@ stdcall NdrSimpleStructBufferSize(ptr ptr ptr)
@ stdcall NdrSimpleStructFree(ptr ptr ptr)
@ stdcall NdrSimpleStructMarshall(ptr ptr ptr)
@ stdcall NdrSimpleStructMemorySize(ptr ptr)
@ stdcall NdrSimpleStructUnmarshall(ptr ptr ptr long)
@ stdcall NdrSimpleTypeMarshall(ptr ptr long)
@ stdcall NdrSimpleTypeUnmarshall(ptr ptr long)
@ stdcall NdrStubCall2(ptr ptr ptr ptr)
@ stub NdrStubCall
@ stdcall NdrStubForwardingFunction(ptr ptr ptr ptr)
@ stdcall NdrStubGetBuffer(ptr ptr ptr)
@ stdcall NdrStubInitialize(ptr ptr ptr ptr)
@ stub NdrStubInitializeMarshall
@ stub NdrTypeFlags # wxp
@ stub NdrTypeFree # wxp
@ stub NdrTypeMarshall # wxp
@ stub NdrTypeSize # wxp
@ stub NdrTypeUnmarshall # wxp
@ stub NdrUnmarshallBasetypeInline # wxp
@ stdcall NdrUserMarshalBufferSize(ptr ptr ptr)
@ stdcall NdrUserMarshalFree(ptr ptr ptr)
@ stdcall NdrUserMarshalMarshall(ptr ptr ptr)
@ stdcall NdrUserMarshalMemorySize(ptr ptr)
@ stub NdrUserMarshalSimpleTypeConvert
@ stdcall NdrUserMarshalUnmarshall(ptr ptr ptr long)
@ stdcall NdrVaryingArrayBufferSize(ptr ptr ptr)
@ stdcall NdrVaryingArrayFree(ptr ptr ptr)
@ stdcall NdrVaryingArrayMarshall(ptr ptr ptr)
@ stdcall NdrVaryingArrayMemorySize(ptr ptr)
@ stdcall NdrVaryingArrayUnmarshall(ptr ptr ptr long)
@ stdcall NdrXmitOrRepAsBufferSize(ptr ptr ptr)
@ stdcall NdrXmitOrRepAsFree(ptr ptr ptr)
@ stdcall NdrXmitOrRepAsMarshall(ptr ptr ptr)
@ stdcall NdrXmitOrRepAsMemorySize(ptr ptr)
@ stdcall NdrXmitOrRepAsUnmarshall(ptr ptr ptr long)
@ stub NdrpCreateProxy # wxp
@ stub NdrpCreateStub # wxp
@ stub NdrpGetProcFormatString # wxp
@ stub NdrpGetTypeFormatString # wxp
@ stub NdrpGetTypeGenCookie # wxp
@ stub NdrpMemoryIncrement # wxp
@ stub NdrpReleaseTypeFormatString # wxp
@ stub NdrpReleaseTypeGenCookie # wxp
@ stub NdrpSetRpcSsDefaults
@ stub NdrpVarVtOfTypeDesc # wxp
@ stdcall RpcAbortAsyncCall(ptr long) RpcAsyncAbortCall
@ stdcall RpcAsyncAbortCall(ptr long)
@ stdcall RpcAsyncCancelCall(ptr long)
@ stdcall RpcAsyncCompleteCall(ptr ptr)
@ stdcall RpcAsyncGetCallStatus(ptr)
@ stdcall RpcAsyncInitializeHandle(ptr long)
@ stub RpcAsyncRegisterInfo
@ stdcall RpcBindingCopy(ptr ptr)
@ stdcall RpcBindingFree(ptr)
@ stdcall RpcBindingFromStringBindingA(str  ptr)
@ stdcall RpcBindingFromStringBindingW(wstr ptr)
@ stub RpcBindingInqAuthClientA
@ stub RpcBindingInqAuthClientExA
@ stub RpcBindingInqAuthClientExW
@ stub RpcBindingInqAuthClientW
@ stdcall RpcBindingInqAuthInfoA(ptr ptr ptr ptr ptr ptr)
@ stdcall RpcBindingInqAuthInfoExA(ptr ptr ptr ptr ptr ptr long ptr)
@ stdcall RpcBindingInqAuthInfoExW(ptr ptr ptr ptr ptr ptr long ptr)
@ stdcall RpcBindingInqAuthInfoW(ptr ptr ptr ptr ptr ptr)
@ stdcall RpcBindingInqObject(ptr ptr)
@ stub RpcBindingInqOption
@ stdcall RpcBindingReset(ptr)
@ stub RpcBindingServerFromClient
@ stdcall RpcBindingSetAuthInfoA(ptr str long long ptr long)
@ stdcall RpcBindingSetAuthInfoExA(ptr str long long ptr long ptr)
@ stdcall RpcBindingSetAuthInfoExW(ptr wstr long long ptr long ptr)
@ stdcall RpcBindingSetAuthInfoW(ptr wstr long long ptr long)
@ stdcall RpcBindingSetObject(ptr ptr)
@ stdcall RpcBindingSetOption(ptr long long)
@ stdcall RpcBindingToStringBindingA(ptr ptr)
@ stdcall RpcBindingToStringBindingW(ptr ptr)
@ stdcall RpcBindingVectorFree(ptr)
@ stdcall RpcCancelAsyncCall(ptr long) RpcAsyncCancelCall
@ stdcall RpcCancelThread(ptr)
@ stub RpcCancelThreadEx
@ stub RpcCertGeneratePrincipalNameA
@ stub RpcCertGeneratePrincipalNameW
@ stdcall RpcCompleteAsyncCall(ptr ptr) RpcAsyncCompleteCall
@ stdcall RpcEpRegisterA(ptr ptr ptr str)
@ stub RpcEpRegisterNoReplaceA
@ stub RpcEpRegisterNoReplaceW
@ stub RpcEpRegisterW
@ stdcall RpcEpResolveBinding(ptr ptr)
@ stdcall RpcEpUnregister(ptr ptr ptr)
@ stub RpcErrorAddRecord # wxp
@ stub RpcErrorClearInformation # wxp
@ stub RpcErrorEndEnumeration # wxp
@ stub RpcErrorGetNextRecord # wxp
@ stub RpcErrorLoadErrorInfo # wxp
@ stub RpcErrorNumberOfRecords # wxp
@ stub RpcErrorResetEnumeration # wxp
@ stub RpcErrorSaveErrorInfo # wxp
@ stdcall RpcErrorStartEnumeration(ptr)
@ stub RpcFreeAuthorizationContext # wxp
@ stdcall RpcGetAsyncCallStatus(ptr) RpcAsyncGetCallStatus
@ stub RpcIfIdVectorFree
@ stub RpcIfInqId
@ stdcall RpcImpersonateClient(ptr)
@ stdcall RpcInitializeAsyncHandle(ptr long) RpcAsyncInitializeHandle
@ stdcall RpcMgmtEnableIdleCleanup()
@ stdcall RpcMgmtEpEltInqBegin(ptr long ptr long ptr ptr)
@ stub RpcMgmtEpEltInqDone
@ stub RpcMgmtEpEltInqNextA
@ stub RpcMgmtEpEltInqNextW
@ stub RpcMgmtEpUnregister
@ stub RpcMgmtInqComTimeout
@ stub RpcMgmtInqDefaultProtectLevel
@ stdcall RpcMgmtInqIfIds(ptr ptr)
@ stub RpcMgmtInqServerPrincNameA
@ stub RpcMgmtInqServerPrincNameW
@ stub RpcMgmtInqStats
@ stdcall RpcMgmtIsServerListening(ptr)
@ stub RpcMgmtSetAuthorizationFn
@ stdcall RpcMgmtSetCancelTimeout(long)
@ stdcall RpcMgmtSetComTimeout(ptr long)
@ stdcall RpcMgmtSetServerStackSize(long)
@ stub RpcMgmtStatsVectorFree
@ stdcall RpcMgmtStopServerListening(ptr)
@ stdcall RpcMgmtWaitServerListen()
@ stub RpcNetworkInqProtseqsA
@ stub RpcNetworkInqProtseqsW
@ stdcall RpcNetworkIsProtseqValidA(ptr)
@ stdcall RpcNetworkIsProtseqValidW(ptr)
@ stub RpcNsBindingInqEntryNameA
@ stub RpcNsBindingInqEntryNameW
@ stub RpcObjectInqType
@ stub RpcObjectSetInqFn
@ stdcall RpcObjectSetType(ptr ptr)
@ stub RpcProtseqVectorFreeA
@ stub RpcProtseqVectorFreeW
@ stdcall RpcRaiseException(long)
@ stub RpcRegisterAsyncInfo
@ stdcall RpcRevertToSelf()
@ stdcall RpcRevertToSelfEx(ptr)
@ stdcall RpcServerInqBindings(ptr)
@ stub RpcServerInqCallAttributesA # wxp
@ stub RpcServerInqCallAttributesW # wxp
@ stub RpcServerInqDefaultPrincNameA
@ stub RpcServerInqDefaultPrincNameW
@ stub RpcServerInqIf
@ stdcall RpcServerListen(long long long)
@ stdcall RpcServerRegisterAuthInfoA(str  long ptr ptr)
@ stdcall RpcServerRegisterAuthInfoW(wstr long ptr ptr)
@ stdcall RpcServerRegisterIf2(ptr ptr ptr long long long ptr)
@ stdcall RpcServerRegisterIf(ptr ptr ptr)
@ stdcall RpcServerRegisterIfEx(ptr ptr ptr long long ptr)
@ stub RpcServerTestCancel
@ stdcall RpcServerUnregisterIf(ptr ptr long)
@ stdcall RpcServerUnregisterIfEx(ptr ptr long)
@ stub RpcServerUseAllProtseqs
@ stub RpcServerUseAllProtseqsEx
@ stub RpcServerUseAllProtseqsIf
@ stub RpcServerUseAllProtseqsIfEx
@ stdcall RpcServerUseProtseqA(str long ptr)
@ stdcall RpcServerUseProtseqEpA(str  long str  ptr)
@ stdcall RpcServerUseProtseqEpExA(str  long str  ptr ptr)
@ stdcall RpcServerUseProtseqEpExW(wstr long wstr ptr ptr)
@ stdcall RpcServerUseProtseqEpW(wstr long wstr ptr)
@ stub RpcServerUseProtseqExA
@ stub RpcServerUseProtseqExW
@ stub RpcServerUseProtseqIfA
@ stub RpcServerUseProtseqIfExA
@ stub RpcServerUseProtseqIfExW
@ stub RpcServerUseProtseqIfW
@ stdcall RpcServerUseProtseqW(wstr long ptr)
@ stub RpcServerYield
@ stub RpcSmAllocate
@ stub RpcSmClientFree
@ stdcall RpcSmDestroyClientContext(ptr)
@ stub RpcSmDisableAllocate
@ stub RpcSmEnableAllocate
@ stub RpcSmFree
@ stub RpcSmGetThreadHandle
@ stub RpcSmSetClientAllocFree
@ stub RpcSmSetThreadHandle
@ stub RpcSmSwapClientAllocFree
@ stub RpcSsAllocate
@ stub RpcSsContextLockExclusive # wxp
@ stub RpcSsContextLockShared # wxp
@ stdcall RpcSsDestroyClientContext(ptr)
@ stub RpcSsDisableAllocate
@ stub RpcSsDontSerializeContext
@ stub RpcSsEnableAllocate
@ stub RpcSsFree
@ stub RpcSsGetContextBinding
@ stub RpcSsGetThreadHandle
@ stub RpcSsSetClientAllocFree
@ stub RpcSsSetThreadHandle
@ stub RpcSsSwapClientAllocFree
@ stdcall RpcStringBindingComposeA(str  str  str  str  str  ptr)
@ stdcall RpcStringBindingComposeW(wstr wstr wstr wstr wstr ptr)
@ stdcall RpcStringBindingParseA(str  ptr ptr ptr ptr ptr)
@ stdcall RpcStringBindingParseW(wstr ptr ptr ptr ptr ptr)
@ stdcall RpcStringFreeA(ptr)
@ stdcall RpcStringFreeW(ptr)
@ stub RpcTestCancel
@ stub RpcUserFree # wxp
@ stub SimpleTypeAlignment # wxp
@ stub SimpleTypeBufferSize # wxp
@ stub SimpleTypeMemorySize # wxp
@ stdcall TowerConstruct(ptr ptr ptr ptr ptr ptr)
@ stdcall TowerExplode(ptr ptr ptr ptr ptr ptr)
@ stdcall UuidCompare(ptr ptr ptr)
@ stdcall UuidCreate(ptr)
@ stdcall UuidCreateNil(ptr)
@ stdcall UuidCreateSequential(ptr) # win 2000
@ stdcall UuidEqual(ptr ptr ptr)
@ stdcall UuidFromStringA(str ptr)
@ stdcall UuidFromStringW(wstr ptr)
@ stdcall UuidHash(ptr ptr)
@ stdcall UuidIsNil(ptr ptr)
@ stdcall UuidToStringA(ptr ptr)
@ stdcall UuidToStringW(ptr ptr)
@ stub char_array_from_ndr
@ stub char_from_ndr
@ stub data_from_ndr
@ stub data_into_ndr
@ stub data_size_ndr
@ stub double_array_from_ndr
@ stub double_from_ndr
@ stub enum_from_ndr
@ stub float_array_from_ndr
@ stub float_from_ndr
@ stub long_array_from_ndr
@ stub long_from_ndr
@ stub long_from_ndr_temp
@ stub pfnFreeRoutines # wxp
@ stub pfnMarshallRouteines # wxp
@ stub pfnSizeRoutines # wxp
@ stub pfnUnmarshallRouteines # wxp
@ stub short_array_from_ndr
@ stub short_from_ndr
@ stub short_from_ndr_temp
@ stub tree_into_ndr
@ stub tree_peek_ndr
@ stub tree_size_ndr
