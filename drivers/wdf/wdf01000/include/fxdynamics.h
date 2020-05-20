/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Wdf01000 driver functions table
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */


#ifndef _FXDYNAMICS_H_
#define _FXDYNAMICS_H_

#include <ntddk.h>
#include "wdf.h"
#include "common/fxmacros.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef
WDFAPI
NTSTATUS
(*PFN_WDFUNIMPLEMENTED)();

typedef struct _WDFFUNCTIONS {
    PFN_WDFCHILDLISTCREATE   pfnWdfChildListCreate;
	PFN_WDFCHILDLISTGETDEVICE   pfnWdfChildListGetDevice;
	PFN_WDFCHILDLISTRETRIEVEPDO   pfnWdfChildListRetrievePdo;
	PFN_WDFCHILDLISTRETRIEVEADDRESSDESCRIPTION   pfnWdfChildListRetrieveAddressDescription;
	PFN_WDFCHILDLISTBEGINSCAN   pfnWdfChildListBeginScan;
	PFN_WDFCHILDLISTENDSCAN   pfnWdfChildListEndScan;
	PFN_WDFCHILDLISTBEGINITERATION   pfnWdfChildListBeginIteration;
	PFN_WDFCHILDLISTRETRIEVENEXTDEVICE   pfnWdfChildListRetrieveNextDevice;
	PFN_WDFCHILDLISTENDITERATION   pfnWdfChildListEndIteration;
	PFN_WDFCHILDLISTADDORUPDATECHILDDESCRIPTIONASPRESENT   pfnWdfChildListAddOrUpdateChildDescriptionAsPresent;
	PFN_WDFCHILDLISTUPDATECHILDDESCRIPTIONASMISSING   pfnWdfChildListUpdateChildDescriptionAsMissing;
	PFN_WDFCHILDLISTUPDATEALLCHILDDESCRIPTIONSASPRESENT   pfnWdfChildListUpdateAllChildDescriptionsAsPresent;
	PFN_WDFCHILDLISTREQUESTCHILDEJECT   pfnWdfChildListRequestChildEject;
	PFN_WDFCOLLECTIONCREATE   pfnWdfCollectionCreate;
	PFN_WDFCOLLECTIONGETCOUNT   pfnWdfCollectionGetCount;
	PFN_WDFCOLLECTIONADD   pfnWdfCollectionAdd;
	PFN_WDFCOLLECTIONREMOVE   pfnWdfCollectionRemove;
	PFN_WDFCOLLECTIONREMOVEITEM   pfnWdfCollectionRemoveItem;
	PFN_WDFCOLLECTIONGETITEM   pfnWdfCollectionGetItem;
	PFN_WDFCOLLECTIONGETFIRSTITEM   pfnWdfCollectionGetFirstItem;
	PFN_WDFCOLLECTIONGETLASTITEM   pfnWdfCollectionGetLastItem;
	PFN_WDFCOMMONBUFFERCREATE   pfnWdfCommonBufferCreate;
	PFN_WDFCOMMONBUFFERGETALIGNEDVIRTUALADDRESS   pfnWdfCommonBufferGetAlignedVirtualAddress;
	PFN_WDFCOMMONBUFFERGETALIGNEDLOGICALADDRESS   pfnWdfCommonBufferGetAlignedLogicalAddress;
	PFN_WDFCOMMONBUFFERGETLENGTH   pfnWdfCommonBufferGetLength;
	PFN_WDFCONTROLDEVICEINITALLOCATE   pfnWdfControlDeviceInitAllocate;
	PFN_WDFCONTROLDEVICEINITSETSHUTDOWNNOTIFICATION   pfnWdfControlDeviceInitSetShutdownNotification;
	PFN_WDFCONTROLFINISHINITIALIZING   pfnWdfControlFinishInitializing;
	PFN_WDFDEVICEGETDEVICESTATE   pfnWdfDeviceGetDeviceState;
	PFN_WDFDEVICESETDEVICESTATE   pfnWdfDeviceSetDeviceState;
	PFN_WDFWDMDEVICEGETWDFDEVICEHANDLE   pfnWdfWdmDeviceGetWdfDeviceHandle;
	PFN_WDFDEVICEWDMGETDEVICEOBJECT   pfnWdfDeviceWdmGetDeviceObject;
	PFN_WDFDEVICEWDMGETATTACHEDDEVICE   pfnWdfDeviceWdmGetAttachedDevice;
	PFN_WDFDEVICEWDMGETPHYSICALDEVICE   pfnWdfDeviceWdmGetPhysicalDevice;
	PFN_WDFDEVICEWDMDISPATCHPREPROCESSEDIRP   pfnWdfDeviceWdmDispatchPreprocessedIrp;
	PFN_WDFDEVICEADDDEPENDENTUSAGEDEVICEOBJECT   pfnWdfDeviceAddDependentUsageDeviceObject;
	PFN_WDFDEVICEADDREMOVALRELATIONSPHYSICALDEVICE   pfnWdfDeviceAddRemovalRelationsPhysicalDevice;
	PFN_WDFDEVICEREMOVEREMOVALRELATIONSPHYSICALDEVICE   pfnWdfDeviceRemoveRemovalRelationsPhysicalDevice;
	PFN_WDFDEVICECLEARREMOVALRELATIONSDEVICES   pfnWdfDeviceClearRemovalRelationsDevices;
	PFN_WDFDEVICEGETDRIVER   pfnWdfDeviceGetDriver;
	PFN_WDFDEVICERETRIEVEDEVICENAME   pfnWdfDeviceRetrieveDeviceName;
	PFN_WDFDEVICEASSIGNMOFRESOURCENAME   pfnWdfDeviceAssignMofResourceName;
	PFN_WDFDEVICEGETIOTARGET   pfnWdfDeviceGetIoTarget;
	PFN_WDFDEVICEGETDEVICEPNPSTATE   pfnWdfDeviceGetDevicePnpState;
	PFN_WDFDEVICEGETDEVICEPOWERSTATE   pfnWdfDeviceGetDevicePowerState;
	PFN_WDFDEVICEGETDEVICEPOWERPOLICYSTATE   pfnWdfDeviceGetDevicePowerPolicyState;
	PFN_WDFDEVICEASSIGNS0IDLESETTINGS   pfnWdfDeviceAssignS0IdleSettings;
	PFN_WDFDEVICEASSIGNSXWAKESETTINGS   pfnWdfDeviceAssignSxWakeSettings;
	PFN_WDFDEVICEOPENREGISTRYKEY   pfnWdfDeviceOpenRegistryKey;
	PFN_WDFDEVICESETSPECIALFILESUPPORT   pfnWdfDeviceSetSpecialFileSupport;
	PFN_WDFDEVICESETCHARACTERISTICS   pfnWdfDeviceSetCharacteristics;
	PFN_WDFDEVICEGETCHARACTERISTICS   pfnWdfDeviceGetCharacteristics;
	PFN_WDFDEVICEGETALIGNMENTREQUIREMENT   pfnWdfDeviceGetAlignmentRequirement;
	PFN_WDFDEVICESETALIGNMENTREQUIREMENT   pfnWdfDeviceSetAlignmentRequirement;
	PFN_WDFDEVICEINITFREE   pfnWdfDeviceInitFree;
	PFN_WDFDEVICEINITSETPNPPOWEREVENTCALLBACKS   pfnWdfDeviceInitSetPnpPowerEventCallbacks;
	PFN_WDFDEVICEINITSETPOWERPOLICYEVENTCALLBACKS             pfnWdfDeviceInitSetPowerPolicyEventCallbacks;
    PFN_WDFDEVICEINITSETPOWERPOLICYOWNERSHIP                  pfnWdfDeviceInitSetPowerPolicyOwnership;
    PFN_WDFDEVICEINITREGISTERPNPSTATECHANGECALLBACK           pfnWdfDeviceInitRegisterPnpStateChangeCallback;
    PFN_WDFDEVICEINITREGISTERPOWERSTATECHANGECALLBACK         pfnWdfDeviceInitRegisterPowerStateChangeCallback;
    PFN_WDFDEVICEINITREGISTERPOWERPOLICYSTATECHANGECALLBACK   pfnWdfDeviceInitRegisterPowerPolicyStateChangeCallback;
    PFN_WDFDEVICEINITSETIOTYPE                                pfnWdfDeviceInitSetIoType;
    PFN_WDFDEVICEINITSETEXCLUSIVE                             pfnWdfDeviceInitSetExclusive;
    PFN_WDFDEVICEINITSETPOWERNOTPAGEABLE                      pfnWdfDeviceInitSetPowerNotPageable;
    PFN_WDFDEVICEINITSETPOWERPAGEABLE                         pfnWdfDeviceInitSetPowerPageable;
    PFN_WDFDEVICEINITSETPOWERINRUSH                           pfnWdfDeviceInitSetPowerInrush;
    PFN_WDFDEVICEINITSETDEVICETYPE                            pfnWdfDeviceInitSetDeviceType;
    PFN_WDFDEVICEINITASSIGNNAME                               pfnWdfDeviceInitAssignName;
    PFN_WDFDEVICEINITASSIGNSDDLSTRING                         pfnWdfDeviceInitAssignSDDLString;
    PFN_WDFDEVICEINITSETDEVICECLASS                           pfnWdfDeviceInitSetDeviceClass;
    PFN_WDFDEVICEINITSETCHARACTERISTICS                       pfnWdfDeviceInitSetCharacteristics;
    PFN_WDFDEVICEINITSETFILEOBJECTCONFIG                      pfnWdfDeviceInitSetFileObjectConfig;
	PFN_WDFDEVICEINITSETREQUESTATTRIBUTES   pfnWdfDeviceInitSetRequestAttributes;
	PFN_WDFDEVICEINITASSIGNWDMIRPPREPROCESSCALLBACK           pfnWdfDeviceInitAssignWdmIrpPreprocessCallback;
    PFN_WDFDEVICEINITSETIOINCALLERCONTEXTCALLBACK             pfnWdfDeviceInitSetIoInCallerContextCallback;
	PFN_WDFDEVICECREATE   pfnWdfDeviceCreate;
	PFN_WDFDEVICESETSTATICSTOPREMOVE                          pfnWdfDeviceSetStaticStopRemove;
	PFN_WDFDEVICECREATEDEVICEINTERFACE   pfnWdfDeviceCreateDeviceInterface;
	PFN_WDFDEVICESETDEVICEINTERFACESTATE                      pfnWdfDeviceSetDeviceInterfaceState;
    PFN_WDFDEVICERETRIEVEDEVICEINTERFACESTRING                pfnWdfDeviceRetrieveDeviceInterfaceString;
    PFN_WDFDEVICECREATESYMBOLICLINK                           pfnWdfDeviceCreateSymbolicLink;
    PFN_WDFDEVICEQUERYPROPERTY                                pfnWdfDeviceQueryProperty;
    PFN_WDFDEVICEALLOCANDQUERYPROPERTY                        pfnWdfDeviceAllocAndQueryProperty;
    PFN_WDFDEVICESETPNPCAPABILITIES                           pfnWdfDeviceSetPnpCapabilities;
    PFN_WDFDEVICESETPOWERCAPABILITIES                         pfnWdfDeviceSetPowerCapabilities;
    PFN_WDFDEVICESETBUSINFORMATIONFORCHILDREN                 pfnWdfDeviceSetBusInformationForChildren;
    PFN_WDFDEVICEINDICATEWAKESTATUS                           pfnWdfDeviceIndicateWakeStatus;
    PFN_WDFDEVICESETFAILED                                    pfnWdfDeviceSetFailed;
	PFN_WDFDEVICESTOPIDLE				      				  pfnWdfDeviceStopIdle;
    PFN_WDFDEVICERESUMEIDLE				                      pfnWdfDeviceResumeIdle;
	PFN_WDFDEVICEGETFILEOBJECT                                pfnWdfDeviceGetFileObject;
    PFN_WDFDEVICEENQUEUEREQUEST                               pfnWdfDeviceEnqueueRequest;
	PFN_WDFDEVICEGETDEFAULTQUEUE   pfnWdfDeviceGetDefaultQueue;
	PFN_WDFDEVICECONFIGUREREQUESTDISPATCHING                  pfnWdfDeviceConfigureRequestDispatching;
	PFN_WDFDMAENABLERCREATE                                   pfnWdfDmaEnablerCreate;
	PFN_WDFDMAENABLERGETMAXIMUMLENGTH                         pfnWdfDmaEnablerGetMaximumLength;
    PFN_WDFDMAENABLERGETMAXIMUMSCATTERGATHERELEMENTS          pfnWdfDmaEnablerGetMaximumScatterGatherElements;
    PFN_WDFDMAENABLERSETMAXIMUMSCATTERGATHERELEMENTS          pfnWdfDmaEnablerSetMaximumScatterGatherElements;
    PFN_WDFDMATRANSACTIONCREATE                               pfnWdfDmaTransactionCreate;
    PFN_WDFDMATRANSACTIONINITIALIZE                           pfnWdfDmaTransactionInitialize;
    PFN_WDFDMATRANSACTIONINITIALIZEUSINGREQUEST               pfnWdfDmaTransactionInitializeUsingRequest;
    PFN_WDFDMATRANSACTIONEXECUTE                              pfnWdfDmaTransactionExecute;
    PFN_WDFDMATRANSACTIONRELEASE                              pfnWdfDmaTransactionRelease;
    PFN_WDFDMATRANSACTIONDMACOMPLETED                         pfnWdfDmaTransactionDmaCompleted;
    PFN_WDFDMATRANSACTIONDMACOMPLETEDWITHLENGTH               pfnWdfDmaTransactionDmaCompletedWithLength;
    PFN_WDFDMATRANSACTIONDMACOMPLETEDFINAL                    pfnWdfDmaTransactionDmaCompletedFinal;
    PFN_WDFDMATRANSACTIONGETBYTESTRANSFERRED                  pfnWdfDmaTransactionGetBytesTransferred;
    PFN_WDFDMATRANSACTIONSETMAXIMUMLENGTH                     pfnWdfDmaTransactionSetMaximumLength;
    PFN_WDFDMATRANSACTIONGETREQUEST                           pfnWdfDmaTransactionGetRequest;
    PFN_WDFDMATRANSACTIONGETCURRENTDMATRANSFERLENGTH          pfnWdfDmaTransactionGetCurrentDmaTransferLength;
    PFN_WDFDMATRANSACTIONGETDEVICE                            pfnWdfDmaTransactionGetDevice;
	PFN_WDFDPCCREATE   pfnWdfDpcCreate;
	PFN_WDFDPCENQUEUE   pfnWdfDpcEnqueue;
	PFN_WDFDPCCANCEL   pfnWdfDpcCancel;
	PFN_WDFDPCGETPARENTOBJECT   pfnWdfDpcGetParentObject;
	PFN_WDFDPCWDMGETDPC   pfnWdfDpcWdmGetDpc;
	PFN_WDFDRIVERCREATE    pfnWdfDriverCreate;
	PFN_WDFDRIVERGETREGISTRYPATH                              pfnWdfDriverGetRegistryPath;
    PFN_WDFDRIVERWDMGETDRIVEROBJECT                           pfnWdfDriverWdmGetDriverObject;
    PFN_WDFDRIVEROPENPARAMETERSREGISTRYKEY                    pfnWdfDriverOpenParametersRegistryKey;
    PFN_WDFWDMDRIVERGETWDFDRIVERHANDLE                        pfnWdfWdmDriverGetWdfDriverHandle;
    PFN_WDFDRIVERREGISTERTRACEINFO                            pfnWdfDriverRegisterTraceInfo;
	PFN_WDFDRIVERRETRIEVEVERSIONSTRING   pfnWdfDriverRetrieveVersionString;
	PFN_WDFDRIVERISVERSIONAVAILABLE   pfnWdfDriverIsVersionAvailable;
	PFN_WDFFDOINITWDMGETPHYSICALDEVICE                        pfnWdfFdoInitWdmGetPhysicalDevice;
    PFN_WDFFDOINITOPENREGISTRYKEY                             pfnWdfFdoInitOpenRegistryKey;
    PFN_WDFFDOINITQUERYPROPERTY                               pfnWdfFdoInitQueryProperty;
    PFN_WDFFDOINITALLOCANDQUERYPROPERTY                       pfnWdfFdoInitAllocAndQueryProperty;
    PFN_WDFFDOINITSETEVENTCALLBACKS                           pfnWdfFdoInitSetEventCallbacks;
    PFN_WDFFDOINITSETFILTER                                   pfnWdfFdoInitSetFilter;
    PFN_WDFFDOINITSETDEFAULTCHILDLISTCONFIG                   pfnWdfFdoInitSetDefaultChildListConfig;
    PFN_WDFFDOQUERYFORINTERFACE                               pfnWdfFdoQueryForInterface;
    PFN_WDFFDOGETDEFAULTCHILDLIST                             pfnWdfFdoGetDefaultChildList;
    PFN_WDFFDOADDSTATICCHILD                                  pfnWdfFdoAddStaticChild;
    PFN_WDFFDOLOCKSTATICCHILDLISTFORITERATION                 pfnWdfFdoLockStaticChildListForIteration;
    PFN_WDFFDORETRIEVENEXTSTATICCHILD                         pfnWdfFdoRetrieveNextStaticChild;
    PFN_WDFFDOUNLOCKSTATICCHILDLISTFROMITERATION              pfnWdfFdoUnlockStaticChildListFromIteration;
    PFN_WDFFILEOBJECTGETFILENAME                              pfnWdfFileObjectGetFileName;
    PFN_WDFFILEOBJECTGETFLAGS                                 pfnWdfFileObjectGetFlags;
    PFN_WDFFILEOBJECTGETDEVICE                                pfnWdfFileObjectGetDevice;
    PFN_WDFFILEOBJECTWDMGETFILEOBJECT                         pfnWdfFileObjectWdmGetFileObject;
    PFN_WDFINTERRUPTCREATE                                    pfnWdfInterruptCreate;
    PFN_WDFINTERRUPTQUEUEDPCFORISR                            pfnWdfInterruptQueueDpcForIsr;
    PFN_WDFINTERRUPTSYNCHRONIZE                               pfnWdfInterruptSynchronize;
    PFN_WDFINTERRUPTACQUIRELOCK                               pfnWdfInterruptAcquireLock;
    PFN_WDFINTERRUPTRELEASELOCK                               pfnWdfInterruptReleaseLock;
    PFN_WDFINTERRUPTENABLE                                    pfnWdfInterruptEnable;
    PFN_WDFINTERRUPTDISABLE                                   pfnWdfInterruptDisable;
    PFN_WDFINTERRUPTWDMGETINTERRUPT                           pfnWdfInterruptWdmGetInterrupt;
    PFN_WDFINTERRUPTGETINFO                                   pfnWdfInterruptGetInfo;
    PFN_WDFINTERRUPTSETPOLICY                                 pfnWdfInterruptSetPolicy;
    PFN_WDFINTERRUPTGETDEVICE                                 pfnWdfInterruptGetDevice;
	PFN_WDFIOQUEUECREATE   pfnWdfIoQueueCreate;
	PFN_WDFIOQUEUEGETSTATE                                    pfnWdfIoQueueGetState;
	PFN_WDFIOQUEUESTART   pfnWdfIoQueueStart;
	PFN_WDFIOQUEUESTOP                                        pfnWdfIoQueueStop;
	PFN_WDFIOQUEUESTOPSYNCHRONOUSLY   pfnWdfIoQueueStopSynchronously;
	PFN_WDFIOQUEUEGETDEVICE                                   pfnWdfIoQueueGetDevice;
    PFN_WDFIOQUEUERETRIEVENEXTREQUEST                         pfnWdfIoQueueRetrieveNextRequest;
    PFN_WDFIOQUEUERETRIEVEREQUESTBYFILEOBJECT                 pfnWdfIoQueueRetrieveRequestByFileObject;
    PFN_WDFIOQUEUEFINDREQUEST                                 pfnWdfIoQueueFindRequest;
    PFN_WDFIOQUEUERETRIEVEFOUNDREQUEST                        pfnWdfIoQueueRetrieveFoundRequest;
    PFN_WDFIOQUEUEDRAINSYNCHRONOUSLY                          pfnWdfIoQueueDrainSynchronously;
    PFN_WDFIOQUEUEDRAIN                                       pfnWdfIoQueueDrain;
    PFN_WDFIOQUEUEPURGESYNCHRONOUSLY                          pfnWdfIoQueuePurgeSynchronously;
    PFN_WDFIOQUEUEPURGE                                       pfnWdfIoQueuePurge;
    PFN_WDFIOQUEUEREADYNOTIFY                                 pfnWdfIoQueueReadyNotify;
    PFN_WDFIOTARGETCREATE                                     pfnWdfIoTargetCreate;
    PFN_WDFIOTARGETOPEN                                       pfnWdfIoTargetOpen;
    PFN_WDFIOTARGETCLOSEFORQUERYREMOVE                        pfnWdfIoTargetCloseForQueryRemove;
    PFN_WDFIOTARGETCLOSE                                      pfnWdfIoTargetClose;
    PFN_WDFIOTARGETSTART                                      pfnWdfIoTargetStart;
    PFN_WDFIOTARGETSTOP                                       pfnWdfIoTargetStop;
    PFN_WDFIOTARGETGETSTATE                                   pfnWdfIoTargetGetState;
    PFN_WDFIOTARGETGETDEVICE                                  pfnWdfIoTargetGetDevice;
    PFN_WDFIOTARGETQUERYTARGETPROPERTY                        pfnWdfIoTargetQueryTargetProperty;
    PFN_WDFIOTARGETALLOCANDQUERYTARGETPROPERTY                pfnWdfIoTargetAllocAndQueryTargetProperty;
    PFN_WDFIOTARGETQUERYFORINTERFACE                          pfnWdfIoTargetQueryForInterface;
    PFN_WDFIOTARGETWDMGETTARGETDEVICEOBJECT                   pfnWdfIoTargetWdmGetTargetDeviceObject;
    PFN_WDFIOTARGETWDMGETTARGETPHYSICALDEVICE                 pfnWdfIoTargetWdmGetTargetPhysicalDevice;
    PFN_WDFIOTARGETWDMGETTARGETFILEOBJECT                     pfnWdfIoTargetWdmGetTargetFileObject;
    PFN_WDFIOTARGETWDMGETTARGETFILEHANDLE                     pfnWdfIoTargetWdmGetTargetFileHandle;
    PFN_WDFIOTARGETSENDREADSYNCHRONOUSLY                      pfnWdfIoTargetSendReadSynchronously;
    PFN_WDFIOTARGETFORMATREQUESTFORREAD                       pfnWdfIoTargetFormatRequestForRead;
    PFN_WDFIOTARGETSENDWRITESYNCHRONOUSLY                     pfnWdfIoTargetSendWriteSynchronously;
    PFN_WDFIOTARGETFORMATREQUESTFORWRITE                      pfnWdfIoTargetFormatRequestForWrite;
    PFN_WDFIOTARGETSENDIOCTLSYNCHRONOUSLY                     pfnWdfIoTargetSendIoctlSynchronously;
    PFN_WDFIOTARGETFORMATREQUESTFORIOCTL                      pfnWdfIoTargetFormatRequestForIoctl;
    PFN_WDFIOTARGETSENDINTERNALIOCTLSYNCHRONOUSLY             pfnWdfIoTargetSendInternalIoctlSynchronously;
    PFN_WDFIOTARGETFORMATREQUESTFORINTERNALIOCTL              pfnWdfIoTargetFormatRequestForInternalIoctl;
    PFN_WDFIOTARGETSENDINTERNALIOCTLOTHERSSYNCHRONOUSLY       pfnWdfIoTargetSendInternalIoctlOthersSynchronously;
    PFN_WDFIOTARGETFORMATREQUESTFORINTERNALIOCTLOTHERS        pfnWdfIoTargetFormatRequestForInternalIoctlOthers;
    PFN_WDFMEMORYCREATE                                       pfnWdfMemoryCreate;
    PFN_WDFMEMORYCREATEPREALLOCATED                           pfnWdfMemoryCreatePreallocated;
    PFN_WDFMEMORYGETBUFFER                                    pfnWdfMemoryGetBuffer;
    PFN_WDFMEMORYASSIGNBUFFER                                 pfnWdfMemoryAssignBuffer;
	PFN_WDFMEMORYCOPYTOBUFFER   pfnWdfMemoryCopyToBuffer;
	PFN_WDFMEMORYCOPYFROMBUFFER   pfnWdfMemoryCopyFromBuffer;
	PFN_WDFLOOKASIDELISTCREATE                                pfnWdfLookasideListCreate;
    PFN_WDFMEMORYCREATEFROMLOOKASIDE                          pfnWdfMemoryCreateFromLookaside;
    PFN_WDFDEVICEMINIPORTCREATE                               pfnWdfDeviceMiniportCreate;
    PFN_WDFDRIVERMINIPORTUNLOAD                               pfnWdfDriverMiniportUnload;
	PFN_WDFOBJECTGETTYPEDCONTEXTWORKER   pfnWdfObjectGetTypedContextWorker;
	PFN_WDFOBJECTALLOCATECONTEXT                              pfnWdfObjectAllocateContext;
    PFN_WDFOBJECTCONTEXTGETOBJECT                             pfnWdfObjectContextGetObject;
    PFN_WDFOBJECTREFERENCEACTUAL                              pfnWdfObjectReferenceActual;
    PFN_WDFOBJECTDEREFERENCEACTUAL                            pfnWdfObjectDereferenceActual;
    PFN_WDFOBJECTCREATE                                       pfnWdfObjectCreate;
	PFN_WDFOBJECTDELETE   pfnWdfObjectDelete;
	PFN_WDFOBJECTQUERY   pfnWdfObjectQuery;
	PFN_WDFPDOINITALLOCATE                                    pfnWdfPdoInitAllocate;
    PFN_WDFPDOINITSETEVENTCALLBACKS                           pfnWdfPdoInitSetEventCallbacks;
    PFN_WDFPDOINITASSIGNDEVICEID                              pfnWdfPdoInitAssignDeviceID;
    PFN_WDFPDOINITASSIGNINSTANCEID                            pfnWdfPdoInitAssignInstanceID;
    PFN_WDFPDOINITADDHARDWAREID                               pfnWdfPdoInitAddHardwareID;
    PFN_WDFPDOINITADDCOMPATIBLEID                             pfnWdfPdoInitAddCompatibleID;
    PFN_WDFPDOINITADDDEVICETEXT                               pfnWdfPdoInitAddDeviceText;
    PFN_WDFPDOINITSETDEFAULTLOCALE                            pfnWdfPdoInitSetDefaultLocale;
    PFN_WDFPDOINITASSIGNRAWDEVICE                             pfnWdfPdoInitAssignRawDevice;
    PFN_WDFPDOMARKMISSING                                     pfnWdfPdoMarkMissing;
    PFN_WDFPDOREQUESTEJECT                                    pfnWdfPdoRequestEject;
    PFN_WDFPDOGETPARENT                                       pfnWdfPdoGetParent;
    PFN_WDFPDORETRIEVEIDENTIFICATIONDESCRIPTION               pfnWdfPdoRetrieveIdentificationDescription;
    PFN_WDFPDORETRIEVEADDRESSDESCRIPTION                      pfnWdfPdoRetrieveAddressDescription;
    PFN_WDFPDOUPDATEADDRESSDESCRIPTION                        pfnWdfPdoUpdateAddressDescription;
    PFN_WDFPDOADDEJECTIONRELATIONSPHYSICALDEVICE              pfnWdfPdoAddEjectionRelationsPhysicalDevice;
    PFN_WDFPDOREMOVEEJECTIONRELATIONSPHYSICALDEVICE           pfnWdfPdoRemoveEjectionRelationsPhysicalDevice;
    PFN_WDFPDOCLEAREJECTIONRELATIONSDEVICES                   pfnWdfPdoClearEjectionRelationsDevices;
	PFN_WDFDEVICEADDQUERYINTERFACE   pfnWdfDeviceAddQueryInterface;
	PFN_WDFREGISTRYOPENKEY                                    pfnWdfRegistryOpenKey;
    PFN_WDFREGISTRYCREATEKEY                                  pfnWdfRegistryCreateKey;
    PFN_WDFREGISTRYCLOSE                                      pfnWdfRegistryClose;
    PFN_WDFREGISTRYWDMGETHANDLE                               pfnWdfRegistryWdmGetHandle;
    PFN_WDFREGISTRYREMOVEKEY                                  pfnWdfRegistryRemoveKey;
    PFN_WDFREGISTRYREMOVEVALUE                                pfnWdfRegistryRemoveValue;
    PFN_WDFREGISTRYQUERYVALUE                                 pfnWdfRegistryQueryValue;
    PFN_WDFREGISTRYQUERYMEMORY                                pfnWdfRegistryQueryMemory;
    PFN_WDFREGISTRYQUERYMULTISTRING                           pfnWdfRegistryQueryMultiString;
    PFN_WDFREGISTRYQUERYUNICODESTRING                         pfnWdfRegistryQueryUnicodeString;
    PFN_WDFREGISTRYQUERYSTRING                                pfnWdfRegistryQueryString;
    PFN_WDFREGISTRYQUERYULONG                                 pfnWdfRegistryQueryULong;
    PFN_WDFREGISTRYASSIGNVALUE                                pfnWdfRegistryAssignValue;
    PFN_WDFREGISTRYASSIGNMEMORY                               pfnWdfRegistryAssignMemory;
    PFN_WDFREGISTRYASSIGNMULTISTRING                          pfnWdfRegistryAssignMultiString;
    PFN_WDFREGISTRYASSIGNUNICODESTRING                        pfnWdfRegistryAssignUnicodeString;
    PFN_WDFREGISTRYASSIGNSTRING                               pfnWdfRegistryAssignString;
    PFN_WDFREGISTRYASSIGNULONG                                pfnWdfRegistryAssignULong;
	PFN_WDFREQUESTCREATE                                      pfnWdfRequestCreate;
    PFN_WDFREQUESTCREATEFROMIRP                               pfnWdfRequestCreateFromIrp;
    PFN_WDFREQUESTREUSE                                       pfnWdfRequestReuse;
    PFN_WDFREQUESTCHANGETARGET                                pfnWdfRequestChangeTarget;
    PFN_WDFREQUESTFORMATREQUESTUSINGCURRENTTYPE               pfnWdfRequestFormatRequestUsingCurrentType;
    PFN_WDFREQUESTWDMFORMATUSINGSTACKLOCATION                 pfnWdfRequestWdmFormatUsingStackLocation;
    PFN_WDFREQUESTSEND                                        pfnWdfRequestSend;
    PFN_WDFREQUESTGETSTATUS                                   pfnWdfRequestGetStatus;
	PFN_WDFREQUESTMARKCANCELABLE   pfnWdfRequestMarkCancelable;
	PFN_WDFREQUESTUNMARKCANCELABLE   pfnWdfRequestUnmarkCancelable;
	PFN_WDFREQUESTISCANCELED                                  pfnWdfRequestIsCanceled;
    PFN_WDFREQUESTCANCELSENTREQUEST                           pfnWdfRequestCancelSentRequest;
    PFN_WDFREQUESTISFROM32BITPROCESS                          pfnWdfRequestIsFrom32BitProcess;
    PFN_WDFREQUESTSETCOMPLETIONROUTINE                        pfnWdfRequestSetCompletionRoutine;
    PFN_WDFREQUESTGETCOMPLETIONPARAMS                         pfnWdfRequestGetCompletionParams;
    PFN_WDFREQUESTALLOCATETIMER                               pfnWdfRequestAllocateTimer;
	PFN_WDFREQUESTCOMPLETE   pfnWdfRequestComplete;
	PFN_WDFREQUESTCOMPLETEWITHPRIORITYBOOST                   pfnWdfRequestCompleteWithPriorityBoost;
	PFN_WDFREQUESTCOMPLETEWITHINFORMATION   pfnWdfRequestCompleteWithInformation;
	PFN_WDFREQUESTGETPARAMETERS   pfnWdfRequestGetParameters;
	PFN_WDFREQUESTRETRIEVEINPUTMEMORY   pfnWdfRequestRetrieveInputMemory;
	PFN_WDFREQUESTRETRIEVEOUTPUTMEMORY   pfnWdfRequestRetrieveOutputMemory;
	PFN_WDFREQUESTRETRIEVEINPUTBUFFER                         pfnWdfRequestRetrieveInputBuffer;
    PFN_WDFREQUESTRETRIEVEOUTPUTBUFFER                        pfnWdfRequestRetrieveOutputBuffer;
    PFN_WDFREQUESTRETRIEVEINPUTWDMMDL                         pfnWdfRequestRetrieveInputWdmMdl;
    PFN_WDFREQUESTRETRIEVEOUTPUTWDMMDL                        pfnWdfRequestRetrieveOutputWdmMdl;
    PFN_WDFREQUESTRETRIEVEUNSAFEUSERINPUTBUFFER               pfnWdfRequestRetrieveUnsafeUserInputBuffer;
    PFN_WDFREQUESTRETRIEVEUNSAFEUSEROUTPUTBUFFER              pfnWdfRequestRetrieveUnsafeUserOutputBuffer;
	PFN_WDFREQUESTSETINFORMATION   pfnWdfRequestSetInformation;
	PFN_WDFREQUESTGETINFORMATION                              pfnWdfRequestGetInformation;
    PFN_WDFREQUESTGETFILEOBJECT                               pfnWdfRequestGetFileObject;
    PFN_WDFREQUESTPROBEANDLOCKUSERBUFFERFORREAD               pfnWdfRequestProbeAndLockUserBufferForRead;
    PFN_WDFREQUESTPROBEANDLOCKUSERBUFFERFORWRITE              pfnWdfRequestProbeAndLockUserBufferForWrite;
    PFN_WDFREQUESTGETREQUESTORMODE                            pfnWdfRequestGetRequestorMode;
    PFN_WDFREQUESTFORWARDTOIOQUEUE                            pfnWdfRequestForwardToIoQueue;
	PFN_WDFREQUESTGETIOQUEUE   pfnWdfRequestGetIoQueue;
	PFN_WDFREQUESTREQUEUE                                     pfnWdfRequestRequeue;
    PFN_WDFREQUESTSTOPACKNOWLEDGE                             pfnWdfRequestStopAcknowledge;
    PFN_WDFREQUESTWDMGETIRP                                   pfnWdfRequestWdmGetIrp;
	PFN_WDFIORESOURCEREQUIREMENTSLISTSETSLOTNUMBER            pfnWdfIoResourceRequirementsListSetSlotNumber;
    PFN_WDFIORESOURCEREQUIREMENTSLISTSETINTERFACETYPE         pfnWdfIoResourceRequirementsListSetInterfaceType;
    PFN_WDFIORESOURCEREQUIREMENTSLISTAPPENDIORESLIST          pfnWdfIoResourceRequirementsListAppendIoResList;
    PFN_WDFIORESOURCEREQUIREMENTSLISTINSERTIORESLIST          pfnWdfIoResourceRequirementsListInsertIoResList;
    PFN_WDFIORESOURCEREQUIREMENTSLISTGETCOUNT                 pfnWdfIoResourceRequirementsListGetCount;
    PFN_WDFIORESOURCEREQUIREMENTSLISTGETIORESLIST             pfnWdfIoResourceRequirementsListGetIoResList;
    PFN_WDFIORESOURCEREQUIREMENTSLISTREMOVE                   pfnWdfIoResourceRequirementsListRemove;
    PFN_WDFIORESOURCEREQUIREMENTSLISTREMOVEBYIORESLIST        pfnWdfIoResourceRequirementsListRemoveByIoResList;
    PFN_WDFIORESOURCELISTCREATE                               pfnWdfIoResourceListCreate;
    PFN_WDFIORESOURCELISTAPPENDDESCRIPTOR                     pfnWdfIoResourceListAppendDescriptor;
    PFN_WDFIORESOURCELISTINSERTDESCRIPTOR                     pfnWdfIoResourceListInsertDescriptor;
    PFN_WDFIORESOURCELISTUPDATEDESCRIPTOR                     pfnWdfIoResourceListUpdateDescriptor;
    PFN_WDFIORESOURCELISTGETCOUNT                             pfnWdfIoResourceListGetCount;
    PFN_WDFIORESOURCELISTGETDESCRIPTOR                        pfnWdfIoResourceListGetDescriptor;
    PFN_WDFIORESOURCELISTREMOVE                               pfnWdfIoResourceListRemove;
    PFN_WDFIORESOURCELISTREMOVEBYDESCRIPTOR                   pfnWdfIoResourceListRemoveByDescriptor;
	PFN_WDFCMRESOURCELISTAPPENDDESCRIPTOR                     pfnWdfCmResourceListAppendDescriptor;
    PFN_WDFCMRESOURCELISTINSERTDESCRIPTOR                     pfnWdfCmResourceListInsertDescriptor;
    PFN_WDFCMRESOURCELISTGETCOUNT                             pfnWdfCmResourceListGetCount;
    PFN_WDFCMRESOURCELISTGETDESCRIPTOR                        pfnWdfCmResourceListGetDescriptor;
    PFN_WDFCMRESOURCELISTREMOVE                               pfnWdfCmResourceListRemove;
    PFN_WDFCMRESOURCELISTREMOVEBYDESCRIPTOR                   pfnWdfCmResourceListRemoveByDescriptor;
	PFN_WDFSTRINGCREATE   pfnWdfStringCreate;
	PFN_WDFSTRINGGETUNICODESTRING   pfnWdfStringGetUnicodeString;
	PFN_WDFOBJECTACQUIRELOCK                                  pfnWdfObjectAcquireLock;
    PFN_WDFOBJECTRELEASELOCK                                  pfnWdfObjectReleaseLock;
    PFN_WDFWAITLOCKCREATE                                     pfnWdfWaitLockCreate;
    PFN_WDFWAITLOCKACQUIRE                                    pfnWdfWaitLockAcquire;
    PFN_WDFWAITLOCKRELEASE                                    pfnWdfWaitLockRelease;
	PFN_WDFSPINLOCKCREATE   pfnWdfSpinLockCreate;
	PFN_WDFSPINLOCKACQUIRE  pfnWdfSpinLockAcquire;
    PFN_WDFSPINLOCKRELEASE  pfnWdfSpinLockRelease;
	PFN_WDFTIMERCREATE   pfnWdfTimerCreate;
	PFN_WDFTIMERSTART   pfnWdfTimerStart;
	PFN_WDFTIMERSTOP   pfnWdfTimerStop;
	PFN_WDFTIMERGETPARENTOBJECT   pfnWdfTimerGetParentObject;
	PFN_WDFUNIMPLEMENTED                              pfnWdfUsbTargetDeviceCreate;
    PFN_WDFUNIMPLEMENTED                 pfnWdfUsbTargetDeviceRetrieveInformation;
    PFN_WDFUNIMPLEMENTED                 pfnWdfUsbTargetDeviceGetDeviceDescriptor;
    PFN_WDFUNIMPLEMENTED            pfnWdfUsbTargetDeviceRetrieveConfigDescriptor;
    PFN_WDFUNIMPLEMENTED                         pfnWdfUsbTargetDeviceQueryString;
    PFN_WDFUNIMPLEMENTED                 pfnWdfUsbTargetDeviceAllocAndQueryString;
    PFN_WDFUNIMPLEMENTED              pfnWdfUsbTargetDeviceFormatRequestForString;
    PFN_WDFUNIMPLEMENTED                    pfnWdfUsbTargetDeviceGetNumInterfaces;
    PFN_WDFUNIMPLEMENTED                        pfnWdfUsbTargetDeviceSelectConfig;
    PFN_WDFUNIMPLEMENTED           pfnWdfUsbTargetDeviceWdmGetConfigurationHandle;
    PFN_WDFUNIMPLEMENTED          pfnWdfUsbTargetDeviceRetrieveCurrentFrameNumber;
    PFN_WDFUNIMPLEMENTED    pfnWdfUsbTargetDeviceSendControlTransferSynchronously;
    PFN_WDFUNIMPLEMENTED     pfnWdfUsbTargetDeviceFormatRequestForControlTransfer;
    PFN_WDFUNIMPLEMENTED              pfnWdfUsbTargetDeviceIsConnectedSynchronous;
    PFN_WDFUNIMPLEMENTED              pfnWdfUsbTargetDeviceResetPortSynchronously;
    PFN_WDFUNIMPLEMENTED              pfnWdfUsbTargetDeviceCyclePortSynchronously;
    PFN_WDFUNIMPLEMENTED           pfnWdfUsbTargetDeviceFormatRequestForCyclePort;
    PFN_WDFUNIMPLEMENTED                pfnWdfUsbTargetDeviceSendUrbSynchronously;
    PFN_WDFUNIMPLEMENTED                 pfnWdfUsbTargetDeviceFormatRequestForUrb;
    PFN_WDFUNIMPLEMENTED                        pfnWdfUsbTargetPipeGetInformation;
    PFN_WDFUNIMPLEMENTED                          pfnWdfUsbTargetPipeIsInEndpoint;
    PFN_WDFUNIMPLEMENTED                         pfnWdfUsbTargetPipeIsOutEndpoint;
    PFN_WDFUNIMPLEMENTED                               pfnWdfUsbTargetPipeGetType;
    PFN_WDFUNIMPLEMENTED           pfnWdfUsbTargetPipeSetNoMaximumPacketSizeCheck;
    PFN_WDFUNIMPLEMENTED                    pfnWdfUsbTargetPipeWriteSynchronously;
    PFN_WDFUNIMPLEMENTED                 pfnWdfUsbTargetPipeFormatRequestForWrite;
    PFN_WDFUNIMPLEMENTED                     pfnWdfUsbTargetPipeReadSynchronously;
    PFN_WDFUNIMPLEMENTED                  pfnWdfUsbTargetPipeFormatRequestForRead;
    PFN_WDFUNIMPLEMENTED                pfnWdfUsbTargetPipeConfigContinuousReader;
    PFN_WDFUNIMPLEMENTED                    pfnWdfUsbTargetPipeAbortSynchronously;
    PFN_WDFUNIMPLEMENTED                 pfnWdfUsbTargetPipeFormatRequestForAbort;
    PFN_WDFUNIMPLEMENTED                    pfnWdfUsbTargetPipeResetSynchronously;
    PFN_WDFUNIMPLEMENTED                 pfnWdfUsbTargetPipeFormatRequestForReset;
    PFN_WDFUNIMPLEMENTED                  pfnWdfUsbTargetPipeSendUrbSynchronously;
    PFN_WDFUNIMPLEMENTED                   pfnWdfUsbTargetPipeFormatRequestForUrb;
    PFN_WDFUNIMPLEMENTED                     pfnWdfUsbInterfaceGetInterfaceNumber;
    PFN_WDFUNIMPLEMENTED                        pfnWdfUsbInterfaceGetNumEndpoints;
    PFN_WDFUNIMPLEMENTED                          pfnWdfUsbInterfaceGetDescriptor;
    PFN_WDFUNIMPLEMENTED                          pfnWdfUsbInterfaceSelectSetting;
    PFN_WDFUNIMPLEMENTED                 pfnWdfUsbInterfaceGetEndpointInformation;
    PFN_WDFUNIMPLEMENTED                        pfnWdfUsbTargetDeviceGetInterface;
    PFN_WDFUNIMPLEMENTED              pfnWdfUsbInterfaceGetConfiguredSettingIndex;
    PFN_WDFUNIMPLEMENTED                  pfnWdfUsbInterfaceGetNumConfiguredPipes;
    PFN_WDFUNIMPLEMENTED                      pfnWdfUsbInterfaceGetConfiguredPipe;
    PFN_WDFUNIMPLEMENTED                      pfnWdfUsbTargetPipeWdmGetPipeHandle;
	PFN_WDFVERIFIERDBGBREAKPOINT   pfnWdfVerifierDbgBreakPoint;
	PFN_WDFVERIFIERKEBUGCHECK                                 pfnWdfVerifierKeBugCheck;
	PFN_WDFUNIMPLEMENTED   pfnWdfWmiProviderCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfWmiProviderGetDevice;
	PFN_WDFUNIMPLEMENTED   pfnWdfWmiProviderIsEnabled;
	PFN_WDFUNIMPLEMENTED   pfnWdfWmiProviderGetTracingHandle;
	PFN_WDFUNIMPLEMENTED   pfnWdfWmiInstanceCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfWmiInstanceRegister;
	PFN_WDFUNIMPLEMENTED   pfnWdfWmiInstanceDeregister;
	PFN_WDFUNIMPLEMENTED   pfnWdfWmiInstanceGetDevice;
	PFN_WDFUNIMPLEMENTED   pfnWdfWmiInstanceGetProvider;
	PFN_WDFUNIMPLEMENTED   pfnWdfWmiInstanceFireEvent;
	PFN_WDFWORKITEMCREATE                                     pfnWdfWorkItemCreate;
    PFN_WDFWORKITEMENQUEUE                                    pfnWdfWorkItemEnqueue;
    PFN_WDFWORKITEMGETPARENTOBJECT                            pfnWdfWorkItemGetParentObject;
    PFN_WDFWORKITEMFLUSH                                      pfnWdfWorkItemFlush;
	PFN_WDFCOMMONBUFFERCREATEWITHCONFIG   pfnWdfCommonBufferCreateWithConfig;
	PFN_WDFDMAENABLERGETFRAGMENTLENGTH                        pfnWdfDmaEnablerGetFragmentLength;
    PFN_WDFDMAENABLERWDMGETDMAADAPTER                         pfnWdfDmaEnablerWdmGetDmaAdapter;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbInterfaceGetNumSettings;
	PFN_WDFDEVICEREMOVEDEPENDENTUSAGEDEVICEOBJECT             pfnWdfDeviceRemoveDependentUsageDeviceObject;
    PFN_WDFDEVICEGETSYSTEMPOWERACTION                         pfnWdfDeviceGetSystemPowerAction;
    PFN_WDFINTERRUPTSETEXTENDEDPOLICY                         pfnWdfInterruptSetExtendedPolicy;
    PFN_WDFIOQUEUEASSIGNFORWARDPROGRESSPOLICY                 pfnWdfIoQueueAssignForwardProgressPolicy;
    PFN_WDFPDOINITASSIGNCONTAINERID                           pfnWdfPdoInitAssignContainerID;
    PFN_WDFPDOINITALLOWFORWARDINGREQUESTTOPARENT              pfnWdfPdoInitAllowForwardingRequestToParent;
	PFN_WDFREQUESTMARKCANCELABLEEX   pfnWdfRequestMarkCancelableEx;
	PFN_WDFREQUESTISRESERVED                                  pfnWdfRequestIsReserved;
    PFN_WDFREQUESTFORWARDTOPARENTDEVICEIOQUEUE                pfnWdfRequestForwardToParentDeviceIoQueue;
} WDFFUNCTIONS, *PWDFFUNCTIONS;

