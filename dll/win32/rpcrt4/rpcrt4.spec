1 stdcall CreateProxyFromTypeInfo(ptr ptr ptr ptr ptr)
2 stdcall CreateStubFromTypeInfo(ptr ptr ptr ptr)
# I_RpcServerTurnOnOffKeepalives
4 stdcall CStdStubBuffer_AddRef(ptr)
5 stdcall CStdStubBuffer_Connect(ptr ptr)
6 stdcall CStdStubBuffer_CountRefs(ptr)
7 stdcall CStdStubBuffer_DebugServerQueryInterface(ptr ptr)
8 stdcall CStdStubBuffer_DebugServerRelease(ptr ptr)
9 stdcall CStdStubBuffer_Disconnect(ptr)
10 stdcall CStdStubBuffer_Invoke(ptr ptr ptr)
11 stdcall CStdStubBuffer_IsIIDSupported(ptr ptr)
12 stdcall CStdStubBuffer_QueryInterface(ptr ptr ptr)
13 stdcall DceErrorInqTextA (long ptr)
14 stdcall DceErrorInqTextW (long ptr)
# DllGetClassObject
# DllInstall
@ stdcall -private DllRegisterServer()
18 stub GlobalMutexClearExternal
19 stub GlobalMutexRequestExternal
20 stdcall IUnknown_AddRef_Proxy(ptr)
21 stdcall IUnknown_QueryInterface_Proxy(ptr ptr ptr)
22 stdcall IUnknown_Release_Proxy(ptr)
23 stdcall I_RpcAbortAsyncCall(ptr long) I_RpcAsyncAbortCall
24 stdcall I_RpcAllocate(long)
25 stdcall I_RpcAsyncAbortCall(ptr long)
26 stdcall I_RpcAsyncSetHandle(ptr ptr)
27 stub I_RpcBCacheAllocate
28 stub I_RpcBCacheFree
29 stub I_RpcBindingCopy
# I_RpcBindingHandleToAsyncHandle
31 stub I_RpcBindingInqConnId
32 stub I_RpcBindingInqDynamicEndPoint
33 stub I_RpcBindingInqDynamicEndPointA
34 stub I_RpcBindingInqDynamicEndPointW
35 stdcall I_RpcBindingInqLocalClientPID(ptr ptr)
# I_RpcBindingInqMarshalledTargetInfo
37 stub I_RpcBindingInqSecurityContext
38 stdcall I_RpcBindingInqTransportType(ptr ptr)
39 stub I_RpcBindingInqWireIdForSnego
40 stub I_RpcBindingIsClientLocal
41 stub I_RpcBindingToStaticStringBindingW
42 stub I_RpcClearMutex
43 stub I_RpcConnectionInqSockBuffSize
44 stub I_RpcConnectionSetSockBuffSize
45 stub I_RpcDeleteMutex
46 stub I_RpcEnableWmiTrace
47 stdcall I_RpcExceptionFilter(long)
48 stdcall I_RpcFree(ptr)
49 stdcall I_RpcFreeBuffer(ptr)
50 stub I_RpcFreePipeBuffer
51 stdcall I_RpcGetBuffer(ptr)
52 stub I_RpcGetBufferWithObject
53 stdcall I_RpcGetCurrentCallHandle()
54 stub I_RpcGetExtendedError
55 stub I_RpcIfInqTransferSyntaxes
56 stub I_RpcLogEvent
57 stdcall I_RpcMapWin32Status(long)
# I_RpcNDRCGetWireRepresentation
# I_RpcNDRSContextEmergencyCleanup
60 stdcall I_RpcNegotiateTransferSyntax(ptr)
61 stub I_RpcNsBindingSetEntryName
62 stub I_RpcNsBindingSetEntryNameA
63 stub I_RpcNsBindingSetEntryNameW
64 stub I_RpcNsInterfaceExported
65 stub I_RpcNsInterfaceUnexported
66 stub I_RpcParseSecurity
67 stub I_RpcPauseExecution
68 stub I_RpcProxyNewConnection
69 stub I_RpcReallocPipeBuffer
70 stdcall I_RpcReceive(ptr)
# I_RpcRecordCalloutFailure
# I_RpcReplyToClientWithStatus
73 stub I_RpcRequestMutex
# I_RpcSNCHOption
75 stdcall I_RpcSend(ptr)
76 stdcall I_RpcSendReceive(ptr)
77 stub I_RpcServerAllocateIpPort
# I_RpcServerCheckClientRestriction
79 stub I_RpcServerInqAddressChangeFn
80 stub I_RpcServerInqLocalConnAddress
81 stub I_RpcServerInqTransportType
# I_RpcServerIsClientDisconnected
83 stub I_RpcServerRegisterForwardFunction
84 stub I_RpcServerSetAddressChangeFn
85 stub I_RpcServerUseProtseq2A
86 stub I_RpcServerUseProtseq2W
87 stub I_RpcServerUseProtseqEp2A
88 stub I_RpcServerUseProtseqEp2W
# I_RpcSessionStrictContextHandle
90 stub I_RpcSetAsyncHandle
91 stub I_RpcSsDontSerializeContext
92 stub I_RpcSystemFunction001
93 stub I_RpcTransConnectionAllocatePacket
94 stub I_RpcTransConnectionFreePacket
95 stub I_RpcTransConnectionReallocPacket
96 stub I_RpcTransDatagramAllocate2
97 stub I_RpcTransDatagramAllocate
98 stub I_RpcTransDatagramFree
99 stub I_RpcTransGetThreadEvent
100 stub I_RpcTransIoCancelled
101 stub I_RpcTransServerNewConnection
102 stub I_RpcTurnOnEEInfoPropagation
103 stdcall I_UuidCreate(ptr)
104 stub MIDL_wchar_strcpy
105 stub MIDL_wchar_strlen
106 stdcall MesBufferHandleReset(ptr long long ptr long ptr)
107 stdcall MesDecodeBufferHandleCreate(ptr long ptr)
108 stdcall MesDecodeIncrementalHandleCreate(ptr ptr ptr)
109 stdcall MesEncodeDynBufferHandleCreate(ptr ptr ptr)
110 stdcall MesEncodeFixedBufferHandleCreate(ptr long ptr ptr)
111 stdcall MesEncodeIncrementalHandleCreate(ptr ptr ptr ptr)
112 stdcall MesHandleFree(ptr)
113 stdcall MesIncrementalHandleReset(ptr ptr ptr ptr ptr long)
114 stub MesInqProcEncodingId
115 stdcall NDRCContextBinding(ptr)
116 stdcall NDRCContextMarshall(ptr ptr)
117 stdcall NDRCContextUnmarshall(ptr ptr ptr long)
118 stdcall NDRSContextMarshall2(ptr ptr ptr ptr ptr long)
119 stdcall NDRSContextMarshall(ptr ptr ptr)
120 stdcall NDRSContextMarshallEx(ptr ptr ptr ptr)
121 stdcall NDRSContextUnmarshall2(ptr ptr ptr ptr long)
122 stdcall NDRSContextUnmarshall(ptr ptr)
123 stdcall NDRSContextUnmarshallEx(ptr ptr ptr)
124 stub NDRcopy
125 stdcall NdrAllocate(ptr long)
126 varargs NdrAsyncClientCall(ptr ptr)
127 stub NdrAsyncServerCall
128 stdcall NdrByteCountPointerBufferSize(ptr ptr ptr)
129 stdcall NdrByteCountPointerFree(ptr ptr ptr)
130 stdcall NdrByteCountPointerMarshall(ptr ptr ptr)
131 stdcall NdrByteCountPointerUnmarshall(ptr ptr ptr long)
132 stdcall NdrCStdStubBuffer2_Release(ptr ptr)
133 stdcall NdrCStdStubBuffer_Release(ptr ptr)
134 stdcall NdrClearOutParameters(ptr ptr ptr)
135 varargs -arch=i386 NdrClientCall(ptr ptr) NdrClientCall2
136 varargs NdrClientCall2(ptr ptr)
137 stdcall NdrClientContextMarshall(ptr ptr long)
138 stdcall NdrClientContextUnmarshall(ptr ptr ptr)
139 stub NdrClientInitialize
140 stdcall NdrClientInitializeNew(ptr ptr ptr long)
141 stdcall NdrComplexArrayBufferSize(ptr ptr ptr)
142 stdcall NdrComplexArrayFree(ptr ptr ptr)
143 stdcall NdrComplexArrayMarshall(ptr ptr ptr)
144 stdcall NdrComplexArrayMemorySize(ptr ptr)
145 stdcall NdrComplexArrayUnmarshall(ptr ptr ptr long)
146 stdcall NdrComplexStructBufferSize(ptr ptr ptr)
147 stdcall NdrComplexStructFree(ptr ptr ptr)
148 stdcall NdrComplexStructMarshall(ptr ptr ptr)
149 stdcall NdrComplexStructMemorySize(ptr ptr)
150 stdcall NdrComplexStructUnmarshall(ptr ptr ptr long)
151 stdcall NdrConformantArrayBufferSize(ptr ptr ptr)
152 stdcall NdrConformantArrayFree(ptr ptr ptr)
153 stdcall NdrConformantArrayMarshall(ptr ptr ptr)
154 stdcall NdrConformantArrayMemorySize(ptr ptr)
155 stdcall NdrConformantArrayUnmarshall(ptr ptr ptr long)
156 stdcall NdrConformantStringBufferSize(ptr ptr ptr)
157 stdcall NdrConformantStringMarshall(ptr ptr ptr)
158 stdcall NdrConformantStringMemorySize(ptr ptr)
159 stdcall NdrConformantStringUnmarshall(ptr ptr ptr long)
160 stdcall NdrConformantStructBufferSize(ptr ptr ptr)
161 stdcall NdrConformantStructFree(ptr ptr ptr)
162 stdcall NdrConformantStructMarshall(ptr ptr ptr)
163 stdcall NdrConformantStructMemorySize(ptr ptr)
164 stdcall NdrConformantStructUnmarshall(ptr ptr ptr long)
165 stdcall NdrConformantVaryingArrayBufferSize(ptr ptr ptr)
166 stdcall NdrConformantVaryingArrayFree(ptr ptr ptr)
167 stdcall NdrConformantVaryingArrayMarshall(ptr ptr ptr)
168 stdcall NdrConformantVaryingArrayMemorySize(ptr ptr)
169 stdcall NdrConformantVaryingArrayUnmarshall(ptr ptr ptr long)
170 stdcall NdrConformantVaryingStructBufferSize(ptr ptr ptr)
171 stdcall NdrConformantVaryingStructFree(ptr ptr ptr)
172 stdcall NdrConformantVaryingStructMarshall(ptr ptr ptr)
173 stdcall NdrConformantVaryingStructMemorySize(ptr ptr)
174 stdcall NdrConformantVaryingStructUnmarshall(ptr ptr ptr long)
175 stdcall NdrContextHandleInitialize(ptr ptr)
176 stdcall NdrContextHandleSize(ptr ptr ptr)
177 stdcall NdrConvert2(ptr ptr long)
178 stdcall NdrConvert(ptr ptr)
179 stdcall NdrCorrelationFree(ptr)
180 stdcall NdrCorrelationInitialize(ptr ptr long long)
181 stdcall NdrCorrelationPass(ptr)
# NdrCreateServerInterfaceFromStub
183 stub NdrDcomAsyncClientCall
184 stub NdrDcomAsyncStubCall
185 stdcall NdrDllCanUnloadNow(ptr)
186 stdcall NdrDllGetClassObject(ptr ptr ptr ptr ptr ptr)
187 stdcall NdrDllRegisterProxy(long ptr ptr)
188 stdcall NdrDllUnregisterProxy(long ptr ptr)
189 stdcall NdrEncapsulatedUnionBufferSize(ptr ptr ptr)
190 stdcall NdrEncapsulatedUnionFree(ptr ptr ptr)
191 stdcall NdrEncapsulatedUnionMarshall(ptr ptr ptr)
192 stdcall NdrEncapsulatedUnionMemorySize(ptr ptr)
193 stdcall NdrEncapsulatedUnionUnmarshall(ptr ptr ptr long)
194 stdcall NdrFixedArrayBufferSize(ptr ptr ptr)
195 stdcall NdrFixedArrayFree(ptr ptr ptr)
196 stdcall NdrFixedArrayMarshall(ptr ptr ptr)
197 stdcall NdrFixedArrayMemorySize(ptr ptr)
198 stdcall NdrFixedArrayUnmarshall(ptr ptr ptr long)
199 stdcall NdrFreeBuffer(ptr)
200 stdcall NdrFullPointerFree(ptr ptr)
201 stdcall NdrFullPointerInsertRefId(ptr long ptr)
202 stdcall NdrFullPointerQueryPointer(ptr ptr long ptr)
203 stdcall NdrFullPointerQueryRefId(ptr long long ptr)
204 stdcall NdrFullPointerXlatFree(ptr)
205 stdcall NdrFullPointerXlatInit(long long) 
206 stdcall NdrGetBuffer(ptr long ptr)
207 stub NdrGetDcomProtocolVersion
208 stub NdrGetSimpleTypeBufferAlignment
209 stub NdrGetSimpleTypeBufferSize
210 stub NdrGetSimpleTypeMemorySize
211 stub NdrGetTypeFlags
212 stdcall NdrGetUserMarshalInfo(ptr long ptr)
213 stdcall NdrInterfacePointerBufferSize(ptr ptr ptr)
214 stdcall NdrInterfacePointerFree(ptr ptr ptr)
215 stdcall NdrInterfacePointerMarshall(ptr ptr ptr)
216 stdcall NdrInterfacePointerMemorySize(ptr ptr)
217 stdcall NdrInterfacePointerUnmarshall(ptr ptr ptr long)
218 stdcall NdrMapCommAndFaultStatus(ptr ptr ptr long)
219 varargs NdrMesProcEncodeDecode(ptr ptr ptr)
220 stub NdrMesProcEncodeDecode2
221 stub NdrMesSimpleTypeAlignSize
222 stub NdrMesSimpleTypeDecode
223 stub NdrMesSimpleTypeEncode
224 stub NdrMesTypeAlignSize2
225 stub NdrMesTypeAlignSize
226 stdcall NdrMesTypeDecode2(ptr ptr ptr ptr ptr)
227 stub NdrMesTypeDecode
228 stdcall NdrMesTypeEncode2(ptr ptr ptr ptr ptr)
229 stub NdrMesTypeEncode
230 stdcall NdrMesTypeFree2(ptr ptr ptr ptr ptr)
231 stdcall NdrNonConformantStringBufferSize(ptr ptr ptr)
232 stdcall NdrNonConformantStringMarshall(ptr ptr ptr)
233 stdcall NdrNonConformantStringMemorySize(ptr ptr)
234 stdcall NdrNonConformantStringUnmarshall(ptr ptr ptr long)
235 stdcall NdrNonEncapsulatedUnionBufferSize(ptr ptr ptr)
236 stdcall NdrNonEncapsulatedUnionFree(ptr ptr ptr)
237 stdcall NdrNonEncapsulatedUnionMarshall(ptr ptr ptr)
238 stdcall NdrNonEncapsulatedUnionMemorySize(ptr ptr)
239 stdcall NdrNonEncapsulatedUnionUnmarshall(ptr ptr ptr long)
240 stub NdrNsGetBuffer
241 stub NdrNsSendReceive
242 stdcall NdrOleAllocate(long)
243 stdcall NdrOleFree(ptr)
244 stub NdrOutInit
245 stub NdrPartialIgnoreClientBufferSize
246 stub NdrPartialIgnoreClientMarshall
247 stub NdrPartialIgnoreServerInitialize
248 stub NdrPartialIgnoreServerUnmarshall
249 stdcall NdrPointerBufferSize(ptr ptr ptr)
250 stdcall NdrPointerFree(ptr ptr ptr)
251 stdcall NdrPointerMarshall(ptr ptr ptr)
252 stdcall NdrPointerMemorySize(ptr ptr)
253 stdcall NdrPointerUnmarshall(ptr ptr ptr long)
254 stdcall NdrProxyErrorHandler(long)
255 stdcall NdrProxyFreeBuffer(ptr ptr)
256 stdcall NdrProxyGetBuffer(ptr ptr)
257 stdcall NdrProxyInitialize(ptr ptr ptr ptr long)
258 stdcall NdrProxySendReceive(ptr ptr)
259 stdcall NdrRangeUnmarshall(ptr ptr ptr long)
260 stub NdrRpcSmClientAllocate
261 stub NdrRpcSmClientFree
262 stdcall NdrRpcSmSetClientToOsf(ptr)
263 stub NdrRpcSsDefaultAllocate
264 stub NdrRpcSsDefaultFree
265 stub NdrRpcSsDisableAllocate
266 stub NdrRpcSsEnableAllocate
267 stdcall NdrSendReceive(ptr ptr)
268 stdcall NdrServerCall2(ptr)
269 stdcall NdrServerCall(ptr)
270 stdcall NdrServerContextMarshall(ptr ptr long)
271 stdcall NdrServerContextNewMarshall(ptr ptr ptr ptr)
272 stdcall NdrServerContextNewUnmarshall(ptr ptr)
273 stdcall NdrServerContextUnmarshall(ptr)
274 stub NdrServerInitialize
275 stub NdrServerInitializeMarshall
276 stdcall NdrServerInitializeNew(ptr ptr ptr)
277 stub NdrServerInitializePartial
278 stub NdrServerInitializeUnmarshall
279 stub NdrServerMarshall
280 stub NdrServerUnmarshall
281 stdcall NdrSimpleStructBufferSize(ptr ptr ptr)
282 stdcall NdrSimpleStructFree(ptr ptr ptr)
283 stdcall NdrSimpleStructMarshall(ptr ptr ptr)
284 stdcall NdrSimpleStructMemorySize(ptr ptr)
285 stdcall NdrSimpleStructUnmarshall(ptr ptr ptr long)
286 stdcall NdrSimpleTypeMarshall(ptr ptr long)
287 stdcall NdrSimpleTypeUnmarshall(ptr ptr long)
288 stdcall NdrStubCall2(ptr ptr ptr ptr)
289 stdcall NdrStubCall(ptr ptr ptr ptr)
290 stdcall NdrStubForwardingFunction(ptr ptr ptr ptr)
291 stdcall NdrStubGetBuffer(ptr ptr ptr)
292 stdcall NdrStubInitialize(ptr ptr ptr ptr)
293 stub NdrStubInitializeMarshall
294 stub NdrTypeFlags
295 stub NdrTypeFree
296 stub NdrTypeMarshall
297 stub NdrTypeSize
298 stub NdrTypeUnmarshall
299 stub NdrUnmarshallBasetypeInline
300 stdcall NdrUserMarshalBufferSize(ptr ptr ptr)
301 stdcall NdrUserMarshalFree(ptr ptr ptr)
302 stdcall NdrUserMarshalMarshall(ptr ptr ptr)
303 stdcall NdrUserMarshalMemorySize(ptr ptr)
304 stub NdrUserMarshalSimpleTypeConvert
305 stdcall NdrUserMarshalUnmarshall(ptr ptr ptr long)
306 stdcall NdrVaryingArrayBufferSize(ptr ptr ptr)
307 stdcall NdrVaryingArrayFree(ptr ptr ptr)
308 stdcall NdrVaryingArrayMarshall(ptr ptr ptr)
309 stdcall NdrVaryingArrayMemorySize(ptr ptr)
310 stdcall NdrVaryingArrayUnmarshall(ptr ptr ptr long)
311 stdcall NdrXmitOrRepAsBufferSize(ptr ptr ptr)
312 stdcall NdrXmitOrRepAsFree(ptr ptr ptr)
313 stdcall NdrXmitOrRepAsMarshall(ptr ptr ptr)
314 stdcall NdrXmitOrRepAsMemorySize(ptr ptr)
315 stdcall NdrXmitOrRepAsUnmarshall(ptr ptr ptr long)
316 stub NdrpCreateProxy
317 stub NdrpCreateStub
318 stub NdrpGetProcFormatString
319 stub NdrpGetTypeFormatString
320 stub NdrpGetTypeGenCookie
321 stub NdrpMemoryIncrement
322 stub NdrpReleaseTypeFormatString
323 stub NdrpReleaseTypeGenCookie
324 stub NdrpSetRpcSsDefaults
325 stub NdrpVarVtOfTypeDesc
326 stdcall RpcAbortAsyncCall(ptr long) RpcAsyncAbortCall
327 stdcall RpcAsyncAbortCall(ptr long)
328 stdcall RpcAsyncCancelCall(ptr long)
329 stdcall RpcAsyncCompleteCall(ptr ptr)
330 stdcall RpcAsyncGetCallStatus(ptr)
331 stdcall RpcAsyncInitializeHandle(ptr long)
332 stub RpcAsyncRegisterInfo
333 stdcall RpcBindingCopy(ptr ptr)
334 stdcall RpcBindingFree(ptr)
335 stdcall RpcBindingFromStringBindingA(str  ptr)
336 stdcall RpcBindingFromStringBindingW(wstr ptr)
337 stdcall RpcBindingInqAuthClientA(ptr ptr ptr ptr ptr ptr)
338 stdcall RpcBindingInqAuthClientExA(ptr ptr ptr ptr ptr ptr long)
339 stdcall RpcBindingInqAuthClientExW(ptr ptr ptr ptr ptr ptr long)
340 stdcall RpcBindingInqAuthClientW(ptr ptr ptr ptr ptr ptr)
341 stdcall RpcBindingInqAuthInfoA(ptr ptr ptr ptr ptr ptr)
342 stdcall RpcBindingInqAuthInfoExA(ptr ptr ptr ptr ptr ptr long ptr)
343 stdcall RpcBindingInqAuthInfoExW(ptr ptr ptr ptr ptr ptr long ptr)
344 stdcall RpcBindingInqAuthInfoW(ptr ptr ptr ptr ptr ptr)
345 stdcall RpcBindingInqObject(ptr ptr)
346 stub RpcBindingInqOption
347 stdcall RpcBindingReset(ptr)
348 stub RpcBindingServerFromClient
349 stdcall RpcBindingSetAuthInfoA(ptr str long long ptr long)
350 stdcall RpcBindingSetAuthInfoExA(ptr str long long ptr long ptr)
351 stdcall RpcBindingSetAuthInfoExW(ptr wstr long long ptr long ptr)
352 stdcall RpcBindingSetAuthInfoW(ptr wstr long long ptr long)
353 stdcall RpcBindingSetObject(ptr ptr)
354 stdcall RpcBindingSetOption(ptr long long)
355 stdcall RpcBindingToStringBindingA(ptr ptr)
356 stdcall RpcBindingToStringBindingW(ptr ptr)
357 stdcall RpcBindingVectorFree(ptr)
358 stdcall RpcCancelAsyncCall(ptr long) RpcAsyncCancelCall
359 stdcall RpcCancelThread(ptr)
360 stdcall RpcCancelThreadEx(ptr long)
361 stub RpcCertGeneratePrincipalNameA
362 stub RpcCertGeneratePrincipalNameW
363 stdcall RpcCompleteAsyncCall(ptr ptr) RpcAsyncCompleteCall
364 stdcall RpcEpRegisterA(ptr ptr ptr str)
365 stdcall RpcEpRegisterNoReplaceA(ptr ptr ptr str)
366 stdcall RpcEpRegisterNoReplaceW(ptr ptr ptr wstr)
367 stdcall RpcEpRegisterW(ptr ptr ptr wstr)
368 stdcall RpcEpResolveBinding(ptr ptr)
369 stdcall RpcEpUnregister(ptr ptr ptr)
370 stub RpcErrorAddRecord
371 stub RpcErrorClearInformation
372 stdcall RpcErrorEndEnumeration(ptr)
373 stdcall RpcErrorGetNextRecord(ptr long ptr)
# RpcErrorGetNumberOfRecords
375 stdcall RpcErrorLoadErrorInfo(ptr long ptr)
376 stub RpcErrorResetEnumeration
377 stdcall RpcErrorSaveErrorInfo(ptr ptr ptr)
378 stdcall RpcErrorStartEnumeration(ptr)
379 stub RpcFreeAuthorizationContext
380 stdcall RpcGetAsyncCallStatus(ptr) RpcAsyncGetCallStatus
# RpcGetAuthorizationContextForClient
382 stub RpcIfIdVectorFree
383 stub RpcIfInqId
384 stdcall RpcImpersonateClient(ptr)
385 stdcall RpcInitializeAsyncHandle(ptr long) RpcAsyncInitializeHandle
386 stdcall RpcMgmtEnableIdleCleanup()
387 stdcall RpcMgmtEpEltInqBegin(ptr long ptr long ptr ptr)
388 stub RpcMgmtEpEltInqDone
389 stub RpcMgmtEpEltInqNextA
390 stub RpcMgmtEpEltInqNextW
391 stub RpcMgmtEpUnregister
392 stub RpcMgmtInqComTimeout
393 stub RpcMgmtInqDefaultProtectLevel
394 stdcall RpcMgmtInqIfIds(ptr ptr)
395 stub RpcMgmtInqServerPrincNameA
396 stub RpcMgmtInqServerPrincNameW
397 stdcall RpcMgmtInqStats(ptr ptr)
398 stdcall RpcMgmtIsServerListening(ptr)
399 stdcall RpcMgmtSetAuthorizationFn(ptr)
400 stdcall RpcMgmtSetCancelTimeout(long)
401 stdcall RpcMgmtSetComTimeout(ptr long)
402 stdcall RpcMgmtSetServerStackSize(long)
403 stdcall RpcMgmtStatsVectorFree(ptr)
404 stdcall RpcMgmtStopServerListening(ptr)
405 stdcall RpcMgmtWaitServerListen()
406 stdcall RpcNetworkInqProtseqsA(ptr)
407 stdcall RpcNetworkInqProtseqsW(ptr)
408 stdcall RpcNetworkIsProtseqValidA(ptr)
409 stdcall RpcNetworkIsProtseqValidW(ptr)
410 stub RpcNsBindingInqEntryNameA
411 stub RpcNsBindingInqEntryNameW
412 stub RpcObjectInqType
413 stub RpcObjectSetInqFn
414 stdcall RpcObjectSetType(ptr ptr)
415 stdcall RpcProtseqVectorFreeA(ptr)
416 stdcall RpcProtseqVectorFreeW(ptr)
417 stdcall RpcRaiseException(long)
418 stub RpcRegisterAsyncInfo
419 stdcall RpcRevertToSelf()
420 stdcall RpcRevertToSelfEx(ptr)
421 stdcall RpcServerInqBindings(ptr)
422 stub RpcServerInqCallAttributesA
423 stub RpcServerInqCallAttributesW
424 stdcall RpcServerInqDefaultPrincNameA(long ptr)
425 stdcall RpcServerInqDefaultPrincNameW(long ptr)
426 stub RpcServerInqIf
427 stdcall RpcServerListen(long long long)
428 stdcall RpcServerRegisterAuthInfoA(str  long ptr ptr)
429 stdcall RpcServerRegisterAuthInfoW(wstr long ptr ptr)
430 stdcall RpcServerRegisterIf2(ptr ptr ptr long long long ptr)
431 stdcall RpcServerRegisterIf(ptr ptr ptr)
432 stdcall RpcServerRegisterIfEx(ptr ptr ptr long long ptr)
433 stub RpcServerTestCancel
434 stdcall RpcServerUnregisterIf(ptr ptr long)
435 stdcall RpcServerUnregisterIfEx(ptr ptr long)
436 stub RpcServerUseAllProtseqs
437 stub RpcServerUseAllProtseqsEx
438 stub RpcServerUseAllProtseqsIf
439 stub RpcServerUseAllProtseqsIfEx
440 stdcall RpcServerUseProtseqA(str long ptr)
441 stdcall RpcServerUseProtseqEpA(str  long str  ptr)
442 stdcall RpcServerUseProtseqEpExA(str  long str  ptr ptr)
443 stdcall RpcServerUseProtseqEpExW(wstr long wstr ptr ptr)
444 stdcall RpcServerUseProtseqEpW(wstr long wstr ptr)
445 stub RpcServerUseProtseqExA
446 stub RpcServerUseProtseqExW
447 stub RpcServerUseProtseqIfA
448 stub RpcServerUseProtseqIfExA
449 stub RpcServerUseProtseqIfExW
450 stub RpcServerUseProtseqIfW
451 stdcall RpcServerUseProtseqW(wstr long ptr)
452 stub RpcServerYield
453 stub RpcSmAllocate
454 stub RpcSmClientFree
455 stdcall RpcSmDestroyClientContext(ptr)
456 stub RpcSmDisableAllocate
457 stub RpcSmEnableAllocate
458 stub RpcSmFree
459 stub RpcSmGetThreadHandle
460 stub RpcSmSetClientAllocFree
461 stub RpcSmSetThreadHandle
462 stub RpcSmSwapClientAllocFree
463 stub RpcSsAllocate
464 stub RpcSsContextLockExclusive
465 stub RpcSsContextLockShared
466 stdcall RpcSsDestroyClientContext(ptr)
467 stub RpcSsDisableAllocate
468 stdcall RpcSsDontSerializeContext()
469 stub RpcSsEnableAllocate
470 stub RpcSsFree
471 stub RpcSsGetContextBinding
472 stub RpcSsGetThreadHandle
473 stub RpcSsSetClientAllocFree
474 stub RpcSsSetThreadHandle
475 stub RpcSsSwapClientAllocFree
476 stdcall RpcStringBindingComposeA(str str str str str ptr)
477 stdcall RpcStringBindingComposeW(wstr wstr wstr wstr wstr ptr)
478 stdcall RpcStringBindingParseA(str  ptr ptr ptr ptr ptr)
479 stdcall RpcStringBindingParseW(wstr ptr ptr ptr ptr ptr)
480 stdcall RpcStringFreeA(ptr)
481 stdcall RpcStringFreeW(ptr)
482 stub RpcTestCancel
483 stub RpcUserFree
484 stub SimpleTypeAlignment
485 stub SimpleTypeBufferSize
486 stub SimpleTypeMemorySize
487 stdcall TowerConstruct(ptr ptr ptr ptr ptr ptr)
488 stdcall TowerExplode(ptr ptr ptr ptr ptr ptr)
489 stdcall UuidCompare(ptr ptr ptr)
490 stdcall UuidCreate(ptr)
491 stdcall UuidCreateNil(ptr)
492 stdcall UuidCreateSequential(ptr)
493 stdcall UuidEqual(ptr ptr ptr)
494 stdcall UuidFromStringA(str ptr)
495 stdcall UuidFromStringW(wstr ptr)
496 stdcall UuidHash(ptr ptr)
497 stdcall UuidIsNil(ptr ptr)
498 stdcall UuidToStringA(ptr ptr)
499 stdcall UuidToStringW(ptr ptr)
500 stub char_array_from_ndr
501 stub char_from_ndr
502 stub data_from_ndr
503 stub data_into_ndr
504 stub data_size_ndr
505 stub double_array_from_ndr
506 stub double_from_ndr
507 stub enum_from_ndr
508 stub float_array_from_ndr
509 stub float_from_ndr
510 stub long_array_from_ndr
511 stub long_from_ndr
512 stub long_from_ndr_temp
513 stub pfnFreeRoutines
514 stub pfnMarshallRouteines
515 stub pfnSizeRoutines
516 stub pfnUnmarshallRouteines
517 stub short_array_from_ndr
518 stub short_from_ndr
519 stub short_from_ndr_temp
520 stub tree_into_ndr
521 stub tree_peek_ndr
522 stub tree_size_ndr
