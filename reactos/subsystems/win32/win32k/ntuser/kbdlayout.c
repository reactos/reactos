
/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/kbdlayout.c
 * PURPOSE:         Keyboard layout management
 * COPYRIGHT:       Copyright 2007 Saveliy Tretiakov
 *                 
 */
 

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

PKBL KBLList = NULL; // Keyboard layout list.

typedef PVOID (*KbdLayerDescriptor)(VOID);
NTSTATUS STDCALL LdrGetProcedureAddress(PVOID module,
                                        PANSI_STRING import_name,
                                        DWORD flags,
                                        PVOID *func_addr);


                                     
/* PRIVATE FUNCTIONS ******************************************************/
 

/*
 * Utility function to read a value from the registry more easily.
 *
 * IN  PUNICODE_STRING KeyName       -> Name of key to open
 * IN  PUNICODE_STRING ValueName     -> Name of value to open
 * OUT PUNICODE_STRING ReturnedValue -> String contained in registry
 *
 * Returns NTSTATUS
 */

static NTSTATUS NTAPI ReadRegistryValue( PUNICODE_STRING KeyName,
      PUNICODE_STRING ValueName,
      PUNICODE_STRING ReturnedValue )
{
   NTSTATUS Status;
   HANDLE KeyHandle;
   OBJECT_ATTRIBUTES KeyAttributes;
   PKEY_VALUE_PARTIAL_INFORMATION KeyValuePartialInfo;
   ULONG Length = 0;
   ULONG ResLength = 0;
   PWCHAR ReturnBuffer;
   
   InitializeObjectAttributes(&KeyAttributes, KeyName, OBJ_CASE_INSENSITIVE,
                              NULL, NULL);
   Status = ZwOpenKey(&KeyHandle, KEY_ALL_ACCESS, &KeyAttributes);
   if( !NT_SUCCESS(Status) )
   {
      return Status;
   }

   Status = ZwQueryValueKey(KeyHandle, ValueName, KeyValuePartialInformation,
                            0,
                            0,
                            &ResLength);

   if( Status != STATUS_BUFFER_TOO_SMALL )
   {
      NtClose(KeyHandle);
      return Status;
   }

   ResLength += sizeof( *KeyValuePartialInfo );
   KeyValuePartialInfo =
      ExAllocatePoolWithTag(PagedPool, ResLength, TAG_STRING);
   Length = ResLength;

   if( !KeyValuePartialInfo )
   {
      NtClose(KeyHandle);
      return STATUS_NO_MEMORY;
   }

   Status = ZwQueryValueKey(KeyHandle, 
      ValueName,
      KeyValuePartialInformation,
      (PVOID)KeyValuePartialInfo,
      Length,
      &ResLength);

   if( !NT_SUCCESS(Status) )
   {
      NtClose(KeyHandle);
      ExFreePool(KeyValuePartialInfo);
      return Status;
   }

   /* At this point, KeyValuePartialInfo->Data contains the key data */
   ReturnBuffer = ExAllocatePoolWithTag(PagedPool, 
      KeyValuePartialInfo->DataLength, 
      TAG_STRING);
   
   if(!ReturnBuffer)
   {
      NtClose(KeyHandle);
      ExFreePool(KeyValuePartialInfo);
      return STATUS_NO_MEMORY;
   }
   
   RtlCopyMemory(ReturnBuffer, 
      KeyValuePartialInfo->Data, 
      KeyValuePartialInfo->DataLength);
   RtlInitUnicodeString(ReturnedValue, ReturnBuffer);

   ExFreePool(KeyValuePartialInfo);
   NtClose(KeyHandle);

   return Status;
}

