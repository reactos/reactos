/*
 * PROJECT:         ReactOS Storage Stack
 * LICENSE:         DDK - see license.txt in the root dir
 * FILE:            drivers/storage/cdrom/cdrom.c
 * PURPOSE:         CDROM driver
 * PROGRAMMERS:     Based on a source code sample from Microsoft NT4 DDK
 */

#include <ntddk.h>
#include <scsi.h>
#include <ntddscsi.h>
#include <ntdddisk.h>
#include <ntddcdrm.h>
#include <include/class2.h>
#include <stdio.h>

//#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
FindScsiAdapter (
    IN HANDLE KeyHandle,
    IN UNICODE_STRING ScsiUnicodeString[],
    OUT PUCHAR IntermediateController
    );

#define  INIT_OPEN_KEY(name, rootHandle, pNewHandle)       \
         InitializeObjectAttributes(                       \
             &objectAttributes,                            \
             (name),                                       \
             OBJ_CASE_INSENSITIVE,                         \
             (rootHandle),                                 \
             NULL                                          \
             );                                            \
                                                           \
         status = ZwOpenKey(                               \
                        (pNewHandle),                      \
                        KEY_READ | KEY_ENUMERATE_SUB_KEYS, \
                        &objectAttributes                  \
                        );


NTSTATUS
NTAPI
FindScsiAdapter (
    IN HANDLE KeyHandle,
    IN UNICODE_STRING ScsiUnicodeString[],
    OUT UCHAR *IntermediateController
    )

/*++

    Routine Description:

       Recursive routine to walk registry tree under KeyHandle looking for
       location of ScsiAdapter and other valid controllers that the ScsiAdapter
       might hang off of.  When ScsiAdapter is found, FindScsiAdapter
       returns an ARC name for the intervening controller(s) between the
       original key and ScsiAdapter.

    Arguments:

       KeyHandle  -- Handle of open registry key (somewhere under
                            \Registry\Machine\Hardware\Description\System)

       ScsiUnicodeString -- NT name of SCSI device being sought in the registry

       IntermediateController -- Null terminated buffer which this routine fills with
                                ARC name for intervening controller(s).  Null is returned
                                if ScsiAdapter sits at the root or if it is not found.

    Return Value:

        STATUS_SUCCESS -- IntermediateController set to something like multi(1)

        STATUS_OBJECT_PATH_NOT_FOUND -- all ok, but no ScsiAdapter
                                     (with correct scsi id & lun info) found;  In this case
                                     IntermediateController is explicitly set to null.

        Other status codes as returned by open\enumerate registry routines
                                     may also be returned.

--*/

