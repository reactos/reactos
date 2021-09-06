/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/gdi/ntgdi/path.c
 * PURPOSE:         Graphics paths (BeginPath, EndPath etc.)
 * PROGRAMMER:      Copyright 1997, 1998 Martin Boehme
 *                            1999 Huw D M Davies
 *                            2005 Dmitry Timoshkov
 *                            2018 Katayama Hirofumi MZ
 */

#include <win32k.h>
#include <suppress.h>

DBG_DEFAULT_CHANNEL(GdiPath);

#ifdef _MSC_VER
#pragma warning(disable:4244)
#endif

#define NUM_ENTRIES_INITIAL 16  /* Initial size of points / flags arrays  */

#define GROW_FACTOR_NUMER    2  /* Numerator of grow factor for the array */
#define GROW_FACTOR_DENOM    1  /* Denominator of grow factor             */

#if DBG
static int PathCount = 0;
#endif

/***********************************************************************
 * Internal functions
 */

PPATH FASTCALL
PATH_CreatePath(int count)
{
    PPATH pPath = PATH_AllocPathWithHandle();

    if (!pPath)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    TRACE("CreatePath p 0x%p\n", pPath);
    // Path handles are shared. Also due to recursion with in the same thread.
    GDIOBJ_vUnlockObject((POBJ)pPath);       // Unlock
    pPath = PATH_LockPath(pPath->BaseObject.hHmgr); // Share Lock.

    /* Make sure that path is empty */
    PATH_EmptyPath(pPath);

    count = max( NUM_ENTRIES_INITIAL, count );

    pPath->numEntriesAllocated = count;

    pPath->pPoints = (POINT *)ExAllocatePoolWithTag(PagedPool, count * sizeof(POINT), TAG_PATH);
    RtlZeroMemory( pPath->pPoints, count * sizeof(POINT));
    pPath->pFlags  =  (BYTE *)ExAllocatePoolWithTag(PagedPool, count * sizeof(BYTE),  TAG_PATH);
    RtlZeroMemory( pPath->pFlags, count * sizeof(BYTE));

    /* Initialize variables for new path */
    pPath->numEntriesUsed = 0;
    pPath->newStroke = TRUE;
    pPath->state     = PATH_Open;
    pPath->pos.x = pPath->pos.y = 0;
#if DBG
    PathCount++;
    TRACE("Create Path %d\n",PathCount);
#endif
    return pPath;
}

/* PATH_DestroyGdiPath
 *
 * Destroys a GdiPath structure (frees the memory in the arrays).
 */
VOID
FASTCALL
PATH_DestroyGdiPath(PPATH pPath)
{
    ASSERT(pPath != NULL);

    if (pPath->pPoints) ExFreePoolWithTag(pPath->pPoints, TAG_PATH);
    if (pPath->pFlags) ExFreePoolWithTag(pPath->pFlags, TAG_PATH);
}

BOOL
FASTCALL
PATH_Delete(HPATH hPath)
{
    PPATH pPath;
    if (!hPath) return FALSE;
    pPath = PATH_LockPath(hPath);
    if (!pPath) return FALSE;
    PATH_DestroyGdiPath(pPath);
    GDIOBJ_vDeleteObject(&pPath->BaseObject);
#if DBG
    PathCount--;
    TRACE("Delete Path %d\n",PathCount);
#endif
    return TRUE;
}


VOID
FASTCALL
IntGdiCloseFigure(PPATH pPath)
{
    ASSERT(pPath->state == PATH_Open);

    // FIXME: Shouldn't we draw a line to the beginning of the figure?
    // Set PT_CLOSEFIGURE on the last entry and start a new stroke
    if (pPath->numEntriesUsed)
    {
        pPath->pFlags[pPath->numEntriesUsed - 1] |= PT_CLOSEFIGURE;
        pPath->newStroke = TRUE;
    }
}

/* MSDN: This fails if the device coordinates exceed 27 bits, or if the converted
         logical coordinates exceed 32 bits. */
BOOL
FASTCALL
GdiPathDPtoLP(
    PDC pdc,
    PPOINT ppt,
    INT count)
{
    XFORMOBJ xo;

    XFORMOBJ_vInit(&xo, &pdc->pdcattr->mxDeviceToWorld);
    return XFORMOBJ_bApplyXform(&xo, XF_LTOL, count, (PPOINTL)ppt, (PPOINTL)ppt);
}

/* PATH_InitGdiPath
 *
 * Initializes the GdiPath structure.
 */
VOID
FASTCALL
PATH_InitGdiPath(
    PPATH pPath)
{
    ASSERT(pPath != NULL);

    pPath->state = PATH_Null;
    pPath->pPoints = NULL;
    pPath->pFlags = NULL;
    pPath->numEntriesUsed = 0;
    pPath->numEntriesAllocated = 0;
}

/* PATH_AssignGdiPath
 *
 * Copies the GdiPath structure "pPathSrc" to "pPathDest". A deep copy is
 * performed, i.e. the contents of the pPoints and pFlags arrays are copied,
 * not just the pointers. Since this means that the arrays in pPathDest may
 * need to be resized, pPathDest should have been initialized using
 * PATH_InitGdiPath (in C++, this function would be an assignment operator,
 * not a copy constructor).
 * Returns TRUE if successful, else FALSE.
 */
BOOL
FASTCALL
PATH_AssignGdiPath(
    PPATH pPathDest,
    const PPATH pPathSrc)
{
    ASSERT(pPathDest != NULL && pPathSrc != NULL);

    /* Make sure destination arrays are big enough */
    if (!PATH_ReserveEntries(pPathDest, pPathSrc->numEntriesUsed))
        return FALSE;

    /* Perform the copy operation */
    memcpy(pPathDest->pPoints, pPathSrc->pPoints, sizeof(POINT)*pPathSrc->numEntriesUsed);
    memcpy(pPathDest->pFlags,  pPathSrc->pFlags,  sizeof(BYTE)*pPathSrc->numEntriesUsed);

    pPathDest->pos = pPathSrc->pos;
    pPathDest->state = pPathSrc->state;
    pPathDest->numEntriesUsed = pPathSrc->numEntriesUsed;
    pPathDest->newStroke = pPathSrc->newStroke;
    return TRUE;
}

BOOL PATH_SavePath( DC *dst, DC *src )
{
    PPATH pdstPath, psrcPath = PATH_LockPath(src->dclevel.hPath);
    TRACE("PATH_SavePath\n");
    if (psrcPath)
    {
       TRACE("PATH_SavePath 1\n");

       pdstPath = PATH_CreatePath(psrcPath->numEntriesAllocated);

       dst->dclevel.flPath = src->dclevel.flPath;

       dst->dclevel.hPath = pdstPath->BaseObject.hHmgr;

       PATH_AssignGdiPath(pdstPath, psrcPath);

       PATH_UnlockPath(pdstPath);
       PATH_UnlockPath(psrcPath);
    }
    return TRUE;
}

BOOL PATH_RestorePath( DC *dst, DC *src )
{
    TRACE("PATH_RestorePath\n");

    if (dst->dclevel.hPath == NULL)
    {
       PPATH pdstPath, psrcPath = PATH_LockPath(src->dclevel.hPath);
       TRACE("PATH_RestorePath 1\n");
       pdstPath = PATH_CreatePath(psrcPath->numEntriesAllocated);
       dst->dclevel.flPath = src->dclevel.flPath;
       dst->dclevel.hPath = pdstPath->BaseObject.hHmgr;

       PATH_AssignGdiPath(pdstPath, psrcPath);

       PATH_UnlockPath(pdstPath);
       PATH_UnlockPath(psrcPath);
    }
    else
    {
       PPATH pdstPath, psrcPath = PATH_LockPath(src->dclevel.hPath);
       pdstPath = PATH_LockPath(dst->dclevel.hPath);
       TRACE("PATH_RestorePath 2\n");
       dst->dclevel.flPath = src->dclevel.flPath & (DCPATH_CLOCKWISE|DCPATH_ACTIVE);
       PATH_AssignGdiPath(pdstPath, psrcPath);

       PATH_UnlockPath(pdstPath);
       PATH_UnlockPath(psrcPath);
    }
    return TRUE;
}

/* PATH_EmptyPath
 *
 * Removes all entries from the path and sets the path state to PATH_Null.
 */
VOID
FASTCALL
PATH_EmptyPath(PPATH pPath)
{
    ASSERT(pPath != NULL);

    pPath->state = PATH_Null;
    pPath->numEntriesUsed = 0;
}

/* PATH_AddEntry
 *
 * Adds an entry to the path. For "flags", pass either PT_MOVETO, PT_LINETO
 * or PT_BEZIERTO, optionally ORed with PT_CLOSEFIGURE. Returns TRUE if
 * successful, FALSE otherwise (e.g. if not enough memory was available).
 */
BOOL
FASTCALL
PATH_AddEntry(
    PPATH pPath,
    const POINT *pPoint,
    BYTE flags)
{
    ASSERT(pPath != NULL);

    /* FIXME: If newStroke is true, perhaps we want to check that we're
     * getting a PT_MOVETO
     */
    TRACE("(%d,%d) - %d\n", pPoint->x, pPoint->y, flags);

    /* Reserve enough memory for an extra path entry */
    if (!PATH_ReserveEntries(pPath, pPath->numEntriesUsed + 1))
        return FALSE;

    /* Store information in path entry */
    pPath->pPoints[pPath->numEntriesUsed] = *pPoint;
    pPath->pFlags[pPath->numEntriesUsed] = flags;

    /* Increment entry count */
    pPath->numEntriesUsed++;

    return TRUE;
}

/* PATH_ReserveEntries
 *
 * Ensures that at least "numEntries" entries (for points and flags) have
 * been allocated; allocates larger arrays and copies the existing entries
 * to those arrays, if necessary. Returns TRUE if successful, else FALSE.
 */
BOOL
FASTCALL
PATH_ReserveEntries(
    PPATH pPath,
    INT numEntries)
{
    INT numEntriesToAllocate;
    POINT *pPointsNew;
    BYTE *pFlagsNew;

    ASSERT(pPath != NULL);
    ASSERT(numEntries >= 0);

    /* Do we have to allocate more memory? */
    if (numEntries > pPath->numEntriesAllocated)
    {
        /* Find number of entries to allocate. We let the size of the array
         * grow exponentially, since that will guarantee linear time
         * complexity. */
        if (pPath->numEntriesAllocated)
        {
            numEntriesToAllocate = pPath->numEntriesAllocated;
            while (numEntriesToAllocate < numEntries)
                numEntriesToAllocate = numEntriesToAllocate * GROW_FACTOR_NUMER / GROW_FACTOR_DENOM;
        }
        else
            numEntriesToAllocate = numEntries;

        /* Allocate new arrays */
        pPointsNew = (POINT *)ExAllocatePoolWithTag(PagedPool, numEntriesToAllocate * sizeof(POINT), TAG_PATH);
        if (!pPointsNew)
            return FALSE;

        pFlagsNew = (BYTE *)ExAllocatePoolWithTag(PagedPool, numEntriesToAllocate * sizeof(BYTE), TAG_PATH);
        if (!pFlagsNew)
        {
            ExFreePoolWithTag(pPointsNew, TAG_PATH);
            return FALSE;
        }

        /* Copy old arrays to new arrays and discard old arrays */
        if (pPath->pPoints)
        {
            ASSERT(pPath->pFlags);

            memcpy(pPointsNew, pPath->pPoints, sizeof(POINT)*pPath->numEntriesUsed);
            memcpy(pFlagsNew, pPath->pFlags, sizeof(BYTE)*pPath->numEntriesUsed);

            ExFreePoolWithTag(pPath->pPoints, TAG_PATH);
            ExFreePoolWithTag(pPath->pFlags, TAG_PATH);
        }

        pPath->pPoints = pPointsNew;
        pPath->pFlags = pFlagsNew;
        pPath->numEntriesAllocated = numEntriesToAllocate;
    }

    return TRUE;
}

/* PATH_ScaleNormalizedPoint
 *
 * Scales a normalized point (x, y) with respect to the box whose corners are
 * passed in "corners". The point is stored in "*pPoint". The normalized
 * coordinates (-1.0, -1.0) correspond to corners[0], the coordinates
 * (1.0, 1.0) correspond to corners[1].
 */
