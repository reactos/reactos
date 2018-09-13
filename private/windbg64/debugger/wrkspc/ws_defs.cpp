#include "precomp.hxx"
#pragma hdrstop





//
//
//

CGenInterface_WKSP::
CGenInterface_WKSP()
: m_pszRegistryName(NULL), m_pParent(NULL), m_bMirror(FALSE), m_bDynamicList(FALSE),
    m_hkeyMirror(NULL), m_hkeyRegistry(NULL)

{
}

CGenInterface_WKSP::
~CGenInterface_WKSP()
{
    LONG lres;

    // Deallocate the duplicated string.
    SetRegistryName(NULL);

    // Close the registry keys
    if (m_hkeyRegistry) {
        
        lres = RegCloseKey(m_hkeyRegistry);
        Assert(ERROR_SUCCESS == lres);
        
        m_hkeyRegistry = NULL;
    }

    if (m_hkeyMirror) {
        Assert(m_bMirror);

        lres = RegCloseKey(m_hkeyMirror);
        Assert(ERROR_SUCCESS == lres);
        
        m_hkeyMirror = NULL;
    }
}

void 
CGenInterface_WKSP::
Init(
    CGenInterface_WKSP * const pParent, 
    const char * const pszRegistryName, 
    BOOL bMirror, 
    BOOL bDynamic
    )
/*++
NOTE: This ugly initializer is necessary because the complete construction of base classes
    is not taking place.
--*/
{
    if (m_pszRegistryName || m_pParent) {
        Assert(!"Attempt to call Init a second time.");
    }

    SetParent(pParent);
    SetRegistryName(pszRegistryName);

    m_bDynamicList = bDynamic;
    m_bMirror = bMirror;
    m_hkeyMirror = NULL;
    m_hkeyRegistry = NULL;

    if (m_pParent) {
        AssertChildOf(m_pParent, CGenInterface_WKSP);

        // If the parent is mirrored, so is this child
        if (m_pParent->m_bMirror) {
            m_bMirror = TRUE;
        }

        // BUGBUG
        m_pParent->AddToContainerList(this);
    }
}

void
CGenInterface_WKSP::
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

void
CGenInterface_WKSP::
SetParent(
    CGenInterface_WKSP * const pParent
    )
/*++
Routine Description:
    Sets the parent class.

Arguments:
    pParent - New parent.
--*/
{
    if (pParent) {
        AssertChildOf(pParent, CGenInterface_WKSP);
    }
    SetConstPointer(CGenInterface_WKSP, m_pParent, pParent);
}

CGenInterface_WKSP *
CGenInterface_WKSP::
GetRootParent()
/*++
Routine Description:
    Returns a pointer to the root container (the container that has no parent).

Returns:
    Pointer to the root container.
--*/
{
    if (m_pParent) {
        return m_pParent->GetRootParent();
    } else {
        return this;
    }
}



