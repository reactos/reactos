/*
 * File Compression Interface
 *
 * Copyright 2002 Patrik Stridvall
 * Copyright 2005 Gerold Jens Wucherpfennig
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*

There is still some work to be done:

- no real compression yet
- unknown behaviour if files>=2GB or cabinet >=4GB
- check if the maximum size for a cabinet is too small to store any data
- call pfnfcignc on exactly the same position as MS FCIAddFile in every case
- probably check err

*/



#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winternl.h"
#include "fci.h"
#include "cabinet.h"
#include "wine/debug.h"


#ifdef WORDS_BIGENDIAN
#define fci_endian_ulong(x) RtlUlongByteSwap(x)
#define fci_endian_uword(x) RtlUshortByteSwap(x)
#else
#define fci_endian_ulong(x) (x)
#define fci_endian_uword(x) (x)
#endif


#define fci_set_error(A,B,C) do {      \
    p_fci_internal->perf->erfOper = A; \
    p_fci_internal->perf->erfType = B; \
    p_fci_internal->perf->fError =  C; \
    if (B) SetLastError(B); } while(0)


typedef struct {
  cab_UBYTE signature[4]; /* !CAB for unfinished cabinets else MSCF */
  cab_ULONG reserved1;
  cab_ULONG cbCabinet;    /*  size of the cabinet file in bytes*/
  cab_ULONG reserved2;
  cab_ULONG coffFiles;    /* offset to first CFFILE section */
  cab_ULONG reserved3;
  cab_UBYTE versionMinor; /* 3 */
  cab_UBYTE versionMajor; /* 1 */
  cab_UWORD cFolders;     /* number of CFFOLDER entries in the cabinet*/
  cab_UWORD cFiles;       /* number of CFFILE entries in the cabinet*/
  cab_UWORD flags;        /* 1=prev cab, 2=next cabinet, 4=reserved setions*/
  cab_UWORD setID;        /* identification number of all cabinets in a set*/
  cab_UWORD iCabinet;     /* number of the cabinet in a set */
  /* additional area if "flags" were set*/
} CFHEADER; /* minimum 36 bytes */

typedef struct {
  cab_ULONG coffCabStart; /* offset to the folder's first CFDATA section */
  cab_UWORD cCFData;      /* number of this folder's CFDATA sections */
  cab_UWORD typeCompress; /* compression type of data in CFDATA section*/
  /* additional area if reserve flag was set */
} CFFOLDER; /* minumum 8 bytes */

typedef struct {
  cab_ULONG cbFile;          /* size of the uncompressed file in bytes */
  cab_ULONG uoffFolderStart; /* offset of the uncompressed file in the folder */
  cab_UWORD iFolder;         /* number of folder in the cabinet 0=first  */
                             /* for special values see below this structure*/
  cab_UWORD date;            /* last modification date*/
  cab_UWORD time;            /* last modification time*/
  cab_UWORD attribs;         /* DOS fat attributes and UTF indicator */
  /* ... and a C string with the name of the file */
} CFFILE; /* 16 bytes + name of file */


typedef struct {
  cab_ULONG csum;          /* checksum of this entry*/
  cab_UWORD cbData;        /* number of compressed bytes  */
  cab_UWORD cbUncomp;      /* number of bytes when data is uncompressed */
  /* optional reserved area */
  /* compressed data */
} CFDATA;


/***********************************************************************
 *		FCICreate (CABINET.10)
 *
 * FCICreate is provided with several callbacks and
 * returns a handle which can be used to create cabinet files.
 *
 * PARAMS
 *   perf       [IO]  A pointer to an ERF structure.  When FCICreate
 *                    returns an error condition, error information may
 *                    be found here as well as from GetLastError.
 *   pfnfiledest [I]  A pointer to a function which is called when a file
 *                    is placed. Only useful for subsequent cabinet files.
 *   pfnalloc    [I]  A pointer to a function which allocates ram.  Uses
 *                    the same interface as malloc.
 *   pfnfree     [I]  A pointer to a function which frees ram.  Uses the
 *                    same interface as free.
 *   pfnopen     [I]  A pointer to a function which opens a file.  Uses
 *                    the same interface as _open.
 *   pfnread     [I]  A pointer to a function which reads from a file into
 *                    a caller-provided buffer.  Uses the same interface
 *                    as _read.
 *   pfnwrite    [I]  A pointer to a function which writes to a file from
 *                    a caller-provided buffer.  Uses the same interface
 *                    as _write.
 *   pfnclose    [I]  A pointer to a function which closes a file handle.
 *                    Uses the same interface as _close.
 *   pfnseek     [I]  A pointer to a function which seeks in a file.
 *                    Uses the same interface as _lseek.
 *   pfndelete   [I]  A pointer to a function which deletes a file.
 *   pfnfcigtf   [I]  A pointer to a function which gets the name of a
 *                    temporary file.
 *   pccab       [I]  A pointer to an initialized CCAB structure.
 *   pv          [I]  A pointer to an application-defined notification
 *                    function which will be passed to other FCI functions
 *                    as a parameter.
 *
 * RETURNS
 *   On success, returns an FCI handle of type HFCI.
 *   On failure, the NULL file handle is returned. Error
 *   info can be retrieved from perf.
 *
 * INCLUDES
 *   fci.h
 *
 */
HFCI __cdecl FCICreate(
	PERF perf,
	PFNFCIFILEPLACED   pfnfiledest,
	PFNFCIALLOC        pfnalloc,
	PFNFCIFREE         pfnfree,
	PFNFCIOPEN         pfnopen,
	PFNFCIREAD         pfnread,
	PFNFCIWRITE        pfnwrite,
	PFNFCICLOSE        pfnclose,
	PFNFCISEEK         pfnseek,
	PFNFCIDELETE       pfndelete,
	PFNFCIGETTEMPFILE  pfnfcigtf,
	PCCAB              pccab,
	void *pv)
{
  HFCI hfci;
  int err;
  PFCI_Int p_fci_internal;

  if (!perf) {
    SetLastError(ERROR_BAD_ARGUMENTS);
    return NULL;
  }
  if ((!pfnalloc) || (!pfnfree) || (!pfnopen) || (!pfnread) ||
      (!pfnwrite) || (!pfnclose) || (!pfnseek) || (!pfndelete) ||
      (!pfnfcigtf) || (!pccab)) {
    perf->erfOper = FCIERR_NONE;
    perf->erfType = ERROR_BAD_ARGUMENTS;
    perf->fError = TRUE;

    SetLastError(ERROR_BAD_ARGUMENTS);
    return NULL;
  }

  if (!((hfci = ((HFCI) (*pfnalloc)(sizeof(FCI_Int)))))) {
    perf->erfOper = FCIERR_ALLOC_FAIL;
    perf->erfType = ERROR_NOT_ENOUGH_MEMORY;
    perf->fError = TRUE;

    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return NULL;
  }

  p_fci_internal=((PFCI_Int)(hfci));
  p_fci_internal->FCI_Intmagic = FCI_INT_MAGIC;
  p_fci_internal->perf = perf;
  p_fci_internal->pfnfiledest = pfnfiledest;
  p_fci_internal->pfnalloc = pfnalloc;
  p_fci_internal->pfnfree = pfnfree;
  p_fci_internal->pfnopen = pfnopen;
  p_fci_internal->pfnread = pfnread;
  p_fci_internal->pfnwrite = pfnwrite;
  p_fci_internal->pfnclose = pfnclose;
  p_fci_internal->pfnseek = pfnseek;
  p_fci_internal->pfndelete = pfndelete;
  p_fci_internal->pfnfcigtf = pfnfcigtf;
  p_fci_internal->pccab = pccab;
  p_fci_internal->fPrevCab = FALSE;
  p_fci_internal->fNextCab = FALSE;
  p_fci_internal->fSplitFolder = FALSE;
  p_fci_internal->fGetNextCabInVain = FALSE;
  p_fci_internal->pv = pv;
  p_fci_internal->data_in  = NULL;
  p_fci_internal->cdata_in = 0;
  p_fci_internal->data_out = NULL;
  p_fci_internal->cCompressedBytesInFolder = 0;
  p_fci_internal->cFolders = 0;
  p_fci_internal->cFiles = 0;
  p_fci_internal->cDataBlocks = 0;
  p_fci_internal->sizeFileCFDATA1 = 0;
  p_fci_internal->sizeFileCFFILE1 = 0;
  p_fci_internal->sizeFileCFDATA2 = 0;
  p_fci_internal->sizeFileCFFILE2 = 0;
  p_fci_internal->sizeFileCFFOLDER = 0;
  p_fci_internal->sizeFileCFFOLDER = 0;
  p_fci_internal->fNewPrevious = FALSE;
  p_fci_internal->estimatedCabinetSize = 0;
  p_fci_internal->statusFolderTotal = 0;

  memcpy(p_fci_internal->szPrevCab, pccab->szCab, CB_MAX_CABINET_NAME);
  memcpy(p_fci_internal->szPrevDisk, pccab->szDisk, CB_MAX_DISK_NAME);

  /* CFDATA */
  if( !PFCI_GETTEMPFILE(hfci,p_fci_internal->szFileNameCFDATA1,
      CB_MAX_FILENAME)) {
    fci_set_error( FCIERR_NONE, ERROR_FUNCTION_FAILED, TRUE );
    return FALSE;
  }
  /* safety */
  if ( strlen(p_fci_internal->szFileNameCFDATA1) >= CB_MAX_FILENAME ) {
    fci_set_error( FCIERR_NONE, ERROR_INVALID_DATA, TRUE );
    return FALSE;
  }

  p_fci_internal->handleCFDATA1 = PFCI_OPEN(hfci,
    p_fci_internal->szFileNameCFDATA1, 34050, 384, &err, pv);
  if(p_fci_internal->handleCFDATA1==0){
    fci_set_error( FCIERR_TEMP_FILE, ERROR_OPEN_FAILED, TRUE );
    return FALSE;
  }
  /* TODO error checking of err */

  /* array of all CFFILE in a folder */
  if( !PFCI_GETTEMPFILE(hfci,p_fci_internal->szFileNameCFFILE1,
      CB_MAX_FILENAME)) {
    fci_set_error( FCIERR_NONE, ERROR_FUNCTION_FAILED, TRUE );
    return FALSE;
  }
  /* safety */
  if ( strlen(p_fci_internal->szFileNameCFFILE1) >= CB_MAX_FILENAME ) {
    fci_set_error( FCIERR_NONE, ERROR_INVALID_DATA, TRUE );
    return FALSE;
  }
  p_fci_internal->handleCFFILE1 = PFCI_OPEN(hfci,
    p_fci_internal->szFileNameCFFILE1, 34050, 384, &err, pv);
  if(p_fci_internal->handleCFFILE1==0){
    fci_set_error( FCIERR_TEMP_FILE, ERROR_OPEN_FAILED, TRUE );
    return FALSE;
  }
  /* TODO error checking of err */

  /* CFDATA with checksum and ready to be copied into cabinet */
  if( !PFCI_GETTEMPFILE(hfci,p_fci_internal->szFileNameCFDATA2,
      CB_MAX_FILENAME)) {
    fci_set_error( FCIERR_NONE, ERROR_FUNCTION_FAILED, TRUE);
    return FALSE;
  }
  /* safety */
  if ( strlen(p_fci_internal->szFileNameCFDATA2) >= CB_MAX_FILENAME ) {
    fci_set_error( FCIERR_NONE, ERROR_INVALID_DATA, TRUE );
    return FALSE;
  }
  p_fci_internal->handleCFDATA2 = PFCI_OPEN(hfci,
    p_fci_internal->szFileNameCFDATA2, 34050, 384, &err, pv);
  if(p_fci_internal->handleCFDATA2==0){
    fci_set_error( FCIERR_TEMP_FILE, ERROR_OPEN_FAILED, TRUE );
    return FALSE;
  }
  /* TODO error checking of err */

  /* array of all CFFILE in a folder, ready to be copied into cabinet */
  if( !PFCI_GETTEMPFILE(hfci,p_fci_internal->szFileNameCFFILE2,
      CB_MAX_FILENAME)) {
    fci_set_error( FCIERR_NONE, ERROR_FUNCTION_FAILED, TRUE );
    return FALSE;
  }
  /* safety */
  if ( strlen(p_fci_internal->szFileNameCFFILE2) >= CB_MAX_FILENAME ) {
    fci_set_error( FCIERR_NONE, ERROR_INVALID_DATA, TRUE );
    return FALSE;
  }
  p_fci_internal->handleCFFILE2 = PFCI_OPEN(hfci,
    p_fci_internal->szFileNameCFFILE2, 34050, 384, &err, pv);
  if(p_fci_internal->handleCFFILE2==0){
    fci_set_error( FCIERR_TEMP_FILE, ERROR_OPEN_FAILED, TRUE );
    return FALSE;
  }
  /* TODO error checking of err */

  /* array of all CFFILE in a folder, ready to be copied into cabinet */
  if( !PFCI_GETTEMPFILE(hfci,p_fci_internal->szFileNameCFFOLDER,
      CB_MAX_FILENAME)) {
    fci_set_error( FCIERR_NONE, ERROR_FUNCTION_FAILED, TRUE );
    return FALSE;
  }
  /* safety */
  if ( strlen(p_fci_internal->szFileNameCFFOLDER) >= CB_MAX_FILENAME ) {
    fci_set_error( FCIERR_NONE, ERROR_INVALID_DATA, TRUE );
    return FALSE;
  }
  p_fci_internal->handleCFFOLDER = PFCI_OPEN(hfci,
    p_fci_internal->szFileNameCFFOLDER, 34050, 384, &err, pv);
  if(p_fci_internal->handleCFFOLDER==0) {
    fci_set_error( FCIERR_TEMP_FILE, ERROR_OPEN_FAILED, TRUE);
    return FALSE;
  }

  /* TODO close and delete new files when return FALSE */
  /* TODO error checking of err */

  return hfci;
} /* end of FCICreate */






