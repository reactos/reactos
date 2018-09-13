//+-------------------------------------------------------------------------
//
//  OLEDBSHL
//  Copyright (C) Microsoft, 1995
//
//  File:       oledbshl.h
//
//  Contents:   External exports of functions and types, in C style
//              for consumers like shell32.dll
//
//  History:    6-26-95  Davepl  Created
//
//--------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {            
#endif                  

HRESULT COFSFolder_CreateFromIDList(LPCITEMIDLIST pidl, REFIID riid, LPVOID * ppvOut);

#ifdef __cplusplus
}
#endif                  

