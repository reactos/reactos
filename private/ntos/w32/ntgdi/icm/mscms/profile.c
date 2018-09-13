/****************************Module*Header******************************\
* Module Name: PROFILE.C
*
* Module Descripton: Profile data manipulation functions
*
* Warnings:
*
* Issues:
*
* Public Routines:
*
* Created:  11 January 1996
* Author:   Srinivasan Chandrasekar    [srinivac]
*
* Copyright (c) 1996, 1997  Microsoft Corporation
\***********************************************************************/

#include "mscms.h"

#define PROFILE_GROWTHCUSHION       16*1024

//
// Local functions
//

HPROFILE InternalOpenColorProfile(PPROFILE, DWORD, DWORD, DWORD);
BOOL InternalCreateProfileFromLCS(LPLOGCOLORSPACE, PBYTE*, BOOL);
BOOL FreeProfileObject(HPROFILE);
BOOL AddTagTableEntry(PPROFOBJ, TAGTYPE, DWORD, DWORD, BOOL);
BOOL AddTaggedElement(PPROFOBJ, TAGTYPE, DWORD);
BOOL DeleteTaggedElement(PPROFOBJ, PTAGDATA);
BOOL ChangeTaggedElementSize(PPROFOBJ, PTAGDATA, DWORD);
BOOL GrowProfile(PPROFOBJ, DWORD);
void MoveProfileData(PPROFOBJ, PBYTE, PBYTE, LONG, BOOL);
BOOL IsReferenceTag(PPROFOBJ, PTAGDATA);


/******************************************************************************
 *
 *                            OpenColorProfile
 *
 *  Function:
 *       These are the ANSI & Unicode wrappers for InternalOpenColorProfile.
 *       Please see InternalOpenColorProfile for more details on this function.
 *
 *  Returns:
 *       Handle to open profile on success, zero on failure.
 *
 ******************************************************************************/

#ifdef UNICODE          // Windows NT versions

