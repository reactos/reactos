//      TITLE("Compare, Move, Zero, and Fill Memory Support")
//++
//
// Copyright (c) 1993  IBM Corporation
//
// Module Name:
//
//    mvmem.s
//
// Abstract:
//
//    This module implements functions to compare, move, zero, and fill
//    blocks of memory. If the memory is aligned, then these functions
//    are very efficient.
//
//    N.B. These routines MUST preserve all floating state since they are
//        frequently called from interrupt service routines that normally
//        do not save or restore floating state.
//
// Author:
//
//    Curt Fawcett (crf) 10-Aug-1993
//
// Environment:
//
//    User or Kernel mode.
//
// Revision History:
//
//    Curt Fawcett      11-Jan-1994     Removed register definitions
//                                      and fixed for new assembler
//
//--

#include <ksppc.h>

//
// Define local constants
//
        .set BLKLN,32
//
//--
//++
//
// ULONG
// RtlCompareMemory (
//    IN PVOID Source1,
//    IN PVOID Source2,
//    IN ULONG Length
//    )
//
// Routine Description:
//
//    This function compares two blocks of memory and returns the number
//    of bytes that compared equal.
//
// Arguments:
//
//    SRC1 (r.3) - Supplies a pointer to the first block of memory to
//                 compare.
//
//    SRC2 (r.4) - Supplies a pointer to the second block of memory to
//                 compare.
//
//    LNGTH (r.5) - Supplies the length, in bytes, of the memory to be
//                  compared.
//
// Return Value:
//
//    The number of bytes that compared equal is returned as the function
//    value. If all bytes compared equal, then the length of the orginal
//    block of memory is returned.
//
//--
//
// Define the routine entry point

        LEAF_ENTRY(RtlCompareMemory)
//
// Compare Memory
//
// Check alignment
//
        or.     r.6,r.5,r.5             // Check for zero length
        mr      r.12,r.5                // Save original length
        beq     GetResults2             // Jump if zero length
        cmpwi   r.5,4                   // Check for less than 4 bytes
        add     r.11,r.3,r.5            // Get ending SRC1 address
        xor     r.9,r.3,r.4             // Check for same alignment
        blt-    CompareByByte           // Jump if single byte compares
        andi.   r.9,r.9,3               // Isolate alignment bits
        bne-    CompUnaligned           // Jump if different alignments
//
// Compare Memory  - Same SRC1 and SRC2 alignment
//
// Compare extra bytes until a word boundary is reached
//

CompAligned:
        andi.   r.6,r.4,3               // Check alignment type
        beq+    CompBlkDiv              // Jump to process 32-Byte blocks
        cmpwi   r.6,3                   // Check for 1 byte unaligned
        lbz     r.7,0(r.3)              // Get unaligned byte
        lbz     r.8,0(r.4)              // Get unaligned byte
        bne+    Comp2                   // If not, check next case
        li      r.6,1                   // Set byte move count
        b       UpdateCompAddrs         // Jump to update addresses
Comp2:
        cmpwi   r.6,2                   // Check for halfword aligned
        li      r.6,2                   // Set byte move count
        bne+    Comp3                   // If not, check next case
        lhz     r.7,0(r.3)              // Get unaligned halfword
        lhz     r.8,0(r.4)              // Get unaligned halfword
        b       UpdateCompAddrs         // Jump to update addresses
Comp3:
        cmpw    r.7,r.8                 // Check for 1st word equal
        lhz     r.7,1(r.3)              // Get unaligned halfword
        lhz     r.8,1(r.4)              // Get unaligned halfword
        li      r.6,3                   // Set byte move count
        bne     Wrd1ne                  // Jump if 1st word not equal
UpdateCompAddrs:
        cmpw    r.7,r.8                 // Check for 1st word equal
        sub     r.5,r.5,r.6             // Decrement LNGTH by unaligned
        bne     Wrd1ne                  // Jump if 1st word not equal
        add     r.3,r.3,r.6             // Update the SRC1 address
        add     r.4,r.4,r.6             // Update the SRC2 address
//
// Divide the block to process into 32-byte blocks
//

CompBlkDiv:
        andi.   r.6,r.5,BLKLN-1         // Isolate remainder of LNGTH/32
        sub.    r.7,r.5,r.6             // Get full block count
        add     r.10,r.3,r.7            // Get address of last full block
        beq-    CompareBy4Bytes         // Jump if no full blocks
        mr      r.5,r.6                 // Set Length = remainder
//
// Compare 32-byte blocks
//
CompFullBlks:
        lwz     r.6,0(r.3)              // Get 1st SRC1 word
        lwz     r.7,0(r.4)              // Get 1st SRC2 word
        lwz     r.8,4(r.3)              // Get 2nd SRC1 word
        cmpw    r.6,r.7                 // Check for 1st word equal
        lwz     r.9,4(r.4)              // Get 2nd SRC2 word
        bne-    Wrd1ne                  // Jump if 1st word not equal
        cmpw    r.8,r.9                 // Check for 2nd word equal
        lwz     r.6,8(r.3)              // Get 3rd SRC1 word
        lwz     r.7,8(r.4)              // Get 3rd SRC2 word
        bne-    Wrd2ne                  // Jump if 2nd word not equal
        cmpw    r.6,r.7                 // Check for 3rd word equal
        lwz     r.8,12(r.3)             // Get 4th SRC1 word
        lwz     r.9,12(r.4)             // Get 4th SRC2 word
        bne-    Wrd3ne                  // Jump if 3rd word not equal
        cmpw    r.8,r.9                 // Check for 4th word equal
        lwz     r.6,16(r.3)             // Get 5th SRC1 word
        lwz     r.7,16(r.4)             // Get 5th SRC2 word
        bne-    Wrd4ne                  // Jump if 4th word not equal
        cmpw    r.6,r.7                 // Check for 5th word equal
        lwz     r.8,20(r.3)             // Get 6th SRC1 word
        lwz     r.9,20(r.4)             // Get 6th SRC2 word
        bne-    Wrd5ne                  // Jump if 5th word not equal
        cmpw    r.8,r.9                 // Check for 6th word equal
        lwz     r.6,24(r.3)             // Get 7th SRC1 word
        lwz     r.7,24(r.4)             // Get 7th SRC2 word
        bne-    Wrd6ne                  // Jump if 6th word not equal
        cmpw    r.6,r.7                 // Check for 7th word equal
        lwz     r.8,28(r.3)             // Get 8th SRC1 word
        lwz     r.9,28(r.4)             // Get 8th SRC2 word
        bne-    Wrd7ne                  // Jump if 7th word not equal
        cmpw    r.8,r.9                 // Check for 8th word equal
        bne-    Wrd8ne                  // Jump if 8th word not equal
        addi    r.3,r.3,32              // Update SRC1 pointer
        cmpw    r.3,r.10                // Check for all blocks done
        addi    r.4,r.4,32              // Update SRC2 pointer
        bne+    CompFullBlks            // Jump if more blocks
//
// Compare 4-byte blocks
//

CompareBy4Bytes:
        andi.   r.6,r.5,4-1             // Isolate remainder of LNGTH/4
        sub.    r.7,r.5,r.6             // Get 4-byte block count
        add     r.10,r.3,r.7            // Get address of last full block
        beq-    CompareByByte           // Jump if no 4-byte blocks
        mr      r.5,r.6                 // Set Length = remainder

CompLpOn4Bytes:
        lwz     r.6,0(r.3)              // Get 1st SRC1 word
        lwz     r.7,0(r.4)              // Get 1st SRC2 word
        cmpw    r.6,r.7                 // Check for 1st word equal
        bne-    Wrd1ne                  // Jump if 1st word not equal
        addi    r.3,r.3,4               // Get pointer to next SRC1 block
        cmpw    r.3,r.10                // Check for last block
        addi    r.4,r.4,4               // Get pointer to next SRC2 block
        bne+    CompLpOn4Bytes          // Jump if more blocks
//
// Compare 1-byte blocks
//

CompareByByte:
        cmpwi   r.5,0                   // Check for no bytes left
        beq+    GetResults              // Jump to return if done
        lbz     r.6,0(r.3)              // Get 1st SRC1 byte
        lbz     r.7,0(r.4)              // Load 1st SRC2 byte
        cmpw    r.6,r.7                 // Check for 1st word equal
        bne-    Wrd1ne                  // Jump if 1st word not equal
        addi    r.3,r.3,1               // Update SRC1 address
        cmpwi   r.5,1                   // Check for no bytes left
        addi    r.4,r.4,1               // Update SRC2 address
        beq+    GetResults              // Jump to return if done
        lbz     r.6,0(r.3)              // Get 2nd SRC1 byte
        lbz     r.7,0(r.4)              // Load 2nd SRC2 byte
        cmpw    r.6,r.7                 // Check for 1st word equal
        bne-    Wrd1ne                  // Jump if 1st word not equal
        cmpwi   r.5,2                   // Check for no bytes left
        addi    r.4,r.4,1               // Update SRC2 address
        addi    r.3,r.3,1               // Update SRC1 address
        beq+    GetResults              // Jump to return if done
        lbz     r.6,0(r.3)              // Get 3rd SRC1 byte
        lbz     r.7,0(r.4)              // Load 3rd SRC2 byte
        cmpw    r.6,r.7                 // Check for 1st word equal
        bne-    Wrd1ne                  // Jump if 1st word not equal

        addi    r.4,r.4,1               // Update SRC2 address
        addi    r.3,r.3,1               // Update SRC1 address
        b       GetResults              // Jump to return
//
//  Compare - SRC1 and SRC2 have different alignments
//

CompUnaligned:
        or      r.9,r.3,r.4                 // Check if either byte unaligned
        andi.   r.9,r.9,3                   // Isolate alignment
        cmpwi   r.9,2                       // Check for even result
        bne+    CompByteUnaligned       // Jump for byte unaligned
//
// Divide the blocks to process into 32-byte blocks
//

CompBlkDivUnaligned:
        andi.   r.6,r.5,BLKLN-1             // Isolate remainder of LNGTH/32
        sub.    r.7,r.5,r.6                 // Get full block count
        add     r.10,r.3,r.7                // Get address of last full block
        beq-    CompHWrdBy4Bytes        // Jump if no full blocks
        mr      r.5,r.6                     // Set Length = remainder
//
//  Compare - SRC1 or SRC2 is halfword aligned, the other is by word
//

CompByHWord:
        lhz     r.6,0(r.3)              // Get 1st hword of 1st SRC1 wrd
        lhz     r.7,0(r.4)              // Get 1st hword of 1st SRC2 wrd
        lhz     r.8,2(r.3)              // Get 2nd hword of 1st SRC1 wrd
        cmpw    r.6,r.7                 // Check for 1st word equal
        lhz     r.9,2(r.4)              // Get 2nd hword of 1st SRC2 wrd
        bne-    Wrd1ne                  // Check for 1st word equal
        cmpw    r.8,r.9                 // Check for 1st word equal
        lhz     r.6,4(r.3)              // Get 1st hword of 2nd SRC1 wrd
        lhz     r.7,4(r.4)              // Get 1st hword of 2nd SRC2 wrd
        bne-    Wrd1ne                  // Check for 1st word equal
        cmpw    r.6,r.7                 // Check for 2nd word equal
        lhz     r.8,6(r.3)              // Get 2nd hword of 2nd SRC1 wrd
        lhz     r.9,6(r.4)              // Get 2nd hword of 2nd SRC2 wrd
        bne-    Wrd2ne                  // Check for 2nd word equal
        cmpw    r.8,r.9                 // Check for 2nd word equal
        lhz     r.6,8(r.3)              // Get 1st hword of 3rd SRC1 wrd
        lhz     r.7,8(r.4)              // Get 1st hword of 3rd SRC2 wrd
        bne-    Wrd2ne                  // Check for 2nd word equal
        cmpw    r.6,r.7                 // Check for 3rd word equal
        lhz     r.8,10(r.3)             // Get 2nd hword of 3rd SRC1 wrd
        lhz     r.9,10(r.4)             // Get 2nd hword of 3rd SRC2 wrd
        bne-    Wrd3ne                  // Check for 3rd word equal
        cmpw    r.8,r.9                 // Check for 3rd word equal
        lhz     r.6,12(r.3)             // Get 1st hword of 4th SRC1 wrd
        lhz     r.7,12(r.4)             // Get 1st hword of 4th SRC2 wrd
        bne-    Wrd3ne                  // Check for 3rd word equal
        cmpw    r.6,r.7                 // Check for 4th word equal
        lhz     r.8,14(r.3)             // Get 2nd hword of 4th SRC1 wrd
        lhz     r.9,14(r.4)             // Get 2nd hword of 4th SRC2 wrd
        bne-    Wrd4ne                  // Check for 4th word equal
        cmpw    r.8,r.9                 // Check for 4th word equal
        lhz     r.6,16(r.3)             // Get 1st hword of 5th SRC1 wrd
        lhz     r.7,16(r.4)             // Get 1st hword of 5th SRC2 wrd
        bne-    Wrd4ne                  // Check for 4th word equal
        cmpw    r.6,r.7                 // Check for 5th word equal
        lhz     r.8,18(r.3)             // Get 2nd hword of 5th SRC1 wrd
        lhz     r.9,18(r.4)             // Get 2nd hword of 5th SRC2 wrd
        bne-    Wrd5ne                  // Check for 5th word equal
        cmpw    r.8,r.9                 // Check for 5th word equal
        lhz     r.6,20(r.3)             // Get 1st hword of 6th SRC1 wrd
        lhz     r.7,20(r.4)             // Get 1st hword of 6th SRC2 wrd
        bne-    Wrd5ne                  // Check for 5th word equal
        cmpw    r.6,r.7                 // Check for 6th word equal
        lhz     r.8,22(r.3)             // Get 2nd hword of 6th SRC1 wrd
        lhz     r.9,22(r.4)             // Get 2nd hword of 6th SRC2 wrd
        bne-    Wrd6ne                  // Check for 6th word equal
        cmpw    r.8,r.9                 // Check for 6th word equal
        lhz     r.6,24(r.3)             // Get 1st hword of 7th SRC1 wrd
        lhz     r.7,24(r.4)             // Get 1st hword of 7th SRC2 wrd
        bne-    Wrd6ne                  // Check for 6th word equal
        cmpw    r.6,r.7                 // Check for 7th word equal
        lhz     r.8,26(r.3)             // Get 2nd hword of 7th SRC1 wrd
        lhz     r.9,26(r.4)             // Get 2nd hword of 7th SRC2 wrd
        bne-    Wrd7ne                  // Check for 7th word equal
        cmpw    r.8,r.9                 // Check for 7th word equal
        lhz     r.6,28(r.3)             // Get 1st hword of 8th SRC1 wrd
        lhz     r.7,28(r.4)             // Get 1st hword of 8th SRC2 wrd
        bne-    Wrd7ne                  // Check for 7th word equal
        cmpw    r.6,r.7                 // Check for 8th word equal
        lhz     r.8,30(r.3)             // Get 2nd hword of 8th SRC1 wrd
        lhz     r.9,30(r.4)             // Get 2nd hword of 8th SRC2 wrd
        bne-    Wrd8ne                  // Check for 8th word equal
        cmpw    r.8,r.9                 // Check for 8th word equal
        bne-    Wrd8ne                  // Check for 8th word equal
        addi    r.3,r.3,32              // Update SRC1 pointer
        cmpw    r.3,r.10                // Check for all blocks done
        addi    r.4,r.4,32              // Update SRC2 pointer
        bne+    CompByHWord             // Jump if more blocks
