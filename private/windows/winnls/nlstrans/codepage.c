/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    codepage.c

Abstract:

    This file contains functions necessary to parse and write the code page
    specific tables to a data file.

    External Routines in this file:
      ParseCodePage
      WriteCodePage

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
GetMBTable(
    PCODEPAGE pCP,
    int Size);

int
GetGlyphTable(
    PCODEPAGE pCP,
    int Size);

int
GetDBCSRanges(
    PCODEPAGE pCP,
    int Size);

int
GetAllDBCSTables(
    int NumTables,
    PDBCS_TBL_ARRAY pArray);

int
GetDBCSTable(
    int Size,
    PDBCS_TBL pTable);

int
GetWCTable(
    PCODEPAGE pCP,
    int Size,
    BOOL IsMBCodePage);

int
WriteCPInfo(
    PCODEPAGE pCP,
    FILE *pOutputFile);

int
WriteMB(
    PCODEPAGE pCP,
    FILE *pOutputFile);

int
WriteWC(
    PCODEPAGE pCP,
    BOOL IsMBCodePage,
    FILE *pOutputFile);

void
FreeMB(
    PCODEPAGE pCP);

int
GetTransDefaultChars(
    PCODEPAGE pCP);





//-------------------------------------------------------------------------//
//                            EXTERNAL ROUTINES                            //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  ParseCodePage
//
//  This routine parses the input file for the code page specific tables.
//  This routine is only entered when the CODEPAGE keyword is found.
//  The parsing continues until the ENDCODEPAGE keyword is found.
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ParseCodePage(
    PCODEPAGE pCP,
    PSZ pszKeyWord)
{
    int size;                     // size of table to follow
    int DefChar;                  // default character
    int UniDefChar;               // unicode default char
    int NumItems;                 // number of items returned from fscanf


    //
    //  Get CodePageValue parameter.
    //
    pCP->CodePageValue = atoi(pCP->pszName);

    //
    //  Read in the rest of the code page information.
    //
    while (fscanf(pInputFile, "%s", pszKeyWord) == 1)
    {
        if (_strcmpi(pszKeyWord, "CPINFO") == 0)
        {
            if (Verbose)
                printf("\n\nFound CPINFO keyword.\n");

            //
            //  Get MaxCharSize parameter.
            //  Get DefaultChar parameter.
            //  Get Unicode Translation of default char parameter.
            //
            NumItems = fscanf( pInputFile,
                               "%d %x %x ;%*[^\n]",
                               &size,
                               &DefChar,
                               &UniDefChar );
            if ((NumItems != 3) || ((size != 1) && (size != 2)))
            {
                return (1);
            }

            pCP->MaxCharSize = size;
            pCP->DefaultChar = (WORD)DefChar;
            pCP->UniDefaultChar = (WORD)UniDefChar;

            if (Verbose)
            {
                printf("  MAXCHARSIZE = %d\n", size);
                printf("  DEFAULTCHAR = %x\n\n", DefChar);
                printf("  UNICODE DEFAULT CHAR = %x\n\n", UniDefChar);
            }

            //
            //  Set WriteFlags for CPINFO Table.
            //
            pCP->WriteFlags |= F_CPINFO;
        }

        else if (_strcmpi(pszKeyWord, "MBTABLE") == 0)
        {
            if (Verbose)
                printf("\n\nFound MBTABLE keyword.\n");

            //
            //  Get size parameter.
            //
            if (GetSize(&size))
                return (1);

            //
            //  Get MB table.
            //
            if (GetMBTable(pCP, size))
            {
                return (1);
            }

            //
            //  Set WriteFlags for MB Table.
            //
            pCP->WriteFlags |= F_MB;
        }

        else if (_strcmpi(pszKeyWord, "GLYPHTABLE") == 0)
        {
            if (Verbose)
                printf("\n\nFound GLYPHTABLE keyword.\n");

            //
            //  Get size parameter.
            //
            if (GetSize(&size))
                return (1);

            //
            //  Get GLYPH table.
            //
            if (GetGlyphTable(pCP, size))
            {
                return (1);
            }

            //
            //  Set WriteFlags for GLYPH Table.
            //
            pCP->WriteFlags |= F_GLYPH;
        }

        else if (_strcmpi(pszKeyWord, "DBCSRANGE") == 0)
        {
            if (Verbose)
                printf("\n\nFound DBCSRANGE keyword.\n");

            //
            //  Get size parameter.
            //
            if (GetSize(&size))
                return (1);

            //
            //  Get DBCS ranges and tables.
            //
            if (GetDBCSRanges(pCP, size))
            {
                return (1);
            }

            //
            //  Set WriteFlags for DBCS Table.
            //
            pCP->WriteFlags |= F_DBCS;
        }

        else if (_strcmpi(pszKeyWord, "WCTABLE") == 0)
        {
            if (Verbose)
                printf("\n\nFound WCTABLE keyword.\n");

            //
            //  Get size parameter.
            //
            if (GetSize(&size))
                return (1);

            //
            //  Get WC Table.
            //
            if (GetWCTable( pCP,
                            size,
                            (pCP->MaxCharSize - 1) ))
            {
                return (1);
            }

            //
            //  Set WriteFlags for WC Table.
            //
            pCP->WriteFlags |= F_WC;
        }

        else if (_strcmpi(pszKeyWord, "ENDCODEPAGE") == 0)
        {
            if (Verbose)
                printf("\n\nFound ENDCODEPAGE keyword.\n");

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
    //  If this point is reached, then the ENDCODEPAGE keyword was
    //  not found.  Return an error.
    //
    printf("Parse Error: Expecting ENDCODEPAGE keyword.\n");
    return (1);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteCodePage
//
//  This routine writes the code page specific tables to an output file.
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteCodePage(
    PCODEPAGE pCP)
{
    char pszFile[FILE_NAME_LEN];       // file name storage
    FILE *pOutputFile;                 // ptr to output file


    //
    //  Make sure all tables are present.
    //
    if (!((pCP->WriteFlags & F_CPINFO) &&
          (pCP->WriteFlags & F_MB) &&
          (pCP->WriteFlags & F_WC)))
    {
        printf("Write Error: All tables must be present -\n");
        printf("             CPInfo, MultiByte, and WideChar Translation Tables.\n");
        return (1);
    }

    //
    //  Get the name of the output file.
    //
    memset(pszFile, 0, FILE_NAME_LEN * sizeof(char));
    strcpy(pszFile, CP_PREFIX);
    strcat(pszFile, pCP->pszName);
    strcat(pszFile, DATA_FILE_SUFFIX);

    //
    //  Make sure output file can be opened for writing.
    //
    if ((pOutputFile = fopen(pszFile, "w+b")) == 0)
    {
        printf("Error opening output file %s.\n", pszFile);
        return (1);
    }

    if (Verbose)
        printf("\n\nWriting output file %s...\n", pszFile);

    //
    //  Write the CPINFO.
    //
    if (WriteCPInfo(pCP, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }


    //
    //  Write MB Table, Glyph Table (if any) and DBCS Table (if any) to
    //  output file.
    //
    if (WriteMB(pCP, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Free MB table structures.
    //
    FreeMB(pCP);


    //
    //  Write WC Table to output file.
    //
    if (WriteWC( pCP,
                 (pCP->WriteFlags & F_DBCS),
                 pOutputFile ))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Free WC table structures.
    //
    free(pCP->pWC);


    //
    //  Close the output file.
    //
    fclose(pOutputFile);

    //
    //  Return success.
    //
    printf("\nSuccessfully wrote output file %s\n", pszFile);
    return (0);
}




//-------------------------------------------------------------------------//
//                            INTERNAL ROUTINES                            //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  GetMBTable
//
//  This routine gets the multibyte table from the input file.  It uses the
//  size parameter to know when to stop reading from the file.  If an error
//  is encountered, a message is printed and an error is returned.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

int GetMBTable(
    PCODEPAGE pCP,
    int Size)
{
    int Ctr;                      // loop counter
    int Index;                    // index into array - single byte char
    int Value;                    // value - WC translation
    int NumItems;                 // number of items returned from fscanf


    //
    //  Allocate MB table.
    //
    if (AllocateMB(pCP))
    {
        return (1);
    }

    //
    //  For each table entry, read the MB char and the wide char
    //  from the input file and store the wide char in the array.
    //
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        //
        //  Get the index and the value to store from the file.
        //
        NumItems = fscanf( pInputFile,
                           "%x %x ;%*[^\n]",
                           &Index,
                           &Value );
        if (NumItems != 2)
        {
            printf("Parse Error: Error reading MBTABLE values.\n");
            return (1);
        }

        if (Index > MB_TABLE_SIZE)
        {
            printf("Parse Error: Multibyte char value too large.\n");
            printf("             Value must be less than 0x%x.\n", MB_TABLE_SIZE);
            return (1);
        }

        //
        //  Store the wide character value in the array.
        //
        (pCP->pMB)[Index] = (WORD)Value;

        if (Verbose)
            printf("  MB = %x\tWC = %x\n", Index, Value);
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetGlyphTable
//
//  This routine gets the glyph table from the input file.  It uses the
//  size parameter to know when to stop reading from the file.  If an error
//  is encountered, a message is printed and an error is returned.
//
//  06-02-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetGlyphTable(
    PCODEPAGE pCP,
    int Size)
{
    int Ctr;                      // loop counter
    int Index;                    // index into array - single byte char
    int Value;                    // value - WC translation
    int NumItems;                 // number of items returned from fscanf


    //
    //  Allocate Glyph table.
    //
    if (AllocateGlyph(pCP))
    {
        return (1);
    }

    //
    //  For each table entry, read the MB char and the wide char
    //  from the input file and store the wide char in the array.
    //
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        //
        //  Get the index and the value to store from the file.
        //
        NumItems = fscanf( pInputFile,
                           "%x %x ;%*[^\n]",
                           &Index,
                           &Value );
        if (NumItems != 2)
        {
            printf("Parse Error: Error reading GLYPHTABLE values.\n");
            return (1);
        }

        if (Index > GLYPH_TABLE_SIZE)
        {
            printf("Parse Error: Multibyte char value too large.\n");
            printf("             Value must be less than 0x%x.\n", GLYPH_TABLE_SIZE);
            return (1);
        }

        //
        //  Store the wide character value in the array.
        //
        (pCP->pGlyph)[Index] = (WORD)Value;

        if (Verbose)
            printf("  MB = %x\tWC = %x\n", Index, Value);
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetDBCSRanges
//
//  This routine gets the DBCS ranges from the input file.  It uses the
//  size parameter to know when to stop reading from the file.  If an error
//  is encountered, a message is printed and an error is returned.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

int GetDBCSRanges(
    PCODEPAGE pCP,
    int Size)
{
    int Ctr;                           // loop counter
    int Ctr2;                          // loop counter
    int Low;                           // low end range value
    int High;                          // high end range value
    int Offset = DBCS_OFFSET_SIZE;     // offset to DBCS table
    int NumItems;                      // # of items returned from fscanf


    //
    //  Save the number of ranges for later.
    //
    if ((Size < 1) || (Size > 5))
    {
        printf("Parse Error: Number of DBCS Ranges must be between 1 and 5.\n");
        return (1);
    }
    pCP->NumDBCSRanges = Size;


    //
    //  Allocate initial DBCS array structure.
    //
    if (AllocateTopDBCS(pCP, Size))
    {
        return (1);
    }

    //
    //  For each range, read the low range, the high range, and the
    //  DBCS tables for these ranges.  DBCS tables MUST be in the
    //  correct order (low range to high range).
    //
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        //
        //  Read low and high range.
        //
        NumItems = fscanf( pInputFile,
                           "%x %x ;%*[^\n]",
                           &Low,
                           &High );
        if (NumItems != 2)
        {
            printf("Parse Error: Error reading DBCS Range values.\n");
            return (1);
        }
        if (High < Low)
        {
            printf("Parse Error: High Range must be greater than Low Range.\n");
            return (1);
        }

        //
        //  Allocate DBCS structures.
        //
        if (AllocateDBCS(pCP, Low, High, Ctr))
        {
            return (1);
        }

        //
        //  Set the range in the structure.
        //
        (pCP->pDBCS)[Ctr]->LowRange = (WORD)Low;
        (pCP->pDBCS)[Ctr]->HighRange = (WORD)High;

        if (Verbose)
            printf("  LOW = %x\tHIGH = %x\n", Low, High);

        //
        //  Get Tables for this range.
        //
        if (GetAllDBCSTables( High - Low + 1,
                              (pCP->pDBCS)[Ctr]->pDBCSTbls ))
        {
            return (1);
        }

        //
        //  Set the offsets for the range.
        //  Offsets are in WORDS.
        //
        for (Ctr2 = Low; Ctr2 <= High; Ctr2++)
        {
            pCP->pDBCSOff[Ctr2] = Offset;
            Offset += DBCS_TABLE_SIZE;

            //
            //  This shouldn't happen, but check it just in case.
            //       ( 254 tables max - (65536/256) - 2 )
            //
            if (Offset > 65536)
            {
                printf("FATAL Error: Too many DBCS tables - 254 max allowed.\n");
                return (1);
            }
        }

        //
        //  Save the LeadByte values in pCP structure again for easy
        //  writing to file.
        //
        (pCP->LeadBytes)[Ctr * 2]       = (BYTE)Low;
        (pCP->LeadBytes)[(Ctr * 2) + 1] = (BYTE)High;
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetAllDBCSTables
//
//  This routine gets the DBCS tables (for one range) from the input file
//  and places them in the appropriate structures.
//
//  07-30-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetAllDBCSTables(
    int NumTables,
    PDBCS_TBL_ARRAY pArray)
{
    int Ctr;                      // loop counter
    char pszKeyWord[MAX];         // input token
    int size;                     // size of table to follow


    //
    //  Read each table.
    //
    for (Ctr = 0; Ctr < NumTables; Ctr++)
    {
        //
        //  Get DBCSTABLE keyword.
        //
        if ((fscanf(pInputFile, "%s", pszKeyWord) != 1) ||
            (_strcmpi(pszKeyWord, "DBCSTABLE") != 0))
        {
            printf("Parse Error: Error reading DBCSTABLE keyword.\n");
            return (1);
        }

        if (Verbose)
            printf("\n  Found DBCSTABLE keyword.\n");

        //
        //  Get size parameter.
        //
        if (GetSize(&size))
            return (1);

        //
        //  Get DBCS Table.
        //
        if (GetDBCSTable(size, pArray[Ctr]))
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
//  GetDBCSTable
//
//  This routine gets the DBCS table from the input file.  It uses the
//  size parameter to know when to stop reading from the file.  If an error
//  is encountered, a message is printed and an error is returned.
//
//  07-30-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetDBCSTable(
    int Size,
    PDBCS_TBL pTable)
{
    int Ctr;                      // loop counter
    int Index;                    // index into array - single byte char
    int Value;                    // value - WC translation
    int NumItems;                 // number of items returned from fscanf


    //
    //  For each table entry, read the MB char and the wide char
    //  from the input file and store the wide char in the array.
    //
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        //
        //  Get the index and the value to store from the file.
        //
        NumItems = fscanf( pInputFile,
                           "%x %x ;%*[^\n]",
                           &Index,
                           &Value );
        if (NumItems != 2)
        {
            printf("Parse Error: Error reading DBCSTABLE values.\n");
            return (1);
        }

        if (Index > DBCS_TABLE_SIZE)
        {
            printf("Parse Error: DBCS character value too large.\n");
            printf("             Value must be less than 0x%x.\n", DBCS_TABLE_SIZE);
            return (1);
        }

        //
        //  Store the wide character value in the array.
        //
        pTable[Index] = (WORD)Value;

        if (Verbose)
            printf("  MB = %x\tWC = %x\n", Index, Value);
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetWCTable
//
//  This routine gets the wide character table from the input file.  It uses
//  the size parameter to know when to stop reading from the file.  If an
//  error is encountered, a message is printed and an error is returned.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

int GetWCTable(
    PCODEPAGE pCP,
    int Size,
    BOOL IsMBCodePage)
{
    int WChar;                    // wide character value
    int MBChar;                   // multibyte character value
    register int Ctr;             // loop counter
    BYTE *pBytePtr;               // ptr to byte table
    WORD *pWordPtr;               // ptr to word table
    int NumItems;                 // number of items returned from fscanf


    //
    //  Allocate buffer for 1-to-1 mapping table.
    //
    if (IsMBCodePage)
    {
        if (AllocateWCTable(pCP, sizeof(WORD)))
        {
            return (1);
        }
        pWordPtr = (WORD *)(pCP->pWC);
    }
    else
    {
        if (AllocateWCTable(pCP, sizeof(BYTE)))
        {
            return (1);
        }
        pBytePtr = (BYTE *)(pCP->pWC);
    }

    //
    //  For each entry in table, read in wide character and multibyte
    //  character from input file.
    //
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        //
        //  Read in wide character and multibyte character.
        //
        NumItems = fscanf( pInputFile,
                           "%x %x ;%*[^\n]",
                           &WChar,
                           &MBChar );
        if (NumItems != 2)
        {
            printf("Parse Error: Error reading WCTABLE values.\n");
            return (1);
        }

        if (Verbose)
            printf("  WC = %x\tMB = %x\n", WChar, MBChar);

        //
        //  Insert MBChar into the appropriate translation table buffer.
        //
        if (IsMBCodePage)
        {
            pWordPtr[WChar] = (WORD)MBChar;
        }
        else
        {
            pBytePtr[WChar] = (BYTE)MBChar;
        }
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteCPInfo
//
//  This routine writes the CP information to the output file.
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteCPInfo(
    PCODEPAGE pCP,
    FILE *pOutputFile)
{
    int Size = CP_INFO_SIZE;      // size of CPINFO information
    WORD wValue;                  // temp storage value


    //
    //  Get the translation of the MB default char and the
    //  Unicode default char.
    //
    if (GetTransDefaultChars(pCP))
    {
        return (1);
    }

    if (Verbose)
        printf("\nWriting CP Info...\n");

    //
    //  Write size of CPInfo to file.
    //
    wValue = (WORD)Size;
    if (FileWrite( pOutputFile,
                   &wValue,
                   sizeof(WORD),
                   1,
                   "CPINFO Table Size" ))
    {
        return (1);
    }

    //
    //  Write CodePageValue to file.
    //
    wValue = (WORD)(pCP->CodePageValue);
    if (FileWrite( pOutputFile,
                   &wValue,
                   sizeof(WORD),
                   1,
                   "CPINFO Code Page Value" ))
    {
        return (1);
    }

    //
    //  Write MaxCharSize to file.
    //
    wValue = (WORD)(pCP->MaxCharSize);
    if (FileWrite( pOutputFile,
                   &wValue,
                   sizeof(WORD),
                   1,
                   "CPINFO Max Char Size" ))
    {
        return (1);
    }

    //
    //  Write Default Char to file.
    //
    if (FileWrite( pOutputFile,
                   &(pCP->DefaultChar),
                   sizeof(WORD),
                   1,
                   "CPINFO Default Char" ))
    {
        return (1);
    }

    //
    //  Write Unicode Default Char to file.
    //
    if (FileWrite( pOutputFile,
                   &(pCP->UniDefaultChar),
                   sizeof(WORD),
                   1,
                   "CPINFO Unicode Default Char" ))
    {
        return (1);
    }

    //
    //  Write Translation of Default Char to file.
    //
    if (FileWrite( pOutputFile,
                   &(pCP->TransDefChar),
                   sizeof(WORD),
                   1,
                   "CPINFO Translation of Default Char" ))
    {
        return (1);
    }

    //
    //  Write Translation of Unicode Default Char to file.
    //
    if (FileWrite( pOutputFile,
                   &(pCP->TransUniDefChar),
                   sizeof(WORD),
                   1,
                   "CPINFO Translation of Unicode Default Char" ))
    {
        return (1);
    }

    //
    //  Write DBCS LeadByte Ranges to file.
    //
    if (FileWrite( pOutputFile,
                   &(pCP->LeadBytes),
                   sizeof(BYTE),
                   MAX_NUM_LEADBYTE,
                   "CPINFO LeadBytes" ))
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
//  WriteMB
//
//  This routine writes the MB table, GLYPH table (if it exists), and
//  DBCS table (if it exists) to the output file.
//
//  07-30-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteMB(
    PCODEPAGE pCP,
    FILE *pOutputFile)
{
    int TblSize;                  // size of table
    int Ctr;                      // loop counter
    int Ctr2;                     // loop counter
    PDBCS_ARRAY pDBCSArray;       // ptr to DBCS array
    PDBCS_RANGE pRange;           // ptr to range structure
    register int NumRanges;       // number of DBCS ranges
    register int NumTables;       // number of tables for range
    WORD wValue;                  // temp storage value


    if (Verbose)
        printf("\nWriting MB Table...\n");

    //
    //  Compute size of table and write it to the output file.
    //
    TblSize = ComputeMBSize(pCP);

    wValue = (WORD)TblSize;
    if (FileWrite( pOutputFile,
                   &wValue,
                   sizeof(WORD),
                   1,
                   "MB size" ))
    {
        return (1);
    }

    //
    //  Write MB Table to output file.
    //
    if (FileWrite( pOutputFile,
                   pCP->pMB,
                   sizeof(WORD),
                   MB_TABLE_SIZE,
                   "MB table" ))
    {
        return (1);
    }

    //
    //  Write Glyph Table to output file (if it exists).
    //
    if (pCP->WriteFlags & F_GLYPH)
    {
        wValue = GLYPH_TABLE_SIZE;
        if (FileWrite( pOutputFile,
                       &wValue,
                       sizeof(WORD),
                       1,
                       "GLYPH table size" ))
        {
            return (1);
        }

        if (FileWrite( pOutputFile,
                       pCP->pGlyph,
                       sizeof(WORD),
                       GLYPH_TABLE_SIZE,
                       "GLYPH table" ))
        {
            return (1);
        }
    }
    else
    {
        wValue = 0;
        if (FileWrite( pOutputFile,
                       &wValue,
                       sizeof(WORD),
                       1,
                       "GLYPH table size" ))
        {
            return (1);
        }
    }

    //
    //  Write number of DBCS ranges to output file.
    //
    wValue = (WORD)(pCP->NumDBCSRanges);
    if (FileWrite( pOutputFile,
                   &wValue,
                   sizeof(WORD),
                   1,
                   "DBCS Ranges" ))
    {
        return (1);
    }

    //
    //  Write the DBCS tables to the file (if any exist).
    //
    NumRanges = pCP->NumDBCSRanges;
    if ((NumRanges > 0) && (pCP->WriteFlags & F_DBCS))
    {
        if (Verbose)
            printf("\n  Writing DBCS Table...\n");

        //
        //  Write the offsets.
        //
        if (FileWrite( pOutputFile,
                       pCP->pDBCSOff,
                       sizeof(WORD),
                       DBCS_OFFSET_SIZE,
                       "DBCS Offsets" ))
        {
            return (1);
        }

        //
        //  Write the tables.
        //
        pDBCSArray = pCP->pDBCS;
        for (Ctr = 0; Ctr < NumRanges; Ctr++)
        {
            pRange = pDBCSArray[Ctr];

            if (Verbose)
                printf("    Writing DBCS range %x to %x\n",
                            pRange->LowRange, pRange->HighRange);

            NumTables = pRange->HighRange - pRange->LowRange + 1;
            for (Ctr2 = 0; Ctr2 < NumTables; Ctr2++)
            {
                if (FileWrite( pOutputFile,
                               pRange->pDBCSTbls[Ctr2],
                               sizeof(WORD),
                               DBCS_TABLE_SIZE,
                               "DBCS Table" ))
                {
                    return (1);
                }

                if (Verbose)
                    printf("      Writing DBCS table %d\n", Ctr2 + 1);
            }
        }
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteWC
//
//  This routine writes the WC information to the output file.
//
//  07-30-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteWC(
    PCODEPAGE pCP,
    BOOL IsMBCodePage,
    FILE *pOutputFile)
{
    WORD wValue;                  // temp storage value


    if (Verbose)
        printf("\nWriting WC Table...\n");

    //
    //  Write 0 for SB or 1 for DB code page to the output file.
    //
    wValue = (WORD)IsMBCodePage;
    if (FileWrite( pOutputFile,
                   &wValue,
                   sizeof(WORD),
                   1,
                   "SB or DB flag" ))
    {
        return (1);
    }

    //
    //  Write WC translation table to the output file.
    //
    if (FileWrite( pOutputFile,
                   pCP->pWC,
                   (IsMBCodePage) ? sizeof(WORD) : sizeof(BYTE),
                   WC_TABLE_SIZE,
                   "WC Table" ))
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
//  FreeMB
//
//  This routine frees the memory used to build the MB table and the DBCS
//  table.
//
//  07-30-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

void FreeMB(
    PCODEPAGE pCP)
{
    int Ctr;                      // loop counter
    int Ctr2;                     // loop counter
    PDBCS_ARRAY pDBCSArray;       // ptr to DBCS array
    PDBCS_RANGE pRange;           // ptr to DBCS Range structure
    PDBCS_TBL_ARRAY pTbls;        // ptr to DBCS table array
    register int NumRanges;       // number of DBCS ranges
    register int NumTables;       // number of tables in range


    //
    //  Free Multibyte Table structures.
    //
    if (pCP->pMB != NULL)
    {
        free(pCP->pMB);
    }

    //
    //  Free Glyph Table structures.
    //
    if (pCP->pGlyph != NULL)
    {
        free(pCP->pGlyph);
    }

    //
    //  Free DBCS Table structures.
    //
    if ((pDBCSArray = pCP->pDBCS) != NULL)
    {
        NumRanges = pCP->NumDBCSRanges;
        for (Ctr = 0; Ctr < NumRanges; Ctr++)
        {
            if ((pRange = pDBCSArray[Ctr]) != NULL)
            {
                if ((pTbls = pRange->pDBCSTbls) != NULL)
                {
                    NumTables = pRange->HighRange - pRange->LowRange + 1;
                    for (Ctr2 = 0; Ctr2 < NumTables; Ctr2++)
                    {
                        if (pTbls[Ctr2] != NULL)
                        {
                            free(pTbls[Ctr2]);
                        }
                    }
                    free(pTbls);
                }
                free(pRange);
            }
        }
        free(pDBCSArray);
    }

    if (pCP->pDBCSOff != NULL)
    {
        free(pCP->pDBCSOff);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  GetTransDefaultChars
//
//  Gets the MB translation for the Unicode default char and
//  the Unicode translation for the MB default char.
//
//  This allows the multi byte default char and the Unicode default char
//  to be different in the data file.
//
//  09-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetTransDefaultChars(
    PCODEPAGE pCP)
{
    WORD wDefChar;           // default char to translate
    WORD Lead;               // lead byte of DBCS char
    WORD Low;                // low part of DBCS range
    WORD High;               // high part of DBCS range
    int ctr;                 // loop counter
    PDBCS_TBL pDBCSTable;    // ptr to appropriate DBCS table


    //
    //  Get the MB translation for the Unicode Default Char.
    //
    wDefChar = pCP->UniDefaultChar;
    if (pCP->MaxCharSize == 1)
    {
        //
        //  Single byte code page.
        //
        pCP->TransUniDefChar = ((WORD)(((BYTE *)(pCP->pWC))[wDefChar]));
    }
    else
    {
        //
        //  Double byte code page.
        //
        pCP->TransUniDefChar =  ((WORD)(((WORD *)(pCP->pWC))[wDefChar]));
    }

    //
    //  Get the Unicode translation for the MB Default Char.
    //
    wDefChar = pCP->DefaultChar;
    if (Lead = (WORD)HIBYTE(wDefChar))
    {
        //
        //  Make sure the DBCS tables exist.
        //
        if (!(pCP->pDBCS))
        {
            printf("Parse Error: Invalid default char '%x'.\n", wDefChar);
            return (1);
        }

        //
        //  Search for the correct range.
        //
        for (ctr = 0; ctr < pCP->NumDBCSRanges; ctr++)
        {
            Low  = ((pCP->pDBCS)[ctr])->LowRange;
            High = ((pCP->pDBCS)[ctr])->HighRange;

            if ((Lead >= Low) && (Lead <= High))
            {
                break;
            }
        }

        //
        //  Make sure the lead byte is valid.
        //
        if (ctr == pCP->NumDBCSRanges)
        {
            printf("Parse Error: Invalid default char '%x'.\n", wDefChar);
            return (1);
        }

        //
        //  Get the Unicode translation of the DBCS char.
        //
        pDBCSTable = (((pCP->pDBCS)[ctr])->pDBCSTbls)[Lead - Low];
        pCP->TransDefChar = ((WORD)(pDBCSTable[LOBYTE(wDefChar)]));

        //
        //  Make sure the trail byte is valid.
        //
        if ((pCP->TransDefChar == pCP->UniDefaultChar) &&
            (wDefChar != pCP->TransUniDefChar))
        {
            printf("Parse Error: Invalid default char '%x'.\n", wDefChar);
            return (1);
        }
    }
    else
    {
        pCP->TransDefChar = ((WORD)((pCP->pMB)[LOBYTE(wDefChar)]));
    }

    //
    //  Return success.
    //
    return (0);
}
