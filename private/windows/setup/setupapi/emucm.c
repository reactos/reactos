/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    emucm.c

Abstract:

    Routines to emulate new Cm API's

Author:

    Jamie Hunter (jamiehun) June 24 1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

static
CONFIGRET
WINAPI
EmuCM_Query_Resource_Conflict_List(
             OUT PCONFLICT_LIST pclConflictList,
             IN  DEVINST        dnDevInst,
             IN  RESOURCEID     ResourceID,
             IN  PCVOID         ResourceData,
             IN  ULONG          ResourceLen,
             IN  ULONG          ulFlags,
             IN  HMACHINE       hMachine
             );

static
CONFIGRET
WINAPI
EmuCM_Free_Resource_Conflict_Handle(
             IN CONFLICT_LIST   clConflictList
             );

static
CONFIGRET
WINAPI
EmuCM_Get_Resource_Conflict_Count(
             IN CONFLICT_LIST   clConflictList,
             OUT PULONG         pulCount
             );

static
CONFIGRET
WINAPI
EmuCM_Get_Resource_Conflict_Details(
             IN CONFLICT_LIST         clConflictList,
             IN ULONG                 ulIndex,
             IN OUT PCONFLICT_DETAILS pConflictDetails
             );

Proto_CM_Query_Resource_Conflict_List Dyn_CM_Query_Resource_Conflict_List = EmuCM_Query_Resource_Conflict_List;
Proto_CM_Free_Resource_Conflict_Handle Dyn_CM_Free_Resource_Conflict_Handle = EmuCM_Free_Resource_Conflict_Handle;
Proto_CM_Get_Resource_Conflict_Count Dyn_CM_Get_Resource_Conflict_Count = EmuCM_Get_Resource_Conflict_Count;
Proto_CM_Get_Resource_Conflict_Details Dyn_CM_Get_Resource_Conflict_Details = EmuCM_Get_Resource_Conflict_Details;

VOID
InitializeEmuCmStubs(VOID)
/*++

Routine Description:

    Routine to initialize CM API specific stubs

    This routine tries to load the function ptr of OS-provided APIs, and if
    they aren't available, stub versions are used instead.  We do this
    for APIs that are unimplemented on a platform that setupapi will
    run on.

Arguments:

    none

Return Value:

    none

--*/
{
    //
    // all or nothing - we make sure we can get all api's before we set the Dyn pointers
    //
    FARPROC Real_CM_Query_Resource_Conflict_List = NULL;
    FARPROC Real_CM_Free_Resource_Conflict_Handle = NULL;
    FARPROC Real_CM_Get_Resource_Conflict_Count = NULL;
    FARPROC Real_CM_Get_Resource_Conflict_Details = NULL;

    Real_CM_Query_Resource_Conflict_List = ObtainFnPtr ("cfgmgr32.dll", "CM_Query_Resource_Conflict_List", NULL);
    if (Real_CM_Query_Resource_Conflict_List) {
        Real_CM_Free_Resource_Conflict_Handle = ObtainFnPtr ("cfgmgr32.dll", "CM_Free_Resource_Conflict_Handle", NULL);
    }
    if (Real_CM_Free_Resource_Conflict_Handle) {
        Real_CM_Get_Resource_Conflict_Count = ObtainFnPtr ("cfgmgr32.dll", "CM_Get_Resource_Conflict_Count", NULL);
    }
    if (Real_CM_Get_Resource_Conflict_Count) {
#ifdef UNICODE
        Real_CM_Get_Resource_Conflict_Details = ObtainFnPtr ("cfgmgr32.dll", "CM_Get_Resource_Conflict_DetailsW", NULL);
#else
        Real_CM_Get_Resource_Conflict_Details = ObtainFnPtr ("cfgmgr32.dll", "CM_Get_Resource_Conflict_DetailsA", NULL);
#endif
    }
    if(Real_CM_Get_Resource_Conflict_Details) {
        //
        // we should have everything
        //
        MYASSERT(Real_CM_Query_Resource_Conflict_List);
        MYASSERT(Real_CM_Free_Resource_Conflict_Handle);
        MYASSERT(Real_CM_Get_Resource_Conflict_Count);
        MYASSERT(Real_CM_Get_Resource_Conflict_Details);

        Dyn_CM_Query_Resource_Conflict_List = (Proto_CM_Query_Resource_Conflict_List)Real_CM_Query_Resource_Conflict_List;
        Dyn_CM_Free_Resource_Conflict_Handle = (Proto_CM_Free_Resource_Conflict_Handle)Real_CM_Free_Resource_Conflict_Handle;
        Dyn_CM_Get_Resource_Conflict_Count = (Proto_CM_Get_Resource_Conflict_Count)Real_CM_Get_Resource_Conflict_Count;
        Dyn_CM_Get_Resource_Conflict_Details = (Proto_CM_Get_Resource_Conflict_Details)Real_CM_Get_Resource_Conflict_Details;
    }
}


static
CONFIGRET
WINAPI
EmuCM_Query_Resource_Conflict_List(
             OUT PCONFLICT_LIST pclConflictList,
             IN  DEVINST        dnDevInst,
             IN  RESOURCEID     ResourceID,
             IN  PCVOID         ResourceData,
             IN  ULONG          ResourceLen,
             IN  ULONG          ulFlags,
             IN  HMACHINE       hMachine
             )
/*++

Routine Description:

    Just a stub that returns failure

Arguments:

    none

Return Value:

    CR_FAILURE

--*/
{
    return CR_FAILURE;
}

static
CONFIGRET
WINAPI
EmuCM_Free_Resource_Conflict_Handle(
             IN CONFLICT_LIST   clConflictList
             )
/*++

Routine Description:

    Just a stub that returns failure

Arguments:

    none

Return Value:

    CR_FAILURE

--*/
{
    return CR_FAILURE;
}

static
CONFIGRET
WINAPI
EmuCM_Get_Resource_Conflict_Count(
             IN CONFLICT_LIST   clConflictList,
             OUT PULONG         pulCount
             )
/*++

Routine Description:

    Just a stub that returns failure

Arguments:

    none

Return Value:

    CR_FAILURE

--*/
{
    return CR_FAILURE;
}

static
CONFIGRET
WINAPI
EmuCM_Get_Resource_Conflict_Details(
             IN CONFLICT_LIST         clConflictList,
             IN ULONG                 ulIndex,
             IN OUT PCONFLICT_DETAILS pConflictDetails
             )
/*++

Routine Description:

    Just a stub that returns failure

Arguments:

    none

Return Value:

    CR_FAILURE

--*/
{
    return CR_FAILURE;
}


