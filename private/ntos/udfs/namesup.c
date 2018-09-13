/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    NameSup.c

Abstract:

    This module implements the Udfs Name support routines

Author:

    Dan Lovinger    [DanLo]     9-October-1996

Revision History:

--*/

#include "UdfProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (UDFS_BUG_CHECK_NAMESUP)

//
//  The local debug trace level
//

#define Dbg                              (UDFS_DEBUG_LEVEL_NAMESUP)

//
//  Local constants
//

static CONST CHAR UdfCrcChar[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ#_~-@";

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UdfCandidateShortName)
#pragma alloc_text(PAGE, UdfCheckLegalCS0Dstring)
#pragma alloc_text(PAGE, UdfConvertCS0DstringToUnicode)
#pragma alloc_text(PAGE, UdfDissectName)
#pragma alloc_text(PAGE, UdfFullCompareNames)
#pragma alloc_text(PAGE, UdfGenerate8dot3Name)
#pragma alloc_text(PAGE, UdfIs8dot3Name)
#pragma alloc_text(PAGE, UdfIsNameInExpression)
#pragma alloc_text(PAGE, UdfRenderNameToLegalUnicode)
#endif


INLINE
ULONG
NativeDosCharLength (
    IN WCHAR Wchar
    )

/*++

Routine Description:

    This routine is a translation layer for asking how big a given UNICODE
    character will be when converted to OEM.  Aside from adding more material
    to the kernel export table, this is how ya do it.

Arguments:

    Wchar - pointer to the character

Return Value:

    Size in bytes.

--*/

{
    NTSTATUS Status;
    CHAR OemBuf[2];
    ULONG Length;

    Status = RtlUpcaseUnicodeToOemN( OemBuf,
                                     sizeof(OemBuf),
                                     &Length,
                                     &Wchar,
                                     sizeof(WCHAR));
    
    ASSERT( NT_SUCCESS( Status ));

    return Length;
}


VOID
UdfDissectName (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PUNICODE_STRING RemainingName,
    OUT PUNICODE_STRING FinalName
    )

/*++

Routine Description:

    This routine is called to strip off leading components of the name strings.  We search
    for either the end of the string or separating characters.  The input remaining
    name strings should have neither a trailing or leading backslash.

Arguments:

    RemainingName - Remaining name.

    FinalName - Location to store next component of name.

Return Value:

    None.

--*/

{
    ULONG NameLength;
    PWCHAR NextWchar;

    PAGED_CODE();

    //
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );

    //
    //  Find the offset of the next component separators.
    //

    for (NameLength = 0, NextWchar = RemainingName->Buffer;
         (NameLength < RemainingName->Length) && (*NextWchar != L'\\');
         NameLength += sizeof( WCHAR) , NextWchar += 1);

    //
    //  Adjust all the strings by this amount.
    //

    FinalName->Buffer = RemainingName->Buffer;

    FinalName->MaximumLength = FinalName->Length = (USHORT) NameLength;

    //
    //  If this is the last component then set the RemainingName lengths to zero.
    //

    if (NameLength == RemainingName->Length) {

        RemainingName->Length = 0;

    //
    //  Otherwise we adjust the string by this amount plus the separating character.
    //

    } else {

        RemainingName->MaximumLength -= (USHORT) (NameLength + sizeof( WCHAR ));
        RemainingName->Length -= (USHORT) (NameLength + sizeof( WCHAR ));
        RemainingName->Buffer = Add2Ptr( RemainingName->Buffer,
                                         NameLength + sizeof( WCHAR ),
                                         PWCHAR );
    }

    return;
}


BOOLEAN
UdfIs8dot3Name (
    IN PIRP_CONTEXT IrpContext,
    IN UNICODE_STRING FileName
    )

/*++

Routine Description:

    This routine checks if the name follows the 8.3 name conventions.  We check for
    the name length and whether the characters are valid.

Arguments:

    FileName - String of bytes containing the name.

Return Value:

    BOOLEAN - TRUE if this name is a legal 8.3 name, FALSE otherwise.

--*/

