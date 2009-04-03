/*
 * PROJECT:         ReactOS Session Manager
 * LICENSE:         GPL v2 or later - See COPYING in the top level directory
 * FILE:            base/system/smss/initheap.c
 * PURPOSE:         Create the SM private heap.
 * PROGRAMMERS:     ReactOS Development Team
 */

/* INCLUDES ******************************************************************/
#include "smss.h"

#define NDEBUG
#include <debug.h>

HANDLE SmpHeap = NULL;

NTSTATUS
SmCreateHeap(VOID)
{
  /* Create our own heap */
  SmpHeap = RtlCreateHeap(HEAP_GROWABLE,
                          NULL,
                          65536,
                          65536,
                          NULL,
                          NULL);
  return (NULL == SmpHeap) ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS;
}

/* EOF */