//
// Compare 4-byte blocks with SRC2 Halfword unaligned
//

CompHWrdBy4Bytes:
        andi.   r.6,r.5,4-1             // Isolate remainder of LNGTH/4
        sub.    r.7,r.5,r.6             // Get 4-byte block count
        add     r.10,r.3,r.7            // Get address of last full block
        beq-    CompareByByte           // Jump if no 4-byte blocks
        mr      r.5,r.6                 // Set Length = remainder

CompHWrdLpOn4Bytes:
        lhz     r.6,0(r.3)              // Get 1st hword of 1st SRC1 wrd
        lhz     r.7,0(r.4)              // Get 1st hword of 1st SRC2 wrd
        lhz     r.8,2(r.3)              // Get 2nd hword of 1st SRC1 wrd
        cmpw    r.6,r.7                 // Check for 1st word equal
        lhz     r.9,2(r.4)              // Get 2nd hword of 1st SRC2 wrd
        bne-    Wrd1ne                  // Check for 1st word equal
        cmpw    r.8,r.9                 // Check for 1st word equal
        bne-    Wrd1ne                  // Check for 1st word equal
        addi    r.3,r.3,4               // Update SRC1 pointer
        cmpw    r.3,r.10                // Check for last block
        addi    r.4,r.4,4               // Update SRC2 pointer
        bne+    CompHWrdLpOn4Bytes      // Jump if more blocks

        b       CompareByByte           // Jump to complete last bytes
//
//  Compare - Byte unaligned
//

CompByteUnaligned:
        and     r.9,r.3,r.4                 // Check for both byte aligned
        andi.   r.9,r.9,1                   // Isolate alignment bits
        beq-    CmpBlksByByte           // Jump if both not byte aligned
//
// Divide the blocks to process into 32-byte blocks
//
        andi.   r.6,r.5,BLKLN-1         // Isolate remainder of LNGTH/32
        sub.    r.7,r.5,r.6             // Get full block count
        add     r.10,r.3,r.7            // Get address of last full block
        beq-    CompByteBy4Bytes        // Jump if no full blocks
        mr      r.5,r.6                 // Set Length = remainder
//
//  Compare - SRC1 and SRC2 are byte unaligned differently
//

CompByByte:
        lbz     r.6,0(r.3)              // Get first byte of 1st SRC1 wrd
        lbz     r.7,0(r.4)              // Get first byte of 1st SRC2 wrd
        lhz     r.8,1(r.3)              // Get mid-h-word of 1st SRC1 wrd
        cmpw    r.6,r.7                 // Check for 1st word equal
        lhz     r.9,1(r.4)              // Get mid-h-word of 1st SRC2 wrd
        bne     Wrd1ne                  // Jump if 1st word not equal
        cmpw    r.8,r.9                 // Check for 1st word equal
        lhz     r.6,3(r.3)              // Get h-word crossing 1st/2nd SRC1 wrd
        lhz     r.7,3(r.4)              // Get h-word crossing 1st/2nd SRC2 wrd
        bne     Wrd1ne                  // Jump if 1st word not equal
        cmpw    r.6,r.7                 // Check for 1st word equal
        lhz     r.8,5(r.3)              // Get mid-h-word of 2nd SRC1 wrd
        lhz     r.9,5(r.4)              // Get mid-h-word of 2nd SRC2 wrd
        bne     Wrd1ne                  // Jump if 2nd word not equal
        cmpw    r.8,r.9                 // Check for 2nd word equal
        lhz     r.6,7(r.3)              // Get h-word crossing 2nd/3rd SRC1 wrd
        lhz     r.7,7(r.4)              // Get h-word crossing 2nd/3rd SRC2 wrd
        bne     Wrd2ne                  // Jump if 2nd word not equal
        cmpw    r.6,r.7                 // Check for 2nd word equal
        lhz     r.8,9(r.3)              // Get mid-h-word of 3rd SRC1 wrd
        lhz     r.9,9(r.4)              // Get mid-h-word of 3rd SRC2 wrd
        bne     Wrd2ne                  // Jump if 3rd word not equal
        cmpw    r.8,r.9                 // Check for 3rd word equal
        lhz     r.6,11(r.3)             // Get h-word crossing 3rd/4th SRC1 wrd
        lhz     r.7,11(r.4)             // Get h-word crossing 3rd/4th SRC2 wrd
        bne     Wrd3ne                  // Jump if 3rd word not equal
        cmpw    r.6,r.7                 // Check for 3rd word equal
        lhz     r.8,13(r.3)             // Get mid-h-word of 4th SRC1 wrd
        lhz     r.9,13(r.4)             // Get mid-h-word of 4th SRC2 wrd
        bne     Wrd3ne                  // Jump if 4th word not equal
        cmpw    r.8,r.9                 // Check for 4th word equal
        lhz     r.6,15(r.3)             // Get h-word crossing 4th/5th SRC1 wrd
        lhz     r.7,15(r.4)             // Get h-word crossing 4th/5th SRC2 wrd
        bne     Wrd4ne                  // Jump if 4th word not equal
        cmpw    r.6,r.7                 // Check for 4th word equal
        lhz     r.8,17(r.3)             // Get mid-h-word of 5th SRC1 wrd
        lhz     r.9,17(r.4)             // Get mid-h-word of 5th SRC2 wrd
        bne     Wrd4ne                  // Jump if 5th word not equal
        cmpw    r.8,r.9                 // Check for 5th word equal
        lhz     r.6,19(r.3)             // Get h-word crossing 5th/6th SRC1 wrd
        lhz     r.7,19(r.4)             // Get h-word crossing 5th/6th SRC2 wrd
        bne     Wrd5ne                  // Jump if 5th word not equal
        cmpw    r.6,r.7                 // Check for 5th word equal
        lhz     r.8,21(r.3)             // Get mid-h-word of 6th SRC1 wrd
        lhz     r.9,21(r.4)             // Get mid-h-word of 6th SRC2 wrd
        bne     Wrd5ne                  // Jump if 6th word not equal
        cmpw    r.8,r.9                 // Check for 6th word equal
        lhz     r.6,23(r.3)             // Get h-word crossing 6th/7th SRC1 wrd
        lhz     r.7,23(r.4)             // Get h-word crossing 6th/7th SRC2 wrd
        bne     Wrd6ne                  // Jump if 6th word not equal
        cmpw    r.6,r.7                 // Check for 6th word equal
        lhz     r.8,25(r.3)             // Get mid-h-word of 7th SRC1 wrd
        lhz     r.9,25(r.4)             // Get mid-h-word of 7th SRC2 wrd
        bne     Wrd6ne                  // Jump if 7th word not equal
        cmpw    r.8,r.9                 // Check for 7th word equal
        lhz     r.6,27(r.3)             // Get h-word crossing 7th/8th SRC1 wrd
        lhz     r.7,27(r.4)             // Get h-word crossing 7th/8th SRC2 wrd
        bne     Wrd7ne                  // Jump if 7th word not equal
        cmpw    r.6,r.7                 // Check for 7th word equal
        lhz     r.8,29(r.3)             // Get mid-h-word of 8th SRC1 wrd
        lhz     r.9,29(r.4)             // Get mid-h-word of 8th SRC2 wrd
        bne     Wrd7ne                  // Jump if 8th word not equal
        cmpw    r.8,r.9                 // Check for 8th word equal
        lbz     r.6,31(r.3)             // Get last byte of 8th SRC1 wrd
        lbz     r.7,31(r.4)             // Get last byte of 8th SRC2 wrd
        bne     Wrd8ne                  // Jump if 8th word not equal
        cmpw    r.6,r.7                 // Check for 8th word equal
        bne     Wrd8ne                  // Jump if 8th word not equal
        addi    r.3,r.3,32              // Update SRC1 pointer
        cmpw    r.3,r.10                // Check for all blocks done
        addi    r.4,r.4,32              // Update SRC2 pointer
        bne+    CompByByte              // Jump if more blocks
//
// Compare 4-byte blocks with SRC2 or SRC1 Byte aligned
//

CompByteBy4Bytes:
        andi.   r.6,r.5,4-1             // Isolate remainder of LNGTH/4
        sub.    r.7,r.5,r.6             // Get 4-byte block count
        add     r.10,r.3,r.7            // Get address of last full block
        beq-    CompareByByte           // Jump if no 4-byte blocks
        mr      r.5,r.6                 // Set Length = remainder

CompByteLpOn4Bytes:
        lbz     r.6,0(r.3)              // Get first byte of 1st SRC1 wrd
        lbz     r.7,0(r.4)              // Get first byte of 1st SRC2 wrd
        lhz     r.8,1(r.3)              // Get mid-h-word of 1st SRC1 wrd
        cmpw    r.6,r.7                 // Check for 1st word equal
        lhz     r.9,1(r.4)              // Get mid-h-word of 1st SRC2 wrd
        bne     Wrd1ne                  // Jump if 1st word not equal
        cmpw    r.8,r.9                 // Check for 1st word equal
        lbz     r.6,3(r.3)              // Get last byte of 1st SRC1 wrd
        lbz     r.7,3(r.4)              // Get last byte of 1st SRC2 wrd
        bne     Wrd1ne                  // Jump if 1st word not equal
        cmpw    r.6,r.7                 // Check for 1st word equal
        bne     Wrd1ne                  // Jump if 1st word not equal
        addi    r.3,r.3,4               // Update SRC1 pointer
        cmpw    r.3,r.10                // Check for last block
        addi    r.4,r.4,4               // Update SRC2 pointer
        bne+    CompByteLpOn4Bytes      // Jump if more blocks

        b       CompareByByte           // Jump to complete last bytes
//
//  Compare - Either SRC1 or SRC2 is byte unaligned but not both
//
// Divide the blocks to process into 32-byte blocks
//

CmpBlksByByte:
        andi.   r.6,r.5,BLKLN-1         // Isolate remainder of LNGTH/32
        sub.    r.7,r.5,r.6             // Get full block count
        add     r.10,r.3,r.7            // Get address of last full block
        beq-    CompBlksOf4Bytes        // Jump if no full blocks
        mr      r.5,r.6                 // Set Length = remainder

