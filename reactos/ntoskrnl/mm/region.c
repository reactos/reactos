/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: region.c,v 1.3 2002/09/08 10:23:36 chorns Exp $
 *
 * PROJECT:     ReactOS kernel
 * FILE:        ntoskrnl/mm/region.c
 * PROGRAMMER:  David Welch
 * PURPOSE:     
 */
 
/* INCLUDE *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/ob.h>
#include <internal/io.h>
#include <internal/ps.h>
#include <internal/pool.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define TAG_MM_REGION    TAG('M', 'R', 'G', 'N')

/* FUNCTIONS *****************************************************************/

VOID STATIC 
InsertAfterEntry(PLIST_ENTRY Previous,
		 PLIST_ENTRY Entry)
/*
 * FUNCTION: Insert a list entry after another entry in the list
 */
{
   Previous->Flink->Blink = Entry;
   
   Entry->Flink = Previous->Flink;
   Entry->Blink = Previous;
   
   Previous->Flink = Entry;
}

PMM_REGION STATIC
MmSplitRegion(PMM_REGION InitialRegion, PVOID InitialBaseAddress,
	      PVOID StartAddress, ULONG Length, ULONG NewType,
	      ULONG NewProtect, PMADDRESS_SPACE AddressSpace,
	      PMM_ALTER_REGION_FUNC AlterFunc)
{
  PMM_REGION NewRegion1; 
  PMM_REGION NewRegion2;
  ULONG InternalLength;
   
  /* Allocate this in front otherwise the failure case is too difficult. */
  NewRegion2 = ExAllocatePoolWithTag(NonPagedPool, sizeof(MM_REGION),
				     TAG_MM_REGION);
  if (NewRegion2 == NULL)
    {
      return(NULL);
    }

  /* Create the new region. */
  NewRegion1 = ExAllocatePoolWithTag(NonPagedPool, sizeof(MM_REGION),
				     TAG_MM_REGION);
  if (NewRegion1 == NULL)
    {
      ExFreePool(NewRegion2);
      return(NULL);
    }
  NewRegion1->Type = NewType;
  NewRegion1->Protect = NewProtect;
  InternalLength = (InitialBaseAddress + InitialRegion->Length) - StartAddress;
  InternalLength = min(InternalLength, Length);
  NewRegion1->Length = InternalLength;
  InsertAfterEntry(&InitialRegion->RegionListEntry,
		   &NewRegion1->RegionListEntry);

  /* 
   * Call our helper function to do the changes on the addresses contained
   * in the initial region.
   */
  AlterFunc(AddressSpace, StartAddress, InternalLength, InitialRegion->Type, 
	    InitialRegion->Protect, NewType, NewProtect);

  /*
   * If necessary create a new region for the portion of the initial region
   * beyond the range of addresses to alter.
   */
  if ((InitialBaseAddress + InitialRegion->Length) > (StartAddress + Length))
    {
      NewRegion2->Type = InitialRegion->Type;
      NewRegion2->Protect = InitialRegion->Protect;
      NewRegion2->Length = (InitialBaseAddress + InitialRegion->Length) - 
	(StartAddress + Length);
      InsertAfterEntry(&NewRegion1->RegionListEntry, 
		       &NewRegion2->RegionListEntry);           
    }
  else
    {
      ExFreePool(NewRegion2);
    }

  /* Either remove or shrink the initial region. */
  if (InitialBaseAddress == StartAddress)
    {
      RemoveEntryList(&InitialRegion->RegionListEntry);
      ExFreePool(InitialRegion);
    }
  else
    {
      InitialRegion->Length = StartAddress - InitialBaseAddress;
    }
  
  return(NewRegion1);
}