{
    CHAR DbcsNameBuffer[ BYTE_COUNT_8_DOT_3 ];
    STRING DbcsName;

    PWCHAR NextWchar;
    ULONG Count;

    ULONG DotCount = 0;
    BOOLEAN LastCharDot = FALSE;

    PAGED_CODE();

    //
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );

    //
    //  The length must be less than 24 bytes.
    //

    ASSERT( FileName.Length != 0 );
    if (FileName.Length > BYTE_COUNT_8_DOT_3) {

        return FALSE;
    }

    //
    //  Walk though and check for a space character.
    //

    NextWchar = FileName.Buffer;
    Count = 0;

    do {

        //
        //  No spaces allowed.
        //

        if (*NextWchar == L' ') { return FALSE; }

        if (*NextWchar == L'.') {

            //
            //  Not an 8.3 name if more than 1 dot or more than 8 characters
            //  remaining.  (It is legal for the dot to be in the ninth
            //  position)
            //

            if ((DotCount > 0) ||
                (Count > 8 * sizeof( WCHAR ))) {

                return FALSE;
            }

            DotCount += 1;
            LastCharDot = TRUE;

        } else {

            LastCharDot = FALSE;
        }

        Count += 2;
        NextWchar += 1;

    } while (Count < FileName.Length);

    //
    //  We can't have a period at the end of the name.
    //

    if (LastCharDot) {

        return FALSE;
    }

    //
    //  Create an Oem name to use to check for a valid short name.
    //

    DbcsName.MaximumLength = BYTE_COUNT_8_DOT_3;
    DbcsName.Buffer = DbcsNameBuffer;

    if (!NT_SUCCESS( RtlUnicodeStringToCountedOemString( &DbcsName,
                                                         &FileName,
                                                         FALSE ))) {

        return FALSE;
    }

    //
    //  We have now initialized the Oem string.  Call the FsRtl package to check for a
    //  valid FAT name.
    //

    return FsRtlIsFatDbcsLegal( DbcsName, FALSE, FALSE, FALSE );
}


BOOLEAN
UdfCandidateShortName (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING Name
    )

/*++

Routine Description:

    This routine is called to determine if the input name could be a generated
    short name.

Arguments:

    Name - Pointer to the name to stare at.

Return Value:

    BOOLEAN True if it is possible that this is a shortname, False otherwise.

--*/

{
    ULONG Index, SubIndex;
    BOOLEAN LooksShort = FALSE;
    
    PAGED_CODE();

    //
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );

    //
    //  The length can't be larger than an 8.3 name and must be
    //  at least as big as the uniqifier stamp.
    //

    ASSERT( Name->Length != 0 );
    
    if (Name->Length > BYTE_COUNT_8_DOT_3 ||
        Name->Length < DOS_CRC_LEN * sizeof(WCHAR)) {

        return FALSE;
    }
    
    //
    //  Walk across the name looking for the uniquifier stamp.  The stamp
    //  is of the form #<hex><hex><hex> so if we can stop before the end
    //  of the full name.
    //
    
    for ( Index = 0;
          Index <= (Name->Length / sizeof(WCHAR)) - DOS_CRC_LEN;
          Index++ ) {

        //
        //  Is the current character the stamp UDF uses to offset the stamp?
        //
        
        if (Name->Buffer[Index] == CRC_MARK) {
        
            //
            //  We may potentially have just a CRC at the end
            //  of the name OR have a period following.  If we
            //  do, it is reasonable to think the name may be
            //  a generated shorty.
            //
            //  #123 (a very special case - orignal name was ".")
            //  FOO#123
            //  FOO#123.TXT
            //
            
            if (Index == (Name->Length / sizeof(WCHAR)) - DOS_CRC_LEN ||
                Name->Buffer[Index + DOS_CRC_LEN] == PERIOD) {

                LooksShort = TRUE;
                break;
            }
        }
    }

    return LooksShort;
}


VOID
UdfGenerate8dot3Name (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING FileName,
    OUT PUNICODE_STRING ShortFileName
    )

/*++

Routine Description:

    This routine is called to generate a short name from the given long name.  We will
    generate a short name from the given long name.

    The short form is to convert all runs of illegal characters to "_" and tack
    on a base41 representation of the CRC of the original name.  The algorithm is
    nearly directly lifted from the UDF (2.01 proposed!) standard, so apologies for the
    style clash.
    
Arguments:

    FileName - String of bytes containing the name.

    ShortFileName - Pointer to the string to store the short name into.
        
Return Value:

    None.

--*/

