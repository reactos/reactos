//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       printsi.cpp
//
//  This file contains the implementation of the CPrintSecurity object.
//
//--------------------------------------------------------------------------

#include "rshx32.h"


// The following array defines the permission names for NT printers.
SI_ACCESS siPrintAccesses[] =
{
    { &GUID_NULL, PRINTER_EXECUTE,           MAKEINTRESOURCE(IDS_PRINT_PRINT),           SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC },
    { &GUID_NULL, PRINTER_ALL_ACCESS,        MAKEINTRESOURCE(IDS_PRINT_ADMINISTER),      SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC },
    { &GUID_NULL, JOB_ALL_ACCESS,            MAKEINTRESOURCE(IDS_PRINT_ADMINISTER_JOBS), SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC | OBJECT_INHERIT_ACE | INHERIT_ONLY_ACE },
//    { &GUID_NULL, DELETE,                    MAKEINTRESOURCE(IDS_PRINT_DELETE),          SI_ACCESS_SPECIFIC },
    { &GUID_NULL, STANDARD_RIGHTS_READ,      MAKEINTRESOURCE(IDS_PRINT_READ),            SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_DAC,                 MAKEINTRESOURCE(IDS_PRINT_CHANGE_PERM),     SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_OWNER,               MAKEINTRESOURCE(IDS_PRINT_CHANGE_OWNER),    SI_ACCESS_SPECIFIC },
    { &GUID_NULL, PRINTER_ALL_ACCESS|JOB_ALL_ACCESS, MAKEINTRESOURCE(IDS_PRINT_JOB_ALL), 0 },
    { &GUID_NULL, 0,                         MAKEINTRESOURCE(IDS_NONE),                  0 },
};
#define iPrintDefAccess     0   // PRINTER_EXECUTE (i.e. "Print" access)

#define PRINTER_ALL_AUDIT           (PRINTER_ALL_ACCESS | ACCESS_SYSTEM_SECURITY)
#define JOB_ALL_AUDIT               (JOB_ALL_ACCESS | ACCESS_SYSTEM_SECURITY)
#define PRINTER_JOB_ALL_AUDIT       (PRINTER_ALL_ACCESS | JOB_ALL_ACCESS | ACCESS_SYSTEM_SECURITY)

// The following array defines the auditting names for NT printers.
SI_ACCESS siPrintAudits[] =
{
    { &GUID_NULL, PRINTER_EXECUTE,           MAKEINTRESOURCE(IDS_PRINT_PRINT),           SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC },
    { &GUID_NULL, PRINTER_ALL_AUDIT,         MAKEINTRESOURCE(IDS_PRINT_ADMINISTER),      SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC },
    { &GUID_NULL, JOB_ALL_AUDIT,             MAKEINTRESOURCE(IDS_PRINT_ADMINISTER_JOBS), SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC | OBJECT_INHERIT_ACE | INHERIT_ONLY_ACE },
//    { &GUID_NULL, DELETE,                    MAKEINTRESOURCE(IDS_PRINT_DELETE),          SI_ACCESS_SPECIFIC },
    { &GUID_NULL, STANDARD_RIGHTS_READ,      MAKEINTRESOURCE(IDS_PRINT_READ),            SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_DAC,                 MAKEINTRESOURCE(IDS_PRINT_CHANGE_PERM),     SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_OWNER,               MAKEINTRESOURCE(IDS_PRINT_CHANGE_OWNER),    SI_ACCESS_SPECIFIC },
    { &GUID_NULL, PRINTER_ALL_AUDIT|JOB_ALL_AUDIT, MAKEINTRESOURCE(IDS_PRINT_JOB_ALL),   0 },
    { &GUID_NULL, 0,                         MAKEINTRESOURCE(IDS_NONE),                  0 },
};
#define iPrintDefAudit      0   // PRINTER_EXECUTE (i.e. "Print" access)

