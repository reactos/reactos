/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: callback.c,v 1.11 2004/06/19 08:48:00 navaraf Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/callback.c
 * PURPOSE:         Executive callbacks
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 *                  Alex Ionescu (alex@relsoft.net)
 * PORTABILITY:     Checked.
 * UPDATE HISTORY:
 *                  Added all functions 30/05/04
 *                  Created 22/05/98
 * NOTE:
 *                  These funtions are not implemented in NT4, but
 *                  they are implemented in Win2k.
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <rosrtl/string.h>
#include <internal/debug.h>
#include <internal/ob.h>
#include <internal/callbacks.h>

/* FUNCTIONS *****************************************************************/

/*
 * ExpInitializeCallbacks
 *
 * FUNCTION:
 * Creates the Callback Object as a valid Object Type in the Kernel.
 *
 * ARGUMENTS:
 * None
 *
 * RETURNS:
 * TRUE if the Callback Object Type was successfully created.
 */
BOOLEAN
ExpInitializeCallbacks(VOID)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   NTSTATUS Status;
   UNICODE_STRING DirName;
   UNICODE_STRING CallbackName;
   HANDLE DirectoryHandle;
   ULONG i;

   /* Allocate memory for Object */
   ExCallbackObjectType = ExAllocatePoolWithTag(NonPagedPool, sizeof(OBJECT_TYPE), CALLBACK_TAG);

   /* Initialize name */
   RtlRosInitUnicodeStringFromLiteral(&ExCallbackObjectType->TypeName,L"Callback");

   /* Create the Object Type */
   ExCallbackObjectType->Tag = CALLBACK_TAG;
   ExCallbackObjectType->TotalObjects = 0;
   ExCallbackObjectType->TotalHandles = 0;
   ExCallbackObjectType->MaxObjects = 0xFFFFFFFF;
   ExCallbackObjectType->MaxHandles = 0xFFFFFFFF;
   ExCallbackObjectType->PagedPoolCharge = 0;
   ExCallbackObjectType->Dump = NULL;
   ExCallbackObjectType->Open = NULL;
   ExCallbackObjectType->Close = NULL;
   ExCallbackObjectType->Delete = NULL;
   ExCallbackObjectType->Parse = NULL;
   ExCallbackObjectType->Security = NULL;
   ExCallbackObjectType->QueryName = NULL;
   ExCallbackObjectType->DuplicationNotify = NULL;
   ExCallbackObjectType->OkayToClose = NULL;
   ExCallbackObjectType->Create = NULL;
   ExCallbackObjectType->Mapping = &ExpCallbackMapping;
   ExCallbackObjectType->NonpagedPoolCharge = sizeof(_INT_CALLBACK_OBJECT);
   Status = ObpCreateTypeObject(ExCallbackObjectType);

   /* Fail if it wasn't created successfully */
   if (!NT_SUCCESS(Status))
   {
      return FALSE;
   }

   /* Initialize the Object */
   RtlRosInitUnicodeStringFromLiteral(&DirName, L"\\Callback" );
   InitializeObjectAttributes(
      &ObjectAttributes,
      &DirName,
      OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
      NULL,
      NULL
   );

   /* Create the Object Directory */
   Status = NtCreateDirectoryObject(
               &DirectoryHandle,
               DIRECTORY_ALL_ACCESS,
               &ObjectAttributes
            );

   /* Fail if couldn't create */
   if (!NT_SUCCESS(Status))
   {
      return FALSE;
   }

   /* Close Handle... */
   NtClose(DirectoryHandle);

   /* Initialize Event used when unregistering */
   KeInitializeEvent(&ExpCallbackEvent, NotificationEvent, 0);

   /* Default NT Kernel Callbacks. */
   for (i=0; ExpInitializeCallback[i].CallbackObject; i++)
   {
      /* Create the name from the structure */
      RtlInitUnicodeString(&CallbackName, ExpInitializeCallback[i].Name);

      /* Initialize the Object Attributes Structure */
      InitializeObjectAttributes(
         &ObjectAttributes,
         &CallbackName,
         OBJ_PERMANENT | OBJ_CASE_INSENSITIVE,
         NULL,
         NULL
      );

      /* Create the Callback Object */
      Status = ExCreateCallback(
                  (PCALLBACK_OBJECT*)&(ExpInitializeCallback[i].CallbackObject),
                  &ObjectAttributes,
                  TRUE,
                  TRUE
               );

      /* Make sure Global Callbacks have been created */
      if (!NT_SUCCESS(Status))
      {
         return FALSE;
      }
   }
   /* Everything successful */
   return TRUE;
}

/*
 * ExCreateCallback
 *
 * FUNCTION:
 * Opens or creates a Callback Object. Creates only if Create is true.
 *  Allows multiple Callback Functions to be registred only if AllowMultipleCallbacks
 *  is true.
 *
 * ARGUMENTS:
 * CallbackObject = Pointer that will receive the Callback Object.
 * CallbackName = Name of Callback
 * Create = Determines if the object will be created if it doesn't exit
 * AllowMultipleCallbacks = Determines if more then one registered callback function
 *  can be attached to this Callback Object.
 *
 * RETURNS:
 * STATUS_SUCESS if not failed.
 *
 * @implemented
 */
