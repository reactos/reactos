/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Manage GDI Handle definitions
 * FILE:              subsys/win32k/eng/handle.h
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 29/8/1999: Created
 */

typedef struct _GDI_HANDLE {
   ULONG Handle;
   PVOID InternalObject;
   PVOID UserObject;
} GDI_HANDLE, *PGDI_HANDLE;

#define INVALID_HANDLE  0
#define MAX_GDI_HANDLES 4096

GDI_HANDLE GDIHandles[MAX_GDI_HANDLES];
ULONG HandleCounter = 1;


