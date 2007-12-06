/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/cm/ntfunc.c
 * PURPOSE:         Ntxxx function for registry access
 *
 * PROGRAMMERS:     Hartmut Birr
 *                  Casper Hornstrup
 *                  Alex Ionescu
 *                  Rex Jolliff
 *                  Eric Kohl
 *                  Filip Navara
 *                  Thomas Weidenmueller
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#include "cm.h"

/* GLOBALS ******************************************************************/

NTSTATUS
NTAPI
CmpCreateHandle(PVOID ObjectBody,
                ACCESS_MASK GrantedAccess,
                ULONG HandleAttributes,
                PHANDLE HandleReturn);

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
NtCreateKey(OUT PHANDLE KeyHandle,
            IN ACCESS_MASK DesiredAccess,
            IN POBJECT_ATTRIBUTES ObjectAttributes,
            IN ULONG TitleIndex,
            IN PUNICODE_STRING Class,
            IN ULONG CreateOptions,
            OUT PULONG Disposition)
{
    UNICODE_STRING RemainingPath = {0}, ReturnedPath = {0};
    ULONG LocalDisposition;
    PCM_KEY_BODY KeyObject, Parent;
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING ObjectName;
    OBJECT_CREATE_INFORMATION ObjectCreateInfo;
    ULONG i;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    HANDLE hKey;
    PCM_KEY_NODE Node, ParentNode;
    CM_PARSE_CONTEXT ParseContext = {0};
    PAGED_CODE();

    /* Setup the parse context */
    ParseContext.CreateOperation = TRUE;
    ParseContext.CreateOptions = CreateOptions;
    if (Class) ParseContext.Class = *Class;

    /* Capture all the info */
    Status = ObpCaptureObjectAttributes(ObjectAttributes,
                                        PreviousMode,
                                        FALSE,
                                        &ObjectCreateInfo,
                                        &ObjectName);
    if (!NT_SUCCESS(Status)) return Status;

    /* Find the key object */
    Status = CmFindObject(&ObjectCreateInfo,
                          &ObjectName,
                          (PVOID*)&Parent,
                          &ReturnedPath,
                          CmpKeyObjectType,
                          NULL,
                          NULL);
    if (!NT_SUCCESS(Status)) goto Cleanup;

    /* Check if we found the entire path */
    RemainingPath = ReturnedPath;
    if (!RemainingPath.Length)
    {
        /* Check if the parent has been deleted */
        if (Parent->KeyControlBlock->Delete)
        {
            /* Fail */
            DPRINT1("Object marked for delete!\n");
            Status = STATUS_UNSUCCESSFUL;
            goto Cleanup;
        }

        /* Create a new handle to the parent */
        Status = CmpCreateHandle(Parent,
                                 DesiredAccess,
                                 ObjectCreateInfo.Attributes,
                                 &hKey);
        if (!NT_SUCCESS(Status)) goto Cleanup;

        /* Tell the caller we did this */
        LocalDisposition = REG_OPENED_EXISTING_KEY;
        goto SuccessReturn;
    }

    /* Loop every leading slash */
    while ((RemainingPath.Length) &&
           (*RemainingPath.Buffer == OBJ_NAME_PATH_SEPARATOR))
    {
        /* And remove it */
        RemainingPath.Length -= sizeof(WCHAR);
        RemainingPath.MaximumLength -= sizeof(WCHAR);
        RemainingPath.Buffer++;
    }

    /* Loop every terminating slash */
    while ((RemainingPath.Length) && 
           (RemainingPath.Buffer[(RemainingPath.Length / sizeof(WCHAR)) - 1] ==
            OBJ_NAME_PATH_SEPARATOR))
    {
        /* And remove it */
        RemainingPath.Length -= sizeof(WCHAR);
        RemainingPath.MaximumLength -= sizeof(WCHAR);
    }

    /* Now loop the entire path */
    for (i = 0; i < RemainingPath.Length / sizeof(WCHAR); i++)
    {
        /* And check if we found slahes */
        if (RemainingPath.Buffer[i] == OBJ_NAME_PATH_SEPARATOR)
        {
            /* We don't create trees -- parent key doesn't exist, so fail */
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            goto Cleanup;
        }
    }

    /* Now check if we're left with no name by this point */
    if (!(RemainingPath.Length) || (RemainingPath.Buffer[0] == UNICODE_NULL))
    {
        /* Then fail since we can't do anything */
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Cleanup;
    }
    
    /* Lock the registry */
    CmpLockRegistry();

    /* Create the key */
    Status = CmpDoCreate(Parent->KeyControlBlock->KeyHive,
                         Parent->KeyControlBlock->KeyCell,
                         NULL,
                         &RemainingPath,
                         KernelMode,
                         &ParseContext,
                         Parent->KeyControlBlock,
                         (PVOID*)&KeyObject);
    if (!NT_SUCCESS(Status)) goto Cleanup;

    /* If we got here, this is a new key */
    LocalDisposition = REG_CREATED_NEW_KEY;

    /* Get the parent node and the child node */
    ParentNode = (PCM_KEY_NODE)HvGetCell(KeyObject->KeyControlBlock->ParentKcb->KeyHive,
                                         KeyObject->KeyControlBlock->ParentKcb->KeyCell);
    Node = (PCM_KEY_NODE)HvGetCell(KeyObject->KeyControlBlock->KeyHive,
                                   KeyObject->KeyControlBlock->KeyCell);

    /* Inherit some information */
    Node->Parent = KeyObject->KeyControlBlock->ParentKcb->KeyCell;
    Node->Security = ParentNode->Security;
    KeyObject->KeyControlBlock->ValueCache.ValueList = Node->ValueList.List;
    KeyObject->KeyControlBlock->ValueCache.Count = Node->ValueList.Count;

    /* Link child to parent */
    InsertTailList(&Parent->KeyControlBlock->KeyBodyListHead, &KeyObject->KeyBodyList);

    /* Create the actual handle to the object */
    Status = CmpCreateHandle(KeyObject,
                             DesiredAccess,
                             ObjectCreateInfo.Attributes,
                             &hKey);

    /* Free the create information */
    ObpFreeAndReleaseCapturedAttributes(OBJECT_TO_OBJECT_HEADER(KeyObject)->ObjectCreateInfo);
    OBJECT_TO_OBJECT_HEADER(KeyObject)->ObjectCreateInfo = NULL;
    if (!NT_SUCCESS(Status)) goto Cleanup;

    /* Add the keep-alive reference */
    ObReferenceObject(KeyObject);
    
    /* Unlock registry */
    CmpUnlockRegistry();

    /* Force a lazy flush */
    CmpLazyFlush();

SuccessReturn:
    /* Return data to user */
    *KeyHandle = hKey;
    if (Disposition) *Disposition = LocalDisposition;

Cleanup:
    /* Cleanup */
    ObpReleaseCapturedAttributes(&ObjectCreateInfo);
    if (ObjectName.Buffer) ObpFreeObjectNameBuffer(&ObjectName);
    RtlFreeUnicodeString(&ReturnedPath);
    if (Parent) ObDereferenceObject(Parent);
    return Status;
}

