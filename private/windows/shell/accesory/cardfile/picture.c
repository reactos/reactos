#include "precomp.h"

/************************************************************************/
/*                                                                      */
/*  Windows Cardfile - Written by Mark Cliggett                         */
/*  (c) Copyright Microsoft Corp. 1985, 1994 - All Rights Reserved      */
/*                                                                      */
/************************************************************************/

#ifndef WIN32
#define RINT int
#else
#define RINT short
typedef struct _rrect
{
   RINT left, top, right, bottom ;
} RRECT ;
#endif
/* This file contains all the picture operations.  It virtualizes the
 * interface so we deal with pictures instead of old Bitmaps and objects.
 *
 * If the picture is a Linked or Embedded object, the OLE client DLL
 * will be called.  Otherwise, we will process the data as we did for
 * Windows 3.0.
 *
 */


BOOL         fCreateFromFile;
BYTE         *vpfbmp;
DWORD        vcbLeftToRead;
DWORD        vcLeft;
FAKEBITMAP   vfbmp;
HCURSOR      hcurOle = NULL;
HWND         hwndError;
LPCARDSTREAM lpStream = NULL;
LPOLECLIENT  lpclient = NULL;
LPOLEOBJECT  lpObjectUndo = NULL;
OBJECTTYPE   otObjectUndo = 0 ;
WORD         cWait = 0;
TCHAR        szClip[] = TEXT("Clipboard");


/* szObjectName if szObjFormat with expanded INT number */
#ifndef OLE_20

CHAR        szUndo[] = "Undo";
CHAR        szObjectName[OBJNAMEMAX + 10];
CHAR        szObjFormat[OBJNAMEMAX];

#else

TCHAR       szUndo[] = TEXT("Undo");
TCHAR       szObjectName[OBJNAMEMAX + 10];
TCHAR       szObjFormat[OBJNAMEMAX];

#endif

int          iPos ;

NOEXPORT void  NEAR SetFile(HANDLE fh);
DWORD (pascal far *pfOldRead)  (LPOLESTREAM, LPBYTE, DWORD);
DWORD (pascal far *pfNewRead)  (LPOLESTREAM, LPBYTE, DWORD);