{
    INT16 index;
    INT16 targetIndex;
    INT16 crcIndex;
    INT16 extLen;
    INT16 nameLen;
    INT16 charLen;
    INT16 overlayBytes;
    INT16 bytesLeft;
    UNICODE_CHAR current;
    BOOLEAN needsCRC;
    UNICODE_CHAR ext[DOS_EXT_LEN];

    //
    //  So as to lift as directly as possible from the standard, chunk things around.
    //
 
    PWCHAR dosName = ShortFileName->Buffer;
    PWCHAR udfName = FileName->Buffer;
    LONG udfNameLen = FileName->Length / sizeof(WCHAR);
    
    needsCRC = FALSE;

    /* Start at the end of the UDF file name and scan for a period */
    /* ('.').  This will be where the DOS extension starts (if     */
    /* any).                                                       */
    index = udfNameLen;
    while (index-- > 0) {
        if (udfName[index] == PERIOD)
            break;
    }

    if (index < 0) {
        /* There name was scanned to the beginning of the buffer   */
        /* and no extension was found.                             */
        extLen = 0;
        nameLen = udfNameLen;
    }
    else {
        /* A DOS extension was found, process it first.            */
        extLen = udfNameLen - index - 1;
        nameLen = index;
        targetIndex = 0;
        bytesLeft = DOS_EXT_LEN;

        while (++index < udfNameLen && bytesLeft > 0) {
            /* Get the current character and convert it to upper   */
            /* case.                                               */
            current = UnicodeToUpper(udfName[index]);
            if (current == SPACE) {
                /* If a space is found, a CRC must be appended to  */
                /* the mangled file name.                          */
                needsCRC = TRUE;
            }
            else {
                /* Determine if this is a valid file name char and */
                /* calculate its corresponding BCS character byte  */
                /* length (zero if the char is not legal or        */
                /* undisplayable on this system).                  */
                charLen = (IsFileNameCharLegal(current)) ?
                    NativeDosCharLength(current) : 0;

                /* If the char is larger than the available space  */
                /* in the buffer, pretend it is undisplayable.     */
                if (charLen > bytesLeft)
                    charLen = 0;

                if (charLen == 0) {
                    /* Undisplayable or illegal characters are     */
                    /* substituted with an underscore ("_"), and   */
                    /* required a CRC code appended to the mangled */
                    /* file name.                                  */
                    needsCRC = TRUE;
                    charLen = 1;
                    current = ILLEGAL_CHAR_MARK;

                    /* Skip over any following undiplayable or     */
                    /* illegal chars.                              */
                    while (index + 1 < udfNameLen &&
                        (!IsFileNameCharLegal(udfName[index + 1]) ||
                        NativeDosCharLength(udfName[index + 1]) == 0))
                        index++;
                }

                /* Assign the resulting char to the next index in  */
                /* the extension buffer and determine how many BCS */
                /* bytes are left.                                 */
                ext[targetIndex++] = current;
                bytesLeft -= charLen;
            }
        }

        /* Save the number of Unicode characters in the extension  */
        extLen = targetIndex;

        /* If the extension was too large, or it was zero length   */
        /* (i.e. the name ended in a period), a CRC code must be   */
        /* appended to the mangled name.                           */
        if (index < udfNameLen || extLen == 0)
            needsCRC = TRUE;
    }

    /* Now process the actual file name.                           */
    index = 0;
    targetIndex = 0;
    crcIndex = 0;
    overlayBytes = -1;
    bytesLeft = DOS_NAME_LEN;
    while (index < nameLen && bytesLeft > 0) {
        /* Get the current character and convert it to upper case. */
        current = UnicodeToUpper(udfName[index]);
        if (current == SPACE || current == PERIOD) {
            /* Spaces and periods are just skipped, a CRC code     */
            /* must be added to the mangled file name.             */
            needsCRC = TRUE;
        }
        else {
            /* Determine if this is a valid file name char and     */
            /* calculate its corresponding BCS character byte      */
            /* length (zero if the char is not legal or            */
            /* undisplayable on this system).                      */
            charLen = (IsFileNameCharLegal(current)) ?
                NativeDosCharLength(current) : 0;

            /* If the char is larger than the available space in   */
            /* the buffer, pretend it is undisplayable.            */
            if (charLen > bytesLeft)
                charLen = 0;

            if (charLen == 0) {
                /* Undisplayable or illegal characters are         */
                /* substituted with an underscore ("_"), and       */
                /* required a CRC code appended to the mangled     */
                /* file name.                                      */
                needsCRC = TRUE;
                charLen = 1;
                current = ILLEGAL_CHAR_MARK;

                /* Skip over any following undiplayable or illegal */
                /* chars.                                          */
                while (index + 1 < nameLen &&
                    (!IsFileNameCharLegal(udfName[index + 1]) ||
                    NativeDosCharLength(udfName[index + 1]) == 0))
                    index++;

                /* Terminate loop if at the end of the file name.  */
                if (index >= nameLen)
                    break;
            }

            /* Assign the resulting char to the next index in the  */
            /* file name buffer and determine how many BCS bytes   */
            /* are left.                                           */
            dosName[targetIndex++] = current;
            bytesLeft -= charLen;

            /* This figures out where the CRC code needs to start  */
            /* in the file name buffer.                            */
            if (bytesLeft >= DOS_CRC_LEN) {
                /* If there is enough space left, just tack it     */
                /* onto the end.                                   */
                crcIndex = targetIndex;
            }
            else {
                /* If there is not enough space left, the CRC      */
                /* must overlay a character already in the file    */
                /* name buffer.  Once this condition has been      */
                /* met, the value will not change.                 */
                if (overlayBytes < 0) {
                    /* Determine the index and save the length of  */
                    /* the BCS character that is overlayed.  It    */
                    /* is possible that the CRC might overlay      */
                    /* half of a two-byte BCS character depending  */
                    /* upon how the character boundaries line up.  */
                    overlayBytes = (bytesLeft + charLen > DOS_CRC_LEN)
                        ? 1 : 0;
                    crcIndex = targetIndex - 1;
                }
            }
        }

        /* Advance to the next character.                          */
        index++;
    }

    /* If the scan did not reach the end of the file name, or the  */
    /* length of the file name is zero, a CRC code is needed.      */
    if (index < nameLen || index == 0)
        needsCRC = TRUE;

    /* If the name has illegal characters or and extension, it     */
    /* is not a DOS device name.                                   */
    if (needsCRC == FALSE && extLen == 0) {
        /* If this is the name of a DOS device, a CRC code should  */
        /* be appended to the file name.                           */
        if (IsDeviceName(udfName, udfNameLen))
            needsCRC = TRUE;
    }

    /* Append the CRC code to the file name, if needed.            */
    if (needsCRC) {
        /* Get the CRC value for the original Unicode string       */
        UINT16 udfCRCValue;
        UINT16 modulus;

        //
        //  In UDF 2.00, the sample code changed to take the CRC
        //  from the UNICODE expansion of the CS0 as opposed to
        //  the CS0 itself.  In UDF 2.01, the wording of the spec
        //  will actually match this.
        //
        //  Additionally, the checksum changes to be byte-order
        //  independent.
        //
        
        udfCRCValue = UdfComputeCrc16Uni(udfName, udfNameLen);

        /* Determine the character index where the CRC should      */
        /* begin.                                                  */
        targetIndex = crcIndex;

        /* If the character being overlayed is a two-byte BCS      */
        /* character, replace the first byte with an underscore.   */
        if (overlayBytes > 0)
            dosName[targetIndex++] = ILLEGAL_CHAR_MARK;

        //
        //  UDF 2.01 changes to a base 41 encoding.  UDF 1.50 and
        //  UDF 2.00 exchanged the # delimeter with the high 4bits
        //  of the CRC.
        //

        dosName[targetIndex++] = CRC_MARK;
        
        dosName[targetIndex++] =
            UdfCrcChar[udfCRCValue / (41 * 41)];
        udfCRCValue %= (41 * 41);
        
        dosName[targetIndex++] =
            UdfCrcChar[udfCRCValue / 41];
        udfCRCValue %= 41;
        
        dosName[targetIndex++] =
            UdfCrcChar[udfCRCValue];
    }

    /* Append the extension, if any.                               */
    if (extLen > 0) {
        /* Tack on a period and each successive byte in the        */
        /* extension buffer.                                       */
        dosName[targetIndex++] = PERIOD;
        for (index = 0; index < extLen; index++)
            dosName[targetIndex++] = ext[index];
    }

    ASSERT( (targetIndex * sizeof(WCHAR)) <= ShortFileName->MaximumLength );
 
    ShortFileName->Length = (USHORT) (targetIndex * sizeof(WCHAR));

    //
    //  Now we upcase the whole name at once.
    //

    UdfUpcaseName( IrpContext,
                   ShortFileName,
                   ShortFileName );
}