NTSTATUS
MmAlterRegion(PMADDRESS_SPACE AddressSpace, PVOID BaseAddress, 
	      PLIST_ENTRY RegionListHead, PVOID StartAddress, ULONG Length, 
	      ULONG NewType, ULONG NewProtect, PMM_ALTER_REGION_FUNC AlterFunc)
{
  PMM_REGION InitialRegion;
  PVOID InitialBaseAddress;  
  PMM_REGION NewRegion;
  PLIST_ENTRY CurrentEntry;
  PMM_REGION CurrentRegion;
  PVOID CurrentBaseAddress;
  ULONG RemainingLength;

  /*
   * Find the first region containing part of the range of addresses to
   * be altered.
   */
  InitialRegion = MmFindRegion(BaseAddress, RegionListHead, StartAddress,
			       &InitialBaseAddress);
  if ((StartAddress + Length) >
      (InitialBaseAddress + InitialRegion->Length))
    {
      RemainingLength = (StartAddress + Length) - 
	(InitialBaseAddress + InitialRegion->Length);
    }
  else
    {
      RemainingLength = 0;
    }
  /*
   * If necessary then split the region into the affected and unaffected parts.
   */
  if (InitialRegion->Type != NewType || InitialRegion->Protect != NewProtect)
    {
      NewRegion = MmSplitRegion(InitialRegion, InitialBaseAddress, 
				StartAddress, Length, NewType, NewProtect, 
				AddressSpace, AlterFunc);
      if (NewRegion == NULL)
	{
	  return(STATUS_NO_MEMORY);
	}
    }
  else
    {
      NewRegion = InitialRegion;
    }
  
  /*
   * Free any complete regions that are containing in the range of addresses
   * and call the helper function to actually do the changes.
   */
  CurrentEntry = NewRegion->RegionListEntry.Flink;
  CurrentBaseAddress = StartAddress + NewRegion->Length;
  while (RemainingLength > 0 && CurrentRegion->Length <= RemainingLength)
    {
      CurrentRegion = CONTAINING_RECORD(CurrentEntry, MM_REGION, 
					RegionListEntry);
      CurrentEntry = CurrentEntry->Flink;      
      if (CurrentRegion->Type != NewType &&
	  CurrentRegion->Protect != NewProtect)
	{
	  AlterFunc(AddressSpace, CurrentBaseAddress, CurrentRegion->Length, 
		    CurrentRegion->Type, CurrentRegion->Protect,
		    NewType, NewProtect);
	}
      CurrentBaseAddress += CurrentRegion->Length;
      NewRegion->Length += CurrentRegion->Length;
      RemainingLength -= CurrentRegion->Length;
      RemoveEntryList(&CurrentRegion->RegionListEntry);
      ExFreePool(CurrentRegion);
    }

  /*
   * Split any final region.
   */
  if (RemainingLength > 0)
    {
      CurrentRegion = CONTAINING_RECORD(CurrentEntry, MM_REGION, 
					RegionListEntry);
      if (CurrentRegion->Type != NewType &&
	  CurrentRegion->Protect != NewProtect)
	{
	  AlterFunc(AddressSpace, CurrentBaseAddress, CurrentRegion->Length, 
		    CurrentRegion->Type, CurrentRegion->Protect,
		    NewType, NewProtect);
	}
      NewRegion->Length += RemainingLength;
      CurrentRegion->Length -= RemainingLength;
    }

  /*
   * If the region after the new region has the same type then merge them.
   */
  if (NewRegion->RegionListEntry.Flink != RegionListHead)
    {
      CurrentEntry = NewRegion->RegionListEntry.Flink;
      CurrentRegion = CONTAINING_RECORD(CurrentEntry, MM_REGION, 
					RegionListEntry);
      if (CurrentRegion->Type == NewRegion->Type &&
	  CurrentRegion->Protect == NewRegion->Protect)
	{
	  NewRegion->Length += CurrentRegion->Length;
	  RemoveEntryList(&CurrentRegion->RegionListEntry);
	  ExFreePool(CurrentRegion);
	}
    }

  /*
   * If the region before the new region has the same type then merge them.
   */
  if (NewRegion->RegionListEntry.Blink != RegionListHead)
    {
      CurrentEntry = NewRegion->RegionListEntry.Blink;
      CurrentRegion = CONTAINING_RECORD(CurrentEntry, MM_REGION, 
					RegionListEntry);
      if (CurrentRegion->Type == NewRegion->Type &&
	  CurrentRegion->Protect == NewRegion->Protect)
	{
	  NewRegion->Length += CurrentRegion->Length;
	  RemoveEntryList(&CurrentRegion->RegionListEntry);
	  ExFreePool(CurrentRegion);
	}
    }

  return(STATUS_SUCCESS);
}

VOID
MmInitialiseRegion(PLIST_ENTRY RegionListHead, ULONG Length, ULONG Type,
		   ULONG Protect)
{
  PMM_REGION Region;

  Region = ExAllocatePoolWithTag(NonPagedPool, sizeof(MM_REGION),
				 TAG_MM_REGION);
  Region->Type = Type;
  Region->Protect = Protect;
  Region->Length = Length;
  InitializeListHead(RegionListHead);
  InsertHeadList(RegionListHead, &Region->RegionListEntry);
}

PMM_REGION
MmFindRegion(PVOID BaseAddress, PLIST_ENTRY RegionListHead, PVOID Address,
	     PVOID* RegionBaseAddress)
{
  PLIST_ENTRY current_entry;
  PMM_REGION current;
  PVOID StartAddress = BaseAddress;
  
  current_entry = RegionListHead->Flink;
  while (current_entry != RegionListHead)
    {
      current = CONTAINING_RECORD(current_entry, MM_REGION, RegionListEntry);

      if (StartAddress <= Address && 
	  (StartAddress + current->Length) > Address)
	{
	  if (RegionBaseAddress != NULL)
	    {
	      *RegionBaseAddress = StartAddress;
	    }
	  return(current);
	}

      current_entry = current_entry->Flink;
      StartAddress += current->Length;
    }
  return(NULL);
}
