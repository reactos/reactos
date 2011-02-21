/*
 *	IMAGEHLP library
 *
 *	Copyright 1998	Patrik Stridvall
 *	Copyright 2003	Mike McCormack
 *	Copyright 2009  Owen Rudge for CodeWeavers
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winternl.h"
#include "winnt.h"
#include "imagehlp.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(imagehlp);

/*
 * These functions are partially documented at:
 *   http://www.cs.auckland.ac.nz/~pgut001/pubs/authenticode.txt
 */

#define HDR_FAIL   -1
#define HDR_NT32    0
#define HDR_NT64    1

/***********************************************************************
 * IMAGEHLP_GetNTHeaders (INTERNAL)
 *
 * Return the IMAGE_NT_HEADERS for a PE file, after validating magic
 * numbers and distinguishing between 32-bit and 64-bit files.
 */
static int IMAGEHLP_GetNTHeaders(HANDLE handle, DWORD *pe_offset, IMAGE_NT_HEADERS32 *nt32, IMAGE_NT_HEADERS64 *nt64)
{
    IMAGE_DOS_HEADER dos_hdr;
    DWORD count;
    BOOL r;

    TRACE("handle %p\n", handle);

    if ((!nt32) || (!nt64))
        return HDR_FAIL;

    /* read the DOS header */
    count = SetFilePointer(handle, 0, NULL, FILE_BEGIN);

    if (count == INVALID_SET_FILE_POINTER)
        return HDR_FAIL;

    count = 0;

    r = ReadFile(handle, &dos_hdr, sizeof dos_hdr, &count, NULL);

    if (!r)
        return HDR_FAIL;

    if (count != sizeof dos_hdr)
        return HDR_FAIL;

    /* verify magic number of 'MZ' */
    if (dos_hdr.e_magic != 0x5A4D)
        return HDR_FAIL;

    if (pe_offset != NULL)
        *pe_offset = dos_hdr.e_lfanew;

    /* read the PE header */
    count = SetFilePointer(handle, dos_hdr.e_lfanew, NULL, FILE_BEGIN);

    if (count == INVALID_SET_FILE_POINTER)
        return HDR_FAIL;

    count = 0;

    r = ReadFile(handle, nt32, sizeof(IMAGE_NT_HEADERS32), &count, NULL);

    if (!r)
        return HDR_FAIL;

    if (count != sizeof(IMAGE_NT_HEADERS32))
        return HDR_FAIL;

    /* verify NT signature */
    if (nt32->Signature != IMAGE_NT_SIGNATURE)
        return HDR_FAIL;

    /* check if we have a 32-bit or 64-bit executable */
    switch (nt32->OptionalHeader.Magic)
    {
        case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
            return HDR_NT32;

        case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
            /* Re-read as 64-bit */

            count = SetFilePointer(handle, dos_hdr.e_lfanew, NULL, FILE_BEGIN);

            if (count == INVALID_SET_FILE_POINTER)
                return HDR_FAIL;

            count = 0;

            r = ReadFile(handle, nt64, sizeof(IMAGE_NT_HEADERS64), &count, NULL);

            if (!r)
                return HDR_FAIL;

            if (count != sizeof(IMAGE_NT_HEADERS64))
                return HDR_FAIL;

            /* verify NT signature */
            if (nt64->Signature != IMAGE_NT_SIGNATURE)
                return HDR_FAIL;

            return HDR_NT64;
    }

    return HDR_FAIL;
}

/***********************************************************************
 * IMAGEHLP_GetSecurityDirOffset (INTERNAL)
 *
 * Read a file's PE header, and return the offset and size of the
 *  security directory.
 */
