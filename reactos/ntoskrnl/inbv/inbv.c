/* $Id$
 *
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/inbv/inbv.c
 * PURPOSE:        Boot video support
 *
 * PROGRAMMERS:    Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, InbvDisplayInitialize)
#endif

/* GLOBALS *******************************************************************/

/* DATA **********************************************************************/

static BOOLEAN BootVidDriverInstalled = FALSE;

static BOOLEAN (NTAPI *VidInitialize)(BOOLEAN);
static VOID (NTAPI *VidCleanUp)(VOID);
static VOID (NTAPI *VidResetDisplay)(VOID);
static VOID (NTAPI *VidBufferToScreenBlt)(PUCHAR, ULONG, ULONG, ULONG, ULONG, ULONG);
static VOID (NTAPI *VidScreenToBufferBlt)(PUCHAR, ULONG, ULONG, ULONG, ULONG, ULONG);
static VOID (NTAPI *VidBitBlt)(PUCHAR, ULONG, ULONG);
static VOID (NTAPI *VidSolidColorFill)(ULONG, ULONG, ULONG, ULONG, ULONG);
static VOID (NTAPI *VidDisplayString)(PCSTR);
static NTSTATUS (NTAPI *BootVidDisplayBootLogo)(PVOID);
static VOID (NTAPI *BootVidUpdateProgress)(ULONG Progress);
static VOID (NTAPI *BootVidFinalizeBootLogo)(VOID);

static KSPIN_LOCK InbvLock;
static KIRQL InbvOldIrql;
static ULONG InbvDisplayState = 0;
static PHAL_RESET_DISPLAY_PARAMETERS InbvResetDisplayParameters = NULL;
static PVOID BootVidBase;

/* FUNCTIONS *****************************************************************/

VOID NTAPI INIT_FUNCTION
InbvDisplayInitialize(VOID)
{
   struct {
      ANSI_STRING Name;
      PVOID *Ptr;
   } Exports[] = {
      { RTL_CONSTANT_STRING("VidInitialize"), (PVOID*)&VidInitialize },
      { RTL_CONSTANT_STRING("VidCleanUp"), (PVOID*)&VidCleanUp },
      { RTL_CONSTANT_STRING("VidResetDisplay"), (PVOID*)&VidResetDisplay },      
      { RTL_CONSTANT_STRING("VidBufferToScreenBlt"), (PVOID*)&VidBufferToScreenBlt },
      { RTL_CONSTANT_STRING("VidScreenToBufferBlt"), (PVOID*)&VidScreenToBufferBlt },
      { RTL_CONSTANT_STRING("VidBitBlt"), (PVOID*)&VidBitBlt },
      { RTL_CONSTANT_STRING("VidSolidColorFill"), (PVOID*)&VidSolidColorFill },
      { RTL_CONSTANT_STRING("VidDisplayString"), (PVOID*)&VidDisplayString },      
      { RTL_CONSTANT_STRING("BootVidDisplayBootLogo"), (PVOID*)&BootVidDisplayBootLogo },
      { RTL_CONSTANT_STRING("BootVidUpdateProgress"), (PVOID*)&BootVidUpdateProgress },
      { RTL_CONSTANT_STRING("BootVidFinalizeBootLogo"), (PVOID*)&BootVidFinalizeBootLogo }
   };
   UNICODE_STRING BootVidPath = RTL_CONSTANT_STRING(L"bootvid.sys");
   PLDR_DATA_TABLE_ENTRY ModuleObject = NULL, LdrEntry;
   ULONG Index;
   NTSTATUS Status;

   /* FIXME: Hack, try to search for boot driver. */
#if 0
   ModuleObject = LdrGetModuleObject(&BootVidPath);
#else
   {
      NTSTATUS LdrProcessModule(PVOID, PUNICODE_STRING, PLDR_DATA_TABLE_ENTRY*);
      PLIST_ENTRY ListHead, NextEntry;

      ListHead = &KeLoaderBlock->LoadOrderListHead;
      NextEntry = ListHead->Flink;
      while (ListHead != NextEntry)
      {
          /* Get the entry */
          LdrEntry = CONTAINING_RECORD(NextEntry,
                                       LDR_DATA_TABLE_ENTRY,
                                       InLoadOrderLinks);

          /* Compare names */
          if (RtlEqualUnicodeString(&LdrEntry->BaseDllName, &BootVidPath, TRUE))
          {
              /* Tell, that the module is already loaded */
              LdrEntry->Flags |= LDRP_ENTRY_INSERTED;
              Status = LdrProcessModule(LdrEntry->DllBase,
                                        &BootVidPath,
                                        &ModuleObject);
              if (!NT_SUCCESS(Status))
              {
                  DPRINT1("%x\n", Status);
                  return;
              }
            break;
         }

          /* Go to the next driver */
          NextEntry= NextEntry->Flink;
      }
   }
#endif

   if (ModuleObject != NULL)
   {
      for (Index = 0; Index < sizeof(Exports) / sizeof(Exports[0]); Index++)
      {
         Status = LdrGetProcedureAddress(ModuleObject->DllBase,
                                         &Exports[Index].Name, 0,
                                         Exports[Index].Ptr);
         if (!NT_SUCCESS(Status))
            return;
      }

      DPRINT("Done!\n");
      KeInitializeSpinLock(&InbvLock);
      BootVidBase = ModuleObject->DllBase;
      BootVidDriverInstalled = TRUE;
   }
}

