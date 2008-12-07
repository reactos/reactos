/*
 * Copyright (C) 1998-2005 ReactOS Team (and the authors from the programmers section)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/marea.c
 * PURPOSE:         Implements memory areas
 *
 * PROGRAMMERS:     Rex Jolliff
 *                  David Welch
 *                  Eric Kohl
 *                  Philip Susi
 *                  Casper Hornstrup
 *                  Eric Kohl
 *                  Ge van Geldorp
 *                  Royce Mitchell III
 *                  Aleksey Bragin
 *                  Jason Filby
 *                  Thomas Weidenmueller
 *                  Gunnar Andre' Dalsnes
 *                  Mike Nordell
 *                  Alex Ionescu
 *                  Filip Navara
 *                  Herve Poussineau
 *                  Steven Edwards
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmInitMemoryAreas)
#endif

/* #define VALIDATE_MEMORY_AREAS */

/* FUNCTIONS *****************************************************************/

/**
 * @name MmIterateFirstNode
 *
 * @param Node
 *        Head node of the MEMORY_AREA tree.
 *
 * @return The leftmost MEMORY_AREA node (ie. the one with lowest
 *         address)
 */

static PMEMORY_AREA MmIterateFirstNode(PMEMORY_AREA Node)
{
   while (Node->LeftChild != NULL)
      Node = Node->LeftChild;

   return Node;
}

/**
 * @name MmIterateNextNode
 *
 * @param Node
 *        Current node in the tree.
 *
 * @return Next node in the tree (sorted by address).
 */

static PMEMORY_AREA MmIterateNextNode(PMEMORY_AREA Node)
{
   if (Node->RightChild != NULL)
   {
      Node = Node->RightChild;
      while (Node->LeftChild != NULL)
         Node = Node->LeftChild;
   }
   else
   {
      PMEMORY_AREA TempNode = NULL;

      do
      {
         /* Check if we're at the end of tree. */
         if (Node->Parent == NULL)
            return NULL;

         TempNode = Node;
         Node = Node->Parent;
      }
      while (TempNode == Node->RightChild);
   }
   return Node;
}

/**
 * @name MmIterateLastNode
 *
 * @param Node
 *        Head node of the MEMORY_AREA tree.
 *
 * @return The rightmost MEMORY_AREA node (ie. the one with highest
 *         address)
 */

static PMEMORY_AREA MmIterateLastNode(PMEMORY_AREA Node)
{
   while (Node->RightChild != NULL)
      Node = Node->RightChild;

   return Node;
}

/**
 * @name MmIteratePreviousNode
 *
 * @param Node
 *        Current node in the tree.
 *
 * @return Previous node in the tree (sorted by address).
 */

static PMEMORY_AREA MmIteratePrevNode(PMEMORY_AREA Node)
{
   if (Node->LeftChild != NULL)
   {
      Node = Node->LeftChild;
      while (Node->RightChild != NULL)
         Node = Node->RightChild;
   }
   else
   {
      PMEMORY_AREA TempNode = NULL;

      do
      {
         /* Check if we're at the end of tree. */
         if (Node->Parent == NULL)
            return NULL;

         TempNode = Node;
         Node = Node->Parent;
      }
      while (TempNode == Node->LeftChild);
   }
   return Node;
}

#ifdef VALIDATE_MEMORY_AREAS
static VOID MmVerifyMemoryAreas(PMM_AVL_TABLE AddressSpace)
{
   PMEMORY_AREA Node;

   ASSERT(AddressSpace != NULL);

   /* Special case for empty tree. */
   if (AddressSpace->BalancedRoot.u1.Parent == NULL)
      return;

   /* Traverse the tree from left to right. */
   for (Node = MmIterateFirstNode(AddressSpace->BalancedRoot.u1.Parent);
        Node != NULL;
        Node = MmIterateNextNode(Node))
   {
      /* FiN: The starting address can be NULL if someone explicitely asks
       * for NULL address. */
      ASSERT(Node->StartingAddress == NULL);
      ASSERT(Node->EndingAddress >= Node->StartingAddress);
   }
}
#else
#define MmVerifyMemoryAreas(x)
#endif

