@ stub HttpAddFragmentToCache
@ stdcall HttpAddUrl(ptr wstr ptr)
@ stub HttpAddUrlToConfigGroup
@ stdcall HttpAddUrlToUrlGroup(int64 wstr int64 long)
@ stub HttpCancelHttpRequest
@ stub HttpCreateAppPool
@ stub HttpCreateConfigGroup
@ stub HttpCreateFilter
@ stdcall HttpCreateHttpHandle(ptr long)
@ stdcall HttpCreateRequestQueue(long wstr ptr long ptr)
@ stdcall HttpCreateServerSession(long ptr long)
@ stdcall HttpCreateUrlGroup(int64 ptr long)
@ stdcall HttpCloseRequestQueue(ptr)
@ stdcall HttpCloseServerSession(int64)
@ stdcall HttpCloseUrlGroup(int64)
@ stub HttpDeleteConfigGroup
@ stdcall HttpDeleteServiceConfiguration(ptr long ptr long ptr)
@ stub HttpFilterAccept
@ stub HttpFilterAppRead
@ stub HttpFilterAppWrite
@ stub HttpFilterAppWriteAndRawRead
@ stub HttpFilterClose
@ stub HttpFilterRawRead
@ stub HttpFilterRawWrite
@ stub HttpFilterRawWriteAndAppRead
@ stub HttpFlushResponseCache
@ stub HttpGetCounters
@ stdcall HttpInitialize(long long ptr)
@ stub HttpInitializeServerContext
@ stub HttpOpenAppPool
@ stub HttpOpenControlChannel
@ stub HttpOpenFilter
@ stub HttpQueryAppPoolInformation
@ stub HttpQueryConfigGroupInformation
@ stub HttpQueryControlChannelInformation
@ stub HttpQueryServerContextInformation
@ stdcall HttpQueryServiceConfiguration(ptr long ptr long ptr long ptr ptr)
@ stub HttpReadFragmentFromCache
@ stub HttpReceiveClientCertificate
@ stdcall HttpReceiveHttpRequest(ptr int64 long ptr long ptr ptr)
@ stub HttpReceiveHttpResponse
@ stub HttpReceiveRequestEntityBody
@ stub HttpRemoveAllUrlsFromConfigGroup
@ stdcall HttpRemoveUrl(ptr wstr)
@ stub HttpRemoveUrlFromConfigGroup
@ stdcall HttpRemoveUrlFromUrlGroup(int64 wstr long)
@ stub HttpSendHttpRequest
@ stdcall HttpSendHttpResponse(ptr int64 long ptr ptr ptr ptr long ptr ptr)
@ stub HttpSendRequestEntityBody
@ stub HttpSendResponseEntityBody
@ stub HttpSetAppPoolInformation
@ stub HttpSetConfigGroupInformation
@ stub HttpSetControlChannelInformation
@ stub HttpSetServerContextInformation
@ stdcall HttpSetServiceConfiguration(ptr long ptr long ptr)
@ stdcall HttpSetUrlGroupProperty(int64 long ptr long)
@ stub HttpShutdownAppPool
@ stub HttpShutdownFilter
@ stdcall HttpTerminate(long ptr)
@ stub HttpWaitForDemandStart
@ stub HttpWaitForDisconnect
