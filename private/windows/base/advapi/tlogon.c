//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       logon2.c
//
//  Contents:   Logon test app
//
//  Classes:
//
//  Functions:
//
//  History:    6-20-94   richardw   Created
//
//----------------------------------------------------------------------------



#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>

#include <windows.h>

#define DUMP_TOKEN  1
#define DUMP_HEX    2


char *User = NULL;
char *Domain = NULL;
char *Password = NULL;
char *SecPackage = NULL;
char *Cmd = NULL;

DWORD   fLogon = 0;
DWORD   fMe = 0;
DWORD   fService = 0;
DWORD   fCookie = 0;
FILE *  fOut;
DWORD   Threads;
DWORD   fDup = 0;
DWORD   LogonType = LOGON32_LOGON_INTERACTIVE;

char *  ImpLevels[] = { "Anonymous", "Identity", "Impersonation", "Delegation"};

char *  LogonTypes[] = { "Invalid", "Invalid", "Interactive", "Network", "Batch", "Service", "Proxy" };

void DumpToken(HANDLE hToken);

void
DoArgs(int argc,
        char **argv)

{
    int i;

    Threads = 1;

    if (argc < 3)
    {
        fprintf( fOut,"usage: %s <name> <domain> [-p pw] [-f flags] [-s] [-d] [-x cmd]\n", argv[0]);
        fprintf( fOut,"Tests logon path\n");
        fprintf( fOut," -p     \tOverride password\n");
        fprintf( fOut," -D     \tDump token\n");
        fprintf( fOut," -d     \tduplicate\n");
        fprintf( fOut," -s     \tLogon as service\n");
        fprintf( fOut," -x cmd \tStart cmd as user\n");
        fprintf( fOut," -o file\tSend output to file\n");
        fprintf( fOut," -t #   \tHit with # threads at once\n");
        fprintf( fOut," -l type\tLogon type\n");
        exit(1);
    }

    for (i = 1; i < argc ; i++ )
    {
        if (*argv[i] == '-')
        {
            switch (*(argv[i]+1))
            {
                case 'f':
                    fLogon = atoi(argv[++i]);
                    break;

                case 'd':
                    fDup = 1;
                    break;

                case 'D':
                    fMe |= DUMP_TOKEN;
                    break;

                case 'x':
                    Cmd = argv[++i];
                    break;

                case 'p':
                    Password = argv[++i];
                    break;

                case 't':
                    Threads = atoi(argv[++i]);
                    break;

                case 's':
                    LogonType = LOGON32_LOGON_SERVICE;
                    break;

                case 'l':
                    ++i;
                    if (argv[i] == NULL )
                    {
                        fprintf(fOut, "No logon type specified\n");
                        exit(1);
                    }
                    for (LogonType = 2 ;
                         LogonType < sizeof(LogonTypes) / sizeof(PSTR) ;
                         LogonType ++ )
                    {
                        if (_stricmp( LogonTypes[LogonType], argv[i]) == 0 )
                        {
                            break;
                        }
                    }

                    if (LogonType == (sizeof(LogonTypes) / sizeof(PSTR) ))
                    {
                        fprintf(fOut, "Invalid logon type '%s'\n", argv[i]);
                        exit(1);
                    }
                    break;

                case 'o':
                    fOut = fopen(argv[++i], "w");
                    if (!fOut)
                    {
                        fOut = stderr;
                    }
                    break;

                default:
                    fprintf( fOut,"Invalid switch %s\n", argv[i]);
                    exit(1);
            }
        }
        else
        {
            if (!User)
                User = argv[i];
             else
                if (!Domain)
                    Domain = argv[i];
        }
    }

    if (!Password)
        Password = User;
}

