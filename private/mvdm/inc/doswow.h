/*++ BUILD Version: 0002
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  DOSWOW.H
 *  Shared structures between NTDOS, DEM and WOW32
 *
 *  History:
 *  Created 03-Dec-1993 by Neil Sandlin (neilsa)
--*/

/* XLATOFF */

#pragma pack(2)

/* XLATON */

/*
 * The following structure is used by some of the WOW functions to
 * point to internal DOS data. This is done to emulate some of the more
 * involved functions in WOW32, while still maintaining DOS data integrity.
 */
typedef struct _DOSWOWDATA {           /* DWD */
    DWORD lpCDSCount;
    DWORD lpCDSFixedTable;
    DWORD lpCDSBuffer;
    DWORD lpCurDrv;
    DWORD lpCurPDB;
    DWORD lpDrvErr;
    DWORD lpExterrLocus; // byte
    DWORD lpSCS_ToSync;
    DWORD lpSftAddr;
    DWORD lpExterr;  // word
    DWORD lpExterrActionClass; // 2 bytes -- action and class
} DOSWOWDATA;
typedef DOSWOWDATA UNALIGNED *PDOSWOWDATA;


/* XLATOFF */

#pragma pack(1)

typedef struct _DOSPDB {                        // DOS Process Data Block
    CHAR   PDB_Not_Interested[44];      // Fields we are not interested in
    USHORT PDB_environ;             // segment of environment
    DWORD  PDB_User_stack;
    USHORT PDB_JFN_Length;          // JFT length
    ULONG  PDB_JFN_Pointer;         // JFT pointer
} DOSPDB, *PDOSPDB;


typedef struct _DOSSF {             // DOS header for SFT chain
    ULONG  SFLink;                  // Link to next SF
    USHORT SFCount;                 // number of entries
    USHORT SFTable;                 // beginning of array of the SFTs
} DOSSF;
typedef DOSSF UNALIGNED *PDOSSF;


#define SFT_NAMED_PIPE 0x2000       // named pipe flag

typedef struct _DOSSFT {            // DOS SFT
    USHORT  SFT_Ref_Count;          // Howmany tasks using it
    USHORT  SFT_Mode;               // Mode of access
    UCHAR   SFT_Attr;               // Attribute of file
    USHORT  SFT_Flags;              // Bit 15 = 1 if remote file
                                    //        = 0 if local or device
    ULONG   SFT_Devptr;             // Device pointer
    USHORT  SFT_Time;
    USHORT  SFT_Date;
    ULONG   SFT_Size;
    ULONG   SFT_Position;
    ULONG   SFT_Chain;
    USHORT  SFT_PID;
    ULONG   SFT_NTHandle;           // NT File Handle
} DOSSFT;
typedef DOSSFT UNALIGNED *PDOSSFT;

#define SF_NT_SEEK 0x0200

#pragma pack()

// these are demLFN structures that work with wow and dem

typedef VOID (*PDOSWOWUPDATETDBDIR)(UCHAR, LPSTR);
typedef BOOL (*PDOSWOWGETTDBDIR)(UCHAR Drive, LPSTR pCurrentDirectory);
typedef BOOL (*PDOSWOWDODIRECTHDPOPUP)(VOID);

typedef struct tagWOWLFNInit {
   PDOSWOWUPDATETDBDIR pDosWowUpdateTDBDir;
   PDOSWOWGETTDBDIR pDosWowGetTDBDir;
   PDOSWOWDODIRECTHDPOPUP pDosWowDoDirectHDPopup;
}  WOWLFNINIT, *PWOWLFNINIT;

/* XLATON */
