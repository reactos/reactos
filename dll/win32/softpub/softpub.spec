1 stdcall GenericChainCertificateTrust(ptr) wintrust.GenericChainCertificateTrust
2 stdcall GenericChainFinalProv(ptr) wintrust.GenericChainFinalProv
3 stdcall HTTPSCertificateTrust(ptr) wintrust.HTTPSCertificateTrust
4 stdcall SoftpubDefCertInit(ptr) wintrust.SoftpubDefCertInit
5 stdcall SoftpubFreeDefUsageCallData(long ptr) wintrust.SoftpubFreeDefUsageCallData
6 stdcall SoftpubLoadDefUsageCallData(long ptr) wintrust.SoftpubLoadDefUsageCallData
7 stdcall AddPersonalTrustDBPages() wintrust.AddPersonalTrustDBPages
@ stdcall -private DllRegisterServer() wintrust.SoftpubDllRegisterServer
@ stdcall -private DllUnregisterServer() wintrust.SoftpubDllUnregisterServer
10 stdcall DriverCleanupPolicy(ptr) wintrust.DriverCleanupPolicy
11 stdcall DriverFinalPolicy(ptr) wintrust.DriverFinalPolicy
12 stdcall DriverInitializePolicy(ptr) wintrust.DriverInitializePolicy
13 stdcall FindCertsByIssuer(ptr ptr ptr ptr long wstr long) wintrust.FindCertsByIssuer
14 stdcall HTTPSFinalProv(ptr) wintrust.HTTPSFinalProv
15 stdcall OfficeCleanupPolicy(ptr) wintrust.OfficeCleanupPolicy
16 stdcall OfficeInitializePolicy(ptr) wintrust.OfficeInitializePolicy
17 stdcall OpenPersonalTrustDBDialog(long) wintrust.OpenPersonalTrustDBDialog
18 stdcall SoftpubAuthenticode(ptr) wintrust.SoftpubAuthenticode
19 stdcall SoftpubCheckCert(ptr long long long) wintrust.SoftpubCheckCert
20 stdcall SoftpubCleanup(ptr) wintrust.SoftpubCleanup
21 stdcall SoftpubDumpStructure(ptr) wintrust.SoftpubDumpStructure
22 stdcall SoftpubInitialize(ptr) wintrust.SoftpubInitialize
23 stdcall SoftpubLoadMessage(ptr) wintrust.SoftpubLoadMessage
24 stdcall SoftpubLoadSignature(ptr) wintrust.SoftpubLoadSignature
