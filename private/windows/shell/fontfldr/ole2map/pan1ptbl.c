/*************************************************************************** 
 * PAN1PTBL.C - ElseWare PANOSE(tm) Font Mapper penalty database.
 *
 * $keywords: pan1ptbl.c 1.9 15-Apr-94 3:53:54 PM$
 *
 * This is a stand-alone program that writes a 'C' style header file
 * containing the penalty tables for the font mapper.  The tables are
 * in MEMORY format, e.g., they use Intel or Motorola byte ordering
 * depending on the platform upon which the program is compiled.
 *
 * This program should accompany ELSEPAN.C, the ElseWare PANOSE Mapper
 * source code, which assumes the output from this program is in a file
 * named PAN1PTBL.H.
 *
 * The command-line is as follows:
 *
 *       pan1ptbl [-b] [-v] [-l] [-d] <in-filename> <out-filename>
 *
 * where:
 *
 *       -b means produce a binary file (instead of a 'C' struct)
 *       -v means get from and put to version control
 *       -l means precede the struct with the keyword FAR
 *       -d means dump the database to the screen
 *       <in-filename> is the name of the text file containing the
 *                     penalty database (PAN1PTBL.TXT)
 *       <out-filename> is the name of the output file (PAN1PTBL.H)
 *
 * The '-b' switch causes the MEMORY representation of the struct to be
 * dumped to file (in binary form). The normal behavior (no '-b' switch)
 * is to produce a text file containing a C-style declaration of the
 * struct.
 *
 * The '-v' switch presumes the user has Sorcerer's Apprentice, and causes
 * a 'vout' of pan1ptbl.h followed by a 'vin' of the newly generated file.
 * Notice that Sorcerer's Apprentice ignores the transaction if the file
 * is not different from the previous revision.
 *
 * The '-l' switch causes the database to be declared 'FAR,' which in
 * DOS and Windows large model programs causes the data structure to be
 * placed in its own data segment.
 *
 * The '-d' switch causes a textual description of the database to be
 * dumped to stdout for debugging purposes.
 *
 * Copyright (C) 1991-93 ElseWare Corporation.  All rights reserved.
 ***************************************************************************/
#define ELSE_MAPPER_CORE
#define NOELSEPANDATA
#include "elsepan.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef MACINTOSH
#include "osutils.h"
#endif /* MACINTOSH */


#ifdef MACINTOSH

#   define SZ_DATABASE "DATABASE"
#   define SZ_TABLE    "TABLE"

#   define  PT_strupr(p)        uprstring((p), FALSE)
BOOL PT_strcmpi(char *s, char *t) {
    uprstring(s, FALSE);
    return strcmp(s, t);
}

#else /* WINDOWS */

#   define SZ_DATABASE TEXT("Database")
#   define SZ_TABLE    TEXT("Table")

#   define  PT_strupr(p)        _strupr(p)
#   define  PT_strcmpi(s, t)    _strcmpi(s, t)

#endif


static EW_BOOL bReadDB(FILE *fpin, EW_BOOL bBuildIt);
static EW_BOOL bReadPTbl(FILE *fpin, EW_BOOL bBuildIt,
      EW_BOOL bExpectSquare, char *pLnBuf, int iBufSize,
      EW_PTBL_MEM *pPTbl, EW_BYTE *pDataBuf);
static int iGetCompressKind(EW_BYTE *pDataBuf,
      int iRow, int iCol, int iNoFit);
static EW_BOOL bAddUniqueTable(EW_PTBL_MEM *pPTbl, int iSize);
static int iReadData(FILE *fpin, char *pLnBuf, int iBufSize,
      EW_BYTE *pDataBuf);
static EW_BOOL bGetLine(char *pBuf, int iLen, FILE *fpin);
static void vShowDB(void);
static EW_BOOL bShowDupTbl(EW_PTBL_MEM *pPTbl);

static EW_BYTE s_buf[8192];
static EW_USHORT s_unOffs;
static int s_iLnPos;

/***************************************************************************
 * FUNCTION: main
 *
 * PURPOSE:  Write the file PAN1PTBL.H, which is a 'C'-style .H file
 *           containing the declaration of the memory copy of the
 *           penalty database.
 *
 * RETURNS:  The program returns 0 if it succeeds, 1 if there is a failure.
 ***************************************************************************/
