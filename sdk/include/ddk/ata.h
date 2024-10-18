#ifndef _NTATA_
#define _NTATA_

#include <pshpack1.h>
typedef struct _IDENTIFY_DEVICE_DATA {
  struct {
    USHORT Reserved1  :1;
    USHORT Retired3  :1;
    USHORT ResponseIncomplete  :1;
    USHORT Retired2  :3;
    USHORT FixedDevice  :1;
    USHORT RemovableMedia  :1;
    USHORT Retired1  :7;
    USHORT DeviceType  :1;
  } GeneralConfiguration;
  USHORT NumCylinders;
  USHORT SpecificConfiguration;
  USHORT NumHeads;
  USHORT Retired1[2];
  USHORT NumSectorsPerTrack;
  USHORT VendorUnique1[3];
  UCHAR  SerialNumber[20];
  USHORT Retired2[2];
  USHORT Obsolete1;
  UCHAR  FirmwareRevision[8];
  UCHAR  ModelNumber[40];
  UCHAR  MaximumBlockTransfer;
  UCHAR  VendorUnique2;
  struct {
    USHORT FeatureSupported  :1;
    USHORT Reserved  :15;
  } TrustedComputing;
  struct {
    UCHAR  CurrentLongPhysicalSectorAlignment  :2;
    UCHAR  ReservedByte49  :6;
    UCHAR  DmaSupported  :1;
    UCHAR  LbaSupported  :1;
    UCHAR  IordyDisable  :1;
    UCHAR  IordySupported  :1;
    UCHAR  Reserved1  :1;
    UCHAR  StandybyTimerSupport  :1;
    UCHAR  Reserved2  :2;
    USHORT ReservedWord50;
  } Capabilities;
  USHORT ObsoleteWords51[2];
  USHORT TranslationFieldsValid  :3;
  USHORT Reserved3  :5;
  USHORT FreeFallControlSensitivity  :8;
  USHORT NumberOfCurrentCylinders;
  USHORT NumberOfCurrentHeads;
  USHORT CurrentSectorsPerTrack;
  ULONG  CurrentSectorCapacity;
  UCHAR  CurrentMultiSectorSetting;
  UCHAR  MultiSectorSettingValid  :1;
  UCHAR  ReservedByte59  :3;
  UCHAR  SanitizeFeatureSupported  :1;
  UCHAR  CryptoScrambleExtCommandSupported  :1;
  UCHAR  OverwriteExtCommandSupported  :1;
  UCHAR  BlockEraseExtCommandSupported  :1;
  ULONG  UserAddressableSectors;
  USHORT ObsoleteWord62;
  USHORT MultiWordDMASupport  :8;
  USHORT MultiWordDMAActive  :8;
  USHORT AdvancedPIOModes  :8;
  USHORT ReservedByte64  :8;
  USHORT MinimumMWXferCycleTime;
  USHORT RecommendedMWXferCycleTime;
  USHORT MinimumPIOCycleTime;
  USHORT MinimumPIOCycleTimeIORDY;
  struct {
    USHORT ZonedCapabilities  :2;
    USHORT NonVolatileWriteCache  :1;
    USHORT ExtendedUserAddressableSectorsSupported  :1;
    USHORT DeviceEncryptsAllUserData  :1;
    USHORT ReadZeroAfterTrimSupported  :1;
    USHORT Optional28BitCommandsSupported  :1;
    USHORT IEEE1667  :1;
    USHORT DownloadMicrocodeDmaSupported  :1;
    USHORT SetMaxSetPasswordUnlockDmaSupported  :1;
    USHORT WriteBufferDmaSupported  :1;
    USHORT ReadBufferDmaSupported  :1;
    USHORT DeviceConfigIdentifySetDmaSupported  :1;
    USHORT LPSAERCSupported  :1;
    USHORT DeterministicReadAfterTrimSupported  :1;
    USHORT CFastSpecSupported  :1;
  } AdditionalSupported;
  USHORT ReservedWords70[5];
  USHORT QueueDepth  :5;
  USHORT ReservedWord75  :11;
  struct {
    USHORT Reserved0  :1;
    USHORT SataGen1  :1;
    USHORT SataGen2  :1;
    USHORT SataGen3  :1;
    USHORT Reserved1  :4;
    USHORT NCQ  :1;
    USHORT HIPM  :1;
    USHORT PhyEvents  :1;
    USHORT NcqUnload  :1;
    USHORT NcqPriority  :1;
    USHORT HostAutoPS  :1;
    USHORT DeviceAutoPS  :1;
    USHORT ReadLogDMA  :1;
    USHORT Reserved2  :1;
    USHORT CurrentSpeed  :3;
    USHORT NcqStreaming  :1;
    USHORT NcqQueueMgmt  :1;
    USHORT NcqReceiveSend  :1;
    USHORT DEVSLPtoReducedPwrState  :1;
    USHORT Reserved3  :8;
  } SerialAtaCapabilities;
  struct {
    USHORT Reserved0  :1;
    USHORT NonZeroOffsets  :1;
    USHORT DmaSetupAutoActivate  :1;
    USHORT DIPM  :1;
    USHORT InOrderData  :1;
    USHORT HardwareFeatureControl  :1;
    USHORT SoftwareSettingsPreservation  :1;
    USHORT NCQAutosense  :1;
    USHORT DEVSLP  :1;
    USHORT HybridInformation  :1;
    USHORT Reserved1  :6;
  } SerialAtaFeaturesSupported;
  struct {
    USHORT Reserved0  :1;
    USHORT NonZeroOffsets  :1;
    USHORT DmaSetupAutoActivate  :1;
    USHORT DIPM  :1;
    USHORT InOrderData  :1;
    USHORT HardwareFeatureControl  :1;
    USHORT SoftwareSettingsPreservation  :1;
    USHORT DeviceAutoPS  :1;
    USHORT DEVSLP  :1;
    USHORT HybridInformation  :1;
    USHORT Reserved1  :6;
  } SerialAtaFeaturesEnabled;
  USHORT MajorRevision;
  USHORT MinorRevision;
  struct {
    USHORT SmartCommands  :1;
    USHORT SecurityMode  :1;
    USHORT RemovableMediaFeature  :1;
    USHORT PowerManagement  :1;
    USHORT Reserved1  :1;
    USHORT WriteCache  :1;
    USHORT LookAhead  :1;
    USHORT ReleaseInterrupt  :1;
    USHORT ServiceInterrupt  :1;
    USHORT DeviceReset  :1;
    USHORT HostProtectedArea  :1;
    USHORT Obsolete1  :1;
    USHORT WriteBuffer  :1;
    USHORT ReadBuffer  :1;
    USHORT Nop  :1;
    USHORT Obsolete2  :1;
    USHORT DownloadMicrocode  :1;
    USHORT DmaQueued  :1;
    USHORT Cfa  :1;
    USHORT AdvancedPm  :1;
    USHORT Msn  :1;
    USHORT PowerUpInStandby  :1;
    USHORT ManualPowerUp  :1;
    USHORT Reserved2  :1;
    USHORT SetMax  :1;
    USHORT Acoustics  :1;
    USHORT BigLba  :1;
    USHORT DeviceConfigOverlay  :1;
    USHORT FlushCache  :1;
    USHORT FlushCacheExt  :1;
    USHORT WordValid83  :2;
    USHORT SmartErrorLog  :1;
    USHORT SmartSelfTest  :1;
    USHORT MediaSerialNumber  :1;
    USHORT MediaCardPassThrough  :1;
    USHORT StreamingFeature  :1;
    USHORT GpLogging  :1;
    USHORT WriteFua  :1;
    USHORT WriteQueuedFua  :1;
    USHORT WWN64Bit  :1;
    USHORT URGReadStream  :1;
    USHORT URGWriteStream  :1;
    USHORT ReservedForTechReport  :2;
    USHORT IdleWithUnloadFeature  :1;
    USHORT WordValid  :2;
  } CommandSetSupport;
  struct {
    USHORT SmartCommands  :1;
    USHORT SecurityMode  :1;
    USHORT RemovableMediaFeature  :1;
    USHORT PowerManagement  :1;
    USHORT Reserved1  :1;
    USHORT WriteCache  :1;
    USHORT LookAhead  :1;
    USHORT ReleaseInterrupt  :1;
    USHORT ServiceInterrupt  :1;
    USHORT DeviceReset  :1;
    USHORT HostProtectedArea  :1;
    USHORT Obsolete1  :1;
    USHORT WriteBuffer  :1;
    USHORT ReadBuffer  :1;
    USHORT Nop  :1;
    USHORT Obsolete2  :1;
    USHORT DownloadMicrocode  :1;
    USHORT DmaQueued  :1;
    USHORT Cfa  :1;
    USHORT AdvancedPm  :1;
    USHORT Msn  :1;
    USHORT PowerUpInStandby  :1;
    USHORT ManualPowerUp  :1;
    USHORT Reserved2  :1;
    USHORT SetMax  :1;
    USHORT Acoustics  :1;
    USHORT BigLba  :1;
    USHORT DeviceConfigOverlay  :1;
    USHORT FlushCache  :1;
    USHORT FlushCacheExt  :1;
    USHORT Resrved3  :1;
    USHORT Words119_120Valid  :1;
    USHORT SmartErrorLog  :1;
    USHORT SmartSelfTest  :1;
    USHORT MediaSerialNumber  :1;
    USHORT MediaCardPassThrough  :1;
    USHORT StreamingFeature  :1;
    USHORT GpLogging  :1;
    USHORT WriteFua  :1;
    USHORT WriteQueuedFua  :1;
    USHORT WWN64Bit  :1;
    USHORT URGReadStream  :1;
    USHORT URGWriteStream  :1;
    USHORT ReservedForTechReport  :2;
    USHORT IdleWithUnloadFeature  :1;
    USHORT Reserved4  :2;
  } CommandSetActive;
  USHORT UltraDMASupport  :8;
  USHORT UltraDMAActive  :8;
  struct {
    USHORT TimeRequired  :15;
    USHORT ExtendedTimeReported  :1;
  } NormalSecurityEraseUnit;
  struct {
    USHORT TimeRequired  :15;
    USHORT ExtendedTimeReported  :1;
  } EnhancedSecurityEraseUnit;
  USHORT CurrentAPMLevel  :8;
  USHORT ReservedWord91  :8;
  USHORT MasterPasswordID;
  USHORT HardwareResetResult;
  USHORT CurrentAcousticValue  :8;
  USHORT RecommendedAcousticValue  :8;
  USHORT StreamMinRequestSize;
  USHORT StreamingTransferTimeDMA;
  USHORT StreamingAccessLatencyDMAPIO;
  ULONG  StreamingPerfGranularity;
  ULONG  Max48BitLBA[2];
  USHORT StreamingTransferTime;
  USHORT DsmCap;
  struct {
    USHORT LogicalSectorsPerPhysicalSector  :4;
    USHORT Reserved0  :8;
    USHORT LogicalSectorLongerThan256Words  :1;
    USHORT MultipleLogicalSectorsPerPhysicalSector  :1;
    USHORT Reserved1  :2;
  } PhysicalLogicalSectorSize;
  USHORT InterSeekDelay;
  USHORT WorldWideName[4];
  USHORT ReservedForWorldWideName128[4];
  USHORT ReservedForTlcTechnicalReport;
  USHORT WordsPerLogicalSector[2];
  struct {
    USHORT ReservedForDrqTechnicalReport  :1;
    USHORT WriteReadVerify  :1;
    USHORT WriteUncorrectableExt  :1;
    USHORT ReadWriteLogDmaExt  :1;
    USHORT DownloadMicrocodeMode3  :1;
    USHORT FreefallControl  :1;
    USHORT SenseDataReporting  :1;
    USHORT ExtendedPowerConditions  :1;
    USHORT Reserved0  :6;
    USHORT WordValid  :2;
  } CommandSetSupportExt;
  struct {
    USHORT ReservedForDrqTechnicalReport  :1;
    USHORT WriteReadVerify  :1;
    USHORT WriteUncorrectableExt  :1;
    USHORT ReadWriteLogDmaExt  :1;
    USHORT DownloadMicrocodeMode3  :1;
    USHORT FreefallControl  :1;
    USHORT SenseDataReporting  :1;
    USHORT ExtendedPowerConditions  :1;
    USHORT Reserved0  :6;
    USHORT Reserved1  :2;
  } CommandSetActiveExt;
  USHORT ReservedForExpandedSupportandActive[6];
  USHORT MsnSupport  :2;
  USHORT ReservedWord127  :14;
  struct {
    USHORT SecuritySupported  :1;
    USHORT SecurityEnabled  :1;
    USHORT SecurityLocked  :1;
    USHORT SecurityFrozen  :1;
    USHORT SecurityCountExpired  :1;
    USHORT EnhancedSecurityEraseSupported  :1;
    USHORT Reserved0  :2;
    USHORT SecurityLevel  :1;
    USHORT Reserved1  :7;
  } SecurityStatus;
  USHORT ReservedWord129[31];
  struct {
    USHORT MaximumCurrentInMA  :12;
    USHORT CfaPowerMode1Disabled  :1;
    USHORT CfaPowerMode1Required  :1;
    USHORT Reserved0  :1;
    USHORT Word160Supported  :1;
  } CfaPowerMode1;
  USHORT ReservedForCfaWord161[7];
  USHORT NominalFormFactor  :4;
  USHORT ReservedWord168  :12;
  struct {
    USHORT SupportsTrim  :1;
    USHORT Reserved0  :15;
  } DataSetManagementFeature;
  USHORT AdditionalProductID[4];
  USHORT ReservedForCfaWord174[2];
  USHORT CurrentMediaSerialNumber[30];
  struct {
    USHORT Supported  :1;
    USHORT Reserved0  :1;
    USHORT WriteSameSuported  :1;
    USHORT ErrorRecoveryControlSupported  :1;
    USHORT FeatureControlSuported  :1;
    USHORT DataTablesSuported  :1;
    USHORT Reserved1  :6;
    USHORT VendorSpecific  :4;
  } SCTCommandTransport;
  USHORT ReservedWord207[2];
  struct {
    USHORT AlignmentOfLogicalWithinPhysical  :14;
    USHORT Word209Supported  :1;
    USHORT Reserved0  :1;
  } BlockAlignment;
  USHORT WriteReadVerifySectorCountMode3Only[2];
  USHORT WriteReadVerifySectorCountMode2Only[2];
  struct {
    USHORT NVCachePowerModeEnabled  :1;
    USHORT Reserved0  :3;
    USHORT NVCacheFeatureSetEnabled  :1;
    USHORT Reserved1  :3;
    USHORT NVCachePowerModeVersion  :4;
    USHORT NVCacheFeatureSetVersion  :4;
  } NVCacheCapabilities;
  USHORT NVCacheSizeLSW;
  USHORT NVCacheSizeMSW;
  USHORT NominalMediaRotationRate;
  USHORT ReservedWord218;
  struct {
    UCHAR NVCacheEstimatedTimeToSpinUpInSeconds;
    UCHAR Reserved;
  } NVCacheOptions;
  USHORT WriteReadVerifySectorCountMode  :8;
  USHORT ReservedWord220  :8;
  USHORT ReservedWord221;
  struct {
    USHORT MajorVersion  :12;
    USHORT TransportType  :4;
  } TransportMajorVersion;
  USHORT TransportMinorVersion;
  USHORT ReservedWord224[6];
  ULONG  ExtendedNumberOfUserAddressableSectors[2];
  USHORT MinBlocksPerDownloadMicrocodeMode03;
  USHORT MaxBlocksPerDownloadMicrocodeMode03;
  USHORT ReservedWord236[19];
  USHORT Signature  :8;
  USHORT CheckSum  :8;
} IDENTIFY_DEVICE_DATA, *PIDENTIFY_DEVICE_DATA;

