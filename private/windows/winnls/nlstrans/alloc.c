/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    alloc.c

Abstract:

    This file contains functions that will allocate the necessary memory
    blocks.

    External Routines in this file:
      AllocateMB
      AllocateGlyph
      AllocateTopDBCS
      AllocateDBCS
      AllocateWCTable
      Allocate8
      Insert844
      Insert844Map
      AllocateTemp844
      AllocateCTMap
      AllocateGrid
      AllocateLangException
      AllocateLangExceptionNodes
      AllocateSortDefault
      AllocateReverseDW
      AllocateDoubleCompression
      AllocateIdeographLcid
      AllocateExpansion
      AllocateCompression
      AllocateCompression2Nodes
      AllocateCompression3Nodes
      AllocateException
      AllocateExceptionNodes
      AllocateMultipleWeights
      AllocateIdeographExceptions
      Free844
      FreeCTMap

Revision History:

    07-30-91    JulieB    Created.

--*/



//
//  Include Files.
//

#include "nlstrans.h"




//
//  Forward Declarations.
//

CT_MAP_VALUE
MapTrioToByte(
    PCT_MAP pMap,
    WORD Value1,
    WORD Value2,
    WORD Value3);





//-------------------------------------------------------------------------//
//                            EXTERNAL ROUTINES                            //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  AllocateMB
//
//  This routine allocates all structures needed for the MB table.
//  If an error is encountered while allocating, an error is returned.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

