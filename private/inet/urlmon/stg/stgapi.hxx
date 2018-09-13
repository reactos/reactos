//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       STGAPI.HXX
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-15-95    JoeS (Joe Souza)    Created
//
//----------------------------------------------------------------------------


HRESULT StgGetFillLockByteOnMem(IFillLockBytes **pFLB);
HRESULT StgGetFillLockByteOnFile(OLECHAR *pwcFileName, IFillLockBytes **pFLB);
HRESULT StgGetFillLockByteILockBytes(ILockBytes *pLB, IFillLockBytes **pFLB);

