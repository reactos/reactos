/*
 * PROJECT:     ReactOS lsass_apitest
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     test functions from PSECPKG_FUNCTION_TABLE
 *              (returned by SpLsaModeInitialize)
 * COPYRIGHT:   Copyright 2020 Andreas Maier (staubim@quantentunnel.de)
 */

#include "precomp.h"

// globals
LUID g_LogonId = {0,0};

// standard heap with tag
#define HeapAllocLSA(h,f,s) HeapAllocTag(h,f,s, "LSA")
#define HeapFreeLSA(h,f,p) HeapFreeTag(h,f,p, "LSA")
#define HeapIsLSA(p) HeapIsTag(p, "LSA")
// private heap with tag
#define HeapAllocPVH(h,f,s) HeapAllocTag(h,f,s, "PVH")
#define HeapFreePVH(h,f,p) HeapFreeTag(h,f,p, "PVH")
#define HeapIsPVH(p) HeapIsTag(p, "PVH")

HANDLE hPrivateHeap;
HANDLE hProcessHeap;

void
LsaFnInit()
{
    hPrivateHeap = HeapCreate(0, 0, 0);
    hProcessHeap = GetProcessHeap();
}

void
LsaFnFini()
{
    if (hPrivateHeap != NULL)
        HeapDestroy(hPrivateHeap);
}