NTSTATUS
STDCALL
ExCreateCallback(
   OUT PCALLBACK_OBJECT *CallbackObject,
   IN POBJECT_ATTRIBUTES ObjectAttributes,
   IN BOOLEAN Create,
   IN BOOLEAN AllowMultipleCallbacks
)
{
   PINT_CALLBACK_OBJECT Callback;
   NTSTATUS    Status;
   HANDLE     Handle;

   /* Open a handle to the callback if it exists */
   if (ObjectAttributes->ObjectName)
   {
      Status = ObOpenObjectByName(ObjectAttributes,
                                  ExCallbackObjectType,
                                  NULL,
                                  KernelMode,
                                  0,
                                  NULL,
                                  &Handle);
   }
   else
   {
      Status = STATUS_UNSUCCESSFUL;
   }

   /* We weren't able to open it...should we create it? */
   if(!NT_SUCCESS(Status) && Create )
   {
      Status = ObCreateObject(KernelMode,
                              ExCallbackObjectType,
                              ObjectAttributes,
                              KernelMode,
                              NULL,
                              sizeof(_INT_CALLBACK_OBJECT),
                              0,
                              0,
                              (PVOID *)&Callback );

      /* We Created it...let's initialize the structure now */
      if(NT_SUCCESS(Status))
      {
         KeInitializeSpinLock (&Callback->Lock);      /*  SpinLock   */
         InitializeListHead(&Callback->RegisteredCallbacks);   /*  Callback Entries */
         Callback->AllowMultipleCallbacks = AllowMultipleCallbacks; /*  Multiple Callbacks */
         Status = ObInsertObject (         /*  Create the object */
                     Callback,
                     NULL,
                     FILE_READ_DATA,
                     0,
                     NULL,
                     &Handle );
      }
   }
   if(NT_SUCCESS(Status))
   {

      /* Get a pointer to the new object from the handle we just got */
      Status = ObReferenceObjectByHandle (
                  Handle,
                  0,
                  ExCallbackObjectType,
                  KernelMode,
                  (PVOID)&Callback,
                  NULL
               );
      /* Close the Handle, since we now have the pointer */
      ZwClose(Handle);
   }

   /* Everything went fine, so return a pointer to the Object */
   if (NT_SUCCESS(Status))
   {
      *CallbackObject = (PCALLBACK_OBJECT)Callback;
   }
   return Status;
}

/*
 * ExNotifyCallback
 *
 * FUNCTION:
 * Calls a function pointer (a registered callback)
 *
 * ARGUMENTS:
 * CallbackObject - Which callback to call
 * Argument1 - Pointer/data to send to callback function
 * Argument2 - Pointer/data to send to callback function
 *
 * RETURNS:
 * Nothing
 *
 * @implemented
 */
VOID
STDCALL
ExNotifyCallback(
   IN PCALLBACK_OBJECT OpaqueCallbackObject,
   IN PVOID Argument1,
   IN PVOID Argument2
)
{
   PINT_CALLBACK_OBJECT    CallbackObject = (PINT_CALLBACK_OBJECT)OpaqueCallbackObject;
   PLIST_ENTRY             RegisteredCallbacks;
   PCALLBACK_REGISTRATION  CallbackRegistration;
   KIRQL                   OldIrql;

   /* Acquire the Lock */
   OldIrql = KfAcquireSpinLock(&CallbackObject->Lock);

   /* Enumerate through all the registered functions */
   for (RegisteredCallbacks = CallbackObject->RegisteredCallbacks.Flink;
         RegisteredCallbacks != &CallbackObject->RegisteredCallbacks;
         RegisteredCallbacks = RegisteredCallbacks->Flink)
   {

      /* Get a pointer to a Callback Registration from the List Entries */
      CallbackRegistration = CONTAINING_RECORD ( RegisteredCallbacks,
                             CALLBACK_REGISTRATION,
                             RegisteredCallbacks);

      /* Don't bother doing Callback Notification if it's pending to be deleted */
      if (!CallbackRegistration->PendingDeletion)
      {

         /* Mark the Callback in use, so it won't get deleted while we are calling it */
         CallbackRegistration->InUse += 1;

         /* Release the Spinlock before making the call */
         KfReleaseSpinLock(&CallbackObject->Lock, OldIrql);

         /* Call the Registered Function */
         CallbackRegistration->CallbackFunction (
            CallbackRegistration->CallbackContext,
            Argument1,
            Argument2
         );

         /* Get SpinLock back */
         OldIrql = KfAcquireSpinLock(&CallbackObject->Lock);

         /* We are not in use anymore */
         CallbackRegistration->InUse -= 1;

         /* If another instance of this function isn't running and deletion is pending, signal the event */
         if (CallbackRegistration->PendingDeletion  && CallbackRegistration->InUse == 0)
         {
            KeSetEvent(&ExpCallbackEvent, 0, FALSE);
         }
      }
   }
   /* Unsynchronize and release the Callback Object */
   KfReleaseSpinLock(&CallbackObject->Lock, OldIrql);
}