typedef struct _WDFSTRUCTURES {
} WDFSTRUCTURES, *PWDFSTRUCTURES;

typedef struct _WDFVERSION {

    ULONG         Size;
    ULONG         FuncCount;
    WDFFUNCTIONS  Functions;
    ULONG         StructCount;
    WDFSTRUCTURES Structures;

} WDFVERSION, *PWDFVERSION;

// ----- WDFCHILDLIST ----- //

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfChildListCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_CHILD_LIST_CONFIG Config,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES ChildListAttributes,
    _Out_
    WDFCHILDLIST* ChildList
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
WDFEXPORT(WdfChildListGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
WDFEXPORT(WdfChildListRetrievePdo)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList,
    _Inout_
    PWDF_CHILD_RETRIEVE_INFO RetrieveInfo
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfChildListRetrieveAddressDescription)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList,
    _In_
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription,
    _Inout_
    PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfChildListBeginScan)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfChildListEndScan)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfChildListBeginIteration)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList,
    _In_
    PWDF_CHILD_LIST_ITERATOR Iterator
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfChildListRetrieveNextDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList,
    _In_
    PWDF_CHILD_LIST_ITERATOR Iterator,
    _Out_
    WDFDEVICE* Device,
    _Inout_opt_
    PWDF_CHILD_RETRIEVE_INFO Info
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfChildListEndIteration)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList,
    _In_
    PWDF_CHILD_LIST_ITERATOR Iterator
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfChildListAddOrUpdateChildDescriptionAsPresent)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList,
    _In_
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription,
    _In_opt_
    PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfChildListUpdateChildDescriptionAsMissing)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList,
    _In_
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfChildListUpdateAllChildDescriptionsAsPresent)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
WDFEXPORT(WdfChildListRequestChildEject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList,
    _In_
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription
    );