HPROFILE WINAPI OpenColorProfileA(
    PPROFILE pProfile,
    DWORD    dwDesiredAccess,
    DWORD    dwShareMode,
    DWORD    dwCreationMode
    )
{
    PROFILE  wProfile;      // Unicode version
    HPROFILE rc = NULL;

    //
    // Validate parameters before touching them
    //

    if (!pProfile ||
        IsBadReadPtr(pProfile, sizeof(PROFILE)) ||
        (pProfile->pProfileData &&
         IsBadReadPtr(pProfile->pProfileData, pProfile->cbDataSize)) ||
        (!pProfile->pProfileData &&
         (pProfile->cbDataSize != 0)) ||
        (pProfile->dwType != PROFILE_FILENAME &&
         pProfile->dwType != PROFILE_MEMBUFFER
        )
       )
    {
        WARNING((__TEXT("Invalid parameter to OpenColorProfile\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (pProfile->dwType == PROFILE_FILENAME)
    {
        //
        // Convert the profile name to Unicode and call the Unicode version
        //

        wProfile.dwType = pProfile->dwType;

        if (!pProfile->pProfileData || pProfile->cbDataSize == 0)
        {
            WARNING((__TEXT("Invalid parameter to OpenColorProfile\n")));
            SetLastError(ERROR_INVALID_PARAMETER);
            return NULL;
        }

        wProfile.pProfileData = (WCHAR*)MemAlloc(pProfile->cbDataSize * sizeof(WCHAR));
        if (!wProfile.pProfileData)
        {
            WARNING((__TEXT("Error allocating memory for Unicode profile structure\n")));
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }

        if (! MultiByteToWideChar(CP_ACP, 0, pProfile->pProfileData, pProfile->cbDataSize,
            wProfile.pProfileData, pProfile->cbDataSize))
        {
            WARNING((__TEXT("Error converting profile structure to Unicode\n")));
            goto EndOpenColorProfileA;
        }

        wProfile.cbDataSize = pProfile->cbDataSize * sizeof(WCHAR);
    }
    else
    {
        //
        // It is a memory based profile, so no ANSI/Unicode conversion
        //

        wProfile = *pProfile;
    }

    rc = InternalOpenColorProfile(&wProfile, dwDesiredAccess,
            dwShareMode, dwCreationMode);

EndOpenColorProfileA:
    if (pProfile->dwType == PROFILE_FILENAME)
    {
        MemFree(wProfile.pProfileData);
    }

    return rc;
}

HPROFILE WINAPI OpenColorProfileW(
    PPROFILE pProfile,
    DWORD    dwDesiredAccess,
    DWORD    dwShareMode,
    DWORD    dwCreationMode
    )
{
    //
    // Internal function is Unicode in Windows NT, call it directly.
    //

    return InternalOpenColorProfile(pProfile, dwDesiredAccess,
            dwShareMode, dwCreationMode);
}

#else                           // Windows 95 versions

HPROFILE WINAPI OpenColorProfileA(
    PPROFILE pProfile,
    DWORD    dwDesiredAccess,
    DWORD    dwShareMode,
    DWORD    dwCreationMode
    )
{
    //
    // Internal function is ANSI in Windows 95, call it directly.
    //

    return InternalOpenColorProfile(pProfile, dwDesiredAccess,
            dwShareMode, dwCreationMode);
}

HPROFILE WINAPI OpenColorProfileW(
    PPROFILE pProfile,
    DWORD    dwDesiredAccess,
    DWORD    dwShareMode,
    DWORD    dwCreationMode
    )
{
    PROFILE  aProfile;      // ANSI version
    HPROFILE rc = NULL;
    BOOL     bUsedDefaultChar;

    //
    // Validate parameters before touching them
    //

    if (!pProfile ||
        IsBadReadPtr(pProfile, sizeof(PROFILE)) ||
        (pProfile->pProfileData &&
         IsBadReadPtr(pProfile->pProfileData, pProfile->cbDataSize)) ||
        (!pProfile->pProfileData &&
         (pProfile->cbDataSize != 0)) ||
        (pProfile->dwType != PROFILE_FILENAME &&
         pProfile->dwType != PROFILE_MEMBUFFER
        )
       )
    {
        WARNING((__TEXT("Invalid parameter to OpenColorProfile\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (pProfile->dwType == PROFILE_FILENAME)
    {
        //
        // Convert the profile name to ANSI and call the ANSI version
        //

        aProfile.dwType = pProfile->dwType;

        if (!pProfile->pProfileData || pProfile->cbDataSize == 0)
        {
            WARNING((__TEXT("Invalid parameter to OpenColorProfile\n")));
            SetLastError(ERROR_INVALID_PARAMETER);
            return NULL;
        }

        aProfile.pProfileData = (char*)MemAlloc(pProfile->cbDataSize * sizeof(char));
        if (!aProfile.pProfileData)
        {
            WARNING((__TEXT("Error allocating memory for ANSI profile structure\n")));
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }

        if (! WideCharToMultiByte(CP_ACP, 0, pProfile->pProfileData, pProfile->cbDataSize/sizeof(WCHAR),
            aProfile.pProfileData, pProfile->cbDataSize,
            NULL, &bUsedDefaultChar) || bUsedDefaultChar)
        {
            WARNING((__TEXT("Error converting profile structure to ANSI\n")));
            goto EndOpenColorProfileW;
        }

        aProfile.cbDataSize = pProfile->cbDataSize * sizeof(char);
    }
    else
    {
        //
        // It is a memory based profile, so no ANSI/Unicode conversion
        //

        aProfile = *pProfile;
    }

    rc = InternalOpenColorProfile(&aProfile, dwDesiredAccess,
            dwShareMode, dwCreationMode);

EndOpenColorProfileW:
    if (pProfile->dwType == PROFILE_FILENAME)
    {
        MemFree(aProfile.pProfileData);
    }

    return rc;
}

#endif                          // ! UNICODE

/******************************************************************************
 *
 *                            CloseColorProfile
 *
 *  Function:
 *       This functions closes a color profile object, and frees all memory
 *       associated with it.
 *
 *  Arguments:
 *       hProfile - handle identifing the profile object
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL WINAPI CloseColorProfile(
    HPROFILE hProfile
    )
{
    TRACEAPI((__TEXT("CloseColorProfile\n")));

    //
    // Validate parameters
    //

    if (!ValidHandle(hProfile, OBJ_PROFILE))
    {
        WARNING((__TEXT("Invalid parameter to CloseColorProfile\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return FreeProfileObject(hProfile);
}


/******************************************************************************
 *
 *                          GetColorProfileFromHandle
 *
 *  Function:
 *       This functions returns a buffer filled with the profile contents.
 *
 *  Arguments:
 *       hProfile     - handle identifing the profile object
 *       pProfileData - pointer to buffer to receive the data. Can be NULL.
 *       pcbData      - pointer to size of buffer. On return it is size
 *                      filled/needed.
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL WINAPI GetColorProfileFromHandle(
    HPROFILE  hProfile,
    PBYTE     pProfileData,
    PDWORD    pcbSize
    )
{
    PPROFOBJ pProfObj;
    DWORD    dwFileSize;
    BOOL     rc = FALSE;

    //
    // ValidatePrameters
    //

    if (!ValidHandle(hProfile, OBJ_PROFILE) ||
        !pcbSize ||
        IsBadWritePtr(pcbSize, sizeof(DWORD)) ||
        (pProfileData &&
         IsBadWritePtr(pProfileData, *pcbSize)))
    {
        WARNING((__TEXT("Invalid parameter to CloseColorProfile\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pProfObj = (PPROFOBJ)HDLTOPTR(hProfile);

    ASSERT(pProfObj != NULL);

    dwFileSize = FIX_ENDIAN(HEADER(pProfObj)->phSize);

    if (pProfileData && *pcbSize >= dwFileSize)
    {
        CopyMemory(pProfileData, pProfObj->pView, dwFileSize);
        rc = TRUE;
    }
    else
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }

    *pcbSize = dwFileSize;

    return rc;
}


/******************************************************************************
 *
 *                            IsColorProfileValid
 *
 *  Function:
 *       This functions checks if a given profile is a valid ICC profile
 *       that can be used for color matching
 *
 *  Arguments:
 *       hProfile - handle identifing the profile object
 *       pbValid  - Pointer to variable that receives TRUE if it is a
 *                  valid profile, FALSE otherwise
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL WINAPI IsColorProfileValid(
    HPROFILE hProfile,
    PBOOL    pbValid
    )
{
    PPROFOBJ pProfObj;
    PCMMOBJ  pCMMObj;
    DWORD    cmmID;
    BOOL     rc = FALSE;

    TRACEAPI((__TEXT("IsColorProfileValid\n")));

    //
    // Validate parameters
    //

    if (!ValidHandle(hProfile, OBJ_PROFILE) ||
        !pbValid ||
        IsBadWritePtr(pbValid, sizeof(BOOL)))
    {
        WARNING((__TEXT("Invalid parameter to IsColorProfileValid\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pProfObj = (PPROFOBJ)HDLTOPTR(hProfile);

    ASSERT(pProfObj != NULL);

    //
    // Quick check on the integrity of the profile before calling the CMM
    //

    if (!ValidProfile(pProfObj))
    {
        *pbValid = FALSE;
        return TRUE;
    }

    //
    // Get CMM associated with profile. If it does not exist or does not
    // support the CMValidate function, get default CMM.
    //

    cmmID = HEADER(pProfObj)->phCMMType;
    cmmID = FIX_ENDIAN(cmmID);

    pCMMObj  = GetColorMatchingModule(cmmID);
    if (!pCMMObj || !pCMMObj->fns.pCMIsProfileValid)
    {
        TERSE((__TEXT("CMM associated with profile could not be used")));

        if (pCMMObj)
        {
            ReleaseColorMatchingModule(pCMMObj);
        }

        pCMMObj = GetColorMatchingModule(CMM_WINDOWS_DEFAULT);
        if (!pCMMObj || !pCMMObj->fns.pCMIsProfileValid)
        {
            RIP((__TEXT("Default CMM doesn't support CMValidateProfile")));
            SetLastError(ERROR_INVALID_CMM);
            goto EndIsColorProfileValid;
        }
    }

    ASSERT(pProfObj->pView != NULL);
    rc = pCMMObj->fns.pCMIsProfileValid(hProfile, pbValid);

EndIsColorProfileValid:

    if (pCMMObj)
    {
        ReleaseColorMatchingModule(pCMMObj);
    }

    return rc;
}


/******************************************************************************
 *
 *                         CreateProfileFromLogColorSpace
 *
 *  Function:
 *       These are the ANSI & Unicode wrappers for InternalCreateProfileFromLCS.
 *       Please see InternalCreateProfileFromLCS for more details on this
 *       function.
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

#ifdef UNICODE          // Windows NT versions

BOOL WINAPI CreateProfileFromLogColorSpaceA(
    LPLOGCOLORSPACEA pLogColorSpace,
    PBYTE            *pBuffer
    )
{
    LOGCOLORSPACEW lcs;

    //
    // Validate Parameters
    //

    if (! pLogColorSpace ||
        IsBadReadPtr(pLogColorSpace, sizeof(LOGCOLORSPACEA)) ||
        pLogColorSpace->lcsFilename[0] != '\0')
    {
        WARNING((__TEXT("Invalid parameter to CreateProfileFromLogColorSpace\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *((LPLOGCOLORSPACEA)&lcs) = *pLogColorSpace;
    lcs.lcsFilename[0] = '\0';

    return InternalCreateProfileFromLCS(&lcs, pBuffer, FALSE);
}

BOOL WINAPI CreateProfileFromLogColorSpaceW(
    LPLOGCOLORSPACEW pLogColorSpace,
    PBYTE           *pBuffer
    )
{
    //
    // Internal function is Unicode in Windows NT, call it directly.
    //

    return InternalCreateProfileFromLCS(pLogColorSpace, pBuffer, TRUE);
}

#else                           // Windows 95 versions

BOOL WINAPI CreateProfileFromLogColorSpaceA(
    LPLOGCOLORSPACEA pLogColorSpace,
    PBYTE            *pBuffer
    )
{
    //
    // Internal function is ANSI in Windows 95, call it directly.
    //

    return InternalCreateProfileFromLCS(pLogColorSpace, pBuffer, TRUE);
}

BOOL WINAPI CreateProfileFromLogColorSpaceW(
    LPLOGCOLORSPACEW pLogColorSpace,
    PBYTE           *pBuffer
    )
{
    LOGCOLORSPACEA lcs;

    //
    // Validate Parameters
    //

    if (! pLogColorSpace ||
        IsBadReadPtr(pLogColorSpace, sizeof(LOGCOLORSPACEW)) ||
        pLogColorSpace->lcsFilename[0] != '\0')
    {
        WARNING((__TEXT("Invalid parameter to CreateProfileFromLogColorSpace\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    lcs = *((LPLOGCOLORSPACEA)pLogColorSpace);
    lcs.lcsFilename[0] = '\0';

    return InternalCreateProfileFromLCS(&lcs, pBuffer, FALSE);
}

#endif                          // ! UNICODE

/******************************************************************************
 *
 *                            IsColorProfileTagPresent
 *
 *  Function:
 *       This functions checks if a given tag is present in the profile
 *
 *  Arguments:
 *       hProfile - handle identifing the profile object
 *       tagType  - the tag to check for
 *       pbPrent  - pointer to variable that receives TRUE if it the tag is
 *                  present, FALSE otherwise
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL WINAPI IsColorProfileTagPresent(
    HPROFILE hProfile,
    TAGTYPE  tagType,
    PBOOL    pbPresent
    )
{
    PPROFOBJ pProfObj;
    PTAGDATA pTagData;
    DWORD    nCount, i;

    TRACEAPI((__TEXT("IsColorProfileTagPresent\n")));

    //
    // Validate parameters
    //

    if (!ValidHandle(hProfile, OBJ_PROFILE) ||
        !pbPresent ||
        IsBadWritePtr(pbPresent, sizeof(BOOL)))
    {
        WARNING((__TEXT("Invalid parameter to IsColorProfileTagPresent\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pProfObj = (PPROFOBJ)HDLTOPTR(hProfile);

    ASSERT(pProfObj != NULL);

    //
    // Check the integrity of the profile
    //

    if (!ValidProfile(pProfObj))
    {
        WARNING((__TEXT("Invalid profile passed to IsColorProfileTagPresent\n")));
        SetLastError(ERROR_INVALID_PROFILE);
        return FALSE;
    }

    //
    // Get count of tag items - it is right after the profile header
    //

    nCount = TAG_COUNT(pProfObj);
    nCount = FIX_ENDIAN(nCount);

    VERBOSE((__TEXT("Profile 0x%x has 0x%x(%d) tags\n"), hProfile, nCount, nCount));

    //
    // Tag data records follow the count.
    //

    pTagData = TAG_DATA(pProfObj);

    //
    // Check if any of these records match the tag passed in.
    //

    *pbPresent = FALSE;
    tagType = FIX_ENDIAN(tagType);      // to match tags in profile
    for (i=0; i<nCount; i++)
    {
        if (pTagData->tagType == tagType)
        {
            *pbPresent = TRUE;
            break;
        }
        pTagData++;                     // Next record
    }

    return TRUE;
}


/******************************************************************************
 *
 *                            GetCountColorProfileElements
 *
 *  Function:
 *       This functions returns the number of tagged elements in the profile
 *
 *  Arguments:
 *       hProfile - handle identifing the profile object
 *       pnCount  - pointer to variable to receive number of tagged elements
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL WINAPI GetCountColorProfileElements(
    HPROFILE hProfile,
    PDWORD   pnCount
    )
{
    PPROFOBJ pProfObj;

    TRACEAPI((__TEXT("GetCountColorProfileElements\n")));

    //
    // Validate parameters
    //

    if (!ValidHandle(hProfile, OBJ_PROFILE) ||
        !pnCount ||
        IsBadWritePtr(pnCount, sizeof(DWORD)))
    {
        WARNING((__TEXT("Invalid parameter to GetCountColorProfileElements\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pProfObj = (PPROFOBJ)HDLTOPTR(hProfile);

    ASSERT(pProfObj != NULL);

    //
    // Check the integrity of the profile
    //

    if (!ValidProfile(pProfObj))
    {
        WARNING((__TEXT("Invalid profile passed to GetCountColorProfileElements\n")));
        SetLastError(ERROR_INVALID_PROFILE);
        return FALSE;
    }

    //
    // Get count of tag items - it is right after the profile header
    //

    *pnCount = TAG_COUNT(pProfObj);
    *pnCount = FIX_ENDIAN(*pnCount);

    return TRUE;
}


/******************************************************************************
 *
 *                            GetColorProfileElementTag
 *
 *  Function:
 *       This functions retrieves the tag name of the dwIndex'th element
 *       in the profile
 *
 *  Arguments:
 *       hProfile - handle identifing the profile object
 *       dwIndex  - one-based index of the tag whose name is required
 *       pTagType - gets the name of the tag on return
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL WINAPI GetColorProfileElementTag(
    HPROFILE  hProfile,
    DWORD     dwIndex,
    PTAGTYPE  pTagType
    )
{
    PPROFOBJ pProfObj;
    PTAGDATA pTagData;
    DWORD    nCount;
    BOOL     rc = FALSE;

    TRACEAPI((__TEXT("GetColorProfileElementTag\n")));

    //
    // Validate parameters
    //

    if (!ValidHandle(hProfile, OBJ_PROFILE) ||
        IsBadWritePtr(pTagType, sizeof(TAGTYPE)) ||
        dwIndex <= 0)
    {
        WARNING((__TEXT("Invalid parameter to GetColorProfileElementTag\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pProfObj = (PPROFOBJ)HDLTOPTR(hProfile);

    ASSERT(pProfObj != NULL);

    //
    // Check the integrity of the profile
    //

    if (!ValidProfile(pProfObj))
    {
        WARNING((__TEXT("Invalid profile passed to GetColorProfileElementTag\n")));
        SetLastError(ERROR_INVALID_PROFILE);
        return FALSE;
    }

    //
    // Get count of tag items - it is right after the profile header
    //

    nCount = TAG_COUNT(pProfObj);
    nCount = FIX_ENDIAN(nCount);

    if (dwIndex > nCount)
    {
        WARNING((__TEXT("GetColorProfileElementTag:index (%d) invalid\n"), dwIndex));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Tag data records follow the count.
    //

    pTagData = TAG_DATA(pProfObj);

    *pTagType = FIX_ENDIAN(pTagData[dwIndex-1].tagType);    // 1-based index

    return TRUE;
}


/******************************************************************************
 *
 *                            GetColorProfileElement
 *
 *  Function:
 *       This functions retrieves the data that a tagged element refers to
 *       starting from the given offset.
 *
 *  Arguments:
 *       hProfile - handle identifing the profile object
 *       tagType  - the tag name of the element whose data is required
 *       dwOffset - offset within the element data from which to retrieve
 *       pcbSize  - number of bytes to get. On return it is the number of
 *                  bytes retrieved
 *       pBuffer  - pointer to buffer to recieve data
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 *  Comments:
 *       If pBuffer is NULL, it returns size of data in *pcbSize and ignores
 *       dwOffset.
 *
 ******************************************************************************/

BOOL WINAPI GetColorProfileElement(
    HPROFILE hProfile,
    TAGTYPE  tagType,
    DWORD    dwOffset,
    PDWORD   pcbSize,
    PVOID    pBuffer,
    PBOOL    pbReference
    )
{
    PPROFOBJ pProfObj;
    PTAGDATA pTagData;
    DWORD    nCount, nBytes, i;
    BOOL     found;
    BOOL     rc = FALSE;            // Assume failure

    TRACEAPI((__TEXT("GetColorProfileElement\n")));

    //
    // Validate parameters
    //

    if (!ValidHandle(hProfile, OBJ_PROFILE) ||
        !pcbSize ||
        IsBadWritePtr(pcbSize, sizeof(DWORD)) ||
        !pbReference ||
        IsBadWritePtr(pbReference, sizeof(BOOL))
       )
    {
        WARNING((__TEXT("Invalid parameter to GetColorProfileElement\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pProfObj = (PPROFOBJ)HDLTOPTR(hProfile);

    ASSERT(pProfObj != NULL);

    //
    // Check the integrity of the profile
    //

    if (!ValidProfile(pProfObj))
    {
        WARNING((__TEXT("Invalid profile passed to GetColorProfileElement\n")));
        SetLastError(ERROR_INVALID_PROFILE);
        return FALSE;
    }

    //
    // Get count of tag items - it is right after the profile header
    //

    nCount = TAG_COUNT(pProfObj);
    nCount = FIX_ENDIAN(nCount);

    //
    // Tag data records follow the count.
    //

    pTagData = TAG_DATA(pProfObj);

    //
    // Check if any of these records match the tag passed in.
    //

    found = FALSE;
    tagType = FIX_ENDIAN(tagType);      // to match tags in profile
    for (i=0; i<nCount; i++)
    {
        if (pTagData->tagType == tagType)
        {
            found = TRUE;
            break;
        }
        pTagData++;                     // next record
    }

    if (found)
    {
        //
        // If pBuffer is NULL, copy size of data
        //

        if (!pBuffer)
        {
            *pcbSize = FIX_ENDIAN(pTagData->cbSize);
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
        else
        {
            //
            // pBuffer is not NULL, get number of bytes to copy
            //

            if (dwOffset >= FIX_ENDIAN(pTagData->cbSize))
            {
                WARNING((__TEXT("dwOffset too large for GetColorProfileElement\n")));
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            else if (dwOffset + *pcbSize > FIX_ENDIAN(pTagData->cbSize))
            {
                nBytes = FIX_ENDIAN(pTagData->cbSize) - dwOffset;
            }
            else
            {
                nBytes = *pcbSize;
            }

            //
            // Check if output buffer is large enough
            //

            if (IsBadWritePtr(pBuffer, nBytes))
            {
                WARNING((__TEXT("Bad buffer passed to GetColorProfileElement\n")));
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }

            //
            // Copy the data into user supplied buffer
            //

            CopyMemory((PVOID)pBuffer,
                (PVOID)(pProfObj->pView + FIX_ENDIAN(pTagData->dwOffset)
                + dwOffset), nBytes);
            *pcbSize = nBytes;
            rc = TRUE;                      // Success!
        }

        //
        // Check if multiple tags reference this tag's data
        //

        *pbReference = IsReferenceTag(pProfObj, pTagData);
    }
    else
    {
        SetLastError(ERROR_TAG_NOT_FOUND);
    }

    return rc;
}


/******************************************************************************
 *
 *                        SetColorProfileElementSize
 *
 *  Function:
 *       This functions sets the data size of a tagged element. If the element
 *       is already present in the profile it's size is changed, and the data
 *       is truncated or extended as the case may be. If the element is not
 *       present, it is created and the data is filled with zeroes.
 *
 *  Arguments:
 *       hProfile - handle identifing the profile object
 *       tagType  - the tag name of the element
 *       cbSize   - new size for the element data
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 *  Comments:
 *       If cbSize is 0, and the element is present, it is deleted.
 *
 ******************************************************************************/

BOOL WINAPI SetColorProfileElementSize(
    HPROFILE  hProfile,
    TAGTYPE   tagType,
    DWORD     cbSize
    )
{
    PTAGDATA pTagData;
    PPROFOBJ pProfObj;
    DWORD    i, nCount, newSize;
    TAGTYPE  rawTag;
    BOOL     found, rc = FALSE;

    TRACEAPI((__TEXT("SetColorProfileElementSize\n")));

    //
    // Validate parameters
    //

    if (!ValidHandle(hProfile, OBJ_PROFILE))
    {
        WARNING((__TEXT("Invalid parameter to SetColorProfileElementSize\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pProfObj = (PPROFOBJ)HDLTOPTR(hProfile);

    ASSERT(pProfObj != NULL);

    //
    // Check the integrity of the profile
    //

    if (!ValidProfile(pProfObj))
    {
        WARNING((__TEXT("Invalid profile passed to SetColorProfileElementSize\n")));
        SetLastError(ERROR_INVALID_PROFILE);
        return FALSE;
    }

    //
    // Check if profile has write access
    //

    if (!(pProfObj->dwFlags & READWRITE_ACCESS))
    {
        WARNING((__TEXT("Writing to a profile without read/write access\n")));
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    //
    // Get count of tag items - it is right after the profile header
    //

    nCount = TAG_COUNT(pProfObj);
    nCount = FIX_ENDIAN(nCount);

    //
    // Tag data records follow the count.
    //

    pTagData = TAG_DATA(pProfObj);

    //
    // Check if any of these records match the tag passed in.
    //

    found = FALSE;
    rawTag = FIX_ENDIAN(tagType);
    for (i=0; i<nCount; i++)
    {
        if (pTagData->tagType == rawTag)
        {
            found = TRUE;
            break;
        }
        pTagData++;                     // Next record
    }

    if (found)
    {
        //
        // If it is a reference tag, create data area for it
        //

        if (IsReferenceTag(pProfObj, pTagData))
        {
            if (cbSize == 0)
            {
                PTAGDATA pTemp;

                //
                // Move everything after the tag table entry up
                // by size of one tag table entry.
                //

                MoveProfileData(pProfObj, (PBYTE)(pTagData+1), (PBYTE)pTagData,
                    PROFILE_SIZE(pProfObj) - (LONG)((PBYTE)(pTagData+1) - VIEW(pProfObj)), TRUE);

                //
                // Go through the tag table and update the pointers
                //

                pTemp = TAG_DATA(pProfObj);

                //
                // Get count of tag items - it is right after the profile header
                //

                nCount = TAG_COUNT(pProfObj);
                nCount = FIX_ENDIAN(nCount) - 1;
                TAG_COUNT(pProfObj) = FIX_ENDIAN(nCount);

                for (i=0; i<nCount; i++)
                {
                    DWORD dwTemp = FIX_ENDIAN(pTemp->dwOffset);

                    dwTemp -= sizeof(TAGDATA);
                    pTemp->dwOffset = FIX_ENDIAN(dwTemp);
                    pTemp++;                     // Next record
                }

                //
                // Use nCount as a placeholder to calculate file size
                //

                nCount = FIX_ENDIAN(HEADER(pProfObj)->phSize) - sizeof(TAGDATA);
                HEADER(pProfObj)->phSize = FIX_ENDIAN(nCount);
            }
            else
            {
                DWORD dwOffset;

                //
                // Resize the profile if needed. For memory buffers, we have to realloc,
                // and for mapped objects, we have to close and reopen the view.
                //

                newSize = FIX_ENDIAN(HEADER(pProfObj)->phSize);
                newSize = DWORD_ALIGN(newSize) + cbSize;

                //
                // Check for overflow
                //

                if (newSize < FIX_ENDIAN(HEADER(pProfObj)->phSize) ||
                    newSize < cbSize)
                {
                    WARNING((__TEXT("Overflow in setting reference element size\n")));
                    SetLastError(ERROR_ARITHMETIC_OVERFLOW);
                    return FALSE;
                }

                if (newSize > pProfObj->dwMapSize)
                {
                    //Sundown: dwOffset should not be over 4gb
                    DWORD dwOffset = (ULONG)(ULONG_PTR)(pTagData - TAG_DATA(pProfObj));

                    if (! GrowProfile(pProfObj, newSize))
                    {
                        return FALSE;
                    }
                    //
                    // Recalculate pointers as mapping or memory pointer
                    // could have changed when growing profile
                    //

                    pTagData = TAG_DATA(pProfObj) + dwOffset;
                }

                //
                // Calculate location of new data - should be DWORD aligned
                //

                dwOffset = DWORD_ALIGN(FIX_ENDIAN(HEADER(pProfObj)->phSize));
                pTagData->dwOffset = FIX_ENDIAN(dwOffset);

                //
                // Set final file size
                //

                HEADER(pProfObj)->phSize = FIX_ENDIAN(dwOffset+cbSize);
            }

            rc = TRUE;
        }
        else
        {
            if (cbSize == 0)
            {
                rc = DeleteTaggedElement(pProfObj, pTagData);
            }
            else
            {
                DWORD cbOldSize;

                //
                // Get current size of element
                //

                cbOldSize = FIX_ENDIAN(pTagData->cbSize);

                //
                // Compress or expand the file as the case may be.
                //

                if (cbSize > cbOldSize)
                {
                    //Sundown: dwOffset should be safe to truncate
                    DWORD dwOffset = (ULONG)(ULONG_PTR)(pTagData - TAG_DATA(pProfObj));

                    //
                    // Check for overflow
                    //

                    newSize = PROFILE_SIZE(pProfObj) + DWORD_ALIGN(cbSize) -
                        DWORD_ALIGN(cbOldSize);

                    if (newSize < PROFILE_SIZE(pProfObj))
                    {
                        WARNING((__TEXT("Overflow in increasing element size\n")));
                        SetLastError(ERROR_ARITHMETIC_OVERFLOW);
                        return FALSE;
                    }

                    if (!GrowProfile(pProfObj, newSize))
                    {
                        return FALSE;
                    }

                    //
                    // Recompute pointers
                    //

                    pTagData = TAG_DATA(pProfObj) + dwOffset;
                }

                rc = ChangeTaggedElementSize(pProfObj, pTagData, cbSize);
            }
        }
    }
    else
    {
        if (cbSize == 0)
        {
            WARNING((__TEXT("SetColorProfileElementSize (deleting): Tag not found\n")));
            SetLastError(ERROR_TAG_NOT_FOUND);
        }
        else
        {
            rc = AddTaggedElement(pProfObj, tagType, cbSize);
        }
    }

    return rc;
}


/******************************************************************************
 *
 *                          SetColorProfileElement
 *
 *  Function:
 *       This functions sets the data for a tagged element. It does not
 *       change the size of the element, and only writes as much data as
 *       would fit within the current size, overwriting any existing data.
 *
 *  Arguments:
 *       hProfile - handle identifing the profile object
 *       tagType  - the tag name of the element
 *       dwOffset - offset within the element data from which to write
 *       pcbSize  - number of bytes to write. On return it is the number of
 *                  bytes written.
 *       pBuffer  - pointer to buffer having data to write
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL WINAPI SetColorProfileElement(
    HPROFILE  hProfile,
    TAGTYPE   tagType,
    DWORD     dwOffset,
    PDWORD    pcbSize,
    PVOID     pBuffer
    )
{
    PPROFOBJ pProfObj;
    PTAGDATA pTagData;
    DWORD    nCount, nBytes, i;
    BOOL     found;
    BOOL     rc = FALSE;            // Assume failure

    TRACEAPI((__TEXT("SetColorProfileElement\n")));

    //
    // Validate parameters
    //

    if (!ValidHandle(hProfile, OBJ_PROFILE) ||
        !pcbSize ||
        IsBadWritePtr(pcbSize, sizeof(DWORD)) ||
        !pBuffer ||
        IsBadReadPtr(pBuffer, *pcbSize)
       )
    {
        WARNING((__TEXT("Invalid parameter to SetColorProfileElement\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pProfObj = (PPROFOBJ)HDLTOPTR(hProfile);

    ASSERT(pProfObj != NULL);

    //
    // Check the integrity of the profile
    //

    if (!ValidProfile(pProfObj))
    {
        WARNING((__TEXT("Invalid profile passed to SetColorProfileElement\n")));
        SetLastError(ERROR_INVALID_PROFILE);
        return FALSE;
    }

    //
    // Check if profile has write access
    //

    if (!(pProfObj->dwFlags & READWRITE_ACCESS))
    {
        WARNING((__TEXT("Writing to a profile without read/write access\n")));
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    //
    // Get count of tag items - it is right after the profile header
    //

    nCount = TAG_COUNT(pProfObj);
    nCount = FIX_ENDIAN(nCount);

    //
    // Tag data records follow the count.
    //

    pTagData = TAG_DATA(pProfObj);

    //
    // Check if any of these records match the tag passed in
    //

    found = FALSE;
    tagType = FIX_ENDIAN(tagType);
    for (i=0; i<nCount; i++)
    {
        if (pTagData->tagType == tagType)
        {
            found = TRUE;
            break;
        }
        pTagData++;                     // Next record
    }

    if (found)
    {
        //
        // If it is a reference tag, create new space for it
        //

        if (IsReferenceTag(pProfObj, pTagData))
        {
            SetColorProfileElementSize(hProfile, tagType, FIX_ENDIAN(pTagData->cbSize));
        }

        //
        // Get number of bytes to set
        //

        if (dwOffset >= FIX_ENDIAN(pTagData->cbSize))
        {
            WARNING((__TEXT("dwOffset too large for SetColorProfileElement\n")));
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        else if (dwOffset + *pcbSize > FIX_ENDIAN(pTagData->cbSize))
        {
            nBytes = FIX_ENDIAN(pTagData->cbSize) - dwOffset;
        }
        else
        {
            nBytes = *pcbSize;
        }

        //
        // Copy the data into the profile
        //

        CopyMemory((PVOID)(pProfObj->pView + FIX_ENDIAN(pTagData->dwOffset)
            + dwOffset), (PVOID)pBuffer, nBytes);
        *pcbSize = nBytes;

        rc = TRUE;
    }
    else
    {
        WARNING((__TEXT("SetColorProfileElement: Tag not found\n")));
        SetLastError(ERROR_TAG_NOT_FOUND);
    }

    return rc;
}


/******************************************************************************
 *
 *                       SetColorProfileElementReference
 *
 *  Function:
 *       This functions creates a new tag and makes it refer to the same
 *       data as an existing tag.
 *
 *  Arguments:
 *       hProfile - handle identifing the profile object
 *       newTag   - new tag to create
 *       refTag   - reference tag whose data newTag should refer to
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL WINAPI SetColorProfileElementReference(
    HPROFILE hProfile,
    TAGTYPE  newTag,
    TAGTYPE  refTag
    )
{
    PPROFOBJ pProfObj;
    PTAGDATA pTagData, pOrigTag;
    DWORD    nCount, i;
    BOOL     found;
    BOOL     rc = FALSE;            // Assume failure

    TRACEAPI((__TEXT("SetColorProfileElementReference\n")));

    //
    // Validate parameters
    //

    if (!ValidHandle(hProfile, OBJ_PROFILE))
    {
        WARNING((__TEXT("Invalid parameter to SetColorProfileElementReference\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pProfObj = (PPROFOBJ)HDLTOPTR(hProfile);

    ASSERT(pProfObj != NULL);

    //
    // Check the integrity of the profile
    //

    if (!ValidProfile(pProfObj))
    {
        WARNING((__TEXT("Invalid profile passed to SetColorProfileElementReference\n")));
        SetLastError(ERROR_INVALID_PROFILE);
        return FALSE;
    }

    //
    // Check if profile has write access
    //

    if (!(pProfObj->dwFlags & READWRITE_ACCESS))
    {
        WARNING((__TEXT("Writing to a profile without read/write access\n")));
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    //
    // Get count of tag items - it is right after the profile header
    //
    nCount = TAG_COUNT(pProfObj);
    nCount = FIX_ENDIAN(nCount);

    //
    // Tag data records follow the count.
    //

    pTagData = TAG_DATA(pProfObj);

    //
    // Check if any of these records match the tag passed in
    //

    found = FALSE;
    refTag = FIX_ENDIAN(refTag);
    for (i=0; i<nCount; i++)
    {
        if (pTagData->tagType == refTag)
        {
            pOrigTag = pTagData;
            found = TRUE;
        }

        if (pTagData->tagType == FIX_ENDIAN(newTag))
        {
            WARNING((__TEXT("Duplicate tag present in SetColorProfileElementReference %x\n"), newTag));
            SetLastError(ERROR_DUPLICATE_TAG);
            return FALSE;
        }
        pTagData++;                     // Next record
    }

    if (found)
    {
        DWORD newSize;

        //
        // Grow profile if needed
        //

        newSize = FIX_ENDIAN(HEADER(pProfObj)->phSize);
        newSize = DWORD_ALIGN(newSize) + sizeof(TAGDATA);

        //
        // Check for overflow
        //

        if (newSize < FIX_ENDIAN(HEADER(pProfObj)->phSize))
        {
            WARNING((__TEXT("Overflow in adding element\n")));
            SetLastError(ERROR_ARITHMETIC_OVERFLOW);
            return FALSE;
        }

        if (newSize > pProfObj->dwMapSize)
        {
            //Sundown: safe truncation
            DWORD dwOffset = (ULONG)(ULONG_PTR)(pOrigTag - TAG_DATA(pProfObj));

            if (! GrowProfile(pProfObj, newSize))
            {
                return FALSE;
            }
            //
            // Recalculate pointers as mapping or memory pointer
            // could have changed when growing profile
            //

            pOrigTag = TAG_DATA(pProfObj) + dwOffset;
        }

        rc = AddTagTableEntry(pProfObj, newTag, FIX_ENDIAN(pOrigTag->dwOffset),
            FIX_ENDIAN(pOrigTag->cbSize), FALSE);
    }
    else
    {
        WARNING((__TEXT("SetColorProfileElementReference: Tag 0x%x not found\n"),
            FIX_ENDIAN(refTag)));       // Re-fix it to reflect data passed in
        SetLastError(ERROR_TAG_NOT_FOUND);
    }

    return rc;
}


/******************************************************************************
 *
 *                       GetColorProfileHeader
 *
 *  Function:
 *       This functions retrieves the header of a profile
 *
 *  Arguments:
 *       hProfile - handle identifing the profile object
 *       pHeader  - pointer to buffer to recieve the header
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL WINAPI GetColorProfileHeader(
    HPROFILE       hProfile,
    PPROFILEHEADER pHeader
    )
{
    PPROFOBJ pProfObj;
    DWORD    nCount, i, temp;

    TRACEAPI((__TEXT("GetColorProfileHeader\n")));

    //
    // Validate parameters
    //

    if (!ValidHandle(hProfile, OBJ_PROFILE) ||
        IsBadWritePtr(pHeader, sizeof(PROFILEHEADER)))
    {
        WARNING((__TEXT("Invalid parameter to GetColorProfileHeader\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pProfObj = (PPROFOBJ)HDLTOPTR(hProfile);

    ASSERT(pProfObj != NULL);

    //
    // Check the integrity of the profile.
    //

    if (!ValidProfile(pProfObj))
    {
        WARNING((__TEXT("Invalid profile passed to GetColorProfileHeader\n")));
        SetLastError(ERROR_INVALID_PROFILE);
        return FALSE;
    }

    CopyMemory((PVOID)pHeader, (PVOID)pProfObj->pView,
        sizeof(PROFILEHEADER));

    //
    // Fix up all fields to platform specific values.
    // The following code assumes that the profile header is a
    // multiple of DWORDs which it is!
    //

    ASSERT(sizeof(PROFILEHEADER) % sizeof(DWORD) == 0);

    nCount = sizeof(PROFILEHEADER)/sizeof(DWORD);
    for (i=0; i<nCount;i++)
    {
        temp = (DWORD)((PDWORD)pHeader)[i];
        ((PDWORD)pHeader)[i] = FIX_ENDIAN(temp);
    }

    return TRUE;
}


/******************************************************************************
 *
 *                       SetColorProfileHeader
 *
 *  Function:
 *       This functions sets the header of a profile
 *
 *  Arguments:
 *       hProfile - handle identifing the profile object
 *       pHeader  - pointer to buffer identifing the header
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL WINAPI SetColorProfileHeader(
    HPROFILE       hProfile,
    PPROFILEHEADER pHeader
    )
{
    PPROFOBJ pProfObj;
    DWORD    nCount, i, temp;

    TRACEAPI((__TEXT("SetColorProfileHeader\n")));

    //
    // Validate parameters
    //

    if (!ValidHandle(hProfile, OBJ_PROFILE) ||
        IsBadReadPtr(pHeader, sizeof(PROFILEHEADER)))
    {
        WARNING((__TEXT("Invalid parameter to SetColorProfileHeader\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pProfObj = (PPROFOBJ)HDLTOPTR(hProfile);

    ASSERT(pProfObj != NULL);

    //
    // Check if profile has write access
    //

    if (!(pProfObj->dwFlags & READWRITE_ACCESS))
    {
        WARNING((__TEXT("Writing to a profile without read/write access\n")));
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    //
    // Fix up all fields to BIG-ENDIAN.
    // The following code assumes that the profile header is a
    // multiple of DWORDs which it is!
    //

    ASSERT(sizeof(PROFILEHEADER) % sizeof(DWORD) == 0);

    nCount = sizeof(PROFILEHEADER)/sizeof(DWORD);
    for (i=0; i<nCount;i++)
    {
        temp = (DWORD)((PDWORD)pHeader)[i];
        ((PDWORD)pHeader)[i] = FIX_ENDIAN(temp);
    }

    CopyMemory((PVOID)pProfObj->pView, (PVOID)pHeader,
        sizeof(PROFILEHEADER));

    //
    // Put back app supplied buffer the way it came in
    //

    for (i=0; i<nCount;i++)
    {
        temp = (DWORD)((PDWORD)pHeader)[i];
        ((PDWORD)pHeader)[i] = FIX_ENDIAN(temp);
    }

    return TRUE;
}


/******************************************************************************
 *
 *                        GetPS2ColorSpaceArray
 *
 *  Function:
 *       This functions retrieves the PostScript Level 2 CSA from the profile
 *
 *  Arguments:
 *       hProfile  - handle identifing the profile object
 *       dwIntent  - rendering intent of CSA
 *       dwCSAType - type of CSA
 *       pbuffer   - pointer to receive the CSA
 *       pcbSize   - pointer to size of buffer. If function fails because
 *                   buffer is not big enough, it is filled with required size.
 *       pcbBinary - TRUE if binary data is requested. On return it is set to
 *                   reflect the data returned
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL WINAPI
GetPS2ColorSpaceArray (
    HPROFILE  hProfile,
    DWORD     dwIntent,
    DWORD     dwCSAType,
    PBYTE     pBuffer,
    PDWORD    pcbSize,
    LPBOOL    pbBinary
    )
{
    PPROFOBJ pProfObj;
    PCMMOBJ  pCMMObj;
    DWORD    cmmID;
    BOOL     rc;

    TRACEAPI((__TEXT("GetPS2ColorSpaceArray\n")));

    //
    // Validate parameters
    //

    if (!ValidHandle(hProfile, OBJ_PROFILE)   ||
        IsBadWritePtr(pcbSize, sizeof(DWORD)) ||
        (pBuffer &&
         IsBadWritePtr(pBuffer, *pcbSize)
        )                                     ||
        IsBadWritePtr(pbBinary, sizeof(BOOL))
       )
    {
        WARNING((__TEXT("Invalid parameter to GetPS2ColorSpaceArray\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pProfObj = (PPROFOBJ)HDLTOPTR(hProfile);

    ASSERT(pProfObj != NULL);

    //
    // Check the integrity of the profile
    //

    if (!ValidProfile(pProfObj))
    {
        WARNING((__TEXT("Invalid profile passed to GetPS2ColorSpaceArray\n")));
        SetLastError(ERROR_INVALID_PROFILE);
        return FALSE;
    }

    //
    // Check if application specified CMM is present
    //

    pCMMObj = GetPreferredCMM();
    if (!pCMMObj || (pCMMObj->dwFlags & CMM_DONT_USE_PS2_FNS) ||
        !pCMMObj->fns.pCMGetPS2ColorSpaceArray)
    {
        if (pCMMObj)
        {
            pCMMObj->dwFlags |= CMM_DONT_USE_PS2_FNS;
            ReleaseColorMatchingModule(pCMMObj);
        }

        //
        // Get CMM associated with profile. If it does not exist or does not
        // support the CMGetPS2ColorSpaceArray function, get default CMM.
        //

        cmmID = HEADER(pProfObj)->phCMMType;
        cmmID = FIX_ENDIAN(cmmID);

        pCMMObj  = GetColorMatchingModule(cmmID);

        if (!pCMMObj || (pCMMObj->dwFlags & CMM_DONT_USE_PS2_FNS) ||
            !pCMMObj->fns.pCMGetPS2ColorSpaceArray)
        {
            TERSE((__TEXT("CMM associated with profile could not be used")));

            if (pCMMObj)
            {
                pCMMObj->dwFlags |= CMM_DONT_USE_PS2_FNS;
                ReleaseColorMatchingModule(pCMMObj);
            }

            pCMMObj = GetColorMatchingModule(CMM_WINDOWS_DEFAULT);

            if (!pCMMObj || !pCMMObj->fns.pCMGetPS2ColorSpaceArray)
            {
                WARNING((__TEXT("Default CMM doesn't support CMGetPS2ColorSpaceArray\n")));
                if (pCMMObj)
                {
                    ReleaseColorMatchingModule(pCMMObj);
                    pCMMObj = NULL;
                }
            }
        }
    }

    ASSERT(pProfObj->pView != NULL);

    if (pCMMObj)
    {
        rc = pCMMObj->fns.pCMGetPS2ColorSpaceArray(hProfile, dwIntent, dwCSAType,
            pBuffer, pcbSize, pbBinary);
    }
    else
    {
        rc = InternalGetPS2ColorSpaceArray(pProfObj->pView, dwIntent,
            dwCSAType, pBuffer, pcbSize, pbBinary);
    }

    if (pCMMObj)
    {
        ReleaseColorMatchingModule(pCMMObj);
    }

    VERBOSE((__TEXT("GetPS2ColorSpaceArray returning %s\n"),
        rc ? "TRUE" : "FALSE"));

    return rc;
}


/******************************************************************************
 *
 *                       GetPS2ColorRenderingIntent
 *
 *  Function:
 *       This functions retrieves the PostScript Level 2 color rendering intent
 *       from the profile
 *
 *  Arguments:
 *       hProfile  - handle identifing the profile object
 *       pbuffer   - pointer to receive the color rendering intent
 *       pcbSize   - pointer to size of buffer. If function fails because
 *                   buffer is not big enough, it is filled with required size.
 *       pcbBinary - TRUE if binary data is requested. On return it is set to
 *                   reflect the data returned
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL WINAPI GetPS2ColorRenderingIntent(
    HPROFILE  hProfile,
    DWORD     dwIntent,
    PBYTE     pBuffer,
    PDWORD    pcbSize
    )
{
    PPROFOBJ pProfObj;
    PCMMOBJ  pCMMObj;
    DWORD    cmmID;
    BOOL     rc;

    TRACEAPI((__TEXT("GetPS2ColorRenderingIntent\n")));

    //
    // Validate parameters
    //

    if (!ValidHandle(hProfile, OBJ_PROFILE) ||
        IsBadWritePtr(pcbSize, sizeof(DWORD)) ||
        (pBuffer &&
         IsBadWritePtr(pBuffer, *pcbSize)
        )
       )
    {
        WARNING((__TEXT("Invalid parameter to GetPS2ColorRenderingIntent\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pProfObj = (PPROFOBJ)HDLTOPTR(hProfile);

    ASSERT(pProfObj != NULL);

    //
    // Check the integrity of the profile
    //

    if (!ValidProfile(pProfObj))
    {
        WARNING((__TEXT("Invalid profile passed to GetPS2ColorRenderingIntent\n")));
        SetLastError(ERROR_INVALID_PROFILE);
        return FALSE;
    }

    //
    // Check if application specified CMM is present
    //

    pCMMObj = GetPreferredCMM();
    if (!pCMMObj || (pCMMObj->dwFlags & CMM_DONT_USE_PS2_FNS) ||
        !pCMMObj->fns.pCMGetPS2ColorRenderingIntent)
    {
        if (pCMMObj)
        {
            pCMMObj->dwFlags |= CMM_DONT_USE_PS2_FNS;
            ReleaseColorMatchingModule(pCMMObj);
        }

        //
        // Get CMM associated with profile. If it does not exist or does not
        // support the CMGetPS2ColorSpaceArray function, get default CMM.
        //

        cmmID = HEADER(pProfObj)->phCMMType;
        cmmID = FIX_ENDIAN(cmmID);

        pCMMObj  = GetColorMatchingModule(cmmID);

        if (!pCMMObj || (pCMMObj->dwFlags & CMM_DONT_USE_PS2_FNS) ||
            !pCMMObj->fns.pCMGetPS2ColorRenderingIntent)
        {
            TERSE((__TEXT("CMM associated with profile could not be used")));

            if (pCMMObj)
            {
                pCMMObj->dwFlags |= CMM_DONT_USE_PS2_FNS;
                ReleaseColorMatchingModule(pCMMObj);
            }

            pCMMObj = GetColorMatchingModule(CMM_WINDOWS_DEFAULT);

            if (!pCMMObj || !pCMMObj->fns.pCMGetPS2ColorRenderingIntent)
            {
                WARNING((__TEXT("Default CMM doesn't support CMGetPS2ColorRenderingIntent\n")));
                if (pCMMObj)
                {
                    ReleaseColorMatchingModule(pCMMObj);
                    pCMMObj = NULL;
                }
            }
        }
    }

    ASSERT(pProfObj->pView != NULL);

    if (pCMMObj)
    {
        rc = pCMMObj->fns.pCMGetPS2ColorRenderingIntent(hProfile,
            dwIntent, pBuffer, pcbSize);
    }
    else
    {
        rc = InternalGetPS2ColorRenderingIntent(pProfObj->pView, dwIntent,
            pBuffer, pcbSize);
    }

    if (pCMMObj)
    {
        ReleaseColorMatchingModule(pCMMObj);
    }

    VERBOSE((__TEXT("GetPS2ColorRenderingIntent returning %s\n"),
        rc ? "TRUE" : "FALSE"));

    return rc;
}


/******************************************************************************
 *
 *                       GetPS2ColorRenderingDictionary
 *
 *  Function:
 *       This functions retrieves the PostScript Level 2 CRD from the profile
 *
 *  Arguments:
 *       hProfile  - handle identifing the profile object
 *       dwIntent  - intent whose CRD is required
 *       pbuffer   - pointer to receive the CSA
 *       pcbSize   - pointer to size of buffer. If function fails because
 *                   buffer is not big enough, it is filled with required size.
 *       pcbBinary - TRUE if binary data is requested. On return it is set to
 *                   reflect the data returned
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL WINAPI GetPS2ColorRenderingDictionary(
    HPROFILE  hProfile,
    DWORD     dwIntent,
    PBYTE     pBuffer,
    PDWORD    pcbSize,
    PBOOL     pbBinary
    )
{
    PPROFOBJ pProfObj;
    PCMMOBJ  pCMMObj;
    DWORD    cmmID;
    BOOL     rc;

    TRACEAPI((__TEXT("GetPS2ColorRenderingDictionary\n")));

    //
    // Validate parameters
    //

    if (!ValidHandle(hProfile, OBJ_PROFILE)     ||
        IsBadWritePtr(pcbSize, sizeof(DWORD))   ||
        (pBuffer &&
         IsBadWritePtr(pBuffer, *pcbSize)
        )                                       ||
        IsBadWritePtr(pbBinary, sizeof(BOOL))
       )
    {
        WARNING((__TEXT("Invalid parameter to GetPS2ColorRenderingDictionary\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pProfObj = (PPROFOBJ)HDLTOPTR(hProfile);

    ASSERT(pProfObj != NULL);

    //
    // Check the integrity of the profile
    //

    if (!ValidProfile(pProfObj))
    {
        WARNING((__TEXT("Invalid profile passed to GetPS2ColorRenderingDictionary\n")));
        SetLastError(ERROR_INVALID_PROFILE);
        return FALSE;
    }

    //
    // Check if application specified CMM is present
    //

    pCMMObj = GetPreferredCMM();
    if (!pCMMObj || (pCMMObj->dwFlags & CMM_DONT_USE_PS2_FNS) ||
        !pCMMObj->fns.pCMGetPS2ColorRenderingDictionary)
    {
        if (pCMMObj)
        {
            pCMMObj->dwFlags |= CMM_DONT_USE_PS2_FNS;
            ReleaseColorMatchingModule(pCMMObj);
        }

        //
        // Get CMM associated with profile. If it does not exist or does not
        // support the CMGetPS2ColorSpaceArray function, get default CMM.
        //

        cmmID = HEADER(pProfObj)->phCMMType;
        cmmID = FIX_ENDIAN(cmmID);

        pCMMObj  = GetColorMatchingModule(cmmID);

        if (!pCMMObj || (pCMMObj->dwFlags & CMM_DONT_USE_PS2_FNS) ||
            !pCMMObj->fns.pCMGetPS2ColorRenderingDictionary)
        {
            TERSE((__TEXT("CMM associated with profile could not be used")));

            if (pCMMObj)
            {
                pCMMObj->dwFlags |= CMM_DONT_USE_PS2_FNS;
                ReleaseColorMatchingModule(pCMMObj);
            }

            pCMMObj = GetColorMatchingModule(CMM_WINDOWS_DEFAULT);

            if (!pCMMObj || !pCMMObj->fns.pCMGetPS2ColorRenderingDictionary)
            {
                WARNING((__TEXT("Default CMM doesn't support CMGetPS2ColorRenderingDictionary\n")));
                if (pCMMObj)
                {
                    ReleaseColorMatchingModule(pCMMObj);
                    pCMMObj = NULL;
                }
            }
        }
    }

    ASSERT(pProfObj->pView != NULL);

    if (pCMMObj)
    {
        rc = pCMMObj->fns.pCMGetPS2ColorRenderingDictionary(hProfile, dwIntent,
            pBuffer, pcbSize, pbBinary);
    }
    else
    {
        rc = InternalGetPS2ColorRenderingDictionary(pProfObj->pView, dwIntent,
            pBuffer, pcbSize, pbBinary);
    }

    if (pCMMObj)
    {
        ReleaseColorMatchingModule(pCMMObj);
    }

    VERBOSE((__TEXT("GetPS2ColorRenderingDictionary returning %s\n"),
        rc ? "TRUE" : "FALSE"));

    return rc;
}


/******************************************************************************
 *
 *                         GetNamedProfileInfo
 *
 *  Function:
 *       This functions returns information about the given named color space
 *       profile. If fails if the given profile is not a named color space profile.
 *
 *  Arguments:
 *       hProfile          - handle identifying the named color space profile object
 *       pNamedProfileInfo - pointer to NAMED_PROFILE_INFO structure that is
 *                           filled on retun if successful.
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL WINAPI GetNamedProfileInfo(
    HPROFILE              hProfile,
    PNAMED_PROFILE_INFO   pNamedProfileInfo
    )
{
    PPROFOBJ pProfObj;
    PCMMOBJ  pCMMObj;
    DWORD    cmmID;
    BOOL     rc;

    TRACEAPI((__TEXT("GetNamedProfileInfo\n")));

    //
    // Validate parameters
    //

    if (!ValidHandle(hProfile, OBJ_PROFILE)     ||
        !pNamedProfileInfo                      ||
        IsBadWritePtr(pNamedProfileInfo, sizeof(NAMED_PROFILE_INFO)))
    {
        WARNING((__TEXT("Invalid parameter to GetNamedProfileInfo\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pProfObj = (PPROFOBJ)HDLTOPTR(hProfile);

    ASSERT(pProfObj != NULL);

    //
    // Check the integrity of the profile
    //

    if (!ValidProfile(pProfObj))
    {
        WARNING((__TEXT("Invalid profile passed to GetNamedProfileInfo\n")));
        SetLastError(ERROR_INVALID_PROFILE);
        return FALSE;
    }

    //
    // Check if application specified CMM is present
    //

    pCMMObj = GetPreferredCMM();
    if (!pCMMObj ||
        !pCMMObj->fns.pCMGetNamedProfileInfo)
    {
        if (pCMMObj)
        {
            ReleaseColorMatchingModule(pCMMObj);
        }

        //
        // Get CMM associated with profile. If it does not exist or does not
        // support the CMGetNamedProfileInfo function, get default CMM.
        //

        cmmID = HEADER(pProfObj)->phCMMType;
        cmmID = FIX_ENDIAN(cmmID);

        pCMMObj  = GetColorMatchingModule(cmmID);

        if (!pCMMObj ||
            !pCMMObj->fns.pCMGetNamedProfileInfo)
        {
            TERSE((__TEXT("CMM associated with profile could not be used")));

            if (pCMMObj)
            {
                ReleaseColorMatchingModule(pCMMObj);
            }

            pCMMObj = GetColorMatchingModule(CMM_WINDOWS_DEFAULT);

            if (!pCMMObj || !pCMMObj->fns.pCMGetNamedProfileInfo)
            {
                RIP((__TEXT("Default CMM doesn't support CMValidateProfile")));
                SetLastError(ERROR_INVALID_CMM);
                if (pCMMObj)
                {
                    ReleaseColorMatchingModule(pCMMObj);
                }
                return FALSE;
            }
        }
    }

    ASSERT(pProfObj->pView != NULL);

    rc = pCMMObj->fns.pCMGetNamedProfileInfo(hProfile, pNamedProfileInfo);

    ReleaseColorMatchingModule(pCMMObj);

    return rc;
}


/******************************************************************************
 *
 *                       ConvertColorNameToIndex
 *
 *  Function:
 *       This functions converts a given array of color names to color indices
 *       using the specified named color space profile
 *
 *  Arguments:
 *       hProfile     - handle identifing the named color space profile object
 *       paColorName  - pointer to array of COLOR_NAME structures
 *       paIndex      - pointer to array of DWORDs to receive the indices
 *       dwCount      - number of color names to convert
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL WINAPI ConvertColorNameToIndex(
    HPROFILE      hProfile,
    PCOLOR_NAME   paColorName,
    PDWORD        paIndex,
    DWORD         dwCount
    )
{
    PPROFOBJ pProfObj;
    PCMMOBJ  pCMMObj;
    DWORD    cmmID;
    BOOL     rc;

    TRACEAPI((__TEXT("ConvertColorNameToIndex\n")));

    //
    // Validate parameters
    //

    if (!ValidHandle(hProfile, OBJ_PROFILE)     ||
        !paColorName                            ||
        dwCount == 0                            ||
        IsBadReadPtr(paColorName, dwCount*sizeof(COLOR_NAME)) ||
        !paIndex                                ||
        IsBadWritePtr(paIndex, dwCount*sizeof(DWORD)))
    {
        WARNING((__TEXT("Invalid parameter to ConvertColorNameToIndex\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pProfObj = (PPROFOBJ)HDLTOPTR(hProfile);

    ASSERT(pProfObj != NULL);

    //
    // Check the integrity of the profile
    //

    if (!ValidProfile(pProfObj))
    {
        WARNING((__TEXT("Invalid profile passed to ConvertColorNameToIndex\n")));
        SetLastError(ERROR_INVALID_PROFILE);
        return FALSE;
    }

    //
    // Check if application specified CMM is present
    //

    pCMMObj = GetPreferredCMM();
    if (!pCMMObj ||
        !pCMMObj->fns.pCMConvertColorNameToIndex)
    {
        if (pCMMObj)
        {
            ReleaseColorMatchingModule(pCMMObj);
        }

        //
        // Get CMM associated with profile. If it does not exist or does not
        // support the CMConvertColorNameToIndex function, get default CMM.
        //

        cmmID = HEADER(pProfObj)->phCMMType;
        cmmID = FIX_ENDIAN(cmmID);

        pCMMObj  = GetColorMatchingModule(cmmID);

        if (!pCMMObj ||
            !pCMMObj->fns.pCMConvertColorNameToIndex)
        {
            TERSE((__TEXT("CMM associated with profile could not be used")));

            if (pCMMObj)
            {
                ReleaseColorMatchingModule(pCMMObj);
            }

            pCMMObj = GetColorMatchingModule(CMM_WINDOWS_DEFAULT);

            if (!pCMMObj || !pCMMObj->fns.pCMConvertColorNameToIndex)
            {
                RIP((__TEXT("Default CMM doesn't support CMConvertColorNameToIndex")));
                SetLastError(ERROR_INVALID_CMM);
                if (pCMMObj)
                {
                    ReleaseColorMatchingModule(pCMMObj);
                }
                return FALSE;
            }
        }
    }

    ASSERT(pProfObj->pView != NULL);

    rc = pCMMObj->fns.pCMConvertColorNameToIndex(
                            hProfile,
                            paColorName,
                            paIndex,
                            dwCount);

    ReleaseColorMatchingModule(pCMMObj);

    return rc;
}


/******************************************************************************
 *
 *                       ConvertIndexToColorName
 *
 *  Function:
 *       This functions converts a given array of color indices to color names
 *       using the specified named color space profile
 *
 *  Arguments:
 *       hProfile     - handle identifing the named color space profile object
 *       paIndex      - pointer to array of color indices
 *       paColorName  - pointer to array of COLOR_NAME structures
 *       dwCount      - number of color indices to convert
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL WINAPI ConvertIndexToColorName(
    HPROFILE     hProfile,
    PDWORD       paIndex,
    PCOLOR_NAME  paColorName,
    DWORD        dwCount
    )
{
    PPROFOBJ pProfObj;
    PCMMOBJ  pCMMObj;
    DWORD    cmmID;
    BOOL     rc;

    TRACEAPI((__TEXT("ConvertIndexToColorName\n")));

    //
    // Validate parameters
    //

    if (!ValidHandle(hProfile, OBJ_PROFILE)     ||
        !paIndex                                ||
        dwCount == 0                            ||
        IsBadWritePtr(paIndex, dwCount*sizeof(DWORD))          ||
        !paColorName                            ||
        IsBadReadPtr(paColorName, dwCount*sizeof(COLOR_NAME)))
    {
        WARNING((__TEXT("Invalid parameter to ConvertIndexToColorName\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pProfObj = (PPROFOBJ)HDLTOPTR(hProfile);

    ASSERT(pProfObj != NULL);

    //
    // Check the integrity of the profile
    //

    if (!ValidProfile(pProfObj))
    {
        WARNING((__TEXT("Invalid profile passed to ConvertIndexToColorName\n")));
        SetLastError(ERROR_INVALID_PROFILE);
        return FALSE;
    }

    //
    // Check if application specified CMM is present
    //

    pCMMObj = GetPreferredCMM();
    if (!pCMMObj ||
        !pCMMObj->fns.pCMConvertIndexToColorName)
    {
        if (pCMMObj)
        {
            ReleaseColorMatchingModule(pCMMObj);
        }

        //
        // Get CMM associated with profile. If it does not exist or does not
        // support the CMConvertIndexToColorName function, get default CMM.
        //

        cmmID = HEADER(pProfObj)->phCMMType;
        cmmID = FIX_ENDIAN(cmmID);

        pCMMObj  = GetColorMatchingModule(cmmID);

        if (!pCMMObj ||
            !pCMMObj->fns.pCMConvertIndexToColorName)
        {
            TERSE((__TEXT("CMM associated with profile could not be used")));

            if (pCMMObj)
            {
                ReleaseColorMatchingModule(pCMMObj);
            }

            pCMMObj = GetColorMatchingModule(CMM_WINDOWS_DEFAULT);

            if (!pCMMObj || !pCMMObj->fns.pCMConvertIndexToColorName)
            {
                RIP((__TEXT("Default CMM doesn't support CMConvertIndexToColorName")));
                SetLastError(ERROR_INVALID_CMM);
                if (pCMMObj)
                {
                    ReleaseColorMatchingModule(pCMMObj);
                }
                return FALSE;
            }
        }
    }

    ASSERT(pProfObj->pView != NULL);

    rc = pCMMObj->fns.pCMConvertIndexToColorName(
                            hProfile,
                            paIndex,
                            paColorName,
                            dwCount);

    ReleaseColorMatchingModule(pCMMObj);

    return rc;
}


/******************************************************************************
 *
 *                         CreateDeviceLinkProfile
 *
 *  Function:
 *       This functions creates a device link profile from the given set
 *       of profiles, using specified intents.
 *
 *  Arguments:
 *       pahProfiles       - pointer to array of handles of profiles
 *       nProfiles         - number of profiles in array
 *       padwIntent        - array of intents to use
 *       nIntents          - size of array - can be 1 or nProfiles
 *       dwFlags           - optimization flags
 *       pProfileData      - pointer to pointer to buffer to receive data which
 *                           this function allocates; caller needs to free.
 *       indexPreferredCMM - zero based index of profile which specifies
 *                           preferred CMM to use.
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL  WINAPI CreateDeviceLinkProfile(
    PHPROFILE   pahProfiles,
    DWORD       nProfiles,
    PDWORD      padwIntent,
    DWORD       nIntents,
    DWORD       dwFlags,
    PBYTE      *pProfileData,
    DWORD       indexPreferredCMM
    )
{
    PPROFOBJ      pProfObj;
    PCMMOBJ       pCMMObj;
    DWORD         cmmID, i;
    BOOL          rc;

    TRACEAPI((__TEXT("CreateDeviceLinkProfile\n")));

    //
    // Validate parameters
    //

    if (nProfiles <= 1 ||
        indexPreferredCMM >= nProfiles ||
        pahProfiles == NULL ||
        IsBadReadPtr(pahProfiles, sizeof(HPROFILE) * nProfiles) ||
        padwIntent == NULL ||
        ((nIntents != nProfiles) && (nIntents != 1)) ||
        IsBadReadPtr(padwIntent, nIntents * sizeof(DWORD)))
    {
        WARNING((__TEXT("Invalid parameter to CreateDeviceLinkProfile\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    for (i=0; i<nProfiles; i++)
    {
        if ((pahProfiles[i] == NULL) ||
            ! ValidHandle(pahProfiles[i], OBJ_PROFILE))

        {
            WARNING((__TEXT("Invalid profile passed to CreateDeviceLinkProfile\n")));
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        pProfObj = (PPROFOBJ)HDLTOPTR(pahProfiles[i]);

        ASSERT(pProfObj != NULL);

        ASSERT(pProfObj->pView != NULL);

        //
        // Quick check on the integrity of the profile
        //

        if (!ValidProfile(pProfObj))
        {
            WARNING((__TEXT("Invalid profile passed to CreateDeviceLinkProfile\n")));
            SetLastError(ERROR_INVALID_PROFILE);
            return FALSE;
        }

        if (i == indexPreferredCMM)
        {
            //
            // Get ID of preferred CMM
            //

            cmmID = HEADER(pProfObj)->phCMMType;
            cmmID = FIX_ENDIAN(cmmID);
        }
    }

    //
    // Get CMM associated with preferred profile. If it does not exist,
    // get default CMM.
    //

    pCMMObj  = GetColorMatchingModule(cmmID);
    if (!pCMMObj || !pCMMObj->fns.pCMCreateDeviceLinkProfile)
    {
        TERSE((__TEXT("CMM associated with profile could not be used")));

        if (pCMMObj)
        {
            ReleaseColorMatchingModule(pCMMObj);
        }

        pCMMObj = GetColorMatchingModule(CMM_WINDOWS_DEFAULT);
        if (!pCMMObj)
        {
            RIP((__TEXT("Default CMM not found\n")));
            SetLastError(ERROR_INVALID_CMM);
            return FALSE;
        }
    }

    ASSERT(pCMMObj->fns.pCMCreateDeviceLinkProfile != NULL);

    rc = pCMMObj->fns.pCMCreateDeviceLinkProfile(
                            pahProfiles,
                            nProfiles,
                            padwIntent,
                            nIntents,
                            dwFlags,
                            pProfileData);

    ReleaseColorMatchingModule(pCMMObj);

    return rc;
}


/*****************************************************************************/
/***************************** Internal Functions ****************************/
/*****************************************************************************/

/******************************************************************************
 *
 *                          InternalOpenColorProfile
 *
 *  Function:
 *       This functions opens a color profile specified by the pProfile
 *       parameter, creates an internal profile object, and returns a handle
 *       to it.
 *
 *  Arguments:
 *       pProfile - ptr to a profile structure that specifies the profile
 *                  to open
 *       dwDesiredAccess - specifies the mode in which to open the profile.
 *                         Any combination of these values can be used:
 *            PROFILE_READ: Allows the app to read the profile.
 *            PROFILE_READWRITE: Allows the app to read and write to the profile.
 *       dwShareMode - specifies the mode to share the profile with other
 *                     processes if it is a file. Any combination of these
 *                     values can be used.
 *            0               : Prevents the file from being shared.
 *            FILE_SHARE_READ: Allows opening for read only by other processes.
 *            FILE_SHARE_WRITE: Allows opening for write by other processes.
 *       dwCreationMode - specifies which actions to take on the profile while
 *                        opening it (if it is a file). Any one of the following
 *                        values can be used.
 *            CREATE_NEW: Creates a new file. Fails if one exists already.
 *            CREATE_ALWAYS: Always create a new file. Overwriting existing.
 *            OPEN_EXISTING: Open exisiting file. Fail if not found.
 *            OPEN_ALWAYS: Open existing. If not found, create a new one.
 *            TRUNCATE_EXISTING: Open existing and truncate to zero bytes. Fail
 *                               if not found.
 *
 *  Returns:
 *       Handle to open profile on success, zero on failure.
 *
 ******************************************************************************/

HPROFILE InternalOpenColorProfile(
    PPROFILE pProfile,
    DWORD    dwDesiredAccess,
    DWORD    dwShareMode,
    DWORD    dwCreationMode
    )
{
    SECURITY_ATTRIBUTES sa;
    PPROFOBJ  pProfObj;
    HPROFILE  hProfile;
    DWORD     dwMapSize;
    BOOL      bNewFile = FALSE;
    BOOL      bError = TRUE;      // Assume failure

    TRACEAPI((__TEXT("OpenColorProfile\n")));

    //
    // Validate parameters
    //

    if (!pProfile ||
        IsBadReadPtr(pProfile, sizeof(PROFILE)) ||
        (pProfile->pProfileData &&
         IsBadReadPtr(pProfile->pProfileData, pProfile->cbDataSize)) ||
        (!pProfile->pProfileData &&
         (pProfile->cbDataSize != 0)) ||
        (pProfile->dwType != PROFILE_FILENAME &&
         pProfile->dwType != PROFILE_MEMBUFFER
        ) ||
        (dwDesiredAccess != PROFILE_READ &&
         dwDesiredAccess != PROFILE_READWRITE
        )
       )
    {
        WARNING((__TEXT("Invalid parameter to OpenColorProfile\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    //
    // Allocate an object on the heap for the profile
    //

    hProfile = AllocateHeapObject(OBJ_PROFILE);
    if (!hProfile)
    {
        WARNING((__TEXT("Could not allocate profile object\n")));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    pProfObj = (PPROFOBJ)HDLTOPTR(hProfile);

    ASSERT(pProfObj != NULL);

    //
    // Copy profile information to our object
    //

    pProfObj->objHdr.dwUseCount = 1;
    pProfObj->dwType       = pProfile->dwType;
    pProfObj->cbDataSize   = pProfile->cbDataSize;
    pProfObj->pProfileData = (PBYTE)MemAlloc(pProfile->cbDataSize + sizeof(TCHAR));
    if (!pProfObj->pProfileData)
    {
        WARNING((__TEXT("Could not allocate memory for profile data\n")));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto EndOpenColorProfile;
    }

    CopyMemory((PVOID)pProfObj->pProfileData,
        (PVOID)pProfile->pProfileData,
        pProfile->cbDataSize);

    if (pProfObj->dwType == PROFILE_FILENAME)
    {
        LPTSTR lpFilename;

        if (!pProfile->pProfileData ||
             pProfile->cbDataSize == 0 ||
             lstrlen((LPCTSTR)pProfile->pProfileData) > MAX_PATH ||
             pProfile->cbDataSize > (MAX_PATH * sizeof(TCHAR)))
        {
            WARNING((__TEXT("Invalid parameter to OpenColorProfile\n")));
            SetLastError(ERROR_INVALID_PARAMETER);
            goto EndOpenColorProfile;
        }

        //
        // If only filename is given, it is wrt color directory
        //

        lpFilename = GetFilenameFromPath((LPTSTR)pProfObj->pProfileData);
        if (lpFilename == pProfObj->pProfileData)
        {
            DWORD dwLen = MAX_PATH;
            lpFilename = MemAlloc(dwLen);
            if (!lpFilename)
            {
                WARNING((__TEXT("Could not allocate memory for file name\n")));
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto EndOpenColorProfile;
            }
            (VOID)GetColorDirectory(NULL, lpFilename, &dwLen);
            lstrcat(lpFilename, gszBackslash);
            lstrcat(lpFilename, (LPTSTR)pProfObj->pProfileData);
            MemFree(pProfObj->pProfileData);
            pProfObj->pProfileData = (PVOID)lpFilename;
            pProfObj->cbDataSize = MAX_PATH;
        }

        //
        // File name already null terminates as we used GHND flag which
        // zero initializes the allocated memory
        //
        // Create file mapping
        //

        pProfObj->dwFlags |= MEMORY_MAPPED;

        if (dwCreationMode == OPEN_ALWAYS)
        {
            //
            // If we find a zero length profile, we should error out
            // saying it is a bad profile. If we create a zero length
            // profile, it is fine. To distinguish these two cases, we
            // check for file's existence and if present, change the
            // creation mode to OPEN_EXISTING
            //

            if (GetFileAttributes(pProfObj->pProfileData) != (DWORD)-1)
            {
                dwCreationMode = OPEN_EXISTING;
            }
        }

        //
        // Set security attribute structure
        //

        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = NULL;         // default security
        sa.bInheritHandle = FALSE;

        pProfObj->hFile = CreateFile(pProfObj->pProfileData,
            (dwDesiredAccess == PROFILE_READWRITE) ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ,
            dwShareMode, &sa, dwCreationMode, FILE_FLAG_RANDOM_ACCESS, 0);

        if (pProfObj->hFile == INVALID_HANDLE_VALUE)
        {
            WARNING((__TEXT("Err %ld, could not open profile %s\n"),
                GetLastError(), pProfObj->pProfileData));
            goto EndOpenColorProfile;
        }

        //
        // Get size of file mapping. Add a cushion so that profile can
        // be grown. When closing the profile, the file size is truncated
        // to the size of the actual data. If the profile size grows beyond
        // the cushion, it is continually grown in chunks.
        //

        dwMapSize = GetFileSize(pProfObj->hFile, NULL);
        if (dwMapSize == 0)
        {
            if (dwCreationMode == OPEN_EXISTING)
            {
                WARNING((__TEXT("Invalid profile  - zero length\n")));
                SetLastError(ERROR_INVALID_PROFILE);
                goto EndOpenColorProfile;

            }
            else
            {
                dwMapSize = PROFILE_GROWTHCUSHION;
                bNewFile = TRUE;
            }
        }

        pProfObj->hMap = CreateFileMapping(pProfObj->hFile, 0,
            (dwDesiredAccess == PROFILE_READWRITE) ? PAGE_READWRITE : PAGE_READONLY,
            0, dwMapSize, 0);

        if (!pProfObj->hMap)
        {
            WARNING((__TEXT("Err %ld, could not create map of profile %s\n"),
                GetLastError(), pProfObj->pProfileData));
            goto EndOpenColorProfile;
        }

        pProfObj->dwMapSize = dwMapSize;

        pProfObj->pView = (PBYTE)MapViewOfFile(pProfObj->hMap,
            (dwDesiredAccess == PROFILE_READWRITE) ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ,
            0, 0, 0);

        if (!pProfObj->pView)
        {
            WARNING((__TEXT("Err %ld, could not create view of profile %s\n"),
                GetLastError(), pProfObj->pProfileData));
            goto EndOpenColorProfile;
        }

        //
        // If a new profile has been created, initialize size
        // and tag table count
        //
        if (bNewFile && dwDesiredAccess == PROFILE_READWRITE)
        {
            HEADER(pProfObj)->phSize = FIX_ENDIAN(sizeof(PROFILEHEADER) +
                                                  sizeof(DWORD));
            HEADER(pProfObj)->phVersion = 0x02000000;
            HEADER(pProfObj)->phSignature = PROFILE_SIGNATURE;
            TAG_COUNT(pProfObj) = 0;
        }
    }
    else
    {
        if (pProfile->cbDataSize == 0)
        {
            //
            // Allocate a small buffer and create a new profile in it
            //

            pProfObj->cbDataSize = PROFILE_GROWTHCUSHION;
            MemFree(pProfObj->pProfileData);
            pProfObj->pView = pProfObj->pProfileData = MemAlloc(pProfObj->cbDataSize);
            if (!pProfObj->pView)
            {
                WARNING((__TEXT("Could not allocate memory for profile data\n")));
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto EndOpenColorProfile;
            }

            HEADER(pProfObj)->phSize = FIX_ENDIAN(sizeof(PROFILEHEADER) +
                                                  sizeof(DWORD));
            HEADER(pProfObj)->phVersion = 0x02000000;
            HEADER(pProfObj)->phSignature = PROFILE_SIGNATURE;
            TAG_COUNT(pProfObj) = 0;
            pProfObj->dwMapSize = pProfObj->cbDataSize;
        }
        else
        {
            //
            // Treat buffer as view of file
            //

            pProfObj->pView = pProfObj->pProfileData;
            pProfObj->dwMapSize = pProfObj->cbDataSize;

            //
            // Do a sanity check on the profile
            //

            if (!ValidProfile(pProfObj))
            {
                WARNING((__TEXT("Invalid profile passed to OpenColorProfile\n")));
                SetLastError(ERROR_INVALID_PROFILE);
                goto EndOpenColorProfile;
            }
        }
    }

    if (dwDesiredAccess == PROFILE_READWRITE)
        pProfObj->dwFlags |= READWRITE_ACCESS;

    bError = FALSE;          // Success!

EndOpenColorProfile:

    if (bError)
    {
        if (hProfile)
            FreeProfileObject(hProfile);
        hProfile = NULL;
    }

    return hProfile;
}


/******************************************************************************
 *
 *                         InternalCreateProfileFromLCS
 *
 *  Function:
 *       This functions takes a logical color space and creates an ICC profile
 *
 *  Arguments:
 *       pLogColorSpace - Pointer to LogColorSpace structure
 *       pBuffer - Pointer to pointer to buffer. This function allocates and
 *                 fills this buffer with the profile on success.
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL InternalCreateProfileFromLCS(
    LPLOGCOLORSPACE pLogColorSpace,
    PBYTE          *pBuffer,
    BOOL           bValidateParams
    )
{
    PCMMOBJ  pCMMObj = NULL;
    BOOL     rc;

    //
    // Validate parameters
    //

    if (bValidateParams &&
        (! pLogColorSpace ||
         IsBadReadPtr(pLogColorSpace, sizeof(LOGCOLORSPACE)) ||
         pLogColorSpace->lcsFilename[0] != '\0'))
    {
        WARNING((__TEXT("Invalid parameter to CreateProfileFromLogColorSpace\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // We use the default CMM for this
    //

    pCMMObj = GetColorMatchingModule(CMM_WINDOWS_DEFAULT);
    if (!pCMMObj)
    {
        RIP((__TEXT("Default CMM not found\n")));
        SetLastError(ERROR_INVALID_CMM);
        return FALSE;
    }


    ASSERT(pCMMObj->fns.pCMCreateProfile != NULL);

    rc = pCMMObj->fns.pCMCreateProfile(pLogColorSpace, pBuffer);

    ReleaseColorMatchingModule(pCMMObj);

    return rc;
}


/******************************************************************************
 *
 *                       FreeProfileObject
 *
 *  Function:
 *       This functions frees a profile object and associated memory
 *
 *  Arguments:
 *       hProfile  - handle identifing the profile object to free
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL FreeProfileObject(
    HANDLE   hProfile
    )
{
    PPROFOBJ pProfObj;
    DWORD    dwFileSize;
    DWORD    dwErr;

    ASSERT(hProfile != NULL);

    pProfObj = (PPROFOBJ)HDLTOPTR(hProfile);

    ASSERT(pProfObj != NULL);

    dwErr = GetLastError();     // remember any errors we may have set

    //
    // Free memory associated with profile data
    //

    if (pProfObj->pProfileData)
        MemFree((PVOID)pProfObj->pProfileData);

    //
    // If it a memory mapped profile, unmap it
    //

    if (pProfObj->dwFlags & MEMORY_MAPPED)
    {
        if (pProfObj->pView)
        {
            dwFileSize = FIX_ENDIAN(HEADER(pProfObj)->phSize);
            UnmapViewOfFile(pProfObj->pView);
        }
        if (pProfObj->hMap)
            CloseHandle(pProfObj->hMap);

        if (pProfObj->hFile)
        {

            //
            // Set the size of the file correctly
            //

            SetFilePointer(pProfObj->hFile, dwFileSize, NULL, FILE_BEGIN);
            SetEndOfFile(pProfObj->hFile);
            CloseHandle(pProfObj->hFile);
        }
    }

    //
    // Free heap object
    //

    pProfObj->objHdr.dwUseCount--;      // decrement before freeing
    FreeHeapObject(hProfile);

    if (dwErr)
    {
        SetLastError(dwErr);            // reset our error
    }

    return TRUE;
}


/******************************************************************************
 *
 *                       GrowProfile
 *
 *  Function:
 *       This functions grows a profile to the new size
 *
 *  Arguments:
 *       pProfObj  - pointer to profile object
 *       dwNewSize - new size for the profile
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL GrowProfile(
    PPROFOBJ pProfObj,
    DWORD dwNewSize
    )
{
    if (pProfObj->dwMapSize >= dwNewSize)
        return TRUE;

    //
    // Add cushion for future growth
    //

    dwNewSize += PROFILE_GROWTHCUSHION;

    if (pProfObj->dwFlags & MEMORY_MAPPED)
    {
        //
        // Profile is a memory mapped file
        //

        //
        // Close previous view and map
        //

        UnmapViewOfFile(pProfObj->pView);
        CloseHandle(pProfObj->hMap);

        pProfObj->hMap = CreateFileMapping(pProfObj->hFile, 0,
            PAGE_READWRITE, 0, dwNewSize, 0);

        if (!pProfObj->hMap)
        {
            WARNING((__TEXT("Err %ld, could not recreate map of profile %s\n"),
                GetLastError(), pProfObj->pProfileData));
            return FALSE;
        }

        pProfObj->pView = (PBYTE) MapViewOfFile(pProfObj->hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if (!pProfObj->pView)
        {
            WARNING((__TEXT("Err %ld, could not recreate view of profile %s\n"),
                GetLastError(), pProfObj->pProfileData));
            return FALSE;
        }


        //
        // Set new size
        //

        pProfObj->dwMapSize = dwNewSize;
    }
    else
    {
        //
        // Profile is an in-memory buffer
        //

        PVOID pTemp = MemReAlloc(pProfObj->pView, dwNewSize);

        if (!pTemp)
        {
            WARNING((__TEXT("Error reallocating memory\n")));
            return FALSE;
        }

        pProfObj->pView = pProfObj->pProfileData = pTemp;
        pProfObj->cbDataSize = pProfObj->dwMapSize = dwNewSize;
    }

    return TRUE;
}


/******************************************************************************
 *
 *                       AddTagTableEntry
 *
 *  Function:
 *       This functions adds a tag to the tag table
 *
 *  Arguments:
 *       pProfObj  - pointer to profile object
 *       tagType   - tag to add
 *       dwOffset  - offset of tag data from start of file
 *       cbSize    - size of tag data
 *       bNewData  - TRUE if new tag is not a reference to existing data
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL AddTagTableEntry(
    PPROFOBJ pProfObj,
    TAGTYPE  tagType,
    DWORD    dwOffset,
    DWORD    cbSize,
    BOOL     bNewData
    )
{
    PTAGDATA pTagData;
    PBYTE    src, dest;
    DWORD    nCount;
    DWORD    cnt, i;

    //
    // Get count of tag items - it is right after the profile header
    //

    nCount = TAG_COUNT(pProfObj);
    nCount = FIX_ENDIAN(nCount);

    //
    // Increase count of tag elements by 1, and add new tag to end of
    // tag table. Move all data below tag table downwards by the size
    // of one tag table entry.
    //

    nCount++;

    dest = (PBYTE)TAG_DATA(pProfObj) + nCount * sizeof(TAGDATA);
    src  = (PBYTE)TAG_DATA(pProfObj) + (nCount - 1) * sizeof(TAGDATA);

    //
    // # bytes to move = file size - header - tag count - tag table
    //

    cnt  = FIX_ENDIAN(HEADER(pProfObj)->phSize) - sizeof(PROFILEHEADER) -
                 sizeof(DWORD) - (nCount - 1) * sizeof(TAGDATA);

    if (cnt > 0)
    {
        //
        // NOTE: CopyMemory() doesn't work for overlapped memory.
        // Use internal function instead.
        //

        MyCopyMemory((PVOID)dest, (PVOID)src, cnt);
    }

    TAG_COUNT(pProfObj) = FIX_ENDIAN(nCount);

    pTagData = (PTAGDATA)src;
    pTagData->tagType  = FIX_ENDIAN(tagType);
    pTagData->cbSize   = FIX_ENDIAN(cbSize);
    pTagData->dwOffset =  FIX_ENDIAN(dwOffset);

    //
    // Go through the tag table and update the offsets of all elements
    // by the size of one tag table entry that we inserted.
    //

    pTagData = TAG_DATA(pProfObj);
    for (i=0; i<nCount; i++)
    {
        cnt = FIX_ENDIAN(pTagData->dwOffset);
        cnt += sizeof(TAGDATA);
        pTagData->dwOffset = FIX_ENDIAN(cnt);
        pTagData++;     // Next element
    }

    //
    // Set final file size
    //

    cnt = DWORD_ALIGN(FIX_ENDIAN(HEADER(pProfObj)->phSize)) + sizeof(TAGDATA);
    if (bNewData)
    {
        //
        // The new tag is not a reference to an old tag. Increment
        // file size of size of new data also
        //

        cnt += cbSize;
    }
    HEADER(pProfObj)->phSize = FIX_ENDIAN(cnt);

    return TRUE;
}


/******************************************************************************
 *
 *                       AddTaggedElement
 *
 *  Function:
 *       This functions adds a new tagged element to a profile
 *
 *  Arguments:
 *       pProfObj  - pointer to profile object
 *       tagType   - tag to add
 *       cbSize    - size of tag data
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL AddTaggedElement(
    PPROFOBJ pProfObj,
    TAGTYPE  tagType,
    DWORD    cbSize
    )
{
    DWORD    dwOffset, newSize;

    ASSERT(pProfObj != NULL);
    ASSERT(cbSize > 0);

    //
    // Resize the profile if needed. For memory buffers, we have to realloc,
    // and for mapped objects, we have to close and reopen the view.
    //

    newSize = FIX_ENDIAN(HEADER(pProfObj)->phSize);
    newSize = DWORD_ALIGN(newSize) + sizeof(TAGDATA) + cbSize;

    //
    // Check for overflow
    //

    if (newSize < FIX_ENDIAN(HEADER(pProfObj)->phSize) ||
        newSize < cbSize)
    {
        WARNING((__TEXT("Overflow in adding element\n")));
        SetLastError(ERROR_ARITHMETIC_OVERFLOW);
        return FALSE;
    }

    if (newSize > pProfObj->dwMapSize)
    {
        if (! GrowProfile(pProfObj, newSize))
        {
            return FALSE;
        }
    }

    //
    // Calculate location of new data - should be DWORD aligned
    //

    dwOffset = FIX_ENDIAN(HEADER(pProfObj)->phSize);
    dwOffset = DWORD_ALIGN(dwOffset);

    return AddTagTableEntry(pProfObj, tagType, dwOffset, cbSize, TRUE);
}


/******************************************************************************
 *
 *                       DeleteTaggedElement
 *
 *  Function:
 *       This functions deletes a tagged element from the profile
 *
 *  Arguments:
 *       pProfObj  - pointer to profile object
 *       pTagData  - pointer to tagged element to delete
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL DeleteTaggedElement(
    PPROFOBJ pProfObj,
    PTAGDATA pTagData
    )
{
    PBYTE    pData;
    PTAGDATA pTemp;
    DWORD    cbSize, nCount, dwOffset, i;

    //
    // Remember location of data and move everything upto the data upwards
    // by size of one tag table entry. Then move everything below the tag data
    // upward by size of data plus size of one tage table entry.
    //

    pData = VIEW(pProfObj) + FIX_ENDIAN(pTagData->dwOffset);
    cbSize = FIX_ENDIAN(pTagData->cbSize);
    cbSize = DWORD_ALIGN(cbSize);
    dwOffset = FIX_ENDIAN(pTagData->dwOffset);

    MoveProfileData(pProfObj, (PBYTE)(pTagData+1), (PBYTE)pTagData,
        (LONG)(pData-(PBYTE)(pTagData+1)), FALSE);

    //
    // Do not attempt to move data past the tag if we are deleting the last tag
    //

    if (pData + cbSize < VIEW(pProfObj) + PROFILE_SIZE(pProfObj))
    {
        MoveProfileData(pProfObj, pData+cbSize, pData-sizeof(TAGDATA),
            PROFILE_SIZE(pProfObj)-(LONG)(pData - VIEW(pProfObj)) - cbSize, TRUE);
    }

    //
    // Get count of tag items - it is right after the profile header, and
    // decrement it by one.
    //

    nCount = TAG_COUNT(pProfObj);
    nCount = FIX_ENDIAN(nCount) - 1;
    TAG_COUNT(pProfObj) = FIX_ENDIAN(nCount);

    //
    // Go through the tag table and update the pointers
    //

    pTemp = TAG_DATA(pProfObj);

    for (i=0; i<nCount; i++)
    {
        DWORD dwTemp = FIX_ENDIAN(pTemp->dwOffset);

        if (dwTemp > dwOffset)
        {
            dwTemp -= cbSize;        // cbSize already DWORD aligned
        }
        dwTemp -= sizeof(TAGDATA);
        pTemp->dwOffset = FIX_ENDIAN(dwTemp);
        pTemp++;                     // Next record
    }

    //
    // Use nCount as a placeholder to calculate file size
    //

    nCount = DWORD_ALIGN(FIX_ENDIAN(HEADER(pProfObj)->phSize));
    nCount -= sizeof(TAGDATA) + cbSize;
    HEADER(pProfObj)->phSize = FIX_ENDIAN(nCount);

    return TRUE;
}


/******************************************************************************
 *
 *                       ChangeTaggedElementSize
 *
 *  Function:
 *       This functions changes the size of a tagged element in the profile
 *
 *  Arguments:
 *       pProfObj  - pointer to profile object
 *       pTagData  - pointer to tagged element whose size is to be changed
 *       cbSize    - new size for the element
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL ChangeTaggedElementSize(
    PPROFOBJ pProfObj,
    PTAGDATA pTagData,
    DWORD    cbSize
    )
{
    PTAGDATA pTemp;
    PBYTE    pData;
    DWORD    nCount, cbOldSize;
    DWORD    dwOffset, cnt, i;

    ASSERT(pProfObj != NULL);
    ASSERT(cbSize > 0);

    //
    // Get current size of element
    //

    cbOldSize = FIX_ENDIAN(pTagData->cbSize);

    if (cbOldSize == cbSize)
    {
        return TRUE;        // Sizes are the same - Do nothing
    }
    pData = VIEW(pProfObj) + FIX_ENDIAN(pTagData->dwOffset);

    //
    // Do not attempt to move data beyond end of file. There is no need to move
    // anything if the last data item is being resized.
    //

    if (pData + DWORD_ALIGN(cbOldSize) < VIEW(pProfObj) + PROFILE_SIZE(pProfObj))
    {
        MoveProfileData(pProfObj, pData + DWORD_ALIGN(cbOldSize), pData + DWORD_ALIGN(cbSize),
            PROFILE_SIZE(pProfObj) - (LONG)(pData - VIEW(pProfObj)) - DWORD_ALIGN(cbOldSize), TRUE);
    }

    pTagData->cbSize = FIX_ENDIAN(cbSize);  // Set the new size

    //
    // Get count of tag items - it is right after the profile header
    //

    nCount = TAG_COUNT(pProfObj);
    nCount = FIX_ENDIAN(nCount);

    //
    // Go through the tag table and update the pointers
    //

    pTemp = TAG_DATA(pProfObj);

    dwOffset = FIX_ENDIAN(pTagData->dwOffset);
    for (i=0; i<nCount; i++)
    {
        DWORD dwTemp = FIX_ENDIAN(pTemp->dwOffset);

        if (dwTemp > dwOffset)
        {
            dwTemp += DWORD_ALIGN(cbSize) - DWORD_ALIGN(cbOldSize);
            pTemp->dwOffset = FIX_ENDIAN(dwTemp);
        }
        pTemp++;                     // Next record
    }

    //
    // Use cnt as a placeholder to calculate file size
    //

    cnt = FIX_ENDIAN(HEADER(pProfObj)->phSize);
    cnt += DWORD_ALIGN(cbSize) - DWORD_ALIGN(cbOldSize);
    HEADER(pProfObj)->phSize = FIX_ENDIAN(cnt);

    return TRUE;
}


/******************************************************************************
 *
 *                       MoveProfileData
 *
 *  Function:
 *       This function moves data in a profile up or down (from src to dest),
 *       and then zeroes out the end of the file or the extra space created.
 *
 *  Arguments:
 *       pProfObj  - pointer to profile object
 *       src       - pointer to source of block to move
 *       dest      - pointer to destination for block to move to
 *       cnt       - number of bytes to move
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

void MoveProfileData(
    PPROFOBJ pProfObj,
    PBYTE src,
    PBYTE dest,
    LONG cnt,
    BOOL bZeroMemory
    )
{
    //
    // NOTE: CopyMemory() doesn't work for overlapped memory.
    // Use internal function instead.
    //

    MyCopyMemory((PVOID)dest, (PVOID)src, cnt);

    if (bZeroMemory)
    {
        cnt = ABS((LONG)(dest - src));

        if (dest < src)
        {
            //
            // Size decreased, so zero out end of file
            //

            dest = VIEW(pProfObj) + FIX_ENDIAN(HEADER(pProfObj)->phSize) -
                   (src - dest);
        }
        else
        {
            //
            // Size increased, so zero out the increased tagdata
            //

            dest = src;
        }
        ZeroMemory(dest, cnt);
    }

    return;
}


/******************************************************************************
 *
 *                           IsReferenceTag
 *
 *  Function:
 *       This function checks if a given tag's data is referred to by another
 *       tag in the profile
 *
 *  Arguments:
 *       pProfObj  - pointer to profile object
 *       pTagData  - pointer to tagdata which should be checked
 *
 *  Returns:
 *       TRUE if it is a referece, FALSE otherwise
 *
 ******************************************************************************/

BOOL IsReferenceTag(
    PPROFOBJ pProfObj,
    PTAGDATA pTagData
    )
{
    PTAGDATA pTemp;
    DWORD    nCount, i;
    BOOL     bReference = FALSE;

    pTemp = TAG_DATA(pProfObj);
    nCount = TAG_COUNT(pProfObj);
    nCount = FIX_ENDIAN(nCount);

    for (i=0; i<nCount; i++)
    {
        if ((pTagData->dwOffset == pTemp->dwOffset) &&
            (pTagData->tagType  != pTemp->tagType))
        {
            bReference = TRUE;
            break;
        }
        pTemp++;                     // next record
    }

    return bReference;
}

