/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

   ldrreloc.c

Abstract:

    This module contains the code to relocate an image when
    the preferred base isn't available. This is called by the
    boot loader, device driver loader, and system loader.

Author:

    Mike O'Leary (mikeol) 03-Feb-1992

Revision History:

--*/

#include "ntrtlp.h"

//
// byte swapping macros (LE/BE) used for IA64 relocations
// source != destination
//

#define SWAP_SHORT(_dst,_src)                                                  \
   ((((unsigned char *)_dst)[1] = ((unsigned char *)_src)[0]),                 \
    (((unsigned char *)_dst)[0] = ((unsigned char *)_src)[1]))

#define SWAP_INT(_dst,_src)                                                    \
   ((((unsigned char *)_dst)[3] = ((unsigned char *)_src)[0]),                 \
    (((unsigned char *)_dst)[2] = ((unsigned char *)_src)[1]),                 \
    (((unsigned char *)_dst)[1] = ((unsigned char *)_src)[2]),                 \
    (((unsigned char *)_dst)[0] = ((unsigned char *)_src)[3]))

#define SWAP_LONG_LONG(_dst,_src)                                              \
   ((((unsigned char *)_dst)[7] = ((unsigned char *)_src)[0]),                 \
    (((unsigned char *)_dst)[6] = ((unsigned char *)_src)[1]),                 \
    (((unsigned char *)_dst)[5] = ((unsigned char *)_src)[2]),                 \
    (((unsigned char *)_dst)[4] = ((unsigned char *)_src)[3]),                 \
    (((unsigned char *)_dst)[3] = ((unsigned char *)_src)[4]),                 \
    (((unsigned char *)_dst)[2] = ((unsigned char *)_src)[5]),                 \
    (((unsigned char *)_dst)[1] = ((unsigned char *)_src)[6]),                 \
    (((unsigned char *)_dst)[0] = ((unsigned char *)_src)[7]))

//
// Mark a HIGHADJ entry as needing an increment if reprocessing.
//
#define LDRP_RELOCATION_INCREMENT   0x1

//
// Mark a HIGHADJ entry as not suitable for reprocessing.
//
#define LDRP_RELOCATION_FINAL       0x2

#if defined(NTOS_KERNEL_RUNTIME)
#if defined(ALLOC_PRAGMA)

ULONG
LdrDoubleRelocateImage (
    IN PVOID NewBase,
    IN PVOID CurrentBase,
    IN PUCHAR LoaderName,
    IN ULONG Success,
    IN ULONG Conflict,
    IN ULONG Invalid
    );

PIMAGE_BASE_RELOCATION
LdrpProcessVolatileRelocationBlock(
    IN ULONG_PTR VA,
    IN ULONG SizeOfBlock,
    IN PUSHORT NextOffset,
    IN LONG_PTR Diff,
    IN LONG_PTR OldDiff,
    IN ULONG_PTR OldBase
    );

#pragma alloc_text(PAGE,LdrRelocateImage)
#pragma alloc_text(PAGE,LdrProcessRelocationBlock)
#pragma alloc_text(INIT,LdrDoubleRelocateImage)
#pragma alloc_text(INIT,LdrpProcessVolatileRelocationBlock)
#endif
#endif

ULONG
LdrRelocateImage (
    IN PVOID NewBase,
    IN PUCHAR LoaderName,
    IN ULONG Success,
    IN ULONG Conflict,
    IN ULONG Invalid
    )

/*++

Routine Description:

    This routine relocates an image file that was not loaded into memory
    at the preferred address.

Arguments:

    NewBase - Supplies a pointer to the image base.

    LoaderName - Indicates which loader routine is being called from.

    Success - Value to return if relocation successful.

    Conflict - Value to return if can't relocate.

    Invalid - Value to return if relocations are invalid.

Return Value:

    Success if image is relocated.
    Conflict if image can't be relocated.
    Invalid if image contains invalid fixups.

--*/