CompBlksByByte:
        lbz     r.6,0(r.3)              // Get first byte of 1st SRC1 wrd
        lbz     r.7,0(r.4)              // Get first byte of 1st SRC2 wrd
        lbz     r.8,1(r.3)              // Get 2nd byte of 1st SRC1 wrd
        cmpw    r.6,r.7                 // Check for 1st word equal
        lbz     r.9,1(r.4)              // Get 2nd byte of 1st SRC2 wrd
        bne     Wrd1ne                  // Jump if 1st word not equal
        cmpw    r.8,r.9                 // Check for 1st word equal
        lbz     r.6,2(r.3)              // Get first byte of 1st SRC1 wrd
        lbz     r.7,2(r.4)              // Get first byte of 1st SRC2 wrd
        bne     Wrd1ne                  // Jump if 1st word not equal
        cmpw    r.6,r.7                 // Check for 1st word equal
        lbz     r.8,3(r.3)              // Get 2nd byte of 1st SRC1 wrd
        lbz     r.9,3(r.4)              // Get 2nd byte of 1st SRC2 wrd
        bne     Wrd1ne                  // Jump if 1st word not equal
        cmpw    r.8,r.9                 // Check for 1st word equal
        lbz     r.6,4(r.3)              // Get first byte of 2nd SRC1 wrd
        lbz     r.7,4(r.4)              // Get first byte of 2nd SRC2 wrd
        bne     Wrd1ne                  // Jump if 1st word not equal
        cmpw    r.6,r.7                 // Check for 2nd word equal
        lbz     r.8,5(r.3)              // Get 2nd byte of 2nd SRC1 wrd
        lbz     r.9,5(r.4)              // Get 2nd byte of 2nd SRC2 wrd
        bne     Wrd2ne                  // Jump if 2nd word not equal
        cmpw    r.8,r.9                 // Check for 2nd word equal
        lbz     r.6,6(r.3)              // Get first byte of 2nd SRC1 wrd
        lbz     r.7,6(r.4)              // Get first byte of 2nd SRC2 wrd
        bne     Wrd2ne                  // Jump if 2nd word not equal
        cmpw    r.6,r.7                 // Check for 2nd word equal
        lbz     r.8,7(r.3)              // Get 2nd byte of 2nd SRC1 wrd
        lbz     r.9,7(r.4)              // Get 2nd byte of 2nd SRC2 wrd
        bne     Wrd2ne                  // Jump if 2nd word not equal
        cmpw    r.8,r.9                 // Check for 2nd word equal
        lbz     r.6,8(r.3)              // Get first byte of 3rd SRC1 wrd
        lbz     r.7,8(r.4)              // Get first byte of 3rd SRC2 wrd
        bne     Wrd2ne                  // Jump if 2nd word not equal
        cmpw    r.6,r.7                 // Check for 3rd word equal
        lbz     r.8,9(r.3)              // Get 2nd byte of 3rd SRC1 wrd
        lbz     r.9,9(r.4)              // Get 2nd byte of 3rd SRC2 wrd
        bne     Wrd3ne                  // Jump if 3rd word not equal
        cmpw    r.8,r.9                 // Check for 3rd word equal
        lbz     r.6,10(r.3)             // Get first byte of 3rd SRC1 wrd
        lbz     r.7,10(r.4)             // Get first byte of 3rd SRC2 wrd
        bne     Wrd3ne                  // Jump if 3rd word not equal
        cmpw    r.6,r.7                 // Check for 3rd word equal
        lbz     r.8,11(r.3)             // Get 2nd byte of 3rd SRC1 wrd
        lbz     r.9,11(r.4)             // Get 2nd byte of 3rd SRC2 wrd
        bne     Wrd3ne                  // Jump if 3rd word not equal
        cmpw    r.8,r.9                 // Check for 3rd word equal
        lbz     r.6,12(r.3)             // Get first byte of 4th SRC1 wrd
        lbz     r.7,12(r.4)             // Get first byte of 4th SRC2 wrd
        bne     Wrd3ne                  // Jump if 3rd word not equal
        cmpw    r.6,r.7                 // Check for 4th word equal
        lbz     r.8,13(r.3)             // Get 2nd byte of 4th SRC1 wrd
        lbz     r.9,13(r.4)             // Get 2nd byte of 4th SRC2 wrd
        bne     Wrd4ne                  // Jump if 4th word not equal
        cmpw    r.8,r.9                 // Check for 4th word equal
        lbz     r.6,14(r.3)             // Get first byte of 4th SRC1 wrd
        lbz     r.7,14(r.4)             // Get first byte of 4th SRC2 wrd
        bne     Wrd4ne                  // Jump if 4th word not equal
        cmpw    r.6,r.7                 // Check for 4th word equal
        lbz     r.8,15(r.3)             // Get 2nd byte of 4th SRC1 wrd
        lbz     r.9,15(r.4)             // Get 2nd byte of 4th SRC2 wrd
        bne     Wrd4ne                  // Jump if 4th word not equal
        cmpw    r.8,r.9                 // Check for 4th word equal
        lbz     r.6,16(r.3)             // Get first byte of 5th SRC1 wrd
        lbz     r.7,16(r.4)             // Get first byte of 5th SRC2 wrd
        bne     Wrd4ne                  // Jump if 4th word not equal
        cmpw    r.6,r.7                 // Check for 5th word equal
        lbz     r.8,17(r.3)             // Get 2nd byte of 5th SRC1 wrd
        lbz     r.9,17(r.4)             // Get 2nd byte of 5th SRC2 wrd
        bne     Wrd5ne                  // Jump if 5th word not equal
        cmpw    r.8,r.9                 // Check for 5th word equal
        lbz     r.6,18(r.3)             // Get first byte of 5th SRC1 wrd
        lbz     r.7,18(r.4)             // Get first byte of 5th SRC2 wrd
        bne     Wrd5ne                  // Jump if 5th word not equal
        cmpw    r.6,r.7                 // Check for 5th word equal
        lbz     r.8,19(r.3)             // Get 2nd byte of 5th SRC1 wrd
        lbz     r.9,19(r.4)             // Get 2nd byte of 5th SRC2 wrd
        bne     Wrd5ne                  // Jump if 5th word not equal
        cmpw    r.8,r.9                 // Check for 5th word equal
        lbz     r.6,20(r.3)             // Get first byte of 6th SRC1 wrd
        lbz     r.7,20(r.4)             // Get first byte of 6th SRC2 wrd
        bne     Wrd5ne                  // Jump if 5th word not equal
        cmpw    r.6,r.7                 // Check for 6th word equal
        lbz     r.8,21(r.3)             // Get 2nd byte of 6th SRC1 wrd
        lbz     r.9,21(r.4)             // Get 2nd byte of 6th SRC2 wrd
        bne     Wrd6ne                  // Jump if 6th word not equal
        cmpw    r.8,r.9                 // Check for 6th word equal
        lbz     r.6,22(r.3)             // Get first byte of 6th SRC1 wrd
        lbz     r.7,22(r.4)             // Get first byte of 6th SRC2 wrd
        bne     Wrd6ne                  // Jump if 6th word not equal
        cmpw    r.6,r.7                 // Check for 6th word equal
        lbz     r.8,23(r.3)             // Get 2nd byte of 6th SRC1 wrd
        lbz     r.9,23(r.4)             // Get 2nd byte of 6th SRC2 wrd
        bne     Wrd6ne                  // Jump if 6th word not equal
        cmpw    r.8,r.9                 // Check for 6th word equal
        lbz     r.6,24(r.3)             // Get first byte of 7th SRC1 wrd
        lbz     r.7,24(r.4)             // Get first byte of 7th SRC2 wrd
        bne     Wrd6ne                  // Jump if 6th word not equal
        cmpw    r.6,r.7                 // Check for 7th word equal
        lbz     r.8,25(r.3)             // Get 2nd byte of 7th SRC1 wrd
        lbz     r.9,25(r.4)             // Get 2nd byte of 7th SRC2 wrd
        bne     Wrd7ne                  // Jump if 7th word not equal
        cmpw    r.8,r.9                 // Check for 7th word equal
        lbz     r.6,26(r.3)             // Get first byte of 7th SRC1 wrd
        lbz     r.7,26(r.4)             // Get first byte of 7th SRC2 wrd
        bne     Wrd7ne                  // Jump if 7th word not equal
        cmpw    r.6,r.7                 // Check for 7th word equal
        lbz     r.8,27(r.3)             // Get 2nd byte of 7th SRC1 wrd
        lbz     r.9,27(r.4)             // Get 2nd byte of 7th SRC2 wrd
        bne     Wrd7ne                  // Jump if 7th word not equal
        cmpw    r.8,r.9                 // Check for 7th word equal
        lbz     r.6,28(r.3)             // Get first byte of 8th SRC1 wrd
        lbz     r.7,28(r.4)             // Get first byte of 8th SRC2 wrd
        bne     Wrd7ne                  // Jump if 7th word not equal
        cmpw    r.6,r.7                 // Check for 8th word equal
        lbz     r.8,29(r.3)             // Get 2nd byte of 8th SRC1 wrd
        lbz     r.9,29(r.4)             // Get ond byte of 8th SRC2 wrd
        bne     Wrd8ne                  // Jump if 8th word not equal
        cmpw    r.8,r.9                 // Check for 8th word equal
        lbz     r.6,30(r.3)             // Get first byte of 8th SRC1 wrd
        lbz     r.7,30(r.4)             // Get first byte of 8th SRC2 wrd
        bne     Wrd8ne                  // Jump if 8th word not equal
        cmpw    r.6,r.7                 // Check for 8th word equal
        lbz     r.8,31(r.3)             // Get 2nd byte of 8th SRC1 wrd
        lbz     r.9,31(r.4)             // Get 2nd byte of 8th SRC2 wrd
        bne     Wrd8ne                  // Jump if 8th word not equal
        cmpw    r.8,r.9                 // Check for 8th word equal
        bne     Wrd8ne                  // Jump if 8th word not equal
        addi    r.3,r.3,32              // Update SRC1 pointer
        cmpw    r.3,r.10                // Check for all blocks done
        addi    r.4,r.4,32              // Update SRC2 pointer
        bne+    CompBlksByByte          // Jump if more blocks
//
// Divide the blocks to process into 32-byte blocks
//
CompBlksOf4Bytes:
        andi.   r.6,r.5,4-1             // Isolate remainder of LNGTH/4
        sub.    r.7,r.5,r.6             // Get 4-byte block count
        add     r.10,r.3,r.7            // Get address of last full block
        beq-    CompareByByte           // Jump if no 4-byte blocks
        mr      r.5,r.6                 // Set Length = remainder

CompBlksLpOn4Bytes:
        lbz     r.6,0(r.3)              // Get first byte of 1st SRC1 wrd
        lbz     r.7,0(r.4)              // Get first byte of 1st SRC2 wrd
        lbz     r.8,1(r.3)              // Get 2nd byte of 1st SRC1 wrd
        cmpw    r.6,r.7                 // Check for 1st word equal
        lbz     r.9,1(r.4)              // Get 2nd byte of 1st SRC2 wrd
        bne     Wrd1ne                  // Jump if 1st word not equal
        cmpw    r.8,r.9                 // Check for 1st word equal
        lbz     r.6,2(r.3)              // Get first byte of 1st SRC1 wrd
        lbz     r.7,2(r.4)              // Get first byte of 1st SRC2 wrd
        bne     Wrd1ne                  // Jump if 1st word not equal
        cmpw    r.6,r.7                 // Check for 1st word equal
        lbz     r.8,3(r.3)              // Get 2nd byte of 1st SRC1 wrd
        lbz     r.9,3(r.4)              // Get 2nd byte of 1st SRC2 wrd
        bne     Wrd1ne                  // Jump if 1st word not equal
        cmpw    r.8,r.9                 // Check for 1st word equal
        bne     Wrd1ne                  // Jump if 1st word not equal
        addi    r.3,r.3,4               // Update SRC1 pointer
        cmpw    r.3,r.10                // Check for last block
        addi    r.4,r.4,4               // Update SRC2 pointer
        bne+    CompBlksLpOn4Bytes      // Jump if more blocks

        b       CompareByByte           // Jump to complete last bytes
//
// Adjust pointers to SRC1 and SRC2 to isolate offending byte compare
//
Wrd2ne:
        addi    r.3,r.3,4               // Adjust to point to 2nd word
        addi    r.4,r.4,4               // Adjust to point to 2nd word
        b       Compare1byte            // Jump to isolate the bad byte
Wrd3ne:
        addi    r.3,r.3,8               // Adjust to point to 3rd word
        addi    r.4,r.4,8               // Adjust to point to 3rd word
        b       Compare1byte            // Jump to isolate the bad byte
Wrd4ne:
        addi    r.3,r.3,12              // Adjust to point to 4th word
        addi    r.4,r.4,12              // Adjust to point to 4th word
        b       Compare1byte            // Jump to isolate the bad byte
Wrd5ne:
        addi    r.3,r.3,16              // Adjust to point to 5th word
        addi    r.4,r.4,16              // Adjust to point to 5th word
        b       Compare1byte            // Jump to isolate the bad byte
Wrd6ne:
        addi    r.3,r.3,20              // Adjust to point to 6th word
        addi    r.4,r.4,20              // Adjust to point to 6th word
        b       Compare1byte            // Jump to isolate the bad byte
Wrd7ne:
        addi    r.3,r.3,24              // Adjust to point to 7th word
        addi    r.4,r.4,24              // Adjust to point to 7th word
        b       Compare1byte            // Jump to isolate the bad byte
Wrd8ne:
        addi    r.3,r.3,28              // Adjust to point to 8th word
        addi    r.4,r.4,28              // Adjust to point to 8th word
Wrd1ne:
Compare1byte:
        sub     r.5,r.11,r.3            // Calculate remaining byte count
        add     r.8,r.3,r.5             // Get new ending address
        cmpwi   r.5,0                   // Check for no block to compare
        beq-    GetResults              // Jump if processing completed
SingleByte:
        lbz     r.6,0(r.3)              // Get next SRC1 byte
        lbz     r.7,0(r.4)              // Get next SRC2 byte
        addi    r.4,r.4,1               // Update SRC2 to next byte
        cmpw    r.6,r.7                 // Check for unequal bytes
        bne-    GetResults              // Jump if bytes aren't equal
        addi    r.3,r.3,1               // Update SRC1 to next byte
        cmpw    r.3,r.8                 // Check for being done
        bne+    SingleByte              // Jump if more bytes
//
// Compute the results
//
GetResults:
        sub     r.6,r.11,r.3            // Get no. of bytes not compared
GetResults2:
        sub     r.3,r.12,r.6            // Get no. of bytes that match
//
// Exit the routine
//
        LEAF_EXIT(RtlCompareMemory)

//++
//
// ULONG
// RtlEqualMemory (
//    IN PVOID Source1,
//    IN PVOID Source2,
//    IN ULONG Length
//    )
//
// Routine Description:
//
//    This function compares two blocks of memory for equality.
//
// Arguments:
//
//    Source1 (r.3) - Supplies a pointer to the first block of memory to
//       compare.
//
//    Source2 (r.4) - Supplies a pointer to the second block of memory to
//       compare.
//
//    Length (r.5) - Supplies the length, in bytes, of the memory to be
//       compared.
//
// Return Value:
//
//    If all bytes in the source strings match, then a value of TRUE is
//    returned. Otherwise, FALSE is returned.
//
//--

        LEAF_ENTRY(RtlEqualMemory)

//
// Check alignment
//

        clrlwi  r.12,r.5,28             // isolate residual bytes (Length & 15)
        or      r.9,r.3,r.4             // merge alignment bits
        sub.    r.11,r.5,r.12           // subtract out residual bytes
        add     r.10,r.3,r.5            // get ending Source1 address
        beq+    EqualByByte             // if eq, no 16-byte block to compare
        andi.   r.9,r.9,3               // isolate alignment bits
        add     r.5,r.3,r.11            // compute ending block address
        bne-    EqualUnaligned          // if ne, different alignments

EqualAligned:

//
// Both blocks are word-aligned, and there are at least 16 bytes to compare.
//

        lwz     r.6,0(r.3)              // Get 1st Source1 word
        lwz     r.7,0(r.4)              // Get 1st Source2 word
        lwz     r.8,4(r.3)              // Get 2nd Source1 word
        cmpw    r.6,r.7                 // Check for 1st word equal
        lwz     r.9,4(r.4)              // Get 2nd Source2 word
        bne-    EqualNotEqual           // Jump if 1st word not equal
        cmpw    r.8,r.9                 // Check for 2nd word equal
        lwz     r.6,8(r.3)              // Get 3rd Source1 word
        lwz     r.7,8(r.4)              // Get 3rd Source2 word
        bne-    EqualNotEqual           // Jump if 2nd word not equal
        cmpw    r.6,r.7                 // Check for 3rd word equal
        lwz     r.8,12(r.3)             // Get 4th Source1 word
        lwz     r.9,12(r.4)             // Get 4th Source2 word
        bne-    EqualNotEqual           // Jump if 3rd word not equal
        cmpw    r.8,r.9                 // Check for 4th word equal
        addi    r.3,r.3,16              // Update Source1 pointer
        bne-    EqualNotEqual           // Jump if 4th word not equal
        cmpw    r.3,r.5                 // Check for all blocks done
        addi    r.4,r.4,16              // Update Source2 pointer
        bne-    EqualAligned            // Jump if more blocks

        sub     r.5,r.10,r.3            // compute remaining bytes

EqualByByte:

//
// Compare 1-byte blocks until done.
//

        cmpwi   r.5,0                   // Check for no bytes left
        beq+    EqualEqual              // Jump to return if done

EqualByByteLoop:

        lbz     r.6,0(r.3)              // Get Source1 byte
        lbz     r.7,0(r.4)              // Get Source2 byte
        addi    r.3,r.3,1               // Update Source1 address
        cmpw    r.6,r.7                 // Check for equality
        addi    r.4,r.4,1               // Update Source2 address
        bne-    EqualNotEqual           // Jump if not equal
        cmpw    r.10,r.3                // Check for end of block
        bne-    EqualByByteLoop         // Loop if not done

