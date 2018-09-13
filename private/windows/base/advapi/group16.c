/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    group16.c

Abstract:

    This module contains routines for reading 16-bit Windows 3.1
    group files.

Author:

    Steve Wood (stevewo) 22-Feb-1993

Revision History:

--*/

#include "advapi.h"
#include <stdio.h>

#include <winbasep.h>
#include "win31io.h"

#define ROUND_UP( X, A ) (((ULONG_PTR)(X) + (A) - 1) & ~((A) - 1))
#define PAGE_SIZE 4096
#define PAGE_NUMBER( A ) ((DWORD)(A) / PAGE_SIZE)

PGROUP_DEF16
LoadGroup16(
    PWSTR GroupFileName
    )
{
    HANDLE File, Mapping;
    LPVOID Base;

    File = CreateFileW( GroupFileName,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL
                      );

    if (File == INVALID_HANDLE_VALUE) {
        return NULL;
        }


    Mapping = CreateFileMapping( File,
                                 NULL,
                                 PAGE_WRITECOPY,
                                 0,
                                 0,
                                 NULL
                               );
    CloseHandle( File );
    if (Mapping == NULL) {
        return NULL;
        }


    Base = MapViewOfFile( Mapping,
                          FILE_MAP_COPY,
                          0,
                          0,
                          0
                        );
    CloseHandle( Mapping );
    return (PGROUP_DEF16)Base;
}


BOOL
UnloadGroup16(
    PGROUP_DEF16 Group
    )
{
    return UnmapViewOfFile( (LPVOID)Group );
}


BOOL
ExtendGroup16(
    PGROUP_DEF16 Group,
    BOOL AppendToGroup,
    DWORD cb
    )
{
    PBYTE Start, Commit, End;

    if (((DWORD)Group->wReserved + cb) > 0xFFFF) {
        return FALSE;
        }

    Start = (PBYTE)Group + Group->cbGroup;
    End = Start + cb;

    if (PAGE_NUMBER( Group->wReserved ) != PAGE_NUMBER( Group->wReserved + cb )) {
        Commit = (PBYTE)ROUND_UP( (PBYTE)Group + Group->wReserved, PAGE_SIZE );
        if (!VirtualAlloc( Commit, ROUND_UP( cb, PAGE_SIZE ), MEM_COMMIT, PAGE_READWRITE )) {
            return FALSE;
            }
        }

    if (!AppendToGroup) {
        memmove( End, Start, Group->wReserved - Group->cbGroup );
        }

    Group->wReserved += (WORD)cb;
    return TRUE;
}

WORD
AddDataToGroup16(
    PGROUP_DEF16 Group,
    PBYTE Data,
    DWORD cb
    )
{
    WORD Offset;

    if (cb == 0) {
        cb = strlen( Data ) + 1;
        }
    cb = (DWORD)ROUND_UP( cb, sizeof( WORD ) );

    if (!ExtendGroup16( Group, FALSE, cb )) {
        return 0;
        }

    if (((DWORD)Group->cbGroup + cb) > 0xFFFF) {
        return 0;
        }

    Offset = Group->cbGroup;
    Group->cbGroup += (WORD)cb;

    if (Data != NULL) {
        memmove( (PBYTE)Group + Offset, Data, cb );
        }
    else {
        memset( (PBYTE)Group + Offset, 0, cb );
        }

    return Offset;
}

BOOL
AddTagToGroup16(
    PGROUP_DEF16 Group,
    WORD wID,
    WORD wItem,
    WORD cb,
    PBYTE rgb
    )
{
    WORD Offset;
    PTAG_DEF16 Tag;

    cb = (WORD)(ROUND_UP( cb, sizeof( WORD ) ));

    Offset = Group->wReserved;
    if (!ExtendGroup16( Group, TRUE, FIELD_OFFSET( TAG_DEF16, rgb[ 0 ] ) + cb )) {
        return FALSE;
        }

    Tag = (PTAG_DEF16)PTR( Group, Offset );
    Tag->wID = wID;
    Tag->wItem = (int)wItem;
    if (cb) {
        Tag->cb = (WORD)(cb + FIELD_OFFSET( TAG_DEF16, rgb[ 0 ] ));
        memmove( &Tag->rgb[ 0 ], rgb, cb );
        }
    else {
        Tag->cb = (WORD)(FIELD_OFFSET( TAG_DEF16, rgb[ 0 ] ));
        }
    return TRUE;
}


