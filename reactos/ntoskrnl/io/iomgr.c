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
#include <internal/objmgr.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

OBJECT_TYPE DeviceObjectType = {{0,0,NULL},
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

OBJECT_TYPE FileObjectType = {{0,0,NULL},
                                0,
                                0,
                                ULONG_MAX,
                                ULONG_MAX,
                                sizeof(FILE_OBJECT),
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
   
   /*
    * Register iomgr types
    */
   CHECKPOINT;
   RtlInitAnsiString(&astring,"Device");
   CHECKPOINT;
   RtlAnsiStringToUnicodeString(&DeviceObjectType.TypeName,&astring,TRUE);
   CHECKPOINT;
   ObRegisterType(OBJTYP_DEVICE,&DeviceObjectType);
   CHECKPOINT;
   RtlInitAnsiString(&astring,"File");
   RtlAnsiStringToUnicodeString(&FileObjectType.TypeName,&astring,TRUE);   
   ObRegisterType(OBJTYP_FILE,&FileObjectType);
   CHECKPOINT;
   /*
    * Create the device directory
    */
   RtlInitAnsiString(&astring,"\\Device");
   CHECKPOINT;
   RtlAnsiStringToUnicodeString(&string,&astring,TRUE);
   CHECKPOINT;
   InitializeObjectAttributes(&attr,&string,0,NULL,NULL);
   CHECKPOINT;
   ZwCreateDirectoryObject(&handle,0,&attr);
   CHECKPOINT;
   IoInitCancelHandling();
   IoInitSymbolicLinkImplementation();
}
