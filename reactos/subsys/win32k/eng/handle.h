/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Manage GDI Handle definitions
 * FILE:              subsys/win32k/eng/handle.h
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 29/8/1999: Created
 */
#ifndef __ENG_HANDLE_H
#define __ENG_HANDLE_H

#include "objects.h"
#include <include/object.h>

typedef struct _GDI_HANDLE {
  PENGOBJ		pEngObj;
} GDI_HANDLE, *PGDI_HANDLE;

#define INVALID_HANDLE  0
#define MAX_GDI_HANDLES 4096

GDI_HANDLE GDIHandles[MAX_GDI_HANDLES];

#define ValidEngHandle( x )  (!( (x) == INVALID_HANDLE ))

#endif
