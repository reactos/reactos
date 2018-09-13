//--------------------------------------------------------------------------;
//
//  File: dslevel.h
//
//  Copyright (c) 1997 Microsoft Corporation.  All rights reserved
//
//
//--------------------------------------------------------------------------;

#ifndef DSLEVEL_HEADER
#define DSLEVEL_HEADER

#include "advaudio.h"

HRESULT DSGetGuidFromName(LPTSTR szName, BOOL fRecord, LPGUID pGuid);
HRESULT DSGetCplValues(GUID guid, BOOL fRecord, LPCPLDATA pData);
HRESULT DSSetCplValues(GUID guid, BOOL fRecord, const LPCPLDATA pData);

#endif // DSLEVEL_HEADER