#ifndef _SSPI_H
#define _SSPI_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SECPKG_CRED_INBOUND 1
#define SECPKG_CRED_OUTBOUND 2
#define SECPKG_CRED_BOTH (SECPKG_CRED_OUTBOUND|SECPKG_CRED_INBOUND)
#define SECPKG_CRED_ATTR_NAMES 1

#define SECPKG_FLAG_INTEGRITY 1
#define SECPKG_FLAG_PRIVACY 2
#define SECPKG_FLAG_TOKEN_ONLY 4
#define SECPKG_FLAG_DATAGRAM 8
#define SECPKG_FLAG_CONNECTION 16
#define SECPKG_FLAG_MULTI_REQUIRED 32
#define SECPKG_FLAG_CLIENT_ONLY 64
#define SECPKG_FLAG_EXTENDED_ERROR 128
#define SECPKG_FLAG_IMPERSONATION 256
#define SECPKG_FLAG_ACCEPT_WIN32_NAME 512
#define SECPKG_FLAG_STREAM 1024

#define SECPKG_ATTR_AUTHORITY 6
#define SECPKG_ATTR_CONNECTION_INFO 90
#define SECPKG_ATTR_ISSUER_LIST 80
#define SECPKG_ATTR_ISSUER_LIST_EX 89
#define SECPKG_ATTR_KEY_INFO 5
#define SECPKG_ATTR_LIFESPAN 2
#define SECPKG_ATTR_LOCAL_CERT_CONTEXT 84
#define SECPKG_ATTR_LOCAL_CRED 82
#define SECPKG_ATTR_NAMES 1
#define SECPKG_ATTR_PROTO_INFO 7
#define SECPKG_ATTR_REMOTE_CERT_CONTEXT 83
#define SECPKG_ATTR_REMOTE_CRED 81
#define SECPKG_ATTR_SIZES 0
#define SECPKG_ATTR_STREAM_SIZES 4

#define SECBUFFER_EMPTY 0
#define SECBUFFER_DATA 1
#define SECBUFFER_TOKEN 2
#define SECBUFFER_PKG_PARAMS 3
#define SECBUFFER_MISSING 4
#define SECBUFFER_EXTRA 5
#define SECBUFFER_STREAM_TRAILER 6
#define SECBUFFER_STREAM_HEADER 7
#define SECBUFFER_PADDING 9
#define SECBUFFER_STREAM 10
#define SECBUFFER_READONLY 0x80000000
#define SECBUFFER_ATTRMASK 0xf0000000

#define UNISP_NAME_A "Microsoft Unified Security Protocol Provider"
#define UNISP_NAME_W L"Microsoft Unified Security Protocol Provider"
#define SECBUFFER_VERSION 0