VOID
UdfConvertCS0DstringToUnicode (
    IN PIRP_CONTEXT IrpContext,
    IN PUCHAR Dstring,
    IN UCHAR Length OPTIONAL,
    IN UCHAR FieldLength OPTIONAL,
    IN OUT PUNICODE_STRING Name
    )

/*++

Routine Description:

    This routine will convert the CS0 input dstring (1/7.2.12) to Unicode.  We assume that
    the length is sane.
    
    This "compression" in CS0 is really just a special case hack for ASCII.

Arguments:

    Dstring - the input dstring field
    
    Length - length of the dstring.  If unspecified, we assume that the characters come
        from a proper 1/7.2.12 dstring that specifies length in the last character of the
        field.
    
    FieldLength - length of the dstring field.  If unspecified, we assume that the characters
        come from an uncounted length of CS0 characters and that the Length parameter is
        specified.
    
    Name - the output Unicode string

Return Value:

    None.

--*/

{
    ULONG CompressID;
    ULONG UnicodeIndex, ByteIndex;
    PWCHAR Unicode = Name->Buffer;

    UCHAR NameLength;
    ULONG CopyNameLength;

    PAGED_CODE();
   
    //
    //  Check input.
    //

    ASSERT_IRP_CONTEXT( IrpContext );

    CompressID = *Dstring;

    //
    //  If the length is unspecified, this is a real 1/7.2.12 dstring and the length is in
    //  the last character of the field.
    //
    
    ASSERT( Length || FieldLength );

    if (Length) {

        NameLength = FieldLength = Length;
    
    } else {

        NameLength = *(Dstring + FieldLength - 1);
    }
    
    //
    //  If the caller specified a size, they should have made sure the buffer is big enough.
    //  Otherwise, we will trim to fit.
    //
    
    ASSERT( Length == 0 || Name->MaximumLength >= UdfCS0DstringUnicodeSize( IrpContext, Dstring, NameLength ) );
 
    //
    //  Decide how many UNICODE bytes to "copy".
    //
    
    CopyNameLength = Min( Name->MaximumLength, UdfCS0DstringUnicodeSize( IrpContext, Dstring, NameLength ));
    
    //
    //  Reset the name length and advance over the compression ID in the dstring.
    //
    
    Name->Length = 0;
    Dstring++;
 
    //
    //  Loop through all the bytes.
    //

    while (CopyNameLength > Name->Length) {
      
        if (CompressID == 16) {
       
            //
            //  We're little endian, and this is the single place in the entire UDF/ISO standard
            //  where they use big endian.
            //
            //  Thank you.  Thank you very much.
            //
            //  Do an unaligned swapcopy of this 16bit value.
            //

            SwapCopyUchar2( Unicode, Dstring );
            Dstring += sizeof(WCHAR);
       
        } else {

            //
            //  Drop the byte into the low bits.
            //
                
            *Unicode = *Dstring;
            Dstring += sizeof(CHAR);
        }

        Name->Length += sizeof(WCHAR);
        Unicode++;
    }

    return;
}


