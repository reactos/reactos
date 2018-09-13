/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Regnccls.c

Abstract:

    This file contains functions needed for handling 
    change notifications in the classes portion of the registry

Author:

    Adam P. Edwards     (adamed)  14-Nov-1997

Key Functions:

    BaseRegNotifyClassKey

Notes:

--*/


#ifdef LOCAL

#include <rpc.h>
#include <string.h>
#include <wchar.h>
#include "regrpc.h"
#include "localreg.h"
#include "regclass.h"
#include "regnccls.h"
#include <malloc.h>

NTSTATUS BaseRegNotifyClassKey(
    IN  HKEY                     hKey,
    IN  HANDLE                   hEvent,
    IN  PIO_STATUS_BLOCK         pLocalIoStatusBlock,
    IN  DWORD                    dwNotifyFilter,
    IN  BOOLEAN                  fWatchSubtree,
    IN  BOOLEAN                  fAsynchronous)
{
    NTSTATUS           Status;
    HKEY               hkUser;
    HKEY               hkMachine;
    SKeySemantics      KeyInfo;
    UNICODE_STRING     EmptyString = {0, 0, 0};
    BYTE               rgNameBuf[REG_MAX_CLASSKEY_LEN + REG_CHAR_SIZE + sizeof(OBJECT_NAME_INFORMATION)];
    OBJECT_ATTRIBUTES  Obja;
    BOOL               fAllocatedPath;

    //
    // Set buffer to store info about this key
    //
    KeyInfo._pFullPath = (PKEY_NAME_INFORMATION) rgNameBuf;
    KeyInfo._cbFullPath = sizeof(rgNameBuf);

    //
    // get information about this key
    //
    Status = BaseRegGetKeySemantics(hKey, &EmptyString, &KeyInfo);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Initialize conditionally freed resources
    //
    hkUser = NULL;
    hkMachine = NULL;

    fAllocatedPath = FALSE;

    //
    // Now get handles for both user and machine versions of the key
    //
    Status = BaseRegGetUserAndMachineClass(
        &KeyInfo,
        hKey,
        KEY_NOTIFY,
        &hkUser,
        &hkMachine);

    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }

    if (fWatchSubtree || (hkUser && hkMachine)) {

        //
        // This will return the closest ancestor to the
        // nonexistent translated key -- note that it allocates memory
        // to the Obja.ObjectName member, so we need to free that on
        // success
        //
        Status = BaseRegGetBestAncestor(
            &KeyInfo,
            hkUser,
            hkMachine,
            &Obja);

        fAllocatedPath = Obja.ObjectName != NULL;

        if (!NT_SUCCESS(Status)) {
            goto cleanup;
        }

        //
        // Ask for the notify on both user and machine keys (or
        // the closest approximation).  Note that we pass a full path --
        // if we used an relative path with an object handle instead, we
        // would never have an opportunity to close the object, so we would
        // leak objects
        // 
        //
        Status = NtNotifyChangeMultipleKeys(
            hKey,
            1,
            &Obja,
            hEvent,
            NULL,
            NULL,
            pLocalIoStatusBlock,
            dwNotifyFilter,
            fWatchSubtree,
            NULL,
            0,
            fAsynchronous
            );

    } else {

        Status = NtNotifyChangeKey(
            hkUser ? hkUser : hkMachine,
            hEvent,
            NULL,
            NULL,
            pLocalIoStatusBlock,
            dwNotifyFilter,
            fWatchSubtree,
            NULL,
            0,
            fAsynchronous
            );
    }

cleanup:

    if (!NT_SUCCESS(Status)) {
        
        if (hkUser && (hkUser != hKey)) {
            NtClose(hkUser);
        }

        if (hkMachine && (hkMachine != hKey)) {
            NtClose(hkMachine);
        }
    }

    if (fAllocatedPath) {
        RegClassHeapFree(Obja.ObjectName);
    }

    return Status;
}

NTSTATUS BaseRegGetBestAncestor(
    IN SKeySemantics*      pKeySemantics,
    IN HKEY                hkUser,
    IN HKEY                hkMachine,
    IN POBJECT_ATTRIBUTES  pObja)
