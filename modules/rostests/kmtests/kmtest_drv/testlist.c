/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Kernel-Mode Test Suite kernel-mode test list
 */

#include <kmt_test.h>

KMT_TESTFUNC Test_CmSecurity;
KMT_TESTFUNC Test_Example;
KMT_TESTFUNC Test_ExCallback;
KMT_TESTFUNC Test_ExDoubleList;
KMT_TESTFUNC Test_ExFastMutex;
KMT_TESTFUNC Test_ExHardError;
KMT_TESTFUNC Test_ExHardErrorInteractive;
KMT_TESTFUNC Test_ExInterlocked;
KMT_TESTFUNC Test_ExPools;
KMT_TESTFUNC Test_ExResource;
KMT_TESTFUNC Test_ExSequencedList;
KMT_TESTFUNC Test_ExSingleList;
KMT_TESTFUNC Test_ExTimer;
KMT_TESTFUNC Test_ExUuid;
KMT_TESTFUNC Test_FsRtlDissect;
KMT_TESTFUNC Test_FsRtlExpression;
KMT_TESTFUNC Test_FsRtlLegal;
KMT_TESTFUNC Test_FsRtlMcb;
KMT_TESTFUNC Test_FsRtlRemoveDotsFromPath;
KMT_TESTFUNC Test_FsRtlTunnel;
KMT_TESTFUNC Test_HalSystemInfo;
KMT_TESTFUNC Test_IoCreateFile;
KMT_TESTFUNC Test_IoDeviceInterface;
KMT_TESTFUNC Test_IoEvent;
KMT_TESTFUNC Test_IoFilesystem;
KMT_TESTFUNC Test_IoInterrupt;
KMT_TESTFUNC Test_IoIrp;
KMT_TESTFUNC Test_IoMdl;
KMT_TESTFUNC Test_IoVolume;
KMT_TESTFUNC Test_KdSystemDebugControl;
KMT_TESTFUNC Test_KeApc;
KMT_TESTFUNC Test_KeDeviceQueue;
KMT_TESTFUNC Test_KeDpc;
KMT_TESTFUNC Test_KeEvent;
KMT_TESTFUNC Test_KeFloatPointState;
KMT_TESTFUNC Test_KeGuardedMutex;
KMT_TESTFUNC Test_KeIrql;
KMT_TESTFUNC Test_KeMutex;
KMT_TESTFUNC Test_KeProcessor;
KMT_TESTFUNC Test_KeSpinLock;
KMT_TESTFUNC Test_KeTimer;
KMT_TESTFUNC Test_KernelType;
KMT_TESTFUNC Test_MmMdl;
KMT_TESTFUNC Test_MmSection;
KMT_TESTFUNC Test_MmReservedMapping;
KMT_TESTFUNC Test_NpfsConnect;
KMT_TESTFUNC Test_NpfsCreate;
KMT_TESTFUNC Test_NpfsFileInfo;
KMT_TESTFUNC Test_NpfsReadWrite;
KMT_TESTFUNC Test_NpfsVolumeInfo;
KMT_TESTFUNC Test_ObHandle;
KMT_TESTFUNC Test_ObQuery;
KMT_TESTFUNC Test_ObReference;
KMT_TESTFUNC Test_ObSecurity;
KMT_TESTFUNC Test_ObSymbolicLink;
KMT_TESTFUNC Test_ObType;
KMT_TESTFUNC Test_ObTypeClean;
KMT_TESTFUNC Test_ObTypeNoClean;
KMT_TESTFUNC Test_ObTypes;
KMT_TESTFUNC Test_PsNotify;
KMT_TESTFUNC Test_PsQuota;
KMT_TESTFUNC Test_SeInheritance;
KMT_TESTFUNC Test_SeLogonSession;
KMT_TESTFUNC Test_SeQueryInfoToken;
KMT_TESTFUNC Test_SeTokenFiltering;
KMT_TESTFUNC Test_RtlAvlTree;
KMT_TESTFUNC Test_RtlCaptureContext;
KMT_TESTFUNC Test_RtlException;
KMT_TESTFUNC Test_RtlIntSafe;
KMT_TESTFUNC Test_RtlIsValidOemCharacter;
KMT_TESTFUNC Test_RtlMemory;
KMT_TESTFUNC Test_RtlRangeList;
KMT_TESTFUNC Test_RtlRegistry;
KMT_TESTFUNC Test_RtlSplayTree;
KMT_TESTFUNC Test_RtlStack;
KMT_TESTFUNC Test_RtlStrSafe;
KMT_TESTFUNC Test_RtlUnicodeString;
KMT_TESTFUNC Test_ZwAllocateVirtualMemory;
KMT_TESTFUNC Test_ZwCreateSection;
KMT_TESTFUNC Test_ZwMapViewOfSection;
KMT_TESTFUNC Test_ZwWaitForMultipleObjects;

