/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#ifndef __INDICREARRANGEMENT_H
#define __INDICREARRANGEMENT_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "LayoutTables.h"
#include "StateTables.h"
#include "MorphTables.h"
#include "MorphStateTables.h"

U_NAMESPACE_BEGIN

struct IndicRearrangementSubtableHeader : MorphStateTableHeader
{
};

enum IndicRearrangementFlags
{
    irfMarkFirst    = 0x8000,
    irfDontAdvance  = 0x4000,
    irfMarkLast     = 0x2000,
    irfReserved     = 0x1FF0,
    irfVerbMask     = 0x000F
};

enum IndicRearrangementVerb
{
    irvNoAction = 0x0000,               /*   no action    */
    irvxA       = 0x0001,               /*    Ax => xA    */
    irvDx       = 0x0002,               /*    xD => Dx    */
    irvDxA      = 0x0003,               /*   AxD => DxA   */
        
    irvxAB      = 0x0004,               /*   ABx => xAB   */
    irvxBA      = 0x0005,               /*   ABx => xBA   */
    irvCDx      = 0x0006,               /*   xCD => CDx   */
    irvDCx      = 0x0007,               /*   xCD => DCx   */

    irvCDxA     = 0x0008,               /*  AxCD => CDxA  */
    irvDCxA     = 0x0009,               /*  AxCD => DCxA  */
    irvDxAB     = 0x000A,               /*  ABxD => DxAB  */
    irvDxBA     = 0x000B,               /*  ABxD => DxBA  */

    irvCDxAB    = 0x000C,               /* ABxCD => CDxAB */
    irvCDxBA    = 0x000D,               /* ABxCD => CDxBA */
    irvDCxAB    = 0x000E,               /* ABxCD => DCxAB */
    irvDCxBA    = 0x000F                /* ABxCD => DCxBA */
};

struct IndicRearrangementStateEntry : StateEntry
{
};

U_NAMESPACE_END
#endif

