/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    snmputil.c

Abstract:

    Sample SNMP Management API usage for Windows NT.

    This file is an example of how to code management applications using
    the SNMP Management API for Windows NT.  It is similar in operation to
    the other commonly available SNMP command line utilities.

    Extensive comments have been included to describe its structure and
    operation.  See also "Microsoft Windows NT SNMP Programmer's Reference".

--*/


// General notes:
//   Microsoft's SNMP Management API for Windows NT is implemented as a DLL
// that is linked with the developer's code.  These APIs (examples follow in
// this file) allow the developer's code to generate SNMP queries and receive
// SNMP traps.  A simple MIB compiler and related APIs are also available to
// allow conversions between OBJECT IDENTIFIERS and OBJECT DESCRIPTORS.


// Necessary includes.

#include <windows.h>

#include <stdio.h>
#include <string.h>

#include <snmp.h>
#include <mgmtapi.h>


// Constants used in this example.

#define GET     1
#define GETNEXT 2
#define WALK    3
#define TRAP    4

#define TIMEOUT 6000 /* milliseconds */
#define RETRIES 3

void
SNMP_FUNC_TYPE AsnValueFree(
    IN AsnAny *asnValue
    )
    {
    // Free any data in the varbind value
    switch ( asnValue->asnType )
        {
        case ASN_OBJECTIDENTIFIER:
            SnmpUtilOidFree( &asnValue->asnValue.object );
            break;

        case ASN_RFC1155_IPADDRESS:
        case ASN_RFC1155_OPAQUE:
        case ASN_OCTETSTRING:
            if ( asnValue->asnValue.string.dynamic == TRUE )
                {
                SnmpUtilMemFree( asnValue->asnValue.string.stream );
                }
            break;

        default:
            break;
            // Purposefully do nothing, because no storage alloc'ed for others
        }

    // Set type to NULL
    asnValue->asnType = ASN_NULL;
    }

// Main program.

