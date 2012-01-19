/*
 * Copyright (C) 2002 Patrik Stridvall
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

#ifndef __WINE_FDI_H
#define __WINE_FDI_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#include <pshpack4.h>

#ifndef INCLUDED_TYPES_FCI_FDI
#define INCLUDED_TYPES_FCI_FDI 1

/***********************************************************************
 * Common FCI/TDI declarations
 */

typedef ULONG CHECKSUM;

typedef ULONG UOFF;
typedef ULONG COFF;

/**********************************************************************/

typedef struct {
    int   erfOper;  /* FCI/FDI error code - see {FCI,FDI}ERROR_XXX for details. */
    int   erfType;  /* Optional error value filled in by FCI/FDI. */
    BOOL  fError;   /* TRUE => error present */
} ERF, *PERF;

/**********************************************************************/

#define CB_MAX_CHUNK         32768U
#define CB_MAX_DISK          0x7fffffffL
#define CB_MAX_FILENAME      256
#define CB_MAX_CABINET_NAME  256
#define CB_MAX_CAB_PATH      256
#define CB_MAX_DISK_NAME     256

/**********************************************************************/

typedef unsigned short TCOMP;

#define tcompMASK_TYPE          0x000F  /* Mask for compression type */
#define tcompTYPE_NONE          0x0000  /* No compression */
#define tcompTYPE_MSZIP         0x0001  /* MSZIP */
#define tcompTYPE_QUANTUM       0x0002  /* Quantum */
#define tcompTYPE_LZX           0x0003  /* LZX */
#define tcompBAD                0x000F  /* Unspecified compression type */

#define tcompMASK_LZX_WINDOW    0x1F00  /* Mask for LZX Compression Memory */
#define tcompLZX_WINDOW_LO      0x0F00  /* Lowest LZX Memory (15) */
#define tcompLZX_WINDOW_HI      0x1500  /* Highest LZX Memory (21) */
#define tcompSHIFT_LZX_WINDOW        8  /* Amount to shift over to get int */

#define tcompMASK_QUANTUM_LEVEL 0x00F0  /* Mask for Quantum Compression Level */
#define tcompQUANTUM_LEVEL_LO   0x0010  /* Lowest Quantum Level (1) */
#define tcompQUANTUM_LEVEL_HI   0x0070  /* Highest Quantum Level (7) */
#define tcompSHIFT_QUANTUM_LEVEL     4  /* Amount to shift over to get int */

#define tcompMASK_QUANTUM_MEM   0x1F00  /* Mask for Quantum Compression Memory */
#define tcompQUANTUM_MEM_LO     0x0A00  /* Lowest Quantum Memory (10) */
#define tcompQUANTUM_MEM_HI     0x1500  /* Highest Quantum Memory (21) */
#define tcompSHIFT_QUANTUM_MEM       8  /* Amount to shift over to get int */

#define tcompMASK_RESERVED      0xE000  /* Reserved bits (high 3 bits) */

/**********************************************************************/

#define CompressionTypeFromTCOMP(tc) \
    ((tc) & tcompMASK_TYPE)

#define CompressionLevelFromTCOMP(tc) \
    (((tc) & tcompMASK_QUANTUM_LEVEL) >> tcompSHIFT_QUANTUM_LEVEL)

#define CompressionMemoryFromTCOMP(tc) \
    (((tc) & tcompMASK_QUANTUM_MEM) >> tcompSHIFT_QUANTUM_MEM)

#define TCOMPfromTypeLevelMemory(t, l, m) \
    (((m) << tcompSHIFT_QUANTUM_MEM  ) | \
     ((l) << tcompSHIFT_QUANTUM_LEVEL) | \
     ( t                             ))

#define LZXCompressionWindowFromTCOMP(tc) \
    (((tc) & tcompMASK_LZX_WINDOW) >> tcompSHIFT_LZX_WINDOW)