static BOOL IMAGEHLP_GetSecurityDirOffset( HANDLE handle,
                                           DWORD *pdwOfs, DWORD *pdwSize )
{
    IMAGE_NT_HEADERS32 nt_hdr32;
    IMAGE_NT_HEADERS64 nt_hdr64;
    IMAGE_DATA_DIRECTORY *sd;
    int ret;

    ret = IMAGEHLP_GetNTHeaders(handle, NULL, &nt_hdr32, &nt_hdr64);

    if (ret == HDR_NT32)
        sd = &nt_hdr32.OptionalHeader.DataDirectory[IMAGE_FILE_SECURITY_DIRECTORY];
    else if (ret == HDR_NT64)
        sd = &nt_hdr64.OptionalHeader.DataDirectory[IMAGE_FILE_SECURITY_DIRECTORY];
    else
        return FALSE;

    TRACE("ret = %d size = %x addr = %x\n", ret, sd->Size, sd->VirtualAddress);

    *pdwSize = sd->Size;
    *pdwOfs = sd->VirtualAddress;

    return TRUE;
}

/***********************************************************************
 * IMAGEHLP_SetSecurityDirOffset (INTERNAL)
 *
 * Read a file's PE header, and update the offset and size of the
 *  security directory.
 */
static BOOL IMAGEHLP_SetSecurityDirOffset(HANDLE handle,
                                          DWORD dwOfs, DWORD dwSize)
{
    IMAGE_NT_HEADERS32 nt_hdr32;
    IMAGE_NT_HEADERS64 nt_hdr64;
    IMAGE_DATA_DIRECTORY *sd;
    int ret, nt_hdr_size = 0;
    DWORD pe_offset;
    void *nt_hdr;
    DWORD count;
    BOOL r;

    ret = IMAGEHLP_GetNTHeaders(handle, &pe_offset, &nt_hdr32, &nt_hdr64);

    if (ret == HDR_NT32)
    {
        sd = &nt_hdr32.OptionalHeader.DataDirectory[IMAGE_FILE_SECURITY_DIRECTORY];

        nt_hdr = &nt_hdr32;
        nt_hdr_size = sizeof(IMAGE_NT_HEADERS32);
    }
    else if (ret == HDR_NT64)
    {
        sd = &nt_hdr64.OptionalHeader.DataDirectory[IMAGE_FILE_SECURITY_DIRECTORY];

        nt_hdr = &nt_hdr64;
        nt_hdr_size = sizeof(IMAGE_NT_HEADERS64);
    }
    else
        return FALSE;

    sd->Size = dwSize;
    sd->VirtualAddress = dwOfs;

    TRACE("size = %x addr = %x\n", sd->Size, sd->VirtualAddress);

    /* write the header back again */
    count = SetFilePointer(handle, pe_offset, NULL, FILE_BEGIN);

    if (count == INVALID_SET_FILE_POINTER)
        return FALSE;

    count = 0;

    r = WriteFile(handle, nt_hdr, nt_hdr_size, &count, NULL);

    if (!r)
        return FALSE;

    if (count != nt_hdr_size)
        return FALSE;

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

        /* padded out to the nearest 8-byte boundary */
        if( len % 8 )
            offset += 8 - (len % 8);

        if( offset >= size )
            return FALSE;
    }

    *pdwOfs = sd_VirtualAddr + offset;
    *pdwSize = len;

    TRACE("len = %x addr = %x\n", len, sd_VirtualAddr + offset);

    return TRUE;
}

/***********************************************************************
 * IMAGEHLP_RecalculateChecksum (INTERNAL)
 *
 * Update the NT header checksum for the specified file.
 */