#if DBG
BOOL
DumpGroup16(
    PWSTR GroupFileName,
    PGROUP_DEF16 Group
    )
{
    PICON_HEADER16 Icon;
    PITEM_DEF16 Item;
    PTAG_DEF16 Tag;
    UINT i;
    PULONG p;
    int cb;

    DbgPrint( "%ws - Group at %08x\n", GroupFileName, Group );
    DbgPrint( "     dwMagic:     %08x\n", Group->dwMagic );
    DbgPrint( "     wCheckSum:       %04x\n", Group->wCheckSum );
    DbgPrint( "     cbGroup:         %04x\n", Group->cbGroup );
    DbgPrint( "     nCmdShow:        %04x\n", Group->nCmdShow );
    DbgPrint( "     rcNormal:       [%04x,%04x,%04x,%04x]\n",
                      Group->rcNormal.Left,
                      Group->rcNormal.Top,
                      Group->rcNormal.Right,
                      Group->rcNormal.Bottom
            );
    DbgPrint( "     ptMin:          [%04x,%04x]\n",
                      Group->ptMin.x,
                      Group->ptMin.y
            );
    DbgPrint( "     pName:          [%04x] %s\n",
                      Group->pName,
                      (PSZ)PTR( Group, Group->pName )
            );
    DbgPrint( "     cxIcon:          %04x\n", Group->cxIcon );
    DbgPrint( "     cyIcon:          %04x\n", Group->cyIcon );
    DbgPrint( "     wIconFormat:     %04x\n", Group->wIconFormat );
    DbgPrint( "     cItems:          %04x\n", Group->cItems );

    for (i=0; i<Group->cItems; i++) {
        DbgPrint( "     Item[ %02x ] at %04x\n", i, Group->rgiItems[ i ] );
        if (Group->rgiItems[ i ] != 0) {
            Item = ITEM16( Group, i );
            DbgPrint( "         pt:      [%04x, %04x]\n",
                               Item->pt.x,
                               Item->pt.y
                    );
            DbgPrint( "         iIcon:    %04x\n", Item->iIcon );
            DbgPrint( "         cbHeader: %04x\n", Item->cbHeader );
            DbgPrint( "         cbANDPln: %04x\n", Item->cbANDPlane );
            DbgPrint( "         cbXORPln: %04x\n", Item->cbXORPlane );
            DbgPrint( "         pHeader:  %04x\n", Item->pHeader );
            Icon = (PICON_HEADER16)PTR( Group, Item->pHeader );
            DbgPrint( "             xHot: %04x\n", Icon->xHotSpot );
            DbgPrint( "             yHot: %04x\n", Icon->yHotSpot );
            DbgPrint( "             cx:   %04x\n", Icon->cx );
            DbgPrint( "             cy:   %04x\n", Icon->cy );
            DbgPrint( "             cbWid:%04x\n", Icon->cbWidth );
            DbgPrint( "             Plane:%04x\n", Icon->Planes );
            DbgPrint( "             BPP:  %04x\n", Icon->BitsPixel );
            DbgPrint( "         pANDPln:  %04x\n", Item->pANDPlane );
            DbgPrint( "         pXORPln:  %04x\n", Item->pXORPlane );
            DbgPrint( "         pName:   [%04x] %s\n", Item->pName, PTR( Group, Item->pName ) );
            DbgPrint( "         pCommand:[%04x] %s\n", Item->pCommand, PTR( Group, Item->pCommand ) );
            DbgPrint( "         pIconPth:[%04x] %s\n", Item->pIconPath, PTR( Group, Item->pIconPath ) );

            DbgPrint( "         AND bits:\n" );
            p = (PULONG)PTR( Group, Item->pANDPlane );
            cb = Item->cbANDPlane;
            while (cb > 0) {
                DbgPrint( "             %08x", *p++ );
                cb -= sizeof( *p );
                if (cb >= sizeof( *p )) {
                    cb -= sizeof( *p );
                    DbgPrint( " %08x", *p++ );
                    if (cb >= sizeof( *p )) {
                        cb -= sizeof( *p );
                        DbgPrint( " %08x", *p++ );
                        if (cb >= sizeof( *p )) {
                            cb -= sizeof( *p );
                            DbgPrint( " %08x", *p++ );
                            }
                        }
                    }

                DbgPrint( "\n" );
                }

            DbgPrint( "         XOR bits:\n" );
            p = (PULONG)PTR( Group, Item->pXORPlane );
            cb = Item->cbXORPlane;
            while (cb > 0) {
                DbgPrint( "             %08x", *p++ );
                cb -= sizeof( *p );
                if (cb >= sizeof( *p )) {
                    cb -= sizeof( *p );
                    DbgPrint( " %08x", *p++ );
                    if (cb >= sizeof( *p )) {
                        cb -= sizeof( *p );
                        DbgPrint( " %08x", *p++ );
                        if (cb >= sizeof( *p )) {
                            cb -= sizeof( *p );
                            DbgPrint( " %08x", *p++ );
                            }
                        }
                    }

                DbgPrint( "\n" );
                }
            }
        }

    Tag = (PTAG_DEF16)((PBYTE)Group + Group->cbGroup);
    if (Tag->wID == ID_MAGIC && Tag->wItem == ID_LASTTAG &&
        *(UNALIGNED DWORD *)&Tag->rgb == PMTAG_MAGIC
       ) {
        while (Tag->wID != ID_LASTTAG) {
            DbgPrint( "     Tag at %04x\n", (PBYTE)Tag - (PBYTE)Group );
            DbgPrint( "         wID:      %04x\n", Tag->wID );
            DbgPrint( "         wItem:    %04x\n", Tag->wItem );
            DbgPrint( "         cb:       %04x\n", Tag->cb );


            switch( Tag->wID ) {
                case ID_MAGIC:
                    DbgPrint( "         rgb:      ID_MAGIC( %.4s )\n", Tag->rgb );
                    break;

                case ID_WRITERVERSION:
                    DbgPrint( "         rgb:      ID_WRITERVERSION( %s )\n", Tag->rgb );
                    break;

                case ID_APPLICATIONDIR:
                    DbgPrint( "         rgb:      ID_APPLICATIONDIR( %s )\n", Tag->rgb );
                    break;

                case ID_HOTKEY:
                    DbgPrint( "         rgb:      ID_HOTKEY( %04x )\n", *(LPWORD)Tag->rgb );
                    break;

                case ID_MINIMIZE:
                    DbgPrint( "         rgb:      ID_MINIMIZE()\n" );
                    break;

                case ID_LASTTAG:
                    DbgPrint( "         rgb:      ID_LASTTAG()\n" );
                    break;

                default:
                    DbgPrint( "         rgb:      unknown data format for this ID\n" );
                    break;
                }


            Tag = (PTAG_DEF16)((PBYTE)Tag + Tag->cb);
            }
        }

    return TRUE;
}


#endif // DBG