typedef struct _IDENTIFY_PACKET_DATA {
  struct {
    USHORT PacketType  :2;
    USHORT IncompleteResponse  :1;
    USHORT Reserved1  :2;
    USHORT DrqDelay  :2;
    USHORT RemovableMedia  :1;
    USHORT CommandPacketType  :5;
    USHORT Reserved2  :1;
    USHORT DeviceType  :2;
  } GeneralConfiguration;
  USHORT ResevedWord1;
  USHORT UniqueConfiguration;
  USHORT ReservedWords3[7];
  UCHAR  SerialNumber[20];
  USHORT ReservedWords20[3];
  UCHAR  FirmwareRevision[8];
  UCHAR  ModelNumber[40];
  USHORT ReservedWords47[2];
  struct {
    USHORT VendorSpecific  :8;
    USHORT DmaSupported  :1;
    USHORT LbaSupported  :1;
    USHORT IordyDisabled  :1;
    USHORT IordySupported  :1;
    USHORT Obsolete  :1;
    USHORT OverlapSupported  :1;
    USHORT QueuedCommandsSupported  :1;
    USHORT InterleavedDmaSupported  :1;
    USHORT DeviceSpecificStandbyTimerValueMin  :1;
    USHORT Obsolete1  :1;
    USHORT ReservedWord50  :12;
    USHORT WordValid  :2;
  } Capabilities;
  USHORT ObsoleteWords51[2];
  USHORT TranslationFieldsValid  :3;
  USHORT Reserved3  :13;
  USHORT ReservedWords54[8];
  struct {
    USHORT UDMA0Supported  :1;
    USHORT UDMA1Supported  :1;
    USHORT UDMA2Supported  :1;
    USHORT UDMA3Supported  :1;
    USHORT UDMA4Supported  :1;
    USHORT UDMA5Supported  :1;
    USHORT UDMA6Supported  :1;
    USHORT MDMA0Supported  :1;
    USHORT MDMA1Supported  :1;
    USHORT MDMA2Supported  :1;
    USHORT DMASupported  :1;
    USHORT ReservedWord62  :4;
    USHORT DMADIRBitRequired  :1;
  } DMADIR;
  USHORT MultiWordDMASupport  :8;
  USHORT MultiWordDMAActive  :8;
  USHORT AdvancedPIOModes  :8;
  USHORT ReservedByte64  :8;
  USHORT MinimumMWXferCycleTime;
  USHORT RecommendedMWXferCycleTime;
  USHORT MinimumPIOCycleTime;
  USHORT MinimumPIOCycleTimeIORDY;
  USHORT ReservedWords69[2];
  USHORT BusReleaseDelay;
  USHORT ServiceCommandDelay;
  USHORT ReservedWords73[2];
  USHORT QueueDepth  :5;
  USHORT ReservedWord75  :11;
  struct {
    USHORT Reserved0  :1;
    USHORT SataGen1  :1;
    USHORT SataGen2  :1;
    USHORT SataGen3  :1;
    USHORT Reserved1  :5;
    USHORT HIPM  :1;
    USHORT PhyEvents  :1;
    USHORT Reserved3  :2;
    USHORT HostAutoPS  :1;
    USHORT DeviceAutoPS  :1;
    USHORT Reserved4  :1;
    USHORT Reserved5  :1;
    USHORT CurrentSpeed  :3;
    USHORT SlimlineDeviceAttention  :1;
    USHORT HostEnvironmentDetect  :1;
    USHORT Reserved  :10;
  } SerialAtaCapabilities;
  struct {
    USHORT Reserved0  :1;
    USHORT Reserved1  :2;
    USHORT DIPM  :1;
    USHORT Reserved2  :1;
    USHORT AsynchronousNotification  :1;
    USHORT SoftwareSettingsPreservation  :1;
    USHORT Reserved3  :9;
  } SerialAtaFeaturesSupported;
  struct {
    USHORT Reserved0  :1;
    USHORT Reserved1  :2;
    USHORT DIPM  :1;
    USHORT Reserved2  :1;
    USHORT AsynchronousNotification  :1;
    USHORT SoftwareSettingsPreservation  :1;
    USHORT DeviceAutoPS  :1;
    USHORT Reserved3  :8;
  } SerialAtaFeaturesEnabled;
  USHORT MajorRevision;
  USHORT MinorRevision;
  struct {
    USHORT SmartCommands  :1;
    USHORT SecurityMode  :1;
    USHORT RemovableMedia  :1;
    USHORT PowerManagement  :1;
    USHORT PacketCommands  :1;
    USHORT WriteCache  :1;
    USHORT LookAhead  :1;
    USHORT ReleaseInterrupt  :1;
    USHORT ServiceInterrupt  :1;
    USHORT DeviceReset  :1;
    USHORT HostProtectedArea  :1;
    USHORT Obsolete1  :1;
    USHORT WriteBuffer  :1;
    USHORT ReadBuffer  :1;
    USHORT Nop  :1;
    USHORT Obsolete2  :1;
    USHORT DownloadMicrocode  :1;
    USHORT Reserved1  :2;
    USHORT AdvancedPm  :1;
    USHORT Msn  :1;
    USHORT PowerUpInStandby  :1;
    USHORT ManualPowerUp  :1;
    USHORT Reserved2  :1;
    USHORT SetMax  :1;
    USHORT Reserved3  :3;
    USHORT FlushCache  :1;
    USHORT Reserved4  :1;
    USHORT WordValid  :2;
  } CommandSetSupport;
  struct {
    USHORT Reserved0  :5;
    USHORT GpLogging  :1;
    USHORT Reserved1  :2;
    USHORT WWN64Bit  :1;
    USHORT Reserved2  :5;
    USHORT WordValid  :2;
  } CommandSetSupportExt;
  struct {
    USHORT SmartCommands  :1;
    USHORT SecurityMode  :1;
    USHORT RemovableMedia  :1;
    USHORT PowerManagement  :1;
    USHORT PacketCommands  :1;
    USHORT WriteCache  :1;
    USHORT LookAhead  :1;
    USHORT ReleaseInterrupt  :1;
    USHORT ServiceInterrupt  :1;
    USHORT DeviceReset  :1;
    USHORT HostProtectedArea  :1;
    USHORT Obsolete1  :1;
    USHORT WriteBuffer  :1;
    USHORT ReadBuffer  :1;
    USHORT Nop  :1;
    USHORT Obsolete2  :1;
    USHORT DownloadMicrocode  :1;
    USHORT Reserved1  :2;
    USHORT AdvancedPm  :1;
    USHORT Msn  :1;
    USHORT PowerUpInStandby  :1;
    USHORT ManualPowerUp  :1;
    USHORT Reserved2  :1;
    USHORT SetMax  :1;
    USHORT Reserved3  :3;
    USHORT FlushCache  :1;
    USHORT Reserved  :3;
  } CommandSetActive;
  struct {
    USHORT Reserved0  :5;
    USHORT GpLogging  :1;
    USHORT Reserved1  :2;
    USHORT WWN64Bit  :1;
    USHORT Reserved2  :5;
    USHORT WordValid  :2;
  } CommandSetActiveExt;
  USHORT UltraDMASupport  :8;
  USHORT UltraDMAActive  :8;
  USHORT TimeRequiredForNormalEraseModeSecurityEraseUnit;
  USHORT TimeRequiredForEnhancedEraseModeSecurityEraseUnit;
  USHORT CurrentAPMLevel;
  USHORT MasterPasswordID;
  USHORT HardwareResetResult;
  USHORT ReservedWords94[14];
  USHORT WorldWideName[4];
  USHORT ReservedWords112[13];
  USHORT AtapiZeroByteCount;
  USHORT ReservedWord126;
  USHORT MsnSupport  :2;
  USHORT ReservedWord127  :14;
  USHORT SecurityStatus;
  USHORT VendorSpecific[31];
  USHORT ReservedWord160[16];
  USHORT ReservedWord176[46];
  struct {
    USHORT MajorVersion  :12;
    USHORT TransportType  :4;
  } TransportMajorVersion;
  USHORT TransportMinorVersion;
  USHORT ReservedWord224[31];
  USHORT Signature  :8;
  USHORT CheckSum  :8;
} IDENTIFY_PACKET_DATA, *PIDENTIFY_PACKET_DATA;

