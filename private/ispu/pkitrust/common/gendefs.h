//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       GenDefs.h
//
//  History:    31-Mar-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef GENDEFS_H
#define GENDEFS_H

#include        "cryptver.h"

#if (VER_PRODUCTMINOR > 101) // > IE4 (NT5 b2 and >)

#   define      USE_IEv4CRYPT32     0

#   include     "cryptui.h"
#   define      CVP_STRUCTDEF   CRYPTUI_VIEWCERTIFICATE_STRUCTW
#   define      CVP_DLL         "cryptui.dll"
#   define      CVP_FUNC_NAME   "CryptUIDlgViewCertificateW"

typedef BOOL (WINAPI *pfnCertViewProperties)(CVP_STRUCTDEF * pcvsa, BOOL *fRefreshChain);

#else                       // IE4 and <

#   define      USE_IEv4CRYPT32     1

#   include     "cryptdlg.h"
#   define      CVP_STRUCTDEF    CERT_VIEWPROPERTIES_STRUCT_A
#   define      CVP_DLL         "cryptdlg.dll"
#   define      CVP_FUNC_NAME   "CertViewPropertiesA"

typedef BOOL (WINAPI *pfnCertViewProperties)(CVP_STRUCTDEF * pcvsa);

#endif

#define EVER                (;;)

#ifdef _WINDLL
#   define      APIEXP
#   define      DLL32EXP    __declspec(dllexport)
#else
#   define      APIEXP
#   define      DLL32EXP
#endif

#define SignError()     (GetLastError() > (DWORD)0xFFFF) ? \
                            GetLastError() : \
                            HRESULT_FROM_WIN32(GetLastError())

#define _OFFSETOF(t,f)   ((DWORD)((DWORD_PTR)(&((t*)0)->f)))

#define _ISINSTRUCT(structtypedef, structpassedsize, member) \
                    ((_OFFSETOF(structtypedef, member) < structpassedsize) ? TRUE : FALSE)

#define WIDEN(sz,wsz)                                           \
    int cch##wsz = sz ? strlen(sz) + 1 : 0;                     \
    int cb##wsz  = cch##wsz * sizeof(WCHAR);                    \
    LPWSTR wsz = sz ? (LPWSTR)_alloca(cb##wsz) : NULL;          \
    if (wsz) MultiByteToWideChar(0, 0, sz, -1, wsz, cch##wsz)


#ifdef __cplusplus
#       define          DELETE_OBJECT(obj0)     if (obj0)           \
                                                                        {                   \
                                                                                delete obj0;    \
                                                                                obj0 = NULL;    \
                                                                        }
#else
#       define          DELETE_OBJECT(obj0)     if (obj0)           \
                                                                        {                   \
                                                                                free(obj0);     \
                                                                                obj0 = NULL;    \
                                                                        }
#endif


#endif // GENDEFS_H
