@ stub WerSysprepCleanup
@ stub WerSysprepGeneralize
@ stub WerSysprepSpecialize
@ stub WerUnattendedSetup
@ stub WerpAddAppCompatData
@ stub WerpAddFile
@ stub WerpAddMemoryBlock
@ stub WerpAddRegisteredDataToReport
@ stub WerpAddSecondaryParameter
@ stub WerpAddTextToReport
@ stub WerpArchiveReport
@ stub WerpCancelResponseDownload
@ stub WerpCancelUpload
@ stub WerpCloseStore
@ stub WerpCreateMachineStore
@ stub WerpDeleteReport
@ stub WerpDestroyWerString
@ stub WerpDownloadResponse
@ stub WerpDownloadResponseTemplate
@ stub WerpEnumerateStoreNext
@ stub WerpEnumerateStoreStart
@ stub WerpExtractReportFiles
@ stub WerpGetBucketId
@ stub WerpGetDynamicParameter
@ stub WerpGetEventType
@ stub WerpGetFileByIndex
@ stub WerpGetFilePathByIndex
@ stub WerpGetNumFiles
@ stub WerpGetNumSecParams
@ stub WerpGetNumSigParams
@ stub WerpGetReportFinalConsent
@ stub WerpGetReportFlags
@ stub WerpGetReportInformation
@ stub WerpGetReportTime
@ stub WerpGetReportType
@ stub WerpGetResponseId
@ stub WerpGetResponseUrl
@ stub WerpGetSecParamByIndex
@ stub WerpGetSigParamByIndex
@ stub WerpGetStoreLocation
@ stub WerpGetStoreType
@ stub WerpGetTextFromReport
@ stub WerpGetUIParamByIndex
@ stub WerpGetUploadTime
@ stub WerpGetWerStringData
@ stub WerpIsTransportAvailable
@ stub WerpLoadReport
@ stub WerpOpenMachineArchive
@ stub WerpOpenMachineQueue
@ stub WerpOpenUserArchive
@ stub WerpReportCancel
@ stub WerpRestartApplication
@ stub WerpSetDynamicParameter
@ stub WerpSetEventName
@ stub WerpSetReportFlags
@ stub WerpSetReportInformation
@ stub WerpSetReportTime
@ stub WerpSetReportUploadContextToken
@ stub WerpShowNXNotification
@ stub WerpShowSecondLevelConsent
@ stub WerpShowUpsellUI
@ stub WerpSubmitReportFromStore
@ stub WerpSvcReportFromMachineQueue
@ stdcall WerAddExcludedApplication(wstr long)
@ stdcall WerRemoveExcludedApplication(wstr long)
@ stdcall WerReportAddDump(ptr ptr ptr long ptr ptr long)
@ stdcall WerReportAddFile(ptr wstr long long)
@ stdcall WerReportCloseHandle(ptr)
@ stdcall WerReportCreate(wstr long ptr ptr)
@ stdcall WerReportSetParameter(ptr long wstr wstr)
@ stdcall WerReportSetUIOption(ptr long wstr)
@ stdcall WerReportSubmit(ptr long long ptr)
@ stub WerpGetReportConsent
@ stub WerpIsDisabled
@ stub WerpOpenUserQueue
@ stub WerpPromtUser
@ stub WerpSetCallBack
