/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            include/internal/ldr.h
 * PURPOSE:         Header for loader module
 */

#ifndef __INCLUDE_INTERNAL_LDR_H
#define __INCLUDE_INTERNAL_LDR_H

#include <pe.h>

NTSTATUS
LdrLoadDriver (
	IN	PUNICODE_STRING	Filename
	);
NTSTATUS
LdrLoadInitialProcess (
	VOID
	);
VOID
LdrLoadAutoConfigDrivers (
	VOID
	);
VOID
LdrInitModuleManagement (
	VOID
	);
NTSTATUS
LdrProcessDriver (
	IN	PVOID	ModuleLoadBase
	);
NTSTATUS
LdrpMapSystemDll (
	HANDLE	ProcessHandle,
	PVOID	* LdrStartupAddress
	);
PVOID
LdrpGetSystemDllEntryPoint (
	VOID
	);
NTSTATUS
LdrpMapImage (
	HANDLE	ProcessHandle, 
	HANDLE	SectionHandle,
	PVOID	* ImageBase
	);


NTSTATUS STDCALL
LdrGetProcedureAddress (IN PVOID BaseAddress,
                        IN PANSI_STRING Name,
                        IN ULONG Ordinal,
                        OUT PVOID *ProcedureAddress);

NTSTATUS LdrLoadGdiDriver (PUNICODE_STRING DriverName,
			   PVOID *ImageAddress,
			   PVOID *SectionPointer,
			   PVOID *EntryPoint,
			   PVOID *ExportSectionPointer);

PVOID STDCALL
RtlImageDirectoryEntryToData (
	IN PVOID	BaseAddress,
	IN BOOLEAN	ImageLoaded,
	IN ULONG	Directory,
	OUT PULONG	Size);


#endif /* __INCLUDE_INTERNAL_LDR_H */
