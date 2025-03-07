////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////

#ifndef __DWUDF_REGISTRY__H__
#define __DWUDF_REGISTRY__H__

#define         DWN_MANAGER_SVC_NAME        "DwUdfMgr"
#define         DWN_MANAGER_PIPE_NAME       "\\\\.\\pipe\\DwUdfManager"

#define UDF_SERVICE                         TEXT("DwUdf")
#define CDRW_SERVICE                        TEXT("DwCdrw")

#define         UDF_KEY                     TEXT("Software\\DVD Write Now\\UDF")
#define         CDRW_SERVICE_PATH           TEXT("SYSTEM\\CurrentControlSet\\Services\\") CDRW_SERVICE

#define         UDF_SERVICE_PATH            TEXT("SYSTEM\\CurrentControlSet\\Services\\") UDF_SERVICE
#define         UDF_SERVICE_PATH_W          L"SYSTEM\\CurrentControlSet\\Services\\DwUdf"

#define         UDF_SERVICE_PARAM_PATH      TEXT("SYSTEM\\CurrentControlSet\\Services\\DwUdf\\Parameters")
#define         UDF_SERVICE_PARAM_PATH_W    L"SYSTEM\\CurrentControlSet\\Services\\DwUdf\\Parameters"

#define         UDF_SERVICE_PATH_DEAULT     TEXT("SYSTEM\\CurrentControlSet\\Services\\DwUdf\\Parameters_Default")
#define         UDF_SERVICE_PATH_DEAULT_W   L"SYSTEM\\CurrentControlSet\\Services\\DwUdf\\Parameters_Default"

#define         CDROM_CLASS_PATH            TEXT("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E965-E325-11CE-BFC1-08002BE10318}")
#define         REG_UPPER_FILTER_NAME       TEXT("UpperFilters")

#define         UDF_FS_NAME                 L"\\Udf"
#define         UDF_FS_NAME_CD              L"\\UdfCd"
#define         UDF_FS_NAME_HDD             L"\\UdfHdd"
//#define         UDF_FS_NAME_VIRT            L"\\UdfVirt"
#define         UDF_FS_NAME_OTHER           L"\\UdfOther"
#define         UDF_FS_NAME_TAPE            L"\\UdfTape"

#define         UDF_DOS_FS_NAME             L"\\DosDevices\\DwUdf"
#define         UDF_DOS_FS_NAME_USER        "\\\\.\\DwUdf"

#define         CDFS_REC_DEVICE_OBJECT_NAME L"\\CdfsRecognizer"
#define         CDFS_DEVICE_OBJECT_NAME     L"\\Cdfs"
#define         UDFS_REC_DEVICE_OBJECT_NAME L"\\UdfsCdromRecognizer"
#define         UDFS_DEVICE_OBJECT_NAME     L"\\UdfsCdrom"
#define         UDFSD_REC_DEVICE_OBJECT_NAME L"\\UdfsDiskRecognizer"
#define         UDFSD_DEVICE_OBJECT_NAME     L"\\UdfsDisk"

#define         UDF_ROOTDIR_NAME            L"\\"
#define         UDF_SN_NT_SYM_LINK          L"$UDF NT SymLink"

#ifndef PRETEND_NTFS
#define         UDF_FS_TITLE_DVDRAM         L"UDF-DVDRAM"
#define         UDF_FS_TITLE_DVDpRW         L"UDF-DVD+RW"
#define         UDF_FS_TITLE_DVDpR          L"UDF-DVD+R"
#define         UDF_FS_TITLE_DVDRW          L"UDF-DVDRW"
#define         UDF_FS_TITLE_DVDR           L"UDF-DVDR"
#define         UDF_FS_TITLE_DVDROM         L"UDF-DVDROM"
#define         UDF_FS_TITLE_CDRW           L"UDF-CDRW"
#define         UDF_FS_TITLE_CDR            L"UDF-CDR"
#define         UDF_FS_TITLE_CDROM          L"UDF-CDROM"
#define         UDF_FS_TITLE_HDD            L"UDF"
#else  //PRETEND_NTFS
#define         UDF_FS_TITLE_DVDRAM         L"NTFS"
#define         UDF_FS_TITLE_DVDpR          L"NTFS"
#define         UDF_FS_TITLE_DVDpR          L"NTFS"
#define         UDF_FS_TITLE_DVDRW          L"NTFS"
#define         UDF_FS_TITLE_DVDR           L"NTFS"
#define         UDF_FS_TITLE_DVDROM         L"NTFS"
#define         UDF_FS_TITLE_CDRW           L"NTFS"
#define         UDF_FS_TITLE_CDR            L"NTFS"
#define         UDF_FS_TITLE_CDROM          L"NTFS"
#define         UDF_FS_TITLE_HDD            L"NTFS"
#endif //PRETEND_NTFS