int main (
   int argc,
   char *argv[])
{
   int i;
   int j;
   int iRet = 1;
   EW_BOOL bDumpBin = FALSE;
   EW_BOOL bDoVCS = FALSE;
   EW_BOOL bMakeFar = FALSE;
   EW_BOOL bShowDB = FALSE;
   time_t ltime;
   EW_PDICT_MEM *pHead = (EW_PDICT_MEM *)s_buf;
   FILE *fpin;
   FILE *fpout;
   char *p;
   char *pszInFileNm;
   char *pszOutFileNm;
   char szIncNm[64];
   char szTime[64];

   printf("ElseWare Penalty Table Maker v1.2\n");

   /* Display 'usage' message if the minimum number of parameters
    * was not recieved.
    */
   if( argc < 3 ) {
      printf("Usage: pan1ptbl [-b] [-v] [-l] [-d] <infile> <outfile>\n");
      printf("       -b means dump in binary form (not C-struct)\n");
      printf("       -v means get from and put to version control\n");
      printf("       -l means precede the struct with the keyword FAR\n");
      printf("       -d dump the database to the screen\n");
      printf("       <infile> is the text penalty database (PAN1PTBL.TXT)\n");
      printf("       <outfile> is the output file (PAN1PTBL.H)\n");
      goto backout0;
   }
   /* Set flags.
    */
   for( p = argv[i = 1]; i < (argc - 2 ); p = argv[++i]) {
      if( p[0] != '-' ) {
         printf("Expected command line flag at '%s,' pan1ptbl aborted.\n", p);
         goto backout0;
      }
      switch( p[1] ) {
         case 'b':
         case 'B':
            bDumpBin = TRUE;
            break;
         case 'v':
         case 'V':
            bDoVCS = TRUE;
            break;
         case 'l':
         case 'L':
            bMakeFar = TRUE;
            break;
         case 'd':
         case 'D':
            bShowDB = TRUE;
            break;
         default:
            printf("Unrecognized command line flag '%s' ignored.\n", p);
            break;
      }
   }
   /* Make sure the last few params are not flags (should be
    * file names).
    */
   if( *p == '-' ) {
      printf("Expected input file name at '%s,' pan1ptbl ignored.\n", p);
      goto backout0;
   }
   PT_strupr(pszInFileNm = p);

   p = argv[argc - 1];
   if( *p == '-' ) {
      printf("Expected output file name at '%s,' pan1ptbl ignored.\n", p);
      goto backout0;
   }
   PT_strupr(pszOutFileNm = p);

   /* Extract the file from version control if we're supposed to
    * do that (Sorcerer's Apprentice is assumed).
    */

   if( bDoVCS ) {

#ifdef WINDOWS 
      char cmd[128];
      sprintf(cmd, "vout %s", pszOutFileNm);
      if( system(cmd ) != 0) {
         printf("Failed to get %s from version control, pan1ptbl aborted.\n",
            pszOutFileNm);
         goto backout0;
      }
#endif /* WINDOWS */

#ifdef MACINTOSH
	   printf("VCS mode not valid on the Macintosh.  Continuing anyway...\n");
#endif /* MACINTOSH */
   }

   /* Open the input file.
    */
   if( !(fpin = fopen(pszInFileNm, "rt" ))) {
      printf("Could not open input file %s, pan1ptbl aborted.\n",
         pszInFileNm);
      goto backout0;
   }
   /* Create the output file.
    */
   if( bDumpBin ) {
      fpout = fopen(pszOutFileNm, "wb");
   } else {
      fpout = fopen(pszOutFileNm, "wt");
   }
   if( !fpout ) {
      printf("Could not open output file %s, pan1ptbl aborted.\n",
         pszOutFileNm);
      if( bDoVCS ) {
#ifdef WINDOWS 
         char cmd[128];
         sprintf(cmd, "vadmin u %s", pszOutFileNm);
         if( system(cmd ) != 0) {
            printf("Failed to unlock %s from version control.\n",
               pszOutFileNm);
         }
#endif /* WINDOWS */
      }
      goto backout1;
   }
   /* Build the database.
    */
   s_iLnPos = 0;
   if( !bReadDB(fpin, FALSE )) {
      goto backout2;
   }
   rewind(fpin);
   s_iLnPos = 0;
   if( !bReadDB(fpin, TRUE )) {
      goto backout2;
   }
   printf("%s: %u bytes in database.\n", pszOutFileNm, pHead->unSizeDB);

   if( bShowDB ) {
      vShowDB();
   }
   /* If we're writing binary, then dump it now and exit.
    */
   if( bDumpBin ) {
      fwrite(s_buf, pHead->unSizeDB, 1, fpout);
      goto webedun;
   }

   /* 'C' header write.
    *
    * Write the file containing the 'C' structure that defines
    * the penalty database.
    */
   if( time(&ltime )) {
      strcpy(szTime, asctime(localtime(&ltime)));
      szTime[strlen(szTime) - 1] = '\0';
   } else {
      szTime[0] = '\0';
   }
   strcpy(szIncNm, "__");
   if( (p = strrchr(pszOutFileNm, '\\' )) ||
        ( p = strrchr(pszOutFileNm, ':' ))) {
      strcat(szIncNm, &p[1]);
   } else {
      strcat(szIncNm, pszOutFileNm);
   }
   if( p = strchr(szIncNm, '.' )) {
      *p = '_';
   }
   strcat(szIncNm, "__");
   if( bDoVCS ) {
      fprintf(fpout,
         "/*\044change:Updated by PAN1PTBL.EXE on %s.\044*/\n", szTime);
   }
   fprintf(fpout,
      "/***************************************************************************\n");
   fprintf(fpout,
      " * %s - ElseWare PANOSE(tm) default penalty tables.\n", pszOutFileNm);
   fprintf(fpout,
      " *\n");
   if( bDoVCS ) {
      fprintf(fpout,
         " * $keywords: pan1ptbl.c 1.9 15-Apr-94 3:53:54 PM$\n");
      fprintf(fpout,
         " *\n");
   }
   fprintf(fpout,
      " * This file was generated by PAN1PTBL.EXE.\n");
   fprintf(fpout,
      " *\n");
   fprintf(fpout,
      " * This file contains the penalty tables for the PANOSE 1.0 font\n");
   fprintf(fpout,
      " * mapper. It was generated from the file %s.\n", pszInFileNm);
   fprintf(fpout,
      " *\n");
   fprintf(fpout,
      " * Penalty database structure version %x.%02x.\n",
      PANOSE_PENALTY_VERS / 0x100,
      PANOSE_PENALTY_VERS % 0x100);
   fprintf(fpout,
      " *\n");
   if( !bDoVCS ) {
      fprintf(fpout,
         " * File created %s.\n", szTime);
      fprintf(fpout,
         " *\n");
   }
   fprintf(fpout,
      " * Copyright( C ) 1992-93 ElseWare Corporation.  All rights reserved.\n");
   fprintf(fpout,
      " ***************************************************************************/\n");
   fprintf(fpout,
      "\n");
   fprintf(fpout,
      "#ifndef %s\n", szIncNm);
   fprintf(fpout,
      "#define %s\n", szIncNm);
   fprintf(fpout,
      "\n");
   fprintf(fpout,
      "/***************************************************************************\n");
   fprintf(fpout,
      " * PENALTY DATABASE\n");
   fprintf(fpout,
      " *\n");
   fprintf(fpout,
      " * Below is the default penalty database for the PANOSE font\n");
   fprintf(fpout,
      " * mapper.  It is in the MEMORY format.\n");
   fprintf(fpout,
      " *\n");
   fprintf(fpout,
      " * Look at PAN1PTBL.C to see how this is created.\n");
   fprintf(fpout,
      " ***************************************************************************/\n");
   if( bMakeFar ) {
      fprintf(fpout,
         "EW_BYTE FAR s_panDB[] = {");
   } else {
      fprintf(fpout,
         "EW_BYTE s_panDB[] = {");
   }
   for( i = 1, p = s_buf; i < (int )pHead->unSizeDB; ) {
      fprintf(fpout, "\n  ");
      for( j = 0; (i < (int )pHead->unSizeDB) &&( j < 10 );
            ++i, ++j, ++p) {
         fprintf(fpout, " 0x%02x,",( EW_BYTE )*p);
      }
   }
   if( j == 10 ) {
      fprintf(fpout, "\n  ");
   }
   fprintf(fpout,
      " 0x%02x\n};\n",( EW_BYTE )*p);

   fprintf(fpout,
      "\n");
   fprintf(fpout,
      "#endif /* ifndef %s */\n", szIncNm);

   if( bDoVCS ) {
      fprintf(fpout,
         "\n");
      fprintf(fpout,
         "/***************************************************************************\n");
      fprintf(fpout,
         " * Revision log:\n");
      fprintf(fpout,
         " ***************************************************************************/\n");
      fprintf(fpout,
         "/*\n");
      fprintf(fpout,
         " * \044log\044\n");
      fprintf(fpout,
         " */\n");
   }
webedun:
   fclose(fpout);
   fpout = NULL;

   /* Return the file to version control.
    */
   if( bDoVCS ) {

#ifdef WINDOWS
      char cmd[128];
      sprintf(cmd, "vin -c\"File updated by PAN1PTBL.EXE %s\" %s",
         szTime, pszOutFileNm);
      if( system(cmd ) != 0) {
         printf("Failed to put %s into version control.\n", pszOutFileNm);
      }
#endif /* WINDOWS */
   }
   iRet = 0;
   goto backout1;

backout2:
   fclose(fpout);
backout1:
   fclose(fpin);
backout0:
   return( iRet );
}