static BOOL fci_flush_data_block (HFCI hfci, int* err,
    PFNFCISTATUS pfnfcis) {

  /* attention no hfci checks!!! */
  /* attention no checks if there is data available!!! */
  CFDATA data;
  CFDATA* cfdata=&data;
  char* reserved;
  PFCI_Int p_fci_internal=((PFCI_Int)(hfci));
  UINT cbReserveCFData=p_fci_internal->pccab->cbReserveCFData;
  UINT i;

  /* TODO compress the data of p_fci_internal->data_in */
  /* and write it to p_fci_internal->data_out */
  memcpy(p_fci_internal->data_out, p_fci_internal->data_in,
    p_fci_internal->cdata_in /* number of bytes to copy */);

  cfdata->csum=0; /* checksum has to be set later */
  /* TODO set realsize of compressed data */
  cfdata->cbData   = p_fci_internal->cdata_in;
  cfdata->cbUncomp = p_fci_internal->cdata_in;

  /* write cfdata to p_fci_internal->handleCFDATA1 */
  if( PFCI_WRITE(hfci, p_fci_internal->handleCFDATA1, /* file handle */
      cfdata, sizeof(*cfdata), err, p_fci_internal->pv)
      != sizeof(*cfdata) ) {
    fci_set_error( FCIERR_TEMP_FILE, ERROR_WRITE_FAULT, TRUE );
    return FALSE;
  }
  /* TODO error handling of err */

  p_fci_internal->sizeFileCFDATA1 += sizeof(*cfdata);

  /* add optional reserved area */

  /* This allocation and freeing at each CFData block is a bit */
  /* inefficent, but it's harder to forget about freeing the buffer :-). */
  /* Reserved areas are used seldom besides that... */
  if (cbReserveCFData!=0) {
    if(!(reserved = (char*)PFCI_ALLOC(hfci, cbReserveCFData))) {
      fci_set_error( FCIERR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY, TRUE );
      return FALSE;
    }
    for(i=0;i<cbReserveCFData;) {
      reserved[i++]='\0';
    }
    if( PFCI_WRITE(hfci, p_fci_internal->handleCFDATA1, /* file handle */
        reserved, /* memory buffer */
        cbReserveCFData, /* number of bytes to copy */
        err, p_fci_internal->pv) != cbReserveCFData ) {
      PFCI_FREE(hfci, reserved);
      fci_set_error( FCIERR_TEMP_FILE, ERROR_WRITE_FAULT, TRUE );
      return FALSE;
    }
    /* TODO error handling of err PFCI_FREE(hfci, reserved)*/

    p_fci_internal->sizeFileCFDATA1 += cbReserveCFData;
    PFCI_FREE(hfci, reserved);
  }

  /* write p_fci_internal->data_out to p_fci_internal->handleCFDATA1 */
  if( PFCI_WRITE(hfci, p_fci_internal->handleCFDATA1, /* file handle */
      p_fci_internal->data_out, /* memory buffer */
      cfdata->cbData, /* number of bytes to copy */
      err, p_fci_internal->pv) != cfdata->cbData) {
    fci_set_error( FCIERR_TEMP_FILE, ERROR_WRITE_FAULT, TRUE );
    return FALSE;
  }
  /* TODO error handling of err */

  p_fci_internal->sizeFileCFDATA1 += cfdata->cbData;

  /* reset the offset */
  p_fci_internal->cdata_in = 0;
  p_fci_internal->cCompressedBytesInFolder += cfdata->cbData;

  /* report status with pfnfcis about uncompressed and compressed file data */
  if( (*pfnfcis)(statusFile, cfdata->cbData, cfdata->cbUncomp,
      p_fci_internal->pv) == -1) {
    fci_set_error( FCIERR_USER_ABORT, 0, TRUE );
    return FALSE;
  }

  ++(p_fci_internal->cDataBlocks);

  return TRUE;
} /* end of fci_flush_data_block */





static cab_ULONG fci_get_checksum(const void *pv, UINT cb, CHECKSUM seed)
{
  cab_ULONG     csum;
  cab_ULONG     ul;
  int           cUlong;
  const BYTE    *pb;

  csum = seed;
  cUlong = cb / 4;
  pb = pv;

  while (cUlong-- > 0) {
    ul = *pb++;
    ul |= (((cab_ULONG)(*pb++)) <<  8);
    ul |= (((cab_ULONG)(*pb++)) << 16);
    ul |= (((cab_ULONG)(*pb++)) << 24);

    csum ^= ul;
  }

  ul = 0;
  switch (cb % 4) {
    case 3:
      ul |= (((ULONG)(*pb++)) << 16);
    case 2:
      ul |= (((ULONG)(*pb++)) <<  8);
    case 1:
      ul |= *pb++;
    default:
      break;
  }
  csum ^= ul;

  return csum;
} /* end of fci_get_checksum */



static BOOL fci_flushfolder_copy_cfdata(HFCI hfci, char* buffer, UINT cbReserveCFData,
  PFNFCISTATUS pfnfcis, int* err, int handleCFDATA1new,
  cab_ULONG* psizeFileCFDATA1new, cab_ULONG* payload)
{
  cab_ULONG read_result;
  CFDATA* pcfdata=(CFDATA*)buffer;
  BOOL split_block=FALSE;
  cab_UWORD savedUncomp=0;
  PFCI_Int p_fci_internal=((PFCI_Int)(hfci));

  *payload=0;

  /* while not all CFDATAs have been copied do */
  while(!FALSE) {
    if( p_fci_internal->fNextCab ) {
      if( split_block ) {
        /* internal error should never happen */
        fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
        return FALSE;
      }
    }
    /* REUSE the variable read_result */
    if ( p_fci_internal->oldCCAB.cbReserveCFHeader != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFFolder != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFData   != 0 ) {
      read_result=4;
    } else {
      read_result=0;
    }
    if (p_fci_internal->fPrevCab) {
      read_result+=strlen(p_fci_internal->szPrevCab)+1 +
        strlen(p_fci_internal->szPrevDisk)+1;
    }
    /* No more CFDATA fits into the cabinet under construction */
    /* So don't try to store more data into it */
    if( p_fci_internal->fNextCab &&
        (p_fci_internal->oldCCAB.cb <= sizeof(CFDATA) + cbReserveCFData +
        p_fci_internal->sizeFileCFFILE1 + p_fci_internal->sizeFileCFDATA2 +
        p_fci_internal->sizeFileCFFILE2 + p_fci_internal->sizeFileCFFOLDER +
        sizeof(CFHEADER) +
        read_result +
        p_fci_internal->oldCCAB.cbReserveCFHeader +
        sizeof(CFFOLDER) +
        p_fci_internal->oldCCAB.cbReserveCFFolder +
        strlen(p_fci_internal->pccab->szCab)+1 +
        strlen(p_fci_internal->pccab->szDisk)+1
    )) {
      /* This may never be run for the first time the while loop is entered.
      Pray that the code that calls fci_flushfolder_copy_cfdata handles this.*/
      split_block=TRUE;  /* In this case split_block is abused to store */
      /* the complete data block into the next cabinet and not into the */
      /* current one. Originally split_block is the indicator that a */
      /* data block has been splitted across different cabinets. */
    } else {

      /* read CFDATA from p_fci_internal->handleCFDATA1 to cfdata*/
      read_result= PFCI_READ(hfci, p_fci_internal->handleCFDATA1,/*file handle*/
          buffer, /* memory buffer */
          sizeof(CFDATA)+cbReserveCFData, /* number of bytes to copy */
          err, p_fci_internal->pv);
      if (read_result!=sizeof(CFDATA)+cbReserveCFData) {
        if (read_result==0) break; /* ALL DATA has been copied */
        /* read error */
        fci_set_error( FCIERR_NONE, ERROR_READ_FAULT, TRUE );
        return FALSE;
      }
      /* TODO error handling of err */

      /* REUSE buffer p_fci_internal->data_out !!! */
      /* read data from p_fci_internal->handleCFDATA1 to */
      /*      p_fci_internal->data_out */
      if( PFCI_READ(hfci, p_fci_internal->handleCFDATA1 /* file handle */,
          p_fci_internal->data_out /* memory buffer */,
          pcfdata->cbData /* number of bytes to copy */,
          err, p_fci_internal->pv) != pcfdata->cbData ) {
        /* read error */
        fci_set_error( FCIERR_NONE, ERROR_READ_FAULT, TRUE );
        return FALSE;
      }
      /* TODO error handling of err */

      /* if cabinet size is too large */

      /* REUSE the variable read_result */
      if ( p_fci_internal->oldCCAB.cbReserveCFHeader != 0 ||
          p_fci_internal->oldCCAB.cbReserveCFFolder != 0 ||
          p_fci_internal->oldCCAB.cbReserveCFData   != 0 ) {
        read_result=4;
      } else {
        read_result=0;
      }
      if (p_fci_internal->fPrevCab) {
        read_result+=strlen(p_fci_internal->szPrevCab)+1 +
          strlen(p_fci_internal->szPrevDisk)+1;
      }

      /* Is cabinet with new CFDATA too large? Then data block has to be split */
      if( p_fci_internal->fNextCab &&
          (p_fci_internal->oldCCAB.cb < sizeof(CFDATA) + cbReserveCFData +
          pcfdata->cbData +
          p_fci_internal->sizeFileCFFILE1 + p_fci_internal->sizeFileCFDATA2 +
          p_fci_internal->sizeFileCFFILE2 + p_fci_internal->sizeFileCFFOLDER +
          sizeof(CFHEADER) +
          read_result +
          p_fci_internal->oldCCAB.cbReserveCFHeader +
          sizeof(CFFOLDER) + /* size of new CFFolder entry */
          p_fci_internal->oldCCAB.cbReserveCFFolder +
          strlen(p_fci_internal->pccab->szCab)+1 + /* name of next cabinet */
          strlen(p_fci_internal->pccab->szDisk)+1  /* name of next disk */
      )) {
        /* REUSE read_result to save the size of the compressed data */
        read_result=pcfdata->cbData;
        /* Modify the size of the compressed data to store only a part of the */
        /* data block into the current cabinet. This is done to prevent */
        /* that the maximum cabinet size will be exceeded. The remainder */
        /* will be stored into the next following cabinet. */

        /* The cabinet will be of size "p_fci_internal->oldCCAB.cb". */
        /* Substract everything except the size of the block of data */
        /* to get it's actual size */
        pcfdata->cbData = p_fci_internal->oldCCAB.cb - (
          sizeof(CFDATA) + cbReserveCFData +
          p_fci_internal->sizeFileCFFILE1 + p_fci_internal->sizeFileCFDATA2 +
          p_fci_internal->sizeFileCFFILE2 + p_fci_internal->sizeFileCFFOLDER +
          sizeof(CFHEADER) +
          p_fci_internal->oldCCAB.cbReserveCFHeader +
          sizeof(CFFOLDER) + /* set size of new CFFolder entry */
          p_fci_internal->oldCCAB.cbReserveCFFolder );
        /* substract the size of special header fields */
        if ( p_fci_internal->oldCCAB.cbReserveCFHeader != 0 ||
            p_fci_internal->oldCCAB.cbReserveCFFolder != 0 ||
            p_fci_internal->oldCCAB.cbReserveCFData   != 0 ) {
          pcfdata->cbData-=4;
        }
        if (p_fci_internal->fPrevCab) {
          pcfdata->cbData-=strlen(p_fci_internal->szPrevCab)+1 +
            strlen(p_fci_internal->szPrevDisk)+1;
        }
        pcfdata->cbData-=strlen(p_fci_internal->pccab->szCab)+1 +
          strlen(p_fci_internal->pccab->szDisk)+1;

        savedUncomp = pcfdata->cbUncomp;
        pcfdata->cbUncomp = 0; /* on splitted blocks of data this is zero */

        /* if split_block==TRUE then the above while loop won't */
        /* be executed again */
        split_block=TRUE; /* split_block is the indicator that */
                          /* a data block has been splitted across */
                          /* diffentent cabinets.*/
      }

      /* This should never happen !!! */
      if (pcfdata->cbData==0) {
        /* set error */
        fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
        return FALSE;
      }

      /* set little endian */
      pcfdata->cbData=fci_endian_uword(pcfdata->cbData);
      pcfdata->cbUncomp=fci_endian_uword(pcfdata->cbUncomp);

      /* get checksum and write to cfdata.csum */
      pcfdata->csum = fci_get_checksum( &(pcfdata->cbData),
        sizeof(CFDATA)+cbReserveCFData -
        sizeof(pcfdata->csum), fci_get_checksum( p_fci_internal->data_out, /*buffer*/
        pcfdata->cbData, 0 ) );

      /* set little endian */
      pcfdata->csum=fci_endian_ulong(pcfdata->csum);

      /* write cfdata with checksum to p_fci_internal->handleCFDATA2 */
      if( PFCI_WRITE(hfci, p_fci_internal->handleCFDATA2, /* file handle */
          buffer, /* memory buffer */
          sizeof(CFDATA)+cbReserveCFData, /* number of bytes to copy */
          err, p_fci_internal->pv) != sizeof(CFDATA)+cbReserveCFData ) {
         fci_set_error( FCIERR_TEMP_FILE, ERROR_WRITE_FAULT, TRUE );
         return FALSE;
      }
      /* TODO error handling of err */

      p_fci_internal->sizeFileCFDATA2 += sizeof(CFDATA)+cbReserveCFData;

      /* reset little endian */
      pcfdata->cbData=fci_endian_uword(pcfdata->cbData);
      pcfdata->cbUncomp=fci_endian_uword(pcfdata->cbUncomp);
      pcfdata->csum=fci_endian_ulong(pcfdata->csum);

      /* write compressed data into p_fci_internal->handleCFDATA2 */
      if( PFCI_WRITE(hfci, p_fci_internal->handleCFDATA2, /* file handle */
          p_fci_internal->data_out, /* memory buffer */
          pcfdata->cbData, /* number of bytes to copy */
          err, p_fci_internal->pv) != pcfdata->cbData) {
        fci_set_error( FCIERR_TEMP_FILE, ERROR_WRITE_FAULT, TRUE );
        return FALSE;
      }
      /* TODO error handling of err */

      p_fci_internal->sizeFileCFDATA2 += pcfdata->cbData;
      ++(p_fci_internal->cDataBlocks);
      p_fci_internal->statusFolderCopied += pcfdata->cbData;
      (*payload)+=pcfdata->cbUncomp;
      /* if cabinet size too large and data has been split */
      /* write the remainder of the data block to the new CFDATA1 file */
      if( split_block  ) { /* This does not include the */
                                  /* abused one (just search for "abused" )*/
      /* copy all CFDATA structures from handleCFDATA1 to handleCFDATA1new */
        if (p_fci_internal->fNextCab==FALSE ) {
          /* internal error */
          fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
          return FALSE;
        }

        /* set cbData to the size of the remainder of the data block */
        pcfdata->cbData = read_result - pcfdata->cbData;
        /*recover former value of cfdata.cbData; read_result will be the offset*/
        read_result -= pcfdata->cbData;
        pcfdata->cbUncomp = savedUncomp;

        /* reset checksum, it will be computed later */
        pcfdata->csum=0;

        /* write cfdata WITHOUT checksum to handleCFDATA1new */
        if( PFCI_WRITE(hfci, handleCFDATA1new, /* file handle */
            buffer, /* memory buffer */
            sizeof(CFDATA)+cbReserveCFData, /* number of bytes to copy */
            err, p_fci_internal->pv) != sizeof(CFDATA)+cbReserveCFData ) {
          fci_set_error( FCIERR_TEMP_FILE, ERROR_WRITE_FAULT, TRUE );
          return FALSE;
        }
        /* TODO error handling of err don't forget PFCI_FREE(hfci, reserved) */

        *psizeFileCFDATA1new += sizeof(CFDATA)+cbReserveCFData;

        /* write compressed data into handleCFDATA1new */
        if( PFCI_WRITE(hfci, handleCFDATA1new, /* file handle */
            p_fci_internal->data_out + read_result, /* memory buffer + offset */
                                                /* to last part of split data */
            pcfdata->cbData, /* number of bytes to copy */
            err, p_fci_internal->pv) != pcfdata->cbData) {
          fci_set_error( FCIERR_TEMP_FILE, ERROR_WRITE_FAULT, TRUE );
          return FALSE;
        }
        /* TODO error handling of err */

        p_fci_internal->statusFolderCopied += pcfdata->cbData;

        *psizeFileCFDATA1new += pcfdata->cbData;
        /* the two blocks of the split data block have been written */
        /* don't reset split_data yet, because it is still needed see below */
      }

      /* report status with pfnfcis about copied size of folder */
      if( (*pfnfcis)(statusFolder,
          p_fci_internal->statusFolderCopied, /*cfdata.cbData(+previous ones)*/
          p_fci_internal->statusFolderTotal, /* total folder size */
          p_fci_internal->pv) == -1) {
        fci_set_error( FCIERR_USER_ABORT, 0, TRUE );
        return FALSE;
      }
    }

    /* if cabinet size too large */
    /* write the remaining data blocks to the new CFDATA1 file */
    if ( split_block ) { /* This does include the */
                               /* abused one (just search for "abused" )*/
      if (p_fci_internal->fNextCab==FALSE ) {
        /* internal error */
        fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
        return FALSE;
      }
      /* copy all CFDATA structures from handleCFDATA1 to handleCFDATA1new */
      while(!FALSE) {
        /* read CFDATA from p_fci_internal->handleCFDATA1 to cfdata*/
        read_result= PFCI_READ(hfci, p_fci_internal->handleCFDATA1,/* handle */
            buffer, /* memory buffer */
            sizeof(CFDATA)+cbReserveCFData, /* number of bytes to copy */
            err, p_fci_internal->pv);
        if (read_result!=sizeof(CFDATA)+cbReserveCFData) {
          if (read_result==0) break; /* ALL DATA has been copied */
          /* read error */
          fci_set_error(FCIERR_NONE, ERROR_READ_FAULT, TRUE );
          return FALSE;
        }
        /* TODO error handling of err */

        /* REUSE buffer p_fci_internal->data_out !!! */
        /* read data from p_fci_internal->handleCFDATA1 to */
        /*      p_fci_internal->data_out */
        if( PFCI_READ(hfci, p_fci_internal->handleCFDATA1 /* file handle */,
            p_fci_internal->data_out /* memory buffer */,
            pcfdata->cbData /* number of bytes to copy */,
            err, p_fci_internal->pv) != pcfdata->cbData ) {
          /* read error */
          fci_set_error( FCIERR_NONE, ERROR_READ_FAULT, TRUE);
          return FALSE;
        }
        /* TODO error handling of err don't forget PFCI_FREE(hfci, reserved) */

        /* write cfdata with checksum to handleCFDATA1new */
        if( PFCI_WRITE(hfci, handleCFDATA1new, /* file handle */
            buffer, /* memory buffer */
            sizeof(CFDATA)+cbReserveCFData, /* number of bytes to copy */
            err, p_fci_internal->pv) != sizeof(CFDATA)+cbReserveCFData ) {
          fci_set_error( FCIERR_TEMP_FILE, ERROR_WRITE_FAULT, TRUE );
          return FALSE;
        }
        /* TODO error handling of err don't forget PFCI_FREE(hfci, reserved) */

        *psizeFileCFDATA1new += sizeof(CFDATA)+cbReserveCFData;

        /* write compressed data into handleCFDATA1new */
        if( PFCI_WRITE(hfci, handleCFDATA1new, /* file handle */
            p_fci_internal->data_out, /* memory buffer */
            pcfdata->cbData, /* number of bytes to copy */
            err, p_fci_internal->pv) != pcfdata->cbData) {
          fci_set_error( FCIERR_TEMP_FILE, ERROR_WRITE_FAULT, TRUE );
          return FALSE;
        }
        /* TODO error handling of err */

        *psizeFileCFDATA1new += pcfdata->cbData;
        p_fci_internal->statusFolderCopied += pcfdata->cbData;

        /* report status with pfnfcis about copied size of folder */
        if( (*pfnfcis)(statusFolder,
            p_fci_internal->statusFolderCopied,/*cfdata.cbData(+revious ones)*/
            p_fci_internal->statusFolderTotal, /* total folder size */
            p_fci_internal->pv) == -1) {
          fci_set_error( FCIERR_USER_ABORT, 0, TRUE );
          return FALSE;
        }

      } /* end of WHILE */
      break; /* jump out of the next while loop */
    } /* end of if( split_data  ) */
  } /* end of WHILE */
  return TRUE;
} /* end of fci_flushfolder_copy_cfdata */