BOOLEAN
UdfCheckLegalCS0Dstring (
    PIRP_CONTEXT IrpContext,
    PUCHAR Dstring,
    UCHAR Length OPTIONAL,
    UCHAR FieldLength OPTIONAL,
    BOOLEAN ReturnOnError
    )

/*++

Routine Description:

    This routine inspects a CS0 Dstring for conformance.
    
Arguments:

    Dstring - a dstring to check
    
    Length - length of the dstring.  If unspecified, we assume that the characters come
        from a proper 1/7.2.12 dstring that specifies length in the last character of the
        field.
    
    FieldLength - length of the dstring field.  If unspecified, we assume that the characters
        come from an uncounted length of CS0 characters and that the Length parameter is
        specified.
    
    ReturnOnError - whether to return or raise on a discovered error
    
Return Value:

    None. Raised status if corruption is found.
    
--*/

{
    UCHAR NameLength;

    //
    //  Check input.
    //

    ASSERT_IRP_CONTEXT( IrpContext );

    //
    //  If the length is unspecified, this is a real 1/7.2.12 dstring and the length is in
    //  the last character of the field.
    //
    
    ASSERT( Length || FieldLength );

    if (Length) {

        NameLength = FieldLength = Length;
    
    } else {

        NameLength = *(Dstring + FieldLength - 1);
    }

    DebugTrace(( +1, Dbg,
                 "UdfCheckLegalCS0Dstring, Dstring %08x Length %02x FieldLength %02x (NameLength %02x)\n",
                 Dstring,
                 Length,
                 FieldLength,
                 NameLength ));

    //
    //  The string must be "compressed" in 8bit or 16bit chunks.  If it
    //  is in 16bit chunks, we better have an integral number of them -
    //  remember we have the compression ID, so the length will be odd.
    //
    
    if ((NameLength <= 1 &&
         DebugTrace(( 0, Dbg,
                      "UdfCheckLegalCS0Dstring, NameLength is too small!\n" ))) ||

        (NameLength > FieldLength &&
         DebugTrace(( 0, Dbg,
                      "UdfCheckLegalCS0Dstring, NameLength is bigger than the field itself!\n" ))) ||

        ((*Dstring != 8 && *Dstring != 16) &&
         DebugTrace(( 0, Dbg,
                      "UdfCheckLegalCS0Dstring, claims encoding %02x, unknown! (not 0x8 or 0x10)\n",
                      *Dstring ))) ||

        ((*Dstring == 16 && !FlagOn( NameLength, 1)) &&
         DebugTrace(( 0, Dbg,
                     "UdfCheckLegalCS0Dstring, NameLength not odd, encoding 0x10!\n" )))) {

        if (ReturnOnError) {

            DebugTrace(( -1, Dbg, "UdfCheckLegalCS0Dstring -> FALSE\n" ));

            return FALSE;
        }

        DebugTrace(( -1, Dbg, "UdfCheckLegalCS0Dstring -> raised status\n" ));

        UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
    }

    DebugTrace(( -1, Dbg, "UdfCheckLegalCS0Dstring -> TRUE\n" ));

    return TRUE;
}