#define         REG_DEFAULT_UNKNOWN         L"_Default\\Unknown"
#define         REG_DEFAULT_HDD             L"_Default\\Hdd"
#define         REG_DEFAULT_CDR             L"_Default\\Cdr"
#define         REG_DEFAULT_CDRW            L"_Default\\Cdrw"
#define         REG_DEFAULT_CDROM           L"_Default\\Cdrom"
#define         REG_DEFAULT_ZIP             L"_Default\\Zip"
#define         REG_DEFAULT_FLOPPY          L"_Default\\Floppy"
#define         REG_DEFAULT_DVDR            L"_Default\\Dvdr"
#define         REG_DEFAULT_DVDRW           L"_Default\\Dvdrw"

#define         REG_NAMELESS_DEV            L"\\_Nameless_"

#define         UDF_DEFAULT_LABEL           L"Write Now"
#define         UDF_DEFAULT_LABEL_USER      "Write Now"
#define         UDF_MAX_LABEL_LENGTH        11  // Windows shell limitation

#define         UDF_FS_TITLE_BLANK          L"Blank media"
#define         UDF_FS_TITLE_UNKNOWN        L"Unknown"
#define         UDF_BLANK_VOLUME_LABEL      L"Blank CD"

#define         REG_CD_BURNER_KEY_NAME      L"\\REGISTRY\\USER\\CURRENTUSER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CD Burning"
#define         REG_CD_BURNER_VOLUME_NAME   L"CD Recorder Drive"

#define         REG_USEEXTENDEDFE_NAME      L"UseExtendedFE"
#define         REG_USEEXTENDEDFE_NAME_USER "UseExtendedFE"

#define         REG_DEFALLOCMODE_NAME       L"DefaultAllocMode"
#define         REG_DEFALLOCMODE_NAME_USER  "DefaultAllocMode"

#define         UDF_DEFAULT_UID_NAME        L"DefaultUID"
#define         UDF_DEFAULT_UID_NAME_USER   "DefaultUID"

#define         UDF_DEFAULT_GID_NAME        L"DefaultGID"
#define         UDF_DEFAULT_GID_NAME_USER   "DefaultGID"

#define         UDF_DIR_PACK_THRESHOLD_NAME L"PackDirThreshold"
#define         UDF_DIR_PACK_THRESHOLD_NAME_USER "PackDirThreshold"

#define         UDF_FE_CHARGE_NAME          L"FECharge"
#define         UDF_FE_CHARGE_NAME_USER     "FECharge"

#define         UDF_FE_CHARGE_SDIR_NAME     L"FEChargeSDir"
#define         UDF_FE_CHARGE_SDIR_NAME_USER "FEChargeSDir"

#define         UDF_BM_FLUSH_PERIOD_NAME    L"BitmapFlushPeriod"
#define         UDF_BM_FLUSH_PERIOD_NAME_USER "BitmapFlushPeriod"

#define         UDF_TREE_FLUSH_PERIOD_NAME  L"DirTreeFlushPeriod"
#define         UDF_TREE_FLUSH_PERIOD_NAME_USER "DirTreeFlushPeriod"

#define         UDF_NO_UPDATE_PERIOD_NAME   L"MaxNoUpdatePeriod"
#define         UDF_NO_UPDATE_PERIOD_NAME_USER "MaxNoUpdatePeriod"

#define         UDF_NO_EJECT_PERIOD_NAME   L"MaxNoEjectPeriod"
#define         UDF_NO_EJECT_PERIOD_NAME_USER "MaxNoEjectPeriod"

