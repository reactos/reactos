/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    language.c

Abstract:

    This file contains functions necessary to parse and write the language
    specific tables to a data file.

    External Routines in this file:
      ParseLanguage
      WriteLanguage
      ParseLangException
      WriteLangException

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
GetUpperTable(
    PLANGUAGE pLang,
    int Size);

int
GetLowerTable(
    PLANGUAGE pLang,
    int Size);

int
GetLangExceptionTable(
    PLANG_EXCEPT pLangExcept,
    int Size);
int
WriteUpper(
    PLANGUAGE pLang,
    FILE *pOutputFile);

int
WriteLower(
    PLANGUAGE pLang,
    FILE *pOutputFile);

int
WriteLangExceptionTable(
    PLANG_EXCEPT pLangExcept,
    FILE *pOutputFile);





//-------------------------------------------------------------------------//
//                            EXTERNAL ROUTINES                            //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  ParseLanguage
//
//  This routine parses the input file for the language specific tables.
//  This routine is only entered when the LANGUAGE keyword is found.
//  The parsing continues until the ENDLANGUAGE keyword is found.
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ParseLanguage(
    PLANGUAGE pLang,
    PSZ pszKeyWord)
{
    int size;                          // size of table to follow


    while (fscanf(pInputFile, "%s", pszKeyWord) == 1)
    {
        if (_strcmpi(pszKeyWord, "UPPERCASE") == 0)
        {
            if (Verbose)
                printf("\n\nFound UPPERCASE keyword.\n");

            //
            //  Get size parameter.
            //
            if (GetSize(&size))
                return (1);

            //
            //  Get UPPERCASE Table.
            //
            if (GetUpperTable(pLang, size))
            {
                return (1);
            }

            //
            //  Set WriteFlags for UPPERCASE Table.
            //
            pLang->WriteFlags |= F_UPPER;
        }

        else if (_strcmpi(pszKeyWord, "LOWERCASE") == 0)
        {
            if (Verbose)
                printf("\n\nFound LOWERCASE keyword.\n");

            //
            //  Get size parameter.
            //
            if (GetSize(&size))
                return (1);

            //
            //  Get LOWERCASE Table.
            //
            if (GetLowerTable(pLang, size))
            {
                return (1);
            }

            //
            //  Set WriteFlags for LOWERCASE Table.
            //
            pLang->WriteFlags |= F_LOWER;
        }

        else if (_strcmpi(pszKeyWord, "ENDLANGUAGE") == 0)
        {
            if (Verbose)
                printf("\n\nFound ENDLANGUAGE keyword.\n");

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
    //  If this point is reached, then the ENDLANGUAGE keyword was
    //  not found.  Return an error.
    //
    printf("Parse Error: Expecting ENDLANGUAGE keyword.\n");
    return (1);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteLanguage
//
//  This routine writes the language specific tables to an output file.
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteLanguage(
    PLANGUAGE pLang)
{
    FILE *pOutputFile;            // ptr to output file
    WORD wValue;                  // temp storage value


    //
    //  Make sure all tables are present.
    //
    if (!((pLang->WriteFlags & F_UPPER) && (pLang->WriteFlags & F_LOWER)))
    {
        printf("Write Error: All tables must be present -\n");
        printf("             Uppercase and Lowercase Tables.\n");
        return (1);
    }


    //
    //  Make sure output file can be opened for writing.
    //
    if ((pOutputFile = fopen(LANGUAGE_FILE, "w+b")) == 0)
    {
        printf("Error opening output file %s.\n", LANGUAGE_FILE);
        return (1);
    }

    if (Verbose)
        printf("\n\nWriting output file %s...\n", LANGUAGE_FILE);

    //
    //  Write IfDefault value to file.
    //
    pLang->IfDefault = 1;
    wValue = (WORD)(pLang->IfDefault);
    if (FileWrite( pOutputFile,
                   &wValue,
                   sizeof(WORD),
                   1,
                   "IfDefault" ))
    {
        return (1);
    }

    //
    //  Write UPPERCASE Table to output file.
    //
    if (WriteUpper(pLang, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Free UPPERCASE table structures.
    //
    Free844(pLang->pUpper);


    //
    //  Write LOWERCASE Table to output file.
    //
    if (WriteLower(pLang, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Free LOWERCASE table structures.
    //
    Free844(pLang->pLower);


    //
    //  Close the output file.
    //
    fclose(pOutputFile);

    //
    //  Return success.
    //
    printf("\nSuccessfully wrote output file %s\n", LANGUAGE_FILE);
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  ParseLangException
//
//  This routine parses the input file for the language exception specific
//  tables.  This routine is only entered when the LANGUAGE_EXCEPTION keyword
//  is found.
//
//  08-30-95    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ParseLangException(
    PLANG_EXCEPT pLangExcept,
    PSZ pszKeyWord)
{
    int size;                          // size of table to follow


    //
    //  Get size parameter.
    //
    if (GetSize(&size))
        return (1);

    //
    //  Get EXCEPTION Table.
    //
    if (GetLangExceptionTable(pLangExcept, size))
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
//  WriteLangException
//
//  This routine writes the language excpetion specific tables to an output
//  file.
//
//  08-30-95    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteLangException(
    PLANG_EXCEPT pLangExcept)
{
    FILE *pOutputFile;                 // ptr to output file
    int ctr;                           // loop counter


    //
    //  Make sure output file can be opened for writing.
    //
    if ((pOutputFile = fopen(LANG_EXCEPT_FILE, "w+b")) == 0)
    {
        printf("Error opening output file %s.\n", LANG_EXCEPT_FILE);
        return (1);
    }

    if (Verbose)
        printf("\n\nWriting output file %s...\n", LANG_EXCEPT_FILE);


    //
    //  Write EXCEPTION Table to output file.
    //
    if (WriteLangExceptionTable(pLangExcept, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Free EXCEPTION header and table structures.
    //
    for (ctr = 0; ctr < pLangExcept->NumException; ctr++)
    {
        if ((pLangExcept->pExceptTbl)[ctr])
        {
            free((pLangExcept->pExceptTbl)[ctr]);
        }
    }
    free(pLangExcept->pExceptTbl);
    free(pLangExcept->pExceptHdr);


    //
    //  Close the output file.
    //
    fclose(pOutputFile);

    //
    //  Return success.
    //
    printf("\nSuccessfully wrote output file %s\n", LANG_EXCEPT_FILE);
    return (0);
}




//-------------------------------------------------------------------------//
//                            INTERNAL ROUTINES                            //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  GetUpperTable
//
//  This routine gets the upper case table from the input file.  It uses
//  the size parameter to know when to stop reading from the file.  If an
//  error is encountered, a message is printed and an error is returned.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

int GetUpperTable(
    PLANGUAGE pLang,
    int Size)
{
    int LoChar;                   // lower case value
    int UpChar;                   // upper case value
    register int Ctr;             // loop counter
    int NumItems;                 // number of items returned from fscanf


    //
    //  Allocate top buffer for 8:4:4 table - 256 pointers.
    //
    if (Allocate8(&pLang->pUpper))
    {
        return (1);
    }

    //
    //  For each entry in table, read in the upper case and lower case
    //  character from input file, allocate necessary 16 word buffers
    //  based on upper case value, and store difference to lower case
    //  character.
    //
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        //
        //  Read in lower case and upper case characters.
        //
        NumItems = fscanf( pInputFile,
                           "%x %x ;%*[^\n]",
                           &LoChar,
                           &UpChar );
        if (NumItems != 2)
        {
            printf("Parse Error: Error reading UPPERCASE values.\n");
            return (1);
        }

        if (Verbose)
            printf("  Lower = %x\tUpper = %x\n", LoChar, UpChar);

        //
        //  Insert difference (UpChar - LoChar) into 8:4:4 table.
        //
        if (Insert844( pLang->pUpper,
                       (WORD)LoChar,
                       (WORD)(UpChar - LoChar),
                       &pLang->UPBuf2,
                       &pLang->UPBuf3,
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
//  GetLowerTable
//
//  This routine gets the lower case table from the input file.  It uses
//  the size parameter to know when to stop reading from the file.  If an
//  error is encountered, a message is printed and an error is returned.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

int GetLowerTable(
    PLANGUAGE pLang,
    int Size)
{
    int UpChar;                   // upper case value
    int LoChar;                   // lower case value
    register int Ctr;             // loop counter
    int NumItems;                 // number of items returned from fscanf


    //
    //  Allocate top buffer for 8:4:4 table - 256 pointers.
    //
    if (Allocate8(&pLang->pLower))
    {
        return (1);
    }

    //
    //  For each entry in table, read in the upper case and lower case
    //  character from input file, allocate necessary 16 word buffers
    //  based on lower case value, and store difference to upper case
    //  character.
    //
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        //
        //  Read in lower case and upper case characters.
        //
        NumItems = fscanf( pInputFile,
                           "%x %x ;%*[^\n]",
                           &UpChar,
                           &LoChar );
        if (NumItems != 2)
        {
            printf("Parse Error: Error reading LOWERCASE values.\n");
            return (1);
        }

        if (Verbose)
            printf("  Upper = %x\tLower = %x\n", UpChar, LoChar);

        //
        //  Insert difference (LoChar - UpChar) into 8:4:4 table.
        //
        if (Insert844( pLang->pLower,
                       (WORD)UpChar,
                       (WORD)(LoChar - UpChar),
                       &pLang->LOBuf2,
                       &pLang->LOBuf3,
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
//  GetLangExceptionTable
//
//  This routine gets the exception table from the input file.
//  It uses the size parameter to know when to stop reading from the file.
//  If an error is encountered, a message is printed and an error is returned.
//
//  08-30-95    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetLangExceptionTable(
    PLANG_EXCEPT pLangExcept,
    int Size)
{
    DWORD Locale;                 // locale id
    int NumUp;                    // number of entries for upper case
    int NumLo;                    // number of entries for lower case
    int TotalNum;                 // total number of exceptions for locale
    int UCP1;                     // exception code point 1
    int UCP2;                     // exception code point 2
    int Offset = 0;               // offset to store
    int Ctr;                      // loop counter
    int Ctr2;                     // loop counter
    int LcidCtr;                  // loop counter
    int NumItems;                 // number of items returned from fscanf
    int Num;                      // temp value
    char pszTemp[MAX];            // temp buffer for string


    //
    //  Allocate exception header and exception table and set size of
    //  table in language exception tables structure.
    //
    if (AllocateLangException(pLangExcept, Size))
    {
        return (1);
    }

    //
    //  For each entry in table, read in the LCID keyword, locale id,
    //  the number of upper case entries for that locale id, and the
    //  number of lower case entries for that locale id.  Then, for all
    //  entries for the locale id, read in the exception code point and
    //  the upper/lower case code point.  Store all values in the
    //  exception header and the exception table.
    //
    Ctr = 0;
    while (Ctr < Size)
    {
        //
        //  Read in the LCID keyword, locale id, and the number of
        //  entries for the locale id.
        //
        NumItems = fscanf( pInputFile,
                           " LCID %i %i %i ;%*[^\n]",
                           &Locale,
                           &NumUp,
                           &NumLo );
        if (NumItems != 3)
        {
            printf("Parse Error: Error reading EXCEPTION LCID values.\n");
            return (1);
        }

        if (Verbose)
            printf("\n  LCID = %lx\tNumUpper = %d\tNumLower = %d\n\n",
                   Locale, NumUp, NumLo);

        //
        //  Store the locale id and the number of entries in the header.
        //
        ((pLangExcept->pExceptHdr)[Ctr]).Locale = (DWORD)Locale;
        ((pLangExcept->pExceptHdr)[Ctr]).Offset = (DWORD)Offset;
        ((pLangExcept->pExceptHdr)[Ctr]).NumUpEntries = (DWORD)NumUp;
        ((pLangExcept->pExceptHdr)[Ctr]).NumLoEntries = (DWORD)NumLo;

        //
        //  See if there are any other LCIDs for this exception table.
        //
        LcidCtr = 1;
        while (NumItems = fscanf( pInputFile,
                                  " LCID %i ;%*[^\n]",
                                  &Locale ))
        {
            if (NumItems > 2)
            {
                printf("Parse Error: Error reading secondary EXCEPTION LCID values.\n");
                return (1);
            }

            if (Verbose)
                printf("\n  LCID = %lx\tNumUpper = %d\tNumLower = %d\n\n",
                       Locale, NumUp, NumLo);

            //
            //  Store the locale id and the number of entries in the header.
            //
            ((pLangExcept->pExceptHdr)[Ctr + LcidCtr]).Locale = (DWORD)Locale;
            ((pLangExcept->pExceptHdr)[Ctr + LcidCtr]).Offset = (DWORD)Offset;
            ((pLangExcept->pExceptHdr)[Ctr + LcidCtr]).NumUpEntries = (DWORD)NumUp;
            ((pLangExcept->pExceptHdr)[Ctr + LcidCtr]).NumLoEntries = (DWORD)NumLo;

            LcidCtr++;
        }

        //
        //  Add (Num times number of words in exception node) to Offset
        //  to get the offset of the next LCID entries.
        //
        TotalNum = NumUp + NumLo;
        Offset += (TotalNum * NUM_L_EXCEPT_WORDS);

        //
        //  Allocate exception nodes for current LCID.
        //
        if (AllocateLangExceptionNodes(pLangExcept, TotalNum, Ctr))
        {
            return (1);
        }

        //
        //  Read in the UPPERCASE keyword.
        //
        NumItems = fscanf(pInputFile, "%s", pszTemp);
        if ((NumItems != 1) ||
            (_strcmpi(pszTemp, "UPPERCASE") != 0))
        {
            printf("Parse Error: Error reading UPPERCASE keyword for LCID %lx.\n",
                    Locale);
            return (1);
        }
        else
        {
            if (Verbose)
                printf("\n\nFound UPPERCASE keyword.\n");
        }

        //
        //  For each entry for the locale id, read in the exception code
        //  point and the upper/lower case code point.  Store the values
        //  in the exception table nodes.
        //
        for (Ctr2 = 0; Ctr2 < TotalNum; Ctr2++)
        {
            if (Ctr2 == NumUp)
            {
                //
                //  Read in the LOWERCASE keyword.
                //
                NumItems = fscanf(pInputFile, "%s", pszTemp);
                if ((NumItems != 1) ||
                    (_strcmpi(pszTemp, "LOWERCASE") != 0))
                {
                    printf("Parse Error: Error reading LOWERCASE keyword for LCID %lx.\n",
                            Locale);
                    return (1);
                }
                else
                {
                    if (Verbose)
                        printf("\n\nFound LOWERCASE keyword.\n");
                }
            }

            //
            //  Read in code point and the upper/lower case code point.
            //
            NumItems = fscanf( pInputFile,
                               "%i %i ;%*[^\n]",
                               &UCP1,
                               &UCP2 );
            if (NumItems != 2)
            {
                printf("Parse Error: Error reading EXCEPTION values for LCID %lx.\n",
                        Locale);
                return (1);
            }

            if (Verbose)
                printf("    UCP1 = %x\tUCP2 = %x\n", UCP1, UCP2);

            //
            //  Store the weights in the exception table.
            //
            (((pLangExcept->pExceptTbl)[Ctr])[Ctr2]).UCP       = (WORD)UCP1;
            (((pLangExcept->pExceptTbl)[Ctr])[Ctr2]).AddAmount = (WORD)(UCP2 - UCP1);
        }

        Ctr += LcidCtr;
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteUpper
//
//  This routine writes the UPPERCASE information to the output file.
//
//  07-30-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteUpper(
    PLANGUAGE pLang,
    FILE *pOutputFile)
{
    int TblSize;                  // size of table
    WORD wValue;                  // temp storage value


    if (Verbose)
        printf("\nWriting UPPERCASE Table...\n");

    //
    //  Compute size of table.
    //
    TblSize = Compute844Size( pLang->UPBuf2,
                              pLang->UPBuf3,
                              sizeof(WORD) ) + 1;

    //
    //  Make sure the total size of the table is not greater than 64K.
    //  If it is, then the WORD offsets are too small.
    //
    if (TblSize > MAX_844_TBL_SIZE)
    {
       printf("Write Error: Size of UPPER table is greater than 64K.\n");
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
                   "UPPER size" ))
    {
        return (1);
    }

    //
    //  Write UPPERCASE 8:4:4 table to file.
    //
    if (Write844Table( pOutputFile,
                       pLang->pUpper,
                       pLang->UPBuf2,
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
//  WriteLower
//
//  This routine writes the LOWERCASE information to the output file.
//
//  07-30-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteLower(
    PLANGUAGE pLang,
    FILE *pOutputFile)
{
    int TblSize;                  // size of table
    WORD wValue;                  // temp storage value


    if (Verbose)
        printf("\nWriting LOWERCASE Table...\n");

    //
    //  Compute size of table.
    //
    TblSize = Compute844Size( pLang->LOBuf2,
                              pLang->LOBuf3,
                              sizeof(WORD) ) + 1;

    //
    //  Make sure the total size of the table is not greater than 64K.
    //  If it is, then the WORD offsets are too small.
    //
    if (TblSize > MAX_844_TBL_SIZE)
    {
       printf("Write Error: Size of LOWER table is greater than 64K.\n");
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
                   "LOWER size" ))
    {
        return (1);
    }

    //
    //  Write LOWERCASE 8:4:4 table to file.
    //
    if (Write844Table( pOutputFile,
                       pLang->pLower,
                       pLang->LOBuf2,
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
//  WriteLangExceptionTable
//
//  This routine writes the EXCEPTION information to the output file.
//
//  08-30-95    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteLangExceptionTable(
    PLANG_EXCEPT pLangExcept,
    FILE *pOutputFile)
{
    int TblSize;                  // size of table
    int Ctr;                      // loop counter
    WORD wValue;                  // temp storage value


    if (Verbose)
        printf("\nWriting EXCEPTION Table...\n");

    //
    //  Get the size of the table.
    //
    TblSize = pLangExcept->NumException;

    //
    //  Write the number of exception locales to the output file.
    //
    wValue = (WORD)TblSize;
    if (FileWrite( pOutputFile,
                   &wValue,
                   sizeof(DWORD),
                   1,
                   "Exception Size" ))
    {
        return (1);
    }

    //
    //  Write the exception header to the output file.
    //
    if (FileWrite( pOutputFile,
                   pLangExcept->pExceptHdr,
                   sizeof(L_EXCEPT_HDR),
                   TblSize,
                   "Exception Header" ))
    {
        return (1);
    }

    //
    //  Write the exception table to the output file.
    //
    for (Ctr = 0; Ctr < TblSize; Ctr++)
    {
        if ((pLangExcept->pExceptTbl)[Ctr])
        {
            if (FileWrite( pOutputFile,
                           (pLangExcept->pExceptTbl)[Ctr],
                           sizeof(L_EXCEPT_NODE),
                           ( ((pLangExcept->pExceptHdr)[Ctr]).NumUpEntries +
                             ((pLangExcept->pExceptHdr)[Ctr]).NumLoEntries ),
                           "Exception Table" ))
            {
                return (1);
            }
        }
    }

    //
    //  Return success.
    //
    return (0);
}