// ----- WDFCHILDLIST ----- //


// ----- WDFCOLLECTION ----- //

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfCollectionCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES CollectionAttributes,
    _Out_
    WDFCOLLECTION* Collection
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG
WDFEXPORT(WdfCollectionGetCount)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOLLECTION Collection
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfCollectionAdd)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOLLECTION Collection,
    _In_
    WDFOBJECT Object
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfCollectionRemove)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOLLECTION Collection,
    _In_
    WDFOBJECT Item
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfCollectionRemoveItem)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOLLECTION Collection,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFOBJECT
WDFEXPORT(WdfCollectionGetItem)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOLLECTION Collection,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFOBJECT
WDFEXPORT(WdfCollectionGetFirstItem)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOLLECTION Collection
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFOBJECT
WDFEXPORT(WdfCollectionGetLastItem)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOLLECTION Collection
    );

// ----- WDFCOLLECTION ----- //


// ----- WDFCOMMONBUFFER ----- //

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfCommonBufferCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMAENABLER DmaEnabler,
    _In_
    _When_(Length == 0, __drv_reportError(Length cannot be zero))
    size_t Length,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFCOMMONBUFFER* CommonBuffer
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfCommonBufferCreateWithConfig)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMAENABLER DmaEnabler,
    _In_
    _When_(Length == 0, __drv_reportError(Length cannot be zero))
    size_t Length,
    _In_
    PWDF_COMMON_BUFFER_CONFIG Config,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFCOMMONBUFFER* CommonBuffer
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PVOID
WDFEXPORT(WdfCommonBufferGetAlignedVirtualAddress)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOMMONBUFFER CommonBuffer
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PHYSICAL_ADDRESS
WDFEXPORT(WdfCommonBufferGetAlignedLogicalAddress)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOMMONBUFFER CommonBuffer
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
size_t
WDFEXPORT(WdfCommonBufferGetLength)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOMMONBUFFER CommonBuffer
    );

