/*
 *	IMAGEHLP library
 *
 *	Copyright 1998	Patrik Stridvall
 *	Copyright 2003	Mike McCormack
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * These functions are partially documented at:
 *   http://www.cs.auckland.ac.nz/~pgut001/pubs/authenticode.txt
 */

/* INCLUDES ******************************************************************/

#include "precomp.h"

//#define NDEBUG
#include <debug.h>
#define _WINNT_H
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(imagehlp);

/* FUNCTIONS *****************************************************************/

static
BOOL
IMAGEHLP_GetSecurityDirOffset(HANDLE handle,
                              DWORD *pdwOfs,
                              DWORD *pdwSize)
{
    IMAGE_DOS_HEADER dos_hdr;
    IMAGE_NT_HEADERS nt_hdr;
    DWORD count;
    BOOL r;
    IMAGE_DATA_DIRECTORY *sd;

    TRACE("handle %p\n", handle );

    /* read the DOS header */
    count = SetFilePointer( handle, 0, NULL, FILE_BEGIN );
    if( count == INVALID_SET_FILE_POINTER )
        return FALSE;
    count = 0;
    r = ReadFile( handle, &dos_hdr, sizeof dos_hdr, &count, NULL );
    if( !r )
        return FALSE;
    if( count != sizeof dos_hdr )
        return FALSE;

    /* read the PE header */
    count = SetFilePointer( handle, dos_hdr.e_lfanew, NULL, FILE_BEGIN );
    if( count == INVALID_SET_FILE_POINTER )
        return FALSE;
    count = 0;
    r = ReadFile( handle, &nt_hdr, sizeof nt_hdr, &count, NULL );
    if( !r )
        return FALSE;
    if( count != sizeof nt_hdr )
        return FALSE;

    sd = &nt_hdr.OptionalHeader.
                    DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];

    TRACE("size = %lx addr = %lx\n", sd->Size, sd->VirtualAddress);
    *pdwSize = sd->Size;
    *pdwOfs = sd->VirtualAddress;

    return TRUE;
}

/***********************************************************************
 * IMAGEHLP_GetCertificateOffset (INTERNAL)
 *
 * Read a file's PE header, and return the offset and size of the
 *  security directory.
 */
static BOOL IMAGEHLP_GetCertificateOffset( HANDLE handle, DWORD num,
                                           DWORD *pdwOfs, DWORD *pdwSize )
{
    DWORD size, count, offset, len, sd_VirtualAddr;
    BOOL r;

    r = IMAGEHLP_GetSecurityDirOffset( handle, &sd_VirtualAddr, &size );
    if( !r )
        return FALSE;

    offset = 0;
    /* take the n'th certificate */
    while( 1 )
    {
        /* read the length of the current certificate */
        count = SetFilePointer( handle, sd_VirtualAddr + offset,
                                 NULL, FILE_BEGIN );
        if( count == INVALID_SET_FILE_POINTER )
            return FALSE;
        r = ReadFile( handle, &len, sizeof len, &count, NULL );
        if( !r )
            return FALSE;
        if( count != sizeof len )
            return FALSE;

        /* check the certificate is not too big or too small */
        if( len < sizeof len )
            return FALSE;
        if( len > (size-offset) )
            return FALSE;
        if( !num-- )
            break;

        /* calculate the offset of the next certificate */
        offset += len;
        if( offset >= size )
            return FALSE;
    }

    *pdwOfs = sd_VirtualAddr + offset;
    *pdwSize = len;

    TRACE("len = %lx addr = %lx\n", len, sd_VirtualAddr + offset);

    return TRUE;
}

static
WORD
CalcCheckSum(DWORD StartValue,
             LPVOID BaseAddress,
             DWORD WordCount)
{
   LPWORD Ptr;
   DWORD Sum;
   DWORD i;

   Sum = StartValue;
   Ptr = (LPWORD)BaseAddress;
   for (i = 0; i < WordCount; i++)
     {
    Sum += *Ptr;
    if (HIWORD(Sum) != 0)
      {
         Sum = LOWORD(Sum) + HIWORD(Sum);
      }
    Ptr++;
     }

   return (WORD)(LOWORD(Sum) + HIWORD(Sum));
}

/*
 * @unimplemented
 */
BOOL
IMAGEAPI
ImageAddCertificate(HANDLE FileHandle,
                    LPWIN_CERTIFICATE Certificate,
                    PDWORD Index)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *		ImageEnumerateCertificates (IMAGEHLP.@)
 */
BOOL IMAGEAPI ImageEnumerateCertificates(
    HANDLE FileHandle, WORD TypeFilter, PDWORD CertificateCount,
    PDWORD Indices, DWORD IndexCount)
{
    DWORD size, count, offset, sd_VirtualAddr;
    WIN_CERTIFICATE hdr;
    const size_t cert_hdr_size = sizeof hdr - sizeof hdr.bCertificate;
    BOOL r;

    TRACE("%p %hd %p %p %ld\n",
           FileHandle, TypeFilter, CertificateCount, Indices, IndexCount);

    if( Indices )
    {
        FIXME("Indicies not handled!\n");
        return FALSE;
    }

    r = IMAGEHLP_GetSecurityDirOffset( FileHandle, &sd_VirtualAddr, &size );
    if( !r )
        return FALSE;

    offset = 0;
    *CertificateCount = 0;
    while( offset < size )
    {
        /* read the length of the current certificate */
        count = SetFilePointer( FileHandle, sd_VirtualAddr + offset,
                                 NULL, FILE_BEGIN );
        if( count == INVALID_SET_FILE_POINTER )
            return FALSE;
        r = ReadFile( FileHandle, &hdr, (DWORD)cert_hdr_size, &count, NULL );
        if( !r )
            return FALSE;
        if( count != cert_hdr_size )
            return FALSE;

        TRACE("Size = %08lx  id = %08hx\n",
               hdr.dwLength, hdr.wCertificateType );

        /* check the certificate is not too big or too small */
        if( hdr.dwLength < cert_hdr_size )
            return FALSE;
        if( hdr.dwLength > (size-offset) )
            return FALSE;

        if( (TypeFilter == CERT_SECTION_TYPE_ANY) ||
            (TypeFilter == hdr.wCertificateType) )
        {
            (*CertificateCount)++;
        }

        /* next certificate */
        offset += hdr.dwLength;
    }

    return TRUE;
}

