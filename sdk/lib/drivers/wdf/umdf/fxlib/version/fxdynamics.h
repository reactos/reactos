/*++

Copyright (c) Microsoft. All rights reserved.

Module Name: FxDynamics.h

Abstract:
    Generated header for WDF APIs

Environment:
    User mode only

    Warning: manual changes to this file will be lost.
--*/

#ifndef _FXDYNAMICS_H_
#define _FXDYNAMICS_H_


typedef struct _WDFFUNCTIONS {

    PFN_WDFCOLLECTIONCREATE                                   pfnWdfCollectionCreate;
    PFN_WDFCOLLECTIONGETCOUNT                                 pfnWdfCollectionGetCount;
    PFN_WDFCOLLECTIONADD                                      pfnWdfCollectionAdd;
    PFN_WDFCOLLECTIONREMOVE                                   pfnWdfCollectionRemove;
    PFN_WDFCOLLECTIONREMOVEITEM                               pfnWdfCollectionRemoveItem;
    PFN_WDFCOLLECTIONGETITEM                                  pfnWdfCollectionGetItem;
    PFN_WDFCOLLECTIONGETFIRSTITEM                             pfnWdfCollectionGetFirstItem;
    PFN_WDFCOLLECTIONGETLASTITEM                              pfnWdfCollectionGetLastItem;
    PFN_WDFCXDEVICEINITALLOCATE                               pfnWdfCxDeviceInitAllocate;
    PFN_WDFCXDEVICEINITSETREQUESTATTRIBUTES                   pfnWdfCxDeviceInitSetRequestAttributes;
    PFN_WDFCXDEVICEINITSETFILEOBJECTCONFIG                    pfnWdfCxDeviceInitSetFileObjectConfig;
    PFN_WDFCXVERIFIERKEBUGCHECK                               pfnWdfCxVerifierKeBugCheck;
    PFN_WDFDEVICEGETDEVICESTATE                               pfnWdfDeviceGetDeviceState;
    PFN_WDFDEVICESETDEVICESTATE                               pfnWdfDeviceSetDeviceState;
    PFN_WDFDEVICEGETDRIVER                                    pfnWdfDeviceGetDriver;
    PFN_WDFDEVICEGETIOTARGET                                  pfnWdfDeviceGetIoTarget;
    PFN_WDFDEVICEASSIGNS0IDLESETTINGS                         pfnWdfDeviceAssignS0IdleSettings;
    PFN_WDFDEVICEASSIGNSXWAKESETTINGS                         pfnWdfDeviceAssignSxWakeSettings;
    PFN_WDFDEVICEOPENREGISTRYKEY                              pfnWdfDeviceOpenRegistryKey;
    PFN_WDFDEVICEINITSETPNPPOWEREVENTCALLBACKS                pfnWdfDeviceInitSetPnpPowerEventCallbacks;
    PFN_WDFDEVICEINITSETPOWERPOLICYEVENTCALLBACKS             pfnWdfDeviceInitSetPowerPolicyEventCallbacks;
    PFN_WDFDEVICEINITSETPOWERPOLICYOWNERSHIP                  pfnWdfDeviceInitSetPowerPolicyOwnership;
    PFN_WDFDEVICEINITSETIOTYPE                                pfnWdfDeviceInitSetIoType;
    PFN_WDFDEVICEINITSETFILEOBJECTCONFIG                      pfnWdfDeviceInitSetFileObjectConfig;
    PFN_WDFDEVICEINITSETREQUESTATTRIBUTES                     pfnWdfDeviceInitSetRequestAttributes;
    PFN_WDFDEVICECREATE                                       pfnWdfDeviceCreate;
    PFN_WDFDEVICESETSTATICSTOPREMOVE                          pfnWdfDeviceSetStaticStopRemove;
    PFN_WDFDEVICECREATEDEVICEINTERFACE                        pfnWdfDeviceCreateDeviceInterface;
    PFN_WDFDEVICESETDEVICEINTERFACESTATE                      pfnWdfDeviceSetDeviceInterfaceState;
    PFN_WDFDEVICERETRIEVEDEVICEINTERFACESTRING                pfnWdfDeviceRetrieveDeviceInterfaceString;
    PFN_WDFDEVICECREATESYMBOLICLINK                           pfnWdfDeviceCreateSymbolicLink;
    PFN_WDFDEVICEQUERYPROPERTY                                pfnWdfDeviceQueryProperty;
    PFN_WDFDEVICEALLOCANDQUERYPROPERTY                        pfnWdfDeviceAllocAndQueryProperty;
    PFN_WDFDEVICESETPNPCAPABILITIES                           pfnWdfDeviceSetPnpCapabilities;
    PFN_WDFDEVICESETPOWERCAPABILITIES                         pfnWdfDeviceSetPowerCapabilities;
    PFN_WDFDEVICESETFAILED                                    pfnWdfDeviceSetFailed;
    PFN_WDFDEVICESTOPIDLENOTRACK                              pfnWdfDeviceStopIdleNoTrack;
    PFN_WDFDEVICERESUMEIDLENOTRACK                            pfnWdfDeviceResumeIdleNoTrack;
    PFN_WDFDEVICEGETFILEOBJECT                                pfnWdfDeviceGetFileObject;
    PFN_WDFDEVICEGETDEFAULTQUEUE                              pfnWdfDeviceGetDefaultQueue;
    PFN_WDFDEVICECONFIGUREREQUESTDISPATCHING                  pfnWdfDeviceConfigureRequestDispatching;
    PFN_WDFDEVICEGETSYSTEMPOWERACTION                         pfnWdfDeviceGetSystemPowerAction;
    PFN_WDFDEVICEINITSETRELEASEHARDWAREORDERONFAILURE         pfnWdfDeviceInitSetReleaseHardwareOrderOnFailure;
    PFN_WDFDEVICEINITSETIOTYPEEX                              pfnWdfDeviceInitSetIoTypeEx;
    PFN_WDFDEVICEPOSTEVENT                                    pfnWdfDevicePostEvent;
    PFN_WDFDEVICEMAPIOSPACE                                   pfnWdfDeviceMapIoSpace;
    PFN_WDFDEVICEUNMAPIOSPACE                                 pfnWdfDeviceUnmapIoSpace;
    PFN_WDFDEVICEGETHARDWAREREGISTERMAPPEDADDRESS             pfnWdfDeviceGetHardwareRegisterMappedAddress;
    PFN_WDFDEVICEREADFROMHARDWARE                             pfnWdfDeviceReadFromHardware;
    PFN_WDFDEVICEWRITETOHARDWARE                              pfnWdfDeviceWriteToHardware;
    PFN_WDFDEVICEASSIGNINTERFACEPROPERTY                      pfnWdfDeviceAssignInterfaceProperty;
    PFN_WDFDEVICEALLOCANDQUERYINTERFACEPROPERTY               pfnWdfDeviceAllocAndQueryInterfaceProperty;
    PFN_WDFDEVICEQUERYINTERFACEPROPERTY                       pfnWdfDeviceQueryInterfaceProperty;
    PFN_WDFDEVICEGETDEVICESTACKIOTYPE                         pfnWdfDeviceGetDeviceStackIoType;
    PFN_WDFDEVICEQUERYPROPERTYEX                              pfnWdfDeviceQueryPropertyEx;
    PFN_WDFDEVICEALLOCANDQUERYPROPERTYEX                      pfnWdfDeviceAllocAndQueryPropertyEx;
    PFN_WDFDEVICEASSIGNPROPERTY                               pfnWdfDeviceAssignProperty;
    PFN_WDFDRIVERCREATE                                       pfnWdfDriverCreate;
    PFN_WDFDRIVERGETREGISTRYPATH                              pfnWdfDriverGetRegistryPath;
    PFN_WDFDRIVEROPENPARAMETERSREGISTRYKEY                    pfnWdfDriverOpenParametersRegistryKey;
    PFN_WDFDRIVERRETRIEVEVERSIONSTRING                        pfnWdfDriverRetrieveVersionString;
    PFN_WDFDRIVERISVERSIONAVAILABLE                           pfnWdfDriverIsVersionAvailable;
    PFN_WDFFDOINITOPENREGISTRYKEY                             pfnWdfFdoInitOpenRegistryKey;
    PFN_WDFFDOINITQUERYPROPERTY                               pfnWdfFdoInitQueryProperty;
    PFN_WDFFDOINITALLOCANDQUERYPROPERTY                       pfnWdfFdoInitAllocAndQueryProperty;
    PFN_WDFFDOINITQUERYPROPERTYEX                             pfnWdfFdoInitQueryPropertyEx;
    PFN_WDFFDOINITALLOCANDQUERYPROPERTYEX                     pfnWdfFdoInitAllocAndQueryPropertyEx;
    PFN_WDFFDOINITSETFILTER                                   pfnWdfFdoInitSetFilter;
    PFN_WDFFILEOBJECTGETFILENAME                              pfnWdfFileObjectGetFileName;
    PFN_WDFFILEOBJECTGETDEVICE                                pfnWdfFileObjectGetDevice;
    PFN_WDFFILEOBJECTGETINITIATORPROCESSID                    pfnWdfFileObjectGetInitiatorProcessId;
    PFN_WDFFILEOBJECTGETRELATEDFILEOBJECT                     pfnWdfFileObjectGetRelatedFileObject;
    PFN_WDFINTERRUPTCREATE                                    pfnWdfInterruptCreate;
    PFN_WDFINTERRUPTQUEUEDPCFORISR                            pfnWdfInterruptQueueDpcForIsr;
    PFN_WDFINTERRUPTQUEUEWORKITEMFORISR                       pfnWdfInterruptQueueWorkItemForIsr;
    PFN_WDFINTERRUPTSYNCHRONIZE                               pfnWdfInterruptSynchronize;
    PFN_WDFINTERRUPTACQUIRELOCK                               pfnWdfInterruptAcquireLock;
    PFN_WDFINTERRUPTRELEASELOCK                               pfnWdfInterruptReleaseLock;
    PFN_WDFINTERRUPTENABLE                                    pfnWdfInterruptEnable;
    PFN_WDFINTERRUPTDISABLE                                   pfnWdfInterruptDisable;
    PFN_WDFINTERRUPTGETINFO                                   pfnWdfInterruptGetInfo;
    PFN_WDFINTERRUPTSETPOLICY                                 pfnWdfInterruptSetPolicy;
    PFN_WDFINTERRUPTSETEXTENDEDPOLICY                         pfnWdfInterruptSetExtendedPolicy;
    PFN_WDFINTERRUPTGETDEVICE                                 pfnWdfInterruptGetDevice;
    PFN_WDFINTERRUPTTRYTOACQUIRELOCK                          pfnWdfInterruptTryToAcquireLock;
    PFN_WDFIOQUEUECREATE                                      pfnWdfIoQueueCreate;
    PFN_WDFIOQUEUEGETSTATE                                    pfnWdfIoQueueGetState;
    PFN_WDFIOQUEUESTART                                       pfnWdfIoQueueStart;
    PFN_WDFIOQUEUESTOP                                        pfnWdfIoQueueStop;
    PFN_WDFIOQUEUESTOPSYNCHRONOUSLY                           pfnWdfIoQueueStopSynchronously;
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
    PFN_WDFIOQUEUESTOPANDPURGE                                pfnWdfIoQueueStopAndPurge;
    PFN_WDFIOQUEUESTOPANDPURGESYNCHRONOUSLY                   pfnWdfIoQueueStopAndPurgeSynchronously;
    PFN_WDFIOTARGETCREATE                                     pfnWdfIoTargetCreate;
    PFN_WDFIOTARGETOPEN                                       pfnWdfIoTargetOpen;
    PFN_WDFIOTARGETCLOSEFORQUERYREMOVE                        pfnWdfIoTargetCloseForQueryRemove;
    PFN_WDFIOTARGETCLOSE                                      pfnWdfIoTargetClose;
    PFN_WDFIOTARGETSTART                                      pfnWdfIoTargetStart;
    PFN_WDFIOTARGETSTOP                                       pfnWdfIoTargetStop;
    PFN_WDFIOTARGETPURGE                                      pfnWdfIoTargetPurge;
    PFN_WDFIOTARGETGETSTATE                                   pfnWdfIoTargetGetState;
    PFN_WDFIOTARGETGETDEVICE                                  pfnWdfIoTargetGetDevice;
    PFN_WDFIOTARGETSENDREADSYNCHRONOUSLY                      pfnWdfIoTargetSendReadSynchronously;
    PFN_WDFIOTARGETFORMATREQUESTFORREAD                       pfnWdfIoTargetFormatRequestForRead;
    PFN_WDFIOTARGETSENDWRITESYNCHRONOUSLY                     pfnWdfIoTargetSendWriteSynchronously;
    PFN_WDFIOTARGETFORMATREQUESTFORWRITE                      pfnWdfIoTargetFormatRequestForWrite;
    PFN_WDFIOTARGETSENDIOCTLSYNCHRONOUSLY                     pfnWdfIoTargetSendIoctlSynchronously;
    PFN_WDFIOTARGETFORMATREQUESTFORIOCTL                      pfnWdfIoTargetFormatRequestForIoctl;
    PFN_WDFMEMORYCREATE                                       pfnWdfMemoryCreate;
    PFN_WDFMEMORYCREATEPREALLOCATED                           pfnWdfMemoryCreatePreallocated;
    PFN_WDFMEMORYGETBUFFER                                    pfnWdfMemoryGetBuffer;
    PFN_WDFMEMORYASSIGNBUFFER                                 pfnWdfMemoryAssignBuffer;
    PFN_WDFMEMORYCOPYTOBUFFER                                 pfnWdfMemoryCopyToBuffer;
    PFN_WDFMEMORYCOPYFROMBUFFER                               pfnWdfMemoryCopyFromBuffer;
    PFN_WDFOBJECTGETTYPEDCONTEXTWORKER                        pfnWdfObjectGetTypedContextWorker;
    PFN_WDFOBJECTALLOCATECONTEXT                              pfnWdfObjectAllocateContext;
    PFN_WDFOBJECTCONTEXTGETOBJECT                             pfnWdfObjectContextGetObject;
    PFN_WDFOBJECTREFERENCEACTUAL                              pfnWdfObjectReferenceActual;
    PFN_WDFOBJECTDEREFERENCEACTUAL                            pfnWdfObjectDereferenceActual;
    PFN_WDFOBJECTCREATE                                       pfnWdfObjectCreate;
    PFN_WDFOBJECTDELETE                                       pfnWdfObjectDelete;
    PFN_WDFOBJECTQUERY                                        pfnWdfObjectQuery;
    PFN_WDFREGISTRYOPENKEY                                    pfnWdfRegistryOpenKey;
    PFN_WDFREGISTRYCREATEKEY                                  pfnWdfRegistryCreateKey;
    PFN_WDFREGISTRYCLOSE                                      pfnWdfRegistryClose;
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
    PFN_WDFREQUESTREUSE                                       pfnWdfRequestReuse;
    PFN_WDFREQUESTCHANGETARGET                                pfnWdfRequestChangeTarget;
    PFN_WDFREQUESTFORMATREQUESTUSINGCURRENTTYPE               pfnWdfRequestFormatRequestUsingCurrentType;
    PFN_WDFREQUESTSEND                                        pfnWdfRequestSend;
    PFN_WDFREQUESTGETSTATUS                                   pfnWdfRequestGetStatus;
    PFN_WDFREQUESTMARKCANCELABLE                              pfnWdfRequestMarkCancelable;
    PFN_WDFREQUESTMARKCANCELABLEEX                            pfnWdfRequestMarkCancelableEx;
    PFN_WDFREQUESTUNMARKCANCELABLE                            pfnWdfRequestUnmarkCancelable;
    PFN_WDFREQUESTISCANCELED                                  pfnWdfRequestIsCanceled;
    PFN_WDFREQUESTCANCELSENTREQUEST                           pfnWdfRequestCancelSentRequest;
    PFN_WDFREQUESTISFROM32BITPROCESS                          pfnWdfRequestIsFrom32BitProcess;
    PFN_WDFREQUESTSETCOMPLETIONROUTINE                        pfnWdfRequestSetCompletionRoutine;
    PFN_WDFREQUESTGETCOMPLETIONPARAMS                         pfnWdfRequestGetCompletionParams;
    PFN_WDFREQUESTALLOCATETIMER                               pfnWdfRequestAllocateTimer;
    PFN_WDFREQUESTCOMPLETE                                    pfnWdfRequestComplete;
    PFN_WDFREQUESTCOMPLETEWITHINFORMATION                     pfnWdfRequestCompleteWithInformation;
    PFN_WDFREQUESTGETPARAMETERS                               pfnWdfRequestGetParameters;
    PFN_WDFREQUESTRETRIEVEINPUTMEMORY                         pfnWdfRequestRetrieveInputMemory;
    PFN_WDFREQUESTRETRIEVEOUTPUTMEMORY                        pfnWdfRequestRetrieveOutputMemory;
    PFN_WDFREQUESTRETRIEVEINPUTBUFFER                         pfnWdfRequestRetrieveInputBuffer;
    PFN_WDFREQUESTRETRIEVEOUTPUTBUFFER                        pfnWdfRequestRetrieveOutputBuffer;
    PFN_WDFREQUESTSETINFORMATION                              pfnWdfRequestSetInformation;
    PFN_WDFREQUESTGETINFORMATION                              pfnWdfRequestGetInformation;
    PFN_WDFREQUESTGETFILEOBJECT                               pfnWdfRequestGetFileObject;
    PFN_WDFREQUESTGETREQUESTORMODE                            pfnWdfRequestGetRequestorMode;
    PFN_WDFREQUESTFORWARDTOIOQUEUE                            pfnWdfRequestForwardToIoQueue;
    PFN_WDFREQUESTGETIOQUEUE                                  pfnWdfRequestGetIoQueue;
    PFN_WDFREQUESTREQUEUE                                     pfnWdfRequestRequeue;
    PFN_WDFREQUESTSTOPACKNOWLEDGE                             pfnWdfRequestStopAcknowledge;
    PFN_WDFREQUESTIMPERSONATE                                 pfnWdfRequestImpersonate;
    PFN_WDFREQUESTGETREQUESTORPROCESSID                       pfnWdfRequestGetRequestorProcessId;
    PFN_WDFREQUESTISFROMUSERMODEDRIVER                        pfnWdfRequestIsFromUserModeDriver;
    PFN_WDFREQUESTSETUSERMODEDRIVERINITIATEDIO                pfnWdfRequestSetUserModeDriverInitiatedIo;
    PFN_WDFREQUESTGETUSERMODEDRIVERINITIATEDIO                pfnWdfRequestGetUserModeDriverInitiatedIo;
    PFN_WDFREQUESTSETACTIVITYID                               pfnWdfRequestSetActivityId;
    PFN_WDFREQUESTRETRIEVEACTIVITYID                          pfnWdfRequestRetrieveActivityId;
    PFN_WDFREQUESTGETEFFECTIVEIOTYPE                          pfnWdfRequestGetEffectiveIoType;
    PFN_WDFCMRESOURCELISTGETCOUNT                             pfnWdfCmResourceListGetCount;
    PFN_WDFCMRESOURCELISTGETDESCRIPTOR                        pfnWdfCmResourceListGetDescriptor;
    PFN_WDFSTRINGCREATE                                       pfnWdfStringCreate;
    PFN_WDFSTRINGGETUNICODESTRING                             pfnWdfStringGetUnicodeString;
    PFN_WDFOBJECTACQUIRELOCK                                  pfnWdfObjectAcquireLock;
    PFN_WDFOBJECTRELEASELOCK                                  pfnWdfObjectReleaseLock;
    PFN_WDFWAITLOCKCREATE                                     pfnWdfWaitLockCreate;
    PFN_WDFWAITLOCKACQUIRE                                    pfnWdfWaitLockAcquire;
    PFN_WDFWAITLOCKRELEASE                                    pfnWdfWaitLockRelease;
    PFN_WDFSPINLOCKCREATE                                     pfnWdfSpinLockCreate;
    PFN_WDFSPINLOCKACQUIRE                                    pfnWdfSpinLockAcquire;
    PFN_WDFSPINLOCKRELEASE                                    pfnWdfSpinLockRelease;
    PFN_WDFTIMERCREATE                                        pfnWdfTimerCreate;
    PFN_WDFTIMERSTART                                         pfnWdfTimerStart;
    PFN_WDFTIMERSTOP                                          pfnWdfTimerStop;
    PFN_WDFTIMERGETPARENTOBJECT                               pfnWdfTimerGetParentObject;
    PFN_WDFUSBTARGETDEVICECREATE                              pfnWdfUsbTargetDeviceCreate;
    PFN_WDFUSBTARGETDEVICECREATEWITHPARAMETERS                pfnWdfUsbTargetDeviceCreateWithParameters;
    PFN_WDFUSBTARGETDEVICERETRIEVEINFORMATION                 pfnWdfUsbTargetDeviceRetrieveInformation;
    PFN_WDFUSBTARGETDEVICEGETDEVICEDESCRIPTOR                 pfnWdfUsbTargetDeviceGetDeviceDescriptor;
    PFN_WDFUSBTARGETDEVICERETRIEVECONFIGDESCRIPTOR            pfnWdfUsbTargetDeviceRetrieveConfigDescriptor;
    PFN_WDFUSBTARGETDEVICEQUERYSTRING                         pfnWdfUsbTargetDeviceQueryString;
    PFN_WDFUSBTARGETDEVICEALLOCANDQUERYSTRING                 pfnWdfUsbTargetDeviceAllocAndQueryString;
    PFN_WDFUSBTARGETDEVICEFORMATREQUESTFORSTRING              pfnWdfUsbTargetDeviceFormatRequestForString;
    PFN_WDFUSBTARGETDEVICEGETNUMINTERFACES                    pfnWdfUsbTargetDeviceGetNumInterfaces;
    PFN_WDFUSBTARGETDEVICESELECTCONFIG                        pfnWdfUsbTargetDeviceSelectConfig;
    PFN_WDFUSBTARGETDEVICESENDCONTROLTRANSFERSYNCHRONOUSLY    pfnWdfUsbTargetDeviceSendControlTransferSynchronously;
    PFN_WDFUSBTARGETDEVICEFORMATREQUESTFORCONTROLTRANSFER     pfnWdfUsbTargetDeviceFormatRequestForControlTransfer;
    PFN_WDFUSBTARGETDEVICERESETPORTSYNCHRONOUSLY              pfnWdfUsbTargetDeviceResetPortSynchronously;
    PFN_WDFUSBTARGETDEVICEQUERYUSBCAPABILITY                  pfnWdfUsbTargetDeviceQueryUsbCapability;
    PFN_WDFUSBTARGETPIPEGETINFORMATION                        pfnWdfUsbTargetPipeGetInformation;
    PFN_WDFUSBTARGETPIPEISINENDPOINT                          pfnWdfUsbTargetPipeIsInEndpoint;
    PFN_WDFUSBTARGETPIPEISOUTENDPOINT                         pfnWdfUsbTargetPipeIsOutEndpoint;
    PFN_WDFUSBTARGETPIPEGETTYPE                               pfnWdfUsbTargetPipeGetType;
    PFN_WDFUSBTARGETPIPESETNOMAXIMUMPACKETSIZECHECK           pfnWdfUsbTargetPipeSetNoMaximumPacketSizeCheck;
    PFN_WDFUSBTARGETPIPEWRITESYNCHRONOUSLY                    pfnWdfUsbTargetPipeWriteSynchronously;
    PFN_WDFUSBTARGETPIPEFORMATREQUESTFORWRITE                 pfnWdfUsbTargetPipeFormatRequestForWrite;
    PFN_WDFUSBTARGETPIPEREADSYNCHRONOUSLY                     pfnWdfUsbTargetPipeReadSynchronously;
    PFN_WDFUSBTARGETPIPEFORMATREQUESTFORREAD                  pfnWdfUsbTargetPipeFormatRequestForRead;
    PFN_WDFUSBTARGETPIPECONFIGCONTINUOUSREADER                pfnWdfUsbTargetPipeConfigContinuousReader;
    PFN_WDFUSBTARGETPIPEABORTSYNCHRONOUSLY                    pfnWdfUsbTargetPipeAbortSynchronously;
    PFN_WDFUSBTARGETPIPEFORMATREQUESTFORABORT                 pfnWdfUsbTargetPipeFormatRequestForAbort;
    PFN_WDFUSBTARGETPIPERESETSYNCHRONOUSLY                    pfnWdfUsbTargetPipeResetSynchronously;
    PFN_WDFUSBTARGETPIPEFORMATREQUESTFORRESET                 pfnWdfUsbTargetPipeFormatRequestForReset;
    PFN_WDFUSBINTERFACEGETINTERFACENUMBER                     pfnWdfUsbInterfaceGetInterfaceNumber;
    PFN_WDFUSBINTERFACEGETNUMENDPOINTS                        pfnWdfUsbInterfaceGetNumEndpoints;
    PFN_WDFUSBINTERFACEGETDESCRIPTOR                          pfnWdfUsbInterfaceGetDescriptor;
    PFN_WDFUSBINTERFACEGETNUMSETTINGS                         pfnWdfUsbInterfaceGetNumSettings;
    PFN_WDFUSBINTERFACESELECTSETTING                          pfnWdfUsbInterfaceSelectSetting;
    PFN_WDFUSBINTERFACEGETENDPOINTINFORMATION                 pfnWdfUsbInterfaceGetEndpointInformation;
    PFN_WDFUSBTARGETDEVICEGETINTERFACE                        pfnWdfUsbTargetDeviceGetInterface;
    PFN_WDFUSBINTERFACEGETCONFIGUREDSETTINGINDEX              pfnWdfUsbInterfaceGetConfiguredSettingIndex;
    PFN_WDFUSBINTERFACEGETNUMCONFIGUREDPIPES                  pfnWdfUsbInterfaceGetNumConfiguredPipes;
    PFN_WDFUSBINTERFACEGETCONFIGUREDPIPE                      pfnWdfUsbInterfaceGetConfiguredPipe;
    PFN_WDFVERIFIERDBGBREAKPOINT                              pfnWdfVerifierDbgBreakPoint;
    PFN_WDFVERIFIERKEBUGCHECK                                 pfnWdfVerifierKeBugCheck;
    PFN_WDFGETTRIAGEINFO                                      pfnWdfGetTriageInfo;
    PFN_WDFWORKITEMCREATE                                     pfnWdfWorkItemCreate;
    PFN_WDFWORKITEMENQUEUE                                    pfnWdfWorkItemEnqueue;
    PFN_WDFWORKITEMGETPARENTOBJECT                            pfnWdfWorkItemGetParentObject;
    PFN_WDFWORKITEMFLUSH                                      pfnWdfWorkItemFlush;
    PFN_WDFREGISTRYWDMGETHANDLE                               pfnWdfRegistryWdmGetHandle;
    PFN_WDFDEVICESTOPIDLEACTUAL                               pfnWdfDeviceStopIdleActual;
    PFN_WDFDEVICERESUMEIDLEACTUAL                             pfnWdfDeviceResumeIdleActual;
    PFN_WDFDEVICEINITENABLEHIDINTERFACE                       pfnWdfDeviceInitEnableHidInterface;
    PFN_WDFDEVICEHIDNOTIFYPRESENCE                            pfnWdfDeviceHidNotifyPresence;
    PFN_WDFDEVICEGETSELFIOTARGET                              pfnWdfDeviceGetSelfIoTarget;
    PFN_WDFDEVICEINITALLOWSELFIOTARGET                        pfnWdfDeviceInitAllowSelfIoTarget;
    PFN_WDFIOTARGETSELFASSIGNDEFAULTIOQUEUE                   pfnWdfIoTargetSelfAssignDefaultIoQueue;
    PFN_WDFDEVICEOPENDEVICEMAPKEY                             pfnWdfDeviceOpenDevicemapKey;
    PFN_WDFIOTARGETWDMGETTARGETFILEHANDLE                     pfnWdfIoTargetWdmGetTargetFileHandle;
    PFN_WDFDEVICEWDMDISPATCHIRP                               pfnWdfDeviceWdmDispatchIrp;
    PFN_WDFDEVICEWDMDISPATCHIRPTOIOQUEUE                      pfnWdfDeviceWdmDispatchIrpToIoQueue;
    PFN_WDFDEVICECONFIGUREWDMIRPDISPATCHCALLBACK              pfnWdfDeviceConfigureWdmIrpDispatchCallback;

} WDFFUNCTIONS, *PWDFFUNCTIONS;


