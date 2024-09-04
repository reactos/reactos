/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Kernel-Mode Test Suite user-mode test list
 */

#include <kmt_test.h>

KMT_TESTFUNC Test_CcCopyRead;
KMT_TESTFUNC Test_CcCopyWrite;
KMT_TESTFUNC Test_CcMapData;
KMT_TESTFUNC Test_CcPinMappedData;
KMT_TESTFUNC Test_CcPinRead;
KMT_TESTFUNC Test_CcSetFileSizes;
KMT_TESTFUNC Test_Example;
KMT_TESTFUNC Test_FileAttributes;
KMT_TESTFUNC Test_FindFile;
KMT_TESTFUNC Test_FltMgrLoad;
KMT_TESTFUNC Test_FltMgrReg;
KMT_TESTFUNC Test_HidPDescription;
KMT_TESTFUNC Test_IoCreateFile;
KMT_TESTFUNC Test_IoDeviceObject;
KMT_TESTFUNC Test_IoReadWrite;
KMT_TESTFUNC Test_MmMapLockedPagesSpecifyCache;
KMT_TESTFUNC Test_NtCreateSection;
KMT_TESTFUNC Test_NtSystemDebugControl;
KMT_TESTFUNC Test_PoIrp;
KMT_TESTFUNC Test_RtlAvlTree;
KMT_TESTFUNC Test_RtlCaptureContext;
KMT_TESTFUNC Test_RtlException;
KMT_TESTFUNC Test_RtlIntSafe;
KMT_TESTFUNC Test_RtlMemory;
KMT_TESTFUNC Test_RtlRegistry;
KMT_TESTFUNC Test_RtlSplayTree;
KMT_TESTFUNC Test_RtlStack;
KMT_TESTFUNC Test_RtlStrSafe;
KMT_TESTFUNC Test_RtlUnicodeString;
KMT_TESTFUNC Test_TcpIpIoctl;
KMT_TESTFUNC Test_TcpIpTdi;
KMT_TESTFUNC Test_TcpIpConnect;

/* tests with a leading '-' will not be listed */
const KMT_TEST TestList[] =
{
    { "-CcCopyRead",                   Test_CcCopyRead },   // TODO: Crashes on TestWHS
    { "-CcCopyWrite",                  Test_CcCopyWrite },  // TODO: Crashes on TestWHS
    { "-CcMapData",                    Test_CcMapData },
    { "-CcPinMappedData",              Test_CcPinMappedData },
    { "-CcPinRead",                    Test_CcPinRead },
    { "-CcSetFileSizes",               Test_CcSetFileSizes },
    { "-Example",                     Test_Example },
    { "FileAttributes",               Test_FileAttributes },
    { "FindFile",                     Test_FindFile },
    { "-FltMgrLoad",                  Test_FltMgrLoad }, // TODO: WIP/untested/crashes.
    { "-FltMgrReg",                   Test_FltMgrReg }, // TODO: WIP/untested/crashes.
    { "HidPDescription",              Test_HidPDescription },
    { "IoCreateFile",                 Test_IoCreateFile },
    { "IoDeviceObject",               Test_IoDeviceObject },
    { "IoReadWrite",                  Test_IoReadWrite },
    { "MmMapLockedPagesSpecifyCache", Test_MmMapLockedPagesSpecifyCache },
    { "NtCreateSection",              Test_NtCreateSection },
    { "NtSystemDebugControl",         Test_NtSystemDebugControl },
    { "PoIrp",                        Test_PoIrp },
    { "RtlAvlTree",                   Test_RtlAvlTree },
    { "RtlException",                 Test_RtlException },
    { "RtlIntSafe",                   Test_RtlIntSafe },
    { "RtlMemory",                    Test_RtlMemory },
    { "RtlRegistry",                  Test_RtlRegistry },
    { "RtlSplayTree",                 Test_RtlSplayTree },
    { "RtlStack",                     Test_RtlStack },
    { "RtlStrSafe",                   Test_RtlStrSafe },
    { "RtlUnicodeString",             Test_RtlUnicodeString },
    { "TcpIpTdi",                     Test_TcpIpTdi },
    { "TcpIpConnect",                 Test_TcpIpConnect },
#ifdef _M_AMD64
    { "RtlCaptureContextUM",          Test_RtlCaptureContext },
#endif
    { NULL,                           NULL },
};