VOID NTAPI
MmDumpMemoryAreas(PMM_AVL_TABLE AddressSpace)
{
   PMEMORY_AREA Node;

   DbgPrint("MmDumpMemoryAreas()\n");

   /* Special case for empty tree. */
   if (AddressSpace->BalancedRoot.u1.Parent == NULL)
      return;

   /* Traverse the tree from left to right. */
   for (Node = MmIterateFirstNode((PMEMORY_AREA)AddressSpace->BalancedRoot.u1.Parent);
        Node != NULL;
        Node = MmIterateNextNode(Node))
   {
      DbgPrint("Start %p End %p Protect %x Flags %x\n",
               Node->StartingAddress, Node->EndingAddress,
               Node->Protect, Node->Flags);
   }

   DbgPrint("Finished MmDumpMemoryAreas()\n");
}

PMEMORY_AREA NTAPI
MmLocateMemoryAreaByAddress(
   PMM_AVL_TABLE AddressSpace,
   PVOID Address)
{
   PMEMORY_AREA Node = (PMEMORY_AREA)AddressSpace->BalancedRoot.u1.Parent;

   DPRINT("MmLocateMemoryAreaByAddress(AddressSpace %p, Address %p)\n",
           AddressSpace, Address);

   MmVerifyMemoryAreas(AddressSpace);

   while (Node != NULL)
   {
      if (Address < Node->StartingAddress)
         Node = Node->LeftChild;
      else if (Address >= Node->EndingAddress)
         Node = Node->RightChild;
      else
      {
         DPRINT("MmLocateMemoryAreaByAddress(%p): %p [%p - %p]\n",
                Address, Node, Node->StartingAddress, Node->EndingAddress);
         return Node;
      }
   }

   DPRINT("MmLocateMemoryAreaByAddress(%p): 0\n", Address);
   return NULL;
}

PMEMORY_AREA NTAPI
MmLocateMemoryAreaByRegion(
   PMM_AVL_TABLE AddressSpace,
   PVOID Address,
   ULONG_PTR Length)
{
   PMEMORY_AREA Node;
   PVOID Extent = (PVOID)((ULONG_PTR)Address + Length);

   MmVerifyMemoryAreas(AddressSpace);

   /* Special case for empty tree. */
   if (AddressSpace->BalancedRoot.u1.Parent == NULL)
      return NULL;

   /* Traverse the tree from left to right. */
   for (Node = MmIterateFirstNode((PMEMORY_AREA)AddressSpace->BalancedRoot.u1.Parent);
        Node != NULL;
        Node = MmIterateNextNode(Node))
   {
      if (Node->StartingAddress >= Address &&
          Node->StartingAddress < Extent)
      {
         DPRINT("MmLocateMemoryAreaByRegion(%p - %p): %p - %p\n",
                Address, (ULONG_PTR)Address + Length, Node->StartingAddress,
                Node->EndingAddress);
         return Node;
      }
      if (Node->EndingAddress > Address &&
          Node->EndingAddress < Extent)
      {
         DPRINT("MmLocateMemoryAreaByRegion(%p - %p): %p - %p\n",
                Address, (ULONG_PTR)Address + Length, Node->StartingAddress,
                Node->EndingAddress);
         return Node;
      }
      if (Node->StartingAddress <= Address &&
          Node->EndingAddress >= Extent)
      {
         DPRINT("MmLocateMemoryAreaByRegion(%p - %p): %p - %p\n",
                Address, (ULONG_PTR)Address + Length, Node->StartingAddress,
                Node->EndingAddress);
         return Node;
      }
      if (Node->StartingAddress >= Extent)
      {
         DPRINT("Finished MmLocateMemoryAreaByRegion() = NULL\n");
         return NULL;
      }
   }

   return NULL;
}

/**
 * @name MmCompressHelper
 *
 * This is helper of MmRebalanceTree. Performs a compression transformation
 * count times, starting at root.
 */

static VOID
MmCompressHelper(
   PMM_AVL_TABLE AddressSpace,
   ULONG Count)
{
   PMEMORY_AREA Root = NULL;
   PMEMORY_AREA Red = (PMEMORY_AREA)AddressSpace->BalancedRoot.u1.Parent;
   PMEMORY_AREA Black = Red->LeftChild;

   while (Count--)
   {
      if (Root)
         Root->LeftChild = Black;
      else
         AddressSpace->BalancedRoot.u1.Parent = (PVOID)Black;
      Black->Parent = Root;
      Red->LeftChild = Black->RightChild;
      if (Black->RightChild)
         Black->RightChild->Parent = Red;
      Black->RightChild = Red;
      Red->Parent = Black;
      Root = Black;

      if (Count)
      {
         Red = Root->LeftChild;
         Black = Red->LeftChild;
      }
   }
}