// ----- WDFCOMMONBUFFER ----- //


// ----- WDFCONTROL ----- //

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
PWDFDEVICE_INIT
WDFEXPORT(WdfControlDeviceInitAllocate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDRIVER Driver,
    _In_
    CONST UNICODE_STRING* SDDLString
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfControlDeviceInitSetShutdownNotification)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PFN_WDF_DEVICE_SHUTDOWN_NOTIFICATION Notification,
    _In_
    UCHAR Flags
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfControlFinishInitializing)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

// ----- WDFCONTROL ----- //


// ----- WDFDRIVER ----- //
_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
NTAPI
WDFEXPORT(WdfDriverCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PDRIVER_OBJECT DriverObject,
    _In_
    PCUNICODE_STRING RegistryPath,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES DriverAttributes,
    _In_
    PWDF_DRIVER_CONFIG DriverConfig,
    _Out_opt_
    WDFDRIVER* Driver
    );

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
NTAPI
WDFEXPORT(WdfDriverRetrieveVersionString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDRIVER Driver,
    __in
    WDFSTRING String
    );

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
BOOLEAN
NTAPI
WDFEXPORT(WdfDriverIsVersionAvailable)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDRIVER Driver,
    __in
    PWDF_DRIVER_VERSION_AVAILABLE_PARAMS VersionAvailableParams
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
PWSTR
WDFEXPORT(WdfDriverGetRegistryPath)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDRIVER Driver
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PDRIVER_OBJECT
WDFEXPORT(WdfDriverWdmGetDriverObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDRIVER Driver
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDriverOpenParametersRegistryKey)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDRIVER Driver,
    _In_
    ACCESS_MASK DesiredAccess,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES KeyAttributes,
    _Out_
    WDFKEY* Key
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDRIVER
WDFEXPORT(WdfWdmDriverGetWdfDriverHandle)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PDRIVER_OBJECT DriverObject
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDriverRegisterTraceInfo)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PDRIVER_OBJECT DriverObject,
    _In_
    PFN_WDF_TRACE_CALLBACK EvtTraceCallback,
    _In_
    PVOID ControlBlock
    );

// ----- WDFDRIVER ----- //


// ----- WDFFDO ----- //

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PDEVICE_OBJECT
WDFEXPORT(WdfFdoInitWdmGetPhysicalDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfFdoInitOpenRegistryKey)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    ULONG DeviceInstanceKeyType,
    _In_
    ACCESS_MASK DesiredAccess,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES KeyAttributes,
    _Out_
    WDFKEY* Key
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfFdoInitQueryProperty)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    DEVICE_REGISTRY_PROPERTY DeviceProperty,
    _In_
    ULONG BufferLength,
    _Out_writes_bytes_all_opt_(BufferLength)
    PVOID PropertyBuffer,
    _Out_
    PULONG ResultLength
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfFdoInitAllocAndQueryProperty)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    DEVICE_REGISTRY_PROPERTY DeviceProperty,
    _In_
    _Strict_type_match_
    POOL_TYPE PoolType,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES PropertyMemoryAttributes,
    _Out_
    WDFMEMORY* PropertyMemory
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfFdoInitSetEventCallbacks)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_FDO_EVENT_CALLBACKS FdoEventCallbacks
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfFdoInitSetFilter)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfFdoInitSetDefaultChildListConfig)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _Inout_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_CHILD_LIST_CONFIG Config,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES DefaultChildListAttributes
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfFdoQueryForInterface)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Fdo,
    _In_
    LPCGUID InterfaceType,
    _Out_
    PINTERFACE Interface,
    _In_
    USHORT Size,
    _In_
    USHORT Version,
    _In_opt_
    PVOID InterfaceSpecificData
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFCHILDLIST
WDFEXPORT(WdfFdoGetDefaultChildList)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Fdo
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfFdoAddStaticChild)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Fdo,
    _In_
    WDFDEVICE Child
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfFdoLockStaticChildListForIteration)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Fdo
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
WDFEXPORT(WdfFdoRetrieveNextStaticChild)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Fdo,
    _In_opt_
    WDFDEVICE PreviousChild,
    _In_
    ULONG Flags
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfFdoUnlockStaticChildListFromIteration)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Fdo
    );

// ----- WDFFDO ----- //


// ----- WDFFILEOBJECT ----- //

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
PUNICODE_STRING
WDFEXPORT(WdfFileObjectGetFileName)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFFILEOBJECT FileObject
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG
WDFEXPORT(WdfFileObjectGetFlags)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFFILEOBJECT FileObject
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
WDFEXPORT(WdfFileObjectGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFFILEOBJECT FileObject
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PFILE_OBJECT
WDFEXPORT(WdfFileObjectWdmGetFileObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFFILEOBJECT FileObject
    );

// ----- WDFFILEOBJECT ----- //


// ----- WDFINTERRUPT ----- //

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfInterruptCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_INTERRUPT_CONFIG Configuration,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFINTERRUPT* Interrupt
    );

WDFAPI
BOOLEAN
WDFEXPORT(WdfInterruptQueueDpcForIsr)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
WDFEXPORT(WdfInterruptSynchronize)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt,
    _In_
    PFN_WDF_INTERRUPT_SYNCHRONIZE Callback,
    _In_
    WDFCONTEXT Context
    );

_IRQL_requires_max_(DISPATCH_LEVEL + 1)
WDFAPI
VOID
WDFEXPORT(WdfInterruptAcquireLock)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    _Requires_lock_not_held_(_Curr_)
    _Acquires_lock_(_Curr_)
    WDFINTERRUPT Interrupt
    );

_IRQL_requires_max_(DISPATCH_LEVEL + 1)
WDFAPI
VOID
WDFEXPORT(WdfInterruptReleaseLock)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    _Requires_lock_held_(_Curr_)
    _Releases_lock_(_Curr_)
    WDFINTERRUPT Interrupt
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfInterruptEnable)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfInterruptDisable)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    );

_Must_inspect_result_
WDFAPI
PKINTERRUPT
WDFEXPORT(WdfInterruptWdmGetInterrupt)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfInterruptGetInfo)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt,
    _Out_
    PWDF_INTERRUPT_INFO Info
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfInterruptSetPolicy)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt,
    _In_
    WDF_INTERRUPT_POLICY Policy,
    _In_
    WDF_INTERRUPT_PRIORITY Priority,
    _In_
    KAFFINITY TargetProcessorSet
    );

WDFAPI
WDFDEVICE
WDFEXPORT(WdfInterruptGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfInterruptSetExtendedPolicy)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt,
    _In_
    PWDF_INTERRUPT_EXTENDED_POLICY PolicyAndGroup
    );

// ----- WDFINTERRUPT ----- //


// ----- WDFDEVICE -----//

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
NTAPI
WDFEXPORT(WdfDeviceInitSetPnpPowerEventCallbacks)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PWDF_PNPPOWER_EVENT_CALLBACKS PnpPowerEventCallbacks
    );

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
NTAPI
WDFEXPORT(WdfDeviceCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __inout
    PWDFDEVICE_INIT* DeviceInit,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES DeviceAttributes,
    __out
    WDFDEVICE* Device
    );

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
NTAPI
WDFEXPORT(WdfDeviceCreateDeviceInterface)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    CONST GUID *InterfaceClassGUID,
    __in_opt
    PCUNICODE_STRING ReferenceString
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFQUEUE
NTAPI
WDFEXPORT(WdfDeviceGetDefaultQueue)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
NTAPI
WDFEXPORT(WdfDeviceInitSetRequestAttributes)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PWDF_OBJECT_ATTRIBUTES RequestAttributes
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceGetDeviceState)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _Out_
    PWDF_DEVICE_STATE DeviceState
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceSetDeviceState)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_STATE DeviceState
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
WDFEXPORT(WdfWdmDeviceGetWdfDeviceHandle)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PDEVICE_OBJECT DeviceObject
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
PDEVICE_OBJECT
WDFEXPORT(WdfDeviceWdmGetDeviceObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
PDEVICE_OBJECT
WDFEXPORT(WdfDeviceWdmGetAttachedDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
PDEVICE_OBJECT
WDFEXPORT(WdfDeviceWdmGetPhysicalDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceWdmDispatchPreprocessedIrp)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PIRP Irp
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceAddDependentUsageDeviceObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PDEVICE_OBJECT DependentDevice
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceAddRemovalRelationsPhysicalDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PDEVICE_OBJECT PhysicalDevice
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceRemoveRemovalRelationsPhysicalDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PDEVICE_OBJECT PhysicalDevice
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceClearRemovalRelationsDevices)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDFDRIVER
WDFEXPORT(WdfDeviceGetDriver)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceRetrieveDeviceName)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    WDFSTRING String
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceAssignMofResourceName)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PCUNICODE_STRING MofResourceName
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDFIOTARGET
WDFEXPORT(WdfDeviceGetIoTarget)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDF_DEVICE_PNP_STATE
WDFEXPORT(WdfDeviceGetDevicePnpState)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDF_DEVICE_POWER_STATE
WDFEXPORT(WdfDeviceGetDevicePowerState)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDF_DEVICE_POWER_POLICY_STATE
WDFEXPORT(WdfDeviceGetDevicePowerPolicyState)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceAssignS0IdleSettings)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_POWER_POLICY_IDLE_SETTINGS Settings
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceAssignSxWakeSettings)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_POWER_POLICY_WAKE_SETTINGS Settings
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceOpenRegistryKey)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    ULONG DeviceInstanceKeyType,
    _In_
    ACCESS_MASK DesiredAccess,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES KeyAttributes,
    _Out_
    WDFKEY* Key
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceSetSpecialFileSupport)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    WDF_SPECIAL_FILE_TYPE FileType,
    _In_
    BOOLEAN FileTypeIsSupported
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceSetCharacteristics)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    ULONG DeviceCharacteristics
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
ULONG
WDFEXPORT(WdfDeviceGetCharacteristics)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
ULONG
WDFEXPORT(WdfDeviceGetAlignmentRequirement)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceSetAlignmentRequirement)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    ULONG AlignmentRequirement
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceInitFree)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceInitSetPowerPolicyEventCallbacks)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_POWER_POLICY_EVENT_CALLBACKS PowerPolicyEventCallbacks
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceInitSetPowerPolicyOwnership)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    BOOLEAN IsPowerPolicyOwner
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceInitRegisterPnpStateChangeCallback)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    WDF_DEVICE_PNP_STATE PnpState,
    _In_
    PFN_WDF_DEVICE_PNP_STATE_CHANGE_NOTIFICATION EvtDevicePnpStateChange,
    _In_
    ULONG CallbackTypes
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceInitRegisterPowerStateChangeCallback)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    WDF_DEVICE_POWER_STATE PowerState,
    _In_
    PFN_WDF_DEVICE_POWER_STATE_CHANGE_NOTIFICATION EvtDevicePowerStateChange,
    _In_
    ULONG CallbackTypes
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceInitRegisterPowerPolicyStateChangeCallback)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    WDF_DEVICE_POWER_POLICY_STATE PowerPolicyState,
    _In_
    PFN_WDF_DEVICE_POWER_POLICY_STATE_CHANGE_NOTIFICATION EvtDevicePowerPolicyStateChange,
    _In_
    ULONG CallbackTypes
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceInitSetExclusive)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    BOOLEAN IsExclusive
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceInitSetIoType)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    WDF_DEVICE_IO_TYPE IoType
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceInitSetPowerNotPageable)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceInitSetPowerPageable)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceInitSetPowerInrush)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceInitSetDeviceType)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    DEVICE_TYPE DeviceType
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceInitAssignName)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_opt_
    PCUNICODE_STRING DeviceName
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceInitAssignSDDLString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_opt_
    PCUNICODE_STRING SDDLString
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceInitSetDeviceClass)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    CONST GUID* DeviceClassGuid
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceInitSetCharacteristics)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    ULONG DeviceCharacteristics,
    _In_
    BOOLEAN OrInValues
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceInitSetFileObjectConfig)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_FILEOBJECT_CONFIG FileObjectConfig,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES FileObjectAttributes
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceInitAssignWdmIrpPreprocessCallback)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PFN_WDFDEVICE_WDM_IRP_PREPROCESS EvtDeviceWdmIrpPreprocess,
    _In_
    UCHAR MajorFunction,
    _When_(NumMinorFunctions > 0, _In_reads_bytes_(NumMinorFunctions))
    _When_(NumMinorFunctions == 0, _In_opt_)
    PUCHAR MinorFunctions,
    _In_
    ULONG NumMinorFunctions
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceInitSetIoInCallerContextCallback)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PFN_WDF_IO_IN_CALLER_CONTEXT EvtIoInCallerContext
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceSetStaticStopRemove)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    BOOLEAN Stoppable
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceSetDeviceInterfaceState)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    CONST GUID* InterfaceClassGUID,
    _In_opt_
    PCUNICODE_STRING ReferenceString,
    _In_
    BOOLEAN IsInterfaceEnabled
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceRetrieveDeviceInterfaceString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    CONST GUID* InterfaceClassGUID,
    _In_opt_
    PCUNICODE_STRING ReferenceString,
    _In_
    WDFSTRING String
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceCreateSymbolicLink)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PCUNICODE_STRING SymbolicLinkName
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceQueryProperty)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    DEVICE_REGISTRY_PROPERTY DeviceProperty,
    _In_
    ULONG BufferLength,
    _Out_writes_bytes_all_(BufferLength)
    PVOID PropertyBuffer,
    _Out_
    PULONG ResultLength
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceAllocAndQueryProperty)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    DEVICE_REGISTRY_PROPERTY DeviceProperty,
    _In_
    _Strict_type_match_
    POOL_TYPE PoolType,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES PropertyMemoryAttributes,
    _Out_
    WDFMEMORY* PropertyMemory
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceSetPnpCapabilities)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_PNP_CAPABILITIES PnpCapabilities
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceSetPowerCapabilities)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_POWER_CAPABILITIES PowerCapabilities
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceSetBusInformationForChildren)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PPNP_BUS_INFORMATION BusInformation
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceIndicateWakeStatus)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    NTSTATUS WaitWakeStatus
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceSetFailed)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    WDF_DEVICE_FAILED_ACTION FailedAction
    );

