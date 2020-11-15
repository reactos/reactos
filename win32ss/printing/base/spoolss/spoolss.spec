@ stdcall AbortPrinter(ptr)
@ stub AddDriverCatalog
@ stdcall AddFormW(ptr long ptr)
@ stdcall AddJobW(long long ptr long ptr)
@ stdcall AddMonitorW(wstr long ptr)
@ stub AddPerMachineConnectionW
@ stdcall AddPortExW(wstr long ptr wstr)
@ stdcall AddPortW(wstr ptr wstr)
@ stub AddPrinterConnectionW
@ stdcall AddPrinterDriverExW(wstr long ptr long)
@ stdcall AddPrinterDriverW(wstr long ptr)
@ stdcall AddPrinterExW(wstr long ptr ptr long)
@ stdcall AddPrinterW(wstr long ptr)
@ stdcall AddPrintProcessorW(wstr wstr wstr wstr)
@ stdcall AddPrintProvidorW(wstr long ptr)
@ stub AdjustPointers
@ stub AdjustPointersInStructuresArray
@ stub AlignKMPtr
@ stdcall AlignRpcPtr(ptr ptr)
@ stdcall AllocSplStr(ptr)
@ stub AllowRemoteCalls
@ stub AppendPrinterNotifyInfoData
@ stub bGetDevModePerUser
@ stub bSetDevModePerUser
@ stdcall BuildOtherNamesFromMachineName(ptr ptr)
@ stub CacheAddName
@ stub CacheCreateAndAddNode
@ stub CacheCreateAndAddNodeWithIPAddresses
@ stub CacheDeleteNode
@ stub CacheIsNameCluster
@ stub CacheIsNameInNodeList
@ stub CallDrvDevModeConversion
@ stub CallRouterFindFirstPrinterChangeNotification
@ stub CheckLocalCall
@ stdcall ClosePrinter(long)
@ stub ClusterSplClose
@ stub ClusterSplIsAlive
@ stub ClusterSplOpen
@ stdcall ConfigurePortW(wstr ptr wstr)
@ stub CreatePrinterIC
@ stub DbgGetPointers
@ stdcall DeleteFormW(ptr wstr)
@ stdcall DeleteMonitorW(wstr wstr wstr)
@ stub DeletePerMachineConnectionW
@ stdcall DeletePortW(wstr ptr wstr)
@ stdcall DeletePrinter(ptr)
@ stub DeletePrinterConnectionW
@ stdcall DeletePrinterDataExW(ptr wstr wstr)
@ stdcall DeletePrinterDataW(ptr wstr)
@ stdcall DeletePrinterDriverExW(wstr wstr wstr long long)
@ stdcall DeletePrinterDriverW(wstr wstr wstr)
@ stub DeletePrinterIC
@ stdcall DeletePrinterKeyW(ptr wstr)
@ stdcall DeletePrintProcessorW(wstr wstr wstr)
@ stdcall DeletePrintProvidorW(wstr wstr wstr)
@ stdcall DllAllocSplMem(long)
@ stdcall DllFreeSplMem(ptr)
@ stdcall DllFreeSplStr(ptr)
@ stdcall EndDocPrinter(long)
@ stdcall EndPagePrinter(long)
@ stdcall EnumFormsW(ptr long ptr long ptr ptr)
@ stdcall EnumJobsW(long long long long ptr long ptr ptr)
@ stdcall EnumMonitorsW(wstr long ptr long ptr ptr)
@ stub EnumPerMachineConnectionsW
@ stdcall EnumPortsW(wstr long ptr long ptr ptr)
@ stdcall EnumPrinterDataExW(ptr wstr ptr long ptr ptr)
@ stdcall EnumPrinterDataW(ptr long wstr long ptr ptr ptr long ptr)
@ stdcall EnumPrinterDriversW(wstr wstr long ptr long ptr ptr)
@ stdcall EnumPrinterKeyW(ptr wstr wstr long ptr)
@ stdcall EnumPrintersW(long wstr long ptr long ptr ptr)
@ stdcall EnumPrintProcessorDatatypesW(wstr wstr long ptr long ptr ptr)
@ stdcall EnumPrintProcessorsW(wstr wstr long ptr long ptr ptr)
@ stub FindClosePrinterChangeNotification
@ stub FlushPrinter
@ stub FormatPrinterForRegistryKey
@ stub FormatRegistryKeyForPrinter
@ stub FreeOtherNames
@ stub GetClientUserHandle
@ stdcall GetFormW(ptr wstr long ptr long ptr)
@ stub GetJobAttributes
@ stdcall GetJobW(long long long ptr long ptr)
@ stub GetNetworkId
@ stdcall GetPrinterDataExW(long wstr wstr ptr ptr long ptr)
@ stdcall GetPrinterDataW(long wstr ptr ptr long ptr)
@ stdcall GetPrinterDriverDirectoryW(wstr wstr long ptr long ptr)
@ stdcall GetPrinterDriverExW(ptr wstr long ptr long ptr long long ptr ptr)
@ stdcall GetPrinterDriverW(long wstr long ptr long ptr)
@ stdcall GetPrinterW(long long ptr long ptr)
@ stdcall GetPrintProcessorDirectoryW(wstr wstr long ptr long ptr)
@ stub GetServerPolicy
@ stub GetShrinkedSize
@ stdcall ImpersonatePrinterClient(long)
@ stdcall InitializeRouter(long)
@ stub IsNamedPipeRpcCall
@ stub LoadDriver
@ stub LoadDriverFiletoConvertDevmode
@ stub LoadDriverWithVersion
@ stub LogWmiTraceEvent
@ stdcall MarshallDownStructure(ptr ptr long long)
@ stdcall MarshallDownStructuresArray(ptr long ptr long long)
@ stdcall MarshallUpStructure(long ptr ptr long long)
@ stdcall MarshallUpStructuresArray(long ptr long ptr long long)
@ stub MIDL_user_allocate1
@ stub MIDL_user_free1
@ stdcall OldGetPrinterDriverW(long wstr long ptr long ptr) GetPrinterDriverW
@ stub OpenPrinterExW
@ stub OpenPrinterPortW
@ stdcall OpenPrinterW(wstr ptr ptr)
@ stdcall PackStrings(ptr ptr ptr ptr)
@ stub PartialReplyPrinterChangeNotification
@ stub PlayGdiScriptOnPrinterIC
@ stub PrinterHandleRundown
@ stdcall PrinterMessageBoxW(ptr long ptr wstr wstr long)
@ stub ProvidorFindClosePrinterChangeNotification
@ stub ProvidorFindFirstPrinterChangeNotification
@ stub pszDbgAllocMsgA
@ stdcall ReadPrinter(long ptr long ptr)
@ stdcall ReallocSplMem(ptr long long)
@ stdcall ReallocSplStr(ptr ptr)
@ stub RemoteFindFirstPrinterChangeNotification
@ stub ReplyClosePrinter
@ stub ReplyOpenPrinter
@ stub ReplyPrinterChangeNotification
@ stdcall -stub ResetPrinterW(ptr ptr)
@ stdcall RevertToPrinterSelf()
@ stub RouterAllocBidiMem
@ stub RouterAllocBidiResponseContainer
@ stub RouterAllocPrinterNotifyInfo
@ stub RouterFindFirstPrinterChangeNotification
@ stub RouterFindNextPrinterChangeNotification
@ stub RouterFreeBidiMem
@ stub RouterFreePrinterNotifyInfo
@ stub RouterRefreshPrinterChangeNotification
@ stub RouterReplyPrinter
@ stdcall ScheduleJob(long long)
@ stdcall SeekPrinter(ptr int64 ptr long long)
@ stub SendRecvBidiData
@ stub SetAllocFailCount
@ stdcall SetFormW(ptr wstr long ptr)
@ stdcall SetJobW(long long long ptr long)
@ stdcall SetPortW(wstr wstr long ptr)
@ stdcall SetPrinterDataExW(long wstr wstr long ptr long)
@ stdcall SetPrinterDataW(long wstr long ptr long)
@ stdcall SetPrinterW(ptr long ptr long)
@ stdcall SplCloseSpoolFileHandle(ptr)
@ stdcall SplCommitSpoolData(ptr ptr long long ptr long ptr)
@ stdcall -stub SplDriverUnloadComplete(wstr)
@ stdcall SplGetSpoolFileInfo(ptr ptr long ptr long ptr)
@ stdcall SplInitializeWinSpoolDrv(ptr)
@ stub SplIsSessionZero
@ stdcall SplIsUpgrade()
@ stub SplPowerEvent
@ stub SplProcessPnPEvent
@ stub SplPromptUIInUsersSession
@ stub SplReadPrinter
@ stub SplRegisterForDeviceEvents
@ stub SplShutDownRouter
@ stub SplUnregisterForDeviceEvents
@ stub SpoolerFindClosePrinterChangeNotification
@ stub SpoolerFindFirstPrinterChangeNotification
@ stub SpoolerFindNextPrinterChangeNotification
@ stub SpoolerFreePrinterNotifyInfo
@ stub SpoolerHasInitialized
@ stdcall SpoolerInit()
@ stdcall StartDocPrinterW(long long ptr)
@ stdcall StartPagePrinter(long)
@ stub UndoAlignKMPtr
@ stdcall UndoAlignRpcPtr(ptr ptr long ptr)
@ stub UnloadDriver
@ stub UnloadDriverFile
@ stub UpdateBufferSize
@ stub UpdatePrinterRegAll
@ stub UpdatePrinterRegUser
@ stub vDbgLogError
@ stub WaitForPrinterChange
@ stub WaitForSpoolerInitialization
@ stdcall WritePrinter(long ptr long ptr)
@ stdcall XcvDataW(long wstr ptr long ptr long ptr ptr)