/**
 * @name MmRebalanceTree
 *
 * Rebalance a memory area tree using the Tree->Vine->Balanced Tree
 * method described in libavl documentation in chapter 4.12.
 * (http://www.stanford.edu/~blp/avl/libavl.html/)
 */

static VOID
MmRebalanceTree(
   PMM_AVL_TABLE AddressSpace)
{
   PMEMORY_AREA PreviousNode;
   PMEMORY_AREA CurrentNode;
   PMEMORY_AREA TempNode;
   ULONG NodeCount = 0;
   ULONG Vine;   /* Number of nodes in main vine. */
   ULONG Leaves; /* Nodes in incomplete bottom level, if any. */
   INT Height;   /* Height of produced balanced tree. */

   /* Transform the tree into Vine. */

   PreviousNode = NULL;
   CurrentNode = (PMEMORY_AREA)AddressSpace->BalancedRoot.u1.Parent;
   while (CurrentNode != NULL)
   {
      if (CurrentNode->RightChild == NULL)
      {
         PreviousNode = CurrentNode;
         CurrentNode = CurrentNode->LeftChild;
         NodeCount++;
      }
      else
      {
         TempNode = CurrentNode->RightChild;

         CurrentNode->RightChild = TempNode->LeftChild;
         if (TempNode->LeftChild)
            TempNode->LeftChild->Parent = CurrentNode;

         TempNode->LeftChild = CurrentNode;
         CurrentNode->Parent = TempNode;

         CurrentNode = TempNode;

         if (PreviousNode != NULL)
            PreviousNode->LeftChild = TempNode;
         else
            AddressSpace->BalancedRoot.u1.Parent = (PVOID)TempNode;
         TempNode->Parent = PreviousNode;
      }
   }

   /* Transform Vine back into a balanced tree. */

   Leaves = NodeCount + 1;
   for (;;)
   {
      ULONG Next = Leaves & (Leaves - 1);
      if (Next == 0)
         break;
      Leaves = Next;
   }
   Leaves = NodeCount + 1 - Leaves;

   MmCompressHelper(AddressSpace, Leaves);

   Vine = NodeCount - Leaves;
   Height = 1 + (Leaves > 0);
   while (Vine > 1)
   {
      MmCompressHelper(AddressSpace, Vine / 2);
      Vine /= 2;
      Height++;
   }
}

static VOID
MmInsertMemoryArea(
   PMM_AVL_TABLE AddressSpace,
   PMEMORY_AREA marea)
{
   PMEMORY_AREA Node;
   PMEMORY_AREA PreviousNode;
   ULONG Depth = 0;

   MmVerifyMemoryAreas(AddressSpace);

   if (AddressSpace->BalancedRoot.u1.Parent == NULL)
   {
      AddressSpace->BalancedRoot.u1.Parent = (PVOID)marea;
      marea->LeftChild = marea->RightChild = marea->Parent = NULL;
      return;
   }

   Node = (PMEMORY_AREA)AddressSpace->BalancedRoot.u1.Parent;
   do
   {
      DPRINT("marea->EndingAddress: %p Node->StartingAddress: %p\n",
             marea->EndingAddress, Node->StartingAddress);
      DPRINT("marea->StartingAddress: %p Node->EndingAddress: %p\n",
             marea->StartingAddress, Node->EndingAddress);
      ASSERT(marea->EndingAddress <= Node->StartingAddress ||
             marea->StartingAddress >= Node->EndingAddress);
      ASSERT(marea->StartingAddress != Node->StartingAddress);

      PreviousNode = Node;

      if (marea->StartingAddress < Node->StartingAddress)
         Node = Node->LeftChild;
      else
         Node = Node->RightChild;

      if (Node)
      {
         Depth++;
         if (Depth == 22)
         {
            MmRebalanceTree(AddressSpace);
            PreviousNode = Node->Parent;
         }
      }
   }
   while (Node != NULL);

   marea->LeftChild = marea->RightChild = NULL;
   marea->Parent = PreviousNode;
   if (marea->StartingAddress < PreviousNode->StartingAddress)
      PreviousNode->LeftChild = marea;
   else
      PreviousNode->RightChild = marea;
}

