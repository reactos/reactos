/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    worldmap.h

Abstract:

    This module contains the information for the world map for the
    Date/Time applet.

Revision History:

--*/



#ifndef _WORLDMAP_H
#define _WORLDMAP_H


//
//  Constant Declarations.
//

#define WORLDMAP_MAX_DIRTY_SPANS       4
#define WORLDMAP_MAX_COLORS            256




//
//  Typedef Declarations.
//

typedef struct
{
    HDC dc;
    HBITMAP bitmap;
    HBITMAP defbitmap;

} CDC, *LPCDC;

typedef struct dirtyspan
{
    int left;
    int right;
    struct dirtyspan *next;

} DIRTYSPAN;

typedef struct
{
    int first;
    int last;
    DIRTYSPAN *spans;
    DIRTYSPAN *freespans;
    RGBQUAD colors[WORLDMAP_MAX_COLORS];

} DIRTYSTUFF;

typedef struct tagWORLDMAP
{
    CDC original;
    CDC prepared;
    SIZE size;
    BYTE *bits;
    LONG scanbytes;
    int rotation;
    HDC source;
    DIRTYSTUFF dirty;  // keep at end (>1k)

} WORLDMAP, *LPWORLDMAP;




//
//  Function Prototypes.
//

typedef void (*ENUMSPANPROC)(DWORD data, int left, int right);

BOOL
LoadWorldMap(
    LPWORLDMAP map,
    HINSTANCE instance,
    LPCTSTR resource);

void
FreeWorldMap(
    LPWORLDMAP map);

void
SetWorldMapRotation(
    LPWORLDMAP map,
    int rotation);

void
RotateWorldMap(
    LPWORLDMAP map,
    int delta);

int
WorldMapGetDisplayedLocation(
    LPWORLDMAP map,
    int pos);

void
EnumWorldMapDirtySpans(
    LPWORLDMAP map,
    ENUMSPANPROC proc,
    DWORD data,
    BOOL rotate);

void
ChangeWorldMapColor(
    LPWORLDMAP map,
    int index,
    const RGBQUAD *color,
    int x,
    int cx);

int
GetWorldMapColorIndex(
    LPWORLDMAP map,
    int x,
    int y);

void
DrawWorldMap(
    HDC dc,
    int xdst,
    int ydst,
    int cx,
    int cy,
    LPWORLDMAP map,
    int xmap,
    int ymap,
    DWORD rop);


#endif // _WORLDMAP_H
