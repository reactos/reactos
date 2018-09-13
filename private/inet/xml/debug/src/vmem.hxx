/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
//+------------------------------------------------------------------------
//
//  File:       vmem.hxx
//
//  Contents:   Strict memory allocation utilities
//
//-------------------------------------------------------------------------

#ifndef I_VMEM_HXX_
#define I_VMEM_HXX_

#define VMEM_BACKSIDESTRICT     0x00000001
#define VMEM_BACKSIDEALIGN8     0x00000002

struct VMEMINFO
{
    size_t      cb;
    DWORD       dwFlags;
    size_t      cbFill1;
    size_t      cbFill2;
    void *      pv;
    void *      pvUser;
};

void *      VMemAlloc(size_t cb, DWORD dwFlags = VMEM_BACKSIDESTRICT, void * pvUser = NULL);
void *      VMemAllocClear(size_t cb, DWORD dwFlags = VMEM_BACKSIDESTRICT, void * pvUser = NULL);
HRESULT     VMemRealloc(void ** ppv, size_t cb, DWORD dwFlags = VMEM_BACKSIDESTRICT, void * pvUser = NULL);
void        VMemFree(void * pv);
size_t      VMemGetSize(void * pv);
VMEMINFO *  VMemIsValid(void * pv);

#endif
