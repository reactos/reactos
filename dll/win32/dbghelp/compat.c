#include "dbghelp_private.h"

void* __HeapAlloc(int heap, int flags, size_t size)
{
    void * ret = malloc(size);
    if(flags & HEAP_ZERO_MEMORY)
        memset(ret, 0, size);
    return ret;
}

void* __HeapReAlloc(int heap, DWORD d2, void *slab, SIZE_T newsize)
{
    return realloc(slab, newsize);
}

WCHAR* lstrcpynW(WCHAR* lpString1, const WCHAR* lpString2, int iMaxLength)
{
    LPWSTR d = lpString1;
    const WCHAR* s = lpString2;
    UINT count = iMaxLength;

    while ((count > 1) && *s)
    {
        count--;
        *d++ = *s++;
    }

    if (count)
        *d = 0;

    return lpString1;
}

PIMAGE_NT_HEADERS __RtlImageNtHeader(void *data)
{
    PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)data;
    PIMAGE_NT_HEADERS NtHeaders;
    PCHAR NtHeaderPtr;
    if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return NULL;
    NtHeaderPtr = ((PCHAR)data) + DosHeader->e_lfanew;
    NtHeaders = (PIMAGE_NT_HEADERS)NtHeaderPtr;
    if (NtHeaders->Signature != IMAGE_NT_SIGNATURE)
        return NULL;
    return NtHeaders;
}

PIMAGE_SECTION_HEADER
__RtlImageRvaToSection(
    const IMAGE_NT_HEADERS* NtHeader,
    PVOID BaseAddress,
    ULONG Rva)
{
    PIMAGE_SECTION_HEADER Section;
    ULONG Va;
    ULONG Count;

    Count = SWAPW(NtHeader->FileHeader.NumberOfSections);
    Section = IMAGE_FIRST_SECTION(NtHeader);

    while (Count--)
    {
        Va = SWAPD(Section->VirtualAddress);
        if ((Va <= Rva) && (Rva < Va + SWAPD(Section->SizeOfRawData)))
            return Section;
        Section++;
    }

    return NULL;
}

PVOID
__RtlImageRvaToVa
(const IMAGE_NT_HEADERS* NtHeader,
 PVOID BaseAddress,
 ULONG Rva,
 PIMAGE_SECTION_HEADER *SectionHeader)
{
    PIMAGE_SECTION_HEADER Section = NULL;

    if (SectionHeader)
        Section = *SectionHeader;

    if ((Section == NULL) ||
        (Rva < SWAPD(Section->VirtualAddress)) ||
        (Rva >= SWAPD(Section->VirtualAddress) + SWAPD(Section->SizeOfRawData)))
    {
        Section = RtlImageRvaToSection(NtHeader, BaseAddress, Rva);
        if (Section == NULL)
            return NULL;

        if (SectionHeader)
            *SectionHeader = Section;
    }

    return (PVOID)((ULONG_PTR)BaseAddress + Rva +
                   (ULONG_PTR)SWAPD(Section->PointerToRawData) -
                   (ULONG_PTR)SWAPD(Section->VirtualAddress));
}

PVOID
__RtlImageDirectoryEntryToData(
    PVOID BaseAddress,
    BOOLEAN MappedAsImage,
    USHORT Directory,
    PULONG Size)
{
    PIMAGE_NT_HEADERS NtHeader;
    ULONG Va;

    /* Magic flag for non-mapped images. */
    if ((ULONG_PTR)BaseAddress & 1)
    {
        BaseAddress = (PVOID)((ULONG_PTR)BaseAddress & ~1);
        MappedAsImage = FALSE;
    }

    NtHeader = RtlImageNtHeader(BaseAddress);
    if (NtHeader == NULL)
        return NULL;

    if (Directory >= SWAPD(NtHeader->OptionalHeader.NumberOfRvaAndSizes))
        return NULL;

    Va = SWAPD(NtHeader->OptionalHeader.DataDirectory[Directory].VirtualAddress);
    if (Va == 0)
        return NULL;

    *Size = SWAPD(NtHeader->OptionalHeader.DataDirectory[Directory].Size);

    if (MappedAsImage || Va < SWAPD(NtHeader->OptionalHeader.SizeOfHeaders))
        return (PVOID)((ULONG_PTR)BaseAddress + Va);

    /* Image mapped as ordinary file, we must find raw pointer */
    return RtlImageRvaToVa(NtHeader, BaseAddress, Va, NULL);
}

