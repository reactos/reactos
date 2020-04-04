@ stdcall AssociateColorProfileWithDeviceA(str str str)
@ stdcall AssociateColorProfileWithDeviceW(wstr wstr wstr)
@ stdcall CheckBitmapBits(ptr ptr ptr long long long ptr ptr long)
@ stdcall CheckColors(ptr ptr long long ptr)
@ stdcall CloseColorProfile(ptr)
@ stub CloseDisplay
@ stub ColorCplGetDefaultProfileScope
@ stub ColorCplGetDefaultRenderingIntentScope
@ stub ColorCplGetProfileProperties
@ stub ColorCplHasSystemWideAssociationListChanged
@ stub ColorCplInitialize
@ stub ColorCplLoadAssociationList
@ stub ColorCplMergeAssociationLists
@ stub ColorCplOverwritePerUserAssociationList
@ stub ColorCplReleaseProfileProperties
@ stub ColorCplResetSystemWideAssociationListChangedWarning
@ stub ColorCplSaveAssociationList
@ stub ColorCplSetUsePerUserProfiles
@ stub ColorCplUninitialize
@ stdcall ConvertColorNameToIndex(ptr ptr ptr long)
@ stdcall ConvertIndexToColorName(ptr ptr ptr long)
@ stdcall CreateColorTransformA(ptr ptr ptr long)
@ stdcall CreateColorTransformW(ptr ptr ptr long)
@ stdcall CreateDeviceLinkProfile(ptr long ptr long long ptr long)
@ stdcall CreateMultiProfileTransform(ptr long ptr long long long)
@ stdcall CreateProfileFromLogColorSpaceA(ptr ptr)
@ stdcall CreateProfileFromLogColorSpaceW(ptr ptr)
@ stub DccwCreateDisplayProfileAssociationList
@ stub DccwGetDisplayProfileAssociationList
@ stub DccwGetGamutSize
@ stub DccwReleaseDisplayProfileAssociationList
@ stub DccwSetDisplayProfileAssociationList
@ stdcall DeleteColorTransform(ptr)
@ stub DeviceRenameEvent
@ stdcall DisassociateColorProfileFromDeviceA(str str str)
@ stdcall DisassociateColorProfileFromDeviceW(wstr wstr wstr)
#@ stub DllCanUnloadNow
#@ stub DllGetClassObject
@ stdcall EnumColorProfilesA(str ptr ptr ptr ptr)
@ stdcall EnumColorProfilesW(wstr ptr ptr ptr ptr)
@ stdcall GenerateCopyFilePaths(wstr wstr ptr long ptr ptr ptr ptr long)
@ stdcall GetCMMInfo(ptr long)
@ stdcall GetColorDirectoryA(str ptr ptr)
@ stdcall GetColorDirectoryW(wstr ptr ptr)
@ stdcall GetColorProfileElement(ptr long long ptr ptr ptr)
@ stdcall GetColorProfileElementTag(ptr long ptr)
@ stdcall GetColorProfileFromHandle(ptr ptr ptr)
@ stdcall GetColorProfileHeader(ptr ptr)
@ stdcall GetCountColorProfileElements(ptr ptr)
@ stdcall GetNamedProfileInfo(ptr ptr)
@ stdcall GetPS2ColorRenderingDictionary(ptr long ptr ptr ptr)
@ stdcall GetPS2ColorRenderingIntent(ptr long ptr ptr)
@ stdcall GetPS2ColorSpaceArray(ptr long long ptr ptr ptr)
@ stdcall GetStandardColorSpaceProfileA(str long ptr ptr)
@ stdcall GetStandardColorSpaceProfileW(wstr long ptr ptr)
@ stdcall InstallColorProfileA(str str)
@ stdcall InstallColorProfileW(wstr wstr)
@ stub InternalGetDeviceConfig
@ stub InternalGetPS2CSAFromLCS
@ stub InternalGetPS2ColorRenderingDictionary
@ stub InternalGetPS2ColorSpaceArray
@ stub InternalGetPS2PreviewCRD
@ stub InternalRefreshCalibration
@ stub InternalSetDeviceConfig
@ stub InternalWcsAssociateColorProfileWithDevice
@ stdcall IsColorProfileTagPresent(ptr long ptr)
@ stdcall IsColorProfileValid(ptr ptr)
@ stdcall OpenColorProfileA(ptr long long long)
@ stdcall OpenColorProfileW(ptr long long long)
@ stub OpenDisplay
@ stdcall RegisterCMMA(str long str)
@ stdcall RegisterCMMW(wstr long wstr)
@ stdcall SelectCMM(long)
@ stdcall SetColorProfileElement(ptr long long ptr ptr)
@ stdcall SetColorProfileElementReference(ptr long long)
@ stdcall SetColorProfileElementSize(ptr long long)
@ stdcall SetColorProfileHeader(ptr ptr)
@ stdcall SetStandardColorSpaceProfileA(str long str)
@ stdcall SetStandardColorSpaceProfileW(wstr long wstr)
@ stdcall SpoolerCopyFileEvent(wstr wstr long)
@ stdcall TranslateBitmapBits(ptr ptr long long long long ptr long long ptr long)
@ stdcall TranslateColors(ptr ptr long long ptr long)
@ stdcall UninstallColorProfileA(str str long)
@ stdcall UninstallColorProfileW(wstr wstr long)
@ stdcall UnregisterCMMA(str long)
@ stdcall UnregisterCMMW(wstr long)
@ stub WcsAssociateColorProfileWithDevice
@ stub WcsCheckColors
@ stub WcsCreateIccProfile
@ stub WcsDisassociateColorProfileFromDevice
@ stub WcsEnumColorProfiles
@ stdcall WcsEnumColorProfilesSize(long ptr ptr)
@ stub WcsGetCalibrationManagementState
@ stub WcsGetDefaultColorProfile
@ stub WcsGetDefaultColorProfileSize
@ stub WcsGetDefaultRenderingIntent
@ stdcall WcsGetUsePerUserProfiles(wstr long ptr)
@ stub WcsGpCanInstallOrUninstallProfiles
@ stdcall WcsOpenColorProfileA(ptr ptr ptr long long long long)
@ stdcall WcsOpenColorProfileW(ptr ptr ptr long long long long)
@ stub WcsSetCalibrationManagementState
@ stub WcsSetDefaultColorProfile
@ stub WcsSetDefaultRenderingIntent
@ stub WcsSetUsePerUserProfiles
@ stub WcsTranslateColors
