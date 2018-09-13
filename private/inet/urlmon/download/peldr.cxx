//  PELDR.CXX kernel32
//
//      (C) Copyright Microsoft Corp., 1988-1994
//
//      Swiped without thanks from the Win32 loader
//

#include <cdlpch.h>
#include "pefile.h"

extern int g_CPUType;
extern BOOL g_bOmniPresent;

BOOL WINAPI HasDllRegisterServer(LPVOID lpFile);
DWORD WINAPI ImageFileType(LPVOID lpFile);

HRESULT
GetMachineTypeOfFile(const char *szName, LPDWORD pdwMachine)
{
    IMAGE_DOS_HEADER idh;
    IMAGE_NT_HEADERS nth;
    DWORD       cbT;
    DWORD       size;
    DWORD dwBytesRead = 0;
    HANDLE dfhFile = INVALID_HANDLE_VALUE;
    HRESULT hr = S_OK;

    *pdwMachine = IMAGE_FILE_MACHINE_UNKNOWN;

    if ( (dfhFile = CreateFile(szName, GENERIC_READ, FILE_SHARE_READ,
                    NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, 0)) == INVALID_HANDLE_VALUE) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // Read DOS header
    if ((!ReadFile(dfhFile, &idh, sizeof(idh), &dwBytesRead, NULL)) ||
        (idh.e_magic != 0x5a4d)) {

        // not PE file!
        hr = HRESULT_FROM_WIN32(GetLastError());

        if (SUCCEEDED(hr)) {
            // not enough bytes read
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_EXE_SIGNATURE);
        }
        goto Exit;
    }

    // Read PE header
    SetFilePointer (dfhFile, idh.e_lfanew, NULL, FILE_BEGIN);


    if ((!ReadFile(dfhFile, &nth, sizeof(IMAGE_NT_HEADERS), &dwBytesRead, NULL))) {
        hr = HRESULT_FROM_WIN32(GetLastError());

        if (SUCCEEDED(hr)) {
            // not enough bytes read
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_EXE_SIGNATURE);
        }

        goto Exit;
    }

    cbT = dwBytesRead;

    // Valid PE header?
    if ((cbT != sizeof(IMAGE_NT_HEADERS)) || (nth.Signature != 0x00004550)) {

        // not PE file!
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_EXE_SIGNATURE);
        goto Exit;
    }

    *pdwMachine = nth.FileHeader.Machine;

Exit:

    if (dfhFile != INVALID_HANDLE_VALUE)
        CloseHandle(dfhFile);

    return hr ;

}

HRESULT
IsCompatibleType(DWORD dwBinaryType)
{
    int CPUType = PROCESSOR_ARCHITECTURE_UNKNOWN;

    switch (dwBinaryType) {
        
    case IMAGE_FILE_MACHINE_OMNI:

        // OMNI can run on all platforms provide we have the omni VM
        if (g_bOmniPresent)
            return S_OK;
        break;

    case IMAGE_FILE_MACHINE_ALPHA:
        
        CPUType = PROCESSOR_ARCHITECTURE_ALPHA;
        break;

    case IMAGE_FILE_MACHINE_POWERPC:
        
        CPUType = PROCESSOR_ARCHITECTURE_PPC;
        break;

    case IMAGE_FILE_MACHINE_R3000:
    case IMAGE_FILE_MACHINE_R4000:
    case IMAGE_FILE_MACHINE_R10000:
        
        CPUType = PROCESSOR_ARCHITECTURE_MIPS;
        break;

    case IMAGE_FILE_MACHINE_I386:
        
#ifdef WX86
        if (g_fWx86Present) {
            // Wx86 is installed - I386 images are OK.
            return S_OK;
        }
#endif
        CPUType = PROCESSOR_ARCHITECTURE_INTEL;
        break;

    }

    HRESULT hr = (g_CPUType == CPUType)?S_OK:HRESULT_FROM_WIN32(ERROR_EXE_MACHINE_TYPE_MISMATCH);

    return hr;
}

// IsCompatibleFile(const char *szFileName, LPDWORD lpdwMachineType=NULL);
// returns:
//      S_OK: file is compatible install it and LoadLibrary it
//      S_FALSE: file is not a PE
//      ERROR_EXE_MACHINE_TYPE_MISMATCH: not compatible
HRESULT
IsCompatibleFile(const char *szFileName, LPDWORD lpdwMachineType)
{
    DWORD dwMachine = 0;

    HRESULT hr = GetMachineTypeOfFile(szFileName, &dwMachine);

    if (SUCCEEDED(hr)) {

        hr = IsCompatibleType(dwMachine);

    } else {

        hr = S_FALSE;
    }

    if (lpdwMachineType)
        *lpdwMachineType = dwMachine;

    return hr;

}