// The following array defines the inheritance types for NT printers.
SI_INHERIT_TYPE siPrintInheritTypes[] =
{
    &GUID_NULL, 0,                                     MAKEINTRESOURCE(IDS_PRINT_PRINTER),
    &GUID_NULL, INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE, MAKEINTRESOURCE(IDS_PRINT_DOCUMENT_ONLY),
    &GUID_NULL, OBJECT_INHERIT_ACE,                    MAKEINTRESOURCE(IDS_PRINT_PRINTER_DOCUMENT),
};


BOOL
GetPrinterAlloc(HANDLE hPrinter, DWORD dwLevel, LPBYTE *ppBuffer)
{
    BOOL bResult;
    DWORD dwLength = 0;
    LPBYTE pBuffer = NULL;

    bResult = GetPrinter(hPrinter, dwLevel, NULL, 0, &dwLength);
    if (dwLength)
    {
        bResult = FALSE;
        pBuffer = (LPBYTE)LocalAlloc(LPTR, dwLength);
        if (pBuffer)
        {
            bResult = GetPrinter(hPrinter, dwLevel, pBuffer, dwLength, &dwLength);
            if (!bResult)
            {
                LocalFree(pBuffer);
                pBuffer = NULL;
            }
        }
    }
    *ppBuffer = pBuffer;
    return bResult;
}