typedef struct _WDFVERSION {

    ULONG         Size;
    ULONG         FuncCount;
    WDFFUNCTIONS  Functions;

} WDFVERSION, *PWDFVERSION;


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

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
PWDFCXDEVICE_INIT
WDFEXPORT(WdfCxDeviceInitAllocate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfCxDeviceInitSetRequestAttributes)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFCXDEVICE_INIT CxDeviceInit,
    _In_
    PWDF_OBJECT_ATTRIBUTES RequestAttributes
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfCxDeviceInitSetFileObjectConfig)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFCXDEVICE_INIT CxDeviceInit,
    _In_
    PWDFCX_FILEOBJECT_CONFIG CxFileObjectConfig,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES FileObjectAttributes
    );

WDFAPI
VOID
WDFEXPORT(WdfCxVerifierKeBugCheck)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    WDFOBJECT Object,
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

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceWdmDispatchIrp)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PIRP Irp,
    _In_
    WDFCONTEXT DispatchContext
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceWdmDispatchIrpToIoQueue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PIRP Irp,
    _In_
    WDFQUEUE Queue,
    _In_
    ULONG Flags
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDRIVER
WDFEXPORT(WdfDeviceGetDriver)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFIOTARGET
WDFEXPORT(WdfDeviceGetIoTarget)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
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

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
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

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
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

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceOpenDevicemapKey)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PCUNICODE_STRING KeyName,
    _In_
    ACCESS_MASK DesiredAccess,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES KeyAttributes,
    _Out_
    WDFKEY* Key
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceInitSetPnpPowerEventCallbacks)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_PNPPOWER_EVENT_CALLBACKS PnpPowerEventCallbacks
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

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceInitSetRequestAttributes)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_OBJECT_ATTRIBUTES RequestAttributes
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _Inout_
    PWDFDEVICE_INIT* DeviceInit,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES DeviceAttributes,
    _Out_
    WDFDEVICE* Device
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

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceCreateDeviceInterface)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    CONST GUID* InterfaceClassGUID,
    _In_opt_
    PCUNICODE_STRING ReferenceString
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
WDFEXPORT(WdfDeviceSetFailed)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    WDF_DEVICE_FAILED_ACTION FailedAction
    );