static BOOL UserLoadKbdDll(WCHAR *wsKLID, 
   HANDLE *phModule, 
   PKBDTABLES *pKbdTables)
{
   NTSTATUS Status;
   KbdLayerDescriptor layerDescGetFn;
   ANSI_STRING kbdProcedureName;
   UNICODE_STRING LayoutKeyName;
   UNICODE_STRING LayoutValueName;
   UNICODE_STRING LayoutFile;
   UNICODE_STRING FullLayoutPath;
   UNICODE_STRING klid;
   WCHAR LayoutPathBuffer[MAX_PATH] = L"\\SystemRoot\\System32\\";
   WCHAR KeyNameBuffer[MAX_PATH] = L"\\REGISTRY\\Machine\\SYSTEM\\"
                   L"CurrentControlSet\\Control\\KeyboardLayouts\\";
   
   RtlInitUnicodeString(&klid, wsKLID);
   RtlInitUnicodeString(&LayoutValueName,L"Layout File");   
   RtlInitUnicodeString(&LayoutKeyName, KeyNameBuffer);
   LayoutKeyName.MaximumLength = sizeof(KeyNameBuffer);
   
   RtlAppendUnicodeStringToString(&LayoutKeyName, &klid);
   Status = ReadRegistryValue(&LayoutKeyName, &LayoutValueName, &LayoutFile);
   
   if(!NT_SUCCESS(Status))
   {
      DPRINT1("Can't get layout filename for %wZ. (%08lx)\n", klid, Status);
      return FALSE;
   }
   
   DPRINT("Read registry and got %wZ\n", &LayoutFile);
   RtlInitUnicodeString(&FullLayoutPath, LayoutPathBuffer);
   FullLayoutPath.MaximumLength = sizeof(LayoutPathBuffer);
   RtlAppendUnicodeStringToString(&FullLayoutPath, &LayoutFile);
   DPRINT("Loading Keyboard DLL %wZ\n", &FullLayoutPath);
   RtlFreeUnicodeString(&LayoutFile);

   *phModule = EngLoadImage(FullLayoutPath.Buffer);
   
   if(*phModule) 
   {
      DPRINT("Loaded %wZ\n", &FullLayoutPath);
      
      RtlInitAnsiString( &kbdProcedureName, "KbdLayerDescriptor" );
      LdrGetProcedureAddress((PVOID)*phModule,
                             &kbdProcedureName,
                             0,
                             (PVOID*)&layerDescGetFn);
   
      if(layerDescGetFn)
      {
         *pKbdTables = layerDescGetFn();
      }
      else
      {
         DPRINT1("Error: %wZ has no KbdLayerDescriptor()\n", &FullLayoutPath);
      }

      if(!layerDescGetFn || !*pKbdTables)
      {
         DPRINT1("Failed to load the keyboard layout.\n");
         EngUnloadImage(*phModule);
         return FALSE;
      }  
   }
   else
   {
      DPRINT1("Failed to load dll %wZ\n", &FullLayoutPath);
      return FALSE;
   }
   
   return TRUE;
}

static PKBL UserLoadDllAndCreateKbl(DWORD LocaleId)
{
   PKBL NewKbl;
   ULONG hKl;
   LANGID langid;
   
   NewKbl = ExAllocatePool(PagedPool, sizeof(KBL));
   
   if(!NewKbl)
   { 
      DPRINT1("%s: Can't allocate memory!\n", __FUNCTION__);
      return NULL;
   }
   
   swprintf(NewKbl->Name, L"%08lx", LocaleId);
   
   if(!UserLoadKbdDll(NewKbl->Name, &NewKbl->hModule, &NewKbl->KBTables))
   {
      DPRINT1("%s: failed to load %x dll!\n", __FUNCTION__, LocaleId);
      ExFreePool(NewKbl);
      return NULL;
   }
   
   /* Microsoft Office expects this value to be something specific
    * for Japanese and Korean Windows with an IME the value is 0xe001
    * We should probably check to see if an IME exists and if so then
    * set this word properly.
    */
   langid = PRIMARYLANGID(LANGIDFROMLCID(LocaleId));
   hKl = LocaleId;
  
   if (langid == LANG_CHINESE || langid == LANG_JAPANESE || langid == LANG_KOREAN)
      hKl |= 0xe001 << 16; /* FIXME */
   else hKl |= hKl << 16;
   
   NewKbl->hkl = (HKL) hKl;
   NewKbl->klid = LocaleId;
   NewKbl->Flags = 0;
   NewKbl->RefCount = 0;
   
   return NewKbl;
}

BOOL UserInitDefaultKeyboardLayout()
{
   LCID LocaleId;
   NTSTATUS Status;

   Status = ZwQueryDefaultLocale(FALSE, &LocaleId);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Could not get default locale (%08lx).\n", Status);
   }
   else 
   {
      DPRINT("DefaultLocale = %08lx\n", LocaleId);
   }

   if(!NT_SUCCESS(Status) || !(KBLList = UserLoadDllAndCreateKbl(LocaleId)))
   {
      DPRINT1("Trying to load US Keyboard Layout.\n");
      LocaleId = 0x409;

      if(!(KBLList = UserLoadDllAndCreateKbl(LocaleId)))
      {
         DPRINT1("Failed to load any Keyboard Layout\n");
         return FALSE;
      }
   }
   
   InitializeListHead(&KBLList->List);
   return TRUE;
}

PKBL W32kGetDefaultKeyLayout(VOID)
{
   LCID LocaleId;
   NTSTATUS Status;
   PKBL pKbl;

   // This is probably wrong...
   // I need to do more research.
   Status = ZwQueryDefaultLocale(FALSE, &LocaleId);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Could not get default locale (%08lx).\n", Status);
      DPRINT1("Assuming default locale = 0x409 (US).\n");
      LocaleId = 0x409;
   }
   
   pKbl = KBLList;
   do
   {
      if(pKbl->klid == LocaleId)
      {
         return pKbl;
      }
      
      pKbl = (PKBL) pKbl->List.Flink;
   } while(pKbl != KBLList);
   
   DPRINT("Loading new default keyboard layout.\n");
   pKbl = UserLoadDllAndCreateKbl(LocaleId);
   
   if(!pKbl)
   {
      DPRINT1("Failed to load %x!!! Returning any availableKL.\n", LocaleId);
      return KBLList;
   }
   
   InsertTailList(&KBLList->List, &pKbl->List);
   return pKbl;
}

