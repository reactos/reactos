/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    ctype.c

Abstract:

    This file contains functions necessary to parse and write the ctype
    specific tables to a data file.

    External Routines in this file:
      ParseCTypes
      WriteCTypes

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
GetCTypeTables(
    PCTYPES pCType,
    int Size);

int
WriteCTypeFile(
    PCT_ARRAY pCT,
    PCT_MAP pMap,
    int CTBuf2,
    int CTBuf3,
    PSZ pszFileName);





//-------------------------------------------------------------------------//
//                            EXTERNAL ROUTINES                            //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  ParseCTypes
//
//  This routine parses the input file for the ctype tables.
//  This routine is only entered when the CTYPES keyword is found.
//  The parsing continues until the correct number of entries is read
//  in from the file.
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ParseCTypes(
    PCTYPES pCType)
{
    int size;                          // size of table to follow


    //
    //  Get size parameter.
    //
    if (GetSize(&size))
        return (1);

    //
    //  Get CTYPE Tables.
    //
    if (GetCTypeTables(pCType, size))
    {
        return (1);
    }

    //
    //  Remove any duplicate tables from the 8:4:4 table.
    //
    RemoveDuplicate844Levels( (P844_ARRAY)(pCType->pCType),
                              &(pCType->CTBuf2),
                              &(pCType->CTBuf3),
                              sizeof(BYTE) );

    //
    //  Set WriteFlags for all CTYPE Tables.
    //
    pCType->WriteFlags |= (F_CTYPE_1 | F_CTYPE_2 | F_CTYPE_3);

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  WriteCTypes
//
//  This routine writes the ctype tables to their appropriate output files.
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteCTypes(
    PCTYPES pCType)
{
    //
    //  Make sure all tables are present.
    //
    if (!((pCType->WriteFlags & F_CTYPE_1) &&
          (pCType->WriteFlags & F_CTYPE_2) &&
          (pCType->WriteFlags & F_CTYPE_3)))
    {
        printf("Write Error: All tables must be present -\n");
        printf("             CType 1, CType 2, and CType 3 Tables.\n");
        return (1);
    }

    //
    //  Write CTYPE Table to output file.
    //
    if (WriteCTypeFile( pCType->pCType,
                        pCType->pMap,
                        pCType->CTBuf2,
                        pCType->CTBuf3,
                        CTYPE_FILE ))
    {
        return (1);
    }

    //
    //  Free CType table structures.
    //
    Free844(pCType->pCType);
    FreeCTMap(pCType->pMap);

    //
    //  Return success.
    //
    return (0);
}




//-------------------------------------------------------------------------//
//                            INTERNAL ROUTINES                            //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  GetCTypeTables
//
//  This routine gets the character type tables from the input file.  It uses
//  the size parameter to know when to stop reading from the file.  If an
//  error is encountered, a message is printed and an error is returned.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

int GetCTypeTables(
    PCTYPES pCType,
    int Size)
{
    int WChar;                    // wide character value
    int CType1;                   // ctype1 value
    int CType2;                   // ctype2 value
    int CType3;                   // ctype3 value
    register int Ctr;             // loop counter
    int NumItems;                 // number of items returned from fscanf


    //
    //  Allocate top buffer for 8:4:4 tables - 256 pointers.
    //
    if (Allocate8(&pCType->pCType))
    {
        return (1);
    }

    //
    //  Allocate buffer for mapping table.
    //
    if (AllocateCTMap(&pCType->pMap))
    {
        return (1);
    }

    //
    //  For each entry in table, read in wide character, ctype1, ctype2,
    //  and ctype3 from input file, allocate necessary buffers based on
    //  wide character value, and store mapped ctype information in the
    //  appropriate table.
    //
    for (Ctr = 0; Ctr < Size; Ctr++)
    {
        //
        //  Read in wide character, ctype1, ctype2, and ctype3.
        //
        NumItems = fscanf( pInputFile,
                           "%x %x %x %x ;%*[^\n]",
                           &WChar,
                           &CType1,
                           &CType2,
                           &CType3 );
        if (NumItems != 4)
        {
            printf("Parse Error: Error reading CTYPE values.\n");
            return (1);
        }

        if (Verbose)
            printf("  WC = %x\tCT1 = %x\tCT2 = %x\tCT3 = %x\n",
                      WChar, CType1, CType2, CType3);

        //
        //  Insert CTYPE information into the 8:4:4 table.
        //
        if (Insert844Map( pCType->pCType,
                          pCType->pMap,
                          (WORD)WChar,
                          (WORD)CType1,
                          (WORD)CType2,
                          (WORD)CType3,
                          &pCType->CTBuf2,
                          &pCType->CTBuf3 ))
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
//  WriteCTypeFile
//
//  This routine writes the given CTYPE information to the output file.
//
//  12-10-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int WriteCTypeFile(
    PCT_ARRAY pCT,
    PCT_MAP pMap,
    int CTBuf2,
    int CTBuf3,
    PSZ pszFileName)
{
    DWORD TblSize844;             // size of 8:4:4 table
    DWORD TblSizeMap;             // size of mapping table
    DWORD TblSize;                // size of entire table
    FILE *pOutputFile;            // ptr to output file
    WORD wValue;                  // temp storage value


    //
    //  Make sure output file can be opened for writing.
    //
    if ((pOutputFile = fopen(pszFileName, "w+b")) == 0)
    {
        printf("Error opening output file %s.\n", pszFileName);
        return (1);
    }

    if (Verbose)
        printf("\n\nWriting output file %s...\n", pszFileName);

    //
    //  Compute size of ctype table (in bytes).
    //
    TblSizeMap = ComputeCTMapSize(pMap);
    TblSize844 = Compute844Size( CTBuf2,
                                 CTBuf3,
                                 sizeof(BYTE) ) * sizeof(WORD);
    TblSize = TblSize844 + TblSizeMap + sizeof(WORD);

    //
    //  Make sure the total size of the table is not greater than 64K.
    //  If it is, then the WORD offsets are too small.
    //
    if (TblSize > MAX_844_TBL_SIZE)
    {
       printf("Write Error: Size of CType table is greater than 64K.\n");
       return (1);
    }

    //
    //  Write the size of the ctype table to the output file.
    //
    wValue = (WORD)TblSize;
    if (FileWrite( pOutputFile,
                   &wValue,
                   sizeof(WORD),
                   1,
                   "CType size" ))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Write CTYPE Mapping table and 8:4:4 table to file.
    //
    if (WriteCTMapTable( pOutputFile,
                         pMap,
                         (WORD)TblSizeMap ))
    {
        fclose(pOutputFile);
        return (1);
    }
    if (Write844TableMap( pOutputFile,
                          pCT,
                          (WORD)TblSize844 ))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Close the output file.
    //
    fclose(pOutputFile);

    //
    //  Print out successful write.
    //
    printf("\nSuccessfully wrote output file %s\n", pszFileName);

    //
    //  Return success.
    //
    return (0);
}
