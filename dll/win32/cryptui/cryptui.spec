1 stub ACUIProviderInvokeUI
2 stdcall CryptUIDlgCertMgr(ptr)
3 stub CryptUIDlgFreeCAContext
4 stub CryptUIDlgSelectCA
5 stdcall CryptUIDlgSelectCertificateA(ptr)
6 stdcall CryptUIDlgSelectCertificateFromStore(ptr ptr wstr wstr long long ptr)
7 stdcall CryptUIDlgSelectCertificateW(ptr)
8 stdcall CryptUIDlgSelectStoreA(ptr)
9 stdcall CryptUIDlgSelectStoreW(ptr)
10 stub CryptUIDlgViewCRLA
11 stub CryptUIDlgViewCRLW
12 stub CryptUIDlgViewCTLA
13 stub CryptUIDlgViewCTLW
14 stdcall CryptUIDlgViewCertificateA(ptr ptr)
15 stub CryptUIDlgViewCertificatePropertiesA
16 stub CryptUIDlgViewCertificatePropertiesW
17 stdcall CryptUIDlgViewCertificateW(ptr ptr)
18 stdcall CryptUIDlgViewContext(long ptr ptr wstr long ptr)
19 stdcall CryptUIDlgViewSignerInfoA(ptr)
20 stub CryptUIDlgViewSignerInfoW
21 stub CryptUIFreeCertificatePropertiesPagesA
22 stub CryptUIFreeCertificatePropertiesPagesW
23 stub CryptUIFreeViewSignaturesPagesA
24 stub CryptUIFreeViewSignaturesPagesW
25 stub CryptUIGetCertificatePropertiesPagesA
26 stub CryptUIGetCertificatePropertiesPagesW
27 stub CryptUIGetViewSignaturesPagesA
28 stub CryptUIGetViewSignaturesPagesW
29 stub CryptUIStartCertMgr
30 stub CryptUIWizBuildCTL
31 stub CryptUIWizCertRequest
32 stub CryptUIWizCreateCertRequestNoDS
33 stdcall CryptUIWizDigitalSign(long long wstr ptr ptr)
34 stdcall CryptUIWizExport(long ptr wstr ptr ptr)
35 stub CryptUIWizFreeCertRequestNoDS
36 stub CryptUIWizFreeDigitalSignContext
37 stdcall CryptUIWizImport(long ptr wstr ptr ptr)
38 stub CryptUIWizQueryCertRequestNoDS
39 stub CryptUIWizSubmitCertRequestNoDS
40 stdcall -private DllRegisterServer()
41 stdcall -private DllUnregisterServer()
42 stub EnrollmentCOMObjectFactory_getInstance
43 stub I_CryptUIProtect
44 stub I_CryptUIProtectFailure
45 stub LocalEnroll
46 stub LocalEnrollNoDS
47 stub RetrievePKCS7FromCA
48 stub WizardFree