/***************************************************************************
 * FUNCTION: bReadDB
 *
 * PURPOSE:  Read the text penalty database. This routine is called twice:
 *           once to gather statistics, and then a second time to fill in
 *           everything else.
 *
 * RETURNS:  The function returns TRUE if parses the whole file without
 *           detecting errors.
 ***************************************************************************/
static EW_BOOL bReadDB(
   FILE *fpin,
   EW_BOOL bBuildIt)
{
   int i;
   int j;
   int w;
   int iNumTbl;
   EW_BOOL bUseAtoB;
   EW_PDICT_MEM *pHead =( EW_PDICT_MEM * )s_buf;
   EW_PIND_MEM *pInd = pHead->pind;
   EW_BYTE *pWt = NULL;
   EW_ATOB_MEM *pAtoB = NULL;
   EW_ATOB_ITEM_MEM *pAtoBItem = NULL;
   EW_PTBL_MEM *pPTbl = NULL;
   char szBuf[256];
   char szKey[128];

   /* First pass: init header. On the second pass it contains
    * the count of dictionaries in the database.
    */
   if( bBuildIt ) {
      s_unOffs = sizeof(EW_PDICT_MEM) +
        ( sizeof(EW_PIND_MEM ) *( pHead->unNumDicts - 1 ));
   } else {
      pHead->unVersion = PANOSE_PENALTY_VERS;
      pHead->unByteOrder = PTBL_BYTE_ORDER;
      pHead->unNumDicts = 0;
      pHead->unSizeDB = 0;
      s_unOffs = sizeof(EW_PDICT_MEM);
   }
   /* Get the first line.
    */
   if( !bGetLine(szBuf, sizeof(szBuf ), fpin)) {
      printf("No data found in the input file, pan1ptbl aborted.\n");
      return( FALSE );
   }
   /* For each database.
    *
    * Look for [ Database ]
    */
   while( (sscanf(szBuf, "[ %s ]", szKey ) == 1) &&
        ( PT_strcmpi(szKey, "Database" ) == 0)) {

      /* Init vars.
       */
      iNumTbl = 0;
      bUseAtoB = FALSE;
      pWt = NULL;
      pAtoB = NULL;
      pAtoBItem = NULL;
      pPTbl = NULL;

      /* If we're really building then set up the vars
       * to recieve the tables.
       */
      if( bBuildIt ) {
         /* The weight array is always exactly 10 bytes long,
          * initialized to all zeroes. The first byte contains
          * the weight for the family digit, which does not
          * have a penalty table associated with it.
          */
         pWt = &s_buf[s_unOffs];
         pInd->unOffsWts = s_unOffs;
         memset(pWt, 0, j =( sizeof(EW_BYTE ) * NUM_PAN_DIGITS));
         ++pWt;
         s_unOffs += j;

         /* For the A-to-B remapping array, the offset
          * variable in the index contains the count of
          * items from the first pass. Use that to set
          * up the array.
          */
         if( pInd->unOffsAtoB > 0 ) {
            pAtoB =( EW_ATOB_MEM * )&s_buf[s_unOffs];
            pAtoBItem = pAtoB->AtoBItem;
            i = pInd->unOffsAtoB;
            pInd->unOffsAtoB = s_unOffs;
            memset((char *)pAtoB, 0, j =( sizeof(EW_ATOB_MEM ) +
              ( sizeof(EW_ATOB_ITEM_MEM ) *( i - 1 ))));
            pAtoB->unNumAtoB = i;
            s_unOffs += j;
         } else {
            i = NUM_PAN_DIGITS - 1;
         }
         /* Set up penalty table pointer.
          */
         pPTbl =( EW_PTBL_MEM * )&s_buf[s_unOffs];
         pInd->unOffsPTbl = s_unOffs;
         memset((char *)pPTbl, 0, j =( sizeof(EW_PTBL_MEM ) * i));
         s_unOffs += j;

      } else {
         pInd->jFamilyA = 0;
         pInd->jFamilyB = 0;
         pInd->jDefAnyPenalty = 0;
         pInd->jDefNoFitPenalty = 10;
         pInd->jDefMatchPenalty = 0;
         pInd->jReserved = 0;
         pInd->unOffsWts = 0;
         pInd->unOffsAtoB = 0;
         pInd->unOffsPTbl = 0;
      }
      /* Look for row = <PANOSE-family-digit-for-row>
       */
      if( !bGetLine(szBuf, sizeof(szBuf ), fpin) ||
           ( sscanf(szBuf, "row = %d", &i ) != 1)) {
         printf("%d: Expected 'row' statement, pan1ptbl aborted.\n",
               s_iLnPos);
         return( FALSE );
      }
      if( (i < PANOSE_NOFIT ) ||( i > MAX_PAN1_FAMILY )) {
         printf("%d: Row PANOSE digit out of range, pan1ptbl aborted.\n",
            s_iLnPos);
         return( FALSE );
      }
      pInd->jFamilyA = i;

      /* Look for col = <PANOSE-family-digit-for-column>
       */
      if( !bGetLine(szBuf, sizeof(szBuf ), fpin) ||
           ( sscanf(szBuf, "col = %d", &i ) != 1)) {
         printf("%d: Expected 'col' statement, pan1ptbl aborted.\n",
               s_iLnPos);
         return( FALSE );
      }
      if( (i < PANOSE_NOFIT ) ||( i > MAX_PAN1_FAMILY )) {
         printf("%d: Column PANOSE digit out of range, pan1ptbl aborted.\n",
               s_iLnPos);
         return( FALSE );
      }
      pInd->jFamilyB = i;

      /* Read in a table heading:
       *
       * [ Table : <ind-row> : <ind-col> : <default-weight> ]
       */
      if( !bGetLine(szBuf, sizeof(szBuf ), fpin)) {
         printf("%d: Expected penalty table after database header, pan1ptbl aborted.\n",
               s_iLnPos);
         return( FALSE );
      }
      /* For each table.
       */
      while ((sscanf(szBuf, "[ %s : %d : %d : %d ]",
            szKey, &i, &j, &w) == 4) &&( PT_strcmpi(szKey, "Table" ) == 0)) {

         /* Make sure indices in range.
          */
         if( (i <= 0 ) ||( j <= 0 ) ||
              ( i >= NUM_PAN_DIGITS ) ||( j >= NUM_PAN_DIGITS )) {
            printf("%d: Digit index out of range, pan1ptbl aborted.\n",
                  s_iLnPos);
            return( FALSE );
         }
         /* Test non-zero weight.
          */
         if( !w && !bBuildIt ) {
            printf("%d: Warning: Weight value of zero.\n", s_iLnPos);
         }
         /* Detect need for A-to-B remapping array. This happens
          * when the row-ind != col-ind or when the inds do not
          * count from 1 to 9.
          */
         if( (i != j ) ||( i != (iNumTbl + 1 ))) {
            bUseAtoB = TRUE;
         }
         /* Attempt to read the table.
          *
          * This routine bumps s_unOffs.
          */
         if (!bReadPTbl(fpin, bBuildIt,
              ( EW_BOOL )((i == j) &&( pInd->jFamilyA == pInd->jFamilyB )),
               szBuf, sizeof(szBuf), pPTbl, &s_buf[s_unOffs])) {
            return( FALSE );
         }
         /* Pick up default weight value and A-to-B settings.
          */
         if( bBuildIt ) {
            *pWt++ = w;
            if( pAtoBItem ) {
               pAtoBItem->jAttrA = i;
               pAtoBItem->jAttrB = j;
               {
                  EW_ATOB_ITEM_MEM *pAtoBCmp = pAtoB->AtoBItem;
                  for( ; pAtoBCmp < pAtoBItem; ++pAtoBCmp ) {
                     if( (pAtoBCmp->jAttrA == pAtoBItem->jAttrA ) &&
                          ( pAtoBCmp->jAttrB == pAtoBItem->jAttrB )) {
                        printf("%u: Row & col pair used in a previous table, pan1ptbl aborted.\n",
                              s_iLnPos);
                        return( FALSE );
                     }
                  }
               }
               ++pAtoBItem;
            }
            ++pPTbl;
         }
         ++iNumTbl;
      }
      /* End of dictionary handling.
       */
      if( !iNumTbl ) {
         printf("%d: Expected penalty table header, pan1ptbl aborted.\n",
               s_iLnPos);
         return( FALSE );
      }
      /* Verify count of tables: there should be exactly 9 without
       * an A-to-B remapping array.
       */
      if( bUseAtoB ) {
         if( bBuildIt ) {
            if( iNumTbl != (int )pAtoB->unNumAtoB) {
               printf("%d: Second-pass: Error in AtoB count.\n", s_iLnPos);
               return( FALSE );
            }
         } else if( iNumTbl > (NUM_PAN_DIGITS - 1 )) {
            printf("%d: Cannot have more that %u tables, pan1ptbl aborted.\n",
                  s_iLnPos,( int )(NUM_PAN_DIGITS - 1));
            return( FALSE );
         } else {
            /* Save the count of tables here for second pass.
             */
            pInd->unOffsAtoB = iNumTbl;
         }
      } else if( iNumTbl != (NUM_PAN_DIGITS - 1 )) {
         printf("%d: Expected exactly %d tables, pan1ptbl aborted.\n",
               s_iLnPos,( int )(NUM_PAN_DIGITS - 1));
         return( FALSE );
      }
      /* Bump count of dictionaries, as we successfully read one.
       */
      if( !bBuildIt ) {
         ++pHead->unNumDicts;
         s_unOffs += sizeof(EW_PIND_MEM);
      }
      ++pInd;
   }
   if( !pHead->unNumDicts ) {
      printf("No penalty dictionaries found, pan1ptbl aborted.\n");
      return( FALSE );
   }
   if( bBuildIt ) {
      pHead->unSizeDB = s_unOffs;
   }
   return( TRUE );
}

