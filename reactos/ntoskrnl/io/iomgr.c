/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 ntoskrnl/io/iomgr.c
 * PURPOSE:              Initializes the io manager
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *             29/07/98: Created
 */

/* INCLUDES ****************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/kernel.h>
#include <internal/objmgr.h>

/* GLOBALS *******************************************************************/

OBJECT_TYPE DeviceObjectType = {{NULL,0,0},
                                0,
                                0,
                                ULONG_MAX,
                                ULONG_MAX,
                                sizeof(DEVICE_OBJECT),
                                0,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               };
                           

/* FUNCTIONS ****************************************************************/

VOID IoInit(VOID)
{
   OBJECT_ATTRIBUTES attr;
   HANDLE handle;
   UNICODE_STRING string;
   ANSI_STRING astring;
   
   ObRegisterType(OBJTYP_DEVICE,&DeviceObjectType);

   RtlInitAnsiString(&astring,"\\Device");
   RtlAnsiStringToUnicodeString(&string,&astring,TRUE);
   InitializeObjectAttributes(&attr,&string,0,NULL,NULL);
   ZwCreateDirectoryObject(&handle,0,&attr);
   
   IoInitCancelHandling();
}