{
#if 0
   NTSTATUS  status;
   ULONG        index;
   ULONG        resultLength;
   UCHAR        lowerController[64];
   BOOLEAN    validControllerNumber;
   UNICODE_STRING                unicodeString[64];
   OBJECT_ATTRIBUTES          objectAttributes;

   HANDLE      controllerHandle;
   HANDLE      controllerNumberHandle;
   ULONG        controllerIndex;
   ULONG        controllerNumberIndex;
   UCHAR        keyBuffer[256];                    // Allow for variable length name at end
   UCHAR        numberKeyBuffer[64];
   PKEY_BASIC_INFORMATION pControllerKeyInformation;
   PKEY_BASIC_INFORMATION pControllerNumberKeyInformation;

   // TODO:  Any PAGED_CODE stuff...

   //
   //  Walk enumerated subkeys, looking for valid controllers
   //

   for (controllerIndex = 0; TRUE; controllerIndex++) {

      //
      // Ensure pControllerKeyInformation->Name is null terminated
      //

      RtlZeroMemory(keyBuffer, sizeof(keyBuffer));

      pControllerKeyInformation = (PKEY_BASIC_INFORMATION) keyBuffer;

      status = ZwEnumerateKey(
                      KeyHandle,
                      controllerIndex,
                      KeyBasicInformation,
                      pControllerKeyInformation,
                      sizeof(keyBuffer),
                      &resultLength
                      );

      if (!NT_SUCCESS(status)) {

         if (status != STATUS_NO_MORE_ENTRIES) {
            DebugPrint ((2, "FindScsiAdapter: Error 0x%x enumerating key\n", status));
            return(status);
         }

         break;     // return NOT_FOUND
      }

      DebugPrint ((3, "FindScsiAdapter: Found Adapter=%S\n", pControllerKeyInformation->Name));

      if (!_wcsicmp(pControllerKeyInformation->Name, L"ScsiAdapter")) {

         //
         //  Found scsi, now verify that it's the same one we're trying to initialize.
         //

         INIT_OPEN_KEY (ScsiUnicodeString, KeyHandle, &controllerHandle);

         ZwClose(controllerHandle);

         if (NT_SUCCESS(status)) {

             //
             //  Found correct scsi, now build ARC name of IntermediateController
             //  (i.e. the intervening controllers, or everything above ScsiAdapter)
             //  start with null, and build string one controller at a time as we
             //  return up the recursively called routine.
             //

             IntermediateController = "\0";

             return (STATUS_SUCCESS);
          }

         //
         //  Found ScsiAdapter, but wrong scsi id or Lun info doesn't match,
         //  (ignore other ZwOpenKey errors &) keep looking...
         //

      }

      else if (!_wcsicmp(pControllerKeyInformation->Name, L"MultifunctionAdapter") ||
                !_wcsicmp(pControllerKeyInformation->Name, L"EisaAdapter")) {

         //
         // This is a valid controller that may have ScsiAdapter beneath it.
         //  Open controller & walk controller's subkeys: 0, 1, 2, etc....
         //

         RtlInitUnicodeString (unicodeString, pControllerKeyInformation->Name);

         INIT_OPEN_KEY (unicodeString, KeyHandle, &controllerHandle);

         if (!NT_SUCCESS(status)) {
            DebugPrint ((2, "FindScsiAdapter:  ZwOpenKey got status = %x \n", status));
            ZwClose (controllerHandle);
            return (status);
         }



         for (controllerNumberIndex = 0; TRUE; controllerNumberIndex++) {

            RtlZeroMemory(numberKeyBuffer, sizeof(numberKeyBuffer));

            pControllerNumberKeyInformation = (PKEY_BASIC_INFORMATION) numberKeyBuffer;

            status = ZwEnumerateKey(
                            controllerHandle,
                            controllerNumberIndex,
                            KeyBasicInformation,
                            pControllerNumberKeyInformation,
                            sizeof(numberKeyBuffer),
                            &resultLength
                            );

            if (!NT_SUCCESS(status)) {

               if (status != STATUS_NO_MORE_ENTRIES) {
                  DebugPrint ((2, "FindScsiAdapter: Status %x enumerating key\n", status));
                  ZwClose(controllerHandle);
                  return (status);
               }

               ZwClose(controllerHandle);

               break;   // next controller
            }

            DebugPrint ((3, "FindScsiAdapter: Found Adapter #=%S\n", pControllerNumberKeyInformation->Name));

            validControllerNumber = TRUE;

            for (index = 0; index < pControllerNumberKeyInformation->NameLength / 2; index++) {

               if (!isxdigit(pControllerNumberKeyInformation->Name[index])) {
                  validControllerNumber = FALSE;
                  break;
               }

            }

            if (validControllerNumber) {

               //
               //  Found valid controller and controller number: check children for scsi.
               //

               RtlInitUnicodeString (unicodeString, pControllerNumberKeyInformation->Name);

               INIT_OPEN_KEY (unicodeString, controllerHandle, &controllerNumberHandle);

               if (!NT_SUCCESS(status)) {
                  DebugPrint ((2, "FindScsiAdapter: Status %x opening controller number key\n", status));
                  ZwClose(controllerNumberHandle);
                  ZwClose(controllerHandle);
                  return (status);
               }

               RtlZeroMemory(lowerController, sizeof(lowerController));

               status = FindScsiAdapter(
                              controllerNumberHandle,
                              ScsiUnicodeString,
                              &lowerController[0]
                              );

               ZwClose(controllerNumberHandle);

               if (NT_SUCCESS(status)) {

                  //
                  //  SUCCESS!
                  //
                  //  Scsi adapter DOES exist under this node,
                  //  prepend Arc Name for the current adapter to whatever was returned
                  //  by other calls to this routine.
                  //

                  if (!_wcsicmp(pControllerKeyInformation->Name, L"MultifunctionAdapter")) {
                     sprintf(IntermediateController, "multi(0)%s", lowerController);
                  } else {
                     sprintf(IntermediateController, "eisa(0)%s", lowerController);
                  }

                  ZwClose(controllerHandle);

                  return(STATUS_SUCCESS);
               }

               else if (status != STATUS_OBJECT_PATH_NOT_FOUND) {
                  ZwClose(controllerHandle);
                  return(status);
               }

               //
               //  Scsi not found under this controller number, check next one
               //

            }  // if validControllerNumber

         }  // for controllerNumberIndex



         //
         //  ScsiAdapter not found under this controller
         //

         ZwClose(controllerHandle);

      }  // else if valid subkey (i.e., scsi, multi, eisa)

   }  // for controllerIndex

         //
         //  ScsiAdapter not found under key we were called with
         //

   IntermediateController = "\0";
#endif
   return (STATUS_OBJECT_PATH_NOT_FOUND);
}