/***************************************************************************
 * FUNCTION: bReadPTbl
 *
 * PURPOSE:  Read, validate, and compress a table.
 *
 * RETURNS:  The function returns TRUE if it succesfully processes the
 *           table, FALSE if it does not.
 ***************************************************************************/
static EW_BOOL bReadPTbl(
   FILE *fpin,
   EW_BOOL bBuildIt,
   EW_BOOL bExpectSquare,
   char *pLnBuf,
   int iBufSize,
   EW_PTBL_MEM *pPTbl,
   EW_BYTE *pDataBuf)
{
   int i;
   int j;
   int k;
   int iNoFit;
   int iCol;
   int iRow;

   *pDataBuf = 0;

   /* Read in the data table.
    *
    * First line is the column headings.
    */
   if( !(iCol = iReadData(fpin, pLnBuf, iBufSize, &pDataBuf[1] ))) {
      printf("%d: Expected table data, pan1ptbl aborted.\n", s_iLnPos);
      return( FALSE );
   }
   ++iCol;

   /* Read the remaining rows, watching for the same number of columns.
    */
   for (iRow = 1; (i = iReadData(fpin, pLnBuf, iBufSize,
         &pDataBuf[iRow * iCol]));
         ++iRow) {
      if( i != iCol ) {
         printf("%d: Inconsistent number of columns, pan1ptbl aborted.\n",
               s_iLnPos);
         return( FALSE );
      }
   }
   /* Test for table too small.
    */
   if( (iRow < 5 ) ||( iCol < 5 )) {
      printf("%d: The table is too small, pan1ptbl aborted.\n", s_iLnPos);
      return( FALSE );
   }
   /* Look for a square table.
    */
   if( bExpectSquare && (iRow != iCol )) {
      printf("%d: Expected a square table( #row = #col ), pan1ptbl aborted.\n",
            s_iLnPos);
      return( FALSE );
   }
   /* Test 'any' penalties. They should all equal zero.
    */
   for (i = 1, j = iCol;
        ( i < j ) &&( (i == 2 ) ||( pDataBuf[iCol + i] == 0 ));
         ++i)
      ;
   if( i >= j ) {
      for (i = 1, j = iRow;
           ( i < j ) &&( (i == 2 ) ||( pDataBuf[(iCol * i ) + 1] == 0));
            ++i)
         ;
   }
   if( i < j ) {
      printf("%d: All 'any' penalties should be zero, pan1ptbl aborted.\n",
            s_iLnPos);
      return( FALSE );
   }
   /* Test 'no-fit' penalties. They should all have the same value.
    */
   iNoFit = pDataBuf[iCol + 2];
   for (i = 1, j = iCol;
        ( i < j ) &&( pDataBuf[(iCol * 2 ) + i] ==( EW_BYTE )iNoFit);
         ++i)
      ;
   if( i >= j ) {
      for (i = 1, j = iRow;
           ( i < j ) &&( pDataBuf[(iCol * i ) + 2] ==( EW_BYTE )iNoFit);
            ++i)
         ;
   }
   if( i < j ) {
      printf("%d: All 'no-fit' penalties should match, pan1ptbl aborted.\n",
            s_iLnPos);
      return( FALSE );
   }
   /* If we're just scanning, then we're done collecting info.
    */
   if( !bBuildIt || !pPTbl ) {
      return( TRUE );
   }
   /* Init table.
    */
   pPTbl->jRangeLast =( (iRow > iCol ) ? iRow : iCol) - 2;
   pPTbl->unOffsTbl = 0;
   pPTbl->unTblSize = 0;
   k = 0;

   /* Determine the compression type, and then copy the table
    * based upon the type.
    */
   switch (pPTbl->jCompress =
        ( EW_BYTE )iGetCompressKind(pDataBuf, iRow, iCol, iNoFit)) {

      case PAN_COMPRESS_C0:
         /* C0 compression: suck in the whole table except
          * the any and no-fit values. This table is preceded
          * by a header containing the row and column dimensions.
          */
         {
            EW_PTBL_C0_MEM *pPC0 =( EW_PTBL_C0_MEM * )&pDataBuf[k];
            pPC0->jARangeLast =( EW_BYTE )iRow - 2;
            pPC0->jBRangeLast =( EW_BYTE )iCol - 2;
            pPC0->jReserved = 0;
            k +=( sizeof(EW_PTBL_C0_MEM ) - sizeof(EW_BYTE));
         }
         for( i = 3; i < iRow; ++i ) {
            for( j = 3; j < iCol; ++j, ++k ) {
               pDataBuf[k] = pDataBuf[(i * iCol) + j];
            }
         }
         bAddUniqueTable(pPTbl, k);
         break;

      case PAN_COMPRESS_C1:
         /* C1 compression: perfectly symmetrical table with
          * penalties increasing the further they are away
          * from the diagonal. No additional data stored.
          */
         break;

      case PAN_COMPRESS_C3:
         /* C3 compression: same as C2 compression except the
          * first byte is the no-fit penalty.
          */
         pDataBuf[k++] = iNoFit;

      case PAN_COMPRESS_C2:
         /* C2 compression: symmetrical about the diagonal,
          * suck in the lower left corner.
          */
         for( i = 4; i < iRow; ++i ) {
            for( j = 3; j < i; ++j, ++k ) {
               pDataBuf[k] = pDataBuf[(i * iCol) + j];
            }
         }
         bAddUniqueTable(pPTbl, k);
         break;

      case PAN_COMPRESS_C4:
         /* C4 compression: similar to C1, except the start
          * and increment values are specified.
          */
         {
            EW_PTBL_C4_MEM *pPC4 =( EW_PTBL_C4_MEM * )&pDataBuf[k];
            pPC4->jStart = pDataBuf[(iCol * 4) + 3];
            pPC4->jIncrement = pDataBuf[(iCol * 5) + 3] - pPC4->jStart;
            k += sizeof(EW_PTBL_C4_MEM);
         }
         bAddUniqueTable(pPTbl, k);
         break;
   }
   return( TRUE );
}