/*
 * ExRegisterCallback
 *
 * FUNCTION:
 * Allows a function to associate a callback pointer (Function) 
 *  to a created Callback object
 *
 * ARGUMENTS:
 * CallbackObject = The Object Created with ExCreateCallBack
 * CallBackFunction = Pointer to the function to be called back
 * CallBackContext = Block of memory that can contain user-data
 *  which will be passed on to the callback
 *
 * RETURNS:
 * A handle to a Callback Registration Structure (MSDN Documentation)
 *
 * @implemented
 */
PVOID
STDCALL
ExRegisterCallback(
   IN PCALLBACK_OBJECT OpaqueCallbackObject,
   IN PCALLBACK_FUNCTION CallbackFunction,
   IN PVOID CallbackContext
)
{
   PINT_CALLBACK_OBJECT CallbackObject = (PINT_CALLBACK_OBJECT)OpaqueCallbackObject;
   PCALLBACK_REGISTRATION  CallbackRegistration = NULL;
   KIRQL     OldIrql;

   /* Create reference to Callback Object */
   ObReferenceObject (CallbackObject);

   /* Allocate memory for the structure */
   CallbackRegistration = ExAllocatePoolWithTag(
                             NonPagedPool,
                             sizeof(CallbackRegistration),
                             CALLBACK_TAG
                          );
   /* Fail if memory allocation failed */
   if(!CallbackRegistration)
   {
      ObDereferenceObject (CallbackObject);
      return NULL;
   }

   /* Create Callback Registration */
   CallbackRegistration->CallbackObject = CallbackObject; /* When unregistering, drivers send a handle to the Registration, not the object...     */
   CallbackRegistration->CallbackFunction = CallbackFunction; /* NotifyCallback uses Objects, so this needs to be here in order to call the registered functions */
   CallbackRegistration->CallbackContext = CallbackContext; /* The documented NotifyCallback returns the Context, so we must save this somewhere    */

   /* Acquire SpinLock */
   OldIrql = KfAcquireSpinLock (&CallbackObject->Lock);

   /* Add Callback if 1) No Callbacks registered or 2) Multiple Callbacks allowed */
   if(CallbackObject->AllowMultipleCallbacks || IsListEmpty(&CallbackObject->RegisteredCallbacks))
   {
      InsertTailList(&CallbackObject->RegisteredCallbacks,&CallbackRegistration->RegisteredCallbacks)
   }
   else
   {
      ExFreePool(CallbackRegistration);
      CallbackRegistration = NULL;
   }

   /* Release SpinLock */
   KfReleaseSpinLock(&CallbackObject->Lock, OldIrql);

   /* Return handle to Registration Object */
   return (PVOID) CallbackRegistration;
}

/*
 * ExUnregisterCallback
 *
 * FUNCTION:
 * Deregisters a CallBack
 *
 * ARGUMENTS:
 * CallbackRegistration = Callback Registration Handle
 *
 * RETURNS:
 * Nothing
 *
 * @implemented
 */
VOID STDCALL
ExUnregisterCallback(
   IN PVOID CallbackRegistrationHandle
)
{
   PCALLBACK_REGISTRATION  CallbackRegistration;
   PINT_CALLBACK_OBJECT    CallbackObject;
   KIRQL                   OldIrql;

   /* Convert Handle to valid Structure Pointer */
   CallbackRegistration = (PCALLBACK_REGISTRATION) CallbackRegistrationHandle;

   /* Get the Callback Object */
   CallbackObject = CallbackRegistration->CallbackObject;

   /* Lock the Object */
   OldIrql = KfAcquireSpinLock (&CallbackObject->Lock);

   /* We can't Delete the Callback if it's in use, because this would create a call towards a null pointer => crash */
   while (CallbackRegistration->InUse)
   {

      /* Similarly, we also don't want to wait ages for all pending callbacks to be called */
      CallbackRegistration->PendingDeletion = TRUE;

      /* We are going to wait for the event, so the Lock isn't necessary */
      KfReleaseSpinLock (&CallbackObject->Lock, OldIrql);

      /* Make sure the event is cleared */
      KeClearEvent (&ExpCallbackEvent);

      /* Wait for the Event */
      KeWaitForSingleObject (
         &ExpCallbackEvent,
         Executive,
         KernelMode,
         FALSE,
         NULL
      );

      /* We need the Lock again */
      OldIrql = KfAcquireSpinLock(&CallbackObject->Lock);
   }

   /* Remove the Callback */
   RemoveEntryList(&CallbackRegistration->RegisteredCallbacks);

   /* It's now safe to release the lock */
   KfReleaseSpinLock(&CallbackObject->Lock, OldIrql);

   /* Delete this registration */
   ExFreePool(CallbackRegistration);

   /* Remove the reference */
   ObDereferenceObject(CallbackObject);
}

/* EOF */