typedef struct _SecHandle {
	ULONG_PTR dwLower;
	ULONG_PTR dwUpper;
} SecHandle, *PSecHandle;
typedef struct _SecBuffer {
	ULONG cbBuffer;
	ULONG BufferType;
	PVOID pvBuffer;
} SecBuffer, *PSecBuffer;
typedef SecHandle CredHandle;
typedef PSecHandle PCredHandle;
typedef SecHandle CtxtHandle;
typedef PSecHandle PCtxtHandle;
typedef struct _SECURITY_INTEGER {
	unsigned long LowPart;
	long HighPart;
} SECURITY_INTEGER;
typedef SECURITY_INTEGER TimeStamp, *PTimeStamp;
typedef struct _SecBufferDesc {
	ULONG ulVersion;
	ULONG cBuffers;
	PSecBuffer pBuffers;
} SecBufferDesc, *PSecBufferDesc;
typedef struct _SecPkgContext_StreamSizes {
	ULONG cbHeader;
	ULONG cbTrailer;
	ULONG cbMaximumMessage;
	ULONG cBuffers;
	ULONG cbBlockSize;
} SecPkgContext_StreamSizes, *PSecPkgContext_StreamSizes;
typedef struct _SecPkgContext_Sizes {
	ULONG cbMaxToken;
	ULONG cbMaxSIgnature;
	ULONG cbBlockSize;
	ULONG cbSecurityTrailer;
} SecPkgContext_Sizes, *PSecPkgContext_Sizes;
typedef struct _SecPkgContext_AuthorityW {
	SEC_WCHAR* sAuthorityName;
} SecPkgContext_AuthorityW, *PSecPkgContext_AuthorityW;
typedef struct _SecPkgContext_AuthorityA {
	SEC_CHAR* sAuthorityName;
} SecPkgContext_AuthorityA, *PSecPkgContext_AuthorityA;
typedef struct _SecPkgContext_KeyInfoW {
	SEC_WCHAR* sSignatureAlgorithmName;
	SEC_WCHAR* sEncryptAlgorithmName;
	ULONG KeySize;
	ULONG SignatureAlgorithm;
	ULONG EncryptAlgorithm;
} SecPkgContext_KeyInfoW, *PSecPkgContext_KeyInfoW;
typedef struct _SecPkgContext_KeyInfoA {
	SEC_CHAR* sSignatureAlgorithmName;
	SEC_CHAR* sEncryptAlgorithmName;
	ULONG KeySize;
	ULONG SignatureAlgorithm;
	ULONG EncryptAlgorithm;
} SecPkgContext_KeyInfoA, *PSecPkgContext_KeyInfoA;
typedef struct _SecPkgContext_LifeSpan {
	TimeStamp tsStart;
	TimeStamp tsExpiry;
} SecPkgContext_LifeSpan, *PSecPkgContext_LifeSpan;
typedef struct _SecPkgContext_NamesW {
	SEC_WCHAR* sUserName;
} SecPkgContext_NamesW, *PSecPkgContext_NamesW;
typedef struct _SecPkgContext_NamesA {
	SEC_CHAR* sUserName;
} SecPkgContext_NamesA, *PSecPkgContext_NamesA;
typedef struct _SecPkgInfoW {
	ULONG fCapabilities;
	USHORT wVersion;
	USHORT wRPCID;
	ULONG cbMaxToken;
	SEC_WCHAR* Name;
	SEC_WCHAR* Comment;
} SecPkgInfoW, *PSecPkgInfoW;
typedef struct _SecPkgInfoA {
	ULONG fCapabilities;
	USHORT wVersion;
	USHORT wRPCID;
	ULONG cbMaxToken;
	SEC_CHAR* Name;
	SEC_CHAR* Comment;
} SecPkgInfoA, *PSecPkgInfoA;
/* supported only in win2k+, so it should be a PSecPkgInfoW */
/* PSDK does not say it has ANSI/Unicode versions */
typedef struct _SecPkgContext_PackageInfo {
	PSecPkgInfoW PackageInfo;
} SecPkgContext_PackageInfo, *PSecPkgContext_PackageInfo;
typedef struct _SecPkgCredentials_NamesW {
	SEC_WCHAR* sUserName;
} SecPkgCredentialsNamesW, *PSecPkgCredentialsNamesW;
typedef struct _SecPkgCredentials_NamesA {
	SEC_CHAR* sUserName;
} SecPkgCredentialsNamesA, *PSecPkgCredentialsNamesA;

/* TODO: missing type in SDK */
typedef void (*SEC_GET_KEY_FN)();