EqualEqual:

//
// The blocks are not equal.
//

        li      r.3,TRUE                // indicate blocks are equal

        blr                             // return to caller

EqualUnaligned:

//
// There are at least 16 bytes to compare, but at least one of the blocks
// is not word-aligned.
//

        andi.   r.9,r.9,1               // isolate alignment bits
        bne-    EqualByteUnaligned      // jump if at least one not halfword aligned

EqualUnalignedLoop:

//
// Both blocks are halfword-aligned, and there are at least 16 bytes to compare.
//

        lhz     r.6,0(r.3)              // Get 1st hword of 1st Source1 wrd
        lhz     r.7,0(r.4)              // Get 1st hword of 1st Source2 wrd
        lhz     r.8,2(r.3)              // Get 2nd hword of 1st Source1 wrd
        cmpw    r.6,r.7                 // Check for 1st word equal
        lhz     r.9,2(r.4)              // Get 2nd hword of 1st Source2 wrd
        bne-    EqualNotEqual           // Check for 1st word equal
        cmpw    r.8,r.9                 // Check for 1st word equal
        lhz     r.6,4(r.3)              // Get 1st hword of 2nd Source1 wrd
        lhz     r.7,4(r.4)              // Get 1st hword of 2nd Source2 wrd
        bne-    EqualNotEqual           // Check for 1st word equal
        cmpw    r.6,r.7                 // Check for 2nd word equal
        lhz     r.8,6(r.3)              // Get 2nd hword of 2nd Source1 wrd
        lhz     r.9,6(r.4)              // Get 2nd hword of 2nd Source2 wrd
        bne-    EqualNotEqual           // Check for 2nd word equal
        cmpw    r.8,r.9                 // Check for 2nd word equal
        lhz     r.6,8(r.3)              // Get 1st hword of 3rd Source1 wrd
        lhz     r.7,8(r.4)              // Get 1st hword of 3rd Source2 wrd
        bne-    EqualNotEqual           // Check for 2nd word equal
        cmpw    r.6,r.7                 // Check for 3rd word equal
        lhz     r.8,10(r.3)             // Get 2nd hword of 3rd Source1 wrd
        lhz     r.9,10(r.4)             // Get 2nd hword of 3rd Source2 wrd
        bne-    EqualNotEqual           // Check for 3rd word equal
        cmpw    r.8,r.9                 // Check for 3rd word equal
        lhz     r.6,12(r.3)             // Get 1st hword of 4th Source1 wrd
        lhz     r.7,12(r.4)             // Get 1st hword of 4th Source2 wrd
        bne-    EqualNotEqual           // Check for 3rd word equal
        cmpw    r.6,r.7                 // Check for 4th word equal
        lhz     r.8,14(r.3)             // Get 2nd hword of 4th Source1 wrd
        lhz     r.9,14(r.4)             // Get 2nd hword of 4th Source2 wrd
        bne-    EqualNotEqual           // Check for 4th word equal
        cmpw    r.8,r.9                 // Check for 4th word equal
        addi    r.3,r.3,16              // Update Source1 pointer
        bne-    EqualNotEqual           // Check for 4th word equal
        cmpw    r.3,r.5                 // Check for all blocks done
        addi    r.4,r.4,16              // Update Source2 pointer
        bne-    EqualUnalignedLoop      // Jump if more blocks

        sub     r.5,r.10,r.3            // compute remaining bytes
        b       EqualByByte             // compare rest byte-by-byte

EqualByteUnaligned:

//
// There are at least 16 bytes to compare, but at least one of the blocks
// is not halfword-aligned.
//
// Because we don't expect very high byte counts in RtlEqualMemory, and
// we also don't expect unaligned buffers very often, we don't bother
// with the byte/halfword fetches that RtlCompareMemory does.
//

        lbz     r.6,0(r.3)              // Get first byte of 1st Source1 wrd
        lbz     r.7,0(r.4)              // Get first byte of 1st Source2 wrd
        lbz     r.8,1(r.3)              // Get 2nd byte of 1st Source1 wrd
        cmpw    r.6,r.7                 // Check for 1st word equal
        lbz     r.9,1(r.4)              // Get 2nd byte of 1st Source2 wrd
        bne     EqualNotEqual           // Jump if 1st word not equal
        cmpw    r.8,r.9                 // Check for 1st word equal
        lbz     r.6,2(r.3)              // Get first byte of 1st Source1 wrd
        lbz     r.7,2(r.4)              // Get first byte of 1st Source2 wrd
        bne     EqualNotEqual           // Jump if 1st word not equal
        cmpw    r.6,r.7                 // Check for 1st word equal
        lbz     r.8,3(r.3)              // Get 2nd byte of 1st Source1 wrd
        lbz     r.9,3(r.4)              // Get 2nd byte of 1st Source2 wrd
        bne     EqualNotEqual           // Jump if 1st word not equal
        cmpw    r.8,r.9                 // Check for 1st word equal
        lbz     r.6,4(r.3)              // Get first byte of 2nd Source1 wrd
        lbz     r.7,4(r.4)              // Get first byte of 2nd Source2 wrd
        bne     EqualNotEqual           // Jump if 1st word not equal
        cmpw    r.6,r.7                 // Check for 2nd word equal
        lbz     r.8,5(r.3)              // Get 2nd byte of 2nd Source1 wrd
        lbz     r.9,5(r.4)              // Get 2nd byte of 2nd Source2 wrd
        bne     EqualNotEqual           // Jump if 2nd word not equal
        cmpw    r.8,r.9                 // Check for 2nd word equal
        lbz     r.6,6(r.3)              // Get first byte of 2nd Source1 wrd
        lbz     r.7,6(r.4)              // Get first byte of 2nd Source2 wrd
        bne     EqualNotEqual           // Jump if 2nd word not equal
        cmpw    r.6,r.7                 // Check for 2nd word equal
        lbz     r.8,7(r.3)              // Get 2nd byte of 2nd Source1 wrd
        lbz     r.9,7(r.4)              // Get 2nd byte of 2nd Source2 wrd
        bne     EqualNotEqual           // Jump if 2nd word not equal
        cmpw    r.8,r.9                 // Check for 2nd word equal
        lbz     r.6,8(r.3)              // Get first byte of 3rd Source1 wrd
        lbz     r.7,8(r.4)              // Get first byte of 3rd Source2 wrd
        bne     EqualNotEqual           // Jump if 2nd word not equal
        cmpw    r.6,r.7                 // Check for 3rd word equal
        lbz     r.8,9(r.3)              // Get 2nd byte of 3rd Source1 wrd
        lbz     r.9,9(r.4)              // Get 2nd byte of 3rd Source2 wrd
        bne     EqualNotEqual           // Jump if 3rd word not equal
        cmpw    r.8,r.9                 // Check for 3rd word equal
        lbz     r.6,10(r.3)             // Get first byte of 3rd Source1 wrd
        lbz     r.7,10(r.4)             // Get first byte of 3rd Source2 wrd
        bne     EqualNotEqual           // Jump if 3rd word not equal
        cmpw    r.6,r.7                 // Check for 3rd word equal
        lbz     r.8,11(r.3)             // Get 2nd byte of 3rd Source1 wrd
        lbz     r.9,11(r.4)             // Get 2nd byte of 3rd Source2 wrd
        bne     EqualNotEqual           // Jump if 3rd word not equal
        cmpw    r.8,r.9                 // Check for 3rd word equal
        lbz     r.6,12(r.3)             // Get first byte of 4th Source1 wrd
        lbz     r.7,12(r.4)             // Get first byte of 4th Source2 wrd
        bne     EqualNotEqual           // Jump if 3rd word not equal
        cmpw    r.6,r.7                 // Check for 4th word equal
        lbz     r.8,13(r.3)             // Get 2nd byte of 4th Source1 wrd
        lbz     r.9,13(r.4)             // Get 2nd byte of 4th Source2 wrd
        bne     EqualNotEqual           // Jump if 4th word not equal
        cmpw    r.8,r.9                 // Check for 4th word equal
        lbz     r.6,14(r.3)             // Get first byte of 4th Source1 wrd
        lbz     r.7,14(r.4)             // Get first byte of 4th Source2 wrd
        bne     EqualNotEqual           // Jump if 4th word not equal
        cmpw    r.6,r.7                 // Check for 4th word equal
        lbz     r.8,15(r.3)             // Get 2nd byte of 4th Source1 wrd
        lbz     r.9,15(r.4)             // Get 2nd byte of 4th Source2 wrd
        bne     EqualNotEqual           // Jump if 4th word not equal
        cmpw    r.8,r.9                 // Check for 4th word equal
        addi    r.3,r.3,16              // Update Source1 pointer
        bne     EqualNotEqual           // Jump if 4th word not equal
        cmpw    r.3,r.5                 // Check for all blocks done
        addi    r.4,r.4,16              // Update Source2 pointer
        bne-    EqualByteUnaligned      // Jump if more blocks

        sub     r.5,r.10,r.3            // compute remaining bytes
        b       EqualByByte             // compare rest byte-by-byte

EqualNotEqual:

//
// The blocks are not equal.
//

        li      r.3, FALSE              // indicate blocks are not equal

        LEAF_EXIT(RtlEqualMemory)       // return to caller

//++
//
// VOID
// RtlMoveMemory (
//    IN PVOID Destination,
//    IN PVOID Source,
//    IN ULONG Length
//    )
//
// Routine Description:
//
//    This function moves memory either forward or backward, aligned or
//    unaligned, in 32-byte blocks, followed by 4-byte blocks, followed
//    by any remaining bytes.
//
//    The alternate entry point, RtlCopyMemory, moves non-overlapping
//    blocks only, in the forward direction.
//
//    RtlCopyMemory32 is the same as RtlCopyMemory but is guaranteed
//    never to copy more than 32 bits at a time.  RtlCopyMemory may
//    (probably will) be modified in the future to copy 64 bits at
//    a time.
//
// Arguments:
//
//    DEST (r.3) - Supplies a pointer to the destination address of
//                 the move operation.
//
//    SRC (r.4) - Supplies a pointer to the source address of the move
//                operation.
//
//    LNGTH (r.5) - Supplies the length, in bytes, of the memory to be
//                  moved.
//
// Return Value:
//
//    None.
//
//--
//
// Define the routine entry point
//
        LEAF_ENTRY(RtlMoveMemory)
//
// Check to see if destination block overlaps the source block
// If so, jump to a backward move to preserve source block from
// being corrupted.
//
        cmpw    r.4,r.3                 // Check to see if DEST > SRC
        bge+    MoveForward             // Jump if no overlap possible
        add     r.10,r.4,r.5            // Get ending SRC address
        cmpw    r.10,r.3                // Check for overlap
        bgt-    MoveBackward            // Jump for overlap
//
// Move Memory Forward
//
// Check alignment
//

        ALTERNATE_ENTRY(RtlCopyMemory)
        ALTERNATE_ENTRY(RtlCopyMemory32)

MoveForward:
        cmpwi   r.5,4                   // Check for less than 4 bytes
        blt-    FwdMoveByByte           // Jump if single byte moves
        xor     r.9,r.4,r.3             // Check for same alignment
        andi.   r.9,r.9,3               // Isolate alignment bits
        bne-    MvFwdUnaligned          // Jump if different alignments
//
// Move Memory Forward - Same SRC and DEST alignment
//
// Load and store extra bytes until a word boundary is reached
//

MvFwdAligned:
        andi.   r.6,r.3,3               // Check alignment type
        beq+    FwdBlkDiv               // Jump to process 32-Byte blocks
        cmpwi   r.6,3                   // Check for 1 byte unaligned
        bne+    FwdChkFor2              // If not, check next case
        lbz     r.7,0(r.4)              // Get unaligned byte
        li      r.6,1                   // Set byte move count
        stb     r.7,0(r.3)              // Store unaligned byte
        b       UpdateAddrs             // Jump to update addresses
FwdChkFor2:
        cmpwi   r.6,2                   // Check for halfword aligned
        bne+    FwdChkFor1              // If not, check next case
        lhz     r.7,0(r.4)              // Get unaligned halfword
        li      r.6,2                   // Set byte move count
        sth     r.7,0(r.3)              // Store unaligned halfword
        b       UpdateAddrs             // Jump to update addresses
FwdChkFor1:
        lbz     r.8,0(r.4)              // Get unaligned byte
        lhz     r.7,1(r.4)              // Get unaligned halfword
        stb     r.8,0(r.3)              // Store unaligned byte
        sth     r.7,1(r.3)              // Store unaligned halfword
        li      r.6,3                   // Set byte move count
UpdateAddrs:
        sub     r.5,r.5,r.6             // Decrement LNGTH by unaligned
        add     r.4,r.4,r.6             // Update the SRC address
        add     r.3,r.3,r.6             // Update the DEST address
//
// Divide the block to process into 32-byte blocks
//

FwdBlkDiv:
        andi.   r.6,r.5,BLKLN-1         // Isolate remainder of LNGTH/32
        sub.    r.7,r.5,r.6             // Get full block count
        add     r.10,r.4,r.7            // Get address of last full block
        beq-    FwdMoveBy4Bytes         // Jump if no full blocks
        mr      r.5,r.6                 // Set Length = remainder
//
// Move 32-byte blocks
//
FwdMvFullBlks:
        lwz     r.6,0(r.4)              // Get 1st SRC word
        lwz     r.7,4(r.4)              // Get 2nd SRC word
        stw     r.6,0(r.3)              // Store 1st DEST word
        stw     r.7,4(r.3)              // Store 2nd DEST word
        lwz     r.6,8(r.4)              // Get 3rd SRC word
        lwz     r.7,12(r.4)             // Get 4th SRC word
        stw     r.6,8(r.3)              // Store 3rd DEST word
        stw     r.7,12(r.3)             // Store 4th DEST word
        lwz     r.6,16(r.4)             // Get 5th SRC word
        lwz     r.7,20(r.4)             // Get 6th SRC word
        stw     r.6,16(r.3)             // Store 5th DEST word
        stw     r.7,20(r.3)             // Store 6th DEST word
        lwz     r.6,24(r.4)             // Get 7th SRC word
        lwz     r.7,28(r.4)             // Get 8th SRC word
        addi    r.4,r.4,32              // Update SRC pointer
        cmpw    r.4,r.10                // Check for all blocks done
        stw     r.6,24(r.3)             // Store 7th DEST word
        stw     r.7,28(r.3)             // Store 8th DEST word
        addi    r.3,r.3,32              // Update DEST pointer
        bne+    FwdMvFullBlks           // Jump if more blocks
