/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    rcunicod.c

Abstract:

    Routines added to rcpp to support 16-bit unicode file parsing.
        Note that as of Aug 91, rcpp will not fully transfer the unicode
        characters but only the string constants are guaranteed to be passed
        cleanly.

Author:

    David J. Marsyla (t-davema) 25-Aug-1991

Revision History:


--*/


#include "rc.h"

extern BOOL WINAPI LocalIsTextUnicode(CONST LPVOID Buffer, int Size, LPINT Result);

INT
DetermineFileType (
    IN      PFILE        fpInputFile
    )

/*++

Routine Description:

    This function is used to determine what type of file is being read.
        Note, the file is returned to it's proper position after function.

Arguments:

    fpInputFile                 - File pointer to file we are checking, must be
                                                  open with read permissions.

Return Value:

        DFT_FILE_IS_UNKNOWN     - It was impossible to determine what type of file
                                                          we were checking.  This usually happens when EOF
                                                          is unexpectedly reached.
        DFT_FILE_IS_8_BIT       - File was determined to be in standard 8-bit
                                                          format.
        DFT_FILE_IS_16_BIT      - File was determined to be a 16 bit unicode file
                                                          which can be directly read into a WCHAR array.
        DFT_FILE_IS_16_BIT_REV  - File was determined to be a 16 bit unicode file
                                                          which has it's bytes reversed in order.

--*/

{
   LONG   lStartFilePos;                     // Storage for file position.
   BYTE   buf[DFT_TEST_SIZE+2];
   LONG   chRead;
   INT    val = 0xFFFF;
   INT    fFileType;

    //
    // Store position so we can get back to it.
    //
    lStartFilePos = ftell (fpInputFile);

    //
    // Make sure we start on an even byte to simplify routines.
    //
    if (lStartFilePos % 2)
        fgetc (fpInputFile);

    chRead = fread (buf, 1, DFT_TEST_SIZE, fpInputFile);
    memset (buf + chRead, 0, sizeof(WCHAR));

    if (LocalIsTextUnicode (buf, chRead, &val))
    {
        if ((val & IS_TEXT_UNICODE_REVERSE_SIGNATURE) == IS_TEXT_UNICODE_REVERSE_SIGNATURE)
            fFileType = DFT_FILE_IS_16_BIT_REV;
        else
            fFileType = DFT_FILE_IS_16_BIT;
    }
    else
        fFileType = DFT_FILE_IS_8_BIT;

    //
    // Return to starting file position.  (usually beginning)
    //

    fseek (fpInputFile, lStartFilePos, SEEK_SET);

    return (fFileType);
}


INT
DetermineSysEndianType (
        VOID
    )

/*++

Routine Description:

    This function is used to determine how the current system stores its
        integers in memory.

    For those of us who are confused by little endian and big endian formats,
        here is a brief recap.

    Little Endian:  (This is used on Intel 80x86 chips.  The MIPS RS4000 chip
                 is switchable, but will run in little endian format for NT.)
       This is where the high order bytes of a short or long are stored higher
       in memory.  For example the number 0x80402010 is stored as follows.
         Address:        Value:
             00            10
             01            20
             02            40
             03            80
       This looks backwards when memory is dumped in order: 10 20 40 80

    Big Endian:  (This is not currently used on any NT systems but hey, this
         is supposed to be portable!!)
       This is where the high order bytes of a short or long are stored lower
       in memory.  For example the number 0x80402010 is stored as follows.
         Address:        Value:
             00            80
             01            40
             02            20
             03            10
       This looks correct when memory is dumped in order: 80 40 20 10

Arguments:

        None.

Return Value:

        DSE_SYS_LITTLE_ENDIAN   - The system stores integers in little endian
                                                          format.  (this is 80x86 default).
        DSE_SYS_BIG_ENDIAN      - The system stores integers in big endian format.

--*/

{
    INT     nCheckInteger;
    CHAR    rgchTestBytes [sizeof (INT)];

    //
    // Clear the test bytes to zero.
    //

    *((INT *)rgchTestBytes) = 0;

    //
    // Set first to some value.
    //

    rgchTestBytes [0] = (CHAR)0xFF;

    //
    // Map it to an integer.
    //

    nCheckInteger = *((INT *)rgchTestBytes);

    //
    // See if value was stored in low order of integer.
    // If so then system is little endian.
    //

    if (nCheckInteger == 0xFF)
        return (DSE_SYS_LITTLE_ENDIAN);
    else
        return (DSE_SYS_LITTLE_ENDIAN);
}


//
// UnicodeCommandLine
//
// Makes a Unicode buffer copy of command line argv arguments
//
WCHAR ** UnicodeCommandLine (int argc, char ** argv)
{
    WCHAR ** argv_U;
    WCHAR ** pU;
    WCHAR *  str;
    int      nbytes;
    int      i;

    // Calculate the size of buffer
    for (i = 0, nbytes = 0; i < argc; i++)
        nbytes += strlen(argv[i]) + 1;
    nbytes *= sizeof(WCHAR);

    /* allocate space for argv[] vector and strings */
    argv_U = (WCHAR **) MyAlloc((argc + 1) * sizeof(WCHAR *) + nbytes);
    if (!argv_U)
        return (NULL);

    /* store args and argv ptrs in just allocated block */
    str = (WCHAR *)(((PBYTE)argv_U) + (argc + 1) * sizeof(WCHAR *));
    for (i = 0, pU = argv_U; i < argc; i++)
    {
        *pU++ = str;
        nbytes = strlen(argv[i]) + 1;
        MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, argv[i], nbytes, str, nbytes);
        str += nbytes;
    }
    *pU = NULL;

    return (argv_U);
}