typedef SECURITY_STATUS (WINAPI *ENUMERATE_SECURITY_PACKAGES_FN_W)(PULONG,PSecPkgInfoW*);
typedef SECURITY_STATUS (WINAPI *ENUMERATE_SECURITY_PACKAGES_FN_A)(PULONG,PSecPkgInfoA*);
typedef SECURITY_STATUS (WINAPI *QUERY_CREDENTIALS_ATTRIBUTES_FN_W)(PCredHandle,ULONG,PVOID);
typedef SECURITY_STATUS (WINAPI *QUERY_CREDENTIALS_ATTRIBUTES_FN_A)(PCredHandle,ULONG,PVOID);
typedef SECURITY_STATUS (WINAPI *ACQUIRE_CREDENTIALS_HANDLE_FN_W)(SEC_WCHAR*,SEC_WCHAR*,ULONG,PLUID,PVOID,SEC_GET_KEY_FN,PVOID,PCredHandle,PTimeStamp);
typedef SECURITY_STATUS (WINAPI *ACQUIRE_CREDENTIALS_HANDLE_FN_A)(SEC_CHAR*,SEC_CHAR*,ULONG,PLUID,PVOID,SEC_GET_KEY_FN,PVOID,PCredHandle,PTimeStamp);
typedef SECURITY_STATUS (WINAPI *FREE_CREDENTIALS_HANDLE_FN)(PCredHandle);
typedef SECURITY_STATUS (WINAPI *INITIALIZE_SECURITY_CONTEXT_FN_W)(PCredHandle,PCtxtHandle,SEC_WCHAR*,ULONG,ULONG,ULONG,PSecBufferDesc,ULONG,PCtxtHandle,PSecBufferDesc,PULONG,PTimeStamp);
typedef SECURITY_STATUS (WINAPI *INITIALIZE_SECURITY_CONTEXT_FN_A)(PCredHandle,PCtxtHandle,SEC_CHAR*,ULONG,ULONG,ULONG,PSecBufferDesc,ULONG,PCtxtHandle,PSecBufferDesc,PULONG,PTimeStamp);
typedef SECURITY_STATUS (WINAPI *ACCEPT_SECURITY_CONTEXT_FN)(PCredHandle,PCtxtHandle,PSecBufferDesc,ULONG,ULONG,PCtxtHandle,PSecBufferDesc,PULONG,PTimeStamp);
typedef SECURITY_STATUS (WINAPI *COMPLETE_AUTH_TOKEN_FN)(PCtxtHandle,PSecBufferDesc);
typedef SECURITY_STATUS (WINAPI *DELETE_SECURITY_CONTEXT_FN)(PCtxtHandle);
typedef SECURITY_STATUS (WINAPI *APPLY_CONTROL_TOKEN_FN_W)(PCtxtHandle,PSecBufferDesc);
typedef SECURITY_STATUS (WINAPI *APPLY_CONTROL_TOKEN_FN_A)(PCtxtHandle,PSecBufferDesc);
typedef SECURITY_STATUS (WINAPI *QUERY_CONTEXT_ATTRIBUTES_FN_A)(PCtxtHandle,ULONG,PVOID);
typedef SECURITY_STATUS (WINAPI *QUERY_CONTEXT_ATTRIBUTES_FN_W)(PCtxtHandle,ULONG,PVOID);
typedef SECURITY_STATUS (WINAPI *IMPERSONATE_SECURITY_CONTEXT_FN)(PCtxtHandle);
typedef SECURITY_STATUS (WINAPI *REVERT_SECURITY_CONTEXT_FN)(PCtxtHandle);
typedef SECURITY_STATUS (WINAPI *MAKE_SIGNATURE_FN)(PCtxtHandle,ULONG,PSecBufferDesc,ULONG);
typedef SECURITY_STATUS (WINAPI *VERIFY_SIGNATURE_FN)(PCtxtHandle,PSecBufferDesc,ULONG,PULONG);
typedef SECURITY_STATUS (WINAPI *FREE_CONTEXT_BUFFER_FN)(PVOID);
typedef SECURITY_STATUS (WINAPI *QUERY_SECURITY_PACKAGE_INFO_FN_A)(SEC_CHAR*,PSecPkgInfoA*);
typedef SECURITY_STATUS (WINAPI *QUERY_SECURITY_PACKAGE_INFO_FN_W)(SEC_WCHAR*,PSecPkgInfoW*);
typedef SECURITY_STATUS (WINAPI *ENCRYPT_MESSAGE_FN)(PCtxtHandle,ULONG,PSecBufferDesc,ULONG);
typedef SECURITY_STATUS (WINAPI *DECRYPT_MESSAGE_FN)(PCtxtHandle,PSecBufferDesc,ULONG,PULONG);

/* No, it really is FreeCredentialsHandle, see the thread beginning 
 * http://sourceforge.net/mailarchive/message.php?msg_id=4321080 for a
 * discovery discussion. */