static BOOL fci_flushfolder_copy_cffolder(HFCI hfci, int* err, UINT cbReserveCFFolder,
  cab_ULONG sizeFileCFDATA2old)
{
  CFFOLDER cffolder;
  UINT i;
  char* reserved;
  PFCI_Int p_fci_internal=((PFCI_Int)(hfci));

  /* absolute offset cannot be set yet, because the size of cabinet header, */
  /* the number of CFFOLDERs and the number of CFFILEs may change. */
  /* Instead the size of all previous data blocks will be stored and */
  /* the remainder of the offset will be added when the cabinet will be */
  /* flushed to disk. */
  /* This is exactly the way the original CABINET.DLL works!!! */
  cffolder.coffCabStart=sizeFileCFDATA2old;

  /* set the number of this folder's CFDATA sections */
  cffolder.cCFData=p_fci_internal->cDataBlocks;
  /* TODO set compression type */
  cffolder.typeCompress = tcompTYPE_NONE;

  /* write cffolder to p_fci_internal->handleCFFOLDER */
  if( PFCI_WRITE(hfci, p_fci_internal->handleCFFOLDER, /* file handle */
      &cffolder, /* memory buffer */
      sizeof(cffolder), /* number of bytes to copy */
      err, p_fci_internal->pv) != sizeof(cffolder) ) {
    fci_set_error( FCIERR_TEMP_FILE, ERROR_WRITE_FAULT, TRUE );
    return FALSE;
  }
  /* TODO error handling of err */

  p_fci_internal->sizeFileCFFOLDER += sizeof(cffolder);

  /* add optional reserved area */
  if (cbReserveCFFolder!=0) {
    if(!(reserved = (char*)PFCI_ALLOC(hfci, cbReserveCFFolder))) {
      fci_set_error( FCIERR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY, TRUE );
      return FALSE;
    }
    for(i=0;i<cbReserveCFFolder;) {
      reserved[i++]='\0';
    }
    if( PFCI_WRITE(hfci, p_fci_internal->handleCFFOLDER, /* file handle */
        reserved, /* memory buffer */
        cbReserveCFFolder, /* number of bytes to copy */
        err, p_fci_internal->pv) != cbReserveCFFolder ) {
      PFCI_FREE(hfci, reserved);
      fci_set_error( FCIERR_TEMP_FILE, ERROR_WRITE_FAULT, TRUE );
      return FALSE;
    }
    /* TODO error handling of err */

    p_fci_internal->sizeFileCFFOLDER += cbReserveCFFolder;

    PFCI_FREE(hfci, reserved);
  }
  return TRUE;
} /* end of fci_flushfolder_copy_cffolder */





static BOOL fci_flushfolder_copy_cffile(HFCI hfci, int* err, int handleCFFILE1new,
  cab_ULONG *psizeFileCFFILE1new, cab_ULONG payload)
{
  CFFILE cffile;
  cab_ULONG read_result;
  cab_ULONG seek=0;
  cab_ULONG sizeOfFiles=0, sizeOfFilesPrev;
  BOOL may_be_prev=TRUE;
  cab_ULONG cbFileRemainer=0;
  PFCI_Int p_fci_internal=((PFCI_Int)(hfci));
  /* set seek of p_fci_internal->handleCFFILE1 to 0 */
  if( PFCI_SEEK(hfci,p_fci_internal->handleCFFILE1,0,SEEK_SET,err,
    p_fci_internal->pv) !=0 ) {
    /* wrong return value */
    fci_set_error( FCIERR_NONE, ERROR_SEEK, TRUE );
    return FALSE;
  }
  /* TODO error handling of err */

  /* while not all CFFILE structures have been copied do */
  while(!FALSE) {
    /* REUSE the variable read_result */
    /* read data from p_fci_internal->handleCFFILE1 to cffile */
    read_result = PFCI_READ(hfci,p_fci_internal->handleCFFILE1/* file handle */,
      &cffile, /* memory buffer */
      sizeof(cffile), /* number of bytes to copy */
      err, p_fci_internal->pv);
    if( read_result != sizeof(cffile) ) {
      if( read_result == 0 ) break; /* ALL CFFILE structures have been copied */
      /* read error */
      fci_set_error( FCIERR_NONE, ERROR_READ_FAULT, TRUE );
      return FALSE;
    }
    /* TODO error handling of err */

    /* Microsoft's(R) CABINET.DLL would do a seek to the current! */
    /* position. I don't know why so I'll just omit it */

    /* read the filename from p_fci_internal->handleCFFILE1 */
    /* REUSE the variable read_result AGAIN */
    /* REUSE the memory buffer PFCI(hfci)->data_out */
    if( PFCI_READ(hfci, p_fci_internal->handleCFFILE1 /*file handle*/,
        p_fci_internal->data_out, /* memory buffer */
        CB_MAX_FILENAME, /* number of bytes to copy */
        err, p_fci_internal->pv) <2) {
      /* read error */
      fci_set_error( FCIERR_NONE, ERROR_READ_FAULT, TRUE );
      return FALSE;
    }
    /* TODO maybe other checks of read_result */
    /* TODO error handling of err */

    /* safety */
    if( strlen(p_fci_internal->data_out)>=CB_MAX_FILENAME ) {
      /* set error code internal error */
      fci_set_error( FCIERR_NONE, ERROR_INVALID_DATA, TRUE );
      return FALSE;
    }

    seek+=sizeof(cffile) + strlen(p_fci_internal->data_out)+1;

    /* set seek of p_fci_internal->handleCFFILE1 to end of file name */
    /* i.e. seek to the next CFFILE area */
    if( PFCI_SEEK(hfci,p_fci_internal->handleCFFILE1,
        seek, /* seek position*/
        SEEK_SET ,err,
        p_fci_internal->pv)
        != seek) {
      /* wrong return value */
      fci_set_error( FCIERR_NONE, ERROR_SEEK, TRUE );
      return FALSE;
    }
    /* TODO error handling of err */

    /* fnfilfnfildest: placed file on cabinet */
    if (p_fci_internal->fNextCab ||
        p_fci_internal->fGetNextCabInVain) {
      PFCI_FILEPLACED( hfci, &(p_fci_internal->oldCCAB),
        p_fci_internal->data_out, /* the file name*/
        cffile.cbFile, /* file size */
        (cffile.iFolder==cffileCONTINUED_FROM_PREV),
        p_fci_internal->pv
      );
    } else {
      PFCI_FILEPLACED( hfci, p_fci_internal->pccab,
        p_fci_internal->data_out, /* the file name*/
        cffile.cbFile, /* file size */
        (cffile.iFolder==cffileCONTINUED_FROM_PREV),
        p_fci_internal->pv
      );
    }

    /* Check special iFolder values */
    if( cffile.iFolder==cffileCONTINUED_FROM_PREV &&
        p_fci_internal->fPrevCab==FALSE ) {
      /* THIS MAY NEVER HAPPEN */
      /* set error code */
      fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
      return FALSE;
    }
    if( cffile.iFolder==cffileCONTINUED_PREV_AND_NEXT ||
        cffile.iFolder==cffileCONTINUED_TO_NEXT ) {
      /* THIS MAY NEVER HAPPEN */
      /* set error code */
      fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
      return FALSE;
    }
    if( may_be_prev && cffile.iFolder!=cffileCONTINUED_FROM_PREV ) {
      may_be_prev=FALSE;
    }
    if( cffile.iFolder==cffileCONTINUED_FROM_PREV && may_be_prev==FALSE ) {
      /* THIS MAY NEVER HAPPEN */
      /* set error code */
      fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
      return FALSE;
    }
    if( cffile.iFolder!=cffileCONTINUED_FROM_PREV ) {
      may_be_prev=FALSE;
    }

    sizeOfFilesPrev=sizeOfFiles;
    /* Set complete size of all processed files */
    if( cffile.iFolder==cffileCONTINUED_FROM_PREV &&
        p_fci_internal->cbFileRemainer!=0
    ) {
      sizeOfFiles+=p_fci_internal->cbFileRemainer;
      p_fci_internal->cbFileRemainer=0;
    } else {
      sizeOfFiles+=cffile.cbFile;
    }

    /* Check if spanned file fits into this cabinet folder */
    if( cffile.iFolder==cffileCONTINUED_FROM_PREV && sizeOfFiles>payload ) {
      cffile.iFolder=cffileCONTINUED_PREV_AND_NEXT;
    } else

    /* Check if file doesn't fit into this cabinet folder */
    if( sizeOfFiles>payload ) {
      cffile.iFolder=cffileCONTINUED_TO_NEXT;
    }

    /* set little endian */
    cffile.cbFile=fci_endian_ulong(cffile.cbFile);
    cffile.uoffFolderStart=fci_endian_ulong(cffile.uoffFolderStart);
    cffile.iFolder=fci_endian_uword(cffile.iFolder);
    cffile.date=fci_endian_uword(cffile.date);
    cffile.time=fci_endian_uword(cffile.time);
    cffile.attribs=fci_endian_uword(cffile.attribs);

    /* write cffile to p_fci_internal->handleCFFILE2 */
    if( PFCI_WRITE(hfci, p_fci_internal->handleCFFILE2, /* file handle */
        &cffile, /* memory buffer */
        sizeof(cffile), /* number of bytes to copy */
        err, p_fci_internal->pv) != sizeof(cffile) ) {
      fci_set_error( FCIERR_TEMP_FILE, ERROR_WRITE_FAULT, TRUE );
      return FALSE;
    }
    /* TODO error handling of err */

    p_fci_internal->sizeFileCFFILE2 += sizeof(cffile);

    /* reset little endian */
    cffile.cbFile=fci_endian_ulong(cffile.cbFile);
    cffile.uoffFolderStart=fci_endian_ulong(cffile.uoffFolderStart);
    cffile.iFolder=fci_endian_uword(cffile.iFolder);
    cffile.date=fci_endian_uword(cffile.date);
    cffile.time=fci_endian_uword(cffile.time);
    cffile.attribs=fci_endian_uword(cffile.attribs);

    /* write file name to p_fci_internal->handleCFFILE2 */
    if( PFCI_WRITE(hfci, p_fci_internal->handleCFFILE2, /* file handle */
        p_fci_internal->data_out, /* memory buffer */
        strlen(p_fci_internal->data_out)+1, /* number of bytes to copy */
        err, p_fci_internal->pv) != strlen(p_fci_internal->data_out)+1 ) {
      fci_set_error( FCIERR_TEMP_FILE, ERROR_WRITE_FAULT, TRUE );
      return FALSE;
    }
    /* TODO error handling of err */

    p_fci_internal->sizeFileCFFILE2 += strlen(p_fci_internal->data_out)+1;

    /* cFiles is used to count all files of a cabinet */
    ++(p_fci_internal->cFiles);

    /* This is only true for files which will be written into the */
    /* next cabinet of the spanning folder */
    if( sizeOfFiles>payload ) {

      /* Files which data will be partially written into the current cabinet */
      if( cffile.iFolder==cffileCONTINUED_PREV_AND_NEXT ||
          cffile.iFolder==cffileCONTINUED_TO_NEXT
        ) {
        if( sizeOfFilesPrev<=payload ) {
          /* The size of the uncompressed, data of a spanning file in a */
          /* spanning data */
          cbFileRemainer=sizeOfFiles-payload;
        }
        cffile.iFolder=cffileCONTINUED_FROM_PREV;
      } else {
        cffile.iFolder=0;
      }

      /* write cffile into handleCFFILE1new */
      if( PFCI_WRITE(hfci, handleCFFILE1new, /* file handle */
          &cffile, /* memory buffer */
          sizeof(cffile), /* number of bytes to copy */
          err, p_fci_internal->pv) != sizeof(cffile) ) {
        fci_set_error( FCIERR_TEMP_FILE, ERROR_WRITE_FAULT, TRUE );
        return FALSE;
      }
      /* TODO error handling of err */

      *psizeFileCFFILE1new += sizeof(cffile);
      /* write name of file into handleCFFILE1new */
      if( PFCI_WRITE(hfci, handleCFFILE1new, /* file handle */
          p_fci_internal->data_out, /* memory buffer */
          strlen(p_fci_internal->data_out)+1, /* number of bytes to copy */
          err, p_fci_internal->pv) != strlen(p_fci_internal->data_out)+1 ) {
        fci_set_error( FCIERR_TEMP_FILE, ERROR_WRITE_FAULT, TRUE );
        return FALSE;
      }
      /* TODO error handling of err */

      *psizeFileCFFILE1new += strlen(p_fci_internal->data_out)+1;
    }

  } /* END OF while */
  p_fci_internal->cbFileRemainer=cbFileRemainer;
  return TRUE;
} /* end of fci_flushfolder_copy_cffile */




