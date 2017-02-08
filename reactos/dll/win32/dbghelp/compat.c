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
