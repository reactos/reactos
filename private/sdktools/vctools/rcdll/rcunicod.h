/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    rcunicod.h

Abstract:

    This is the header file for rcpp 16-bit unicode support.  It contains
        the translatation table for codepage 1252.  This was taken from the
        nls1252.txt file.

Author:

    David J. Marsyla (t-davema) 25-Aug-1991

Revision History:


--*/

#define IN
#define OUT


#define DFT_TEST_SIZE                     250      // The number of bytes to test to get
                                                   //    an accurate determination of file type.

//
// The following may be retruned from DetermineFileType ().
//

#define DFT_FILE_IS_UNKNOWN             0       // File type not yet determined.
#define DFT_FILE_IS_8_BIT               1       // File is an 8-bit ascii file.
#define DFT_FILE_IS_16_BIT              2       // File is standard 16-bit unicode file.
#define DFT_FILE_IS_16_BIT_REV          3       // File is reversed 16-bit unicode file.

//
// This function can be used to determine the format of a disk file.
//
INT
DetermineFileType (
    IN      PFILE        fpInputFile
    );

//
// The following may be returned from DetermnineSysEndianType ().
//

#define DSE_SYS_LITTLE_ENDIAN   1       // Return values from determine system
#define DSE_SYS_BIG_ENDIAN      2       // endian type.

//
// This function will return the endian type of the current system.
//
INT
DetermineSysEndianType (
        VOID
    );


//
// This function converts command line arguments to Unicode buffer
//
WCHAR ** UnicodeCommandLine (
    int,
    char **
    );

