/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    nukesub.c

Abstract:

    SNMP subagent stress generator.

Author:

    Don Ryan (donryan)   21-Jan-1997

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <snmp.h>
#include <snmputil.h>
#include <stdio.h>


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private Procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

VOID
FreeAsnAny(
    AsnAny *asnAny
    )
{
    switch (asnAny->asnType) {

        case ASN_OBJECTIDENTIFIER:
            SnmpUtilOidFree(&asnAny->asnValue.object);
            break;

        case ASN_RFC1155_IPADDRESS:
        case ASN_RFC1155_OPAQUE:
        case ASN_OCTETSTRING:
            if (asnAny->asnValue.string.dynamic == TRUE) {
                SnmpUtilMemFree(asnAny->asnValue.string.stream);
            }
            break;

        default:
            break;
        }

    asnAny->asnType = ASN_NULL;
}
    
VOID
NukeSubagent(
    LPSTR SubagentDll
    )
{
    BOOL    fOk;
    HANDLE  DllHandle;
    FARPROC pfnSnmpExtensionInit   = NULL;
    FARPROC pfnSnmpExtensionInitEx = NULL;
    FARPROC pfnSnmpExtensionQuery  = NULL;
    AsnObjectIdentifier oidSupportedView;
    HANDLE  hPollForTrapEvent = NULL;
    RFC1157VarBindList variableBindings;
    INT loopIndex = 1;
    INT errorStatus;
    INT errorIndex;

    // attempt to load subagent dll
    DllHandle = LoadLibrary(SubagentDll);

    // validate handle
    if (DllHandle == NULL) { 
        printf("Could not load %s [error=%d]\n",SubagentDll,GetLastError());
        return;
    }

    // obtain pointers to the subagent interface functions
    pfnSnmpExtensionInit   = GetProcAddress(DllHandle,"SnmpExtensionInit");
    pfnSnmpExtensionInitEx = GetProcAddress(DllHandle,"SnmpExtensionInitEx");
    pfnSnmpExtensionQuery  = GetProcAddress(DllHandle,"SnmpExtensionQuery");       

    // validate function pointers
    if ((pfnSnmpExtensionInit  == NULL) ||
        (pfnSnmpExtensionQuery == NULL)) {
        printf("Could open subagent interface [error=%d]\n",GetLastError());
        goto cleanup;
    }
    
    printf("Initializing subagent...\n");

    // attempt to initialize sub
    fOk = (*pfnSnmpExtensionInit)(
                0,
                &hPollForTrapEvent,
                &oidSupportedView
                );

    if (fOk) {
        printf("Subagent supports %s\n",SnmpUtilOidToA(&oidSupportedView));
    } else {
        printf("Could initialize subagent [error=%d]\n",GetLastError());
        goto cleanup;
    }

    // loop if extended views supported
    while (fOk && pfnSnmpExtensionInitEx) {

        // re-initialize structure
        oidSupportedView.ids = NULL;
        oidSupportedView.idLength = 0;

        // attempt to obtain extended view information
        fOk = (*pfnSnmpExtensionInitEx)(&oidSupportedView);

        if (fOk) {
            printf("Subagent supports %s\n",SnmpUtilOidToA(&oidSupportedView));        
        }
    }

    printf("Starting stress...\n");

    while (1) {

        fOk = TRUE;

        errorStatus = SNMP_ERRORSTATUS_NOERROR;
        errorIndex  = 0;
    
        variableBindings.len  = 1;
        variableBindings.list = (RFC1157VarBind*)SnmpUtilMemAlloc(
            variableBindings.len * sizeof(RFC1157VarBind)
            );

        variableBindings.list->name.idLength = 2;
        variableBindings.list->name.ids = (UINT*)SnmpUtilMemAlloc(
            variableBindings.list->name.idLength * sizeof(UINT)
            );    
            
        variableBindings.list->value.asnType = ASN_NULL;

        while (fOk && (errorStatus == SNMP_ERRORSTATUS_NOERROR)) {

            fOk = (*pfnSnmpExtensionQuery)(
                        ASN_RFC1157_GETNEXTREQUEST, 
                        &variableBindings,
                        &errorStatus,
                        &errorIndex
                        );

            if (fOk && (errorStatus == SNMP_ERRORSTATUS_NOERROR)) {

                printf("                                         "
                       "                                      \r");
                printf("i=0x%lx,oid=%s\r", loopIndex,
                    SnmpUtilOidToA(&variableBindings.list->name));

                if (variableBindings.list->value.asnType == ASN_NULL) {
                    break;
                }

                FreeAsnAny(&variableBindings.list->value);
            }
        }

        SnmpUtilVarBindListFree(&variableBindings);
        loopIndex++;
    }    

cleanup:

    // free subagent dll
    FreeLibrary(DllHandle);
}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Entry point                                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

INT 
__cdecl 
main(
    IN INT   argc,
    IN LPSTR argv[]
    )

/*++

Routine Description:

    Program entry point. 

Arguments:

    argc - number of command line arguments.
    argv - pointer to array of command line arguments.

Return Values:

    Returns 0 if successful.

--*/

{
    if (argc == 2) {

        NukeSubagent(argv[1]);

    } else {

        printf("\nSYNTAX: nukesub.exe myagent.dll\n");
    }    
        
    return 0;
}
