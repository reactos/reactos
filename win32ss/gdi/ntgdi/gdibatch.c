
#include <win32k.h>

#define NDEBUG
#include <debug.h>

BOOL FASTCALL IntPatBlt( PDC,INT,INT,INT,INT,DWORD,PEBRUSHOBJ);
BOOL APIENTRY IntExtTextOutW(IN PDC,IN INT,IN INT,IN UINT,IN OPTIONAL PRECTL,IN LPCWSTR,IN INT,IN OPTIONAL LPINT,IN DWORD);


//
// Gdi Batch Flush support functions.
//

//
// DoDeviceSync
//
// based on IntEngEnter from eng/engmisc.c
//
VOID
FASTCALL
DoDeviceSync( SURFOBJ *Surface, PRECTL Rect, FLONG fl)
{
  PPDEVOBJ Device = (PDEVOBJ*)Surface->hdev;
// No punting and "Handle to a surface, provided that the surface is device-managed.
// Otherwise, dhsurf is zero".
  if (!(Device->flFlags & PDEV_DRIVER_PUNTED_CALL) && (Surface->dhsurf))
  {
     if (Device->DriverFunctions.SynchronizeSurface)
     {
       Device->DriverFunctions.SynchronizeSurface(Surface, Rect, fl);
     }
     else
     {
       if (Device->DriverFunctions.Synchronize)
       {
         Device->DriverFunctions.Synchronize(Surface->dhpdev, Rect);
       }
     }
  }
}

VOID
FASTCALL
SynchronizeDriver(FLONG Flags)
{
  SURFOBJ *SurfObj;
  //PPDEVOBJ Device;

  if (Flags & GCAPS2_SYNCFLUSH)
      Flags = DSS_FLUSH_EVENT;
  if (Flags & GCAPS2_SYNCTIMER)
      Flags = DSS_TIMER_EVENT;

  //Device = IntEnumHDev();
//  UNIMPLEMENTED;
//ASSERT(FALSE);
  SurfObj = 0;// EngLockSurface( Device->pSurface );
  if(!SurfObj) return;
  DoDeviceSync( SurfObj, NULL, Flags);
  EngUnlockSurface(SurfObj);
  return;
}

