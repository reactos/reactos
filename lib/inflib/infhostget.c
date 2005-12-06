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
                     const char *Section,
                     const char *Key,
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
                          const char *Key,
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
                         const char *Key,
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


long
InfHostGetLineCount(HINF InfHandle,
                    PCTSTR Section)
{
  return InfpGetLineCount(InfHandle, Section);
}


/* InfGetLineText */


long
InfHostGetFieldCount(PINFCONTEXT Context)
{
  return InfpGetFieldCount(Context);
}


int
InfHostGetBinaryField(PINFCONTEXT Context,
                      unsigned long FieldIndex,
                      unsigned char *ReturnBuffer,
                      unsigned long ReturnBufferSize,
                      unsigned long *RequiredSize)
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
                   unsigned long FieldIndex,
                   unsigned long *IntegerValue)
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
                       unsigned long FieldIndex,
                       char * ReturnBuffer,
                       unsigned long ReturnBufferSize,
                       unsigned long *RequiredSize)
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
                      unsigned long FieldIndex,
                      char *ReturnBuffer,
                      unsigned long ReturnBufferSize,
                      unsigned long *RequiredSize)
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
               char **Key,
               char **Data)
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
                    unsigned long FieldIndex,
                    char **Data)
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
