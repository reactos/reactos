/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/atom.c
 * PURPOSE:         Atom managment
 * PROGRAMMER:      Thomas Weidenmueller
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

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
NTSTATUS NTAPI
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

   /* Use default if size was incorrect */
   if (TableSize <= 1) TableSize = 37;

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
NTSTATUS NTAPI
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
NTSTATUS NTAPI
RtlEmptyAtomTable(PRTL_ATOM_TABLE AtomTable,
                  BOOLEAN DeletePinned)
{
   PRTL_ATOM_TABLE_ENTRY *CurrentBucket, *LastBucket;
   PRTL_ATOM_TABLE_ENTRY CurrentEntry, NextEntry, *PtrEntry;

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
        PtrEntry = CurrentBucket;

        while (NextEntry != NULL)
          {
             CurrentEntry = NextEntry;
             NextEntry = NextEntry->HashLink;

             if (DeletePinned || !(CurrentEntry->Flags & RTL_ATOM_IS_PINNED))
               {
                 *PtrEntry = NextEntry;

                 RtlpFreeAtomHandle(AtomTable,
                                    CurrentEntry);

                 RtlpFreeAtomTableEntry(CurrentEntry);
               }
             else
               {
                 PtrEntry = &CurrentEntry->HashLink;
               }
          }
     }

   RtlpUnlockAtomTable(AtomTable);

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS NTAPI
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

             if (AtomNameLen > RTL_MAXIMUM_ATOM_LENGTH)
             {
                Status = STATUS_INVALID_PARAMETER;
                goto end;
             }

             Entry = RtlpAllocAtomTableEntry(sizeof(RTL_ATOM_TABLE_ENTRY) -
                                             sizeof(Entry->Name) +
                                             (AtomNameLen + 1) * sizeof(WCHAR));
             if (Entry != NULL)
               {
                  Entry->HashLink = NULL;
                  Entry->ReferenceCount = 1;
                  Entry->Flags = 0x0;

                  Entry->NameLength = (UCHAR)AtomNameLen;
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
             Status = STATUS_OBJECT_NAME_INVALID;
          }
     }
end:
   RtlpUnlockAtomTable(AtomTable);

   return Status;
}


/*
 * @implemented
 */
NTSTATUS NTAPI
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
NTSTATUS NTAPI
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
NTSTATUS NTAPI
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

   return Status;
}


/*
 * @implemented
 *
 * This API is really messed up with regards to NameLength. If you pass in a
 * valid buffer for AtomName, NameLength should be the size of the buffer
 * (in bytes, not characters). So if you expect the string to be 6 char long,
 * you need to allocate a buffer of 7 WCHARs and pass 14 for NameLength.
 * The AtomName returned is always null terminated. If the NameLength you pass
 * is smaller than 4 (4 would leave room for 1 character) the function will
 * return with status STATUS_BUFFER_TOO_SMALL. If you pass more than 4, the
 * return status will be STATUS_SUCCESS, even if the buffer is not large enough
 * to hold the complete string. In that case, the string is silently truncated
 * and made to fit in the provided buffer. On return NameLength is set to the
 * number of bytes (but EXCLUDING the bytes for the null terminator) copied.
 * So, if the string is 6 char long, you pass a buffer of 10 bytes, on return
 * NameLength will be set to 8.
 * If you pass in a NULL value for AtomName, the length of the string in bytes
 * (again EXCLUDING the null terminator) is returned in NameLength, at least
 * on Win2k, XP and ReactOS. NT4 will return 0 in that case.
 */
NTSTATUS NTAPI
RtlQueryAtomInAtomTable(PRTL_ATOM_TABLE AtomTable,
                        RTL_ATOM Atom,
                        PULONG RefCount,
                        PULONG PinCount,
                        PWSTR AtomName,
                        PULONG NameLength)
{
   ULONG Length;
   BOOL Unlock = FALSE;

   union
     {
     /* A RTL_ATOM_TABLE_ENTRY has a "WCHAR Name[1]" entry at the end.
      * Make sure we reserve enough room to facilitate a 12 character name */
     RTL_ATOM_TABLE_ENTRY AtomTableEntry;
     WCHAR StringBuffer[sizeof(RTL_ATOM_TABLE_ENTRY) / sizeof(WCHAR) + 12];
     } NumberEntry;
   PRTL_ATOM_TABLE_ENTRY Entry;
   NTSTATUS Status = STATUS_SUCCESS;

   if (Atom < 0xC000)
     {
        /* Synthesize an entry */
        NumberEntry.AtomTableEntry.Atom = Atom;
        NumberEntry.AtomTableEntry.NameLength = swprintf(NumberEntry.AtomTableEntry.Name,
                                                         L"#%lu",
                                                         (ULONG)Atom);
        NumberEntry.AtomTableEntry.ReferenceCount = 1;
        NumberEntry.AtomTableEntry.Flags = RTL_ATOM_IS_PINNED;
        Entry = &NumberEntry.AtomTableEntry;
     }
   else
     {
        RtlpLockAtomTable(AtomTable);
        Unlock = TRUE;

        Entry = RtlpGetAtomEntry(AtomTable,
                                 (ULONG)((USHORT)Atom - 0xC000));
     }

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

        if (NULL != NameLength)
          {
             Length = Entry->NameLength * sizeof(WCHAR);
             if (NULL != AtomName)
               {
                  if (*NameLength < Length + sizeof(WCHAR))
                    {
                       if (*NameLength < 4)
                         {
                            *NameLength = Length;
                            Status = STATUS_BUFFER_TOO_SMALL;
                         }
                       else
                         {
                            Length = *NameLength - sizeof(WCHAR);
                         }
                    }
                  if (NT_SUCCESS(Status))
                    {
                       RtlCopyMemory(AtomName,
                                     Entry->Name,
                                     Length);
                       AtomName[Length / sizeof(WCHAR)] = L'\0';
                       *NameLength = Length;
                    }
               }
             else
               {
                  *NameLength = Length;
               }
          }
        else if (NULL != AtomName)
          {
             Status = STATUS_INVALID_PARAMETER;
          }
     }
   else
     {
        Status = STATUS_INVALID_HANDLE;
     }

   if (Unlock) RtlpUnlockAtomTable(AtomTable);

   return Status;
}


/*
 * @private - only used by NtQueryInformationAtom
 */
NTSTATUS NTAPI
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