__checkReturn
__drv_when(WaitForD0 == 0, __drv_maxIRQL(DISPATCH_LEVEL))
__drv_when(WaitForD0 != 0, __drv_maxIRQL(PASSIVE_LEVEL))
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceStopIdle)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    BOOLEAN WaitForD0
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceResumeIdle)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFFILEOBJECT
WDFEXPORT(WdfDeviceGetFileObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PFILE_OBJECT FileObject
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceEnqueueRequest)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    WDFREQUEST Request
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceConfigureRequestDispatching)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    WDFQUEUE Queue,
    _In_
    _Strict_type_match_
    WDF_REQUEST_TYPE RequestType
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceRemoveDependentUsageDeviceObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PDEVICE_OBJECT DependentDevice
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
POWER_ACTION
WDFEXPORT(WdfDeviceGetSystemPowerAction)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

// ----- WDFDEVICE -----//


// ----- WDFDMA -----//

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDmaEnablerCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DMA_ENABLER_CONFIG Config,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFDMAENABLER* DmaEnablerHandle
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
size_t
WDFEXPORT(WdfDmaEnablerGetMaximumLength)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMAENABLER DmaEnabler
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
size_t
WDFEXPORT(WdfDmaEnablerGetMaximumScatterGatherElements)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMAENABLER DmaEnabler
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDmaEnablerSetMaximumScatterGatherElements)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMAENABLER DmaEnabler,
    _In_
    _When_(MaximumFragments == 0, __drv_reportError(MaximumFragments cannot be zero))
    size_t MaximumFragments
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
size_t
WDFEXPORT(WdfDmaEnablerGetFragmentLength)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMAENABLER DmaEnabler,
    _In_
    WDF_DMA_DIRECTION DmaDirection
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PDMA_ADAPTER
WDFEXPORT(WdfDmaEnablerWdmGetDmaAdapter)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMAENABLER DmaEnabler,
    _In_
    WDF_DMA_DIRECTION DmaDirection
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDmaTransactionCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMAENABLER DmaEnabler,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFDMATRANSACTION* DmaTransaction
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDmaTransactionInitialize)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction,
    _In_
    PFN_WDF_PROGRAM_DMA EvtProgramDmaFunction,
    _In_
    WDF_DMA_DIRECTION DmaDirection,
    _In_
    PMDL Mdl,
    _In_
    PVOID VirtualAddress,
    _In_
    _When_(Length == 0, __drv_reportError(Length cannot be zero))
    size_t Length
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDmaTransactionInitializeUsingRequest)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction,
    _In_
    WDFREQUEST Request,
    _In_
    PFN_WDF_PROGRAM_DMA EvtProgramDmaFunction,
    _In_
    WDF_DMA_DIRECTION DmaDirection
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDmaTransactionExecute)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction,
    _In_opt_
    WDFCONTEXT Context
    );

_Success_(TRUE)
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDmaTransactionRelease)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
WDFEXPORT(WdfDmaTransactionDmaCompleted)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction,
    _Out_
    NTSTATUS* Status
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
WDFEXPORT(WdfDmaTransactionDmaCompletedWithLength)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction,
    _In_
    size_t TransferredLength,
    _Out_
    NTSTATUS* Status
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
WDFEXPORT(WdfDmaTransactionDmaCompletedFinal)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction,
    _In_
    size_t FinalTransferredLength,
    _Out_
    NTSTATUS* Status
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
size_t
WDFEXPORT(WdfDmaTransactionGetBytesTransferred)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDmaTransactionSetMaximumLength)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction,
    _In_
    size_t MaximumLength
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFREQUEST
WDFEXPORT(WdfDmaTransactionGetRequest)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
size_t
WDFEXPORT(WdfDmaTransactionGetCurrentDmaTransferLength)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
WDFEXPORT(WdfDmaTransactionGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
size_t
WDFEXPORT(WdfDmaEnablerGetFragmentLength)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMAENABLER DmaEnabler,
    _In_
    WDF_DMA_DIRECTION DmaDirection
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PDMA_ADAPTER
WDFEXPORT(WdfDmaEnablerWdmGetDmaAdapter)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMAENABLER DmaEnabler,
    _In_
    WDF_DMA_DIRECTION DmaDirection
    );

// ----- WDFDMA -----//


// ----- WDFDPC -----//

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDpcCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDF_DPC_CONFIG Config,
    _In_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFDPC* Dpc
    );

_IRQL_requires_max_(HIGH_LEVEL)
WDFAPI
BOOLEAN
WDFEXPORT(WdfDpcEnqueue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDPC Dpc
    );

_When_(Wait == __true, _IRQL_requires_max_(PASSIVE_LEVEL))
_When_(Wait == __false, _IRQL_requires_max_(HIGH_LEVEL))
WDFAPI
BOOLEAN
WDFEXPORT(WdfDpcCancel)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDPC Dpc,
    _In_
    BOOLEAN Wait
    );

_IRQL_requires_max_(HIGH_LEVEL)
WDFAPI
WDFOBJECT
WDFEXPORT(WdfDpcGetParentObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDPC Dpc
    );

_IRQL_requires_max_(HIGH_LEVEL)
WDFAPI
PKDPC
WDFEXPORT(WdfDpcWdmGetDpc)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDPC Dpc
    );

// ----- WDFDPC -----//


// ----- WDFIO -----//

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
NTAPI
WDFEXPORT(WdfIoQueueStart)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue
    );

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
NTAPI
WDFEXPORT(WdfIoQueueCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_IO_QUEUE_CONFIG Config,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES QueueAttributes,
    __out_opt
    WDFQUEUE* Queue
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDF_IO_QUEUE_STATE
WDFEXPORT(WdfIoQueueGetState)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue,
    _Out_opt_
    PULONG QueueRequests,
    _Out_opt_
    PULONG DriverRequests
    );

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
NTAPI
WDFEXPORT(WdfIoQueueStopSynchronously)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfIoQueueStop)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue,
    _When_(Context != 0, _In_)
    _When_(Context == 0, _In_opt_)
    PFN_WDF_IO_QUEUE_STATE StopComplete,
    _When_(StopComplete != 0, _In_)
    _When_(StopComplete == 0, _In_opt_)
    WDFCONTEXT Context
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
WDFEXPORT(WdfIoQueueGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoQueueRetrieveNextRequest)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue,
    _Out_
    WDFREQUEST* OutRequest
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoQueueRetrieveRequestByFileObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue,
    _In_
    WDFFILEOBJECT FileObject,
    _Out_
    WDFREQUEST* OutRequest
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoQueueFindRequest)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue,
    _In_opt_
    WDFREQUEST FoundRequest,
    _In_opt_
    WDFFILEOBJECT FileObject,
    _Inout_opt_
    PWDF_REQUEST_PARAMETERS Parameters,
    _Out_
    WDFREQUEST* OutRequest
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoQueueRetrieveFoundRequest)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue,
    _In_
    WDFREQUEST FoundRequest,
    _Out_
    WDFREQUEST* OutRequest
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfIoQueueDrainSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfIoQueueDrain)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue,
    _When_(Context != 0, _In_)
    _When_(Context == 0, _In_opt_)
    PFN_WDF_IO_QUEUE_STATE DrainComplete,
    _When_(DrainComplete != 0, _In_)
    _When_(DrainComplete == 0, _In_opt_)
    WDFCONTEXT Context
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfIoQueuePurgeSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfIoQueuePurge)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue,
    _When_(Context != 0, _In_)
    _When_(Context == 0, _In_opt_)
    PFN_WDF_IO_QUEUE_STATE PurgeComplete,
    _When_(PurgeComplete != 0, _In_)
    _When_(PurgeComplete == 0, _In_opt_)
    WDFCONTEXT Context
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoQueueReadyNotify)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue,
    _In_opt_
    PFN_WDF_IO_QUEUE_STATE QueueReady,
    _In_opt_
    WDFCONTEXT Context
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoQueueAssignForwardProgressPolicy)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue,
    _In_
    PWDF_IO_QUEUE_FORWARD_PROGRESS_POLICY ForwardProgressPolicy
    );

// ----- WDFIO -----//


// ----- WDFIOTARGET -----//

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoTargetCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES IoTargetAttributes,
    _Out_
    WDFIOTARGET* IoTarget
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoTargetOpen)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    PWDF_IO_TARGET_OPEN_PARAMS OpenParams
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfIoTargetCloseForQueryRemove)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfIoTargetClose)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoTargetStart)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    );

_When_(Action == 3, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(Action == 0 || Action == 1 || Action == 2, _IRQL_requires_max_(PASSIVE_LEVEL))
WDFAPI
VOID
WDFEXPORT(WdfIoTargetStop)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    _Strict_type_match_
    WDF_IO_TARGET_SENT_IO_ACTION Action
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDF_IO_TARGET_STATE
WDFEXPORT(WdfIoTargetGetState)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
WDFEXPORT(WdfIoTargetGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoTargetQueryTargetProperty)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    DEVICE_REGISTRY_PROPERTY DeviceProperty,
    _In_
    ULONG BufferLength,
    _When_(BufferLength != 0, _Out_writes_bytes_to_opt_(BufferLength, *ResultLength))
    _When_(BufferLength == 0, _Out_opt_)
    PVOID PropertyBuffer,
    _Deref_out_range_(<=,BufferLength)
    PULONG ResultLength
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoTargetAllocAndQueryTargetProperty)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    DEVICE_REGISTRY_PROPERTY DeviceProperty,
    _In_
    _Strict_type_match_
    POOL_TYPE PoolType,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES PropertyMemoryAttributes,
    _Out_
    WDFMEMORY* PropertyMemory
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoTargetQueryForInterface)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    LPCGUID InterfaceType,
    _Out_
    PINTERFACE Interface,
    _In_
    USHORT Size,
    _In_
    USHORT Version,
    _In_opt_
    PVOID InterfaceSpecificData
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PDEVICE_OBJECT
WDFEXPORT(WdfIoTargetWdmGetTargetDeviceObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PDEVICE_OBJECT
WDFEXPORT(WdfIoTargetWdmGetTargetPhysicalDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PFILE_OBJECT
WDFEXPORT(WdfIoTargetWdmGetTargetFileObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
HANDLE
WDFEXPORT(WdfIoTargetWdmGetTargetFileHandle)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoTargetSendReadSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_opt_
    WDFREQUEST Request,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR OutputBuffer,
    _In_opt_
    PLONGLONG DeviceOffset,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    _Out_opt_
    PULONG_PTR BytesRead
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoTargetFormatRequestForRead)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    WDFREQUEST Request,
    _In_opt_
    WDFMEMORY OutputBuffer,
    _In_opt_
    PWDFMEMORY_OFFSET OutputBufferOffset,
    _In_opt_
    PLONGLONG DeviceOffset
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoTargetSendWriteSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_opt_
    WDFREQUEST Request,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR InputBuffer,
    _In_opt_
    PLONGLONG DeviceOffset,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    _Out_opt_
    PULONG_PTR BytesWritten
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoTargetFormatRequestForWrite)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    WDFREQUEST Request,
    _In_opt_
    WDFMEMORY InputBuffer,
    _In_opt_
    PWDFMEMORY_OFFSET InputBufferOffset,
    _In_opt_
    PLONGLONG DeviceOffset
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoTargetSendIoctlSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_opt_
    WDFREQUEST Request,
    _In_
    ULONG IoctlCode,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR InputBuffer,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR OutputBuffer,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    _Out_opt_
    PULONG_PTR BytesReturned
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoTargetFormatRequestForIoctl)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    WDFREQUEST Request,
    _In_
    ULONG IoctlCode,
    _In_opt_
    WDFMEMORY InputBuffer,
    _In_opt_
    PWDFMEMORY_OFFSET InputBufferOffset,
    _In_opt_
    WDFMEMORY OutputBuffer,
    _In_opt_
    PWDFMEMORY_OFFSET OutputBufferOffset
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoTargetSendInternalIoctlSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_opt_
    WDFREQUEST Request,
    _In_
    ULONG IoctlCode,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR InputBuffer,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR OutputBuffer,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    _Out_opt_
    PULONG_PTR BytesReturned
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoTargetFormatRequestForInternalIoctl)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    WDFREQUEST Request,
    _In_
    ULONG IoctlCode,
    _In_opt_
    WDFMEMORY InputBuffer,
    _In_opt_
    PWDFMEMORY_OFFSET InputBufferOffset,
    _In_opt_
    WDFMEMORY OutputBuffer,
    _In_opt_
    PWDFMEMORY_OFFSET OutputBufferOffset
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoTargetSendInternalIoctlOthersSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_opt_
    WDFREQUEST Request,
    _In_
    ULONG IoctlCode,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR OtherArg1,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR OtherArg2,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR OtherArg4,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    _Out_opt_
    PULONG_PTR BytesReturned
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoTargetFormatRequestForInternalIoctlOthers)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    WDFREQUEST Request,
    _In_
    ULONG IoctlCode,
    _In_opt_
    WDFMEMORY OtherArg1,
    _In_opt_
    PWDFMEMORY_OFFSET OtherArg1Offset,
    _In_opt_
    WDFMEMORY OtherArg2,
    _In_opt_
    PWDFMEMORY_OFFSET OtherArg2Offset,
    _In_opt_
    WDFMEMORY OtherArg4,
    _In_opt_
    PWDFMEMORY_OFFSET OtherArg4Offset
    );

