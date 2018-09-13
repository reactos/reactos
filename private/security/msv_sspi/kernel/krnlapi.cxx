//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1999
//
// File:        krnlapi.cxx
//
// Contents:    Kernel-mode APIs to the NTLM package
//
//
// History:     07-Sep-1996   Created         ChandanS
//
//------------------------------------------------------------------------

#include <ntlmkrnl.h>
extern "C"
{
#include <cryptdll.h>
}

#include "crc32.h"  // How to use crc32

extern "C"
{
#include <rc4.h>    // How to use RC4 routine
#include <md5.h>
#include <hmac.h>
}


// Context Signatures

#define NTLM_CONTEXT_SIGNATURE 'MLTN'
#define NTLM_CONTEXT_DELETED_SIGNATURE 'XXXX'

// Keep this is sync with NTLM_KERNEL_CONTEXT defined in
// security\msv_sspi\userapi.cxx

typedef struct _NTLM_KERNEL_CONTEXT{
    KSEC_LIST_ENTRY      List;
    ULONG_PTR            LsaContext;
    ULONG                NegotiateFlags;
    HANDLE               ClientTokenHandle;
    PACCESS_TOKEN        AccessToken;

    PULONG                  pSendNonce;      // ptr to nonce to use for send
    PULONG                  pRecvNonce;      // ptr to nonce to use for receive
    struct RC4_KEYSTRUCT *  pSealRc4Sched;   // ptr to key sched used for Seal
    struct RC4_KEYSTRUCT *  pUnsealRc4Sched; // ptr to key sched used to Unseal

    ULONG                   SendNonce;
    ULONG                   RecvNonce;
    LPWSTR               ContextNames;
    UCHAR                SessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    ULONG                ContextSignature;
    ULONG                References ;
    TimeStamp            PasswordExpiry;
    ULONG                UserFlags;
    UCHAR                   SignSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    UCHAR                   VerifySessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    UCHAR                   SealSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    UCHAR                   UnsealSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];

    ULONG64                 Pad1;           // pad keystructs to 64.
    struct RC4_KEYSTRUCT    SealRc4Sched;   // key struct used for Seal
    ULONG64                 Pad2;           // pad keystructs to 64.
    struct RC4_KEYSTRUCT    UnsealRc4Sched; // key struct used to Unseal
} NTLM_KERNEL_CONTEXT, * PNTLM_KERNEL_CONTEXT;

#define CSSEALMAGIC "session key to client-to-server sealing key magic constant"
#define SCSEALMAGIC "session key to server-to-client sealing key magic constant"
#define CSSIGNMAGIC "session key to client-to-server signing key magic constant"
#define SCSIGNMAGIC "session key to server-to-client signing key magic constant"

typedef enum _eSignSealOp {
    eSign,      // MakeSignature is calling
    eVerify,    // VerifySignature is calling
    eSeal,      // SealMessage is calling
    eUnseal     // UnsealMessage is calling
} eSignSealOp;


//
// Make these extern "C" to allow them to be pageable.
//

extern "C"
{
KspInitPackageFn       NtLmInitKernelPackage;
KspDeleteContextFn     NtLmDeleteKernelContext;
KspInitContextFn       NtLmInitKernelContext;
KspMapHandleFn         NtLmMapKernelHandle;
KspMakeSignatureFn     NtLmMakeSignature;
KspVerifySignatureFn   NtLmVerifySignature;
KspSealMessageFn       NtLmSealMessage;
KspUnsealMessageFn     NtLmUnsealMessage;
KspGetTokenFn          NtLmGetContextToken;
KspQueryAttributesFn   NtLmQueryContextAttributes;
KspCompleteTokenFn     NtLmCompleteToken;
SpExportSecurityContextFn NtLmExportSecurityContext;
SpImportSecurityContextFn NtLmImportSecurityContext;
KspSetPagingModeFn     NtlmSetPagingMode ;

//
// Local prototypes:
//

NTSTATUS
NtLmCreateKernelModeContext(
    IN ULONG ContextHandle,
    IN PSecBuffer MarshalledContext,
    OUT PNTLM_KERNEL_CONTEXT * NewContext
    );

NTSTATUS
NtLmMakePackedContext(
    IN PNTLM_KERNEL_CONTEXT Context,
    OUT PBOOLEAN MappedContext,
    OUT PSecBuffer ContextData,
    IN ULONG Flags
    );

NTSTATUS
NtlmFreeKernelContext (
    PNTLM_KERNEL_CONTEXT KernelContext
    );

#define NtlmReferenceContext( Context, Remove ) \
            KSecReferenceListEntry( (PKSEC_LIST_ENTRY) Context, \
                                    NTLM_CONTEXT_SIGNATURE, \
                                    Remove )

VOID
NtlmDerefContext(
    PNTLM_KERNEL_CONTEXT Context
    );

void
SspGenCheckSum(
    IN  PSecBuffer  pMessage,
    OUT PNTLMSSP_MESSAGE_SIGNATURE  pSig
    );

VOID
SspEncryptBuffer(
    IN PNTLM_KERNEL_CONTEXT pContext,
    IN struct RC4_KEYSTRUCT * pRc4Key,
    IN ULONG BufferSize,
    IN OUT PVOID Buffer
    );

VOID
SspRc4Key(
    IN ULONG                NegotiateFlags,
    OUT struct RC4_KEYSTRUCT *pRc4Key,
    IN PUCHAR               pSessionKey
    );

SECURITY_STATUS
SspSignSealHelper(
    IN PNTLM_KERNEL_CONTEXT pContext,
    IN eSignSealOp Op,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    OUT PNTLMSSP_MESSAGE_SIGNATURE pSig,
    OUT PNTLMSSP_MESSAGE_SIGNATURE * ppSig
    );

} // extern "C"




#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtLmInitKernelPackage)
#pragma alloc_text(PAGE, NtLmDeleteKernelContext)
#pragma alloc_text(PAGE, NtLmInitKernelContext)
#pragma alloc_text(PAGE, NtLmMapKernelHandle)
#pragma alloc_text(PAGEMSG, NtLmMakeSignature)
#pragma alloc_text(PAGEMSG, NtLmVerifySignature)
#pragma alloc_text(PAGEMSG, NtLmSealMessage)
#pragma alloc_text(PAGEMSG, NtLmUnsealMessage)
#pragma alloc_text(PAGEMSG, NtLmGetContextToken)
#pragma alloc_text(PAGEMSG, NtLmQueryContextAttributes)
#pragma alloc_text(PAGEMSG, NtlmDerefContext )
#pragma alloc_text(PAGE, NtLmCompleteToken)
#pragma alloc_text(PAGE, NtLmExportSecurityContext)
#pragma alloc_text(PAGE, NtLmImportSecurityContext)
#pragma alloc_text(PAGEMSG, NtlmFreeKernelContext )

#pragma alloc_text(PAGE, NtLmCreateKernelModeContext )
#pragma alloc_text(PAGE, NtLmMakePackedContext )

#pragma alloc_text(PAGEMSG, SspGenCheckSum)
#pragma alloc_text(PAGEMSG, SspEncryptBuffer)
#pragma alloc_text(PAGE, SspRc4Key)
#pragma alloc_text(PAGEMSG, SspSignSealHelper)

#endif

SECPKG_KERNEL_FUNCTION_TABLE NtLmFunctionTable = {
    NtLmInitKernelPackage,
    NtLmDeleteKernelContext,
    NtLmInitKernelContext,
    NtLmMapKernelHandle,
    NtLmMakeSignature,
    NtLmVerifySignature,
    NtLmSealMessage,
    NtLmUnsealMessage,
    NtLmGetContextToken,
    NtLmQueryContextAttributes,
    NtLmCompleteToken,
    NtLmExportSecurityContext,
    NtLmImportSecurityContext,
    NtlmSetPagingMode
};

LIST_ENTRY NtLmKernelContextList;
ERESOURCE  NtLmKernelContextLock;
PSECPKG_KERNEL_FUNCTIONS LsaKernelFunctions;
POOL_TYPE NtlmPoolType ;
PVOID NtlmPagedList ;
PVOID NtlmNonPagedList ;
PVOID NtlmActiveList ;

#define MAYBE_PAGED_CODE() \
    if ( NtlmPoolType == PagedPool )    \
    {                                   \
        PAGED_CODE();                   \
    }



//+-------------------------------------------------------------------------
//
//  Function:   FreeKernelContext
//
//  Synopsis:   frees alloced pointers in this context and
//              then frees the context
//
//  Arguments:  KernelContext  - the unlinked kernel context
//
//  Returns:    STATUS_SUCCESS on success
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS
NtlmFreeKernelContext (
    PNTLM_KERNEL_CONTEXT KernelContext
    )
{

    NTSTATUS Status = STATUS_SUCCESS;

    MAYBE_PAGED_CODE();

    DebugLog(( DEB_TRACE, "Entering FreeKernelContext\n" ));

    if (KernelContext->ContextNames != NULL)
    {
        NtLmFree (KernelContext->ContextNames);
    }

    if (KernelContext->ClientTokenHandle != NULL)
    {
        NTSTATUS IgnoreStatus;
        IgnoreStatus = NtClose(KernelContext->ClientTokenHandle);
        ASSERT (NT_SUCCESS (IgnoreStatus));
    }

    if (KernelContext->AccessToken != NULL)
    {
        ObDereferenceObject (KernelContext->AccessToken);
    }

    DebugLog(( DEB_TRACE, "Deleting Context 0x%lx\n", KernelContext));

    NtLmFree (KernelContext);

    DebugLog(( DEB_TRACE, "Leaving FreeKernelContext: 0x%lx\n", Status ));

    return Status;
}