static
BOOLEAN
PATH_ScaleNormalizedPoint(
    POINT corners[],
    FLOATL x,
    FLOATL y,
    POINT *pPoint)
{
    FLOATOBJ tmp;

    ASSERT(corners);
    ASSERT(pPoint);

    /* pPoint->x = (double)corners[0].x + (double)(corners[1].x - corners[0].x) * 0.5 * (x + 1.0); */
    FLOATOBJ_SetFloat(&tmp, x);
    FLOATOBJ_Add(&tmp, (FLOATOBJ*)&gef1);
    FLOATOBJ_Div(&tmp, (FLOATOBJ*)&gef2);
    FLOATOBJ_MulLong(&tmp, corners[1].x - corners[0].x);
    FLOATOBJ_AddLong(&tmp, corners[0].x);
    if (!FLOATOBJ_bConvertToLong(&tmp, &pPoint->x))
        return FALSE;

    /* pPoint->y = (double)corners[0].y + (double)(corners[1].y - corners[0].y) * 0.5 * (y + 1.0); */
    FLOATOBJ_SetFloat(&tmp, y);
    FLOATOBJ_Add(&tmp, (FLOATOBJ*)&gef1);
    FLOATOBJ_Div(&tmp, (FLOATOBJ*)&gef2);
    FLOATOBJ_MulLong(&tmp, corners[1].y - corners[0].y);
    FLOATOBJ_AddLong(&tmp, corners[0].y);
    if (!FLOATOBJ_bConvertToLong(&tmp, &pPoint->y))
        return FALSE;
    return TRUE;
}

/* PATH_NormalizePoint
 *
 * Normalizes a point with respect to the box whose corners are passed in
 * corners. The normalized coordinates are stored in *pX and *pY.
 */
static
VOID
PATH_NormalizePoint(
    POINTL corners[],
    const POINTL *pPoint,
    FLOATL *pX,
    FLOATL *pY)
{
    FLOATOBJ tmp;

    ASSERT(corners);
    ASSERT(pPoint);
    ASSERT(pX);
    ASSERT(pY);

    /* *pX = (float)(pPoint->x - corners[0].x) / (float)(corners[1].x - corners[0].x) * 2.0 - 1.0; */
    FLOATOBJ_SetLong(&tmp, (pPoint->x - corners[0].x) * 2);
    FLOATOBJ_DivLong(&tmp, corners[1].x - corners[0].x);
    FLOATOBJ_Sub(&tmp, (PFLOATOBJ)&gef1);
    *pX = FLOATOBJ_GetFloat(&tmp);

    /* *pY = (float)(pPoint->y - corners[0].y) / (float)(corners[1].y - corners[0].y) * 2.0 - 1.0; */
    FLOATOBJ_SetLong(&tmp, (pPoint->y - corners[0].y) * 2);
    FLOATOBJ_DivLong(&tmp, corners[1].y - corners[0].y);
    FLOATOBJ_Sub(&tmp, (PFLOATOBJ)&gef1);
    *pY = FLOATOBJ_GetFloat(&tmp);
}

/* PATH_CheckCorners
 *
 * Helper function for PATH_RoundRect() and PATH_Rectangle()
 */
static
BOOL
PATH_CheckRect(
    DC *dc,
    RECTL* rect,
    INT x1,
    INT y1,
    INT x2,
    INT y2)
{
    PDC_ATTR pdcattr = dc->pdcattr;

    /* Convert points to device coordinates */
    RECTL_vSetRect(rect, x1, y1, x2, y2);
    IntLPtoDP(dc, (PPOINT)rect, 2);

    /* Make sure first corner is top left and second corner is bottom right */
    RECTL_vMakeWellOrdered(rect);

    /* In GM_COMPATIBLE, don't include bottom and right edges */
    if (pdcattr->iGraphicsMode == GM_COMPATIBLE)
    {
        if (rect->left == rect->right) return FALSE;
        if (rect->top == rect->bottom) return FALSE;
        rect->right--;
        rect->bottom--;
    }
    return TRUE;
}

/* add a number of points, converting them to device coords */
/* return a pointer to the first type byte so it can be fixed up if necessary */
static BYTE *add_log_points( DC *dc, PPATH path, const POINT *points,
                             DWORD count, BYTE type )
{
    BYTE *ret;

    if (!PATH_ReserveEntries( path, path->numEntriesUsed + count )) return NULL;

    ret = &path->pFlags[path->numEntriesUsed];
    memcpy( &path->pPoints[path->numEntriesUsed], points, count * sizeof(*points) );
    IntLPtoDP( dc, &path->pPoints[path->numEntriesUsed], count );
    memset( ret, type, count );
    path->numEntriesUsed += count;
    return ret;
}

/* add a number of points that are already in device coords */
/* return a pointer to the first type byte so it can be fixed up if necessary */
static BYTE *add_points( PPATH path, const POINT *points, DWORD count, BYTE type )
{
    BYTE *ret;

    if (!PATH_ReserveEntries( path, path->numEntriesUsed + count )) return NULL;

    ret = &path->pFlags[path->numEntriesUsed];
    memcpy( &path->pPoints[path->numEntriesUsed], points, count * sizeof(*points) );
    memset( ret, type, count );
    path->numEntriesUsed += count;
    return ret;
}

/* reverse the order of an array of points */
static void reverse_points( POINT *points, UINT count )
{
    UINT i;
    for (i = 0; i < count / 2; i++)
    {
        POINT pt = points[i];
        points[i] = points[count - i - 1];
        points[count - i - 1] = pt;
    }
}

/* start a new path stroke if necessary */
static BOOL start_new_stroke( PPATH path )
{
    if (!path->newStroke && path->numEntriesUsed &&
        !(path->pFlags[path->numEntriesUsed - 1] & PT_CLOSEFIGURE) &&
        path->pPoints[path->numEntriesUsed - 1].x == path->pos.x &&
        path->pPoints[path->numEntriesUsed - 1].y == path->pos.y)
        return TRUE;

    path->newStroke = FALSE;
    return add_points( path, &path->pos, 1, PT_MOVETO ) != NULL;
}

/* set current position to the last point that was added to the path */
static void update_current_pos( PPATH path )
{
    ASSERT(path->numEntriesUsed);
    path->pos = path->pPoints[path->numEntriesUsed - 1];
}

/* close the current figure */
static void close_figure( PPATH path )
{
    ASSERT(path->numEntriesUsed);
    path->pFlags[path->numEntriesUsed - 1] |= PT_CLOSEFIGURE;
}

/* add a number of points, starting a new stroke if necessary */
static BOOL add_log_points_new_stroke( DC *dc, PPATH path, const POINT *points,
                                       DWORD count, BYTE type )
{
    if (!start_new_stroke( path )) return FALSE;
    if (!add_log_points( dc, path, points, count, type )) return FALSE;
    update_current_pos( path );

    TRACE("ALPNS : Pos X %d Y %d\n",path->pos.x, path->pos.y);
    IntGdiMoveToEx(dc, path->pos.x, path->pos.y, NULL);

    return TRUE;
}

/* PATH_MoveTo
 *
 * Should be called when a MoveTo is performed on a DC that has an
 * open path. This starts a new stroke. Returns TRUE if successful, else
 * FALSE.
 */
BOOL
FASTCALL
PATH_MoveTo(
    PDC dc,
    PPATH pPath)
{
    if (!pPath) return FALSE;

    // GDI32 : Signal from user space of a change in position.
    if (dc->pdcattr->ulDirty_ & DIRTY_STYLESTATE)
    {
       TRACE("MoveTo has changed\n");
       pPath->newStroke = TRUE;
       // Set position and clear the signal flag.
       IntGetCurrentPositionEx(dc, &pPath->pos);
       IntLPtoDP( dc, &pPath->pos, 1 );
       return TRUE;
    }

    return FALSE;
}

/* PATH_LineTo
 *
 * Should be called when a LineTo is performed on a DC that has an
 * open path. This adds a PT_LINETO entry to the path (and possibly
 * a PT_MOVETO entry, if this is the first LineTo in a stroke).
 * Returns TRUE if successful, else FALSE.
 */
BOOL
FASTCALL
PATH_LineTo(
    PDC dc,
    INT x,
    INT y)
{
    BOOL Ret;
    PPATH pPath;
    POINT point, pointCurPos;

    pPath = PATH_LockPath(dc->dclevel.hPath);
    if (!pPath) return FALSE;

    point.x = x;
    point.y = y;

    // Coalesce a MoveTo point.
    if ( !PATH_MoveTo(dc, pPath) )
    {
       /* Add a PT_MOVETO if necessary */
       if (pPath->newStroke)
       {
           TRACE("Line To : New Stroke\n");
           pPath->newStroke = FALSE;
           IntGetCurrentPositionEx(dc, &pointCurPos);
           CoordLPtoDP(dc, &pointCurPos);
           if (!PATH_AddEntry(pPath, &pointCurPos, PT_MOVETO))
           {
               PATH_UnlockPath(pPath);
               return FALSE;
           }
       }
    }
    Ret = add_log_points_new_stroke( dc, pPath, &point, 1, PT_LINETO );
    PATH_UnlockPath(pPath);
    return Ret;
}

/* PATH_Rectangle
 *
 * Should be called when a call to Rectangle is performed on a DC that has
 * an open path. Returns TRUE if successful, else FALSE.
 */
BOOL
FASTCALL
PATH_Rectangle(
    PDC dc,
    INT x1,
    INT y1,
    INT x2,
    INT y2)
{
    PPATH pPath;
    RECTL rect;
    POINTL points[4];
    BYTE *type;

    pPath = PATH_LockPath(dc->dclevel.hPath);
    if (!pPath) return FALSE;

    if (!PATH_CheckRect(dc, &rect, x1, y1, x2, y2))
    {
        PATH_UnlockPath(pPath);
        return TRUE;
    }

    points[0].x = rect.right; points[0].y = rect.top;
    points[1].x = rect.left; points[1].y = rect.top;
    points[2].x = rect.left; points[2].y = rect.bottom;
    points[3].x = rect.right; points[3].y = rect.bottom;

    if (dc->dclevel.flPath & DCPATH_CLOCKWISE) reverse_points(points, 4 );

    if (!(type = add_points( pPath, points, 4, PT_LINETO )))
    {
       PATH_UnlockPath(pPath);
       return FALSE;
    }
    type[0] = PT_MOVETO;

    /* Close the rectangle figure */
    IntGdiCloseFigure(pPath) ;
    PATH_UnlockPath(pPath);
    return TRUE;
}

/* PATH_RoundRect
 *
 * Should be called when a call to RoundRect is performed on a DC that has
 * an open path. Returns TRUE if successful, else FALSE.
 *
 */
