/********************************************************************************
**    Copyright (c) 1998-1999 Microsoft Corporation. All Rights Reserved.
**
**       Portions Copyright (c) 1998-1999 Intel Corporation
**
********************************************************************************/

/* The file adapter.h was reviewed by LCA in June 2011 and is acceptable for use by Microsoft. */

#ifndef _ADAPTER_H_
#define _ADAPTER_H_

#include "shared.h"

/*****************************************************************************
 * Defines
 *****************************************************************************
 */
const ULONG MAX_MINIPORTS = 2;

/*****************************************************************************
 * Functions
 *****************************************************************************
 */
//
// both wave & topology miniport create function prototypes have this form:
//
typedef HRESULT (*PFNCREATEMINIPORT)(
    OUT PUNKNOWN *  Unknown,
    IN  REFCLSID    ClassId,
    IN  PUNKNOWN    OuterUnknown        OPTIONAL,
    IN  POOL_TYPE   PoolType
);

/*****************************************************************************
 * Externals
 *****************************************************************************
 */
extern NTSTATUS CreateAC97MiniportWaveRT
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
);

extern NTSTATUS CreateAC97MiniportWavePCI
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
);

extern NTSTATUS CreateAC97MiniportTopology
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
);

#endif  //_ADAPTER_H_
