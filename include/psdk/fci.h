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

#ifndef __WINE_FCI_H
#define __WINE_FCI_H

#include <basetsd.h>

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
 * FCI declarations
 */

typedef enum {
    FCIERR_NONE,
    FCIERR_OPEN_SRC,
    FCIERR_READ_SRC,
    FCIERR_ALLOC_FAIL,
    FCIERR_TEMP_FILE,
    FCIERR_BAD_COMPR_TYPE,
    FCIERR_CAB_FILE,
    FCIERR_USER_ABORT,
    FCIERR_MCI_FAIL,
} FCIERROR;

/**********************************************************************/

#ifndef _A_NAME_IS_UTF
#define _A_NAME_IS_UTF  0x80
#endif

#ifndef _A_EXEC
#define _A_EXEC         0x40
#endif

/**********************************************************************/

typedef void *HFCI;

/**********************************************************************/

typedef struct {
    ULONG cb;              /* Size available for cabinet on this media */
    ULONG cbFolderThresh;  /* Threshold for forcing a new Folder */

    UINT  cbReserveCFHeader;     /* Space to reserve in CFHEADER */
    UINT  cbReserveCFFolder;     /* Space to reserve in CFFOLDER */
    UINT  cbReserveCFData;       /* Space to reserve in CFDATA */
    int   iCab;                  /* Sequential numbers for cabinets */
    int   iDisk;                 /* Disk number */
#ifndef REMOVE_CHICAGO_M6_HACK
    int   fFailOnIncompressible; /* TRUE => Fail if a block is incompressible */
#endif

    USHORT setID; /* Cabinet set ID */

    char szDisk[CB_MAX_DISK_NAME];   /* Current disk name */
    char szCab[CB_MAX_CABINET_NAME]; /* Current cabinet name */
    char szCabPath[CB_MAX_CAB_PATH]; /* Path for creating cabinet */
} CCAB, *PCCAB;

/**********************************************************************/

typedef void * (__cdecl __WINE_ALLOC_SIZE(1) *PFNFCIALLOC)(ULONG cb);
#define FNFCIALLOC(fn) void * __cdecl fn(ULONG cb)

typedef void (__cdecl *PFNFCIFREE)(void *memory);
#define FNFCIFREE(fn) void __cdecl fn(void *memory)

typedef INT_PTR (__cdecl *PFNFCIOPEN) (char *pszFile, int oflag, int pmode, int *err, void *pv);
#define FNFCIOPEN(fn) INT_PTR __cdecl fn(char *pszFile, int oflag, int pmode, int *err, void *pv)

typedef UINT (__cdecl *PFNFCIREAD) (INT_PTR hf, void *memory, UINT cb, int *err, void *pv);
#define FNFCIREAD(fn) UINT __cdecl fn(INT_PTR hf, void *memory, UINT cb, int *err, void *pv)

typedef UINT (__cdecl *PFNFCIWRITE)(INT_PTR hf, void *memory, UINT cb, int *err, void *pv);
#define FNFCIWRITE(fn) UINT __cdecl fn(INT_PTR hf, void *memory, UINT cb, int *err, void *pv)

typedef int  (__cdecl *PFNFCICLOSE)(INT_PTR hf, int *err, void *pv);
#define FNFCICLOSE(fn) int __cdecl fn(INT_PTR hf, int *err, void *pv)

typedef LONG (__cdecl *PFNFCISEEK) (INT_PTR hf, LONG dist, int seektype, int *err, void *pv);
#define FNFCISEEK(fn) LONG __cdecl fn(INT_PTR hf, LONG dist, int seektype, int *err, void *pv)

typedef int  (__cdecl *PFNFCIDELETE) (char *pszFile, int *err, void *pv);
#define FNFCIDELETE(fn) int __cdecl fn(char *pszFile, int *err, void *pv)

typedef BOOL (__cdecl *PFNFCIGETNEXTCABINET)(PCCAB pccab, ULONG  cbPrevCab, void *pv);
#define FNFCIGETNEXTCABINET(fn) BOOL __cdecl fn(PCCAB pccab, \
						ULONG  cbPrevCab, \
						void *pv)

typedef int (__cdecl *PFNFCIFILEPLACED)(PCCAB pccab,
					char *pszFile,
					LONG  cbFile,
					BOOL  fContinuation,
					void *pv);
#define FNFCIFILEPLACED(fn) int __cdecl fn(PCCAB pccab, \
					   char *pszFile, \
                                           LONG  cbFile, \
					   BOOL  fContinuation, \
					   void *pv)

typedef INT_PTR (__cdecl *PFNFCIGETOPENINFO)(char *pszName,
					     USHORT *pdate,
					     USHORT *ptime,
					     USHORT *pattribs,
					     int *err,
					     void *pv);
#define FNFCIGETOPENINFO(fn) INT_PTR __cdecl fn(char *pszName, \
						USHORT *pdate, \
						USHORT *ptime, \
						USHORT *pattribs, \
						int *err, \
						void *pv)

#define statusFile     0  /* Add File to Folder callback */
#define statusFolder   1  /* Add Folder to Cabinet callback */
#define statusCabinet  2  /* Write out a completed cabinet callback */

typedef LONG (__cdecl *PFNFCISTATUS)(UINT typeStatus,
				     ULONG cb1,
				     ULONG cb2,
				     void *pv);
#define FNFCISTATUS(fn) LONG __cdecl fn(UINT typeStatus, \
					ULONG  cb1, \
					ULONG  cb2, \
					void *pv)

typedef BOOL (__cdecl *PFNFCIGETTEMPFILE)(char *pszTempName,
					  int   cbTempName,
					  void *pv);
#define FNFCIGETTEMPFILE(fn) BOOL __cdecl fn(char *pszTempName, \
                                             int   cbTempName, \
                                             void *pv)

/**********************************************************************/

HFCI __cdecl FCICreate(PERF, PFNFCIFILEPLACED, PFNFCIALLOC, PFNFCIFREE,
		       PFNFCIOPEN, PFNFCIREAD, PFNFCIWRITE, PFNFCICLOSE,
		       PFNFCISEEK, PFNFCIDELETE, PFNFCIGETTEMPFILE, PCCAB,
		       void *);
BOOL __cdecl FCIAddFile(HFCI, char *, char *, BOOL, PFNFCIGETNEXTCABINET,
			PFNFCISTATUS, PFNFCIGETOPENINFO, TCOMP);
BOOL __cdecl FCIFlushCabinet(HFCI, BOOL, PFNFCIGETNEXTCABINET, PFNFCISTATUS);
BOOL __cdecl FCIFlushFolder(HFCI, PFNFCIGETNEXTCABINET, PFNFCISTATUS);
BOOL __cdecl FCIDestroy(HFCI hfci);

/**********************************************************************/

#include <poppack.h>

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif  /* __WINE_FCI_H */