static PVOID
MmFindGapBottomUp(
   PMM_AVL_TABLE AddressSpace,
   ULONG_PTR Length,
   ULONG_PTR Granularity)
{
   PVOID LowestAddress  = MmGetAddressSpaceOwner(AddressSpace) ? MM_LOWEST_USER_ADDRESS : MmSystemRangeStart;
   PVOID HighestAddress = MmGetAddressSpaceOwner(AddressSpace) ?
                          (PVOID)((ULONG_PTR)MmSystemRangeStart - 1) : (PVOID)MAXULONG_PTR;
   PVOID AlignedAddress;
   PMEMORY_AREA Node;
   PMEMORY_AREA FirstNode;
   PMEMORY_AREA PreviousNode;

   MmVerifyMemoryAreas(AddressSpace);

   DPRINT("LowestAddress: %p HighestAddress: %p\n",
          LowestAddress, HighestAddress);

   AlignedAddress = MM_ROUND_UP(LowestAddress, Granularity);

   /* Special case for empty tree. */
   if (AddressSpace->BalancedRoot.u1.Parent == NULL)
   {
      if ((ULONG_PTR)HighestAddress - (ULONG_PTR)AlignedAddress >= Length)
      {
         DPRINT("MmFindGapBottomUp: %p\n", AlignedAddress);
         return AlignedAddress;
      }
      DPRINT("MmFindGapBottomUp: 0\n");
      return 0;
   }

   /* Go to the node with lowest address in the tree. */
   FirstNode = Node = MmIterateFirstNode((PMEMORY_AREA)AddressSpace->BalancedRoot.u1.Parent);

   /* Traverse the tree from left to right. */
   PreviousNode = Node;
   for (;;)
   {
      Node = MmIterateNextNode(Node);
      if (Node == NULL)
         break;

      AlignedAddress = MM_ROUND_UP(PreviousNode->EndingAddress, Granularity);
      if (Node->StartingAddress > AlignedAddress &&
          (ULONG_PTR)Node->StartingAddress - (ULONG_PTR)AlignedAddress >= Length)
      {
         DPRINT("MmFindGapBottomUp: %p\n", AlignedAddress);
         return AlignedAddress;
      }

      PreviousNode = Node;
   }

   /* Check if there is enough space after the last memory area. */
   AlignedAddress = MM_ROUND_UP(PreviousNode->EndingAddress, Granularity);
   if ((ULONG_PTR)HighestAddress > (ULONG_PTR)AlignedAddress &&
       (ULONG_PTR)HighestAddress - (ULONG_PTR)AlignedAddress >= Length)
   {
      DPRINT("MmFindGapBottomUp: %p\n", AlignedAddress);
      return AlignedAddress;
   }

   /* Check if there is enough space before the first memory area. */
   AlignedAddress = MM_ROUND_UP(LowestAddress, Granularity);
   if (FirstNode->StartingAddress > AlignedAddress &&
       (ULONG_PTR)FirstNode->StartingAddress - (ULONG_PTR)AlignedAddress >= Length)
   {
      DPRINT("MmFindGapBottomUp: %p\n", AlignedAddress);
      return AlignedAddress;
   }

   DPRINT("MmFindGapBottomUp: 0\n");
   return 0;
}


