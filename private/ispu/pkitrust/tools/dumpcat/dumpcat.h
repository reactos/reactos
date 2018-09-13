//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       makecat.h
//
//  Contents:   Microsoft Internet Security Catalog utility
//
//  History:    06-May-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef MAKECAT_H
#define MAKECAT_H

extern BOOL         fVerbose;
extern BOOL         fFailAllErrors;
extern BOOL         fMoveAllCerts;
extern BOOL         fTesting;
extern DWORD        dwExpectedError;


extern WCHAR        *pwszCDFFile;

extern PrintfU_     *pPrint;

#endif // MAKECAT_H