//
// Process the batch.
//
ULONG
FASTCALL
GdiFlushUserBatch(PDC dc, PGDIBATCHHDR pHdr)
{
  ULONG Cmd = 0, Size = 0;
  PDC_ATTR pdcattr = NULL;

  if (dc)
  {
     pdcattr = dc->pdcattr;
  }

  _SEH2_TRY
  {
     Cmd = pHdr->Cmd;
     Size = pHdr->Size; // Return the full size of the structure.
  }
  _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
  {
     DPRINT1("WARNING! GdiBatch Fault!\n");
     _SEH2_YIELD(return 0;)
  }
  _SEH2_END;

  switch(Cmd)
  {
     case GdiBCPatBlt:
     {
        PGDIBSPATBLT pgDPB;
        DWORD dwRop, flags;
        HBRUSH hOrgBrush;
        COLORREF crColor, crBkColor, crBrushClr;
        ULONG ulForegroundClr, ulBackgroundClr, ulBrushClr;
        if (!dc) break;
        pgDPB = (PGDIBSPATBLT) pHdr;
        /* Convert the ROP3 to a ROP4 */
        dwRop = pgDPB->dwRop;
        dwRop = MAKEROP4(dwRop & 0xFF0000, dwRop);
        /* Check if the rop uses a source */
        if (WIN32_ROP4_USES_SOURCE(dwRop))
        {
           /* This is not possible */
           break;
        }
        /* Check if the DC has no surface (empty mem or info DC) */
        if (dc->dclevel.pSurface == NULL)
        {
           /* Nothing to do */
           break;
        }
        // Save current attributes and flags
        crColor         = dc->pdcattr->crForegroundClr;
        crBkColor       = dc->pdcattr->ulBackgroundClr;
        crBrushClr      = dc->pdcattr->crBrushClr;
        ulForegroundClr = dc->pdcattr->ulForegroundClr;
        ulBackgroundClr = dc->pdcattr->ulBackgroundClr;
        ulBrushClr      = dc->pdcattr->ulBrushClr;
        hOrgBrush       = dc->pdcattr->hbrush;
        flags = dc->pdcattr->ulDirty_ & (DIRTY_BACKGROUND | DIRTY_TEXT | DIRTY_FILL | DC_BRUSH_DIRTY);
        // Set the attribute snapshot
        dc->pdcattr->hbrush          = pgDPB->hbrush; 
        dc->pdcattr->crForegroundClr = pgDPB->crForegroundClr;
        dc->pdcattr->crBackgroundClr = pgDPB->crBackgroundClr;
        dc->pdcattr->crBrushClr      = pgDPB->crBrushClr;
        dc->pdcattr->ulForegroundClr = pgDPB->ulForegroundClr;
        dc->pdcattr->ulBackgroundClr = pgDPB->ulBackgroundClr;
        dc->pdcattr->ulBrushClr      = pgDPB->ulBrushClr;
        // Process dirty attributes if any.
        if (dc->pdcattr->ulDirty_ & (DIRTY_FILL | DC_BRUSH_DIRTY))
            DC_vUpdateFillBrush(dc);
        if (dc->pdcattr->ulDirty_ & DIRTY_TEXT)
            DC_vUpdateTextBrush(dc);
        if (pdcattr->ulDirty_ & DIRTY_BACKGROUND)
            DC_vUpdateBackgroundBrush(dc);
        /* Call the internal function */
        IntPatBlt(dc, pgDPB->nXLeft, pgDPB->nYLeft, pgDPB->nWidth, pgDPB->nHeight, dwRop, &dc->eboFill);
        // Restore attributes and flags
        dc->pdcattr->hbrush          = hOrgBrush;
        dc->pdcattr->crForegroundClr = crColor;
        dc->pdcattr->crBackgroundClr = crBkColor;
        dc->pdcattr->crBrushClr      = crBrushClr;
        dc->pdcattr->ulForegroundClr = ulForegroundClr;
        dc->pdcattr->ulBackgroundClr = ulBackgroundClr;
        dc->pdcattr->ulBrushClr      = ulBrushClr;
        dc->pdcattr->ulDirty_ |= flags;
        break;
     }

     case GdiBCPolyPatBlt:
     {
        PGDIBSPPATBLT pgDPB;
        EBRUSHOBJ eboFill;
        PBRUSH pbrush;
        PPATRECT pRects;
        INT i;
        DWORD dwRop, flags;
        COLORREF crColor, crBkColor, crBrushClr;
        ULONG ulForegroundClr, ulBackgroundClr, ulBrushClr;
        if (!dc) break;
        pgDPB = (PGDIBSPPATBLT) pHdr;
        /* Convert the ROP3 to a ROP4 */
        dwRop = pgDPB->rop4;
        dwRop = MAKEROP4(dwRop & 0xFF0000, dwRop);
        /* Check if the rop uses a source */
        if (WIN32_ROP4_USES_SOURCE(dwRop))
        {
           /* This is not possible */
           break;
        }
        /* Check if the DC has no surface (empty mem or info DC) */
        if (dc->dclevel.pSurface == NULL)
        {
           /* Nothing to do */
           break;
        }
        // Save current attributes and flags
        crColor         = dc->pdcattr->crForegroundClr;
        crBkColor       = dc->pdcattr->ulBackgroundClr;
        crBrushClr      = dc->pdcattr->crBrushClr;
        ulForegroundClr = dc->pdcattr->ulForegroundClr;
        ulBackgroundClr = dc->pdcattr->ulBackgroundClr;
        ulBrushClr      = dc->pdcattr->ulBrushClr;
        flags = dc->pdcattr->ulDirty_ & (DIRTY_BACKGROUND | DIRTY_TEXT | DIRTY_FILL | DC_BRUSH_DIRTY);
        // Set the attribute snapshot
        dc->pdcattr->crForegroundClr = pgDPB->crForegroundClr;
        dc->pdcattr->crBackgroundClr = pgDPB->crBackgroundClr;
        dc->pdcattr->crBrushClr      = pgDPB->crBrushClr;
        dc->pdcattr->ulForegroundClr = pgDPB->ulForegroundClr;
        dc->pdcattr->ulBackgroundClr = pgDPB->ulBackgroundClr;
        dc->pdcattr->ulBrushClr      = pgDPB->ulBrushClr;
        // Process dirty attributes if any
        if (dc->pdcattr->ulDirty_ & DIRTY_TEXT)
            DC_vUpdateTextBrush(dc);
        if (pdcattr->ulDirty_ & DIRTY_BACKGROUND)
            DC_vUpdateBackgroundBrush(dc);

        DPRINT1("GdiBCPolyPatBlt Testing\n");
        pRects = &pgDPB->pRect[0];

        for (i = 0; i < pgDPB->Count; i++)
        {
            pbrush = BRUSH_ShareLockBrush(pRects->hBrush);

            /* Check if we could lock the brush */
            if (pbrush != NULL)
            {
                /* Initialize a brush object */
                EBRUSHOBJ_vInitFromDC(&eboFill, pbrush, dc);

                IntPatBlt(
                    dc,
                    pRects->r.left,
                    pRects->r.top,
                    pRects->r.right,
                    pRects->r.bottom,
                    dwRop,
                    &eboFill);

                /* Cleanup the brush object and unlock the brush */
                EBRUSHOBJ_vCleanup(&eboFill);
                BRUSH_ShareUnlockBrush(pbrush);
            }
            pRects++;
        }

        // Restore attributes and flags
        dc->pdcattr->crForegroundClr = crColor;
        dc->pdcattr->crBackgroundClr = crBkColor;
        dc->pdcattr->crBrushClr      = crBrushClr;
        dc->pdcattr->ulForegroundClr = ulForegroundClr;
        dc->pdcattr->ulBackgroundClr = ulBackgroundClr;
        dc->pdcattr->ulBrushClr      = ulBrushClr;
        dc->pdcattr->ulDirty_ |= flags;
        break;
     }

     case GdiBCTextOut:
     {
        PGDIBSTEXTOUT pgO;
        COLORREF crColor = -1, crBkColor;
        ULONG ulForegroundClr, ulBackgroundClr;
        DWORD flags = 0, flXform = 0, saveflags, saveflXform = 0;
        FLONG flTextAlign = -1;
        HANDLE hlfntNew;
        PRECTL lprc;
        USHORT jBkMode;
        LONG lBkMode;
        POINTL ptlViewportOrg;
        if (!dc) break;
        pgO = (PGDIBSTEXTOUT) pHdr;

        // Save current attributes, flags and Set the attribute snapshots
        saveflags = dc->pdcattr->ulDirty_ & (DIRTY_BACKGROUND|DIRTY_LINE|DIRTY_TEXT|DIRTY_FILL|DC_BRUSH_DIRTY|DIRTY_CHARSET);

        // In this instance check for differences and set the appropriate dirty flags.
        if ( dc->pdcattr->crForegroundClr != pgO->crForegroundClr)
        {
            crColor = dc->pdcattr->crForegroundClr;
            dc->pdcattr->crForegroundClr = pgO->crForegroundClr;
            ulForegroundClr = dc->pdcattr->ulForegroundClr;
            dc->pdcattr->ulForegroundClr = pgO->ulForegroundClr;
            flags |= (DIRTY_FILL|DIRTY_LINE|DIRTY_TEXT);
        }
        if (dc->pdcattr->crBackgroundClr != pgO->crBackgroundClr)
        {
            crBkColor = dc->pdcattr->ulBackgroundClr;
            dc->pdcattr->crBackgroundClr = pgO->crBackgroundClr;
            ulBackgroundClr = dc->pdcattr->ulBackgroundClr;
            dc->pdcattr->ulBackgroundClr = pgO->ulBackgroundClr;
            flags |= (DIRTY_FILL|DIRTY_LINE|DIRTY_TEXT|DIRTY_BACKGROUND);
        }
        if (dc->pdcattr->flTextAlign != pgO->flTextAlign)
        {
            flTextAlign = dc->pdcattr->flTextAlign;
            dc->pdcattr->flTextAlign = pgO->flTextAlign;
        }
        if (dc->pdcattr->hlfntNew != pgO->hlfntNew)
        {
            hlfntNew = dc->pdcattr->hlfntNew;
            dc->pdcattr->hlfntNew = pgO->hlfntNew;
            dc->pdcattr->ulDirty_ &= ~SLOW_WIDTHS;
            flags |= DIRTY_CHARSET;
        }

        if ( dc->pdcattr->ptlViewportOrg.x != pgO->ptlViewportOrg.x ||
             dc->pdcattr->ptlViewportOrg.y != pgO->ptlViewportOrg.y )
        {
            saveflXform = dc->pdcattr->flXform & (PAGE_XLATE_CHANGED|WORLD_XFORM_CHANGED|DEVICE_TO_WORLD_INVALID);
            ptlViewportOrg = dc->pdcattr->ptlViewportOrg;
            dc->pdcattr->ptlViewportOrg = pgO->ptlViewportOrg;
            flXform = (PAGE_XLATE_CHANGED|WORLD_XFORM_CHANGED|DEVICE_TO_WORLD_INVALID);
        }

        dc->pdcattr->flXform  |= flXform;
        dc->pdcattr->ulDirty_ |= flags;

        jBkMode = dc->pdcattr->jBkMode;
        dc->pdcattr->jBkMode = pgO->lBkMode;
        lBkMode = dc->pdcattr->lBkMode;
        dc->pdcattr->lBkMode = pgO->lBkMode;

        lprc = (pgO->Options & GDIBS_NORECT) ? NULL : &pgO->Rect;
        pgO->Options &= ~GDIBS_NORECT;

        IntExtTextOutW( dc,
                        pgO->x,
                        pgO->y,
                        pgO->Options,
                        lprc,
                        (LPCWSTR)&pgO->String[pgO->Size/sizeof(WCHAR)],
                        pgO->cbCount,
                        pgO->Size ? (LPINT)&pgO->Buffer : NULL,
                        pgO->iCS_CP );

        // Restore attributes and flags
        dc->pdcattr->jBkMode = jBkMode;
        dc->pdcattr->lBkMode = lBkMode;

        if (saveflXform)
        {
            dc->pdcattr->ptlViewportOrg = ptlViewportOrg;
            dc->pdcattr->flXform |= saveflXform|flXform;
        }

        if (flags & DIRTY_TEXT && crColor != -1)
        {
            dc->pdcattr->crForegroundClr = crColor;
            dc->pdcattr->ulForegroundClr = ulForegroundClr;
        }
        if (flags & DIRTY_BACKGROUND)
        {
            dc->pdcattr->crBackgroundClr = crBkColor;
            dc->pdcattr->ulBackgroundClr = ulBackgroundClr;
        }
        if (flTextAlign != -1)
        {
            dc->pdcattr->flTextAlign = flTextAlign;
        }

        if (flags & DIRTY_CHARSET)
        {
           dc->pdcattr->hlfntNew = hlfntNew;
           dc->pdcattr->ulDirty_ &= ~SLOW_WIDTHS;
        }
        dc->pdcattr->ulDirty_ |= saveflags | flags;
        dc->pdcattr->flXform  |= saveflXform | flXform;
        break;
     }

     case GdiBCExtTextOut:
     {
        PGDIBSEXTTEXTOUT pgO;
        COLORREF crBkColor;
        ULONG ulBackgroundClr;
        POINTL ptlViewportOrg;
        DWORD flags = 0, flXform = 0, saveflags, saveflXform = 0;
        if (!dc) break;
        pgO = (PGDIBSEXTTEXTOUT) pHdr;

        saveflags = dc->pdcattr->ulDirty_ & (DIRTY_BACKGROUND|DIRTY_TEXT|DIRTY_FILL|DC_BRUSH_DIRTY|DIRTY_CHARSET);

        if (dc->pdcattr->crBackgroundClr != pgO->ulBackgroundClr)
        {
            crBkColor = dc->pdcattr->crBackgroundClr;
            ulBackgroundClr = dc->pdcattr->ulBackgroundClr;
            dc->pdcattr->crBackgroundClr = pgO->ulBackgroundClr;
            dc->pdcattr->ulBackgroundClr = pgO->ulBackgroundClr;
            flags |= (DIRTY_BACKGROUND|DIRTY_LINE|DIRTY_FILL);
        }

        if ( dc->pdcattr->ptlViewportOrg.x != pgO->ptlViewportOrg.x ||
             dc->pdcattr->ptlViewportOrg.y != pgO->ptlViewportOrg.y )
        {
            saveflXform = dc->pdcattr->flXform & (PAGE_XLATE_CHANGED|WORLD_XFORM_CHANGED|DEVICE_TO_WORLD_INVALID);
            ptlViewportOrg = dc->pdcattr->ptlViewportOrg;
            dc->pdcattr->ptlViewportOrg = pgO->ptlViewportOrg;
            flXform = (PAGE_XLATE_CHANGED|WORLD_XFORM_CHANGED|DEVICE_TO_WORLD_INVALID);
        }

        dc->pdcattr->flXform  |= flXform;
        dc->pdcattr->ulDirty_ |= flags;

        IntExtTextOutW( dc,
                        0,
                        0,
                        pgO->Options,
                       &pgO->Rect,
                        NULL,
                        pgO->Count,
                        NULL,
                        0 );

        if (saveflXform)
        {
            dc->pdcattr->ptlViewportOrg = ptlViewportOrg;
            dc->pdcattr->flXform |= saveflXform|flXform;
        }

        if (flags & DIRTY_BACKGROUND)
        {
            dc->pdcattr->crBackgroundClr = crBkColor;
            dc->pdcattr->ulBackgroundClr = ulBackgroundClr;
        }
        dc->pdcattr->ulDirty_ |= saveflags | flags;
        dc->pdcattr->flXform  |= saveflXform | flXform;
        break;
     }

     case GdiBCSetBrushOrg:
     {
        PGDIBSSETBRHORG pgSBO;
        if (!dc) break;
        pgSBO = (PGDIBSSETBRHORG) pHdr;
        pdcattr->ptlBrushOrigin = pgSBO->ptlBrushOrigin;
        DC_vSetBrushOrigin(dc, pgSBO->ptlBrushOrigin.x, pgSBO->ptlBrushOrigin.y);
        break;
     }

     case GdiBCExtSelClipRgn:
     {
        PGDIBSEXTSELCLPRGN pgO;
        if (!dc) break;
        pgO = (PGDIBSEXTSELCLPRGN) pHdr;
        IntGdiExtSelectClipRect( dc, &pgO->rcl, pgO->fnMode);
        break;
     }

     case GdiBCSelObj:
     {
        PGDIBSOBJECT pgO;

        if (!dc) break;
        pgO = (PGDIBSOBJECT) pHdr;

        DC_hSelectFont(dc, (HFONT)pgO->hgdiobj);
        break;
     }

     case GdiBCDelRgn:
        DPRINT("Delete Region Object!\n");
        /* Fall through */
     case GdiBCDelObj:
     {
        PGDIBSOBJECT pgO = (PGDIBSOBJECT) pHdr;
        GreDeleteObject( pgO->hgdiobj );
        break;
     }

     default:
        break;
  }

  return Size;
}

