/****************************** Module Header ******************************\
* Module Name: sas.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Defines apis to perform event logging in winlogon.
*
* History:
* 03-19-93 Robertre     Created.
\***************************************************************************/

#ifdef LOGGING

BOOL
WriteLog(
    HANDLE FileHandle,
    LPWSTR LogString
    );

BOOL
OpenLogFile(
    PHANDLE LogFileHandle
    );

extern HANDLE LogFileHandle;

#endif
