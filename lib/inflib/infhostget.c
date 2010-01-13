/*
 * PROJECT:    .inf file parser
 * LICENSE:    GPL - See COPYING in the top level directory
 * PROGRAMMER: Royce Mitchell III
 *             Eric Kohl
 *             Ge van Geldorp <gvg@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "inflib.h"
#include "infhost.h"

#define NDEBUG
#include <debug.h>

int
InfHostFindFirstLine(HINF InfHandle,
                     const CHAR *Section,
                     const CHAR *Key,
                     PINFCONTEXT *Context)
{
  INFSTATUS Status;

  Status = InfpFindFirstLine(InfHandle, Section, Key, Context);
  if (INF_SUCCESS(Status))
    {
      return 0;
    }
  else
    {
      errno = Status;
      return -1;
    }
}


int
InfHostFindNextLine(PINFCONTEXT ContextIn,
                    PINFCONTEXT ContextOut)
{
  INFSTATUS Status;

  Status = InfpFindNextLine(ContextIn, ContextOut);
  if (INF_SUCCESS(Status))
    {
      return 0;
    }
  else
    {
      errno = Status;
      return -1;
    }
}


int
InfHostFindFirstMatchLine(PINFCONTEXT ContextIn,
                          const CHAR *Key,
                          PINFCONTEXT ContextOut)
{
  INFSTATUS Status;

  Status = InfpFindFirstMatchLine(ContextIn, Key, ContextOut);
  if (INF_SUCCESS(Status))
    {
      return 0;
    }
  else
    {
      errno = Status;
      return -1;
    }
}


int
InfHostFindNextMatchLine(PINFCONTEXT ContextIn,
                         const CHAR *Key,
                         PINFCONTEXT ContextOut)
{
  INFSTATUS Status;

  Status = InfpFindNextMatchLine(ContextIn, Key, ContextOut);
  if (INF_SUCCESS(Status))
    {
      return 0;
    }
  else
    {
      errno = Status;
      return -1;
    }
}


LONG
InfHostGetLineCount(HINF InfHandle,
                    PCTSTR Section)
{
  return InfpGetLineCount(InfHandle, Section);
}


/* InfGetLineText */


LONG
InfHostGetFieldCount(PINFCONTEXT Context)
{
  return InfpGetFieldCount(Context);
}


int
InfHostGetBinaryField(PINFCONTEXT Context,
                      ULONG FieldIndex,
                      UCHAR *ReturnBuffer,
                      ULONG ReturnBufferSize,
                      ULONG *RequiredSize)
{
  INFSTATUS Status;

  Status = InfpGetBinaryField(Context, FieldIndex, ReturnBuffer,
                              ReturnBufferSize, RequiredSize);
  if (INF_SUCCESS(Status))
    {
      return 0;
    }
  else
    {
      errno = Status;
      return -1;
    }
}


int
InfHostGetIntField(PINFCONTEXT Context,
                   ULONG FieldIndex,
                   ULONG *IntegerValue)
{
  INFSTATUS Status;

  Status = InfpGetIntField(Context, FieldIndex, (PLONG)IntegerValue);
  if (INF_SUCCESS(Status))
    {
      return 0;
    }
  else
    {
      errno = Status;
      return -1;
    }
}


int
InfHostGetMultiSzField(PINFCONTEXT Context,
                       ULONG FieldIndex,
                       CHAR * ReturnBuffer,
                       ULONG ReturnBufferSize,
                       ULONG *RequiredSize)
{
  INFSTATUS Status;

  Status = InfpGetMultiSzField(Context, FieldIndex, ReturnBuffer,
                               ReturnBufferSize, RequiredSize);
  if (INF_SUCCESS(Status))
    {
      return 0;
    }
  else
    {
      errno = Status;
      return -1;
    }
}


int
InfHostGetStringField(PINFCONTEXT Context,
                      ULONG FieldIndex,
                      CHAR *ReturnBuffer,
                      ULONG ReturnBufferSize,
                      ULONG *RequiredSize)
{
  INFSTATUS Status;

  Status = InfpGetStringField(Context, FieldIndex, ReturnBuffer,
                              ReturnBufferSize, RequiredSize);
  if (INF_SUCCESS(Status))
    {
      return 0;
    }
  else
    {
      errno = Status;
      return -1;
    }
}


int
InfHostGetData(PINFCONTEXT Context,
               CHAR **Key,
               CHAR **Data)
{
  INFSTATUS Status;

  Status = InfpGetData(Context, Key, Data);
  if (INF_SUCCESS(Status))
    {
      return 0;
    }
  else
    {
      errno = Status;
      return -1;
    }
}


int
InfHostGetDataField(PINFCONTEXT Context,
                    ULONG FieldIndex,
                    CHAR **Data)
{
  INFSTATUS Status;

  Status = InfpGetDataField(Context, FieldIndex, Data);
  if (INF_SUCCESS(Status))
    {
      return 0;
    }
  else
    {
      errno = Status;
      return -1;
    }
}

VOID
InfHostFreeContext(PINFCONTEXT Context)
{
  InfpFreeContext(Context);
}

/* EOF */