// ----- WDFIOTARGET -----//


// ----- WDFTIMER -----//

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
NTAPI
WDFEXPORT(WdfTimerCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDF_TIMER_CONFIG Config,
    __in
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFTIMER * Timer
    );

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
NTAPI
WDFEXPORT(WdfTimerStart)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFTIMER Timer,
    __in
    LONGLONG DueTime
    );

__drv_when(Wait == __true, __drv_maxIRQL(PASSIVE_LEVEL))
__drv_when(Wait == __false, __drv_maxIRQL(DISPATCH_LEVEL))
BOOLEAN
NTAPI
WDFEXPORT(WdfTimerStop)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFTIMER Timer,
    __in
    BOOLEAN  Wait
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFOBJECT
NTAPI
WDFEXPORT(WdfTimerGetParentObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFTIMER Timer
    );

// ----- WDFTIMER -----//


// ----- WDFUSB -----//


// ----- WDFUSB -----//


// ----- WDFREQUEST -----//

__drv_maxIRQL(DISPATCH_LEVEL)
WDFQUEUE
WDFAPI
WDFEXPORT(WdfRequestGetIoQueue)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
NTAPI
WDFEXPORT(WdfRequestCompleteWithInformation)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in
    NTSTATUS RequestStatus,
    __in
    ULONG_PTR Information
    );

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
NTAPI
WDFEXPORT(WdfRequestRetrieveOutputMemory)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __out
    WDFMEMORY *Memory
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
NTAPI
WDFEXPORT(WdfRequestComplete)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in
    NTSTATUS RequestStatus
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
NTAPI
WDFEXPORT(WdfRequestSetInformation)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in
    ULONG_PTR Information
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfRequestMarkCancelable)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in
   WDFREQUEST Request,
   __in
   PFN_WDF_REQUEST_CANCEL  EvtRequestCancel
   );

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
NTAPI
WDFEXPORT(WdfRequestRetrieveInputMemory)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __out
    WDFMEMORY *Memory
    );

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
NTAPI
WDFEXPORT(WdfRequestUnmarkCancelable)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in
   WDFREQUEST Request
   );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES RequestAttributes,
    _In_opt_
    WDFIOTARGET IoTarget,
    _Out_
    WDFREQUEST* Request
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestCreateFromIrp)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES RequestAttributes,
    _In_
    PIRP Irp,
    _In_
    BOOLEAN RequestFreesIrp,
    _Out_
    WDFREQUEST* Request
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestReuse)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    PWDF_REQUEST_REUSE_PARAMS ReuseParams
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestChangeTarget)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    WDFIOTARGET IoTarget
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfRequestFormatRequestUsingCurrentType)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfRequestWdmFormatUsingStackLocation)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    PIO_STACK_LOCATION Stack
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_When_(Options->Flags & WDF_REQUEST_SEND_OPTION_SYNCHRONOUS == 0, _Must_inspect_result_)
WDFAPI
BOOLEAN
WDFEXPORT(WdfRequestSend)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    WDFIOTARGET Target,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS Options
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestGetStatus)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
WDFEXPORT(WdfRequestIsCanceled)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
WDFEXPORT(WdfRequestCancelSentRequest)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
WDFAPI
BOOLEAN
WDFEXPORT(WdfRequestIsFrom32BitProcess)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfRequestSetCompletionRoutine)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_opt_
    PFN_WDF_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
    _In_opt_ __drv_aliasesMem
    WDFCONTEXT CompletionContext
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfRequestGetCompletionParams)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _Out_
    PWDF_REQUEST_COMPLETION_PARAMS Params
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestAllocateTimer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfRequestCompleteWithPriorityBoost)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    NTSTATUS Status,
    _In_
    CCHAR PriorityBoost
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfRequestGetParameters)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _Out_
    PWDF_REQUEST_PARAMETERS Parameters
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestRetrieveInputBuffer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    size_t MinimumRequiredLength,
    _Outptr_result_bytebuffer_(*Length)
    PVOID* Buffer,
    _Out_opt_
    size_t* Length
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestRetrieveOutputBuffer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    size_t MinimumRequiredSize,
    _Outptr_result_bytebuffer_(*Length)
    PVOID* Buffer,
    _Out_opt_
    size_t* Length
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestRetrieveInputWdmMdl)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _Outptr_
    PMDL* Mdl
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestRetrieveOutputWdmMdl)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _Outptr_
    PMDL* Mdl
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestRetrieveUnsafeUserInputBuffer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    size_t MinimumRequiredLength,
    _Outptr_result_bytebuffer_maybenull_(*Length)
    PVOID* InputBuffer,
    _Out_opt_
    size_t* Length
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestRetrieveUnsafeUserOutputBuffer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    size_t MinimumRequiredLength,
    _Outptr_result_bytebuffer_maybenull_(*Length)
    PVOID* OutputBuffer,
    _Out_opt_
    size_t* Length
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG_PTR
WDFEXPORT(WdfRequestGetInformation)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFFILEOBJECT
WDFEXPORT(WdfRequestGetFileObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestProbeAndLockUserBufferForRead)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_reads_bytes_(Length)
    PVOID Buffer,
    _In_
    size_t Length,
    _Out_
    WDFMEMORY* MemoryObject
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestProbeAndLockUserBufferForWrite)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_reads_bytes_(Length)
    PVOID Buffer,
    _In_
    size_t Length,
    _Out_
    WDFMEMORY* MemoryObject
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
KPROCESSOR_MODE
WDFEXPORT(WdfRequestGetRequestorMode)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestForwardToIoQueue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    WDFQUEUE DestinationQueue
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestRequeue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfRequestStopAcknowledge)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    BOOLEAN Requeue
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PIRP
WDFEXPORT(WdfRequestWdmGetIrp)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
WDFEXPORT(WdfRequestIsReserved)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestForwardToParentDeviceIoQueue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    WDFQUEUE ParentDeviceQueue,
    _In_
    PWDF_REQUEST_FORWARD_OPTIONS ForwardOptions
    );

// ----- WDFREQUEST -----//


// ----- WDFRESOURCE -----//

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfIoResourceRequirementsListSetSlotNumber)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESREQLIST RequirementsList,
    _In_
    ULONG SlotNumber
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfIoResourceRequirementsListSetInterfaceType)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESREQLIST RequirementsList,
    _In_
    _Strict_type_match_
    INTERFACE_TYPE InterfaceType
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoResourceRequirementsListAppendIoResList)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESREQLIST RequirementsList,
    _In_
    WDFIORESLIST IoResList
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoResourceRequirementsListInsertIoResList)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESREQLIST RequirementsList,
    _In_
    WDFIORESLIST IoResList,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG
WDFEXPORT(WdfIoResourceRequirementsListGetCount)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESREQLIST RequirementsList
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFIORESLIST
WDFEXPORT(WdfIoResourceRequirementsListGetIoResList)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESREQLIST RequirementsList,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfIoResourceRequirementsListRemove)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESREQLIST RequirementsList,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfIoResourceRequirementsListRemoveByIoResList)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESREQLIST RequirementsList,
    _In_
    WDFIORESLIST IoResList
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoResourceListCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESREQLIST RequirementsList,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFIORESLIST* ResourceList
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoResourceListAppendDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESLIST ResourceList,
    _In_
    PIO_RESOURCE_DESCRIPTOR Descriptor
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoResourceListInsertDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESLIST ResourceList,
    _In_
    PIO_RESOURCE_DESCRIPTOR Descriptor,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfIoResourceListUpdateDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESLIST ResourceList,
    _In_
    PIO_RESOURCE_DESCRIPTOR Descriptor,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG
WDFEXPORT(WdfIoResourceListGetCount)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESLIST ResourceList
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PIO_RESOURCE_DESCRIPTOR
WDFEXPORT(WdfIoResourceListGetDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESLIST ResourceList,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfIoResourceListRemove)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESLIST ResourceList,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfIoResourceListRemoveByDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESLIST ResourceList,
    _In_
    PIO_RESOURCE_DESCRIPTOR Descriptor
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfCmResourceListAppendDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCMRESLIST List,
    _In_
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfCmResourceListInsertDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCMRESLIST List,
    _In_
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG
WDFEXPORT(WdfCmResourceListGetCount)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCMRESLIST List
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PCM_PARTIAL_RESOURCE_DESCRIPTOR
WDFEXPORT(WdfCmResourceListGetDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCMRESLIST List,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfCmResourceListRemove)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCMRESLIST List,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfCmResourceListRemoveByDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCMRESLIST List,
    _In_
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    );

// ----- WDFRESOURCE -----//


// ----- WDFMEMORY -----//

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
NTAPI
WDFEXPORT(WdfMemoryCopyToBuffer)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFMEMORY SourceMemory,
    __in
    size_t SourceOffset,
    __out_bcount( NumBytesToCopyTo)
    PVOID Buffer,
    __in
    __drv_when(NumBytesToCopyTo == 0, __drv_reportError(NumBytesToCopyTo cannot be zero))
    size_t NumBytesToCopyTo
    );

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
NTAPI
WDFEXPORT(WdfMemoryCopyFromBuffer)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFMEMORY DestinationMemory,
    __in
    size_t DestinationOffset,
    __in
    PVOID Buffer,
    __in
    __drv_when(NumBytesToCopyFrom == 0, __drv_reportError(NumBytesToCopyFrom cannot be zero))
    size_t NumBytesToCopyFrom
    );

_Must_inspect_result_
_When_(PoolType == 1 || PoolType == 257, _IRQL_requires_max_(APC_LEVEL))
_When_(PoolType == 0 || PoolType == 256 || PoolType == 512, _IRQL_requires_max_(DISPATCH_LEVEL))
WDFAPI
NTSTATUS
WDFEXPORT(WdfMemoryCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _In_
    _Strict_type_match_
    POOL_TYPE PoolType,
    _In_opt_
    ULONG PoolTag,
    _In_
    _When_(BufferSize == 0, __drv_reportError(BufferSize cannot be zero))
    size_t BufferSize,
    _Out_
    WDFMEMORY* Memory,
    _Outptr_opt_result_bytebuffer_(BufferSize)
    PVOID* Buffer
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfMemoryCreatePreallocated)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _In_ __drv_aliasesMem
    PVOID Buffer,
    _In_
    _When_(BufferSize == 0, __drv_reportError(BufferSize cannot be zero))
    size_t BufferSize,
    _Out_
    WDFMEMORY* Memory
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PVOID
WDFEXPORT(WdfMemoryGetBuffer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFMEMORY Memory,
    _Out_opt_
    size_t* BufferSize
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfMemoryAssignBuffer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFMEMORY Memory,
    _Pre_notnull_ _Pre_writable_byte_size_(BufferSize)
    PVOID Buffer,
    _In_
    _When_(BufferSize == 0, __drv_reportError(BufferSize cannot be zero))
    size_t BufferSize
    );

_Must_inspect_result_
_When_(PoolType == 1 || PoolType == 257, _IRQL_requires_max_(APC_LEVEL))
_When_(PoolType == 0 || PoolType == 256, _IRQL_requires_max_(DISPATCH_LEVEL))
WDFAPI
NTSTATUS
WDFEXPORT(WdfLookasideListCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES LookasideAttributes,
    _In_
    _When_(BufferSize == 0, __drv_reportError(BufferSize cannot be zero))
    size_t BufferSize,
    _In_
    _Strict_type_match_
    POOL_TYPE PoolType,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES MemoryAttributes,
    _In_opt_
    ULONG PoolTag,
    _Out_
    WDFLOOKASIDE* Lookaside
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfMemoryCreateFromLookaside)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFLOOKASIDE Lookaside,
    _Out_
    WDFMEMORY* Memory
    );

// ----- WDFMEMORY -----//


// ----- WDFMINIPORT -----//

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceMiniportCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDRIVER Driver,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _In_
    PDEVICE_OBJECT DeviceObject,
    _In_opt_
    PDEVICE_OBJECT AttachedDeviceObject,
    _In_opt_
    PDEVICE_OBJECT Pdo,
    _Out_
    WDFDEVICE* Device
    );

WDFAPI
VOID
WDFEXPORT(WdfDriverMiniportUnload)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDRIVER Driver
    );

// ----- WDFMINIPORT -----//


// ----- WDFSTRING -----//
_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
NTAPI
WDFEXPORT(WdfStringCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    PCUNICODE_STRING UnicodeString,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES StringAttributes,
    __out
    WDFSTRING* String
    );

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
NTAPI
WDFEXPORT(WdfStringGetUnicodeString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFSTRING String,
    __out
    PUNICODE_STRING UnicodeString
    );

// ----- WDFSTRING -----//


// ----- WDFSYNC -----//

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfObjectAcquireLock)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    _Requires_lock_not_held_(_Curr_)
    _Acquires_lock_(_Curr_)
    WDFOBJECT Object
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfObjectReleaseLock)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    _Requires_lock_held_(_Curr_)
    _Releases_lock_(_Curr_)
    WDFOBJECT Object
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfWaitLockCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES LockAttributes,
    _Out_
    WDFWAITLOCK* Lock
    );

_When_(Timeout == NULL, _IRQL_requires_max_(PASSIVE_LEVEL))
_When_(Timeout != NULL && *Timeout == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(Timeout != NULL && *Timeout != 0, _IRQL_requires_max_(PASSIVE_LEVEL))
_Always_(_When_(Timeout == NULL, _Acquires_lock_(Lock)))
_When_(Timeout != NULL && return == STATUS_SUCCESS, _Acquires_lock_(Lock))
_When_(Timeout != NULL, _Must_inspect_result_)
WDFAPI
NTSTATUS
WDFEXPORT(WdfWaitLockAcquire)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    _Requires_lock_not_held_(_Curr_)
    WDFWAITLOCK Lock,
    _In_opt_
    PLONGLONG Timeout
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfWaitLockRelease)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    _Requires_lock_held_(_Curr_)
    _Releases_lock_(_Curr_)
    WDFWAITLOCK Lock
    );

// ----- WDFSYNC -----//


// ----- WDFOBJECT -----//

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI    
VOID
NTAPI
WDFEXPORT(WdfObjectDelete)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFOBJECT Object
    );

_IRQL_requires_max_(DISPATCH_LEVEL+1)
WDFAPI
PVOID
FASTCALL
WDFEXPORT(WdfObjectGetTypedContextWorker)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFOBJECT Handle,
    _In_
    PCWDF_OBJECT_CONTEXT_TYPE_INFO TypeInfo
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfObjectAllocateContext)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFOBJECT Handle,
    _In_
    PWDF_OBJECT_ATTRIBUTES ContextAttributes,
    _Outptr_opt_
    PVOID* Context
    );

_IRQL_requires_max_(DISPATCH_LEVEL+1)
WDFAPI
WDFOBJECT
FASTCALL
WDFEXPORT(WdfObjectContextGetObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PVOID ContextPointer
    );