typedef struct _GP_LOG_NCQ_COMMAND_ERROR {
  UCHAR NcqTag : 5;
  UCHAR Reserved0 : 1;
  UCHAR UNL : 1;
  UCHAR NonQueuedCmd : 1;
  UCHAR Reserved1;
  UCHAR Status;
  UCHAR Error;
  UCHAR LBA7_0;
  UCHAR LBA15_8;
  UCHAR LBA23_16;
  UCHAR Device;
  UCHAR LBA31_24;
  UCHAR LBA39_32;
  UCHAR LBA47_40;
  UCHAR Reserved2;
  UCHAR Count7_0;
  UCHAR Count15_8;
  UCHAR SenseKey;
  UCHAR ASC;
  UCHAR ASCQ;
  UCHAR Reserved3[239];
  UCHAR Vendor[255];
  UCHAR Checksum;
} GP_LOG_NCQ_COMMAND_ERROR, *PGP_LOG_NCQ_COMMAND_ERROR;
#include <poppack.h>

#define IDE_LBA_MODE                          (1 << 6)

#define IDE_DC_DISABLE_INTERRUPTS    0x02
#define IDE_DC_RESET_CONTROLLER      0x04
#define IDE_DC_REENABLE_CONTROLLER   0x00

#define IDE_STATUS_ERROR             0x01
#define IDE_STATUS_INDEX             0x02
#define IDE_STATUS_CORRECTED_ERROR   0x04
#define IDE_STATUS_DRQ               0x08
#define IDE_STATUS_DSC               0x10
#define IDE_STATUS_DEVICE_FAULT      0x20
#define IDE_STATUS_DRDY              0x40
#define IDE_STATUS_IDLE              0x50
#define IDE_STATUS_BUSY              0x80

