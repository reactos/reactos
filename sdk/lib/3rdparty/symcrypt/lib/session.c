//
// session.c   code for Session API implementation
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSessionSenderInit(
    _Inout_ PSYMCRYPT_SESSION   pSession,
            UINT32              senderId,
            UINT32              flags )
{
    // Make sure we only specify the correct flags
    if (flags != 0)
    {
        return SYMCRYPT_INVALID_ARGUMENT;
    }

    pSession->replayState.messageNumber = 0;
    pSession->senderId                  = senderId;
    pSession->flags                     = SYMCRYPT_FLAG_SESSION_ENCRYPT;
    pSession->pMutex                    = NULL;

    return SYMCRYPT_NO_ERROR;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSessionReceiverInit(
    _Inout_ PSYMCRYPT_SESSION   pSession,
            UINT32              senderId,
            UINT32              flags )
{
    PVOID pMutex = NULL;

    // Make sure we only specify the correct flags
    if (flags != 0)
    {
        return SYMCRYPT_INVALID_ARGUMENT;
    }

#if SYMCRYPT_CPU_AMD64
    if ( !SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_CMPXCHG16B ) )
    {
        pMutex = SymCryptCallbackAllocateMutexFastInproc();
        if( pMutex == NULL )
        {
            return SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        }
    }
#elif SYMCRYPT_CPU_ARM64 // Arm64 always has support for CAS128 - so never need a lock
#else // 32b and generic platforms will always need to use a lock
    pMutex = SymCryptCallbackAllocateMutexFastInproc();
    if( pMutex == NULL )
    {
        return SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
    }
#endif
    pSession->pMutex = pMutex;

    // This represents that the message numbers 1-64 inclusive have not yet been successfully use in decryption
    pSession->replayState.replayMask    = 0;
    pSession->replayState.messageNumber = 64;

    pSession->senderId      = senderId;
    pSession->flags         = 0;

    return SYMCRYPT_NO_ERROR;
}