//+---------------------------------------------------------------------------
//
//  Function:   NtlmDerefContext
//
//  Synopsis:   Dereference a kernel context
//
//  Arguments:  [Context] --
//
//  History:    7-07-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
NtlmDerefContext(
    PNTLM_KERNEL_CONTEXT Context
    )
{
    BOOLEAN Delete ;

    MAYBE_PAGED_CODE();

    KSecDereferenceListEntry(
                    &Context->List,
                    &Delete );

    if ( Delete )
    {
        NtlmFreeKernelContext( Context );
    }

}

//+-------------------------------------------------------------------------
//
//  Function:   NtLmInitKernelPackage
//
//  Synopsis:   Initialize an instance of the NtLm package in
//              a client's (kernel) address space
//
//  Arguments:  None
//
//  Returns:    STATUS_SUCCESS or
//              returns from ExInitializeResource
//
//  Notes:      we do what was done in SpInstanceInit()
//              from security\msv_sspi\userapi.cxx
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
NtLmInitKernelPackage(
    IN PSECPKG_KERNEL_FUNCTIONS KernelFunctions
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    DebugLog(( DEB_TRACE, "Entering NtLmInitKernelPackage\n" ));

    LsaKernelFunctions = KernelFunctions;
    //
    // Set up Context list support:
    //

    NtlmPoolType = PagedPool ;
    NtlmPagedList = LsaKernelFunctions->CreateContextList( KSecPaged );
    NtlmActiveList = NtlmPagedList ;

    InitializeListHead (&NtLmKernelContextList);

    // BUGBUG When do we delete the resource?
    Status = ExInitializeResource(&NtLmKernelContextLock);

    if (!NT_SUCCESS(Status))
    {
        DebugLog(( DEB_ERROR,
          "Failed to initialize resource NtlmKernelContextLock, ret 0x%lx\n",
          Status ));
    }

    DebugLog(( DEB_TRACE, "Leaving NtLmInitKernelPackage 0x%lx\n", Status ));
    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   NtLmDeleteKernelContext
//
//  Synopsis:   Deletes a kernel mode context by unlinking it and then
//              dereferencing it.
//
//  Effects:
//
//  Arguments:  KernelContextHandle - Kernel context handle of the context to delete
//              LsaContextHandle    - The Lsa mode handle
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success, STATUS_INVALID_HANDLE if the
//              context can't be located
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
NtLmDeleteKernelContext(
    IN ULONG_PTR KernelContextHandle,
    OUT PULONG_PTR LsaContextHandle
    )
{

    PNTLM_KERNEL_CONTEXT pContext = NULL;
    NTSTATUS Status = STATUS_SUCCESS, SaveStatus = STATUS_SUCCESS;
    BOOLEAN Delete ;

    PAGED_CODE();

    DebugLog(( DEB_TRACE, "Entering NtLmDeleteKernelContext\n" ));


    Status = NtlmReferenceContext( KernelContextHandle, TRUE );

    if ( NT_SUCCESS( Status ) )
    {
        pContext = (PNTLM_KERNEL_CONTEXT) KernelContextHandle ;

    }
    else
    {
        *LsaContextHandle = KernelContextHandle;
        DebugLog(( DEB_ERROR,
          "Bad kernel context 0x%lx\n", KernelContextHandle));
        goto CleanUp;

    }

    *LsaContextHandle = pContext->LsaContext;
    if ((pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_EXPORTED_CONTEXT) != 0)
    {
        // Ignore all other errors and pass back
        SaveStatus = SEC_I_NO_LSA_CONTEXT;
    }


CleanUp:


    if (pContext != NULL)
    {
        NtlmDerefContext( pContext );

    }

    if (SaveStatus == SEC_I_NO_LSA_CONTEXT)
    {
        Status = SaveStatus;
    }

    DebugLog(( DEB_TRACE, "Leaving NtLmDeleteKernelContext 0x%lx\n", Status ));
    return(Status);
}





//+-------------------------------------------------------------------------
//
//  Function:   NtLmInitKernelContext
//
//  Synopsis:   Creates a kernel-mode context from a packed LSA mode context
//
//  Arguments:  LsaContextHandle - Lsa mode context handle for the context
//              PackedContext - A marshalled buffer containing the LSA
//                  mode context.
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES
//
//  Notes:
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
NtLmInitKernelContext(
    IN ULONG_PTR LsaContextHandle,
    IN PSecBuffer PackedContext,
    OUT PULONG_PTR NewContextHandle
    )
{

    NTSTATUS Status = STATUS_SUCCESS;
    PNTLM_KERNEL_CONTEXT pContext = NULL;
    unsigned int Length = 0;

    PNTLM_KERNEL_CONTEXT pTmpContext  = (PNTLM_KERNEL_CONTEXT) PackedContext->pvBuffer;

    PAGED_CODE();

    DebugLog(( DEB_TRACE, "Entering NtLmInitKernelContext\n" ));

    *NewContextHandle = NULL;

    if (PackedContext->cbBuffer < sizeof(NTLM_KERNEL_CONTEXT))
    {
        Status = STATUS_INVALID_PARAMETER;
        DebugLog(( DEB_ERROR,
          "Bad size of Packed context 0x%lx\n", PackedContext->cbBuffer));
        goto Cleanup;
    }

    pContext = (PNTLM_KERNEL_CONTEXT) NtLmAllocate( sizeof(NTLM_KERNEL_CONTEXT) );

    if (!pContext)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        DebugLog(( DEB_ERROR, "Allocation error for pContext\n"));
        goto Cleanup;
    }

    RtlZeroMemory(
        pContext,
        sizeof(NTLM_KERNEL_CONTEXT)
        );

    KsecInitializeListEntry( &pContext->List, NTLM_CONTEXT_SIGNATURE );

    // Copy contents of PackedContext->pvBuffer to pContext

    pContext->ClientTokenHandle = pTmpContext->ClientTokenHandle;
    pContext->LsaContext = LsaContextHandle;
    pContext->NegotiateFlags = pTmpContext->NegotiateFlags;

    //
    // keep all 128 bits here, so signing can be strong even if encrypt can't be
    //

    RtlCopyMemory(  pContext->SessionKey,
                        pTmpContext->SessionKey,
                        MSV1_0_USER_SESSION_KEY_LENGTH);

    //
    // if doing full duplex as part of NTLM2, generate different sign
    // and seal keys for each direction
    //  all we do is MD5 the base session key with a different magic constant
    //

    if ( pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2 ) {
        MD5_CTX Md5Context;
        ULONG KeyLen;

        ASSERT(MD5DIGESTLEN == MSV1_0_USER_SESSION_KEY_LENGTH);

        if( pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_128 )
            KeyLen = 16;
        else if( pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_56 )
            KeyLen = 7;
        else
            KeyLen = 5;

//        DebugLog(( SSP_SESSION_KEYS, "NTLMv2 session key size: %lu\n", KeyLen));

        //
        // make client to server encryption key
        //

        MD5Init(&Md5Context);
        MD5Update(&Md5Context, pContext->SessionKey, KeyLen);
        MD5Update(&Md5Context, (unsigned char*)CSSEALMAGIC, sizeof(CSSEALMAGIC));
        MD5Final(&Md5Context);

        //
        // if TokenHandle == NULL, this is the client side
        //  put key in the right place: for client it's seal, for server it's unseal
        //

        if (pContext->ClientTokenHandle == NULL)
            RtlCopyMemory(pContext->SealSessionKey, Md5Context.digest, MSV1_0_USER_SESSION_KEY_LENGTH);
        else
            RtlCopyMemory(pContext->UnsealSessionKey, Md5Context.digest, MSV1_0_USER_SESSION_KEY_LENGTH);

        //
        // make server to client encryption key
        //

        MD5Init(&Md5Context);
        MD5Update(&Md5Context, pContext->SessionKey, KeyLen);
        MD5Update(&Md5Context, (unsigned char*)SCSEALMAGIC, sizeof(SCSEALMAGIC));
        MD5Final(&Md5Context);
        ASSERT(MD5DIGESTLEN == MSV1_0_USER_SESSION_KEY_LENGTH);
        if (pContext->ClientTokenHandle == NULL)
            RtlCopyMemory(pContext->UnsealSessionKey, Md5Context.digest, MSV1_0_USER_SESSION_KEY_LENGTH);
        else
            RtlCopyMemory(pContext->SealSessionKey, Md5Context.digest, MSV1_0_USER_SESSION_KEY_LENGTH);

        //
        // make client to server signing key -- always 128 bits!
        //

        MD5Init(&Md5Context);
        MD5Update(&Md5Context, pContext->SessionKey, MSV1_0_USER_SESSION_KEY_LENGTH);
        MD5Update(&Md5Context, (unsigned char*)CSSIGNMAGIC, sizeof(CSSIGNMAGIC));
        MD5Final(&Md5Context);
        if (pContext->ClientTokenHandle == NULL)
            RtlCopyMemory(pContext->SignSessionKey, Md5Context.digest, MSV1_0_USER_SESSION_KEY_LENGTH);
        else
            RtlCopyMemory(pContext->VerifySessionKey, Md5Context.digest, MSV1_0_USER_SESSION_KEY_LENGTH);

        //
        // make server to client signing key
        //

        MD5Init(&Md5Context);
        MD5Update(&Md5Context, pContext->SessionKey, MSV1_0_USER_SESSION_KEY_LENGTH);
        MD5Update(&Md5Context, (unsigned char*)SCSIGNMAGIC, sizeof(SCSIGNMAGIC));
        MD5Final(&Md5Context);
        if (pContext->ClientTokenHandle == NULL)
            RtlCopyMemory(pContext->VerifySessionKey, Md5Context.digest, MSV1_0_USER_SESSION_KEY_LENGTH);
        else
            RtlCopyMemory(pContext->SignSessionKey, Md5Context.digest, MSV1_0_USER_SESSION_KEY_LENGTH);

        //
        // set pointers to different key schedule and nonce for each direction
        //  key schedule will be filled in later...
        //

        pContext->pSealRc4Sched = &pContext->SealRc4Sched;
        pContext->pUnsealRc4Sched = &pContext->UnsealRc4Sched;
        pContext->pSendNonce = &pContext->SendNonce;
        pContext->pRecvNonce = &pContext->RecvNonce;
   } else {

        //
        // just copy session key to all four keys
        //  leave them 128 bits -- they get cut to 40 bits later
        //

        RtlCopyMemory(  pContext->SealSessionKey,
                        pContext->SessionKey,
                        MSV1_0_USER_SESSION_KEY_LENGTH);
        RtlCopyMemory(  pContext->UnsealSessionKey,
                        pContext->SessionKey,
                        MSV1_0_USER_SESSION_KEY_LENGTH);
        RtlCopyMemory(  pContext->SignSessionKey,
                        pContext->SessionKey,
                        MSV1_0_USER_SESSION_KEY_LENGTH);
        RtlCopyMemory(  pContext->VerifySessionKey,
                        pContext->SessionKey,
                        MSV1_0_USER_SESSION_KEY_LENGTH);

        //
        // set pointers to share a key schedule and nonce for each direction
        //  (OK because half duplex!)
        //

        pContext->pSealRc4Sched = &pContext->SealRc4Sched;
        pContext->pUnsealRc4Sched = &pContext->SealRc4Sched;
        pContext->pSendNonce = &pContext->SendNonce;
        pContext->pRecvNonce = &pContext->SendNonce;
    }


    Length = (unsigned int) (PackedContext->cbBuffer -
                             sizeof(NTLM_KERNEL_CONTEXT));

    if (Length == 0)
    {
        //There's no string after the NTLM_KERNEL_CONTEXT struct
        pContext->ContextNames = NULL;
    }
    else
    {
        pContext->ContextNames = (LPWSTR) NtLmAllocate(Length + sizeof(WCHAR));

        if (!pContext->ContextNames)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        RtlCopyMemory(pContext->ContextNames,
                      pTmpContext + 1,
                      Length );

        // null terminate the string

        *(pContext->ContextNames + (Length/sizeof(WCHAR))) = L'\0';
    }

    pContext->SendNonce = pTmpContext->SendNonce;
    pContext->RecvNonce = pTmpContext->RecvNonce;

    SspRc4Key(pContext->NegotiateFlags, &pContext->SealRc4Sched, pContext->SealSessionKey);
    SspRc4Key(pContext->NegotiateFlags, &pContext->UnsealRc4Sched, pContext->UnsealSessionKey);


    pContext->PasswordExpiry = pTmpContext->PasswordExpiry;
    pContext->UserFlags = pTmpContext->UserFlags;

    KSecInsertListEntry(
            NtlmActiveList,
            &pContext->List );

    NtlmDerefContext( pContext );

    *NewContextHandle = (ULONG_PTR) pContext;

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        if (pContext != NULL)
        {
            NtlmFreeKernelContext( pContext );
        }
    }

    if (PackedContext->pvBuffer != NULL)
    {
        LsaKernelFunctions->FreeHeap(PackedContext->pvBuffer);
        PackedContext->pvBuffer = NULL;
    }

    DebugLog(( DEB_TRACE, "Leaving NtLmInitKernelContext 0x%lx\n", Status ));
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   NtLmMapKernelHandle
//
//  Synopsis:   Maps a kernel handle into an LSA handle
//
//  Arguments:  KernelContextHandle - Kernel context handle of the context to map
//              LsaContextHandle - Receives LSA context handle of the context
//                      to map
//
//  Returns:    STATUS_SUCCESS on success
//
//  Notes:
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
NtLmMapKernelHandle(
    IN ULONG_PTR KernelContextHandle,
    OUT PULONG_PTR LsaContextHandle
    )
{

    NTSTATUS Status = STATUS_SUCCESS;
    PNTLM_KERNEL_CONTEXT Context = NULL;

    PAGED_CODE();

    DebugLog((DEB_TRACE,"Entering NtLmMapKernelhandle\n"));

    Status = NtlmReferenceContext( KernelContextHandle, FALSE );

    if ( NT_SUCCESS( Status ) )
    {
        Context = (PNTLM_KERNEL_CONTEXT) KernelContextHandle ;

        *LsaContextHandle = Context->LsaContext ;

        NtlmDerefContext( Context );

    }
    else
    {
        DebugLog(( DEB_WARN, "Invalid context handle - %x\n",
                    KernelContextHandle ));
        *LsaContextHandle = KernelContextHandle ;
    }

    DebugLog((DEB_TRACE,"Leaving NtLmMapKernelhandle 0x%lx\n", Status));

    return (Status);
}