/***************************************************************************
 * FUNCTION: iGetCompressKind
 *
 * PURPOSE:  Examine the data table and determine what kind, if any, of
 *           data compression can be used.
 *
 *           Notice the tables have an extra row and column for the
 *           headings. The minimum table size should be 5 x 5, which
 *           is really a 4 x 4 table (the parse loop catches anything
 *           smaller and aborts).
 *
 * RETURNS:  The function returns the compression id.
 ***************************************************************************/
static int iGetCompressKind(
   EW_BYTE *pDataBuf,
   int iRow,
   int iCol,
   int iNoFit)
{
   int i;
   int j;

   /* All compression mechanisms require a square table.
    */
   if( (iRow != iCol ) ||( iCol < 5 )) {
      return( PAN_COMPRESS_C0 );
   }
   /* Test for symmetry around the diagonal.
    */
   for( i = 3; i < iCol; ++i ) {
      /* The diagonal( exact match value ) must be zero.
       */
      if( pDataBuf[(iCol * i ) + i] != 0) {
         return( PAN_COMPRESS_C0 );
      }
      /* Test for value at( i,j ) ==( j,i ).
       */
      for( j = i + 1; (j < iCol ); ++j) {
         if( pDataBuf[(iCol * i ) + j] != pDataBuf[(iCol * j) + i]) {
            return( PAN_COMPRESS_C0 );
         }
      }
   }
   /* The table is symmetrical, now walk it looking for a special
    * pattern: it starts at a given value and increments by a given
    * value. If we find this, then we just store the start and
    * increment values.
    *
    * The table must be atleast 4 x 4 to try this test. Note if
    * it is exactly 4 x 4 and the one value in the table is a 1,
    * then C1 is the most compact way to store it (the 'else'
    * statement below checks for that). Otherwise C2 or C3 is
    * the best.
    */
   if( (iCol > 5 ) &&( iNoFit == 10 )) {
      EW_BOOL bFits = TRUE;
      EW_BYTE jStart = pDataBuf[(iCol * 4) + 3];
      EW_BYTE jInc = pDataBuf[(iCol * 5) + 3] - jStart;

      for( i = 5; (i < iCol ) && bFits; ++i) {
         for( j = 3; (j < i ) && bFits; ++j) {
            bFits =( pDataBuf[(iCol * i ) + j] ==
              ( EW_BYTE )(((EW_BYTE)(i - j - 1) * jInc) + jStart));
         }
      }
      /* C1 compression implies( start, inc ) ==( 1, 1 ),
       * C4 stores the start and inc values.
       */
      if( bFits ) {
         return( ((jStart == 1 ) &&( jInc == 1 )) ?
               PAN_COMPRESS_C1 : PAN_COMPRESS_C4);
      }
   } else if( (pDataBuf[(iCol * 4 ) + 3] == 1) &&( iNoFit == 10 )) {
      return( PAN_COMPRESS_C1 );
   }
   /* The table is symmetrical about the diagonal, but there
    * is no recognized pattern within it, so return C2 or C3
    * compression (C3 is C2 compression with a specifier for
    * the 'no-fit' value).
    */
   return( (iNoFit == 10 ) ? PAN_COMPRESS_C2 : PAN_COMPRESS_C3);
}

