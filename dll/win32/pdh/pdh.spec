1 stub PdhPlaGetLogFileNameA
@ stub PdhAdd009CounterA
@ stub PdhAdd009CounterW
@ stdcall PdhAddCounterA(ptr str long ptr)
@ stdcall PdhAddCounterW(ptr wstr long ptr)
@ stdcall PdhAddEnglishCounterA(ptr str long ptr)
@ stdcall PdhAddEnglishCounterW(ptr wstr long ptr)
@ stdcall PdhBindInputDataSourceA(ptr str)
@ stdcall PdhBindInputDataSourceW(ptr wstr)
@ stub PdhBrowseCountersA
@ stub PdhBrowseCountersHA
@ stub PdhBrowseCountersHW
@ stub PdhBrowseCountersW
@ stdcall PdhCalculateCounterFromRawValue(ptr long ptr ptr ptr)
@ stub PdhCloseLog
@ stdcall PdhCloseQuery(ptr)
@ stdcall PdhCollectQueryData(ptr)
@ stdcall PdhCollectQueryDataWithTime(ptr ptr)
@ stdcall PdhCollectQueryDataEx(ptr long ptr)
@ stub PdhComputeCounterStatistics
@ stdcall PdhConnectMachineA(str)
@ stub PdhConnectMachineW
@ stub PdhCreateSQLTablesA
@ stub PdhCreateSQLTablesW
@ stub PdhEnumLogSetNamesA
@ stub PdhEnumLogSetNamesW
@ stub PdhEnumMachinesA
@ stub PdhEnumMachinesHA
@ stub PdhEnumMachinesHW
@ stub PdhEnumMachinesW
@ stdcall PdhEnumObjectItemsA(str str str ptr ptr ptr ptr long long)
@ stub PdhEnumObjectItemsHA
@ stub PdhEnumObjectItemsHW
@ stdcall PdhEnumObjectItemsW(wstr wstr wstr ptr ptr ptr ptr long long)
@ stub PdhEnumObjectsA
@ stub PdhEnumObjectsHA
@ stub PdhEnumObjectsHW
@ stub PdhEnumObjectsW
@ stdcall PdhExpandCounterPathA(str ptr ptr)
@ stdcall PdhExpandCounterPathW(wstr ptr ptr)
@ stdcall PdhExpandWildCardPathA(str str ptr ptr long)
@ stub PdhExpandWildCardPathHA
@ stub PdhExpandWildCardPathHW
@ stdcall PdhExpandWildCardPathW(wstr wstr ptr ptr long)
@ stub PdhFormatFromRawValue
@ stdcall PdhGetCounterInfoA(ptr long ptr ptr)
@ stdcall PdhGetCounterInfoW(ptr long ptr ptr)
@ stdcall PdhGetCounterTimeBase(ptr ptr)
@ stub PdhGetDataSourceTimeRangeA
@ stub PdhGetDataSourceTimeRangeH
@ stub PdhGetDataSourceTimeRangeW
@ stub PdhGetDefaultPerfCounterA
@ stub PdhGetDefaultPerfCounterHA
@ stub PdhGetDefaultPerfCounterHW
@ stub PdhGetDefaultPerfCounterW
@ stub PdhGetDefaultPerfObjectA
@ stub PdhGetDefaultPerfObjectHA
@ stub PdhGetDefaultPerfObjectHW
@ stub PdhGetDefaultPerfObjectW
@ stdcall PdhGetDllVersion(ptr)
@ stub PdhGetFormattedCounterArrayA
@ stub PdhGetFormattedCounterArrayW
@ stdcall PdhGetFormattedCounterValue(ptr long ptr ptr)
@ stub PdhGetLogFileSize
@ stdcall PdhGetLogFileTypeA(str ptr)
@ stdcall PdhGetLogFileTypeW(wstr ptr)
@ stub PdhGetLogSetGUID
@ stub PdhGetRawCounterArrayA
@ stub PdhGetRawCounterArrayW
@ stdcall PdhGetRawCounterValue(ptr ptr ptr)
@ stub PdhIsRealTimeQuery
@ stub PdhListLogFileHeaderA
@ stub PdhListLogFileHeaderW
@ stub PdhLogServiceCommandA
@ stub PdhLogServiceCommandW
@ stub PdhLogServiceControlA
@ stub PdhLogServiceControlW
@ stdcall PdhLookupPerfIndexByNameA(str str ptr)
@ stdcall PdhLookupPerfIndexByNameW(wstr wstr ptr)
@ stdcall PdhLookupPerfNameByIndexA(str long ptr ptr)
@ stdcall PdhLookupPerfNameByIndexW(wstr long ptr ptr)
@ stdcall PdhMakeCounterPathA(ptr ptr ptr long)
@ stdcall PdhMakeCounterPathW(ptr ptr ptr long)
@ stub PdhOpenLogA
@ stub PdhOpenLogW
@ stdcall PdhOpenQuery(wstr long ptr) PdhOpenQueryW
@ stdcall PdhOpenQueryA(str long ptr)
@ stub PdhOpenQueryH
@ stdcall PdhOpenQueryW(wstr long ptr)
@ stub PdhParseCounterPathA
@ stub PdhParseCounterPathW
@ stub PdhParseInstanceNameA
@ stub PdhParseInstanceNameW
@ stub PdhPlaAddItemA
@ stub PdhPlaAddItemW
@ stub PdhPlaCreateA
@ stub PdhPlaCreateW
@ stub PdhPlaDeleteA
@ stub PdhPlaDeleteW
@ stub PdhPlaEnumCollectionsA
@ stub PdhPlaEnumCollectionsW
@ stub PdhPlaGetInfoA
@ stub PdhPlaGetInfoW
@ stub PdhPlaGetLogFileNameW
@ stub PdhPlaGetScheduleA
@ stub PdhPlaGetScheduleW
@ stub PdhPlaRemoveAllItemsA
@ stub PdhPlaRemoveAllItemsW
@ stub PdhPlaScheduleA
@ stub PdhPlaScheduleW
@ stub PdhPlaSetInfoA
@ stub PdhPlaSetInfoW
@ stub PdhPlaSetItemListA
@ stub PdhPlaSetItemListW
@ stub PdhPlaSetRunAsA
@ stub PdhPlaSetRunAsW
@ stub PdhPlaStartA
@ stub PdhPlaStartW
@ stub PdhPlaStopA
@ stub PdhPlaStopW
@ stub PdhPlaValidateInfoA
@ stub PdhPlaValidateInfoW
@ stub PdhReadRawLogRecord
@ stub PdhRelogA
@ stub PdhRelogW
@ stdcall PdhRemoveCounter(ptr)
@ stub PdhSelectDataSourceA
@ stub PdhSelectDataSourceW
@ stdcall PdhSetCounterScaleFactor(ptr long)
@ stdcall PdhSetDefaultRealTimeDataSource(long)
@ stub PdhSetLogSetRunID
@ stub PdhSetQueryTimeRange
@ stub PdhTranslate009CounterA
@ stub PdhTranslate009CounterW
@ stub PdhTranslateLocaleCounterA
@ stub PdhTranslateLocaleCounterW
@ stub PdhUpdateLogA
@ stub PdhUpdateLogFileCatalog
@ stub PdhUpdateLogW
@ stdcall PdhValidatePathA(str)
@ stdcall PdhValidatePathExA(ptr str)
@ stdcall PdhValidatePathExW(ptr wstr)
@ stdcall PdhValidatePathW(wstr)
@ stdcall PdhVbAddCounter(ptr str ptr)
@ stub PdhVbCreateCounterPathList
@ stub PdhVbGetCounterPathElements
@ stub PdhVbGetCounterPathFromList
@ stdcall PdhVbGetDoubleCounterValue(ptr ptr)
@ stub PdhVbGetLogFileSize
@ stub PdhVbGetOneCounterPath
@ stub PdhVbIsGoodStatus
@ stub PdhVbOpenLog
@ stub PdhVbOpenQuery
@ stub PdhVbUpdateLog
@ stub PdhVerifySQLDBA
@ stub PdhVerifySQLDBW
@ stub PdhiPla2003SP1Installed
@ stub PdhiPlaFormatBlanksA
@ stub PdhiPlaFormatBlanksW
@ stub PdhiPlaGetVersion
@ stub PdhiPlaRunAs
@ stub PdhiPlaSetRunAs
@ stub PlaTimeInfoToMilliSeconds