//
// Move 4-byte blocks
//

FwdMoveBy4Bytes:
        andi.   r.6,r.5,4-1             // Isolate remainder of LNGTH/4
        sub.    r.7,r.5,r.6             // Get 4-byte block count
        add     r.10,r.4,r.7            // Get address of last full block
        beq-    FwdMoveByByte           // Jump if no 4-byte blocks
        mr      r.5,r.6                 // Set Length = remainder

FwdLpOn4Bytes:
        lwz     r.6,0(r.4)              // Load next set of 4 bytes
        addi    r.4,r.4,4               // Get pointer to next SRC block
        cmpw    r.4,r.10                // Check for last block
        stw     r.6,0(r.3)              // Store next DEST block
        addi    r.3,r.3,4               // Get pointer to next DEST block
        bne+    FwdLpOn4Bytes           // Jump if more blocks
//
// Move 1-byte blocks
//

FwdMoveByByte:
        cmpwi   r.5,0                   // Check for no bytes left
        beqlr+                          // Return if done
        cmpwi   r.5,1                   // Check for no bytes left
        lbz     r.6,0(r.4)              // Get 1st SRC byte
        stb     r.6,0(r.3)              // Store 1st DEST byte
        beqlr+                          // Return if done
        cmpwi   r.5,2                   // Check for no bytes left
        lbz     r.6,1(r.4)              // Get 2nd SRC byte
        stb     r.6,1(r.3)              // Store 2nd DEST byte
        beqlr+                          // Return if done
        lbz     r.6,2(r.4)              // Get 3rd SRC byte
        stb     r.6,2(r.3)              // Store 3rd byte word
        blr                             // Return
//
// Forward Move - SRC and DEST have different alignments
//

MvFwdUnaligned:
        or      r.9,r.4,r.3             // Check if either byte unaligned
        andi.   r.9,r.9,3               // Isolate alignment
        cmpwi   r.9,2                   // Check for even result
        bne+    FwdMvByteUnaligned      // Jump for byte unaligned
//
// Divide the blocks to process into 32-byte blocks
//

FwdBlkDivUnaligned:
        andi.   r.6,r.5,BLKLN-1         // Isolate remainder of LNGTH/32
        sub.    r.7,r.5,r.6             // Get full block count
        add     r.10,r.4,r.7            // Get address of last full block
        beq-    FwdMvHWrdBy4Bytes       // Jump if no full blocks
        mr      r.5,r.6                 // Set Length = remainder
//
// Forward Move - SRC or DEST is halfword aligned, the other is by word
//
FwdMvByHWord:
        lhz     r.6,0(r.4)              // Get 1st 2 bytes of 1st SRC wrd
        lhz     r.7,2(r.4)              // Get 2nd 2 bytes of 1st SRC wrd
        sth     r.6,0(r.3)              // Put 1st 2 bytes of 1st DST wrd
        sth     r.7,2(r.3)              // Put 2nd 2 bytes of 1st DST wrd
        lhz     r.6,4(r.4)              // Get 1st 2 bytes of 2nd SRC wrd
        lhz     r.7,6(r.4)              // Get 2nd 2 bytes of 2nd SRC wrd
        sth     r.6,4(r.3)              // Put 1st 2 bytes of 2nd DST wrd
        sth     r.7,6(r.3)              // Put 2nd 2 bytes of 2nd DST wrd
        lhz     r.6,8(r.4)              // Get 1st 2 bytes of 3rd SRC wrd
        lhz     r.7,10(r.4)             // Get 2nd 2 bytes of 3rd SRC wrd
        sth     r.6,8(r.3)              // Put 1st 2 bytes of 3rd DST wrd
        sth     r.7,10(r.3)             // Put 2nd 2 bytes of 3rd DST wrd
        lhz     r.6,12(r.4)             // Get 1st 2 bytes of 4th SRC wrd
        lhz     r.7,14(r.4)             // Get 2nd 2 bytes of 4th SRC wrd
        sth     r.6,12(r.3)             // Put 1st 2 bytes of 4th DST wrd
        sth     r.7,14(r.3)             // Put 2nd 2 bytes of 4th DST wrd
        lhz     r.6,16(r.4)             // Get 1st 2 bytes of 5th SRC wrd
        lhz     r.7,18(r.4)             // Get 2nd 2 bytes of 5th SRC wrd
        sth     r.6,16(r.3)             // Put 1st 2 bytes of 5th DST wrd
        sth     r.7,18(r.3)             // Put 2nd 2 bytes of 5th DST wrd
        lhz     r.6,20(r.4)             // Get 1st 2 bytes of 6th SRC wrd
        lhz     r.7,22(r.4)             // Get 2nd 2 bytes of 6th SRC wrd
        sth     r.6,20(r.3)             // Put 1st 2 bytes of 6th DST wrd
        sth     r.7,22(r.3)             // Put 2nd 2 bytes of 6th DST wrd
        lhz     r.6,24(r.4)             // Get 1st 2 bytes of 7th SRC wrd
        lhz     r.7,26(r.4)             // Get 2nd 2 bytes of 7th SRC wrd
        sth     r.6,24(r.3)             // Put 1st 2 bytes of 7th DST wrd
        sth     r.7,26(r.3)             // Put 2nd 2 bytes of 7th DST wrd
        lhz     r.6,28(r.4)             // Get 1st 2 bytes of 8th SRC wrd
        lhz     r.7,30(r.4)             // Get 2nd 2 bytes of 8th SRC wrd
        addi    r.4,r.4,32              // Update SRC pointer
        cmpw    r.4,r.10                // Check for all blocks done
        sth     r.6,28(r.3)             // Put 1st 2 bytes of 8th DST wrd
        sth     r.7,30(r.3)             // Put 2nd 2 bytes of 8th DST wrd
        addi    r.3,r.3,32              // Update DEST pointer
        bne+    FwdMvByHWord            // Jump if more blocks
//
// Move 4-byte blocks with DEST Halfword unaligned
//

FwdMvHWrdBy4Bytes:
        andi.   r.6,r.5,4-1             // Isolate remainder of LNGTH/4
        sub.    r.7,r.5,r.6             // Get 4-byte block count
        add     r.10,r.4,r.7            // Get address of last full block
        beq-    FwdMoveByByte           // Jump if no 4-byte blocks
        mr      r.5,r.6                 // Set Length = remainder

FwdHWrdLpOn4Bytes:
        lhz     r.6,0(r.4)              // Get 1st 2 bytes of 1st SRC wrd
        lhz     r.7,2(r.4)              // Get 2nd 2 bytes of 1st SRC wrd
        addi    r.4,r.4,4               // Update SRC pointer
        cmpw    r.4,r.10                // Check for last block
        sth     r.6,0(r.3)              // Put 1st 2 bytes of 1st DST wrd
        sth     r.7,2(r.3)              // Put 2nd 2 bytes of 1st DST wrd
        addi    r.3,r.3,4               // Update DEST pointer
        bne+    FwdHWrdLpOn4Bytes       // Jump if more blocks

        b       FwdMoveByByte           // Jump to complete last bytes
//
// Forward Move - DEST is byte unaligned - Check SRC
//

FwdMvByteUnaligned:
        and     r.9,r.3,r.4             // Check for both byte aligned
        andi.   r.9,r.9,1               // Isolate alignment bits
        beq-    FwdBlksByByte           // Jump if both not byte aligned
//
// Divide the blocks to process into 32-byte blocks
//
        andi.   r.6,r.5,BLKLN-1         // Isolate remainder of LNGTH/32
        sub.    r.7,r.5,r.6             // Get full block count
        add     r.10,r.4,r.7            // Get address of last full block
        beq-    FwdMvByteBy4Bytes       // Jump if no full blocks
        mr      r.5,r.6                 // Set Length = remainder
//
// Forward Move - Both DEST and SRC are byte unaligned, but differently
//

FwdMvByByte:
        lbz     r.6,0(r.4)              // Get first byte of 1st SRC word
        lhz     r.7,1(r.4)              // Get mid-h-word of 1st SRC word
        lhz     r.8,3(r.4)              // Get h-word crossing 1st/2nd SRC word
        stb     r.6,0(r.3)              // Put first byte of 1st DEST word
        sth     r.7,1(r.3)              // Put mid-h-word of 1st DEST word
        sth     r.8,3(r.3)              // Put h-word crossing 1st/2nd DEST word
        lhz     r.6,5(r.4)              // Get mid-h-word of 2nd SRC word
        lhz     r.7,7(r.4)              // Get h-word crossing 2nd/3rd SRC word
        lhz     r.8,9(r.4)              // Get mid-h-word of 3rd SRC word
        sth     r.6,5(r.3)              // Put mid-h-word of 2nd DEST word
        sth     r.7,7(r.3)              // Put h-word crossing 2nd/3rd DEST word
        sth     r.8,9(r.3)              // Put mid-h-word of 3rd DEST word
        lhz     r.6,11(r.4)             // Get h-word crossing 3rd/4th SRC word
        lhz     r.7,13(r.4)             // Get mid-h-word of 4th SRC word
        lhz     r.8,15(r.4)             // Get h-word crossing 4th/5th SRC word
        sth     r.6,11(r.3)             // Put h-word crossing 3rd/4th DEST word
        sth     r.7,13(r.3)             // Put mid-h-word of 4th DEST word
        sth     r.8,15(r.3)             // Put h-word crossing 4th/5th DEST word
        lhz     r.6,17(r.4)             // Get mid-h-word of 5th SRC word
        lhz     r.7,19(r.4)             // Get h-word crossing 5th/6th SRC word
        lhz     r.8,21(r.4)             // Get mid-h-word of 6th SRC word
        sth     r.6,17(r.3)             // Put mid-h-word of 5th DEST word
        sth     r.7,19(r.3)             // Put h-word crossing 5th/6th DEST word
        sth     r.8,21(r.3)             // Put mid-h-word of 6th DEST word
        lhz     r.6,23(r.4)             // Get h-word crossing 6th/7th SRC word
        lhz     r.7,25(r.4)             // Get mid-h-word of 7th SRC word
        lhz     r.8,27(r.4)             // Get h-word crossing 7th/8th SRC word
        sth     r.6,23(r.3)             // Put h-word crossing 6th/7th DEST word
        sth     r.7,25(r.3)             // Put mid-h-word of 7th DEST word
        sth     r.8,27(r.3)             // Put h-word crossing 7th/8th DEST word
        lhz     r.6,29(r.4)             // Get mid-h-word of 8th SRC word
        lbz     r.7,31(r.4)             // Get last byte of 8th SRC word
        addi    r.4,r.4,32              // Update SRC pointer
        cmpw    r.4,r.10                // Check for all blocks done
        sth     r.6,29(r.3)             // Put mid-h-word of 8th DEST word
        stb     r.7,31(r.3)             // Put last byte of 8th DEST word
        addi    r.3,r.3,32              // Update DEST pointer
        bne+    FwdMvByByte             // Jump if more blocks
//
// Move 4-byte blocks with DEST or SRC byte aligned
//

FwdMvByteBy4Bytes:
        andi.   r.6,r.5,4-1             // Isolate remainder of LNGTH/4
        sub.    r.7,r.5,r.6             // Get 4-byte block count
        add     r.10,r.4,r.7            // Get address of last full block
        beq-    FwdMoveByByte           // Jump if no 4-byte blocks
        mr      r.5,r.6                 // Set Length = remainder

FwdByteLpOn4Bytes:
        lbz     r.6,0(r.4)              // Get first byte of 1st SRC word
        lhz     r.7,1(r.4)              // Get mid-h-word of 1st SRC word
        lbz     r.8,3(r.4)              // Get last byte of 1st SRC word
        stb     r.6,0(r.3)              // Put first byte of 1st DEST wd
        addi    r.4,r.4,4               // Update SRC pointer
        cmpw    r.4,r.10                // Check for last block
        sth     r.7,1(r.3)              // Put mid-h-word of 1st DEST wd
        stb     r.8,3(r.3)              // Put last byte of 1st DEST wrd
        addi    r.3,r.3,4               // Update DEST pointer
        bne+    FwdByteLpOn4Bytes       // Jump if more blocks

        b       FwdMoveByByte           // Jump to complete last bytes
//
// Forward Move - Either SRC or DEST are byte unaligned but not both
//
// Divide the blocks to process into 32-byte blocks
//

FwdBlksByByte:
        andi.   r.6,r.5,BLKLN-1         // Isolate remainder of LNGTH/32
        sub.    r.7,r.5,r.6             // Get full block count
        add     r.10,r.4,r.7            // Get address of last full block
        beq-    FwdMvBlksOf4Bytes       // Jump if no full blocks
        mr      r.5,r.6                 // Set Length = remainder