STDMETHODIMP
CheckPrinterAccess(LPCTSTR pszObjectName,
                   LPDWORD pdwAccessGranted,
                   LPTSTR  pszServer,
                   ULONG   cchServer)
{
    HRESULT hr = S_OK;
    UINT i;
    PRINTER_DEFAULTS PrinterDefaults;
    DWORD dwAccessDesired[] = { ALL_SECURITY_ACCESS,
                                READ_CONTROL,
                                WRITE_DAC,
                                WRITE_OWNER,
                                ACCESS_SYSTEM_SECURITY };
    HANDLE hPrinter = NULL;

    PrinterDefaults.pDatatype = NULL;
    PrinterDefaults.pDevMode  = NULL;

    TraceEnter(TRACE_PRINTSI, "CheckPrinterAccess");
    TraceAssert(pdwAccessGranted != NULL);

    __try
    {
        *pdwAccessGranted = 0;

        for (i = 0; i < ARRAYSIZE(dwAccessDesired); i++)
        {
            if ((dwAccessDesired[i] & *pdwAccessGranted) == dwAccessDesired[i])
                continue;   // already have this access

            PrinterDefaults.DesiredAccess = dwAccessDesired[i];

            if (OpenPrinter((LPTSTR)pszObjectName, &hPrinter, &PrinterDefaults))
            {
                *pdwAccessGranted |= dwAccessDesired[i];
                ClosePrinter(hPrinter);
            }
            else
            {
                DWORD dwErr = GetLastError();

                if (dwErr != ERROR_ACCESS_DENIED &&
                    dwErr != ERROR_PRIVILEGE_NOT_HELD)
                {
                    ExitGracefully(hr, HRESULT_FROM_WIN32(dwErr), "OpenPrinter failed");
                }
            }
        }

        if (pszServer)
        {
            PrinterDefaults.DesiredAccess = PRINTER_READ;
            if (OpenPrinter((LPTSTR)pszObjectName, &hPrinter, &PrinterDefaults))
            {
                PPRINTER_INFO_2 ppi = NULL;
                if (GetPrinterAlloc(hPrinter, 2, (LPBYTE*)&ppi))
                {
                    if (ppi->pServerName)
                        lstrcpyn(pszServer, ppi->pServerName, cchServer);
                    else
                        *pszServer = TEXT('\0');
                    LocalFree(ppi);
                }
                ClosePrinter(hPrinter);
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
    }

exit_gracefully:

    Trace((TEXT("Access = 0x%08x"), *pdwAccessGranted));
    TraceLeaveResult(hr);
}


STDMETHODIMP
CPrintSecurity::Initialize(HDPA     hItemList,
                           DWORD    dwFlags,
                           LPTSTR   pszServer,
                           LPTSTR   pszObject)
{
    return CSecurityInformation::Initialize(hItemList,
                                            dwFlags | SI_NO_TREE_APPLY | SI_NO_ACL_PROTECT,
                                            pszServer,
                                            pszObject);
}


//
// NT6 REVIEW
//
// GetAceSid, FindManagePrinterACE, MungeAclForPrinter and
// CPrintSecurity::SetSecurity only exist here because
// 1) The spooler removes JOB_ACCESS_ADMINISTER from an ACE unless the
//    ACE has INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE.
// 2) The NT4 ACL editor (ACLEDIT) needs extra bogus ACEs to recognize
//    "Manage Documents" access. (Must support downlevel clients.)
//
// The first case should be rare, since you have to perform certain
// steps in the NT5 ACL editor (ACLUI) to cause this situation. The
// second situation is common, since CREATER_OWNER and Administrators
// usually have "Manage Documents" access.
//
// If the spooler guys decide to not support NT4 clients for NT6, and they
// stop stripping JOB_ACCESS_ADMINISTER from ACEs, then MungeAclForPrinter
// and CPrintSecurity::SetSecurity can be removed entirely. ENCOURAGE THEM
// TO MAKE THAT CHANGE. (They can also remove similar hacks from their own
// code that add bogus ACEs for the old ACL editor.)
//

PSID
GetAceSid(PACE_HEADER pAce)
{
    switch (pAce->AceType)
    {
    case ACCESS_ALLOWED_ACE_TYPE:
    case ACCESS_DENIED_ACE_TYPE:
    case SYSTEM_AUDIT_ACE_TYPE:
    case SYSTEM_ALARM_ACE_TYPE:
        return (PSID)&((PKNOWN_ACE)pAce)->SidStart;

    case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
        return (PSID)&((PCOMPOUND_ACCESS_ALLOWED_ACE)pAce)->SidStart;

    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
    case ACCESS_DENIED_OBJECT_ACE_TYPE:
    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
    case SYSTEM_ALARM_OBJECT_ACE_TYPE:
        return RtlObjectAceSid(pAce);
    }

    return NULL;
}

PACE_HEADER
FindManagePrinterACE(PACL pAcl, PSID pSid)
{
    UINT i;
    PACE_HEADER pAce;

    if (!pAcl || !pSid)
        return NULL;

    for (i = 0, pAce = (PACE_HEADER)FirstAce(pAcl);
         i < pAcl->AceCount;
         i++, pAce = (PACE_HEADER)NextAce(pAce))
    {
        if (pAce->AceType == ACCESS_ALLOWED_ACE_TYPE
            && (((PKNOWN_ACE)pAce)->Mask & PRINTER_ALL_ACCESS) == PRINTER_ALL_ACCESS
            && !(pAce->AceFlags & INHERIT_ONLY_ACE)
            && EqualSid(pSid, GetAceSid(pAce)))
        {
            return pAce;
        }
    }

    return NULL;
}

BOOL
MungeAclForPrinter(PACL pAcl, PACL *ppAclOut)
{
    USHORT i;
    PACE_HEADER pAce;
    PACE_HEADER pAceCopy = NULL;

    if (ppAclOut == NULL)
        return FALSE;

    *ppAclOut = NULL;

    if (pAcl == NULL)
        return TRUE;

    TraceEnter(TRACE_PRINTSI, "MungeAclForPrinter");

    for (i = 0, pAce = (PACE_HEADER)FirstAce(pAcl);
         i < pAcl->AceCount;
         i++, pAce = (PACE_HEADER)NextAce(pAce))
    {
        //
        // If this ACE has the JOB_ACCESS_ADMINISTER bit and the inherit
        // flags indicate that it applies to both printers and documents,
        // then we need to treat it specially, since the spooler won't save
        // JOB_ACCESS_ADMINISTER on a printer ACE (INHERIT_ONLY_ACE not set).
        //
        if ((((PKNOWN_ACE)pAce)->Mask & JOB_ACCESS_ADMINISTER) &&
            (pAce->AceFlags & (INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE)) == OBJECT_INHERIT_ACE)
        {
            //
            // Split into 2 aces: one with no inheritance, and one with
            // INHERIT_ONLY_ACE turned on. Let the spooler do whatever
            // it wants with the mask.
            //
            // This requires allocating a larger ACL and copying all
            // previous aces over.
            //

            TraceMsg("Splitting JOB_ACCESS_ADMINISTER ACE into 2");

            if (*ppAclOut == NULL)
            {
                //
                // Allocate new ACL and copy previous aces.  The length is enough
                // for 1 copy of all previous aces, and 3 copies (max) of all
                // remaining aces.
                //
                ULONG nPrevLength = (ULONG)((ULONG_PTR)pAce - (ULONG_PTR)pAcl);
                *ppAclOut = (PACL)LocalAlloc(LPTR, nPrevLength + (pAcl->AclSize - nPrevLength) * 3);
                if (!*ppAclOut)
                    TraceLeaveValue(FALSE);

                CopyMemory(*ppAclOut, pAcl, nPrevLength);
                (*ppAclOut)->AclSize = (USHORT)LocalSize(*ppAclOut);
                (*ppAclOut)->AceCount = i;
                pAceCopy = (PACE_HEADER)ByteOffset(*ppAclOut, nPrevLength);
            }

            // Turn off inheritance and copy this ace
            pAce->AceFlags &= ~OBJECT_INHERIT_ACE;
            CopyMemory(pAceCopy, pAce, pAce->AceSize);
            pAceCopy = (PACE_HEADER)NextAce(pAceCopy);
            (*ppAclOut)->AceCount++;

            // Now turn on inheritance (with INHERIT_ONLY_ACE) and copy it
            // again (it gets copied way down below).  Note that this may
            // causes the next IF clause to add a bogus ACE also.
            pAce->AceFlags |= OBJECT_INHERIT_ACE | INHERIT_ONLY_ACE;
        }

        //
        // If this ACE has JOB_ALL_ACCESS and INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE,
        // and there isn't also a "Manage Printers" ACE for the same SID, add a
        // bogus ACE with READ_CONTROL and CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE.
        // The old ACL editor on downlevel clients needs this to recognize
        // "Manage Documents" access.
        //
        if (pAce->AceType == ACCESS_ALLOWED_ACE_TYPE
            && (((PKNOWN_ACE)pAce)->Mask & JOB_ALL_ACCESS) == JOB_ALL_ACCESS
            && (pAce->AceFlags & (INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE)) == (INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE)
            && !FindManagePrinterACE(pAcl, GetAceSid(pAce)))
        {
            TraceMsg("Adding bogus ACE for downlevel support");

            if (*ppAclOut == NULL)
            {
                //
                // Allocate new ACL and copy previous aces.  The length is enough
                // for 1 copy of all previous aces, and 3 copies (max) of all
                // remaining aces.
                //
                ULONG nPrevLength = (ULONG)((ULONG_PTR)pAce - (ULONG_PTR)pAcl);
                *ppAclOut = (PACL)LocalAlloc(LPTR, nPrevLength + (pAcl->AclSize - nPrevLength) * 3);
                if (!*ppAclOut)
                    TraceLeaveValue(FALSE);

                CopyMemory(*ppAclOut, pAcl, nPrevLength);
                (*ppAclOut)->AclSize = (USHORT)LocalSize(*ppAclOut);
                (*ppAclOut)->AceCount = i;
                pAceCopy = (PACE_HEADER)ByteOffset(*ppAclOut, nPrevLength);
            }

            // Copy this ace, turn on CONTAINER_INHERIT_ACE, and set
            // the mask to STANDARD_RIGHTS_READ.
            CopyMemory(pAceCopy, pAce, pAce->AceSize);
            pAceCopy->AceFlags &= ~OBJECT_INHERIT_ACE;
            pAceCopy->AceFlags |= INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE;
            ((PKNOWN_ACE)pAceCopy)->Mask = STANDARD_RIGHTS_READ;
            pAceCopy = (PACE_HEADER)NextAce(pAceCopy);
            (*ppAclOut)->AceCount++;
        }

        if (*ppAclOut != NULL)
        {
            // Copy current ace
            CopyMemory(pAceCopy, pAce, pAce->AceSize);
            pAceCopy = (PACE_HEADER)NextAce(pAceCopy);
            (*ppAclOut)->AceCount++;
        }
    }

    if (*ppAclOut != NULL)
    {
        TraceAssert((ULONG_PTR)pAceCopy > (ULONG_PTR)*ppAclOut &&
                    (ULONG_PTR)pAceCopy <= (ULONG_PTR)*ppAclOut + (*ppAclOut)->AclSize);

        // Set the ACL size to the correct value
        (*ppAclOut)->AclSize = (WORD)((ULONG_PTR)pAceCopy - (ULONG_PTR)*ppAclOut);
    }

    TraceLeaveValue(TRUE);
}


///////////////////////////////////////////////////////////
//
// ISecurityInformation methods
//
///////////////////////////////////////////////////////////

STDMETHODIMP
CPrintSecurity::SetSecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR pSD)
{
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    PACL pDaclCopy = NULL;
    PACL pSaclCopy = NULL;
    BOOL bPresent;
    BOOL bDefaulted;
    SECURITY_DESCRIPTOR sd;

    TraceEnter(TRACE_PRINTSI, "CPrintSecurity::SetSecurity");

    if ((si & DACL_SECURITY_INFORMATION)
        && GetSecurityDescriptorDacl(pSD, &bPresent, &pDacl, &bDefaulted)
        && bPresent)
    {
        if (MungeAclForPrinter(pDacl, &pDaclCopy) && pDaclCopy)
            pDacl = pDaclCopy;
    }

    if ((si & SACL_SECURITY_INFORMATION)
        && GetSecurityDescriptorSacl(pSD, &bPresent, &pSacl, &bDefaulted)
        && bPresent)
    {
        if (MungeAclForPrinter(pSacl, &pSaclCopy) && pSaclCopy)
            pSacl = pSaclCopy;
    }

    if (pDaclCopy || pSaclCopy)
    {
        // Build a new SECURITY_DESCRIPTOR
        PSID psid;
        DWORD dwRevision;
        SECURITY_DESCRIPTOR_CONTROL sdControl = 0;

        GetSecurityDescriptorControl(pSD, &sdControl, &dwRevision);
        InitializeSecurityDescriptor(&sd, dwRevision);
        sd.Control = (SECURITY_DESCRIPTOR_CONTROL)(sdControl & ~SE_SELF_RELATIVE);

        if ((si & OWNER_SECURITY_INFORMATION)
            && GetSecurityDescriptorOwner(pSD, &psid, &bDefaulted))
        {
            SetSecurityDescriptorOwner(&sd, psid, bDefaulted);
        }

        if ((si & GROUP_SECURITY_INFORMATION)
            && GetSecurityDescriptorGroup(pSD, &psid, &bDefaulted))
        {
            SetSecurityDescriptorGroup(&sd, psid, bDefaulted);
        }

        if (si & SACL_SECURITY_INFORMATION)
        {
            SetSecurityDescriptorSacl(&sd,
                                      sdControl & SE_SACL_PRESENT,
                                      pSacl,
                                      sdControl & SE_SACL_DEFAULTED);
        }

        if (si & DACL_SECURITY_INFORMATION)
        {
            SetSecurityDescriptorDacl(&sd,
                                      sdControl & SE_DACL_PRESENT,
                                      pDacl,
                                      sdControl & SE_DACL_DEFAULTED);
        }

        // Switch to the new security descriptor
        pSD = &sd;
    }

    // The base class does the rest of the work
    HRESULT hr = CSecurityInformation::SetSecurity(si, pSD);

    if (pDaclCopy)
        LocalFree(pDaclCopy);

    if (pSaclCopy)
        LocalFree(pSaclCopy);

    TraceLeaveResult(hr);
}

STDMETHODIMP
CPrintSecurity::GetAccessRights(const GUID* /*pguidObjectType*/,
                               DWORD dwFlags,
                               PSI_ACCESS *ppAccesses,
                               ULONG *pcAccesses,
                               ULONG *piDefaultAccess)
{
    TraceEnter(TRACE_PRINTSI, "CPrintSecurity::GetAccessRights");
    TraceAssert(ppAccesses != NULL);
    TraceAssert(pcAccesses != NULL);
    TraceAssert(piDefaultAccess != NULL);

    if (dwFlags & SI_EDIT_AUDITS)
    {
        *ppAccesses = siPrintAudits;
        *pcAccesses = ARRAYSIZE(siPrintAudits);
        *piDefaultAccess = iPrintDefAudit;
    }
    else
    {
        *ppAccesses = siPrintAccesses;
        *pcAccesses = ARRAYSIZE(siPrintAccesses);
        *piDefaultAccess = iPrintDefAccess;
    }

    TraceLeaveResult(S_OK);
}


GENERIC_MAPPING JobMap =
{
    JOB_READ,
    JOB_WRITE,
    JOB_EXECUTE,
    JOB_ALL_ACCESS
};

GENERIC_MAPPING PrinterMap =
{
    PRINTER_READ,
    PRINTER_WRITE,
    PRINTER_EXECUTE,
    PRINTER_ALL_ACCESS
};

GENERIC_MAPPING FullPrinterMap =
{
    PRINTER_READ | JOB_READ,
    PRINTER_WRITE | JOB_WRITE,
    PRINTER_EXECUTE | JOB_EXECUTE,
    PRINTER_ALL_ACCESS | JOB_ALL_ACCESS
};

STDMETHODIMP
CPrintSecurity::MapGeneric(const GUID* /*pguidObjectType*/,
                           UCHAR *pAceFlags,
                           ACCESS_MASK *pmask)
{
    PGENERIC_MAPPING pMap;

    TraceEnter(TRACE_PRINTSI, "CPrintSecurity::MapGeneric");
    TraceAssert(pAceFlags != NULL);
    TraceAssert(pmask != NULL);

    // This flag has no meaning for printers, but it's often turned on
    // in legacy ACLs.  Turn it off here
    *pAceFlags &= ~CONTAINER_INHERIT_ACE;

    // Choose the correct generic mapping according to the inherit
    // scope of this ACE.
    if (*pAceFlags & OBJECT_INHERIT_ACE)
    {
        if (*pAceFlags & INHERIT_ONLY_ACE)
            pMap = &JobMap;                 // documents only
        else
            pMap = &FullPrinterMap;         // printers & documents
    }
    else
        pMap = &PrinterMap;                 // printers only

    // Note that the case where INHERIT_ONLY_ACE is ON but OBJECT_INHERIT_ACE
    // is OFF falls under the "printers only" case above. However, this
    // case makes no sense (inherit-only, but not onto documents) and it
    // doesn't matter how we do the mapping.

    // Map any generic bits to standard & specific bits.
    // When using the NT5 ACL APIs, ntmarta.dll maps generic bits, so this
    // isn't always necessary, but we'll do it anyway to be sure.
    MapGenericMask(pmask, pMap);

    // BUGBUG Turn off any extra bits that ntmarta.dll may have turned on
    // (ntmarta uses a different mapping).  But leave ACCESS_SYSTEM_SECURITY
    // alone in case we're editing a SACL.
    *pmask &= (pMap->GenericAll | ACCESS_SYSTEM_SECURITY);

    TraceLeaveResult(S_OK);
}

STDMETHODIMP
CPrintSecurity::GetInheritTypes(PSI_INHERIT_TYPE *ppInheritTypes,
                                ULONG *pcInheritTypes)
{
    TraceEnter(TRACE_PRINTSI, "CPrintSecurity::GetInheritTypes");
    TraceAssert(ppInheritTypes != NULL);
    TraceAssert(pcInheritTypes != NULL);

    *ppInheritTypes = siPrintInheritTypes;
    *pcInheritTypes = ARRAYSIZE(siPrintInheritTypes);

    TraceLeaveResult(S_OK);
}

//
// The base class versions of ReadObjectSecurity and WriteObjectSecurity
// use Get/SetNamedSecurityInfo, et al.  These API's are generic,
// involve lots of conversions, and are problematic.  Since there is no
// inheritance propagation required for printers, override them here
// and use GetPrinter/SetPrinter.
//
STDMETHODIMP
CPrintSecurity::ReadObjectSecurity(LPCTSTR pszObject,
                                   SECURITY_INFORMATION si,
                                   PSECURITY_DESCRIPTOR *ppSD)
{
    HRESULT hr;
    DWORD dwErr = NOERROR;
    HANDLE hPrinter;
    PRINTER_DEFAULTS pd = {0};
    DWORD dwLength = 0;

    TraceEnter(TRACE_PRINTSI, "CPrintSecurity::ReadObjectSecurity");
    TraceAssert(pszObject != NULL);
    TraceAssert(si != 0);
    TraceAssert(ppSD != NULL);

    //
    // Assume that required privileges have already been
    // enabled, if appropriate.
    //
    if (si & (DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION))
        pd.DesiredAccess |= READ_CONTROL;

    if (si & SACL_SECURITY_INFORMATION)
        pd.DesiredAccess |= ACCESS_SYSTEM_SECURITY;

    __try
    {
        *ppSD = NULL;

        if (OpenPrinter((LPTSTR)pszObject, &hPrinter, &pd))
        {
            PPRINTER_INFO_3 ppi = NULL;

            if (GetPrinterAlloc(hPrinter, 3, (LPBYTE*)&ppi))
            {
                //
                // Rather than allocating a new buffer and copying the
                // security descriptor, we can re-use the existing buffer
                // by simply moving the security descriptor to the top.
                //
                dwLength = GetSecurityDescriptorLength(ppi->pSecurityDescriptor);
                *ppSD = ppi;
                // This is an overlapped copy, so use MoveMemory.
                MoveMemory(*ppSD,
                           ppi->pSecurityDescriptor,
                           dwLength);
            }
            else
                dwErr = GetLastError();

            ClosePrinter(hPrinter);
        }
        else
            dwErr = GetLastError();
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErr = ERROR_PROC_NOT_FOUND;
    }

    hr = HRESULT_FROM_WIN32(dwErr);
    TraceLeaveResult(hr);
}

STDMETHODIMP
CPrintSecurity::WriteObjectSecurity(LPCTSTR pszObject,
                                    SECURITY_INFORMATION si,
                                    PSECURITY_DESCRIPTOR pSD)
{
    HRESULT hr;
    DWORD dwErr = NOERROR;
    HANDLE hPrinter;
    PRINTER_DEFAULTS pd = {0};

    TraceEnter(TRACE_PRINTSI, "CPrintSecurity::WriteObjectSecurity");
    TraceAssert(pszObject != NULL);
    TraceAssert(si != 0);
    TraceAssert(pSD != NULL);

    //
    // Assume that required privileges have already been
    // enabled, if appropriate.
    //
    if (si & (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION))
        pd.DesiredAccess |= WRITE_OWNER;

    if (si & SACL_SECURITY_INFORMATION)
        pd.DesiredAccess |= ACCESS_SYSTEM_SECURITY;

    if (si & DACL_SECURITY_INFORMATION)
        pd.DesiredAccess |= WRITE_DAC;

    __try
    {
        if (OpenPrinter((LPTSTR)pszObject, &hPrinter, &pd))
        {
            PRINTER_INFO_3 pi = { pSD };

            if (!SetPrinter(hPrinter, 3, (LPBYTE)&pi, 0))
                dwErr = GetLastError();

            ClosePrinter(hPrinter);
        }
        else
            dwErr = GetLastError();
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErr = ERROR_PROC_NOT_FOUND;
    }

    hr = HRESULT_FROM_WIN32(dwErr);
    TraceLeaveResult(hr);
}