#define         UDF_FSP_THREAD_PER_CPU_NAME L"ThreadsPerCpu"
#define         UDF_FSP_THREAD_PER_CPU_NAME_USER "ThreadsPerCpu"

#define         UDF_READAHEAD_GRAN_NAME     L"ReadAheadGranlarity"
#define         UDF_READAHEAD_GRAN_NAME_USER "ReadAheadGranlarity"

/*#define         UDF_W_SECURITY_CDRW_NAME    L"WriteSecurityOnCDRW"
#define         UDF_W_SECURITY_CDRW_NAME_USER "WriteSecurityOnCDRW"

#define         UDF_W_SECURITY_CDR_NAME     L"WriteSecurityOnCDR"
#define         UDF_W_SECURITY_CDR_NAME_USER "WriteSecurityOnCDR"*/

#define         UDF_SPARSE_THRESHOLD_NAME   L"SparseThreshold"
#define         UDF_SPARSE_THRESHOLD_NAME_USER "SparseThreshold"

#define         UDF_VERIFY_ON_WRITE_NAME    L"VerifyOnWrite"
#define         UDF_VERIFY_ON_WRITE_NAME_USER "VerifyOnWrite"

#define         UDF_UPDATE_TIMES_ATTR       L"UpdateFileTimesAttrChg"
#define         UDF_UPDATE_TIMES_ATTR_USER  "UpdateFileTimesAttrChg"

#define         UDF_UPDATE_TIMES_MOD        L"UpdateFileTimesLastWrite"
#define         UDF_UPDATE_TIMES_MOD_USER   "UpdateFileTimesLastWrite"

#define         UDF_UPDATE_TIMES_ACCS       L"UpdateFileTimesLastAccess"
#define         UDF_UPDATE_TIMES_ACCS_USER  "UpdateFileTimesLastAccess"

#define         UDF_UPDATE_ATTR_ARCH        L"UpdateFileAttrArchive"
#define         UDF_UPDATE_ATTR_ARCH_USER   "UpdateFileAttrArchive"

#define         UDF_UPDATE_DIR_TIMES_ATTR_W L"UpdateDirAttrAndTimesOnModify"
#define         UDF_UPDATE_DIR_TIMES_ATTR_W_USER "UpdateDirAttrAndTimesOnModify"

#define         UDF_UPDATE_DIR_TIMES_ATTR_R L"UpdateDirAttrAndTimesOnAccess"
#define         UDF_UPDATE_DIR_TIMES_ATTR_R_USER "UpdateDirAttrAndTimesOnAccess"

#define         UDF_ALLOW_WRITE_IN_RO_DIR   L"AllowCreateInsideReadOnlyDirectory"
#define         UDF_ALLOW_WRITE_IN_RO_DIR_USER "AllowCreateInsideReadOnlyDirectory"

#define         UDF_ALLOW_UPDATE_TIMES_ACCS_UCHG_DIR L"AllowUpdateAccessTimeInUnchangedDir"
#define         UDF_ALLOW_UPDATE_TIMES_ACCS_UCHG_DIR_USER "AllowUpdateAccessTimeInUnchangedDir"

#define         UDF_W2K_COMPAT_ALLOC_DESCS  L"AllocDescCompatW2K"
#define         UDF_W2K_COMPAT_ALLOC_DESCS_USER "AllocDescCompatW2K"

#define         UDF_W2K_COMPAT_VLABEL       L"VolumeLabelCompatW2K"
#define         UDF_W2K_COMPAT_VLABEL_USER  "VolumeLabelCompatW2K"

#define         UDF_INSTANT_COMPAT_ALLOC_DESCS  L"AllocDescCompatInstantBurner"
#define         UDF_INSTANT_COMPAT_ALLOC_DESCS_USER "AllocDescCompatInstantBurner"

#define         UDF_HANDLE_HW_RO            L"HandleHWReadOnly"
#define         UDF_HANDLE_HW_RO_USER       "HandleHWReadOnly"

#define         UDF_HANDLE_SOFT_RO          L"HandleSoftReadOnly"
#define         UDF_HANDLE_SOFT_RO_USER     "HandleSoftReadOnly"