typedef struct _SECURITY_FUNCTION_TABLEW {
	unsigned long dwVersion;
	ENUMERATE_SECURITY_PACKAGES_FN_W EnumerateSecurityPackagesW;
	QUERY_CREDENTIALS_ATTRIBUTES_FN_W QueryCredentialsAttributesW;
	ACQUIRE_CREDENTIALS_HANDLE_FN_W AcquireCredentialsHandleW;
	FREE_CREDENTIALS_HANDLE_FN FreeCredentialsHandle;
	void SEC_FAR* Reserved2;
	INITIALIZE_SECURITY_CONTEXT_FN_A InitializeSecurityContextA;
	ACCEPT_SECURITY_CONTEXT_FN AcceptSecurityContext;
	COMPLETE_AUTH_TOKEN_FN CompleteAuthToken;
	DELETE_SECURITY_CONTEXT_FN DeleteSecurityContext;
	APPLY_CONTROL_TOKEN_FN_W ApplyControlTokenW;
	QUERY_CONTEXT_ATTRIBUTES_FN_W QueryContextAttributesW;
	IMPERSONATE_SECURITY_CONTEXT_FN ImpersonateSecurityContext;
	REVERT_SECURITY_CONTEXT_FN RevertSecurityContext;
	MAKE_SIGNATURE_FN MakeSignature;
	VERIFY_SIGNATURE_FN VerifySignature;
	FREE_CONTEXT_BUFFER_FN FreeContextBuffer;
	QUERY_SECURITY_PACKAGE_INFO_FN_A QuerySecurityPackageInfoA;
	void SEC_FAR* Reserved3;
	void SEC_FAR* Reserved4;
        void SEC_FAR* Unknown1;
        void SEC_FAR* Unknown2;
        void SEC_FAR* Unknown3;
        void SEC_FAR* Unknown4;
        void SEC_FAR* Unknown5;
        ENCRYPT_MESSAGE_FN EncryptMessage;
        DECRYPT_MESSAGE_FN DecryptMessage;
} SecurityFunctionTableW, *PSecurityFunctionTableW;
typedef struct _SECURITY_FUNCTION_TABLEA {
	unsigned long dwVersion;
	ENUMERATE_SECURITY_PACKAGES_FN_A EnumerateSecurityPackagesA;
	QUERY_CREDENTIALS_ATTRIBUTES_FN_A QueryCredentialsAttributesA;
	ACQUIRE_CREDENTIALS_HANDLE_FN_A AcquireCredentialsHandleA;
	FREE_CREDENTIALS_HANDLE_FN FreeCredentialsHandle;
	void SEC_FAR* Reserved2;
	INITIALIZE_SECURITY_CONTEXT_FN_A InitializeSecurityContextA;
	ACCEPT_SECURITY_CONTEXT_FN AcceptSecurityContext;
	COMPLETE_AUTH_TOKEN_FN CompleteAuthToken;
	DELETE_SECURITY_CONTEXT_FN DeleteSecurityContext;
	APPLY_CONTROL_TOKEN_FN_A ApplyControlTokenA;
	QUERY_CONTEXT_ATTRIBUTES_FN_A QueryContextAttributesA;
	IMPERSONATE_SECURITY_CONTEXT_FN ImpersonateSecurityContext;
	REVERT_SECURITY_CONTEXT_FN RevertSecurityContext;
	MAKE_SIGNATURE_FN MakeSignature;
	VERIFY_SIGNATURE_FN VerifySignature;
	FREE_CONTEXT_BUFFER_FN FreeContextBuffer;
	QUERY_SECURITY_PACKAGE_INFO_FN_A QuerySecurityPackageInfoA;
	void SEC_FAR* Reserved3;
	void SEC_FAR* Reserved4;
        void SEC_FAR* Unknown1;
        void SEC_FAR* Unknown2;
        void SEC_FAR* Unknown3;
        void SEC_FAR* Unknown4;
        void SEC_FAR* Unknown5;
        ENCRYPT_MESSAGE_FN EncryptMessage;
        DECRYPT_MESSAGE_FN DecryptMessage;
} SecurityFunctionTableA, *PSecurityFunctionTableA;
typedef PSecurityFunctionTableA (WINAPI *INIT_SECURITY_INTERFACE_A)(VOID);
typedef PSecurityFunctionTableW (WINAPI *INIT_SECURITY_INTERFACE_W)(VOID);

