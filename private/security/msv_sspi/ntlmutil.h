//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        ntlmutil.h
//
// Contents:    prototypes for NtLm utility functions
//
//
// History:     ChandanS 25-Jul-1996   Stolen from kerberos\client2\kerbutil.h
//
//------------------------------------------------------------------------

#ifndef __NTLMUTIL_H__
#define __NTLMUTIL_H__


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Miscellaneous macros                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C"
{
#endif

NTSTATUS
NtLmDuplicateUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN OPTIONAL PUNICODE_STRING SourceString
    );

NTSTATUS
NtLmDuplicateString(
    OUT PSTRING DestinationString,
    IN OPTIONAL PSTRING SourceString
    );

NTSTATUS
NtLmDuplicateSid(
    OUT PSID *DestinationSid,
    IN PSID SourceSid
    );

VOID
NtLmFree(
    IN PVOID Buffer
    );

PVOID
NtLmAllocate(
    IN ULONG BufferSize
    );

#ifdef __cplusplus
}
#endif

#endif // __KERBUTIL_H__