/***************************************************************************
 * FUNCTION: bAddUniqueTable
 *
 * PURPOSE:  Add a unique table to the accumulated penalty database, if
 *           a table matching this table's data already exists, then re-use
 *           that data.
 *
 *           Upon entry to this routine, s_unOffs should point to the
 *           data that is about to be added. This func walks all the
 *           tables that have already been added looking for duplicate
 *           data.
 *
 *           This function should be called when we're building the
 *           database( second pass ).
 *
 * RETURNS:  The function returns TRUE if a new table was added, or FALSE
 *           if an existing table was re-used.
 ***************************************************************************/
static EW_BOOL bAddUniqueTable(
   EW_PTBL_MEM *pPTbl,
   int iSize)
{
   int i;
   int j;
   int iNumTbl;
   EW_PDICT_MEM *pHead =( EW_PDICT_MEM * )s_buf;
   EW_PIND_MEM *pInd = pHead->pind;
   EW_PTBL_MEM *pPTblTst;

   /* Shortcut for C1 compression: it has no data.
    */
   if( !iSize ) {
      return( FALSE );
   }
   /* This walk is kind of tricky: we are walking a penalty
    * database that is in the process of being created. Some
    * fields are valid, some are not.
    *
    * For each dictionary.
    */
   for( i = 0; i < (int )pHead->unNumDicts; ++i, ++pInd) {

      /* If the ptbl offset is NULL, then we've reached the
       * end of the filled-in dictionaries.
       */
      if( !pInd->unOffsPTbl ) {
         break;
      }
      /* Get the count of tables.
       */
      if( pInd->unOffsAtoB ) {
         iNumTbl =( int )((EW_ATOB_MEM *)&s_buf[pInd->unOffsAtoB])->unNumAtoB;
      } else {
         iNumTbl = NUM_PAN_DIGITS - 1;
      }
      /* For each penalty table.
       */
      for( j = 0, pPTblTst = (EW_PTBL_MEM * )&s_buf[pInd->unOffsPTbl];
            j < iNumTbl;
            ++j, ++pPTblTst) {

         /* The last filled-in table should be the one that
          * was passed in.
          */
         if( (pPTblTst >= pPTbl ) || !pPTblTst->jRangeLast) {
            break;
         }
         /* Look for matching data, and return if it is found.
          */
         if( (pPTblTst->unOffsTbl > 0 ) &&
              ( pPTbl->jCompress == pPTblTst->jCompress ) &&
              ( iSize == (int )pPTblTst->unTblSize) &&
               (memcmp(&s_buf[s_unOffs],
               &s_buf[pPTblTst->unOffsTbl], iSize) == 0)) {
            pPTbl->unTblSize = iSize;
            pPTbl->unOffsTbl = pPTblTst->unOffsTbl;
            return( FALSE );
         }
      }
   }
   /* No duplicate was found, keep the structure that is currently
    * in the buffer.
    */
   pPTbl->unOffsTbl = s_unOffs;
   s_unOffs += pPTbl->unTblSize = iSize;

   return( TRUE );
}

/***************************************************************************
 * FUNCTION: iReadData
 *
 * PURPOSE:  Read an array of numbers from file.
 *
 * RETURNS:  The function returns the count of numbers it read from the
 *           line.
 ***************************************************************************/
static int iReadData(
   FILE *fpin,
   char *pLnBuf,
   int iBufSize,
   EW_BYTE *pDataBuf)
{
   int i;
   int j = 0;

   /* Get the test line.
    */
   if( !bGetLine(pLnBuf, iBufSize, fpin )) {
      return( 0 );
   }
   /* For each number.
    */
   while( TRUE ) {

      /* Walk white space.
       */
      for( ; *pLnBuf && ((*pLnBuf == ' ' ) ||( *pLnBuf == '\t' )); ++pLnBuf)
         ;
      /* Stop if not at a number.
       */
      if( !*pLnBuf || (*pLnBuf < '0' ) ||( *pLnBuf > '9' )) {
         break;
      }
      /* Read the number.
       */
      for( i = 0; *pLnBuf && (*pLnBuf >= '0' ) &&( *pLnBuf <= '9' );
            ++pLnBuf) {
         i =( i * 10 ) +( *pLnBuf - '0' );
      }
      *pDataBuf++ = i;
      ++j;
   }
   /* Sanity check. At this point the buffer should be empty,
    * the end of the line, or the start of a comment.
    */
   if( *pLnBuf && (*pLnBuf >= ' ' ) &&( *pLnBuf != ';' )) {
      return( 0 );
   }
   return( j );
}