BOOL
PATH_RoundRect(
    DC *dc,
    INT x1,
    INT y1,
    INT x2,
    INT y2,
    INT ell_width,
    INT ell_height)
{
    PPATH pPath;
    RECTL rect, ellipse;
    POINT points[16];
    BYTE *type;
    INT xOffset, yOffset;

    if (!ell_width || !ell_height) return PATH_Rectangle( dc, x1, y1, x2, y2 );

    pPath = PATH_LockPath(dc->dclevel.hPath);
    if (!pPath) return FALSE;

    if (!PATH_CheckRect(dc, &rect, x1, y1, x2, y2))
    {
        PATH_UnlockPath(pPath);
        return TRUE;
    }

    RECTL_vSetRect(&ellipse, 0, 0, ell_width, ell_height);
    IntLPtoDP( dc, (PPOINT)&ellipse, 2 );
    RECTL_vMakeWellOrdered(&ellipse);
    ell_width = min(RECTL_lGetWidth(&ellipse), RECTL_lGetWidth(&rect));
    ell_height = min(RECTL_lGetHeight(&ellipse), RECTL_lGetHeight(&rect));

    /*
     * See here to understand what's happening
     * https://stackoverflow.com/questions/1734745/how-to-create-circle-with-b%C3%A9zier-curves
     */
    xOffset = EngMulDiv(ell_width, 44771525, 200000000); /* w * (1 - 0.5522847) / 2 */
    yOffset = EngMulDiv(ell_height, 44771525, 200000000); /* h * (1 - 0.5522847) / 2 */
    TRACE("xOffset %d, yOffset %d, Rect WxH: %dx%d.\n",
        xOffset, yOffset, RECTL_lGetWidth(&rect), RECTL_lGetHeight(&rect));

    /*
     * Get half width & height.
     * Do not use integer division, we need the rounding made by EngMulDiv.
     */
    ell_width = EngMulDiv(ell_width, 1, 2);
    ell_height = EngMulDiv(ell_height, 1, 2);

    /* starting point */
    points[0].x  = rect.right;
    points[0].y  = rect.top + ell_height;
    /* first curve */
    points[1].x = rect.right;
    points[1].y = rect.top + yOffset;
    points[2].x = rect.right - xOffset;
    points[2].y = rect.top;
    points[3].x  = rect.right - ell_width;
    points[3].y  = rect.top;
    /* horizontal line */
    points[4].x  = rect.left + ell_width;
    points[4].y  = rect.top;
    /* second curve */
    points[5].x  = rect.left + xOffset;
    points[5].y  = rect.top;
    points[6].x  = rect.left;
    points[6].y  = rect.top + yOffset;
    points[7].x  = rect.left;
    points[7].y  = rect.top + ell_height;
    /* vertical line */
    points[8].x  = rect.left;
    points[8].y  = rect.bottom - ell_height;
    /* third curve */
    points[9].x  = rect.left;
    points[9].y  = rect.bottom - yOffset;
    points[10].x = rect.left + xOffset;
    points[10].y = rect.bottom;
    points[11].x = rect.left + ell_width;
    points[11].y = rect.bottom;
    /* horizontal line */
    points[12].x = rect.right - ell_width;
    points[12].y = rect.bottom;
    /* fourth curve */
    points[13].x = rect.right - xOffset;
    points[13].y = rect.bottom;
    points[14].x = rect.right;
    points[14].y = rect.bottom - yOffset;
    points[15].x = rect.right;
    points[15].y = rect.bottom - ell_height;

    if (dc->dclevel.flPath & DCPATH_CLOCKWISE) reverse_points( points, 16 );
    if (!(type = add_points( pPath, points, 16, PT_BEZIERTO )))
    {
       PATH_UnlockPath(pPath);
       return FALSE;
    }
    type[0] = PT_MOVETO;
    type[4] = type[8] = type[12] = PT_LINETO;

    IntGdiCloseFigure(pPath);
    PATH_UnlockPath(pPath);
    return TRUE;
}

/* PATH_Ellipse
 *
 */
BOOL
PATH_Ellipse(
    PDC dc,
    INT x1,
    INT y1,
    INT x2,
    INT y2)
{
    PPATH pPath;
    POINT points[13];
    RECTL rect;
    BYTE *type;
    LONG xRadius, yRadius, xOffset, yOffset;
    POINT left, top, right, bottom;

    TRACE("PATH_Ellipse: %p -> (%d, %d) - (%d, %d)\n",
            dc, x1, y1, x2, y2);

    if (!PATH_CheckRect(dc, &rect, x1, y1, x2, y2))
    {
        return TRUE;
    }

    xRadius = RECTL_lGetWidth(&rect) / 2;
    yRadius = RECTL_lGetHeight(&rect) / 2;

    /* Get the four points which box our ellipse */
    left.x = rect.left; left.y = rect.top + yRadius;
    top.x = rect.left + xRadius; top.y = rect.top;
    right.x = rect.right; right.y = rect.bottom - yRadius;
    bottom.x = rect.right - xRadius; bottom.y = rect.bottom;

    /*
     * See here to understand what's happening
     * https://stackoverflow.com/questions/1734745/how-to-create-circle-with-b%C3%A9zier-curves
     */
    xOffset = EngMulDiv(RECTL_lGetWidth(&rect), 55428475, 200000000); /* w * 0.55428475 / 2 */
    yOffset = EngMulDiv(RECTL_lGetHeight(&rect), 55428475, 200000000); /* h * 0.55428475 / 2 */
    TRACE("xOffset %d, yOffset %d, Rect WxH: %dx%d.\n",
        xOffset, yOffset, RECTL_lGetWidth(&rect), RECTL_lGetHeight(&rect));

    pPath = PATH_LockPath(dc->dclevel.hPath);
    if (!pPath)
        return FALSE;

    /* Starting point: Right */
    points[0] = right;

    /* first curve - going up, left */
    points[1] = right;
    points[1].y -= yOffset;
    points[2] = top;
    points[2].x += xOffset;

    /* top */
    points[3] = top;

    /* second curve - going left, down*/
    points[4] = top;
    points[4].x -= xOffset;
    points[5] = left;
    points[5].y -= yOffset;

    /* Left */
    points[6] = left;

    /* Third curve - going down, right */
    points[7] = left;
    points[7].y += yOffset;
    points[8] = bottom;
    points[8].x -= xOffset;

    /* bottom */
    points[9] = bottom;

    /* Fourth curve - Going right, up */
    points[10] = bottom;
    points[10].x += xOffset;
    points[11] = right;
    points[11].y += yOffset;

    /* Back to starting point */
    points[12] = right;

    if (dc->dclevel.flPath & DCPATH_CLOCKWISE) reverse_points( points, 13 );
    if (!(type = add_points( pPath, points, 13, PT_BEZIERTO )))
    {
        ERR("PATH_Ellipse No add\n");
        PATH_UnlockPath(pPath);
        return FALSE;
    }
    type[0] = PT_MOVETO;

    IntGdiCloseFigure(pPath);
    PATH_UnlockPath(pPath);
    return TRUE;
}

/* PATH_DoArcPart
 *
 * Creates a Bezier spline that corresponds to part of an arc and appends the
 * corresponding points to the path. The start and end angles are passed in
 * "angleStart" and "angleEnd"; these angles should span a quarter circle
 * at most. If "startEntryType" is non-zero, an entry of that type for the first
 * control point is added to the path; otherwise, it is assumed that the current
 * position is equal to the first control point.
 */
static
BOOL
PATH_DoArcPart(
    PPATH pPath,
    POINT corners[],
    double angleStart,
    double angleEnd,
    BYTE startEntryType)
{
    double  halfAngle, a;
    float  xNorm[4], yNorm[4];
    POINT points[4];
    BYTE *type;
    int i, start;

    ASSERT(fabs(angleEnd - angleStart) <= M_PI_2);

    /* FIXME: Is there an easier way of computing this? */

    /* Compute control points */
    halfAngle = (angleEnd - angleStart) / 2.0;
    if (fabs(halfAngle) > 1e-8)
    {
        a = 4.0 / 3.0 * (1 - cos(halfAngle)) / sin(halfAngle);
        xNorm[0] = cos(angleStart);
        yNorm[0] = sin(angleStart);
        xNorm[1] = xNorm[0] - a * yNorm[0];
        yNorm[1] = yNorm[0] + a * xNorm[0];
        xNorm[3] = cos(angleEnd);
        yNorm[3] = sin(angleEnd);
        xNorm[2] = xNorm[3] + a * yNorm[3];
        yNorm[2] = yNorm[3] - a * xNorm[3];
    }
    else
        for (i = 0; i < 4; i++)
        {
            xNorm[i] = cos(angleStart);
            yNorm[i] = sin(angleStart);
        }

    /* Add starting point to path if desired */
    start = !startEntryType;

    /* Add remaining control points */
    for (i = start; i < 4; i++)
    {
        if (!PATH_ScaleNormalizedPoint(corners, *(FLOATL*)&xNorm[i], *(FLOATL*)&yNorm[i], &points[i]))
            return FALSE;
    }
    if (!(type = add_points( pPath, points + start, 4 - start, PT_BEZIERTO ))) return FALSE;
    if (!start) type[0] = startEntryType;

    return TRUE;
}

/* PATH_Arc
 *
 * Should be called when a call to Arc is performed on a DC that has
 * an open path. This adds up to five Bezier splines representing the arc
 * to the path. When 'lines' is 1, we add 1 extra line to get a chord,
 * when 'lines' is 2, we add 2 extra lines to get a pie, and when 'lines' is
 * -1 we add 1 extra line from the current DC position to the starting position
 * of the arc before drawing the arc itself (arcto). Returns TRUE if successful,
 * else FALSE.
 */
BOOL
FASTCALL
PATH_Arc(
    PDC dc,
    INT x1,
    INT y1,
    INT x2,
    INT y2,
    INT xStart,
    INT yStart,
    INT xEnd,
    INT yEnd,
    INT direction,
    INT lines)
{
    double  angleStart, angleEnd, angleStartQuadrant, angleEndQuadrant = 0.0;
    /* Initialize angleEndQuadrant to silence gcc's warning */
    FLOATL  x, y;
    POINT corners[2], pointStart, pointEnd;
    POINT   centre, pointCurPos;
    BOOL    start, end, Ret = TRUE;
    INT     temp;
    BOOL    clockwise;
    PPATH   pPath;

    /* FIXME: This function should check for all possible error returns */
    /* FIXME: Do we have to respect newStroke? */

    ASSERT(dc);

    pPath = PATH_LockPath(dc->dclevel.hPath);
    if (!pPath) return FALSE;

    if (direction)
       clockwise = ((direction == AD_CLOCKWISE) !=0 );
    else
       clockwise = ((dc->dclevel.flPath & DCPATH_CLOCKWISE) != 0);

    /* Check for zero height / width */
    /* FIXME: Only in GM_COMPATIBLE? */
    if (x1 == x2 || y1 == y2)
    {
        Ret = TRUE;
        goto ArcExit;
    }
    /* Convert points to device coordinates */
    corners[0].x = x1; corners[0].y = y1;
    corners[1].x = x2; corners[1].y = y2;
    pointStart.x = xStart; pointStart.y = yStart;
    pointEnd.x = xEnd; pointEnd.y = yEnd;
    INTERNAL_LPTODP(dc, corners, 2);
    INTERNAL_LPTODP(dc, &pointStart, 1);
    INTERNAL_LPTODP(dc, &pointEnd, 1);

    /* Make sure first corner is top left and second corner is bottom right */
    if (corners[0].x > corners[1].x)
    {
        temp = corners[0].x;
        corners[0].x = corners[1].x;
        corners[1].x = temp;
    }
    if (corners[0].y > corners[1].y)
    {
        temp = corners[0].y;
        corners[0].y = corners[1].y;
        corners[1].y = temp;
    }

    /* Compute start and end angle */
    PATH_NormalizePoint(corners, &pointStart, &x, &y);
    angleStart = atan2(*(FLOAT*)&y, *(FLOAT*)&x);
    PATH_NormalizePoint(corners, &pointEnd, &x, &y);
    angleEnd = atan2(*(FLOAT*)&y, *(FLOAT*)&x);

    /* Make sure the end angle is "on the right side" of the start angle */
    if (clockwise)
    {
        if (angleEnd <= angleStart)
        {
            angleEnd += 2 * M_PI;
            ASSERT(angleEnd >= angleStart);
        }
    }
    else
    {
        if (angleEnd >= angleStart)
        {
            angleEnd -= 2 * M_PI;
            ASSERT(angleEnd <= angleStart);
        }
    }

    /* In GM_COMPATIBLE, don't include bottom and right edges */
    if (dc->pdcattr->iGraphicsMode == GM_COMPATIBLE)
    {
        corners[1].x--;
        corners[1].y--;
    }

    /* arcto: Add a PT_MOVETO only if this is the first entry in a stroke */
    if (lines == GdiTypeArcTo && pPath->newStroke) // -1
    {
        pPath->newStroke = FALSE;
        IntGetCurrentPositionEx(dc, &pointCurPos);
        CoordLPtoDP(dc, &pointCurPos);
        if (!PATH_AddEntry(pPath, &pointCurPos, PT_MOVETO))
        {
            Ret = FALSE;
            goto ArcExit;
        }
    }

    /* Add the arc to the path with one Bezier spline per quadrant that the
     * arc spans */
    start = TRUE;
    end = FALSE;
    do
    {
        /* Determine the start and end angles for this quadrant */
        if (start)
        {
            angleStartQuadrant = angleStart;
            if (clockwise)
                angleEndQuadrant = (floor(angleStart / M_PI_2) + 1.0) * M_PI_2;
            else
                angleEndQuadrant = (ceil(angleStart / M_PI_2) - 1.0) * M_PI_2;
        }
        else
        {
            angleStartQuadrant = angleEndQuadrant;
            if (clockwise)
                angleEndQuadrant += M_PI_2;
            else
                angleEndQuadrant -= M_PI_2;
        }

        /* Have we reached the last part of the arc? */
        if ((clockwise && angleEnd < angleEndQuadrant) ||
            (!clockwise && angleEnd > angleEndQuadrant))
        {
            /* Adjust the end angle for this quadrant */
            angleEndQuadrant = angleEnd;
            end = TRUE;
        }

        /* Add the Bezier spline to the path */
        PATH_DoArcPart(pPath,
                       corners,
                       angleStartQuadrant,
                       angleEndQuadrant,
                       start ? (lines == GdiTypeArcTo ? PT_LINETO : PT_MOVETO) : FALSE); // -1
        start = FALSE;
    }
    while (!end);

    if (lines == GdiTypeArcTo)
    {
       update_current_pos( pPath );
    }
    else /* chord: close figure. pie: add line and close figure */
    if (lines == GdiTypeChord) // 1
    {
        IntGdiCloseFigure(pPath);
    }
    else if (lines == GdiTypePie) // 2
    {
        centre.x = (corners[0].x + corners[1].x) / 2;
        centre.y = (corners[0].y + corners[1].y) / 2;
        if (!PATH_AddEntry(pPath, &centre, PT_LINETO | PT_CLOSEFIGURE))
            Ret = FALSE;
    }
ArcExit:
    PATH_UnlockPath(pPath);
    return Ret;
}