DWORD
DoIt(
    PVOID   pv)
{
    NTSTATUS         scRet, SubStatus, Status;
    PISID                   pSid;
    LUID                    Luid;
    TOKEN_GROUPS            TokenGroups;
    STRING                  sMe;
    HANDLE                  hToken;
    HANDLE                  hImp;
    HANDLE                  hDup;
    STRING                  sPackage;
    ULONG                   Package;
    LUID                    LogonId;
    TOKEN_SOURCE            TokenSource;
    char                    ImpersonateName[MAX_PATH];
    DWORD                   cbImpersonateName = MAX_PATH;
    STARTUPINFO             si;
    PROCESS_INFORMATION     pi;
    POBJECT_TYPE_INFORMATION    pTypeInfo;
    POBJECT_NAME_INFORMATION    pNameInfo;
    POBJECT_BASIC_INFORMATION   pBasicInfo;
    UCHAR   Buffer[1024];
    HANDLE  hWait;

    hWait = (HANDLE) pv;

    if (hWait != NULL)
    {
        WaitForSingleObjectEx( hWait, INFINITE, FALSE );
    }

    fprintf( fOut,"Logging on %s to %s\n", User, Domain);

    //
    // Copy the strings into the right places:
    //

    if (!LogonUserA(User, Domain, Password,
                LogonType,
                LOGON32_PROVIDER_DEFAULT, &hToken))
    {
        fprintf( fOut,"FAILED to logon, GetLastError is %d\n", GetLastError());
    }

    else
    {
        if (fMe & DUMP_TOKEN)
            DumpToken(hToken);

        if (!ImpersonateLoggedOnUser(hToken))
        {
            fprintf( fOut, "FAILED to impersonate, GetLastError is %d\n", GetLastError());
        }

        GetUserName(ImpersonateName, &cbImpersonateName);
        if (fDup)
        {
            if (OpenThreadToken( GetCurrentThread(),
                            MAXIMUM_ALLOWED,
                            TRUE,
                            &hImp))
            {
                DumpToken( hImp );
                if (DuplicateTokenEx(   hImp,
                                        MAXIMUM_ALLOWED,
                                        NULL,
                                        SecurityImpersonation,
                                        TokenPrimary,
                                        &hDup ) )
                {
                    fprintf( fOut, "Success!  Duplicated that token!\n");
                    DumpToken( hToken );
                    CloseHandle( hToken );
                }
                else
                {
                    fprintf( fOut, "DuplicateTokenEx FAILED, %d\n", GetLastError() );

                }

                CloseHandle( hImp );

            }
            else
            {
                fprintf( fOut, "OpenThreadToken FAILED, %d\n", GetLastError() );
            }

        }
        RevertToSelf();
        fprintf( fOut,"Hey look!  I'm %s\n", ImpersonateName);

        if (Cmd)
        {
            fprintf( fOut,"Starting '%s' as user\n", Cmd);
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            if (!CreateProcessAsUser(hToken, NULL, Cmd, NULL, NULL, FALSE,
                                CREATE_SEPARATE_WOW_VDM, NULL,
                                NULL, &si, &pi))
            {
                fprintf( fOut,"FAILED, %d\n", GetLastError());
            }

            fprintf( fOut,"Process Info:\n");
            fprintf( fOut,"  Process Handle    \t%x\n", pi.hProcess );
            fprintf( fOut,"  Thread Handle     \t%x\n", pi.hThread );
            fprintf( fOut,"  Process Id        \t%d\n", pi.dwProcessId );
            fprintf( fOut,"  Thread Id         \t%d\n", pi.dwThreadId );

            ZeroMemory( Buffer, 1024 );

            pTypeInfo = (POBJECT_TYPE_INFORMATION) Buffer;
            pNameInfo = (POBJECT_NAME_INFORMATION) Buffer;
            pBasicInfo = (POBJECT_BASIC_INFORMATION) Buffer;

            Status = NtQueryObject( pi.hProcess, ObjectTypeInformation, pTypeInfo, 1024, NULL );

            if (NT_SUCCESS(Status))
            {
                fprintf( fOut,"  Type         \t%ws\n", pTypeInfo->TypeName.Buffer );
            }

            ZeroMemory( Buffer, 1024 );
            Status = NtQueryObject(pi.hProcess, ObjectBasicInformation, pBasicInfo, 1024, NULL);
            if (NT_SUCCESS(Status))
            {
                fprintf( fOut,"  Attributes   \t%#x\n", pBasicInfo->Attributes );
                fprintf( fOut,"  GrantedAccess\t%#x\n", pBasicInfo->GrantedAccess );
                fprintf( fOut,"  HandleCount  \t%d\n", pBasicInfo->HandleCount );
                fprintf( fOut,"  PointerCount \t%d\n", pBasicInfo->PointerCount );
            }
            else
            {
                fprintf( fOut,"FAILED %x to query basic info\n", Status );
            }

            ZeroMemory( Buffer, 1024 );
            Status = NtQueryObject( pi.hProcess, ObjectNameInformation, pNameInfo, 1024, NULL );

            if (NT_SUCCESS(Status))
            {
                fprintf( fOut,"  Name         \t%ws\n", pNameInfo->Name.Buffer);
            }
            else
            {
                fprintf( fOut,"FAILED %x to query name info\n", Status );
            }

            CloseHandle( pi.hProcess );
            CloseHandle( pi.hThread );

        }
        CloseHandle(hToken);

    }



    return(0);

}