{
    LONG_PTR Diff;
    ULONG TotalCountBytes;
    ULONG_PTR VA;
    ULONG_PTR OldBase;
    ULONG SizeOfBlock;
    PUCHAR FixupVA;
    USHORT Offset;
    PUSHORT NextOffset;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_BASE_RELOCATION NextBlock;

    RTL_PAGED_CODE();

    NtHeaders = RtlImageNtHeader( NewBase );
    if ( NtHeaders ) {
        OldBase = NtHeaders->OptionalHeader.ImageBase;
        }
    else {
        return Invalid;
        }

    //
    // Locate the relocation section.
    //

    NextBlock = (PIMAGE_BASE_RELOCATION)RtlImageDirectoryEntryToData(
            NewBase, TRUE, IMAGE_DIRECTORY_ENTRY_BASERELOC, &TotalCountBytes);

    if (!NextBlock || !TotalCountBytes) {

        //
        // The image does not contain a relocation table, and therefore
        // cannot be relocated.
        //
#if DBG
        DbgPrint("%s: Image can't be relocated, no fixup information.\n", LoaderName);
#endif // DBG
        return Conflict;
    }

    //
    // If the image has a relocation table, then apply the specified fixup
    // information to the image.
    //

    while (TotalCountBytes) {
        SizeOfBlock = NextBlock->SizeOfBlock;
        TotalCountBytes -= SizeOfBlock;
        SizeOfBlock -= sizeof(IMAGE_BASE_RELOCATION);
        SizeOfBlock /= sizeof(USHORT);
        NextOffset = (PUSHORT)((PCHAR)NextBlock + sizeof(IMAGE_BASE_RELOCATION));

        VA = (ULONG_PTR)NewBase + NextBlock->VirtualAddress;
        Diff = (PCHAR)NewBase - (PCHAR)OldBase;

        if ( !(NextBlock = LdrProcessRelocationBlock(VA,SizeOfBlock,NextOffset,Diff)) ) {
#if DBG
            DbgPrint("%s: Unknown base relocation type\n", LoaderName);
#endif
            return Invalid;
        }
    }

    return Success;
}

