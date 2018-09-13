//-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       tcrack.h
//
//  Contents:   The header of tcrack.cpp.  
//
//  History:    29-January-97   xiaohs   created
//              
//--------------------------------------------------------------------------

#ifndef __TCRACK_H__
#define __TCRACK_H__


#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <assert.h>


#include "wincrypt.h"


//--------------------------------------------------------------------------
//    Contant Defines
//--------------------------------------------------------------------------
#define CRYPT_DECODE_FLAG               CRYPT_DECODE_NOCOPY_FLAG 
#define CRYPT_ENCODE_TYPE               X509_ASN_ENCODING   

//--------------------------------------------------------------------------
//   Macros
//--------------------------------------------------------------------------

//Macros for memory management
#define SAFE_FREE(p1)   {if(p1) {LocalFree(p1);p1=NULL;}}  
#define SAFE_ALLOC(p1)  LocalAlloc(LPTR,p1)
#define CHECK_POINTER(pv) { if(!pv) goto TCLEANUP;}


//Macros for error checking
#define TESTC(rev,exp)   {if(!((rev)==(exp))) goto TCLEANUP; }

//--------------------------------------------------------------------------
//  Inline Function 
//--------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

//--------------------------------------------------------------------------
//   Function Prototype
//--------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//Error Manipulations
void    SetData(DWORD   cbNewData, BYTE *pbNewData,
                DWORD   *pcbOldData, BYTE **ppbOldData);


///////////////////////////////////////////////////////////////////////////////
//Certificate Manipulations
BOOL    Fix7FCert(DWORD cbEncoded, BYTE *pbEncoded, DWORD *pcbEncoded,
                        BYTE    **ppbEncoded);


BOOL    DecodeBLOB(LPCSTR   lpszStructType,DWORD cbEncoded, BYTE *pbEncoded,
                   DWORD    *pcbStructInfo, void **ppvStructInfo);

BOOL    EncodeStruct(LPCSTR lpszStructType, void *pStructInfo,DWORD *pcbEncoded,
                     BYTE **ppbEncoded);

BOOL    DecodeX509_CERT(DWORD   cbEncoded, BYTE *pbEncoded, DWORD *pcbEncoded,
                        BYTE    **ppbEncoded);

BOOL    DecodeX509_CERT_TO_BE_SIGNED(DWORD  cbEncoded, BYTE *pbEncoded, DWORD *pcbEncoded,
                        BYTE    **ppbEncoded);

BOOL    DecodeX509_NAME(DWORD   cbEncoded, BYTE *pbEncoded, DWORD *pcbEncoded,
                        BYTE    **ppbEncoded);




///////////////////////////////////////////////////////////////////////////////
//General Decode/Encode Testing routines
BOOL    BadCert(DWORD   cbEncoded, BYTE *pbEncoded);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // __TCRACK_H__