VOID
SYMCRYPT_CALL
SymCryptSessionDestroy(_Inout_ PSYMCRYPT_SESSION   pSession )
{
    if ( pSession->pMutex != NULL )
    {
        SymCryptCallbackFreeMutexFastInproc(pSession->pMutex);
    }
    SymCryptWipeKnownSize(pSession, sizeof(*pSession));
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSessionGcmEncrypt(
    _Inout_                         PSYMCRYPT_SESSION           pSession,
    _In_                            PCSYMCRYPT_GCM_EXPANDED_KEY pExpandedKey,
    _In_reads_opt_( cbAuthData )    PCBYTE                      pbAuthData,
                                    SIZE_T                      cbAuthData,
    _In_reads_( cbData )            PCBYTE                      pbSrc,
    _Out_writes_( cbData )          PBYTE                       pbDst,
                                    SIZE_T                      cbData,
    _Out_writes_( cbTag )           PBYTE                       pbTag,
                                    SIZE_T                      cbTag,
    _Out_opt_                       PUINT64                     pu64MessageNumber )
{
    BYTE    nonce[12];
    UINT64  messageNumber;

    if ( (pSession->flags & SYMCRYPT_FLAG_SESSION_ENCRYPT) != SYMCRYPT_FLAG_SESSION_ENCRYPT )
    {
        return SYMCRYPT_INVALID_ARGUMENT;
    }

    messageNumber = SYMCRYPT_ATOMIC_ADD64_POST_RELAXED(&pSession->replayState.messageNumber, 1);

    // We do not allow messageNumber to go above some maximum value (currently 2^64 - 2^32)
    if ( messageNumber > SYMCRYPT_SESSION_MAX_MESSAGE_NUMBER )
    {
        // Decrement the session messageNumber on the error path so that this session will continue
        // to only generate errors
        SYMCRYPT_ATOMIC_ADD64_POST_RELAXED(&pSession->replayState.messageNumber, -1ll);
        return SYMCRYPT_INVALID_ARGUMENT;
    }

    SYMCRYPT_STORE_MSBFIRST32(&nonce[0], pSession->senderId);
    SYMCRYPT_STORE_MSBFIRST64(&nonce[4], messageNumber);

    SymCryptGcmEncrypt(
        pExpandedKey,
        nonce,
        sizeof(nonce),
        pbAuthData,
        cbAuthData,
        pbSrc,
        pbDst,
        cbData,
        pbTag,
        cbTag);

    if( pu64MessageNumber != NULL )
    {
        *pu64MessageNumber = messageNumber;
    }

    return SYMCRYPT_NO_ERROR;
}

// Convenience function used in SymCryptSessionDecryptUpdateState*
//
// Given an observedState check whether messageNumber represents a replay
// If it does, return SYMCRYPT_SESSION_REPLAY_FAILURE
// Otherwise, set desiredState to the observedState updated to represent messageNumber has been seen
// and return SYMCRYPT_NO_ERROR
FORCEINLINE
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSessionDecryptComputeDesiredReplayState(
    _In_    PCSYMCRYPT_SESSION_REPLAY_STATE observedState,
    _Out_   PSYMCRYPT_SESSION_REPLAY_STATE  desiredState,
            UINT64                          messageNumber )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    UINT64 messageMask;
    UINT64 shiftAmount;
    UINT64 shiftedMask;

    if ( messageNumber > observedState->messageNumber )
    {
        // The observed message number is behind messageNumber that we want to mark successful
        // Shift replayMask appropriately to preserve previously seen message numbers
        shiftedMask = 0;
        shiftAmount = messageNumber - observedState->messageNumber;
        if( shiftAmount < 64 )
        {
            shiftedMask = observedState->replayMask << shiftAmount;
        }
        // Mark messageNumber as seen in the replayMask
        desiredState->replayMask = shiftedMask | 1;
        desiredState->messageNumber = messageNumber;
    }
    else if ( messageNumber <= observedState->messageNumber - 64 )
    {
        // The observed message number is too far ahead of messageNumber
        // We cannot hope to succeed
        scError = SYMCRYPT_SESSION_REPLAY_FAILURE;
        goto cleanup;
    }
    else
    {
        // The observed message number is ahead of or equal to messageNumber
        // Check if messageNumber has already been used
        messageMask = 1ull << (observedState->messageNumber - messageNumber); // shiftAmount is in [0, 63]
        if ((messageMask & observedState->replayMask) == messageMask)
        {
            scError = SYMCRYPT_SESSION_REPLAY_FAILURE;
            goto cleanup;
        }
        // This is first time we have seen messageNumber - set the replayMask bit appropriately
        desiredState->replayMask = observedState->replayMask | messageMask;
        desiredState->messageNumber = observedState->messageNumber;
    }

cleanup:
    return scError;
}

#if SYMCRYPT_USE_CAS128

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSessionDecryptUpdateStateCAS128(
    _Inout_ PSYMCRYPT_SESSION   pSession,
            UINT64              messageNumber )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    SYMCRYPT_SESSION_REPLAY_STATE expectedState;
    SYMCRYPT_SESSION_REPLAY_STATE desiredState;

    // Non-atomic read of pSession's replayState. We can use this initial value as a good guess of
    // the expected state, but we cannot fail based on it (as replayMask and messageNumber may have
    // been read from different writes to the replayState)
    expectedState = pSession->replayState;

    // Compute desiredState based on non-atomic read
    // If it looks like this may be a replay, ensure we fail first CAS so we recompute desiredState
    // from an atomic read in the loop below
    if ( SymCryptSessionDecryptComputeDesiredReplayState(&expectedState, &desiredState, messageNumber) != SYMCRYPT_NO_ERROR )
    {
        // pSession->replayState.messageNumber can never take the value 0 as it starts at 64 and is
        // monotonic increasing
        expectedState.messageNumber = 0;
    }

    while( scError == SYMCRYPT_NO_ERROR )
    {
        if ( SymCryptAtomicCas128Relaxed((PUINT64)&pSession->replayState, (PUINT64)&expectedState, (PUINT64)&desiredState) )
        {
            // We succeeded in updating pSession->replayState and are done
            break;
        }

        // Compute new desiredState based on atomic read from CAS failure
        // We may now correctly fall out of loop if a replay is detected
        scError = SymCryptSessionDecryptComputeDesiredReplayState(&expectedState, &desiredState, messageNumber);
    }

    return scError;
}