BOOL
FASTCALL
PATH_PolyBezierTo(
    PDC dc,
    const POINT *pts,
    DWORD cbPoints)
{
    PPATH pPath;
    BOOL ret;

    ASSERT(dc);
    ASSERT(pts);
    ASSERT(cbPoints);

    pPath = PATH_LockPath(dc->dclevel.hPath);
    if (!pPath) return FALSE;

    ret = add_log_points_new_stroke( dc, pPath, pts, cbPoints, PT_BEZIERTO );

    PATH_UnlockPath(pPath);
    return ret;
}

BOOL
FASTCALL
PATH_PolyBezier(
    PDC dc,
    const POINT *pts,
    DWORD cbPoints)
{
    PPATH pPath;
    BYTE *type;

    ASSERT(dc);
    ASSERT(pts);
    ASSERT(cbPoints);

    pPath = PATH_LockPath(dc->dclevel.hPath);
    if (!pPath) return FALSE;

    type = add_log_points( dc, pPath, pts, cbPoints, PT_BEZIERTO );
    if (!type) return FALSE;

    type[0] = PT_MOVETO;

    PATH_UnlockPath(pPath);
    return TRUE;
}

BOOL
FASTCALL
PATH_PolyDraw(
    PDC dc,
    const POINT *pts,
    const BYTE *types,
    DWORD cbPoints)
{
    PPATH pPath;
    POINT orig_pos, cur_pos;
    ULONG i, lastmove = 0;

    pPath = PATH_LockPath(dc->dclevel.hPath);
    if (!pPath) return FALSE;

    if (pPath->state != PATH_Open)
    {
        PATH_UnlockPath(pPath);
        return FALSE;
    }

    for (i = 0; i < pPath->numEntriesUsed; i++) if (pPath->pFlags[i] == PT_MOVETO) lastmove = i;
    orig_pos = pPath->pos;

    IntGetCurrentPositionEx(dc, &cur_pos);

    TRACE("PPD : Current pos X %d Y %d\n",pPath->pos.x, pPath->pos.y);
    TRACE("PPD : last %d pos X %d Y %d\n",lastmove, pPath->pPoints[lastmove].x, pPath->pPoints[lastmove].y);


    for(i = 0; i < cbPoints; i++)
    {
        switch (types[i])
        {
        case PT_MOVETO:
            pPath->newStroke = TRUE;
            pPath->pos = pts[i];
            IntLPtoDP( dc, &pPath->pos, 1);
            lastmove = pPath->numEntriesUsed;
            break;
        case PT_LINETO:
        case PT_LINETO | PT_CLOSEFIGURE:
            if (!add_log_points_new_stroke( dc, pPath, &pts[i], 1, PT_LINETO ))
            {
               PATH_UnlockPath(pPath);
               return FALSE;
            }
            break;
        case PT_BEZIERTO:
            if ((i + 2 < cbPoints) && (types[i + 1] == PT_BEZIERTO) &&
                (types[i + 2] & ~PT_CLOSEFIGURE) == PT_BEZIERTO)
            {
                if (!add_log_points_new_stroke( dc, pPath, &pts[i], 3, PT_BEZIERTO ))
                {
                   PATH_UnlockPath(pPath);
                   return FALSE;
                }
                i += 2;
                break;
            }
            /* fall through */
        default:
            /* restore original position */
            pPath->pos = orig_pos;

            TRACE("PPD Bad   : pos X %d Y %d\n",pPath->pos.x, pPath->pos.y);

            IntGdiMoveToEx(dc, cur_pos.x, cur_pos.y, NULL);

            PATH_UnlockPath(pPath);
            return FALSE;
        }

        if (types[i] & PT_CLOSEFIGURE)
        {
            close_figure( pPath );
            pPath->pos = pPath->pPoints[lastmove];
            TRACE("PPD close : pos X %d Y %d\n",pPath->pos.x, pPath->pos.y);
        }
    }
    PATH_UnlockPath(pPath);
    return TRUE;
}

BOOL
FASTCALL
PATH_PolylineTo(
    PDC dc,
    const POINT *pts,
    DWORD cbPoints)
{
    PPATH pPath;
    BOOL ret;

    ASSERT(dc);
    ASSERT(pts);
    ASSERT(cbPoints);

    if (cbPoints < 1) return FALSE;

    pPath = PATH_LockPath(dc->dclevel.hPath);
    if (!pPath) return FALSE;

    ret = add_log_points_new_stroke( dc, pPath, pts, cbPoints, PT_LINETO );
    PATH_UnlockPath(pPath);
    return ret;
}

BOOL
FASTCALL
PATH_PolyPolygon(
    PDC dc,
    const POINT* pts,
    const INT* counts,
    UINT polygons)
{
    UINT poly, count;
    BYTE *type;
    PPATH pPath;

    ASSERT(dc);
    ASSERT(pts);
    ASSERT(counts);
    ASSERT(polygons);

    if (!polygons) return FALSE;

    pPath = PATH_LockPath(dc->dclevel.hPath);
    if (!pPath) return FALSE;


    for (poly = count = 0; poly < polygons; poly++)
    {
        if (counts[poly] < 2)
        {
           PATH_UnlockPath(pPath);
           return FALSE;
        }
        count += counts[poly];
    }

    type = add_log_points( dc, pPath, pts, count, PT_LINETO );
    if (!type)
    {
       PATH_UnlockPath(pPath);
       return FALSE;
    }

    /* make the first point of each polyline a PT_MOVETO, and close the last one */
    for (poly = 0; poly < polygons; type += counts[poly++])
    {
        type[0] = PT_MOVETO;
        type[counts[poly] - 1] = PT_LINETO | PT_CLOSEFIGURE;
    }
    PATH_UnlockPath(pPath);
    return TRUE;
}

BOOL
FASTCALL
PATH_PolyPolyline(
    PDC dc,
    const POINT* pts,
    const DWORD* counts,
    DWORD polylines)
{
    POINT pt;
    ULONG poly, point, i;
    PPATH pPath;

    ASSERT(dc);
    ASSERT(pts);
    ASSERT(counts);
    ASSERT(polylines);

    pPath = PATH_LockPath(dc->dclevel.hPath);
    if (!pPath)
    {
       return FALSE;
    }

    for (i = 0, poly = 0; poly < polylines; poly++)
    {
        for (point = 0; point < counts[poly]; point++, i++)
        {
            pt = pts[i];
            CoordLPtoDP(dc, &pt);
            PATH_AddEntry(pPath, &pt, (point == 0) ? PT_MOVETO : PT_LINETO);
        }
    }
    TRACE("PATH_PolyPolyline end count %d\n",pPath->numEntriesUsed);
    PATH_UnlockPath(pPath);
    return TRUE;
}

/* PATH_AddFlatBezier
 *
 */
BOOL
FASTCALL
PATH_AddFlatBezier(
    PPATH pPath,
    POINT *pt,
    BOOL closed)
{
    POINT *pts;
    BOOL ret = FALSE;
    INT no, i;

    pts = GDI_Bezier(pt, 4, &no);
    if (!pts) return FALSE;

    for (i = 1; i < no; i++)
    {
        if (!(ret = PATH_AddEntry(pPath, &pts[i],  (i == no - 1 && closed) ? PT_LINETO | PT_CLOSEFIGURE : PT_LINETO)))
           break;
    }

    ExFreePoolWithTag(pts, TAG_BEZIER);
    return ret;
}

/* PATH_FlattenPath
 *
 * Replaces Beziers with line segments
 *
 */
PPATH
FASTCALL
PATH_FlattenPath(PPATH pPath)
{
    PPATH newPath;
    INT srcpt;
    TRACE("PATH_FlattenPath\n");
    if (!(newPath = PATH_CreatePath(pPath->numEntriesUsed))) return NULL;

    for (srcpt = 0; srcpt < pPath->numEntriesUsed; srcpt++)
    {
        switch(pPath->pFlags[srcpt] & ~PT_CLOSEFIGURE)
        {
            case PT_MOVETO:
            case PT_LINETO:
                if (!PATH_AddEntry(newPath, &pPath->pPoints[srcpt], pPath->pFlags[srcpt]))
                {
                   PATH_UnlockPath(newPath);
                   PATH_Delete(newPath->BaseObject.hHmgr);
                   return NULL;
                }
                break;
            case PT_BEZIERTO:
                if(!PATH_AddFlatBezier(newPath, &pPath->pPoints[srcpt - 1], pPath->pFlags[srcpt + 2] & PT_CLOSEFIGURE))
                {
                   PATH_UnlockPath(newPath);
                   PATH_Delete(newPath->BaseObject.hHmgr);
                   return NULL;
                }
                srcpt += 2;
                break;
        }
    }
    TRACE("PATH_FlattenPath good\n");
    newPath->state = pPath->state;
    return newPath;
}

/* PATH_PathToRegion
 *
 * Creates a region from the specified path using the specified polygon
 * filling mode. The path is left unchanged. A handle to the region that
 * was created is stored in *pHrgn.
 */