/***********************************************************************
 *		ImageGetCertificateData (IMAGEHLP.@)
 *
 *  FIXME: not sure that I'm dealing with the Index the right way
 */
BOOL IMAGEAPI ImageGetCertificateData(
                HANDLE handle, DWORD Index,
                LPWIN_CERTIFICATE Certificate, PDWORD RequiredLength)
{
    DWORD r, offset, ofs, size, count;

    TRACE("%p %ld %p %p\n", handle, Index, Certificate, RequiredLength);

    if( !IMAGEHLP_GetCertificateOffset( handle, Index, &ofs, &size ) )
        return FALSE;

    if( !Certificate )
    {
        *RequiredLength = size;
        return TRUE;
    }

    if( *RequiredLength < size )
    {
        *RequiredLength = size;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    *RequiredLength = size;

    offset = SetFilePointer( handle, ofs, NULL, FILE_BEGIN );
    if( offset == INVALID_SET_FILE_POINTER )
        return FALSE;

    r = ReadFile( handle, Certificate, size, &count, NULL );
    if( !r )
        return FALSE;
    if( count != size )
        return FALSE;

    TRACE("OK\n");

    return TRUE;
}

/***********************************************************************
 *		ImageGetCertificateHeader (IMAGEHLP.@)
 */
BOOL IMAGEAPI ImageGetCertificateHeader(
    HANDLE FileHandle, DWORD CertificateIndex, LPWIN_CERTIFICATE Certificateheader)
{
    DWORD r, offset, ofs, size, count;
    const size_t cert_hdr_size = sizeof *Certificateheader -
                                 sizeof Certificateheader->bCertificate;

    DPRINT("%p %ld %p\n", FileHandle, CertificateIndex, Certificateheader);

    if( !IMAGEHLP_GetCertificateOffset( FileHandle, CertificateIndex, &ofs, &size ) )
        return FALSE;

    if( size < cert_hdr_size )
        return FALSE;

    offset = SetFilePointer( FileHandle, ofs, NULL, FILE_BEGIN );
    if( offset == INVALID_SET_FILE_POINTER )
        return FALSE;

    r = ReadFile( FileHandle, Certificateheader, (DWORD)cert_hdr_size, &count, NULL );
    if( !r )
        return FALSE;
    if( count != cert_hdr_size )
        return FALSE;

    TRACE("OK\n");

    return TRUE;
}

/***********************************************************************
 *		ImageGetDigestStream (IMAGEHLP.@)
 */
BOOL IMAGEAPI ImageGetDigestStream(
  HANDLE FileHandle, DWORD DigestLevel,
  DIGEST_FUNCTION DigestFunction, DIGEST_HANDLE DigestHandle)
{
  FIXME("(%p, %ld, %p, %p): stub\n",
    FileHandle, DigestLevel, DigestFunction, DigestHandle
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImageRemoveCertificate (IMAGEHLP.@)
 */
BOOL IMAGEAPI ImageRemoveCertificate(HANDLE FileHandle, DWORD Index)
{
  FIXME("(%p, %ld): stub\n", FileHandle, Index);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @implemented
 */
PIMAGE_NT_HEADERS
IMAGEAPI
CheckSumMappedFile(LPVOID BaseAddress,
                   DWORD FileLength,
                   LPDWORD HeaderSum,
                   LPDWORD CheckSum)
{
  PIMAGE_NT_HEADERS Header;
  DWORD CalcSum;
  DWORD HdrSum;
  DPRINT("stub\n");

  CalcSum = (DWORD)CalcCheckSum(0,
                BaseAddress,
                (FileLength + 1) / sizeof(WORD));

  Header = ImageNtHeader(BaseAddress);
  HdrSum = Header->OptionalHeader.CheckSum;

  /* Subtract image checksum from calculated checksum. */
  /* fix low word of checksum */
  if (LOWORD(CalcSum) >= LOWORD(HdrSum))
  {
    CalcSum -= LOWORD(HdrSum);
  }
  else
  {
    CalcSum = ((LOWORD(CalcSum) - LOWORD(HdrSum)) & 0xFFFF) - 1;
  }

   /* fix high word of checksum */
  if (LOWORD(CalcSum) >= HIWORD(HdrSum))
  {
    CalcSum -= HIWORD(HdrSum);
  }
  else
  {
    CalcSum = ((LOWORD(CalcSum) - HIWORD(HdrSum)) & 0xFFFF) - 1;
  }

  /* add file length */
  CalcSum += FileLength;

  *CheckSum = CalcSum;
  *HeaderSum = Header->OptionalHeader.CheckSum;

  return Header;
}
