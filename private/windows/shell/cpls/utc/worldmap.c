/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    worldmap.c

Abstract:

    This module implements the world map for the Date/Time applet.

Revision History:

--*/



//
//  Include Files.
//

#include "timedate.h"
#include <commctrl.h>
#include "worldmap.h"





////////////////////////////////////////////////////////////////////////////
//
//  ZeroCDC
//
////////////////////////////////////////////////////////////////////////////

static void ZeroCDC(
    LPCDC cdc)
{
    cdc->dc = NULL;
    cdc->bitmap = cdc->defbitmap = NULL;
}


////////////////////////////////////////////////////////////////////////////
//
//  CreateCDC
//
////////////////////////////////////////////////////////////////////////////

static BOOL CreateCDC(
    LPCDC cdc,
    HBITMAP bitmap)
{
    cdc->dc = CreateCompatibleDC(NULL);
    cdc->bitmap = cdc->defbitmap = NULL;

    if (!bitmap)
    {
        return (FALSE);
    }

    if (!cdc->dc)
    {
        if (bitmap)
        {
            DeleteBitmap(bitmap);
        }
        return (FALSE);
    }

    if (bitmap)
    {
        cdc->defbitmap = SelectBitmap(cdc->dc, cdc->bitmap = bitmap);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  DestroyCDC
//
////////////////////////////////////////////////////////////////////////////

static void DestroyCDC(
    LPCDC cdc)
{
    if (cdc->dc)
    {
        if (cdc->defbitmap)
        {
            SelectBitmap(cdc->dc, cdc->defbitmap);
        }
        if (cdc->bitmap)
        {
            DeleteBitmap(cdc->bitmap);
        }
        DeleteDC(cdc->dc);
        cdc->dc = NULL;
    }

    cdc->bitmap = cdc->defbitmap = NULL;
}


////////////////////////////////////////////////////////////////////////////
//
//  LoadWorldMap
//
////////////////////////////////////////////////////////////////////////////

BOOL LoadWorldMap(
    LPWORLDMAP map,
    HINSTANCE instance,
    LPCTSTR resource)
{
    HDC tempdc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
    BOOL result = FALSE;

    ZeroCDC(&map->original);
    ZeroCDC(&map->prepared);

    map->size.cx = map->size.cy = 0;
    map->rotation = 0;

    if (tempdc)
    {
        if (CreateCDC( &map->original,
                       LoadImage( instance,
                                  resource,
                                  IMAGE_BITMAP,
                                  0,
                                  0,
                                  LR_CREATEDIBSECTION ) ))
        {
            DIBSECTION ds;

            if (GetObject(map->original.bitmap, sizeof(DIBSECTION), &ds))
            {
                map->size.cx = ds.dsBm.bmWidth;
                map->size.cy = ds.dsBm.bmHeight;
                map->bits = (BYTE *)ds.dsBm.bmBits;
                map->scanbytes = ds.dsBm.bmWidthBytes;

                if (( (GetDeviceCaps(tempdc, BITSPIXEL) *
                       GetDeviceCaps(tempdc, PLANES)) > 4 ) ||
                    CreateCDC( &map->prepared,
                               CreateCompatibleBitmap( tempdc,
                                                       ds.dsBm.bmWidth,
                                                       ds.dsBm.bmHeight ) ))
                {
                    RGBQUAD init = { 127, 0, 0, 0 };
                    RGBQUAD *color = map->dirty.colors;
                    int i = WORLDMAP_MAX_COLORS;

                    while (i--)
                    {
                        *color++ = init;
                    }

                    //
                    //  Mark everything as dirty.
                    //
                    map->dirty.first = 0;
                    map->dirty.last = WORLDMAP_MAX_COLORS - 1;
                    map->dirty.spans = NULL;
                    map->dirty.freespans = NULL;

                    map->source = (map->prepared.dc)
                                      ? map->prepared.dc
                                      : map->original.dc;

                    result = TRUE;
                }
            }
            else
            {
                DestroyCDC(&map->original);
            }
        }

        DeleteDC(tempdc);
    }

    return (result);
}


////////////////////////////////////////////////////////////////////////////
//
//  FreeWorldMap
//
////////////////////////////////////////////////////////////////////////////

void FreeWorldMap(
    LPWORLDMAP map)
{
    DIRTYSPAN *span = map->dirty.spans;

    while (span)
    {
        DIRTYSPAN *next = span->next;

        LocalFree((HANDLE)span);
        span = next;
    }

    span = map->dirty.freespans;

    while (span)
    {
        DIRTYSPAN *next = span->next;

        LocalFree((HANDLE)span);
        span = next;
    }

    DestroyCDC(&map->original);
    DestroyCDC(&map->prepared);
}


////////////////////////////////////////////////////////////////////////////
//
//  SetWorldMapRotation
//
////////////////////////////////////////////////////////////////////////////

void SetWorldMapRotation(
    LPWORLDMAP map,
    int rotation)
{
    rotation %= (int)map->size.cx;
    if (rotation < 0)
    {
        rotation += (int)map->size.cx;
    }
    map->rotation = rotation;
}


////////////////////////////////////////////////////////////////////////////
//
//  RotateWorldMap
//
////////////////////////////////////////////////////////////////////////////

void RotateWorldMap(
    LPWORLDMAP map,
    int delta)
{
    SetWorldMapRotation(map, map->rotation + delta);
}


////////////////////////////////////////////////////////////////////////////
//
//  WorldMapGetDisplayedLocation
//
////////////////////////////////////////////////////////////////////////////

int WorldMapGetDisplayedLocation(
    LPWORLDMAP map,
    int pos)
{
    return ( (pos + map->rotation) % map->size.cx );
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumWorldMapDirtySpans
//
////////////////////////////////////////////////////////////////////////////

void EnumWorldMapDirtySpans(
    LPWORLDMAP map,
    ENUMSPANPROC proc,
    DWORD data,
    BOOL rotate)
{
    DIRTYSPAN *span = map->dirty.spans;

    while (span)
    {
        if (rotate)
        {
            int left = (span->left + map->rotation) % map->size.cx;
            int right = left + span->right - span->left;

            if (right > map->size.cx)
            {
                proc(data, left, map->size.cx);

                left = 0;
                right -= map->size.cx;
            }

            proc(data, left, right);
        }
        else
        {
            proc(data, span->left, span->right);
        }

        span = span->next;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  GetWorldMapColorIndex
//
////////////////////////////////////////////////////////////////////////////

int GetWorldMapColorIndex(
    LPWORLDMAP map,
    int x,
    int y)
{
    //
    //  Protect against faulting.
    //
    if ( !map->bits ||
         (x < 0) || (x >= map->size.cx) ||
         (y < 0) || (y >= map->size.cy) )
    {
        return (-1);
    }

    //
    //  Correct source X coordinate for map's virtual rotation.
    //
    x += 2 * (int)map->size.cx - map->rotation;
    x %= (int)map->size.cx;

    //
    //  Correct for dib origin.
    //
    y = (LONG)map->size.cy - 1 - y;

    return ( map->bits[map->scanbytes * y + x] );
}


////////////////////////////////////////////////////////////////////////////
//
//  NewSpan
//
////////////////////////////////////////////////////////////////////////////

DIRTYSPAN *NewSpan(
    DIRTYSTUFF *dirty,
    DIRTYSPAN *a,
    DIRTYSPAN *b)
{
    DIRTYSPAN *span = dirty->freespans;

    if (span)
    {
        dirty->freespans = span->next;
    }
    else
    {
        if ((span = (DIRTYSPAN *)LocalAlloc(LPTR, sizeof(DIRTYSPAN))) == NULL)
        {
            return (NULL);
        }
    }

    span->next = b;

    if (a)
    {
        a->next = span;
    }
    else
    {
        dirty->spans = span;
    }

    return (span);
}


////////////////////////////////////////////////////////////////////////////
//
//  DeleteSpan
//
////////////////////////////////////////////////////////////////////////////

void DeleteSpan(
    DIRTYSTUFF *dirty,
    DIRTYSPAN *a,
    DIRTYSPAN *b)
{
    if (a)
    {
        a->next = b->next;
    }
    else
    {
        dirty->spans = b->next;
    }

    b->next = dirty->freespans;
    dirty->freespans = b;
}


////////////////////////////////////////////////////////////////////////////
//
//  AddDirtySpan
//
////////////////////////////////////////////////////////////////////////////

void AddDirtySpan(
    DIRTYSTUFF *dirty,
    int left,
    int cx)
{
    int right = left + cx;
    DIRTYSPAN *curr = dirty->spans;
    DIRTYSPAN *temp = NULL;
    DIRTYSPAN *span;

    cx = left - 1;
    while (curr && (cx > curr->right))
    {
        temp = curr;
        curr = curr->next;
    }

    cx = right + 1;
    if (curr && (cx >= curr->left))
    {
        if (left < curr->left)
        {
            curr->left = left;
        }
        span = temp = curr;

        if (right > curr->right)
        {
            curr->right = right;
            curr = curr->next;

            while (curr && (cx >= curr->left))
            {
                span->right = curr->right;

                DeleteSpan(dirty, temp, curr);
                temp = temp->next;
                curr = (temp ? temp->next : NULL);
            }
        }
    }
    else
    {
        if ((span = NewSpan(dirty, temp, curr)) == NULL)
        {
            if (!temp)
            {
                return;
            }
            span = temp;
            left = span->left;
        }

        span->left = left;
        span->right = right;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  ChangeWorldMapColor
//
////////////////////////////////////////////////////////////////////////////

void ChangeWorldMapColor(
    LPWORLDMAP map,
    int index,
    const RGBQUAD *color,
    int x,
    int cx)
{
    if ((index >= 0) && (index < WORLDMAP_MAX_COLORS))
    {
        //
        //  Store the new color.
        //
        map->dirty.colors[index] = *color;

        //
        //  Update the dirty markers to include this entry.
        //
        if (index < map->dirty.first)
        {
            map->dirty.first = index;
        }
        if (index > map->dirty.last)
        {
            map->dirty.last = index;
        }
    }

    while (((x + cx) > map->size.cx) && (x >= 0))
    {
        x -= map->size.cx;
    }

    if (x < 0)
    {
        AddDirtySpan(&map->dirty, map->size.cx + x, -x);
        cx += x;
        x = 0;
    }

    AddDirtySpan(&map->dirty, x, cx);
}


////////////////////////////////////////////////////////////////////////////
//
//  CommitChanges
//
////////////////////////////////////////////////////////////////////////////

void CommitChanges(
    LPWORLDMAP map)
{
    if (map->dirty.last >= 0)
    {
        SetDIBColorTable( map->original.dc,
                          map->dirty.first,
                          1 + map->dirty.last - map->dirty.first,
                          map->dirty.colors + map->dirty.first );

        //
        //  Reset the dirty markers.
        //
        map->dirty.first = WORLDMAP_MAX_COLORS;
        map->dirty.last = -1;
    }

    while (map->dirty.spans)
    {
        DIRTYSPAN *span = map->dirty.spans;

        if (map->prepared.dc)
        {
            BitBlt( map->prepared.dc,
                    span->left,
                    0,
                    span->right - span->left,
                    (int)map->size.cy,
                    map->original.dc,
                    span->left,
                    0,
                    SRCCOPY );
        }

        DeleteSpan(&map->dirty, NULL, span);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  DrawWorldMap
//
////////////////////////////////////////////////////////////////////////////

void DrawWorldMap(
    HDC dc,
    int xdst,
    int ydst,
    int cx,
    int cy,
    LPWORLDMAP map,
    int xmap,
    int ymap,
    DWORD rop)
{
    CommitChanges(map);

    //
    //  Lop off extra Y stuff cause there's nothing there.
    //
    if ((ymap + cy) > (int)map->size.cy)
    {
        cy = (int)map->size.cy - ymap;
    }

    //
    //  Clip off extra X so we'll enter the case below only when we need to.
    //
    if (cx > (int)map->size.cx)
    {
        cx = (int)map->size.cx;
    }

    //
    //  Correct source X coordinate for map's virtual rotation.
    //
    xmap += 2 * (int)map->size.cx - map->rotation;
    xmap %= (int)map->size.cx;

    //
    //  See if the blt rect falls off the end of our flat little world.
    //
    if ((xmap + cx) > (int)map->size.cx)
    {
        //
        //  Compute the width of the first blt.
        //
        int firstcx = (int)map->size.cx - xmap;

        //
        //  See bits.  See bits blt.
        //
        BitBlt(dc, xdst, ydst, firstcx, cy, map->source, xmap, ymap, rop);

        //
        //  Adjust the params so the second blt does the right wrapping.
        //
        xdst += firstcx;
        cx -= firstcx;
        xmap = 0;
    }

    //
    //  blt bits blt!
    //
    BitBlt(dc, xdst, ydst, cx, cy, map->source, xmap, ymap, rop);
}