VOID
UdfRenderNameToLegalUnicode (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING Name,
    IN PUNICODE_STRING RenderedName
    )

/*++

Routine Description:

    This routine will take a Unicode string containing illegal characters and
    run it through the UDF standard algorithim to render it into a "legal"
    name.
    
    The short form is to convert all runs of illegal characters to "_" and tack
    on a hex representation of the CRC of the original name.  The algorithm is
    nearly directly lifted from the UDF (2.01 proposed!) standard, so apologies
    for the style clash.
    
Arguments:

    Name - the actual name
    
    RenderedName - the name rendered into legal characters
    
Return Value:

    BOOLEAN - TRUE if the expressions match, FALSE otherwise.

--*/

{
    INT16 index;
    INT16 targetIndex;
    INT16 crcIndex;
    INT16 extLen;
    INT16 nameLen;
    INT16 charLen;
    INT16 overlayBytes;
    INT16 bytesLeft;
    UNICODE_CHAR current;
    BOOLEAN needsCRC;
    BOOLEAN foundDot;
    UNICODE_CHAR ext[EXT_LEN];

    //
    //  So as to lift as directly as possible from the standard, chunk things around.
    //
 
    PWCHAR newName = RenderedName->Buffer;
    PWCHAR udfName = Name->Buffer;
    LONG udfNameLen = Name->Length / sizeof(WCHAR);

    /* Remove trailing periods ('.') and spaces (' '), Windows     */
    /* does not like them.                                         */
    foundDot = FALSE;
    index = udfNameLen;
    while (index-- > 0) {
        if (udfName[index] == PERIOD)
            foundDot = TRUE;
        else if (udfName[index] != SPACE)
            break;
    }

    /* If any trailing periods or spaces were found, a CRC code    */
    /* needs to be added to the resulting file name.               */
    nameLen = index + 1;
    if (nameLen < udfNameLen)
        needsCRC = TRUE;

    needsCRC = FALSE;
    bytesLeft = MAX_LEN;
    extLen = 0;

    if (needsCRC == FALSE || foundDot == FALSE) {
        /* Look for an extension in the file name.  We do not      */
        /* need to look for one if there were any trailing periods */
        /* removed.                                                */
        INT16 endIndex;
        INT16 prevCharLen = 1;
        INT16 extBytes = 0;

        targetIndex = 0;
        index = nameLen;

        /* Determine how many bytes we need to scan to find the    */
        /* extension delimiter.  The extension has a maximum of    */
        /* five characters, but we do not want to scan past the    */
        /* beginning of the buffer.                                */
        endIndex = (udfNameLen > EXT_LEN + 1) ?
            udfNameLen - EXT_LEN - 1 : 1;

        /* Start at the end of the name and scan backward, looking */
        /* for the extension delimiter (".").                      */
        while (index-- > endIndex) {
            /* Get the character to test.                          */
            current = udfName[index];

            if (current == '.') {
                /* The extension delimiter was found, figure out   */
                /* how many characters the extension contains and  */
                /* the length of the resulting file name without   */
                /* the extension.                                  */
                extLen = nameLen - index - 1;
                nameLen = index;
                break;
            }

            /* Determine the byte length of the current character  */
            /* when converted to native format.                    */
            charLen = (IsFileNameCharLegal(current)) ?
                NativeCharLength(current) : 0;

            if (charLen == 0) {
                /* If the character byte length is zero, it is     */
                /* illegal or unprintable, place an underscore     */
                /* ("_") in the extension buffer if the previous   */
                /* character tested was legal.  Not that the       */
                /* characters placed in the extension buffer are   */
                /* in reverse order.                               */
                if (prevCharLen != 0) {
                    ext[targetIndex++] = ILLEGAL_CHAR_MARK;
                    extBytes++;
                }
            }
            else {
                /* The current character is legal and printable,   */
                /* put it in the extension buffer.  Note that the  */
                /* characters placed in the extension buffer are   */
                /* in reverse order.                               */
                ext[targetIndex++] = current;
                extBytes += charLen;
            }

            /* Save the byte length of the current character, so   */
            /* we can determine if it was a legal character during */
            /* the next test.                                      */
            prevCharLen = charLen;
        }

        /* If an extension was found, determine how many bytes     */
        /* remain in the file name buffer once we account for it.  */
        if (extLen > 0)
            bytesLeft -= extBytes + 1;
    }

    index = 0;
    targetIndex = 0;
    crcIndex = 0;
    overlayBytes = -1;
    while (index < nameLen && bytesLeft > 0) {
        /* Get the current character and convert it to upper case. */
        current = udfName[index];

        /* Determine if this is a valid file name char and         */
        /* calculate its corresponding native character byte       */
        /* length (zero if the char is not legal or undiplayable   */
        /* on this system).                                        */
        charLen = (IsFileNameCharLegal(current)) ?
            NativeCharLength(current) : 0;

        /* If the char is larger than the available space in the   */
        /* buffer, pretend it is undisplayable.                    */
        if (charLen > bytesLeft)
            charLen = 0;

        if (charLen == 0) {
            /* Undisplayable or illegal characters are substituted */
            /* with an underscore ("_"), and requires a CRC code   */
            /* appended to the mangled file name.                  */
            needsCRC = TRUE;
            charLen = 1;
            current = '_';

            /* Skip over any following undiplayable or illegal     */
            /* chars.                                              */
            while (index + 1 < udfNameLen &&
                (!IsFileNameCharLegal(udfName[index + 1]) ||
                NativeCharLength(udfName[index + 1]) == 0))
                index++;

            /* Terminate loop if at the end of the file name.      */
            if (index >= udfNameLen)
                break;
        }

        /* Assign the resulting char to the next index in the file */
        /* name buffer and determine how many native bytes are     */
        /* left.                                                   */
        newName[targetIndex++] = current;
        bytesLeft -= charLen;

        /* This figures out where the CRC code needs to start in   */
        /* the file name buffer.                                   */
        if (bytesLeft >= CRC_LEN) {
            /* If there is enough space left, just tack it onto    */
            /* the end.                                            */
            crcIndex = targetIndex;
        }
        else {
            /* If there is not enough space left, the CRC must     */
            /* overlay a character already in the file name        */
            /* buffer.  Once this condition has been met, the      */
            /* value will not change.                              */
            if (overlayBytes < 0) {
                /* Determine the index and save the length of the  */
                /* native character that is overlayed.  It is      */
                /* possible that the CRC might overlay half of a   */
                /* two-byte native character depending upon how    */
                /* the character boundaries line up.               */
                overlayBytes = (bytesLeft + charLen > CRC_LEN)
                    ? 1 : 0;
                crcIndex = targetIndex - 1;
            }
        }

        /* Advance to the next character.                          */
        index++;
    }

    /* If the scan did not reach the end of the file name, or the  */
    /* length of the file name is zero, a CRC code is needed.      */
    if (index < nameLen || index == 0)
        needsCRC = TRUE;

    /* If the name has illegal characters or and extension, it     */
    /* is not a DOS device name.                                   */
    if (needsCRC == FALSE && extLen == 0) {
        /* If this is the name of a DOS device, a CRC code should  */
        /* be appended to the file name.                           */
        if (IsDeviceName(udfName, udfNameLen))
            needsCRC = TRUE;
    }

    /* Append the CRC code to the file name, if needed.            */
    if (needsCRC) {
        /* Get the CRC value for the original Unicode string       */
        UINT16 udfCRCValue = UdfComputeCrc16Uni(udfName, udfNameLen);

        /* Determine the character index where the CRC should      */
        /* begin.                                                  */
        targetIndex = crcIndex;

        /* If the character being overlayed is a two-byte native   */
        /* character, replace the first byte with an underscore.   */
        if (overlayBytes > 0)
            newName[targetIndex++] = ILLEGAL_CHAR_MARK;

        /* Append the encoded CRC value with delimiter.            */
        newName[targetIndex++] = CRC_MARK;
        newName[targetIndex++] = UdfCrcChar[(udfCRCValue & 0xf000) >> 12];
        newName[targetIndex++] = UdfCrcChar[(udfCRCValue & 0x0f00) >> 8];
        newName[targetIndex++] = UdfCrcChar[(udfCRCValue & 0x00f0) >> 4];
        newName[targetIndex++] = UdfCrcChar[(udfCRCValue & 0x000f)];
    }


    /* If an extension was found, append it here.                  */
    if (extLen > 0) {
        /* Add the period ('.') for the extension delimiter.       */
        newName[targetIndex++] = PERIOD;

        /* Append the characters in the extension buffer.  They    */
        /* were stored in reverse order, so we need to begin with  */
        /* the last character and work forward.                    */
        while (extLen-- > 0)
            newName[targetIndex++] = ext[extLen];
    }

    ASSERT( (targetIndex * sizeof(WCHAR)) <= RenderedName->MaximumLength );
 
    RenderedName->Length = (USHORT) (targetIndex * sizeof(WCHAR));
}