static BOOL IMAGEHLP_RecalculateChecksum(HANDLE handle)
{
    DWORD FileLength, count, HeaderSum, pe_offset, nt_hdr_size;
    IMAGE_NT_HEADERS32 nt_hdr32;
    IMAGE_NT_HEADERS64 nt_hdr64;
    LPVOID BaseAddress;
    HANDLE hMapping;
    DWORD *CheckSum;
    void *nt_hdr;
    int ret;
    BOOL r;

    TRACE("handle %p\n", handle);

    ret = IMAGEHLP_GetNTHeaders(handle, &pe_offset, &nt_hdr32, &nt_hdr64);

    if (ret == HDR_NT32)
    {
        CheckSum = &nt_hdr32.OptionalHeader.CheckSum;

        nt_hdr = &nt_hdr32;
        nt_hdr_size = sizeof(IMAGE_NT_HEADERS32);
    }
    else if (ret == HDR_NT64)
    {
        CheckSum = &nt_hdr64.OptionalHeader.CheckSum;

        nt_hdr = &nt_hdr64;
        nt_hdr_size = sizeof(IMAGE_NT_HEADERS64);
    }
    else
        return FALSE;

    hMapping = CreateFileMappingW(handle, NULL, PAGE_READONLY, 0, 0, NULL);

    if (!hMapping)
        return FALSE;

    BaseAddress = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);

    if (!BaseAddress)
    {
        CloseHandle(hMapping);
        return FALSE;
    }

    FileLength = GetFileSize(handle, NULL);

    *CheckSum = 0;
    CheckSumMappedFile(BaseAddress, FileLength, &HeaderSum, CheckSum);

    UnmapViewOfFile(BaseAddress);
    CloseHandle(hMapping);

    if (*CheckSum)
    {
        /* write the header back again */
        count = SetFilePointer(handle, pe_offset, NULL, FILE_BEGIN);

        if (count == INVALID_SET_FILE_POINTER)
            return FALSE;

        count = 0;

        r = WriteFile(handle, nt_hdr, nt_hdr_size, &count, NULL);

        if (!r)
            return FALSE;

        if (count != nt_hdr_size)
            return FALSE;

        return TRUE;
    }

    return FALSE;
}

/***********************************************************************
 *		ImageAddCertificate (IMAGEHLP.@)
 *
 * Adds the specified certificate to the security directory of
 * open PE file.
 */

BOOL WINAPI ImageAddCertificate(
  HANDLE FileHandle, LPWIN_CERTIFICATE Certificate, PDWORD Index)
{
    DWORD size = 0, count = 0, offset = 0, sd_VirtualAddr = 0, index = 0;
    WIN_CERTIFICATE hdr;
    const size_t cert_hdr_size = sizeof hdr - sizeof hdr.bCertificate;
    BOOL r;

    TRACE("(%p, %p, %p)\n", FileHandle, Certificate, Index);

    r = IMAGEHLP_GetSecurityDirOffset(FileHandle, &sd_VirtualAddr, &size);

    /* If we've already got a security directory, find the end of it */
    if ((r) && (sd_VirtualAddr != 0))
    {
        offset = 0;
        index = 0;
        count = 0;

        /* Check if the security directory is at the end of the file.
           If not, we should probably relocate it. */
        if (GetFileSize(FileHandle, NULL) != sd_VirtualAddr + size)
        {
            FIXME("Security directory already present but not located at EOF, not adding certificate\n");

            SetLastError(ERROR_NOT_SUPPORTED);
            return FALSE;
        }

        while (offset < size)
        {
            /* read the length of the current certificate */
            count = SetFilePointer (FileHandle, sd_VirtualAddr + offset,
                                     NULL, FILE_BEGIN);

            if (count == INVALID_SET_FILE_POINTER)
                return FALSE;

            r = ReadFile(FileHandle, &hdr, cert_hdr_size, &count, NULL);

            if (!r)
                return FALSE;

            if (count != cert_hdr_size)
                return FALSE;

            /* check the certificate is not too big or too small */
            if (hdr.dwLength < cert_hdr_size)
                return FALSE;

            if (hdr.dwLength > (size-offset))
                return FALSE;

            /* next certificate */
            offset += hdr.dwLength;

            /* padded out to the nearest 8-byte boundary */
            if (hdr.dwLength % 8)
                offset += 8 - (hdr.dwLength % 8);

            index++;
        }

        count = SetFilePointer (FileHandle, sd_VirtualAddr + offset, NULL, FILE_BEGIN);

        if (count == INVALID_SET_FILE_POINTER)
            return FALSE;
    }
    else
    {
        sd_VirtualAddr = SetFilePointer(FileHandle, 0, NULL, FILE_END);

        if (sd_VirtualAddr == INVALID_SET_FILE_POINTER)
            return FALSE;
    }

    /* Write the certificate to the file */
    r = WriteFile(FileHandle, Certificate, Certificate->dwLength, &count, NULL);

    if (!r)
        return FALSE;

    /* Pad out if necessary */
    if (Certificate->dwLength % 8)
    {
        char null[8];

        ZeroMemory(null, 8);
        WriteFile(FileHandle, null, 8 - (Certificate->dwLength % 8), NULL, NULL);

        size += 8 - (Certificate->dwLength % 8);
    }

    size += Certificate->dwLength;

    /* Update the security directory offset and size */
    if (!IMAGEHLP_SetSecurityDirOffset(FileHandle, sd_VirtualAddr, size))
        return FALSE;

    if (!IMAGEHLP_RecalculateChecksum(FileHandle))
        return FALSE;

    return TRUE;
}