void PicDelete(PCARD pCard)
{
    if (pCard->lpObject) {
#if !defined(WIN32)
        if (HIWORD(pCard->lpObject)) {
#else
        if (GetObjectType(pCard->lpObject) != OBJ_BITMAP) {
#endif
            WaitForObject(pCard->lpObject);
            /* OleRelease() because the object is still
             * part of the Card File.
             */
            switch (OleError(OleRelease(pCard->lpObject)))
              {
                case FOLEERROR_NOTGIVEN:
                    ErrorMessage(E_FAILED_TO_DELETE_OBJECT);
                    break;
#if 0
                case FOLEERROR_GIVEN:
                    break;
#endif
                case FOLEERROR_OK:
                    InvalidateRect(hEditWnd, NULL, TRUE);
                    WaitForObject(pCard->lpObject);
                    break;
              }
        } else
#if !defined(WIN32)
            DeleteObject((HBITMAP)LOWORD(pCard->lpObject));
#else
            DeleteObject((HBITMAP)(pCard->lpObject));
#endif
    }
    pCard->lpObject = NULL;

}

/* SIDE EFFECT:  Advances the file pointer past the object */
BOOL PicRead(PCARD pCard, HANDLE fh, BOOL fOld)
{
    RINT bmSize;
    RINT cxBitmap;
    RINT cyBitmap;
    RINT cEighths;
    OLESTATUS olestat;
    LPOLEOBJECT lpObject = NULL;
    DWORD idObject;

    MyByteReadFile(fh, &bmSize, sizeof(RINT));

    /* Does the card contain a picture? */
    if (!bmSize) {        /* No... */
        pCard->lpObject = NULL;
        return TRUE;        /* Succeeded in reading 0 size object */
    }

    if (fOLE)         /* Reading a new file... */
    {
        unsigned long fhLoc;

        SetFile(fh);

        if (fOld)         /* Set up to read from old format card */
        {
            lpStream->lpstbl->Get = pfOldRead;
            vcbLeftToRead = ((DWORD)bmSize);
        }
        else
            MyByteReadFile(fh, &idObject, sizeof(DWORD));

    /* Save current position of file pointer */
        if ((fhLoc = MyFileSeek(fh, 0L, 1)) == -1)
            return FALSE;

        /* Synchronously:  Load the new object */
#ifndef OLE_20
        wsprintfA (szObjectName, szObjFormat, idObject + 1);
#else
        wsprintfW (szObjectName, szObjFormat, idObject + 1);
#endif
        olestat = OleLoadFromStream ((LPOLESTREAM)lpStream, szPStdFile,
                                     lpclient, lhcdoc, szObjectName, &lpObject);

        if (olestat == OLE_WAIT_FOR_RELEASE)
        {
            cOleWait++;
            WaitForObject (lpObject);
            olestat = oleloadstat;
        }

        if (OleError(olestat))
        {
            /* Reset file pointer, and try again */
            vcbLeftToRead = ((DWORD)bmSize);
            SetFile(fh);
            if (MyFileSeek(fh, fhLoc, 0) == -1)
                return FALSE;

            /* Synchronously:  Read it with the "Static" protocol */
            olestat = OleLoadFromStream ((LPOLESTREAM)lpStream, szPStatic,
                                         lpclient, lhcdoc, szObjectName, &lpObject);

            if (olestat == OLE_WAIT_FOR_RELEASE)
            {
                cOleWait++;
                WaitForObject (lpObject);
                olestat = oleloadstat;
            }

            if (OleError(olestat))
            {
                lpStream->lpstbl->Get = pfNewRead;          /* Restore */
                return FALSE;
            }
        }
        pCard->lpObject = lpObject;

        if (fOld)
        {
            /* Side effect:  pCard->rcObject has already been filled in */
            pCard->otObject = STATIC;
            MyFileSeek (fh, vcbLeftToRead, 1);    /* For Video 7 bug */
            lpStream->lpstbl->Get = pfNewRead;    /* Restore so new is fastest */
            pCard->idObject = idObjectMax++;
        }
        else
        {
            RINT cx;
            RINT cy;

            /* Read the character size when saving, and scale to the
             * current character size.
             */
            MyByteReadFile (fh, &cx, sizeof(RINT));
            MyByteReadFile (fh, &cy, sizeof(RINT));
#ifndef WIN32
            MyByteReadFile (fh, &(pCard->rcObject), sizeof(RECT));
#else
            {
               RRECT rrec;

               MyByteReadFile(fh, &(rrec), 8) ;
               SetRect (&(pCard->rcObject), (LONG)rrec.left, (LONG)rrec.top,
                        (LONG)rrec.right, (LONG)rrec.bottom);
            }
#endif

            /* Character width differs, scale in the x direction */
            if (cx != (RINT)CharFixWidth)
            {
                pCard->rcObject.left  = Scale(pCard->rcObject.left, CharFixWidth, cx);
                pCard->rcObject.right = Scale(pCard->rcObject.right, CharFixWidth, cx);
            }

            /* Character height differs, scale in the y direction */
            if (cy != (RINT)CharFixHeight)
            {
                pCard->rcObject.top    = Scale(pCard->rcObject.top, CharFixHeight, cy);
                pCard->rcObject.bottom = Scale(pCard->rcObject.bottom, CharFixHeight, cy);
            }

            /* Retrieve the object type */
            MyByteReadFile (fh, &(pCard->otObject), sizeof(OBJECTTYPE));
            pCard->idObject = idObject;
        }
    }
    else
    {
        HANDLE hBits;
        LPBYTE lpBits;

        if (!fOld)         /* No OLE, can't read new objects! */
        {
            pCard->lpObject = NULL;
            ErrorMessage(E_NEW_FILE_NOT_READABLE);
            return FALSE;
        }

        /* Read in phony monochrome BITMAP header */
        MyByteReadFile(fh, &cxBitmap, sizeof(RINT));
        MyByteReadFile(fh, &cyBitmap, sizeof(RINT));

        MyByteReadFile(fh, &cEighths, sizeof(RINT));
        pCard->rcObject.left = (cEighths * CharFixWidth) / 8;
        pCard->rcObject.right = pCard->rcObject.left + cxBitmap - 1;

        MyByteReadFile(fh, &cEighths, sizeof(RINT));
        pCard->rcObject.top = (cEighths * CharFixHeight) / 8;
        pCard->rcObject.bottom = pCard->rcObject.top + cyBitmap - 1;
        pCard->idObject = idObjectMax++;

        /* Read in the BITMAP bits */
        if (hBits = GlobalAlloc(GHND, (WORD)bmSize))
        {
            if (lpBits = (LPBYTE)GlobalLock(hBits))
            {
                MyByteReadFile(fh, lpBits, bmSize);
                /* Make the selector zero */
                pCard->lpObject = (LPOLEOBJECT) MAKELONG(
                CreateBitmap(cxBitmap, cyBitmap, 1, 1, lpBits), 0);
                GlobalUnlock(hBits);
            }
            GlobalFree(hBits);
        }
        else              /* Skip past the object */
            MyFileSeek(fh, (unsigned long)bmSize, 1);
    }
    return TRUE;
}

/* If fForceOld is specified, or the OLE library isn't loaded,
 * PicWrite() will write new format files.
 */

/* SIDE EFFECT:  moves pointer to end of written object */
BOOL PicWrite(PCARD pCard, HANDLE fh, BOOL fForceOld) {
    HANDLE  hBitmap = NULL;
    RINT    bmSize;
    RINT    bmWidth;
    RINT    bmHeight;
    RINT    cEighths;
    HANDLE  hBits = NULL;
    LPBYTE  lpBits = NULL;

    if (fOLE)
    {
        if (fForceOld)
        {
            /* Convert picture to monochrome bitmap (0 colors) */
            hBitmap = MakeObjectCopy(pCard, (HDC)NULL);
            goto OldWrite;
        }
        else
        {
        /* Write a BOOL so that cardfiles can be compared */
            bmSize = (RINT)!!pCard->lpObject;

            if (!MyByteWriteFile(fh, &bmSize, sizeof(RINT)))
                goto Disk_Full;

            if (bmSize)
            {
                SetFile(fh);
                if (!MyByteWriteFile(fh, &(pCard->idObject), sizeof(DWORD)))
                    goto Disk_Full;

                if (OLE_OK != OleSaveToStream(pCard->lpObject, (LPOLESTREAM)lpStream))
                    goto Disk_Full;

                /* Write current character size, rectangle, and object type.
                 */
#ifndef WIN32
                if (!MyByteWriteFile(fh, &CharFixWidth, sizeof(RINT))
                 || !MyByteWriteFile(fh, &CharFixHeight, sizeof(RINT))
                 || !MyByteWriteFile(fh, &(pCard->rcObject), sizeof(RECT))
                 || !MyByteWriteFile(fh, &(pCard->otObject), sizeof(OBJECTTYPE)))
                {
                    goto Disk_Full;
                }
#else
                if (!MyByteWriteFile(fh, &CharFixWidth, sizeof(RINT))
                    || !MyByteWriteFile(fh, &CharFixHeight, sizeof(RINT)))
                     goto Disk_Full;

                {
                   RRECT rrec;

                   rrec.left   = (short)(pCard->rcObject).left ;
                   rrec.right  = (short)(pCard->rcObject).right ;
                   rrec.top    = (short)(pCard->rcObject).top ;
                   rrec.bottom = (short)(pCard->rcObject).bottom ;

                   if(!MyByteWriteFile(fh, &(rrec), 8))
                      goto Disk_Full ;

                   if(!MyByteWriteFile(fh, &(pCard->otObject), sizeof(OBJECTTYPE)))
                      goto Disk_Full;
                }
#endif
            }
        }
    }
    else          /* Can only write the old format (sigh) */
    {
OldWrite:
        if (!hBitmap)
#if !defined(WIN32)
            hBitmap = (HBITMAP)LOWORD(pCard->lpObject);
#else
            hBitmap = (HBITMAP)(pCard->lpObject);
#endif

        if (hBitmap)
        {
            /* Calculate the BITMAP dimensions */
            bmWidth     = (RINT)((pCard->rcObject.right - pCard->rcObject.left) + 1);
            bmHeight    = (RINT)((pCard->rcObject.bottom - pCard->rcObject.top) + 1);
            bmSize      = (RINT)((((bmWidth + 0x000f) >> 4) << 1) * bmHeight);

            if ((hBits = GlobalAlloc(GHND, (WORD)bmSize))
                && (lpBits = (LPBYTE)GlobalLock(hBits)))
            {
                /* Write out the size, width, height */
                if (!MyByteWriteFile(fh, &bmSize, sizeof(RINT)) ||
                    !MyByteWriteFile(fh, &bmWidth, sizeof(RINT)) ||
                    !MyByteWriteFile(fh, &bmHeight, sizeof(RINT)))
                    goto Disk_Full;

                /* Write out the x and y positions */
                cEighths = (RINT)((pCard->rcObject.left * 8) / CharFixWidth);
                if (!MyByteWriteFile(fh, &cEighths, sizeof(RINT)))
                    goto Disk_Full;

                cEighths = (RINT)((pCard->rcObject.top * 8) / CharFixHeight);
                if (!MyByteWriteFile(fh, &cEighths, sizeof(RINT)))
                    goto Disk_Full;

                /* Write out the actual BITMAP bits */
                GetBitmapBits(hBitmap, (unsigned long)bmSize, lpBits);
                if (!MyByteWriteFile(fh, lpBits, bmSize))
                    goto Disk_Full;
                GlobalUnlock(hBits);
                GlobalFree(hBits);
            }
            else
            {
Disk_Full:
                if (lpBits)
                    GlobalUnlock(hBits);

                if (hBits)
                    GlobalFree(hBits);
                return FALSE;
            }
        }
        else
        {
            RINT zero = 0;

            if (!MyByteWriteFile(fh, &zero, sizeof(RINT)))
                goto Disk_Full;
        }
    }
    return TRUE;
}

BOOL PicDraw(PCARD pCard, HDC hDC, BOOL fAtOrigin) {
    BOOL   bSuccess;
    HANDLE hOldObject;
    HDC    hMemoryDC;

    /* If we have an object, call OleDraw() */
    if (GetObjectType(pCard->lpObject) != OBJ_BITMAP) {
        HRGN    hrgn;
        RECT    rc, rcClip;
        POINT   pt;
        INT     iPrevStretchMode;

        iPrevStretchMode= SetStretchBltMode( hDC, HALFTONE );
        SetBrushOrgEx( hDC, 0,0, &pt );

        rc = pCard->rcObject;

        if (fAtOrigin)        /* Move rect to (0,0) */
            OffsetRect(&rc, -rc.left, -rc.top);

        /* Force OleDraw to draw within the clipping region */
        if (hrgn = CreateRectRgnIndirect(&rc))
            SelectObject(hDC, hrgn);

/* Bug 11290: Don't draw outside the edit window's client area
 *  11 January 1992         Clark R. Cyr
 */
        GetClientRect (hEditWnd, &rcClip);
        IntersectClipRect (hDC, rcClip.left, rcClip.top,
                           rcClip.right, rcClip.bottom);

        /* Draw the object */
        bSuccess = (OLE_OK == OleDraw(pCard->lpObject, hDC, (LPRECT)&rc, NULL, NULL));

        /* If we had a clipping region, delete it */
        if (hrgn)
            DeleteObject(hrgn);

        SetStretchBltMode( hDC, iPrevStretchMode );
        SetBrushOrgEx( hDC, pt.x, pt.y, NULL );
    }
    else         /* If we have a BITMAP, BitBlt it into the DC */
    {
        if (pCard->lpObject && (hMemoryDC = CreateCompatibleDC(hDC)))
        {
            RINT cxBitmap, cyBitmap;

            cxBitmap = (RINT)(pCard->rcObject.right - pCard->rcObject.left) + 1,
            cyBitmap = (RINT)(pCard->rcObject.bottom - pCard->rcObject.top) + 1,

            hOldObject = SelectObject(hMemoryDC, (HBITMAP)(pCard->lpObject));
            BitBlt(hDC, pCard->rcObject.left, pCard->rcObject.top,
                   cxBitmap, cyBitmap, hMemoryDC, 0, 0, SRCAND);
            SelectObject(hMemoryDC, hOldObject);
            DeleteDC(hMemoryDC);
            bSuccess = TRUE;
        }
    }
    return bSuccess;
}

void PicCutCopy(PCARD pCard, BOOL fCut)
{
    /* If no object or we can't open the clipboard, fail */
    if (!pCard->lpObject || !OpenClipboard(hIndexWnd))
        return;

    EmptyClipboard();

    Hourglass(TRUE);
    if (GetObjectType(pCard->lpObject) != OBJ_BITMAP) {
          if (OLE_OK != OleCopyToClipboard(pCard->lpObject))
            IndexOkError(EINSMEMORY);
    }
    else if (pCard->lpObject)        /* Old BITMAP */
    {
        BITMAP bm;
        HBITMAP hBitmap;

        GetObject((HBITMAP)(pCard->lpObject), sizeof(BITMAP), (LPVOID)&bm);
        if (!(hBitmap = MakeBitmapCopy((HBITMAP)(pCard->lpObject), &bm, (HDC)NULL )))
            IndexOkError(EINSMEMORY);
        else
            SetClipboardData(CF_BITMAP, hBitmap);
    }
    CloseClipboard();

    if (fCut && pCard->lpObject) {         /* Delete the object */
        /* If Undo object exists, delete it */
        DeleteUndoObject();

        /* Instead of deleting then cloning, just save the object */
        lpObjectUndo = pCard->lpObject;
        otObjectUndo = pCard->otObject;
        OleRename(lpObjectUndo, szUndo);
        pCard->lpObject = NULL;

        InvalidateRect(hEditWnd, (LPRECT)&(pCard->rcObject), TRUE);
        CurCardHead.flags |= FDIRTY;
    }
    Hourglass(FALSE);
}

/*
 * Paste/PasteLink an Object
 *
 * pCard  - Card where the object should be pasted,
 * fPaste - specifies Paste/PasteLink
 * ClipFormat - Should be NULL for Edit.Paste, Edit.PasteLink
 *            - will contain a specific Clipbrd format when user chooses
 *              Paste/PasteLink in Edit.PasteSpecial dlg.
 * Parameters for various calls:
 *
 * On Edit.Paste - PicPaste(pCard, TRUE, 0);
 * On Edit.PasteLink - PicPaste(pCard, FALSE, 0);
 * On Paste/PasteLink from PasteSpecial Dlg,
 *     if (class name chosen from the Objects list)
 *         Similar to Edit.Paste/PasteLink.
 *     else if (specific format chosen)
 *     {
 *         PicPaste(pCard, TRUE, Format);
 *         (Paste a static object in given format)
 *         OR
 *         PasteLink(pCard, FALSE, Format);
 */
void PicPaste(
    PCARD pCard,
    BOOL fPaste,    /* Paste/PasteLink */
    WORD ClipFormat) /* For Paste from PasteSpecial Dlg, may specify a format */
{
    LONG objType;
    LPOLEOBJECT lpObject;
    OBJECTTYPE otObject;
    BOOL fError;

    if (!OpenClipboard(hIndexWnd))
        return;                /* Couldn't open the clipboard */

    Hourglass(TRUE);
    if (fOLE)
    {
        /* Don't replace the current object unless we're successful */
#ifndef OLE_20
        wsprintfA (szObjectName, szObjFormat, idObjectMax + 1);
#else
        wsprintfW (szObjectName, szObjFormat, idObjectMax + 1);
#endif
        if (fPaste)
        {
            if (ClipFormat) /* Paste a specific format (from PasteSpecial Dlg) */
            {
                fError = OleError(OleCreateFromClip(szPStatic, lpclient, lhcdoc, szObjectName,
                                                    &lpObject, olerender_format, ClipFormat));
            }
            else        /* Paste from Edit.Paste */
            {
                /* Try StdFileEditing protocol */
                fError = OleError(OleCreateFromClip(szPStdFile, lpclient, lhcdoc, szObjectName,
                                                    &lpObject, olerender_draw, 0));
                /* If unsuccessful, try Static protocol */
                if (fError)
                    fError = OleError(OleCreateFromClip(szPStatic, lpclient, lhcdoc, szObjectName,
                                                        &lpObject, olerender_draw, 0));
            }
        } else
        {
            /* create a link, in response to Edit.PasteLink or
             * PasteLink from PasteSpecial dlg
             */
            if (ClipFormat) /* PasteLink a specific format (from PasteSpecial Dlg) */
            {
                fError = OleError(OleCreateLinkFromClip(szPStdFile, lpclient, lhcdoc, szObjectName,
                                                        &lpObject,  olerender_format, ClipFormat));
            }
            else    /* PasteLink an object */
            {
                fError = OleError(OleCreateLinkFromClip(szPStdFile, lpclient, lhcdoc, szObjectName,
                                                        &lpObject,  olerender_draw, 0));
            }
        }
        if (fError)
        {
            lpObject = NULL;
            if (fError == FOLEERROR_NOTGIVEN)
                ErrorMessage(E_GET_FROM_CLIPBOARD_FAILED);
        } else
        {
            /* Figure out what kind of object we have, OleCreateFromLink
             * can create an embedded object */
            OleQueryType(lpObject, &objType);
            switch (objType)
            {
                case OT_EMBEDDED:   otObject = EMBEDDED;    break;
                case OT_LINK:       otObject = LINK;        break;
                default:            otObject = STATIC;      break;
            }
        }
        if (lpObject)
        {
            if (pCard->lpObject)
                PicDelete(pCard);
            pCard->lpObject = lpObject;
            pCard->otObject = otObject;
            pCard->idObject = idObjectMax++;
            DoSetHostNames(CurCard.lpObject, CurCard.otObject);
            InvalidateRect(hEditWnd, NULL, TRUE);
            SetRect(&(pCard->rcObject), 0, 0, 0, 0);
            CurCardHead.flags |= FDIRTY;
        }
    }
    else
    {            /* Create an old object the hard way */
        HBITMAP hBitmap;
        BITMAP bm;

        if (hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP))
        {
            GetObject(hBitmap, sizeof(BITMAP), &bm);
            if (!(hBitmap = MakeBitmapCopy(hBitmap, &bm, (HDC) NULL )))
                IndexOkError(EINSMEMORY);
            else
            {
                if (pCard->lpObject)
                    PicDelete(pCard);

                /* Make the selector zero */
                pCard->lpObject = (LPOLEOBJECT)MAKELONG(hBitmap, 0);
                SetRect(&(pCard->rcObject), 0, 0, bm.bmWidth-1, bm.bmHeight-1);
                InvalidateRect(hEditWnd, (LPRECT)&(pCard->rcObject), TRUE);
                CurCardHead.flags |= FDIRTY;
            }
        }
    }
    CloseClipboard();
    Hourglass(FALSE);
}