_Must_inspect_result_
_When_(WaitForD0 == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(WaitForD0 != 0, _IRQL_requires_max_(PASSIVE_LEVEL))
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceStopIdleNoTrack)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    BOOLEAN WaitForD0
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceResumeIdleNoTrack)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_Must_inspect_result_
_When_(WaitForD0 == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(WaitForD0 != 0, _IRQL_requires_max_(PASSIVE_LEVEL))
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceStopIdleActual)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    BOOLEAN WaitForD0,
    _In_opt_
    PVOID Tag,
    _In_
    LONG Line,
    _In_z_
    PCHAR File
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceResumeIdleActual)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_opt_
    PVOID Tag,
    _In_
    LONG Line,
    _In_z_
    PCHAR File
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

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFQUEUE
WDFEXPORT(WdfDeviceGetDefaultQueue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
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

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceConfigureWdmIrpDispatchCallback)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_opt_
    WDFDRIVER Driver,
    _In_
    UCHAR MajorFunction,
    _In_
    PFN_WDFDEVICE_WDM_IRP_DISPATCH EvtDeviceWdmIrpDisptach,
    _In_opt_
    WDFCONTEXT DriverContext
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

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceInitSetReleaseHardwareOrderOnFailure)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    WDF_RELEASE_HARDWARE_ORDER_ON_FAILURE ReleaseHardwareOrderOnFailure
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceInitSetIoTypeEx)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_IO_TYPE_CONFIG IoTypeConfig
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDevicePostEvent)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    REFGUID EventGuid,
    _In_
    WDF_EVENT_TYPE WdfEventType,
    _In_reads_bytes_(DataSizeCb)
    BYTE* Data,
    _In_
    ULONG DataSizeCb
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceMapIoSpace)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PHYSICAL_ADDRESS PhysicalAddress,
    _In_
    SIZE_T NumberOfBytes,
    _In_
    MEMORY_CACHING_TYPE CacheType,
    _Out_
    PVOID* PseudoBaseAddress
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceUnmapIoSpace)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PVOID PseudoBaseAddress,
    _In_
    SIZE_T NumberOfBytes
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
PVOID
WDFEXPORT(WdfDeviceGetHardwareRegisterMappedAddress)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PVOID PseudoBaseAddress
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
SIZE_T
WDFEXPORT(WdfDeviceReadFromHardware)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    WDF_DEVICE_HWACCESS_TARGET_TYPE Type,
    _In_
    WDF_DEVICE_HWACCESS_TARGET_SIZE Size,
    _In_
    PVOID TargetAddress,
    _Out_writes_all_opt_(Count)
    PVOID Buffer,
    _In_opt_
    ULONG Count
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceWriteToHardware)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    WDF_DEVICE_HWACCESS_TARGET_TYPE Type,
    _In_
    WDF_DEVICE_HWACCESS_TARGET_SIZE Size,
    _In_
    PVOID TargetAddress,
    _In_
    SIZE_T Value,
    _In_reads_opt_(Count)
    PVOID Buffer,
    _In_opt_
    ULONG Count
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceAssignInterfaceProperty)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_INTERFACE_PROPERTY_DATA PropertyData,
    _In_
    DEVPROPTYPE Type,
    _In_
    ULONG BufferLength,
    _In_reads_bytes_opt_(BufferLength)
    PVOID PropertyBuffer
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceAllocAndQueryInterfaceProperty)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_INTERFACE_PROPERTY_DATA PropertyData,
    _In_
    POOL_TYPE PoolType,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES PropertyMemoryAttributes,
    _Out_
    WDFMEMORY* PropertyMemory,
    _Out_
    PDEVPROPTYPE Type
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceQueryInterfaceProperty)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_INTERFACE_PROPERTY_DATA PropertyData,
    _In_
    ULONG BufferLength,
    _Out_writes_bytes_opt_(BufferLength)
    PVOID PropertyBuffer,
    _Out_
    PULONG ResultLength,
    _Out_
    PDEVPROPTYPE Type
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceGetDeviceStackIoType)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _Out_
    WDF_DEVICE_IO_TYPE* ReadWriteIoType,
    _Out_
    WDF_DEVICE_IO_TYPE* IoControlIoType
    );

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceQueryPropertyEx)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_PROPERTY_DATA DeviceProperty,
    _In_
    ULONG BufferLength,
    _Out_
    PVOID PropertyBuffer,
    _Out_
    PULONG RequiredSize,
    _Out_
    PDEVPROPTYPE Type
    );

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceAllocAndQueryPropertyEx)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_PROPERTY_DATA DeviceProperty,
    _In_
    _Strict_type_match_
    POOL_TYPE PoolType,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES PropertyMemoryAttributes,
    _Out_
    WDFMEMORY* PropertyMemory,
    _Out_
    PDEVPROPTYPE Type
    );

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceAssignProperty)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_PROPERTY_DATA DeviceProperty,
    _In_
    DEVPROPTYPE Type,
    _In_
    ULONG Size,
    _In_opt_
    PVOID Data
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFIOTARGET
WDFEXPORT(WdfDeviceGetSelfIoTarget)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceInitAllowSelfIoTarget)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
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

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
PWSTR
WDFEXPORT(WdfDriverGetRegistryPath)(
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

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDriverRetrieveVersionString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDRIVER Driver,
    _In_
    WDFSTRING String
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
BOOLEAN
WDFEXPORT(WdfDriverIsVersionAvailable)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDRIVER Driver,
    _In_
    PWDF_DRIVER_VERSION_AVAILABLE_PARAMS VersionAvailableParams
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

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfFdoInitQueryPropertyEx)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_DEVICE_PROPERTY_DATA DeviceProperty,
    _In_
    ULONG BufferLength,
    _Out_
    PVOID PropertyBuffer,
    _Out_
    PULONG ResultLength,
    _Out_
    PDEVPROPTYPE Type
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfFdoInitAllocAndQueryPropertyEx)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_DEVICE_PROPERTY_DATA DeviceProperty,
    _In_
    _Strict_type_match_
    POOL_TYPE PoolType,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES PropertyMemoryAttributes,
    _Out_
    WDFMEMORY* PropertyMemory,
    _Out_
    PDEVPROPTYPE Type
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
PUNICODE_STRING
WDFEXPORT(WdfFileObjectGetFileName)(
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

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
ULONG
WDFEXPORT(WdfFileObjectGetInitiatorProcessId)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFFILEOBJECT FileObject
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
WDFFILEOBJECT
WDFEXPORT(WdfFileObjectGetRelatedFileObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFFILEOBJECT FileObject
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfDeviceInitEnableHidInterface)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceHidNotifyPresence)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    BOOLEAN IsPresent
    );

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