NTSTATUS
NTAPI
NtOpenKey(OUT PHANDLE KeyHandle,
          IN ACCESS_MASK DesiredAccess,
          IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    UNICODE_STRING RemainingPath = {0};
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PCM_KEY_BODY Object = NULL;
    HANDLE hKey = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING ObjectName;
    OBJECT_CREATE_INFORMATION ObjectCreateInfo;
    PAGED_CODE();
 
    /* Capture all the info */
    Status = ObpCaptureObjectAttributes(ObjectAttributes,
                                        PreviousMode,
                                        FALSE,
                                        &ObjectCreateInfo,
                                        &ObjectName);
    if (!NT_SUCCESS(Status)) return Status;
    
    /* Loop every terminating slash */
    while ((ObjectName.Length) && 
           (ObjectName.Buffer[(ObjectName.Length / sizeof(WCHAR)) - 1] ==
            OBJ_NAME_PATH_SEPARATOR))
    {
        /* And remove it */
        ObjectName.Length -= sizeof(WCHAR);
        ObjectName.MaximumLength -= sizeof(WCHAR);
    }
    
    /* Find the key */
    Status = CmFindObject(&ObjectCreateInfo,
                          &ObjectName,
                          (PVOID*)&Object,
                          &RemainingPath,
                          CmpKeyObjectType,
                          NULL,
                          NULL);
    if (!NT_SUCCESS(Status)) goto openkey_cleanup;

    /* Make sure we don't have any remaining path */
    if ((RemainingPath.Buffer) && (RemainingPath.Buffer[0] != UNICODE_NULL))
    {
        /* Fail */
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto openkey_cleanup;
    }

    /* Check if the key has been deleted */
    if (Object->KeyControlBlock->Delete)
    {
        /* Fail */
        Status = STATUS_UNSUCCESSFUL;
        goto openkey_cleanup;
    }

    /* Create the actual handle */
    Status = CmpCreateHandle(Object,
                             DesiredAccess,
                             ObjectCreateInfo.Attributes,
                             &hKey);

openkey_cleanup:
    /* Cleanup */
    ObpReleaseCapturedAttributes(&ObjectCreateInfo);
    if (ObjectName.Buffer) ObpFreeObjectNameBuffer(&ObjectName);
    RtlFreeUnicodeString(&RemainingPath);
    if (Object) ObDereferenceObject(Object);
    
    /* Return information and status to user */
    *KeyHandle = hKey;
    return Status;
}

/* EOF */