#define TCOMPfromLZXWindow(w) \
    (((w) << tcompSHIFT_LZX_WINDOW) | \
     ( tcompTYPE_LZX              ))

#endif /* !defined(INCLUDED_TYPES_FCI_FDI) */

/***********************************************************************
 * FDI declarations
 */

typedef enum {
    FDIERROR_NONE,
    FDIERROR_CABINET_NOT_FOUND,
    FDIERROR_NOT_A_CABINET,
    FDIERROR_UNKNOWN_CABINET_VERSION,
    FDIERROR_CORRUPT_CABINET,
    FDIERROR_ALLOC_FAIL,
    FDIERROR_BAD_COMPR_TYPE,
    FDIERROR_MDI_FAIL,
    FDIERROR_TARGET_FILE,
    FDIERROR_RESERVE_MISMATCH,
    FDIERROR_WRONG_CABINET,
    FDIERROR_USER_ABORT,
} FDIERROR;

/**********************************************************************/

#ifndef _A_NAME_IS_UTF
#define _A_NAME_IS_UTF  0x80
#endif

#ifndef _A_EXEC
#define _A_EXEC         0x40
#endif

/**********************************************************************/

typedef void *HFDI;

/**********************************************************************/

typedef struct {
    LONG    cbCabinet;  /* Total length of cabinet file */
    USHORT  cFolders;   /* Count of folders in cabinet */
    USHORT  cFiles;     /* Count of files in cabinet */
    USHORT  setID;      /* Cabinet set ID */
    USHORT  iCabinet;   /* Cabinet number in set (0 based) */
    BOOL    fReserve;   /* TRUE => RESERVE present in cabinet */
    BOOL    hasprev;    /* TRUE => Cabinet is chained prev */
    BOOL    hasnext;    /* TRUE => Cabinet is chained next */
} FDICABINETINFO, *PFDICABINETINFO; /* pfdici */

/**********************************************************************/

typedef enum {
    fdidtNEW_CABINET,  /* New cabinet */
    fdidtNEW_FOLDER,   /* New folder */
    fdidtDECRYPT,      /* Decrypt a data block */
} FDIDECRYPTTYPE;

/**********************************************************************/

typedef struct {
    FDIDECRYPTTYPE fdidt; /* Command type (selects union below) */

    void *pvUser; /* Decryption context */

    union {
        struct {                      /* fdidtNEW_CABINET */
	    void   *pHeaderReserve;   /* RESERVE section from CFHEADER */
	    USHORT  cbHeaderReserve;  /* Size of pHeaderReserve */
	    USHORT  setID;            /* Cabinet set ID */
	    int     iCabinet;         /* Cabinet number in set (0 based) */
        } cabinet;

        struct {                      /* fdidtNEW_FOLDER */
	    void   *pFolderReserve;   /* RESERVE section from CFFOLDER */
	    USHORT  cbFolderReserve;  /* Size of pFolderReserve */
	    USHORT  iFolder;          /* Folder number in cabinet (0 based) */
        } folder;

        struct {                      /* fdidtDECRYPT */
	    void   *pDataReserve;     /* RESERVE section from CFDATA */
	    USHORT  cbDataReserve;    /* Size of pDataReserve */
	    void   *pbData;           /* Data buffer */
	    USHORT  cbData;           /* Size of data buffer */
	    BOOL    fSplit;           /* TRUE if this is a split data block */
	    USHORT  cbPartial;        /* 0 if this is not a split block, or
				       * the first piece of a split block;
                                       * Greater than 0 if this is the
                                       * second piece of a split block.
                                       */
        } decrypt;
    } DUMMYUNIONNAME;
} FDIDECRYPT, *PFDIDECRYPT;

/**********************************************************************/

typedef void * (__cdecl *PFNALLOC)(ULONG cb);
#define FNALLOC(fn) void * __cdecl fn(ULONG cb)