INT __cdecl main(
    IN int  argumentCount,
    IN char *argumentVector[])
    {
    INT                operation;
    LPSTR              agent;
    LPSTR              community;
    RFC1157VarBindList variableBindings;
    LPSNMP_MGR_SESSION session;

    INT        timeout = TIMEOUT;
    INT        retries = RETRIES;

    BYTE       requestType;
    AsnInteger requestId;
    AsnInteger errorStatus;
    AsnInteger errorIndex;
    char        *chkPtr = NULL;


    // Parse command line arguments to determine requested operation.

    // Verify number of arguments...
    if      (argumentCount < 5 && argumentCount != 2)
        {
        printf("Error:  Incorrect number of arguments specified.\n");
        printf(
"\nusage:  snmputil [get|getnext|walk] agent community oid [oid ...]\n");
        printf(
  "        snmputil trap\n");

        return 1;
        }

    // Get/verify operation...
    argumentVector++;
    argumentCount--;
    if      (!strcmp(*argumentVector, "get"))
        operation = GET;
    else if (!strcmp(*argumentVector, "getnext"))
        operation = GETNEXT;
    else if (!strcmp(*argumentVector, "walk"))
        operation = WALK;
    else if (!strcmp(*argumentVector, "trap"))
        operation = TRAP;
    else
        {
        printf("Error:  Invalid operation, '%s', specified.\n",
               *argumentVector);

        return 1;
        }

    if (operation != TRAP)
        {
        if (argumentCount < 4)
            {
            printf("Error:  Incorrect number of arguments specified.\n");
            printf(
"\nusage:  snmputil [get|getnext|walk] agent community oid [oid ...]\n");
            printf(
  "        snmputil trap\n");

            return 1;
            }

        // Get agent address...
        argumentVector++;
        argumentCount--;
        agent = (LPSTR)SnmpUtilMemAlloc(strlen(*argumentVector) + 1);
        strcpy(agent, *argumentVector);

        // Get agent community...
        argumentVector++;
        argumentCount--;
        community = (LPSTR)SnmpUtilMemAlloc(strlen(*argumentVector) + 1);
        strcpy(community, *argumentVector);

        // Get oid's...
        variableBindings.list = NULL;
        variableBindings.len = 0;

        while(--argumentCount)
            {
            AsnObjectIdentifier reqObject;

            argumentVector++;

            // Convert the string representation to an internal representation.
            if (!SnmpMgrStrToOid(*argumentVector, &reqObject))
                {
                printf("Error: Invalid oid, %s, specified.\n", *argumentVector);

                return 1;
                }
            else
                {
                // Since sucessfull, add to the variable bindings list.
                variableBindings.len++;
                if ((variableBindings.list = (RFC1157VarBind *)SnmpUtilMemReAlloc(
                    variableBindings.list, sizeof(RFC1157VarBind) *
                    variableBindings.len)) == NULL)
                    {
                    printf("Error: Error allocating oid, %s.\n",
                           *argumentVector);

                    return 1;
                    }

                variableBindings.list[variableBindings.len - 1].name =
                    reqObject; // NOTE!  structure copy
                variableBindings.list[variableBindings.len - 1].value.asnType =
                    ASN_NULL;
                }
            } // end while()

        // Establish a SNMP session to communicate with the remote agent.  The
        // community, communications timeout, and communications retry count
        // for the session are also required.

        if ((session = SnmpMgrOpen(agent, community, timeout, retries)) == NULL)
            {
            printf("error on SnmpMgrOpen %d\n", GetLastError());

            return 1;
            }

        } // end if(TRAP)


    // Determine and perform the requested operation.

    if      (operation == GET || operation == GETNEXT)
        {
        // Get and GetNext are relatively simple operations to perform.
        // Simply initiate the request and process the result and/or
        // possible error conditions.


        if (operation == GET)
            requestType = ASN_RFC1157_GETREQUEST;
        else
            requestType = ASN_RFC1157_GETNEXTREQUEST;


        // Request that the API carry out the desired operation.

        if (!SnmpMgrRequest(session, requestType, &variableBindings,
                            &errorStatus, &errorIndex))
            {
            // The API is indicating an error.

            printf("error on SnmpMgrRequest %d\n", GetLastError());
            }
        else
            {
            // The API succeeded, errors may be indicated from the remote
            // agent.

            if (errorStatus > 0)
                {
                printf("Error: errorStatus=%d, errorIndex=%d\n",
                       errorStatus, errorIndex);
                }
            else
                {
                // Display the resulting variable bindings.

                UINT i;
                char *string = NULL;

                for(i=0; i < variableBindings.len; i++)
                    {
                    SnmpMgrOidToStr(&variableBindings.list[i].name, &string);
                    printf("Variable = %s\n", string);
                    if (string) SnmpUtilMemFree(string);

                    printf("Value    = ");
                    SnmpUtilPrintAsnAny(&variableBindings.list[i].value);

                    printf("\n");
                    } // end for()
                }
            }


        // Free the variable bindings that have been allocated.

        SnmpUtilVarBindListFree(&variableBindings);


        }
    else if (operation == WALK)
        {
        // Walk is a common term used to indicate that all MIB variables
        // under a given OID are to be traversed and displayed.  This is
        // a more complex operation requiring tests and looping in addition
        // to the steps for get/getnext above.

        UINT i;
        UINT j;

        AsnObjectIdentifier *rootOidList;
        UINT                 rootOidLen;
        UINT                *rootOidXlat;

        rootOidLen = variableBindings.len;

        rootOidList = (AsnObjectIdentifier*)SnmpUtilMemAlloc(rootOidLen *
                      sizeof(AsnObjectIdentifier));

        rootOidXlat = (UINT *)SnmpUtilMemAlloc(rootOidLen * sizeof(UINT));

        for (i=0; i < rootOidLen; i++)
            {
            SnmpUtilOidCpy(&rootOidList[i], &variableBindings.list[i].name);
            rootOidXlat[i] = i;
            }

        requestType = ASN_RFC1157_GETNEXTREQUEST;

        while(1)
            {
            if (!SnmpMgrRequest(session, requestType, &variableBindings,
                                &errorStatus, &errorIndex))
                {
                // The API is indicating an error.

                printf("error on SnmpMgrRequest %d\n", GetLastError());

                break;
                }
            else
                {
                // The API succeeded, errors may be indicated from the remote
                // agent.

                char *string = NULL;
                UINT nBindingsLeft = variableBindings.len;
                UINT nSubTreesDone = 0;
                RFC1157VarBind *tempVarBindList;

                if (errorStatus == SNMP_ERRORSTATUS_NOERROR)
                    {
                    // Test for end of subtree or end of MIB.

                    for(i=0; i < nBindingsLeft; i++)
                        {
                        // obtain root
                        j = rootOidXlat[i];

                        if (SnmpUtilOidNCmp(&variableBindings.list[i].name,
                                &rootOidList[j], rootOidList[j].idLength))
                            {
                            nSubTreesDone++;
                            rootOidXlat[i] = 0xffffffff;
                            }
                        else
                            {
                            SnmpMgrOidToStr(&variableBindings.list[i].name, &string);
                            printf("Variable = %s\n", string);
                            if (string) SnmpUtilMemFree(string);

                            printf("Value    = ");
                            SnmpUtilPrintAsnAny(&variableBindings.list[i].value);

                            printf("\n");
                            }

                        AsnValueFree(&variableBindings.list[i].value);
                        }

                        if (nBindingsLeft > 1)
                            {
                            printf("\n"); // separate table entries
                            }
                    }
                else if (errorStatus == SNMP_ERRORSTATUS_NOSUCHNAME)
                    {
                    if (!(errorIndex && (errorIndex <= (INT)nBindingsLeft)))
                        {
                        errorIndex = 1; // invalidate first variable
                        }

                    nSubTreesDone++;
                    rootOidXlat[errorIndex-1] = 0xffffffff;

                    errorStatus = 0;
                    errorIndex  = 0;
                    }
                else
                    {
                    printf("Error: errorStatus=%d, errorIndex=%d \n",
                           errorStatus, errorIndex);

                    break;
                    }

                // Test to see if any or all subtrees walked

                if (nSubTreesDone == 0)
                    {
                    continue;
                    }
                else if (nSubTreesDone >= nBindingsLeft)
                    {
                    printf("End of MIB subtree.\n\n");
                    break;
                    }

                // Fixup variable list

                tempVarBindList = variableBindings.list;

                variableBindings.len = nBindingsLeft - nSubTreesDone;

                variableBindings.list = (RFC1157VarBind *)SnmpUtilMemAlloc(
                    variableBindings.len * sizeof(RFC1157VarBind));

                for(i=0, j=0; i < nBindingsLeft; i++)
                    {
                    if ((rootOidXlat[i] != 0xffffffff) &&
                        (j < variableBindings.len))
                        {
                        SnmpUtilVarBindCpy(
                            &variableBindings.list[j],
                            &tempVarBindList[i]
                            );

                        rootOidXlat[j++] = rootOidXlat[i];
                        }

                    SnmpUtilVarBindFree(
                        &tempVarBindList[i]
                        );
                    }

                SnmpUtilMemFree(tempVarBindList);

                } // end if()

            } // end while()

        // Free the variable bindings that have been allocated.

        SnmpUtilVarBindListFree(&variableBindings);

        for (i=0; i < rootOidLen; i++)
            {
            SnmpUtilOidFree(&rootOidList[i]);
            }

        SnmpUtilMemFree(rootOidList);

        }
    else if (operation == TRAP)
        {
        // Trap handling can be done two different ways: event driven or
        // polled.  The following code illustrates the steps to use event
        // driven trap reception in a management application.


        HANDLE hNewTraps = NULL;


        if (!SnmpMgrTrapListen(&hNewTraps))
            {
            printf("error on SnmpMgrTrapListen %d\n", GetLastError());
            return 1;
            }
        else
            {
            printf("snmputil: listening for traps...\n");
            }


        while(1)
            {
            DWORD dwResult;

            if ((dwResult = WaitForSingleObject(hNewTraps, 0xffffffff))
                == 0xffffffff)
                {
                printf("error on WaitForSingleObject %d\n",
                       GetLastError());
                }
            else if (!ResetEvent(hNewTraps))
                {
                printf("error on ResetEvent %d\n", GetLastError());
                }
            else
                {
                AsnObjectIdentifier enterprise;
                AsnNetworkAddress   agentAddress;
                AsnNetworkAddress   sourceAddress;
                AsnInteger          genericTrap;
                AsnInteger          specificTrap;
                AsnOctetString      community;
                AsnTimeticks        timeStamp;
                RFC1157VarBindList  variableBindings;

                UINT i;
                char *string = NULL;

                while(SnmpMgrGetTrapEx(
                        &enterprise,
                        &agentAddress,
                        &sourceAddress,
                        &genericTrap,
                        &specificTrap,
                        &community,
                        &timeStamp,
                        &variableBindings))
                    {

                    printf("Incoming Trap:\n"
                           "  generic    = %d\n"
                           "  specific   = %d\n",
                           genericTrap,
                           specificTrap);

                    SnmpMgrOidToStr(&enterprise, &string);
                        printf ("  enterprise = %s\n", string);
                    if (string) 
                        SnmpUtilMemFree(string);
                    SnmpUtilOidFree(&enterprise);

                    if (agentAddress.length == 4) {
                        printf ("  agent      = %d.%d.%d.%d\n",
                             (int)agentAddress.stream[0],
                             (int)agentAddress.stream[1],
                             (int)agentAddress.stream[2],
                             (int)agentAddress.stream[3]);
                    }
                    if (agentAddress.dynamic) {
                        SnmpUtilMemFree(agentAddress.stream);
                    }

                    if (sourceAddress.length == 4) {
                        printf ("  source IP  = %d.%d.%d.%d\n",
                             (int)sourceAddress.stream[0],
                             (int)sourceAddress.stream[1],
                             (int)sourceAddress.stream[2],
                             (int)sourceAddress.stream[3]);
                    }
                    else if (sourceAddress.length == 10) {
                        printf ("  source IPX = %.2x%.2x%.2x%.2x."
                                "%.2x%.2x%.2x%.2x%.2x%.2x\n",
                             (int)sourceAddress.stream[0],
                             (int)sourceAddress.stream[1],
                             (int)sourceAddress.stream[2],
                             (int)sourceAddress.stream[3],
                             (int)sourceAddress.stream[4],
                             (int)sourceAddress.stream[5],
                             (int)sourceAddress.stream[6],
                             (int)sourceAddress.stream[7],
                             (int)sourceAddress.stream[8],
                             (int)sourceAddress.stream[9]);
                    }
                    if (sourceAddress.dynamic) {
                        SnmpUtilMemFree(sourceAddress.stream);
                    }

                    if (community.length)
                    {
                        string = SnmpUtilMemAlloc (community.length + 1);
                        memcpy (string, community.stream, community.length);
                        string[community.length] = '\0';
                        printf ("  community  = %s\n", string);
                        SnmpUtilMemFree(string);
                    }
                    if (community.dynamic) {
                        SnmpUtilMemFree(community.stream);
                    }

                    for(i=0; i < variableBindings.len; i++)
                        {
                        SnmpMgrOidToStr(&variableBindings.list[i].name, &string);
                        printf("  variable   = %s\n", string);
                        if (string) SnmpUtilMemFree(string);

                        printf("  value      = ");
                        SnmpUtilPrintAsnAny(&variableBindings.list[i].value);
                        } // end for()
                    printf("\n");


                    SnmpUtilVarBindListFree(&variableBindings);
                    }

                dwResult = GetLastError(); // check for errors...

                if ((dwResult != NOERROR) && (dwResult != SNMP_MGMTAPI_NOTRAPS))
                    {
                    printf("error on SnmpMgrGetTrap %d\n", dwResult);
                    }
                }

            } // end while()


        } // end if(operation)

    if (operation != TRAP)
        {
        // Close SNMP session with the remote agent.

        if (!SnmpMgrClose(session))
            {
            printf("error on SnmpMgrClose %d\n", GetLastError());

            return 1;
            }
        }


    // Let the command interpreter know things went ok.

    return 0;

    } // end main()