static PVOID
MmFindGapTopDown(
   PMM_AVL_TABLE AddressSpace,
   ULONG_PTR Length,
   ULONG_PTR Granularity)
{
   PVOID LowestAddress  = MmGetAddressSpaceOwner(AddressSpace) ? MM_LOWEST_USER_ADDRESS : MmSystemRangeStart;
   PVOID HighestAddress = MmGetAddressSpaceOwner(AddressSpace) ?
                          (PVOID)((ULONG_PTR)MmSystemRangeStart - 1) : (PVOID)MAXULONG_PTR;
   PVOID AlignedAddress;
   PMEMORY_AREA Node;
   PMEMORY_AREA PreviousNode;

   MmVerifyMemoryAreas(AddressSpace);

   DPRINT("LowestAddress: %p HighestAddress: %p\n",
          LowestAddress, HighestAddress);

   AlignedAddress = MM_ROUND_DOWN((ULONG_PTR)HighestAddress - Length + 1, Granularity);

   /* Check for overflow. */
   if (AlignedAddress > HighestAddress)
      return NULL;

   /* Special case for empty tree. */
   if (AddressSpace->BalancedRoot.u1.Parent == NULL)
   {
      if (AlignedAddress >= LowestAddress)
      {
         DPRINT("MmFindGapTopDown: %p\n", AlignedAddress);
         return AlignedAddress;
      }
      DPRINT("MmFindGapTopDown: 0\n");
      return 0;
   }

   /* Go to the node with highest address in the tree. */
   Node = MmIterateLastNode((PMEMORY_AREA)AddressSpace->BalancedRoot.u1.Parent);

   /* Check if there is enough space after the last memory area. */
   if (Node->EndingAddress <= AlignedAddress)
   {
      DPRINT("MmFindGapTopDown: %p\n", AlignedAddress);
      return AlignedAddress;
   }

   /* Traverse the tree from left to right. */
   PreviousNode = Node;
   for (;;)
   {
      Node = MmIteratePrevNode(Node);
      if (Node == NULL)
         break;

      AlignedAddress = MM_ROUND_DOWN((ULONG_PTR)PreviousNode->StartingAddress - Length + 1, Granularity);

      /* Check for overflow. */
      if (AlignedAddress > PreviousNode->StartingAddress)
         return NULL;

      if (Node->EndingAddress <= AlignedAddress)
      {
         DPRINT("MmFindGapTopDown: %p\n", AlignedAddress);
         return AlignedAddress;
      }

      PreviousNode = Node;
   }

   AlignedAddress = MM_ROUND_DOWN((ULONG_PTR)PreviousNode->StartingAddress - Length + 1, Granularity);

   /* Check for overflow. */
   if (AlignedAddress > PreviousNode->StartingAddress)
      return NULL;

   if (AlignedAddress >= LowestAddress)
   {
      DPRINT("MmFindGapTopDown: %p\n", AlignedAddress);
      return AlignedAddress;
   }

   DPRINT("MmFindGapTopDown: 0\n");
   return 0;
}


PVOID NTAPI
MmFindGap(
   PMM_AVL_TABLE AddressSpace,
   ULONG_PTR Length,
   ULONG_PTR Granularity,
   BOOLEAN TopDown)
{
   if (TopDown)
      return MmFindGapTopDown(AddressSpace, Length, Granularity);

   return MmFindGapBottomUp(AddressSpace, Length, Granularity);
}

ULONG_PTR NTAPI
MmFindGapAtAddress(
   PMM_AVL_TABLE AddressSpace,
   PVOID Address)
{
   PMEMORY_AREA Node = (PMEMORY_AREA)AddressSpace->BalancedRoot.u1.Parent;
   PMEMORY_AREA RightNeighbour = NULL;
   PVOID LowestAddress  = MmGetAddressSpaceOwner(AddressSpace) ? MM_LOWEST_USER_ADDRESS : MmSystemRangeStart;
   PVOID HighestAddress = MmGetAddressSpaceOwner(AddressSpace) ?
                          (PVOID)((ULONG_PTR)MmSystemRangeStart - 1) : (PVOID)MAXULONG_PTR;

   MmVerifyMemoryAreas(AddressSpace);

   Address = MM_ROUND_DOWN(Address, PAGE_SIZE);

   if (LowestAddress < MmSystemRangeStart)
   {
      if (Address >= MmSystemRangeStart)
      {
         return 0;
      }
   }
   else
   {
      if (Address < LowestAddress)
      {
         return 0;
      }
   }

   while (Node != NULL)
   {
      if (Address < Node->StartingAddress)
      {
         RightNeighbour = Node;
         Node = Node->LeftChild;
      }
      else if (Address >= Node->EndingAddress)
      {
         Node = Node->RightChild;
      }
      else
      {
         DPRINT("MmFindGapAtAddress: 0\n");
         return 0;
      }
   }

   if (RightNeighbour)
   {
      DPRINT("MmFindGapAtAddress: %p [%p]\n", Address,
             (ULONG_PTR)RightNeighbour->StartingAddress - (ULONG_PTR)Address);
      return (ULONG_PTR)RightNeighbour->StartingAddress - (ULONG_PTR)Address;
   }
   else
   {
      DPRINT("MmFindGapAtAddress: %p [%p]\n", Address,
             (ULONG_PTR)HighestAddress - (ULONG_PTR)Address);
      return (ULONG_PTR)HighestAddress - (ULONG_PTR)Address;
   }
}