/***********************************************************************
 *		ImageEnumerateCertificates (IMAGEHLP.@)
 */
BOOL WINAPI ImageEnumerateCertificates(
    HANDLE handle, WORD TypeFilter, PDWORD CertificateCount,
    PDWORD Indices, DWORD IndexCount)
{
    DWORD size, count, offset, sd_VirtualAddr, index;
    WIN_CERTIFICATE hdr;
    const size_t cert_hdr_size = sizeof hdr - sizeof hdr.bCertificate;
    BOOL r;

    TRACE("%p %hd %p %p %d\n",
           handle, TypeFilter, CertificateCount, Indices, IndexCount);

    r = IMAGEHLP_GetSecurityDirOffset( handle, &sd_VirtualAddr, &size );
    if( !r )
        return FALSE;

    offset = 0;
    index = 0;
    *CertificateCount = 0;
    while( offset < size )
    {
        /* read the length of the current certificate */
        count = SetFilePointer( handle, sd_VirtualAddr + offset,
                                 NULL, FILE_BEGIN );
        if( count == INVALID_SET_FILE_POINTER )
            return FALSE;
        r = ReadFile( handle, &hdr, cert_hdr_size, &count, NULL );
        if( !r )
            return FALSE;
        if( count != cert_hdr_size )
            return FALSE;

        TRACE("Size = %08x  id = %08hx\n",
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
            if(Indices && *CertificateCount <= IndexCount)
                *Indices++ = index;
        }

        /* next certificate */
        offset += hdr.dwLength;

        /* padded out to the nearest 8-byte boundary */
        if (hdr.dwLength % 8)
            offset += 8 - (hdr.dwLength % 8);

        index++;
    }

    return TRUE;
}

/***********************************************************************
 *		ImageGetCertificateData (IMAGEHLP.@)
 *
 *  FIXME: not sure that I'm dealing with the Index the right way
 */