WDFAPI
VOID
WDFEXPORT(WdfObjectReferenceActual)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFOBJECT Handle,
    _In_opt_
    PVOID Tag,
    _In_
    LONG Line,
    _In_z_
    PCHAR File
    );

WDFAPI
VOID
WDFEXPORT(WdfObjectDereferenceActual)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFOBJECT Handle,
    _In_opt_
    PVOID Tag,
    _In_
    LONG Line,
    _In_z_
    PCHAR File
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfObjectCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFOBJECT* Object
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfObjectQuery)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFOBJECT Object,
    _In_
    CONST GUID* Guid,
    _In_
    ULONG QueryBufferLength,
    _Out_writes_bytes_(QueryBufferLength)
    PVOID QueryBuffer
    );

// ----- WDFOBJECT -----//


// ----- WDFPDO -----//

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
PWDFDEVICE_INIT
WDFEXPORT(WdfPdoInitAllocate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE ParentDevice
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfPdoInitSetEventCallbacks)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_PDO_EVENT_CALLBACKS DispatchTable
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfPdoInitAssignDeviceID)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PCUNICODE_STRING DeviceID
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfPdoInitAssignInstanceID)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PCUNICODE_STRING InstanceID
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfPdoInitAddHardwareID)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PCUNICODE_STRING HardwareID
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfPdoInitAddCompatibleID)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PCUNICODE_STRING CompatibleID
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfPdoInitAssignContainerID)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PCUNICODE_STRING ContainerID
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfPdoInitAddDeviceText)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PCUNICODE_STRING DeviceDescription,
    _In_
    PCUNICODE_STRING DeviceLocation,
    _In_
    LCID LocaleId
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfPdoInitSetDefaultLocale)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    LCID LocaleId
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfPdoInitAssignRawDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    CONST GUID* DeviceClassGuid
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfPdoInitAllowForwardingRequestToParent)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfPdoMarkMissing)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfPdoRequestEject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
WDFEXPORT(WdfPdoGetParent)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfPdoRetrieveIdentificationDescription)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _Inout_
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfPdoRetrieveAddressDescription)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _Inout_
    PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfPdoUpdateAddressDescription)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _Inout_
    PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfPdoAddEjectionRelationsPhysicalDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PDEVICE_OBJECT PhysicalDevice
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfPdoRemoveEjectionRelationsPhysicalDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PDEVICE_OBJECT PhysicalDevice
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfPdoClearEjectionRelationsDevices)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfPdoInitAssignContainerID)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PCUNICODE_STRING ContainerID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfPdoInitAllowForwardingRequestToParent)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    );

// ----- WDFPDO -----//


// ----- WDFQUERYINTERFACE -----//

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceAddQueryInterface)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_QUERY_INTERFACE_CONFIG InterfaceConfig
    );

// ----- WDFQUERYINTERFACE -----//


// ----- WDFREGISTRY -----//

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRegistryOpenKey)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    WDFKEY ParentKey,
    _In_
    PCUNICODE_STRING KeyName,
    _In_
    ACCESS_MASK DesiredAccess,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES KeyAttributes,
    _Out_
    WDFKEY* Key
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRegistryCreateKey)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    WDFKEY ParentKey,
    _In_
    PCUNICODE_STRING KeyName,
    _In_
    ACCESS_MASK DesiredAccess,
    _In_
    ULONG CreateOptions,
    _Out_opt_
    PULONG CreateDisposition,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES KeyAttributes,
    _Out_
    WDFKEY* Key
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfRegistryClose)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
HANDLE
WDFEXPORT(WdfRegistryWdmGetHandle)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRegistryRemoveKey)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRegistryRemoveValue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRegistryQueryValue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _In_
    ULONG ValueLength,
    _Out_writes_bytes_opt_( ValueLength)
    PVOID Value,
    _Out_opt_
    PULONG ValueLengthQueried,
    _Out_opt_
    PULONG ValueType
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRegistryQueryMemory)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _In_
    _Strict_type_match_
    POOL_TYPE PoolType,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES MemoryAttributes,
    _Out_
    WDFMEMORY* Memory,
    _Out_opt_
    PULONG ValueType
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRegistryQueryMultiString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES StringsAttributes,
    _In_
    WDFCOLLECTION Collection
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRegistryQueryUnicodeString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _Out_opt_
    PUSHORT ValueByteLength,
    _Inout_opt_
    PUNICODE_STRING Value
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRegistryQueryString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _In_
    WDFSTRING String
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRegistryQueryULong)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _Out_
    PULONG Value
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRegistryAssignValue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _In_
    ULONG ValueType,
    _In_
    ULONG ValueLength,
    _In_reads_( ValueLength)
    PVOID Value
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRegistryAssignMemory)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _In_
    ULONG ValueType,
    _In_
    WDFMEMORY Memory,
    _In_opt_
    PWDFMEMORY_OFFSET MemoryOffsets
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRegistryAssignMultiString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _In_
    WDFCOLLECTION StringsCollection
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRegistryAssignUnicodeString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _In_
    PCUNICODE_STRING Value
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRegistryAssignString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _In_
    WDFSTRING String
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRegistryAssignULong)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _In_
    ULONG Value
    );

// ----- WDFREGISTRY -----//


// ----- WDFVERIFIER -----//

VOID
NTAPI
WDFEXPORT(WdfVerifierDbgBreakPoint)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals
    );

WDFAPI
VOID
WDFEXPORT(WdfVerifierKeBugCheck)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    ULONG BugCheckCode,
    _In_
    ULONG_PTR BugCheckParameter1,
    _In_
    ULONG_PTR BugCheckParameter2,
    _In_
    ULONG_PTR BugCheckParameter3,
    _In_
    ULONG_PTR BugCheckParameter4
    );

// ----- WDFVERIFIER -----//


// ----- WDFWORKITEM -----//

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfWorkItemCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDF_WORKITEM_CONFIG Config,
    _In_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFWORKITEM* WorkItem
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfWorkItemEnqueue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFWORKITEM WorkItem
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFOBJECT
WDFEXPORT(WdfWorkItemGetParentObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFWORKITEM WorkItem
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfWorkItemFlush)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFWORKITEM WorkItem
    );

// ----- WDFWORKITEM -----//


// ----- WDFSYNC ----- //
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
NTAPI
WDFEXPORT(WdfSpinLockCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES SpinLockAttributes,
    _Out_
    WDFSPINLOCK* SpinLock
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_raises_(DISPATCH_LEVEL)
WDFAPI
VOID
NTAPI
WDFEXPORT(WdfSpinLockAcquire)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    _Requires_lock_not_held_(_Curr_)
    _Acquires_lock_(_Curr_)
    _IRQL_saves_
    WDFSPINLOCK SpinLock
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(DISPATCH_LEVEL)
WDFAPI
VOID
NTAPI
WDFEXPORT(WdfSpinLockRelease)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    _Requires_lock_held_(_Curr_)
    _Releases_lock_(_Curr_)
    _IRQL_restores_
    WDFSPINLOCK SpinLock
    );

// ----- WDFSYNC ----- //

// ----- WDFREQUEST ----- //
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
NTAPI
WDFEXPORT(WdfRequestMarkCancelableEx)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    PFN_WDF_REQUEST_CANCEL EvtRequestCancel
    );

// ----- WDFREQUEST ----- //
#ifndef __GNUC__
extern WDFVERSION WdfVersion;
#endif

WDFAPI
NTSTATUS
static
NotImplemented();