WDFAPI
BOOLEAN
WDFEXPORT(WdfInterruptQueueWorkItemForIsr)(
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

WDFAPI
WDFDEVICE
WDFEXPORT(WdfInterruptGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    );

_Must_inspect_result_
_Post_satisfies_(return == 1 || return == 0)
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
BOOLEAN
WDFEXPORT(WdfInterruptTryToAcquireLock)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    _Requires_lock_not_held_(_Curr_)
    _When_(return!=0, _Acquires_lock_(_Curr_))
    WDFINTERRUPT Interrupt
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoQueueCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_IO_QUEUE_CONFIG Config,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES QueueAttributes,
    _Out_opt_
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

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfIoQueueStart)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
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

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfIoQueueStopSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue
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

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfIoQueueStopAndPurge)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue,
    _When_(Context != 0, _In_)
    _When_(Context == 0, _In_opt_)
    PFN_WDF_IO_QUEUE_STATE StopAndPurgeComplete,
    _When_(StopAndPurgeComplete != 0, _In_)
    _When_(StopAndPurgeComplete == 0, _In_opt_)
    WDFCONTEXT Context
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfIoQueueStopAndPurgeSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue
    );

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

_When_(Action == 2, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(Action == 0 || Action == 1, _IRQL_requires_max_(PASSIVE_LEVEL))
WDFAPI
VOID
WDFEXPORT(WdfIoTargetPurge)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    _Strict_type_match_
    WDF_IO_TARGET_PURGE_IO_ACTION Action
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

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfIoTargetSelfAssignDefaultIoQueue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    WDFQUEUE Queue
    );

