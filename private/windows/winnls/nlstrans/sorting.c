/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    sorting.c

Abstract:

    This file contains functions necessary to parse and write the sorting
    specific tables to a data file.

    External Routines in this file:
      ParseSortkey
      ParseSortTables
      ParseIdeographExceptions
      WriteSortkey
      WriteSortTables
      WriteIdeographExceptions

Revision History:

    11-04-92    JulieB    Created.

--*/



//
//  Include Files.
//

#include "nlstrans.h"




//
//  Forward Declarations.
//

int
GetDefaultSortkeyTable(
    PSORTKEY pSortkey,
    int Size);

int
GetReverseDWTable(
    PSORT_TABLES pSortTbls,
    int Size);

int
GetDoubleCompressionTable(
    PSORT_TABLES pSortTbls,
    int Size);

int
GetIdeographLcidExceptionTable(
    PSORT_TABLES pSortTbls,
    int Size);

int
GetExpansionTable(
    PSORT_TABLES pSortTbls,
    int Size);

int
GetCompressionTable(
    PSORT_TABLES pSortTbls,
    int Size);

int
GetExceptionTable(
    PSORT_TABLES pSortTbls,
    int Size);

int
GetMultipleWeightsTable(
    PSORT_TABLES pSortTbls,
    int Size);

int
GetIdeographExceptions(
    PIDEOGRAPH_EXCEPT pIdeograph,
    int Size);

int
WriteDefaultSortkey(
    PSORTKEY pSortkey,
    FILE *pOutputFile);

int
WriteReverseDW(
    PSORT_TABLES pSortTbls,
    FILE *pOutputFile);

int
WriteDoubleCompression(
    PSORT_TABLES pSortTbls,
    FILE *pOutputFile);

int
WriteIdeographLcidException(
    PSORT_TABLES pSortTbls,
    FILE *pOutputFile);

int
WriteExpansion(
    PSORT_TABLES pSortTbls,
    FILE *pOutputFile);

int
WriteCompressionTable(
    PSORT_TABLES pSortTbls,
    FILE *pOutputFile);

int
WriteExceptionTable(
    PSORT_TABLES pSortTbls,
    FILE *pOutputFile);

int
WriteMultipleWeights(
    PSORT_TABLES pSortTbls,
    FILE *pOutputFile);

int
WriteIdeographExceptionTable(
    PIDEOGRAPH_EXCEPT pIdeograph,
    FILE *pOutputFile);





