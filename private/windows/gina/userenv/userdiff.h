//*************************************************************
//
//   userdiff.h     -   Header file for userdiff.c
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************


#define USERDIFF            TEXT("UserDiff")
#define USERDIFR_LOCATION   TEXT("%SystemRoot%\\system32\\config\\userdifr")
#define USERDIFF_LOCATION   TEXT("%SystemRoot%\\system32\\config\\userdiff")


//
// Hive processing key words
//

#define UD_ACTION              TEXT("Action")
#define UD_KEYNAME             TEXT("KeyName")
#define UD_VALUE               TEXT("Value")
#define UD_VALUENAME           TEXT("ValueName")
#define UD_VALUENAMES          TEXT("ValueNames")
#define UD_FLAGS               TEXT("Flags")
#define UD_ITEM                TEXT("Item")
#define UD_COMMANDLINE         TEXT("CommandLine")
#define UD_PRODUCTTYPE         TEXT("Product")



#define MAX_BUILD_NUMBER    30

typedef struct _UDNODE {
    TCHAR           szBuildNumber[MAX_BUILD_NUMBER];
    DWORD           dwBuildNumber;
    struct _UDNODE *pNext;
} UDNODE, * LPUDNODE;


BOOL ProcessUserDiff (LPPROFILE lpProfile, DWORD dwBuildNumber);