#define IDE_ERROR_BAD_BLOCK          0x80
#define IDE_ERROR_CRC_ERROR          IDE_ERROR_BAD_BLOCK
#define IDE_ERROR_DATA_ERROR         0x40
#define IDE_ERROR_MEDIA_CHANGE       0x20
#define IDE_ERROR_ID_NOT_FOUND       0x10
#define IDE_ERROR_MEDIA_CHANGE_REQ   0x08
#define IDE_ERROR_COMMAND_ABORTED    0x04
#define IDE_ERROR_END_OF_MEDIA       0x02
#define IDE_ERROR_ILLEGAL_LENGTH     0x01
#define IDE_ERROR_ADDRESS_NOT_FOUND  IDE_ERROR_ILLEGAL_LENGTH

#define IDE_COMMAND_NOP                         0x00
#define IDE_COMMAND_DATA_SET_MANAGEMENT         0x06
#define IDE_COMMAND_ATAPI_RESET                 0x08
#define IDE_COMMAND_READ                        0x20
#define IDE_COMMAND_READ_EXT                    0x24
#define IDE_COMMAND_READ_DMA_EXT                0x25
#define IDE_COMMAND_READ_DMA_QUEUED_EXT         0x26
#define IDE_COMMAND_READ_MULTIPLE_EXT           0x29
#define IDE_COMMAND_READ_LOG_EXT                0x2F
#define IDE_COMMAND_WRITE                       0x30
#define IDE_COMMAND_WRITE_EXT                   0x34
#define IDE_COMMAND_WRITE_DMA_EXT               0x35
#define IDE_COMMAND_WRITE_DMA_QUEUED_EXT        0x36
#define IDE_COMMAND_WRITE_MULTIPLE_EXT          0x39
#define IDE_COMMAND_WRITE_DMA_FUA_EXT           0x3D
#define IDE_COMMAND_WRITE_DMA_QUEUED_FUA_EXT    0x3E
#define IDE_COMMAND_VERIFY                      0x40
#define IDE_COMMAND_VERIFY_EXT                  0x42
#define IDE_COMMAND_READ_FPDMA_QUEUED           0x60
#define IDE_COMMAND_WRITE_FPDMA_QUEUED          0x61
#define IDE_COMMAND_EXECUTE_DEVICE_DIAGNOSTIC   0x90
#define IDE_COMMAND_SET_DRIVE_PARAMETERS        0x91
#define IDE_COMMAND_ATAPI_PACKET                0xA0
#define IDE_COMMAND_ATAPI_IDENTIFY              0xA1
#define IDE_COMMAND_SMART                       0xB0
#define IDE_COMMAND_READ_MULTIPLE               0xC4
#define IDE_COMMAND_WRITE_MULTIPLE              0xC5
#define IDE_COMMAND_SET_MULTIPLE                0xC6
#define IDE_COMMAND_READ_DMA                    0xC8
#define IDE_COMMAND_WRITE_DMA                   0xCA
#define IDE_COMMAND_WRITE_DMA_QUEUED            0xCC
#define IDE_COMMAND_WRITE_MULTIPLE_FUA_EXT      0xCE
#define IDE_COMMAND_GET_MEDIA_STATUS            0xDA
#define IDE_COMMAND_DOOR_LOCK                   0xDE
#define IDE_COMMAND_DOOR_UNLOCK                 0xDF
#define IDE_COMMAND_STANDBY_IMMEDIATE           0xE0
#define IDE_COMMAND_IDLE_IMMEDIATE              0xE1
#define IDE_COMMAND_CHECK_POWER                 0xE5
#define IDE_COMMAND_SLEEP                       0xE6
#define IDE_COMMAND_FLUSH_CACHE                 0xE7
#define IDE_COMMAND_FLUSH_CACHE_EXT             0xEA
#define IDE_COMMAND_IDENTIFY                    0xEC
#define IDE_COMMAND_MEDIA_EJECT                 0xED
#define IDE_COMMAND_SET_FEATURE                 0xEF
#define IDE_COMMAND_SECURITY_FREEZE_LOCK        0xF5
#define IDE_COMMAND_NOT_VALID                   0xFF

#define IDE_FEATURE_ENABLE_WRITE_CACHE          0x2
#define IDE_FEATURE_SET_TRANSFER_MODE           0x3
#define IDE_FEATURE_ENABLE_PUIS                 0x6
#define IDE_FEATURE_PUIS_SPIN_UP                0x7
#define IDE_FEATURE_ENABLE_SATA_FEATURE         0x10
#define IDE_FEATURE_DISABLE_MSN                 0x31
#define IDE_FEATURE_DISABLE_REVERT_TO_POWER_ON  0x66
#define IDE_FEATURE_DISABLE_WRITE_CACHE         0x82
#define IDE_FEATURE_DISABLE_PUIS                0x86
#define IDE_FEATURE_DISABLE_SATA_FEATURE        0x90
#define IDE_FEATURE_ENABLE_MSN                  0x95

#define IDE_GP_LOG_NCQ_COMMAND_ERROR_ADDRESS        0x10

#define IDE_GP_LOG_SECTOR_SIZE                      0x200

#endif
