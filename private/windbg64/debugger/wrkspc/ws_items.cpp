#include "precomp.hxx"
#pragma hdrstop



//
// CItemInterface_WKSP implementation
//
CItemInterface_WKSP::
CItemInterface_WKSP(
    const char * const  pszRegistryName
    )
: m_pszRegistryName(NULL)
/*++
Routine Description:
    Expects to receive a valid registry name. The name that is passed in
    is duplicated, so that temp strings can be used to initialize the object.

Arguments:
    pParent - Parent container. Can be NULL.
    pszRegistryName - Name of entry in registry. Can be NULL.
--*/
{
    SetRegistryName(pszRegistryName);
}



CItemInterface_WKSP::
~CItemInterface_WKSP()
/*++
Routine Description:
    Deallocate the duplicated string.
--*/
{
    SetRegistryName(NULL);
}



void
CItemInterface_WKSP::
SetRegistryName(
    const char * const pszRegistryName
    )
/*++
Routine Description:
    Sets the registry name. Frees the previous registry name, and
    duplicates the string arg using _strdup.

Arguments:
    pszRegistryName - Registry name that will be dupiclated and assigned
        to this class.
--*/
{
    if (m_pszRegistryName) {
        free( (PVOID) m_pszRegistryName);
        SetConstPointer(char, m_pszRegistryName, NULL);
    }

    if (pszRegistryName) {
        // If not NULL duplicate it.
        SetConstPointer(char, m_pszRegistryName, _strdup(pszRegistryName));
    }
}



BOOL
CItemInterface_WKSP::
Read(
    const HKEY hkey
    )
const
/*++
Routine Description:
    Will read the value of the data member from the registry. Simple wrapper
    around "RegQueryValueEx".

Arguments:
    hkey - Registry key of that contains this value. Cannot be NULL, and it
        must reference an open registry key.

Returns:
    TRUE - success
    FALSE - error
--*/
{
    Assert(hkey);
    
    DWORD dwSize, dwType;
    PBYTE pbyData = NULL;

    // Get the required size
    if (ERROR_SUCCESS != RegQueryValueEx(hkey,
                                         m_pszRegistryName, 
                                         0, 
                                         &dwType, 
                                         NULL, 
                                         &dwSize
                                         )) {

        // Error
        return FALSE;
    }

    // Sanity check
    Assert(GetRegType() == dwType);

    // Value does exist, let's do it.
    switch (GetRegType()) {
    default:
        Assert(!"Unsupported data type.");
        break;

    case REG_BINARY:
    case REG_DWORD:
        pbyData = (PBYTE) GetPtrToData();
        Assert(CalcSizeOfData() == dwSize);
        break;

    case REG_SZ:
    case REG_MULTI_SZ:
        {
            PSTR * ppsz = (PSTR *) GetPtrToData();

            // Reallocate our buffer
            if (*ppsz) {
                free( (PVOID) *ppsz);
                *ppsz = NULL;
            }

            *ppsz = (PSTR) calloc(dwSize, 1);
        
            pbyData = (PBYTE) *ppsz;
        }
        break;
    }

    RegQueryValueEx(hkey,
                    m_pszRegistryName, 
                    0, 
                    &dwType, 
                    pbyData, 
                    &dwSize
                    );

    return TRUE;
}



void
CItemInterface_WKSP::
Save(
    HKEY hkey
    )
const
/*++
Routine Description:
    Saves the contents of a workspace item out to registry.

Arguments:
    hkey - Open registry key. Can't be NULL.
--*/
{
    Assert(hkey);
    
    PBYTE pbyData = NULL;

    switch (GetRegType()) {
    default:
        Assert(!"Unsupported data type.");
        break;

    case REG_BINARY:
    case REG_DWORD:
        pbyData = (PBYTE) GetPtrToData();
        break;

    case REG_SZ:
    case REG_MULTI_SZ:
        pbyData = *(UCHAR * *) GetPtrToData();
        break;
    }


    RegSetValueEx(hkey,
                  m_pszRegistryName,
                  0,
                  GetRegType(),
                  pbyData,
                  CalcSizeOfData()
                  );
}



