/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    unicode.c

Abstract:

    This file contains functions necessary to parse and write the locale
    independent (Unicode) tables to a data file.

    External Routines in this file:
      ParseUnicode
      WriteUnicode

Revision History:

    12-10-91    JulieB    Created.

--*/



//
//  Include Files.
//

#include "nlstrans.h"




//
//  Forward Declarations.
//

int
GetAsciiDigits(
    PUNICODE pUnic,
    int Size);

int
GetFoldCZone(
    PUNICODE pUnic,
    int Size);

int
GetHiragana(
    PUNICODE pUnic,
    int Size);

int
GetKatakana(
    PUNICODE pUnic,
    int Size);

int
GetHalfWidth(
    PUNICODE pUnic,
    int Size);

int
GetFullWidth(
    PUNICODE pUnic,
    int Size);

int
GetTraditional(
    PUNICODE pUnic,
    int Size);

int
GetSimplified(
    PUNICODE pUnic,
    int Size);

int
GetCompTable(
    PUNICODE pUnic,
    int Size);

void
Get844Value(
    P844_ARRAY pArr,
    WORD WChar,
    WORD *Value);

void
InsertCompGrid(
    PCOMP_GRID pCompGrid,
    WORD PreComp,
    WORD BaseOff,
    WORD NonSpOff);

int
WriteAsciiDigits(
    PUNICODE pUnic,
    FILE *pOutputFile);

int
WriteFoldCZone(
    PUNICODE pUnic,
    FILE *pOutputFile);

int
WriteHiragana(
    PUNICODE pUnic,
    FILE *pOutputFile);

int
WriteKatakana(
    PUNICODE pUnic,
    FILE *pOutputFile);

int
WriteHalfWidth(
    PUNICODE pUnic,
    FILE *pOutputFile);

int
WriteFullWidth(
    PUNICODE pUnic,
    FILE *pOutputFile);

int
WriteTraditional(
    PUNICODE pUnic,
    FILE *pOutputFile);

int
WriteSimplified(
    PUNICODE pUnic,
    FILE *pOutputFile);

int
WritePrecomposed(
    PUNICODE pUnic,
    FILE *pOutputFile);

int
WriteComposite(
    PUNICODE pUnic,
    FILE *pOutputFile);

int
WriteGrid(
    PUNICODE pUnic,
    FILE *pOutputFile);





