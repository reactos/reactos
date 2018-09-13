//*************************************************************
//
//  Group Policy Processing
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1997-1998
//  All rights reserved
//
//  History:    28-Oct-98   SitaramR    Created
//
//*************************************************************


void InitializeGPOCriticalSection();
void CloseGPOCriticalSection();


//
// These keys are used in gpt.c. The per user per machine keys will
// be deleted when profile gets deleted. Changes in the following keys
// should be reflected in the prefixes as well...
//

#define GP_SHADOW_KEY         TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\Shadow\\%ws")
#define GP_HISTORY_KEY        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\History\\%ws")

#define GP_SHADOW_SID_KEY     TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\%ws\\Shadow\\%ws")
#define GP_HISTORY_SID_KEY    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\%ws\\History\\%ws")

#define GP_EXTENSIONS_KEY     TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions\\%ws")
#define GP_EXTENSIONS_SID_KEY TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\%ws\\GPExtensions\\%ws")

#define GP_HISTORY_SID_ROOT_KEY    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\%ws\\History")
#define GP_MEMBERSHIP_KEY          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\%ws\\GroupMembership")
#define GP_EXTENSIONS_SID_ROOT_KEY TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\%ws\\GPExtensions")

#define GP_POLICY_SID_KEY     TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\%ws")
#define GP_LOGON_SID_KEY      TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\%ws")

//
// Comon prefix for both history and shadow
//

#define GP_XXX_SID_PREFIX           TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy")
#define GP_EXTENSIONS_SID_PREFIX    TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")