typedef void (__cdecl *PFNFREE)(void *pv);
#define FNFREE(fn) void __cdecl fn(void *pv)

typedef INT_PTR (__cdecl *PFNOPEN) (char *pszFile, int oflag, int pmode);
#define FNOPEN(fn) INT_PTR __cdecl fn(char *pszFile, int oflag, int pmode)

typedef UINT (__cdecl *PFNREAD) (INT_PTR hf, void *pv, UINT cb);
#define FNREAD(fn) UINT __cdecl fn(INT_PTR hf, void *pv, UINT cb)

typedef UINT (__cdecl *PFNWRITE)(INT_PTR hf, void *pv, UINT cb);
#define FNWRITE(fn) UINT __cdecl fn(INT_PTR hf, void *pv, UINT cb)

typedef int  (__cdecl *PFNCLOSE)(INT_PTR hf);
#define FNCLOSE(fn) int __cdecl fn(INT_PTR hf)

typedef LONG (__cdecl *PFNSEEK) (INT_PTR hf, LONG dist, int seektype);
#define FNSEEK(fn) LONG __cdecl fn(INT_PTR hf, LONG dist, int seektype)

typedef int (__cdecl *PFNFDIDECRYPT)(PFDIDECRYPT pfdid);
#define FNFDIDECRYPT(fn) int __cdecl fn(PFDIDECRYPT pfdid)

typedef struct {
    LONG  cb;
    char *psz1;
    char *psz2;
    char *psz3;  /* Points to a 256 character buffer */
    void *pv;    /* Value for client */

    INT_PTR hf;

    USHORT date;
    USHORT time;
    USHORT attribs;

    USHORT setID;     /* Cabinet set ID */
    USHORT iCabinet;  /* Cabinet number (0-based) */
    USHORT iFolder;   /* Folder number (0-based) */

    FDIERROR fdie;
} FDINOTIFICATION, *PFDINOTIFICATION;

typedef enum {
    fdintCABINET_INFO,     /* General information about cabinet */
    fdintPARTIAL_FILE,     /* First file in cabinet is continuation */
    fdintCOPY_FILE,        /* File to be copied */
    fdintCLOSE_FILE_INFO,  /* Close the file, set relevant info */
    fdintNEXT_CABINET,     /* File continued to next cabinet */
    fdintENUMERATE,        /* Enumeration status */
} FDINOTIFICATIONTYPE;

typedef INT_PTR (__cdecl *PFNFDINOTIFY)(FDINOTIFICATIONTYPE fdint,
					PFDINOTIFICATION  pfdin);
#define FNFDINOTIFY(fn) INT_PTR __cdecl fn(FDINOTIFICATIONTYPE fdint, \
					   PFDINOTIFICATION pfdin)

#include <pshpack1.h>

typedef struct {
    char ach[2];  /* Set to { '*', '\0' } */
    LONG cbFile;  /* Required spill file size */
} FDISPILLFILE, *PFDISPILLFILE;

#include <poppack.h>

#define cpuUNKNOWN (-1)  /* FDI does detection */
#define cpu80286   (0)   /* '286 opcodes only */
#define cpu80386   (1)   /* '386 opcodes used */

/**********************************************************************/

HFDI __cdecl FDICreate(PFNALLOC, PFNFREE, PFNOPEN, PFNREAD, PFNWRITE,
		       PFNCLOSE, PFNSEEK, int, PERF);
BOOL __cdecl FDIIsCabinet(HFDI, INT_PTR, PFDICABINETINFO);
BOOL __cdecl FDICopy(HFDI, char *, char *, int, PFNFDINOTIFY,
		     PFNFDIDECRYPT, void *pvUser);
BOOL __cdecl FDIDestroy(HFDI);
BOOL __cdecl FDITruncateCabinet(HFDI, char *, USHORT);

/**********************************************************************/

#include <poppack.h>

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif  /* __WINE_FDI_H */