/**
 * @name MmInitMemoryAreas
 *
 * Initialize the memory area list implementation.
 */

NTSTATUS
INIT_FUNCTION
NTAPI
MmInitMemoryAreas(VOID)
{
   DPRINT("MmInitMemoryAreas()\n");
   return(STATUS_SUCCESS);
}


/**
 * @name MmFreeMemoryArea
 *
 * Free an existing memory area.
 *
 * @param AddressSpace
 *        Address space to free the area from.
 * @param MemoryArea
 *        Memory area we're about to free.
 * @param FreePage
 *        Callback function for each freed page.
 * @param FreePageContext
 *        Context passed to the callback function.
 *
 * @return Status
 *
 * @remarks Lock the address space before calling this function.
 */

NTSTATUS NTAPI
MmFreeMemoryArea(
   PMM_AVL_TABLE AddressSpace,
   PMEMORY_AREA MemoryArea,
   PMM_FREE_PAGE_FUNC FreePage,
   PVOID FreePageContext)
{
   PMEMORY_AREA *ParentReplace;
   ULONG_PTR Address;
   PVOID EndAddress;
   PEPROCESS CurrentProcess = PsGetCurrentProcess();
   PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);
    
   if (Process != NULL &&
       Process != CurrentProcess)
   {
      KeAttachProcess(&Process->Pcb);
   }

   EndAddress = MM_ROUND_UP(MemoryArea->EndingAddress, PAGE_SIZE);
   for (Address = (ULONG_PTR)MemoryArea->StartingAddress;
        Address < (ULONG_PTR)EndAddress;
        Address += PAGE_SIZE)
   {
      if (MemoryArea->Type == MEMORY_AREA_IO_MAPPING)
      {
         MmRawDeleteVirtualMapping((PVOID)Address);
      }
      else
      {
         BOOLEAN Dirty = FALSE;
         SWAPENTRY SwapEntry = 0;
         PFN_TYPE Page = 0;

         if (MmIsPageSwapEntry(Process, (PVOID)Address))
         {
            MmDeletePageFileMapping(Process, (PVOID)Address, &SwapEntry);
         }
         else
         {
            MmDeleteVirtualMapping(Process, (PVOID)Address, FALSE, &Dirty, &Page);
         }
         if (FreePage != NULL)
         {
            FreePage(FreePageContext, MemoryArea, (PVOID)Address,
                     Page, SwapEntry, (BOOLEAN)Dirty);
         }
      }
   }

   if (Process != NULL &&
       Process != CurrentProcess)
   {
      KeDetachProcess();
   }

   /* Remove the tree item. */
   {
      if (MemoryArea->Parent != NULL)
      {
         if (MemoryArea->Parent->LeftChild == MemoryArea)
            ParentReplace = &MemoryArea->Parent->LeftChild;
         else
            ParentReplace = &MemoryArea->Parent->RightChild;
      }
      else
         ParentReplace = (PMEMORY_AREA*)&AddressSpace->BalancedRoot.u1.Parent;

      if (MemoryArea->RightChild == NULL)
      {
         *ParentReplace = MemoryArea->LeftChild;
         if (MemoryArea->LeftChild)
            MemoryArea->LeftChild->Parent = MemoryArea->Parent;
      }
      else
      {
         if (MemoryArea->RightChild->LeftChild == NULL)
         {
            MemoryArea->RightChild->LeftChild = MemoryArea->LeftChild;
            if (MemoryArea->LeftChild)
               MemoryArea->LeftChild->Parent = MemoryArea->RightChild;

            *ParentReplace = MemoryArea->RightChild;
            MemoryArea->RightChild->Parent = MemoryArea->Parent;
         }
         else
         {
            PMEMORY_AREA LowestNode;

            LowestNode = MemoryArea->RightChild->LeftChild;
            while (LowestNode->LeftChild != NULL)
               LowestNode = LowestNode->LeftChild;

            LowestNode->Parent->LeftChild = LowestNode->RightChild;
            if (LowestNode->RightChild)
               LowestNode->RightChild->Parent = LowestNode->Parent;

            LowestNode->LeftChild = MemoryArea->LeftChild;
            if (MemoryArea->LeftChild)
               MemoryArea->LeftChild->Parent = LowestNode;

            LowestNode->RightChild = MemoryArea->RightChild;
            MemoryArea->RightChild->Parent = LowestNode;

            *ParentReplace = LowestNode;
            LowestNode->Parent = MemoryArea->Parent;
         }
      }
   }

   ExFreePoolWithTag(MemoryArea, TAG_MAREA);

   DPRINT("MmFreeMemoryAreaByNode() succeeded\n");

   return STATUS_SUCCESS;
}

