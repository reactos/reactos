/*
 *  @doc    INTERNAL
 *
 *  @module UNIPROP.H -- Unicode property bits
 *
 *
 *  Owner: <nl>
 *      Michael Jochimsen <nl>
 *
 *  History: <nl>
 *      11/30/98     mikejoch created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#ifndef I__UNIPROP_H_
#define I__UNIPROP_H_
#pragma INCMSG("--- Beg 'uniprop.h'")

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

struct tagUNIPROP
{
	BYTE fNeedsGlyphing : 1;	// Partition needs glyphing
	BYTE fCombiningMark : 1;	// Partition consists of combining marks
	BYTE fZeroWidth		: 1;	// Characters in partition have zero width
	BYTE fUnused		: 5;	// Unused bits
};

typedef tagUNIPROP UNIPROP;

extern const UNIPROP s_aPropBitsFromCharClass[];

#pragma INCMSG("--- End 'uniprop.h'")
#else
#pragma INCMSG("*** Dup 'uniprop.h'")
#endif

