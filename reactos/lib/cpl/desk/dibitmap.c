/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            lib/cpl/desk/dibitmap.c
 * PURPOSE:         DIB loading
 * 
 * PROGRAMMERS:     Trevor McCort (lycan359@gmail.com)
 */

#include "dibitmap.h"

DIBitmap *DibLoadImage(TCHAR *filename)
{
    BOOL               bSuccess;
    DWORD              dwFileSize, dwHighSize, dwBytesRead;
    HANDLE             hFile;
    DIBitmap           *bitmap;
    
    hFile = CreateFile(filename,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_FLAG_SEQUENTIAL_SCAN,
                       NULL);
    
    if(hFile == INVALID_HANDLE_VALUE)
        return NULL;
    
    dwFileSize = GetFileSize(hFile, &dwHighSize);
    
    if(dwHighSize)
    {
        CloseHandle(hFile);
        return NULL;
    }

    bitmap = malloc(sizeof(DIBitmap));
    if(!bitmap)
        return NULL;
    
    bitmap->header = malloc(dwFileSize);
    if(!bitmap->header)
    {
        CloseHandle(hFile);
        return NULL;
    }
    
    bSuccess = ReadFile(hFile, bitmap->header, dwFileSize, &dwBytesRead, NULL);
    CloseHandle(hFile);
    
    if(!bSuccess || (dwBytesRead != dwFileSize)         
                 || (bitmap->header->bfType != * (WORD *) "BM") 
                 || (bitmap->header->bfSize != dwFileSize))
    {
        free(bitmap->header);
        return NULL;
    }
    
    bitmap->info = (BITMAPINFO *)(bitmap->header + 1);
    bitmap->bits = (BYTE *)bitmap->header + bitmap->header->bfOffBits;
    
    /* Get the DIB width and height */
    if(bitmap->info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        bitmap->width  = ((BITMAPCOREHEADER *)bitmap->info)->bcWidth;
        bitmap->height = ((BITMAPCOREHEADER *)bitmap->info)->bcHeight;
    }
    else
    {
        bitmap->width   =     bitmap->info->bmiHeader.biWidth;
        bitmap->height  = abs(bitmap->info->bmiHeader.biHeight);
    }
    
    return bitmap;
}

void DibFreeImage(DIBitmap *bitmap)
{
    if(bitmap == NULL)
        return;

    /* Free the header */
    if(bitmap->header != NULL)
        free(bitmap->header);

    /* Free the bitmap structure */
    if(bitmap != NULL)
        free(bitmap);
}