_Must_inspect_result_
_When_(PoolType == 1 || PoolType == 257, _IRQL_requires_max_(APC_LEVEL))
_When_(PoolType == 0 || PoolType == 256, _IRQL_requires_max_(DISPATCH_LEVEL))
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
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfMemoryCopyToBuffer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFMEMORY SourceMemory,
    _In_
    size_t SourceOffset,
    _Out_writes_bytes_( NumBytesToCopyTo )
    PVOID Buffer,
    _In_
    _When_(NumBytesToCopyTo == 0, __drv_reportError(NumBytesToCopyTo cannot be zero))
    size_t NumBytesToCopyTo
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfMemoryCopyFromBuffer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFMEMORY DestinationMemory,
    _In_
    size_t DestinationOffset,
    _In_
    PVOID Buffer,
    _In_
    _When_(NumBytesToCopyFrom == 0, __drv_reportError(NumBytesToCopyFrom cannot be zero))
    size_t NumBytesToCopyFrom
    );

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

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfObjectDelete)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFOBJECT Object
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

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfRequestMarkCancelable)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    PFN_WDF_REQUEST_CANCEL EvtRequestCancel
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestMarkCancelableEx)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    PFN_WDF_REQUEST_CANCEL EvtRequestCancel
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestUnmarkCancelable)(
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
WDFEXPORT(WdfRequestComplete)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    NTSTATUS Status
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfRequestCompleteWithInformation)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    NTSTATUS Status,
    _In_
    ULONG_PTR Information
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
WDFEXPORT(WdfRequestRetrieveInputMemory)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _Out_
    WDFMEMORY* Memory
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestRetrieveOutputMemory)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _Out_
    WDFMEMORY* Memory
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

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfRequestSetInformation)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    ULONG_PTR Information
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

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFQUEUE
WDFEXPORT(WdfRequestGetIoQueue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
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

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestImpersonate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    _In_
    PFN_WDF_REQUEST_IMPERSONATE EvtRequestImpersonate,
    _In_opt_
    PVOID Context
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
ULONG
WDFEXPORT(WdfRequestGetRequestorProcessId)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
BOOLEAN
WDFEXPORT(WdfRequestIsFromUserModeDriver)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfRequestSetUserModeDriverInitiatedIo)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    BOOLEAN IsUserModeDriverInitiated
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
BOOLEAN
WDFEXPORT(WdfRequestGetUserModeDriverInitiatedIo)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfRequestSetActivityId)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    LPGUID ActivityId
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestRetrieveActivityId)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _Out_
    LPGUID ActivityId
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
WDF_DEVICE_IO_TYPE
WDFEXPORT(WdfRequestGetEffectiveIoType)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
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

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfStringCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    PCUNICODE_STRING UnicodeString,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES StringAttributes,
    _Out_
    WDFSTRING* String
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfStringGetUnicodeString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFSTRING String,
    _Out_
    PUNICODE_STRING UnicodeString
    );

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

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
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
WDFEXPORT(WdfSpinLockRelease)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    _Requires_lock_held_(_Curr_)
    _Releases_lock_(_Curr_)
    _IRQL_restores_
    WDFSPINLOCK SpinLock
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfTimerCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDF_TIMER_CONFIG Config,
    _In_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFTIMER* Timer
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
WDFEXPORT(WdfTimerStart)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFTIMER Timer,
    _In_
    LONGLONG DueTime
    );