//-------------------------------------------------------------------------//
//                            EXTERNAL ROUTINES                            //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  ParseUnicode
//
//  This routine parses the input file for the locale independent (Unicode)
//  tables.  This routine is only entered when the UNICODE keyword is found.
//  The parsing continues until the ENDUNICODE keyword is found.
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ParseUnicode(
    PUNICODE pUnic,
    PSZ pszKeyWord)
{
    int size;                          // size of table to follow


    while (fscanf(pInputFile, "%s", pszKeyWord) == 1)
    {
        if (_strcmpi(pszKeyWord, "ASCIIDIGITS") == 0)
        {
            if (Verbose)
                printf("\n\nFound ASCIIDIGITS keyword.\n");

            //
            //  Get size parameter.
            //
            if (GetSize(&size))
                return (1);

            //
            //  Get ASCIIDIGITS Table.
            //
            if (GetAsciiDigits(pUnic, size))
                return (1);

            //
            //  Set WriteFlags for ASCIIDIGITS Table.
            //
            pUnic->WriteFlags |= F_ADIGIT;
        }

        else if (_strcmpi(pszKeyWord, "FOLDCZONE") == 0)
        {
            if (Verbose)
                printf("\n\nFound FOLDCZONE keyword.\n");

            //
            //  Get size parameter.
            //
            if (GetSize(&size))
                return (1);

            //
            //  Get FOLDCZONE Table.
            //
            if (GetFoldCZone(pUnic, size))
                return (1);

            //
            //  Set WriteFlags for FOLDCZONE Table.
            //
            pUnic->WriteFlags |= F_CZONE;
        }

        else if (_strcmpi(pszKeyWord, "HIRAGANA") == 0)
        {
            if (Verbose)
                printf("\n\nFound HIRAGANA keyword.\n");

            //
            //  Get size parameter.
            //
            if (GetSize(&size))
                return (1);

            //
            //  Get HIRAGANA Table.
            //
            if (GetHiragana(pUnic, size))
                return (1);

            //
            //  Set WriteFlags for HIRAGANA Table.
            //
            pUnic->WriteFlags |= F_HIRAGANA;
        }

        else if (_strcmpi(pszKeyWord, "KATAKANA") == 0)
        {
            if (Verbose)
                printf("\n\nFound KATAKANA keyword.\n");

            //
            //  Get size parameter.
            //
            if (GetSize(&size))
                return (1);

            //
            //  Get KATAKANA Table.
            //
            if (GetKatakana(pUnic, size))
                return (1);

            //
            //  Set WriteFlags for KATAKANA Table.
            //
            pUnic->WriteFlags |= F_KATAKANA;
        }

        else if (_strcmpi(pszKeyWord, "HALFWIDTH") == 0)
        {
            if (Verbose)
                printf("\n\nFound HALFWIDTH keyword.\n");

            //
            //  Get size parameter.
            //
            if (GetSize(&size))
                return (1);

            //
            //  Get HALFWIDTH Table.
            //
            if (GetHalfWidth(pUnic, size))
                return (1);

            //
            //  Set WriteFlags for HALFWIDTH Table.
            //
            pUnic->WriteFlags |= F_HALFWIDTH;
        }

        else if (_strcmpi(pszKeyWord, "FULLWIDTH") == 0)
        {
            if (Verbose)
                printf("\n\nFound FULLWIDTH keyword.\n");

            //
            //  Get size parameter.
            //
            if (GetSize(&size))
                return (1);

            //
            //  Get FULLWIDTH Table.
            //
            if (GetFullWidth(pUnic, size))
                return (1);

            //
            //  Set WriteFlags for FULLWIDTH Table.
            //
            pUnic->WriteFlags |= F_FULLWIDTH;
        }

        else if (_strcmpi(pszKeyWord, "TRADITIONAL_CHINESE") == 0)
        {
            if (Verbose)
                printf("\n\nFound TRADITIONAL_CHINESE keyword.\n");

            //
            //  Get size parameter.
            //
            if (GetSize(&size))
                return (1);

            //
            //  Get TRADITIONAL_CHINESE Table.
            //
            if (GetTraditional(pUnic, size))
                return (1);

            //
            //  Set WriteFlags for TRADITIONAL_CHINESE Table.
            //
            pUnic->WriteFlags |= F_TRADITIONAL;
        }

        else if (_strcmpi(pszKeyWord, "SIMPLIFIED_CHINESE") == 0)
        {
            if (Verbose)
                printf("\n\nFound SIMPLIFIED_CHINESE keyword.\n");

            //
            //  Get size parameter.
            //
            if (GetSize(&size))
                return (1);

            //
            //  Get SIMPLIFIED_CHINESE Table.
            //
            if (GetSimplified(pUnic, size))
                return (1);

            //
            //  Set WriteFlags for SIMPLIFIED_CHINESE Table.
            //
            pUnic->WriteFlags |= F_SIMPLIFIED;
        }

        else if (_strcmpi(pszKeyWord, "COMP") == 0)
        {
            if (Verbose)
                printf("\n\nFound COMP keyword.\n");

            //
            //  Get size parameter.
            //
            if (GetSize(&size))
                return (1);

            //
            //  Get Precomposed and Composite Tables.
            //
            if (GetCompTable(pUnic, size))
                return (1);

            //
            //  Set WriteFlags for COMP Tables.
            //
            pUnic->WriteFlags |= F_COMP;
        }

        else if (_strcmpi(pszKeyWord, "ENDUNICODE") == 0)
        {
            if (Verbose)
                printf("\n\nFound ENDUNICODE keyword.\n");

            //
            //  Return success.
            //
            return (0);
        }

        else
        {
            printf("Parse Error: Invalid Instruction '%s'.\n", pszKeyWord);
            return (1);
        }
    }

    //
    //  If this point is reached, then the ENDUNICODE keyword was
    //  not found.  Return an error.
    //
    printf("Parse Error: Expecting ENDUNICODE keyword.\n");
    return (1);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteUnicode
//
//  This routine writes the locale independent (Unicode) tables to an
//  output file.
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteUnicode(
    PUNICODE pUnic)
{
    FILE *pOutputFile;                 // ptr to output file


    //
    //  Make sure all tables are present.
    //
    if (!((pUnic->WriteFlags & F_ADIGIT) &&
          (pUnic->WriteFlags & F_CZONE) &&
          (pUnic->WriteFlags & F_COMP) &&
          (pUnic->WriteFlags & F_HIRAGANA) &&
          (pUnic->WriteFlags & F_KATAKANA) &&
          (pUnic->WriteFlags & F_HALFWIDTH) &&
          (pUnic->WriteFlags & F_FULLWIDTH) &&
          (pUnic->WriteFlags & F_TRADITIONAL) &&
          (pUnic->WriteFlags & F_SIMPLIFIED)))
    {
        printf("Write Error: All tables must be present -\n");
        printf("             Ascii Digits, Compatibility Zone, Composite Tables,\n");
        printf("             Hiragana, Katakana, Half Width, Full Width,\n");
        printf("             Traditional Chinese, and Simplified Chinese.\n");
        return (1);
    }

    //
    //  Make sure output file can be opened for writing.
    //
    if ((pOutputFile = fopen(UNICODE_FILE, "w+b")) == 0)
    {
        printf("Error opening output file %s.\n", UNICODE_FILE);
        return (1);
    }

    if (Verbose)
        printf("\n\nWriting output file %s...\n", UNICODE_FILE);

    //
    //  Write Ascii Digits Table to output file.
    //
    if (WriteAsciiDigits(pUnic, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Free Ascii Digits table structures.
    //
    Free844(pUnic->pADigit);


    //
    //  Write Fold Compatibility Zone Table to output file.
    //
    if (WriteFoldCZone(pUnic, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Free Fold Compatibility Zone table structures.
    //
    Free844(pUnic->pCZone);


    //
    //  Write Hiragana Table to output file.
    //
    if (WriteHiragana(pUnic, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Free Hiragana table structures.
    //
    Free844(pUnic->pHiragana);


    //
    //  Write Katakana Table to output file.
    //
    if (WriteKatakana(pUnic, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Free Katakana table structures.
    //
    Free844(pUnic->pKatakana);


    //
    //  Write Half Width Table to output file.
    //
    if (WriteHalfWidth(pUnic, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Free Half Width table structures.
    //
    Free844(pUnic->pHalfWidth);


    //
    //  Write Full Width Table to output file.
    //
    if (WriteFullWidth(pUnic, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Free Full Width table structures.
    //
    Free844(pUnic->pFullWidth);


    //
    //  Write Traditional Chinese Table to output file.
    //
    if (WriteTraditional(pUnic, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Free Traditional Chinese table structures.
    //
    Free844(pUnic->pTraditional);


    //
    //  Write Simplified Chinese Table to output file.
    //
    if (WriteSimplified(pUnic, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Free Simplified Chinese table structures.
    //
    Free844(pUnic->pSimplified);


    //
    //  Write Precomposed Table to output file.
    //
    if (WritePrecomposed(pUnic, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Write Composite Table to output file.
    //
    if (WriteComposite(pUnic, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Free Comp table structures.
    //
    Free844(pUnic->pPreComp);
    Free844(pUnic->pBase);
    Free844(pUnic->pNonSp);
    if (pUnic->pCompGrid != NULL)
    {
        free(pUnic->pCompGrid);
    }


    //
    //  Close the output file.
    //
    fclose(pOutputFile);

    //
    //  Return success.
    //
    printf("\nSuccessfully wrote output file %s\n", UNICODE_FILE);
    return (0);
}




//-------------------------------------------------------------------------//
//                            INTERNAL ROUTINES                            //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  GetAsciiDigits
//
//  This routine gets the ascii digits table from the input file.  It uses
//  the size parameter to know when to stop reading from the file.  If an
//  error is encountered, a message is printed and an error is returned.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

int GetAsciiDigits(
    PUNICODE pUnic,
    int Size)
{
    int Digit;                    // digit value
    int Ascii;                    // ascii digit value
    register int Ctr;             // loop counter
    int NumItems;                 // number of items returned from fscanf


    //
    //  Allocate top buffer for 8:4:4 table - 256 pointers.
    //
    if (Allocate8(&pUnic->pADigit))
    {
        return (1);
    }

    //
    //  For each entry in table, read in digit value and ascii digit
    //  translation value from input file, allocate necessary 16 word
    //  buffers based on wide character value, and store difference
    //  to ascii digit.
    //
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        //
        //  Read in digit and ascii digit values.
        //
        NumItems = fscanf( pInputFile,
                           "%x %x ;%*[^\n]",
                           &Digit,
                           &Ascii );
        if (NumItems != 2)
        {
            printf("Parse Error: Error reading ASCIIDIGITS values.\n");
            return (1);
        }

        if (Verbose)
            printf("  Digit = %x\tAscii = %x\n", Digit, Ascii);

        //
        //  Insert difference (Ascii - Digit) into 8:4:4 table.
        //
        if (Insert844( pUnic->pADigit,
                       (WORD)Digit,
                       (WORD)(Ascii - Digit),
                       &pUnic->ADBuf2,
                       &pUnic->ADBuf3,
                       sizeof(WORD) ))
        {
            return (1);
        }
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetFoldCZone
//
//  This routine gets the FOLDCZONE table from the input file.  It uses
//  the size parameter to know when to stop reading from the file.  If an
//  error is encountered, a message is printed and an error is returned.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

int GetFoldCZone(
    PUNICODE pUnic,
    int Size)
{
    int CZone;                    // compatibility zone value
    int Ascii;                    // ascii value
    register int Ctr;             // loop counter
    int NumItems;                 // number of items returned from fscanf


    //
    //  Allocate top buffer for 8:4:4 table - 256 pointers.
    //
    if (Allocate8(&pUnic->pCZone))
    {
        return (1);
    }

    //
    //  For each entry in table, read in czone value and ascii
    //  translation value from input file, allocate necessary 16 word
    //  buffers based on wide character value, and store difference to
    //  ascii value.
    //
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        //
        //  Read in CZone and Ascii values.
        //
        NumItems = fscanf( pInputFile,
                           "%x %x ;%*[^\n]",
                           &CZone,
                           &Ascii );
        if (NumItems != 2)
        {
            printf("Parse Error: Error reading FOLDCZONE values.\n");
            return (1);
        }

        if (Verbose)
            printf("  CZone = %x\tAscii = %x\n", CZone, Ascii);

        //
        //  Insert difference (Ascii - CZone) into 8:4:4 table.
        //
        if (Insert844( pUnic->pCZone,
                       (WORD)CZone,
                       (WORD)(Ascii - CZone),
                       &pUnic->CZBuf2,
                       &pUnic->CZBuf3,
                       sizeof(WORD) ))
        {
            return (1);
        }
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetHiragana
//
//  This routine gets the Hiragana table (Katakana to Hiragana) from the
//  input file.  It uses the size parameter to know when to stop reading
//  from the file.  If an error is encountered, a message is printed and
//  an error is returned.
//
//  07-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetHiragana(
    PUNICODE pUnic,
    int Size)
{
    int Kata;                     // Katakana value
    int Hira;                     // Hiragana value
    register int Ctr;             // loop counter
    int NumItems;                 // number of items returned from fscanf


    //
    //  Allocate top buffer for 8:4:4 table - 256 pointers.
    //
    if (Allocate8(&pUnic->pHiragana))
    {
        return (1);
    }

    //
    //  For each entry in table, read in katakana value and hiragana
    //  translation value from input file, allocate necessary 16 word
    //  buffers based on wide character value, and store difference to
    //  hiragana value.
    //
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        //
        //  Read in katakana and hiragana values.
        //
        NumItems = fscanf( pInputFile,
                           "%x %x ;%*[^\n]",
                           &Kata,
                           &Hira );
        if (NumItems != 2)
        {
            printf("Parse Error: Error reading HIRAGANA values.\n");
            return (1);
        }

        if (Verbose)
            printf("  Katakana = %x\tHiragana = %x\n", Kata, Hira);

        //
        //  Insert difference (Kata - Hira) into 8:4:4 table.
        //
        if (Insert844( pUnic->pHiragana,
                       (WORD)Kata,
                       (WORD)(Hira - Kata),
                       &pUnic->HGBuf2,
                       &pUnic->HGBuf3,
                       sizeof(WORD) ))
        {
            return (1);
        }
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetKatakana
//
//  This routine gets the Katakana table (Hiragana to Katakana) from the
//  input file.  It uses the size parameter to know when to stop reading
//  from the file.  If an error is encountered, a message is printed and
//  an error is returned.
//
//  07-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetKatakana(
    PUNICODE pUnic,
    int Size)
{
    int Hira;                     // Hiragana value
    int Kata;                     // Katakana value
    register int Ctr;             // loop counter
    int NumItems;                 // number of items returned from fscanf


    //
    //  Allocate top buffer for 8:4:4 table - 256 pointers.
    //
    if (Allocate8(&pUnic->pKatakana))
    {
        return (1);
    }

    //
    //  For each entry in table, read in hiragana value and katakana
    //  translation value from input file, allocate necessary 16 word
    //  buffers based on wide character value, and store difference to
    //  katakana value.
    //
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        //
        //  Read in hiragana and katakana values.
        //
        NumItems = fscanf( pInputFile,
                           "%x %x ;%*[^\n]",
                           &Hira,
                           &Kata );
        if (NumItems != 2)
        {
            printf("Parse Error: Error reading KATAKANA values.\n");
            return (1);
        }

        if (Verbose)
            printf("  Hiragana = %x\tKatakana = %x\n", Hira, Kata);

        //
        //  Insert difference (Hira - Kata) into 8:4:4 table.
        //
        if (Insert844( pUnic->pKatakana,
                       (WORD)Hira,
                       (WORD)(Kata - Hira),
                       &pUnic->KKBuf2,
                       &pUnic->KKBuf3,
                       sizeof(WORD) ))
        {
            return (1);
        }
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetHalfWidth
//
//  This routine gets the Half Width table (Full Width to Half Width) from
//  the input file.  It uses the size parameter to know when to stop reading
//  from the file.  If an error is encountered, a message is printed and
//  an error is returned.
//
//  07-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetHalfWidth(
    PUNICODE pUnic,
    int Size)
{
    int Full;                     // Full Width value
    int Half;                     // Half Width value
    register int Ctr;             // loop counter
    int NumItems;                 // number of items returned from fscanf


    //
    //  Allocate top buffer for 8:4:4 table - 256 pointers.
    //
    if (Allocate8(&pUnic->pHalfWidth))
    {
        return (1);
    }

    //
    //  For each entry in table, read in full width value and half width
    //  translation value from input file, allocate necessary 16 word
    //  buffers based on wide character value, and store difference to
    //  half width value.
    //
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        //
        //  Read in full width and half width values.
        //
        NumItems = fscanf( pInputFile,
                           "%x %x ;%*[^\n]",
                           &Full,
                           &Half );
        if (NumItems != 2)
        {
            printf("Parse Error: Error reading HALFWIDTH values.\n");
            return (1);
        }

        if (Verbose)
            printf("  Full Width = %x\tHalf Width = %x\n", Full, Half);

        //
        //  Insert difference (Full - Half) into 8:4:4 table.
        //
        if (Insert844( pUnic->pHalfWidth,
                       (WORD)Full,
                       (WORD)(Half - Full),
                       &pUnic->HWBuf2,
                       &pUnic->HWBuf3,
                       sizeof(WORD) ))
        {
            return (1);
        }
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetFullWidth
//
//  This routine gets the Full Width table (Half Width to Full Width) from
//  the input file.  It uses the size parameter to know when to stop reading
//  from the file.  If an error is encountered, a message is printed and
//  an error is returned.
//
//  07-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetFullWidth(
    PUNICODE pUnic,
    int Size)
{
    int Half;                     // Half Width value
    int Full;                     // Full Width value
    register int Ctr;             // loop counter
    int NumItems;                 // number of items returned from fscanf


    //
    //  Allocate top buffer for 8:4:4 table - 256 pointers.
    //
    if (Allocate8(&pUnic->pFullWidth))
    {
        return (1);
    }

    //
    //  For each entry in table, read in half width value and full width
    //  translation value from input file, allocate necessary 16 word
    //  buffers based on wide character value, and store difference to
    //  full width value.
    //
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        //
        //  Read in half width and full width values.
        //
        NumItems = fscanf( pInputFile,
                           "%x %x ;%*[^\n]",
                           &Half,
                           &Full );
        if (NumItems != 2)
        {
            printf("Parse Error: Error reading FULLWIDTH values.\n");
            return (1);
        }

        if (Verbose)
            printf("  Half Width = %x\tFull Width = %x\n", Half, Full);

        //
        //  Insert difference (Half - Full) into 8:4:4 table.
        //
        if (Insert844( pUnic->pFullWidth,
                       (WORD)Half,
                       (WORD)(Full - Half),
                       &pUnic->FWBuf2,
                       &pUnic->FWBuf3,
                       sizeof(WORD) ))
        {
            return (1);
        }
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetTraditional
//
//  This routine gets the Traditional table (Simplified to Traditional) from
//  the input file.  It uses the size parameter to know when to stop reading
//  from the file.  If an error is encountered, a message is printed and
//  an error is returned.
//
//  05-07-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetTraditional(
    PUNICODE pUnic,
    int Size)
{
    int Simplified;               // Simplified value
    int Traditional;              // Traditional value
    register int Ctr;             // loop counter
    int NumItems;                 // number of items returned from fscanf


    //
    //  Allocate top buffer for 8:4:4 table - 256 pointers.
    //
    if (Allocate8(&pUnic->pTraditional))
    {
        return (1);
    }

    //
    //  For each entry in table, read in simplified value and traditional
    //  translation value from input file, allocate necessary 16 word
    //  buffers based on wide character value, and store difference to
    //  traditional value.
    //
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        //
        //  Read in simplified and traditional values.
        //
        NumItems = fscanf( pInputFile,
                           "%x %x ;%*[^\n]",
                           &Simplified,
                           &Traditional );
        if (NumItems != 2)
        {
            printf("Parse Error: Error reading TRADITIONAL_CHINESE values.\n");
            return (1);
        }

        if (Verbose)
            printf("  Simplified = %x\tTraditional = %x\n",
                    Simplified, Traditional);

        //
        //  Insert difference (Simplified - Traditional) into 8:4:4 table.
        //
        if (Insert844( pUnic->pTraditional,
                       (WORD)Simplified,
                       (WORD)(Traditional - Simplified),
                       &pUnic->TRBuf2,
                       &pUnic->TRBuf3,
                       sizeof(WORD) ))
        {
            return (1);
        }
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetSimplified
//
//  This routine gets the Simplified table (Traditional to Simplified) from
//  the input file.  It uses the size parameter to know when to stop reading
//  from the file.  If an error is encountered, a message is printed and
//  an error is returned.
//
//  05-07-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetSimplified(
    PUNICODE pUnic,
    int Size)
{
    int Traditional;              // Traditional value
    int Simplified;               // Simplified value
    register int Ctr;             // loop counter
    int NumItems;                 // number of items returned from fscanf


    //
    //  Allocate top buffer for 8:4:4 table - 256 pointers.
    //
    if (Allocate8(&pUnic->pSimplified))
    {
        return (1);
    }

    //
    //  For each entry in table, read in traditional value and simplified
    //  translation value from input file, allocate necessary 16 word
    //  buffers based on wide character value, and store difference to
    //  simplified value.
    //
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        //
        //  Read in traditional and simplified values.
        //
        NumItems = fscanf( pInputFile,
                           "%x %x ;%*[^\n]",
                           &Traditional,
                           &Simplified );
        if (NumItems != 2)
        {
            printf("Parse Error: Error reading SIMPLIFIED_CHINESE values.\n");
            return (1);
        }

        if (Verbose)
            printf("  Traditional = %x\tSimplified = %x\n",
                    Traditional, Simplified);

        //
        //  Insert difference (Traditional - Simplified) into 8:4:4 table.
        //
        if (Insert844( pUnic->pSimplified,
                       (WORD)Traditional,
                       (WORD)(Simplified - Traditional),
                       &pUnic->SPBuf2,
                       &pUnic->SPBuf3,
                       sizeof(WORD) ))
        {
            return (1);
        }
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetCompTable
//
//  This routine gets the precomposed table and the composite table from
//  the input file.  It uses the size parameter to know when to stop reading
//  from the file.  If an error is encountered, a message is printed and an
//  error is returned.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

int GetCompTable(
    PUNICODE pUnic,
    int Size)
{
    int  PreComp;                 // precomposed character
    int  Base;                    // base character
    int  NonSp;                   // nonspace character
    DWORD Combo;                  // combined base and nonspace
    register int Ctr;             // loop counter
    WORD BOff, NOff;              // offsets for Base and NonSpace chars
    int NumItems;                 // number of items returned from fscanf


    //
    //  Allocate top buffers for 8:4:4 tables - 256 pointers.
    //
    if (Allocate8(&pUnic->pPreComp))
    {
        return (1);
    }
    if (Allocate8(&pUnic->pBase))
    {
        return (1);
    }
    if (Allocate8(&pUnic->pNonSp))
    {
        return (1);
    }

    //
    //  Allocate 2D grid for Composite table and save Size
    //  in the first position of the grid.
    //
    if (AllocateGrid(&pUnic->pCompGrid, Size))
    {
        return (1);
    }

    //
    //  For each entry in table, read in precomposed, base, and nonspace
    //  characters from input file and build the necessary tables.
    //
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        //
        //  Read in precomposed, base, and nonspace characters.
        //
        NumItems = fscanf( pInputFile,
                           "%x %x %x ;%*[^\n]",
                           &PreComp,
                           &Base,
                           &NonSp );
        if (NumItems != 3)
        {
            printf("Parse Error: Error reading COMPTABLE values.\n");
            return (1);
        }

        if (Verbose)
            printf("  PreComp = %x\tBase = %x\tNonSp = %x\n",
                      PreComp, Base, NonSp);

        //
        //  PRECOMPOSED TABLE:
        //
        //  Convert Base and NonSp into one DWORD value - Combo.
        //  Base = high word, NonSp = low word.
        //
        //  Insert Combo information into Precomposed 8:4:4 table.
        //
        Combo = MAKE_DWORD((WORD)NonSp, (WORD)Base);
        if (Insert844( pUnic->pPreComp,
                       (WORD)PreComp,
                       Combo,
                       &pUnic->PCBuf2,
                       &pUnic->PCBuf3,
                       sizeof(DWORD) ))
        {
            return (1);
        }

        if (Verbose)
            printf("    Combo = %x\n", Combo);

        //
        //  COMPOSITE TABLE:
        //
        //  Insert offsets into BASE and NONSPACE 8:4:4 tables.
        //  Insert PRECOMPOSED into 2D grid.
        //
        Get844Value( pUnic->pBase,
                     (WORD)Base,
                     &BOff );
        Get844Value( pUnic->pNonSp,
                     (WORD)NonSp,
                     &NOff );
        if (BOff == 0)
        {
            BOff = (WORD)(++(pUnic->NumBase));
            if (Insert844( pUnic->pBase,
                           (WORD)Base,
                           BOff,
                           &pUnic->BSBuf2,
                           &pUnic->BSBuf3,
                           sizeof(WORD) ))
            {
                return (1);
            }
        }
        if (NOff == 0)
        {
            NOff = (WORD)(++(pUnic->NumNonSp));
            if (Insert844( pUnic->pNonSp,
                           (WORD)NonSp,
                           NOff,
                           &pUnic->NSBuf2,
                           &pUnic->NSBuf3,
                           sizeof(WORD) ))
            {
                return (1);
            }
        }

        if (Verbose)
            printf("    BOff = %x\tNOff = %x\n", BOff, NOff);

        InsertCompGrid( pUnic->pCompGrid,
                        (WORD)PreComp,
                        BOff,
                        NOff );
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  Get844Value
//
//  This routine gets the value in the 8:4:4 table of the given wide character.
//  If a value is found, it returns the value found in Value.  Otherwise,
//  it returns 0 in Value.
//
//  07-30-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

void Get844Value(
    P844_ARRAY pArr,
    WORD WChar,
    WORD *Value)
{
    P844_ARRAY pMidFour;
    WORD *pFour;


    //
    //  Traverse 8:4:4 table for value.
    //
    if (pMidFour = (P844_ARRAY)(pArr[GET8(WChar)]))
    {
        if (pFour = pMidFour[GETHI4(WChar)])
        {
            *Value = pFour[GETLO4(WChar)];
        }
        else
        {
            *Value = 0;
        }
    }
    else
    {
        *Value = 0;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  InsertCompGrid
//
//  This routine inserts the given precomposed character at the given
//  offset in the 2D grid.  The matrix size of the grid is located in
//  the first spot of the grid array.
//
//  The grid is set up in memory as follows:
//
//     [  base 1  ][  base 2 ][  base 3  ]
//
//        where the size of each is GridSize.
//
//     In other words, BASE     characters  =>  ROWS
//                     NONSPACE characters  =>  COLUMNS
//
//  NOTE: BaseOff and NonSpOff start at 1, not 0.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

void InsertCompGrid(
    PCOMP_GRID pCompGrid,
    WORD PreComp,
    WORD BaseOff,
    WORD NonSpOff)
{
    int Index;


    Index = ((BaseOff - 1) * pCompGrid[0] + (NonSpOff - 1)) + 1;
    pCompGrid[Index] = PreComp;

    if (Verbose)
        printf("    Grid Spot = %d\tMax = %d\n", Index, pCompGrid[0]);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteAsciiDigits
//
//  This routine writes the Ascii Digits information to the output file.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

int WriteAsciiDigits(
    PUNICODE pUnic,
    FILE *pOutputFile)
{
    int TblSize;                  // size of table
    WORD wValue;                  // temp storage value


    if (Verbose)
        printf("\nWriting Ascii Digits Table...\n");

    //
    //  Compute size of table.
    //
    TblSize = Compute844Size( pUnic->ADBuf2,
                              pUnic->ADBuf3,
                              sizeof(WORD) ) + 1;

    //
    //  Make sure the total size of the table is not greater than 64K.
    //  If it is, then the WORD offsets are too small.
    //
    if (TblSize > MAX_844_TBL_SIZE)
    {
       printf("Write Error: Size of Ascii Digits table is greater than 64K.\n");
       return (1);
    }

    //
    //  Write the size to the output file.
    //
    wValue = (WORD)TblSize;
    if (FileWrite( pOutputFile,
                   &wValue,
                   sizeof(WORD),
                   1,
                   "Ascii Digits size" ))
    {
        return (1);
    }

    //
    //  Write Ascii Digits 8:4:4 table to file.
    //
    if (Write844Table( pOutputFile,
                       pUnic->pADigit,
                       pUnic->ADBuf2,
                       TblSize - 1,
                       sizeof(WORD) ))
    {
        return (1);
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteFoldCZone
//
//  This routine writes the Fold Compatibility Zone information to the
//  output file.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

int WriteFoldCZone(
    PUNICODE pUnic,
    FILE *pOutputFile)
{
    int TblSize;                  // size of table
    WORD wValue;                  // temp storage value


    if (Verbose)
        printf("\nWriting Fold CZone Table...\n");

    //
    //  Compute size of table.
    //
    TblSize = Compute844Size( pUnic->CZBuf2,
                              pUnic->CZBuf3,
                              sizeof(WORD) ) + 1;

    //
    //  Make sure the total size of the table is not greater than 64K.
    //  If it is, then the WORD offsets are too small.
    //
    if (TblSize > MAX_844_TBL_SIZE)
    {
       printf("Write Error: Size of Fold CZone table is greater than 64K.\n");
       return (1);
    }

    //
    //  Write the size to the output file.
    //
    wValue = (WORD)TblSize;
    if (FileWrite( pOutputFile,
                   &wValue,
                   sizeof(WORD),
                   1,
                   "Fold CZone size" ))
    {
        return (1);
    }

    //
    //  Write Ascii Digits 8:4:4 table to file.
    //
    if (Write844Table( pOutputFile,
                       pUnic->pCZone,
                       pUnic->CZBuf2,
                       TblSize - 1,
                       sizeof(WORD) ))
    {
        return (1);
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteHiragana
//
//  This routine writes the Hiragana information to the output file.
//
//  07-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteHiragana(
    PUNICODE pUnic,
    FILE *pOutputFile)
{
    int TblSize;                  // size of table
    WORD wValue;                  // temp storage value


    if (Verbose)
        printf("\nWriting Hiragana Table...\n");

    //
    //  Compute size of table.
    //
    TblSize = Compute844Size( pUnic->HGBuf2,
                              pUnic->HGBuf3,
                              sizeof(WORD) ) + 1;

    //
    //  Make sure the total size of the table is not greater than 64K.
    //  If it is, then the WORD offsets are too small.
    //
    if (TblSize > MAX_844_TBL_SIZE)
    {
       printf("Write Error: Size of Hiragana table is greater than 64K.\n");
       return (1);
    }

    //
    //  Write the size to the output file.
    //
    wValue = (WORD)TblSize;
    if (FileWrite( pOutputFile,
                   &wValue,
                   sizeof(WORD),
                   1,
                   "Hiragana size" ))
    {
        return (1);
    }

    //
    //  Write Hiragana 8:4:4 table to file.
    //
    if (Write844Table( pOutputFile,
                       pUnic->pHiragana,
                       pUnic->HGBuf2,
                       TblSize - 1,
                       sizeof(WORD) ))
    {
        return (1);
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteKatakana
//
//  This routine writes the Katakana information to the output file.
//
//  07-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteKatakana(
    PUNICODE pUnic,
    FILE *pOutputFile)
{
    int TblSize;                  // size of table
    WORD wValue;                  // temp storage value


    if (Verbose)
        printf("\nWriting Katakana Table...\n");

    //
    //  Compute size of table.
    //
    TblSize = Compute844Size( pUnic->KKBuf2,
                              pUnic->KKBuf3,
                              sizeof(WORD) ) + 1;

    //
    //  Make sure the total size of the table is not greater than 64K.
    //  If it is, then the WORD offsets are too small.
    //
    if (TblSize > MAX_844_TBL_SIZE)
    {
       printf("Write Error: Size of Katakana table is greater than 64K.\n");
       return (1);
    }

    //
    //  Write the size to the output file.
    //
    wValue = (WORD)TblSize;
    if (FileWrite( pOutputFile,
                   &wValue,
                   sizeof(WORD),
                   1,
                   "Katakana size" ))
    {
        return (1);
    }

    //
    //  Write Katakana 8:4:4 table to file.
    //
    if (Write844Table( pOutputFile,
                       pUnic->pKatakana,
                       pUnic->KKBuf2,
                       TblSize - 1,
                       sizeof(WORD) ))
    {
        return (1);
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteHalfWidth
//
//  This routine writes the Half Width information to the output file.
//
//  07-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteHalfWidth(
    PUNICODE pUnic,
    FILE *pOutputFile)
{
    int TblSize;                  // size of table
    WORD wValue;                  // temp storage value


    if (Verbose)
        printf("\nWriting Half Width Table...\n");

    //
    //  Compute size of table.
    //
    TblSize = Compute844Size( pUnic->HWBuf2,
                              pUnic->HWBuf3,
                              sizeof(WORD) ) + 1;

    //
    //  Make sure the total size of the table is not greater than 64K.
    //  If it is, then the WORD offsets are too small.
    //
    if (TblSize > MAX_844_TBL_SIZE)
    {
       printf("Write Error: Size of Half Width table is greater than 64K.\n");
       return (1);
    }

    //
    //  Write the size to the output file.
    //
    wValue = (WORD)TblSize;
    if (FileWrite( pOutputFile,
                   &wValue,
                   sizeof(WORD),
                   1,
                   "Half Width size" ))
    {
        return (1);
    }

    //
    //  Write Half Width 8:4:4 table to file.
    //
    if (Write844Table( pOutputFile,
                       pUnic->pHalfWidth,
                       pUnic->HWBuf2,
                       TblSize - 1,
                       sizeof(WORD) ))
    {
        return (1);
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteFullWidth
//
//  This routine writes the Full Width information to the output file.
//
//  07-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteFullWidth(
    PUNICODE pUnic,
    FILE *pOutputFile)
{
    int TblSize;                  // size of table
    WORD wValue;                  // temp storage value


    if (Verbose)
        printf("\nWriting Full Width Table...\n");

    //
    //  Compute size of table.
    //
    TblSize = Compute844Size( pUnic->FWBuf2,
                              pUnic->FWBuf3,
                              sizeof(WORD) ) + 1;

    //
    //  Make sure the total size of the table is not greater than 64K.
    //  If it is, then the WORD offsets are too small.
    //
    if (TblSize > MAX_844_TBL_SIZE)
    {
       printf("Write Error: Size of Full Width table is greater than 64K.\n");
       return (1);
    }

    //
    //  Write the size to the output file.
    //
    wValue = (WORD)TblSize;
    if (FileWrite( pOutputFile,
                   &wValue,
                   sizeof(WORD),
                   1,
                   "Full Width size" ))
    {
        return (1);
    }

    //
    //  Write Full Width 8:4:4 table to file.
    //
    if (Write844Table( pOutputFile,
                       pUnic->pFullWidth,
                       pUnic->FWBuf2,
                       TblSize - 1,
                       sizeof(WORD) ))
    {
        return (1);
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteTraditional
//
//  This routine writes the Traditional information to the output file.
//
//  05-07-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteTraditional(
    PUNICODE pUnic,
    FILE *pOutputFile)
{
    int TblSize;                  // size of table
    WORD wValue;                  // temp storage value


    if (Verbose)
        printf("\nWriting Traditional Chinese Table...\n");

    //
    //  Compute size of table.
    //
    TblSize = Compute844Size( pUnic->TRBuf2,
                              pUnic->TRBuf3,
                              sizeof(WORD) ) + 1;

    //
    //  Make sure the total size of the table is not greater than 64K.
    //  If it is, then the WORD offsets are too small.
    //
    if (TblSize > MAX_844_TBL_SIZE)
    {
       printf("Write Error: Size of Traditional Chinese table is greater than 64K.\n");
       return (1);
    }

    //
    //  Write the size to the output file.
    //
    wValue = (WORD)TblSize;
    if (FileWrite( pOutputFile,
                   &wValue,
                   sizeof(WORD),
                   1,
                   "Traditional Chinese size" ))
    {
        return (1);
    }

    //
    //  Write Traditional 8:4:4 table to file.
    //
    if (Write844Table( pOutputFile,
                       pUnic->pTraditional,
                       pUnic->TRBuf2,
                       TblSize - 1,
                       sizeof(WORD) ))
    {
        return (1);
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteSimplified
//
//  This routine writes the Simplified information to the output file.
//
//  05-07-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteSimplified(
    PUNICODE pUnic,
    FILE *pOutputFile)
{
    int TblSize;                  // size of table
    WORD wValue;                  // temp storage value


    if (Verbose)
        printf("\nWriting Simplified Chinese Table...\n");

    //
    //  Compute size of table.
    //
    TblSize = Compute844Size( pUnic->SPBuf2,
                              pUnic->SPBuf3,
                              sizeof(WORD) ) + 1;

    //
    //  Make sure the total size of the table is not greater than 64K.
    //  If it is, then the WORD offsets are too small.
    //
    if (TblSize > MAX_844_TBL_SIZE)
    {
       printf("Write Error: Size of Simplified Chinese table is greater than 64K.\n");
       return (1);
    }

    //
    //  Write the size to the output file.
    //
    wValue = (WORD)TblSize;
    if (FileWrite( pOutputFile,
                   &wValue,
                   sizeof(WORD),
                   1,
                   "Simplified Chinese size" ))
    {
        return (1);
    }

    //
    //  Write Simplified 8:4:4 table to file.
    //
    if (Write844Table( pOutputFile,
                       pUnic->pSimplified,
                       pUnic->SPBuf2,
                       TblSize - 1,
                       sizeof(WORD) ))
    {
        return (1);
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  WritePrecomposed
//
//  This routine writes the Precomposed information to the output file.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

int WritePrecomposed(
    PUNICODE pUnic,
    FILE *pOutputFile)
{
    int TblSize;                  // size of table
    WORD wValue;                  // temp storage value


    if (Verbose)
        printf("\nWriting PRECOMPOSED Table...\n");

    //
    //  Compute size of table.
    //
    TblSize = Compute844Size( pUnic->PCBuf2,
                              pUnic->PCBuf3,
                              sizeof(DWORD) ) + 1;

    //
    //  Make sure the total size of the table is not greater than 64K.
    //  If it is, then the WORD offsets are too small.
    //
    if (TblSize > MAX_844_TBL_SIZE)
    {
       printf("Write Error: Size of PRECOMPOSED table is greater than 64K.\n");
       return (1);
    }

    //
    //  Write the size to the output file.
    //
    wValue = (WORD)TblSize;
    if (FileWrite( pOutputFile,
                   &wValue,
                   sizeof(WORD),
                   1,
                   "PRECOMPOSED size" ))
    {
        return (1);
    }

    //
    //  Write PRECOMPOSED 8:4:4 table to file.
    //
    if (Write844Table( pOutputFile,
                       pUnic->pPreComp,
                       pUnic->PCBuf2,
                       TblSize - 1,
                       sizeof(DWORD) ))
    {
        return (1);
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteComposite
//
//  This routine writes the Composite information to the output file.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

int WriteComposite(
    PUNICODE pUnic,
    FILE *pOutputFile)
{
    int BaseSize;                 // size of Base table
    int NonSpSize;                // size of NonSpace table
    WORD wValue;                  // temp storage value
    BYTE bValue;                  // temp storage value


    if (Verbose)
        printf("\nWriting COMPOSITE Table...\n");

    //
    //  Compute size of base table.
    //
    BaseSize = Compute844Size( pUnic->BSBuf2,
                               pUnic->BSBuf3,
                               sizeof(WORD) );

    //
    //  Make sure the total size of the table is not greater than 64K.
    //  If it is, then the WORD offsets are too small.
    //
    if (BaseSize > MAX_844_TBL_SIZE)
    {
       printf("Write Error: Size of PRECOMPOSED table is greater than 64K.\n");
       return (1);
    }

    //
    //  Write the size to the output file.
    //
    wValue = (WORD)BaseSize;
    if (FileWrite( pOutputFile,
                   &wValue,
                   sizeof(WORD),
                   1,
                   "COMPOSITE BASE size" ))
    {
        return (1);
    }

    //
    //  Compute size of nonspace table.
    //
    NonSpSize = Compute844Size( pUnic->NSBuf2,
                                pUnic->NSBuf3,
                                sizeof(WORD) );

    //
    //  Make sure the total size of the table is not greater than 64K.
    //  If it is, then the WORD offsets are too small.
    //
    if (NonSpSize > MAX_844_TBL_SIZE)
    {
       printf("Write Error: Size of PRECOMPOSED table is greater than 64K.\n");
       return (1);
    }

    //
    //  Write the size to the output file.
    //
    wValue = (WORD)NonSpSize;
    if (FileWrite( pOutputFile,
                   &wValue,
                   sizeof(WORD),
                   1,
                   "COMPOSITE NONSPACE size" ))
    {
        return (1);
    }

    //
    //  Write number of base chars to output file.
    //
    bValue = (BYTE)(pUnic->NumBase);
    if (FileWrite( pOutputFile,
                   &bValue,
                   sizeof(BYTE),
                   1,
                   "COMPOSITE BASE number" ))
    {
        return (1);
    }

    //
    //  Write number of nonspace chars to output file.
    //
    bValue = (BYTE)(pUnic->NumNonSp);
    if (FileWrite( pOutputFile,
                   &bValue,
                   sizeof(BYTE),
                   1,
                   "COMPOSITE NONSPACE number" ))
    {
        return (1);
    }

    //
    //  Write Base Character 8:4:4 table to output file.
    //
    if (Write844Table( pOutputFile,
                       pUnic->pBase,
                       pUnic->BSBuf2,
                       BaseSize,
                       sizeof(WORD) ))
    {
        return (1);
    }

    //
    //  Write NonSpace Character 8:4:4 table to output file.
    //
    if (Write844Table( pOutputFile,
                       pUnic->pNonSp,
                       pUnic->NSBuf2,
                       NonSpSize,
                       sizeof(WORD) ))
    {
        return (1);
    }

    //
    //  Write 2D Grid to output file.
    //
    if (WriteGrid( pUnic,
                   pOutputFile ))
    {
        return (1);
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteGrid
//
//  This routine writes the Composite Table 2D Grid to the output file.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

int WriteGrid(
    PUNICODE pUnic,
    FILE *pOutputFile)
{
    int GridSize;                      // get matrix size of grid
    register int Ctr;                  // loop counter
    register PCOMP_GRID ptr;           // temp ptr into grid


    GridSize = (pUnic->pCompGrid)[0];
    ptr = pUnic->pCompGrid + 1;

    for (Ctr = 0; Ctr < pUnic->NumBase; Ctr++)
    {
        if (FileWrite( pOutputFile,
                       ptr,
                       sizeof(WORD),
                       pUnic->NumNonSp,
                       "COMPOSITE GRID" ))
        {
            return (1);
        }

        ptr += GridSize;
    }

    //
    //  Return success.
    //
    return (0);
}