/***************************************************************************
 * FUNCTION: bGetLine
 *
 * PURPOSE:  Read a line from file. Skip blank and comment lines.
 *
 * RETURNS:  The function returns TRUE if it finds a line, FALSE if it
 *           does not.
 ***************************************************************************/
static EW_BOOL bGetLine(
   char *pBuf,
   int iLen,
   FILE *fpin)
{
   int i;
   char ch;
   char *p;

#define M_EOL(ch)( ((ch ) == '\r') ||( (ch ) == '\n'))

   do {
      /* First read through end of line characters.
       */
      while( ((ch = fgetc(fpin )) != EOF) && M_EOL(ch)) {
         if( ch == '\n' ) {
            ++s_iLnPos;
         }
      }
      if( ch == EOF ) {
         return( FALSE );
      }
      /* Read the line.
       */
      for (i = 1, p = pBuf, *p++ = ch;
           ( i < iLen ) &&( (*p = fgetc(fpin )) != EOF) && !M_EOL(*p);
            ++i, ++p)
         ;
      *p = '\0';

      /* Bump the line counter.
       */
      ++s_iLnPos;

      /* Skip this line if it begins with a comment character.
       */
   } while( pBuf[0] == ';' );

   return( TRUE );
}

/***************************************************************************
 * FUNCTION: vShowDB
 *
 * PURPOSE:  Dump the database to stdout.
 *
 * RETURNS:  Nothing.
 ***************************************************************************/
static void vShowDB()
{
   int i;
   int j;
   int k;
   int m;
   int n;
   int iNumPTbl;
   EW_PDICT_MEM *pHead =( EW_PDICT_MEM * )s_buf;
   EW_PIND_MEM *pInd = pHead->pind;
   EW_BYTE *pWt;
   EW_ATOB_MEM *pAtoB;
   EW_ATOB_ITEM_MEM *pAtoBItem;
   EW_PTBL_MEM *pPTbl;
   EW_BYTE *pData;

   printf("\n\nDATABASE DUMP\n");
   printf("~~~~~~~~~~~~~\n");
   printf("vers      = 0x%04x\n", pHead->unVersion);
   printf("num dicts = %u\n", pHead->unNumDicts);
   printf("size db   = %u\n", pHead->unSizeDB);

   for( i = 0; i < (int )pHead->unNumDicts; ++i, ++pInd) {
      printf("\n=======================================================\n");
      printf("0x%04x:Index entry:\n",
        ( (unsigned )(char *)pInd -( unsigned )(char *)pHead));
      printf("family row = %u\n",( EW_USHORT )pInd->jFamilyA);
      printf("family col = %u\n",( EW_USHORT )pInd->jFamilyB);
      printf("any        = %u\n",( EW_USHORT )pInd->jDefAnyPenalty);
      printf("no-fit     = %u\n",( EW_USHORT )pInd->jDefNoFitPenalty);
      printf("match      = %u\n",( EW_USHORT )pInd->jDefMatchPenalty);
      printf("reserved   = %u\n",( EW_USHORT )pInd->jReserved);
      printf("offs wt    = 0x%04x\n", pInd->unOffsWts);
      printf("offs atob  = 0x%04x\n", pInd->unOffsAtoB);
      printf("offs ptbl  = 0x%04x\n", pInd->unOffsPTbl);

      if( pInd->unOffsWts ) {
         pWt =( EW_BYTE * )&s_buf[pInd->unOffsWts];
         printf("\n0x%04x:weights:", pInd->unOffsWts);
         for( j = 0; j < NUM_PAN_DIGITS; ++j, ++pWt ) {
            printf(" %u",( EW_USHORT )*pWt);
         }
         printf("\n");
      } else {
         printf("\nERROR: NO WEIGHTS\n");
      }

      if( pInd->unOffsAtoB ) {
         pAtoB =( EW_ATOB_MEM * )&s_buf[pInd->unOffsAtoB];
         pAtoBItem = pAtoB->AtoBItem;
         printf("\n0x%04x:AtoB:\n", pInd->unOffsAtoB);
         printf("count = %u\n", pAtoB->unNumAtoB);
         for( j = 0; j < (int )pAtoB->unNumAtoB; ++j, ++pAtoBItem) {
            printf("pair  = %u, %u\n",( EW_USHORT )pAtoBItem->jAttrA,
              ( EW_USHORT )pAtoBItem->jAttrB);
         }
         iNumPTbl = pAtoB->unNumAtoB;
      } else {
         iNumPTbl = NUM_PAN_DIGITS - 1;
      }

      if( pInd->unOffsPTbl && (iNumPTbl > 0 )) {
         pPTbl =( EW_PTBL_MEM * )&s_buf[pInd->unOffsPTbl];

         for( j = 0; j < iNumPTbl; ++j, ++pPTbl ) {
            printf("\n0x%04x:penalty table:\n",
              ( (unsigned )(char *)pPTbl -( unsigned )(char *)pHead));
            printf("rng last = %u\n",( EW_USHORT )pPTbl->jRangeLast);
            printf("compress = %u\n",( EW_USHORT )pPTbl->jCompress);
            printf("offs tbl = 0x%04x\n", pPTbl->unOffsTbl);
            printf("tbl size = %u\n", pPTbl->unTblSize);

            if( pPTbl->unOffsTbl && !bShowDupTbl(pPTbl )) {
               k = 0;
               pData =( EW_BYTE * )&s_buf[pPTbl->unOffsTbl];

               switch( pPTbl->jCompress ) {

                  case PAN_COMPRESS_C0:
                     {
                        EW_PTBL_C0_MEM *pPC0 =( EW_PTBL_C0_MEM * )&pData[k];
                        pData += k =( sizeof(EW_PTBL_C0_MEM ) - sizeof(EW_BYTE));
                        printf("Non-symmetrical table, row = %u, col = %u, reserved = %u\n",
                          ( EW_USHORT )pPC0->jARangeLast,
                          ( EW_USHORT )pPC0->jBRangeLast,
                          ( EW_USHORT )pPC0->jReserved);
                        printf("  ");
                        for( m = 0; m <= (int )pPC0->jBRangeLast; ++m) {
                           printf(" %2u", m);
                        }
                        printf("\n");
                        for( m = 0; m <= (int )pPC0->jARangeLast; ++m) {
                           printf("%2u", m);
                           if( m > 1 ) {
                              printf("      ");
                              for( n = 2; (n <= (int )pPC0->jBRangeLast) &&
                                   ( k < (int )pPTbl->unTblSize);
                                    ++n, ++k, ++pData) {
                                 printf(" %2u",( EW_USHORT )*pData);
                              }
                           }
                           printf("\n");
                        }
                     }
                     break;

                  case PAN_COMPRESS_C1:
                     printf("ERROR: TABLE SHOULD BE EMPTY\n");
                     break;

                  case PAN_COMPRESS_C3:
                     printf("no-fit value = %u\n",( EW_USHORT )*pData);
                     ++pData, ++k;

                  case PAN_COMPRESS_C2:
                     printf("  ");
                     for( m = 0; m <= (int )pPTbl->jRangeLast; ++m) {
                        printf(" %2u", m);
                     }
                     printf("\n");
                     for( m = 0; m <= (int )pPTbl->jRangeLast; ++m) {
                        printf("%2u", m);
                        if( m > 2 ) {
                           printf("      ");
                           for( n = 2; (n < m ) &&( k < (int )pPTbl->unTblSize);
                                 ++n, ++k, ++pData) {
                              printf(" %2u",( EW_USHORT )*pData);
                           }
                        }
                        printf("\n");
                     }
                     break;

                  case PAN_COMPRESS_C4:
                     {
                        EW_PTBL_C4_MEM *pPC4 =( EW_PTBL_C4_MEM * )&pData[k];
                        pData += k = sizeof(EW_PTBL_C4_MEM);
                        printf("start = %u, increment = %u\n",
                          ( EW_USHORT )pPC4->jStart,
                          ( EW_USHORT )pPC4->jIncrement);
                     }
                     break;

                  default:
                     printf("ERROR: INVALID COMPRESSION TYPE\n");
                     break;
               }
            } else if (!pPTbl->unOffsTbl &&
                 ( pPTbl->jCompress != PAN_COMPRESS_C1 )) {
               printf("ERROR: NO DATA\n");
            }
         }
      } else {
         printf("\nERROR: NO PENALTIES\n");
      }
   }
   printf("\n=======================================================\n");
   printf("Raw data dump:\n");
   pData =( EW_BYTE * )s_buf;
   printf("        00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\n");
   for( k = 0; k < (int )pHead->unSizeDB; ) {
      printf("0x%04x:", k);
      for( i = 0; (i < 16 ) &&( k < (int )pHead->unSizeDB);
            ++i, ++k, ++pData) {
         printf(" %02x",( EW_USHORT )*pData);
      }
      printf("\n");
   }
}