BOOL __GetFileSizeEx(HANDLE file, PLARGE_INTEGER fsize)
{
    if (fseek((FILE*)file, 0, 2) == -1)
        return FALSE;
    fsize->QuadPart = ftell((FILE*)file);
    return TRUE;
}

BOOL __CloseHandle(HANDLE handle)
{
    fclose(handle);
    return TRUE;
}

HANDLE __CreateFileW(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    char buf[MAX_PATH];
    HANDLE res;

    WideCharToMultiByte(CP_ACP, 0, lpFileName, -1, buf, MAX_PATH, NULL, NULL);
    res = CreateFileA(buf, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    return res;
}

BOOL __ReadFile(HANDLE file, PVOID buffer, DWORD len, PDWORD outlen, PVOID overlap)
{
    size_t read;

    assert(overlap == NULL);

    read = fread(buffer, 1, len, file);

    if (ferror(file) != 0)
        return FALSE;

    *outlen = (DWORD)read;
    return TRUE;
}

DWORD __SetFilePointer(HANDLE file,LONG low, PLONG high, DWORD move)
{
    assert(move == FILE_BEGIN);
    assert(high == NULL);

    if (fseek(file, low, SEEK_SET))
        return INVALID_SET_FILE_POINTER;
    return low;
}

void* __MapViewOfFile(HANDLE file,DWORD d1,DWORD d2,DWORD d3,SIZE_T s)
{
    FILE *f = (FILE*)file;
    LARGE_INTEGER size;
    char *result;

    if (file == INVALID_HANDLE_VALUE)
        return NULL;

    if (!GetFileSizeEx(file, &size))
        return NULL;

    if (fseek(f, 0, 0) == -1)
        return NULL;

    result = malloc(size.LowPart);
    if (fread(result, 1, size.LowPart, f) != size.LowPart)
    {
        free(result);
        return NULL;
    }

    return result;
}

BOOL __UnmapViewOfFile(const void* data)
{
    free((void *)data);
    return TRUE;
}

LPSTR __lstrcpynA(LPSTR d,LPCSTR s,int c)
{
    LPSTR r = d;
    while(*s && c)
    {
        *d++ = *s++;
        c--;
    }
    return r;
}

/* From Wine implementation over their unicode library */
INT
__WideCharToMultiByte(UINT page, DWORD flags, LPCWSTR src, INT srclen,
                                LPSTR dst, INT dstlen, LPCSTR defchar, BOOL *used )
{
    int i;

    if (!src || !srclen || (!dst && dstlen))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (srclen < 0) srclen = strlenW(src) + 1;

    if(!dstlen)
        return srclen;

    for(i=0; i<srclen && i<dstlen; i++)
        dst[i] = src[i] & 0xFF;

    if (used) *used = FALSE;

    return i;
}

INT
__MultiByteToWideChar(UINT page, DWORD flags, LPCSTR src, INT srclen,
                                LPWSTR dst, INT dstlen )
{
    int i;

    if (!src || !srclen || (!dst && dstlen))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (srclen < 0) srclen = strlen(src) + 1;

    if(!dstlen)
        return srclen;

    for(i=0; i<srclen && i<dstlen; i++)
        dst[i] = src[i];

    return i;
}

/* In our case, the provided file path is the one we are looking for */
HANDLE __FindExecutableImageExW(PCWSTR file, PCWSTR path, PWSTR out_buffer, PFIND_EXE_FILE_CALLBACKW x, PVOID y)
{
    HANDLE ret = CreateFileW(file, 0, 0, NULL, 0, 0, NULL);
    if(ret)
        memcpy(out_buffer, file, (strlenW(file) + 1)*sizeof(WCHAR));

    return ret;
}

/* printf with temp buffer allocation */
const char *wine_dbg_sprintf( const char *format, ... )
{
    static const int max_size = 200;
    static char buffer[256];
    char *ret;
    int len;
    va_list valist;

    va_start(valist, format);
    ret = buffer;
    len = vsnprintf( ret, max_size, format, valist );
    if (len == -1 || len >= max_size) ret[max_size-1] = 0;
    va_end(valist);
    return ret;
}

/* default implementation of wine_dbgstr_an */
const char *wine_dbgstr_an( const char *str, int n )
{
    static const char hex[16] = "0123456789abcdef";
    char *dst, *res;
    size_t size;
    static char buffer[256];

    if (!((ULONG_PTR)str >> 16))
    {
        if (!str) return "(null)";
        res = buffer;
        sprintf( res, "#%04x", LOWORD(str) );
        return res;
    }
    if (n == -1) n = strlen(str);
    if (n < 0) n = 0;
    size = 10 + min( 300, n * 4 );
    dst = res = buffer;
    *dst++ = '"';
    while (n-- > 0 && dst <= res + size - 9)
    {
        unsigned char c = *str++;
        switch (c)
        {
        case '\n': *dst++ = '\\'; *dst++ = 'n'; break;
        case '\r': *dst++ = '\\'; *dst++ = 'r'; break;
        case '\t': *dst++ = '\\'; *dst++ = 't'; break;
        case '"':  *dst++ = '\\'; *dst++ = '"'; break;
        case '\\': *dst++ = '\\'; *dst++ = '\\'; break;
        default:
            if (c >= ' ' && c <= 126)
                *dst++ = c;
            else
            {
                *dst++ = '\\';
                *dst++ = 'x';
                *dst++ = hex[(c >> 4) & 0x0f];
                *dst++ = hex[c & 0x0f];
            }
        }
    }
    *dst++ = '"';
    if (n > 0)
    {
        *dst++ = '.';
        *dst++ = '.';
        *dst++ = '.';
    }
    *dst++ = 0;
    return res;
}


/* default implementation of wine_dbgstr_wn */
const char *wine_dbgstr_wn( const WCHAR *str, int n )
{
    char *dst, *res;
    size_t size;
    static char buffer[256];

    if (!((ULONG_PTR)str >> 16))
    {
        if (!str) return "(null)";
        res = buffer;
        sprintf( res, "#%04x", LOWORD(str) );
        return res;
    }
    if (n == -1)
    {
        const WCHAR *end = str;
        while (*end) end++;
        n = end - str;
    }
    if (n < 0) n = 0;
    size = 12 + min( 300, n * 5 );
    dst = res = buffer;
    *dst++ = 'L';
    *dst++ = '"';
    while (n-- > 0 && dst <= res + size - 10)
    {
        WCHAR c = *str++;
        switch (c)
        {
        case '\n': *dst++ = '\\'; *dst++ = 'n'; break;
        case '\r': *dst++ = '\\'; *dst++ = 'r'; break;
        case '\t': *dst++ = '\\'; *dst++ = 't'; break;
        case '"':  *dst++ = '\\'; *dst++ = '"'; break;
        case '\\': *dst++ = '\\'; *dst++ = '\\'; break;
        default:
            if (c >= ' ' && c <= 126)
                *dst++ = c;
            else
            {
                *dst++ = '\\';
                sprintf(dst,"%04x",c);
                dst+=4;
            }
        }
    }
    *dst++ = '"';
    if (n > 0)
    {
        *dst++ = '.';
        *dst++ = '.';
        *dst++ = '.';
    }
    *dst++ = 0;
    return res;
}

BOOL __IsWow64Process(HANDLE Process, BOOL* is_wow64)
{
	*is_wow64 = FALSE;
	return TRUE;
}

/* from sdk/lib/rtl/crc2.c */
/* This work is based off of rtl.c in Wine.
 * Please give credit where credit is due:
 *
 * Copyright 2003      Thomas Mertes
 * Crc32 code Copyright 1986 Gary S. Brown (Public domain)
 */

/* CRC polynomial 0xedb88320 */
static const ULONG CrcTable[256] =
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

/*********************************************************************
 *                  RtlComputeCrc32   [NTDLL.@]
 *
 * Calculate the CRC32 checksum of a block of bytes
 *
 * PARAMS
 *  Initial [I] Initial CRC value
 *  Data    [I] Data block
 *  Length  [I] Length of the byte block
 *
 * RETURNS
 *  The cumulative CRC32 of Initial and Length bytes of the Data block.
 *
 * @implemented
 */
ULONG
__RtlComputeCrc32(ULONG Initial, PUCHAR Data, ULONG Length)
{
  ULONG CrcValue = ~Initial;

  while (Length > 0)
  {
    CrcValue = CrcTable[(CrcValue ^ *Data) & 0xff] ^ (CrcValue >> 8);
    Data++;
    Length--;
  }
  return ~CrcValue;
}