// IsRegisterableDLL(const char *szFileName)
// returns:
//      S_OK: file is registerable, LoadLibrary and call GetProcAddress
//      S_FALSE: file is not registerable
HRESULT 
IsRegisterableDLL(const char *szFileName)
{

    HANDLE dfhFile = INVALID_HANDLE_VALUE;
    HANDLE hMapping = INVALID_HANDLE_VALUE;
    LPVOID lpFile = NULL;
    HRESULT hr = S_OK;

    if ( (dfhFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ,
                    NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, 0)) == INVALID_HANDLE_VALUE) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }


    if ( (hMapping = CreateFileMapping( dfhFile, NULL, PAGE_READONLY, 0,0,NULL)) == NULL ) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    if ( !(lpFile = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0,0))) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // Make sure this a we know what kind of file this is

    if (!ImageFileType(lpFile)) {
        // Don't know what type of image this is! Bail out!
        hr = E_UNEXPECTED;
        goto Exit;
    }

    if (!HasDllRegisterServer(lpFile))
        hr = S_FALSE;

Exit:

    if (lpFile)
        UnmapViewOfFile(lpFile);

    if (dfhFile != INVALID_HANDLE_VALUE)
        CloseHandle(dfhFile);

    if (hMapping != INVALID_HANDLE_VALUE)
        CloseHandle(hMapping);

    return hr;
}

/* return file signature */
DWORD  WINAPI ImageFileType (
    LPVOID    lpFile)
{
    /* dos file signature comes first */
    if (*(USHORT *)lpFile == IMAGE_DOS_SIGNATURE)
    {
    /* determine location of PE File header from dos header */
    if (LOWORD (*(DWORD *)NTSIGNATURE (lpFile)) == IMAGE_OS2_SIGNATURE ||
        LOWORD (*(DWORD *)NTSIGNATURE (lpFile)) == IMAGE_OS2_SIGNATURE_LE)
        return (DWORD)LOWORD(*(DWORD *)NTSIGNATURE (lpFile));

    else if (*(DWORD *)NTSIGNATURE (lpFile) == IMAGE_NT_SIGNATURE)
        return IMAGE_NT_SIGNATURE;

    else
        return IMAGE_DOS_SIGNATURE;
    }

    else
    /* unknown file type */
    return 0;
}

/* return the total number of sections in the module */
int   WINAPI NumOfSections (
    LPVOID    lpFile)
{
    /* number os sections is indicated in file header */
    return ((int)((PIMAGE_FILE_HEADER)PEFHDROFFSET (lpFile))->NumberOfSections);
}


