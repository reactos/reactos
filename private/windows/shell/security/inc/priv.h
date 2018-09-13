//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       priv.h
//
//--------------------------------------------------------------------------

#ifndef _PRIV_H_
#define _PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif

HANDLE  EnablePrivileges(PDWORD pdwPrivileges, ULONG cPrivileges);
void    ReleasePrivileges(HANDLE hToken);

#ifdef __cplusplus
}
#endif

#endif  // _PRIV_H_