/* MakeObjectCopy
 *
 * pCard - card to get object from
 * hDestDC - if present, the format to get, otherwise monochrome
 *
 */

HBITMAP MakeObjectCopy(PCARD pCard, HDC  hDestDC )
{
    HBITMAP hBitmap = NULL;
    HBRUSH  hBrush = NULL;
    HDC hDCDest = NULL;
    HDC hDC;
    int cxBitmap;
    int cyBitmap;
    BOOL fError = TRUE;
    HANDLE hObject;
    BITMAP bm;
    SIZE sz ;

    /* LOWORD(lpObject) is NULL only if no bitmap in 3.0 file */
    if (pCard->lpObject == NULL)
        return NULL;

    /* First, try to load a normal BITMAP */
    if (GetObjectType(pCard->lpObject) != OBJ_BITMAP)
    {
        if (OLE_OK != OleGetData(pCard->lpObject, CF_BITMAP, &hBitmap))
            hBitmap = NULL;
    }
    else
    {
        hBitmap = (HBITMAP)(pCard->lpObject);
    }

    /* got a bitmap, either this is a bitmap in OLE object OR
       it is a bitmap in 3.0 card file.
       return a copy of the bitmap */
    if (hBitmap)
    {
        GetObject(hBitmap, sizeof(BITMAP), &bm);
        return MakeBitmapCopy(hBitmap, &bm, hDestDC );
    }

    /* Ole Object is not a bitmap. May be a metafile or s'thing else.
       Draw it into a mono DC and return a handle to this bitmap */
    Hourglass(TRUE);

    /* If we don't succeed, draw the picture into a monochrome DC */
    if( hDestDC == NULL )
    {
        hDC = GetDC(hIndexWnd);
        hDCDest = CreateCompatibleDC(hDC);
        ReleaseDC(hIndexWnd, hDC);
    }
    else
    {
        hDCDest= CreateCompatibleDC(hDestDC);
    }
    if (!hDCDest)
        goto MakeObjectCopyEnd;

    /* Create a new monochrome BITMAP */
    cxBitmap = (pCard->rcObject.right - pCard->rcObject.left);
    cyBitmap = (pCard->rcObject.bottom - pCard->rcObject.top);
    if( hDestDC == NULL )
    {
        hBitmap= CreateBitmap( cxBitmap, cyBitmap, 1, 1, NULL );
    }
    else
    {
        hBitmap= CreateCompatibleBitmap( hDestDC,cxBitmap, cyBitmap );
    }

    if( !hBitmap )
    {
        goto MakeObjectCopyEnd;
    }

    SetWindowOrgEx(hDCDest, 0, 0, (LPPOINT)NULL);
    SetWindowExtEx(hDCDest, cxBitmap, cyBitmap, &sz);

    /* Draw into the DC */
    if (hObject = SelectObject(hDCDest, hBitmap))
    {
        /* Start by clearing the DC (white's enough 'cause it's monochrome) */
        if (hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW)))
        {
            RECT rc;

            rc = pCard->rcObject;
            OffsetRect(&rc, -rc.left, -rc.top);
            FillRect(hDCDest, &rc, hBrush);
            DeleteObject(hBrush);
        }

        /* Draw the picture */
        fError = !PicDraw(pCard, hDCDest, TRUE);
        SelectObject(hDCDest, hObject);                 /* Needed? */
    }

