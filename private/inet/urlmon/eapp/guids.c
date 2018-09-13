//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       GUIDS.C
//
//  Contents:   Defines GUIDS used in this DLL.
//
//  Classes:
//
//  Functions:
//
//  History:    11-02-95   JoeS (Joe Souza)     Created
//
//----------------------------------------------------------------------------
//#include <urlint.h>


#ifdef __cplusplus
extern "C" {
#endif


#ifndef GUID_DEFINED
#define GUID_DEFINED

typedef struct _GUID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} GUID;
#endif // GUID_DEFINED

const GUID CLSID_UMkSrvDll =
{
    0x79ead9d0, 0xbaf9, 0x11ce, {0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b }
};

const GUID IID_IDebugRegister       = { 0xc733e4f0, 0x576e, 0x11d0, {0xb2, 0x8c, 0x00, 0xc0, 0x4f, 0xd7, 0xcd, 0x22} };
const GUID IID_IDebugOut            = { 0xc733e4f1, 0x576e, 0x11d0, {0xb2, 0x8c, 0x00, 0xc0, 0x4f, 0xd7, 0xcd, 0x22} };

// Notification sink texs class ids
const GUID CLSID_NotificaitonTest1  = {0xc733e501, 0x576e, 0x11d0, {0xb2, 0x8c, 0x00, 0xc0, 0x4f, 0xd7, 0xcd, 0x22}};
const GUID CLSID_NotificaitonTest2  = {0xc733e502, 0x576e, 0x11d0, {0xb2, 0x8c, 0x00, 0xc0, 0x4f, 0xd7, 0xcd, 0x22}};
const GUID CLSID_NotificaitonTest3  = {0xc733e503, 0x576e, 0x11d0, {0xb2, 0x8c, 0x00, 0xc0, 0x4f, 0xd7, 0xcd, 0x22}};
const GUID CLSID_NotificaitonTest4  = {0xc733e504, 0x576e, 0x11d0, {0xb2, 0x8c, 0x00, 0xc0, 0x4f, 0xd7, 0xcd, 0x22}};


#ifdef __cplusplus
}
#endif