FwdMvBlksByByte:
        lbz     r.6,0(r.4)              // Get first byte of 1st SRC wrd
        lbz     r.7,1(r.4)              // Get second byte of 1st SRC wrd
        stb     r.6,0(r.3)              // Put first byte of 1st DEST wrd
        stb     r.7,1(r.3)              // Put second byte of 1st DST wrd
        lbz     r.6,2(r.4)              // Get third byte of 1st SRC wrd
        lbz     r.7,3(r.4)              // Get fourth byte of 1st SRC wrd
        stb     r.6,2(r.3)              // Put third byte of 1st DEST wrd
        stb     r.7,3(r.3)              // Put fourth byte of 1st DST wrd
        lbz     r.6,4(r.4)              // Get first byte of 2nd SRC wrd
        lbz     r.7,5(r.4)              // Get 2nd byte of 2nd SRC wrd
        stb     r.6,4(r.3)              // Put first byte of 2nd DEST wrd
        stb     r.7,5(r.3)              // Put second byte of 2nd DST wrd
        lbz     r.6,6(r.4)              // Get third byte of 2nd SRC wrd
        lbz     r.7,7(r.4)              // Get fourth byte of 2nd SRC wrd
        stb     r.6,6(r.3)              // Put third byte of 2nd DEST wrd
        stb     r.7,7(r.3)              // Put fourth byte of 2nd DST wrd
        lbz     r.6,8(r.4)              // Get first byte of 3rd SRC wrd
        lbz     r.7,9(r.4)              // Get second byte of 3rd SRC wrd
        stb     r.6,8(r.3)              // Put first byte of 3rd DEST wrd
        stb     r.7,9(r.3)              // Put second byte of 3rd DST wrd
        lbz     r.6,10(r.4)             // Get third byte of 3rd SRC wrd
        lbz     r.7,11(r.4)             // Get fourth byte of 3rd SRC wrd
        stb     r.6,10(r.3)             // Put third byte of 3rd DEST wrd
        stb     r.7,11(r.3)             // Put fourth byte of 3rd DST wrd
        lbz     r.6,12(r.4)             // Get first byte of 4th SRC wrd
        lbz     r.7,13(r.4)             // Get second byte of 4th SRC wrd
        stb     r.6,12(r.3)             // Put first byte of 4th DEST wrd
        stb     r.7,13(r.3)             // Put second byte of 4th DST wrd
        lbz     r.6,14(r.4)             // Get third byte of 4th SRC wrd
        lbz     r.7,15(r.4)             // Get fourth byte of 4th SRC wrd
        stb     r.6,14(r.3)             // Put third byte of 4th DEST wrd
        stb     r.7,15(r.3)             // Put fourth byte of 4th DST wrd
        lbz     r.6,16(r.4)             // Get first byte of 5th SRC wrd
        lbz     r.7,17(r.4)             // Get second byte of 5th SRC wrd
        stb     r.6,16(r.3)             // Put first byte of 5th DEST wrd
        stb     r.7,17(r.3)             // Put second byte of 5th DST wrd
        lbz     r.6,18(r.4)             // Get third byte of 5th SRC wrd
        lbz     r.7,19(r.4)             // Get fourth byte of 5th SRC wrd
        stb     r.6,18(r.3)             // Put third byte of 5th DEST wrd
        stb     r.7,19(r.3)             // Put fourth byte of 5th DST wrd
        lbz     r.6,20(r.4)             // Get first byte of 6th SRC wrd
        lbz     r.7,21(r.4)             // Get second byte of 6th SRC wrd
        stb     r.6,20(r.3)             // Put first byte of 6th DEST wrd
        stb     r.7,21(r.3)             // Put second byte of 6th DST wrd
        lbz     r.6,22(r.4)             // Get third byte of 6th SRC wrd
        lbz     r.7,23(r.4)             // Get fourth byte of 6th SRC wrd
        stb     r.6,22(r.3)             // Put third byte of 6th DEST wrd
        stb     r.7,23(r.3)             // Put fourth byte of 6th DST wrd
        lbz     r.6,24(r.4)             // Get first byte of 7th SRC wrd
        lbz     r.7,25(r.4)             // Get second byte of 7th SRC wrd
        stb     r.6,24(r.3)             // Put first byte of 7th DEST wrd
        stb     r.7,25(r.3)             // Put second byte of 7th DST wrd
        lbz     r.6,26(r.4)             // Get third byte of 7th SRC wrd
        lbz     r.7,27(r.4)             // Get fourth byte of 7th SRC wrd
        stb     r.6,26(r.3)             // Put third byte of 7th DEST wrd
        stb     r.7,27(r.3)             // Put fourth byte of 7th DST wrd
        lbz     r.6,28(r.4)             // Get first byte of 8th SRC wrd
        lbz     r.7,29(r.4)             // Get second byte of 8th SRC wrd
        stb     r.6,28(r.3)             // Put first byte of 8th DEST wrd
        stb     r.7,29(r.3)             // Put second byte of 8th DST wrd
        lbz     r.6,30(r.4)             // Get third byte of 8th SRC wrd
        lbz     r.7,31(r.4)             // Get fourth byte of 8th SRC wrd
        addi    r.4,r.4,32              // Update SRC pointer
        cmpw    r.4,r.10                // Check for all blocks done
        stb     r.6,30(r.3)             // Put third byte of 8th DEST wrd
        stb     r.7,31(r.3)             // Put fourth byte of 8th DST wrd
        addi    r.3,r.3,32              // Update DEST pointer
        bne+    FwdMvBlksByByte         // Jump if more blocks
//
// Move 4-byte blocks with DEST or SRC Byte aligned
//

FwdMvBlksOf4Bytes:
        andi.   r.6,r.5,4-1             // Isolate remainder of LNGTH/4
        sub.    r.7,r.5,r.6             // Get 4-byte block count
        add     r.10,r.4,r.7            // Get address of last full block
        beq-    FwdMoveByByte           // Jump if no 4-byte blocks
        mr      r.5,r.6                 // Set Length = remainder

FwdBlksLpOn4Bytes:
        lbz     r.6,0(r.4)              // Get first byte of 1st SRC wrd
        lbz     r.7,1(r.4)              // Get second byte of 1st SRC wrd
        stb     r.6,0(r.3)              // Put first byte of 1st DEST wrd
        stb     r.7,1(r.3)              // Put second byte of 1st DST wrd
        lbz     r.6,2(r.4)              // Get third byte of 1st SRC wrd
        lbz     r.7,3(r.4)              // Get fourth byte of 1st SRC wrd
        addi    r.4,r.4,4               // Update SRC pointer
        cmpw    r.4,r.10                // Check for last block
        stb     r.6,2(r.3)              // Put third byte of 1st DEST wrd
        stb     r.7,3(r.3)              // Put fourth byte of 1st DST wrd
        addi    r.3,r.3,4               // Update DEST pointer
        bne+    FwdBlksLpOn4Bytes       // Jump if more blocks

        b       FwdMoveByByte           // Jump to complete last bytes
//
// Move Memory Backward
//
// Check alignment
//

MoveBackward:
        add     r.4,r.4,r.5             // Compute ending SRC address
        add     r.3,r.3,r.5             // Compute ending DEST address
        cmpwi   r.5,4                   // Check for less than 4 bytes
        blt-    BckMoveByByte           // Jump if single byte moves
        xor     r.9,r.4,r.3             // Check for same alignment
        andi.   r.9,r.9,3               // Isolate alignment bits
        bne-    MvBckUnaligned          // Jump if different alignments
//
// Move Memory Backword - Same SRC and DEST alignment
//
// Load and store extra bytes until a word boundary is reached
//

MvBckAligned:
        andi.   r.6,r.3,3               // Check alignment type
        beq+    BckBlkDiv               // Jump to process 32-Byte blocks
        cmpwi   r.6,1                   // Check for 1 byte unaligned
        bne+    BckChkFor2              // If not, check next case
        lbz     r.7,-1(r.4)             // Get unaligned byte
        sub     r.5,r.5,r.6             // Decrement LNGTH by unaligned
        stb     r.7,-1(r.3)             // Store unaligned byte
        b       BckUpdateAddrs          // Jump to update addresses
BckChkFor2:
        cmpwi   r.6,2                   // Check for halfword aligned
        bne+    BckChkFor3              // If not, check next case
        lhz     r.7,-2(r.4)             // Get unaligned halfword
        sub     r.5,r.5,r.6             // Decrement LNGTH by unaligned
        sth     r.7,-2(r.3)             // Store unaligned halfword
        b       BckUpdateAddrs          // Jump to update addresses
BckChkFor3:
        lbz     r.8,-1(r.4)             // Get unaligned byte
        lhz     r.7,-3(r.4)             // Get unaligned halfword
        stb     r.8,-1(r.3)             // Store unaligned byte
        sth     r.7,-3(r.3)             // Store unaligned halfword
        sub     r.5,r.5,r.6             // Decrement LNGTH by unaligned
BckUpdateAddrs:
        sub     r.4,r.4,r.6             // Update the SRC address
        sub     r.3,r.3,r.6             // Update the DEST address
//
// Divide the block to process into 32-byte blocks
//

BckBlkDiv:
        andi.   r.6,r.5,BLKLN-1         // Isolate remainder of LNGTH/32
        sub.    r.7,r.5,r.6             // Get full block count
        sub     r.10,r.4,r.7            // Get address of last full block
        beq-    BckMoveBy4Bytes         // Jump if no full blocks
        mr      r.5,r.6                 // Set Length = remainder
//
// Move 32-byte blocks
//
BckMvFullBlks:
        lwz     r.6,-4(r.4)             // Get 1st SRC word
        lwz     r.7,-8(r.4)             // Get 2nd SRC word
        stw     r.6,-4(r.3)             // Store 1st DEST word
        stw     r.7,-8(r.3)             // Store 2nd DEST word
        lwz     r.6,-12(r.4)            // Get 3rd SRC word
        lwz     r.7,-16(r.4)            // Get 4th SRC word
        stw     r.6,-12(r.3)            // Store 3rd DEST word
        stw     r.7,-16(r.3)            // Store 4th DEST word
        lwz     r.6,-20(r.4)            // Get 5th SRC word
        lwz     r.7,-24(r.4)            // Get 6th SRC word
        stw     r.6,-20(r.3)            // Store 5th DEST word
        stw     r.7,-24(r.3)            // Store 6th DEST word
        lwz     r.6,-28(r.4)            // Get 7th SRC word
        lwz     r.7,-32(r.4)            // Get 8th SRC word
        subi    r.4,r.4,32              // Update SRC pointer
        cmpw    r.4,r.10                // Check for all blocks done
        stw     r.6,-28(r.3)            // Store 7th DEST word
        stw     r.7,-32(r.3)            // Store 8th DEST word
        subi    r.3,r.3,32              // Update DEST pointer
        bne+    BckMvFullBlks           // Jump if more blocks
//
// Move 4-byte blocks
//

BckMoveBy4Bytes:
        andi.   r.6,r.5,4-1             // Isolate remainder of LNGTH/4
        sub.    r.7,r.5,r.6             // Get 4-byte block count
        sub     r.10,r.4,r.7            // Get address of last full block
        beq-    BckMoveByByte           // Jump if no 4-byte blocks
        mr      r.5,r.6                 // Set Length = remainder

BckLpOn4Bytes:
        lwz     r.6,-4(r.4)             // Load next set of 4 bytes
        subi    r.4,r.4,4               // Get pointer to next SRC block
        cmpw    r.4,r.10                // Check for last block
        stw     r.6,-4(r.3)             // Store next DEST block
        subi    r.3,r.3,4               // Get pointer to next DEST block
        bne+    BckLpOn4Bytes           // Jump if more blocks
//
// Move 1-byte blocks
//

BckMoveByByte:
        cmpwi   r.5,0                   // Check for no bytes left
        beqlr+                          // Return if done
        lbz     r.6,-1(r.4)             // Get 1st SRC byte
        cmpwi   r.5,1                   // Check for no bytes left
        stb     r.6,-1(r.3)             // Store 1st DEST byte
        beqlr+                          // Return if done
        lbz     r.6,-2(r.4)             // Get 2nd SRC byte
        cmpwi   r.5,2                   // Check for no bytes left
        stb     r.6,-2(r.3)             // Store 2nd DEST byte
        beqlr+                          // Return if done
        lbz     r.6,-3(r.4)             // Get 3rd SRC byte
        stb     r.6,-3(r.3)             // Store 3rd byte word
        blr                             // Return
//
// Backward Move - SRC and DEST have different alignments
//

MvBckUnaligned:
        or      r.9,r.4,r.3             // Check for either byte unaligned
        andi.   r.9,r.9,3               // Isolate alignment
        cmpwi   r.9,2                   // Check for even result
        bne+    BckMvByteUnaligned      // Jump for byte unaligned
//
// Divide the blocks to process into 32-byte blocks
//

BckBlkDivUnaligned:
        andi.   r.6,r.5,BLKLN-1         // Isolate remainder of LNGTH/32
        sub.    r.7,r.5,r.6             // Get full block count
        sub     r.10,r.4,r.7            // Get address of last full block
        beq-    BckMvHWrdBy4Bytes       // Jump if no full blocks
        mr      r.5,r.6                 // Set Length = remainder
//
// Backward Move - SRC or DEST is halfword aligned, the other is by word
//
BckMvByHWord:
        lhz     r.6,-2(r.4)             // Get 1st 2 bytes of 1st SRC wrd
        lhz     r.7,-4(r.4)             // Get 2nd 2 bytes of 1st SRC wrd
        sth     r.6,-2(r.3)             // Put 1st 2 bytes of 1st DST wrd
        sth     r.7,-4(r.3)             // Put 2nd 2 bytes of 1st DST wrd
        lhz     r.6,-6(r.4)             // Get 1st 2 bytes of 2nd SRC wrd
        lhz     r.7,-8(r.4)             // Get 2nd 2 bytes of 2nd SRC wrd
        sth     r.6,-6(r.3)             // Put 1st 2 bytes of 2nd DST wrd
        sth     r.7,-8(r.3)             // Put 2nd 2 bytes of 2nd DST wrd
        lhz     r.6,-10(r.4)            // Get 1st 2 bytes of 3rd SRC wrd
        lhz     r.7,-12(r.4)            // Get 2nd 2 bytes of 3rd SRC wrd
        sth     r.6,-10(r.3)            // Put 1st 2 bytes of 3rd DST wrd
        sth     r.7,-12(r.3)            // Put 2nd 2 bytes of 3rd DST wrd
        lhz     r.6,-14(r.4)            // Get 1st 2 bytes of 4th SRC wrd
        lhz     r.7,-16(r.4)            // Get 2nd 2 bytes of 4th SRC wrd
        sth     r.6,-14(r.3)            // Put 1st 2 bytes of 4th DST wrd
        sth     r.7,-16(r.3)            // Put 2nd 2 bytes of 4th DST wrd
        lhz     r.6,-18(r.4)            // Get 1st 2 bytes of 5th SRC wrd
        lhz     r.7,-20(r.4)            // Get 2nd 2 bytes of 5th SRC wrd
        sth     r.6,-18(r.3)            // Put 1st 2 bytes of 5th DST wrd
        sth     r.7,-20(r.3)            // Put 2nd 2 bytes of 5th DST wrd
        lhz     r.6,-22(r.4)            // Get 1st 2 bytes of 6th SRC wrd
        lhz     r.7,-24(r.4)            // Get 2nd 2 bytes of 6th SRC wrd
        sth     r.6,-22(r.3)            // Put 1st 2 bytes of 6th DST wrd
        sth     r.7,-24(r.3)            // Put 2nd 2 bytes of 6th DST wrd
        lhz     r.6,-26(r.4)            // Get 1st 2 bytes of 7th SRC wrd
        lhz     r.7,-28(r.4)            // Get 2nd 2 bytes of 7th SRC wrd
        sth     r.6,-26(r.3)            // Put 1st 2 bytes of 7th DST wrd
        sth     r.7,-28(r.3)            // Put 2nd 2 bytes of 7th DST wrd
        lhz     r.6,-30(r.4)            // Get 1st 2 bytes of 8th SRC wrd
        lhz     r.7,-32(r.4)            // Get 2nd 2 bytes of 8th SRC wrd
        subi    r.4,r.4,32              // Update SRC pointer
        cmpw    r.4,r.10                // Check for all blocks done
        sth     r.6,-30(r.3)            // Put 1st 2 bytes of 8th DST wrd
        sth     r.7,-32(r.3)            // Put 2nd 2 bytes of 8th DST wrd
        subi    r.3,r.3,32              // Update DEST pointer
        bne+    BckMvByHWord            // Jump if more blocks
