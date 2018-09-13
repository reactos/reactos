/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    table.c

Abstract:

    This file contains functions necessary to manipulate the various
    table structures.

    External Routines in this file:
      ComputeMBSize
      Compute844Size
      ComputeCTMapSize
      Write844Table
      Write844TableMap
      WriteCTMapTable
      WriteWords
      FileWrite
      RemoveDuplicate844Levels

Revision History:

    06-14-91    JulieB    Created.

--*/



//
//  Include Files.
//

#include "nlstrans.h"




//
//  Constant Declarations.
//

#define HASH_SIZE  65521          // size of hash table (prime #)




//
//  Typedef Declarations.
//

typedef struct hash_object_s
{
    PVOID pTable;                      // ptr to table
    struct hash_object_s *pNext;       // ptr to next hash node
} CT_HASH_OBJECT, *PCT_HASH_OBJECT;




//
//  Forward Declarations.
//

void
RemoveDuplicate844Level2(
    P844_ARRAY pArr,
    int *pBuf2);

void
RemoveDuplicate844Level3(
    P844_ARRAY pArr,
    int *pBuf3,
    int Size);

PVOID
FindHashTable(
    PVOID pTbl,
    PCT_HASH_OBJECT *pHashTbl,
    int Size);

DWORD
GetHashVal(
    PVOID pTbl,
    int Size);

void
FreeHashTable(
    PCT_HASH_OBJECT *pHashTbl);





//-------------------------------------------------------------------------//
//                            EXTERNAL ROUTINES                            //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  ComputeMBSize
//
//  This routine returns the size (in words) of the MB, Glyph, and DBCS
//  tables.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

int ComputeMBSize(
    PCODEPAGE pCP)
{
    int TblSize;                  // size of table
    int Ctr;                      // loop counter
    register int NumRanges;       // number of DBCS ranges
    PDBCS_ARRAY pArray;           // ptr to DBCS array


    //
    //  Compute static size of table.
    //
    if (pCP->WriteFlags & F_GLYPH)
    {
        TblSize = 1 + MB_TABLE_SIZE + (1 + GLYPH_TABLE_SIZE) + 1;
    }
    else
    {
        TblSize = 1 + MB_TABLE_SIZE + (1) + 1;
    }

    //
    //  Compute size with DBCS tables (if any).
    //
    NumRanges = pCP->NumDBCSRanges;
    if ((NumRanges > 0) && (pCP->WriteFlags & F_DBCS))
    {
        TblSize += DBCS_OFFSET_SIZE;

        pArray = pCP->pDBCS;
        for (Ctr = 0; Ctr < NumRanges; Ctr++)
        {
             TblSize += ((pArray[Ctr]->HighRange -
                          pArray[Ctr]->LowRange + 1) * DBCS_TABLE_SIZE);
        }
    }

    if (Verbose)
        printf("  Complete MB Table Size = %d\n", TblSize);

    //
    //  Return the table size.
    //
    return (TblSize);
}


////////////////////////////////////////////////////////////////////////////
//
//  Compute844Size
//
//  This routine returns the size (in words) of the 8:4:4 WORD or DWORD
//  table.
//
//  07-30-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int Compute844Size(
    int cbBuf2,
    int cbBuf3,
    int Size)
{
    int TblSize;                  // size of table


    //
    //  Adjust size of cbBuf2 and cbBuf3 for the two empty levels.
    //
    cbBuf2++;
    cbBuf3++;

    //
    //  Compute size of table.
    //
    TblSize = TABLE_SIZE_8 +
              (TABLE_SIZE_4 * cbBuf2) +
              (TABLE_SIZE_4 * cbBuf3 * Size / sizeof(WORD));

    if (Verbose)
        printf("  844 Table Size = %d\t\tBuf2 = %d\tBuf3 = %d\n",
                TblSize, cbBuf2, cbBuf3);

    //
    //  Return the table size.
    //
    return (TblSize);
}


////////////////////////////////////////////////////////////////////////////
//
//  ComputeCTMapSize
//
//  This routine returns the size of the ctype mapping table.
//
////////////////////////////////////////////////////////////////////////////