//
// Bogus add-shift check sum
//

void
SspGenCheckSum(
    IN  PSecBuffer  pMessage,
    OUT PNTLMSSP_MESSAGE_SIGNATURE  pSig
    )

/*++

RoutineDescription:

    Generate a crc-32 checksum for a buffer

Arguments:

Return Value:
Notes: This was stolen from net\svcdlls\ntlmssp\client\sign.c ,
       routine SspGenCheckSum. It's possible that
       bugs got copied too

--*/

{
    MAYBE_PAGED_CODE();

    Crc32(pSig->CheckSum,pMessage->cbBuffer,pMessage->pvBuffer,&pSig->CheckSum);
}


VOID
SspEncryptBuffer(
    IN PNTLM_KERNEL_CONTEXT pContext,
    IN struct RC4_KEYSTRUCT * pRc4Key,
    IN ULONG BufferSize,
    IN OUT PVOID Buffer
    )

/*++

RoutineDescription:

    Encrypts a buffer with the RC4 key in the context.  If the context
    is for a datagram session, then the key is copied before being used
    to encrypt the buffer.

Arguments:

    pContext - Context containing the key to encrypt the data

    BufferSize - Length of buffer in bytes

    Buffer - Buffer to encrypt.
    Notes: This was stolen from net\svcdlls\ntlmssp\client\sign.c ,
           routine SspEncryptBuffer. It's possible that
           bugs got copied too

Return Value:

--*/

{
    MAYBE_PAGED_CODE();

    struct RC4_KEYSTRUCT TemporaryKey;
    struct RC4_KEYSTRUCT * EncryptionKey = pRc4Key;

    if (BufferSize == 0)
    {
        return;
    }

    //
    // For datagram (application supplied sequence numbers) before NTLM2
    // we used to copy the key before encrypting so we don't
    // have a changing key; but that reused the key stream. Now we only
    // do that when backwards compatibility is explicitly called for.
    //

    if (((pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM) != 0) &&
        ((pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2) == 0) ) {

        RtlCopyMemory(
            &TemporaryKey,
            EncryptionKey,
            sizeof(TemporaryKey)
            );
        EncryptionKey = &TemporaryKey;

    }

    rc4(
        EncryptionKey,
        BufferSize,
        (PUCHAR) Buffer
        );

}


VOID
SspRc4Key(
    IN ULONG                NegotiateFlags,
    OUT struct RC4_KEYSTRUCT *pRc4Key,
    IN PUCHAR               pSessionKey
    )
/*++

RoutineDescription:

    Create an RC4 key schedule, making sure key length is OK for export

Arguments:

    NegotiateFlags  negotiate feature flags; NTLM2 bit is only one looked at
    pRc4Key         pointer to RC4 key schedule structure; filled in by this routine
    pSessionKey     pointer to session key -- must be full 16 bytes

Return Value:

--*/
{
    PAGED_CODE();

    //
    // For NTLM2, effective length was already cut down
    //

    if ((NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2) != 0) {

        rc4_key(pRc4Key, MSV1_0_USER_SESSION_KEY_LENGTH, pSessionKey);

    } else if( NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY ) {
        UCHAR Key[MSV1_0_LANMAN_SESSION_KEY_LENGTH];
        ULONG KeyLen;

        ASSERT(MSV1_0_LANMAN_SESSION_KEY_LENGTH == 8);

        // prior to Win2k, negotiated key strength had no bearing on
        // key size.  So, to allow proper interop to NT4, we don't
        // worry about 128bit.  56bit and 40bit are the only supported options.
        // 56bit is enabled because this was introduced in Win2k, and
        // Win2k -> Win2k interops correctly.
        //
#if 0
        if( NegotiateFlags & NTLMSSP_NEGOTIATE_128 ) {
            KeyLen = 8;

        } else
#endif
        if( NegotiateFlags & NTLMSSP_NEGOTIATE_56 ) {
            KeyLen = 7;

            //
            // Put a well-known salt at the end of the key to
            // limit the changing part to 56 bits.
            //

            Key[7] = 0xa0;
        } else {
            KeyLen = 5;

            //
            // Put a well-known salt at the end of the key to
            // limit the changing part to 40 bits.
            //

            Key[5] = 0xe5;
            Key[6] = 0x38;
            Key[7] = 0xb0;
        }

///        DebugLog(( SSP_SESSION_KEYS, "Non NTLMv2 session key size: %lu\n", KeyLen));

        RtlCopyMemory(Key,pSessionKey,KeyLen);

        rc4_key(pRc4Key, MSV1_0_LANMAN_SESSION_KEY_LENGTH, Key);
    } else {
        rc4_key(pRc4Key, MSV1_0_USER_SESSION_KEY_LENGTH, pSessionKey);
    }
}