//
// Move 4-byte blocks with DEST Halfword unaligned
//

BckMvHWrdBy4Bytes:
        andi.   r.6,r.5,4-1             // Isolate remainder of LNGTH/4
        sub.    r.7,r.5,r.6             // Get 4-byte block count
        sub     r.10,r.4,r.7            // Get address of last full block
        beq-    BckMoveByByte           // Jump if no 4-byte blocks
        mr      r.5,r.6                 // Set Length = remainder

BckHWrdLpOn4Bytes:
        lhz     r.6,-2(r.4)             // Get 1st 2 bytes of 1st SRC wrd
        lhz     r.7,-4(r.4)             // Get 2nd 2 bytes of 1st SRC wrd
        subi    r.4,r.4,4               // Update SRC pointer
        cmpw    r.4,r.10                // Check for last block
        sth     r.6,-2(r.3)             // Put 1st 2 bytes of 1st DST wrd
        sth     r.7,-4(r.3)             // Put 2nd 2 bytes of 1st DST wrd
        subi    r.3,r.3,4               // Update DEST pointer
        bne+    BckHWrdLpOn4Bytes       // Jump if more blocks

        b       BckMoveByByte           // Jump to complete last bytes
//
// Check for both byte unaligned
//

BckMvByteUnaligned:
        and     r.9,r.3,r.4             // Check for both byte aligned
        and     r.9,r.9,1               // Isolate alignment bits
        bne-    BckBlksByByte           // Jump if both not byte aligned
//
// Divide the blocks to process into 32-byte blocks
//
        andi.   r.6,r.5,BLKLN-1         // Isolate remainder of LNGTH/32
        sub.    r.7,r.5,r.6             // Get full block count
        sub     r.10,r.4,r.7            // Get address of last full block
        beq-    BckMvByteBy4Bytes       // Jump if no full blocks
        mr      r.5,r.6                 // Set Length = remainder
//
// Backward Move - Both SRC and DEST are byte unaligned, but differently
//

BckMvByByte:
        lbz     r.6,-1(r.4)              // Get first byte of 1st SRC word
        lhz     r.7,-3(r.4)              // Get mid-h-word of 1st SRC word
        lhz     r.8,-5(r.4)              // Get h-word crossing 1st/2nd SRC word
        stb     r.6,-1(r.3)              // Put first byte of 1st DEST word
        sth     r.7,-3(r.3)              // Put mid-h-word of 1st DEST word
        sth     r.8,-5(r.3)              // Put h-word crossing 1st/2nd DEST word
        lhz     r.6,-7(r.4)              // Get mid-h-word of 2nd SRC word
        lhz     r.7,-9(r.4)              // Get h-word crossing 2nd/3rd SRC word
        lhz     r.8,-11(r.4)              // Get mid-h-word of 3rd SRC word
        sth     r.6,-7(r.3)              // Put mid-h-word of 2nd DEST word
        sth     r.7,-9(r.3)              // Put h-word crossing 2nd/3rd DEST word
        sth     r.8,-11(r.3)              // Put mid-h-word of 3rd DEST word
        lhz     r.6,-13(r.4)             // Get h-word crossing 3rd/4th SRC word
        lhz     r.7,-15(r.4)             // Get mid-h-word of 4th SRC word
        lhz     r.8,-17(r.4)             // Get h-word crossing 4th/5th SRC word
        sth     r.6,-13(r.3)             // Put h-word crossing 3rd/4th DEST word
        sth     r.7,-15(r.3)             // Put mid-h-word of 4th DEST word
        sth     r.8,-17(r.3)             // Put h-word crossing 4th/5th DEST word
        lhz     r.6,-19(r.4)             // Get mid-h-word of 5th SRC word
        lhz     r.7,-21(r.4)             // Get h-word crossing 5th/6th SRC word
        lhz     r.8,-23(r.4)             // Get mid-h-word of 6th SRC word
        sth     r.6,-19(r.3)             // Put mid-h-word of 5th DEST word
        sth     r.7,-21(r.3)             // Put h-word crossing 5th/6th DEST word
        sth     r.8,-23(r.3)             // Put mid-h-word of 6th DEST word
        lhz     r.6,-25(r.4)             // Get h-word crossing 6th/7th SRC word
        lhz     r.7,-27(r.4)             // Get mid-h-word of 7th SRC word
        lhz     r.8,-29(r.4)             // Get h-word crossing 7th/8th SRC word
        sth     r.6,-25(r.3)             // Put h-word crossing 6th/7th DEST word
        sth     r.7,-27(r.3)             // Put mid-h-word of 7th DEST word
        sth     r.8,-29(r.3)             // Put h-word crossing 7th/8th DEST word
        lhz     r.6,-31(r.4)             // Get mid-h-word of 8th SRC word
        lbz     r.7,-32(r.4)             // Get last byte of 8th SRC word
        subi    r.4,r.4,32              // Update SRC pointer
        cmpw    r.4,r.10                // Check for all blocks done
        sth     r.7,-31(r.3)            // Put mid-h-word of 8th DEST wd
        stb     r.8,-32(r.3)            // Put last byte of 8th DEST wrd
        subi    r.3,r.3,32              // Update DEST pointer
        bne+    BckMvByByte             // Jump if more blocks
//
// Move 4-byte blocks with DEST and SRC Byte aligned
//

BckMvByteBy4Bytes:
        andi.   r.6,r.5,4-1             // Isolate remainder of LNGTH/4
        sub.    r.7,r.5,r.6             // Get 4-byte block count
        sub     r.10,r.4,r.7            // Get address of last full block
        beq-    BckMoveByByte           // Jump if no 4-byte blocks
        mr      r.5,r.6                 // Set Length = remainder

BckByteLpOn4Bytes:
        lbz     r.6,-1(r.4)             // Get first byte of 1st SRC word
        lhz     r.7,-3(r.4)             // Get mid-h-word of 1st SRC word
        lbz     r.8,-4(r.4)             // Get last byte of 1st SRC word
        stb     r.6,-1(r.3)             // Put first byte of 1st DEST wd
        subi    r.4,r.4,4               // Update SRC pointer
        cmpw    r.4,r.10                // Check for last block
        sth     r.7,-3(r.3)             // Put mid-h-word of 1st DEST wd
        stb     r.8,-4(r.3)             // Put last byte of 1st DEST wrd
        subi    r.3,r.3,4               // Update DEST pointer
        bne+    BckByteLpOn4Bytes       // Jump if more blocks

        b       BckMoveByByte           // Jump to complete last bytes
//
// Backward Move - Either DEST or SRC byte unaligned but not both
//
// Divide the blocks to process into 32-byte blocks
//

BckBlksByByte:
        andi.   r.6,r.5,BLKLN-1         // Isolate remainder of LNGTH/32
        sub.    r.7,r.5,r.6             // Get full block count
        sub     r.10,r.4,r.7            // Get address of last full block
        beq-    BckMvBlksOf4Bytes       // Jump if no full blocks
        mr      r.5,r.6                 // Set Length = remainder

BckMvBlksByByte:
        lbz     r.6,-1(r.4)             // Get first byte of 1st SRC wrd
        lbz     r.7,-2(r.4)             // Get second byte of 1st SRC wrd
        stb     r.6,-1(r.3)             // Put first byte of 1st DEST wrd
        stb     r.7,-2(r.3)             // Put second byte of 1st DST wrd
        lbz     r.6,-3(r.4)             // Get third byte of 1st SRC wrd
        lbz     r.7,-4(r.4)             // Get fourth byte of 1st SRC wrd
        stb     r.6,-3(r.3)             // Put third byte of 1st DEST wrd
        stb     r.7,-4(r.3)             // Put fourth byte of 1st DST wrd
        lbz     r.6,-5(r.4)             // Get first byte of 2nd SRC wrd
        lbz     r.7,-6(r.4)             // Get second byte of 2nd SRC wrd
        stb     r.6,-5(r.3)             // Put first byte of 2nd DEST wrd
        stb     r.7,-6(r.3)             // Put second byte of 2nd DST wrd
        lbz     r.6,-7(r.4)             // Get third byte of 2nd SRC wrd
        lbz     r.7,-8(r.4)             // Get fourth byte of 2nd SRC wrd
        stb     r.6,-7(r.3)             // Put third byte of 2nd DEST wrd
        stb     r.7,-8(r.3)             // Put fourth byte of 2nd DST wrd
        lbz     r.6,-9(r.4)             // Get first byte of 3rd SRC wrd
        lbz     r.7,-10(r.4)            // Get second byte of 3rd SRC wrd
        stb     r.6,-9(r.3)             // Put first byte of 3rd DST wrd
        stb     r.7,-10(r.3)            // Put second byte of 3rd DST wrd
        lbz     r.6,-11(r.4)            // Get third byte of 3rd SRC wrd
        lbz     r.7,-12(r.4)            // Get fourth byte of 3rd SRC wrd
        stb     r.6,-11(r.3)            // Put third byte of 3rd DEST wrd
        stb     r.7,-12(r.3)            // Put fourth byte of 3rd DST wrd
        lbz     r.6,-13(r.4)            // Get first byte of 4th SRC wrd
        lbz     r.7,-14(r.4)            // Get second byte of 4th SRC wrd
        stb     r.6,-13(r.3)            // Put first byte of 4th DEST wrd
        stb     r.7,-14(r.3)            // Put second byte of 4th DST wrd
        lbz     r.6,-15(r.4)            // Get third byte of 4th SRC wrd
        lbz     r.7,-16(r.4)            // Get fourth byte of 4th SRC wrd
        stb     r.6,-15(r.3)            // Put third byte of 4th DEST wrd
        stb     r.7,-16(r.3)            // Put fourth byte of 4th DST wrd
        lbz     r.6,-17(r.4)            // Get first byte of 5th SRC wrd
        lbz     r.7,-18(r.4)            // Get second byte of 5th SRC wrd
        stb     r.6,-17(r.3)            // Put first byte of 5th DEST wrd
        stb     r.7,-18(r.3)            // Put second byte of 5th DST wrd
        lbz     r.6,-19(r.4)            // Get third byte of 5th SRC wrd
        lbz     r.7,-20(r.4)            // Get fourth byte of 5th SRC wrd
        stb     r.6,-19(r.3)            // Put third byte of 5th DEST wrd
        stb     r.7,-20(r.3)            // Put fourth byte of 5th DST wrd
        lbz     r.6,-21(r.4)            // Get first byte of 6th SRC wrd
        lbz     r.7,-22(r.4)            // Get second byte of 6th SRC wrd
        stb     r.6,-21(r.3)            // Put first byte of 6th DEST wrd
        stb     r.7,-22(r.3)            // Put second byte of 6th DST wrd
        lbz     r.6,-23(r.4)            // Get third byte of 6th SRC wrd
        lbz     r.7,-24(r.4)            // Get fourth byte of 6th SRC wrd
        stb     r.6,-23(r.3)            // Put third byte of 6th DEST wrd
        stb     r.7,-24(r.3)            // Put fourth byte of 6th DST wrd
        lbz     r.6,-25(r.4)            // Get first byte of 7th SRC wrd
        lbz     r.7,-26(r.4)            // Get second byte of 7th SRC wrd
        stb     r.6,-25(r.3)            // Put first byte of 7th DEST wrd
        stb     r.7,-26(r.3)            // Put second byte of 7th DST wrd
        lbz     r.6,-27(r.4)            // Get third byte of 7th SRC wrd
        lbz     r.7,-28(r.4)            // Get fourth byte of 7th SRC wrd
        stb     r.6,-27(r.3)            // Put third byte of 7th DEST wrd
        stb     r.7,-28(r.3)            // Put fourth byte of 7th DST wrd
        lbz     r.6,-29(r.4)            // Get first byte of 8th SRC wrd
        lbz     r.7,-30(r.4)            // Get second byte of 8th SRC wrd
        stb     r.6,-29(r.3)            // Put first byte of 8th DEST wrd
        stb     r.7,-30(r.3)            // Put second byte of 8th DST wrd
        lbz     r.6,-31(r.4)            // Get third byte of 8th SRC wrd
        lbz     r.7,-32(r.4)            // Get fourth byte of 8th SRC wrd
        subi    r.4,r.4,32              // Update SRC pointer
        cmpw    r.4,r.10                // Check for all blocks done
        stb     r.6,-31(r.3)            // Put third byte of 8th DEST wrd
        stb     r.7,-32(r.3)            // Put fourth byte of 8th DST wrd
        subi    r.3,r.3,32              // Update DEST pointer
        bne+    BckMvBlksByByte         // Jump if more blocks
//
// Move 4-byte blocks with DEST or SRC Byte aligned, but not the other
//

BckMvBlksOf4Bytes:
        andi.   r.6,r.5,4-1             // Isolate remainder of LNGTH/4
        sub.    r.7,r.5,r.6             // Get 4-byte block count
        sub     r.10,r.4,r.7            // Get address of last full block
        beq-    BckMoveByByte           // Jump if no 4-byte blocks
        mr      r.5,r.6                 // Set Length = remainder

BckBlksLpOn4Bytes:
        lbz     r.6,-1(r.4)             // Get first byte of 1st SRC wrd
        lbz     r.7,-2(r.4)             // Get second byte of 1st SRC wrd
        stb     r.6,-1(r.3)             // Put first byte of 1st DEST wrd
        stb     r.7,-2(r.3)             // Put second byte of 1st DST wrd
        lbz     r.6,-3(r.4)             // Get third byte of 1st SRC wrd
        lbz     r.7,-4(r.4)             // Get fourth byte of 1st SRC wrd
        subi    r.4,r.4,4               // Update SRC pointer
        cmpw    r.4,r.10                // Check for last block
        stb     r.6,-3(r.3)             // Put third byte of 1st DEST wrd
        stb     r.7,-4(r.3)             // Put fourth byte of 1st DST wrd
        subi    r.3,r.3,4               // Update DEST pointer
        bne+    BckBlksLpOn4Bytes       // Jump if more blocks

        b       BckMoveByByte           // Jump to complete last bytes
//
// Exit the routine
//
MvExit:
        LEAF_EXIT(RtlMoveMemory)