SECURITY_STATUS WINAPI FreeCredentialsHandle(PCredHandle);
SECURITY_STATUS WINAPI EnumerateSecurityPackagesA(PULONG,PSecPkgInfoA*);
SECURITY_STATUS WINAPI EnumerateSecurityPackagesW(PULONG,PSecPkgInfoW*);
SECURITY_STATUS WINAPI AcquireCredentialsHandleA(SEC_CHAR*,SEC_CHAR*,ULONG,PLUID,PVOID,SEC_GET_KEY_FN,PVOID,PCredHandle,PTimeStamp);
SECURITY_STATUS WINAPI AcquireCredentialsHandleW(SEC_WCHAR*,SEC_WCHAR*,ULONG,PLUID,PVOID,SEC_GET_KEY_FN,PVOID,PCredHandle,PTimeStamp);
SECURITY_STATUS WINAPI AcceptSecurityContext(PCredHandle,PCtxtHandle,PSecBufferDesc,ULONG,ULONG,PCtxtHandle,PSecBufferDesc,PULONG,PTimeStamp);
SECURITY_STATUS WINAPI InitializeSecurityContextA(PCredHandle,PCtxtHandle,SEC_CHAR*,ULONG,ULONG,ULONG,PSecBufferDesc,ULONG,PCtxtHandle,PSecBufferDesc,PULONG,PTimeStamp);
SECURITY_STATUS WINAPI InitializeSecurityContextW(PCredHandle,PCtxtHandle,SEC_WCHAR*,ULONG,ULONG,ULONG,PSecBufferDesc,ULONG,PCtxtHandle,PSecBufferDesc,PULONG,PTimeStamp);
SECURITY_STATUS WINAPI FreeContextBuffer(PVOID);
SECURITY_STATUS WINAPI QueryContextAttributesA(PCtxtHandle,ULONG,PVOID);
SECURITY_STATUS WINAPI QueryContextAttributesW(PCtxtHandle,ULONG,PVOID);
SECURITY_STATUS WINAPI QueryCredentialsAttributesA(PCredHandle,ULONG,PVOID);
SECURITY_STATUS WINAPI QueryCredentialsAttributesW(PCredHandle,ULONG,PVOID);
SECURITY_STATUS WINAPI DecryptMessage(PCtxtHandle,PSecBufferDesc,ULONG,PULONG);
SECURITY_STATUS WINAPI EncryptMessage(PCtxtHandle,ULONG,PSecBufferDesc,ULONG);
SECURITY_STATUS WINAPI DeleteSecurityContext(PCtxtHandle);
SECURITY_STATUS WINAPI CompleteAuthToken(PCtxtHandle,PSecBufferDesc);
SECURITY_STATUS WINAPI ApplyControlTokenA(PCtxtHandle,PSecBufferDesc);
SECURITY_STATUS WINAPI ApplyControlTokenW(PCtxtHandle,PSecBufferDesc);
SECURITY_STATUS WINAPI ImpersonateSecurityContext(PCtxtHandle);
SECURITY_STATUS WINAPI RevertSecurityContext(PCtxtHandle);
SECURITY_STATUS WINAPI MakeSignature(PCtxtHandle,ULONG,PSecBufferDesc,ULONG);
SECURITY_STATUS WINAPI VerifySignature(PCtxtHandle,PSecBufferDesc,ULONG,PULONG);
SECURITY_STATUS WINAPI QuerySecurityPackageInfoA(SEC_CHAR*,PSecPkgInfoA*);
SECURITY_STATUS WINAPI QuerySecurityPackageInfoW(SEC_WCHAR*,PSecPkgInfoW*);
PSecurityFunctionTableA WINAPI InitSecurityInterfaceA(VOID);
PSecurityFunctionTableW WINAPI InitSecurityInterfaceW(VOID);

