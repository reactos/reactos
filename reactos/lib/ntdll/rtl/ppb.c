/* $Id: ppb.c,v 1.15 2002/10/01 19:27:20 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/ppb.c
 * PURPOSE:         Process parameters functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/ldr.h>
#include <napi/teb.h>
#include <ntdll/base.h>
#include <ntdll/rtl.h>

#define NDEBUG
#include <ntdll/ntdll.h>

/* MACROS ****************************************************************/

#define NORMALIZE(x,addr)   {if(x) x=(VOID*)((ULONG)(x)+(ULONG)(addr));}
#define DENORMALIZE(x,addr) {if(x) x=(VOID*)((ULONG)(x)-(ULONG)(addr));}
#define ALIGN(x,align)      (((ULONG)(x)+(align)-1UL)&(~((align)-1UL)))


/* FUNCTIONS ****************************************************************/

VOID STDCALL
RtlAcquirePebLock(VOID)
{
   PPEB Peb = NtCurrentPeb ();
   Peb->FastPebLockRoutine (Peb->FastPebLock);
}


VOID STDCALL
RtlReleasePebLock(VOID)
{
   PPEB Peb = NtCurrentPeb ();
   Peb->FastPebUnlockRoutine (Peb->FastPebLock);
}

static inline VOID
RtlpCopyParameterString(PWCHAR *Ptr,
			PUNICODE_STRING Destination,
			PUNICODE_STRING Source,
			ULONG Size)
{
   Destination->Length = Source->Length;
   Destination->MaximumLength = Size ? Size : Source->MaximumLength;
   Destination->Buffer = (PWCHAR)(*Ptr);
   if (Source->Length)
     memmove (Destination->Buffer, Source->Buffer, Source->Length);
   Destination->Buffer[Destination->Length / sizeof(WCHAR)] = 0;
   *Ptr += Destination->MaximumLength/sizeof(WCHAR);
}


NTSTATUS STDCALL
RtlCreateProcessParameters(PRTL_USER_PROCESS_PARAMETERS *ProcessParameters,
			   PUNICODE_STRING ImagePathName,
			   PUNICODE_STRING DllPath,
			   PUNICODE_STRING CurrentDirectory,
			   PUNICODE_STRING CommandLine,
			   PWSTR Environment,
			   PUNICODE_STRING WindowTitle,
			   PUNICODE_STRING DesktopInfo,
			   PUNICODE_STRING ShellInfo,
			   PUNICODE_STRING RuntimeInfo)
{
   NTSTATUS Status = STATUS_SUCCESS;
   PRTL_USER_PROCESS_PARAMETERS Param = NULL;
   ULONG RegionSize = 0;
   ULONG Length = 0;
   PWCHAR Dest;
   UNICODE_STRING EmptyString;
   HANDLE CurrentDirectoryHandle;
   HANDLE ConsoleHandle;
   ULONG ConsoleFlags;

   DPRINT ("RtlCreateProcessParameters\n");

   RtlAcquirePebLock();

   EmptyString.Length = 0;
   EmptyString.MaximumLength = sizeof(WCHAR);
   EmptyString.Buffer = L"";

   if (NtCurrentPeb()->ProcessParameters)
     {
	if (DllPath == NULL)
	  DllPath = &NtCurrentPeb()->ProcessParameters->DllPath;
	if (Environment == NULL)
	  Environment  = NtCurrentPeb()->ProcessParameters->Environment;
	if (CurrentDirectory == NULL)
	  CurrentDirectory = &NtCurrentPeb()->ProcessParameters->CurrentDirectory.DosPath;
	CurrentDirectoryHandle = NtCurrentPeb()->ProcessParameters->CurrentDirectory.Handle;
	ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
	ConsoleFlags = NtCurrentPeb()->ProcessParameters->ConsoleFlags;
     }
   else
     {
	if (DllPath == NULL)
	  DllPath = &EmptyString;
	if (CurrentDirectory == NULL)
	  CurrentDirectory = &EmptyString;
	CurrentDirectoryHandle = NULL;
	ConsoleHandle = NULL;
	ConsoleFlags = 0;
     }

   if (CommandLine == NULL)
     CommandLine = &EmptyString;
   if (WindowTitle == NULL)
     WindowTitle = &EmptyString;
   if (DesktopInfo == NULL)
     DesktopInfo = &EmptyString;
   if (ShellInfo == NULL)
     ShellInfo = &EmptyString;
   if (RuntimeInfo == NULL)
     RuntimeInfo = &EmptyString;

   /* size of process parameter block */
   Length = sizeof(RTL_USER_PROCESS_PARAMETERS);

   /* size of current directory buffer */
   Length += (MAX_PATH * sizeof(WCHAR));

   /* add string lengths */
   Length += ALIGN(DllPath->MaximumLength, sizeof(ULONG));
   Length += ALIGN(ImagePathName->Length + sizeof(WCHAR), sizeof(ULONG));
   Length += ALIGN(CommandLine->Length + sizeof(WCHAR), sizeof(ULONG));
   Length += ALIGN(WindowTitle->MaximumLength, sizeof(ULONG));
   Length += ALIGN(DesktopInfo->MaximumLength, sizeof(ULONG));
   Length += ALIGN(ShellInfo->MaximumLength, sizeof(ULONG));
   Length += ALIGN(RuntimeInfo->MaximumLength, sizeof(ULONG));

   /* Calculate the required block size */
   RegionSize = ROUNDUP(Length, PAGE_SIZE);

   Status = NtAllocateVirtualMemory(NtCurrentProcess(),
				    (PVOID*)&Param,
				    0,
				    &RegionSize,
				    MEM_RESERVE | MEM_COMMIT,
				    PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
	RtlReleasePebLock();
	return Status;
     }

   DPRINT ("Process parameters allocated\n");

   Param->MaximumLength = RegionSize;
   Param->Length = Length;
   Param->Flags = PPF_NORMALIZED;
   Param->Environment = Environment;
   Param->CurrentDirectory.Handle = CurrentDirectoryHandle;
   Param->ConsoleHandle = ConsoleHandle;
   Param->ConsoleFlags = ConsoleFlags;

   Dest = (PWCHAR)(((PBYTE)Param) + sizeof(RTL_USER_PROCESS_PARAMETERS));

   /* copy current directory */
   RtlpCopyParameterString(&Dest,
			   &Param->CurrentDirectory.DosPath,
			   CurrentDirectory,
			   MAX_PATH * sizeof(WCHAR));

   /* make sure the current directory has a trailing backslash */
   if (Param->CurrentDirectory.DosPath.Length > 0)
     {
	ULONG Length;

	Length = Param->CurrentDirectory.DosPath.Length / sizeof(WCHAR);
	if (Param->CurrentDirectory.DosPath.Buffer[Length-1] != L'\\')
	  {
	     Param->CurrentDirectory.DosPath.Buffer[Length] = L'\\';
	     Param->CurrentDirectory.DosPath.Buffer[Length + 1] = 0;
	     Param->CurrentDirectory.DosPath.Length += sizeof(WCHAR);
	  }
     }

   /* copy dll path */
   RtlpCopyParameterString(&Dest,
			   &Param->DllPath,
			   DllPath,
			   0);

   /* copy image path name */
   RtlpCopyParameterString(&Dest,
			   &Param->ImagePathName,
			   ImagePathName,
			   ImagePathName->Length + sizeof(WCHAR));

   /* copy command line */
   RtlpCopyParameterString(&Dest,
			   &Param->CommandLine,
			   CommandLine,
			   CommandLine->Length + sizeof(WCHAR));

   /* copy title */
   RtlpCopyParameterString(&Dest,
			   &Param->WindowTitle,
			   WindowTitle,
			   0);

   /* copy desktop */
   RtlpCopyParameterString(&Dest,
			   &Param->DesktopInfo,
			   DesktopInfo,
			   0);

   /* copy shell info */
   RtlpCopyParameterString(&Dest,
			   &Param->ShellInfo,
			   ShellInfo,
			   0);

   /* copy runtime info */
   RtlpCopyParameterString(&Dest,
			   &Param->RuntimeInfo,
			   RuntimeInfo,
			   0);

   RtlDeNormalizeProcessParams(Param);
   *ProcessParameters = Param;
   RtlReleasePebLock();

   return STATUS_SUCCESS;
}

