/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/rtl/rtl.h
 * PURPOSE:         Run-Time Libary Header
 * PROGRAMMER:      Alex Ionescu
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <reactos/helper.h>
#include <reactos/rosrtl/thread.h>

/* Required for Lib Support */
PVOID STDCALL ExAllocatePool(IN ULONG PoolType, IN SIZE_T NumberOfBytes);
PVOID STDCALL ExAllocatePoolWithTag(IN ULONG PoolType, IN SIZE_T NumberOfBytes, IN ULONG Tag);
VOID STDCALL ExFreePool(IN PVOID P);
VOID STDCALL ExFreePoolWithTag(IN PVOID P, IN ULONG Tag);

/* Internal Private Functions */
NTSTATUS
FASTCALL
RtlpOemStringToCountedUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN POEM_STRING OemSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
NTSTATUS 
FASTCALL
RtlpUpcaseUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PCUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
NTSTATUS 
FASTCALL
RtlpUpcaseUnicodeStringToAnsiString(
   IN OUT PANSI_STRING AnsiDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
NTSTATUS 
FASTCALL
RtlpUpcaseUnicodeStringToCountedOemString(
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
NTSTATUS 
FASTCALL
RtlpUpcaseUnicodeStringToOemString (
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
NTSTATUS 
FASTCALL
RtlpDowncaseUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
NTSTATUS
FASTCALL
RtlpAnsiStringToUnicodeString(
   IN OUT PUNICODE_STRING DestinationString,
   IN PANSI_STRING SourceString,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType);   
   
NTSTATUS
FASTCALL
RtlpUnicodeStringToAnsiString(
   IN OUT PANSI_STRING AnsiDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
NTSTATUS
FASTCALL
RtlpOemStringToUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN POEM_STRING OemSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType);

NTSTATUS
FASTCALL
RtlpUnicodeStringToOemString(
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
BOOLEAN
FASTCALL
RtlpCreateUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PCWSTR  Source,
   IN POOL_TYPE PoolType);   

NTSTATUS
FASTCALL
RtlpUnicodeStringToCountedOemString(
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType);

NTSTATUS STDCALL
RtlpDuplicateUnicodeString(
   INT AddNull,
   IN PUNICODE_STRING SourceString,
   PUNICODE_STRING DestinationString,
   POOL_TYPE PoolType);

/* EOF */
