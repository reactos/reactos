//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       storprov.h
//
//  Contents:   Microsoft Internet Security Trust Provider
//
//  History:    15-Oct-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef STORPROV_H
#define STORPROV_H

#ifdef __cplusplus
extern "C" 
{
#endif


#define     WVT_STOREID_ROOT        0
#define     WVT_STOREID_TRUST       1
#define     WVT_STOREID_CA          2
#define     WVT_STOREID_MY          3
#define     WVT_STOREID_SPC         4
#define     WVT_STOREID_MAX         5

typedef struct STORE_REF_
{
    DWORD       dwFlags;
    WCHAR       *pwszStoreName;
    HCERTSTORE  hStore;
} STORE_REF;


extern HCERTSTORE   StoreProviderGetStore(HCRYPTPROV hProv, DWORD dwStoreId);
extern BOOL         StoreProviderUnload(void);


#ifdef __cplusplus
}
#endif

#endif // STORPROV_H
