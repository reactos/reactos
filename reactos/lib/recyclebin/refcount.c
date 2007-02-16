/*
 * PROJECT:     Recycle bin management
 * LICENSE:     GPL v2 - See COPYING in the top level directory
 * FILE:        lib/recyclebin/openclose.c
 * PURPOSE:     Do reference counting on objects
 * PROGRAMMERS: Copyright 2006 Hervé Poussineau (hpoussin@reactos.org)
 */

#include "recyclebin_private.h"

BOOL
InitializeHandle(
	IN PREFCOUNT_DATA pData,
	IN PDESTROY_DATA pFnClose OPTIONAL)
{
	pData->ReferenceCount = 1;
	pData->Close = pFnClose;
	return TRUE;
}

BOOL
ReferenceHandle(
	IN PREFCOUNT_DATA pData)
{
	pData->ReferenceCount++;
	return TRUE;
}

BOOL
DereferenceHandle(
	IN PREFCOUNT_DATA pData)
{
	pData->ReferenceCount--;
	if (pData->ReferenceCount == 0)
	{
		if (pData->Close)
			return pData->Close(pData);
		else
			return TRUE;
	}
	return TRUE;
}
