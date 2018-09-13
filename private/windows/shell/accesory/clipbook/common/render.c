
/*****************************************************************************

                                    R E N D E R

    Name:       render.c
    Date:       19-Apr-1994
    Creator:    Unknown

    Description:

    History:
        08-Jul-1994 John Fu     Fix RenderFormatDibToBitmap() to accept
                                palette as parameter and use it.

*****************************************************************************/



#include    <windows.h>
#include    "common.h"
#include    "clipfile.h"
#include    "render.h"
#include    "debugout.h"
#include    "dib.h"





/*
 *
 *      RenderFormat() -
 *
 *  Read the data from fh and SetClipboardData() with it.
 *  CLAUSGI - don't setClipboardData - just return the handle...
 */

HANDLE RenderFormat(
    FORMATHEADER    *pfmthdr,
    register HANDLE fh)
{
HANDLE            hBitmap;
register HANDLE   hData;
LPSTR             lpData;
DWORD             MetaOffset;     /* special case hack for metafiles */
BITMAP            bitmap;
HPALETTE          hPalette;
LPLOGPALETTE      lpLogPalette;
DWORD             dwBytesRead;
DWORD             dwOffset;

    if (PRIVATE_FORMAT(pfmthdr->FormatID))
       pfmthdr->FormatID = RegisterClipboardFormatW(pfmthdr->Name);




    // Special case hack for metafiles to get hData referencing
    // the metafile bits, not the METAFILEPICT structure.


    switch (pfmthdr->FormatID)
        {
        case CF_METAFILEPICT:
            if (!fNTReadFileFormat)
               {
               MetaOffset = sizeof(WIN31METAFILEPICT);
               }
            else
               {
               MetaOffset = sizeof(METAFILEPICT);
               }
            break;
        case CF_BITMAP:
            if (!fNTReadFileFormat)
               {
               MetaOffset = sizeof(WIN31BITMAP);
               }
            else
               {
               MetaOffset = sizeof(BITMAP);
               }
            break;
        default:
            MetaOffset = 0;
            break;
        }



    if (!(hData = GlobalAlloc(GHND, pfmthdr->DataLen - MetaOffset)))
        {
        PERROR(TEXT("GlobalAlloc failure in RenderFormat\n\r"));
        return NULL;
        }


    if (!(lpData = GlobalLock(hData)))
        {
        PERROR(TEXT("GlobalLock failure in RenderFormat\n\r"));
        GlobalFree(hData);
        return NULL;
        }



    dwOffset = pfmthdr->DataOffset + MetaOffset;

    PINFO("Getting data for %ws at offset %ld\r\n",pfmthdr->Name, dwOffset);
    SetFilePointer(fh, dwOffset, NULL, FILE_BEGIN);

    ReadFile (fh, lpData, pfmthdr->DataLen - MetaOffset, &dwBytesRead, NULL);

    if(pfmthdr->DataLen - MetaOffset != dwBytesRead)
         {
         // Error in reading the file
         GlobalUnlock(hData);
         GlobalFree(hData);
         PERROR(TEXT("RenderFormat: Read err, expected %d bytes, got %d\r\n"),
               pfmthdr->DataLen - MetaOffset, dwBytesRead);
         return (NULL);
         }

    // As when we write these we have to special case a few of
    // these guys.  This code and the write code should match in terms
    // of the sizes and positions of data blocks being written out.
    // HEY, YOU! READ THIS: EVERY case in this switch should have a
    // GlobalUnlock(hData);
    // statement in it. We go in with the block locked, but should come
    // out with the block unlocked. Yeah, it's not structured, but most
    // of these formats require access to the data.. calling GlobalUnlock()
    // then GlobalLock() again looked even worse.
    switch (pfmthdr->FormatID)
        {
        case CF_ENHMETAFILE:
           {
           HENHMETAFILE hemf;

           hemf = SetEnhMetaFileBits(pfmthdr->DataLen, lpData);

           GlobalUnlock(hData);
           GlobalFree(hData);
           hData = hemf;
           break;
           }

        case CF_METAFILEPICT:
           {
           HANDLE            hMF;
           HANDLE            hMFP;
           LPMETAFILEPICT    lpMFP;

           /* Create the METAFILE with the bits we read in. */
           hMF = SetMetaFileBitsEx(pfmthdr->DataLen, lpData);
           GlobalUnlock(hData);
           GlobalFree(hData);
           hData = NULL;

           if (hMF)
              {
              /* Alloc a METAFILEPICT header. */

              if (hMFP = GlobalAlloc(GHND, (DWORD)sizeof(METAFILEPICT)))
                 {
                 if (!(lpMFP = (LPMETAFILEPICT)GlobalLock(hMFP)))
                    {
                    GlobalFree(hMFP);
                    }
                 else
                    {
                    /* Reposition to the start of the METAFILEPICT header. */
                    SetFilePointer(fh, pfmthdr->DataOffset, NULL, FILE_BEGIN);

                    /* Read in the data */
                    if (fNTReadFileFormat)
                       {
                       ReadFile(fh, lpMFP, sizeof(METAFILEPICT),
                             &dwBytesRead, NULL);
                       }
                    else
                       {
                       WIN31METAFILEPICT w31mfp;

                       ReadFile(fh, &w31mfp, sizeof(w31mfp), &dwBytesRead, NULL);
                       if (sizeof(w31mfp) == dwBytesRead)
                          {
                          lpMFP->mm = w31mfp.mm;
                          lpMFP->xExt = w31mfp.xExt;
                          lpMFP->yExt = w31mfp.yExt;
                          }
                       }

                    lpMFP->hMF = hMF;         /* Update the METAFILE handle  */
                    GlobalUnlock(hMFP);       /* Unlock the header           */
                    hData = hMFP;             /* Stuff this in the clipboard */
                    }
                 }
              }
           break;
           }

        case CF_BITMAP:
           // Reposition to the start of the METAFILEPICT header.
           SetFilePointer(fh, pfmthdr->DataOffset, NULL, FILE_BEGIN);


           /* Read in the BITMAP struct */
           if (fNTReadFileFormat)
              {
              ReadFile(fh, &bitmap, sizeof(BITMAP), &dwBytesRead, NULL);
              }
           else
              {
              // Read in an old-style BITMAP struct, and set the fields
              // of the new-style BITMAP from that.
              WIN31BITMAP w31bm;
              ReadFile(fh, &w31bm, sizeof(w31bm), &dwBytesRead, NULL);

              bitmap.bmType       = w31bm.bmType;
              bitmap.bmWidth      = w31bm.bmWidth;
              bitmap.bmHeight     = w31bm.bmHeight;
              bitmap.bmWidthBytes = w31bm.bmWidthBytes;
              bitmap.bmPlanes     = w31bm.bmPlanes;
              bitmap.bmBitsPixel  = w31bm.bmBitsPixel;
              }

           // Set the bmBits member of the BITMAP to point to our existing
           // bits and make the bitmap.
           bitmap.bmBits = lpData;
           hBitmap = CreateBitmapIndirect(&bitmap);

           // Dump the original data (which was just the bitmap bits) and
           // make the bitmap handle our data handle.
           GlobalUnlock(hData);
           GlobalFree(hData);
           hData = hBitmap;       // Stuff this in the clipboard
           break;

        case CF_PALETTE:
           lpLogPalette = (LPLOGPALETTE)lpData;

           hPalette = CreatePalette(lpLogPalette);

           GlobalUnlock(hData);
           GlobalFree(hData);

           hData = hPalette;
           break;

        default:
           GlobalUnlock(hData);
           break;
        }


    return(hData);

}