MakeObjectCopyEnd:
    if (fError)
    {
        ErrorMessage(E_BITMAP_COPY_FAILED);
        if (hBitmap)
        {
            DeleteObject(hBitmap);
            hBitmap = NULL;
        }
    }
    if (hDCDest)
        DeleteDC(hDCDest);

    Hourglass(FALSE);
    return hBitmap;
}

NOEXPORT void NEAR SetFile (HANDLE fh)
{
    lpStream->fh = fh;
    vfbmp.bm.bmType = -1;
}

/******************* OLE STREAM Get, Put, ... routines **********************/

 /*
  * This Read is used to make Old Card bitmaps read like the BITMAP
  * object in OLECLI.DLL.  We must return:
  *
  * <type signature>     0x0700
  * <type string>        "BITMAP" (NULL terminated)
  * BITMAP structure
  * <BITMAP bits>
  *
  * So, I fill in "FAKEBITMAP" and read out of it until I'm through;
  * then I just read the bitmap bits directly from the file.
  */

DWORD ReadOldStream(LPCARDSTREAM lpStream, LPBYTE lpbit, DWORD cb)
{
    int      xTmp, yTmp;
    DWORD    cRead = 0L;
    short    Temp ;

    if (vfbmp.bm.bmType == -1)
    {
        iPos = 0;
        vfbmp.ulVersion = 1L;
        vfbmp.ulWhat = OT_STATIC;
        vfbmp.cbName = 7L;
        CopyMemory (vfbmp.szName, "BITMAP\0", 7);
        vfbmp.res[0] = vfbmp.res[1] = vfbmp.res[2] = 0L;
        vfbmp.bm.bmType = 0;

        /* Read in the card dimensions */
        MyByteReadFile(lpStream->fh, &vfbmp.bm.bmWidth, sizeof(vfbmp.bm.bmWidth));
        MyByteReadFile(lpStream->fh, &vfbmp.bm.bmHeight, sizeof(vfbmp.bm.bmHeight));

        vfbmp.bm.bmWidthBytes = ((vfbmp.bm.bmWidth + 0x000f) >> 4) << 1;
        vfbmp.bm.bmPlanes = 1;
        vfbmp.bm.bmBitsPixel = 1;
        vfbmp.bm.bmBits = 0L;

        /* Read in the card position */
        MyByteReadFile(lpStream->fh, &Temp, sizeof(Temp));
        xTmp = Temp ;

        xTmp = (xTmp * CharFixWidth) / 8;

        MyByteReadFile(lpStream->fh, &Temp, sizeof(Temp));
        yTmp = Temp ;

        yTmp = (yTmp * CharFixHeight) / 8;

        /* We don't scale for compatibility's sake */
        SetRect(&(CurCard.rcObject), xTmp, yTmp,
                xTmp + vfbmp.bm.bmWidth - 1, yTmp + vfbmp.bm.bmHeight - 1);
        vcLeft = (DWORD) sizeof(FAKEBITMAP);
        vpfbmp = (BYTE *)&vfbmp;
    }
#if 0
    if (vcLeft)
    {
        cRead = min(vcLeft, cb);
        for (i = 0; i < (int) cRead; i++)
            *lpbit++ = *vpfbmp++;
            vcLeft -= cRead;
        cb -= cRead;
    }
    if (cb) {
        /* Old bitmaps are always under 64K */
        cRead += (DWORD) MyByteReadFile(lpStream->fh, lpbit + cRead, (int)cb);
        vcbLeftToRead -= cRead;
    }

    return cRead;
#endif

   switch (iPos)
   {
      case 0:
         *(DWORD *)lpbit = vfbmp.ulVersion;
         break;

      case 1:
         *(DWORD *)lpbit = vfbmp.ulWhat;
         break;

      case 2:
         *(DWORD *)lpbit = vfbmp.cbName;
         break;

      case 3:
         lstrcpy ((LPTSTR) lpbit, vfbmp.szName);
         break;

      case 4:
         *(DWORD *)lpbit = vfbmp.res[0];
         break;

      case 5:
         *(DWORD *)lpbit = vfbmp.res[1];
         break;

      case 6:
         // This needs to return the amount of data
         *(DWORD *)lpbit = vcbLeftToRead + sizeof(WIN16BITMAP);
         break;

      case 7:
         // Should now be asking for the 16bit BITMAP Structure
         // so copy it over
         CopyMemory(lpbit, &vfbmp.bm, sizeof(WIN16BITMAP));
         break;

      case 8:
         MyByteReadFile(lpStream->fh, lpbit, vcbLeftToRead);
         vcbLeftToRead = 0;
         break;

      default:
#if DBG
         MessageBox( NULL, TEXT("Should not have been called 10 times!"),
                     TEXT("ReadOldStream"), MB_OK );
#endif
         cb = 0;
         break;
   };
   iPos++;

   return cb;
}

