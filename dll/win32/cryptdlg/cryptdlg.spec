1 stub CertConfigureTrustA
2 stub CertConfigureTrustW
3 stdcall CertTrustCertPolicy(ptr long long long)
4 stdcall CertTrustCleanup(ptr)
5 stdcall CertTrustFinalPolicy(ptr)
6 stdcall CertTrustInit(ptr)
7 stub DecodeAttrSequence
8 stub DecodeRecipientID
9 stub EncodeAttrSequence
10 stub EncodeRecipientID
11 stub FormatPKIXEmailProtection
12 stdcall FormatVerisignExtension(long long long ptr str ptr long ptr ptr)
13 stub CertModifyCertificatesToTrust
14 stub CertSelectCertificateA
15 stub CertSelectCertificateW
16 stdcall CertViewPropertiesA(ptr)
17 stdcall CertViewPropertiesW(ptr)
18 stdcall -private DllRegisterServer()
19 stdcall -private DllUnregisterServer()
20 stdcall GetFriendlyNameOfCertA(ptr ptr long)
21 stdcall GetFriendlyNameOfCertW(ptr ptr long)
