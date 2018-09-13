/****************************** Module Header ******************************\
* Module Name: notify.h
*
* Copyright (c) 1992, Microsoft Corporation
*
* Handles notification of key value changes in the registry that affect
* the Program Manager.
*
* History:
* 04-16-92 JohanneC       Created.
\***************************************************************************/

//
// 2 watch events: common groups key & personal groups key
//
extern HANDLE gahEvents[2];

BOOL APIENTRY InitializeGroupKeyNotification();
VOID APIENTRY ResetProgramGroupsEvent(BOOL bCommonGroup);
VOID HandleGroupKeyChange(BOOL bPersonalGroup);