DWORD ReadStream(LPCARDSTREAM lpStream, LPBYTE lpbit, DWORD cb)
{
    if (MyByteReadFile(lpStream->fh, lpbit, cb))
        return (cb);
    return (0);
}

DWORD WriteStream(LPCARDSTREAM lpStream, LPBYTE lpbit, DWORD cb)
{
   if (MyByteWriteFile(lpStream->fh, lpbit, cb))
        return (cb);
    return (0);
}

HBITMAP MakeBitmapCopy(HBITMAP hbmSrc, PBITMAP pBitmap, HDC hDestDC )
{
    HBITMAP hBitmap = NULL;
    HDC hDCSrc = NULL;
    HDC hDCDest = NULL;
    HDC hDC;
    BOOL fError = TRUE;

    if( hDestDC == NULL )
    {
        hDC = GetDC(hIndexWnd);
        hDCSrc  = CreateCompatibleDC(hDC); /* get memory dc */
        hDCDest = CreateCompatibleDC(hDC);
        ReleaseDC(hIndexWnd, hDC);
    }
    else
    {
        hDCSrc  = CreateCompatibleDC( hDestDC );
        hDCDest = CreateCompatibleDC( hDestDC );
    }
    if (!hDCSrc || !hDCDest)
        goto MakeCopyEnd;

    /* select in passed bitmap */
    if (!SelectObject(hDCSrc, hbmSrc))
        goto MakeCopyEnd;

    /* create new monochrome bitmap */
    hBitmap= CreateBitmap( pBitmap->bmWidth, pBitmap->bmHeight, 1, GetDeviceCaps(hDCDest,NUMCOLORS), NULL );
    if( !hBitmap )
    {
        goto MakeCopyEnd;
    }

    /* Now blt the bitmap contents.  The screen driver in the source will
       "do the right thing" in copying color to black-and-white. */

    if( SelectObject(hDCDest, hBitmap) )
    {
        if( BitBlt(hDCDest, 0, 0, pBitmap->bmWidth, pBitmap->bmHeight, hDCSrc, 0, 0, SRCCOPY) )
        {
            fError= FALSE;
        }
    }
    if( fError )
    {

        DeleteObject(hBitmap);
        hBitmap = NULL;
        goto MakeCopyEnd;
    }
    fError  = FALSE;

MakeCopyEnd:
    if (fError)
        ErrorMessage(E_BITMAP_COPY_FAILED);
    if (hDCSrc)
        DeleteObject(hDCSrc);
    if (hDCDest)
        DeleteObject(hDCDest);
    return hBitmap;
}