static VOID NTAPI
InbvAcquireLock(VOID)
{
   if ((InbvOldIrql = KeGetCurrentIrql()) < DISPATCH_LEVEL)
      InbvOldIrql = KfRaiseIrql(DISPATCH_LEVEL);
   KiAcquireSpinLock(&InbvLock);
}

static VOID NTAPI
InbvReleaseLock(VOID)
{
   KiReleaseSpinLock(&InbvLock);
   if (InbvOldIrql < DISPATCH_LEVEL)
      KfLowerIrql(InbvOldIrql);
}

VOID STDCALL 
InbvEnableBootDriver(IN BOOLEAN Enable)
{
   if (BootVidDriverInstalled)
   {
      if (InbvDisplayState >= 2)
         return;
      InbvAcquireLock();
      if (InbvDisplayState == 0)
         VidCleanUp();
      InbvDisplayState = !Enable;
      InbvReleaseLock();
   }
   else
   {
      InbvDisplayState = !Enable;
   }
}

VOID NTAPI
InbvAcquireDisplayOwnership(VOID)
{
   if (InbvResetDisplayParameters && InbvDisplayState == 2)
   { 
      if (InbvResetDisplayParameters != NULL)
         InbvResetDisplayParameters(80, 50);
   }
   InbvDisplayState = 0;
}

BOOLEAN STDCALL
InbvCheckDisplayOwnership(VOID)
{
   return InbvDisplayState != 2;
}

BOOLEAN STDCALL
InbvDisplayString(IN PCHAR String)
{
   if (BootVidDriverInstalled && InbvDisplayState == 0)
   {
      InbvAcquireLock();
      VidDisplayString(String);
      InbvReleaseLock();

      /* Call Headless (We don't support headless for now) 
      HeadlessDispatch(DISPLAY_STRING);
      */

      return TRUE;
   }

   return FALSE;
}


BOOLEAN STDCALL
InbvEnableDisplayString(IN BOOLEAN Enable)
{
   return FALSE;
}


VOID STDCALL
InbvInstallDisplayStringFilter(IN PVOID Unknown)
{
}


BOOLEAN STDCALL
InbvIsBootDriverInstalled(VOID)
{
   return BootVidDriverInstalled;
}


VOID STDCALL
InbvNotifyDisplayOwnershipLost(
   IN PVOID Callback)
{
   if (BootVidDriverInstalled)
   {
      InbvAcquireLock();
      if (InbvDisplayState != 2)
         VidCleanUp();
      else if (InbvResetDisplayParameters != NULL)
         InbvResetDisplayParameters(80, 50);
      InbvResetDisplayParameters = Callback;
      InbvDisplayState = 2;
      InbvReleaseLock();
   }
   else
   {
      InbvResetDisplayParameters = Callback;
      InbvDisplayState = 2;
   }
}


BOOLEAN STDCALL
InbvResetDisplay(VOID)
{
   if (BootVidDriverInstalled && InbvDisplayState == 0)
   {
      VidResetDisplay();
      return TRUE;
   }
   return FALSE;
}


VOID STDCALL
InbvSetScrollRegion(
   IN ULONG Left,
   IN ULONG Top,
   IN ULONG Width,
   IN ULONG Height)
{
}


VOID STDCALL
InbvSetTextColor(
   IN ULONG Color)
{
}


VOID STDCALL
InbvSolidColorFill(
   IN ULONG Left,
   IN ULONG Top,
   IN ULONG Width,
   IN ULONG Height,
   IN ULONG Color)
{
   if (BootVidDriverInstalled && InbvDisplayState == 0)
   {
      VidSolidColorFill(Left, Top, Width, Height, Color);
   }
}


BOOLEAN NTAPI
BootVidResetDisplayParameters(ULONG SizeX, ULONG SizeY)
{
   BootVidFinalizeBootLogo();
   return TRUE;
}


VOID NTAPI
InbvDisplayInitialize2(BOOLEAN NoGuiBoot)
{
   VidInitialize(!NoGuiBoot);
}


VOID NTAPI
InbvDisplayBootLogo(IN BOOLEAN SosEnabled)
{
   InbvEnableBootDriver(TRUE);

   if (BootVidDriverInstalled)
   {
      InbvResetDisplayParameters = BootVidResetDisplayParameters;
      if (!SosEnabled) BootVidDisplayBootLogo(BootVidBase);
   }
}


VOID NTAPI
InbvUpdateProgressBar(
   IN ULONG Progress)
{
   if (BootVidDriverInstalled)
   {
      BootVidUpdateProgress(Progress);
   }
}


VOID NTAPI
InbvFinalizeBootLogo(VOID)
{
   if (BootVidDriverInstalled)
   {
      /* Notify the hal we have released the display. */
      /* InbvReleaseDisplayOwnership(); */
      BootVidFinalizeBootLogo();
      InbvEnableBootDriver(FALSE);
   }
}


NTSTATUS STDCALL
NtDisplayString(
   IN PUNICODE_STRING DisplayString)
{
   OEM_STRING OemString;

   RtlUnicodeStringToOemString(&OemString, DisplayString, TRUE);
   InbvDisplayString(OemString.Buffer);
   RtlFreeOemString(&OemString);

   return STATUS_SUCCESS;
}