DWORD ComputeCTMapSize(
    PCT_MAP pMap)
{
    DWORD TblSize;                // size of table


    //
    //  Compute size of table.
    //
    TblSize = sizeof(WORD) + (pMap->Length * sizeof(CT_VALUES));

    if (Verbose)
        printf("  Mapping Table Size = %d\n", TblSize);

    //
    //  Return the table size.
    //
    return (TblSize);
}


////////////////////////////////////////////////////////////////////////////
//
//  Write844Table
//
//  This routine writes the 8:4:4 WORD table to the output file.
//
//  07-30-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int Write844Table(
    FILE *pOutputFile,
    P844_ARRAY pArr,
    int cbBuf2,
    int TblSize,
    int Size)
{
    WORD EmptyLevel2Offset;            // empty level 2 offset
    WORD EmptyLevel3Offset;            // empty level 3 offset
    WORD Pos2;                         // position array 2
    WORD Pos3;                         // position array 3
    WORD PosTemp2 = TABLE_SIZE_4;      // position in pTemp2
    WORD *pTemp;                       // temporary storage
    PVOID pTemp2;                      // temporary storage
    DWORD Ctr, Ctr2, Ctr3;             // loop counters
    P844_ARRAY ptr2;                   // ptr to second array
    P844_ARRAY ptr3;                   // ptr to third array


    //
    //  Need to adjust cbBuf2 for the empty level 2 table.
    //  TblSize was already adjusted for the empty level tables
    //  in Compute844Size.
    //
    cbBuf2++;

    //
    //  Set up the position offsets and the empty second and third level
    //  table offsets.  All Unicode characters that have no mappings will
    //  point to the empty tables.  Grab the first table in the second
    //  level tables (offset TABLE_SIZE_8 in pTemp) and the first table in
    //  the third level tables (offset TABLE_SIZE_8 + (cbBuf2 * TABLE_SIZE_4))
    //  for the empty tables.
    //
    EmptyLevel2Offset = TABLE_SIZE_8;
    Pos2 = EmptyLevel2Offset + TABLE_SIZE_4;
    EmptyLevel3Offset = TABLE_SIZE_8 + (cbBuf2 * TABLE_SIZE_4);
    Pos3 = EmptyLevel3Offset + (TABLE_SIZE_4 * Size / sizeof(WORD));

    //
    //  Allocate temporary storage buffers.
    //
    if (AllocateTemp844( &pTemp,
                         EmptyLevel3Offset,
                         sizeof(WORD) ))
    {
        return (1);
    }
    if (AllocateTemp844( &pTemp2,
                         TblSize - EmptyLevel3Offset,
                         Size ))
    {
        return (1);
    }

    //
    //  Set up the empty second level table to point to the empty third
    //  level table.
    //
    for (Ctr2 = 0; Ctr2 < TABLE_SIZE_4; Ctr2++)
    {
        pTemp[EmptyLevel2Offset + Ctr2] = EmptyLevel3Offset;
    }

    //
    //  For each entry in the array, copy the appropriate offsets
    //  to the storage buffer.
    //
    for (Ctr = 0; Ctr < TABLE_SIZE_8; Ctr++)
    {
        if ((ptr2 = (P844_ARRAY)(pArr[Ctr])) != NULL)
        {
            pTemp[Ctr] = Pos2;
            for (Ctr2 = 0; Ctr2 < TABLE_SIZE_4; Ctr2++)
            {
                if ((ptr3 = ptr2[Ctr2]) != NULL)
                {
                    pTemp[Pos2 + Ctr2] = Pos3;
                    for (Ctr3 = 0; Ctr3 < TABLE_SIZE_4; Ctr3++)
                    {
                        memcpy( ((BYTE *)pTemp2) + ((PosTemp2 + Ctr3) * Size),
                                ((BYTE *)ptr3) + (Ctr3 * Size),
                                Size );
                    }

                    //
                    //  When advancing the Pos3 counter, must compensate
                    //  for the Size (multiply by 2 for DWORD).
                    //
                    Pos3 += (TABLE_SIZE_4 * Size / sizeof(WORD));
                    PosTemp2 += TABLE_SIZE_4;
                }
                else
                {
                    pTemp[Pos2 + Ctr2] = EmptyLevel3Offset;
                }
            }
            Pos2 += TABLE_SIZE_4;
        }
        else
        {
            pTemp[Ctr] = EmptyLevel2Offset;
        }
    }

    //
    //  Write temp storage buffers to output file.
    //
    if (FileWrite( pOutputFile,
                   pTemp,
                   sizeof(WORD),
                   Pos2,
                   "8:4:4 buffer" ))
    {
        return (1);
    }

    if (FileWrite( pOutputFile,
                   pTemp2,
                   Size,
                   PosTemp2,
                   "8:4:4 buffer" ))
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
//  Write844TableMap
//
//  This routine writes the 8:4:4 BYTE "mapped" table to the output file.
//
//  10-29-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int Write844TableMap(
    FILE *pOutputFile,
    P844_ARRAY pArr,
    WORD TblSize)
{
    WORD EmptyLevel2Offset;            // empty level 2 offset
    WORD EmptyLevel3Offset;            // empty level 3 offset
    WORD Pos;                          // position level 2 & 3
    WORD *pTemp2;                      // temporary storage - level 2
    BYTE *pTempTbl;                    // temporary storage - entire table
    DWORD Ctr, Ctr2, Ctr3;             // loop counters
    P844_ARRAY ptr2;                   // ptr to second array
    PCT_MAP_VALUE ptr3;                // ptr to third array


    //
    //  Set up the second and third level empty tables.
    //
    EmptyLevel3Offset = (TABLE_SIZE_8 * sizeof(WORD));
    EmptyLevel2Offset = EmptyLevel3Offset + (TABLE_SIZE_4 * sizeof(BYTE));

    //
    //  Set up position offset for the regular second and third level
    //  tables.
    //
    Pos = EmptyLevel2Offset + (TABLE_SIZE_4 * sizeof(WORD));

    //
    //  Allocate temporary storage buffers.
    //
    if (AllocateTemp844( &pTemp2,
                         TABLE_SIZE_4,
                         sizeof(WORD) ))
    {
        return (1);
    }
    if (AllocateTemp844( &pTempTbl,
                         TblSize,
                         sizeof(BYTE) ))
    {
        return (1);
    }

    //
    //  Set up the empty second level table to point to the empty third
    //  level table.
    //
    for (Ctr = 0; Ctr < TABLE_SIZE_4; Ctr++)
    {
        pTemp2[Ctr] = EmptyLevel3Offset;
    }
    memcpy( &pTempTbl[EmptyLevel2Offset],
            pTemp2,
            TABLE_SIZE_4 * sizeof(WORD) );

    //
    //  For each entry in the array, copy the appropriate offsets
    //  to the storage buffers.
    //
    for (Ctr = 0; Ctr < TABLE_SIZE_8; Ctr++)
    {
        if ((ptr2 = (P844_ARRAY)(pArr[Ctr])) != NULL)
        {
            //
            //  See if the table is a duplicate.
            //
            if (ptr2[DUPLICATE_OFFSET] != 0)
            {
                //
                //  Table IS a duplicate, so just save the offset.
                //
                ((WORD *)pTempTbl)[Ctr] = (WORD)(ptr2[DUPLICATE_OFFSET]);

                //
                //  Set the duplicate pointer to null in the previous level,
                //  so that freeing of the 844 table is simpler.
                //
                pArr[Ctr] = NULL;
            }
            else
            {
                //
                //  Table is NOT a duplicate.
                //  Copy it and save the position for use later if
                //  it's a duplicate.
                //
                for (Ctr2 = 0; Ctr2 < TABLE_SIZE_4; Ctr2++)
                {
                    if ((ptr3 = ptr2[Ctr2]) != NULL)
                    {
                        //
                        //  See if the table is a duplicate.
                        //
                        if (*(WORD *)(ptr3 + DUPLICATE_OFFSET) != 0)
                        {
                            //
                            //  Table IS a duplicate, so just save the
                            //  offset.
                            //
                            pTemp2[Ctr2] = *(WORD *)(ptr3 + DUPLICATE_OFFSET);

                            //
                            //  Set the duplicate pointer to null in the
                            //  previous level, so that freeing of the
                            //  844 table is simpler.
                            //
                            ptr2[Ctr2] = NULL;
                        }
                        else
                        {
                            //
                            //  Table is NOT yet a duplicate.
                            //  Save the position in case this third
                            //  level table is used again.
                            //
                            *(WORD *)(ptr3 + DUPLICATE_OFFSET) = Pos;
                            pTemp2[Ctr2] = Pos;

                            //
                            //  Copy the third level table to the buffer
                            //  and update the position counter.
                            //
                            for (Ctr3 = 0; Ctr3 < TABLE_SIZE_4; Ctr3++)
                            {
                                pTempTbl[Pos + Ctr3] = ptr3[Ctr3];
                            }

                            Pos += TABLE_SIZE_4;
                        }
                    }
                    else
                    {
                        pTemp2[Ctr2] = EmptyLevel3Offset;
                    }
                }

                //
                //  Save the position in case this second level table is
                //  used again.
                //
                (WORD)ptr2[DUPLICATE_OFFSET] = Pos;

                //
                //  Copy the second level table to the buffer,
                //  update the first level table with the position of
                //  the second level table, and update the position
                //  counter.
                //
                memcpy( &pTempTbl[Pos],
                        pTemp2,
                        TABLE_SIZE_4 * sizeof(WORD) );

                ((WORD *)pTempTbl)[Ctr] = Pos;

                Pos += (TABLE_SIZE_4 * sizeof(WORD));
            }
        }
        else
        {
            ((WORD *)pTempTbl)[Ctr] = EmptyLevel2Offset;
        }
    }

    //
    //  Write temp storage buffers to output file.
    //
    if (FileWrite( pOutputFile,
                   pTempTbl,
                   sizeof(BYTE),
                   TblSize,
                   "8:4:4 buffer" ))
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
//  WriteCTMapTable
//
//  This routine writes the ctype mapping table to the output file.
//
////////////////////////////////////////////////////////////////////////////

int WriteCTMapTable(
    FILE *pOutputFile,
    PCT_MAP pMap,
    WORD MapSize)
{
    //
    //  Write the size of the mapping table.
    //
    if (FileWrite( pOutputFile,
                   &MapSize,
                   sizeof(WORD),
                   1,
                   "Mapping Table size" ))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Write mapping table to output file.
    //
    if (FileWrite( pOutputFile,
                   pMap->pCTValues,
                   MapSize - sizeof(WORD),
                   1,
                   "Mapping Table buffer" ))
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
//  WriteWords
//
//  This routine writes multiple words of the same value to the output file.
//  The number of values written is determined by the Num parameter.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

int WriteWords(
    FILE *pOutputFile,
    WORD Value,
    int Num)
{
    //
    //  Write the given 'Value' as a WORD 'Num' times to the output file.
    //
    if (FileWrite( pOutputFile,
                   &Value,
                   sizeof(WORD),
                   Num,
                   "WRITE WORDS" ))
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
//  FileWrite
//
//  This routine writes the given buffer to the output file.  If an error is
//  encountered, then it is returned.
//
//  07-30-91    JulieB    Created.
//  12-10-91    JulieB    Modified for new table format.
////////////////////////////////////////////////////////////////////////////

int FileWrite(
    FILE *pOutputFile,
    void *Buffer,
    int Size,
    int Count,
    char *ErrStr)
{
    //
    //  Write information to output file.
    //
    if (fwrite( Buffer,
                Size,
                Count,
                pOutputFile ) != (unsigned int)Count)
    {
        printf("Write Error: Can't write %s to file.\n", ErrStr);
        return (1);
    }

    //
    //  Return success.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  RemoveDuplicate844Levels
//
//  This routine removes all duplicate second levels and all duplicate
//  third levels from an 8:4:4 table.
//
////////////////////////////////////////////////////////////////////////////

void RemoveDuplicate844Levels(
    P844_ARRAY pArr,
    int *pBuf2,
    int *pBuf3,
    int Size)
{
    //
    //  Remove the duplicates from the third level of the 8:4:4 table
    //  first.
    //
    RemoveDuplicate844Level3( pArr,
                              pBuf3,
                              Size );

    //
    //  Remove the duplicates from the second level of the 8:4:4 table.
    //
    RemoveDuplicate844Level2( pArr,
                              pBuf2 );
}




//-------------------------------------------------------------------------//
//                            INTERNAL ROUTINES                            //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  RemoveDuplicate844Level2
//
//  This routine removes all duplicate second levels from the given 8:4:4
//  table.
//
////////////////////////////////////////////////////////////////////////////

void RemoveDuplicate844Level2(
    P844_ARRAY pArr,
    int *pBuf2)
{
    P844_ARRAY pTbl2;             // ptr to second array
    P844_ARRAY pCmp;              // ptr to second array to compare
    int Ctr, Ctr2, Ctr3;          // loop counters


    //
    //  Search through all second level tables.  If there is a duplicate,
    //  fix the affected pointers in the first level table and free the
    //  duplicate table.
    //
    for (Ctr = 1; Ctr < TABLE_SIZE_8; Ctr++)
    {
        if ((pTbl2 = (P844_ARRAY)(pArr[Ctr])) != NULL)
        {
            //
            //  See if any of the previous second level tables are the
            //  same as the current one.
            //
            for (Ctr2 = Ctr - 1; Ctr2 >= 0; Ctr2--)
            {
                if ((pCmp = (P844_ARRAY)(pArr[Ctr2])) != NULL)
                {
                    //
                    //  Compare each entry in both tables to see if
                    //  the tables are the same.
                    //
                    for (Ctr3 = 0; Ctr3 < TABLE_SIZE_4; Ctr3++)
                    {
                        if (pTbl2[Ctr3] != pCmp[Ctr3])
                        {
                            break;
                        }
                    }
                    if (Ctr3 == TABLE_SIZE_4)
                    {
                        //
                        //  Tables are the same.  Fix the pointer
                        //  in the first level table.
                        //
                        pArr[Ctr] = pCmp;

                        //
                        //  Free the duplicate second level table.
                        //
                        free(pTbl2);

                        //
                        //  Decrement the number of second level tables.
                        //
                        (*pBuf2)--;

                        //
                        //  Found the duplicate, so break out of the
                        //  comparison loop.
                        //
                        break;
                    }
                }
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  RemoveDuplicate844Level3
//
//  This routine removes all duplicate third levels from the given 8:4:4
//  table.
//
////////////////////////////////////////////////////////////////////////////

void RemoveDuplicate844Level3(
    P844_ARRAY pArr,
    int *pBuf3,
    int Size)
{
    P844_ARRAY pTbl2;             // ptr to second array
    PVOID pTbl3;                  // ptr to third array
    PVOID pCmp;                   // ptr to third array to compare
    PCT_HASH_OBJECT *pHashTbl;    // hash table
    int Ctr, Ctr2;                // loop counters


    //
    //  Allocate the hash table.
    //
    pHashTbl = (PCT_HASH_OBJECT *)malloc(HASH_SIZE * sizeof(PCT_HASH_OBJECT));
    memset( pHashTbl,
            0,
            (HASH_SIZE * sizeof(PCT_HASH_OBJECT)) );

    //
    //  Search through all third level tables.  If there is a duplicate,
    //  fix the affected pointers in the second level tables and free the
    //  duplicate table.
    //
    for (Ctr = 0; Ctr < TABLE_SIZE_8; Ctr++)
    {
        if ((pTbl2 = (P844_ARRAY)(pArr[Ctr])) != NULL)
        {
            //
            //  See if any of the previous third level tables are the
            //  same as the current one.
            //
            for (Ctr2 = 0; Ctr2 < TABLE_SIZE_4; Ctr2++)
            {
                if ((pTbl3 = pTbl2[Ctr2]) != NULL)
                {
                    //
                    //  Compare each entry in the table to see if
                    //  the table is the same as any of the previous
                    //  tables.
                    //
                    pCmp = FindHashTable( pTbl3,
                                          pHashTbl,
                                          Size );

                    if (pCmp != NULL)
                    {
                        //
                        //  Tables are the same.  Fix the pointer
                        //  in the second level table.
                        //
                        pTbl2[Ctr2] = pCmp;

                        //
                        //  Free the duplicate third level table.
                        //
                        free(pTbl3);

                        //
                        //  Decrement the number of second level tables.
                        //
                        (*pBuf3)--;
                    }
                }
            }
        }
    }

    //
    //  Free the hash table.
    //
    FreeHashTable(pHashTbl);
}


////////////////////////////////////////////////////////////////////////////
//
//  FindHashTable
//
//  This routine searches the hash table for the given third level table.
//  If a matching table is found, the pointer to the table is returned.
//  Otherwise, it returns NULL.
//
////////////////////////////////////////////////////////////////////////////

PVOID FindHashTable(
    PVOID pTbl,
    PCT_HASH_OBJECT *pHashTbl,
    int Size)
{
    DWORD HashVal;                // hash value
    PCT_HASH_OBJECT pHashN;       // ptr to hash node
    PCT_HASH_OBJECT pNewHash;     // ptr to new hash node
    int Ctr;                      // loop counter


    //
    //  Get hash value of the given table.
    //
    HashVal = GetHashVal(pTbl, Size);

    //
    //  Search through all hash tables.
    //
    for (pHashN = pHashTbl[HashVal]; pHashN != NULL; pHashN = pHashN->pNext)
    {
        //
        //  See if the two tables are the same.  If they are, return the
        //  pointer to the table.
        //
        for (Ctr = 0; Ctr < TABLE_SIZE_4; Ctr++)
        {
            if (memcmp( ((BYTE *)(pHashN->pTable)) + (Ctr * Size),
                        ((BYTE *)pTbl) + (Ctr * Size),
                        Size ))
            {
                break;
            }
        }
        if (Ctr == TABLE_SIZE_4)
        {
            //
            //  Tables are the same.  Return the pointer to the table.
            //
            return (pHashN->pTable);
        }
    }

    //
    //  Could not find a table that matched the given table.
    //  Create a new hash node and insert it in the hash table.
    //
    pNewHash = (PCT_HASH_OBJECT)malloc(sizeof(CT_HASH_OBJECT));
    pNewHash->pTable = pTbl;
    pNewHash->pNext = pHashTbl[HashVal];
    pHashTbl[HashVal] = pNewHash;

    //
    //  Return NULL to indicate that an identical table could not be found.
    //
    return (NULL);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetHashVal
//
//  This routine calculates the hash value of a table.
//
////////////////////////////////////////////////////////////////////////////

DWORD GetHashVal(
    PVOID pTbl,
    int Size)
{
    DWORD HashVal = 0;            // hash value
    DWORD Multiplier = 1;         // multiplier for each entry
    int Ctr;                      // loop counter


    for (Ctr = 0; Ctr < TABLE_SIZE_4; Ctr++)
    {
        HashVal += ((*(((BYTE *)pTbl) + (Ctr * Size))) * Multiplier);
        Multiplier *= 2;
    }

    return ((DWORD)(HashVal / HASH_SIZE));
}


////////////////////////////////////////////////////////////////////////////
//
//  FreeHashTable
//
//  This routine frees the hash table.
//
////////////////////////////////////////////////////////////////////////////

void FreeHashTable(
    PCT_HASH_OBJECT *pHashTbl)
{
    PCT_HASH_OBJECT pHashN;       // ptr to hash node
    PCT_HASH_OBJECT pNext;        // ptr to next hash node
    int Ctr = 0;                  // loop counter


    //
    //  Search through each entry in the hash table and free each node.
    //
    for (Ctr = 0; Ctr < HASH_SIZE; Ctr++)
    {
        pHashN = pHashTbl[Ctr];
        while (pHashN != NULL)
        {
            pNext = pHashN->pNext;
            free(pHashN);
            pHashN = pNext;
        }
    }

    //
    //  Free the hash table array.
    //
    free(pHashTbl);
}