VOID STDCALL
RtlDestroyProcessParameters(PRTL_USER_PROCESS_PARAMETERS ProcessParameters)
{
   ULONG RegionSize = 0;

   NtFreeVirtualMemory (NtCurrentProcess (),
			(PVOID)ProcessParameters,
			&RegionSize,
			MEM_RELEASE);
}

/*
 * denormalize process parameters (Pointer-->Offset)
 */
PRTL_USER_PROCESS_PARAMETERS STDCALL
RtlDeNormalizeProcessParams(PRTL_USER_PROCESS_PARAMETERS Params)
{
   if (Params && (Params->Flags & PPF_NORMALIZED))
     {
	DENORMALIZE(Params->CurrentDirectory.DosPath.Buffer, Params);
	DENORMALIZE(Params->DllPath.Buffer, Params);
	DENORMALIZE(Params->ImagePathName.Buffer, Params);
	DENORMALIZE(Params->CommandLine.Buffer, Params);
	DENORMALIZE(Params->WindowTitle.Buffer, Params);
	DENORMALIZE(Params->DesktopInfo.Buffer, Params);
	DENORMALIZE(Params->ShellInfo.Buffer, Params);
	DENORMALIZE(Params->RuntimeInfo.Buffer, Params);

	Params->Flags &= ~PPF_NORMALIZED;
     }

   return Params;
}

/*
 * normalize process parameters (Offset-->Pointer)
 */
PRTL_USER_PROCESS_PARAMETERS STDCALL
RtlNormalizeProcessParams(PRTL_USER_PROCESS_PARAMETERS Params)
{
   if (Params && !(Params->Flags & PPF_NORMALIZED))
     {
	NORMALIZE(Params->CurrentDirectory.DosPath.Buffer, Params);
	NORMALIZE(Params->DllPath.Buffer, Params);
	NORMALIZE(Params->ImagePathName.Buffer, Params);
	NORMALIZE(Params->CommandLine.Buffer, Params);
	NORMALIZE(Params->WindowTitle.Buffer, Params);
	NORMALIZE(Params->DesktopInfo.Buffer, Params);
	NORMALIZE(Params->ShellInfo.Buffer, Params);
	NORMALIZE(Params->RuntimeInfo.Buffer, Params);

	Params->Flags |= PPF_NORMALIZED;
     }

   return Params;
}

/* EOF */