BOOL WINAPI ImageGetCertificateData(
                HANDLE handle, DWORD Index,
                LPWIN_CERTIFICATE Certificate, PDWORD RequiredLength)
{
    DWORD r, offset, ofs, size, count;

    TRACE("%p %d %p %p\n", handle, Index, Certificate, RequiredLength);

    if( !RequiredLength)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if( !IMAGEHLP_GetCertificateOffset( handle, Index, &ofs, &size ) )
        return FALSE;

    if( *RequiredLength < size )
    {
        *RequiredLength = size;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    if( !Certificate )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
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
    SetLastError( NO_ERROR );

    return TRUE;
}

/***********************************************************************
 *		ImageGetCertificateHeader (IMAGEHLP.@)
 */
BOOL WINAPI ImageGetCertificateHeader(
    HANDLE handle, DWORD index, LPWIN_CERTIFICATE pCert)
{
    DWORD r, offset, ofs, size, count;
    const size_t cert_hdr_size = sizeof *pCert - sizeof pCert->bCertificate;

    TRACE("%p %d %p\n", handle, index, pCert);

    if( !IMAGEHLP_GetCertificateOffset( handle, index, &ofs, &size ) )
        return FALSE;

    if( size < cert_hdr_size )
        return FALSE;

    offset = SetFilePointer( handle, ofs, NULL, FILE_BEGIN );
    if( offset == INVALID_SET_FILE_POINTER )
        return FALSE;

    r = ReadFile( handle, pCert, cert_hdr_size, &count, NULL );
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
BOOL WINAPI ImageGetDigestStream(
  HANDLE FileHandle, DWORD DigestLevel,
  DIGEST_FUNCTION DigestFunction, DIGEST_HANDLE DigestHandle)
{
  FIXME("(%p, %d, %p, %p): stub\n",
    FileHandle, DigestLevel, DigestFunction, DigestHandle
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImageRemoveCertificate (IMAGEHLP.@)
 */
BOOL WINAPI ImageRemoveCertificate(HANDLE FileHandle, DWORD Index)
{
    DWORD size = 0, count = 0, sd_VirtualAddr = 0, offset = 0;
    DWORD data_size = 0, cert_size = 0, cert_size_padded = 0, ret = 0;
    LPVOID cert_data;
    BOOL r;

    TRACE("(%p, %d)\n", FileHandle, Index);

    r = ImageEnumerateCertificates(FileHandle, CERT_SECTION_TYPE_ANY, &count, NULL, 0);

    if ((!r) || (count == 0))
        return FALSE;

    if ((!IMAGEHLP_GetSecurityDirOffset(FileHandle, &sd_VirtualAddr, &size)) ||
        (!IMAGEHLP_GetCertificateOffset(FileHandle, Index, &offset, &cert_size)))
        return FALSE;

    /* Ignore any padding we have, too */
    if (cert_size % 8)
        cert_size_padded = cert_size + (8 - (cert_size % 8));
    else
        cert_size_padded = cert_size;

    data_size = size - (offset - sd_VirtualAddr) - cert_size_padded;

    if (data_size == 0)
    {
        ret = SetFilePointer(FileHandle, sd_VirtualAddr, NULL, FILE_BEGIN);

        if (ret == INVALID_SET_FILE_POINTER)
            return FALSE;
    }
    else
    {
        cert_data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, data_size);

        if (!cert_data)
            return FALSE;

        ret = SetFilePointer(FileHandle, offset + cert_size_padded, NULL, FILE_BEGIN);

        if (ret == INVALID_SET_FILE_POINTER)
            goto error;

        /* Read any subsequent certificates */
        r = ReadFile(FileHandle, cert_data, data_size, &count, NULL);

        if ((!r) || (count != data_size))
            goto error;

        SetFilePointer(FileHandle, offset, NULL, FILE_BEGIN);

        /* Write them one index back */
        r = WriteFile(FileHandle, cert_data, data_size, &count, NULL);

        if ((!r) || (count != data_size))
            goto error;

        HeapFree(GetProcessHeap(), 0, cert_data);
    }

    /* If security directory is at end of file, trim the file */
    if (GetFileSize(FileHandle, NULL) == sd_VirtualAddr + size)
        SetEndOfFile(FileHandle);

    if (count == 1)
        r = IMAGEHLP_SetSecurityDirOffset(FileHandle, 0, 0);
    else
        r = IMAGEHLP_SetSecurityDirOffset(FileHandle, sd_VirtualAddr, size - cert_size_padded);

    if (!r)
        return FALSE;

    if (!IMAGEHLP_RecalculateChecksum(FileHandle))
        return FALSE;

    return TRUE;

error:
    HeapFree(GetProcessHeap(), 0, cert_data);
    return FALSE;
}