#define         UDF_FLUSH_MEDIA             L"FlushMedia"
#define         UDF_FLUSH_MEDIA_USER        "FlushMedia"

#define         UDF_FORCE_MOUNT_ALL         L"ForcedMountAllAsUDF"
#define         UDF_FORCE_MOUNT_ALL_USER    "ForcedMountAllAsUDF"

#define         UDF_COMPARE_BEFORE_WRITE    L"CompareBeforeWrite"
#define         UDF_COMPARE_BEFORE_WRITE_USER "CompareBeforeWrite"

#define         UDF_CACHE_SIZE_MULTIPLIER   L"WCacheSizeMultiplier"
#define         UDF_CACHE_SIZE_MULTIPLIER_USER "WCacheSizeMultiplier"

#define         UDF_CHAINED_IO              L"CacheChainedIo"
#define         UDF_CHAINED_IO_USER         "CacheChainedIo"

#define         UDF_OS_NATIVE_DOS_NAME      L"UseOsNativeDOSName"
#define         UDF_OS_NATIVE_DOS_NAME_USER "UseOsNativeDOSName"

#define         UDF_FORCE_WRITE_THROUGH_NAME L"ForceWriteThrough"
#define         UDF_FORCE_WRITE_THROUGH_NAME_USER "ForceWriteThrough"

#define         UDF_FORCE_HW_RO             L"ForceHWReadOnly"
#define         UDF_FORCE_HW_RO_USER        "ForceHWReadOnly"

#define         UDF_IGNORE_SEQUENTIAL_IO    L"IgnoreSequantialIo"
#define         UDF_IGNORE_SEQUENTIAL_IO_USER "IgnoreSequantialIo"

#define         UDF_PART_DAMAGED_BEHAVIOR   L"PartitialDamagedVolumeAction"
#define         UDF_PART_DAMAGED_BEHAVIOR_USER "PartitialDamagedVolumeAction"

#define         UDF_NO_SPARE_BEHAVIOR       L"NoFreeRelocationSpaceVolumeAction"
#define         UDF_NO_SPARE_BEHAVIOR_USER  "NoFreeRelocationSpaceVolumeAction"

#define         UDF_DIRTY_VOLUME_BEHAVIOR   L"DirtyVolumeVolumeAction"
#define         UDF_DIRTY_VOLUME_BEHAVIOR_USER "DirtyVolumeVolumeAction"

#define         UDF_SHOW_BLANK_CD           L"ShowBlankCd"
#define         UDF_SHOW_BLANK_CD_USER      "ShowBlankCd"

#define         UDF_WAIT_CD_SPINUP          L"WaitCdSpinUpOnMount"
#define         UDF_WAIT_CD_SPINUP_USER     "WaitCdSpinUpOnMount"

#define         UDF_AUTOFORMAT              L"Autoformat"
#define         UDF_AUTOFORMAT_USER         "Autoformat"

#define         UDF_CACHE_BAD_VDS           L"CacheBadVDSLocations"
#define         UDF_CACHE_BAD_VDS_USER      "CacheBadVDSLocations"

#define         UDF_USE_EJECT_BUTTON        L"UseEjectButton"
#define         UDF_USE_EJECT_BUTTON_USER   "UseEjectButton"

#define         UDF_LICENSE_KEY             L"LicenseKey"
#define         UDF_LICENSE_KEY_USER        "LicenseKey"

#define         REG_MOUNT_ON_CDONLY_NAME      L"Mount_CdOnly"
#define         REG_MOUNT_ON_CDONLY_NAME_USER "Mount_CdOnly"

#define         REG_MOUNT_ON_HDD_NAME      L"Mount_Hdd"
#define         REG_MOUNT_ON_HDD_NAME_USER "Mount_Hdd"

#define         REG_MOUNT_ON_ZIP_NAME      L"Mount_Zip"
#define         REG_MOUNT_ON_ZIP_NAME_USER "Mount_Zip"


/// Name of the console formatter tool
#define UDFFMT        TEXT("DwConFmtUdf.exe")
#define UDFFMTGUI     TEXT("DwGuiFmtUdf.exe")
#define REGISTER_APP  TEXT("DwRegister.exe")


#endif //__DWUDF_REGISTRY__H__