/**
 * @name MmFreeMemoryAreaByPtr
 *
 * Free an existing memory area given a pointer inside it.
 *
 * @param AddressSpace
 *        Address space to free the area from.
 * @param BaseAddress
 *        Address in the memory area we're about to free.
 * @param FreePage
 *        Callback function for each freed page.
 * @param FreePageContext
 *        Context passed to the callback function.
 *
 * @return Status
 *
 * @see MmFreeMemoryArea
 *
 * @todo Should we require the BaseAddress to be really the starting
 *       address of the memory area or is the current relaxed check
 *       (BaseAddress can point anywhere in the memory area) acceptable?
 *
 * @remarks Lock the address space before calling this function.
 */

NTSTATUS NTAPI
MmFreeMemoryAreaByPtr(
   PMM_AVL_TABLE AddressSpace,
   PVOID BaseAddress,
   PMM_FREE_PAGE_FUNC FreePage,
   PVOID FreePageContext)
{
   PMEMORY_AREA MemoryArea;

   DPRINT("MmFreeMemoryArea(AddressSpace %p, BaseAddress %p, "
          "FreePageContext %p)\n", AddressSpace, BaseAddress,
          FreePageContext);

   MmVerifyMemoryAreas(AddressSpace);

   MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace,
                                            BaseAddress);
   if (MemoryArea == NULL)
   {
      KeBugCheck(MEMORY_MANAGEMENT);
      return(STATUS_UNSUCCESSFUL);
   }

   return MmFreeMemoryArea(AddressSpace, MemoryArea, FreePage, FreePageContext);
}

/**
 * @name MmCreateMemoryArea
 *
 * Create a memory area.
 *
 * @param AddressSpace
 *        Address space to create the area in.
 * @param Type
 *        Type of the memory area.
 * @param BaseAddress
 *        Base address for the memory area we're about the create. On
 *        input it contains either 0 (auto-assign address) or preferred
 *        address. On output it contains the starting address of the
 *        newly created area.
 * @param Length
 *        Length of the area to allocate.
 * @param Attributes
 *        Protection attributes for the memory area.
 * @param Result
 *        Receives a pointer to the memory area on successful exit.
 *
 * @return Status
 *
 * @remarks Lock the address space before calling this function.
 */