BOOLEAN
UdfIsNameInExpression (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING CurrentName,
    IN PUNICODE_STRING SearchExpression,
    IN BOOLEAN Wild
    )

/*++

Routine Description:

    This routine will compare two Unicode strings.  We assume that if this
    is to be a case-insensitive search then they are already upcased.

Arguments:

    CurrentName - Filename from the disk.

    SearchExpression - Filename expression to use for match.
    
    Wild - True if wildcards are present in SearchExpression.

Return Value:

    BOOLEAN - TRUE if the expressions match, FALSE otherwise.

--*/

{
    BOOLEAN Match = TRUE;
    
    PAGED_CODE();

    //
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );

    //
    //  If there are wildcards in the expression then we call the
    //  appropriate FsRtlRoutine.
    //

    if (Wild) {

        Match = FsRtlIsNameInExpression( SearchExpression,
                                         CurrentName,
                                         FALSE,
                                         NULL );

    //
    //  Otherwise do a direct memory comparison for the name string.
    //

    } else {

        if ((CurrentName->Length != SearchExpression->Length) ||
            (!RtlEqualMemory( CurrentName->Buffer,
                              SearchExpression->Buffer,
                              CurrentName->Length ))) {

            Match = FALSE;
        }
    }

    return Match;
}


FSRTL_COMPARISON_RESULT
UdfFullCompareNames (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING NameA,
    IN PUNICODE_STRING NameB
    )

