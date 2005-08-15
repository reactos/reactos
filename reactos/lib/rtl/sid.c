/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security manager
 * FILE:              lib/rtl/sid.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 * REVISION HISTORY:
 *                 26/07/98: Added stubs for security functions
 */

/* INCLUDES *****************************************************************/
#define __NTDRIVER__
#include <rtl.h>

#define NDEBUG
#include <debug.h>

#define TAG_SID TAG('p', 'S', 'i', 'd')

/* FUNCTIONS ***************************************************************/

BOOLEAN STDCALL
RtlValidSid(IN PSID Sid_)
{
  PISID Sid =  Sid_;

  PAGED_CODE_RTL();

  if ((Sid->Revision != SID_REVISION) ||
      (Sid->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES))
    {
      return FALSE;
    }

   return TRUE;
}


/*
 * @implemented
 */
ULONG STDCALL
RtlLengthRequiredSid(IN UCHAR SubAuthorityCount)
{
  PAGED_CODE_RTL();

  return (sizeof(SID) + (SubAuthorityCount - 1) * sizeof(ULONG));
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlInitializeSid(IN PSID Sid_,
                 IN PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
                 IN UCHAR SubAuthorityCount)
{
  PISID Sid =  Sid_;

  PAGED_CODE_RTL();

  Sid->Revision = SID_REVISION;
  Sid->SubAuthorityCount = SubAuthorityCount;
  memcpy(&Sid->IdentifierAuthority,
         IdentifierAuthority,
         sizeof(SID_IDENTIFIER_AUTHORITY));

  return STATUS_SUCCESS;
}


/*
 * @implemented
 */
PULONG STDCALL
RtlSubAuthoritySid(IN PSID Sid_,
                   IN ULONG SubAuthority)
{
  PISID Sid =  Sid_;

  PAGED_CODE_RTL();

  return &Sid->SubAuthority[SubAuthority];
}


/*
 * @implemented
 */
PUCHAR STDCALL
RtlSubAuthorityCountSid(IN PSID Sid_)
{
  PISID Sid =  Sid_;

  PAGED_CODE_RTL();

  return &Sid->SubAuthorityCount;
}


/*
 * @implemented
 */
BOOLEAN STDCALL
RtlEqualSid(IN PSID Sid1_,
            IN PSID Sid2_)
{
  PISID Sid1 =  Sid1_;
  PISID Sid2 =  Sid2_;

  PAGED_CODE_RTL();

  if (Sid1->Revision != Sid2->Revision)
   {
      return(FALSE);
   }
   if ((*RtlSubAuthorityCountSid(Sid1)) != (*RtlSubAuthorityCountSid(Sid2)))
   {
      return(FALSE);
   }
   if (RtlCompareMemory(Sid1, Sid2, RtlLengthSid(Sid1)) != RtlLengthSid(Sid1))
   {
      return(FALSE);
   }
   return(TRUE);
}


/*
 * @implemented
 */
ULONG STDCALL
RtlLengthSid(IN PSID Sid_)
{
  PISID Sid =  Sid_;

  PAGED_CODE_RTL();

  return (sizeof(SID) + (Sid->SubAuthorityCount-1) * sizeof(ULONG));
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlCopySid(ULONG BufferLength,
           PSID Dest,
           PSID Src)
{
  PAGED_CODE_RTL();

  if (BufferLength < RtlLengthSid(Src))
    {
      return STATUS_UNSUCCESSFUL;
    }

  memmove(Dest,
          Src,
          RtlLengthSid(Src));

  return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlCopySidAndAttributesArray(ULONG Count,
                             PSID_AND_ATTRIBUTES Src,
                             ULONG SidAreaSize,
                             PSID_AND_ATTRIBUTES Dest,
                             PVOID SidArea,
                             PVOID* RemainingSidArea,
                             PULONG RemainingSidAreaSize)
{
   ULONG SidLength;
   ULONG Length;
   ULONG i;

   PAGED_CODE_RTL();

   Length = SidAreaSize;

   for (i=0; i<Count; i++)
   {
      if (RtlLengthSid(Src[i].Sid) > Length)
      {
         return(STATUS_BUFFER_TOO_SMALL);
      }
      SidLength = RtlLengthSid(Src[i].Sid);
      Length = Length - SidLength;
      Dest[i].Sid = SidArea;
      Dest[i].Attributes = Src[i].Attributes;
      RtlCopySid(SidLength,
                 SidArea,
                 Src[i].Sid);
      SidArea = (PVOID)((ULONG_PTR)SidArea + SidLength);
   }
   *RemainingSidArea = SidArea;
   *RemainingSidAreaSize = Length;
   return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
PSID_IDENTIFIER_AUTHORITY STDCALL
RtlIdentifierAuthoritySid(IN PSID Sid_)
{
  PISID Sid =  Sid_;

  PAGED_CODE_RTL();

  return &Sid->IdentifierAuthority;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlAllocateAndInitializeSid(PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
			    UCHAR SubAuthorityCount,
			    ULONG SubAuthority0,
			    ULONG SubAuthority1,
			    ULONG SubAuthority2,
			    ULONG SubAuthority3,
			    ULONG SubAuthority4,
			    ULONG SubAuthority5,
			    ULONG SubAuthority6,
			    ULONG SubAuthority7,
			    PSID *Sid)
{
  PISID pSid;

  PAGED_CODE_RTL();

  if (SubAuthorityCount > 8)
    return STATUS_INVALID_SID;

  if (Sid == NULL)
    return STATUS_INVALID_PARAMETER;

  pSid = RtlpAllocateMemory(sizeof(SID) + (SubAuthorityCount - 1) * sizeof(ULONG),
                            TAG_SID);
  if (pSid == NULL)
    return STATUS_NO_MEMORY;

  pSid->Revision = SID_REVISION;
  pSid->SubAuthorityCount = SubAuthorityCount;
  memcpy(&pSid->IdentifierAuthority,
         IdentifierAuthority,
         sizeof(SID_IDENTIFIER_AUTHORITY));

  switch (SubAuthorityCount)
    {
      case 8:
         pSid->SubAuthority[7] = SubAuthority7;
      case 7:
         pSid->SubAuthority[6] = SubAuthority6;
      case 6:
         pSid->SubAuthority[5] = SubAuthority5;
      case 5:
         pSid->SubAuthority[4] = SubAuthority4;
      case 4:
         pSid->SubAuthority[3] = SubAuthority3;
      case 3:
         pSid->SubAuthority[2] = SubAuthority2;
      case 2:
         pSid->SubAuthority[1] = SubAuthority1;
      case 1:
         pSid->SubAuthority[0] = SubAuthority0;
         break;
    }

  *Sid = pSid;

  return STATUS_SUCCESS;
}


/*
 * @implemented
 *
 * RETURNS
 *  Docs says FreeSid does NOT return a value
 *  even thou it's defined to return a PVOID...
 */
PVOID STDCALL
RtlFreeSid(IN PSID Sid)
{
   PAGED_CODE_RTL();

   RtlpFreeMemory(Sid, TAG_SID);
   return NULL;
}


/*
 * @implemented
 */
BOOLEAN STDCALL
RtlEqualPrefixSid(IN PSID Sid1_,
                  IN PSID Sid2_)
{
  PISID Sid1 =  Sid1_;
  PISID Sid2 =  Sid2_;

  PAGED_CODE_RTL();

   return(Sid1->SubAuthorityCount == Sid2->SubAuthorityCount &&
          !RtlCompareMemory(Sid1, Sid2,
                            (Sid1->SubAuthorityCount - 1) * sizeof(DWORD) + 8));
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlConvertSidToUnicodeString(PUNICODE_STRING String,
                             PSID Sid_,
                             BOOLEAN AllocateBuffer)
{
   WCHAR Buffer[256];
   PWSTR wcs;
   ULONG Length;
   ULONG i;
   PISID Sid =  Sid_;

   PAGED_CODE_RTL();

   if (RtlValidSid (Sid) == FALSE)
      return STATUS_INVALID_SID;

   wcs = Buffer;
   wcs += swprintf (wcs, L"S-%u-", Sid->Revision);
   if (Sid->IdentifierAuthority.Value[0] == 0 &&
         Sid->IdentifierAuthority.Value[1] == 0)
   {
      wcs += swprintf (wcs,
                       L"%lu",
                       (ULONG)Sid->IdentifierAuthority.Value[2] << 24 |
                       (ULONG)Sid->IdentifierAuthority.Value[3] << 16 |
                       (ULONG)Sid->IdentifierAuthority.Value[4] << 8 |
                       (ULONG)Sid->IdentifierAuthority.Value[5]);
   }
   else
   {
      wcs += swprintf (wcs,
                       L"0x%02hx%02hx%02hx%02hx%02hx%02hx",
                       Sid->IdentifierAuthority.Value[0],
                       Sid->IdentifierAuthority.Value[1],
                       Sid->IdentifierAuthority.Value[2],
                       Sid->IdentifierAuthority.Value[3],
                       Sid->IdentifierAuthority.Value[4],
                       Sid->IdentifierAuthority.Value[5]);
   }

   for (i = 0; i < Sid->SubAuthorityCount; i++)
   {
      wcs += swprintf (wcs,
                       L"-%u",
                       Sid->SubAuthority[i]);
   }

   Length = (wcs - Buffer) * sizeof(WCHAR);
   if (AllocateBuffer)
   {
      String->Buffer = RtlpAllocateMemory(Length + sizeof(WCHAR),
                                          TAG_SID);
      if (String->Buffer == NULL)
         return STATUS_NO_MEMORY;
      String->MaximumLength = Length + sizeof(WCHAR);
   }
   else
   {
      if (Length > String->MaximumLength)
         return STATUS_BUFFER_TOO_SMALL;
   }

   String->Length = Length;
   RtlCopyMemory (String->Buffer,
                  Buffer,
                  Length);
   if (Length < String->MaximumLength)
      String->Buffer[Length / sizeof(WCHAR)] = 0;

   return STATUS_SUCCESS;
}

/* EOF */
