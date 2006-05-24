/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ob/init.c
 * PURPOSE:         Handles Object Manager Initialization and Shutdown
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl
 *                  Thomas Weidenmueller (w3seek@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, ObInit)
#endif

GENERIC_MAPPING ObpTypeMapping =
{
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    0x000F0001
};

GENERIC_MAPPING ObpDirectoryMapping =
{
    STANDARD_RIGHTS_READ    | DIRECTORY_QUERY               |
    DIRECTORY_TRAVERSE,
    STANDARD_RIGHTS_WRITE   | DIRECTORY_CREATE_SUBDIRECTORY |
    DIRECTORY_CREATE_OBJECT,
    STANDARD_RIGHTS_EXECUTE | DIRECTORY_QUERY               |
    DIRECTORY_TRAVERSE,
    DIRECTORY_ALL_ACCESS
};

PDEVICE_MAP ObSystemDeviceMap = NULL;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
INIT_FUNCTION
ObInit(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name;
    SECURITY_DESCRIPTOR SecurityDescriptor;
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    OBP_LOOKUP_CONTEXT Context;

    /* Initialize the security descriptor cache */
    ObpInitSdCache();

    /* Initialize the Default Event */
    KeInitializeEvent(&ObpDefaultObject, NotificationEvent, TRUE );

    /* Create the Type Type */
    DPRINT("Creating Type Type\n");
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Type");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.ValidAccessMask = OBJECT_TYPE_ALL_ACCESS;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.MaintainTypeList = TRUE;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.GenericMapping = ObpTypeMapping;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(OBJECT_TYPE);
    ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &ObTypeObjectType);
  
    /* Create the Directory Type */
    DPRINT("Creating Directory Type\n");
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Directory");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.ValidAccessMask = DIRECTORY_ALL_ACCESS;
    ObjectTypeInitializer.UseDefaultObject = FALSE;
    ObjectTypeInitializer.MaintainTypeList = FALSE;
    ObjectTypeInitializer.GenericMapping = ObpDirectoryMapping;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(OBJECT_DIRECTORY);
    ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &ObDirectoryType);

    /* Create security descriptor */
    RtlCreateSecurityDescriptor(&SecurityDescriptor,
                                SECURITY_DESCRIPTOR_REVISION1);
    RtlSetOwnerSecurityDescriptor(&SecurityDescriptor,
                                  SeAliasAdminsSid,
                                  FALSE);
    RtlSetGroupSecurityDescriptor(&SecurityDescriptor,
                                  SeLocalSystemSid,
                                  FALSE);
    RtlSetDaclSecurityDescriptor(&SecurityDescriptor,
                                 TRUE,
                                 SePublicDefaultDacl,
                                 FALSE);

    /* Create root directory */
    DPRINT("Creating Root Directory\n");    
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_PERMANENT,
                               NULL,
                               &SecurityDescriptor);
    ObCreateObject(KernelMode,
                   ObDirectoryType,
                   &ObjectAttributes,
                   KernelMode,
                   NULL,
                   sizeof(OBJECT_DIRECTORY),
                   0,
                   0,
                   (PVOID*)&NameSpaceRoot);
    ObInsertObject((PVOID)NameSpaceRoot,
                   NULL,
                   DIRECTORY_ALL_ACCESS,
                   0,
                   NULL,
                   NULL);

    /* Create '\ObjectTypes' directory */
    RtlInitUnicodeString(&Name, L"\\ObjectTypes");
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_PERMANENT,
                               NULL,
                               &SecurityDescriptor);
    ObCreateObject(KernelMode,
                   ObDirectoryType,
                   &ObjectAttributes,
                   KernelMode,
                   NULL,
                   sizeof(OBJECT_DIRECTORY),
                   0,
                   0,
                   (PVOID*)&ObpTypeDirectoryObject);
    ObInsertObject((PVOID)ObpTypeDirectoryObject,
                   NULL,
                   DIRECTORY_ALL_ACCESS,
                   0,
                   NULL,
                   NULL);
    
    /* Insert the two objects we already created but couldn't add */
    /* NOTE: Uses TypeList & Creator Info in OB 2.0 */
    Context.Directory = ObpTypeDirectoryObject;
    Context.DirectoryLocked = TRUE;
    if (!ObpLookupEntryDirectory(ObpTypeDirectoryObject,
                                 &HEADER_TO_OBJECT_NAME(BODY_TO_HEADER(ObTypeObjectType))->Name,
                                 OBJ_CASE_INSENSITIVE,
                                 FALSE,
                                 &Context))
    {
        ObpInsertEntryDirectory(ObpTypeDirectoryObject, &Context, BODY_TO_HEADER(ObTypeObjectType));
    }
    if (!ObpLookupEntryDirectory(ObpTypeDirectoryObject,
                                 &HEADER_TO_OBJECT_NAME(BODY_TO_HEADER(ObDirectoryType))->Name,
                                 OBJ_CASE_INSENSITIVE,
                                 FALSE,
                                 &Context))
    {
        ObpInsertEntryDirectory(ObpTypeDirectoryObject, &Context, BODY_TO_HEADER(ObDirectoryType));
    }

    /* Create 'symbolic link' object type */
    ObInitSymbolicLinkImplementation();

    /* FIXME: Hack Hack! */
    ObSystemDeviceMap = ExAllocatePoolWithTag(NonPagedPool, sizeof(*ObSystemDeviceMap), TAG('O', 'b', 'D', 'm'));
    RtlZeroMemory(ObSystemDeviceMap, sizeof(*ObSystemDeviceMap));
}
/* EOF */