//-------------------------------------------------------------------------//
//                            EXTERNAL ROUTINES                            //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  ParseSortkey
//
//  This routine parses the input file for the sortkey specific tables.
//  This routine is only entered when the SORTKEY keyword is found.
//  The parsing continues until the ENDSORTKEY keyword is found.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ParseSortkey(
    PSORTKEY pSortkey,
    PSZ pszKeyWord)
{
    int size;                          // size of table to follow


    while (fscanf(pInputFile, "%s", pszKeyWord) == 1)
    {
        if (_strcmpi(pszKeyWord, "DEFAULT") == 0)
        {
            if (Verbose)
                printf("\n\nFound DEFAULT keyword.\n");

            //
            //  Get size parameter.
            //
            if (GetSize(&size))
                return (1);

            //
            //  Get DEFAULT SORTKEY Table.
            //
            if (GetDefaultSortkeyTable(pSortkey, size))
            {
                return (1);
            }

            //
            //  Set WriteFlags for DEFAULT SORTKEY Table.
            //
            pSortkey->WriteFlags |= F_DEFAULT_SORTKEY;
        }

        else if (_strcmpi(pszKeyWord, "ENDSORTKEY") == 0)
        {
            if (Verbose)
                printf("\n\nFound ENDSORTKEY keyword.\n");

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
    //  If this point is reached, then the ENDSORTKEY keyword was
    //  not found.  Return an error.
    //
    printf("Parse Error: Expecting ENDSORTKEY keyword.\n");
    return (1);
}


////////////////////////////////////////////////////////////////////////////
//
//  ParseSortTables
//
//  This routine parses the input file for the "sort tables" specific tables.
//  This routine is only entered when the SORTTABLES keyword is found.
//  The parsing continues until the ENDSORTTABLES keyword is found.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ParseSortTables(
    PSORT_TABLES pSortTbls,
    PSZ pszKeyWord)
{
    int size;                          // size of table to follow


    while (fscanf(pInputFile, "%s", pszKeyWord) == 1)
    {
        if (_strcmpi(pszKeyWord, "REVERSEDIACRITICS") == 0)
        {
            if (Verbose)
                printf("\n\nFound REVERSEDIACRITICS keyword.\n");

            //
            //  Get size parameter.
            //
            if (GetSize(&size))
                return (1);

            //
            //  Get REVERSE DIACRITICS Table.
            //
            if (GetReverseDWTable(pSortTbls, size))
            {
                return (1);
            }

            //
            //  Set WriteFlags for REVERSE DIACRITICS Table.
            //
            pSortTbls->WriteFlags |= F_REVERSE_DW;
        }

        else if (_strcmpi(pszKeyWord, "DOUBLECOMPRESSION") == 0)
        {
            if (Verbose)
                printf("\n\nFound DOUBLECOMPRESSION keyword.\n");

            //
            //  Get size parameter.
            //
            if (GetSize(&size))
                return (1);

            //
            //  Get DOUBLE COMPRESSION Table.
            //
            if (GetDoubleCompressionTable(pSortTbls, size))
            {
                return (1);
            }

            //
            //  Set WriteFlags for DOUBLE COMPRESSION Table.
            //
            pSortTbls->WriteFlags |= F_DOUBLE_COMPRESS;
        }

        else if (_strcmpi(pszKeyWord, "IDEOGRAPH_LCID_EXCEPTION") == 0)
        {
            if (Verbose)
                printf("\n\nFound IDEOGRAPH_LCID_EXCEPTION keyword.\n");

            //
            //  Get size parameter.
            //
            if (GetSize(&size))
                return (1);

            //
            //  Get EXCEPTION Table.
            //
            if (GetIdeographLcidExceptionTable(pSortTbls, size))
            {
                return (1);
            }

            //
            //  Set WriteFlags for EXCEPTION Table.
            //
            pSortTbls->WriteFlags |= F_IDEOGRAPH_LCID;
        }

        else if (_strcmpi(pszKeyWord, "EXPANSION") == 0)
        {
            if (Verbose)
                printf("\n\nFound EXPANSION keyword.\n");

            //
            //  Get size parameter.
            //
            if (GetSize(&size))
                return (1);

            //
            //  Get EXPANSION Table.
            //
            if (GetExpansionTable(pSortTbls, size))
            {
                return (1);
            }

            //
            //  Set WriteFlags for EXPANSION Table.
            //
            pSortTbls->WriteFlags |= F_EXPANSION;
        }

        else if (_strcmpi(pszKeyWord, "COMPRESSION") == 0)
        {
            if (Verbose)
                printf("\n\nFound COMPRESSION keyword.\n");

            //
            //  Get size parameter.
            //
            if (GetSize(&size))
                return (1);

            //
            //  Get COMPRESSION Table.
            //
            if (GetCompressionTable(pSortTbls, size))
            {
                return (1);
            }

            //
            //  Set WriteFlags for COMPRESSION Table.
            //
            pSortTbls->WriteFlags |= F_COMPRESSION;
        }

        else if (_strcmpi(pszKeyWord, "EXCEPTION") == 0)
        {
            if (Verbose)
                printf("\n\nFound EXCEPTION keyword.\n");

            //
            //  Get size parameter.
            //
            if (GetSize(&size))
                return (1);

            //
            //  Get EXCEPTION Table.
            //
            if (GetExceptionTable(pSortTbls, size))
            {
                return (1);
            }

            //
            //  Set WriteFlags for EXCEPTION Table.
            //
            pSortTbls->WriteFlags |= F_EXCEPTION;
        }

        else if (_strcmpi(pszKeyWord, "MULTIPLEWEIGHTS") == 0)
        {
            if (Verbose)
                printf("\n\nFound MULTIPLEWEIGHTS keyword.\n");

            //
            //  Get size parameter.
            //
            if (GetSize(&size))
                return (1);

            //
            //  Get MULTIPLE WEIGHTS Table.
            //
            if (GetMultipleWeightsTable(pSortTbls, size))
            {
                return (1);
            }

            //
            //  Set WriteFlags for MULTIPLE WEIGHTS Table.
            //
            pSortTbls->WriteFlags |= F_MULTIPLE_WEIGHTS;
        }

        else if (_strcmpi(pszKeyWord, "ENDSORTTABLES") == 0)
        {
            if (Verbose)
                printf("\n\nFound ENDSORTTABLES keyword.\n");

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
    //  If this point is reached, then the ENDSORTTABLES keyword was
    //  not found.  Return an error.
    //
    printf("Parse Error: Expecting ENDSORTTABLES keyword.\n");
    return (1);
}


////////////////////////////////////////////////////////////////////////////
//
//  ParseIdeographExceptions
//
//  This routine parses the input file for the ideograph exception table.
//  This routine is only entered when the IDEOGRAPH_EXCEPTION keyword is found.
//  The parsing continues until the correct number of entries is read
//  in from the file.
//
//  09-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ParseIdeographExceptions(
    PIDEOGRAPH_EXCEPT pIdeographExcept)
{
    int size;                          // size of table to follow


    //
    //  Get size parameter.
    //
    if (GetSize(&size))
        return (1);

    //
    //  Get IDEOGRAPH Exceptions.
    //
    if (GetIdeographExceptions(pIdeographExcept, size))
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
//  WriteSortkey
//
//  This routine writes the sortkey specific tables to an output file.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteSortkey(
    PSORTKEY pSortkey)
{
    FILE *pOutputFile;                 // ptr to output file


    //
    //  Make sure all tables are present.
    //
    if (!(pSortkey->WriteFlags & F_DEFAULT_SORTKEY))
    {
        printf("Write Error: All tables must be present -\n");
        printf("             Default Sortkey Table\n");
        return (1);
    }


    //
    //  Make sure output file can be opened for writing.
    //
    if ((pOutputFile = fopen(SORTKEY_FILE, "w+b")) == 0)
    {
        printf("Error opening output file %s.\n", SORTKEY_FILE);
        return (1);
    }

    if (Verbose)
        printf("\n\nWriting output file %s...\n", SORTKEY_FILE);

    //
    //  Write the DWORD semaphore - set to zero.
    //
    WriteWords(pOutputFile, 0, 2);

    //
    //  Write DEFAULT SORTKEY Table to output file.
    //
    if (WriteDefaultSortkey(pSortkey, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Free DEFAULT SORTKEY table structure.
    //
    free(pSortkey->pDefault);

    //
    //  Close the output file.
    //
    fclose(pOutputFile);

    //
    //  Return success.
    //
    printf("\nSuccessfully wrote output file %s\n", SORTKEY_FILE);
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteSortTables
//
//  This routine writes the sort tables to an output file.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteSortTables(
    PSORT_TABLES pSortTbls)
{
    FILE *pOutputFile;                 // ptr to output file
    int ctr;                           // loop counter


    //
    //  Make sure all tables are present.
    //
    if (!((pSortTbls->WriteFlags & F_REVERSE_DW) &&
          (pSortTbls->WriteFlags & F_DOUBLE_COMPRESS) &&
          (pSortTbls->WriteFlags & F_EXPANSION) &&
          (pSortTbls->WriteFlags & F_COMPRESSION) &&
          (pSortTbls->WriteFlags & F_EXCEPTION) &&
          (pSortTbls->WriteFlags & F_MULTIPLE_WEIGHTS)))
    {
        printf("Write Error: All tables must be present -\n");
        printf("             Reverse DW, Double Compression, Expansion,\n");
        printf("             Compression, Exception, and Multiple Weights.\n");
        return (1);
    }


    //
    //  Make sure output file can be opened for writing.
    //
    if ((pOutputFile = fopen(SORTTBLS_FILE, "w+b")) == 0)
    {
        printf("Error opening output file %s.\n", SORTTBLS_FILE);
        return (1);
    }

    if (Verbose)
        printf("\n\nWriting output file %s...\n", SORTTBLS_FILE);

    //
    //  Write REVERSE DIACRITIC Table to output file.
    //
    if (WriteReverseDW(pSortTbls, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Free REVERSE DIACRITIC table structure.
    //
    free(pSortTbls->pReverseDW);


    //
    //  Write DOUBLE COMPRESSION Table to output file.
    //
    if (WriteDoubleCompression(pSortTbls, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Free DOUBLE COMPRESSION table structure.
    //
    free(pSortTbls->pDblCompression);


    //
    //  Write IDEOGRAPH LCID EXCEPTION Table to output file.
    //
    if (WriteIdeographLcidException(pSortTbls, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Free IDEOGRAPH LCID EXCEPTION table structure.
    //
    free(pSortTbls->pIdeographLcid);


    //
    //  Write EXPANSION Table to output file.
    //
    if (WriteExpansion(pSortTbls, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Free EXPANSION table structure.
    //
    free(pSortTbls->pExpansion);


    //
    //  Write COMPRESSION Table to output file.
    //
    if (WriteCompressionTable(pSortTbls, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Free COMPRESSION table structure.
    //
    for (ctr = 0; ctr < pSortTbls->NumCompression; ctr++)
    {
        if ((pSortTbls->pCompress2Tbl)[ctr])
        {
            free((pSortTbls->pCompress2Tbl)[ctr]);
        }
        if ((pSortTbls->pCompress3Tbl)[ctr])
        {
            free((pSortTbls->pCompress3Tbl)[ctr]);
        }
    }
    free(pSortTbls->pCompress2Tbl);
    free(pSortTbls->pCompress3Tbl);
    free(pSortTbls->pCompressHdr);


    //
    //  Write EXCEPTION Table to output file.
    //
    if (WriteExceptionTable(pSortTbls, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Free EXCEPTION header and table structures.
    //
    for (ctr = 0; ctr < pSortTbls->NumException; ctr++)
    {
        if ((pSortTbls->pExceptTbl)[ctr])
        {
            free((pSortTbls->pExceptTbl)[ctr]);
        }
    }
    free(pSortTbls->pExceptTbl);
    free(pSortTbls->pExceptHdr);


    //
    //  Write MULTIPLE WEIGHTS Table to output file.
    //
    if (WriteMultipleWeights(pSortTbls, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Free MULTIPLE WEIGHTS table structure.
    //
    free(pSortTbls->pMultiWeight);


    //
    //  Close the output file.
    //
    fclose(pOutputFile);

    //
    //  Return success.
    //
    printf("\nSuccessfully wrote output file %s\n", SORTTBLS_FILE);
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteIdeographExceptions
//
//  This routine writes the ideograph exception table to the specified
//  output file.
//
//  09-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteIdeographExceptions(
    PIDEOGRAPH_EXCEPT pIdeographExcept)
{
    FILE *pOutputFile;                 // ptr to output file


    //
    //  Make sure output file can be opened for writing.
    //
    if ((pOutputFile = fopen(pIdeographExcept->pFileName, "w+b")) == 0)
    {
        printf("Error opening output file %s.\n", pIdeographExcept->pFileName);
        return (1);
    }

    if (Verbose)
        printf("\n\nWriting output file %s...\n", pIdeographExcept->pFileName);

    //
    //  Write Ideograph Exception Table to output file.
    //
    if (WriteIdeographExceptionTable(pIdeographExcept, pOutputFile))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Free Ideograph Exception table structure.
    //
    if (pIdeographExcept->pExcept)
    {
        free(pIdeographExcept->pExcept);
    }
    else if (pIdeographExcept->pExceptEx)
    {
        free(pIdeographExcept->pExceptEx);
    }

    //
    //  Close the output file.
    //
    fclose(pOutputFile);

    //
    //  Return success.
    //
    printf("\nSuccessfully wrote output file %s\n", pIdeographExcept->pFileName);
    return (0);
}




//-------------------------------------------------------------------------//
//                            INTERNAL ROUTINES                            //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  GetDefaultSortkeyTable
//
//  This routine gets the default sortkey table from the input file.  It
//  uses the size parameter to know when to stop reading from the file.  If
//  an error is encountered, a message is printed and an error is returned.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetDefaultSortkeyTable(
    PSORTKEY pSortkey,
    int Size)
{
    int UCP;                      // unicode code point
    int SM;                       // script member
    int AW;                       // alphanumeric weight
    int DW;                       // diacritic weight
    int CW;                       // case weight
    int Comp;                     // compression value

    register int Ctr;             // loop counter
    int NumItems;                 // number of items returned from fscanf


    //
    //  Allocate default sortkey table - 64K DWORDS.
    //
    if (AllocateSortDefault(pSortkey))
    {
        return (1);
    }

    //
    //  For each entry in table, read in the code point, the script member,
    //  the alphanumeric weight, the diacritic weight, the case weight, and
    //  the compression value from the input file.  Then store each of the
    //  weights in the default sortkey table.
    //
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        //
        //  Read in code point, script member, alphanumeric weight,
        //  diacritic weight, case weight, and compression value.
        //
        NumItems = fscanf( pInputFile,
                           "%i %i %i %i %i %i ;%*[^\n]",
                           &UCP,
                           &SM,
                           &AW,
                           &DW,
                           &CW,
                           &Comp );
        if (NumItems != 6)
        {
            printf("Parse Error: Error reading SORTKEY DEFAULT values.\n");
            return (1);
        }

        if (Verbose)
            printf("  UCP = %x\tSM = %d\tAW = %d\tDW = %d\tCW = %d\tComp = %d\n",
                    UCP, SM, AW, DW, CW, Comp);

        //
        //  Store the weights in the default sortkey table.
        //
        ((pSortkey->pDefault)[UCP]).Alpha     = (BYTE)AW;
        ((pSortkey->pDefault)[UCP]).Script    = (BYTE)SM;
        ((pSortkey->pDefault)[UCP]).Diacritic = (BYTE)DW;
        ((pSortkey->pDefault)[UCP]).Case      = (BYTE)MAKE_CASE_WT(CW, Comp);
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetReverseDWTable
//
//  This routine gets the reverse diacritic weight table from the input file.
//  It uses the size parameter to know when to stop reading from the file.
//  If an error is encountered, a message is printed and an error is returned.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetReverseDWTable(
    PSORT_TABLES pSortTbls,
    int Size)
{
    DWORD Locale;                 // locale id
    register int Ctr;             // loop counter
    int NumItems;                 // number of items returned from fscanf


    //
    //  Allocate reverse diacritic weight table and set size of table
    //  in sorttables structure.
    //
    if (AllocateReverseDW(pSortTbls, Size))
    {
        return (1);
    }

    //
    //  For each entry in table, read in the locale id from the input
    //  file and store it in the reverse DW table.
    //
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        //
        //  Read in the locale id.
        //
        NumItems = fscanf( pInputFile,
                           "%i ;%*[^\n]",
                           &Locale );
        if (NumItems != 1)
        {
            printf("Parse Error: Error reading REVERSE DIACRITIC values.\n");
            return (1);
        }

        if (Verbose)
            printf("  Locale ID = %lx\n", Locale);

        //
        //  Store the locale id in the reverse DW table.
        //
        (pSortTbls->pReverseDW)[Ctr] = Locale;
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetDoubleCompressionTable
//
//  This routine gets the double compression table from the input file.
//  It uses the size parameter to know when to stop reading from the file.
//  If an error is encountered, a message is printed and an error is returned.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetDoubleCompressionTable(
    PSORT_TABLES pSortTbls,
    int Size)
{
    DWORD Locale;                 // locale id
    register int Ctr;             // loop counter
    int NumItems;                 // number of items returned from fscanf


    //
    //  Allocate double compression table and set size of table
    //  in sorttables structure.
    //
    if (AllocateDoubleCompression(pSortTbls, Size))
    {
        return (1);
    }

    //
    //  For each entry in table, read in the locale id from the input
    //  file and store it in the double compression table.
    //
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        //
        //  Read in the locale id.
        //
        NumItems = fscanf( pInputFile,
                           "%i ;%*[^\n]",
                           &Locale );
        if (NumItems != 1)
        {
            printf("Parse Error: Error reading DOUBLE COMPRESSION values.\n");
            return (1);
        }

        if (Verbose)
            printf("  Locale ID = %lx\n", Locale);

        //
        //  Store the locale id in the double compression table.
        //
        (pSortTbls->pDblCompression)[Ctr] = Locale;
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetIdeographLcidExceptionTable
//
//  This routine gets the ideograph lcid exception table from the input file.
//  It uses the size parameter to know when to stop reading from the file.
//  If an error is encountered, a message is printed and an error is returned.
//
//  09-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetIdeographLcidExceptionTable(
    PSORT_TABLES pSortTbls,
    int Size)
{
    DWORD Locale;                 // locale id
    WORD *pFileName;              // ptr to file name string
    register int Ctr;             // loop counter
    int NumItems;                 // number of items returned from fscanf


    //
    //  Allocate ideograph lcid exception table and set size of table
    //  in sorttables structure.
    //
    if (AllocateIdeographLcid(pSortTbls, Size))
    {
        return (1);
    }

    //
    //  For each entry in table, read in the locale id and the file name
    //  from the input file and store it in the ideograph lcid table.
    //
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        //
        //  Read in the locale id and file name.
        //
        pFileName = ((pSortTbls->pIdeographLcid)[Ctr]).pFileName;
        NumItems = fscanf( pInputFile,
                           "%i %8ws ;%*[^\n]",
                           &Locale,
                           pFileName );
        if (NumItems != 2)
        {
            printf("Parse Error: Error reading IDEOGRAPH LCID values.\n");
            return (1);
        }

        //
        //  Add the .nls extension to the file name.
        //
        wcscat(pFileName, L".nls");

        if (Verbose)
            printf("  Locale ID = %lx\tFile Name = %s\n", Locale, pFileName);

        //
        //  Store the locale id in the ideograph lcid table.
        //
        ((pSortTbls->pIdeographLcid)[Ctr]).Locale = Locale;
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetExpansionTable
//
//  This routine gets the expansion table from the input file.
//  It uses the size parameter to know when to stop reading from the file.
//  If an error is encountered, a message is printed and an error is returned.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetExpansionTable(
    PSORT_TABLES pSortTbls,
    int Size)
{
    int ECP;                      // expansion code point
    int CP1;                      // code point 1
    int CP2;                      // code point 2
    register int Ctr;             // loop counter
    int NumItems;                 // number of items returned from fscanf


    //
    //  Allocate expansion table and set size of table in
    //  sorttables structure.
    //
    if (AllocateExpansion(pSortTbls, Size))
    {
        return (1);
    }

    //
    //  For each entry in table, read in the expansion code point, code
    //  point 1, and code point 2 from the input file.  Store the values
    //  in the expansion table.
    //
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        //
        //  Read in the expansion code point, code point 1, and
        //  code point 2.
        //
        NumItems = fscanf( pInputFile,
                           "%i %i %i ;%*[^\n]",
                           &ECP,
                           &CP1,
                           &CP2 );
        if (NumItems != 3)
        {
            printf("Parse Error: Error reading EXPANSION values.\n");
            return (1);
        }

        if (Verbose)
            printf("  ECP = %x\tCP1 = %x\tCP2 = %x\n", ECP, CP1, CP2);

        //
        //  Store code point 1 and code point2 in the Expansion table.
        //  The expansion code point is not stored.
        //
        ((pSortTbls->pExpansion)[Ctr]).CP1 = (WORD)CP1;
        ((pSortTbls->pExpansion)[Ctr]).CP2 = (WORD)CP2;
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetCompressionTable
//
//  This routine gets the compression table from the input file.
//  It uses the size parameter to know when to stop reading from the file.
//  If an error is encountered, a message is printed and an error is returned.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetCompressionTable(
    PSORT_TABLES pSortTbls,
    int Size)
{
    DWORD Locale;                 // locale id
    int Num;                      // number of entries for locale id
    int UCP1;                     // compression code point 1
    int UCP2;                     // compression code point 2
    int UCP3;                     // compression code point 3
    int SM;                       // script member
    int AW;                       // alphanumeric weight
    int DW;                       // diacritic weight
    int CW;                       // case weight
    int Offset = 0;               // offset to store
    register int Ctr;             // loop counter
    register int Ctr2;            // loop counter
    register int LcidCtr;         // loop counter
    int NumItems;                 // number of items returned from fscanf


    //
    //  Allocate compression header and compression table and set size of
    //  table in sort tables structure.
    //
    if (AllocateCompression(pSortTbls, Size))
    {
        return (1);
    }

    //
    //  For each entry in table, read in the LCID keyword and the locale
    //  id. Then, read in either the TWO or THREE keyword and the number
    //  of entries.  Then, for all entries, read in the compression code
    //  points (2 or 3) and all weights associated with the compression.
    //  Store all values in the compression header and the appropriate
    //  compression table.
    //
    Ctr = 0;
    while (Ctr < Size)
    {
        //
        //  Read in the locale id and the number of entries for the
        //  locale id.
        //
        NumItems = fscanf( pInputFile,
                           " LCID %i ;%*[^\n]",
                           &Locale );
        if (NumItems != 1)
        {
            printf("Parse Error: Error reading COMPRESSION LCID values.\n");
            return (1);
        }

        LcidCtr = 0;
        do
        {
            if (Verbose)
                printf("\n  LCID = %lx\n", Locale);

            //
            //  Store the locale id and the offset in the header.
            //
            ((pSortTbls->pCompressHdr)[Ctr + LcidCtr]).Locale = (DWORD)Locale;
            ((pSortTbls->pCompressHdr)[Ctr + LcidCtr]).Offset = (DWORD)Offset;

            LcidCtr++;

        } while (NumItems = fscanf( pInputFile,
                                    " LCID %i ;%*[^\n]",
                                    &Locale ));

        //
        //  Read in the TWO keyword and the number of TWO entries.
        //
        NumItems = fscanf( pInputFile,
                           " TWO %i ;%*[^\n]",
                           &Num );
        if (NumItems != 1)
        {
            printf("Parse Error: Error reading COMPRESSION TWO values.\n");
            return (1);
        }

        if (Verbose)
            printf("\n    TWO Num = %d\n\n", Num);

        //
        //  Store the number of 2 compressions in the header.
        //
        for (Ctr2 = 0; Ctr2 < LcidCtr; Ctr2++)
        {
            ((pSortTbls->pCompressHdr)[Ctr + Ctr2]).Num2 = (WORD)Num;
        }

        //
        //  Allocate compression 2 nodes for current LCID.
        //
        if (AllocateCompression2Nodes(pSortTbls, Num, Ctr))
        {
            return (1);
        }

        //
        //  For each entry, read in the 2 compression code points, the
        //  script member, the alphanumeric weight, the diacritic
        //  weight, the case weight, and the compression value.  Store
        //  the values in the compression 2 table nodes.
        //
        for (Ctr2 = 0; Ctr2 < Num; Ctr2++)
        {
            //
            //  Read in 2 code points, script member, alphanumeric weight,
            //  diacritic weight, and case weight.
            //
            NumItems = fscanf( pInputFile,
                               "%i %i %i %i %i %i ;%*[^\n]",
                               &UCP1,
                               &UCP2,
                               &SM,
                               &AW,
                               &DW,
                               &CW );
            if (NumItems != 6)
            {
                printf("Parse Error: Error reading COMPRESSION TWO values for LCID %lx.\n",
                        Locale);
                return (1);
            }

            if (Verbose)
                printf("      UCP1 = %x\tUCP2 = %x\tSM = %d\tAW = %d\tDW = %d\tCW = %d\n",
                        UCP1, UCP2, SM, AW, DW, CW);

            //
            //  Store the weights in the compression 2 table.
            //
            (((pSortTbls->pCompress2Tbl)[Ctr])[Ctr2]).UCP1      = (WORD)UCP1;
            (((pSortTbls->pCompress2Tbl)[Ctr])[Ctr2]).UCP2      = (WORD)UCP2;
            (((pSortTbls->pCompress2Tbl)[Ctr])[Ctr2]).Alpha     = (BYTE)AW;
            (((pSortTbls->pCompress2Tbl)[Ctr])[Ctr2]).Script    = (BYTE)SM;
            (((pSortTbls->pCompress2Tbl)[Ctr])[Ctr2]).Diacritic = (BYTE)DW;
            (((pSortTbls->pCompress2Tbl)[Ctr])[Ctr2]).Case      = (BYTE)MAKE_CASE_WT(CW, 0);
        }

        //
        //  Increment Offset amount by Num times the number of words
        //  in the compression 2 node.
        //
        Offset += (Num * NUM_COMPRESS_2_WORDS);

        //
        //  Read in the THREE keyword and the number of THREE entries.
        //
        NumItems = fscanf( pInputFile,
                           " THREE %i ;%*[^\n]",
                           &Num );
        if (NumItems != 1)
        {
            printf("Parse Error: Error reading COMPRESSION THREE values.\n");
            return (1);
        }

        if (Verbose)
            printf("\n    THREE Num = %d\n\n", Num);

        //
        //  Store the number of 3 compressions in the header.
        //
        for (Ctr2 = 0; Ctr2 < LcidCtr; Ctr2++)
        {
            ((pSortTbls->pCompressHdr)[Ctr + Ctr2]).Num3 = (WORD)Num;
        }

        //
        //  Allocate compression 3 nodes for current LCID.
        //
        if (AllocateCompression3Nodes(pSortTbls, Num, Ctr))
        {
            return (1);
        }

        //
        //  For each entry, read in the 3 compression code points, the
        //  script member, the alphanumeric weight, the diacritic
        //  weight, the case weight, and the compression value.  Store
        //  the values in the compression 3 table nodes.
        //
        for (Ctr2 = 0; Ctr2 < Num; Ctr2++)
        {
            //
            //  Read in 3 code points, script member, alphanumeric weight,
            //  diacritic weight, and case weight.
            //
            NumItems = fscanf( pInputFile,
                               "%i %i %i %i %i %i %i ;%*[^\n]",
                               &UCP1,
                               &UCP2,
                               &UCP3,
                               &SM,
                               &AW,
                               &DW,
                               &CW );
            if (NumItems != 7)
            {
                printf("Parse Error: Error reading COMPRESSION THREE values for LCID %lx.\n",
                        Locale);
                return (1);
            }

            if (Verbose)
                printf("      UCP1 = %x\tUCP2 = %x\tUCP3 = %x\tSM = %d\tAW = %d\tDW = %d\tCW = %d\n",
                        UCP1, UCP2, UCP3, SM, AW, DW, CW);

            //
            //  Store the weights in the compression 3 table.
            //
            (((pSortTbls->pCompress3Tbl)[Ctr])[Ctr2]).UCP1      = (WORD)UCP1;
            (((pSortTbls->pCompress3Tbl)[Ctr])[Ctr2]).UCP2      = (WORD)UCP2;
            (((pSortTbls->pCompress3Tbl)[Ctr])[Ctr2]).UCP3      = (WORD)UCP3;
            (((pSortTbls->pCompress3Tbl)[Ctr])[Ctr2]).Reserved  = (WORD)0;
            (((pSortTbls->pCompress3Tbl)[Ctr])[Ctr2]).Alpha     = (BYTE)AW;
            (((pSortTbls->pCompress3Tbl)[Ctr])[Ctr2]).Script    = (BYTE)SM;
            (((pSortTbls->pCompress3Tbl)[Ctr])[Ctr2]).Diacritic = (BYTE)DW;
            (((pSortTbls->pCompress3Tbl)[Ctr])[Ctr2]).Case      = (BYTE)MAKE_CASE_WT(CW, 0);
        }

        //
        //  Increment Offset amount by Num times the number of words
        //  in the compression 3 node.
        //
        Offset += (Num * NUM_COMPRESS_3_WORDS);

        Ctr += LcidCtr;
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetExceptionTable
//
//  This routine gets the exception table from the input file.
//  It uses the size parameter to know when to stop reading from the file.
//  If an error is encountered, a message is printed and an error is returned.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetExceptionTable(
    PSORT_TABLES pSortTbls,
    int Size)
{
    DWORD Locale;                 // locale id
    int Num;                      // number of entries for locale id
    int UCP;                      // exception code point
    int SM;                       // script member
    int AW;                       // alphanumeric weight
    int DW;                       // diacritic weight
    int CW;                       // case weight
    int Comp;                     // compression value
    int Offset = 0;               // offset to store
    int Ctr;                      // loop counter
    int Ctr2;                     // loop counter
    int LcidCtr;                  // loop counter
    int NumItems;                 // number of items returned from fscanf


    //
    //  Allocate exception header and exception table and set size of
    //  table in sort tables structure.
    //
    if (AllocateException(pSortTbls, Size))
    {
        return (1);
    }

    //
    //  For each entry in table, read in the LCID keyword, locale id,
    //  and the number of entries for that locale id.  Then, for all
    //  entries for the locale id, read in the exception code point and
    //  all weights associated with that code point.  Store all values
    //  in the exception header and the exception table.
    //
    Ctr = 0;
    while (Ctr < Size)
    {
        //
        //  Read in the LCID keyword, locale id, and the number of
        //  entries for the locale id.
        //
        NumItems = fscanf( pInputFile,
                           " LCID %i %i ;%*[^\n]",
                           &Locale,
                           &Num );
        if (NumItems != 2)
        {
            printf("Parse Error: Error reading EXCEPTION LCID values.\n");
            return (1);
        }

        if (Verbose)
            printf("\n  LCID = %lx\tNumEntries = %d\n\n", Locale, Num);

        //
        //  Store the locale id and the number of entries in the header.
        //
        ((pSortTbls->pExceptHdr)[Ctr]).Locale = (DWORD)Locale;
        ((pSortTbls->pExceptHdr)[Ctr]).Offset = (DWORD)Offset;
        ((pSortTbls->pExceptHdr)[Ctr]).NumEntries = (DWORD)Num;

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
                printf("\n  LCID = %lx\tNumEntries = %d\n\n", Locale, Num);

            //
            //  Store the locale id and the number of entries in the header.
            //
            ((pSortTbls->pExceptHdr)[Ctr + LcidCtr]).Locale = (DWORD)Locale;
            ((pSortTbls->pExceptHdr)[Ctr + LcidCtr]).Offset = (DWORD)Offset;
            ((pSortTbls->pExceptHdr)[Ctr + LcidCtr]).NumEntries = (DWORD)Num;

            LcidCtr++;
        }

        //
        //  Add (Num times number of words in exception node) to Offset
        //  to get the offset of the next LCID entries.
        //
        Offset += (Num * NUM_EXCEPT_WORDS);

        //
        //  Allocate exception nodes for current LCID.
        //
        if (AllocateExceptionNodes(pSortTbls, Num, Ctr))
        {
            return (1);
        }

        //
        //  For each entry for the locale id, read in the exception code
        //  point, the script member, the alphanumeric weight, the diacritic
        //  weight, the case weight, and the compression value.  Store the
        //  values in the exception table nodes.
        //
        for (Ctr2 = 0; Ctr2 < Num; Ctr2++)
        {
            //
            //  Read in code point, script member, alphanumeric weight,
            //  diacritic weight, case weight, and compression value.
            //
            NumItems = fscanf( pInputFile,
                               "%i %i %i %i %i %i ;%*[^\n]",
                               &UCP,
                               &SM,
                               &AW,
                               &DW,
                               &CW,
                               &Comp );
            if (NumItems != 6)
            {
                printf("Parse Error: Error reading EXCEPTION values for LCID %lx.\n",
                        Locale);
                return (1);
            }

            if (Verbose)
                printf("    UCP = %x\tSM = %d\tAW = %d\tDW = %d\tCW = %d\tComp = %d\n",
                        UCP, SM, AW, DW, CW, Comp);

            //
            //  Store the weights in the exception table.
            //
            (((pSortTbls->pExceptTbl)[Ctr])[Ctr2]).UCP       = (WORD)UCP;
            (((pSortTbls->pExceptTbl)[Ctr])[Ctr2]).Alpha     = (BYTE)AW;
            (((pSortTbls->pExceptTbl)[Ctr])[Ctr2]).Script    = (BYTE)SM;
            (((pSortTbls->pExceptTbl)[Ctr])[Ctr2]).Diacritic = (BYTE)DW;
            (((pSortTbls->pExceptTbl)[Ctr])[Ctr2]).Case      = (BYTE)MAKE_CASE_WT(CW, Comp);
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
//  GetMultipleWeightsTable
//
//  This routine gets the multiple weights table from the input file.
//  It uses the size parameter to know when to stop reading from the file.
//  If an error is encountered, a message is printed and an error is returned.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetMultipleWeightsTable(
    PSORT_TABLES pSortTbls,
    int Size)
{
    int FirstSM;                  // first SM in range
    int NumSM;                    // number of SMs in range
    register int Ctr;             // loop counter
    int NumItems;                 // number of items returned from fscanf


    //
    //  Allocate multiple weights table and set size of table in
    //  sorttables structure.
    //
    if (AllocateMultipleWeights(pSortTbls, Size))
    {
        return (1);
    }

    //
    //  For each entry in table, read in the first SM in range and the
    //  number of SMs in the range from the input file.  Store the values
    //  in the multiple weights table.
    //
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        //
        //  Read in the first SM in range and the number of SMs in the
        //  range.
        //
        NumItems = fscanf( pInputFile,
                           "%i %i ;%*[^\n]",
                           &FirstSM,
                           &NumSM );
        if (NumItems != 2)
        {
            printf("Parse Error: Error reading MULTIPLE WEIGHTS values.\n");
            return (1);
        }

        if (Verbose)
            printf("  FirstSM = %d\tNumSM = %d\n", FirstSM, NumSM);

        //
        //  Store the first SM and the number of SMs in the range
        //  in the Multiple Weights table.
        //
        ((pSortTbls->pMultiWeight)[Ctr]).FirstSM = (BYTE)FirstSM;
        ((pSortTbls->pMultiWeight)[Ctr]).NumSM = (BYTE)NumSM;
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetIdeographExceptions
//
//  This routine gets the ideograph exceptions from the input file.  It
//  uses the size parameter to know when to stop reading from the file.  If
//  an error is encountered, a message is printed and an error is returned.
//
//  09-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetIdeographExceptions(
    PIDEOGRAPH_EXCEPT pIdeograph,
    int Size)
{
    int NumColumns;               // number of columns in table
    int UCP;                      // unicode code point
    int SM;                       // script member
    int AW;                       // alphanumeric weight
    int DW;                       // diacritic weight
    int CW;                       // case weight
    register int Ctr;             // loop counter
    int NumItems;                 // number of items returned from fscanf


    //
    //  Get the file name and add the ".nls" extension.
    //  This file should be read in as ANSI, not Unicode.
    //
    //  Also, get the number of columns to be read in.
    //
    NumItems = fscanf( pInputFile,
                       "%i %8s ;%*[^\n]",
                       &NumColumns,
                       pIdeograph->pFileName );
    if (NumItems != 2)
    {
        printf("Parse Error: Error reading IDEOGRAPH FILE NAME string.\n");
        return (1);
    }
    strcat(pIdeograph->pFileName, ".nls");
    pIdeograph->NumColumns = NumColumns;

    if (Verbose)
        printf("  NumColumns = %d\tFileName = %s\n",
                NumColumns, pIdeograph->pFileName);

    //
    //  Allocate default ideograph exceptions table.
    //
    if (AllocateIdeographExceptions(pIdeograph, Size, NumColumns))
    {
        return (1);
    }

    //
    //  For each entry in table, read in the code point, the script member,
    //  and the alphanumeric weight from the input file.  Then store each
    //  of the weights in the ideograph exception table.
    //
    if (NumColumns == 2)
    {
        for (Ctr = 0; Ctr < Size; Ctr++)
        {
            //
            //  Read in code point, script member, and alphanumeric weight.
            //
            NumItems = fscanf( pInputFile,
                               "%i %i %i ;%*[^\n]",
                               &UCP,
                               &SM,
                               &AW );
            if (NumItems != 3)
            {
                printf("Parse Error: Error reading IDEOGRAPH EXCEPTION values.\n");
                return (1);
            }

            if (Verbose)
                printf("  UCP = %x\tSM = %d\tAW = %d\n", UCP, SM, AW);

            //
            //  Store the weights in the ideograph exception table.
            //
            ((pIdeograph->pExcept)[Ctr]).UCP     = (WORD)UCP;
            ((pIdeograph->pExcept)[Ctr]).Alpha   = (BYTE)AW;
            ((pIdeograph->pExcept)[Ctr]).Script  = (BYTE)SM;
        }
    }
    else if (NumColumns == 4)
    {
        for (Ctr = 0; Ctr < Size; Ctr++)
        {
            //
            //  Read in code point, script member, alphanumeric weight,
            //  diacritic weight, and case weight.
            //
            NumItems = fscanf( pInputFile,
                               "%i %i %i %i %i ;%*[^\n]",
                               &UCP,
                               &SM,
                               &AW,
                               &DW,
                               &CW );
            if (NumItems != 5)
            {
                printf("Parse Error: Error reading IDEOGRAPH EXCEPTION values.\n");
                return (1);
            }

            if (Verbose)
                printf("  UCP = %x\tSM = %d\tAW = %d\tDW = %d\tCW = %d\n",
                        UCP, SM, AW, DW, CW);

            //
            //  Store the weights in the ideograph exception table.
            //
            ((pIdeograph->pExceptEx)[Ctr]).UCP       = (WORD)UCP;
            ((pIdeograph->pExceptEx)[Ctr]).Alpha     = (BYTE)AW;
            ((pIdeograph->pExceptEx)[Ctr]).Script    = (BYTE)SM;
            ((pIdeograph->pExceptEx)[Ctr]).Diacritic = (BYTE)DW;
            ((pIdeograph->pExceptEx)[Ctr]).Case      = (BYTE)CW;
        }
    }
    else
    {
        printf("Parse Error: The Number of columns must be either 2 or 4.\n");
        return (1);
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteDefaultSortkey
//
//  This routine writes the DEFAULT SORTKEY information to the output file.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteDefaultSortkey(
    PSORTKEY pSortkey,
    FILE *pOutputFile)
{
    if (Verbose)
        printf("\nWriting DEFAULT SORTKEY Table...\n");

    //
    //  Write the default table to the output file.
    //
    if (FileWrite( pOutputFile,
                   pSortkey->pDefault,
                   sizeof(DWORD),
                   SKEY_TBL_SIZE,
                   "Default Sortkey Table" ))
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
//  WriteReverseDW
//
//  This routine writes the REVERSE DIACRITIC information to the output file.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteReverseDW(
    PSORT_TABLES pSortTbls,
    FILE *pOutputFile)
{
    int TblSize;                  // size of table
    DWORD dwValue;                // temp storage value


    if (Verbose)
        printf("\nWriting REVERSE DIACRITIC Table...\n");

    //
    //  Get the size of the table.
    //
    TblSize = pSortTbls->NumReverseDW;

    //
    //  Write the number of reverse diacritics to the output file.
    //
    dwValue = (DWORD)TblSize;
    if (FileWrite( pOutputFile,
                   &dwValue,
                   sizeof(DWORD),
                   1,
                   "Reverse DW Size" ))
    {
        return (1);
    }

    //
    //  Write the reverse diacritic table to the output file.
    //
    if (FileWrite( pOutputFile,
                   pSortTbls->pReverseDW,
                   sizeof(REV_DW),
                   TblSize,
                   "Reverse Diacritics Table" ))
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
//  WriteDoubleCompression
//
//  This routine writes the DOUBLE COMPRESSION information to the output file.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteDoubleCompression(
    PSORT_TABLES pSortTbls,
    FILE *pOutputFile)
{
    int TblSize;                  // size of table
    DWORD dwValue;                // temp storage value


    if (Verbose)
        printf("\nWriting DOUBLE COMPRESSION Table...\n");

    //
    //  Get the size of the table.
    //
    TblSize = pSortTbls->NumDblCompression;

    //
    //  Write the number of double compression to the output file.
    //
    dwValue = (DWORD)TblSize;
    if (FileWrite( pOutputFile,
                   &dwValue,
                   sizeof(DWORD),
                   1,
                   "Double Compression Size" ))
    {
        return (1);
    }

    //
    //  Write the double compression table to the output file.
    //
    if (FileWrite( pOutputFile,
                   pSortTbls->pDblCompression,
                   sizeof(DBL_COMPRESS),
                   TblSize,
                   "Double Compression Table" ))
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
//  WriteIdeographLcidException
//
//  This routine writes the IDEOGRAPH LCID EXCEPTION information to the
//  output file.
//
//  09-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteIdeographLcidException(
    PSORT_TABLES pSortTbls,
    FILE *pOutputFile)
{
    int TblSize;                  // size of table
    DWORD dwValue;                // temp storage value


    if (Verbose)
        printf("\nWriting IDEOGRAPH LCID EXCEPTION Table...\n");

    //
    //  Get the size of the table.
    //
    TblSize = pSortTbls->NumIdeographLcid;

    //
    //  Write the number of ideograph lcids to the output file.
    //
    dwValue = (DWORD)TblSize;
    if (FileWrite( pOutputFile,
                   &dwValue,
                   sizeof(DWORD),
                   1,
                   "Ideograph Lcid Size" ))
    {
        return (1);
    }

    //
    //  Write the ideograph lcid table to the output file.
    //
    if (FileWrite( pOutputFile,
                   pSortTbls->pIdeographLcid,
                   sizeof(IDEOGRAPH_LCID),
                   TblSize,
                   "Ideograph Lcid Exception Table" ))
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
//  WriteExpansion
//
//  This routine writes the EXPANSION information to the output file.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteExpansion(
    PSORT_TABLES pSortTbls,
    FILE *pOutputFile)
{
    int TblSize;                  // size of table
    DWORD dwValue;                // temp storage value


    if (Verbose)
        printf("\nWriting EXPANSION Table...\n");

    //
    //  Get the size of the table.
    //
    TblSize = pSortTbls->NumExpansion;

    //
    //  Write the number of expansion ranges to the output file.
    //
    dwValue = (DWORD)TblSize;
    if (FileWrite( pOutputFile,
                   &dwValue,
                   sizeof(DWORD),
                   1,
                   "Expansion Size" ))
    {
        return (1);
    }

    //
    //  Write the expansion table to the output file.
    //
    if (FileWrite( pOutputFile,
                   pSortTbls->pExpansion,
                   sizeof(EXPAND),
                   TblSize,
                   "Expansion Table" ))
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
//  WriteCompressionTable
//
//  This routine writes the COMPRESSION information to the output file.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteCompressionTable(
    PSORT_TABLES pSortTbls,
    FILE *pOutputFile)
{
    int TblSize;                  // size of table
    int Ctr;                      // loop counter
    DWORD dwValue;                // temp storage value


    if (Verbose)
        printf("\nWriting COMPRESSION Table...\n");

    //
    //  Get the size of the table.
    //
    TblSize = pSortTbls->NumCompression;

    //
    //  Write the number of compression locales to the output file.
    //
    dwValue = (DWORD)TblSize;
    if (FileWrite( pOutputFile,
                   &dwValue,
                   sizeof(DWORD),
                   1,
                   "Compression Size" ))
    {
        return (1);
    }

    //
    //  Write the compression header to the output file.
    //
    if (FileWrite( pOutputFile,
                   pSortTbls->pCompressHdr,
                   sizeof(COMPRESS_HDR),
                   TblSize,
                   "Compression Header" ))
    {
        return (1);
    }

    //
    //  Write the compression 2 and compression 3 tables to the output file.
    //
    for (Ctr = 0; Ctr < TblSize; Ctr++)
    {
        //
        //  Write the compression 2 table.
        //
        if ((pSortTbls->pCompress2Tbl)[Ctr])
        {
            if (FileWrite( pOutputFile,
                           (pSortTbls->pCompress2Tbl)[Ctr],
                           sizeof(COMPRESS_2_NODE),
                           ((pSortTbls->pCompressHdr)[Ctr]).Num2,
                           "Compression 2 Table" ))
            {
                return (1);
            }
        }

        //
        //  Write the compression 3 table.
        //
        if ((pSortTbls->pCompress3Tbl)[Ctr])
        {
            if (FileWrite( pOutputFile,
                           (pSortTbls->pCompress3Tbl)[Ctr],
                           sizeof(COMPRESS_3_NODE),
                           ((pSortTbls->pCompressHdr)[Ctr]).Num3,
                           "Compression 3 Table" ))
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


////////////////////////////////////////////////////////////////////////////
//
//  WriteExceptionTable
//
//  This routine writes the EXCEPTION information to the output file.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteExceptionTable(
    PSORT_TABLES pSortTbls,
    FILE *pOutputFile)
{
    int TblSize;                  // size of table
    int Ctr;                      // loop counter
    DWORD dwValue;                // temp storage value


    if (Verbose)
        printf("\nWriting EXCEPTION Table...\n");

    //
    //  Get the size of the table.
    //
    TblSize = pSortTbls->NumException;

    //
    //  Write the number of exception locales to the output file.
    //
    dwValue = (DWORD)TblSize;
    if (FileWrite( pOutputFile,
                   &dwValue,
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
                   pSortTbls->pExceptHdr,
                   sizeof(EXCEPT_HDR),
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
        if ((pSortTbls->pExceptTbl)[Ctr])
        {
            if (FileWrite( pOutputFile,
                           (pSortTbls->pExceptTbl)[Ctr],
                           sizeof(EXCEPT_NODE),
                           ((pSortTbls->pExceptHdr)[Ctr]).NumEntries,
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


////////////////////////////////////////////////////////////////////////////
//
//  WriteMultiWeights
//
//  This routine writes the MULTIPLE WEIGHTS information to the output file.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteMultipleWeights(
    PSORT_TABLES pSortTbls,
    FILE *pOutputFile)
{
    int TblSize;                  // size of table
    WORD wValue;                  // temp storage value


    if (Verbose)
        printf("\nWriting MULTIPLE WEIGHTS Table...\n");

    //
    //  Get the size of the table.
    //
    TblSize = pSortTbls->NumMultiWeight;

    //
    //  Write the number of multiple weights ranges to the output file.
    //
    wValue = (WORD)TblSize;
    if (FileWrite( pOutputFile,
                   &wValue,
                   sizeof(WORD),
                   1,
                   "Multi Weight Size" ))
    {
        return (1);
    }

    //
    //  Write the multiple weights table to the output file.
    //
    if (FileWrite( pOutputFile,
                   pSortTbls->pMultiWeight,
                   sizeof(BYTE),
                   TblSize * 2,
                   "Multiple Weights Table" ))
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
//  WriteIdeographExceptionTable
//
//  This routine writes the IDEOGRAPH EXCEPTION information to the
//  specified output file.
//
//  09-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteIdeographExceptionTable(
    PIDEOGRAPH_EXCEPT pIdeograph,
    FILE *pOutputFile)
{
    int TblSize;                  // size of table
    int NumColumns;               // number of columns in table
    DWORD dwValue;                // temp storage value


    if (Verbose)
        printf("\nWriting IDEOGRAPH EXCEPTION Table...\n");

    //
    //  Get the size of the table and the number of columns.
    //
    TblSize = pIdeograph->NumEntries;
    NumColumns = pIdeograph->NumColumns;

    //
    //  Write the number of ideograph exceptions to the output file.
    //
    dwValue = (DWORD)TblSize;
    if (FileWrite( pOutputFile,
                   &dwValue,
                   sizeof(DWORD),
                   1,
                   "Ideograph Exception Size" ))
    {
        return (1);
    }

    //
    //  Write the number of columns of ideograph exceptions to the output
    //  file.
    //
    dwValue = (DWORD)NumColumns;
    if (FileWrite( pOutputFile,
                   &dwValue,
                   sizeof(DWORD),
                   1,
                   "Ideograph Exception Columns" ))
    {
        return (1);
    }

    //
    //  Write the ideograph exceptions table to the output file.
    //
    if (pIdeograph->pExcept)
    {
        if (FileWrite( pOutputFile,
                       pIdeograph->pExcept,
                       sizeof(IDEOGRAPH_NODE),
                       TblSize,
                       "Ideograph Exception Table" ))
        {
            return (1);
        }
    }
    else if (pIdeograph->pExceptEx)
    {
        if (FileWrite( pOutputFile,
                       pIdeograph->pExceptEx,
                       sizeof(IDEOGRAPH_NODE_EX),
                       TblSize,
                       "Ideograph Exception Table" ))
        {
            return (1);
        }
    }
    else
    {
        return (1);
    }

    //
    //  Return success.
    //
    return (0);
}