BOOL
FASTCALL
PATH_PathToRegion(
    PPATH pPath,
    INT Mode,
    PREGION Rgn)
{
    int i, pos, polygons;
    PULONG counts;
    int Ret;

    if (!pPath->numEntriesUsed) return FALSE;

    counts = ExAllocatePoolWithTag(PagedPool, (pPath->numEntriesUsed / 2) * sizeof(counts), TAG_PATH);
    if (!counts)
    {
        ERR("Failed to allocate %lu strokes\n", (pPath->numEntriesUsed / 2) * sizeof(*counts));
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    pos = polygons = 0;
    ASSERT( pPath->pFlags[0] == PT_MOVETO );
    for (i = 1; i < pPath->numEntriesUsed; i++)
    {
        if (pPath->pFlags[i] != PT_MOVETO) continue;
        counts[polygons++] = i - pos;
        pos = i;
    }
    if (i > pos + 1) counts[polygons++] = i - pos;

    ASSERT( polygons <= pPath->numEntriesUsed / 2 );

    /* Fill the region with the strokes */
    Ret = REGION_SetPolyPolygonRgn(Rgn,
                                   pPath->pPoints,
                                   counts,
                                   polygons,
                                   Mode);
    if (!Ret)
    {
        ERR("REGION_SetPolyPolygonRgn failed\n");
    }

    ExFreePoolWithTag(counts, TAG_PATH);

    /* Success! */
    return Ret;
}

/* PATH_FillPath
 *
 * You can play with this as long as you like, but if you break Area.exe the purge will Begain on Path!!!
 *
 */
BOOL
FASTCALL
PATH_FillPath(
    PDC dc,
    PPATH pPath)
{
    return PATH_FillPathEx(dc, pPath, NULL);
}

BOOL
FASTCALL
PATH_FillPathEx(
    PDC dc,
    PPATH pPath,
    PBRUSH pbrFill)
{
    INT   mapMode, graphicsMode;
    SIZE  ptViewportExt, ptWindowExt;
    POINTL ptViewportOrg, ptWindowOrg;
    XFORML xform;
    PREGION  Rgn;
    PDC_ATTR pdcattr = dc->pdcattr;

    /* Allocate a temporary region */
    Rgn = IntSysCreateRectpRgn(0, 0, 0, 0);
    if (!Rgn)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if (!PATH_PathToRegion(pPath, pdcattr->jFillMode, Rgn))
    {
        TRACE("PFP : Fail P2R\n");
        /* EngSetLastError ? */
        REGION_Delete(Rgn);
        return FALSE;
    }

    /* Since PaintRgn interprets the region as being in logical coordinates
     * but the points we store for the path are already in device
     * coordinates, we have to set the mapping mode to MM_TEXT temporarily.
     * Using SaveDC to save information about the mapping mode / world
     * transform would be easier but would require more overhead, especially
     * now that SaveDC saves the current path.
     */

    /* Save the information about the old mapping mode */
    mapMode = pdcattr->iMapMode;
    ptViewportExt = pdcattr->szlViewportExt;
    ptViewportOrg = pdcattr->ptlViewportOrg;
    ptWindowExt   = pdcattr->szlWindowExt;
    ptWindowOrg   = pdcattr->ptlWindowOrg;

    /* Save world transform
     * NB: The Windows documentation on world transforms would lead one to
     * believe that this has to be done only in GM_ADVANCED; however, my
     * tests show that resetting the graphics mode to GM_COMPATIBLE does
     * not reset the world transform.
     */
    MatrixS2XForm(&xform, &dc->pdcattr->mxWorldToPage);

    /* Set MM_TEXT */
    IntGdiSetMapMode(dc, MM_TEXT);
    pdcattr->ptlViewportOrg.x = 0;
    pdcattr->ptlViewportOrg.y = 0;
    pdcattr->ptlWindowOrg.x = 0;
    pdcattr->ptlWindowOrg.y = 0;

    graphicsMode = pdcattr->iGraphicsMode;
    pdcattr->iGraphicsMode = GM_ADVANCED;
    GreModifyWorldTransform(dc, &xform, MWT_IDENTITY);
    pdcattr->iGraphicsMode =  graphicsMode;

    /* Paint the region */
    IntGdiFillRgn(dc, Rgn, pbrFill);
    REGION_Delete(Rgn);
    /* Restore the old mapping mode */
    IntGdiSetMapMode(dc, mapMode);
    pdcattr->szlViewportExt = ptViewportExt;
    pdcattr->ptlViewportOrg = ptViewportOrg;
    pdcattr->szlWindowExt   = ptWindowExt;
    pdcattr->ptlWindowOrg   = ptWindowOrg;

    /* Go to GM_ADVANCED temporarily to restore the world transform */
    graphicsMode = pdcattr->iGraphicsMode;
    pdcattr->iGraphicsMode = GM_ADVANCED;
    GreModifyWorldTransform(dc, &xform, MWT_SET);
    pdcattr->iGraphicsMode = graphicsMode;
    return TRUE;
}

BOOL
FASTCALL
PATH_StrokePath(
    DC *dc,
    PPATH pPath)
{
    BOOL ret = FALSE;
    INT i = 0;
    INT nLinePts, nAlloc;
    POINT *pLinePts = NULL;
    POINT ptViewportOrg, ptWindowOrg;
    SIZE szViewportExt, szWindowExt;
    DWORD mapMode, graphicsMode;
    XFORM xform;
    PDC_ATTR pdcattr = dc->pdcattr;

    TRACE("Enter %s\n", __FUNCTION__);

    /* Save the mapping mode info */
    mapMode = pdcattr->iMapMode;

    szViewportExt = *DC_pszlViewportExt(dc);
    ptViewportOrg = dc->pdcattr->ptlViewportOrg;
    szWindowExt = dc->pdcattr->szlWindowExt;
    ptWindowOrg = dc->pdcattr->ptlWindowOrg;

    MatrixS2XForm(&xform, &dc->pdcattr->mxWorldToPage);

    /* Set MM_TEXT */
    pdcattr->iMapMode = MM_TEXT;
    pdcattr->ptlViewportOrg.x = 0;
    pdcattr->ptlViewportOrg.y = 0;
    pdcattr->ptlWindowOrg.x = 0;
    pdcattr->ptlWindowOrg.y = 0;
    graphicsMode = pdcattr->iGraphicsMode;
    pdcattr->iGraphicsMode = GM_ADVANCED;
    GreModifyWorldTransform(dc, (XFORML*)&xform, MWT_IDENTITY);
    pdcattr->iGraphicsMode = graphicsMode;

    /* Allocate enough memory for the worst case without beziers (one PT_MOVETO
     * and the rest PT_LINETO with PT_CLOSEFIGURE at the end) plus some buffer
     * space in case we get one to keep the number of reallocations small. */
    nAlloc = pPath->numEntriesUsed + 1 + 300;
    pLinePts = ExAllocatePoolWithTag(PagedPool, nAlloc * sizeof(POINT), TAG_PATH);
    if (!pLinePts)
    {
        ERR("Can't allocate pool!\n");
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto end;
    }
    nLinePts = 0;

    for (i = 0; i < pPath->numEntriesUsed; i++)
    {
        if ((i == 0 || (pPath->pFlags[i - 1] & PT_CLOSEFIGURE))
                && (pPath->pFlags[i] != PT_MOVETO))
        {
            ERR("Expected PT_MOVETO %s, got path flag %d\n",
                    i == 0 ? "as first point" : "after PT_CLOSEFIGURE",
                    (INT)pPath->pFlags[i]);
            goto end;
        }

        switch(pPath->pFlags[i])
        {
            case PT_MOVETO:
                TRACE("Got PT_MOVETO (%ld, %ld)\n",
                       pPath->pPoints[i].x, pPath->pPoints[i].y);
                if (nLinePts >= 2) IntGdiPolyline(dc, pLinePts, nLinePts);
                nLinePts = 0;
                pLinePts[nLinePts++] = pPath->pPoints[i];
                break;
            case PT_LINETO:
            case (PT_LINETO | PT_CLOSEFIGURE):
                TRACE("Got PT_LINETO (%ld, %ld)\n",
                       pPath->pPoints[i].x, pPath->pPoints[i].y);
                pLinePts[nLinePts++] = pPath->pPoints[i];
                break;
            case PT_BEZIERTO:
                TRACE("Got PT_BEZIERTO\n");
                if (pPath->pFlags[i + 1] != PT_BEZIERTO ||
                        (pPath->pFlags[i + 2] & ~PT_CLOSEFIGURE) != PT_BEZIERTO)
                {
                    ERR("Path didn't contain 3 successive PT_BEZIERTOs\n");
                    ret = FALSE;
                    goto end;
                }
                else
                {
                    INT nBzrPts, nMinAlloc;
                    POINT *pBzrPts = GDI_Bezier(&pPath->pPoints[i - 1], 4, &nBzrPts);
                    /* Make sure we have allocated enough memory for the lines of
                     * this bezier and the rest of the path, assuming we won't get
                     * another one (since we won't reallocate again then). */
                    nMinAlloc = nLinePts + (pPath->numEntriesUsed - i) + nBzrPts;
                    if (nAlloc < nMinAlloc)
                    {
                        // Reallocate memory

                        POINT *Realloc = NULL;
                        nAlloc = nMinAlloc * 2;

                        Realloc = ExAllocatePoolWithTag(PagedPool,
                                                        nAlloc * sizeof(POINT),
                                                        TAG_PATH);

                        if (!Realloc)
                        {
                            ERR("Can't allocate pool!\n");
                            ExFreePoolWithTag(pBzrPts, TAG_BEZIER);
                            goto end;
                        }

                        memcpy(Realloc, pLinePts, nLinePts * sizeof(POINT));
                        ExFreePoolWithTag(pLinePts, TAG_PATH);
                        pLinePts = Realloc;
                    }
                    memcpy(&pLinePts[nLinePts], &pBzrPts[1], (nBzrPts - 1) * sizeof(POINT));
                    nLinePts += nBzrPts - 1;
                    ExFreePoolWithTag(pBzrPts, TAG_BEZIER);
                    i += 2;
                }
                break;
            default:
                ERR("Got path flag %d (not supported)\n", (INT)pPath->pFlags[i]);
                goto end;
        }

        if (pPath->pFlags[i] & PT_CLOSEFIGURE)
        {
            pLinePts[nLinePts++] = pLinePts[0];
        }
    }
    if (nLinePts >= 2)
        IntGdiPolyline(dc, pLinePts, nLinePts);

    ret = TRUE;

end:
    if (pLinePts) ExFreePoolWithTag(pLinePts, TAG_PATH);

    /* Restore the old mapping mode */
    pdcattr->iMapMode =  mapMode;
    pdcattr->szlWindowExt.cx = szWindowExt.cx;
    pdcattr->szlWindowExt.cy = szWindowExt.cy;
    pdcattr->ptlWindowOrg.x = ptWindowOrg.x;
    pdcattr->ptlWindowOrg.y = ptWindowOrg.y;

    pdcattr->szlViewportExt.cx = szViewportExt.cx;
    pdcattr->szlViewportExt.cy = szViewportExt.cy;
    pdcattr->ptlViewportOrg.x = ptViewportOrg.x;
    pdcattr->ptlViewportOrg.y = ptViewportOrg.y;

    /* Restore the world transform */
    XForm2MatrixS(&dc->pdcattr->mxWorldToPage, &xform);

    /* If we've moved the current point then get its new position
       which will be in device (MM_TEXT) co-ords, convert it to
       logical co-ords and re-set it.  This basically updates
       dc->CurPosX|Y so that their values are in the correct mapping
       mode.
    */
    if (i > 0)
    {
        POINT pt;
        IntGetCurrentPositionEx(dc, &pt);
        IntDPtoLP(dc, &pt, 1);
        IntGdiMoveToEx(dc, pt.x, pt.y, NULL);
    }
    TRACE("Leave %s, ret=%d\n", __FUNCTION__, ret);
    return ret;
}

#define round(x) ((int)((x)>0?(x)+0.5:(x)-0.5))

PPATH FASTCALL
IntGdiWidenPath(PPATH pPath, UINT penWidth, UINT penStyle, FLOAT eMiterLimit)
{
    INT i, j, numStrokes, numOldStrokes, penWidthIn, penWidthOut;
    PPATH flat_path, pNewPath, *pStrokes = NULL, *pOldStrokes, pUpPath, pDownPath;
    BYTE *type;
    DWORD joint, endcap;

    endcap = (PS_ENDCAP_MASK & penStyle);
    joint = (PS_JOIN_MASK & penStyle);

    if (!(flat_path = PATH_FlattenPath(pPath)))
    {
        ERR("PATH_FlattenPath\n");
        return NULL;
    }

    penWidthIn = penWidth / 2;
    penWidthOut = penWidth / 2;
    if (penWidthIn + penWidthOut < penWidth)
        penWidthOut++;

    numStrokes = 0;

    for (i = 0, j = 0; i < flat_path->numEntriesUsed; i++, j++)
    {
        POINT point;
        if ((i == 0 || (flat_path->pFlags[i - 1] & PT_CLOSEFIGURE)) &&
                (flat_path->pFlags[i] != PT_MOVETO))
        {
            ERR("Expected PT_MOVETO %s, got path flag %c\n",
                    i == 0 ? "as first point" : "after PT_CLOSEFIGURE",
                    flat_path->pFlags[i]);
            if (pStrokes)
                ExFreePoolWithTag(pStrokes, TAG_PATH);
            PATH_UnlockPath(flat_path);
            PATH_Delete(flat_path->BaseObject.hHmgr);
            return NULL;
        }
        switch(flat_path->pFlags[i])
        {
            case PT_MOVETO:
                if (numStrokes > 0)
                {
                    pStrokes[numStrokes - 1]->state = PATH_Closed;
                }
                numOldStrokes = numStrokes;
                numStrokes++;
                j = 0;
                if (numStrokes == 1)
                    pStrokes = ExAllocatePoolWithTag(PagedPool, sizeof(*pStrokes), TAG_PATH);
                else
                {
                    pOldStrokes = pStrokes; // Save old pointer.
                    pStrokes = ExAllocatePoolWithTag(PagedPool, numStrokes * sizeof(*pStrokes), TAG_PATH);
                    if (!pStrokes)
                    {
                       ExFreePoolWithTag(pOldStrokes, TAG_PATH);
                       PATH_UnlockPath(flat_path);
                       PATH_Delete(flat_path->BaseObject.hHmgr);
                       return NULL;
                    }
                    RtlCopyMemory(pStrokes, pOldStrokes, numOldStrokes * sizeof(PPATH));
                    ExFreePoolWithTag(pOldStrokes, TAG_PATH); // Free old pointer.
                }
                if (!pStrokes)
                {
                   PATH_UnlockPath(flat_path);
                   PATH_Delete(flat_path->BaseObject.hHmgr);
                   return NULL;
                }
                pStrokes[numStrokes - 1] = ExAllocatePoolWithTag(PagedPool, sizeof(PATH), TAG_PATH);
                if (!pStrokes[numStrokes - 1])
                {
                    ASSERT(FALSE); // FIXME
                }
                PATH_InitGdiPath(pStrokes[numStrokes - 1]);
                pStrokes[numStrokes - 1]->state = PATH_Open;
            case PT_LINETO:
            case (PT_LINETO | PT_CLOSEFIGURE):
                point.x = flat_path->pPoints[i].x;
                point.y = flat_path->pPoints[i].y;
                PATH_AddEntry(pStrokes[numStrokes - 1], &point, flat_path->pFlags[i]);
                break;
            case PT_BEZIERTO:
                /* Should never happen because of the FlattenPath call */
                ERR("Should never happen\n");
                break;
            default:
                ERR("Got path flag %c\n", flat_path->pFlags[i]);
                if (pStrokes)
                    ExFreePoolWithTag(pStrokes, TAG_PATH);
                PATH_UnlockPath(flat_path);
                PATH_Delete(flat_path->BaseObject.hHmgr);
                return NULL;
        }
    }

    pNewPath = PATH_CreatePath( flat_path->numEntriesUsed );

    for (i = 0; i < numStrokes; i++)
    {
        pUpPath = ExAllocatePoolWithTag(PagedPool, sizeof(PATH), TAG_PATH);
        PATH_InitGdiPath(pUpPath);
        pUpPath->state = PATH_Open;
        pDownPath = ExAllocatePoolWithTag(PagedPool, sizeof(PATH), TAG_PATH);
        PATH_InitGdiPath(pDownPath);
        pDownPath->state = PATH_Open;

        for (j = 0; j < pStrokes[i]->numEntriesUsed; j++)
        {
            /* Beginning or end of the path if not closed */
            if ((!(pStrokes[i]->pFlags[pStrokes[i]->numEntriesUsed - 1] & PT_CLOSEFIGURE)) && (j == 0 || j == pStrokes[i]->numEntriesUsed - 1))
            {
                /* Compute segment angle */
                INT xo, yo, xa, ya;
                double theta;
                POINT pt;
                POINT corners[2];
                if (j == 0)
                {
                    xo = pStrokes[i]->pPoints[j].x;
                    yo = pStrokes[i]->pPoints[j].y;
                    xa = pStrokes[i]->pPoints[1].x;
                    ya = pStrokes[i]->pPoints[1].y;
                }
                else
                {
                    xa = pStrokes[i]->pPoints[j - 1].x;
                    ya = pStrokes[i]->pPoints[j - 1].y;
                    xo = pStrokes[i]->pPoints[j].x;
                    yo = pStrokes[i]->pPoints[j].y;
                }
                theta = atan2(ya - yo, xa - xo);
                switch(endcap)
                {
                    case PS_ENDCAP_SQUARE :
                        pt.x = xo + round(sqrt(2) * penWidthOut * cos(M_PI_4 + theta));
                        pt.y = yo + round(sqrt(2) * penWidthOut * sin(M_PI_4 + theta));
                        PATH_AddEntry(pUpPath, &pt, (j == 0 ? PT_MOVETO : PT_LINETO));
                        pt.x = xo + round(sqrt(2) * penWidthIn * cos(- M_PI_4 + theta));
                        pt.y = yo + round(sqrt(2) * penWidthIn * sin(- M_PI_4 + theta));
                        PATH_AddEntry(pUpPath, &pt, PT_LINETO);
                        break;
                    case PS_ENDCAP_FLAT :
                        pt.x = xo + round(penWidthOut * cos(theta + M_PI_2));
                        pt.y = yo + round(penWidthOut * sin(theta + M_PI_2));
                        PATH_AddEntry(pUpPath, &pt, (j == 0 ? PT_MOVETO : PT_LINETO));
                        pt.x = xo - round(penWidthIn * cos(theta + M_PI_2));
                        pt.y = yo - round(penWidthIn * sin(theta + M_PI_2));
                        PATH_AddEntry(pUpPath, &pt, PT_LINETO);
                        break;
                    case PS_ENDCAP_ROUND :
                    default :
                        corners[0].x = xo - penWidthIn;
                        corners[0].y = yo - penWidthIn;
                        corners[1].x = xo + penWidthOut;
                        corners[1].y = yo + penWidthOut;
                        PATH_DoArcPart(pUpPath , corners, theta + M_PI_2 , theta + 3 * M_PI_4, (j == 0 ? PT_MOVETO : FALSE));
                        PATH_DoArcPart(pUpPath , corners, theta + 3 * M_PI_4 , theta + M_PI, FALSE);
                        PATH_DoArcPart(pUpPath , corners, theta + M_PI, theta +  5 * M_PI_4, FALSE);
                        PATH_DoArcPart(pUpPath , corners, theta + 5 * M_PI_4 , theta + 3 * M_PI_2, FALSE);
                        break;
                }
            }
            /* Corpse of the path */
            else
            {
                /* Compute angle */
                INT previous, next;
                double xa, ya, xb, yb, xo, yo;
                double alpha, theta, miterWidth;
                DWORD _joint = joint;
                POINT pt;
                PPATH pInsidePath, pOutsidePath;
                if (j > 0 && j < pStrokes[i]->numEntriesUsed - 1)
                {
                    previous = j - 1;
                    next = j + 1;
                }
                else if (j == 0)
                {
                    previous = pStrokes[i]->numEntriesUsed - 1;
                    next = j + 1;
                }
                else
                {
                    previous = j - 1;
                    next = 0;
                }
                xo = pStrokes[i]->pPoints[j].x;
                yo = pStrokes[i]->pPoints[j].y;
                xa = pStrokes[i]->pPoints[previous].x;
                ya = pStrokes[i]->pPoints[previous].y;
                xb = pStrokes[i]->pPoints[next].x;
                yb = pStrokes[i]->pPoints[next].y;
                theta = atan2(yo - ya, xo - xa);
                alpha = atan2(yb - yo, xb - xo) - theta;
                if (alpha > 0) alpha -= M_PI;
                else alpha += M_PI;
                if (_joint == PS_JOIN_MITER && eMiterLimit < fabs(1 / sin(alpha / 2)))
                {
                    _joint = PS_JOIN_BEVEL;
                }
                if (alpha > 0)
                {
                    pInsidePath = pUpPath;
                    pOutsidePath = pDownPath;
                }
                else if (alpha < 0)
                {
                    pInsidePath = pDownPath;
                    pOutsidePath = pUpPath;
                }
                else
                {
                    continue;
                }
                /* Inside angle points */
                if (alpha > 0)
                {
                    pt.x = xo - round(penWidthIn * cos(theta + M_PI_2));
                    pt.y = yo - round(penWidthIn * sin(theta + M_PI_2));
                }
                else
                {
                    pt.x = xo + round(penWidthIn * cos(theta + M_PI_2));
                    pt.y = yo + round(penWidthIn * sin(theta + M_PI_2));
                }
                PATH_AddEntry(pInsidePath, &pt, PT_LINETO);
                if (alpha > 0)
                {
                    pt.x = xo + round(penWidthIn * cos(M_PI_2 + alpha + theta));
                    pt.y = yo + round(penWidthIn * sin(M_PI_2 + alpha + theta));
                }
                else
                {
                    pt.x = xo - round(penWidthIn * cos(M_PI_2 + alpha + theta));
                    pt.y = yo - round(penWidthIn * sin(M_PI_2 + alpha + theta));
                }
                PATH_AddEntry(pInsidePath, &pt, PT_LINETO);
                /* Outside angle point */
                switch(_joint)
                {
                    case PS_JOIN_MITER :
                        miterWidth = fabs(penWidthOut / cos(M_PI_2 - fabs(alpha) / 2));
                        pt.x = xo + round(miterWidth * cos(theta + alpha / 2));
                        pt.y = yo + round(miterWidth * sin(theta + alpha / 2));
                        PATH_AddEntry(pOutsidePath, &pt, PT_LINETO);
                        break;
                    case PS_JOIN_BEVEL :
                        if (alpha > 0)
                        {
                            pt.x = xo + round(penWidthOut * cos(theta + M_PI_2));
                            pt.y = yo + round(penWidthOut * sin(theta + M_PI_2));
                        }
                        else
                        {
                            pt.x = xo - round(penWidthOut * cos(theta + M_PI_2));
                            pt.y = yo - round(penWidthOut * sin(theta + M_PI_2));
                        }
                        PATH_AddEntry(pOutsidePath, &pt, PT_LINETO);
                        if (alpha > 0)
                        {
                            pt.x = xo - round(penWidthOut * cos(M_PI_2 + alpha + theta));
                            pt.y = yo - round(penWidthOut * sin(M_PI_2 + alpha + theta));
                        }
                        else
                        {
                            pt.x = xo + round(penWidthOut * cos(M_PI_2 + alpha + theta));
                            pt.y = yo + round(penWidthOut * sin(M_PI_2 + alpha + theta));
                        }
                        PATH_AddEntry(pOutsidePath, &pt, PT_LINETO);
                        break;
                    case PS_JOIN_ROUND :
                    default :
                        if (alpha > 0)
                        {
                            pt.x = xo + round(penWidthOut * cos(theta + M_PI_2));
                            pt.y = yo + round(penWidthOut * sin(theta + M_PI_2));
                        }
                        else
                        {
                            pt.x = xo - round(penWidthOut * cos(theta + M_PI_2));
                            pt.y = yo - round(penWidthOut * sin(theta + M_PI_2));
                        }
                        PATH_AddEntry(pOutsidePath, &pt, PT_BEZIERTO);
                        pt.x = xo + round(penWidthOut * cos(theta + alpha / 2));
                        pt.y = yo + round(penWidthOut * sin(theta + alpha / 2));
                        PATH_AddEntry(pOutsidePath, &pt, PT_BEZIERTO);
                        if (alpha > 0)
                        {
                            pt.x = xo - round(penWidthOut * cos(M_PI_2 + alpha + theta));
                            pt.y = yo - round(penWidthOut * sin(M_PI_2 + alpha + theta));
                        }
                        else
                        {
                            pt.x = xo + round(penWidthOut * cos(M_PI_2 + alpha + theta));
                            pt.y = yo + round(penWidthOut * sin(M_PI_2 + alpha + theta));
                        }
                        PATH_AddEntry(pOutsidePath, &pt, PT_BEZIERTO);
                        break;
                }
            }
        }
        type = add_points( pNewPath, pUpPath->pPoints, pUpPath->numEntriesUsed, PT_LINETO );
        type[0] = PT_MOVETO;
        reverse_points( pDownPath->pPoints, pDownPath->numEntriesUsed );
        type = add_points( pNewPath, pDownPath->pPoints, pDownPath->numEntriesUsed, PT_LINETO );
        if (pStrokes[i]->pFlags[pStrokes[i]->numEntriesUsed - 1] & PT_CLOSEFIGURE) type[0] = PT_MOVETO;

        PATH_DestroyGdiPath(pStrokes[i]);
        ExFreePoolWithTag(pStrokes[i], TAG_PATH);
        PATH_DestroyGdiPath(pUpPath);
        ExFreePoolWithTag(pUpPath, TAG_PATH);
        PATH_DestroyGdiPath(pDownPath);
        ExFreePoolWithTag(pDownPath, TAG_PATH);
    }
    if (pStrokes) ExFreePoolWithTag(pStrokes, TAG_PATH);

    PATH_UnlockPath(flat_path);
    PATH_Delete(flat_path->BaseObject.hHmgr);
    pNewPath->state = PATH_Closed;
    PATH_UnlockPath(pNewPath);
    return pNewPath;
}

static
PPATH
FASTCALL
PATH_WidenPath(DC *dc)
{
    INT size;
    UINT penWidth, penStyle;
    DWORD obj_type;
    PPATH pPath, pNewPath;
    LPEXTLOGPEN elp;
    PDC_ATTR pdcattr = dc->pdcattr;

    pPath = PATH_LockPath(dc->dclevel.hPath);
    if (!pPath)
    {
        EngSetLastError( ERROR_CAN_NOT_COMPLETE );
        return NULL;
    }

    if (pPath->state != PATH_Closed)
    {
        TRACE("PWP 1\n");
        PATH_UnlockPath(pPath);
        EngSetLastError(ERROR_CAN_NOT_COMPLETE);
        return NULL;
    }

    size = GreGetObject(pdcattr->hpen, 0, NULL);
    if (!size)
    {
        TRACE("PWP 2\n");
        PATH_UnlockPath(pPath);
        EngSetLastError(ERROR_CAN_NOT_COMPLETE);
        return NULL;
    }

    elp = ExAllocatePoolWithTag(PagedPool, size, TAG_PATH);
    if (elp == NULL)
    {
        TRACE("PWP 3\n");
        PATH_UnlockPath(pPath);
        EngSetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }

    GreGetObject(pdcattr->hpen, size, elp);

    obj_type = GDI_HANDLE_GET_TYPE(pdcattr->hpen);
    if (obj_type == GDI_OBJECT_TYPE_PEN)
    {
        penStyle = ((LOGPEN*)elp)->lopnStyle;
    }
    else if (obj_type == GDI_OBJECT_TYPE_EXTPEN)
    {
        penStyle = elp->elpPenStyle;
    }
    else
    {
        TRACE("PWP 4\n");
        EngSetLastError(ERROR_CAN_NOT_COMPLETE);
        ExFreePoolWithTag(elp, TAG_PATH);
        PATH_UnlockPath(pPath);
        return NULL;
    }

    penWidth = elp->elpWidth;
    ExFreePoolWithTag(elp, TAG_PATH);

    /* The function cannot apply to cosmetic pens */
    if (obj_type == GDI_OBJECT_TYPE_EXTPEN &&
        (PS_TYPE_MASK & penStyle) == PS_COSMETIC)
    {
        TRACE("PWP 5\n");
        PATH_UnlockPath(pPath);
        EngSetLastError(ERROR_CAN_NOT_COMPLETE);
        return FALSE;
    }

    pNewPath = IntGdiWidenPath(pPath, penWidth, penStyle, dc->dclevel.laPath.eMiterLimit);
    PATH_UnlockPath(pPath);
    return pNewPath;
}

static inline INT int_from_fixed(FIXED f)
{
    return (f.fract >= 0x8000) ? (f.value + 1) : f.value;
}

/**********************************************************************
 *      PATH_BezierTo
 *
 * Internally used by PATH_add_outline
 */
static
VOID
FASTCALL
PATH_BezierTo(
    PPATH pPath,
    POINT *lppt,
    INT n)
{
    if (n < 2) return;

    if (n == 2)
    {
        PATH_AddEntry(pPath, &lppt[1], PT_LINETO);
    }
    else if (n == 3)
    {
        add_points( pPath, lppt, 3, PT_BEZIERTO );
    }
    else
    {
        POINT pt[3];
        INT i = 0;

        pt[2] = lppt[0];
        n--;

        while (n > 2)
        {
            pt[0] = pt[2];
            pt[1] = lppt[i + 1];
            pt[2].x = (lppt[i + 2].x + lppt[i + 1].x) / 2;
            pt[2].y = (lppt[i + 2].y + lppt[i + 1].y) / 2;
            add_points( pPath, pt, 3, PT_BEZIERTO );
            n--;
            i++;
        }

        pt[0] = pt[2];
        pt[1] = lppt[i + 1];
        pt[2] = lppt[i + 2];
        add_points( pPath, pt, 3, PT_BEZIERTO );
    }
}

static
BOOL
FASTCALL
PATH_add_outline(
    PDC dc,
    PPATH pPath,
    INT x,
    INT y,
    TTPOLYGONHEADER *header,
    DWORD size)
{
    TTPOLYGONHEADER *start;
    POINT pt;
    BOOL bResult = FALSE;

    start = header;

    while ((char *)header < (char *)start + size)
    {
        TTPOLYCURVE *curve;

        if (header->dwType != TT_POLYGON_TYPE)
        {
            ERR("Unknown header type %lu\n", header->dwType);
            goto cleanup;
        }

        pt.x = x + int_from_fixed(header->pfxStart.x);
        pt.y = y - int_from_fixed(header->pfxStart.y);
        PATH_AddEntry(pPath, &pt, PT_MOVETO);

        curve = (TTPOLYCURVE *)(header + 1);

        while ((char *)curve < (char *)header + header->cb)
        {
            TRACE("curve->wType %d\n", curve->wType);

            switch(curve->wType)
            {
                case TT_PRIM_LINE:
                {
                    WORD i;

                    for (i = 0; i < curve->cpfx; i++)
                    {
                        pt.x = x + int_from_fixed(curve->apfx[i].x);
                        pt.y = y - int_from_fixed(curve->apfx[i].y);
                        PATH_AddEntry(pPath, &pt, PT_LINETO);
                    }
                    break;
                }

                case TT_PRIM_QSPLINE:
                case TT_PRIM_CSPLINE:
                {
                    WORD i;
                    POINTFX ptfx;
                    POINT *pts = ExAllocatePoolWithTag(PagedPool, (curve->cpfx + 1) * sizeof(POINT), TAG_PATH);

                    if (!pts) goto cleanup;

                    ptfx = *(POINTFX *)((char *)curve - sizeof(POINTFX));

                    pts[0].x = x + int_from_fixed(ptfx.x);
                    pts[0].y = y - int_from_fixed(ptfx.y);

                    for (i = 0; i < curve->cpfx; i++)
                    {
                        pts[i + 1].x = x + int_from_fixed(curve->apfx[i].x);
                        pts[i + 1].y = y - int_from_fixed(curve->apfx[i].y);
                    }

                    PATH_BezierTo(pPath, pts, curve->cpfx + 1);

                    ExFreePoolWithTag(pts, TAG_PATH);
                    break;
                }

                default:
                    ERR("Unknown curve type %04x\n", curve->wType);
                    goto cleanup;
            }

            curve = (TTPOLYCURVE *)&curve->apfx[curve->cpfx];
        }
        header = (TTPOLYGONHEADER *)((char *)header + header->cb);
    }

    bResult = TRUE;

cleanup:
    IntGdiCloseFigure(pPath);
    return bResult;
}

/**********************************************************************
 *      PATH_ExtTextOut
 */
BOOL
FASTCALL
PATH_ExtTextOut(
    PDC dc,
    INT x,
    INT y,
    UINT flags,
    const RECTL *lprc,
    LPCWSTR str,
    UINT count,
    const INT *dx)
{
    PPATH pPath;
    unsigned int idx, ggo_flags = GGO_NATIVE;
    POINT offset = {0, 0};

    pPath = PATH_LockPath(dc->dclevel.hPath);
    if (!pPath)
    {
        return FALSE;
    }

    if (pPath->state != PATH_Open)
    {
        ERR("PATH_ExtTextOut not open\n");
        return FALSE;
    }

    if (!count) return TRUE;
    if (flags & ETO_GLYPH_INDEX) ggo_flags |= GGO_GLYPH_INDEX;

    for (idx = 0; idx < count; idx++)
    {
        MAT2 identity = { {0, 1}, {0, 0}, {0, 0}, {0, 1} };
        GLYPHMETRICS gm;
        DWORD dwSize;
        void *outline;

        dwSize = ftGdiGetGlyphOutline(dc,
                                      str[idx],
                                      ggo_flags,
                                      &gm,
                                      0,
                                      NULL,
                                      &identity,
                                      TRUE);
        if (dwSize == GDI_ERROR)
        {
           // With default DC font,,, bitmap font?
           // ExtTextOut on a path with bitmap font selected shouldn't fail.
           // This just leads to empty path generated.
           // Ref : test_emf_ExtTextOut_on_path
           continue;
        }

        /* Add outline only if char is printable */
        if (dwSize)
        {
            outline = ExAllocatePoolWithTag(PagedPool, dwSize, TAG_PATH);
            if (!outline)
            {
               PATH_UnlockPath(pPath);
               return FALSE;
            }

            ftGdiGetGlyphOutline(dc,
                                 str[idx],
                                 ggo_flags,
                                 &gm,
                                 dwSize,
                                 outline,
                                 &identity,
                                 TRUE);

            PATH_add_outline(dc, pPath, x + offset.x, y + offset.y, outline, dwSize);

            ExFreePoolWithTag(outline, TAG_PATH);
        }

        if (dx)
        {
            if (flags & ETO_PDY)
            {
                offset.x += dx[idx * 2];
                offset.y += dx[idx * 2 + 1];
            }
            else
                offset.x += dx[idx];
        }
        else
        {
            offset.x += gm.gmCellIncX;
            offset.y += gm.gmCellIncY;
        }
    }
    PATH_UnlockPath(pPath);
    return TRUE;
}


/***********************************************************************
 * Exported functions
 */

BOOL
APIENTRY
NtGdiAbortPath(HDC hDC)
{
    PDC dc = DC_LockDc(hDC);
    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!dc->dclevel.hPath)
    {
       DC_UnlockDc(dc);
       return TRUE;
    }

    if (!PATH_Delete(dc->dclevel.hPath))
    {
       DC_UnlockDc(dc);
       return FALSE;
    }

    dc->dclevel.hPath = 0;
    dc->dclevel.flPath &= ~DCPATH_ACTIVE;

    DC_UnlockDc(dc);
    return TRUE;
}

