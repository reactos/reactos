/****************************** Module Header ******************************\
* Module Name: envvar.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define apis in envvar.c
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

//
// Prototypes
//

BOOL
CreateUserEnvironment(
    PVOID *pEnv
    );

BOOL
SetUserEnvironmentVariable(
    PVOID *pEnv,
    LPTSTR lpVariable,
    LPTSTR lpValue,
    BOOL bOverwrite
    );

BOOL
SetHomeDirectoryEnvVars(
    PVOID *pEnv,
    LPTSTR lpHomeDirectory,
    LPTSTR lpHomeDrive,
    LPTSTR lpHomeShare,
    LPTSTR lpHomePath
    );


