@ stub AbortPrinter
@ stub AddDriverCatalog
@ stub AddFormW
@ stdcall AddJobW(long long ptr long ptr)
@ stub AddMonitorW
@ stub AddPerMachineConnectionW
@ stub AddPortExW
@ stub AddPortW
@ stub AddPrinterConnectionW
@ stub AddPrinterDriverExW
@ stub AddPrinterDriverW
@ stub AddPrinterExW
@ stub AddPrinterW
@ stub AddPrintProcessorW
@ stub AddPrintProvidorW
@ stub AdjustPointers
@ stub AdjustPointersInStructuresArray
@ stub AlignKMPtr
@ stdcall AlignRpcPtr(ptr ptr)
@ stdcall AllocSplStr(ptr)
@ stub AllowRemoteCalls
@ stub AppendPrinterNotifyInfoData
@ stub bGetDevModePerUser
@ stub bSetDevModePerUser
@ stub BuildOtherNamesFromMachineName
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
@ stub ConfigurePortW
@ stub CreatePrinterIC
@ stub DbgGetPointers
@ stub DeleteFormW
@ stub DeleteMonitorW
@ stub DeletePerMachineConnectionW
@ stub DeletePortW
@ stub DeletePrinter
@ stub DeletePrinterConnectionW
@ stub DeletePrinterDataExW
@ stub DeletePrinterDataW
@ stub DeletePrinterDriverExW
@ stub DeletePrinterDriverW
@ stub DeletePrinterIC
@ stub DeletePrinterKeyW
@ stub DeletePrintProcessorW
@ stub DeletePrintProvidorW
@ stdcall DllAllocSplMem(long)
@ stdcall DllFreeSplMem(ptr)
@ stdcall DllFreeSplStr(ptr)
@ stdcall EndDocPrinter(long)
@ stdcall EndPagePrinter(long)
@ stub EnumFormsW
@ stdcall EnumJobsW(long long long long ptr long ptr ptr)
@ stdcall EnumMonitorsW(wstr long ptr long ptr ptr)
@ stub EnumPerMachineConnectionsW
@ stdcall EnumPortsW(wstr long ptr long ptr ptr)
@ stub EnumPrinterDataExW
@ stub EnumPrinterDataW
@ stub EnumPrinterDriversW
@ stub EnumPrinterKeyW
@ stdcall EnumPrintersW(long wstr long ptr long ptr ptr)
@ stdcall EnumPrintProcessorDatatypesW(wstr wstr long ptr long ptr ptr)
@ stdcall EnumPrintProcessorsW(wstr wstr long ptr long ptr ptr)
@ stub FindClosePrinterChangeNotification
@ stub FlushPrinter
@ stub FormatPrinterForRegistryKey
@ stub FormatRegistryKeyForPrinter
@ stub FreeOtherNames
@ stub GetClientUserHandle
@ stub GetFormW
@ stub GetJobAttributes
@ stdcall GetJobW(long long long ptr long ptr)
@ stub GetNetworkId
@ stub GetPrinterDataExW
@ stub GetPrinterDataW
@ stub GetPrinterDriverDirectoryW
@ stub GetPrinterDriverExW
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
@ stub MarshallDownStructuresArray
@ stub MarshallUpStructure
@ stub MarshallUpStructuresArray
@ stub MIDL_user_allocate1
@ stub MIDL_user_free1
@ stub OldGetPrinterDriverW
@ stub OpenPrinterExW
@ stub OpenPrinterPortW
@ stdcall OpenPrinterW(wstr ptr ptr)
@ stdcall PackStrings(ptr ptr ptr ptr)
@ stub PartialReplyPrinterChangeNotification
@ stub PlayGdiScriptOnPrinterIC
@ stub PrinterHandleRundown
@ stub PrinterMessageBoxW
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
@ stub ResetPrinterW
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
@ stub SeekPrinter
@ stub SendRecvBidiData
@ stub SetAllocFailCount
@ stub SetFormW
@ stdcall SetJobW(long long long ptr long)
@ stub SetPortW
@ stub SetPrinterDataExW
@ stub SetPrinterDataW
@ stub SetPrinterW
@ stub SplCloseSpoolFileHandle
@ stub SplCommitSpoolData
@ stub SplDriverUnloadComplete
@ stub SplGetSpoolFileInfo
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