__cdecl
main (int argc, char *argv[])
{
    HANDLE  hWait;
    DWORD   i;
    DWORD   tid;
    HANDLE  hThreads[64];

    fOut = stdout;

    //
    // Get params
    //
    DoArgs(argc, argv);

    if (Threads == 1)
    {
        DoIt(NULL);

    }
    else
    {
        if (Threads > 64 )
        {
            Threads = 64;
        }

        hWait = CreateEvent( NULL, TRUE, FALSE, NULL );

        for (i = 0; i < Threads ; i++ )
        {
            hThreads[i] = CreateThread( NULL, 0, DoIt, hWait, 0, &tid);
        }

        SetEvent( hWait );

        WaitForMultipleObjectsEx( Threads, hThreads, TRUE, INFINITE, FALSE );

        for ( i = 0 ; i < Threads ; i++ )
        {
            CloseHandle( hThreads[i] );
        }
    }


    return( 0 );
}


#define SATYPE_USER     1
#define SATYPE_GROUP    2
#define SATYPE_PRIV     3


ULONG   PID;

void
DumpSid(PSID    pxSid)
{
    PISID   pSid = pxSid;
    int i, j =0;


    fprintf( fOut,"  S-%d-", pSid->Revision);
    for (i = 0;i < 6 ; i++ )
    {
        if (j)
        {
            fprintf( fOut,"%x", pSid->IdentifierAuthority.Value[i]);
        }
        else
        {
            if (pSid->IdentifierAuthority.Value[i])
            {
                j = 1;
                fprintf( fOut,"%x", pSid->IdentifierAuthority.Value[i]);
            }
        }
        if (i==4)
        {
            j = 1;
        }
    }
    for (i = 0; i < pSid->SubAuthorityCount ; i++ )
    {
        fprintf( fOut,(fMe & DUMP_HEX ? "-%x" : "-%lu"), pSid->SubAuthority[i]);
    }
}

void
DumpSidAttr(PSID_AND_ATTRIBUTES pSA,
            int                 SAType)
{
    DumpSid(pSA->Sid);

    if (SAType == SATYPE_GROUP)
    {
        fprintf( fOut,"\tAttributes - ");
        if (pSA->Attributes & SE_GROUP_MANDATORY)
        {
            fprintf( fOut,"Mandatory ");
        }
        if (pSA->Attributes & SE_GROUP_ENABLED_BY_DEFAULT)
        {
            fprintf( fOut,"Default ");
        }
        if (pSA->Attributes & SE_GROUP_ENABLED)
        {
            fprintf( fOut,"Enabled ");
        }
        if (pSA->Attributes & SE_GROUP_OWNER)
        {
            fprintf( fOut,"Owner ");
        }
        if (pSA->Attributes & SE_GROUP_LOGON_ID)
        {
            fprintf( fOut,"LogonId ");
        }
    }

}

CHAR *  GetPrivName(PLUID   pPriv)
{
    switch (pPriv->LowPart)
    {
        case SE_CREATE_TOKEN_PRIVILEGE:
            return(SE_CREATE_TOKEN_NAME);
        case SE_ASSIGNPRIMARYTOKEN_PRIVILEGE:
            return(SE_ASSIGNPRIMARYTOKEN_NAME);
        case SE_LOCK_MEMORY_PRIVILEGE:
            return(SE_LOCK_MEMORY_NAME);
        case SE_INCREASE_QUOTA_PRIVILEGE:
            return(SE_INCREASE_QUOTA_NAME);
        case SE_UNSOLICITED_INPUT_PRIVILEGE:
            return(SE_UNSOLICITED_INPUT_NAME);
        case SE_TCB_PRIVILEGE:
            return(SE_TCB_NAME);
        case SE_SECURITY_PRIVILEGE:
            return(SE_SECURITY_NAME);
        case SE_TAKE_OWNERSHIP_PRIVILEGE:
            return(SE_TAKE_OWNERSHIP_NAME);
        case SE_LOAD_DRIVER_PRIVILEGE:
            return(SE_LOAD_DRIVER_NAME);
        case SE_SYSTEM_PROFILE_PRIVILEGE:
            return(SE_SYSTEM_PROFILE_NAME);
        case SE_SYSTEMTIME_PRIVILEGE:
            return(SE_SYSTEMTIME_NAME);
        case SE_PROF_SINGLE_PROCESS_PRIVILEGE:
            return(SE_PROF_SINGLE_PROCESS_NAME);
        case SE_INC_BASE_PRIORITY_PRIVILEGE:
            return(SE_INC_BASE_PRIORITY_NAME);
        case SE_CREATE_PAGEFILE_PRIVILEGE:
            return(SE_CREATE_PAGEFILE_NAME);
        case SE_CREATE_PERMANENT_PRIVILEGE:
            return(SE_CREATE_PERMANENT_NAME);
        case SE_BACKUP_PRIVILEGE:
            return(SE_BACKUP_NAME);
        case SE_RESTORE_PRIVILEGE:
            return(SE_RESTORE_NAME);
        case SE_SHUTDOWN_PRIVILEGE:
            return(SE_SHUTDOWN_NAME);
        case SE_DEBUG_PRIVILEGE:
            return(SE_DEBUG_NAME);
        case SE_AUDIT_PRIVILEGE:
            return(SE_AUDIT_NAME);
        case SE_SYSTEM_ENVIRONMENT_PRIVILEGE:
            return(SE_SYSTEM_ENVIRONMENT_NAME);
        case SE_CHANGE_NOTIFY_PRIVILEGE:
            return(SE_CHANGE_NOTIFY_NAME);
        case SE_REMOTE_SHUTDOWN_PRIVILEGE:
            return(SE_REMOTE_SHUTDOWN_NAME);
        default:
            return("Unknown Privilege");
    }
}

