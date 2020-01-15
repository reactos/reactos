#ifndef _SPUSER_H_
#define _SPUSER_H_


// functions provided by LSA in SpInitialize (user mode)
extern PSECPKG_DLL_FUNCTIONS UsrFunctionTable;
// functions we provide to LSA in SpLsaModeInitialize (user mode)
extern SECPKG_USER_FUNCTION_TABLE UsrTables[1];

NTSTATUS NTAPI
SpInstanceInit(
    ULONG Version,
    PSECPKG_DLL_FUNCTIONS FunctionTable,
    PVOID *UserFunctions);

NTSTATUS NTAPI
UsrSpInitUserModeContext(
    LSA_SEC_HANDLE p1,
    PSecBuffer p2);

NTSTATUS NTAPI
UsrSpMakeSignature(
    LSA_SEC_HANDLE p1,
    ULONG p2,
    PSecBufferDesc p3,
    ULONG p4);

NTSTATUS NTAPI
UsrSpVerifySignature(
    LSA_SEC_HANDLE p1,
    PSecBufferDesc p2,
    ULONG p3,
    PULONG p4);

NTSTATUS NTAPI
UsrSpSealMessage(
    LSA_SEC_HANDLE p1,
    ULONG p2,
    PSecBufferDesc p3,
    ULONG p4);

NTSTATUS NTAPI
UsrSpUnsealMessage(
    LSA_SEC_HANDLE p1,
    PSecBufferDesc p2,
    ULONG p3,
    PULONG p4);

NTSTATUS NTAPI
UsrSpGetContextToken(
    LSA_SEC_HANDLE p1,
    PHANDLE p2);

NTSTATUS NTAPI
UsrSpQueryContextAttributes(
    LSA_SEC_HANDLE p1,
    ULONG p2,
    PVOID p3);

NTSTATUS NTAPI
UsrSpCompleteAuthToken(
    LSA_SEC_HANDLE p1,
    PSecBufferDesc p2);

NTSTATUS NTAPI
UsrSpDeleteUserModeContext(
    LSA_SEC_HANDLE p1);

NTSTATUS NTAPI
UsrSpFormatCredentials(
    PSecBuffer p1,
    PSecBuffer p2);

NTSTATUS NTAPI
UsrSpMarshallSupplementalCreds(
    ULONG p1,
    PUCHAR p2,
    PULONG p3,
    PVOID *p4);

NTSTATUS NTAPI
UsrSpExportSecurityContext(
    LSA_SEC_HANDLE p1,
    ULONG p2,
    PSecBuffer p3,
    PHANDLE p4);

NTSTATUS NTAPI
UsrSpImportSecurityContext(
    PSecBuffer p1,
    HANDLE p2,
    PLSA_SEC_HANDLE p3);

void _fdTRACE(
    IN const char *s,
    IN const char *file,
    IN const int line, ...);
#define fdTRACE(a,...) _fdTRACE(a,__FILE__,__LINE__ ,##__VA_ARGS__)

#endif /* _SPUSER_H_ */
