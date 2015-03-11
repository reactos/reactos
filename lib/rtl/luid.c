/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * PURPOSE:         Locally unique identifier (LUID) helper functions
 * FILE:            lib/rtl/luid.c
 * PROGRAMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

VOID NTAPI
RtlCopyLuid(PLUID LuidDest,
            PLUID LuidSrc)
{
   PAGED_CODE_RTL();

   LuidDest->LowPart = LuidSrc->LowPart;
   LuidDest->HighPart = LuidSrc->HighPart;
}


/*
 * @implemented
 */
VOID NTAPI
RtlCopyLuidAndAttributesArray(ULONG Count,
                              PLUID_AND_ATTRIBUTES Src,
                              PLUID_AND_ATTRIBUTES Dest)
{
   ULONG i;

   PAGED_CODE_RTL();

   for (i = 0; i < Count; i++)
   {
      RtlCopyMemory(&Dest[i],
                    &Src[i],
                    sizeof(LUID_AND_ATTRIBUTES));
   }
}


#undef RtlEqualLuid
/*
 * @implemented
 */
BOOLEAN NTAPI
RtlEqualLuid(PLUID Luid1,
             PLUID Luid2)
{
   PAGED_CODE_RTL();

   return (Luid1->LowPart == Luid2->LowPart &&
           Luid1->HighPart == Luid2->HighPart);
}

/* EOF */