/* return offset to specified IMAGE_DIRECTORY entry */
LPVOID  WINAPI ImageDirectoryOffset (
    LPVOID    lpFile,
    DWORD     dwIMAGE_DIRECTORY)
{
    PIMAGE_OPTIONAL_HEADER   poh = (PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (lpFile);
    PIMAGE_SECTION_HEADER    psh = (PIMAGE_SECTION_HEADER)SECHDROFFSET (lpFile);
    int              nSections = NumOfSections (lpFile);
    int              i = 0;
    LPVOID           VAImageDir;

    /* must be 0 thru (NumberOfRvaAndSizes-1) */
    if (dwIMAGE_DIRECTORY >= poh->NumberOfRvaAndSizes)
    return NULL;

    /* locate specific image directory's relative virtual address */
    VAImageDir = (LPVOID)poh->DataDirectory[dwIMAGE_DIRECTORY].VirtualAddress;

    /* locate section containing image directory */
    while (i++<nSections)
    {
    if (psh->VirtualAddress <= (DWORD_PTR) VAImageDir &&
        psh->VirtualAddress + psh->SizeOfRawData > (DWORD_PTR) VAImageDir)
        break;
    psh++;
    }

    if (i > nSections)
    return NULL;

    /* return image import directory offset */
    return (LPVOID)(((INT_PTR)lpFile + (INT_PTR)VAImageDir - psh->VirtualAddress) +
                   (INT_PTR)psh->PointerToRawData);
}




/* function gets the function header for a section identified by name */
BOOL    WINAPI GetSectionHdrByName (
    LPVOID           lpFile,
    IMAGE_SECTION_HEADER     *sh,
    char             *szSection)
{
    PIMAGE_SECTION_HEADER    psh;
    int              nSections = NumOfSections (lpFile);
    int              i;


    if ((psh = (PIMAGE_SECTION_HEADER)SECHDROFFSET (lpFile)) != NULL)
    {
    /* find the section by name */
    for (i=0; i<nSections; i++)
        {
        if (!lstrcmp ((const char *)psh->Name, szSection))
        {
        /* copy data to header */
        CopyMemory ((LPVOID)sh, (LPVOID)psh, sizeof (IMAGE_SECTION_HEADER));
        return TRUE;
        }
        else
        psh++;
        }
    }

    return FALSE;
}


/* return section offset to specified RVA */
BOOL WINAPI GetRVASectionHdr (
                                         LPVOID   lpFile,
                                         DWORD    dwRVA,
                                         IMAGE_SECTION_HEADER *sh)
{
    PIMAGE_OPTIONAL_HEADER   poh = (PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (lpFile);
    PIMAGE_SECTION_HEADER    psh = (PIMAGE_SECTION_HEADER)SECHDROFFSET (lpFile);
    int              nSections = NumOfSections (lpFile);
    int              i = 0;

    /* locate section containing image directory */
    while (i++<nSections)
    {
    if (psh->VirtualAddress <= dwRVA &&
        psh->VirtualAddress + psh->SizeOfRawData > dwRVA)
        break;
    psh++;
    }

    if (i > nSections)
    return 0;


    CopyMemory((LPVOID)sh, (LPVOID)psh, sizeof(IMAGE_SECTION_HEADER));
    
    return (TRUE);
}
/* return section offset to specified IMAGE_DIRECTORY entry */
BOOL WINAPI ImageDirectorySectionHdr (
                                         LPVOID   lpFile,
                                         DWORD    dwIMAGE_DIRECTORY,
                                         IMAGE_SECTION_HEADER *sh)
{
    PIMAGE_OPTIONAL_HEADER   poh = (PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (lpFile);
    PIMAGE_SECTION_HEADER    psh = (PIMAGE_SECTION_HEADER)SECHDROFFSET (lpFile);
    int              nSections = NumOfSections (lpFile);
    int              i = 0;
    LPVOID           VAImageDir;

    /* must be 0 thru (NumberOfRvaAndSizes-1) */
    if (dwIMAGE_DIRECTORY >= poh->NumberOfRvaAndSizes)
    return 0;

    /* locate specific image directory's relative virtual address */
    VAImageDir = (LPVOID)poh->DataDirectory[dwIMAGE_DIRECTORY].VirtualAddress;

    /* locate section containing image directory */
    while (i++<nSections)
    {
    if (psh->VirtualAddress <= (DWORD_PTR)VAImageDir &&
        psh->VirtualAddress + psh->SizeOfRawData > (DWORD_PTR)VAImageDir)
        break;
    psh++;
    }

    if (i > nSections)
    return 0;


    CopyMemory((LPVOID)sh, (LPVOID)psh, sizeof(IMAGE_SECTION_HEADER));
    
    return (TRUE);
}



BOOL  WINAPI HasDllRegisterServer (
    LPVOID    lpFile)
{
    IMAGE_SECTION_HEADER       sh;
    PIMAGE_EXPORT_DIRECTORY    ped;
    char *pCurrent;
    int *szNameTable;

    /* get section header and pointer to data directory for .edata section */
    if ((ped = (PIMAGE_EXPORT_DIRECTORY)ImageDirectoryOffset
            (lpFile, IMAGE_DIRECTORY_ENTRY_EXPORT)) == NULL)
    return FALSE;

    if (!GetRVASectionHdr(lpFile, (DWORD)ped->AddressOfNames, &sh))
        return FALSE;

    /* determine the offset of the export function names */
    szNameTable = (int *)((int)ped->AddressOfNames -
                   (int)sh.VirtualAddress   +
                   (int)sh.PointerToRawData +
                   (INT_PTR)lpFile);

    for (int i=0; i<(int)ped->NumberOfNames; i++) {

        if (!GetRVASectionHdr(lpFile, szNameTable[i], &sh))
            return FALSE;

        pCurrent = (char *)(szNameTable[i] -
                      (int)sh.VirtualAddress   +
                      (int)sh.PointerToRawData +
                      (INT_PTR)lpFile);

        int iCmp;
        if ((iCmp = lstrcmp(pCurrent, "DllRegisterServer")) == 0)
            return TRUE;

        // name table is sorted!
        if (iCmp > 0)
            return FALSE;

    }

    return FALSE;
}
