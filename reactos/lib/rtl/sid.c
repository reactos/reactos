/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Security manager
 * FILE:              lib/rtl/sid.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

#define TAG_SID TAG('p', 'S', 'i', 'd')

/* FUNCTIONS ***************************************************************/

BOOLEAN NTAPI
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
ULONG NTAPI
RtlLengthRequiredSid(IN ULONG SubAuthorityCount)
{
  PAGED_CODE_RTL();

  return (ULONG)FIELD_OFFSET(SID,
                             SubAuthority[SubAuthorityCount]);
}


/*
 * @implemented
 */
NTSTATUS NTAPI
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
PULONG NTAPI
RtlSubAuthoritySid(IN PSID Sid_,
                   IN ULONG SubAuthority)
{
  PISID Sid =  Sid_;

  PAGED_CODE_RTL();

  return (PULONG)&Sid->SubAuthority[SubAuthority];
}


/*
 * @implemented
 */
PUCHAR NTAPI
RtlSubAuthorityCountSid(IN PSID Sid_)
{
  PISID Sid =  Sid_;

  PAGED_CODE_RTL();

  return &Sid->SubAuthorityCount;
}


/*
 * @implemented
 */
BOOLEAN NTAPI
RtlEqualSid(IN PSID Sid1_,
            IN PSID Sid2_)
{
  PISID Sid1 =  Sid1_;
  PISID Sid2 =  Sid2_;
  SIZE_T SidLen;

  PAGED_CODE_RTL();

  if (Sid1->Revision != Sid2->Revision ||
      (*RtlSubAuthorityCountSid(Sid1)) != (*RtlSubAuthorityCountSid(Sid2)))
   {
      return(FALSE);
   }

   SidLen = RtlLengthSid(Sid1);
   return RtlCompareMemory(Sid1, Sid2, SidLen) == SidLen;
}


/*
 * @implemented
 */
ULONG NTAPI
RtlLengthSid(IN PSID Sid_)
{
  PISID Sid =  Sid_;

  PAGED_CODE_RTL();

  return (ULONG)FIELD_OFFSET(SID,
                             SubAuthority[Sid->SubAuthorityCount]);
}


/*
 * @implemented
 */
NTSTATUS NTAPI
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
NTSTATUS NTAPI
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
PSID_IDENTIFIER_AUTHORITY NTAPI
RtlIdentifierAuthoritySid(IN PSID Sid_)
{
  PISID Sid =  Sid_;

  PAGED_CODE_RTL();

  return &Sid->IdentifierAuthority;
}


/*
 * @implemented
 */
NTSTATUS NTAPI
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

  pSid = RtlpAllocateMemory(RtlLengthRequiredSid(SubAuthorityCount),
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
PVOID NTAPI
RtlFreeSid(IN PSID Sid)
{
   PAGED_CODE_RTL();

   RtlpFreeMemory(Sid, TAG_SID);
   return NULL;
}


/*
 * @implemented
 */
BOOLEAN NTAPI
RtlEqualPrefixSid(IN PSID Sid1_,
                  IN PSID Sid2_)
{
   PISID Sid1 =  Sid1_;
   PISID Sid2 =  Sid2_;
   SIZE_T SidLen;

   PAGED_CODE_RTL();

   if (Sid1->SubAuthorityCount == Sid2->SubAuthorityCount)
   {
      SidLen = FIELD_OFFSET(SID,
                            SubAuthority[Sid1->SubAuthorityCount]);
      return RtlCompareMemory(Sid1,
                              Sid2,
                              SidLen) == SidLen;
   }

   return FALSE;
}


/*
 * @implemented
 */
NTSTATUS NTAPI
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

   if (AllocateBuffer)
   {
      if (!RtlCreateUnicodeString(String,
                                  Buffer))
      {
         return STATUS_NO_MEMORY;
      }
   }
   else
   {
      Length = (wcs - Buffer) * sizeof(WCHAR);

      if (Length > String->MaximumLength)
         return STATUS_BUFFER_TOO_SMALL;

      String->Length = Length;
      RtlCopyMemory (String->Buffer,
                     Buffer,
                     Length);
      if (Length < String->MaximumLength)
         String->Buffer[Length / sizeof(WCHAR)] = 0;
   }

   return STATUS_SUCCESS;
}

/* EOF */