void
CItemInterface_WKSP::
Restore(
    HKEY hkey
    )
const
/*++
Routine Description:
    Reads

Arguments:
    hkey - Open registry key. Can't be NULL.
--*/
{
    Assert(hkey);

    if (!Read(hkey)) {
        // Error reading the value (hopefully the error was caused
        // because it didn't exist). Let's try writing it out.
        Save(hkey);
    }
}



BOOL
CItemInterface_WKSP::
GetValue(
    PDWORD pdwType, 
    PVOID pvData, 
    PDWORD pcbData
    )
const
/*++
Routine Description:
    Helper function, used to get the value of a class. Implemented because we never know
    whether we are dealing with a data type of a pointer to a data type.

    Foo.GetValue(NULL, NULL, NULL);         // useless but ok.
    Foo.GetValue(&dwType, NULL, NULL);      // ok.
    Foo.GetValue(NULL, pv, &dwSize);        // ok.
    Foo.GetValue(&dwType, pv, &dwSize);     // ok.
    Foo.GetValue(NULL, NULL, &dwSize);      // ok.

    Foo.GetValue(&dwType, pv, NULL);        // Error: Must specify the size of the buffer
    Foo.GetValue(NULL, pv, NULL);           // Error: Must specify the size of the buffer

Arguments:
    pdwType - Receives the registry key value type. May be NULL.
    pvData - Pointer to a buffer that will receive the data. May be NULL.
    pcbData - Receives the size of the buffer in bytes. May be NULL if pvData is NULL. If pvData is
        not NULL, pcbData must point to a DWORD.

Returns:
    TRUE - if successful.
    FALSE - An attempt to retrieve the data and the buffer is too small.
--*/
{
    if (pvData && NULL == pcbData) {
        Assert(!"Bad Arguments!");
    }
    
    if (pdwType) {
        *pdwType = GetRegType();
    }

    if (pcbData) {
        *pcbData = CalcSizeOfData();
    }

    if (pvData) {
        if (*pcbData < CalcSizeOfData()) {
            // Buffer too small
            return FALSE;
        }
        
        PVOID pv = NULL;
        switch (GetRegType()) {
        default:
            Assert(!"Unsupported data type.");
            break;

        case REG_BINARY:
        case REG_DWORD:
            pv = (PVOID) GetPtrToData();
            break;

        case REG_SZ:
        case REG_MULTI_SZ:
            pv = *(void * *) GetPtrToData();
            break;
        }

        memcpy(pvData, pv, CalcSizeOfData());
    }

    return TRUE;
}



void
CItemInterface_WKSP::
Duplicate(
    const CItemInterface_WKSP & arg
    )
/*++
Routine Description:
    Does NOT copy the parent pointer.

    Only copies the registry name.

Arguments:
    arg - Other CItemInterface_WKSP from which to copy the data.
--*/
{
    Assert(GetRegType() == arg.GetRegType());

    SetRegistryName( arg.m_pszRegistryName);

    DWORD dwSize = arg.CalcSizeOfData();

    switch (GetRegType()) {
    default:
        Assert(!"Unsupported data type.");
        break;

    case REG_BINARY:
    case REG_DWORD:
        {
            PDWORD pdwDest = (PDWORD) GetPtrToData();
            PDWORD pdwSrc = (PDWORD) arg.GetPtrToData();
            Assert(pdwDest);
            Assert(pdwSrc);
            
            memcpy(pdwDest, pdwSrc, dwSize);
        }
        break;

    case REG_SZ:
    case REG_MULTI_SZ:
        {
            PSTR * ppszDest = (PSTR *) GetPtrToData();
            Assert(ppszDest);

            PSTR * ppszSrc = (PSTR *) arg.GetPtrToData();
            Assert(ppszSrc);
            
            if (*ppszDest) {
                free(*ppszDest);
                *ppszDest = NULL;
            }
            
            if (*ppszSrc) {
                *ppszDest = (PSTR) calloc(dwSize, 1);
                memcpy(*ppszDest, *ppszSrc, dwSize);
            }
        }
        break;
    }

}




