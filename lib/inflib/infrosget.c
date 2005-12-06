/*
 * PROJECT:    .inf file parser
 * LICENSE:    GPL - See COPYING in the top level directory
 * PROGRAMMER: Royce Mitchell III
 *             Eric Kohl
 *             Ge van Geldorp <gvg@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "inflib.h"
#include "infros.h"

#define NDEBUG
#include <debug.h>


BOOLEAN
InfFindFirstLine(HINF InfHandle,
                 PCWSTR Section,
                 PCWSTR Key,
                 PINFCONTEXT *Context)
{
  return INF_SUCCESS(InfpFindFirstLine(InfHandle, Section, Key, Context));
}


BOOLEAN
InfFindNextLine(PINFCONTEXT ContextIn,
                 PINFCONTEXT ContextOut)
{
  return INF_SUCCESS(InfpFindNextLine(ContextIn, ContextOut));
}


BOOLEAN
InfFindFirstMatchLine(PINFCONTEXT ContextIn,
                      PCWSTR Key,
                      PINFCONTEXT ContextOut)
{
  return INF_SUCCESS(InfpFindFirstMatchLine(ContextIn, Key, ContextOut));
}


BOOLEAN
InfFindNextMatchLine(PINFCONTEXT ContextIn,
                     PCWSTR Key,
                     PINFCONTEXT ContextOut)
{
  return INF_SUCCESS(InfpFindNextMatchLine(ContextIn, Key, ContextOut));
}


LONG
InfGetLineCount(HINF InfHandle,
                PCWSTR Section)
{
  return InfpGetLineCount(InfHandle, Section);
}


/* InfGetLineText */


LONG
InfGetFieldCount(PINFCONTEXT Context)
{
  return InfpGetFieldCount(Context);
}


BOOLEAN
InfGetBinaryField(PINFCONTEXT Context,
                  ULONG FieldIndex,
                  PUCHAR ReturnBuffer,
                  ULONG ReturnBufferSize,
                  PULONG RequiredSize)
{
  return INF_SUCCESS(InfpGetBinaryField(Context, FieldIndex, ReturnBuffer,
                                        ReturnBufferSize, RequiredSize));
}


BOOLEAN
InfGetIntField(PINFCONTEXT Context,
               ULONG FieldIndex,
               PLONG IntegerValue)
{
  return INF_SUCCESS(InfpGetIntField(Context, FieldIndex, IntegerValue));
}


BOOLEAN
InfGetMultiSzField(PINFCONTEXT Context,
                   ULONG FieldIndex,
                   PWSTR ReturnBuffer,
                   ULONG ReturnBufferSize,
                   PULONG RequiredSize)
{
  return INF_SUCCESS(InfpGetMultiSzField(Context, FieldIndex, ReturnBuffer,
                                         ReturnBufferSize, RequiredSize));
}


BOOLEAN
InfGetStringField(PINFCONTEXT Context,
                  ULONG FieldIndex,
                  PWSTR ReturnBuffer,
                  ULONG ReturnBufferSize,
                  PULONG RequiredSize)
{
  return INF_SUCCESS(InfpGetStringField(Context, FieldIndex, ReturnBuffer,
                                        ReturnBufferSize, RequiredSize));
}


BOOLEAN
InfGetData(PINFCONTEXT Context,
           PWCHAR *Key,
           PWCHAR *Data)
{
  return INF_SUCCESS(InfpGetData(Context, Key, Data));
}


BOOLEAN
InfGetDataField (PINFCONTEXT Context,
                 ULONG FieldIndex,
                 PWCHAR *Data)
{
  return INF_SUCCESS(InfpGetDataField(Context, FieldIndex, Data));
}

VOID
InfFreeContext(PINFCONTEXT Context)
{
  InfpFreeContext(Context);
}

/* EOF */
