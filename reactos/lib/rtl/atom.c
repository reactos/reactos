/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/rtl/atom.c
 * PURPOSE:         Atom managment
 * PROGRAMMER:      Nobody
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include "rtl.h"

#define NDEBUG
#include <debug.h>

/* PROTOTYPES ****************************************************************/

extern NTSTATUS RtlpInitAtomTableLock(PRTL_ATOM_TABLE AtomTable);
extern VOID RtlpDestroyAtomTableLock(PRTL_ATOM_TABLE AtomTable);
extern BOOLEAN RtlpLockAtomTable(PRTL_ATOM_TABLE AtomTable);
extern VOID RtlpUnlockAtomTable(PRTL_ATOM_TABLE AtomTable);

extern BOOLEAN RtlpCreateAtomHandleTable(PRTL_ATOM_TABLE AtomTable);
extern VOID RtlpDestroyAtomHandleTable(PRTL_ATOM_TABLE AtomTable);

extern PRTL_ATOM_TABLE RtlpAllocAtomTable(ULONG Size);
extern VOID RtlpFreeAtomTable(PRTL_ATOM_TABLE AtomTable);
extern PRTL_ATOM_TABLE_ENTRY RtlpAllocAtomTableEntry(ULONG Size);
extern VOID RtlpFreeAtomTableEntry(PRTL_ATOM_TABLE_ENTRY Entry);

extern BOOLEAN RtlpCreateAtomHandle(PRTL_ATOM_TABLE AtomTable, PRTL_ATOM_TABLE_ENTRY Entry);
extern VOID RtlpFreeAtomHandle(PRTL_ATOM_TABLE AtomTable, PRTL_ATOM_TABLE_ENTRY Entry);
extern PRTL_ATOM_TABLE_ENTRY RtlpGetAtomEntry(PRTL_ATOM_TABLE AtomTable, ULONG Index);

/* FUNCTIONS *****************************************************************/

static PRTL_ATOM_TABLE_ENTRY
RtlpHashAtomName(IN PRTL_ATOM_TABLE AtomTable,
                 IN PWSTR AtomName,
                 OUT PRTL_ATOM_TABLE_ENTRY **HashLink)
{
   UNICODE_STRING Name;
   ULONG Hash;

   RtlInitUnicodeString(&Name,
                        AtomName);

   if (Name.Length != 0 &&
       NT_SUCCESS(RtlHashUnicodeString(&Name,
                                       TRUE,
                                       HASH_STRING_ALGORITHM_X65599,
                                       &Hash)))
     {
        PRTL_ATOM_TABLE_ENTRY Current;
        PRTL_ATOM_TABLE_ENTRY *Link;

        Link = &AtomTable->Buckets[Hash % AtomTable->NumberOfBuckets];

        /* search for an existing entry */
        Current = *Link;
        while (Current != NULL)
          {
             if (Current->NameLength == Name.Length / sizeof(WCHAR) &&
                 !_wcsicmp(Current->Name, Name.Buffer))
               {
                  *HashLink = Link;
                  return Current;
               }
             Link = &Current->HashLink;
             Current = Current->HashLink;
          }

        /* no matching atom found, return the hash link */
        *HashLink = Link;
     }
   else
     *HashLink = NULL;

   return NULL;
}