SECURITY_STATUS
SspSignSealHelper(
    IN PNTLM_KERNEL_CONTEXT pContext,
    IN eSignSealOp Op,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    OUT PNTLMSSP_MESSAGE_SIGNATURE pSig,
    OUT PNTLMSSP_MESSAGE_SIGNATURE * ppSig
    )
/*++

RoutineDescription:

    Handle signing a message

Arguments:

Return Value:

--*/

{

    HMACMD5_CTX HMACMD5Context;
    UCHAR TempSig[MD5DIGESTLEN];
    NTLMSSP_MESSAGE_SIGNATURE  Sig;
    int Signature;
    ULONG i;
    PUCHAR pKey;                            // ptr to key to use for encryption
    PUCHAR pSignKey;                        // ptr to key to use for signing
    PULONG pNonce;                          // ptr to nonce to use
    struct RC4_KEYSTRUCT * pRc4Sched;       // ptr to key schedule to use


    MAYBE_PAGED_CODE();



    Signature = -1;
    for (i = 0; i < pMessage->cBuffers; i++)
    {
        if ((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_TOKEN)
        {
            Signature = i;
            break;
        }
    }
    if (Signature == -1)
    {
        return(SEC_E_INVALID_TOKEN);
    }

    if (pMessage->pBuffers[Signature].cbBuffer < NTLMSSP_MESSAGE_SIGNATURE_SIZE)
    {
        return(SEC_E_INVALID_TOKEN);
    }

    *ppSig = (NTLMSSP_MESSAGE_SIGNATURE*)pMessage->pBuffers[Signature].pvBuffer;

    //
    // If sequence detect wasn't requested, put on an empty
    // security token . Don't do the check if Seal/Unseal is called.
    //

    if (!(pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN) &&
       (Op == eSign || Op == eVerify))
    {
        RtlZeroMemory(pSig,NTLMSSP_MESSAGE_SIGNATURE_SIZE);
        pSig->Version = NTLM_SIGN_VERSION;
        return(SEC_E_OK);
    }

    // figure out which key, key schedule, and nonce to use
    //  depends on the op. SspAddLocalContext set up so that code on client
    //  and server just (un)seals with (un)seal key or key schedule, etc.
    //  and also sets pointers to share sending/receiving key schedule/nonce
    //  when in half duplex mode. Hence, this code gets to act as if it were
    //  always in full duplex mode.
    switch (Op) {
    case eSeal:
        pSignKey = pContext->SignSessionKey;    // if NTLM2
        pKey = pContext->SealSessionKey;
        pRc4Sched = pContext->pSealRc4Sched;
        pNonce = pContext->pSendNonce;
        break;
    case eUnseal:
        pSignKey = pContext->VerifySessionKey;  // if NTLM2
        pKey = pContext->UnsealSessionKey;
        pRc4Sched = pContext->pUnsealRc4Sched;
        pNonce = pContext->pRecvNonce;
        break;
    case eSign:
        pSignKey = pContext->SignSessionKey;    // if NTLM2
        pKey = pContext->SealSessionKey;        // might be used to encrypt the signature
        pRc4Sched = pContext->pSealRc4Sched;
        pNonce = pContext->pSendNonce;
        break;
    case eVerify:
        pSignKey = pContext->VerifySessionKey;  // if NTLM2
        pKey = pContext->UnsealSessionKey;      // might be used to decrypt the signature
        pRc4Sched = pContext->pUnsealRc4Sched;
        pNonce = pContext->pRecvNonce;
        break;
    }

    //
    // Either we can supply the sequence number, or
    // the application can supply the message sequence number.
    //

    Sig.Version = NTLM_SIGN_VERSION;

    // if we're doing the new NTLM2 version:
    if (pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2) {

        if ((pContext->NegotiateFlags & NTLMSSP_APP_SEQ) == 0)
        {
            Sig.Nonce = *pNonce;    // use our sequence number
            (*pNonce) += 1;
        }
        else {

            if (Op == eSeal || Op == eSign || MessageSeqNo != 0)
                Sig.Nonce = MessageSeqNo;
            else
                Sig.Nonce = (*ppSig)->Nonce;

            //   if using RC4, must rekey for each packet
            //   RC4 is used for seal, unseal; and for encrypting the HMAC hash if
            //   key exchange was negotiated (we use just HMAC if no key exchange,
            //   so that a good signing option exists with no RC4 encryption needed)

            if (Op == eSeal || Op == eUnseal || pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH)
            {
                MD5_CTX Md5ContextReKey;

                MD5Init(&Md5ContextReKey);
                MD5Update(&Md5ContextReKey, pKey, MSV1_0_USER_SESSION_KEY_LENGTH);
                MD5Update(&Md5ContextReKey, (unsigned char*)&Sig.Nonce, sizeof(Sig.Nonce));
                MD5Final(&Md5ContextReKey);
                ASSERT(MD5DIGESTLEN == MSV1_0_USER_SESSION_KEY_LENGTH);
                SspRc4Key(pContext->NegotiateFlags, pRc4Sched, Md5ContextReKey.digest);
            }
        }

        //
        // using HMAC hash, init it with the key
        //

        HMACMD5Init(&HMACMD5Context, pSignKey, MSV1_0_USER_SESSION_KEY_LENGTH);

        //
        // include the message sequence number
        //

        HMACMD5Update(&HMACMD5Context, (unsigned char*)&Sig.Nonce, sizeof(Sig.Nonce));

        for (i = 0; i < pMessage->cBuffers ; i++ )
        {
            if (((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_DATA) &&
                (pMessage->pBuffers[i].cbBuffer != 0))
            {
                // decrypt (before checksum...) if it's not READ_ONLY
                if ((Op==eUnseal)
                    && !(pMessage->pBuffers[i].BufferType & SECBUFFER_READONLY)
                )
                {
                    SspEncryptBuffer(
                        pContext,
                        pRc4Sched,
                        pMessage->pBuffers[i].cbBuffer,
                        pMessage->pBuffers[i].pvBuffer
                        );
                }

                HMACMD5Update(
                            &HMACMD5Context,
                            (unsigned char*)pMessage->pBuffers[i].pvBuffer,
                            pMessage->pBuffers[i].cbBuffer);

                //
                // Encrypt if its not READ_ONLY
                //

                if ((Op==eSeal)
                    && !(pMessage->pBuffers[i].BufferType & SECBUFFER_READONLY)
                )
                {
                    SspEncryptBuffer(
                        pContext,
                        pRc4Sched,
                        pMessage->pBuffers[i].cbBuffer,
                        pMessage->pBuffers[i].pvBuffer
                        );
                }
            }
        }

        HMACMD5Final(&HMACMD5Context, TempSig);

        //
        // use RandomPad and Checksum fields for 8 bytes of MD5 hash
        //

        RtlCopyMemory(&Sig.RandomPad, TempSig, 8);

        //
        // if we're using crypto for KEY_EXCH, may as well use it for signing too...
        //

        if (pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH)
            SspEncryptBuffer(
                pContext,
                pRc4Sched,
                8,
                &Sig.RandomPad
                );
    }
    //
    // pre-NTLM2 methods
    //
    else {

        //
        // required by CRC-32 algorithm
        //
        Sig.CheckSum = 0xffffffff;

        for (i = 0; i < pMessage->cBuffers ; i++ )
        {
            if (((pMessage->pBuffers[i].BufferType & 0xFF) == SECBUFFER_DATA) &&
                !(pMessage->pBuffers[i].BufferType & SECBUFFER_READONLY) &&
                (pMessage->pBuffers[i].cbBuffer != 0))
            {
                // decrypt (before checksum...)
                if (Op==eUnseal)
                {
                    SspEncryptBuffer(
                        pContext,
                        pRc4Sched,
                        pMessage->pBuffers[i].cbBuffer,
                        pMessage->pBuffers[i].pvBuffer
                        );
                }

                SspGenCheckSum(&pMessage->pBuffers[i], &Sig);

                // Encrypt
                if (Op==eSeal)
                {
                    SspEncryptBuffer(
                        pContext,
                        pRc4Sched,
                        pMessage->pBuffers[i].cbBuffer,
                        pMessage->pBuffers[i].pvBuffer
                        );
                }
            }
        }

        //
        // Required by CRC-32 algorithm
        //

        Sig.CheckSum ^= 0xffffffff;

        // when we encrypt 0, we will get the cipher stream for the nonce!
        Sig.Nonce = 0;

        SspEncryptBuffer(
            pContext,
            pRc4Sched,
            sizeof(NTLMSSP_MESSAGE_SIGNATURE) - sizeof(ULONG),
            &Sig.RandomPad
            );


        if ((pContext->NegotiateFlags & NTLMSSP_APP_SEQ) == 0)
        {
            Sig.Nonce ^= *pNonce;    // use our sequence number and encrypt it
            (*pNonce) += 1;
        }
        else if (Op == eSeal || Op == eSign || MessageSeqNo != 0)
            Sig.Nonce ^= MessageSeqNo;   // use caller's sequence number and encrypt it
        else
            Sig.Nonce = (*ppSig)->Nonce;    // use sender's sequence number

        //
        // for SignMessage calling, does nothing (copies garbage)
        // For VerifyMessage calling, allows it to compare sig block
        // upon return to Verify without knowing whether its MD5 or CRC32
        //

        Sig.RandomPad = (*ppSig)->RandomPad;
    }

    pMessage->pBuffers[Signature].cbBuffer = sizeof(NTLMSSP_MESSAGE_SIGNATURE);

    RtlCopyMemory(
        pSig,
        &Sig,
        NTLMSSP_MESSAGE_SIGNATURE_SIZE
        );

    return(SEC_E_OK);
}



//+-------------------------------------------------------------------------
//
//  Function:   NtLmMakeSignature
//
//  Synopsis:   Signs a message buffer by calculatinga checksum over all
//              the non-read only data buffers and encrypting the checksum
//              along with a nonce.
//
//  Effects:
//
//  Arguments:  KernelContextHandle - Handle of the context to use to sign the
//                      message.
//              QualityOfProtection - Unused flags.
//              MessageBuffers - Contains an array of buffers to sign and
//                      to store the signature.
//              MessageSequenceNumber - Sequence number for this message,
//                      only used in datagram cases.
//
//  Requires:   STATUS_INVALID_HANDLE - the context could not be found or
//                      was not configured for message integrity.
//              STATUS_INVALID_PARAMETER - the signature buffer could not
//                      be found.
//              STATUS_BUFFER_TOO_SMALL - the signature buffer is too small
//                      to hold the signature
//
//  Returns:
//
//  Notes: This was stolen from net\svcdlls\ntlmssp\client\sign.c ,
//         routine SspHandleSignMessage. It's possible that
//         bugs got copied too
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
NtLmMakeSignature(
    IN ULONG_PTR KernelContextHandle,
    IN ULONG fQOP,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PNTLM_KERNEL_CONTEXT pContext;
    NTLMSSP_MESSAGE_SIGNATURE  Sig;
    NTLMSSP_MESSAGE_SIGNATURE  *pSig;

    MAYBE_PAGED_CODE();

    DebugLog(( DEB_TRACE, "Entering NtLmMakeSignature\n" ));

    UNREFERENCED_PARAMETER(fQOP);

    Status = NtlmReferenceContext( KernelContextHandle, FALSE );

    if ( NT_SUCCESS( Status ) )
    {
        pContext = (PNTLM_KERNEL_CONTEXT) KernelContextHandle ;
    }
    else
    {
        DebugLog(( DEB_ERROR,
          "Bad kernel context 0x%lx\n", KernelContextHandle));
        goto CleanUp_NoDeref;

    }


    Status = SspSignSealHelper(
                        pContext,
                        eSign,
                        pMessage,
                        MessageSeqNo,
                        &Sig,
                        &pSig
                        );


    if( !NT_SUCCESS( Status ) )
    {
        DebugLog(( DEB_ERROR, "NtLmMakeSignature, SspSignSealHelper returns %lx\n", Status ));
        goto CleanUp;
    }

    RtlCopyMemory(
        pSig,
        &Sig,
        NTLMSSP_MESSAGE_SIGNATURE_SIZE
        );

CleanUp:

    NtlmDerefContext( pContext );

CleanUp_NoDeref:

    DebugLog(( DEB_TRACE, "Leaving NtLmMakeSignature 0x%lx\n", Status ));
    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   NtLmVerifySignature
//
//  Synopsis:   Verifies a signed message buffer by calculating a checksum over all
//              the non-read only data buffers and encrypting the checksum
//              along with a nonce.
//
//  Effects:
//
//  Arguments:  KernelContextHandle - Handle of the context to use to sign the
//                      message.
//              MessageBuffers - Contains an array of signed buffers  and
//                      a signature buffer.
//              MessageSequenceNumber - Sequence number for this message,
//                      only used in datagram cases.
//              QualityOfProtection - Unused flags.
//
//  Requires:   STATUS_INVALID_HANDLE - the context could not be found or
//                      was not configured for message integrity.
//              STATUS_INVALID_PARAMETER - the signature buffer could not
//                      be found or was too small.
//
//  Returns:
//
//  Notes: This was stolen from net\svcdlls\ntlmssp\client\sign.c ,
//         routine SspHandleVerifyMessage. It's possible that
//         bugs got copied too
//
//
//--------------------------------------------------------------------------



NTSTATUS NTAPI
NtLmVerifySignature(
    IN ULONG_PTR KernelContextHandle,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    OUT PULONG pfQOP
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PNTLM_KERNEL_CONTEXT pContext;
    NTLMSSP_MESSAGE_SIGNATURE   Sig;
    PNTLMSSP_MESSAGE_SIGNATURE  pSig;    // pointer to buffer with sig in it

    MAYBE_PAGED_CODE();

    DebugLog(( DEB_TRACE, "Entering NtLmVerifySignature\n" ));

    UNREFERENCED_PARAMETER(pfQOP);

    Status = NtlmReferenceContext( KernelContextHandle, FALSE );

    if ( NT_SUCCESS( Status ) )
    {
        pContext = (PNTLM_KERNEL_CONTEXT) KernelContextHandle ;
    }
    else
    {
        DebugLog(( DEB_ERROR,
          "Bad kernel context 0x%lx\n", KernelContextHandle));
        goto CleanUp_NoDeref;

    }


    Status = SspSignSealHelper(
                        pContext,
                        eVerify,
                        pMessage,
                        MessageSeqNo,
                        &Sig,
                        &pSig
                        );


    if( !NT_SUCCESS( Status ) )
    {
        DebugLog(( DEB_ERROR, "NtLmVerifySignature, SspSignSealHelper returns %lx\n", Status ));
        goto CleanUp;
    }

    if (pSig->Version != NTLM_SIGN_VERSION) {
        Status = SEC_E_INVALID_TOKEN;
        goto CleanUp;
    }

    // validate the signature...
    if (pSig->CheckSum != Sig.CheckSum)
    {
        Status = SEC_E_MESSAGE_ALTERED;
        goto CleanUp;
    }

    // with MD5 sig, this now matters!
    if (pSig->RandomPad != Sig.RandomPad)
    {
        Status = SEC_E_MESSAGE_ALTERED;
        goto CleanUp;
    }

    if (pSig->Nonce != Sig.Nonce)
    {
        Status = SEC_E_OUT_OF_SEQUENCE;
        goto CleanUp;
    }

CleanUp:

    NtlmDerefContext( pContext );

CleanUp_NoDeref:


    DebugLog(( DEB_TRACE, "Leaving NtLmVerifySignature 0x%lx\n", Status ));
    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   NtLmSealMessage
//
//  Synopsis:   Verifies a signed message buffer by calculating a checksum over all
//              the non-read only data buffers and encrypting the checksum
//              along with a nonce.
//
//  Effects:
//
//  Arguments:  KernelContextHandle - Handle of the context to use to sign the
//                      message.
//              MessageBuffers - Contains an array of signed buffers  and
//                      a signature buffer.
//              MessageSequenceNumber - Sequence number for this message,
//                      only used in datagram cases.
//              QualityOfProtection - Unused flags.
//
//  Requires:   STATUS_INVALID_HANDLE - the context could not be found or
//                      was not configured for message integrity.
//              STATUS_INVALID_PARAMETER - the signature buffer could not
//                      be found or was too small.
//
//  Returns:
//
//  Notes: This was stolen from net\svcdlls\ntlmssp\client\sign.c ,
//         routine SspHandleSealMessage. It's possible that
//         bugs got copied too
//
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
NtLmSealMessage(
    IN ULONG_PTR KernelContextHandle,
    IN ULONG fQOP,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PNTLM_KERNEL_CONTEXT pContext;
    NTLMSSP_MESSAGE_SIGNATURE  Sig;
    PNTLMSSP_MESSAGE_SIGNATURE  pSig;    // pointer to buffer where sig goes

    MAYBE_PAGED_CODE();

    DebugLog(( DEB_TRACE, "Entering NtLmSealMessage\n" ));

    UNREFERENCED_PARAMETER(fQOP);

    Status = NtlmReferenceContext( KernelContextHandle, FALSE );

    if ( NT_SUCCESS( Status ) )
    {
        pContext = (PNTLM_KERNEL_CONTEXT) KernelContextHandle ;
    }
    else
    {
        DebugLog(( DEB_ERROR,
          "Bad kernel context 0x%lx\n", KernelContextHandle));
        goto CleanUp_NoDeref;

    }


    Status = SspSignSealHelper(
                    pContext,
                    eSeal,
                    pMessage,
                    MessageSeqNo,
                    &Sig,
                    &pSig
                    );

    if (!NT_SUCCESS(Status))
    {
        DebugLog(( DEB_ERROR, "SpSealMessage, SspSignSealHelper returns %lx\n", Status ));
        goto CleanUp;
    }

    RtlCopyMemory(
        pSig,
        &Sig,
        NTLMSSP_MESSAGE_SIGNATURE_SIZE
        );


CleanUp:

    NtlmDerefContext( pContext );

CleanUp_NoDeref:


    DebugLog(( DEB_TRACE, "Leaving NtLmSealMessage 0x%lx\n", Status ));
    return(Status);

}

//+-------------------------------------------------------------------------
//
//  Function:   NtLmUnsealMessage
//
//  Synopsis:   Verifies a signed message buffer by calculating a checksum over all
//              the non-read only data buffers and encrypting the checksum
//              along with a nonce.
//
//  Effects:
//
//  Arguments:  KernelContextHandle - Handle of the context to use to sign the
//                      message.
//              MessageBuffers - Contains an array of signed buffers  and
//                      a signature buffer.
//              MessageSequenceNumber - Sequence number for this message,
//                      only used in datagram cases.
//              QualityOfProtection - Unused flags.
//
//  Requires:   STATUS_INVALID_HANDLE - the context could not be found or
//                      was not configured for message integrity.
//              STATUS_INVALID_PARAMETER - the signature buffer could not
//                      be found or was too small.
//
//  Returns:
//
//  Notes: This was stolen from net\svcdlls\ntlmssp\client\sign.c ,
//         routine SspHandleUnsealMessage. It's possible that
//         bugs got copied too
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
NtLmUnsealMessage(
    IN ULONG_PTR KernelContextHandle,
    IN PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    OUT PULONG pfQOP
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PNTLM_KERNEL_CONTEXT pContext;
    NTLMSSP_MESSAGE_SIGNATURE  Sig;
    PNTLMSSP_MESSAGE_SIGNATURE  pSig;    // pointer to buffer where sig goes

    MAYBE_PAGED_CODE();

    DebugLog(( DEB_TRACE, "Entering NtLmUnsealMessage\n" ));

    UNREFERENCED_PARAMETER(pfQOP);

    Status = NtlmReferenceContext( KernelContextHandle, FALSE );

    if ( NT_SUCCESS( Status ) )
    {
        pContext = (PNTLM_KERNEL_CONTEXT) KernelContextHandle ;
    }
    else
    {
        DebugLog(( DEB_ERROR,
          "Bad kernel context 0x%lx\n", KernelContextHandle));
        goto CleanUp_NoDeref;

    }

    Status = SspSignSealHelper(
                    pContext,
                    eUnseal,
                    pMessage,
                    MessageSeqNo,
                    &Sig,
                    &pSig
                    );

    if (!NT_SUCCESS(Status))
    {
        DebugLog(( DEB_ERROR, "SpUnsealMessage, SspSignSealHelper returns %lx\n", Status ));
        goto CleanUp;
    }

    if (pSig->Version != NTLM_SIGN_VERSION) {
        Status = SEC_E_INVALID_TOKEN;
        goto CleanUp;
    }

    // validate the signature...
    if (pSig->CheckSum != Sig.CheckSum)
    {
        Status = SEC_E_MESSAGE_ALTERED;
        goto CleanUp;
    }

    if (pSig->RandomPad != Sig.RandomPad)
    {
        Status = SEC_E_MESSAGE_ALTERED;
        goto CleanUp;
    }

    if (pSig->Nonce != Sig.Nonce)
    {
        Status = SEC_E_OUT_OF_SEQUENCE;
        goto CleanUp;
    }


CleanUp:

    NtlmDerefContext( pContext );

CleanUp_NoDeref:

    DebugLog(( DEB_TRACE, "Leaving NtLmUnsealMessage 0x%lx\n", Status ));
    return (Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   NtLmGetContextToken
//
//  Synopsis:   returns a pointer to the token for a server-side context
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
NtLmGetContextToken(
    IN ULONG_PTR KernelContextHandle,
    OUT PHANDLE ImpersonationToken,
    OUT OPTIONAL PACCESS_TOKEN *RawToken
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PNTLM_KERNEL_CONTEXT pContext = NULL;

    MAYBE_PAGED_CODE();

    DebugLog(( DEB_TRACE, "Entering NtLmGetContextToken\n" ));

    Status = NtlmReferenceContext( KernelContextHandle, FALSE );

    if ( NT_SUCCESS( Status ) )
    {
        pContext = (PNTLM_KERNEL_CONTEXT) KernelContextHandle ;
    }
    else
    {
        DebugLog(( DEB_ERROR,
          "Bad kernel context 0x%lx\n", KernelContextHandle));
        goto CleanUp_NoDeref;

    }

    if (pContext->ClientTokenHandle == NULL)
    {
        DebugLog(( DEB_ERROR, "Invalid TokenHandle for context 0x%lx\n", pContext ));
        Status= SEC_E_NO_IMPERSONATION;
        goto CleanUp;
    }

    if (ARGUMENT_PRESENT(ImpersonationToken))
    {
        *ImpersonationToken = pContext->ClientTokenHandle;
    }

    if (ARGUMENT_PRESENT(RawToken))
    {
        if (pContext->ClientTokenHandle != NULL)
        {
            if (pContext->AccessToken == NULL)
            {
                Status = ObReferenceObjectByHandle(
                             pContext->ClientTokenHandle,
                             TOKEN_IMPERSONATE,
                             NULL,
                             KeGetPreviousMode(),
                             (PVOID *) &pContext->AccessToken,
                             NULL);
            }
        }

        if (NT_SUCCESS(Status))
        {
            ASSERT(pContext->AccessToken != NULL);
            *RawToken = pContext->AccessToken;
        }
    }


CleanUp:

    NtlmDerefContext( pContext );

CleanUp_NoDeref:
    DebugLog(( DEB_TRACE, "Leaving NtLmGetContextToken 0x%lx\n", Status ));
    return (Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   NtLmQueryContextAttributes
//
//  Synopsis:   Querys attributes of the specified context
//              This API allows a customer of the security
//              services to determine certain attributes of
//              the context.  These are: sizes, names, and lifespan.
//
//  Effects:
//
//  Arguments:
//
//    ContextHandle - Handle to the context to query.
//
//    Attribute - Attribute to query.
//
//        #define SECPKG_ATTR_SIZES    0
//        #define SECPKG_ATTR_NAMES    1
//        #define SECPKG_ATTR_LIFESPAN 2
//
//    Buffer - Buffer to copy the data into.  The buffer must
//             be large enough to fit the queried attribute.
//
//
//  Requires:
//
//  Returns:
//
//        STATUS_SUCCESS - Call completed successfully
//
//        STATUS_INVALID_HANDLE -- Credential/Context Handle is invalid
//        STATUS_UNSUPPORTED_FUNCTION -- Function code is not supported
//
//  Notes:
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
NtLmQueryContextAttributes(
    IN ULONG_PTR KernelContextHandle,
    IN ULONG Attribute,
    IN OUT PVOID Buffer
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSecPkgContext_NamesW ContextNames = NULL;
    PSecPkgContext_DceInfo ContextDceInfo = NULL;
    PSecPkgContext_SessionKey ContextSessionKeyInfo = NULL;
    PSecPkgContext_Sizes ContextSizes = NULL;
    PSecPkgContext_Flags ContextFlags = NULL;
    PSecPkgContext_PasswordExpiry PasswordExpires;
    PSecPkgContext_UserFlags UserFlags;
    PSecPkgContext_PackageInfo PackageInfo = NULL;
    PNTLM_KERNEL_CONTEXT pContext = NULL;
    unsigned int Length = 0;

    MAYBE_PAGED_CODE();

    DebugLog(( DEB_TRACE, "Entering NtLmQueryContextAttributes\n" ));



    Status = NtlmReferenceContext( KernelContextHandle, FALSE );

    if ( NT_SUCCESS( Status ) )
    {
        pContext = (PNTLM_KERNEL_CONTEXT) KernelContextHandle ;
    } else {

        //
        // for PACKAGE_INFO or NEGOTIATION_INFO, don't require a completed
        // context.
        //

        if( (Attribute != SECPKG_ATTR_PACKAGE_INFO) &&
            (Attribute != SECPKG_ATTR_NEGOTIATION_INFO)
            )
        {
            DebugLog(( DEB_ERROR,
            "Bad kernel context 0x%lx\n", KernelContextHandle));
            goto CleanUp_NoDeref;
        }

        Status = STATUS_SUCCESS;
    }

    //
    // Handle each of the various queried attributes
    //

    switch ( Attribute ) {
    case SECPKG_ATTR_SIZES:

        ContextSizes = (PSecPkgContext_Sizes) Buffer;
        ContextSizes->cbMaxToken = NTLMSP_MAX_TOKEN_SIZE;

        if (pContext->NegotiateFlags & (NTLMSSP_NEGOTIATE_ALWAYS_SIGN |
                                       NTLMSSP_NEGOTIATE_SIGN |
                                       NTLMSSP_NEGOTIATE_SEAL) ) {
            ContextSizes->cbMaxSignature = NTLMSSP_MESSAGE_SIGNATURE_SIZE;
        } else {
            ContextSizes->cbMaxSignature = 0;
        }

        if (pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_SEAL) {
            ContextSizes->cbBlockSize = 1;
            ContextSizes->cbSecurityTrailer = NTLMSSP_MESSAGE_SIGNATURE_SIZE;
        }
        else
        {
            ContextSizes->cbBlockSize = 0;
            ContextSizes->cbSecurityTrailer = 0;
        }

        break;

    //
    // No one uses the function so don't go to the overhead of maintaining
    // the username in the context structure.
    //

    case SECPKG_ATTR_DCE_INFO:

        ContextDceInfo = (PSecPkgContext_DceInfo) Buffer;

        if (ContextDceInfo == NULL)
        {
            DebugLog(( DEB_ERROR, "Null buffer SECPKG_ATTR_DCE_INFO.\n" ));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        if (pContext->ContextNames)
        {
            Length = wcslen(pContext->ContextNames);
        }

        ContextDceInfo->pPac = (LPWSTR) LsaKernelFunctions->AllocateHeap(
                 (Length + 1) * sizeof(WCHAR));

        if (ContextDceInfo->pPac != NULL)
        {
            RtlCopyMemory(
                ContextDceInfo->pPac,
                pContext->ContextNames,
                Length * sizeof(WCHAR));

            LPWSTR Temp =  (LPWSTR)ContextDceInfo->pPac;
            Temp[Length] = L'\0';
        }
        else
        {
            DebugLog(( DEB_ERROR, "Bad Context->pPac in SECPKG_ATTR_DCE_INFO.\n" ));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        ContextDceInfo->AuthzSvc = 0;

        break;

    case SECPKG_ATTR_NAMES:

        ContextNames = (PSecPkgContext_Names) Buffer;

        if (ContextNames == NULL)
        {
            DebugLog(( DEB_ERROR, "Null buffer SECPKG_ATTR_NAMES.\n" ));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        if (pContext->ContextNames)
        {
            Length = wcslen(pContext->ContextNames);
            DebugLog(( DEB_TRACE, "NtLmQueryContextAttributes: ContextNames length is 0x%lx\n", Length));
        }

        ContextNames->sUserName = (LPWSTR) LsaKernelFunctions->AllocateHeap(
                 (Length + 1) * sizeof(WCHAR));

        if (ContextNames->sUserName != NULL)
        {
            RtlCopyMemory(
                ContextNames->sUserName,
                pContext->ContextNames,
                Length * sizeof(WCHAR));

            ContextNames->sUserName[Length] = L'\0';
        }
        else
        {
            DebugLog(( DEB_ERROR, "Bad Context->sUserName in SECPKG_ATTR_NAMES.\n" ));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        break;

    case SECPKG_ATTR_SESSION_KEY:
       ContextSessionKeyInfo = (PSecPkgContext_SessionKey) Buffer;
       ContextSessionKeyInfo->SessionKeyLength = MSV1_0_USER_SESSION_KEY_LENGTH;
       ContextSessionKeyInfo->SessionKey =
           (PUCHAR) LsaKernelFunctions->AllocateHeap(
               ContextSessionKeyInfo->SessionKeyLength);
       if (ContextSessionKeyInfo->SessionKey != NULL)
       {
           RtlCopyMemory(
               ContextSessionKeyInfo->SessionKey,
               pContext->SessionKey,
               MSV1_0_USER_SESSION_KEY_LENGTH);
       }
       else
       {
           Status = STATUS_INSUFFICIENT_RESOURCES;
       }

       break;

    case SECPKG_ATTR_PASSWORD_EXPIRY:
        PasswordExpires = (PSecPkgContext_PasswordExpiry) Buffer;
        if(pContext->PasswordExpiry.QuadPart != 0) {
            PasswordExpires->tsPasswordExpires = pContext->PasswordExpiry;
        } else {
            Status = SEC_E_UNSUPPORTED_FUNCTION;
        }
        break;

    case SECPKG_ATTR_USER_FLAGS:
        UserFlags = (PSecPkgContext_UserFlags) Buffer;
        UserFlags->UserFlags = pContext->UserFlags;
        break;

    case SECPKG_ATTR_FLAGS:
    {
        BOOLEAN Client = (pContext->ClientTokenHandle == 0);
        ULONG Flags = 0;

        //
        // BUGBUG: this doesn't return the complete flags
        //
        ContextFlags = (PSecPkgContext_Flags) Buffer;
        ContextFlags->Flags = 0;

        if (pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_SEAL) {
            if( Client )
            {
                Flags |= ISC_RET_CONFIDENTIALITY;
            } else {
                Flags |= ASC_RET_CONFIDENTIALITY;
            }
        }

        if (pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN) {
            if( Client )
            {
                Flags |= ISC_RET_SEQUENCE_DETECT | ISC_RET_REPLAY_DETECT | ISC_RET_INTEGRITY;
            } else {
                Flags |= ASC_RET_SEQUENCE_DETECT | ASC_RET_REPLAY_DETECT | ASC_RET_INTEGRITY;
            }
        }

        if (pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_NULL_SESSION) {
            if( Client )
            {
                Flags |= ISC_RET_NULL_SESSION;
            } else {
                Flags |= ASC_RET_NULL_SESSION;
            }
        }

        if (pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM) {
            if( Client )
            {
                Flags |= ISC_RET_DATAGRAM;
            } else {
                Flags |= ASC_RET_DATAGRAM;
            }
        }

        if (pContext->NegotiateFlags & NTLMSSP_NEGOTIATE_IDENTIFY) {
            if( Client )
            {
                Flags |= ISC_RET_IDENTIFY;
            } else {
                Flags |= ASC_RET_IDENTIFY;
            }
        }

        ContextFlags->Flags |= Flags;

        break;
    }

    case SECPKG_ATTR_PACKAGE_INFO:
    case SECPKG_ATTR_NEGOTIATION_INFO:
        //
        // Return the information about this package. This is useful for
        // callers who used SPNEGO and don't know what package they got.
        //

        PackageInfo = (PSecPkgContext_PackageInfo) Buffer;

        PackageInfo->PackageInfo = (PSecPkgInfoW) LsaKernelFunctions->AllocateHeap(
                                                    sizeof(SecPkgInfoW) +
                                                    sizeof(NTLMSP_NAME) +
                                                    sizeof(NTLMSP_COMMENT)
                                                    );

        if (PackageInfo->PackageInfo == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        PackageInfo->PackageInfo->Name = (LPWSTR) (PackageInfo->PackageInfo + 1);
        PackageInfo->PackageInfo->Comment = (LPWSTR) ((((PCHAR) PackageInfo->PackageInfo->Name)) + sizeof(NTLMSP_NAME));
        wcscpy(
            PackageInfo->PackageInfo->Name,
            NTLMSP_NAME
            );

        wcscpy(
            PackageInfo->PackageInfo->Comment,
            NTLMSP_COMMENT
            );
        PackageInfo->PackageInfo->wVersion      = SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION;
        PackageInfo->PackageInfo->wRPCID        = NTLMSP_RPCID;
        PackageInfo->PackageInfo->fCapabilities = NTLMSP_CAPS;
        PackageInfo->PackageInfo->cbMaxToken    = NTLMSP_MAX_TOKEN_SIZE;

        if ( Attribute == SECPKG_ATTR_NEGOTIATION_INFO )
        {
            PSecPkgContext_NegotiationInfo NegInfo ;

            NegInfo = (PSecPkgContext_NegotiationInfo) PackageInfo ;
            if( pContext ) {
                NegInfo->NegotiationState = SECPKG_NEGOTIATION_COMPLETE ;
            } else {
                NegInfo->NegotiationState = 0;
            }
        }
        break;

    case SECPKG_ATTR_LIFESPAN:
    default:

        Status = STATUS_NOT_SUPPORTED;
        break;
    }

Cleanup:

    if (!NT_SUCCESS(Status)) {

        switch ( Attribute) {

          case SECPKG_ATTR_NAMES:

              if (ContextNames && ContextNames->sUserName)
              {
                  LsaKernelFunctions->FreeHeap(ContextNames->sUserName);
                  ContextNames->sUserName = NULL;
              }
          break;

          case SECPKG_ATTR_DCE_INFO:

              if (ContextDceInfo && ContextDceInfo->pPac)
              {
                  LsaKernelFunctions->FreeHeap(ContextDceInfo->pPac);
                  ContextDceInfo->pPac = NULL;
              }
          break;

          case SECPKG_ATTR_SESSION_KEY:

              if(ContextSessionKeyInfo && ContextSessionKeyInfo->SessionKey)
              {
                  LsaKernelFunctions->FreeHeap(ContextSessionKeyInfo->SessionKey);
                  ContextSessionKeyInfo->SessionKey = NULL;
              }
          break;

          case SECPKG_ATTR_NEGOTIATION_INFO:

              if(PackageInfo && PackageInfo->PackageInfo)
              {
                  LsaKernelFunctions->FreeHeap(PackageInfo->PackageInfo);
                  PackageInfo->PackageInfo = NULL;
              }
          break;

        }
    }

    if( pContext ) {
        NtlmDerefContext( pContext );
    }

CleanUp_NoDeref:
    DebugLog(( DEB_TRACE, "Leaving NtLmQueryContextAttributes 0x%lx\n", Status ));
    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   NtLmCompleteToken
//
//  Synopsis:   Completes a context
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS NTAPI
NtLmCompleteToken(
    IN ULONG_PTR ContextHandle,
    IN PSecBufferDesc InputBuffer
    )
{
    UNREFERENCED_PARAMETER (ContextHandle);
    UNREFERENCED_PARAMETER (InputBuffer);
    PAGED_CODE();
    DebugLog(( DEB_TRACE, "Entering NtLmCompleteToken\n" ));
    DebugLog(( DEB_TRACE, "Leaving NtLmCompleteToken\n" ));
    return(STATUS_NOT_SUPPORTED);
}

//+-------------------------------------------------------------------------
//
//  Function:   NtLmMakePackedContext
//
//  Synopsis:   Maps a context to the caller's address space
//
//  Effects:
//
//  Arguments:  Context - The context to map
//              MappedContext - Set to TRUE on success
//              ContextData - Receives a buffer in the caller's address space
//                      with the mapped context.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
NtLmMakePackedContext(
    IN PNTLM_KERNEL_CONTEXT Context,
    OUT PBOOLEAN MappedContext,
    OUT PSecBuffer ContextData,
    IN ULONG Flags
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PNTLM_KERNEL_CONTEXT PackedContext = NULL;
    ULONG ContextSize, ContextNameSize = 0;

    PAGED_CODE();

    // ExAcquireResourceExclusiveLite(&NtLmKernelContextLock, TRUE);

    if (Context->ContextNames)
    {
        ContextNameSize = wcslen(Context->ContextNames);
    }

    // ExReleaseResource(&NtLmKernelContextLock);

    ContextSize =  sizeof(NTLM_KERNEL_CONTEXT) +
                   ContextNameSize * sizeof(WCHAR);

    PackedContext = (PNTLM_KERNEL_CONTEXT) NtLmAllocate(ContextSize);

    if (PackedContext == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    // Copy all fields of the old context

    // ExAcquireResourceExclusiveLite(&NtLmKernelContextLock, TRUE);

    RtlCopyMemory(
        PackedContext,
        Context,
        sizeof(NTLM_KERNEL_CONTEXT)
        );


    if (ContextNameSize > 0)
    {
        PackedContext->ContextNames = (LPWSTR) sizeof(NTLM_KERNEL_CONTEXT);

        RtlCopyMemory(
            PackedContext+1,
            Context->ContextNames,
            ContextNameSize * sizeof(WCHAR));
    }
    else
    {
        PackedContext->ContextNames=NULL;
    }


    // ExReleaseResource(&NtLmKernelContextLock);

    // Replace some fields

    //
    // Token will be returned by the caller of this routine
    //

    PackedContext->ClientTokenHandle = NULL;

    PackedContext->NegotiateFlags |= NTLMSSP_NEGOTIATE_EXPORTED_CONTEXT;

    if ((Flags & SECPKG_CONTEXT_EXPORT_RESET_NEW) != 0)
    {
        PackedContext->SendNonce = (ULONG) -1;
        PackedContext->RecvNonce = (ULONG) -1;
    }

    // BUGBUG ?
    RtlZeroMemory(
        &PackedContext->SessionKey,
        MSV1_0_USER_SESSION_KEY_LENGTH
        );

    ContextData->pvBuffer = PackedContext;
    ContextData->cbBuffer = ContextSize;


    *MappedContext = TRUE;

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        if (PackedContext != NULL)
        {
            NtLmFree(PackedContext);
        }
    }

    return(Status);

}

//+-------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
NtLmExportSecurityContext(
    IN ULONG_PTR ContextHandle,
    IN ULONG Flags,
    OUT PSecBuffer PackedContext,
    IN OUT PHANDLE TokenHandle
    )
{
    PNTLM_KERNEL_CONTEXT Context = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ContextSize = 0;
    BOOLEAN MappedContext = FALSE;

    PAGED_CODE();

    DebugLog(( DEB_TRACE, "Entering NtLmExportSecurityContext\n" ));

    if (ARGUMENT_PRESENT(TokenHandle))
    {
        *TokenHandle = NULL;
    }

    PackedContext->pvBuffer = NULL;
    PackedContext->cbBuffer = 0;
    PackedContext->BufferType = 0;

    Status = NtlmReferenceContext( ContextHandle, FALSE );

    if ( NT_SUCCESS( Status ) )
    {
        Context = (PNTLM_KERNEL_CONTEXT) ContextHandle ;
    }
    else
    {
        goto Cleanup_NoDeref ;
    }

    Status = NtLmMakePackedContext(
                Context,
                &MappedContext,
                PackedContext,
                Flags
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    ASSERT(MappedContext);

    //
    // Now either duplicate the token or copy it.
    //

    if (ARGUMENT_PRESENT(TokenHandle))
    {
        // ExAcquireResourceExclusiveLite(&NtLmKernelContextLock, TRUE);
        if ((Flags & SECPKG_CONTEXT_EXPORT_DELETE_OLD) != 0)
        {
            *TokenHandle = Context->ClientTokenHandle;
            Context->ClientTokenHandle = NULL;
        }
        else
        {
            Status = NtDuplicateObject(
                        NtCurrentProcess(),
                        Context->ClientTokenHandle,
                        NULL,
                        TokenHandle,
                        0,              // no new access
                        0,              // no handle attributes
                        DUPLICATE_SAME_ACCESS
                        );
        }
        // ExReleaseResource(&NtLmKernelContextLock);

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

    }

Cleanup:

    NtlmDerefContext( Context );

Cleanup_NoDeref:

    return (Status);

}

//+-------------------------------------------------------------------------
//
//  Function:   NtLmCreateKernelModeContext
//
//  Synopsis:   Creates a kernel-mode context to support impersonation and
//              message integrity and privacy
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
NtLmCreateKernelModeContext(
    IN ULONG_PTR ContextHandle,
    IN OPTIONAL HANDLE TokenHandle,
    IN PSecBuffer MarshalledContext,
    OUT PNTLM_KERNEL_CONTEXT * NewContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PNTLM_KERNEL_CONTEXT Context = NULL;
    PNTLM_KERNEL_CONTEXT PackedContext;
    PUCHAR Where;
    unsigned int Length = 0;

    PAGED_CODE();

    if (MarshalledContext->cbBuffer < sizeof(NTLM_KERNEL_CONTEXT))
    {
        DebugLog((DEB_ERROR,"NtLmCreateKernelModeContext: Invalid buffer size for marshalled context: was 0x%x, needed 0x%x\n",
            MarshalledContext->cbBuffer, sizeof(NTLM_KERNEL_CONTEXT)));
        return(STATUS_INVALID_PARAMETER);
    }

    PackedContext = (PNTLM_KERNEL_CONTEXT) MarshalledContext->pvBuffer;

    Context = (PNTLM_KERNEL_CONTEXT) NtLmAllocate( sizeof(NTLM_KERNEL_CONTEXT));
    if (!Context)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        DebugLog((DEB_ERROR,"NtLmCreateKernelModeContext: Allocation error for Context\n"));
        goto Cleanup;
    }

    RtlZeroMemory(
            Context,
            sizeof(NTLM_KERNEL_CONTEXT));

    // Copy contenets of PackedContext->pvBuffer to Context
    *Context = *PackedContext;

    KsecInitializeListEntry( &Context->List, NTLM_CONTEXT_SIGNATURE );


    // These need to be changed

    Context->ClientTokenHandle = TokenHandle;

    if (Context->SendNonce == (ULONG) -1)
    {
        // The context was exported with the reset flag
        Context->SendNonce = 0;
    }

    if (Context->RecvNonce == (ULONG) -1)
    {
        // The context was exported with the reset flag
        Context->RecvNonce = 0;
    }

    if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2 ) {

        Context->pSealRc4Sched = &Context->SealRc4Sched;
        Context->pUnsealRc4Sched = &Context->UnsealRc4Sched;
        Context->pSendNonce = &Context->SendNonce;
        Context->pRecvNonce = &Context->RecvNonce;
    } else {

        Context->pSealRc4Sched = &Context->SealRc4Sched;
        Context->pUnsealRc4Sched = &Context->SealRc4Sched;
        Context->pSendNonce = &Context->SendNonce;
        Context->pRecvNonce = &Context->SendNonce;
    }


    Context->ContextNames = NULL;

    Length = (MarshalledContext->cbBuffer - sizeof(NTLM_KERNEL_CONTEXT));
    if (Length > 0)
    {
        Context->ContextNames = (LPWSTR) NtLmAllocate(Length + sizeof(WCHAR));
        if (!Context->ContextNames)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        RtlCopyMemory(Context->ContextNames, PackedContext + 1, Length);
        // null terminate the string
        *(Context->ContextNames + (Length/2)) = UNICODE_NULL;
    }

    Context->PasswordExpiry = PackedContext->PasswordExpiry;
    Context->UserFlags = PackedContext->UserFlags;

    KSecInsertListEntry(
            NtlmActiveList,
            &Context->List );

    *NewContext = Context;

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        if (Context != NULL)
        {
            NtlmFreeKernelContext(Context);
        }
    }

    DebugLog(( DEB_TRACE, "Leaving NtLmCreateKernelContext 0x%lx\n", Status ));
    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
NtLmImportSecurityContext(
    IN PSecBuffer PackedContext,
    IN OPTIONAL HANDLE TokenHandle,
    OUT PULONG_PTR ContextHandle
    )
{
    NTSTATUS Status;
    PNTLM_KERNEL_CONTEXT Context = NULL;

    PAGED_CODE();
    DebugLog((DEB_TRACE,"Entering NtLmImportSecurityContext\n"));

    Status = NtLmCreateKernelModeContext(
                0,              // LsaContextHandle not present
                TokenHandle,
                PackedContext,
                &Context
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"NtLmImportSecurityContext: Failed to create kernel mode context: 0x%x\n",
            Status));
        goto Cleanup;
    }

    *ContextHandle = (ULONG_PTR) Context;

Cleanup:
    if (Context != NULL)
    {
        NtlmDerefContext( Context );
    }

    return(Status);
}


//+---------------------------------------------------------------------------
//
//  Function:   NtlmSetPagingMode
//
//  Synopsis:   Switch the paging mode for cluster support
//
//  Arguments:  [Pagable] --
//
//  History:    7-07-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
NtlmSetPagingMode(
    BOOLEAN Pagable
    )
{
    if ( Pagable )
    {
        NtlmPoolType = PagedPool ;
        NtlmActiveList = NtlmPagedList ;
    }
    else
    {
        if ( NtlmNonPagedList == NULL )
        {
            NtlmNonPagedList = LsaKernelFunctions->CreateContextList( KSecNonPaged );
            if ( NtlmNonPagedList == NULL )
            {
                return STATUS_NO_MEMORY ;
            }
        }

        NtlmActiveList = NtlmNonPagedList ;

        NtlmPoolType = NonPagedPool ;
    }
    return STATUS_SUCCESS ;

}