PIMAGE_BASE_RELOCATION
LdrProcessRelocationBlock(
    IN ULONG_PTR VA,
    IN ULONG SizeOfBlock,
    IN PUSHORT NextOffset,
    IN LONG_PTR Diff
    )
{
    PUCHAR FixupVA;
    USHORT Offset;
    LONG Temp;
    LONG TempOrig;
    ULONG Temp32;
    ULONGLONG Value64;
    LONGLONG Temp64;
    LONG_PTR ActualDiff;

    RTL_PAGED_CODE();

    while (SizeOfBlock--) {

       Offset = *NextOffset & (USHORT)0xfff;
       FixupVA = (PUCHAR)(VA + Offset);

       //
       // Apply the fixups.
       //

       switch ((*NextOffset) >> 12) {

            case IMAGE_REL_BASED_HIGHLOW :
                //
                // HighLow - (32-bits) relocate the high and low half
                //      of an address.
                //
                *(LONG UNALIGNED *)FixupVA += (ULONG) Diff;
                break;

            case IMAGE_REL_BASED_HIGH :
                //
                // High - (16-bits) relocate the high half of an address.
                //
                Temp = *(PUSHORT)FixupVA << 16;
                Temp += (ULONG) Diff;
                *(PUSHORT)FixupVA = (USHORT)(Temp >> 16);
                break;

            case IMAGE_REL_BASED_HIGHADJ :
                //
                // Adjust high - (16-bits) relocate the high half of an
                //      address and adjust for sign extension of low half.
                //

#if defined(NTOS_KERNEL_RUNTIME)
                //
                // If the address has already been relocated then don't
                // process it again now or information will be lost.
                //
                if (Offset & LDRP_RELOCATION_FINAL) {
                    ++NextOffset;
                    --SizeOfBlock;
                    break;
                }
#endif

                Temp = *(PUSHORT)FixupVA << 16;
#if defined(BLDR_KERNEL_RUNTIME)
                TempOrig = Temp;
#endif
                ++NextOffset;
                --SizeOfBlock;
                Temp += (LONG)(*(PSHORT)NextOffset);
                Temp += (ULONG) Diff;
                Temp += 0x8000;
                *(PUSHORT)FixupVA = (USHORT)(Temp >> 16);

#if defined(BLDR_KERNEL_RUNTIME)
                ActualDiff = ((((ULONG_PTR)(Temp - TempOrig)) >> 16) -
                              (((ULONG_PTR)Diff) >> 16 ));

                if (ActualDiff == 1) {
                    //
                    // Mark the relocation as needing an increment if it is
                    // relocated again.
                    //
                    *(NextOffset - 1) |= LDRP_RELOCATION_INCREMENT;
                }
                else if (ActualDiff != 0) {
                    //
                    // Mark the relocation as cannot be reprocessed.
                    //
                    *(NextOffset - 1) |= LDRP_RELOCATION_FINAL;
                }
#endif

                break;

            case IMAGE_REL_BASED_LOW :
                //
                // Low - (16-bit) relocate the low half of an address.
                //
                Temp = *(PSHORT)FixupVA;
                Temp += (ULONG) Diff;
                *(PUSHORT)FixupVA = (USHORT)Temp;
                break;

            case IMAGE_REL_BASED_IA64_IMM64:

                //
                // Align it to bundle address before fixing up the
                // 64-bit immediate value of the movl instruction.
                //

                FixupVA = (PUCHAR)((ULONG_PTR)FixupVA & ~(15));
                Value64 = (ULONGLONG)0;

                //
                // Extract the lower 32 bits of IMM64 from bundle
                //


                EXT_IMM64(Value64,
                        (PULONG)FixupVA + EMARCH_ENC_I17_IMM7B_INST_WORD_X,
                        EMARCH_ENC_I17_IMM7B_SIZE_X,
                        EMARCH_ENC_I17_IMM7B_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM7B_VAL_POS_X);
                EXT_IMM64(Value64,
                        (PULONG)FixupVA + EMARCH_ENC_I17_IMM9D_INST_WORD_X,
                        EMARCH_ENC_I17_IMM9D_SIZE_X,
                        EMARCH_ENC_I17_IMM9D_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM9D_VAL_POS_X);
                EXT_IMM64(Value64,
                        (PULONG)FixupVA + EMARCH_ENC_I17_IMM5C_INST_WORD_X,
                        EMARCH_ENC_I17_IMM5C_SIZE_X,
                        EMARCH_ENC_I17_IMM5C_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM5C_VAL_POS_X);
                EXT_IMM64(Value64,
                        (PULONG)FixupVA + EMARCH_ENC_I17_IC_INST_WORD_X,
                        EMARCH_ENC_I17_IC_SIZE_X,
                        EMARCH_ENC_I17_IC_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IC_VAL_POS_X);
                EXT_IMM64(Value64,
                        (PULONG)FixupVA + EMARCH_ENC_I17_IMM41a_INST_WORD_X,
                        EMARCH_ENC_I17_IMM41a_SIZE_X,
                        EMARCH_ENC_I17_IMM41a_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM41a_VAL_POS_X);

                //
                // Update 64-bit address
                //

                Value64+=Diff;

                //
                // Insert IMM64 into bundle
                //

                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IMM7B_INST_WORD_X),
                        EMARCH_ENC_I17_IMM7B_SIZE_X,
                        EMARCH_ENC_I17_IMM7B_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM7B_VAL_POS_X);
                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IMM9D_INST_WORD_X),
                        EMARCH_ENC_I17_IMM9D_SIZE_X,
                        EMARCH_ENC_I17_IMM9D_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM9D_VAL_POS_X);
                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IMM5C_INST_WORD_X),
                        EMARCH_ENC_I17_IMM5C_SIZE_X,
                        EMARCH_ENC_I17_IMM5C_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM5C_VAL_POS_X);
                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IC_INST_WORD_X),
                        EMARCH_ENC_I17_IC_SIZE_X,
                        EMARCH_ENC_I17_IC_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IC_VAL_POS_X);
                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IMM41a_INST_WORD_X),
                        EMARCH_ENC_I17_IMM41a_SIZE_X,
                        EMARCH_ENC_I17_IMM41a_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM41a_VAL_POS_X);
                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IMM41b_INST_WORD_X),
                        EMARCH_ENC_I17_IMM41b_SIZE_X,
                        EMARCH_ENC_I17_IMM41b_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM41b_VAL_POS_X);
                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_IMM41c_INST_WORD_X),
                        EMARCH_ENC_I17_IMM41c_SIZE_X,
                        EMARCH_ENC_I17_IMM41c_INST_WORD_POS_X,
                        EMARCH_ENC_I17_IMM41c_VAL_POS_X);
                INS_IMM64(Value64,
                        ((PULONG)FixupVA + EMARCH_ENC_I17_SIGN_INST_WORD_X),
                        EMARCH_ENC_I17_SIGN_SIZE_X,
                        EMARCH_ENC_I17_SIGN_INST_WORD_POS_X,
                        EMARCH_ENC_I17_SIGN_VAL_POS_X);
                break;

            case IMAGE_REL_BASED_DIR64:

                *(ULONG_PTR UNALIGNED *)FixupVA += Diff;

                break;

            case IMAGE_REL_BASED_MIPS_JMPADDR :
                //
                // JumpAddress - (32-bits) relocate a MIPS jump address.
                //
                Temp = (*(PULONG)FixupVA & 0x3ffffff) << 2;
                Temp += (ULONG) Diff;
                *(PULONG)FixupVA = (*(PULONG)FixupVA & ~0x3ffffff) |
                                                ((Temp >> 2) & 0x3ffffff);

                break;

            case IMAGE_REL_BASED_ABSOLUTE :
                //
                // Absolute - no fixup required.
                //
                break;

            case IMAGE_REL_BASED_SECTION :
                //
                // Section Relative reloc.  Ignore for now.
                //
                break;

            case IMAGE_REL_BASED_REL32 :
                //
                // Relative intrasection. Ignore for now.
                //
                break;

           case IMAGE_REL_BASED_HIGH3ADJ :
               //
               // Similar to HIGHADJ except this is the third word.
               //  Adjust low half of high dword of an address and adjust for
               //   sign extension of the low dword.
               //

               Temp64 = *(PUSHORT)FixupVA << 16;
               ++NextOffset;
               --SizeOfBlock;
               Temp64 += (LONG)((SHORT)NextOffset[1]);
               Temp64 <<= 16;
               Temp64 += (LONG)((USHORT)NextOffset[0]);
               Temp64 += Diff;
               Temp64 += 0x8000;
               Temp64 >>=16;
               Temp64 += 0x8000;
               *(PUSHORT)FixupVA = (USHORT)(Temp64 >> 16);
               ++NextOffset;
               --SizeOfBlock;
               break;

            default :
                //
                // Illegal - illegal relocation type.
                //

                return (PIMAGE_BASE_RELOCATION)NULL;
       }
       ++NextOffset;
    }
    return (PIMAGE_BASE_RELOCATION)NextOffset;
}