HANDLE RenderFormatDibToBitmap(
    FORMATHEADER    *pfmthdr,
    register HANDLE fh,
    HPALETTE        hPalette)
{
HANDLE            hBitmap;
register HANDLE   hData;
LPSTR             lpData;
DWORD             dwBytesRead;
DWORD             dwOffset;



    if (PRIVATE_FORMAT(pfmthdr->FormatID))
        pfmthdr->FormatID = RegisterClipboardFormatW(pfmthdr->Name);




    if (!(hData = GlobalAlloc(GHND, pfmthdr->DataLen)))
        {
        PERROR(TEXT("GlobalAlloc failure in RenderFormat\n\r"));
        return NULL;
        }


    if (!(lpData = GlobalLock(hData)))
        {
        PERROR(TEXT("GlobalLock failure in RenderFormat\n\r"));
        GlobalFree(hData);
        return NULL;
        }



    dwOffset = pfmthdr->DataOffset;

    PINFO("Getting data for %ws at offset %ld\r\n",pfmthdr->Name, dwOffset);
    SetFilePointer(fh, dwOffset, NULL, FILE_BEGIN);

    ReadFile (fh, lpData, pfmthdr->DataLen, &dwBytesRead, NULL);

    if(pfmthdr->DataLen != dwBytesRead)
        {
        // Error in reading the file
        GlobalUnlock(hData);
        GlobalFree(hData);

        PERROR (TEXT("RenderFormat: Read err, expected %d bytes, got %d\r\n"),
                pfmthdr->DataLen, dwBytesRead);
        return (NULL);
        }


    GlobalUnlock(hData);

    hBitmap = BitmapFromDib (hData, hPalette);

    GlobalFree (hData);


    return (hBitmap);

}
