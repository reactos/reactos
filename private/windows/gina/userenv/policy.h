//*************************************************************
//
//  Policy functions header file
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

BOOL ApplyPolicy (LPPROFILE lpProfile);

typedef struct _ADTTHREADINFO {
    LPPROFILE lpProfile;
    HDESK     hDesktop;
    FILETIME  ftPolicyFile;
    LPTSTR    lpADTPath;
} ADTTHREADINFO, *LPADTTHREADINFO;