#endif

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSessionDecryptUpdateStateLock(
    _Inout_ PSYMCRYPT_SESSION   pSession,
            UINT64              messageNumber )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    SYMCRYPT_SESSION_REPLAY_STATE desiredState;

    if ( pSession->pMutex == NULL )
    {
        return SYMCRYPT_INVALID_ARGUMENT;
    }

    // Check whether we are definitely too late to proceed before attempting to acquire mutex
    // Do not need atomic read of full replayState here, but do need atomic 64b read of
    // pSession->replayState.messageNumber
    if ( messageNumber <= (UINT64) SYMCRYPT_ATOMIC_LOAD64_RELAXED(&pSession->replayState.messageNumber) - 64 )
    {
        return SYMCRYPT_SESSION_REPLAY_FAILURE;
    }

    SymCryptCallbackAcquireMutexFastInproc(pSession->pMutex);
    //////
    // !!! Do not return until we have called SymCryptCallbackReleaseMutexFastInproc !!!
    //////

    scError = SymCryptSessionDecryptComputeDesiredReplayState(&pSession->replayState, &desiredState, messageNumber);
    if ( scError == SYMCRYPT_NO_ERROR )
    {
        pSession->replayState = desiredState;
    }

    SymCryptCallbackReleaseMutexFastInproc(pSession->pMutex);

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSessionDecryptUpdateState(
    _Inout_ PSYMCRYPT_SESSION   pSession,
            UINT64              messageNumber )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

#if SYMCRYPT_CPU_AMD64
    if ( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_CMPXCHG16B ) )
    {
        scError = SymCryptSessionDecryptUpdateStateCAS128( pSession, messageNumber );
    }
    else
    {
        scError = SymCryptSessionDecryptUpdateStateLock( pSession, messageNumber );
    }
#elif SYMCRYPT_CPU_ARM64 // Arm64 always has support for CAS128 (possibly via LDXP + STXP)
    scError = SymCryptSessionDecryptUpdateStateCAS128( pSession, messageNumber );
#else // 32b and generic platforms will always need to use a lock
    scError = SymCryptSessionDecryptUpdateStateLock( pSession, messageNumber );
#endif

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSessionGcmDecrypt(
    _Inout_                         PSYMCRYPT_SESSION           pSession,
                                    UINT64                      messageNumber,
    _In_                            PCSYMCRYPT_GCM_EXPANDED_KEY pExpandedKey,
    _In_reads_opt_( cbAuthData )    PCBYTE                      pbAuthData,
                                    SIZE_T                      cbAuthData,
    _In_reads_( cbData )            PCBYTE                      pbSrc,
    _Out_writes_( cbData )          PBYTE                       pbDst,
                                    SIZE_T                      cbData,
    _In_reads_( cbTag )             PCBYTE                      pbTag,
                                    SIZE_T                      cbTag )
{
    BYTE    nonce[12];
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    if ( (pSession->flags & SYMCRYPT_FLAG_SESSION_ENCRYPT) != 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Check for messageNumbers which are too high or not valid
    if ( (messageNumber > SYMCRYPT_SESSION_MAX_MESSAGE_NUMBER) || (messageNumber == 0) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Check whether we are definitely too late to proceed before attempting to acquire mutex
    // Do not need atomic read of full replayState here, but do need atomic 64b read of
    // pSession->replayState.messageNumber
    if ( messageNumber <= (UINT64) SYMCRYPT_ATOMIC_LOAD64_RELAXED(&pSession->replayState.messageNumber) - 64 )
    {
        scError = SYMCRYPT_SESSION_REPLAY_FAILURE;
        goto cleanup;
    }

    SYMCRYPT_STORE_MSBFIRST32(&nonce[0], pSession->senderId);
    SYMCRYPT_STORE_MSBFIRST64(&nonce[4], messageNumber);

    scError = SymCryptGcmDecrypt(
        pExpandedKey,
        nonce,
        sizeof(nonce),
        pbAuthData,
        cbAuthData,
        pbSrc,
        pbDst,
        cbData,
        pbTag,
        cbTag);

    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup; // wipes pbDst twice, but we don't care about performance in the error case
    }

    scError = SymCryptSessionDecryptUpdateState(pSession, messageNumber);

cleanup:
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        SymCryptWipe( pbDst, cbData );
    }

    return scError;
}