_When_(Wait == __true, _IRQL_requires_max_(PASSIVE_LEVEL))
_When_(Wait == __false, _IRQL_requires_max_(DISPATCH_LEVEL))
WDFAPI
BOOLEAN
WDFEXPORT(WdfTimerStop)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFTIMER Timer,
    _In_
    BOOLEAN Wait
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFOBJECT
WDFEXPORT(WdfTimerGetParentObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFTIMER Timer
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfUsbTargetDeviceCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFUSBDEVICE* UsbDevice
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfUsbTargetDeviceCreateWithParameters)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_USB_DEVICE_CREATE_CONFIG Config,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFUSBDEVICE* UsbDevice
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfUsbTargetDeviceRetrieveInformation)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _Out_
    PWDF_USB_DEVICE_INFORMATION Information
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfUsbTargetDeviceGetDeviceDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _Out_
    PUSB_DEVICE_DESCRIPTOR UsbDeviceDescriptor
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfUsbTargetDeviceRetrieveConfigDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _Out_writes_bytes_to_opt_(*ConfigDescriptorLength,*ConfigDescriptorLength)
    PVOID ConfigDescriptor,
    _Inout_
    PUSHORT ConfigDescriptorLength
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfUsbTargetDeviceQueryString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _In_opt_
    WDFREQUEST Request,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    _Out_writes_opt_(*NumCharacters)
    PUSHORT String,
    _Inout_
    PUSHORT NumCharacters,
    _In_
    UCHAR StringIndex,
    _In_opt_
    USHORT LangID
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfUsbTargetDeviceAllocAndQueryString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES StringMemoryAttributes,
    _Out_
    WDFMEMORY* StringMemory,
    _Out_opt_
    PUSHORT NumCharacters,
    _In_
    UCHAR StringIndex,
    _In_opt_
    USHORT LangID
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfUsbTargetDeviceFormatRequestForString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _In_
    WDFREQUEST Request,
    _In_
    WDFMEMORY Memory,
    _In_opt_
    PWDFMEMORY_OFFSET Offset,
    _In_
    UCHAR StringIndex,
    _In_opt_
    USHORT LangID
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
UCHAR
WDFEXPORT(WdfUsbTargetDeviceGetNumInterfaces)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfUsbTargetDeviceSelectConfig)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES PipeAttributes,
    _Inout_
    PWDF_USB_DEVICE_SELECT_CONFIG_PARAMS Params
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfUsbTargetDeviceSendControlTransferSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _In_opt_
    WDFREQUEST Request,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    _In_
    PWDF_USB_CONTROL_SETUP_PACKET SetupPacket,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR MemoryDescriptor,
    _Out_opt_
    PULONG BytesTransferred
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfUsbTargetDeviceFormatRequestForControlTransfer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _In_
    WDFREQUEST Request,
    _In_
    PWDF_USB_CONTROL_SETUP_PACKET SetupPacket,
    _In_opt_
    WDFMEMORY TransferMemory,
    _In_opt_
    PWDFMEMORY_OFFSET TransferOffset
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfUsbTargetDeviceResetPortSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfUsbTargetDeviceQueryUsbCapability)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _In_
    CONST GUID* CapabilityType,
    _In_
    ULONG CapabilityBufferLength,
    _When_(CapabilityBufferLength == 0, _Out_opt_)
    _When_(CapabilityBufferLength != 0 && ResultLength == NULL, _Out_writes_bytes_(CapabilityBufferLength))
    _When_(CapabilityBufferLength != 0 && ResultLength != NULL, _Out_writes_bytes_to_opt_(CapabilityBufferLength, *ResultLength))
    PVOID CapabilityBuffer,
    _Out_opt_
    _When_(ResultLength != NULL,_Deref_out_range_(<=,CapabilityBufferLength))
    PULONG ResultLength
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfUsbTargetPipeGetInformation)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe,
    _Out_
    PWDF_USB_PIPE_INFORMATION PipeInformation
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
WDFEXPORT(WdfUsbTargetPipeIsInEndpoint)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
WDFEXPORT(WdfUsbTargetPipeIsOutEndpoint)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDF_USB_PIPE_TYPE
WDFEXPORT(WdfUsbTargetPipeGetType)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfUsbTargetPipeSetNoMaximumPacketSizeCheck)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfUsbTargetPipeWriteSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe,
    _In_opt_
    WDFREQUEST Request,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR MemoryDescriptor,
    _Out_opt_
    PULONG BytesWritten
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfUsbTargetPipeFormatRequestForWrite)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe,
    _In_
    WDFREQUEST Request,
    _In_opt_
    WDFMEMORY WriteMemory,
    _In_opt_
    PWDFMEMORY_OFFSET WriteOffset
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfUsbTargetPipeReadSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe,
    _In_opt_
    WDFREQUEST Request,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR MemoryDescriptor,
    _Out_opt_
    PULONG BytesRead
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfUsbTargetPipeFormatRequestForRead)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe,
    _In_
    WDFREQUEST Request,
    _In_opt_
    WDFMEMORY ReadMemory,
    _In_opt_
    PWDFMEMORY_OFFSET ReadOffset
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfUsbTargetPipeConfigContinuousReader)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe,
    _In_
    PWDF_USB_CONTINUOUS_READER_CONFIG Config
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfUsbTargetPipeAbortSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe,
    _In_opt_
    WDFREQUEST Request,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfUsbTargetPipeFormatRequestForAbort)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe,
    _In_
    WDFREQUEST Request
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfUsbTargetPipeResetSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe,
    _In_opt_
    WDFREQUEST Request,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfUsbTargetPipeFormatRequestForReset)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe,
    _In_
    WDFREQUEST Request
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BYTE
WDFEXPORT(WdfUsbInterfaceGetInterfaceNumber)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBINTERFACE UsbInterface
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BYTE
WDFEXPORT(WdfUsbInterfaceGetNumEndpoints)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBINTERFACE UsbInterface,
    _In_
    UCHAR SettingIndex
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfUsbInterfaceGetDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBINTERFACE UsbInterface,
    _In_
    UCHAR SettingIndex,
    _Out_
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BYTE
WDFEXPORT(WdfUsbInterfaceGetNumSettings)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBINTERFACE UsbInterface
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfUsbInterfaceSelectSetting)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBINTERFACE UsbInterface,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES PipesAttributes,
    _In_
    PWDF_USB_INTERFACE_SELECT_SETTING_PARAMS Params
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfUsbInterfaceGetEndpointInformation)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBINTERFACE UsbInterface,
    _In_
    UCHAR SettingIndex,
    _In_
    UCHAR EndpointIndex,
    _Out_
    PWDF_USB_PIPE_INFORMATION EndpointInfo
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFUSBINTERFACE
WDFEXPORT(WdfUsbTargetDeviceGetInterface)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _In_
    UCHAR InterfaceIndex
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BYTE
WDFEXPORT(WdfUsbInterfaceGetConfiguredSettingIndex)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBINTERFACE Interface
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BYTE
WDFEXPORT(WdfUsbInterfaceGetNumConfiguredPipes)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBINTERFACE UsbInterface
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFUSBPIPE
WDFEXPORT(WdfUsbInterfaceGetConfiguredPipe)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBINTERFACE UsbInterface,
    _In_
    UCHAR PipeIndex,
    _Out_opt_
    PWDF_USB_PIPE_INFORMATION PipeInfo
    );