static BOOL fci_flush_folder(
	HFCI                  hfci,
	BOOL                  fGetNextCab,
	PFNFCIGETNEXTCABINET  pfnfcignc,
	PFNFCISTATUS          pfnfcis)
{
  int err;
  int handleCFDATA1new;                         /* handle for new  temp file */
  char szFileNameCFDATA1new[CB_MAX_FILENAME];  /* name buffer for temp file */
  int handleCFFILE1new;                         /* handle for new  temp file */
  char szFileNameCFFILE1new[CB_MAX_FILENAME];  /* name buffer for temp file */
  UINT cbReserveCFData, cbReserveCFFolder;
  char* reserved;
  cab_ULONG sizeFileCFDATA1new=0;
  cab_ULONG sizeFileCFFILE1new=0;
  cab_ULONG sizeFileCFDATA2old;
  cab_ULONG payload;
  cab_ULONG read_result;
  PFCI_Int p_fci_internal=((PFCI_Int)(hfci));

  /* test hfci */
  if (!REALLY_IS_FCI(hfci)) {
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  if ((!pfnfcignc) || (!pfnfcis)) {
    fci_set_error( FCIERR_NONE, ERROR_BAD_ARGUMENTS, TRUE );
    return FALSE;
  }

  if( p_fci_internal->fGetNextCabInVain &&
      p_fci_internal->fNextCab ){
    /* internal error */
    fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
    return FALSE;
  }

  /* If there was no FCIAddFile or FCIFlushFolder has already been called */
  /* this function will return TRUE */
  if( p_fci_internal->sizeFileCFFILE1 == 0 ) {
    if ( p_fci_internal->sizeFileCFDATA1 != 0 ) {
      /* error handling */
      fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
      return FALSE;
    }
    return TRUE;
  }

  if (p_fci_internal->data_in==NULL || p_fci_internal->data_out==NULL ) {
    /* error handling */
    fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
    return FALSE;
  }

  /* FCIFlushFolder has already been called... */
  if (p_fci_internal->fSplitFolder && p_fci_internal->sizeFileCFFILE2!=0) {
    return TRUE;
  }

  /* This can be set already, because it makes only a difference */
  /* when the current function exits with return FALSE */
  p_fci_internal->fSplitFolder=FALSE;


  if( p_fci_internal->fGetNextCabInVain ||
      p_fci_internal->fNextCab ){
    cbReserveCFData   = p_fci_internal->oldCCAB.cbReserveCFData;
    cbReserveCFFolder = p_fci_internal->oldCCAB.cbReserveCFFolder;
  } else {
    cbReserveCFData   = p_fci_internal->pccab->cbReserveCFData;
    cbReserveCFFolder = p_fci_internal->pccab->cbReserveCFFolder;
  }

  /* START of COPY */
  /* if there is data in p_fci_internal->data_in */
  if (p_fci_internal->cdata_in!=0) {

    if( !fci_flush_data_block(hfci, &err, pfnfcis) ) return FALSE;

  }
  /* reset to get the number of data blocks of this folder which are */
  /* actually in this cabinet ( at least partially ) */
  p_fci_internal->cDataBlocks=0;

  if ( p_fci_internal->fNextCab ||
       p_fci_internal->fGetNextCabInVain ) {
    read_result= p_fci_internal->oldCCAB.cbReserveCFHeader+
                 p_fci_internal->oldCCAB.cbReserveCFFolder;
    if ( p_fci_internal->oldCCAB.cbReserveCFHeader != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFFolder != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFData   != 0 ) {
      read_result+=4;
    }
  } else {
    read_result= p_fci_internal->pccab->cbReserveCFHeader+
                 p_fci_internal->pccab->cbReserveCFFolder;
    if ( p_fci_internal->pccab->cbReserveCFHeader != 0 ||
        p_fci_internal->pccab->cbReserveCFFolder != 0 ||
        p_fci_internal->pccab->cbReserveCFData   != 0 ) {
      read_result+=4;
    }
  }
  if (p_fci_internal->fPrevCab) {
    read_result+=strlen(p_fci_internal->szPrevCab)+1 +
      strlen(p_fci_internal->szPrevDisk)+1;
  }
  if (p_fci_internal->fNextCab) {
    read_result+=strlen(p_fci_internal->pccab->szCab)+1 +
      strlen(p_fci_internal->pccab->szDisk)+1;
  }

  p_fci_internal->statusFolderTotal = sizeof(CFHEADER)+read_result+
      sizeof(CFFOLDER) + p_fci_internal->sizeFileCFFILE2+
      p_fci_internal->sizeFileCFDATA2 + p_fci_internal->sizeFileCFFILE1+
      p_fci_internal->sizeFileCFDATA1 + p_fci_internal->sizeFileCFFOLDER;
  p_fci_internal->statusFolderCopied = 0;

  /* report status with pfnfcis about copied size of folder */
  if( (*pfnfcis)(statusFolder, p_fci_internal->statusFolderCopied,
      p_fci_internal->statusFolderTotal, /* TODO total folder size */
      p_fci_internal->pv) == -1) {
    fci_set_error( FCIERR_USER_ABORT, 0, TRUE );
    return FALSE;
  }

  /* get a new temp file */
  if(!PFCI_GETTEMPFILE(hfci,szFileNameCFDATA1new,CB_MAX_FILENAME)) {
    fci_set_error( FCIERR_NONE, ERROR_FUNCTION_FAILED, TRUE );
    return FALSE;
  }
  /* safety */
  if ( strlen(szFileNameCFDATA1new) >= CB_MAX_FILENAME ) {
    fci_set_error( FCIERR_NONE, ERROR_INVALID_DATA, TRUE );
    return FALSE;
  }
  handleCFDATA1new = PFCI_OPEN(hfci,szFileNameCFDATA1new,34050,384,&err,
    p_fci_internal->pv);
  if(handleCFDATA1new==0){
    fci_set_error( FCIERR_TEMP_FILE, ERROR_OPEN_FAILED, TRUE );
    return FALSE;
  }
  /* TODO error handling of err */



  /* get a new temp file */
  if(!PFCI_GETTEMPFILE(hfci,szFileNameCFFILE1new,CB_MAX_FILENAME)) {
    fci_set_error( FCIERR_NONE, ERROR_FUNCTION_FAILED, TRUE );
    PFCI_CLOSE(hfci,handleCFDATA1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    return FALSE;
  }
  /* safety */
  if ( strlen(szFileNameCFFILE1new) >= CB_MAX_FILENAME ) {
    fci_set_error( FCIERR_NONE, ERROR_INVALID_DATA, TRUE );
    PFCI_CLOSE(hfci,handleCFDATA1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    return FALSE;
  }
  handleCFFILE1new = PFCI_OPEN(hfci,szFileNameCFFILE1new,34050,384,&err,
    p_fci_internal->pv);
  if(handleCFFILE1new==0){
    fci_set_error( FCIERR_TEMP_FILE, ERROR_OPEN_FAILED, TRUE );
    return FALSE;
  }
  /* TODO error handling of err */

  /* USE the variable read_result */
  if ( p_fci_internal->fNextCab ||
       p_fci_internal->fGetNextCabInVain ) {
    read_result= p_fci_internal->oldCCAB.cbReserveCFHeader;
    if ( p_fci_internal->oldCCAB.cbReserveCFHeader != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFFolder != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFData   != 0 ) {
      read_result+=4;
    }
  } else {
    read_result= p_fci_internal->pccab->cbReserveCFHeader;
    if ( p_fci_internal->pccab->cbReserveCFHeader != 0 ||
        p_fci_internal->pccab->cbReserveCFFolder != 0 ||
        p_fci_internal->pccab->cbReserveCFData   != 0 ) {
      read_result+=4;
    }
  }
  if (p_fci_internal->fPrevCab) {
    read_result+=strlen(p_fci_internal->szPrevCab)+1 +
      strlen(p_fci_internal->szPrevDisk)+1;
  }
  read_result+= sizeof(CFHEADER) + p_fci_internal->sizeFileCFDATA2 +
    p_fci_internal->sizeFileCFFILE2 + p_fci_internal->sizeFileCFFOLDER;

  if(p_fci_internal->sizeFileCFFILE1!=0) {
    read_result+= sizeof(CFFOLDER)+p_fci_internal->pccab->cbReserveCFFolder;
  }

  /* Check if multiple cabinets have to be created. */

  /* Might be too much data for the maximum allowed cabinet size.*/
  /* When any further data will be added later, it might not */
  /* be possible to flush the cabinet, because there might */
  /* not be enough space to store the name of the following */
  /* cabinet and name of the corresponding disk. */
  /* So take care of this and get the name of the next cabinet */
  if( p_fci_internal->fGetNextCabInVain==FALSE &&
      p_fci_internal->fNextCab==FALSE &&
      (
        (
          p_fci_internal->pccab->cb < read_result +
          p_fci_internal->sizeFileCFDATA1 +
          p_fci_internal->sizeFileCFFILE1 +
          CB_MAX_CABINET_NAME +   /* next cabinet name */
          CB_MAX_DISK_NAME        /* next disk name */
        ) || fGetNextCab
      )
  ) {
    /* save CCAB */
    memcpy(&(p_fci_internal->oldCCAB), p_fci_internal->pccab, sizeof(CCAB));
    /* increment cabinet index */
    ++(p_fci_internal->pccab->iCab);
    /* get name of next cabinet */
    p_fci_internal->estimatedCabinetSize=p_fci_internal->statusFolderTotal;
    if (!(*pfnfcignc)(p_fci_internal->pccab,
        p_fci_internal->estimatedCabinetSize, /* estimated size of cab */
        p_fci_internal->pv)) {
      /* error handling */
      fci_set_error( FCIERR_NONE, ERROR_FUNCTION_FAILED, TRUE );
      PFCI_CLOSE(hfci,handleCFDATA1new,&err,p_fci_internal->pv);
      /* TODO error handling of err */
      PFCI_CLOSE(hfci,handleCFFILE1new,&err,p_fci_internal->pv);
      /* TODO error handling of err */
      return FALSE;
    }

    /* Skip a few lines of code. This is caught by the next if. */
    p_fci_internal->fGetNextCabInVain=TRUE;
  }

  /* too much data for cabinet */
  if( (p_fci_internal->fGetNextCabInVain ||
        p_fci_internal->fNextCab ) &&
      (
        (
          p_fci_internal->oldCCAB.cb < read_result +
          p_fci_internal->sizeFileCFDATA1 +
          p_fci_internal->sizeFileCFFILE1 +
          strlen(p_fci_internal->pccab->szCab)+1 +   /* next cabinet name */
          strlen(p_fci_internal->pccab->szDisk)+1    /* next disk name */
        ) || fGetNextCab
      )
  ) {
    p_fci_internal->fGetNextCabInVain=FALSE;
    p_fci_internal->fNextCab=TRUE;

    /* return FALSE if there is not enough space left*/
    /* this should never happen */
    if (p_fci_internal->oldCCAB.cb <=
        p_fci_internal->sizeFileCFFILE1 +
        read_result +
        strlen(p_fci_internal->pccab->szCab)+1 + /* next cabinet name */
        strlen(p_fci_internal->pccab->szDisk)+1  /* next disk name */
    ) {

      PFCI_CLOSE(hfci,handleCFDATA1new,&err,p_fci_internal->pv);
      /* TODO error handling of err */
      PFCI_DELETE(hfci,szFileNameCFDATA1new,&err,p_fci_internal->pv);
      /* TODO error handling of err */

      /* close and delete p_fci_internal->handleCFFILE1 */
      PFCI_CLOSE(hfci,handleCFFILE1new,&err,p_fci_internal->pv);
      /* TODO error handling of err */
      PFCI_DELETE(hfci,szFileNameCFFILE1new,&err,p_fci_internal->pv);
      /* TODO error handling of err */

      return FALSE;
    }

    /* the folder will be split across cabinets */
    p_fci_internal->fSplitFolder=TRUE;

  } else {
    /* this should never happen */
    if (p_fci_internal->fNextCab) {
      /* internal error */
      fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
      return FALSE;
    }
  }

  /* set seek of p_fci_internal->handleCFDATA1 to 0 */
  if( PFCI_SEEK(hfci,p_fci_internal->handleCFDATA1,0,SEEK_SET,&err,
    p_fci_internal->pv) !=0 ) {
    /* wrong return value */
    fci_set_error( FCIERR_NONE, ERROR_SEEK, TRUE );
    PFCI_CLOSE(hfci,handleCFDATA1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    PFCI_CLOSE(hfci,handleCFFILE1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    return FALSE;
  }
  /* TODO error handling of err */

  /* save size of file CFDATA2 - required for the folder's offset to data */
  sizeFileCFDATA2old = p_fci_internal->sizeFileCFDATA2;

  if(!(reserved = (char*)PFCI_ALLOC(hfci, cbReserveCFData+sizeof(CFDATA)))) {
    fci_set_error( FCIERR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY, TRUE );
    PFCI_CLOSE(hfci,handleCFDATA1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    PFCI_CLOSE(hfci,handleCFFILE1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    return FALSE;
  }

  if(!fci_flushfolder_copy_cfdata(hfci, reserved, cbReserveCFData, pfnfcis, &err,
      handleCFDATA1new, &sizeFileCFDATA1new, &payload
  )) {
    PFCI_CLOSE(hfci,handleCFDATA1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    PFCI_DELETE(hfci,szFileNameCFDATA1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    PFCI_CLOSE(hfci,handleCFFILE1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    PFCI_FREE(hfci,reserved);
    return FALSE;
  }

  PFCI_FREE(hfci,reserved);

  if(!fci_flushfolder_copy_cffolder(hfci, &err, cbReserveCFFolder,
       sizeFileCFDATA2old )) {
    PFCI_CLOSE(hfci,handleCFDATA1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    PFCI_DELETE(hfci,szFileNameCFDATA1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    PFCI_CLOSE(hfci,handleCFFILE1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    return FALSE;
  }

  if(!fci_flushfolder_copy_cffile(hfci, &err, handleCFFILE1new,
      &sizeFileCFFILE1new, payload)) {
    PFCI_CLOSE(hfci,handleCFDATA1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    PFCI_DELETE(hfci,szFileNameCFDATA1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    PFCI_CLOSE(hfci,handleCFFILE1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    PFCI_DELETE(hfci,szFileNameCFFILE1new,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    return FALSE;
  }

  /* close and delete p_fci_internal->handleCFDATA1 */
  PFCI_CLOSE(hfci,p_fci_internal->handleCFDATA1,&err,p_fci_internal->pv);
  /* TODO error handling of err */
  PFCI_DELETE(hfci,p_fci_internal->szFileNameCFDATA1,&err,p_fci_internal->pv);
  /* TODO error handling of err */

  /* put new CFDATA1 into hfci */
  memcpy(p_fci_internal->szFileNameCFDATA1,szFileNameCFDATA1new,
    CB_MAX_FILENAME);

  /* put CFDATA1 file handle */
  PFCI_INT(hfci)->handleCFDATA1 = handleCFDATA1new;
  /* set file size */
  PFCI_INT(hfci)->sizeFileCFDATA1 = sizeFileCFDATA1new;

  /* close and delete PFCI_INT(hfci)->handleCFFILE1 */
  PFCI_CLOSE(hfci,p_fci_internal->handleCFFILE1,&err,PFCI_INT(hfci)->pv);
  /* TODO error handling of err */
  PFCI_DELETE(hfci,p_fci_internal->szFileNameCFFILE1,&err,p_fci_internal->pv);
  /* TODO error handling of err */

  /* put new CFFILE1 into hfci */
  memcpy(p_fci_internal->szFileNameCFFILE1,szFileNameCFFILE1new,
    CB_MAX_FILENAME);

  /* put CFFILE1 file handle */
  p_fci_internal->handleCFFILE1 = handleCFFILE1new;
  /* set file size */
  p_fci_internal->sizeFileCFFILE1 = sizeFileCFFILE1new;

  ++(p_fci_internal->cFolders);

  /* reset CFFolder specific information */
  p_fci_internal->cDataBlocks=0;
  p_fci_internal->cCompressedBytesInFolder=0;

  return TRUE;
}  /* end of fci_flush_folder */




static BOOL fci_flush_cabinet(
	HFCI                  hfci,
	BOOL                  fGetNextCab,
	PFNFCIGETNEXTCABINET  pfnfcignc,
	PFNFCISTATUS          pfnfcis)
{
  int err;
  CFHEADER cfheader;
  struct {
    cab_UWORD  cbCFHeader;
    cab_UBYTE  cbCFFolder;
    cab_UBYTE  cbCFData;
  } cfreserved;
  CFFOLDER cffolder;
  cab_ULONG read_result=0;
  int handleCABINET;                            /* file handle for cabinet   */
  char szFileNameCABINET[CB_MAX_CAB_PATH+CB_MAX_CABINET_NAME];/* name buffer */
  UINT cbReserveCFHeader, cbReserveCFFolder, i;
  char* reserved;
  BOOL returntrue=FALSE;
  PFCI_Int p_fci_internal=((PFCI_Int)(hfci));

  /* TODO test if fci_flush_cabinet really aborts if there was no FCIAddFile */

  /* when FCIFlushCabinet was or FCIAddFile hasn't been called */
  if( p_fci_internal->sizeFileCFFILE1==0 && fGetNextCab ) {
    returntrue=TRUE;
  }

  if (!fci_flush_folder(hfci,fGetNextCab,pfnfcignc,pfnfcis)){
    /* TODO set error */
    return FALSE;
  }

  if(returntrue) return TRUE;

  if ( (p_fci_internal->fSplitFolder && p_fci_internal->fNextCab==FALSE)||
       (p_fci_internal->sizeFileCFFOLDER==0 &&
         (p_fci_internal->sizeFileCFFILE1!=0 ||
          p_fci_internal->sizeFileCFFILE2!=0 )
     ) )
  {
      /* error */
      fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
      return FALSE;
  }

  if( p_fci_internal->fNextCab ||
      p_fci_internal->fGetNextCabInVain ) {
    cbReserveCFFolder=p_fci_internal->oldCCAB.cbReserveCFFolder;
    cbReserveCFHeader=p_fci_internal->oldCCAB.cbReserveCFHeader;
    /* safety */
    if (strlen(p_fci_internal->oldCCAB.szCabPath)>=CB_MAX_CAB_PATH ||
        strlen(p_fci_internal->oldCCAB.szCab)>=CB_MAX_CABINET_NAME) {
      /* set error */
      fci_set_error( FCIERR_NONE, ERROR_INVALID_DATA, TRUE );
      return FALSE;
    }
    /* get the full name of the cabinet */
    memcpy(szFileNameCABINET,p_fci_internal->oldCCAB.szCabPath,
      CB_MAX_CAB_PATH);
    memcpy(szFileNameCABINET+strlen(szFileNameCABINET),
      p_fci_internal->oldCCAB.szCab, CB_MAX_CABINET_NAME);
  } else {
    cbReserveCFFolder=p_fci_internal->pccab->cbReserveCFFolder;
    cbReserveCFHeader=p_fci_internal->pccab->cbReserveCFHeader;
    /* safety */
    if (strlen(p_fci_internal->pccab->szCabPath)>=CB_MAX_CAB_PATH ||
        strlen(p_fci_internal->pccab->szCab)>=CB_MAX_CABINET_NAME) {
      /* set error */
      fci_set_error( FCIERR_NONE, ERROR_INVALID_DATA, TRUE );
      return FALSE;
    }
    /* get the full name of the cabinet */
    memcpy(szFileNameCABINET,p_fci_internal->pccab->szCabPath,
      CB_MAX_CAB_PATH);
    memcpy(szFileNameCABINET+strlen(szFileNameCABINET),
      p_fci_internal->pccab->szCab, CB_MAX_CABINET_NAME);
  }

  memcpy(cfheader.signature,"!CAB",4);
  cfheader.reserved1=0;
  cfheader.cbCabinet=   /* size of the cabinet file in bytes */
    sizeof(CFHEADER) +
    p_fci_internal->sizeFileCFFOLDER +
    p_fci_internal->sizeFileCFFILE2 +
    p_fci_internal->sizeFileCFDATA2;

  if (p_fci_internal->fPrevCab) {
    cfheader.cbCabinet+=strlen(p_fci_internal->szPrevCab)+1 +
      strlen(p_fci_internal->szPrevDisk)+1;
  }
  if (p_fci_internal->fNextCab) {
    cfheader.cbCabinet+=strlen(p_fci_internal->pccab->szCab)+1 +
      strlen(p_fci_internal->pccab->szDisk)+1;
  }
  if( p_fci_internal->fNextCab ||
      p_fci_internal->fGetNextCabInVain ) {
    cfheader.cbCabinet+=p_fci_internal->oldCCAB.cbReserveCFHeader;
    if ( p_fci_internal->oldCCAB.cbReserveCFHeader != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFFolder != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFData   != 0 ) {
      cfheader.cbCabinet+=4;
    }
  } else {
    cfheader.cbCabinet+=p_fci_internal->pccab->cbReserveCFHeader;
    if ( p_fci_internal->pccab->cbReserveCFHeader != 0 ||
        p_fci_internal->pccab->cbReserveCFFolder != 0 ||
        p_fci_internal->pccab->cbReserveCFData   != 0 ) {
      cfheader.cbCabinet+=4;
    }
  }

  if( ( ( p_fci_internal->fNextCab ||
          p_fci_internal->fGetNextCabInVain ) &&
        cfheader.cbCabinet > p_fci_internal->oldCCAB.cb
      ) ||
      ( ( p_fci_internal->fNextCab==FALSE &&
          p_fci_internal->fGetNextCabInVain==FALSE ) &&
        cfheader.cbCabinet > p_fci_internal->pccab->cb
      )
    )
  {
    fci_set_error( FCIERR_NONE, ERROR_MORE_DATA, TRUE );
    return FALSE;
  }


  cfheader.reserved2=0;
  cfheader.coffFiles=    /* offset to first CFFILE section */
   cfheader.cbCabinet - p_fci_internal->sizeFileCFFILE2 -
   p_fci_internal->sizeFileCFDATA2;

  cfheader.reserved3=0;
  cfheader.versionMinor=3;
  cfheader.versionMajor=1;
  /* number of CFFOLDER entries in the cabinet */
  cfheader.cFolders=p_fci_internal->cFolders;
  /* number of CFFILE entries in the cabinet */
  cfheader.cFiles=p_fci_internal->cFiles;
  cfheader.flags=0;    /* 1=prev cab, 2=next cabinet, 4=reserved setions */

  if( p_fci_internal->fPrevCab ) {
    cfheader.flags = cfheadPREV_CABINET;
  }

  if( p_fci_internal->fNextCab ) {
    cfheader.flags |= cfheadNEXT_CABINET;
  }

  if( p_fci_internal->fNextCab ||
      p_fci_internal->fGetNextCabInVain ) {
    if( p_fci_internal->oldCCAB.cbReserveCFHeader != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFFolder != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFData   != 0 ) {
      cfheader.flags |= cfheadRESERVE_PRESENT;
    }
    cfheader.setID = p_fci_internal->oldCCAB.setID;
    cfheader.iCabinet = p_fci_internal->oldCCAB.iCab-1;
  } else {
    if( p_fci_internal->pccab->cbReserveCFHeader != 0 ||
        p_fci_internal->pccab->cbReserveCFFolder != 0 ||
        p_fci_internal->pccab->cbReserveCFData   != 0 ) {
      cfheader.flags |= cfheadRESERVE_PRESENT;
    }
    cfheader.setID = p_fci_internal->pccab->setID;
    cfheader.iCabinet = p_fci_internal->pccab->iCab-1;
  }

  /* create the cabinet */
  handleCABINET = PFCI_OPEN(hfci, szFileNameCABINET,
    33538, 384, &err, p_fci_internal->pv );
  if(handleCABINET==0){
    fci_set_error( FCIERR_CAB_FILE, ERROR_OPEN_FAILED, TRUE );
    return FALSE;
  }
  /* TODO error checking of err */

  /* set little endian */
  cfheader.reserved1=fci_endian_ulong(cfheader.reserved1);
  cfheader.cbCabinet=fci_endian_ulong(cfheader.cbCabinet);
  cfheader.reserved2=fci_endian_ulong(cfheader.reserved2);
  cfheader.coffFiles=fci_endian_ulong(cfheader.coffFiles);
  cfheader.reserved3=fci_endian_ulong(cfheader.reserved3);
  cfheader.cFolders=fci_endian_uword(cfheader.cFolders);
  cfheader.cFiles=fci_endian_uword(cfheader.cFiles);
  cfheader.flags=fci_endian_uword(cfheader.flags);
  cfheader.setID=fci_endian_uword(cfheader.setID);
  cfheader.iCabinet=fci_endian_uword(cfheader.iCabinet);

  /* write CFHEADER into cabinet file */
  if( PFCI_WRITE(hfci, handleCABINET, /* file handle */
      &cfheader, /* memory buffer */
      sizeof(cfheader), /* number of bytes to copy */
      &err, p_fci_internal->pv) != sizeof(cfheader) ) {
    /* write error */
    fci_set_error( FCIERR_CAB_FILE, ERROR_WRITE_FAULT, TRUE );
    return FALSE;
  }
  /* TODO error handling of err */

  /* reset little endian */
  cfheader.reserved1=fci_endian_ulong(cfheader.reserved1);
  cfheader.cbCabinet=fci_endian_ulong(cfheader.cbCabinet);
  cfheader.reserved2=fci_endian_ulong(cfheader.reserved2);
  cfheader.coffFiles=fci_endian_ulong(cfheader.coffFiles);
  cfheader.reserved3=fci_endian_ulong(cfheader.reserved3);
  cfheader.cFolders=fci_endian_uword(cfheader.cFolders);
  cfheader.cFiles=fci_endian_uword(cfheader.cFiles);
  cfheader.flags=fci_endian_uword(cfheader.flags);
  cfheader.setID=fci_endian_uword(cfheader.setID);
  cfheader.iCabinet=fci_endian_uword(cfheader.iCabinet);

  if( cfheader.flags & cfheadRESERVE_PRESENT ) {
    /* NOTE: No checks for maximum value overflows as designed by MS!!! */
    cfreserved.cbCFHeader = cbReserveCFHeader;
    cfreserved.cbCFFolder = cbReserveCFFolder;
    if( p_fci_internal->fNextCab ||
        p_fci_internal->fGetNextCabInVain ) {
      cfreserved.cbCFData = p_fci_internal->oldCCAB.cbReserveCFData;
    } else {
      cfreserved.cbCFData = p_fci_internal->pccab->cbReserveCFData;
    }

    /* set little endian */
    cfreserved.cbCFHeader=fci_endian_uword(cfreserved.cbCFHeader);

    /* write reserved info into cabinet file */
    if( PFCI_WRITE(hfci, handleCABINET, /* file handle */
        &cfreserved, /* memory buffer */
        sizeof(cfreserved), /* number of bytes to copy */
        &err, p_fci_internal->pv) != sizeof(cfreserved) ) {
      /* write error */
      fci_set_error( FCIERR_CAB_FILE, ERROR_WRITE_FAULT, TRUE );
      return FALSE;
    }
    /* TODO error handling of err */

    /* reset little endian */
    cfreserved.cbCFHeader=fci_endian_uword(cfreserved.cbCFHeader);
  }

  /* add optional reserved area */
  if (cbReserveCFHeader!=0) {
    if(!(reserved = (char*)PFCI_ALLOC(hfci, cbReserveCFHeader))) {
      fci_set_error( FCIERR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY, TRUE );
      return FALSE;
    }
    for(i=0;i<cbReserveCFHeader;) {
      reserved[i++]='\0';
    }
    if( PFCI_WRITE(hfci, handleCABINET, /* file handle */
        reserved, /* memory buffer */
        cbReserveCFHeader, /* number of bytes to copy */
        &err, p_fci_internal->pv) != cbReserveCFHeader ) {
      PFCI_FREE(hfci, reserved);
      /* write error */
      fci_set_error( FCIERR_CAB_FILE, ERROR_WRITE_FAULT, TRUE );
      return FALSE;
    }
    /* TODO error handling of err */
    PFCI_FREE(hfci, reserved);
  }

  if( cfheader.flags & cfheadPREV_CABINET ) {
    if( PFCI_WRITE(hfci, handleCABINET, /* file handle */
        p_fci_internal->szPrevCab, /* memory buffer */
        strlen(p_fci_internal->szPrevCab)+1, /* number of bytes to copy */
        &err, p_fci_internal->pv) != strlen(p_fci_internal->szPrevCab)+1 ) {
      /* write error */
      fci_set_error( FCIERR_CAB_FILE, ERROR_WRITE_FAULT, TRUE );
      return FALSE;
    }
    /* TODO error handling of err */

    if( PFCI_WRITE(hfci, handleCABINET, /* file handle */
        p_fci_internal->szPrevDisk, /* memory buffer */
        strlen(p_fci_internal->szPrevDisk)+1, /* number of bytes to copy */
        &err, p_fci_internal->pv) != strlen(p_fci_internal->szPrevDisk)+1 ) {
      /* write error */
      fci_set_error( FCIERR_CAB_FILE, ERROR_WRITE_FAULT, TRUE );
      return FALSE;
    }
    /* TODO error handling of err */
  }

  if( cfheader.flags & cfheadNEXT_CABINET ) {
    if( PFCI_WRITE(hfci, handleCABINET, /* file handle */
        p_fci_internal->pccab->szCab, /* memory buffer */
        strlen(p_fci_internal->pccab->szCab)+1, /* number of bytes to copy */
        &err, p_fci_internal->pv) != strlen(p_fci_internal->pccab->szCab)+1 ) {
      /* write error */
      fci_set_error( FCIERR_CAB_FILE, ERROR_WRITE_FAULT, TRUE );
      return FALSE;
    }
    /* TODO error handling of err */

    if( PFCI_WRITE(hfci, handleCABINET, /* file handle */
        p_fci_internal->pccab->szDisk, /* memory buffer */
        strlen(p_fci_internal->pccab->szDisk)+1, /* number of bytes to copy */
        &err, p_fci_internal->pv) != strlen(p_fci_internal->pccab->szDisk)+1 ) {
      /* write error */
      fci_set_error( FCIERR_CAB_FILE, ERROR_WRITE_FAULT, TRUE );
      return FALSE;
    }
    /* TODO error handling of err */
  }

  /* set seek of p_fci_internal->handleCFFOLDER to 0 */
  if( PFCI_SEEK(hfci,p_fci_internal->handleCFFOLDER,
      0, SEEK_SET, &err, p_fci_internal->pv) != 0 ) {
    /* wrong return value */
    fci_set_error( FCIERR_NONE, ERROR_SEEK, TRUE );
    return FALSE;
  }
  /* TODO error handling of err */

  /* while not all CFFOLDER structures have been copied into the cabinet do */
  while(!FALSE) {
    /* use the variable read_result */
    /* read cffolder of p_fci_internal->handleCFFOLDER */
    read_result = PFCI_READ(hfci, p_fci_internal->handleCFFOLDER, /* handle */
        &cffolder, /* memory buffer */
        sizeof(cffolder), /* number of bytes to copy */
        &err, p_fci_internal->pv);
    if( read_result != sizeof(cffolder) ) {
      if( read_result == 0 ) break;/*ALL CFFOLDER structures have been copied*/
      /* read error */
      fci_set_error( FCIERR_NONE, ERROR_READ_FAULT, TRUE );
      return FALSE;
    }
    /* TODO error handling of err */

    /* add size of header size of all CFFOLDERs and size of all CFFILEs */
    cffolder.coffCabStart +=
      p_fci_internal->sizeFileCFFILE2 + p_fci_internal->sizeFileCFFOLDER +
      sizeof(CFHEADER);
    if( p_fci_internal->fNextCab ||
        p_fci_internal->fGetNextCabInVain ) {
      cffolder.coffCabStart+=p_fci_internal->oldCCAB.cbReserveCFHeader;
    } else {
      cffolder.coffCabStart+=p_fci_internal->pccab->cbReserveCFHeader;
    }

    if (p_fci_internal->fPrevCab) {
      cffolder.coffCabStart += strlen(p_fci_internal->szPrevCab)+1 +
        strlen(p_fci_internal->szPrevDisk)+1;
    }

    if (p_fci_internal->fNextCab) {
      cffolder.coffCabStart += strlen(p_fci_internal->oldCCAB.szCab)+1 +
        strlen(p_fci_internal->oldCCAB.szDisk)+1;
    }

    if( p_fci_internal->fNextCab ||
        p_fci_internal->fGetNextCabInVain ) {
      cffolder.coffCabStart += p_fci_internal->oldCCAB.cbReserveCFHeader;
      if( p_fci_internal->oldCCAB.cbReserveCFHeader != 0 ||
          p_fci_internal->oldCCAB.cbReserveCFFolder != 0 ||
          p_fci_internal->oldCCAB.cbReserveCFData   != 0 ) {
        cffolder.coffCabStart += 4;
      }
    } else {
      cffolder.coffCabStart += p_fci_internal->pccab->cbReserveCFHeader;
      if( p_fci_internal->pccab->cbReserveCFHeader != 0 ||
          p_fci_internal->pccab->cbReserveCFFolder != 0 ||
          p_fci_internal->pccab->cbReserveCFData   != 0 ) {
        cffolder.coffCabStart += 4;
      }
    }

    /* set little endian */
    cffolder.coffCabStart=fci_endian_ulong(cffolder.coffCabStart);
    cffolder.cCFData=fci_endian_uword(cffolder.cCFData);
    cffolder.typeCompress=fci_endian_uword(cffolder.typeCompress);

    /* write cffolder to cabinet file */
    if( PFCI_WRITE(hfci, handleCABINET, /* file handle */
        &cffolder, /* memory buffer */
        sizeof(cffolder), /* number of bytes to copy */
        &err, p_fci_internal->pv) != sizeof(cffolder) ) {
      /* write error */
      fci_set_error( FCIERR_CAB_FILE, ERROR_WRITE_FAULT, TRUE );
      return FALSE;
    }
    /* TODO error handling of err */

    /* reset little endian */
    cffolder.coffCabStart=fci_endian_ulong(cffolder.coffCabStart);
    cffolder.cCFData=fci_endian_uword(cffolder.cCFData);
    cffolder.typeCompress=fci_endian_uword(cffolder.typeCompress);

    /* add optional reserved area */

    /* This allocation and freeing at each CFFolder block is a bit */
    /* inefficent, but it's harder to forget about freeing the buffer :-). */
    /* Reserved areas are used seldom besides that... */
    if (cbReserveCFFolder!=0) {
      if(!(reserved = PFCI_ALLOC(hfci, cbReserveCFFolder))) {
        fci_set_error( FCIERR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY, TRUE );
        return FALSE;
      }

      if( PFCI_READ(hfci, p_fci_internal->handleCFFOLDER, /* file handle */
          reserved, /* memory buffer */
          cbReserveCFFolder, /* number of bytes to copy */
          &err, p_fci_internal->pv) != cbReserveCFFolder ) {
        PFCI_FREE(hfci, reserved);
        /* read error */
        fci_set_error( FCIERR_NONE, ERROR_READ_FAULT, TRUE );
        return FALSE;
      }
      /* TODO error handling of err */

      if( PFCI_WRITE(hfci, handleCABINET, /* file handle */
          reserved, /* memory buffer */
          cbReserveCFFolder, /* number of bytes to copy */
          &err, p_fci_internal->pv) != cbReserveCFFolder ) {
        PFCI_FREE(hfci, reserved);
        /* write error */
        fci_set_error( FCIERR_CAB_FILE, ERROR_WRITE_FAULT, TRUE );
        return FALSE;
      }
      /* TODO error handling of err */

      PFCI_FREE(hfci, reserved);
    }

  } /* END OF while */

  /* set seek of p_fci_internal->handleCFFILE2 to 0 */
  if( PFCI_SEEK(hfci,p_fci_internal->handleCFFILE2,
      0, SEEK_SET, &err, p_fci_internal->pv) != 0 ) {
    /* wrong return value */
    fci_set_error( FCIERR_NONE, ERROR_SEEK, TRUE );
    return FALSE;
  }
  /* TODO error handling of err */

  /* while not all CFFILE structures have been copied to the cabinet do */
  while(!FALSE) {
    /* REUSE the variable read_result */
    /* REUSE the buffer p_fci_internal->data_out AGAIN */
    /* read a block from p_fci_internal->handleCFFILE2 */
    read_result = PFCI_READ(hfci, p_fci_internal->handleCFFILE2 /* handle */,
        p_fci_internal->data_out, /* memory buffer */
        CB_MAX_CHUNK, /* number of bytes to copy */
        &err, p_fci_internal->pv);
    if( read_result == 0 ) break; /* ALL CFFILE structures have been copied */
    /* TODO error handling of err */

    /* write the block to the cabinet file */
    if( PFCI_WRITE(hfci, handleCABINET, /* file handle */
        p_fci_internal->data_out, /* memory buffer */
        read_result, /* number of bytes to copy */
        &err, p_fci_internal->pv) != read_result ) {
      /* write error */
      fci_set_error( FCIERR_CAB_FILE, ERROR_WRITE_FAULT, TRUE );
      return FALSE;
    }
    /* TODO error handling of err */

    if (p_fci_internal->fSplitFolder==FALSE) {
      p_fci_internal->statusFolderCopied = 0;
      /* TODO TEST THIS further */
      p_fci_internal->statusFolderTotal = p_fci_internal->sizeFileCFDATA2+
        p_fci_internal->sizeFileCFFILE2;
    }
    p_fci_internal->statusFolderCopied += read_result;

    /* report status with pfnfcis about copied size of folder */
    if( (*pfnfcis)(statusFolder,
        p_fci_internal->statusFolderCopied, /* length of copied blocks */
        p_fci_internal->statusFolderTotal, /* total size of folder */
        p_fci_internal->pv) == -1) {
      fci_set_error( FCIERR_USER_ABORT, 0, TRUE );
      return FALSE;
    }

  } /* END OF while */

  /* set seek of p_fci_internal->handleCFDATA2 to 0 */
  if( PFCI_SEEK(hfci,p_fci_internal->handleCFDATA2,
      0, SEEK_SET, &err, p_fci_internal->pv) != 0 ) {
    /* wrong return value */
    fci_set_error( FCIERR_NONE, ERROR_SEEK, TRUE );
    return FALSE;
  }
  /* TODO error handling of err */

  /* reset the number of folders for the next cabinet */
  p_fci_internal->cFolders=0;
  /* reset the number of files for the next cabinet */
  p_fci_internal->cFiles=0;

  /* while not all CFDATA structures have been copied to the cabinet do */
  while(!FALSE) {
    /* REUSE the variable read_result AGAIN */
    /* REUSE the buffer p_fci_internal->data_out AGAIN */
    /* read a block from p_fci_internal->handleCFDATA2 */
    read_result = PFCI_READ(hfci, p_fci_internal->handleCFDATA2 /* handle */,
        p_fci_internal->data_out, /* memory buffer */
        CB_MAX_CHUNK, /* number of bytes to copy */
        &err, p_fci_internal->pv);
    if( read_result == 0 ) break; /* ALL CFDATA structures have been copied */
    /* TODO error handling of err */

    /* write the block to the cabinet file */
    if( PFCI_WRITE(hfci, handleCABINET, /* file handle */
        p_fci_internal->data_out, /* memory buffer */
        read_result, /* number of bytes to copy */
        &err, p_fci_internal->pv) != read_result ) {
      /* write error */
      fci_set_error( FCIERR_CAB_FILE, ERROR_WRITE_FAULT, TRUE );
      return FALSE;
    }
    /* TODO error handling of err */

    p_fci_internal->statusFolderCopied += read_result;
    /* report status with pfnfcis about copied size of folder */
    if( (*pfnfcis)(statusFolder,
        p_fci_internal->statusFolderCopied, /* length of copied blocks */
        p_fci_internal->statusFolderTotal, /* total size of folder */
        p_fci_internal->pv) == -1) {
      /* set error code and abort */
      fci_set_error( FCIERR_USER_ABORT, 0, TRUE );
      return FALSE;
    }
  } /* END OF while */

  /* set seek of the cabinet file to 0 */
  if( PFCI_SEEK(hfci, handleCABINET,
      0, SEEK_SET, &err, p_fci_internal->pv) != 0 ) {
    /* wrong return value */
    fci_set_error( FCIERR_NONE, ERROR_SEEK, TRUE );
    return FALSE;
  }
  /* TODO error handling of err */

  /* write the signature "MSCF" into the cabinet file */
  memcpy( cfheader.signature, "MSCF", 4 );
  if( PFCI_WRITE(hfci, handleCABINET, /* file handle */
      &cfheader, /* memory buffer */
      4, /* number of bytes to copy */
      &err, p_fci_internal->pv) != 4 ) {
    /* wrtie error */
    fci_set_error( FCIERR_CAB_FILE, ERROR_WRITE_FAULT, TRUE );
    return FALSE;
  }
  /* TODO error handling of err */

  /* close the cabinet file */
  PFCI_CLOSE(hfci,handleCABINET,&err,p_fci_internal->pv);
  /* TODO error handling of err */


/* COPIED FROM FCIDestroy */

  PFCI_CLOSE (hfci, p_fci_internal->handleCFDATA2,&err,p_fci_internal->pv);
  /* TODO error handling of err */
  PFCI_DELETE(hfci, p_fci_internal->szFileNameCFDATA2, &err,
    p_fci_internal->pv);
  /* TODO error handling of err */
  PFCI_CLOSE (hfci, p_fci_internal->handleCFFILE2,&err,p_fci_internal->pv);
  /* TODO error handling of err */
  PFCI_DELETE(hfci, p_fci_internal->szFileNameCFFILE2, &err,
    p_fci_internal->pv);
  /* TODO error handling of err */
  PFCI_CLOSE (hfci, p_fci_internal->handleCFFOLDER,&err,p_fci_internal->pv);
  /* TODO error handling of err */
  PFCI_DELETE(hfci, p_fci_internal->szFileNameCFFOLDER, &err,
    p_fci_internal->pv);
  /* TODO error handling of err */

/* END OF copied from FCIDestroy */

  /* get 3 temporary files and open them */
  /* write names and handles to hfci */


  p_fci_internal->sizeFileCFDATA2  = 0;
  p_fci_internal->sizeFileCFFILE2  = 0;
  p_fci_internal->sizeFileCFFOLDER = 0;

/* COPIED FROM FCICreate */

  /* CFDATA with checksum and ready to be copied into cabinet */
  if( !PFCI_GETTEMPFILE(hfci, p_fci_internal->szFileNameCFDATA2,
      CB_MAX_FILENAME)) {
    /* error handling */
    fci_set_error( FCIERR_NONE, ERROR_FUNCTION_FAILED, TRUE );
    return FALSE;
  }
  /* safety */
  if ( strlen(p_fci_internal->szFileNameCFDATA2) >= CB_MAX_FILENAME ) {
    /* set error code and abort */
    fci_set_error( FCIERR_NONE, ERROR_INVALID_DATA, TRUE );
    return FALSE;
  }
  p_fci_internal->handleCFDATA2 = PFCI_OPEN(hfci,
    p_fci_internal->szFileNameCFDATA2, 34050, 384, &err, p_fci_internal->pv);
  /* check handle */
  if(p_fci_internal->handleCFDATA2==0){
    fci_set_error( FCIERR_TEMP_FILE, ERROR_OPEN_FAILED, TRUE );
    return FALSE;
  }
  /* TODO error checking of err */

  /* array of all CFFILE in a folder, ready to be copied into cabinet */
  if( !PFCI_GETTEMPFILE(hfci,p_fci_internal->szFileNameCFFILE2,
      CB_MAX_FILENAME)) {
    /* error handling */
    fci_set_error( FCIERR_NONE, ERROR_FUNCTION_FAILED, TRUE );
    return FALSE;
  }
  /* safety */
  if ( strlen(p_fci_internal->szFileNameCFFILE2) >= CB_MAX_FILENAME ) {
    /* set error code and abort */
    fci_set_error( FCIERR_NONE, ERROR_INVALID_DATA, TRUE );
    return FALSE;
  }
  p_fci_internal->handleCFFILE2 = PFCI_OPEN(hfci,
    p_fci_internal->szFileNameCFFILE2, 34050, 384, &err, p_fci_internal->pv);
  /* check handle */
  if(p_fci_internal->handleCFFILE2==0){
    fci_set_error( FCIERR_TEMP_FILE, ERROR_OPEN_FAILED, TRUE);
    return FALSE;
  }
  /* TODO error checking of err */

  /* array of all CFFILE in a folder, ready to be copied into cabinet */
  if (!PFCI_GETTEMPFILE(hfci,p_fci_internal->szFileNameCFFOLDER,CB_MAX_FILENAME)) {
    /* error handling */
    fci_set_error( FCIERR_NONE, ERROR_FUNCTION_FAILED, TRUE );
    return FALSE;
  }
  /* safety */
  if ( strlen(p_fci_internal->szFileNameCFFOLDER) >= CB_MAX_FILENAME ) {
    /* set error code and abort */
    fci_set_error( FCIERR_NONE, ERROR_INVALID_DATA, TRUE );
    return FALSE;
  }
  p_fci_internal->handleCFFOLDER = PFCI_OPEN(hfci,
    p_fci_internal->szFileNameCFFOLDER, 34050, 384, &err, p_fci_internal->pv);
  /* check handle */
  if(p_fci_internal->handleCFFOLDER==0){
    fci_set_error( FCIERR_TEMP_FILE, ERROR_OPEN_FAILED, TRUE );
    return FALSE;
  }
  /* TODO error checking of err */

/* END OF copied from FCICreate */


  /* TODO close and delete new files when return FALSE */


  /* report status with pfnfcis about copied size of folder */
  (*pfnfcis)(statusCabinet,
    p_fci_internal->estimatedCabinetSize, /* estimated cabinet file size */
    cfheader.cbCabinet /* real cabinet file size */, p_fci_internal->pv);

  p_fci_internal->fPrevCab=TRUE;
  /* The sections szPrevCab and szPrevDisk are not being updated, because */
  /* MS CABINET.DLL always puts the first cabinet name and disk into them */

  if (p_fci_internal->fNextCab) {
    p_fci_internal->fNextCab=FALSE;

    if (p_fci_internal->sizeFileCFFILE1==0 && p_fci_internal->sizeFileCFDATA1!=0) {
      /* THIS CAN NEVER HAPPEN */
      /* set error code */
      fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
      return FALSE;
    }

/* COPIED FROM FCIAddFile and modified */

    /* REUSE the variable read_result */
    if (p_fci_internal->fGetNextCabInVain) {
      read_result=p_fci_internal->oldCCAB.cbReserveCFHeader;
      if(p_fci_internal->sizeFileCFFILE1!=0) {
        read_result+=p_fci_internal->oldCCAB.cbReserveCFFolder;
      }
      if ( p_fci_internal->oldCCAB.cbReserveCFHeader != 0 ||
          p_fci_internal->oldCCAB.cbReserveCFFolder != 0 ||
          p_fci_internal->oldCCAB.cbReserveCFData   != 0 ) {
        read_result+=4;
      }
    } else {
      read_result=p_fci_internal->pccab->cbReserveCFHeader;
      if(p_fci_internal->sizeFileCFFILE1!=0) {
        read_result+=p_fci_internal->pccab->cbReserveCFFolder;
      }
      if ( p_fci_internal->pccab->cbReserveCFHeader != 0 ||
          p_fci_internal->pccab->cbReserveCFFolder != 0 ||
          p_fci_internal->pccab->cbReserveCFData   != 0 ) {
        read_result+=4;
      }
    }
    if ( p_fci_internal->fPrevCab ) {
      read_result+= strlen(p_fci_internal->szPrevCab)+1+
        strlen(p_fci_internal->szPrevDisk)+1;
    }
    read_result+= p_fci_internal->sizeFileCFDATA1 +
      p_fci_internal->sizeFileCFFILE1 + p_fci_internal->sizeFileCFDATA2 +
      p_fci_internal->sizeFileCFFILE2 + p_fci_internal->sizeFileCFFOLDER +
      sizeof(CFHEADER) +
      sizeof(CFFOLDER); /* set size of new CFFolder entry */

    if( p_fci_internal->fNewPrevious ) {
      memcpy(p_fci_internal->szPrevCab, p_fci_internal->oldCCAB.szCab,
        CB_MAX_CABINET_NAME);
      memcpy(p_fci_internal->szPrevDisk, p_fci_internal->oldCCAB.szDisk,
        CB_MAX_DISK_NAME);
      p_fci_internal->fNewPrevious=FALSE;
    }

    /* too much data for the maximum size of a cabinet */
    if( p_fci_internal->fGetNextCabInVain==FALSE &&
        p_fci_internal->pccab->cb < read_result ) {
      return fci_flush_cabinet( hfci, FALSE, pfnfcignc, pfnfcis);
    }

    /* Might be too much data for the maximum size of a cabinet.*/
    /* When any further data will be added later, it might not */
    /* be possible to flush the cabinet, because there might */
    /* not be enough space to store the name of the following */
    /* cabinet and name of the corresponding disk. */
    /* So take care of this and get the name of the next cabinet */
    if (p_fci_internal->fGetNextCabInVain==FALSE && (
      p_fci_internal->pccab->cb < read_result +
      CB_MAX_CABINET_NAME + CB_MAX_DISK_NAME
    )) {
      /* save CCAB */
      memcpy(&(p_fci_internal->oldCCAB), p_fci_internal->pccab, sizeof(CCAB));
      /* increment cabinet index */
      ++(p_fci_internal->pccab->iCab);
      /* get name of next cabinet */
      p_fci_internal->estimatedCabinetSize=p_fci_internal->statusFolderTotal;
      if (!(*pfnfcignc)(p_fci_internal->pccab,
          p_fci_internal->estimatedCabinetSize, /* estimated size of cab */
          p_fci_internal->pv)) {
        /* error handling */
        fci_set_error( FCIERR_NONE, ERROR_FUNCTION_FAILED, TRUE );
        return FALSE;
      }
      /* Skip a few lines of code. This is caught by the next if. */
      p_fci_internal->fGetNextCabInVain=TRUE;
    }

    /* too much data for cabinet */
    if (p_fci_internal->fGetNextCabInVain && (
        p_fci_internal->oldCCAB.cb < read_result +
        strlen(p_fci_internal->oldCCAB.szCab)+1+
        strlen(p_fci_internal->oldCCAB.szDisk)+1
    )) {
      p_fci_internal->fGetNextCabInVain=FALSE;
      p_fci_internal->fNextCab=TRUE;
      return fci_flush_cabinet( hfci, FALSE, pfnfcignc, pfnfcis);
    }

    /* if the FolderThreshold has been reached flush the folder automatically */
    if( p_fci_internal->fGetNextCabInVain ) {
      if( p_fci_internal->cCompressedBytesInFolder >=
          p_fci_internal->oldCCAB.cbFolderThresh) {
        return FCIFlushFolder(hfci, pfnfcignc, pfnfcis);
      }
    } else {
      if( p_fci_internal->cCompressedBytesInFolder >=
          p_fci_internal->pccab->cbFolderThresh) {
        return FCIFlushFolder(hfci, pfnfcignc, pfnfcis);
      }
    }

/* END OF COPIED FROM FCIAddFile and modified */

    if( p_fci_internal->sizeFileCFFILE1>0 ) {
      if( !FCIFlushFolder(hfci, pfnfcignc, pfnfcis) ) return FALSE;
      p_fci_internal->fNewPrevious=TRUE;
    }
  } else {
    p_fci_internal->fNewPrevious=FALSE;
    if( p_fci_internal->sizeFileCFFILE1>0 || p_fci_internal->sizeFileCFDATA1) {
      /* THIS MAY NEVER HAPPEN */
      /* set error structures */
      fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
      return FALSE;
    }
  }

  return TRUE;
} /* end of fci_flush_cabinet */





/***********************************************************************
 *		FCIAddFile (CABINET.11)
 *
 * FCIAddFile adds a file to the to be created cabinet file
 *
 * PARAMS
 *   hfci          [I]  An HFCI from FCICreate
 *   pszSourceFile [I]  A pointer to a C string which contains the name and
 *                      location of the file which will be added to the cabinet
 *   pszFileName   [I]  A pointer to a C string which contains the name under
 *                      which the file will be stored in the cabinet
 *   fExecute      [I]  A boolean value which indicates if the file should be
 *                      executed after extraction of self extracting
 *                      executables
 *   pfnfcignc     [I]  A pointer to a function which gets information about
 *                      the next cabinet
 *   pfnfcis      [IO]  A pointer to a function which will report status
 *                      information about the compression process
 *   pfnfcioi      [I]  A pointer to a function which reports file attributes
 *                      and time and date information
 *   typeCompress  [I]  Compression type
 *
 * RETURNS
 *   On success, returns TRUE
 *   On failure, returns FALSE
 *
 * INCLUDES
 *   fci.h
 *
 */
BOOL __cdecl FCIAddFile(
	HFCI                  hfci,
	char                 *pszSourceFile,
	char                 *pszFileName,
	BOOL                  fExecute,
	PFNFCIGETNEXTCABINET  pfnfcignc,
	PFNFCISTATUS          pfnfcis,
	PFNFCIGETOPENINFO     pfnfcigoi,
	TCOMP                 typeCompress)
{
  int err;
  CFFILE cffile;
  cab_ULONG read_result;
  int file_handle;
  PFCI_Int p_fci_internal=((PFCI_Int)(hfci));

  /* test hfci */
  if (!REALLY_IS_FCI(hfci)) {
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  if ((!pszSourceFile) || (!pszFileName) || (!pfnfcignc) || (!pfnfcis) ||
      (!pfnfcigoi) || strlen(pszFileName)>=CB_MAX_FILENAME) {
    fci_set_error( FCIERR_NONE, ERROR_BAD_ARGUMENTS, TRUE );
    return FALSE;
  }

  /* TODO check if pszSourceFile??? */

  if(p_fci_internal->fGetNextCabInVain && p_fci_internal->fNextCab) {
    /* internal error */
    fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
    return FALSE;
  }

  if(p_fci_internal->fNextCab) {
    /* internal error */
    fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
    return FALSE;
  }

  cffile.cbFile=0; /* size of the to be added file*/
  /* offset of the uncompressed file in the folder */
  cffile.uoffFolderStart=p_fci_internal->cDataBlocks*CAB_BLOCKMAX + p_fci_internal->cdata_in;
  /* number of folder in the cabinet or special 0=first  */
  cffile.iFolder = p_fci_internal->cFolders;

  /* allocation of memory */
  if (p_fci_internal->data_in==NULL) {
    if (p_fci_internal->cdata_in!=0) {
      /* error handling */
      fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
      return FALSE;
    }
    if (p_fci_internal->data_out!=NULL) {
      /* error handling */
      fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
      return FALSE;
    }
    if(!(p_fci_internal->data_in = (char*)PFCI_ALLOC(hfci,CB_MAX_CHUNK))) {
      fci_set_error( FCIERR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY, TRUE );
      return FALSE;
    }
    if (p_fci_internal->data_out==NULL) {
      if(!(p_fci_internal->data_out = PFCI_ALLOC(hfci, 2 * CB_MAX_CHUNK))){
        fci_set_error( FCIERR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY, TRUE );
        return FALSE;
      }
    }
  }

  if (p_fci_internal->data_out==NULL) {
    PFCI_FREE(hfci,p_fci_internal->data_in);
    /* error handling */
    fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
    return FALSE;
  }

  /* get information about the file */
  file_handle=(*pfnfcigoi)(pszSourceFile, &(cffile.date), &(cffile.time),
    &(cffile.attribs), &err, p_fci_internal->pv);
  /* check file_handle */
  if(file_handle==0){
    fci_set_error( FCIERR_OPEN_SRC, ERROR_OPEN_FAILED, TRUE );
  }
  /* TODO error handling of err */

  if (fExecute) { cffile.attribs |= _A_EXEC; }

  /* REUSE the variable read_result */
  if (p_fci_internal->fGetNextCabInVain) {
    read_result=p_fci_internal->oldCCAB.cbReserveCFHeader +
      p_fci_internal->oldCCAB.cbReserveCFFolder;
    if ( p_fci_internal->oldCCAB.cbReserveCFHeader != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFFolder != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFData   != 0 ) {
      read_result+=4;
    }
  } else {
    read_result=p_fci_internal->pccab->cbReserveCFHeader +
      p_fci_internal->pccab->cbReserveCFFolder;
    if ( p_fci_internal->pccab->cbReserveCFHeader != 0 ||
        p_fci_internal->pccab->cbReserveCFFolder != 0 ||
        p_fci_internal->pccab->cbReserveCFData   != 0 ) {
      read_result+=4;
    }
  }
  if ( p_fci_internal->fPrevCab ) {
    read_result+= strlen(p_fci_internal->szPrevCab)+1+
      strlen(p_fci_internal->szPrevDisk)+1;
  }
  if ( p_fci_internal->fNextCab ) { /* this is never the case */
    read_result+= strlen(p_fci_internal->pccab->szCab)+1+
      strlen(p_fci_internal->pccab->szDisk)+1;
  }

  read_result+= sizeof(CFFILE) + strlen(pszFileName)+1 +
    p_fci_internal->sizeFileCFFILE1 + p_fci_internal->sizeFileCFDATA2 +
    p_fci_internal->sizeFileCFFILE2 + p_fci_internal->sizeFileCFFOLDER +
    sizeof(CFHEADER) +
    sizeof(CFFOLDER); /* size of new CFFolder entry */

  /* Might be too much data for the maximum size of a cabinet.*/
  /* When any further data will be added later, it might not */
  /* be possible to flush the cabinet, because there might */
  /* not be enough space to store the name of the following */
  /* cabinet and name of the corresponding disk. */
  /* So take care of this and get the name of the next cabinet */
  if( p_fci_internal->fGetNextCabInVain==FALSE &&
      p_fci_internal->fNextCab==FALSE &&
      ( p_fci_internal->pccab->cb < read_result +
        CB_MAX_CABINET_NAME + CB_MAX_DISK_NAME
      )
  ) {
    /* save CCAB */
    memcpy(&(p_fci_internal->oldCCAB), p_fci_internal->pccab, sizeof(CCAB));
    /* increment cabinet index */
    ++(p_fci_internal->pccab->iCab);
    /* get name of next cabinet */
    p_fci_internal->estimatedCabinetSize=p_fci_internal->statusFolderTotal;
    if (!(*pfnfcignc)(p_fci_internal->pccab,
        p_fci_internal->estimatedCabinetSize, /* estimated size of cab */
        p_fci_internal->pv)) {
      /* error handling */
      fci_set_error( FCIERR_NONE, ERROR_FUNCTION_FAILED, TRUE );
      return FALSE;
    }
    /* Skip a few lines of code. This is caught by the next if. */
    p_fci_internal->fGetNextCabInVain=TRUE;
  }

  if( p_fci_internal->fGetNextCabInVain &&
      p_fci_internal->fNextCab
  ) {
    /* THIS CAN NEVER HAPPEN */
    /* set error code */
    fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
    return FALSE;
  }

  /* too much data for cabinet */
  if( p_fci_internal->fGetNextCabInVain &&
     (
      p_fci_internal->oldCCAB.cb < read_result +
      strlen(p_fci_internal->pccab->szCab)+1+
      strlen(p_fci_internal->pccab->szDisk)+1
  )) {
    p_fci_internal->fGetNextCabInVain=FALSE;
    p_fci_internal->fNextCab=TRUE;
    if(!fci_flush_cabinet( hfci, FALSE, pfnfcignc, pfnfcis)) return FALSE;
  }

  if( p_fci_internal->fNextCab ) {
    /* THIS MAY NEVER HAPPEN */
    /* set error code */
    fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
    return FALSE;
  }

  /* read the contents of the file blockwise */
  while (!FALSE) {
    if (p_fci_internal->cdata_in > CAB_BLOCKMAX) {
      /* internal error */
      fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
      return FALSE;
    }

    read_result = PFCI_READ(hfci, file_handle /* file handle */,
      (p_fci_internal->data_in + p_fci_internal->cdata_in) /* memory buffer */,
      (CAB_BLOCKMAX - p_fci_internal->cdata_in) /* number of bytes to copy */,
      &err, p_fci_internal->pv);
    /* TODO error handling of err */

    if( read_result==0 ) break;

    /* increment the block size */
    p_fci_internal->cdata_in += read_result;

    /* increment the file size */
    cffile.cbFile += read_result;

    if ( p_fci_internal->cdata_in > CAB_BLOCKMAX ) {
      /* report internal error */
      fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
      return FALSE;
    }
    /* write a whole block */
    if ( p_fci_internal->cdata_in == CAB_BLOCKMAX ) {

      if( !fci_flush_data_block(hfci, &err, pfnfcis) ) return FALSE;
    }
  }

  /* close the file from FCIAddFile */
  PFCI_CLOSE(hfci,file_handle,&err,p_fci_internal->pv);
  /* TODO error handling of err */

  /* write cffile to p_fci_internal->handleCFFILE1 */
  if( PFCI_WRITE(hfci, p_fci_internal->handleCFFILE1, /* file handle */
      &cffile, sizeof(cffile),&err, p_fci_internal->pv) != sizeof(cffile) ) {
    /* write error */
    fci_set_error( FCIERR_TEMP_FILE, ERROR_WRITE_FAULT, TRUE );
    return FALSE;
  }
  /* TODO error handling of err */

  p_fci_internal->sizeFileCFFILE1 += sizeof(cffile);

  /* append the name of file */
  if (strlen(pszFileName)>=CB_MAX_FILENAME) {
    /* IMPOSSIBLE */
    /* set error code */
    fci_set_error( FCIERR_NONE, ERROR_INVALID_DATA, TRUE );
    return FALSE;
  }
  if( PFCI_WRITE(hfci, p_fci_internal->handleCFFILE1, /* file handle */
      pszFileName, strlen(pszFileName)+1, &err, p_fci_internal->pv)
      != strlen(pszFileName)+1 ) {
    /* write error */
    fci_set_error( FCIERR_TEMP_FILE, ERROR_WRITE_FAULT, TRUE );
    return FALSE;
  }
  /* TODO error handling of err */

  p_fci_internal->sizeFileCFFILE1 += strlen(pszFileName)+1;

  /* REUSE the variable read_result */
  if (p_fci_internal->fGetNextCabInVain ||
      p_fci_internal->fNextCab
     ) {
    read_result=p_fci_internal->oldCCAB.cbReserveCFHeader +
      p_fci_internal->oldCCAB.cbReserveCFFolder;
    if ( p_fci_internal->oldCCAB.cbReserveCFHeader != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFFolder != 0 ||
        p_fci_internal->oldCCAB.cbReserveCFData   != 0 ) {
      read_result+=4;
    }
  } else {
    read_result=p_fci_internal->pccab->cbReserveCFHeader +
      p_fci_internal->pccab->cbReserveCFFolder;
    if ( p_fci_internal->pccab->cbReserveCFHeader != 0 ||
        p_fci_internal->pccab->cbReserveCFFolder != 0 ||
        p_fci_internal->pccab->cbReserveCFData   != 0 ) {
      read_result+=4;
    }
  }
  if ( p_fci_internal->fPrevCab ) {
    read_result+= strlen(p_fci_internal->szPrevCab)+1+
      strlen(p_fci_internal->szPrevDisk)+1;
  }
  if ( p_fci_internal->fNextCab ) { /* this is never the case */
    read_result+= strlen(p_fci_internal->pccab->szCab)+1+
      strlen(p_fci_internal->pccab->szDisk)+1;
  }
  read_result+= p_fci_internal->sizeFileCFDATA1 +
    p_fci_internal->sizeFileCFFILE1 + p_fci_internal->sizeFileCFDATA2 +
    p_fci_internal->sizeFileCFFILE2 + p_fci_internal->sizeFileCFFOLDER +
    sizeof(CFHEADER) +
    sizeof(CFFOLDER); /* set size of new CFFolder entry */

  /* too much data for the maximum size of a cabinet */
  /* (ignoring the unflushed data block) */
  if( p_fci_internal->fGetNextCabInVain==FALSE &&
      p_fci_internal->fNextCab==FALSE && /* this is always the case */
      p_fci_internal->pccab->cb < read_result ) {
    return fci_flush_cabinet( hfci, FALSE, pfnfcignc, pfnfcis);
  }

  /* Might be too much data for the maximum size of a cabinet.*/
  /* When any further data will be added later, it might not */
  /* be possible to flush the cabinet, because there might */
  /* not be enough space to store the name of the following */
  /* cabinet and name of the corresponding disk. */
  /* So take care of this and get the name of the next cabinet */
  /* (ignoring the unflushed data block) */
  if( p_fci_internal->fGetNextCabInVain==FALSE &&
      p_fci_internal->fNextCab==FALSE &&
      ( p_fci_internal->pccab->cb < read_result +
        CB_MAX_CABINET_NAME + CB_MAX_DISK_NAME
      )
  ) {
    /* save CCAB */
    memcpy(&(p_fci_internal->oldCCAB), p_fci_internal->pccab, sizeof(CCAB));
    /* increment cabinet index */
    ++(p_fci_internal->pccab->iCab);
    /* get name of next cabinet */
    p_fci_internal->estimatedCabinetSize=p_fci_internal->statusFolderTotal;
    if (!(*pfnfcignc)(p_fci_internal->pccab,
        p_fci_internal->estimatedCabinetSize,/* estimated size of cab */
        p_fci_internal->pv)) {
      /* error handling */
      fci_set_error( FCIERR_NONE, ERROR_FUNCTION_FAILED, TRUE );
      return FALSE;
    }
    /* Skip a few lines of code. This is caught by the next if. */
    p_fci_internal->fGetNextCabInVain=TRUE;
  }

  if( p_fci_internal->fGetNextCabInVain &&
      p_fci_internal->fNextCab
  ) {
    /* THIS CAN NEVER HAPPEN */
    fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
    return FALSE;
  }

  /* too much data for cabinet */
  if( (p_fci_internal->fGetNextCabInVain ||
      p_fci_internal->fNextCab) && (
      p_fci_internal->oldCCAB.cb < read_result +
      strlen(p_fci_internal->pccab->szCab)+1+
      strlen(p_fci_internal->pccab->szDisk)+1
  )) {

    p_fci_internal->fGetNextCabInVain=FALSE;
    p_fci_internal->fNextCab=TRUE;
    return fci_flush_cabinet( hfci, FALSE, pfnfcignc, pfnfcis);
  }

  if( p_fci_internal->fNextCab ) {
    /* THIS MAY NEVER HAPPEN */
    /* set error code */
    fci_set_error( FCIERR_NONE, ERROR_GEN_FAILURE, TRUE );
    return FALSE;
  }

  /* if the FolderThreshold has been reached flush the folder automatically */
  if( p_fci_internal->fGetNextCabInVain ) {
    if( p_fci_internal->cCompressedBytesInFolder >=
        p_fci_internal->oldCCAB.cbFolderThresh) {
      return FCIFlushFolder(hfci, pfnfcignc, pfnfcis);
    }
  } else {
    if( p_fci_internal->cCompressedBytesInFolder >=
        p_fci_internal->pccab->cbFolderThresh) {
      return FCIFlushFolder(hfci, pfnfcignc, pfnfcis);
    }
  }

  return TRUE;
} /* end of FCIAddFile */





/***********************************************************************
 *		FCIFlushFolder (CABINET.12)
 *
 * FCIFlushFolder completes the CFFolder structure under construction.
 *
 * All further data which is added by FCIAddFile will be associateed to
 * the next CFFolder structure.
 *
 * FCIFlushFolder will be called by FCIAddFile automatically if the
 * threshold (stored in the member cbFolderThresh of the CCAB structure
 * pccab passed to FCICreate) is exceeded.
 *
 * FCIFlushFolder will be called by FCIFlushFolder automatically before
 * any data will be written into the cabinet file.
 *
 * PARAMS
 *   hfci          [I]  An HFCI from FCICreate
 *   pfnfcignc     [I]  A pointer to a function which gets information about
 *                      the next cabinet
 *   pfnfcis      [IO]  A pointer to a function which will report status
 *                      information about the compression process
 *
 * RETURNS
 *   On success, returns TRUE
 *   On failure, returns FALSE
 *
 * INCLUDES
 *   fci.h
 *
 */
BOOL __cdecl FCIFlushFolder(
	HFCI                  hfci,
	PFNFCIGETNEXTCABINET  pfnfcignc,
	PFNFCISTATUS          pfnfcis)
{
  return fci_flush_folder(hfci,FALSE,pfnfcignc,pfnfcis);
} /* end of FCIFlushFolder */



/***********************************************************************
 *		FCIFlushCabinet (CABINET.13)
 *
 * FCIFlushCabinet stores the data which has been added by FCIAddFile
 * into the cabinet file. If the maximum cabinet size (stored in the
 * member cb of the CCAB structure pccab passed to FCICreate) has been
 * exceeded FCIFlushCabinet will be called automatic by FCIAddFile.
 * The remaining data still has to be flushed manually by calling
 * FCIFlushCabinet.
 *
 * After FCIFlushCabinet has been called (manually) FCIAddFile must
 * NOT be called again. Then hfci has to be released by FCIDestroy.
 *
 * PARAMS
 *   hfci          [I]  An HFCI from FCICreate
 *   fGetNextCab   [I]  Whether you want to add additional files to a
 *                      cabinet set (TRUE) or whether you want to
 *                      finalize it (FALSE)
 *   pfnfcignc     [I]  A pointer to a function which gets information about
 *                      the next cabinet
 *   pfnfcis      [IO]  A pointer to a function which will report status
 *                      information about the compression process
 *
 * RETURNS
 *   On success, returns TRUE
 *   On failure, returns FALSE
 *
 * INCLUDES
 *   fci.h
 *
 */
BOOL __cdecl FCIFlushCabinet(
	HFCI                  hfci,
	BOOL                  fGetNextCab,
	PFNFCIGETNEXTCABINET  pfnfcignc,
	PFNFCISTATUS          pfnfcis)
{
  PFCI_Int p_fci_internal=((PFCI_Int)(hfci));

  if(!fci_flush_cabinet(hfci,fGetNextCab,pfnfcignc,pfnfcis)) return FALSE;

  while( p_fci_internal->sizeFileCFFILE1>0 ||
         p_fci_internal->sizeFileCFFILE2>0 ) {
    if(!fci_flush_cabinet(hfci,fGetNextCab,pfnfcignc,pfnfcis)) return FALSE;
  }

  return TRUE;
} /* end of FCIFlushCabinet */


/***********************************************************************
 *		FCIDestroy (CABINET.14)
 *
 * Frees a handle created by FCICreate.
 * Only reason for failure would be an invalid handle.
 *
 * PARAMS
 *   hfci [I] The HFCI to free
 *
 * RETURNS
 *   TRUE for success
 *   FALSE for failure
 */
BOOL __cdecl FCIDestroy(HFCI hfci)
{
  int err;
  PFCI_Int p_fci_internal=((PFCI_Int)(hfci));
  if (REALLY_IS_FCI(hfci)) {

    /* before hfci can be removed all temporary files must be closed */
    /* and deleted */
    p_fci_internal->FCI_Intmagic = 0;

    PFCI_CLOSE (hfci, p_fci_internal->handleCFDATA1,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    PFCI_DELETE(hfci, p_fci_internal->szFileNameCFDATA1, &err,
      p_fci_internal->pv);
    /* TODO error handling of err */
    PFCI_CLOSE (hfci, p_fci_internal->handleCFFILE1,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    PFCI_DELETE(hfci, p_fci_internal->szFileNameCFFILE1, &err,
      p_fci_internal->pv);
    /* TODO error handling of err */
    PFCI_CLOSE (hfci, p_fci_internal->handleCFDATA2,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    PFCI_DELETE(hfci, p_fci_internal->szFileNameCFDATA2, &err,
      p_fci_internal->pv);
    /* TODO error handling of err */
    PFCI_CLOSE (hfci, p_fci_internal->handleCFFILE2,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    PFCI_DELETE(hfci, p_fci_internal->szFileNameCFFILE2, &err,
      p_fci_internal->pv);
    /* TODO error handling of err */
    PFCI_CLOSE (hfci, p_fci_internal->handleCFFOLDER,&err,p_fci_internal->pv);
    /* TODO error handling of err */
    PFCI_DELETE(hfci, p_fci_internal->szFileNameCFFOLDER, &err,
      p_fci_internal->pv);
    /* TODO error handling of err */

    /* data in and out buffers have to be removed */
    if (p_fci_internal->data_in!=NULL)
      PFCI_FREE(hfci, p_fci_internal->data_in);
    if (p_fci_internal->data_out!=NULL)
      PFCI_FREE(hfci, p_fci_internal->data_out);

    /* hfci can now be removed */
    PFCI_FREE(hfci, hfci);
    return TRUE;
  } else {
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }

} /* end of FCIDestroy */