#ifdef UNICODE
#define UNISP_NAME UNISP_NAME_W
#define SecPkgInfo SecPkgInfoW
#define PSecPkgInfo PSecPkgInfoW
#define SecPkgCredentialsNames SecPkgCredentialsNamesW
#define PSecPkgCredentialsNames PSecPkgCredentialsNamesW
#define SecPkgContext_Authority SecPkgContext_AuthorityW
#define PSecPkgContext_Authority PSecPkgContext_AuthorityW
#define SecPkgContext_KeyInfo SecPkgContext_KeyInfoW
#define PSecPkgContext_KeyInfo PSecPkgContext_KeyInfoW
#define SecPkgContext_Names SecPkgContext_NamesW
#define PSecPkgContext_Names PSecPkgContext_NamesW
#define SecurityFunctionTable SecurityFunctionTableW
#define PSecurityFunctionTable PSecurityFunctionTableW
#define AcquireCredentialsHandle AcquireCredentialsHandleW
#define EnumerateSecurityPackages EnumerateSecurityPackagesW
#define InitializeSecurityContext InitializeSecurityContextW
#define QueryContextAttributes QueryContextAttributesW
#define QueryCredentialsAttributes QueryCredentialsAttributesW
#define QuerySecurityPackageInfo QuerySecurityPackageInfoW
#define ApplyControlToken ApplyControlTokenW
#define ENUMERATE_SECURITY_PACKAGES_FN ENUMERATE_SECURITY_PACKAGES_FN_W
#define QUERY_CREDENTIALS_ATTRIBUTES_FN QUERY_CREDENTIALS_ATTRIBUTES_FN_W
#define ACQUIRE_CREDENTIALS_HANDLE_FN ACQUIRE_CREDENTIALS_HANDLE_FN_W
#define INITIALIZE_SECURITY_CONTEXT_FN INITIALIZE_SECURITY_CONTEXT_FN_W
#define APPLY_CONTROL_TOKEN_FN APPLY_CONTROL_TOKEN_FN_W
#define QUERY_CONTEXT_ATTRIBUTES_FN QUERY_CONTEXT_ATTRIBUTES_FN_W
#define QUERY_SECURITY_PACKAGE_INFO_FN QUERY_SECURITY_PACKAGE_INFO_FN_W
#define INIT_SECURITY_INTERFACE INIT_SECURITY_INTERFACE_W
#else
#define UNISP_NAME UNISP_NAME_A
#define SecPkgInfo SecPkgInfoA
#define PSecPkgInfo PSecPkgInfoA
#define SecPkgCredentialsNames SecPkgCredentialsNamesA
#define PSecPkgCredentialsNames PSecPkgCredentialsNamesA
#define SecPkgContext_Authority SecPkgContext_AuthorityA
#define PSecPkgContext_Authority PSecPkgContext_AuthorityA
#define SecPkgContext_KeyInfo SecPkgContext_KeyInfoA
#define PSecPkgContext_KeyInfo PSecPkgContext_KeyInfoA
#define SecPkgContext_Names SecPkgContext_NamesA
#define PSecPkgContext_Names PSecPkgContext_NamesA
#define SecurityFunctionTable SecurityFunctionTableA
#define PSecurityFunctionTable PSecurityFunctionTableA
#define AcquireCredentialsHandle AcquireCredentialsHandleA
#define EnumerateSecurityPackages EnumerateSecurityPackagesA
#define InitializeSecurityContext InitializeSecurityContextA
#define QueryContextAttributes QueryContextAttributesA
#define QueryCredentialsAttributes QueryCredentialsAttributesA
#define QuerySecurityPackageInfo QuerySecurityPackageInfoA
#define ApplyControlToken ApplyControlTokenA
#define ENUMERATE_SECURITY_PACKAGES_FN ENUMERATE_SECURITY_PACKAGES_FN_A
#define QUERY_CREDENTIALS_ATTRIBUTES_FN QUERY_CREDENTIALS_ATTRIBUTES_FN_A
#define ACQUIRE_CREDENTIALS_HANDLE_FN ACQUIRE_CREDENTIALS_HANDLE_FN_A
#define INITIALIZE_SECURITY_CONTEXT_FN INITIALIZE_SECURITY_CONTEXT_FN_A
#define APPLY_CONTROL_TOKEN_FN APPLY_CONTROL_TOKEN_FN_A
#define QUERY_CONTEXT_ATTRIBUTES_FN QUERY_CONTEXT_ATTRIBUTES_FN_A
#define QUERY_SECURITY_PACKAGE_INFO_FN QUERY_SECURITY_PACKAGE_INFO_FN_A
#define INIT_SECURITY_INTERFACE INIT_SECURITY_INTERFACE_A
#endif

#ifdef __cplusplus
}
#endif
#endif