void
DumpLuidAttr(PLUID_AND_ATTRIBUTES   pLA,
             int                    LAType)
{
    char *  PrivName;

    fprintf( fOut,"0x%x%08x", pLA->Luid.HighPart, pLA->Luid.LowPart);
    fprintf( fOut," %-32s", GetPrivName(&pLA->Luid));

    if (LAType == SATYPE_PRIV)
    {
        fprintf( fOut,"  Attributes - ");
        if (pLA->Attributes & SE_PRIVILEGE_ENABLED)
        {
            fprintf( fOut,"Enabled ");
        }

        if (pLA->Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT)
        {
            fprintf( fOut,"Default ");
        }
    }

}

void
DumpToken(HANDLE    hToken)
{
    PTOKEN_USER         pTUser;
    PTOKEN_GROUPS       pTGroups;
    PTOKEN_PRIVILEGES   pTPrivs;
    PTOKEN_OWNER        pTOwner;
    PTOKEN_PRIMARY_GROUP    pTPrimaryGroup;
    TOKEN_STATISTICS    TStats;
    ULONG               cbInfo;
    ULONG               cbRetInfo;
    NTSTATUS            status;
    DWORD               i;


    pTUser = malloc(256);


    status = GetTokenInformation(   hToken,
                                        TokenUser,
                                        pTUser,
                                        256,
                                        &cbRetInfo);

    if (!NT_SUCCESS(status))
    {
        fprintf( fOut,"FAILED querying token, %#x\n", status);
        return;
    }

    fprintf( fOut,"User\n  ");
    DumpSidAttr(&pTUser->User, SATYPE_USER);

    fprintf( fOut,"\nGroups");
    pTGroups = malloc(4096);
    status = GetTokenInformation(   hToken,
                                        TokenGroups,
                                        pTGroups,
                                        4096,
                                        &cbRetInfo);

    for (i = 0; i < pTGroups->GroupCount ; i++ )
    {
        fprintf( fOut,"\n %02d ", i);
        DumpSidAttr(&pTGroups->Groups[i], SATYPE_GROUP);
    }

    pTPrimaryGroup  = malloc(128);
    status = GetTokenInformation(   hToken,
                                        TokenPrimaryGroup,
                                        pTPrimaryGroup,
                                        128,
                                        &cbRetInfo);

    fprintf( fOut,"\nPrimary Group:\n  ");
    DumpSid(pTPrimaryGroup->PrimaryGroup);

    fprintf( fOut,"\nPrivs\n");
    pTPrivs = malloc(4096);
    status = GetTokenInformation(   hToken,
                                        TokenPrivileges,
                                        pTPrivs,
                                        4096,
                                        &cbRetInfo);

    for (i = 0; i < pTPrivs->PrivilegeCount ; i++ )
    {
        fprintf( fOut,"\n %02d ", i);
        DumpLuidAttr(&pTPrivs->Privileges[i], SATYPE_PRIV);
    }

    status = GetTokenInformation(   hToken,
                                        TokenStatistics,
                                        &TStats,
                                        sizeof(TStats),
                                        &cbRetInfo);

    fprintf( fOut, "\n\nAuth ID  %x:%x\n", TStats.AuthenticationId.HighPart, TStats.AuthenticationId.LowPart);
    fprintf( fOut, "TokenId     %x:%x\n", TStats.TokenId.HighPart, TStats.TokenId.LowPart);
    fprintf( fOut, "TokenType   %s\n", TStats.TokenType == TokenPrimary ? "Primary" : "Impersonation");
    fprintf( fOut, "Imp Level   %s\n", ImpLevels[ TStats.ImpersonationLevel ]);

}