//++
//
// VOID
// RtlZeroMemory (
//    IN PVOID Destination,
//    IN ULONG Length
//    )
//
// Routine Description:
//
//    This function zeros memory by first aligning the destination
//    address to a longword boundary, and then zeroing 32-byte blocks,
//    followed by 4-byte blocks, followed by any remaining bytes.
//
// Arguments:
//
//    DEST (r.3) - Supplies a pointer to the memory to zero.
//
//    LENGTH (r.4) - Supplies the length, in bytes, of the memory to be
//                   zeroed.
//
// Return Value:
//
//    None.
//
//--
//
// Define the entry point
//
        LEAF_ENTRY(RtlZeroMemory)
//
// Fill Memory with the zeros
//
// Zero extra bytes until a word boundary is reached
//
        cmpwi   cr.1,r.4,4              // Check for less than 3 bytes
        mtcrf   0x01,r.3                // Check alignment type
        li      r.5,0                   // Set pattern as 0
        blt-    cr.1,ZeroByByte         // Jump to handle small cases
        li      r.6,1
ZeroMem:
        bt      31,ZeroOdd              // Branch if 1 or 3
        bf      30,ZBlkDiv              // Branch if not 2
        sth     r.5,0(r.3)              // Store unaligned halfword
        li      r.6,2
        b       ZUpdteAddr              // Jump to update addresses
ZeroOdd:
        bt      30,Zero1                // Branch if align 3
        sth     r.5,1(r.3)              // Store unaligned halfword
        li      r.6,3
Zero1:
        stb     r.5,0(r.3)              // Store unaligned byte
ZUpdteAddr:
        sub     r.4,r.4,r.6             // Decrement LENGTH by unaligned
        add     r.3,r.3,r.6             // Update the DEST address
//
// Divide the block to process into 32-byte blocks
//
ZBlkDiv:
        andi.   r.6,r.4,BLKLN-1         // Isolate remainder of LENGTH/32
        sub.    r.7,r.4,r.6             // Get full block count
        add     r.10,r.3,r.7            // Get address of last full block
        beq-    ZeroBy4Bytes            // Jump if no full blocks
        mr      r.4,r.6                 // Set Length = Remainder
//
// Zero 32-Byte Blocks
//
// Check for 32-Byte Boundary, if so use the cache zero
//
        andi.   r.9,r.3,31              // Check for cache boundary
        li      r.6,0                   // Set offset=0
        beq+    BlkZeroC                // Jump if on cache boundary
//
// If not 32-byte boundary, fill to 32-bit boundary then use cache zero
//
        srwi    r.8,r.7,5               // Get block count
        cmpwi   r.8,1                   // Check for single block
        mr      r.12,r.9                // Save offset value
        li      r11,32                  // Get full block count
        sub     r9,r.11,r.9             // Get distance to cache boundary
        beq-    BlkZero                 // Jump if single block
//
// Adjust pointers and loop counts
//
        sub.    r.8,r.4,r.9             // TMP=Remainder-Unaligned Count
        add     r.10,r.10,r.9           // Get new end pointer
        mr      r.4,r.8                 // Set new remainder count (TMP)
        bge+    AlignToCache            // Jump if TMP >= 0
        sub     r.10,r.10,r.9           // Subtract previous increment
        add     r4,r.11,r8              // Get new rem cnt (32-abs(TMP))
        sub     r.10,r.10,r.12          // Get new end pointer
//
// Fill to 32-byte boundary - Using 4-byte blocks
//
AlignToCache:
        andi.   r.8,r.9,3               // Isolate remainder of LENGTH/4
        sub.    r.9,r.9,r.8             // Get full word byte count
        li      r.7,4                   // Initialize loop decrement
        beq-    ByteAlignToCache        // Jump if no full blocks
//
Align4Bytes:
        stw     r.5,0(r.3)
        sub.    r.9,r.9,r.7             // Increment the loop counter
        addi    r.3,r.3,4               // Increment the DEST address
        bne+    Align4Bytes             // Jump if more 4-Byte Blk fills
//
// Align to cache boundary using 1-Byte Blocks
//
ByteAlignToCache:
        cmpwi   r.8,0                   // Check for completion
        add     r.3,r.3,r.8             // Update DEST address
        beq+    BlkZeroC                // Jump if cache aligned
//
        cmpwi   r.8,1                   // Check for done
        stb     r.5,0(r.3)              // Zero 1 byte
        beq+    BlkZeroC                // Jump if done
        cmpwi   r.8,2                   // Check cache aligned
        stb     r.5,1(r.3)              // Zero 1 byte
        beq+    BlkZeroC                // Jump cache aligned
        stb     r.5,2(r.3)              // Zero 1 Byte
//
// Zero using the cache
//
BlkZeroC:
#if 0 // BLDR_KERNEL_RUNTIME != 1
//
// In order to allow us to boot in write-through or cache-inhibited
// mode, the boot loader does not use dcbz.
//

        dcbz    r.6,r.3                 // Zero 32-byte cache block
        addi    r.3,r.3,32              // Increment the DEST address
        cmpw    r.3,r.10                // Check for completion
        bne+    BlkZeroC                // Jump if more 32-Byte Blk fills
        b       ZeroBy4Bytes            // Jump to finish
#endif
//
// Zero using normal stores
//
BlkZero:
        stw     r.5,0(r.3)              // Store the 1st DEST word
        stw     r.5,4(r.3)              // Store the 2nd DEST word
        stw     r.5,8(r.3)              // Store the 3rd DEST word
        stw     r.5,12(r.3)             // Store the 4th DEST word
        stw     r.5,16(r.3)             // Store the 5th DEST word
        stw     r.5,20(r.3)             // Store the 6th DEST word
        stw     r.5,24(r.3)             // Store the 7th DEST word
        stw     r.5,28(r.3)             // Store the 8th DEST word
        addi    r.3,r.3,32              // Increment the DEST address
        cmpw    r.3,r.10                // Check for completion
        bne+    BlkZero                 // Jump if more 32-Byte Blk fills
//
// Zero 4-Byte Blocks
//
ZeroBy4Bytes:
        andi.   r.6,r.4,3               // Isolate remainder of LENGTH/4
        sub.    r.7,r.4,r.6             // Get full block count
        add     r.10,r.3,r.7            // Get address of last full block
        beq-    ZeroByByte              // Jump if no full blocks
        mr      r.4,r.6                 // Set Length = Remainder
//
Zero4Bytes:
        stw     r.5,0(r.3)
        addi    r.3,r.3,4               // Increment the DEST address
        cmpw    r.3,r.10                // Check for completion
        bne+    Zero4Bytes              // Jump if more 4-Byte Blk fills
//
// Zero 1-Byte Blocks
//
ZeroByByte:
        cmpwi   r.4,0                   // Check for completion
        beqlr+                          // Return if done
//
Zero1Byte:
        cmpwi   r.4,1                   // Check for done
        stb     r.5,0(r.3)              // Zero 1 byte
        beqlr+                          // Return if done
        cmpwi   r.4,2                   // Check for done
        stb     r.5,1(r.3)              // Zero 1 byte
        beqlr+                          // Return if done
        stb     r.5,2(r.3)              // Zero 1 Byte
//
// Exit
//
ZeroExit:
        LEAF_EXIT(RtlZeroMemory)
//
//++
//
// VOID
// RtlFillMemory (
//    IN PVOID Destination,
//    IN ULONG Length,
//    IN UCHAR Fill
//    )
//
// Routine Description:
//
//    This function fills memory by first aligning the destination
//    address to a longword boundary, and then filling 32-byte blocks,
//    followed by 4-byte blocks, followed by any remaining bytes.
//
// Arguments:
//
//    DEST (r.3) - Supplies a pointer to the memory to fill.
//
//    LENGTH (r.4) - Supplies the length, in bytes, of the memory to be
//                   filled.
//
//    PTTRN (r.5) - Supplies the fill byte.
//
// Return Value:
//
//    None.
//
//--
//
//  Define the entry point
//
        LEAF_ENTRY(RtlFillMemory)

        cmpwi   cr.1,r.4,4              // Check for less than 4 bytes

//
//  Initialize a register with the fill byte duplicated
//

        rlwimi  r.5,r.5,8,0x0000ff00    // propogate rightmost byte
        rlwimi. r.5,r.5,16,0xffff0000   // thru upper 3 bytes

//
// Fill Memory with the pattern
//
//
// Fill extra bytes until a word boundary is reached
//

        mtcrf   0x01,r.3                // Check alignment type
        blt-    cr.1,FillByByte         // Jump to handle small cases
        li      r.6,1                   // Default unaligned count to 1 byte
        beq-    ZeroMem                 // Use RtlZeroMemory if fill 0
        bt      31,FillOdd              // Branch if align 1 or 3
        bf      30,BlkDiv               // Branch if not 2
        sth     r.5,0(r.3)              // Store unaligned halfword
        li      r.6,2                   // Set count to 2 bytes
        b       UpdteAddr               // Jump to update addresses
FillOdd:
        bt      30,Fill1                // Branch if align 3
        sth     r.5,1(r.3)              // Store unaligned halfword
        li      r.6,3                   // Set count to 3 bytes
Fill1:
        stb     r.5,0(r.3)              // Store unaligned byte
UpdteAddr:
        sub     r.4,r.4,r.6             // Decrement LENGTH by unaligned
        add     r.3,r.3,r.6             // Update the DEST address
//
// Divide the block to process into 32-byte blocks
//

BlkDiv:
        andi.   r.6,r.4,BLKLN-1         // Isolate remainder of LENGTH/32
        sub.    r.7,r.4,r.6             // Get full block count
        add     r.10,r.3,r.7            // Get address of last full block
        beq-    FillBy4Bytes            // Jump if no full blocks
        mr      r.4,r.6                 // Set Length = Remainder
//
// Fill 32-Byte Blocks
//
BlkFill:
        stw     r.5,0(r.3)              // Store the 1st DEST word
        stw     r.5,4(r.3)              // Store the 2nd DEST word
        stw     r.5,8(r.3)              // Store the 3rd DEST word
        stw     r.5,12(r.3)             // Store the 4th DEST word
        stw     r.5,16(r.3)             // Store the 5th DEST word
        stw     r.5,20(r.3)             // Store the 6th DEST word
        stw     r.5,24(r.3)             // Store the 7th DEST word
        stw     r.5,28(r.3)             // Store the 8th DEST word
        addi    r.3,r.3,32              // Increment the DEST address
        cmpw    r.3,r.10                // Check for completion
        bne+    BlkFill                 // Jump if more 32-Byte Blk fills
//
// Fill 4-Byte Blocks
//
FillBy4Bytes:
        andi.   r.6,r.4,3               // Isolate remainder of LENGTH/4
        sub.    r.7,r.4,r.6             // Get full block count
        add     r.10,r.3,r.7            // Get address of last full block
        beq-    FillByByte              // Jump if no full blocks
        mr      r.4,r.6                 // Set Length = Remainder
//
Fill4Bytes:
        stw     r.5,0(r.3)
        addi    r.3,r.3,4               // Increment the DEST address
        cmpw    r.3,r.10                // Check for completion
        bne+    Fill4Bytes              // Jump if more 4-Byte Blk fills
//
// Fill 1-Byte Blocks
//
FillByByte:
        cmpwi   r.4,0                   // Check for completion
        beqlr+                          // Return if done
//
Fill1Byte:
        cmpwi   r.4,1                   // Check for done
        stb     r.5,0(r.3)              // Fill 1 byte
        beqlr+                          // Return if done
        cmpwi   r.4,2                   // Check for done
        stb     r.5,1(r.3)              // Fill 1 byte
        beqlr+                          // Return if done
        stb     r.5,2(r.3)              // Fill 1 Byte
//
// Exit
//
FillExit:
        LEAF_EXIT(RtlFillMemory)

//++
//
// VOID
// RtlFillMemoryUlong (
//    IN PVOID Destination,
//    IN ULONG Length,
//    IN ULONG Pattern
//    )
//
// Routine Description:
//
//    This function fills memory with the specified longowrd pattern by
//    filling 32-byte blocks followed by 4-byte blocks.
//
//    N.B. This routine assumes that the destination address is aligned
//         on a longword boundary and that the length is an even multiple
//         of longwords.
//
// Arguments:
//
//    DEST (r.3) - Supplies a pointer to the memory to fill.
//
//    LENGTH (r.4) - Supplies the length, in bytes, of the memory to be
//                   filled.
//
//    PTTRN (r.5) - Supplies the fill pattern.
//
// Return Value:
//
//    None.
//
//--
//
// Define the entry point
//
        LEAF_ENTRY(RtlFillMemoryUlong)
//
// Make sure length is even number of longwords
//
        srwi    r.4,r.4,2               // Shift length to divide by 4
        slwi    r.4,r.4,2               // Make sure LENGTH is even
//
// Divide the block to process into 32-byte blocks
//
        andi.   r.6,r.4,BLKLN-1         // Isolate remainder of LNGTH/32
        sub.    r.7,r.4,r.6             // Get full block count
        add     r.10,r.3,r.7            // Get address of last full block
        beq-    FillUlBy4Bytes          // Jump if no full blocks
        mr      r.4,r.6                 // Set Length = Remainder
//
// Fill 32-Byte Blocks
//
BlkFillUl:
        stw     r.5,0(r.3)              // Store the 1st DEST word
        stw     r.5,4(r.3)              // Store the 2nd DEST word
        stw     r.5,8(r.3)              // Store the 3rd DEST word
        stw     r.5,12(r.3)             // Store the 4th DEST word
        stw     r.5,16(r.3)             // Store the 5th DEST word
        stw     r.5,20(r.3)             // Store the 6th DEST word
        stw     r.5,24(r.3)             // Store the 7th DEST word
        stw     r.5,28(r.3)             // Store the 8th DEST word
        addi    r.3,r.3,32              // Increment the DEST address
        cmpw    r.3,r.10                // Check for completion
        bne+    BlkFillUl               // Jump if more 32-Byte Blk fills
//
// Fill 4-Byte Blocks
//
FillUlBy4Bytes:
        andi.   r.6,r.4,3               // Isolate remainder of LENGTH/4
        sub.    r.7,r.4,r.6             // Get full block count
        add     r.10,r.3,r.7            // Get address of last full block
        beqlr-                          // Return if done
        mr      r.4,r.6                 // Set Length = Remainder
//
FillUl4Bytes:
        stw     r.5,0(r.3)
        addi    r.3,r.3,4               // Increment the DEST address
        cmpw    r.3,r.10                // Check for completion
        bne+    FillUl4Bytes            // Jump if more 4-Byte Blk fills
//
// Exit
//
FillUlExit:
        LEAF_EXIT(RtlFillMemoryUlong)

