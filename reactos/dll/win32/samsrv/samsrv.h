/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Security Account Manager (LSA) Server
 * FILE:            reactos/dll/win32/samsrv/samsrv.h
 * PURPOSE:         Common header file
 *
 * PROGRAMMERS:     Eric Kohl
 */

#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/cmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/umtypes.h>
#include <ddk/ntsam.h>

#include <samsrv/samsrv.h>

#include "sam_s.h"

#include <wine/debug.h>

typedef enum _SAM_DB_OBJECT_TYPE
{
    SamDbIgnoreObject,
    SamDbContainerObject,
    SamDbServerObject,
    SamDbDomainObject,
    SamDbAliasObject,
    SamDbGroupObject,
    SamDbUserObject
} SAM_DB_OBJECT_TYPE;

typedef struct _SAM_DB_OBJECT
{
    ULONG Signature;
    SAM_DB_OBJECT_TYPE ObjectType;
    ULONG RefCount;
    ACCESS_MASK Access;
    HANDLE KeyHandle;
    struct _SAM_DB_OBJECT *ParentObject;
} SAM_DB_OBJECT, *PSAM_DB_OBJECT;

#define SAMP_DB_SIGNATURE 0x87654321

/* database.c */

NTSTATUS
SampInitDatabase(VOID);

NTSTATUS
SampCreateDbObject(IN PSAM_DB_OBJECT ParentObject,
                   IN LPWSTR ContainerName,
                   IN LPWSTR ObjectName,
                   IN SAM_DB_OBJECT_TYPE ObjectType,
                   IN ACCESS_MASK DesiredAccess,
                   OUT PSAM_DB_OBJECT *DbObject);

NTSTATUS
SampOpenDbObject(IN PSAM_DB_OBJECT ParentObject,
                 IN LPWSTR ContainerName,
                 IN LPWSTR ObjectName,
                 IN SAM_DB_OBJECT_TYPE ObjectType,
                 IN ACCESS_MASK DesiredAccess,
                 OUT PSAM_DB_OBJECT *DbObject);

NTSTATUS
SampValidateDbObject(SAMPR_HANDLE Handle,
                     SAM_DB_OBJECT_TYPE ObjectType,
                     ACCESS_MASK DesiredAccess,
                     PSAM_DB_OBJECT *DbObject);

NTSTATUS
SampCloseDbObject(PSAM_DB_OBJECT DbObject);

/* samspc.c */
VOID SampStartRpcServer(VOID);

/* setup.c */
BOOL SampIsSetupRunning(VOID);
BOOL SampInitializeSAM(VOID);