NTSTATUS NTAPI
LsaFnCreateLogonSession(
    _In_ PLUID LogonId)
{
    g_LogonId = *LogonId;
    trace("LsaFnCreateLogonSession\n");
    trace("LogonId 0x%p\n", LogonId);
    trace("LogonId 0x%lx 0x%lx\n", g_LogonId.LowPart, g_LogonId.HighPart);

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnDeleteLogonSession(PLUID p1)
{
    trace("LsaFnDeleteLogonSession\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnAddCredential(
    PLUID p1,
    ULONG p2,
    PLSA_STRING p3,
    PLSA_STRING p4)
{
    trace("LsaFnAddCredential\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnGetCredentials(
    PLUID p1,
    ULONG p2,
    PULONG p3,
    BOOLEAN p4,
    PLSA_STRING p5,
    PULONG p6,
    PLSA_STRING p7)
{
    trace("LsaFnGetCredentials\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnDeleteCredential(
    PLUID p1,
    ULONG p2,
    PLSA_STRING p3)
{
    trace("LsaFnDeleteCredential\n");
    return STATUS_SUCCESS;
}

PVOID NTAPI
LsaFnAllocateLsaHeap(
    ULONG Length)
{
    PVOID mem;
    trace("LsaFnAllocateLsaHeap (%ld)\n", Length);
    mem = HeapAllocLSA(hProcessHeap, HEAP_ZERO_MEMORY, Length);
    trace("< returning %p\n", mem);
    return mem;
}

VOID NTAPI
LsaFnFreeLsaHeap(
    PVOID Base)
{
    trace("LsaFnFreeLsaHeap 0x%p\n", Base);
    HeapFreeLSA(hProcessHeap, 0, Base);
}

NTSTATUS NTAPI
LsaFnAllocateClientBuffer(
    PLSA_CLIENT_REQUEST p1,
    ULONG p2,
    PVOID *p3)
{
    trace("LsaFnAllocateClientBuffer\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnFreeClientBuffer(
    PLSA_CLIENT_REQUEST p1,
    PVOID p2)
{
    trace("LsaFnFreeClientBuffer\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnCopyToClientBuffer(
    PLSA_CLIENT_REQUEST p1,
    ULONG p2,
    PVOID p3,
    PVOID p4)
{
    trace("LsaFnCopyToClientBuffer\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnCopyFromClientBuffer(
    PLSA_CLIENT_REQUEST p1,
    ULONG p2,
    PVOID p3,
    PVOID p4)
{
    trace("LsaFnCopyFromClientBuffer\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnImpersonateClient(void)
{
    trace("LsaFnImpersonateClient\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI LsaFnUnloadPackage(void)
{
    trace("LsaFnUnloadPackage\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnDuplicateHandle(
    HANDLE p1,
    PHANDLE p2)
{
    trace("LsaFnDuplicateHandle\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnSaveSupplementalCredentials(
    PLUID p1,
    ULONG p2,
    PVOID p3,
    BOOLEAN p4)
{
    trace("LsaFnSaveSupplementalCredentials\n");
    return STATUS_SUCCESS;
}

HANDLE NTAPI
LsaFnCreateThread(
    SEC_ATTRS p1,
    ULONG p2,
    SEC_THREAD_START p3,
    PVOID p4, ULONG p5,
    PULONG p6)
{
    trace("LsaFnCreateThread\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnGetClientInfo(
    PSECPKG_CLIENT_INFO ClientInfo)
{
    trace("LsaFnGetClientInfo 0x%p\n", ClientInfo);
    return STATUS_SUCCESS;
}

HANDLE NTAPI
LsaFnRegisterNotification(
    SEC_THREAD_START p1,
    PVOID p2,
    ULONG p3,
    ULONG p4,
    ULONG p5,
    ULONG p6,
    HANDLE p7)
{
    trace("LsaFnRegisterNotification\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnCancelNotification(
    HANDLE p1)
{
    trace("LsaFnCancelNotification\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnMapBuffer(
    PSecBuffer p1,
    PSecBuffer p2)
{
    trace("LsaFnMapBuffer\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnCreateToken(
    PLUID p1,
    PTOKEN_SOURCE p2,
    SECURITY_LOGON_TYPE p3,
    SECURITY_IMPERSONATION_LEVEL p4,
    LSA_TOKEN_INFORMATION_TYPE p5,
    PVOID p6,
    PTOKEN_GROUPS p7,
    PUNICODE_STRING p8,
    PUNICODE_STRING p9,
    PUNICODE_STRING p10,
    PUNICODE_STRING p11,
    PHANDLE p12,
    PNTSTATUS p13)
{
    trace("LsaFnCreateToken\n");
    return STATUS_SUCCESS;
}

VOID NTAPI
LsaFnAuditLogon(
    NTSTATUS p1,
    NTSTATUS p2,
    PUNICODE_STRING p3,
    PUNICODE_STRING p4,
    PUNICODE_STRING p5,
    OPTIONAL PSID p6,
    SECURITY_LOGON_TYPE p7,
    PTOKEN_SOURCE p8,
    PLUID p9)
{
    trace("LsaFnAuditLogon\n");
}

NTSTATUS NTAPI
LsaFnCallPackage(
    PUNICODE_STRING p1,
    PVOID p2,
    ULONG p3,
    PVOID *p4,
    PULONG p5,
    PNTSTATUS p6)
{
    trace("LsaFnCallPackage\n");
    return STATUS_SUCCESS;
}

VOID NTAPI
LsaFnFreeReturnBuffer(
    PVOID p1)
{
    trace("LsaFnFreeReturnBuffer\n");
}

BOOLEAN NTAPI
LsaFnGetCallInfo(
    PSECPKG_CALL_INFO Info)
{
    trace("LsaFnGetCallInfo 0x%p\n", Info);
    return TRUE;
}

NTSTATUS NTAPI
LsaFnCallPackageEx(
    PUNICODE_STRING p1,
    PVOID p2,
    PVOID p3,
    ULONG p4,
    PVOID *p5,
    PULONG p6,
    PNTSTATUS p7)

{
    trace("LsaFnCallPackageEx\n");
    return STATUS_SUCCESS;
}

PVOID NTAPI
LsaFnCreateSharedMemory(
    ULONG p1,
    ULONG p2)
{
    trace("LsaFnCreateSharedMemory\n");
    return STATUS_SUCCESS;
}

PVOID NTAPI
LsaFnAllocateSharedMemory(
    PVOID p1,
    ULONG p2)
{
    trace("LsaFnAllocateSharedMemory\n");
    return NULL;
}

VOID NTAPI
LsaFnFreeSharedMemory(
    PVOID p1,
    PVOID p2)
{
    trace("LsaFnFreeSharedMemory\n");
}

BOOLEAN NTAPI
LsaFnDeleteSharedMemory(
    PVOID p1)
{
    trace("LsaFnDeleteSharedMemory\n");
    return FALSE;
}

NTSTATUS NTAPI
LsaFnOpenSamUser(
    PUNICODE_STRING p1,
    SECPKG_NAME_TYPE p2,
    PUNICODE_STRING p3,
    BOOLEAN p4,
    ULONG p5,
    PVOID* p6)
{
    trace("LsaFnOpenSamUser\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnGetUserCredentials(
    PVOID p1,
    PVOID *p2,
    PULONG p3,
    PVOID *p4,
    PULONG p5)
{
    trace("LsaFnGetUserCredentials\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnGetUserAuthData(
    PVOID p1,
    PUCHAR *p2,
    PULONG p3)
{
    trace("LsaFnGetUserAuthData\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnCloseSamUser(
    PVOID p1)
{
    trace("LsaFnCloseSamUser\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnConvertAuthDataToToken(
    PVOID p1,
    ULONG p2,
    SECURITY_IMPERSONATION_LEVEL p3,
    PTOKEN_SOURCE p4,
    SECURITY_LOGON_TYPE p5,
    PUNICODE_STRING p6,
    PHANDLE p7,
    PLUID p8,
    PUNICODE_STRING p9,
    PNTSTATUS p10)
{
    trace("LsaFnConvertAuthDataToToken\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnClientCallback(
    PCHAR p1,
    ULONG_PTR p2,
    ULONG_PTR p3,
    PSecBuffer p4,
    PSecBuffer p5)
{
    trace("LsaFnClientCallback\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnUpdateCredentials(
    PSECPKG_PRIMARY_CRED p1,
    PSECPKG_SUPPLEMENTAL_CRED_ARRAY p2)
{
    trace("LsaFnUpdateCredentials\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnGetAuthDataForUser(
    PUNICODE_STRING p1,
    SECPKG_NAME_TYPE p2,
    PUNICODE_STRING p3,
    PUCHAR *p4,
    PULONG p5,
    PUNICODE_STRING p6)
{
    trace("LsaFnGetAuthDataForUser\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnCrackSingleName(
    ULONG p1,
    BOOLEAN p2,
    PUNICODE_STRING p3,
    PUNICODE_STRING p4,
    ULONG p5,
    PUNICODE_STRING p6,
    PUNICODE_STRING p7,
    PULONG p8)
{
    trace("LsaFnCrackSingleName\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnAuditAccountLogon(
    ULONG p1,
    BOOLEAN p2,
    PUNICODE_STRING p3,
    PUNICODE_STRING p4,
    PUNICODE_STRING p5,
    NTSTATUS p6)
{
    trace("LsaFnAuditAccountLogon\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnCallPackagePassthrough(
    PUNICODE_STRING p1,
    PVOID p2,
    PVOID p3,
    ULONG p4,
    PVOID*p5,
    PULONG p6,
    PNTSTATUS p7)
{
    trace("LsaFnCallPackagePassthrough\n");
    return STATUS_SUCCESS;
}

void NTAPI
LsaFnDummyFunctionX(
    PVOID p1,
    ULONG p2)
{
    trace("LsaFnDummyFunctionX\n");
}

void NTAPI
LsaFnProtectMemory(
    PVOID Buffer,
    ULONG BufferSize)
{
    trace("LsaFnProtectMemory\n");
}

void NTAPI
LsaFnUnprotectMemory(
    PVOID Buffer,
    ULONG BufferSize)
{
    trace("LsaFnUnprotectMemory\n");
    trace("  BufferSize %ld\n", BufferSize);
    PrintHexDump(BufferSize, Buffer);
}

NTSTATUS NTAPI
LsaFnOpenTokenByLogonId(
    _In_ PLUID LogonId,
    _Out_ HANDLE *RetTokenHandle)
{
    trace("LsaFnOpenTokenByLogonId\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
LsaFnExpandAuthDataForDomain(
    _In_  PUCHAR UserAuthData,
    _In_  ULONG UserAuthDataSize,
    _In_  PVOID Reserved,
    _Out_ PUCHAR *ExpandedAuthData,
    _In_  PULONG ExpandedAuthDataSize)
{
    trace("LsaFnExpandAuthDataForDomain\n");
    return STATUS_SUCCESS;
}

PVOID NTAPI
LsaFnAllocatePrivateHeap(
    _In_ SIZE_T Length)
{
    PVOID mem;
    trace("LsaFnAllocatePrivateHeap (%ld)\n", Length);
    mem = HeapAllocPVH(hPrivateHeap, HEAP_ZERO_MEMORY, Length);
    trace(" returning %p\n", mem);
    return mem;
}

void NTAPI LsaFnFreePrivateHeap(
    _In_ PVOID Base)
{
    trace("LsaFnFreePrivateHeap 0x%p\n", Base);
    HeapFreePVH(hPrivateHeap, 0, Base);
}