/* OleError() - Disposes of OLE errors.
 *
 * Note:  This function (and ErrorMessage()) both use the window
 *        hwndError as parent when putting up dialogs.
 */
BOOL OleError(
    OLESTATUS olestat)
{
    switch (olestat) {
        case OLE_WAIT_FOR_RELEASE:
            cOleWait++;

        case OLE_OK:
            return FOLEERROR_OK;

        case OLE_ERROR_STATIC:              /* Only happens w/ dbl click */
            ErrorMessage(W_STATIC_OBJECT);
            break;

        case OLE_ERROR_COMM:
            ErrorMessage(E_FAILED_TO_LAUNCH_SERVER);
            break;

        case OLE_ERROR_REQUEST_NATIVE:
        case OLE_ERROR_REQUEST_PICT:
        case OLE_ERROR_ADVISE_NATIVE:
        case OLE_ERROR_ADVISE_PICT:
        case OLE_ERROR_OPEN:                /* Invalid link? */
        case OLE_ERROR_NAME:
            if (CurCard.otObject == LINK)
            {
                if (hwndError == hIndexWnd)
                {
                    if (DialogBox(hIndexInstance, (LPTSTR) MAKEINTRESOURCE(DTINVALIDLINK),
                                  hwndError, (WNDPROC)lpfnInvalidLink) == IDD_LINK)
                    PostMessage(hIndexWnd, WM_COMMAND, LINKSDIALOG, 0L);
                }
                else
                {
                    /* Failed, but already in LinksDlg!! */
                    ErrorMessage(E_FAILED_TO_UPDATE_LINK);
                }

                return FALSE;
            }
            break;

        case OLE_BUSY:
            ErrorMessage(E_SERVER_BUSY);
            break;

        default:
            return FOLEERROR_NOTGIVEN;
    }
    return FOLEERROR_GIVEN;
}