#if defined(NTOS_KERNEL_RUNTIME)

ULONG
LdrDoubleRelocateImage (
    IN PVOID NewBase,
    IN PVOID CurrentBase,
    IN PUCHAR LoaderName,
    IN ULONG Success,
    IN ULONG Conflict,
    IN ULONG Invalid
    )

/*++

Routine Description:

    This routine handles the volatile relocations that cannot be easily repeated
    on an image file that has already been relocated at least once.

    Since this only needs to be done once (at kernel startup time), the
    decision was made to split this into a separate routine so as not to
    impact the mainline code.

    N.B. This function is for use by memory management ONLY.

Arguments:

    NewBase - Supplies a pointer to the new (second relocated) image base.

    CurrentBase - Supplies a pointer to the first relocated image base.

    LoaderName - Indicates which loader routine is being called from.

    Success - Value to return if relocation successful.

    Conflict - Value to return if can't relocate.

    Invalid - Value to return if relocations are invalid.

Return Value:

    Success if image is relocated.
    Conflict if image can't be relocated.
    Invalid if image contains invalid fixups.

--*/

{
    LONG_PTR Diff;
    LONG_PTR OldDiff;
    ULONG TotalCountBytes;
    ULONG_PTR VA;
    ULONG_PTR OldBase;
    ULONG SizeOfBlock;
    PUCHAR FixupVA;
    USHORT Offset;
    PUSHORT NextOffset;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_BASE_RELOCATION NextBlock;

    RTL_PAGED_CODE();

    NtHeaders = RtlImageNtHeader( NewBase );

    OldBase = NtHeaders->OptionalHeader.ImageBase;
    OldDiff = (PCHAR)CurrentBase - (PCHAR)OldBase;

    //
    // Locate the relocation section.
    //

    NextBlock = (PIMAGE_BASE_RELOCATION)RtlImageDirectoryEntryToData(
            NewBase, TRUE, IMAGE_DIRECTORY_ENTRY_BASERELOC, &TotalCountBytes);

    if (!NextBlock || !TotalCountBytes) {

        //
        // The image does not contain a relocation table, and therefore
        // cannot be relocated.
        //
#if DBG
        DbgPrint("%s: Image can't be relocated, no fixup information.\n", LoaderName);
#endif // DBG
        return Conflict;
    }

    //
    // If the image has a relocation table, then apply the specified fixup
    // information to the image.
    //

    Diff = (PCHAR)NewBase - (PCHAR)OldBase;

    while (TotalCountBytes) {
        SizeOfBlock = NextBlock->SizeOfBlock;
        TotalCountBytes -= SizeOfBlock;
        SizeOfBlock -= sizeof(IMAGE_BASE_RELOCATION);
        SizeOfBlock /= sizeof(USHORT);
        NextOffset = (PUSHORT)((PCHAR)NextBlock + sizeof(IMAGE_BASE_RELOCATION));

        VA = (ULONG_PTR)NewBase + NextBlock->VirtualAddress;

        if ( !(NextBlock = LdrpProcessVolatileRelocationBlock(VA,SizeOfBlock,NextOffset,Diff, OldDiff, OldBase)) ) {
#if DBG
            DbgPrint("%s: Unknown base relocation type\n", LoaderName);
#endif
            return Invalid;
        }
    }

    return Success;
}