NTSTATUS NTAPI
MmCreateMemoryArea(PMM_AVL_TABLE AddressSpace,
                   ULONG Type,
                   PVOID *BaseAddress,
                   ULONG_PTR Length,
                   ULONG Protect,
                   PMEMORY_AREA *Result,
                   BOOLEAN FixedAddress,
                   ULONG AllocationFlags,
                   PHYSICAL_ADDRESS BoundaryAddressMultiple)
{
   PVOID EndAddress;
   ULONG Granularity;
   ULONG tmpLength;
   PMEMORY_AREA MemoryArea;

   DPRINT("MmCreateMemoryArea(Type %d, BaseAddress %p, "
          "*BaseAddress %p, Length %p, AllocationFlags %x, "
          "FixedAddress %x, Result %p)\n",
          Type, BaseAddress, *BaseAddress, Length, AllocationFlags,
          FixedAddress, Result);

   MmVerifyMemoryAreas(AddressSpace);

   Granularity = (MEMORY_AREA_VIRTUAL_MEMORY == Type ? MM_VIRTMEM_GRANULARITY : PAGE_SIZE);
   if ((*BaseAddress) == 0 && !FixedAddress)
   {
      tmpLength = PAGE_ROUND_UP(Length);
      *BaseAddress = MmFindGap(AddressSpace,
                               tmpLength,
                               Granularity,
                               (AllocationFlags & MEM_TOP_DOWN) == MEM_TOP_DOWN);
      if ((*BaseAddress) == 0)
      {
         DPRINT("No suitable gap\n");
         return STATUS_NO_MEMORY;
      }
   }
   else
   {
      tmpLength = Length + ((ULONG_PTR) *BaseAddress
                         - (ULONG_PTR) MM_ROUND_DOWN(*BaseAddress, Granularity));
      *BaseAddress = MM_ROUND_DOWN(*BaseAddress, Granularity);

      if (!MmGetAddressSpaceOwner(AddressSpace) && *BaseAddress < MmSystemRangeStart)
      {
         return STATUS_ACCESS_VIOLATION;
      }

      if (MmGetAddressSpaceOwner(AddressSpace) &&
          (ULONG_PTR)(*BaseAddress) + tmpLength > (ULONG_PTR)MmSystemRangeStart)
      {
         return STATUS_ACCESS_VIOLATION;
      }

      if (BoundaryAddressMultiple.QuadPart != 0)
      {
         EndAddress = ((char*)(*BaseAddress)) + tmpLength-1;
         ASSERT(((ULONG_PTR)*BaseAddress/BoundaryAddressMultiple.QuadPart) == ((DWORD_PTR)EndAddress/BoundaryAddressMultiple.QuadPart));
      }

      if (MmLocateMemoryAreaByRegion(AddressSpace,
                                     *BaseAddress,
                                     tmpLength) != NULL)
      {
         DPRINT("Memory area already occupied\n");
         return STATUS_CONFLICTING_ADDRESSES;
      }
   }

   MemoryArea = ExAllocatePoolWithTag(NonPagedPool, sizeof(MEMORY_AREA),
                                      TAG_MAREA);
   RtlZeroMemory(MemoryArea, sizeof(MEMORY_AREA));
   MemoryArea->Type = Type;
   MemoryArea->StartingAddress = *BaseAddress;
   MemoryArea->EndingAddress = (PVOID)((ULONG_PTR)*BaseAddress + tmpLength);
   MemoryArea->Protect = Protect;
   MemoryArea->Flags = AllocationFlags;
   //MemoryArea->LockCount = 0;
   MemoryArea->PageOpCount = 0;
   MemoryArea->DeleteInProgress = FALSE;

   MmInsertMemoryArea(AddressSpace, MemoryArea);

   *Result = MemoryArea;

   DPRINT("MmCreateMemoryArea() succeeded (%p)\n", *BaseAddress);
   return STATUS_SUCCESS;
}

VOID NTAPI
MmMapMemoryArea(PVOID BaseAddress,
                ULONG Length,
                ULONG Consumer,
                ULONG Protection)
{
   ULONG i;
   NTSTATUS Status;

   for (i = 0; i < PAGE_ROUND_UP(Length) / PAGE_SIZE; i++)
   {
      PFN_TYPE Page;

      Status = MmRequestPageMemoryConsumer(Consumer, TRUE, &Page);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("Unable to allocate page\n");
         KeBugCheck(MEMORY_MANAGEMENT);
      }
      Status = MmCreateVirtualMapping (NULL,
                                       (PVOID)((ULONG_PTR)BaseAddress + (i * PAGE_SIZE)),
                                       Protection,
                                       &Page,
                                       1);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("Unable to create virtual mapping\n");
         KeBugCheck(MEMORY_MANAGEMENT);
      }
   }
}


VOID NTAPI
MmReleaseMemoryAreaIfDecommitted(PEPROCESS Process,
                                 PMM_AVL_TABLE AddressSpace,
                                 PVOID BaseAddress)
{
   PMEMORY_AREA MemoryArea;
   PLIST_ENTRY Entry;
   PMM_REGION Region;
   BOOLEAN Reserved;

   MmVerifyMemoryAreas(AddressSpace);

   MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, BaseAddress);
   if (MemoryArea != NULL)
   {
      Entry = MemoryArea->Data.VirtualMemoryData.RegionListHead.Flink;
      Reserved = TRUE;
      while (Reserved && Entry != &MemoryArea->Data.VirtualMemoryData.RegionListHead)
      {
         Region = CONTAINING_RECORD(Entry, MM_REGION, RegionListEntry);
         Reserved = (MEM_RESERVE == Region->Type);
         Entry = Entry->Flink;
      }

      if (Reserved)
      {
         MmFreeVirtualMemory(Process, MemoryArea);
      }
   }
}

/* EOF */
