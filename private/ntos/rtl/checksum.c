/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    imagedir.c

Abstract:

    The module contains the code to translate an image directory type to
    the address of the data for that entry.

Author:

    Steve Wood (stevewo) 18-Aug-1989

Environment:

    User Mode or Kernel Mode

Revision History:

--*/

#include "ntrtlp.h"

//
// Define forward referenced prootypes.
//

USHORT
ChkSum(
    ULONG PartialSum,
    PUSHORT Source,
    ULONG Length
    );

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#if !defined(_IA64_)
#pragma alloc_text(PAGE,ChkSum)
#endif // !defined(_IA64_)
#pragma alloc_text(PAGE,LdrVerifyMappedImageMatchesChecksum)
#endif

#if !defined(_IA64_)
USHORT
ChkSum(
    ULONG PartialSum,
    PUSHORT Source,
    ULONG Length
    )

/*++

Routine Description:

    Compute a partial checksum on a portion of an imagefile.

Arguments:

    PartialSum - Supplies the initial checksum value.

    Sources - Supplies a pointer to the array of words for which the
        checksum is computed.

    Length - Supplies the length of the array in words.

Return Value:

    The computed checksum value is returned as the function value.

--*/

{

    RTL_PAGED_CODE();

    //
    // Compute the word wise checksum allowing carries to occur into the
    // high order half of the checksum longword.
    //

    while (Length--) {
        PartialSum += *Source++;
        PartialSum = (PartialSum >> 16) + (PartialSum & 0xffff);
    }

    //
    // Fold final carry into a single word result and return the resultant
    // value.
    //

    return (USHORT)(((PartialSum >> 16) + PartialSum) & 0xffff);
}
#endif // !defined(_IA64_)

BOOLEAN
LdrVerifyMappedImageMatchesChecksum (
    IN PVOID BaseAddress,
    IN ULONG FileLength
    )

/*++

Routine Description:

    This functions computes the checksum of an image mapped as a data file.

Arguments:

    BaseAddress - Supplies a pointer to the base of the mapped file.

    FileLength - Supplies the length of the file in bytes.

Return Value:

    TRUE - The checksum stored in the image matches the checksum of the data.

    FALSE - The checksum in the image is not correct.

--*/

{

    PUSHORT AdjustSum;
    PIMAGE_NT_HEADERS NtHeaders;
    USHORT PartialSum;
    ULONG HeaderSum;
    ULONG CheckSum;

    RTL_PAGED_CODE();

    //
    // Compute the checksum of the file and zero the header checksum value.
    //

    HeaderSum = 0;
    PartialSum = ChkSum(0, (PUSHORT)BaseAddress, (FileLength + 1) >> 1);

    //
    // If the file is an image file, then subtract the two checksum words
    // in the optional header from the computed checksum before adding
    // the file length, and set the value of the header checksum.
    //

    NtHeaders = RtlImageNtHeader(BaseAddress);
    if (NtHeaders != NULL) {
        HeaderSum = NtHeaders->OptionalHeader.CheckSum;

#ifndef NTOS_KERNEL_RUNTIME
        //
        // On Nt 3.1 and 3.5, we allowed printer drivers with 0 checksums into
        // csrss unintentionally. This means that we must allow this forever.
        // I don't want to allow this for kernel mode drivers, so I will only
        // allow 0 checksums of the high order bit is clear ?
        //


        if ( HeaderSum == 0 ) {
            return TRUE;
        }
#endif // NTOS_KERNEL_RUNTIME

        AdjustSum = (PUSHORT)(&NtHeaders->OptionalHeader.CheckSum);
        PartialSum -= (PartialSum < AdjustSum[0]);
        PartialSum -= AdjustSum[0];
        PartialSum -= (PartialSum < AdjustSum[1]);
        PartialSum -= AdjustSum[1];
    } else {
        PartialSum = 0;
        HeaderSum = FileLength;
    }

    //
    // Compute the final checksum value as the sum of the paritial checksum
    // and the file length.
    //

    CheckSum = (ULONG)PartialSum + FileLength;
    return (CheckSum == HeaderSum);
}