const KMT_TEST TestList[] =
{
    { "CmSecurity",                         Test_CmSecurity },
    { "ExCallback",                         Test_ExCallback },
    { "ExDoubleList",                       Test_ExDoubleList },
    { "ExFastMutex",                        Test_ExFastMutex },
    { "ExHardError",                        Test_ExHardError },
    { "-ExHardErrorInteractive",            Test_ExHardErrorInteractive },
    { "ExInterlocked",                      Test_ExInterlocked },
    { "ExPools",                            Test_ExPools },
    { "ExResource",                         Test_ExResource },
    { "ExSequencedList",                    Test_ExSequencedList },
    { "ExSingleList",                       Test_ExSingleList },
    { "-ExTimer",                           Test_ExTimer },
    { "ExUuid",                             Test_ExUuid },
    { "Example",                            Test_Example },
    { "FsRtlDissect",                       Test_FsRtlDissect },
    { "FsRtlExpression",                    Test_FsRtlExpression },
    { "FsRtlLegal",                         Test_FsRtlLegal },
    { "FsRtlMcb",                           Test_FsRtlMcb },
    { "FsRtlRemoveDotsFromPath",            Test_FsRtlRemoveDotsFromPath },
    { "FsRtlTunnel",                        Test_FsRtlTunnel },
    { "HalSystemInfo",                      Test_HalSystemInfo },
    { "IoCreateFile",                       Test_IoCreateFile },
    { "IoDeviceInterface",                  Test_IoDeviceInterface },
    { "IoEvent",                            Test_IoEvent },
    { "IoFilesystem",                       Test_IoFilesystem },
    { "IoInterrupt",                        Test_IoInterrupt },
    { "IoIrp",                              Test_IoIrp },
    { "IoMdl",                              Test_IoMdl },
    { "IoVolume",                           Test_IoVolume },
    { "KdSystemDebugControl",               Test_KdSystemDebugControl },
    { "KeApc",                              Test_KeApc },
    { "KeDeviceQueue",                      Test_KeDeviceQueue },
    { "KeDpc",                              Test_KeDpc },
    { "KeEvent",                            Test_KeEvent },
    { "KeFloatPointState",                  Test_KeFloatPointState },
    { "KeGuardedMutex",                     Test_KeGuardedMutex },
    { "KeIrql",                             Test_KeIrql },
    { "KeMutex",                            Test_KeMutex },
    { "-KeProcessor",                       Test_KeProcessor },
    { "KeSpinLock",                         Test_KeSpinLock },
    { "KeTimer",                            Test_KeTimer },
    { "-KernelType",                        Test_KernelType },
    { "MmMdl",                              Test_MmMdl },
    { "MmSection",                          Test_MmSection },
    { "MmReservedMapping",                  Test_MmReservedMapping },
    { "NpfsConnect",                        Test_NpfsConnect },
    { "NpfsCreate",                         Test_NpfsCreate },
    { "NpfsFileInfo",                       Test_NpfsFileInfo },
    { "NpfsReadWrite",                      Test_NpfsReadWrite },
    { "NpfsVolumeInfo",                     Test_NpfsVolumeInfo },
    { "ObHandle",                           Test_ObHandle },
    { "ObQuery",                            Test_ObQuery },
    { "ObReference",                        Test_ObReference },
    { "ObSecurity",                         Test_ObSecurity },
    { "ObSymbolicLink",                     Test_ObSymbolicLink },
    { "ObType",                             Test_ObType },
    { "-ObTypeClean",                       Test_ObTypeClean },
    { "-ObTypeNoClean",                     Test_ObTypeNoClean },
    { "ObTypes",                            Test_ObTypes },
    { "PsNotify",                           Test_PsNotify },
    { "PsQuota",                            Test_PsQuota },
    { "RtlAvlTreeKM",                       Test_RtlAvlTree },
    { "RtlExceptionKM",                     Test_RtlException },
    { "RtlIntSafeKM",                       Test_RtlIntSafe },
    { "RtlIsValidOemCharacter",             Test_RtlIsValidOemCharacter },
    { "RtlMemoryKM",                        Test_RtlMemory },
    { "RtlRangeList",                       Test_RtlRangeList },
    { "RtlRegistryKM",                      Test_RtlRegistry },
    { "RtlSplayTreeKM",                     Test_RtlSplayTree },
    { "RtlStackKM",                         Test_RtlStack },
    { "RtlStrSafeKM",                       Test_RtlStrSafe },
    { "RtlUnicodeStringKM",                 Test_RtlUnicodeString },
    { "SeInheritance",                      Test_SeInheritance },
    { "SeLogonSession",                     Test_SeLogonSession },
    { "SeQueryInfoToken",                   Test_SeQueryInfoToken },
    { "SeTokenFiltering",                   Test_SeTokenFiltering },
    { "ZwAllocateVirtualMemory",            Test_ZwAllocateVirtualMemory },
    { "ZwCreateSection",                    Test_ZwCreateSection },
    { "ZwMapViewOfSection",                 Test_ZwMapViewOfSection },
    { "ZwWaitForMultipleObjects",           Test_ZwWaitForMultipleObjects},
#ifdef _M_AMD64
    { "RtlCaptureContextKM",                Test_RtlCaptureContext },
#endif
    { NULL,                                 NULL }
};