/***************************************************************************
 * FUNCTION: bShowDupTbl
 *
 * PURPOSE:  Search for a duplicate table and don't show it twice, instead
 *           indicate it is a duplicate table.
 *
 * RETURNS:  The function returns TRUE if this is, indeed, a duplicate
 *           table, or FALSE if it is not.
 ***************************************************************************/
static EW_BOOL bShowDupTbl(
   EW_PTBL_MEM *pPTbl)
{
   int i;
   int j;
   int iNumTbl;
   EW_PDICT_MEM *pHead =( EW_PDICT_MEM * )s_buf;
   EW_PIND_MEM *pInd = pHead->pind;
   EW_PTBL_MEM *pPTblTst;

   /* Quick test for C1 compression.
    */
   if( !pPTbl->unTblSize || !pPTbl->unOffsTbl ) {
      return( FALSE );
   }
   /* For each dictionary.
    */
   for( i = 0; i < (int )pHead->unNumDicts; ++i, ++pInd) {

      /* Get the count of tables.
       */
      if( pInd->unOffsAtoB ) {
         iNumTbl =( int )((EW_ATOB_MEM *)&s_buf[pInd->unOffsAtoB])->unNumAtoB;
      } else {
         iNumTbl = NUM_PAN_DIGITS - 1;
      }
      /* For each penalty table.
       */
      for( j = 0, pPTblTst = (EW_PTBL_MEM * )&s_buf[pInd->unOffsPTbl];
            j < iNumTbl;
            ++j, ++pPTblTst) {

         /* Walk up to the current table.
          */
         if( pPTblTst >= pPTbl ) {
            break;
         }
         /* Look for matching data, and return if it is found.
          */
         if( (pPTblTst->unOffsTbl > 0 ) &&
              ( pPTbl->jRangeLast == pPTblTst->jRangeLast ) &&
              ( pPTbl->jCompress == pPTblTst->jCompress ) &&
              ( pPTbl->unTblSize == pPTblTst->unTblSize ) &&
              ( pPTbl->unOffsTbl == pPTblTst->unOffsTbl )) {
            printf("The data has already dumped( duplicate table ).\n");
            return( TRUE );
         }
      }
   }
   /* No duplicate found.
    */
   return( FALSE );
}

/***************************************************************************
 * Revision log:
 ***************************************************************************/
/*
 * $lgb$
 * 1.0    31-Jan-93    msd PANOSE 1.0 penalties database, textual version.
 * 1.1    31-Jan-93    msd Modified the way a file is written if we know we're checking it in and out of vcs.
 * 1.2     1-Feb-93    msd Fixed a bug with the vcs handling stuff.
 * 1.3     3-Feb-93    msd Removed ctrl-z at EOF. Ifdef'd in Mac code to get this to build as an MPW tool. Ifdef'd out system calls on the Mac.
 * 1.4     3-Feb-93    msd Added generic line-read routine that can handle both mac- and pc-format test files.
 * 1.5     6-Feb-93    msd Init reserved bye in C0 penalty header.
 * 1.6    18-Feb-93    msd Implemented binary file writing, init penalty table byte-ordering variable, and C4 ptbl compression( new version of ptbl ). Identical tables are not repeated.
 * 1.7    26-Feb-93    msd Don't abort on weight values of zero( just warn ). Also repaired line counter.
 * 1.8     1-Apr-93    msd Added _cdecl keyword to main().
 * 1.9    15-Apr-94 jasons Removed cdecl on main for Mac.
 * $lge$
 */
