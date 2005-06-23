/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/mmtypes.h
 * PURPOSE:         Definitions for Memory Manager Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _MMTYPES_H
#define _MMTYPES_H

/* DEPENDENCIES **************************************************************/

/* EXPORTED DATA *************************************************************/

/* CONSTANTS *****************************************************************/

/* ENUMERATIONS **************************************************************/
typedef enum _PP_NPAGED_LOOKASIDE_NUMBER
{
    LookasideSmallIrpList = 0,
    LookasideLargeIrpList = 1,
    LookasideMdlList = 2,
    LookasideCreateInfoList = 3,
    LookasideNameBufferList = 4,
    LookasideTwilightList = 5,
    LookasideCompletionList = 6,
    LookasideMaximumList = 7
} PP_NPAGED_LOOKASIDE_NUMBER;

/* TYPES *********************************************************************/

typedef struct _PP_LOOKASIDE_LIST 
{
   struct _GENERAL_LOOKASIDE *P;
   struct _GENERAL_LOOKASIDE *L;
} PP_LOOKASIDE_LIST, *PPP_LOOKASIDE_LIST;

typedef struct _ADDRESS_RANGE
{
   ULONG BaseAddrLow;
   ULONG BaseAddrHigh;
   ULONG LengthLow;
   ULONG LengthHigh;
   ULONG Type;
} ADDRESS_RANGE, *PADDRESS_RANGE;

#endif
