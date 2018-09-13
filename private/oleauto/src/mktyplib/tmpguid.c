/***
*tmpguid.c
*
*  Copyright (C) 1993, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  Temporary definitions of GUIDs that will eventually be picked
*  up from typeinfo libs.
*
*****************************************************************************/

#include "mktyplib.h"
#include <ole2.h>

// this redefines the DEFINE_GUID() macro to do allocation.
//
#include <initguid.h>

DEFINE_OLEGUID(IID_ITypeInfo,		0x00020401, 0, 0);
#ifdef MAC		// UNDONE: temporary for DIMALLOC hack
DEFINE_OLEGUID(IID_IUnknown,		0x00000000, 0, 0);
#endif //MAC
