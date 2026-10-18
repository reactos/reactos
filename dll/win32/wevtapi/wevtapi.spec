@ stub EvtArchiveExportedLog
@ stub EvtCancel
@ stub EvtClearLog
@ stdcall EvtClose(ptr)
@ stdcall EvtCreateBookmark(wstr)
@ stdcall EvtCreateRenderContext(long ptr long)
@ stdcall EvtExportLog(ptr wstr wstr wstr long)
@ stub EvtFormatMessage
@ stdcall EvtGetChannelConfigProperty(ptr long long long ptr ptr)
@ stub EvtGetEventInfo
@ stub EvtGetEventMetadataProperty
@ stub EvtGetExtendedStatus
@ stub EvtGetLogInfo
@ stub EvtGetObjectArrayProperty
@ stub EvtGetObjectArraySize
@ stub EvtGetPublisherMetadataProperty
@ stub EvtGetQueryInfo
@ stub EvtIntAssertConfig
@ stub EvtIntCreateBinXMLFromCustomXML
@ stub EvtIntCreateLocalLogfile
@ stub EvtIntGetClassicLogDisplayName
@ stub EvtIntRenderResourceEventTemplate
@ stub EvtIntReportAuthzEventAndSourceAsync
@ stub EvtIntReportEventAndSourceAsync
@ stub EvtIntRetractConfig
@ stub EvtIntSysprepCleanup
@ stub EvtIntWriteXmlEventToLocalLogfile
@ stdcall EvtNext(ptr long ptr long long ptr)
@ stdcall EvtNextChannelPath(ptr long ptr ptr)
@ stub EvtNextEventMetadata
@ stub EvtNextPublisherId
@ stdcall EvtOpenChannelConfig(ptr wstr long)
@ stdcall EvtOpenChannelEnum(ptr long)
@ stub EvtOpenEventMetadataEnum
@ stdcall EvtOpenLog(ptr wstr long)
@ stub EvtOpenPublisherEnum
@ stub EvtOpenPublisherMetadata
@ stdcall EvtOpenSession(long ptr long long)
@ stdcall EvtQuery(ptr wstr wstr long)
@ stdcall EvtRender(ptr ptr long long ptr ptr ptr)
@ stdcall EvtSaveChannelConfig(ptr long)
@ stub EvtSeek
@ stdcall EvtSetChannelConfigProperty(ptr long long ptr)
@ stub EvtSetObjectArrayProperty
@ stdcall EvtSubscribe(ptr ptr wstr wstr ptr ptr ptr long)
@ stub EvtUpdateBookmark