WDFAPI
VOID
WDFEXPORT(WdfVerifierDbgBreakPoint)(
    _In_
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

WDFAPI
PVOID
WDFEXPORT(WdfGetTriageInfo)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals
    );

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


#ifdef FX_DYNAMICS_GENERATE_TABLE

WDFVERSION WdfVersion = {
    sizeof(WDFVERSION),
    sizeof(WDFFUNCTIONS)/sizeof(PVOID),
    {
        WDFEXPORT(WdfCollectionCreate),
        WDFEXPORT(WdfCollectionGetCount),
        WDFEXPORT(WdfCollectionAdd),
        WDFEXPORT(WdfCollectionRemove),
        WDFEXPORT(WdfCollectionRemoveItem),
        WDFEXPORT(WdfCollectionGetItem),
        WDFEXPORT(WdfCollectionGetFirstItem),
        WDFEXPORT(WdfCollectionGetLastItem),
        WDFEXPORT(WdfCxDeviceInitAllocate),
        WDFEXPORT(WdfCxDeviceInitSetRequestAttributes),
        WDFEXPORT(WdfCxDeviceInitSetFileObjectConfig),
        WDFEXPORT(WdfCxVerifierKeBugCheck),
        WDFEXPORT(WdfDeviceGetDeviceState),
        WDFEXPORT(WdfDeviceSetDeviceState),
        WDFEXPORT(WdfDeviceGetDriver),
        WDFEXPORT(WdfDeviceGetIoTarget),
        WDFEXPORT(WdfDeviceAssignS0IdleSettings),
        WDFEXPORT(WdfDeviceAssignSxWakeSettings),
        WDFEXPORT(WdfDeviceOpenRegistryKey),
        WDFEXPORT(WdfDeviceInitSetPnpPowerEventCallbacks),
        WDFEXPORT(WdfDeviceInitSetPowerPolicyEventCallbacks),
        WDFEXPORT(WdfDeviceInitSetPowerPolicyOwnership),
        WDFEXPORT(WdfDeviceInitSetIoType),
        WDFEXPORT(WdfDeviceInitSetFileObjectConfig),
        WDFEXPORT(WdfDeviceInitSetRequestAttributes),
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
        WDFEXPORT(WdfDeviceSetFailed),
        WDFEXPORT(WdfDeviceStopIdleNoTrack),
        WDFEXPORT(WdfDeviceResumeIdleNoTrack),
        WDFEXPORT(WdfDeviceGetFileObject),
        WDFEXPORT(WdfDeviceGetDefaultQueue),
        WDFEXPORT(WdfDeviceConfigureRequestDispatching),
        WDFEXPORT(WdfDeviceGetSystemPowerAction),
        WDFEXPORT(WdfDeviceInitSetReleaseHardwareOrderOnFailure),
        WDFEXPORT(WdfDeviceInitSetIoTypeEx),
        WDFEXPORT(WdfDevicePostEvent),
        WDFEXPORT(WdfDeviceMapIoSpace),
        WDFEXPORT(WdfDeviceUnmapIoSpace),
        WDFEXPORT(WdfDeviceGetHardwareRegisterMappedAddress),
        WDFEXPORT(WdfDeviceReadFromHardware),
        WDFEXPORT(WdfDeviceWriteToHardware),
        WDFEXPORT(WdfDeviceAssignInterfaceProperty),
        WDFEXPORT(WdfDeviceAllocAndQueryInterfaceProperty),
        WDFEXPORT(WdfDeviceQueryInterfaceProperty),
        WDFEXPORT(WdfDeviceGetDeviceStackIoType),
        WDFEXPORT(WdfDeviceQueryPropertyEx),
        WDFEXPORT(WdfDeviceAllocAndQueryPropertyEx),
        WDFEXPORT(WdfDeviceAssignProperty),
        WDFEXPORT(WdfDriverCreate),
        WDFEXPORT(WdfDriverGetRegistryPath),
        WDFEXPORT(WdfDriverOpenParametersRegistryKey),
        WDFEXPORT(WdfDriverRetrieveVersionString),
        WDFEXPORT(WdfDriverIsVersionAvailable),
        WDFEXPORT(WdfFdoInitOpenRegistryKey),
        WDFEXPORT(WdfFdoInitQueryProperty),
        WDFEXPORT(WdfFdoInitAllocAndQueryProperty),
        WDFEXPORT(WdfFdoInitQueryPropertyEx),
        WDFEXPORT(WdfFdoInitAllocAndQueryPropertyEx),
        WDFEXPORT(WdfFdoInitSetFilter),
        WDFEXPORT(WdfFileObjectGetFileName),
        WDFEXPORT(WdfFileObjectGetDevice),
        WDFEXPORT(WdfFileObjectGetInitiatorProcessId),
        WDFEXPORT(WdfFileObjectGetRelatedFileObject),
        WDFEXPORT(WdfInterruptCreate),
        WDFEXPORT(WdfInterruptQueueDpcForIsr),
        WDFEXPORT(WdfInterruptQueueWorkItemForIsr),
        WDFEXPORT(WdfInterruptSynchronize),
        WDFEXPORT(WdfInterruptAcquireLock),
        WDFEXPORT(WdfInterruptReleaseLock),
        WDFEXPORT(WdfInterruptEnable),
        WDFEXPORT(WdfInterruptDisable),
        WDFEXPORT(WdfInterruptGetInfo),
        WDFEXPORT(WdfInterruptSetPolicy),
        WDFEXPORT(WdfInterruptSetExtendedPolicy),
        WDFEXPORT(WdfInterruptGetDevice),
        WDFEXPORT(WdfInterruptTryToAcquireLock),
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
        WDFEXPORT(WdfIoQueueStopAndPurge),
        WDFEXPORT(WdfIoQueueStopAndPurgeSynchronously),
        WDFEXPORT(WdfIoTargetCreate),
        WDFEXPORT(WdfIoTargetOpen),
        WDFEXPORT(WdfIoTargetCloseForQueryRemove),
        WDFEXPORT(WdfIoTargetClose),
        WDFEXPORT(WdfIoTargetStart),
        WDFEXPORT(WdfIoTargetStop),
        WDFEXPORT(WdfIoTargetPurge),
        WDFEXPORT(WdfIoTargetGetState),
        WDFEXPORT(WdfIoTargetGetDevice),
        WDFEXPORT(WdfIoTargetSendReadSynchronously),
        WDFEXPORT(WdfIoTargetFormatRequestForRead),
        WDFEXPORT(WdfIoTargetSendWriteSynchronously),
        WDFEXPORT(WdfIoTargetFormatRequestForWrite),
        WDFEXPORT(WdfIoTargetSendIoctlSynchronously),
        WDFEXPORT(WdfIoTargetFormatRequestForIoctl),
        WDFEXPORT(WdfMemoryCreate),
        WDFEXPORT(WdfMemoryCreatePreallocated),
        WDFEXPORT(WdfMemoryGetBuffer),
        WDFEXPORT(WdfMemoryAssignBuffer),
        WDFEXPORT(WdfMemoryCopyToBuffer),
        WDFEXPORT(WdfMemoryCopyFromBuffer),
        WDFEXPORT(WdfObjectGetTypedContextWorker),
        WDFEXPORT(WdfObjectAllocateContext),
        WDFEXPORT(WdfObjectContextGetObject),
        WDFEXPORT(WdfObjectReferenceActual),
        WDFEXPORT(WdfObjectDereferenceActual),
        WDFEXPORT(WdfObjectCreate),
        WDFEXPORT(WdfObjectDelete),
        WDFEXPORT(WdfObjectQuery),
        WDFEXPORT(WdfRegistryOpenKey),
        WDFEXPORT(WdfRegistryCreateKey),
        WDFEXPORT(WdfRegistryClose),
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
        WDFEXPORT(WdfRequestReuse),
        WDFEXPORT(WdfRequestChangeTarget),
        WDFEXPORT(WdfRequestFormatRequestUsingCurrentType),
        WDFEXPORT(WdfRequestSend),
        WDFEXPORT(WdfRequestGetStatus),
        WDFEXPORT(WdfRequestMarkCancelable),
        WDFEXPORT(WdfRequestMarkCancelableEx),
        WDFEXPORT(WdfRequestUnmarkCancelable),
        WDFEXPORT(WdfRequestIsCanceled),
        WDFEXPORT(WdfRequestCancelSentRequest),
        WDFEXPORT(WdfRequestIsFrom32BitProcess),
        WDFEXPORT(WdfRequestSetCompletionRoutine),
        WDFEXPORT(WdfRequestGetCompletionParams),
        WDFEXPORT(WdfRequestAllocateTimer),
        WDFEXPORT(WdfRequestComplete),
        WDFEXPORT(WdfRequestCompleteWithInformation),
        WDFEXPORT(WdfRequestGetParameters),
        WDFEXPORT(WdfRequestRetrieveInputMemory),
        WDFEXPORT(WdfRequestRetrieveOutputMemory),
        WDFEXPORT(WdfRequestRetrieveInputBuffer),
        WDFEXPORT(WdfRequestRetrieveOutputBuffer),
        WDFEXPORT(WdfRequestSetInformation),
        WDFEXPORT(WdfRequestGetInformation),
        WDFEXPORT(WdfRequestGetFileObject),
        WDFEXPORT(WdfRequestGetRequestorMode),
        WDFEXPORT(WdfRequestForwardToIoQueue),
        WDFEXPORT(WdfRequestGetIoQueue),
        WDFEXPORT(WdfRequestRequeue),
        WDFEXPORT(WdfRequestStopAcknowledge),
        WDFEXPORT(WdfRequestImpersonate),
        WDFEXPORT(WdfRequestGetRequestorProcessId),
        WDFEXPORT(WdfRequestIsFromUserModeDriver),
        WDFEXPORT(WdfRequestSetUserModeDriverInitiatedIo),
        WDFEXPORT(WdfRequestGetUserModeDriverInitiatedIo),
        WDFEXPORT(WdfRequestSetActivityId),
        WDFEXPORT(WdfRequestRetrieveActivityId),
        WDFEXPORT(WdfRequestGetEffectiveIoType),
        WDFEXPORT(WdfCmResourceListGetCount),
        WDFEXPORT(WdfCmResourceListGetDescriptor),
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
        WDFEXPORT(WdfUsbTargetDeviceCreate),
        WDFEXPORT(WdfUsbTargetDeviceCreateWithParameters),
        WDFEXPORT(WdfUsbTargetDeviceRetrieveInformation),
        WDFEXPORT(WdfUsbTargetDeviceGetDeviceDescriptor),
        WDFEXPORT(WdfUsbTargetDeviceRetrieveConfigDescriptor),
        WDFEXPORT(WdfUsbTargetDeviceQueryString),
        WDFEXPORT(WdfUsbTargetDeviceAllocAndQueryString),
        WDFEXPORT(WdfUsbTargetDeviceFormatRequestForString),
        WDFEXPORT(WdfUsbTargetDeviceGetNumInterfaces),
        WDFEXPORT(WdfUsbTargetDeviceSelectConfig),
        WDFEXPORT(WdfUsbTargetDeviceSendControlTransferSynchronously),
        WDFEXPORT(WdfUsbTargetDeviceFormatRequestForControlTransfer),
        WDFEXPORT(WdfUsbTargetDeviceResetPortSynchronously),
        WDFEXPORT(WdfUsbTargetDeviceQueryUsbCapability),
        WDFEXPORT(WdfUsbTargetPipeGetInformation),
        WDFEXPORT(WdfUsbTargetPipeIsInEndpoint),
        WDFEXPORT(WdfUsbTargetPipeIsOutEndpoint),
        WDFEXPORT(WdfUsbTargetPipeGetType),
        WDFEXPORT(WdfUsbTargetPipeSetNoMaximumPacketSizeCheck),
        WDFEXPORT(WdfUsbTargetPipeWriteSynchronously),
        WDFEXPORT(WdfUsbTargetPipeFormatRequestForWrite),
        WDFEXPORT(WdfUsbTargetPipeReadSynchronously),
        WDFEXPORT(WdfUsbTargetPipeFormatRequestForRead),
        WDFEXPORT(WdfUsbTargetPipeConfigContinuousReader),
        WDFEXPORT(WdfUsbTargetPipeAbortSynchronously),
        WDFEXPORT(WdfUsbTargetPipeFormatRequestForAbort),
        WDFEXPORT(WdfUsbTargetPipeResetSynchronously),
        WDFEXPORT(WdfUsbTargetPipeFormatRequestForReset),
        WDFEXPORT(WdfUsbInterfaceGetInterfaceNumber),
        WDFEXPORT(WdfUsbInterfaceGetNumEndpoints),
        WDFEXPORT(WdfUsbInterfaceGetDescriptor),
        WDFEXPORT(WdfUsbInterfaceGetNumSettings),
        WDFEXPORT(WdfUsbInterfaceSelectSetting),
        WDFEXPORT(WdfUsbInterfaceGetEndpointInformation),
        WDFEXPORT(WdfUsbTargetDeviceGetInterface),
        WDFEXPORT(WdfUsbInterfaceGetConfiguredSettingIndex),
        WDFEXPORT(WdfUsbInterfaceGetNumConfiguredPipes),
        WDFEXPORT(WdfUsbInterfaceGetConfiguredPipe),
        WDFEXPORT(WdfVerifierDbgBreakPoint),
        WDFEXPORT(WdfVerifierKeBugCheck),
        WDFEXPORT(WdfGetTriageInfo),
        WDFEXPORT(WdfWorkItemCreate),
        WDFEXPORT(WdfWorkItemEnqueue),
        WDFEXPORT(WdfWorkItemGetParentObject),
        WDFEXPORT(WdfWorkItemFlush),
        WDFEXPORT(WdfRegistryWdmGetHandle),
        WDFEXPORT(WdfDeviceStopIdleActual),
        WDFEXPORT(WdfDeviceResumeIdleActual),
        WDFEXPORT(WdfDeviceInitEnableHidInterface),
        WDFEXPORT(WdfDeviceHidNotifyPresence),
        WDFEXPORT(WdfDeviceGetSelfIoTarget),
        WDFEXPORT(WdfDeviceInitAllowSelfIoTarget),
        WDFEXPORT(WdfIoTargetSelfAssignDefaultIoQueue),
        WDFEXPORT(WdfDeviceOpenDevicemapKey),
        WDFEXPORT(WdfIoTargetWdmGetTargetFileHandle),
        WDFEXPORT(WdfDeviceWdmDispatchIrp),
        WDFEXPORT(WdfDeviceWdmDispatchIrpToIoQueue),
        WDFEXPORT(WdfDeviceConfigureWdmIrpDispatchCallback),
    }
};

#endif // FX_DYNAMICS_GENERATE_TABLE

#endif // _FXDYNAMICS_H_