PIMAGE_BASE_RELOCATION
LdrpProcessVolatileRelocationBlock(
    IN ULONG_PTR VA,
    IN ULONG SizeOfBlock,
    IN PUSHORT NextOffset,
    IN LONG_PTR Diff,
    IN LONG_PTR OldDiff,
    IN ULONG_PTR OldBase
    )

/*++

Routine Description:

    This routine handles the volatile relocations that cannot be easily repeated
    on an image file that has already been relocated at least once.

    Since this only needs to be done once (at kernel startup time), the
    decision was made to split this into a separate routine so as not to
    impact the mainline code.

    N.B. This function is for use by memory management ONLY.

Arguments:

    TBD.

Return Value:

    Next relocation entry to process.

--*/

{
    PUCHAR FixupVA;
    USHORT Offset;
    LONG Temp;
    ULONG Temp32;
    USHORT TempShort1;
    USHORT TempShort2;
    ULONGLONG Value64;
    LONGLONG Temp64;
    USHORT RelocationType;
    IN PVOID CurrentBase;

    RTL_PAGED_CODE();

    CurrentBase = (PVOID)((ULONG_PTR)OldDiff + OldBase);

    while (SizeOfBlock--) {

       Offset = *NextOffset & (USHORT)0xfff;
       FixupVA = (PUCHAR)(VA + Offset);

       //
       // Apply the fixups.
       //

       switch ((*NextOffset) >> 12) {

            case IMAGE_REL_BASED_HIGHADJ :
                //
                // Adjust high - (16-bits) relocate the high half of an
                //      address and adjust for sign extension of low half.
                //

                //
                // Return the relocation to its original state, checking for
                // whether the entry was sign extended the 1st time it was
                // relocated.
                //
                FixupVA = (PUCHAR)((LONG_PTR)FixupVA & (LONG_PTR)~(LDRP_RELOCATION_FINAL | LDRP_RELOCATION_INCREMENT));
                Temp = *(PUSHORT)(FixupVA) << 16;

                ++NextOffset;
                --SizeOfBlock;

                // remove the carry bit from the low word
                Temp -= ((LONG)(*(PSHORT)NextOffset) + (USHORT)OldDiff + 0x8000) & ~0xFFFF;

                Temp -= (LONG)(OldDiff & ~0xffff);

                Temp += (LONG)(*(PSHORT)NextOffset);
                Temp += (ULONG) Diff;
                Temp += 0x8000;
                *(PUSHORT)FixupVA = (USHORT)(Temp >> 16);

                //
                // Mark the relocation as needing no further reprocessing.
                //
                *(NextOffset - 1) |= LDRP_RELOCATION_FINAL;
                break;

           case IMAGE_REL_BASED_HIGH3ADJ :
               //
               // This type of relocation always results in a no-op when
               // done by the osloader for kernelmode drivers.  But the
               // subsequent relocation must be done very carefully.
               //
               TempShort1 = *(NextOffset + 1);
               TempShort2 = *(NextOffset + 2);

               Temp64 = (LONGLONG)((TempShort2 << 16) + TempShort1);
               Temp64 -= (LONGLONG)OldBase;
               Temp64 += (LONGLONG)CurrentBase;

               TempShort1 = (USHORT)Temp64;
               TempShort2 = (USHORT)(Temp64 >> 16);

               *(NextOffset + 1) = TempShort1;
               *(NextOffset + 2) = TempShort2;

               ++NextOffset;
               --SizeOfBlock;
               ++NextOffset;
               --SizeOfBlock;

               break;

            default :
               break;
       }
       ++NextOffset;
    }
    return (PIMAGE_BASE_RELOCATION)NextOffset;
}

#endif