BOOL
APIENTRY
NtGdiBeginPath(HDC  hDC)
{
    PPATH pPath;
    PDC dc;

    dc = DC_LockDc(hDC);
    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    /* If path is already open, do nothing. Check if not Save DC state */
    if ((dc->dclevel.flPath & DCPATH_ACTIVE) && !(dc->dclevel.flPath & DCPATH_SAVE))
    {
        DC_UnlockDc(dc);
        return TRUE;
    }

    if (dc->dclevel.hPath)
    {
        TRACE("BeginPath 1 0x%p\n", dc->dclevel.hPath);
        if (!(dc->dclevel.flPath & DCPATH_SAVE))
        {
            // Remove previous handle.
            if (!PATH_Delete(dc->dclevel.hPath))
            {
                DC_UnlockDc(dc);
                return FALSE;
            }
        }
        else
        {
            // Clear flags and Handle.
            dc->dclevel.flPath &= ~(DCPATH_SAVE | DCPATH_ACTIVE);
            dc->dclevel.hPath = NULL;
        }
    }
    pPath = PATH_CreatePath(NUM_ENTRIES_INITIAL);
    dc->dclevel.flPath |= DCPATH_ACTIVE; // Set active ASAP!
    dc->dclevel.hPath = pPath->BaseObject.hHmgr;
    IntGetCurrentPositionEx(dc, &pPath->pos);
    IntLPtoDP( dc, &pPath->pos, 1 );
    TRACE("BP : Current pos X %d Y %d\n",pPath->pos.x, pPath->pos.y);
    PATH_UnlockPath(pPath);
    DC_UnlockDc(dc);

    if (!pPath)
    {
        return FALSE;
    }
    return TRUE;
}