int AllocateMB(
    PCODEPAGE pCP)
{
    //
    //  Allocate MB Table buffer.
    //  Set all entries in MB Table to zero.
    //
    if ((pCP->pMB = (PMB_TBL)malloc(MB_TABLE_SIZE * sizeof(WORD))) == NULL)
    {
        printf("Error: Can't allocate buffer.\n");
        return (1);
    }
    memset(pCP->pMB, 0, MB_TABLE_SIZE * sizeof(WORD));

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  AllocateGlyph
//
//  This routine allocates all structures needed for the Glyph table.
//  If an error is encountered while allocating, an error is returned.
//  All entries in the glyph table are set equal to the entries in the
//  MB table.  If the MB table has not been read in yet, then an error
//  is returned.
//
//  06-02-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int AllocateGlyph(
    PCODEPAGE pCP)
{
    int ctr;                 // loop counter


    //
    //  Allocate Glyph Table buffer.
    //
    if ((pCP->pGlyph = (PGLYPH_TBL)malloc( GLYPH_TABLE_SIZE *
                                           sizeof(WORD) )) == NULL)
    {
        printf("Error: Can't allocate buffer.\n");
        return (1);
    }

    //
    //  Make sure the MB table has already been read in at this point.
    //
    if ((!(pCP->WriteFlags & F_MB)) || (pCP->pMB == NULL))
    {
        printf("Parse Error: MBTABLE must be BEFORE GLYPHTABLE in file.\n");
        return (1);
    }

    //
    //  Set all entries in the Glyph Table to the MB Table entries.
    //  All new glyph values will overwrite the appropriate MB entries
    //  in the glyph table.
    //
    for (ctr = 0; ((ctr < GLYPH_TABLE_SIZE) && (ctr < MB_TABLE_SIZE)); ctr++)
    {
        (pCP->pGlyph)[ctr] = (pCP->pMB)[ctr];
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  AllocateTopDBCS
//
//  This routine allocates the initial DBCS array structure.  If an error
//  is encountered while allocating, an error is returned.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

int AllocateTopDBCS(
    PCODEPAGE pCP,
    int Size)
{
    //
    //  Allocate initial DBCS array structure.
    //
    if ((pCP->pDBCS = (PDBCS_ARRAY)malloc( Size *
                                           sizeof(PDBCS_RANGE) )) == NULL)
    {
        printf("Error: Can't allocate buffer.\n");
        return (1);
    }
    memset(pCP->pDBCS, 0, Size * sizeof(PDBCS_RANGE));

    //
    //  Allocate offset area.
    //
    if ((pCP->pDBCSOff = (PDBCS_OFFSETS)malloc( DBCS_OFFSET_SIZE *
                                                sizeof(WORD) )) == NULL)
    {
        printf("Error: Can't allocate buffer.\n");
        return (1);
    }
    memset(pCP->pDBCSOff, 0, DBCS_OFFSET_SIZE * sizeof(WORD));

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  AllocateDBCS
//
//  This routine allocates all structures needed for the DBCS tables and
//  ranges.  If an error is encountered while allocating, an error is
//  returned.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

int AllocateDBCS(
    PCODEPAGE pCP,
    int Low,
    int High,
    int Index)
{
    PDBCS_RANGE pRange;                // ptr to DBCS range
    PDBCS_TBL_ARRAY pTblArray;         // ptr to DBCS table array
    int Ctr, Ctr2;                     // loop counters
    int NumTables = High - Low + 1;    // number of tables for range
    WORD *pWordPtr;                    // ptr to dbcs buffer


    //
    //  Allocate Range Structure.
    //
    if ((pRange = (PDBCS_RANGE)malloc(sizeof(DBCS_RANGE))) == NULL)
    {
        printf("Error: Can't allocate buffer.\n");
        return (1);
    }
    memset(pRange, 0, sizeof(DBCS_RANGE));

    //
    //  Allocate Table Array.
    //
    if ((pTblArray = (PDBCS_TBL_ARRAY)malloc( NumTables *
                                              sizeof(PDBCS_TBL) )) == NULL)
    {
        printf("Error: Can't allocate buffer.\n");
        return (1);
    }
    memset(pTblArray, 0, NumTables * sizeof(PDBCS_TBL));

    //
    //  Allocate All Tables.
    //
    for (Ctr = 0; Ctr < NumTables; Ctr++)
    {
        //
        //  Allocate table.
        //
        if ((pTblArray[Ctr] = (PDBCS_TBL)malloc( DBCS_TABLE_SIZE *
                                                 sizeof(WORD) )) == NULL)
        {
            printf("Error: Can't allocate buffer.\n");
            return (1);
        }

        //
        //  Set all entries to the Unicode default character.
        //
        pWordPtr = (WORD *)(pTblArray[Ctr]);
        for (Ctr2 = 0; Ctr2 < DBCS_TABLE_SIZE; Ctr2++)
        {
            pWordPtr[Ctr2] = (WORD)(pCP->UniDefaultChar);
        }
    }

    //
    //  Attach the tables to each other.
    //
    pRange->pDBCSTbls = pTblArray;
    (pCP->pDBCS)[Index] = pRange;

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  AllocateWCTable
//
//  This routine allocates the buffer for the Unicode to ANSI translation
//  table.  The buffer is (64K //  Size) bytes in length.
//
//  05-28-92    JulieB      Created.
////////////////////////////////////////////////////////////////////////////

int AllocateWCTable(
    PCODEPAGE pCP,
    int Size)
{
    int Ctr;                      // loop counter
    WORD *pWordPtr;               // ptr to wide character buffer


    //
    //  Allocate translation table buffer.
    //
    if ((pCP->pWC = (PWC_ARRAY)malloc(WC_TABLE_SIZE * Size)) == NULL)
    {
        printf("Error: Can't allocate buffer.\n");
        return (1);
    }

    //
    //  Set all entries to the default character.
    //
    if (Size == sizeof(BYTE))
    {
        memset(pCP->pWC, (BYTE)(pCP->DefaultChar), WC_TABLE_SIZE);
    }
    else if (Size == sizeof(WORD))
    {
        pWordPtr = pCP->pWC;
        for (Ctr = 0; Ctr < WC_TABLE_SIZE; Ctr++)
        {
            pWordPtr[Ctr] = pCP->DefaultChar;
        }
    }
    else
    {
        printf("Code Error: Bad 'Size' parameter for AllocateWCTable.\n");
        return (1);
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  Allocate8
//
//  This routine allocates the top buffer for the 8:4:4 table.
//
//  07-30-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int Allocate8(
    P844_ARRAY *pArr)
{
    //
    //  Allocate top buffer for 8:4:4 table - 256 pointers.
    //
    if ((*pArr = (P844_ARRAY)malloc( TABLE_SIZE_8 *
                                     sizeof(P844_ARRAY) )) == NULL)
    {
        printf("Error: Can't allocate top 8:4:4 buffer.\n");
        return (1);
    }
    memset(*pArr, 0, TABLE_SIZE_8 * sizeof(P844_ARRAY));

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  Insert844
//
//  This routine inserts a WORD or DWORD value into an 8:4:4 table based on
//  the Size parameter.  It does so by allocating the appropriate buffers
//  and filling in the third buffers with the appropriate WORD or DWORD value.
//
//  07-30-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int Insert844(
    P844_ARRAY pArr,
    WORD WChar,
    DWORD Value,
    int *cbBuf2,
    int *cbBuf3,
    int Size)
{
    register int Index;           // index into array
    P844_ARRAY pTbl2;             // pointer to second array
    P844_ARRAY pTbl3;             // pointer to third array


    //
    //  Use the "8" index to get to the second table.
    //  Allocate it if necessary.
    //
    Index = GET8(WChar);
    if ((pTbl2 = (P844_ARRAY)(pArr[Index])) == NULL)
    {
        //
        //  Allocate second table - 16 pointers.
        //
        if ((pTbl2 = (P844_ARRAY)malloc( TABLE_SIZE_4 *
                                         sizeof(P844_ARRAY) )) == NULL)
        {
            printf("Error: Can't allocate second 8:4:4 buffer.\n");
            return (1);
        }
        memset(pTbl2, 0, TABLE_SIZE_4 * sizeof(P844_ARRAY));
        pArr[Index] = pTbl2;

        //
        //  Keep track of how many "second buffer" allocations were made.
        //
        (*cbBuf2)++;
    }

    //
    //  Use the "high 4" index to get to the third table.
    //  Allocate it if necessary.
    //
    Index = GETHI4(WChar);
    if ((pTbl3 = pTbl2[Index]) == NULL)
    {
        //
        //  Allocate third table - 16 words.
        //
        if ((pTbl3 = (P844_ARRAY)malloc(TABLE_SIZE_4 * Size)) == NULL)
        {
            printf("Error: Can't allocate third 8:4:4 buffer.\n");
            return (1);
        }
        memset(pTbl3, 0, TABLE_SIZE_4 * Size);
        pTbl2[Index] = pTbl3;

        //
        //  Keep track of how many "third buffer" allocations were made.
        //
        (*cbBuf3)++;
    }

    //
    //  Use the "low 4" value to index into the third table.
    //  Save the value at this spot.
    //
    Index = GETLO4(WChar);

    if (Size == sizeof(WORD))
    {
        ((WORD *)pTbl3)[Index] = (WORD)Value;
    }
    else if (Size == sizeof(DWORD))
    {
        ((DWORD *)pTbl3)[Index] = (DWORD)Value;
    }
    else
    {
        printf("Code Error: Bad 'Size' parameter for Insert844 Table.\n");
        return (1);
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  Insert844Map
//
//  This routine inserts 3 WORD values into an 8:4:4 table.  It does so by
//  allocating the appropriate buffers and filling in the third buffers
//  with a 1 BYTE value that is the mapping of the given 3 WORD trio.
//
//  10-29-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int Insert844Map(
    P844_ARRAY pArr,
    PCT_MAP pMap,
    WORD WChar,
    WORD Value1,
    WORD Value2,
    WORD Value3,
    int *cbBuf2,
    int *cbBuf3)
{
    register int Index;           // index into array
    P844_ARRAY pTbl2;             // pointer to second array
    PCT_MAP_VALUE pTbl3;          // pointer to third array


    //
    //  Use the "8" index to get to the second table.
    //  Allocate it if necessary.
    //
    Index = GET8(WChar);
    if ((pTbl2 = (P844_ARRAY)(pArr[Index])) == NULL)
    {
        //
        //  Allocate second table - 16 pointers + 1 word.
        //  The additional 1 word will be used when writing this table
        //  to avoid duplicates of the same table.
        //
        if ((pTbl2 = (P844_ARRAY)malloc( (TABLE_SIZE_4 + 1) *
                                         sizeof(P844_ARRAY) )) == NULL)
        {
            printf("Error: Can't allocate second 8:4:4 buffer.\n");
            return (1);
        }
        memset(pTbl2, 0, (TABLE_SIZE_4 + 1) * sizeof(P844_ARRAY));
        pArr[Index] = pTbl2;

        //
        //  Keep track of how many "second buffer" allocations were made.
        //
        (*cbBuf2)++;
    }

    //
    //  Use the "high 4" index to get to the third table.
    //  Allocate it if necessary.
    //
    Index = GETHI4(WChar);
    if ((pTbl3 = pTbl2[Index]) == NULL)
    {
        //
        //  Allocate third table - 16 + 2 bytes.
        //  The 2 extra bytes will be used when writing the table.
        //
        if ((pTbl3 = (PCT_MAP_VALUE)malloc( (TABLE_SIZE_4 + 2) *
                                            (sizeof(CT_MAP_VALUE)) )) == NULL)
        {
            printf("Error: Can't allocate third 8:4:4 buffer.\n");
            return (1);
        }

        //
        //  The last field of the third table is used when writing to the
        //  data file to ensure that each table is written only ONCE
        //  (with muliple pointers to it).  This field takes 1 WORD
        //  (2 bytes) and is initialized to 0.
        //
        memset(pTbl3, 0, (TABLE_SIZE_4 + 2) * (sizeof(CT_MAP_VALUE)));
        pTbl2[Index] = pTbl3;

        //
        //  Keep track of how many "third buffer" allocations were made.
        //
        (*cbBuf3)++;
    }

    //
    //  Use the "low 4" value to index into the third table.
    //  Save the values at this spot.
    //
    Index = GETLO4(WChar);

    //
    //  Map 3 WORD CType trio to 1 BYTE value.
    //
    pTbl3[Index] = MapTrioToByte( pMap,
                                  Value1,
                                  Value2,
                                  Value3 );

    //
    //  Make sure the number of entries in the mapping table is
    //  not greater than MAX_CT_MAP_TBL_SIZE.
    //
    if (pMap->Length >= MAX_CT_MAP_TBL_SIZE)
    {
        printf("Error: CTYPE Mapping Table Too Large.\n");
        return (1);
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  AllocateTemp844
//
//  This routine allocates the temporary storage buffer for the 8:4:4
//  table.  This temporary buffer is used when writing to the output file.
//  The size of the buffer is (TblSize * Size) bytes in length.
//
//  07-30-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int AllocateTemp844(
    PVOID *ppArr,
    int TblSize,
    int Size)
{
    //
    //  Allocate buffer of size TblSize.
    //
    if ((*ppArr = (PVOID)malloc(TblSize * Size)) == NULL)
    {
        printf("Error: Can't allocate temp 8:4:4 buffer.\n");
        return (1);
    }
    memset(*ppArr, 0, TblSize * Size);

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  AllocateCTMap
//
//  This routine allocates all structures needed for the ctype mapping table.
//  If an error is encountered while allocating, an error is returned.
//
////////////////////////////////////////////////////////////////////////////

int AllocateCTMap(
    PCT_MAP *pMap)
{
    //
    //  Allocate buffer mapping table.
    //
    if ((*pMap = (PCT_MAP)malloc(sizeof(CT_MAP))) == NULL)
    {
        printf("Error: Can't allocate buffer for CType Mapping table.\n");
        return (1);
    }
    memset(*pMap, 0, sizeof(CT_MAP));

    //
    //  Allocate mapping table entries.
    //
    if (((*pMap)->pCTValues = (PCT_VALUES)malloc( MAX_CT_MAP_TBL_SIZE *
                                                  sizeof(CT_VALUES) )) == NULL)
    {
        printf("Error: Can't allocate CType mapping table with %d entries.\n",
               MAX_CT_MAP_TBL_SIZE);
        return (1);
    }

    //
    //  Set the first entry to 0 so that any third level table that maps
    //  to 0 will be C1 = 0, C2 = 0, and C3 = 0.
    //
    memset((*pMap)->pCTValues, 0, sizeof(CT_VALUES));
    (*pMap)->Length = 1;

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  AllocateGrid
//
//  This routine allocates the 2D grid for the composite table.
//  The size passed in is the number of precomposed entries that need to
//  go into the table.  Since the exact size of the array is not known yet,
//  the maximum possible size is allocated (size squared).
//
//  07-30-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int AllocateGrid(
    PCOMP_GRID *pCompGrid,
    int TblSize)
{
    //
    //  Allocate 2D grid.
    //  The size of the grid is the TblSize squared plus one to save the
    //  size of the grid.
    //
    if ((*pCompGrid = (PCOMP_GRID)malloc( (TblSize * TblSize + 1) *
                                          sizeof(WORD) )) == NULL)
    {
        printf("Error: Can't allocate buffer.\n");
        return (1);
    }
    memset(*pCompGrid, 0, (TblSize * TblSize + 1) * sizeof(WORD));

    //
    //  Save the size of the grid in the first spot.
    //
    (*pCompGrid)[0] = (WORD)TblSize;

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  AllocateLangException
//
//  This routine allocates the exception header and the exception table.
//  The size of the table is determined by the TblSize parameter.  The
//  pointer to the header and the table are stored in the language
//  structure.
//
//  08-30-95    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int AllocateLangException(
    PLANG_EXCEPT pLangExcept,
    int TblSize)
{
    //
    //  Set the number of Exception entries in table.
    //
    pLangExcept->NumException = TblSize;

    //
    //  Allocate buffer of size TblSize for Exception header.
    //
    if ((pLangExcept->pExceptHdr =
            (PL_EXCEPT_HDR)malloc(TblSize * sizeof(L_EXCEPT_HDR))) == NULL)
    {
        printf("Error: Can't allocate buffer for Exception Header.\n");
        return (1);
    }
    memset(pLangExcept->pExceptHdr, 0, TblSize * sizeof(L_EXCEPT_HDR));

    //
    //  Allocate buffer of size TblSize for Exception table.
    //
    if ((pLangExcept->pExceptTbl =
            (PL_EXCEPT_TBL)malloc(TblSize * sizeof(PL_EXCEPT_NODE))) == NULL)
    {
        printf("Error: Can't allocate buffer for Exception table.\n");
        return (1);
    }
    memset(pLangExcept->pExceptTbl, 0, TblSize * sizeof(PL_EXCEPT_NODE));

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  AllocateLangExceptionNodes
//
//  This routine allocates the exception nodes for the exception table.
//  The size of the table is determined by the TblSize parameter.  The
//  pointer to the nodes is stored in the exception table at the Index
//  given.
//
//  08-30-95    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int AllocateLangExceptionNodes(
    PLANG_EXCEPT pLangExcept,
    int TblSize,
    int Index)
{
    PL_EXCEPT_NODE pExcNode;      // ptr to exception node


    //
    //  Allocate buffer of size TblSize for Exception nodes.
    //
    if ((pExcNode = (PL_EXCEPT_NODE)malloc( TblSize *
                                            sizeof(L_EXCEPT_NODE) )) == NULL)
    {
        printf("Error: Can't allocate buffer for Exception Nodes.\n");
        return (1);
    }
    memset(pExcNode, 0, TblSize * sizeof(L_EXCEPT_NODE));

    //
    //  Set pointer in exception table.
    //
    (pLangExcept->pExceptTbl)[Index] = pExcNode;

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  AllocateSortDefault
//
//  This routine allocates the sort default table - 64K DWORDS.  The pointer
//  to the new table is stored in the sortkey structure.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int AllocateSortDefault(
    PSORTKEY pSKey)
{
    //
    //  Allocate buffer of size 64K DWORDS for sort default table.
    //
    if ((pSKey->pDefault = (PSKEY)malloc( SKEY_TBL_SIZE *
                                          sizeof(SKEY) )) == NULL)
    {
        printf("Error: Can't allocate buffer for sortkey default table.\n");
        return (1);
    }
    memset(pSKey->pDefault, 0, SKEY_TBL_SIZE * sizeof(SKEY));

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  AllocateReverseDW
//
//  This routine allocates the reverse diacritic weight table.  The size of
//  the table is determined by the TblSize parameter.  The pointer to the
//  new table is stored in the sorttables structure.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int AllocateReverseDW(
    PSORT_TABLES pSTbl,
    int TblSize)
{
    //
    //  Allocate buffer of size TblSize for RevrseDW table.
    //
    if ((pSTbl->pReverseDW = (PREV_DW)malloc( TblSize *
                                              sizeof(REV_DW) )) == NULL)
    {
        printf("Error: Can't allocate buffer for Reverse DW table.\n");
        return (1);
    }
    memset(pSTbl->pReverseDW, 0, TblSize * sizeof(REV_DW));

    //
    //  Set the number of ReverseDW entries in table.
    //
    pSTbl->NumReverseDW = TblSize;

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  AllocateDoubleCompression
//
//  This routine allocates the double compression table.  The size of
//  the table is determined by the TblSize parameter.  The pointer to the
//  new table is stored in the sorttables structure.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int AllocateDoubleCompression(
    PSORT_TABLES pSTbl,
    int TblSize)
{
    //
    //  Allocate buffer of size TblSize for Double Compression table.
    //
    if ((pSTbl->pDblCompression =
        (PDBL_COMPRESS)malloc(TblSize * sizeof(DBL_COMPRESS))) == NULL)
    {
        printf("Error: Can't allocate buffer for Double Compression table.\n");
        return (1);
    }
    memset(pSTbl->pDblCompression, 0, TblSize * sizeof(DBL_COMPRESS));

    //
    //  Set the number of Double Compression entries in table.
    //
    pSTbl->NumDblCompression = TblSize;

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  AllocateIdeographLcid
//
//  This routine allocates the ideograph lcid table.  The size of
//  the table is determined by the TblSize parameter.  The pointer to the
//  new table is stored in the sorttables structure.
//
//  09-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int AllocateIdeographLcid(
    PSORT_TABLES pSTbl,
    int TblSize)
{
    //
    //  Allocate buffer of size TblSize for IdeographLcid table.
    //
    if ((pSTbl->pIdeographLcid =
           (PIDEOGRAPH_LCID)malloc(TblSize * sizeof(IDEOGRAPH_LCID))) == NULL)
    {
        printf("Error: Can't allocate buffer for Ideograph Lcid table.\n");
        return (1);
    }
    memset(pSTbl->pIdeographLcid, 0, TblSize * sizeof(IDEOGRAPH_LCID));

    //
    //  Set the number of Ideograph Lcid entries in table.
    //
    pSTbl->NumIdeographLcid = TblSize;

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  AllocateExpansion
//
//  This routine allocates the expansion table.  The size of
//  the table is determined by the TblSize parameter.  The pointer to the
//  new table is stored in the sorttables structure.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int AllocateExpansion(
    PSORT_TABLES pSTbl,
    int TblSize)
{
    //
    //  Allocate buffer of size TblSize for Expansion table.
    //
    if ((pSTbl->pExpansion = (PEXPAND)malloc( TblSize *
                                              sizeof(EXPAND) )) == NULL)
    {
        printf("Error: Can't allocate buffer for Expansion table.\n");
        return (1);
    }
    memset(pSTbl->pExpansion, 0, TblSize * sizeof(EXPAND));

    //
    //  Set the number of Expansion entries in table.
    //
    pSTbl->NumExpansion = TblSize;

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  AllocateCompression
//
//  This routine allocates the compression header and the compression table.
//  The size of the table is determined by the TblSize parameter.  The
//  pointer to the header and the table are stored in the sorttables
//  structure.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int AllocateCompression(
    PSORT_TABLES pSTbl,
    int TblSize)
{
    //
    //  Set the number of Compression entries in table.
    //
    pSTbl->NumCompression = TblSize;

    //
    //  Allocate buffer of size TblSize for Compression header.
    //
    if ((pSTbl->pCompressHdr =
            (PCOMPRESS_HDR)malloc(TblSize * sizeof(COMPRESS_HDR))) == NULL)
    {
        printf("Error: Can't allocate buffer for Compression Header.\n");
        return (1);
    }
    memset(pSTbl->pCompressHdr, 0, TblSize * sizeof(COMPRESS_HDR));

    //
    //  Allocate buffer of size TblSize for Compression 2 table.
    //
    if ((pSTbl->pCompress2Tbl =
            (PCOMPRESS_2_TBL)malloc( TblSize *
                                     sizeof(PCOMPRESS_2_NODE) )) == NULL)
    {
        printf("Error: Can't allocate buffer for Compression 2 table.\n");
        return (1);
    }
    memset(pSTbl->pCompress2Tbl, 0, TblSize * sizeof(PCOMPRESS_2_NODE));

    //
    //  Allocate buffer of size TblSize for Compression 3 table.
    //
    if ((pSTbl->pCompress3Tbl =
            (PCOMPRESS_3_TBL)malloc( TblSize *
                                     sizeof(PCOMPRESS_3_NODE) )) == NULL)
    {
        printf("Error: Can't allocate buffer for Compression 3 table.\n");
        return (1);
    }
    memset(pSTbl->pCompress3Tbl, 0, TblSize * sizeof(PCOMPRESS_3_NODE));

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  AllocateCompression2Nodes
//
//  This routine allocates the compression 2 nodes for the compression table.
//  The size of the table is determined by the TblSize parameter.  The
//  pointer to the nodes is stored in the compression table at the Index
//  given.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int AllocateCompression2Nodes(
    PSORT_TABLES pSTbl,
    int TblSize,
    int Index)
{
    PCOMPRESS_2_NODE pCompNode;          // ptr to compression 2 node


    //
    //  Allocate buffer of size TblSize for Compression nodes.
    //
    if ((pCompNode =
            (PCOMPRESS_2_NODE)malloc( TblSize *
                                      sizeof(COMPRESS_2_NODE) )) == NULL)
    {
        printf("Error: Can't allocate buffer for Compression 2 Nodes.\n");
        return (1);
    }
    memset(pCompNode, 0, TblSize * sizeof(COMPRESS_2_NODE));

    //
    //  Set pointer in compression 2 table.
    //
    (pSTbl->pCompress2Tbl)[Index] = pCompNode;

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  AllocateCompression3Nodes
//
//  This routine allocates the compression 3 nodes for the compression table.
//  The size of the table is determined by the TblSize parameter.  The
//  pointer to the nodes is stored in the compression table at the Index
//  given.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int AllocateCompression3Nodes(
    PSORT_TABLES pSTbl,
    int TblSize,
    int Index)
{
    PCOMPRESS_3_NODE pCompNode;          // ptr to compression 3 node


    //
    //  Allocate buffer of size TblSize for Compression nodes.
    //
    if ((pCompNode =
            (PCOMPRESS_3_NODE)malloc( TblSize *
                                      sizeof(COMPRESS_3_NODE) )) == NULL)
    {
        printf("Error: Can't allocate buffer for Compression 3 Nodes.\n");
        return (1);
    }
    memset(pCompNode, 0, TblSize * sizeof(COMPRESS_3_NODE));

    //
    //  Set pointer in compression 3 table.
    //
    (pSTbl->pCompress3Tbl)[Index] = pCompNode;

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  AllocateException
//
//  This routine allocates the exception header and the exception table.
//  The size of the table is determined by the TblSize parameter.  The
//  pointer to the header and the table are stored in the sorttables
//  structure.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int AllocateException(
    PSORT_TABLES pSTbl,
    int TblSize)
{
    //
    //  Set the number of Exception entries in table.
    //
    pSTbl->NumException = TblSize;

    //
    //  Allocate buffer of size TblSize for Exception header.
    //
    if ((pSTbl->pExceptHdr =
            (PEXCEPT_HDR)malloc(TblSize * sizeof(EXCEPT_HDR))) == NULL)
    {
        printf("Error: Can't allocate buffer for Exception Header.\n");
        return (1);
    }
    memset(pSTbl->pExceptHdr, 0, TblSize * sizeof(EXCEPT_HDR));

    //
    //  Allocate buffer of size TblSize for Exception table.
    //
    if ((pSTbl->pExceptTbl =
            (PEXCEPT_TBL)malloc(TblSize * sizeof(PEXCEPT_NODE))) == NULL)
    {
        printf("Error: Can't allocate buffer for Exception table.\n");
        return (1);
    }
    memset(pSTbl->pExceptTbl, 0, TblSize * sizeof(PEXCEPT_NODE));

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  AllocateExceptionNodes
//
//  This routine allocates the exception nodes for the exception table.
//  The size of the table is determined by the TblSize parameter.  The
//  pointer to the nodes is stored in the exception table at the Index
//  given.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int AllocateExceptionNodes(
    PSORT_TABLES pSTbl,
    int TblSize,
    int Index)
{
    PEXCEPT_NODE pExcNode;        // ptr to exception node


    //
    //  Allocate buffer of size TblSize for Exception nodes.
    //
    if ((pExcNode = (PEXCEPT_NODE)malloc( TblSize *
                                          sizeof(EXCEPT_NODE) )) == NULL)
    {
        printf("Error: Can't allocate buffer for Exception Nodes.\n");
        return (1);
    }
    memset(pExcNode, 0, TblSize * sizeof(EXCEPT_NODE));

    //
    //  Set pointer in exception table.
    //
    (pSTbl->pExceptTbl)[Index] = pExcNode;

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  AllocateMultipleWeights
//
//  This routine allocates the multiple weights table.  The size of
//  the table is determined by the TblSize parameter.  The pointer to the
//  new table is stored in the sorttables structure.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int AllocateMultipleWeights(
    PSORT_TABLES pSTbl,
    int TblSize)
{
    //
    //  Allocate buffer of size TblSize for Multiple Weights table.
    //
    if ((pSTbl->pMultiWeight = (PMULTI_WT)malloc( TblSize *
                                                  sizeof(MULTI_WT) )) == NULL)
    {
        printf("Error: Can't allocate buffer for Multiple Weight table.\n");
        return (1);
    }
    memset(pSTbl->pMultiWeight, 0, TblSize * sizeof(MULTI_WT));

    //
    //  Set the number of Multiple Weight entries in table.
    //
    pSTbl->NumMultiWeight = TblSize;

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  AllocateIdeographExceptions
//
//  This routine allocates the ideograph exception table.  The size of
//  the table is determined by the TblSize parameter.  The pointer to the
//  new table is stored in the ideograph exception structure.
//
//  09-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int AllocateIdeographExceptions(
    PIDEOGRAPH_EXCEPT pIdeograph,
    int TblSize,
    int NumColumns)
{
    //
    //  Allocate buffer of size TblSize for Ideograph Exception table.
    //
    if (NumColumns == 2)
    {
        if ((pIdeograph->pExcept =
                (PIDEOGRAPH_NODE)malloc( TblSize *
                                         sizeof(IDEOGRAPH_NODE) )) == NULL)
        {
            printf("Error: Can't allocate buffer for Ideograph Exception table.\n");
            return (1);
        }
        memset(pIdeograph->pExcept, 0, TblSize * sizeof(IDEOGRAPH_NODE));

        pIdeograph->pExceptEx = NULL;
    }
    else if (NumColumns == 4)
    {
        if ((pIdeograph->pExceptEx =
                (PIDEOGRAPH_NODE_EX)malloc( TblSize *
                                          sizeof(IDEOGRAPH_NODE_EX) )) == NULL)
        {
            printf("Error: Can't allocate buffer for Ideograph Exception table.\n");
            return (1);
        }
        memset(pIdeograph->pExceptEx, 0, TblSize * sizeof(IDEOGRAPH_NODE_EX));

        pIdeograph->pExcept = NULL;
    }
    else
    {
        printf("Parse Error: The Number of Columns must be either 2 or 4.\n");
        return (1);
    }

    //
    //  Set the number of ideograph exception entries in table.
    //
    pIdeograph->NumEntries = TblSize;
    pIdeograph->NumColumns = NumColumns;

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  Free844
//
//  This routine frees the memory used by an 8:4:4 table pointed to by pArr.
//
//  07-30-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

void Free844(
    P844_ARRAY pArr)
{
    int Ctr1, Ctr2;               // loop counters
    P844_ARRAY pArr2;             // ptr to second arrays


    if (pArr != NULL)
    {
        for (Ctr1 = 0; Ctr1 < TABLE_SIZE_8; Ctr1++)
        {
            if ((pArr2 = (P844_ARRAY)(pArr[Ctr1])) != NULL)
            {
                for (Ctr2 = 0; Ctr2 < TABLE_SIZE_4; Ctr2++)
                {
                    if (pArr2[Ctr2] != NULL)
                    {
                        free(pArr2[Ctr2]);
                    }
                }
                free(pArr2);
            }
        }
        free(pArr);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  FreeCTMap
//
//  This routine frees the memory used by the ctype mapping table pointed
//  to by pMap.
//
////////////////////////////////////////////////////////////////////////////

void FreeCTMap(
    PCT_MAP pMap)
{
    if (pMap != NULL)
    {
        free(pMap->pCTValues);
        free(pMap);
    }
}




//-------------------------------------------------------------------------//
//                            INTERNAL ROUTINES                            //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  MapTrioToByte
//
//  This routine searches through the mapping table for the given CType trio.
//  If it already exists, the entry value is returned.  Otherwise, it adds
//  the new trio to the mapping table and returns the new entry value.
//
////////////////////////////////////////////////////////////////////////////

CT_MAP_VALUE MapTrioToByte(
    PCT_MAP pMap,
    WORD Value1,
    WORD Value2,
    WORD Value3)
{
    PCT_VALUES pEntry;            // ptr to entry
    CT_MAP_VALUE EntryNum;        // entry number
    int Ctr;                      // loop counter


    //
    //  Search through the current entries to see if the ctype trio
    //  already exists.
    //
    for (Ctr = 0; Ctr < pMap->Length; Ctr++)
    {
        //
        //  Check the entry.
        //
        if ( ((pMap->pCTValues[Ctr]).CType1 == Value1) &&
             ((pMap->pCTValues[Ctr]).CType2 == Value2) &&
             ((pMap->pCTValues[Ctr]).CType3 == Value3) )
        {
            //
            //  Entry already exists.  Return the entry number.
            //
            if (Verbose)
                printf("Mapping Entry %d:\tCT1 = %x\tCT2 = %x\tCT3 = %x\n",
                        Ctr, Value1, Value2, Value3);

            return (Ctr);
        }
    }

    //
    //  The given CType trio does not yet exist in the table.
    //  Add the new trio entry to the table and increment the
    //  total number of entries.
    //
    pEntry = &(pMap->pCTValues[pMap->Length]);
    pEntry->CType1 = Value1;
    pEntry->CType2 = Value2;
    pEntry->CType3 = Value3;

    EntryNum = (CT_MAP_VALUE)(pMap->Length);

    pMap->Length++;

    if (Verbose)
        printf("Mapping New Entry %d:\tCT1 = %x\tCT2 = %x\tCT3 = %x\n",
                EntryNum, Value1, Value2, Value3);

    //
    //  Return the new entry number.
    //
    return (EntryNum);
}