/*++

Routine Description:

    This function compares two names as fast as possible.  Note that since
    this comparison is case sensitive we can do a direct memory comparison.

Arguments:

    NameA & NameB - The names to compare.

Return Value:

    COMPARISON - returns

        LessThan    if NameA < NameB lexicalgraphically,
        GreaterThan if NameA > NameB lexicalgraphically,
        EqualTo     if NameA is equal to NameB

--*/

{
    ULONG i;
    ULONG MinLength = NameA->Length;
    FSRTL_COMPARISON_RESULT Result = LessThan;

    PAGED_CODE();

    //
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );

    //
    //  Figure out the minimum of the two lengths
    //

    if (NameA->Length > NameB->Length) {

        MinLength = NameB->Length;
        Result = GreaterThan;

    } else if (NameA->Length == NameB->Length) {

        Result = EqualTo;
    }

    //
    //  Loop through looking at all of the characters in both strings
    //  testing for equalilty, less than, and greater than
    //

    i = (ULONG) RtlCompareMemory( NameA->Buffer, NameB->Buffer, MinLength );

    if (i < MinLength) {

        //
        //  We know the offset of the first character which is different.
        //

        return ((NameA->Buffer[ i / 2 ] < NameB->Buffer[ i / 2 ]) ?
                 LessThan :
                 GreaterThan);
    }

    //
    //  The names match up to the length of the shorter string.
    //  The shorter string lexically appears first.
    //

    return Result;
}

