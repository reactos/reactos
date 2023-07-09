/********************************************************************
 *
 *  Header Name : ucefile.h
 *
 *  UCE file data structures
 *
 *
 *  History :
 *         Sep 02, 1997  [samera]   wrote it
 *
 *  Copyright (c) 1997-1999 Microsoft Corporation.
 *********************************************************************/

#ifndef __UCEFILE_H__
#define __UCEFILE_H__

#define MAX_UCE_FILES    32

typedef struct
{
  char Signature[4] ;        // should be "UCEX"
  DWORD OffsetTableName;     // Subset name
  WORD Codepage;
  WORD NumGroup;
  WORD Row;
  WORD Column;

} UCE_HEADER, *PUCE_HEADER;


//  UCEX Group Structure
typedef struct
{
  DWORD OffsetGroupName;
  DWORD OffsetGroupChar;
  DWORD NumChar;
  DWORD Reserved;

} UCE_GROUP, *PUCE_GROUP;


typedef struct Structtag_UCE_FILES
{
  HANDLE hFile;     // UCE physical file handle
  HANDLE hMapFile;  // UCE memory map file handle
  PVOID pvData;     // Start of committed address space for the file

} UCE_MEMORY_FILE, *PUCE_MEMORY_FILE;


// export
UINT UCE_EnumFiles( void );
void UCE_CloseFiles( void );
INT UCE_GetFiles( UCE_MEMORY_FILE **ppUceMemFile );
BOOL UCE_GetTableName( PUCE_MEMORY_FILE pUceMemFile , PWSTR *ppszTableName );
WORD UCE_GetCodepage( PUCE_MEMORY_FILE pUceMemFile );

#endif  // __UCEFILE_H__