static BOOLEAN
RtlpCheckIntegerAtom(PWSTR AtomName,
                     PUSHORT AtomValue)
{
   UNICODE_STRING AtomString;
   ULONG LongValue;
   USHORT LoValue;
   PWCHAR p;

   DPRINT("RtlpCheckIntegerAtom(AtomName '%S' AtomValue %p)\n",
          AtomName, AtomValue);

   if (!((ULONG)AtomName & 0xFFFF0000))
     {
        LoValue = (USHORT)((ULONG)AtomName & 0xFFFF);

        if (LoValue >= 0xC000)
          return FALSE;

        if (LoValue == 0)
          LoValue = 0xC000;

        if (AtomValue != NULL)
          *AtomValue = LoValue;

        return TRUE;
     }

   if (*AtomName != L'#')
     return FALSE;

   p = AtomName;
   p++;
   while (*p)
     {
        if ((*p < L'0') || (*p > L'9'))
          return FALSE;
        p++;
     }

   p = AtomName;
   p++;
   RtlInitUnicodeString(&AtomString,
                        p);

   DPRINT("AtomString: %wZ\n", &AtomString);

   RtlUnicodeStringToInteger(&AtomString,10, &LongValue);

   DPRINT("LongValue: %lu\n", LongValue);

   *AtomValue = (USHORT)(LongValue & 0x0000FFFF);

   return TRUE;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlCreateAtomTable(IN ULONG TableSize,
                   IN OUT PRTL_ATOM_TABLE *AtomTable)
{
   PRTL_ATOM_TABLE Table;
   NTSTATUS Status;

   DPRINT("RtlCreateAtomTable(TableSize %lu AtomTable %p)\n",
          TableSize, AtomTable);

   if (*AtomTable != NULL)
     {
        return STATUS_SUCCESS;
     }

   /* allocate atom table */
   Table = RtlpAllocAtomTable(((TableSize - 1) * sizeof(PRTL_ATOM_TABLE_ENTRY)) +
                              sizeof(RTL_ATOM_TABLE));
   if (Table == NULL)
     {
        return STATUS_NO_MEMORY;
     }

   /* initialize atom table */
   Table->NumberOfBuckets = TableSize;

   Status = RtlpInitAtomTableLock(Table);
   if (!NT_SUCCESS(Status))
     {
        RtlpFreeAtomTable(Table);
        return Status;
     }

   if (!RtlpCreateAtomHandleTable(Table))
     {
        RtlpDestroyAtomTableLock(Table);
        RtlpFreeAtomTable(Table);
        return STATUS_NO_MEMORY;
     }

   *AtomTable = Table;
   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlDestroyAtomTable(IN PRTL_ATOM_TABLE AtomTable)
{
   PRTL_ATOM_TABLE_ENTRY *CurrentBucket, *LastBucket;
   PRTL_ATOM_TABLE_ENTRY CurrentEntry, NextEntry;
   
   DPRINT("RtlDestroyAtomTable (AtomTable %p)\n", AtomTable);

   if (!RtlpLockAtomTable(AtomTable))
     {
        return (STATUS_INVALID_PARAMETER);
     }

   /* delete all atoms */
   LastBucket = AtomTable->Buckets + AtomTable->NumberOfBuckets;
   for (CurrentBucket = AtomTable->Buckets;
        CurrentBucket != LastBucket;
        CurrentBucket++)
     {
        NextEntry = *CurrentBucket;
        *CurrentBucket = NULL;

        while (NextEntry != NULL)
          {
             CurrentEntry = NextEntry;
             NextEntry = NextEntry->HashLink;

             /* no need to delete the atom handle, the handles will all be freed
                up when destroying the atom handle table! */

             RtlpFreeAtomTableEntry(CurrentEntry);
          }
     }

   RtlpDestroyAtomHandleTable(AtomTable);

   RtlpUnlockAtomTable(AtomTable);

   RtlpDestroyAtomTableLock(AtomTable);

   RtlpFreeAtomTable(AtomTable);

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlEmptyAtomTable(PRTL_ATOM_TABLE AtomTable,
                  BOOLEAN DeletePinned)
{
   PRTL_ATOM_TABLE_ENTRY *CurrentBucket, *LastBucket;
   PRTL_ATOM_TABLE_ENTRY CurrentEntry, NextEntry;

   DPRINT("RtlEmptyAtomTable (AtomTable %p DeletePinned %x)\n",
          AtomTable, DeletePinned);

   if (RtlpLockAtomTable(AtomTable) == FALSE)
     {
        return (STATUS_INVALID_PARAMETER);
     }

   /* delete all atoms */
   LastBucket = AtomTable->Buckets + AtomTable->NumberOfBuckets;
   for (CurrentBucket = AtomTable->Buckets;
        CurrentBucket != LastBucket;
        CurrentBucket++)
     {
        NextEntry = *CurrentBucket;
        *CurrentBucket = NULL;

        while (NextEntry != NULL)
          {
             CurrentEntry = NextEntry;
             NextEntry = NextEntry->HashLink;

             RtlpFreeAtomHandle(AtomTable,
                                CurrentEntry);

             RtlpFreeAtomTableEntry(CurrentEntry);
          }
     }

   RtlpUnlockAtomTable(AtomTable);

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlAddAtomToAtomTable(IN PRTL_ATOM_TABLE AtomTable,
                      IN PWSTR AtomName,
                      OUT PRTL_ATOM Atom)
{
   USHORT AtomValue;
   PRTL_ATOM_TABLE_ENTRY *HashLink;
   PRTL_ATOM_TABLE_ENTRY Entry = NULL;
   NTSTATUS Status = STATUS_SUCCESS;

   DPRINT("RtlAddAtomToAtomTable (AtomTable %p AtomName %S Atom %p)\n",
          AtomTable, AtomName, Atom);

   if (RtlpCheckIntegerAtom (AtomName, &AtomValue))
     {
        /* integer atom */
        if (AtomValue >= 0xC000)
          {
             Status = STATUS_INVALID_PARAMETER;
          }
        else if (Atom != NULL)
          {
             *Atom = (RTL_ATOM)AtomValue;
          }

        return Status;
     }

   RtlpLockAtomTable(AtomTable);

   /* string atom, hash it and try to find an existing atom with the same name */
   Entry = RtlpHashAtomName(AtomTable,
                            AtomName,
                            &HashLink);

   if (Entry != NULL)
     {
        /* found another atom, increment the reference counter unless it's pinned */

        if (!(Entry->Flags & RTL_ATOM_IS_PINNED))
          {
             if (++Entry->ReferenceCount == 0)
               {
                  /* FIXME - references overflowed, pin the atom? */
                  Entry->Flags |= RTL_ATOM_IS_PINNED;
               }
          }

        if (Atom != NULL)
          {
             *Atom = (RTL_ATOM)Entry->Atom;
          }
     }
   else
     {
        /* couldn't find an existing atom, HashLink now points to either the
           HashLink pointer of the previous atom or to the bucket so we can
           simply add it to the list */
        if (HashLink != NULL)
          {
             ULONG AtomNameLen = wcslen(AtomName);

             Entry = RtlpAllocAtomTableEntry(sizeof(RTL_ATOM_TABLE_ENTRY) -
                                             sizeof(Entry->Name) +
                                             (AtomNameLen + 1) * sizeof(WCHAR));
             if (Entry != NULL)
               {
                  Entry->HashLink = NULL;
                  Entry->ReferenceCount = 1;
                  Entry->Flags = 0x0;

                  Entry->NameLength = AtomNameLen;
                  RtlCopyMemory(Entry->Name,
                                AtomName,
                                (AtomNameLen + 1) * sizeof(WCHAR));

                  if (RtlpCreateAtomHandle(AtomTable,
                                           Entry))
                    {
                       /* append the atom to the list */
                       *HashLink = Entry;

                       if (Atom != NULL)
                         {
                            *Atom = (RTL_ATOM)Entry->Atom;
                         }
                    }
                  else
                    {
                       RtlpFreeAtomTableEntry(Entry);
                       Status = STATUS_NO_MEMORY;
                    }
               }
             else
               {
                  Status = STATUS_NO_MEMORY;
               }
          }
        else
          {
             /* The caller supplied an empty atom name! */
             Status = STATUS_INVALID_PARAMETER;
          }
     }

   RtlpUnlockAtomTable(AtomTable);

   return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlDeleteAtomFromAtomTable(IN PRTL_ATOM_TABLE AtomTable,
                           IN RTL_ATOM Atom)
{
   PRTL_ATOM_TABLE_ENTRY Entry;
   NTSTATUS Status = STATUS_SUCCESS;

   DPRINT("RtlDeleteAtomFromAtomTable (AtomTable %p Atom %x)\n",
          AtomTable, Atom);

   if (Atom >= 0xC000)
     {
        RtlpLockAtomTable(AtomTable);

        Entry = RtlpGetAtomEntry(AtomTable,
                                 (ULONG)((USHORT)Atom - 0xC000));

        if (Entry != NULL && Entry->Atom == (USHORT)Atom)
          {
             if (!(Entry->Flags & RTL_ATOM_IS_PINNED))
               {
                  if (--Entry->ReferenceCount == 0)
                    {
                       PRTL_ATOM_TABLE_ENTRY *HashLink;

                       /* it's time to delete the atom. we need to unlink it from
                          the list. The easiest way is to take the atom name and
                          hash it again, this way we get the pointer to either
                          the hash bucket or the previous atom that links to the
                          one we want to delete. This way we can easily bypass
                          this item. */
                       if (RtlpHashAtomName(AtomTable,
                                            Entry->Name,
                                            &HashLink) != NULL)
                         {
                            /* bypass this atom */
                            *HashLink = Entry->HashLink;

                            RtlpFreeAtomHandle(AtomTable,
                                               Entry);

                            RtlpFreeAtomTableEntry(Entry);
                         }
                       else
                         {
                            /* WTF?! This should never happen!!! */
                            ASSERT(FALSE);
                         }
                    }
               }
             else
               {
                  /* tried to delete a pinned atom, do nothing and return
                     STATUS_WAS_LOCKED, which is NOT a failure code! */
                  Status = STATUS_WAS_LOCKED;
               }
          }
        else
          {
             Status = STATUS_INVALID_HANDLE;
          }

        RtlpUnlockAtomTable(AtomTable);
     }

   return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlLookupAtomInAtomTable(IN PRTL_ATOM_TABLE AtomTable,
                         IN PWSTR AtomName,
                         OUT PRTL_ATOM Atom)
{
   PRTL_ATOM_TABLE_ENTRY Entry, *HashLink;
   USHORT AtomValue;
   RTL_ATOM FoundAtom = 0;
   NTSTATUS Status = STATUS_SUCCESS;

   DPRINT("RtlLookupAtomInAtomTable (AtomTable %p AtomName %S Atom %p)\n",
          AtomTable, AtomName, Atom);

   if (RtlpCheckIntegerAtom (AtomName, &AtomValue))
     {
        /* integer atom */
        if (AtomValue >= 0xC000)
          {
             Status = STATUS_INVALID_PARAMETER;
          }
        else if (Atom != NULL)
          {
             *Atom = (RTL_ATOM)AtomValue;
          }

        return Status;
     }

   RtlpLockAtomTable(AtomTable);

   Status = STATUS_OBJECT_NAME_NOT_FOUND;

   /* string atom */
   Entry = RtlpHashAtomName(AtomTable,
                            AtomName,
                            &HashLink);

   if (Entry != NULL)
     {
        Status = STATUS_SUCCESS;
        FoundAtom = (RTL_ATOM)Entry->Atom;
     }

   RtlpUnlockAtomTable(AtomTable);

   if (NT_SUCCESS(Status) && Atom != NULL)
     {
        *Atom = FoundAtom;
     }

   return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlPinAtomInAtomTable(IN PRTL_ATOM_TABLE AtomTable,
                      IN RTL_ATOM Atom)
{
   NTSTATUS Status = STATUS_SUCCESS;

   DPRINT("RtlPinAtomInAtomTable (AtomTable %p Atom %x)\n",
          AtomTable, Atom);

   if (Atom >= 0xC000)
     {
        PRTL_ATOM_TABLE_ENTRY Entry;
        
        RtlpLockAtomTable(AtomTable);

        Entry = RtlpGetAtomEntry(AtomTable,
                                 (ULONG)((USHORT)Atom - 0xC000));

        if (Entry != NULL && Entry->Atom == (USHORT)Atom)
          {
             Entry->Flags |= RTL_ATOM_IS_PINNED;
          }
        else
          {
             Status = STATUS_INVALID_HANDLE;
          }

        RtlpUnlockAtomTable(AtomTable);
     }

   RtlpLockAtomTable(AtomTable);

   return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlQueryAtomInAtomTable(PRTL_ATOM_TABLE AtomTable,
                        RTL_ATOM Atom,
                        PULONG RefCount,
                        PULONG PinCount,
                        PWSTR AtomName,
                        PULONG NameLength)
{
   ULONG Length;
   PRTL_ATOM_TABLE_ENTRY Entry;
   NTSTATUS Status = STATUS_SUCCESS;

   if (Atom < 0xC000)
     {
        if (RefCount != NULL)
          {
             *RefCount = 1;
          }

        if (PinCount != NULL)
          {
             *PinCount = 1;
          }

        if ((AtomName != NULL) && (NameLength != NULL) && (NameLength > 0))
          {
             WCHAR NameString[12];
             
             Length = swprintf(NameString, L"#%lu", (ULONG)Atom) * sizeof(WCHAR);

             if (*NameLength < Length + sizeof(WCHAR))
               {
                  /* prevent underflow! */
                  if (*NameLength >= sizeof(WCHAR))
                    {
                       Length = *NameLength - sizeof(WCHAR);
                    }
                  else
                    {
                       Length = 0;
                       Status = STATUS_BUFFER_TOO_SMALL;
                    }
               }

             if (Length)
               {
                  RtlCopyMemory(AtomName,
                                NameString,
                                Length);
                  AtomName[Length / sizeof(WCHAR)] = L'\0';
                  *NameLength = Length;
               }
          }

        return Status;
     }

   RtlpLockAtomTable(AtomTable);

   Entry = RtlpGetAtomEntry(AtomTable,
                            (ULONG)((USHORT)Atom - 0xC000));

   if (Entry != NULL && Entry->Atom == (USHORT)Atom)
     {
        DPRINT("Atom name: %wZ\n", &Entry->Name);
        
        if (RefCount != NULL)
          {
             *RefCount = Entry->ReferenceCount;
          }

        if (PinCount != NULL)
          {
             *PinCount = ((Entry->Flags & RTL_ATOM_IS_PINNED) != 0);
          }

        if ((AtomName != NULL) && (NameLength != NULL))
          {
             Length = Entry->NameLength * sizeof(WCHAR);

             if (*NameLength < Length + sizeof(WCHAR))
               {
                  /* prevent underflow! */
                  if (*NameLength >= sizeof(WCHAR))
                    {
                       Length = *NameLength - sizeof(WCHAR);
                    }
                  else
                    {
                       Length = 0;
                       Status = STATUS_BUFFER_TOO_SMALL;
                    }
               }

             if (Length)
               {
                  RtlCopyMemory(AtomName,
                                Entry->Name,
                                Length);
                  AtomName[Length / sizeof(WCHAR)] = L'\0';
                  *NameLength = Length;
               }
          }
     }
   else
     {
        Status = STATUS_INVALID_HANDLE;
     }

   RtlpUnlockAtomTable(AtomTable);

   return Status;
}


/*
 * @private - only used by NtQueryInformationAtom
 */
NTSTATUS STDCALL
RtlQueryAtomListInAtomTable(IN PRTL_ATOM_TABLE AtomTable,
                            IN ULONG MaxAtomCount,
                            OUT ULONG *AtomCount,
                            OUT RTL_ATOM *AtomList)
{
   PRTL_ATOM_TABLE_ENTRY *CurrentBucket, *LastBucket;
   PRTL_ATOM_TABLE_ENTRY CurrentEntry;
   ULONG Atoms = 0;
   NTSTATUS Status = STATUS_SUCCESS;
   
   RtlpLockAtomTable(AtomTable);
   
   LastBucket = AtomTable->Buckets + AtomTable->NumberOfBuckets;
   for (CurrentBucket = AtomTable->Buckets;
        CurrentBucket != LastBucket;
        CurrentBucket++)
     {
        CurrentEntry = *CurrentBucket;

        while (CurrentEntry != NULL)
          {
             if (MaxAtomCount > 0)
               {
                  *(AtomList++) = (RTL_ATOM)CurrentEntry->Atom;
                  MaxAtomCount--;
               }
             else
               {
                  /* buffer too small, but don't bail. we need to determine the
                     total number of atoms in the table! */
                  Status = STATUS_INFO_LENGTH_MISMATCH;
               }

             Atoms++;
             CurrentEntry = CurrentEntry->HashLink;
          }
     }
   
   *AtomCount = Atoms;
   
   RtlpUnlockAtomTable(AtomTable);

   return Status;
}