/*++

Routine Description:

    Finds a full object path for the closest ancestor for a key
    described by a key semantics structure


Arguments:
    
    pKeySemantics - contains information about a registry key
    hkUser        - handle to a user class version of the key above
    hkMachine     - handle to a machine class version of the key above
    pObja         - Object Attributes structure to initialize with a full
                    object path for the closest ancestor -- not that memory
                    is allocated for the ObjectName member of the structure
                    which must be freed by the caller -- caller should
                    check this member to see if it's non-NULL, regardless
                    of success code returned by function

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/
{
    USHORT             PrefixLen;
    NTSTATUS           Status;
    PUNICODE_STRING    pKeyPath;
    USHORT             uMaxLen;

    //
    // Allocate memory for the Obja's ObjectName member
    //
    uMaxLen = (USHORT) pKeySemantics->_pFullPath->NameLength +  REG_CLASSES_SUBTREE_PADDING;

    pKeyPath = RegClassHeapAlloc(uMaxLen + sizeof(*pKeyPath));

    if (!(pKeyPath)) {
        return STATUS_NO_MEMORY;
    }

    //
    // Now initialize the structure
    //
    pKeyPath->MaximumLength = uMaxLen;
    pKeyPath->Buffer = (WCHAR*) (((PBYTE) pKeyPath) + sizeof(*pKeyPath));

    //
    // Now form a version of this key path in the opposite tree
    //
    if (pKeySemantics->_fUser) {
            
        Status = BaseRegTranslateToMachineClassKey(
            pKeySemantics,
            pKeyPath,
            &PrefixLen);

    } else {

        Status = BaseRegTranslateToUserClassKey(
            pKeySemantics,
            pKeyPath,
            &PrefixLen);
    }
    
    //
    // Make sure the caller has a reference to allocated memory
    //
    pObja->ObjectName = pKeyPath;

    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }

    //
    // Set up the object attributes with this translated key so 
    // we can use the structure to notify keys
    //
    InitializeObjectAttributes(
        pObja,
        pKeyPath,
        OBJ_CASE_INSENSITIVE,
        NULL, // using absolute path, no hkey
        NULL);

    //
    // If we were supplied both keys, then they both exist,
    // so we can simply use the translated path above
    //
    if (hkUser && hkMachine) {
        goto cleanup;
    }

    //
    // At this point, we know the translated path doesn't exist,
    // since we only have a handle for one of the paths.  Therefore
    // we will attempt to find an approximation.  Note that the 
    // manipulation of KeyPath below affects the Obja passed in since
    // the Obja struct references KeyPath
    //
    do
    {
        WCHAR* pBufferEnd;
        HKEY   hkExistingKey;

        //
        // Find the last pathsep in the current key path
        //
        pBufferEnd = wcsrchr(pKeyPath->Buffer, L'\\');

        //
        // We should never get NULL here, because all keys
        // have the ancestory \Registry\User or \Registry\Machine,
        // each which have two pathseps to spare -- the loop
        // terminates once that path is shorter than those prefixes,
        // so we should never encounter this situation
        //
        ASSERT(pBufferEnd);

        //
        // Now truncate the string
        //
        *pBufferEnd = L'\0';

        //
        // Adjust the unicode string structure to conform
        // to the truncated string
        //
        RtlInitUnicodeString(pKeyPath, pKeyPath->Buffer);

        //
        // Now attempt to open with this truncated path
        //
        Status = NtOpenKey(
            &hkExistingKey,
            KEY_NOTIFY,
            pObja);

        //
        // If we do open it, we will close it and not pass this object
        // since we want our obja to use a full path and not a relative
        // path off a kernel object
        //
        if (NT_SUCCESS(Status)) {
            NtClose(hkExistingKey);
            break;
        }

        //
        // If we get any error besides a key not found error, then our reason
        // for failing the open is not because the key did not exist, but because
        // of some other error, most likely access denied.
        //
        if (STATUS_OBJECT_NAME_NOT_FOUND != Status) {
            break;
        }

    } while (pKeyPath->Length > PrefixLen);

cleanup:

    return Status;
    
}


#endif // defined ( LOCAL )