BOOL GetNewLinkName(HWND hwndOwner, PCARD pCard)
{
    BOOL    fPath    = FALSE;
    BOOL    fSuccess = FALSE;
    BOOL    fFile    = FALSE;
    DWORD   dwSize   = 0 ;
    HANDLE  hData;
    HANDLE  hData2   = NULL;
    HANDLE  hData3   = NULL;
    HWND    hwndOwnSave = NULL;
    LPTSTR  lpData2     = NULL;
    LPTSTR  lpstrData   = NULL;
    LPTSTR  lpstrFile   = NULL;
    LPTSTR  lpstrLink   = NULL;
    LPTSTR  lpstrPath   = NULL;
    LPTSTR  lpstrTemp   = NULL;
    OLECHAR*pOleData    = NULL;
    TCHAR   szDocDefExt[10];
    TCHAR   szDocFile[PATHMAX];
    TCHAR   szDocPath[PATHMAX];

    hwndOwnSave = OFN.hwndOwner;

    if (OLE_OK != OleGetData(pCard->lpObject, vcfLink, &hData) ||
        !(pOleData = GlobalLock(hData)))
        goto Error;

    lpstrData= Ole2Native( pOleData,3 );   // convert to native chars

    lpstrTemp = lpstrData;
    while (*lpstrTemp++)
        continue;
    lpstrPath = lpstrFile = lpstrTemp;
    while (*(lpstrTemp = CharNext(lpstrTemp)))
        if (*lpstrTemp == TEXT('\\'))
            lpstrFile = lpstrTemp + 1;

    lstrcpy(szDocFile, lpstrFile);
    fPath = (*(lpstrFile - 1));
    *(lpstrFile - 1) = TEXT('\0');
    lstrcpy(szDocPath, ((lpstrPath != lpstrFile) ? lpstrPath : TEXT("")));

    /* Fix bug:  If we have <drive>:\<filename>, the starting directory
     * should be <drive>:\, not just <drive>:.
     */
    if (lstrlen(szDocPath) == 2)
        lstrcat(szDocPath, TEXT("\\"));

    if (fPath)
        *(lpstrFile - 1) = TEXT('\\');

    while (*lpstrFile != TEXT('.') && *lpstrFile)
        lpstrFile++;

    /* Make a filter that respects the link's class name */
    OFN.nFilterIndex    = MakeFilterSpec(lpstrData, lpstrFile, szServerFilter);
    lstrcpy(szDocDefExt, (*lpstrFile) ? lpstrFile + 1 : TEXT(""));
    OFN.lpstrDefExt       = NULL;
    OFN.lpstrCustomFilter = szCustFilterSpec;
    OFN.lpstrFile       = szDocFile;
    OFN.lpstrTitle      = szLinkCaption;
    OFN.lpstrFilter     = szServerFilter;
    OFN.lpstrInitialDir = szDocPath;
    OFN.hwndOwner       = hwndOwner;
    OFN.Flags             = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;

    LockData(0);
    fFile = GetOpenFileName(&OFN);
    UnlockData(0);
    if (fFile)
    {
        if (!(hData2 = GlobalAlloc(GMEM_DDESHARE | GMEM_ZEROINIT, ByteCountOf(PATHMAX * 2)))
            || !(lpstrLink = lpstrTemp = (LPTSTR) GlobalLock(hData2)))
            goto Error;

        /* If the user didn't specify an extension, use the default */
        if (OFN.nFileExtension == lstrlen(szDocFile) && OFN.nFilterIndex)
        {
            LPTSTR   lpstrFilter = szServerFilter;

            while (*lpstrFilter && --OFN.nFilterIndex)
            {
                while (*lpstrFilter++) ;
                while (*lpstrFilter++) ;
            }
            if (*lpstrFilter)
            {
                 while (*lpstrFilter++) ;
                 lpstrFilter++;
                 lstrcat(szDocFile, lpstrFilter);
            }
        }

        while (*lpstrTemp++ = *lpstrData++)
            continue;
        lstrcpy(lpstrTemp, szDocFile);
        lpstrTemp += lstrlen(lpstrTemp) + 1;
        lpstrData += lstrlen(lpstrData) + 1;
        while (*lpstrTemp++ = *lpstrData++);
        *lpstrTemp = (TCHAR) 0;
        dwSize = (DWORD)(lpstrTemp - lpstrLink + 1);

        // Convert to ascii.  OLE1 doesn't do unicode
        {
             char buff[400];
             INT oleSize;
             oleSize= WideCharToMultiByte(
                        CP_ACP, 0,
                        lpstrLink, dwSize,
                        buff, sizeof(buff),
                        NULL, NULL );
             CopyMemory( lpstrLink, buff, oleSize );
        }

        /* Unlock the appropriate memory blocks */
        GlobalUnlock(hData);    /* Unlock, because OleSetData() may free it */
        GlobalUnlock(hData2);
        GlobalFree( lpstrData );
        lpstrData = NULL;

        /* Compress the block to minimal size */
        hData3 = GlobalReAlloc(hData2, ByteCountOf(lpstrTemp-lpstrLink+1), GMEM_MOVEABLE);
        if (!hData3)
            hData3 = hData2;

        if (!OleError(OleSetData(pCard->lpObject, vcfLink, hData3)))
        {
            WaitForObject(pCard->lpObject);
            fSuccess = (OleStatusCallBack == OLE_OK);
            if (fSuccess)
            {
                fSuccess = !OleError(OleUpdate(pCard->lpObject));
                if (fSuccess)
                {
                    WaitForObject(pCard->lpObject);
                    if (OleStatusCallBack == OLE_OK)
                    {
                        CurCardHead.flags |= FDIRTY;
                        fSuccess = TRUE;
                    }
                    else
                        fSuccess = FALSE;
                }
            }
            if (!fSuccess)
            {
                ErrorMessage(E_FAILED_TO_UPDATE);
                goto Error;
            }
        }
    }

    if(CommDlgExtendedError()) /* Assumes low memory. */
        IndexOkError(EINSMEMORY);

Error:
    if (!fSuccess) {
        if (hData3)
            GlobalFree(hData3);

        if (pOleData)
            GlobalUnlock(hData);

        if( lpstrData )
            LocalFree( lpstrData );
    }

    OFN.hwndOwner       = hwndOwnSave;
    return fSuccess;
}

void PicSaveUndo(PCARD pCard)
{
  WORD fOleErrMsg;

    /* If Undo object exists, delete it */
    DeleteUndoObject();

    /* Clone the current object */
    if (pCard->lpObject)
    {
        if (fOleErrMsg = OleError(OleClone(pCard->lpObject, lpclient,
                 lhcdoc, szUndo, (LPOLEOBJECT FAR *)&lpObjectUndo)))
        {
            lpObjectUndo = NULL;
            if (fOleErrMsg == FOLEERROR_NOTGIVEN)
                ErrorMessage(W_FAILED_TO_CLONE_UNDO);
        }
        else
            otObjectUndo = pCard->otObject;
    }
}

void ErrorMessage(int id)
{
    TCHAR buf[300];

    LoadString(hIndexInstance, id, buf, CharSizeOf(buf));
    MessageBox(hwndError, buf, szWarning, MB_OK | MB_ICONEXCLAMATION);
}

/* WaitForObject() - Waits, dispatching messages, until the object is free.
 *
 * If the object is busy, spin in a dispatch loop.
 */
void WaitForObject(LPOLEOBJECT lpObject)
{
    if (lpObject)
    {
        while (OleQueryReleaseStatus(lpObject) == OLE_BUSY)
            ProcessMessage(hIndexWnd, hAccel);
    }
}