BOOL
APIENTRY
NtGdiCloseFigure(HDC hDC)
{
    BOOL Ret = FALSE; // Default to failure
    PDC pDc;
    PPATH pPath;

    TRACE("Enter %s\n", __FUNCTION__);

    pDc = DC_LockDc(hDC);
    if (!pDc)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pPath = PATH_LockPath(pDc->dclevel.hPath);
    if (!pPath)
    {
        DC_UnlockDc(pDc);
        return FALSE;
    }

    if (pPath->state == PATH_Open)
    {
        IntGdiCloseFigure(pPath);
        Ret = TRUE;
    }
    else
    {
        EngSetLastError(ERROR_CAN_NOT_COMPLETE);
    }

    PATH_UnlockPath(pPath);
    DC_UnlockDc(pDc);
    return Ret;
}

BOOL
APIENTRY
NtGdiEndPath(HDC  hDC)
{
    BOOL ret = TRUE;
    PPATH pPath;
    PDC dc;

    dc = DC_LockDc(hDC);
    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    pPath = PATH_LockPath(dc->dclevel.hPath);
    if (!pPath)
    {
        DC_UnlockDc(dc);
        return FALSE;
    }

    /* Check that path is currently being constructed */
    if ((pPath->state != PATH_Open) || !(dc->dclevel.flPath & DCPATH_ACTIVE))
    {
        TRACE("EndPath ERROR! 0x%p\n", dc->dclevel.hPath);
        EngSetLastError(ERROR_CAN_NOT_COMPLETE);
        ret = FALSE;
    }
    /* Set flag to indicate that path is finished */
    else
    {
        TRACE("EndPath 0x%p\n", dc->dclevel.hPath);
        pPath->state = PATH_Closed;
        dc->dclevel.flPath &= ~DCPATH_ACTIVE;
    }

    PATH_UnlockPath(pPath);
    DC_UnlockDc(dc);
    return ret;
}