/*
 * NtGdiFlush
 *
 * Flushes the calling thread's current batch.
 */
__kernel_entry
NTSTATUS
APIENTRY
NtGdiFlush(
    VOID)
{
    SynchronizeDriver(GCAPS2_SYNCFLUSH);
    return STATUS_SUCCESS;
}

/*
 * NtGdiFlushUserBatch
 *
 * Callback for thread batch flush routine.
 *
 * Think small & fast!
 */
NTSTATUS
APIENTRY
NtGdiFlushUserBatch(VOID)
{
  PTEB pTeb = NtCurrentTeb();
  ULONG GdiBatchCount = pTeb->GdiBatchCount;

  if( (GdiBatchCount > 0) && (GdiBatchCount <= (GDIBATCHBUFSIZE/4)))
  {
    HDC hDC = (HDC) pTeb->GdiTebBatch.HDC;

    /*  If hDC is zero and the buffer fills up with delete objects we need
        to run anyway.
     */
    if (hDC || GdiBatchCount)
    {
      PCHAR pHdr = (PCHAR)&pTeb->GdiTebBatch.Buffer[0];
      PDC pDC = NULL;

      if (GDI_HANDLE_GET_TYPE(hDC) == GDILoObjType_LO_DC_TYPE && GreIsHandleValid(hDC))
      {
          pDC = DC_LockDc(hDC);
      }

       // No need to init anything, just go!
       for (; GdiBatchCount > 0; GdiBatchCount--)
       {
           ULONG Size;
           // Process Gdi Batch!
           Size = GdiFlushUserBatch(pDC, (PGDIBATCHHDR) pHdr);
           if (!Size) break;
           pHdr += Size;
       }

       if (pDC)
       {
           DC_UnlockDc(pDC);
       }

       // Exit and clear out for the next round.
       pTeb->GdiTebBatch.Offset = 0;
       pTeb->GdiBatchCount = 0;
       pTeb->GdiTebBatch.HDC = 0;
    }
  }

  // FIXME: On Windows XP the function returns &pTeb->RealClientId, maybe VOID?
  return STATUS_SUCCESS;
}
