/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WGMETA.H
 *  WOW32 16-bit GDI API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/



ULONG FASTCALL   WG32CloseMetaFile(PVDMFRAME pFrame);
ULONG FASTCALL   WG32CopyMetaFile(PVDMFRAME pFrame);
ULONG FASTCALL   WG32CreateMetaFile(PVDMFRAME pFrame);
ULONG FASTCALL   WG32DeleteMetaFile(PVDMFRAME pFrame);
ULONG FASTCALL   WG32EnumMetaFile(PVDMFRAME pFrame);
ULONG FASTCALL   WG32GetMetaFile(PVDMFRAME pFrame);
ULONG FASTCALL   WG32GetMetaFileBits(PVDMFRAME pFrame);
ULONG FASTCALL   WG32PlayMetaFile(PVDMFRAME pFrame);
ULONG FASTCALL   WG32PlayMetaFileRecord(PVDMFRAME pFrame);
ULONG FASTCALL   WG32SetMetaFileBits(PVDMFRAME pFrame);
HAND16  WinMetaFileFromHMF(HMETAFILE hmf, BOOL fFreeOriginal);
HMETAFILE HMFFromWinMetaFile(HAND16 h16, BOOL fFreeOriginal);


/* MetaFile Enumeration handler data
 */
typedef struct _METADATA {       /* fntdata */
    PARMEMP parmemp;                // 16-bit enumeration data (WOW.H)
    VPPROC  vpfnEnumMetaFileProc;   // 16-bit enumeration function
    DWORD   mtMaxRecordSize;
} METADATA, *PMETADATA;


INT W32EnumMetaFileCallBack(HDC hdc, LPHANDLETABLE lpht, LPMETARECORD lpMR, LONG nObj, PMETADATA pMetaData);