void Hourglass(BOOL fOn)
{
    if (fOn)
    {
        if (!(cWait++))
            hcurOle = SetCursor(hWaitCurs);
    }
    else
    {
        if (!(--cWait) && hcurOle)
        {
            SetCursor(hcurOle);
            hcurOle = NULL;
        }
    }
}

void PicCreateFromFile(LPTSTR szPackageClass, LPTSTR szDropFile, BOOL fLink)
{
    LPOLEOBJECT     lpObject;
    OBJECTTYPE      otObject;
    int fError;
#ifndef OLE_20
    CHAR  aszBuf1[PATHMAX];
    CHAR  aszBuf2[PATHMAX];
#endif

    Hourglass(TRUE);

    /* Don't replace the current object unless we're successful */
#ifndef OLE_20
    wsprintfA (szObjectName, szObjFormat, idObjectMax + 1);
#else
    wsprintfW (szObjectName, szObjFormat, idObjectMax + 1);
#endif

#ifndef OLE_20
    WideCharToMultiByte (CP_ACP, 0, szPackageClass, -1, aszBuf1, PATHMAX, NULL, NULL);
    WideCharToMultiByte (CP_ACP, 0, szDropFile, -1, aszBuf2, PATHMAX, NULL, NULL);
#endif

    if (fLink)
    {
#ifndef OLE_20
        fError = OleCreateLinkFromFile(szPStdFile, lpclient, aszBuf1,
                                       aszBuf2, NULL, lhcdoc, szObjectName,
                                       &lpObject, olerender_draw, 0);
#else
        fError = OleCreateLinkFromFile(szPStdFile, lpclient, szPackageClass,
                                       szDropFile, NULL, lhcdoc, szObjectName,
                                       &lpObject, olerender_draw, 0);
#endif
    }
    else
    {
#ifndef OLE_20
        fError = OleCreateLinkFromFile(szPStdFile, lpclient, aszBuf1,
                                       aszBuf2, NULL, lhcdoc, szObjectName,
                                       &lpObject, olerender_draw, 0);
#else
        fError = OleCreateFromFile(szPStdFile, lpclient, szPackageClass,
                                   szDropFile, lhcdoc, szObjectName,
                                   &lpObject, olerender_draw, 0);
#endif
    }

    WaitForObject(lpObject);
    if (fError != OLE_WAIT_FOR_RELEASE)
    {
        lpObject = NULL;
        ErrorMessage(E_DRAG_DROP_FAILED);
    } else {
        LONG objType;

        /* Figure out what kind of object we have */
        OleQueryType(lpObject, &objType);
        switch (objType) {
            case OT_EMBEDDED:   otObject = EMBEDDED;    break;
            case OT_LINK:       otObject = LINK;        break;
            default:            otObject = STATIC;      break;
        }
        DoSetHostNames(lpObject, otObject);
    }

    if (lpObject)
    {
        if (CurCard.lpObject)
            PicDelete(&CurCard);
        CurCard.lpObject = lpObject;
        CurCard.otObject = otObject;
        CurCard.idObject = idObjectMax++;
        InvalidateRect(hEditWnd, NULL, TRUE);
        SetRect(&(CurCard.rcObject), 0, 0, 0, 0);
        CurCardHead.flags |= FDIRTY;
#if 0
        fCreateFromFile = TRUE;
#endif
    }
}

BOOL EditingEmbObject(
    PCARD pCard)
{
    if (pCard->lpObject && pCard->otObject == EMBEDDED &&
            OleQueryOpen(pCard->lpObject) == OLE_OK) /* embedded object open for editing */
        return TRUE;
    else
        return FALSE;
}

int UpdateEmbObject(
    PCARD pCard,
    int Flags)
{
    int Result = IDYES;
    TCHAR szMsg[100];

    /* If an embedded object is open for editing, try to update */
    if (EditingEmbObject(&CurCard))
    {
        LoadString(hIndexInstance, IDS_UPDATEEMBOBJECT, szMsg, CharSizeOf(szMsg));
        Result = MessageBox(hIndexWnd, szMsg, szCardfile, Flags);
        if (Result == IDYES)
        {
            switch (OleError(OleUpdate(CurCard.lpObject)))
            {
                case FOLEERROR_NOTGIVEN:
                    ErrorMessage(E_FAILED_TO_UPDATE);
                    break;
#if 0
                case FOLEERROR_GIVEN:
                    break;
#endif
                case FOLEERROR_OK:
                    WaitForObject(CurCard.lpObject);
                    CurCardHead.flags |= FDIRTY;
                    break;
            }
        }
    }
    return Result;
}

BOOL InsertObjectInProgress(
    void)
{
    TCHAR szMsg[200];

    if (EditingEmbObject(&CurCard) && !fInsertComplete)
    {
        LoadString(hIndexInstance, IDS_RETRYAFTERINSERT, szMsg, CharSizeOf(szMsg));
        MessageBox(hIndexWnd, szMsg, szCardfile, MB_OK);
        return TRUE;
    }
    return FALSE;
}

void DoSetHostNames(
    LPOLEOBJECT lpObject,
    OBJECTTYPE otObject)
{
    if (lpObject && otObject == EMBEDDED)
    {
#ifndef OLE_20
        CHAR  aszCardfile[60];
        CHAR  aszBuf[PATHMAX];

        WideCharToMultiByte (CP_ACP, 0, szCardfile, -1, aszCardfile, 60, NULL, NULL);
        WideCharToMultiByte (CP_ACP, 0, (*CurIFile) ? CurIFile : szUntitled, -1,
                             aszBuf, PATHMAX, NULL, NULL);
#endif

        WaitForObject(lpObject);
#ifndef OLE_20
        OleError(OleSetHostNames(lpObject, aszCardfile, aszBuf));
#else
        OleError(OleSetHostNames(lpObject, szCardfile,
                                 (*CurIFile) ? CurIFile : szUntitled));
#endif
    }
}

/* delete undo object if any */
void DeleteUndoObject(
    void)
{
    if (!lpObjectUndo)
        return;
    WaitForObject(lpObjectUndo);
    if (lpObjectUndo && OleError(OleDelete(lpObjectUndo))) {
        ErrorMessage(E_FAILED_TO_DELETE_OBJECT);
    }
    lpObjectUndo = NULL;
}
