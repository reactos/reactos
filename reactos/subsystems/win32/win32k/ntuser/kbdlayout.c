
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
      DPRINT("Can't get layout filename for %wZ. (%08lx)\n", klid, Status);
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
      DPRINT("%s: failed to load %x dll!\n", __FUNCTION__, LocaleId);
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
   
   KBLList->Flags |= KBL_PRELOAD;
   
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
      DPRINT("Failed to load %x!!! Returning any availableKL.\n", LocaleId);
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

BOOL UserUnloadKbl(PKBL pKbl)
{
   /* According to msdn, UnloadKeyboardLayout can fail
      if the keyboard layout identifier was preloaded. */
      
   if(pKbl->Flags & KBL_PRELOAD)
   {
      DPRINT1("Attempted to unload preloaded keyboard layout.\n");
      return FALSE;
   }
   
   if(pKbl->RefCount > 0)
   {
      /* Layout is used by other threads.
         Mark it as unloaded and don't do anything else. */
      pKbl->Flags |= KBL_UNLOAD;
   }
   else
   {
      //Unload the layout
      EngUnloadImage(pKbl->hModule);
      RemoveEntryList(&pKbl->List);
      ExFreePool(pKbl);
   }
   
   return TRUE;
}

static PKBL co_UserActivateKbl(PW32THREAD w32Thread, PKBL pKbl, UINT Flags)
{
   PKBL Prev;
   
   Prev = w32Thread->KeyboardLayout;
   Prev->RefCount--;
   w32Thread->KeyboardLayout = pKbl;
   pKbl->RefCount++;
   
   if(Flags & KLF_SETFORPROCESS)
   {
      //FIXME
      
   }
   
   if(Prev->Flags & KBL_UNLOAD && Prev->RefCount == 0)
   {
      UserUnloadKbl(Prev);
   }
   
   // Send WM_INPUTLANGCHANGE to thread's focus window
   co_IntSendMessage(w32Thread->MessageQueue->FocusWindow, 
      WM_INPUTLANGCHANGE, 
      0, // FIXME: put charset here (what is this?)
      (LPARAM)pKbl->hkl); //klid
         
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
            if(!(pKbl->Flags & KBL_UNLOAD))
            {
               pHklBuff[Ret] = pKbl->hkl;
               Ret++;
               pKbl = (PKBL) pKbl->List.Flink;
               if(pKbl == KBLList) break;
            }
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
      ProbeForWrite(lpszName, KL_NAMELENGTH*sizeof(WCHAR), 1);   
      pKbl = PsGetCurrentThreadWin32Thread()->KeyboardLayout;
      RtlCopyMemory(lpszName,  pKbl->Name, KL_NAMELENGTH*sizeof(WCHAR));
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
   IN HANDLE Handle,
   IN DWORD offTable,
   IN HKL hKL,
   IN PUNICODE_STRING puszKLID,
   IN DWORD dwKLID,
   IN UINT Flags)
{
   HKL Ret = NULL;
   PKBL pKbl = NULL, Cur;
   
   UserEnterExclusive();
   
   //Let's see if layout was already loaded.
   Cur = KBLList;
   do
   {
      if(Cur->klid == dwKLID)
      {
         pKbl = Cur;
         pKbl->Flags &= ~KBL_UNLOAD;
         break;
      }
      
      Cur = (PKBL) Cur->List.Flink;
   } while(Cur != KBLList);
   
   //It wasn't, so load it.
   if(!pKbl) 
   {
      pKbl = UserLoadDllAndCreateKbl(dwKLID);
   
      if(!pKbl)
      {
         goto the_end;
      }
      
      InsertTailList(&KBLList->List, &pKbl->List);
   }
   
   if(Flags & KLF_REORDER) KBLList = pKbl;
   
   if(Flags & KLF_ACTIVATE) 
   {
      co_UserActivateKbl(PsGetCurrentThreadWin32Thread(), pKbl, Flags);
   }

   Ret = pKbl->hkl;
   
   //FIXME: KLF_NOTELLSHELL
   //       KLF_REPLACELANG
   //       KLF_SUBSTITUTE_OK 
   
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
   PW32THREAD pWThread;
   
   UserEnterExclusive();
   
   pWThread = PsGetCurrentThreadWin32Thread();
   
   if(pWThread->KeyboardLayout->hkl == hKl)
   {
      Ret = hKl;
      goto the_end;
   }
    
   if(hKl == (HKL)HKL_NEXT)
   {
      pKbl = (PKBL)pWThread->KeyboardLayout->List.Flink;
   }
   else if(hKl == (HKL)HKL_PREV)
   {
      pKbl = (PKBL)pWThread->KeyboardLayout->List.Blink;
   }
   else pKbl = UserHklToKbl(hKl);
   
   //FIXME:  KLF_RESET, KLF_SHIFTLOCK
   
   if(pKbl)
   {
      if(Flags & KLF_REORDER)
         KBLList = pKbl;
      
      if(pKbl == pWThread->KeyboardLayout)
      {
         Ret = pKbl->hkl;
      }
      else
      {
         pKbl = co_UserActivateKbl(pWThread, pKbl, Flags);
         Ret = pKbl->hkl;
      }
   }
   else
   {
      DPRINT1("%s: Invalid HKL %x!\n", __FUNCTION__, hKl);
   }
   
the_end:
   UserLeave();
   return Ret;
}

BOOL
STDCALL
NtUserUnloadKeyboardLayout(
   HKL hKl)
{
   PKBL pKbl;
   BOOL Ret = FALSE;
   
   UserEnterExclusive();
   
   if((pKbl = UserHklToKbl(hKl)))
   {
      Ret = UserUnloadKbl(pKbl);
   }
   else 
   {
      DPRINT1("%s: Invalid HKL %x!\n", __FUNCTION__, hKl);   
   }
   
   UserLeave();
   return Ret;
}

/* EOF */