BOOL
APIENTRY
NtGdiFillPath(HDC  hDC)
{
    BOOL ret = FALSE;
    PPATH pPath, pNewPath;
    PDC_ATTR pdcattr;
    PDC dc;

    dc = DC_LockDc(hDC);
    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pPath = PATH_LockPath(dc->dclevel.hPath);
    if (!pPath)
    {
        DC_UnlockDc(dc);
        return FALSE;
    }

    DC_vPrepareDCsForBlit(dc, NULL, NULL, NULL);

    pdcattr = dc->pdcattr;

    if (pdcattr->ulDirty_ & (DIRTY_LINE | DC_PEN_DIRTY))
        DC_vUpdateLineBrush(dc);

    if (pdcattr->ulDirty_ & (DIRTY_FILL | DC_BRUSH_DIRTY))
        DC_vUpdateFillBrush(dc);

    pNewPath = PATH_FlattenPath(pPath);

    if (pNewPath->state != PATH_Closed)
    {
        EngSetLastError(ERROR_CAN_NOT_COMPLETE);
    }
    else if (pNewPath->numEntriesUsed)
    {
       ret = PATH_FillPath(dc, pNewPath);
    }
    else ret = TRUE;

    PATH_UnlockPath(pNewPath);
    PATH_Delete(pNewPath->BaseObject.hHmgr);

    PATH_UnlockPath(pPath);
    PATH_Delete(pPath->BaseObject.hHmgr);
    dc->dclevel.hPath = 0;
    dc->dclevel.flPath &= ~DCPATH_ACTIVE;

    DC_vFinishBlit(dc, NULL);
    DC_UnlockDc(dc);
    return ret;
}

BOOL
APIENTRY
NtGdiFlattenPath(HDC hDC)
{
    BOOL Ret = FALSE;
    DC *pDc;
    PPATH pPath, pNewPath = NULL;

    TRACE("Enter %s\n", __FUNCTION__);

    pDc = DC_LockDc(hDC);
    if (!pDc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    pPath = PATH_LockPath(pDc->dclevel.hPath);
    if (!pPath)
    {
        EngSetLastError( ERROR_CAN_NOT_COMPLETE );
        DC_UnlockDc(pDc);
        return FALSE;
    }

    if (pPath->state == PATH_Closed)
    {
        pNewPath = PATH_FlattenPath(pPath);
    }

    PATH_UnlockPath(pPath);

    if (pNewPath)
    {
       PATH_Delete(pDc->dclevel.hPath);
       pDc->dclevel.hPath = pNewPath->BaseObject.hHmgr;
       PATH_UnlockPath(pNewPath);
       Ret = TRUE;
    }

    DC_UnlockDc(pDc);
    return Ret;
}

_Success_(return != FALSE)
BOOL
APIENTRY
NtGdiGetMiterLimit(
    _In_ HDC hdc,
    _Out_ PDWORD pdwOut)
{
    DC *pDc;
    BOOL bResult = TRUE;

    if (!(pDc = DC_LockDc(hdc)))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    _SEH2_TRY
    {
        ProbeForWrite(pdwOut, sizeof(DWORD), 1);
        *pdwOut = pDc->dclevel.laPath.eMiterLimit;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        bResult = FALSE;
    }
    _SEH2_END;

    DC_UnlockDc(pDc);
    return bResult;

}

INT
APIENTRY
NtGdiGetPath(
    HDC hDC,
    LPPOINT Points,
    LPBYTE Types,
    INT nSize)
{
    INT ret = -1;
    PPATH pPath;
    DC *dc;

    _SEH2_TRY
    {
        ProbeForWrite(Points, nSize * sizeof(*Points), sizeof(ULONG));
        ProbeForWrite(Types, nSize, 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        _SEH2_YIELD(return -1);
    }
    _SEH2_END

    dc = DC_LockDc(hDC);
    TRACE("NtGdiGetPath start\n");
    if (!dc)
    {
        ERR("Can't lock dc!\n");
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    pPath = PATH_LockPath(dc->dclevel.hPath);
    if (!pPath)
    {
        DC_UnlockDc(dc);
        return -1;
    }

    if (pPath->state != PATH_Closed)
    {
        EngSetLastError(ERROR_CAN_NOT_COMPLETE);
        goto done;
    }

    if (nSize == 0)
    {
        ret = pPath->numEntriesUsed;
    }
    else if (nSize < pPath->numEntriesUsed)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }
    else
    {
        _SEH2_TRY
        {
            memcpy(Points, pPath->pPoints, sizeof(POINT)*pPath->numEntriesUsed);
            memcpy(Types, pPath->pFlags, sizeof(BYTE)*pPath->numEntriesUsed);

            /* Convert the points to logical coordinates */
            if (!GdiPathDPtoLP(dc, Points, pPath->numEntriesUsed))
            {
                EngSetLastError(ERROR_ARITHMETIC_OVERFLOW);
                _SEH2_LEAVE;
            }

            ret = pPath->numEntriesUsed;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastNtError(_SEH2_GetExceptionCode());
        }
        _SEH2_END
    }

done:
    TRACE("NtGdiGetPath exit %d\n",ret);
    PATH_UnlockPath(pPath);
    DC_UnlockDc(dc);
    return ret;
}

HRGN
APIENTRY
NtGdiPathToRegion(HDC  hDC)
{
    PPATH pPath, pNewPath;
    HRGN  hrgnRval = 0;
    int Ret;
    PREGION Rgn;
    DC *pDc;
    PDC_ATTR pdcattr;

    TRACE("Enter %s\n", __FUNCTION__);

    pDc = DC_LockDc(hDC);
    if (!pDc)
    {
        ERR("Failed to lock DC %p\n", hDC);
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    pdcattr = pDc->pdcattr;

    pPath = PATH_LockPath(pDc->dclevel.hPath);
    if (!pPath)
    {
        ERR("Failed to lock DC path %p\n", pDc->dclevel.hPath);
        DC_UnlockDc(pDc);
        return NULL;
    }

    if (pPath->state != PATH_Closed)
    {
        // FIXME: Check that setlasterror is being called correctly
        ERR("Path is not closed!\n");
        EngSetLastError(ERROR_CAN_NOT_COMPLETE);
    }
    else
    {
        /* Create the region and fill it with the path strokes */
        Rgn = REGION_AllocUserRgnWithHandle(1);
        if (!Rgn)
        {
            ERR("Failed to allocate a region\n");
            PATH_UnlockPath(pPath);
            DC_UnlockDc(pDc);
            return NULL;
        }
        hrgnRval = Rgn->BaseObject.hHmgr;

        pNewPath = PATH_FlattenPath(pPath);

        Ret = PATH_PathToRegion(pNewPath, pdcattr->jFillMode, Rgn);

        PATH_UnlockPath(pNewPath);
        PATH_Delete(pNewPath->BaseObject.hHmgr);

        if (!Ret)
        {
            ERR("PATH_PathToRegion failed\n");
            REGION_Delete(Rgn);
            hrgnRval = NULL;
        }
        else
            REGION_UnlockRgn(Rgn);
    }

    PATH_UnlockPath(pPath);
    PATH_Delete(pDc->dclevel.hPath);
    pDc->dclevel.hPath = NULL;
    pDc->dclevel.flPath &= ~DCPATH_ACTIVE;

    DC_UnlockDc(pDc);
    return hrgnRval;
}

BOOL
APIENTRY
NtGdiSetMiterLimit(
    IN HDC hdc,
    IN DWORD dwNew,
    IN OUT OPTIONAL PDWORD pdwOut)
{
    DC *pDc;
    gxf_long worker, worker1;
    BOOL bResult = TRUE;

    if (!(pDc = DC_LockDc(hdc)))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    worker.l  = dwNew;
    worker1.f = pDc->dclevel.laPath.eMiterLimit;
    pDc->dclevel.laPath.eMiterLimit = worker.f;

    if (pdwOut)
    {
        _SEH2_TRY
        {
            ProbeForWrite(pdwOut, sizeof(DWORD), 1);
            *pdwOut = worker1.l;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastNtError(_SEH2_GetExceptionCode());
            bResult = FALSE;
        }
        _SEH2_END;
    }

    DC_UnlockDc(pDc);
    return bResult;
}

BOOL
APIENTRY
NtGdiStrokeAndFillPath(HDC hDC)
{
    DC *pDc;
    PDC_ATTR pdcattr;
    PPATH pPath, pNewPath;
    BOOL bRet = FALSE;

    TRACE("Enter %s\n", __FUNCTION__);

    if (!(pDc = DC_LockDc(hDC)))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    pPath = PATH_LockPath(pDc->dclevel.hPath);
    if (!pPath)
    {
        DC_UnlockDc(pDc);
        return FALSE;
    }

    DC_vPrepareDCsForBlit(pDc, NULL, NULL, NULL);

    pdcattr = pDc->pdcattr;

    if (pdcattr->ulDirty_ & (DIRTY_FILL | DC_BRUSH_DIRTY))
        DC_vUpdateFillBrush(pDc);

    if (pdcattr->ulDirty_ & (DIRTY_LINE | DC_PEN_DIRTY))
        DC_vUpdateLineBrush(pDc);

    pNewPath = PATH_FlattenPath(pPath);

    if (pNewPath->state != PATH_Closed)
    {
        EngSetLastError(ERROR_CAN_NOT_COMPLETE);
    }
    else if (pNewPath->numEntriesUsed)
    {
       bRet = PATH_FillPath(pDc, pNewPath);
       if (bRet) bRet = PATH_StrokePath(pDc, pNewPath);
    }
    else bRet = TRUE;

    PATH_UnlockPath(pNewPath);
    PATH_Delete(pNewPath->BaseObject.hHmgr);

    PATH_UnlockPath(pPath);
    PATH_Delete(pPath->BaseObject.hHmgr);
    pDc->dclevel.hPath = 0;
    pDc->dclevel.flPath &= ~DCPATH_ACTIVE;

    DC_vFinishBlit(pDc, NULL);
    DC_UnlockDc(pDc);
    return bRet;
}

BOOL
APIENTRY
NtGdiStrokePath(HDC hDC)
{
    DC *pDc;
    PDC_ATTR pdcattr;
    PPATH pPath, pNewPath;
    BOOL bRet = FALSE;

    TRACE("Enter %s\n", __FUNCTION__);

    if (!(pDc = DC_LockDc(hDC)))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pPath = PATH_LockPath(pDc->dclevel.hPath);
    if (!pPath)
    {
        DC_UnlockDc(pDc);
        return FALSE;
    }

    DC_vPrepareDCsForBlit(pDc, NULL, NULL, NULL);

    pdcattr = pDc->pdcattr;

    if (pdcattr->ulDirty_ & (DIRTY_LINE | DC_PEN_DIRTY))
        DC_vUpdateLineBrush(pDc);

    pNewPath = PATH_FlattenPath(pPath);

    if (pNewPath->state != PATH_Closed)
    {
        EngSetLastError(ERROR_CAN_NOT_COMPLETE);
    }
    else bRet = PATH_StrokePath(pDc, pNewPath);

    PATH_UnlockPath(pNewPath);
    PATH_Delete(pNewPath->BaseObject.hHmgr);

    DC_vFinishBlit(pDc, NULL);

    PATH_UnlockPath(pPath);
    PATH_Delete(pPath->BaseObject.hHmgr);
    pDc->dclevel.hPath = 0;
    pDc->dclevel.flPath &= ~DCPATH_ACTIVE;

    DC_UnlockDc(pDc);
    return bRet;
}

BOOL
APIENTRY
NtGdiWidenPath(HDC  hDC)
{
    PPATH pPath;
    BOOL Ret = FALSE;
    PDC pdc = DC_LockDc(hDC);
    TRACE("NtGdiWidenPat Enter\n");
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pPath = PATH_WidenPath(pdc);
    if (pPath)
    {
       TRACE("WindenPath New Path\n");
       PATH_Delete(pdc->dclevel.hPath);
       pdc->dclevel.hPath = pPath->BaseObject.hHmgr;
       Ret = TRUE;
    }
    DC_UnlockDc(pdc);
    TRACE("NtGdiWidenPat Ret %d\n",Ret);
    return Ret;
}

/* EOF */