static PKBL UserHklToKbl(HKL hKl)
{
   PKBL pKbl = KBLList;
   do
   {
      if(pKbl->hkl == hKl) return pKbl;
      pKbl = (PKBL) pKbl->List.Flink;
   } while(pKbl != KBLList);
   
   return NULL;
}

static PKBL UserActivateKbl(PW32THREAD Thread, PKBL pKbl)
{
   PKBL Prev;
   
   Prev = Thread->KeyboardLayout;
   Prev->RefCount--;
   Thread->KeyboardLayout = pKbl;
   pKbl->RefCount++;
   
   return Prev;
}

/* EXPORTS *******************************************************************/

HKL FASTCALL
UserGetKeyboardLayout(
   DWORD dwThreadId)
{
   NTSTATUS Status;
   PETHREAD Thread;
   PW32THREAD W32Thread;
   HKL Ret;

   if(!dwThreadId)
   {
      W32Thread = PsGetCurrentThreadWin32Thread();
      return W32Thread->KeyboardLayout->hkl;
   }

   Status = PsLookupThreadByThreadId((HANDLE)dwThreadId, &Thread);
   if(!NT_SUCCESS(Status))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return NULL;
   }
      
   W32Thread = PsGetThreadWin32Thread(Thread); 
   Ret = W32Thread->KeyboardLayout->hkl;
   ObDereferenceObject(Thread);
   return Ret;
}

UINT
STDCALL
NtUserGetKeyboardLayoutList(
   INT nItems,
   HKL* pHklBuff)
{
   UINT Ret = 0;
   PKBL pKbl;
   
   UserEnterShared();
   pKbl = KBLList;
   
   if(nItems == 0)
   {
      do
      {
         Ret++;
         pKbl = (PKBL) pKbl->List.Flink;
      } while(pKbl != KBLList);
   }
   else 
   {
      _SEH_TRY
      {
         ProbeForWrite(pHklBuff, nItems*sizeof(HKL), 4);   
         
         while(Ret < nItems)
         {
            pHklBuff[Ret] = pKbl->hkl;
            Ret++;
            pKbl = (PKBL) pKbl->List.Flink;
            if(pKbl == KBLList) break;
         }
      
      }
      _SEH_HANDLE
      {
         SetLastNtError(_SEH_GetExceptionCode());
         Ret = 0;
      }
      _SEH_END;
   }
   
   UserLeave();
   return Ret;
}

BOOL
STDCALL
NtUserGetKeyboardLayoutName(
   LPWSTR lpszName)
{
   BOOL ret = FALSE;
   PKBL pKbl;

   UserEnterShared();

   _SEH_TRY
   {
      ProbeForWrite(lpszName, 9*sizeof(WCHAR), 1);   
      pKbl = PsGetCurrentThreadWin32Thread()->KeyboardLayout;
      RtlCopyMemory(lpszName,  pKbl->Name, 9*sizeof(WCHAR));
      ret = TRUE;
   }
   _SEH_HANDLE
   {
      SetLastNtError(_SEH_GetExceptionCode());
      ret = FALSE;
   }
   _SEH_END;
   
   UserLeave();
   return ret;
}


HKL
STDCALL
NtUserLoadKeyboardLayoutEx( 
   IN DWORD dwKLID,
   IN UINT Flags,
   IN DWORD Unused1,
   IN DWORD Unused2,
   IN DWORD Unused3,
   IN DWORD Unused4)
{
   HKL Ret = NULL;
   PKBL pKbl;
   
   UserEnterExclusive();
   
   pKbl = KBLList;
   do
   {
      if(pKbl->klid == dwKLID)
      {
         Ret = pKbl->hkl;
         goto the_end;
      }
      
      pKbl = (PKBL) pKbl->List.Flink;
   } while(pKbl != KBLList);
   
   pKbl = UserLoadDllAndCreateKbl(dwKLID);
   
   if(!pKbl)
   {
      goto the_end;
   }
   
   InsertTailList(&KBLList->List, &pKbl->List);
   Ret = pKbl->hkl;
   
   //FIXME: Respect Flags!
   
the_end:
   UserLeave();
   return Ret;
}

HKL
STDCALL
NtUserActivateKeyboardLayout(
   HKL hKl,
   ULONG Flags)
{
   PKBL pKbl;
   HKL Ret = NULL;
   
   UserEnterExclusive();
   
   pKbl = UserHklToKbl(hKl);
   
   if(pKbl)
   {
      pKbl = UserActivateKbl(PsGetCurrentThreadWin32Thread(), pKbl);
      Ret = pKbl->hkl;
   
      //FIXME: Respect flags!
   }
   
   UserLeave();
   return Ret;
}


/* EOF */