static WDFVERSION WdfVersion = {
		sizeof(WDFVERSION),
		sizeof(WDFFUNCTIONS)/sizeof(PVOID),
		{
			WDFEXPORT(WdfChildListCreate),
			WDFEXPORT(WdfChildListGetDevice),
			WDFEXPORT(WdfChildListRetrievePdo),
			WDFEXPORT(WdfChildListRetrieveAddressDescription),
			WDFEXPORT(WdfChildListBeginScan),
			WDFEXPORT(WdfChildListEndScan),
			WDFEXPORT(WdfChildListBeginIteration),
			WDFEXPORT(WdfChildListRetrieveNextDevice),
			WDFEXPORT(WdfChildListEndIteration),
			WDFEXPORT(WdfChildListAddOrUpdateChildDescriptionAsPresent),
			WDFEXPORT(WdfChildListUpdateChildDescriptionAsMissing),
			WDFEXPORT(WdfChildListUpdateAllChildDescriptionsAsPresent),
			WDFEXPORT(WdfChildListRequestChildEject),
			WDFEXPORT(WdfCollectionCreate),
			WDFEXPORT(WdfCollectionGetCount),
			WDFEXPORT(WdfCollectionAdd),
			WDFEXPORT(WdfCollectionRemove),
			WDFEXPORT(WdfCollectionRemoveItem),
			WDFEXPORT(WdfCollectionGetItem),
			WDFEXPORT(WdfCollectionGetFirstItem),
			WDFEXPORT(WdfCollectionGetLastItem),
			WDFEXPORT(WdfCommonBufferCreate),
			WDFEXPORT(WdfCommonBufferGetAlignedVirtualAddress),
			WDFEXPORT(WdfCommonBufferGetAlignedLogicalAddress),
			WDFEXPORT(WdfCommonBufferGetLength),
			WDFEXPORT(WdfControlDeviceInitAllocate),
			WDFEXPORT(WdfControlDeviceInitSetShutdownNotification),
			WDFEXPORT(WdfControlFinishInitializing),
			WDFEXPORT(WdfDeviceGetDeviceState),
			WDFEXPORT(WdfDeviceSetDeviceState),
			WDFEXPORT(WdfWdmDeviceGetWdfDeviceHandle),
			WDFEXPORT(WdfDeviceWdmGetDeviceObject),
			WDFEXPORT(WdfDeviceWdmGetAttachedDevice),
			WDFEXPORT(WdfDeviceWdmGetPhysicalDevice),
			WDFEXPORT(WdfDeviceWdmDispatchPreprocessedIrp),
			WDFEXPORT(WdfDeviceAddDependentUsageDeviceObject),
			WDFEXPORT(WdfDeviceAddRemovalRelationsPhysicalDevice),
			WDFEXPORT(WdfDeviceRemoveRemovalRelationsPhysicalDevice),
			WDFEXPORT(WdfDeviceClearRemovalRelationsDevices),
			WDFEXPORT(WdfDeviceGetDriver),
			WDFEXPORT(WdfDeviceRetrieveDeviceName),
			WDFEXPORT(WdfDeviceAssignMofResourceName),
			WDFEXPORT(WdfDeviceGetIoTarget),
			WDFEXPORT(WdfDeviceGetDevicePnpState),
			WDFEXPORT(WdfDeviceGetDevicePowerState),
			WDFEXPORT(WdfDeviceGetDevicePowerPolicyState),
			WDFEXPORT(WdfDeviceAssignS0IdleSettings),
			WDFEXPORT(WdfDeviceAssignSxWakeSettings),
			WDFEXPORT(WdfDeviceOpenRegistryKey),
			WDFEXPORT(WdfDeviceSetSpecialFileSupport),
			WDFEXPORT(WdfDeviceSetCharacteristics),
			WDFEXPORT(WdfDeviceGetCharacteristics),
			WDFEXPORT(WdfDeviceGetAlignmentRequirement),
			WDFEXPORT(WdfDeviceSetAlignmentRequirement),
			WDFEXPORT(WdfDeviceInitFree),
			WDFEXPORT(WdfDeviceInitSetPnpPowerEventCallbacks),
			WDFEXPORT(WdfDeviceInitSetPowerPolicyEventCallbacks),
			WDFEXPORT(WdfDeviceInitSetPowerPolicyOwnership),
        	WDFEXPORT(WdfDeviceInitRegisterPnpStateChangeCallback),
        	WDFEXPORT(WdfDeviceInitRegisterPowerStateChangeCallback),
        	WDFEXPORT(WdfDeviceInitRegisterPowerPolicyStateChangeCallback),
	        WDFEXPORT(WdfDeviceInitSetIoType),
    	    WDFEXPORT(WdfDeviceInitSetExclusive),
	        WDFEXPORT(WdfDeviceInitSetPowerNotPageable),
        	WDFEXPORT(WdfDeviceInitSetPowerPageable),
	        WDFEXPORT(WdfDeviceInitSetPowerInrush),
        	WDFEXPORT(WdfDeviceInitSetDeviceType),
        	WDFEXPORT(WdfDeviceInitAssignName),
        	WDFEXPORT(WdfDeviceInitAssignSDDLString),
        	WDFEXPORT(WdfDeviceInitSetDeviceClass),
        	WDFEXPORT(WdfDeviceInitSetCharacteristics),
        	WDFEXPORT(WdfDeviceInitSetFileObjectConfig),
			WDFEXPORT(WdfDeviceInitSetRequestAttributes),
			WDFEXPORT(WdfDeviceInitAssignWdmIrpPreprocessCallback),
        	WDFEXPORT(WdfDeviceInitSetIoInCallerContextCallback),
			WDFEXPORT(WdfDeviceCreate),
			WDFEXPORT(WdfDeviceSetStaticStopRemove),
			WDFEXPORT(WdfDeviceCreateDeviceInterface),
			WDFEXPORT(WdfDeviceSetDeviceInterfaceState),
        	WDFEXPORT(WdfDeviceRetrieveDeviceInterfaceString),
        	WDFEXPORT(WdfDeviceCreateSymbolicLink),
        	WDFEXPORT(WdfDeviceQueryProperty),
        	WDFEXPORT(WdfDeviceAllocAndQueryProperty),
        	WDFEXPORT(WdfDeviceSetPnpCapabilities),
        	WDFEXPORT(WdfDeviceSetPowerCapabilities),
        	WDFEXPORT(WdfDeviceSetBusInformationForChildren),
        	WDFEXPORT(WdfDeviceIndicateWakeStatus),
        	WDFEXPORT(WdfDeviceSetFailed),
        	WDFEXPORT(WdfDeviceStopIdle),
        	WDFEXPORT(WdfDeviceResumeIdle),
        	WDFEXPORT(WdfDeviceGetFileObject),
        	WDFEXPORT(WdfDeviceEnqueueRequest),
			WDFEXPORT(WdfDeviceGetDefaultQueue),
			WDFEXPORT(WdfDeviceConfigureRequestDispatching),
			WDFEXPORT(WdfDmaEnablerCreate),
        	WDFEXPORT(WdfDmaEnablerGetMaximumLength),
        	WDFEXPORT(WdfDmaEnablerGetMaximumScatterGatherElements),
        	WDFEXPORT(WdfDmaEnablerSetMaximumScatterGatherElements),
        	WDFEXPORT(WdfDmaTransactionCreate),
        	WDFEXPORT(WdfDmaTransactionInitialize),
        	WDFEXPORT(WdfDmaTransactionInitializeUsingRequest),
        	WDFEXPORT(WdfDmaTransactionExecute),
        	WDFEXPORT(WdfDmaTransactionRelease),
        	WDFEXPORT(WdfDmaTransactionDmaCompleted),
        	WDFEXPORT(WdfDmaTransactionDmaCompletedWithLength),
        	WDFEXPORT(WdfDmaTransactionDmaCompletedFinal),
        	WDFEXPORT(WdfDmaTransactionGetBytesTransferred),
        	WDFEXPORT(WdfDmaTransactionSetMaximumLength),
        	WDFEXPORT(WdfDmaTransactionGetRequest),
        	WDFEXPORT(WdfDmaTransactionGetCurrentDmaTransferLength),
        	WDFEXPORT(WdfDmaTransactionGetDevice),
			WDFEXPORT(WdfDpcCreate),
            WDFEXPORT(WdfDpcEnqueue),
            WDFEXPORT(WdfDpcCancel),
            WDFEXPORT(WdfDpcGetParentObject),
            WDFEXPORT(WdfDpcWdmGetDpc),
            WDFEXPORT(WdfDriverCreate),
			WDFEXPORT(WdfDriverGetRegistryPath),
            WDFEXPORT(WdfDriverWdmGetDriverObject),
            WDFEXPORT(WdfDriverOpenParametersRegistryKey),
            WDFEXPORT(WdfWdmDriverGetWdfDriverHandle),
            WDFEXPORT(WdfDriverRegisterTraceInfo),
			WDFEXPORT(WdfDriverRetrieveVersionString),
			WDFEXPORT(WdfDriverIsVersionAvailable),
			WDFEXPORT(WdfFdoInitWdmGetPhysicalDevice),
            WDFEXPORT(WdfFdoInitOpenRegistryKey),
            WDFEXPORT(WdfFdoInitQueryProperty),
            WDFEXPORT(WdfFdoInitAllocAndQueryProperty),
            WDFEXPORT(WdfFdoInitSetEventCallbacks),
            WDFEXPORT(WdfFdoInitSetFilter),
            WDFEXPORT(WdfFdoInitSetDefaultChildListConfig),
            WDFEXPORT(WdfFdoQueryForInterface),
            WDFEXPORT(WdfFdoGetDefaultChildList),
            WDFEXPORT(WdfFdoAddStaticChild),
            WDFEXPORT(WdfFdoLockStaticChildListForIteration),
            WDFEXPORT(WdfFdoRetrieveNextStaticChild),
            WDFEXPORT(WdfFdoUnlockStaticChildListFromIteration),
			WDFEXPORT(WdfFileObjectGetFileName),
            WDFEXPORT(WdfFileObjectGetFlags),
            WDFEXPORT(WdfFileObjectGetDevice),
            WDFEXPORT(WdfFileObjectWdmGetFileObject),
			WDFEXPORT(WdfInterruptCreate),
            WDFEXPORT(WdfInterruptQueueDpcForIsr),
            WDFEXPORT(WdfInterruptSynchronize),
            WDFEXPORT(WdfInterruptAcquireLock),
            WDFEXPORT(WdfInterruptReleaseLock),
            WDFEXPORT(WdfInterruptEnable),
            WDFEXPORT(WdfInterruptDisable),
            WDFEXPORT(WdfInterruptWdmGetInterrupt),
            WDFEXPORT(WdfInterruptGetInfo),
            WDFEXPORT(WdfInterruptSetPolicy),
            WDFEXPORT(WdfInterruptGetDevice),
			WDFEXPORT(WdfIoQueueCreate),
			WDFEXPORT(WdfIoQueueGetState),
			WDFEXPORT(WdfIoQueueStart),
			WDFEXPORT(WdfIoQueueStop),
			WDFEXPORT(WdfIoQueueStopSynchronously),
			WDFEXPORT(WdfIoQueueGetDevice),
            WDFEXPORT(WdfIoQueueRetrieveNextRequest),
            WDFEXPORT(WdfIoQueueRetrieveRequestByFileObject),
            WDFEXPORT(WdfIoQueueFindRequest),
            WDFEXPORT(WdfIoQueueRetrieveFoundRequest),
            WDFEXPORT(WdfIoQueueDrainSynchronously),
            WDFEXPORT(WdfIoQueueDrain),
            WDFEXPORT(WdfIoQueuePurgeSynchronously),
            WDFEXPORT(WdfIoQueuePurge),
            WDFEXPORT(WdfIoQueueReadyNotify),
			WDFEXPORT(WdfIoTargetCreate),
            WDFEXPORT(WdfIoTargetOpen),
            WDFEXPORT(WdfIoTargetCloseForQueryRemove),
            WDFEXPORT(WdfIoTargetClose),
            WDFEXPORT(WdfIoTargetStart),
            WDFEXPORT(WdfIoTargetStop),
            WDFEXPORT(WdfIoTargetGetState),
            WDFEXPORT(WdfIoTargetGetDevice),
            WDFEXPORT(WdfIoTargetQueryTargetProperty),
            WDFEXPORT(WdfIoTargetAllocAndQueryTargetProperty),
            WDFEXPORT(WdfIoTargetQueryForInterface),
            WDFEXPORT(WdfIoTargetWdmGetTargetDeviceObject),
            WDFEXPORT(WdfIoTargetWdmGetTargetPhysicalDevice),
            WDFEXPORT(WdfIoTargetWdmGetTargetFileObject),
            WDFEXPORT(WdfIoTargetWdmGetTargetFileHandle),
            WDFEXPORT(WdfIoTargetSendReadSynchronously),
            WDFEXPORT(WdfIoTargetFormatRequestForRead),
            WDFEXPORT(WdfIoTargetSendWriteSynchronously),
            WDFEXPORT(WdfIoTargetFormatRequestForWrite),
            WDFEXPORT(WdfIoTargetSendIoctlSynchronously),
            WDFEXPORT(WdfIoTargetFormatRequestForIoctl),
            WDFEXPORT(WdfIoTargetSendInternalIoctlSynchronously),
            WDFEXPORT(WdfIoTargetFormatRequestForInternalIoctl),
            WDFEXPORT(WdfIoTargetSendInternalIoctlOthersSynchronously),
            WDFEXPORT(WdfIoTargetFormatRequestForInternalIoctlOthers),
            WDFEXPORT(WdfMemoryCreate),
            WDFEXPORT(WdfMemoryCreatePreallocated),
            WDFEXPORT(WdfMemoryGetBuffer),
            WDFEXPORT(WdfMemoryAssignBuffer),
			WDFEXPORT(WdfMemoryCopyToBuffer),
			WDFEXPORT(WdfMemoryCopyFromBuffer),
			WDFEXPORT(WdfLookasideListCreate),
            WDFEXPORT(WdfMemoryCreateFromLookaside),
            WDFEXPORT(WdfDeviceMiniportCreate),
            WDFEXPORT(WdfDriverMiniportUnload),
			WDFEXPORT(WdfObjectGetTypedContextWorker),
			WDFEXPORT(WdfObjectAllocateContext),
            WDFEXPORT(WdfObjectContextGetObject),
            WDFEXPORT(WdfObjectReferenceActual),
            WDFEXPORT(WdfObjectDereferenceActual),
            WDFEXPORT(WdfObjectCreate),
			WDFEXPORT(WdfObjectDelete),
			WDFEXPORT(WdfObjectQuery),
			WDFEXPORT(WdfPdoInitAllocate),
            WDFEXPORT(WdfPdoInitSetEventCallbacks),
            WDFEXPORT(WdfPdoInitAssignDeviceID),
            WDFEXPORT(WdfPdoInitAssignInstanceID),
            WDFEXPORT(WdfPdoInitAddHardwareID),
            WDFEXPORT(WdfPdoInitAddCompatibleID),
            WDFEXPORT(WdfPdoInitAddDeviceText),
            WDFEXPORT(WdfPdoInitSetDefaultLocale),
            WDFEXPORT(WdfPdoInitAssignRawDevice),
            WDFEXPORT(WdfPdoMarkMissing),
            WDFEXPORT(WdfPdoRequestEject),
            WDFEXPORT(WdfPdoGetParent),
            WDFEXPORT(WdfPdoRetrieveIdentificationDescription),
            WDFEXPORT(WdfPdoRetrieveAddressDescription),
            WDFEXPORT(WdfPdoUpdateAddressDescription),
            WDFEXPORT(WdfPdoAddEjectionRelationsPhysicalDevice),
            WDFEXPORT(WdfPdoRemoveEjectionRelationsPhysicalDevice),
            WDFEXPORT(WdfPdoClearEjectionRelationsDevices),
			WDFEXPORT(WdfDeviceAddQueryInterface),
			WDFEXPORT(WdfRegistryOpenKey),
            WDFEXPORT(WdfRegistryCreateKey),
            WDFEXPORT(WdfRegistryClose),
            WDFEXPORT(WdfRegistryWdmGetHandle),
            WDFEXPORT(WdfRegistryRemoveKey),
            WDFEXPORT(WdfRegistryRemoveValue),
            WDFEXPORT(WdfRegistryQueryValue),
            WDFEXPORT(WdfRegistryQueryMemory),
            WDFEXPORT(WdfRegistryQueryMultiString),
            WDFEXPORT(WdfRegistryQueryUnicodeString),
            WDFEXPORT(WdfRegistryQueryString),
            WDFEXPORT(WdfRegistryQueryULong),
            WDFEXPORT(WdfRegistryAssignValue),
            WDFEXPORT(WdfRegistryAssignMemory),
            WDFEXPORT(WdfRegistryAssignMultiString),
            WDFEXPORT(WdfRegistryAssignUnicodeString),
            WDFEXPORT(WdfRegistryAssignString),
            WDFEXPORT(WdfRegistryAssignULong),
			WDFEXPORT(WdfRequestCreate),
            WDFEXPORT(WdfRequestCreateFromIrp),
            WDFEXPORT(WdfRequestReuse),
            WDFEXPORT(WdfRequestChangeTarget),
            WDFEXPORT(WdfRequestFormatRequestUsingCurrentType),
            WDFEXPORT(WdfRequestWdmFormatUsingStackLocation),
            WDFEXPORT(WdfRequestSend),
            WDFEXPORT(WdfRequestGetStatus),
			WDFEXPORT(WdfRequestMarkCancelable),
			WDFEXPORT(WdfRequestUnmarkCancelable),
			WDFEXPORT(WdfRequestIsCanceled),
            WDFEXPORT(WdfRequestCancelSentRequest),
            WDFEXPORT(WdfRequestIsFrom32BitProcess),
            WDFEXPORT(WdfRequestSetCompletionRoutine),
            WDFEXPORT(WdfRequestGetCompletionParams),
            WDFEXPORT(WdfRequestAllocateTimer),
			WDFEXPORT(WdfRequestComplete),
			WDFEXPORT(WdfRequestCompleteWithPriorityBoost),
			WDFEXPORT(WdfRequestCompleteWithInformation),
			WDFEXPORT(WdfRequestGetParameters),
			WDFEXPORT(WdfRequestRetrieveInputMemory),
			WDFEXPORT(WdfRequestRetrieveOutputMemory),
			WDFEXPORT(WdfRequestRetrieveInputBuffer),
            WDFEXPORT(WdfRequestRetrieveOutputBuffer),
            WDFEXPORT(WdfRequestRetrieveInputWdmMdl),
            WDFEXPORT(WdfRequestRetrieveOutputWdmMdl),
            WDFEXPORT(WdfRequestRetrieveUnsafeUserInputBuffer),
            WDFEXPORT(WdfRequestRetrieveUnsafeUserOutputBuffer),
			WDFEXPORT(WdfRequestSetInformation),
			WDFEXPORT(WdfRequestGetInformation),
            WDFEXPORT(WdfRequestGetFileObject),
            WDFEXPORT(WdfRequestProbeAndLockUserBufferForRead),
            WDFEXPORT(WdfRequestProbeAndLockUserBufferForWrite),
            WDFEXPORT(WdfRequestGetRequestorMode),
            WDFEXPORT(WdfRequestForwardToIoQueue),
			WDFEXPORT(WdfRequestGetIoQueue),
			WDFEXPORT(WdfRequestRequeue),
            WDFEXPORT(WdfRequestStopAcknowledge),
            WDFEXPORT(WdfRequestWdmGetIrp),
			WDFEXPORT(WdfIoResourceRequirementsListSetSlotNumber),
            WDFEXPORT(WdfIoResourceRequirementsListSetInterfaceType),
            WDFEXPORT(WdfIoResourceRequirementsListAppendIoResList),
            WDFEXPORT(WdfIoResourceRequirementsListInsertIoResList),
            WDFEXPORT(WdfIoResourceRequirementsListGetCount),
            WDFEXPORT(WdfIoResourceRequirementsListGetIoResList),
            WDFEXPORT(WdfIoResourceRequirementsListRemove),
            WDFEXPORT(WdfIoResourceRequirementsListRemoveByIoResList),
            WDFEXPORT(WdfIoResourceListCreate),
            WDFEXPORT(WdfIoResourceListAppendDescriptor),
            WDFEXPORT(WdfIoResourceListInsertDescriptor),
            WDFEXPORT(WdfIoResourceListUpdateDescriptor),
            WDFEXPORT(WdfIoResourceListGetCount),
            WDFEXPORT(WdfIoResourceListGetDescriptor),
            WDFEXPORT(WdfIoResourceListRemove),
            WDFEXPORT(WdfIoResourceListRemoveByDescriptor),
			WDFEXPORT(WdfCmResourceListAppendDescriptor),
            WDFEXPORT(WdfCmResourceListInsertDescriptor),
            WDFEXPORT(WdfCmResourceListGetCount),
            WDFEXPORT(WdfCmResourceListGetDescriptor),
            WDFEXPORT(WdfCmResourceListRemove),
            WDFEXPORT(WdfCmResourceListRemoveByDescriptor),
			WDFEXPORT(WdfStringCreate),
			WDFEXPORT(WdfStringGetUnicodeString),
			WDFEXPORT(WdfObjectAcquireLock),
            WDFEXPORT(WdfObjectReleaseLock),
            WDFEXPORT(WdfWaitLockCreate),
            WDFEXPORT(WdfWaitLockAcquire),
            WDFEXPORT(WdfWaitLockRelease),
			WDFEXPORT(WdfSpinLockCreate),
			WDFEXPORT(WdfSpinLockAcquire),
			WDFEXPORT(WdfSpinLockRelease),
			WDFEXPORT(WdfTimerCreate),
			WDFEXPORT(WdfTimerStart),
			WDFEXPORT(WdfTimerStop),
			WDFEXPORT(WdfTimerGetParentObject),
			NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
			NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
            NotImplemented,
			WDFEXPORT(WdfVerifierDbgBreakPoint),
			WDFEXPORT(WdfVerifierKeBugCheck),
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			WDFEXPORT(WdfWorkItemCreate),
            WDFEXPORT(WdfWorkItemEnqueue),
            WDFEXPORT(WdfWorkItemGetParentObject),
            WDFEXPORT(WdfWorkItemFlush),
			WDFEXPORT(WdfCommonBufferCreateWithConfig),
			WDFEXPORT(WdfDmaEnablerGetFragmentLength),
            WDFEXPORT(WdfDmaEnablerWdmGetDmaAdapter),
			NotImplemented,
			WDFEXPORT(WdfDeviceRemoveDependentUsageDeviceObject),
            WDFEXPORT(WdfDeviceGetSystemPowerAction),
			WDFEXPORT(WdfInterruptSetExtendedPolicy),
			WDFEXPORT(WdfIoQueueAssignForwardProgressPolicy),
            WDFEXPORT(WdfPdoInitAssignContainerID),
            WDFEXPORT(WdfPdoInitAllowForwardingRequestToParent),
			WDFEXPORT(WdfRequestMarkCancelableEx),
			WDFEXPORT(WdfRequestIsReserved),
            WDFEXPORT(WdfRequestForwardToParentDeviceIoQueue)
		}
	};

WDFAPI
NTSTATUS
static
NotImplemented()
{
	DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "Not implemented. WDFFUNCTIONS Table: 0x%x\r\n", &WdfVersion);
    __debugbreak();
	return STATUS_UNSUCCESSFUL;
}

#ifdef __cplusplus
}
#endif

#endif //_FXDYNAMICS_H_
