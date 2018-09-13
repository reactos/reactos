/****************************Module*Header******************************\
* Module Name: COLOR.C
*
* Module Descripton: Functions for color matching outside the DC
*
* Warnings:
*
* Issues:
*
* Public Routines:
*
* Created:  23 April 1996
* Author:   Srinivasan Chandrasekar    [srinivac]
*
* Copyright (c) 1996, 1997  Microsoft Corporation
\***********************************************************************/

#include "mscms.h"
#include "wingdip.h" /* for LCS_DEVICE_CMYK */

//
// Local functions
//

HTRANSFORM InternalCreateColorTransform(LPLOGCOLORSPACE, HPROFILE, HPROFILE, DWORD);
BOOL       InternalRegisterCMM(PTSTR, DWORD, PTSTR);
BOOL       InternalUnregisterCMM(PTSTR, DWORD);
DWORD      GetBitmapBytes(BMFORMAT, DWORD, DWORD);


/******************************************************************************
 *
 *                            CreateColorTransform
 *
 *  Function:
 *       These are the ANSI & Unicode wrappers for InternalCreateColorTransform.
 *       Please see InternalCreateColorTransform for more details on this
 *       function.
 *
 *  Arguments:
 *       pLogColorSpace - pointer to LOGCOLORSPACE structure identifying
 *                        source color space
 *       hDestProfile   - handle identifing the destination profile object
 *       hTargetProfile - handle identifing the target profile object
 *       dwFlags        - optimization flags
 *
 *  Returns:
 *       Handle to color transform if successful, NULL otherwise
 *
 ******************************************************************************/

#ifdef UNICODE              // Windows NT version

HTRANSFORM WINAPI CreateColorTransformA(
    LPLOGCOLORSPACEA pLogColorSpace,
    HPROFILE         hDestProfile,
    HPROFILE         hTargetProfile,
    DWORD            dwFlags
    )
{
    LOGCOLORSPACEW  wLCS;

    TRACEAPI((__TEXT("CreateColorTransformA\n")));

    //
    // Validate parameter before we touch it
    //

    if (IsBadReadPtr(pLogColorSpace, sizeof(LOGCOLORSPACEA)))
    {
        WARNING((__TEXT("Invalid parameter to CreateColorTransform\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    CopyMemory(&wLCS, pLogColorSpace, sizeof(LOGCOLORSPACEA));
    wLCS.lcsSize = sizeof(LOGCOLORSPACEW);

    //
    // Convert filename to Unicode and call the internal version
    //

    if (! MultiByteToWideChar(CP_ACP, 0, pLogColorSpace->lcsFilename, -1,
        wLCS.lcsFilename, MAX_PATH))
    {
        WARNING((__TEXT("Error converting LogColorSpace filename to Unicode\n")));
        return NULL;
    }

    return InternalCreateColorTransform(&wLCS, hDestProfile, hTargetProfile, dwFlags);
}


HTRANSFORM WINAPI CreateColorTransformW(
    LPLOGCOLORSPACEW pLogColorSpace,
    HPROFILE         hDestProfile,
    HPROFILE         hTargetProfile,
    DWORD            dwFlags
    )
{
    TRACEAPI((__TEXT("CreateColorTransformW\n")));

    //
    // Internal version is Unicode in Windows NT, call it directly
    //

    return InternalCreateColorTransform(pLogColorSpace, hDestProfile, hTargetProfile, dwFlags);
}

#else                   // Windows 95 version

HTRANSFORM WINAPI CreateColorTransformA(
    LPLOGCOLORSPACEA pLogColorSpace,
    HPROFILE         hDestProfile,
    HPROFILE         hTargetProfile,
    DWORD            dwFlags
    )
{
    TRACEAPI((__TEXT("CreateColorTransformA\n")));

    //
    // Internal version is ANSI in Windows 95, call it directly
    //

    return InternalCreateColorTransform(pLogColorSpace, hDestProfile, hTargetProfile, dwFlags);
}


HTRANSFORM WINAPI CreateColorTransformW(
    LPLOGCOLORSPACEW  pLogColorSpace,
    HPROFILE          hDestProfile,
    HPROFILE          hTargetProfile,
    DWORD             dwFlags
    )
{
    LOGCOLORSPACEA  aLCS;
    BOOL            bUsedDefaultChar;

    TRACEAPI((__TEXT("CreateColorTransformW\n")));

    //
    // Validate parameter before we touch it
    //

    if (IsBadReadPtr(pLogColorSpace, sizeof(LOGCOLORSPACEW)))
    {
        WARNING((__TEXT("Invalid parameter to CreateColorTransform\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    CopyMemory(&aLCS, pLogColorSpace, sizeof(LOGCOLORSPACEA));
    aLCS.lcsSize = sizeof(LOGCOLORSPACEA);

    //
    // Convert filename to ANSI and call the internal version
    //

    if (! WideCharToMultiByte(CP_ACP, 0, pLogColorSpace->lcsFilename, -1,
        aLCS.lcsFilename, MAX_PATH, NULL, &bUsedDefaultChar) ||
        bUsedDefaultChar)
    {
        WARNING((__TEXT("Error converting LogColorSpace filename to ANSI\n")));
        return NULL;
    }

    return InternalCreateColorTransform(&aLCS, hDestProfile, hTargetProfile, dwFlags);
}

#endif


/******************************************************************************
 *
 *                        CreateMultiProfileTransform
 *
 *  Function:
 *       This functions creates a color transform from a set of profiles.
 *
 *  Arguments:
 *       pahProfiles       - pointer to array of handles of profiles
 *       nProfiles         - number of profiles in array
 *       padwIntent        - array of intents to use
 *       nIntents          - size of array - can be 1 or nProfiles
 *       dwFlags           - optimization flags
 *       indexPreferredCMM - one based index of profile which specifies
 *                           preferred CMM to use.
 *
 *  Returns:
 *       Handle to color transform if successful, NULL otherwise
 *
 ******************************************************************************/

HTRANSFORM WINAPI CreateMultiProfileTransform(
    PHPROFILE   pahProfiles,
    DWORD       nProfiles,
    PDWORD      padwIntent,
    DWORD       nIntents,
    DWORD       dwFlags,
    DWORD       indexPreferredCMM
    )
{
    PPROFOBJ      pProfObj;
    PCMMOBJ       pCMMObj = NULL;
    DWORD         cmmID;
    HTRANSFORM    hxform = NULL;
    PTRANSFORMOBJ pxformObj;
    DWORD         i;

    TRACEAPI((__TEXT("CreateMultiProfileTransform\n")));

    //
    // Validate parameters
    //

    if (nProfiles < 1 ||
        indexPreferredCMM > nProfiles ||
        IsBadReadPtr(pahProfiles, nProfiles * sizeof(HANDLE)) ||
        padwIntent == NULL ||
        ((nIntents != nProfiles) && (nIntents != 1)) ||
        IsBadReadPtr(padwIntent, nIntents * sizeof(DWORD)))
    {
        WARNING((__TEXT("Invalid parameter to CreateMultiProfileTransform\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    for (i=0; i<nProfiles; i++)
    {
        if (pahProfiles[i] == NULL ||
            ! ValidHandle(pahProfiles[i], OBJ_PROFILE))
        {
            WARNING((__TEXT("Invalid profile passed to CreateMultiProfileTransform\n")));
            SetLastError(ERROR_INVALID_PARAMETER);
            return NULL;
        }

        pProfObj = (PPROFOBJ)HDLTOPTR(pahProfiles[i]);

        ASSERT(pProfObj != NULL);

        //
        // Quick check on the integrity of the profile
        //

        if (!ValidProfile(pProfObj))
        {
            WARNING((__TEXT("Invalid profile passed to CreateMultiProfileTransform\n")));
            SetLastError(ERROR_INVALID_PROFILE);
            return NULL;
        }

        ASSERT(pProfObj->pView != NULL);

        if (i+1 == indexPreferredCMM)
        {
            //
            // Get ID of preferred CMM
            //

            cmmID = HEADER(pProfObj)->phCMMType;
            cmmID = FIX_ENDIAN(cmmID);
        }
    }

    if (indexPreferredCMM == INDEX_DONT_CARE)
    {
        pCMMObj = GetPreferredCMM();
    }
    else
    {
        //
        // Get CMM associated with preferred profile
        //

        pCMMObj  = GetColorMatchingModule(cmmID);
    }

    //
    // Finally try Windows default CMM
    //

    if (!pCMMObj)
    {
        pCMMObj = GetColorMatchingModule(CMM_WINDOWS_DEFAULT);
        if (!pCMMObj)
        {
            RIP((__TEXT("Default CMM not found\n")));
            SetLastError(ERROR_INVALID_CMM);
            return NULL;
        }
    }

    //
    // Allocate an object on the heap for the transform
    //

    hxform = AllocateHeapObject(OBJ_TRANSFORM);
    if (!hxform)
    {
        WARNING((__TEXT("Could not allocate transform object\n")));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ReleaseColorMatchingModule(pCMMObj);
        return NULL;
    }

    pxformObj = (PTRANSFORMOBJ)HDLTOPTR(hxform);

    ASSERT(pxformObj != NULL);

    ASSERT(pCMMObj->fns.pCMCreateMultiProfileTransform != NULL);

    pxformObj->pCMMObj = pCMMObj;
    pxformObj->objHdr.dwUseCount = 1;

    pxformObj->hcmxform = pCMMObj->fns.pCMCreateMultiProfileTransform(
                            pahProfiles,
                            nProfiles,
                            padwIntent,
                            nIntents,
                            dwFlags);

    TERSE((__TEXT("CMCreateMultiProfileTransform returned 0x%x\n"), pxformObj->hcmxform));

    //
    // If return value from CMM is less than 256, then it is an error code
    //

    if (pxformObj->hcmxform <= TRANSFORM_ERROR)
    {
        WARNING((__TEXT("CMCreateMultiProfileTransform failed\n")));
        if (GetLastError() == ERROR_SUCCESS)
        {
            WARNING((__TEXT("CMM did not set error code\n")));
            SetLastError(ERROR_INVALID_PROFILE);
        }
        ReleaseColorMatchingModule(pxformObj->pCMMObj);
        pxformObj->objHdr.dwUseCount--;        // decrement before freeing
        FreeHeapObject(hxform);
        hxform = NULL;
    }

    return hxform;
}


/******************************************************************************
 *
 *                          DeleteColorTransform
 *
 *  Function:
 *       This functions deletes a color transform and frees all associated
 *       memory.
 *
 *  Arguments:
 *       hxform - handle to color transform to delete
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL WINAPI DeleteColorTransform(
    HTRANSFORM  hxform
    )
{
    PTRANSFORMOBJ pxformObj;
    BOOL          rc;

    TRACEAPI((__TEXT("DeleteColorTransform\n")));

    //
    // Validate parameters
    //

    if (!ValidHandle(hxform, OBJ_TRANSFORM))
    {
        WARNING((__TEXT("Invalid parameter to DeleteColorTransform\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pxformObj = (PTRANSFORMOBJ)HDLTOPTR(hxform);

    ASSERT(pxformObj != NULL);

    ASSERT(pxformObj->objHdr.dwUseCount > 0);

    //
    // Decrease  object count, and delete if it goes to zero.
    // The following code retains the object and returns failure if the CMM
    // fails the delete transform operation.
    //

    pxformObj->objHdr.dwUseCount--;

    if (pxformObj->objHdr.dwUseCount == 0)
    {
        ASSERT(pxformObj->pCMMObj != NULL);

        rc = pxformObj->pCMMObj->fns.pCMDeleteTransform(pxformObj->hcmxform);
        if (!rc)
        {
            pxformObj->objHdr.dwUseCount++;     // set count back
            return FALSE;
        }
        ReleaseColorMatchingModule(pxformObj->pCMMObj);
        FreeHeapObject(hxform);
    }

    return TRUE;
}


/******************************************************************************
 *
 *                          TranslateColors
 *
 *  Function:
 *       This functions translates an array of color strcutures using the
 *       given transform
 *
 *  Arguments:
 *       hxform         - handle to color transform to use
 *       paInputcolors  - pointer to array of input colors
 *       nColors        - number of colors in the array
 *       ctInput        - color type of input
 *       paOutputColors - pointer to buffer to get translated colors
 *       ctOutput       - output color type
 *
 *  Comments:
 *       Input and output color types must be consistent with the transform
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL  WINAPI TranslateColors(
    HTRANSFORM  hxform,
    PCOLOR      paInputColors,
    DWORD       nColors,
    COLORTYPE   ctInput,
    PCOLOR      paOutputColors,
    COLORTYPE   ctOutput
    )
{
    PTRANSFORMOBJ pxformObj;
    BOOL          rc;

    TRACEAPI((__TEXT("TranslateColors\n")));

    //
    // Validate parameters
    //

    if (!ValidHandle(hxform, OBJ_TRANSFORM) ||
        (nColors == 0) ||
        IsBadReadPtr(paInputColors, nColors*sizeof(COLOR)) ||
        IsBadWritePtr(paOutputColors, nColors*sizeof(COLOR)))
    {
        WARNING((__TEXT("Invalid parameter to TranslateColors\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pxformObj = (PTRANSFORMOBJ)HDLTOPTR(hxform);

    ASSERT(pxformObj != NULL);

    ASSERT(pxformObj->pCMMObj != NULL);

    rc = pxformObj->pCMMObj->fns.pCMTranslateColors(
            pxformObj->hcmxform,
            paInputColors,
            nColors,
            ctInput,
            paOutputColors,
            ctOutput);

    return rc;
}


/******************************************************************************
 *
 *                          CheckColors
 *
 *  Function:
 *       This functions checks if an array of colors fall within the output
 *       gamut of the given transform
 *
 *  Arguments:
 *       hxform         - handle to color transform to use
 *       paInputcolors  - pointer to array of input colors
 *       nColors        - number of colors in the array
 *       ctInput        - color type of input
 *       paResult       - pointer to buffer to hold the result
 *
 *  Comments:
 *       Input color type must be consistent with the transform
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL  WINAPI CheckColors(
    HTRANSFORM      hxform,
    PCOLOR          paInputColors,
    DWORD           nColors,
    COLORTYPE       ctInput,
    PBYTE           paResult
    )
{
    PTRANSFORMOBJ pxformObj;
    BOOL          rc;

    TRACEAPI((__TEXT("CheckColors\n")));

    //
    // Validate parameters
    //

    if (!ValidHandle(hxform, OBJ_TRANSFORM) ||
        (nColors == 0) ||
        IsBadReadPtr(paInputColors, nColors * sizeof(COLOR)) ||
        IsBadWritePtr(paResult, nColors * sizeof(BYTE)))
    {
        WARNING((__TEXT("Invalid parameter to CheckColors\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pxformObj = (PTRANSFORMOBJ)HDLTOPTR(hxform);

    ASSERT(pxformObj != NULL);

    ASSERT(pxformObj->pCMMObj != NULL);

    rc = pxformObj->pCMMObj->fns.pCMCheckColors(
            pxformObj->hcmxform,
            paInputColors,
            nColors,
            ctInput,
            paResult);

    return rc;
}


/******************************************************************************
 *
 *                          TranslateBitmapBits
 *
 *  Function:
 *       This functions translates the colors of a bitmap using the
 *       given transform
 *
 *  Arguments:
 *       hxform         - handle to color transform to use
 *       pSrcBits       - pointer to source bitmap
 *       bmInput        - input bitmap format
 *       dwWidth        - width in pixels of each scanline
 *       dwHeight       - number of scanlines in bitmap
 *       dwInputStride  - number of bytes from beginning of one scanline to next
 *                        in input buffer, 0 means DWORD aligned
 *       pDestBits      - pointer to destination bitmap to store results
 *       bmOutput       - output bitmap format
 *       dwOutputStride - number of bytes from beginning of one scanline to next
 *                        in output buffer, 0 means DWORD aligned
 *       pfnCallback    - callback function to report progress
 *       ulCallbackData - parameter to callback function
 *
 *  Comments:
 *       Input and output bitmap formats must be consistent with the transform
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL  WINAPI TranslateBitmapBits(
    HTRANSFORM      hxform,
    PVOID           pSrcBits,
    BMFORMAT        bmInput,
    DWORD           dwWidth,
    DWORD           dwHeight,
    DWORD           dwInputStride,
    PVOID           pDestBits,
    BMFORMAT        bmOutput,
    DWORD           dwOutputStride,
    PBMCALLBACKFN   pfnCallback,
    LPARAM          ulCallbackData
)
{
    PTRANSFORMOBJ pxformObj;
    DWORD         nBytes, cbSize;
    BOOL          rc;

    TRACEAPI((__TEXT("TranslateBitmapBits\n")));

    //
    // Calculate number of bytes input bitmap should be
    //

    if (dwInputStride)
        cbSize = dwInputStride * dwHeight;
    else
        cbSize = GetBitmapBytes(bmInput, dwWidth, dwHeight);

    //
    // Calculate number of bytes output bitmap should be
    //

    if (dwOutputStride)
        nBytes = dwOutputStride * dwHeight;
    else
        nBytes = GetBitmapBytes(bmOutput, dwWidth, dwHeight);

    //
    // Validate parameters
    //

    if (nBytes == 0 ||
        cbSize == 0 ||
        !ValidHandle(hxform, OBJ_TRANSFORM) ||
        IsBadReadPtr(pSrcBits, cbSize) ||
        IsBadWritePtr(pDestBits, nBytes) ||
        (pfnCallback && IsBadCodePtr((FARPROC)pfnCallback)))
    {
        WARNING((__TEXT("Invalid parameter to TranslateBitmapBits\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pxformObj = (PTRANSFORMOBJ)HDLTOPTR(hxform);

    ASSERT(pxformObj != NULL);

    ASSERT(pxformObj->pCMMObj != NULL);

    rc = pxformObj->pCMMObj->fns.pCMTranslateRGBsExt(
            pxformObj->hcmxform,
            pSrcBits,
            bmInput,
            dwWidth,
            dwHeight,
            dwInputStride,
            pDestBits,
            bmOutput,
            dwOutputStride,
            pfnCallback,
            ulCallbackData);

    return rc;
}


/******************************************************************************
 *
 *                           CheckBitmapBits
 *
 *  Function:
 *       This functions checks if the colors of a bitmap fall within the
 *       output gamut of the given transform
 *
 *  Arguments:
 *       hxform         - handle to color transform to use
 *       pSrcBits       - pointer to source bitmap
 *       bmInput        - input bitmap format
 *       dwWidth        - width in pixels of each scanline
 *       dwHeight       - number of scanlines in bitmap
 *       dwStride       - number of bytes from beginning of one scanline to next
 *       paResult       - pointer to buffer to hold the result
 *       pfnCallback    - callback function to report progress
 *       ulCallbackData - parameter to callback function
 *
 *  Comments:
 *       Input bitmap format must be consistent with the transform
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL  WINAPI CheckBitmapBits(
    HTRANSFORM      hxform,
    PVOID           pSrcBits,
    BMFORMAT        bmInput,
    DWORD           dwWidth,
    DWORD           dwHeight,
    DWORD           dwStride,
    PBYTE           paResult,
    PBMCALLBACKFN   pfnCallback,
    LPARAM          ulCallbackData
)
{
    PTRANSFORMOBJ pxformObj;
    DWORD         cbSize;
    BOOL          rc;

    TRACEAPI((__TEXT("CheckBitmapBits\n")));

    //
    // Calculate number of bytes input bitmap should be
    //

    if (dwStride)
        cbSize = dwStride*dwHeight;
    else
        cbSize = GetBitmapBytes(bmInput, dwWidth, dwHeight);

    //
    // Validate parameters
    //

    if (!ValidHandle(hxform, OBJ_TRANSFORM) ||
        cbSize == 0 ||
        IsBadReadPtr(pSrcBits, cbSize) ||
        IsBadWritePtr(paResult, dwWidth*dwHeight) ||
        (pfnCallback && IsBadCodePtr((FARPROC)pfnCallback)))
    {
        WARNING((__TEXT("Invalid parameter to CheckBitmapBits\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pxformObj = (PTRANSFORMOBJ)HDLTOPTR(hxform);

    ASSERT(pxformObj != NULL);

    ASSERT(pxformObj->pCMMObj != NULL);

    rc = pxformObj->pCMMObj->fns.pCMCheckRGBs(
            pxformObj->hcmxform,
            pSrcBits,
            bmInput,
            dwWidth,
            dwHeight,
            dwStride,
            paResult,
            pfnCallback,
            ulCallbackData);

    return rc;
}


/******************************************************************************
 *
 *                           GetCMMInfo
 *
 *  Function:
 *       This functions retrieves information about the CMM that created the
 *       given transform
 *
 *  Arguments:
 *       hxform         - handle to color transform
 *       dwInfo         - Can be one of the following:
 *                        CMM_WIN_VERSION: Version of Windows support
 *                        CMM_DLL_VERSION: Version of CMM
 *                        CMM_IDENT:       CMM signature registered with ICC
 *
 *  Returns:
 *       For CMM_WIN_VERSION, it returns the Windows version it was written for.
 *       For CMM_DLL_VERSION, it returns the version number of the CMM DLL.
 *       For CMM_IDENT, it returns the CMM signature registered with the ICC.
 *       If the function fails it returns zero.
 *
 ******************************************************************************/

DWORD  WINAPI GetCMMInfo(
        HTRANSFORM      hxform,
        DWORD           dwInfo
        )
{
    PTRANSFORMOBJ pxformObj;
    BOOL          rc;

    TRACEAPI((__TEXT("GetCMMInfo\n")));

    //
    // Validate parameters
    //

    if (!ValidHandle(hxform, OBJ_TRANSFORM) ||
        (dwInfo != CMM_WIN_VERSION &&
         dwInfo != CMM_DLL_VERSION &&
         dwInfo != CMM_IDENT
         )
        )
    {
        WARNING((__TEXT("Invalid parameter to GetCMMInfo\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pxformObj = (PTRANSFORMOBJ)HDLTOPTR(hxform);

    ASSERT(pxformObj != NULL);

    ASSERT(pxformObj->pCMMObj != NULL);

    rc = pxformObj->pCMMObj->fns.pCMGetInfo(dwInfo);

    return rc;
}


/******************************************************************************
 *
 *                              RegisterCMM
 *
 *  Function:
 *       These are the ANSI and Unicode wrappers. For more information on this
 *       function, see InternalRegisterCMM.
 *
 *  Arguments:
 *       pMachineName   - name identifying machine on which the CMM should be
 *                        registered
 *       cmmID          - ID of CMM to register
 *       pCMMdll        - pointer to CMM dll to register
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

#ifdef UNICODE              // Windows NT version

BOOL  WINAPI RegisterCMMA(
    PCSTR       pMachineName,
    DWORD       cmmID,
    PCSTR       pCMMdll
    )
{
    PWSTR pwszMachineName = NULL;   // Unicode machine name
    PWSTR pwszCMMdll = NULL;        // Unicode CMM dll path
    BOOL  rc = TRUE;                // return code

    TRACEAPI((__TEXT("RegisterCMMA\n")));

    //
    // Convert machine name to Unicode
    //

    if (pMachineName)
    {
        rc = ConvertToUnicode(pMachineName, &pwszMachineName, TRUE);
    }
    else
        pwszMachineName = NULL;

    //
    // Convert pCMMdll to Unicode
    //

    rc = rc && ConvertToUnicode(pCMMdll, &pwszCMMdll, TRUE);


    //
    // Call the internal Unicode function
    //

    rc = rc && InternalRegisterCMM(pwszMachineName, cmmID, pwszCMMdll);

    //
    // Free memory before leaving
    //

    if (pwszMachineName)
    {
        MemFree(pwszMachineName);
    }

    if (pwszCMMdll)
    {
        MemFree(pwszCMMdll);
    }

    return rc;
}


BOOL  WINAPI RegisterCMMW(
    PCWSTR      pMachineName,
    DWORD       cmmID,
    PCWSTR      pCMMdll
    )
{
    TRACEAPI((__TEXT("RegisterCMMW\n")));

    //
    // Internal version is Unicode in Windows NT, call it directly
    //

    return InternalRegisterCMM((PWSTR)pMachineName, cmmID, (PWSTR)pCMMdll);
}

#else                   // Windows 95 version

BOOL  WINAPI RegisterCMMA(
    PCSTR       pMachineName,
    DWORD       cmmID,
    PCSTR       pCMMdll
    )
{
    TRACEAPI((__TEXT("RegisterCMMA\n")));

    //
    // Internal version is ANSI in Windows 95, call it directly
    //

    return InternalRegisterCMM((PSTR)pMachineName, cmmID, (PSTR)pCMMdll);
}


BOOL  WINAPI RegisterCMMW(
    PCWSTR      pMachineName,
    DWORD       cmmID,
    PCWSTR      pCMMdll
    )
{
    PSTR  pszMachineName = NULL;    // Ansi machine name
    PSTR  pszCMMdll = NULL;         // Ansi CMM dll path
    BOOL  rc = TRUE;                // return code

    TRACEAPI((__TEXT("RegisterCMMW\n")));

    //
    // Convert machine name to Ansi
    //

    if (pMachineName)
    {
        rc = ConvertToAnsi(pMachineName, &pszMachineName, TRUE);
    }
    else
        pszMachineName = NULL;

    //
    // Convert pCMMdll to Ansi
    //

    rc = rc && ConvertToAnsi(pCMMdll, &pszCMMdll, TRUE);

    //
    // Call the internal Ansi function
    //

    rc = rc && InternalRegisterCMM(pszMachineName, cmmID, pszCMMdll);

    //
    // Free memory before leaving
    //

    if (pszMachineName)
    {
        MemFree(pszMachineName);
    }

    if (pszCMMdll)
    {
        MemFree(pszCMMdll);
    }

    return rc;
}

#endif


/******************************************************************************
 *
 *                            UnregisterCMM
 *
 *  Function:
 *       These are the ANSI and Unicode wrappers. For more information on this
 *       function, see InternalUnregisterCMM.
 *
 *  Arguments:
 *       pMachineName   - name identifying machine on which the CMM is
 *                        registered
 *       cmmID          - ID of CMM to unregister
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

#ifdef UNICODE              // Windows NT version

BOOL  WINAPI UnregisterCMMA(
    PCSTR   pMachineName,
    DWORD   cmmID
    )
{
    PWSTR pwszMachineName = NULL;   // Unicode machine name
    BOOL  rc = TRUE;                // return code

    TRACEAPI((__TEXT("UnregisterCMMA\n")));

    //
    // Convert machine name to Unicode
    //

    if (pMachineName)
    {
        rc = ConvertToUnicode(pMachineName, &pwszMachineName, TRUE);
    }
    else
        pwszMachineName = NULL;

    //
    // Call the internal Unicode function
    //

    rc = rc && InternalUnregisterCMM(pwszMachineName, cmmID);

    //
    // Free memory before leaving
    //

    if (pwszMachineName)
    {
        MemFree(pwszMachineName);
    }

    return rc;
}


BOOL  WINAPI UnregisterCMMW(
    PCWSTR      pMachineName,
    DWORD       cmmID
    )
{
    TRACEAPI((__TEXT("UnregisterCMMW\n")));

    //
    // Internal version is Unicode in Windows NT, call it directly
    //

    return InternalUnregisterCMM((PWSTR)pMachineName, cmmID);
}

#else                   // Windows 95 version

BOOL  WINAPI UnregisterCMMA(
    PCSTR       pMachineName,
    DWORD       cmmID
    )
{
    TRACEAPI((__TEXT("UnregisterCMMA\n")));

    //
    // Internal version is ANSI in Windows 95, call it directly
    //

    return InternalUnregisterCMM((PSTR)pMachineName, cmmID);
}


BOOL  WINAPI UnregisterCMMW(
    PCWSTR      pMachineName,
    DWORD       cmmID
    )
{
    PSTR  pszMachineName = NULL;    // Ansi machine name
    BOOL  rc = TRUE;                // return code

    TRACEAPI((__TEXT("UnregisterCMMW\n")));

    //
    // Convert machine name to Ansi
    //

    if (pMachineName)
    {
        rc = ConvertToAnsi(pMachineName, &pszMachineName, TRUE);
    }
    else
        pszMachineName = NULL;

    //
    // Call the internal Ansi function
    //

    rc = rc && InternalUnregisterCMM(pszMachineName, cmmID);

    //
    // Free memory before leaving
    //

    if (pszMachineName)
    {
        MemFree(pszMachineName);
    }

    return rc;
}

#endif


/******************************************************************************
 *
 *                               SelectCMM
 *
 *  Function:
 *       This function allows an application to select the preferred CMM to use
 *
 *  Arguments:
 *       cmmID          - ID of preferred CMM to use
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL  WINAPI SelectCMM(
        DWORD   dwCMMType
    )
{
    PCMMOBJ pCMMObj = NULL;

    TRACEAPI((__TEXT("SelectCMM\n")));

    if (dwCMMType)
    {
        pCMMObj  = GetColorMatchingModule(dwCMMType);
        if (!pCMMObj)
        {
            SetLastError(ERROR_INVALID_CMM);
            return FALSE;
        }
    }

    //
    // Update Preferred CMM
    //
    EnterCriticalSection(&critsec);         // Critical Section
    gpPreferredCMM = pCMMObj;
    LeaveCriticalSection(&critsec);         // Critical Section

    return TRUE;
}


/*****************************************************************************/
/***************************** Internal Functions ****************************/
/*****************************************************************************/

/******************************************************************************
 *
 *                         InternalCreateColorTransform
 *
 *  Function:
 *       This functions creates a color transform that translates from
 *       the logcolorspace to the optional target device color space to the
 *       destination device color space.
 *
 *  Arguments:
 *       pLogColorSpace - pointer to LOGCOLORSPACE structure identifying
 *                        source color space
 *       hDestProfile   - handle identifing the destination profile object
 *       hTargetProfile - handle identifing the target profile object
 *       dwFlags        - optimization flags
 *
 *  Returns:
 *       Handle to color transform if successful, NULL otherwise
 *
 ******************************************************************************/

HTRANSFORM InternalCreateColorTransform(
    LPLOGCOLORSPACE pLogColorSpace,
    HPROFILE        hDestProfile,
    HPROFILE        hTargetProfile,
    DWORD           dwFlags
    )
{
    PPROFOBJ         pDestProfObj, pTargetProfObj = NULL;
    PCMMOBJ          pCMMObj;
    DWORD            cmmID;
    HTRANSFORM       hxform = NULL;
    PTRANSFORMOBJ    pxformObj;
    LPLOGCOLORSPACE  pLCS;

    //
    // Validate parameters
    //

    if (!pLogColorSpace ||
        IsBadReadPtr(pLogColorSpace, sizeof(LOGCOLORSPACE)) ||
        pLogColorSpace->lcsSignature !=  LCS_SIGNATURE ||
        pLogColorSpace->lcsVersion < 0x00000400 ||
        !hDestProfile ||
        !ValidHandle(hDestProfile, OBJ_PROFILE) ||
        ((hTargetProfile != NULL) &&
         !ValidHandle(hTargetProfile, OBJ_PROFILE)
        )
       )
    {
        WARNING((__TEXT("Invalid parameter to CreateColorTransform\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    //
    // Allocate LogColorSpace and copy incoming data.
    // Leave space for passing in two handles below it
    //

    pLCS = (LPLOGCOLORSPACE)MemAlloc(sizeof(LOGCOLORSPACE) + 2*sizeof(HPROFILE));
    if (!pLCS)
    {
        WARNING((__TEXT("Could not allocate LogColorSpace")));
        return NULL;
    }
    CopyMemory((PVOID)pLCS, (PVOID)pLogColorSpace, sizeof(LOGCOLORSPACE));

    //
    // Copy handles below this structure
    //

    *((PHPROFILE)((PBYTE)pLCS+sizeof(LOGCOLORSPACE))) = hDestProfile;
    *((PHPROFILE)((PBYTE)pLCS+sizeof(LOGCOLORSPACE)+sizeof(HPROFILE))) = hTargetProfile;

    //
    // If input color space is a predefined color space,
    // find the right profile to use
    //

    if (pLCS->lcsCSType > LCS_DEVICE_CMYK)
    {
        DWORD cbSize = MAX_PATH;

        if (! GetStandardColorSpaceProfile(NULL, pLCS->lcsCSType,
                pLCS->lcsFilename, &cbSize))
        {
            WARNING((__TEXT("Could not get profile for predefined color space 0x%x\n"), pLCS->lcsCSType));
            goto EndCreateColorTransform;
        }
    }

    pDestProfObj = (PPROFOBJ)HDLTOPTR(hDestProfile);

    ASSERT(pDestProfObj != NULL);

    //
    // Quick check on the integrity of the profile before calling the CMM
    //

    if (!ValidProfile(pDestProfObj))
    {
        WARNING((__TEXT("Invalid dest profile passed to CreateColorTransform\n")));
        SetLastError(ERROR_INVALID_PROFILE);
        goto EndCreateColorTransform;
    }

    //
    // If target profile is given, get the profile object and check integrity
    //

    if (hTargetProfile)
    {
        pTargetProfObj = (PPROFOBJ)HDLTOPTR(hTargetProfile);

        ASSERT(pTargetProfObj != NULL);

        if (!ValidProfile(pTargetProfObj))
        {
            WARNING((__TEXT("Invalid target profile passed to CreateColorTransform\n")));
            SetLastError(ERROR_INVALID_PROFILE);
            goto EndCreateColorTransform;
        }
    }

    //
    // Check if application specified CMM is present
    //

    pCMMObj = GetPreferredCMM();
    if (!pCMMObj)
    {
        //
        // Get CMM associated with destination profile. If it does not exist,
        // get default CMM.
        //

        cmmID = HEADER(pDestProfObj)->phCMMType;
        cmmID = FIX_ENDIAN(cmmID);

        pCMMObj  = GetColorMatchingModule(cmmID);
        if (!pCMMObj)
        {
            TERSE((__TEXT("CMM associated with profile could not be found")));

            pCMMObj = GetColorMatchingModule(CMM_WINDOWS_DEFAULT);
            if (!pCMMObj)
            {
                RIP((__TEXT("Default CMM not found\n")));
                SetLastError(ERROR_INVALID_CMM);
                goto EndCreateColorTransform;
            }
        }
    }

    //
    // Allocate an object on the heap for the transform
    //

    hxform = AllocateHeapObject(OBJ_TRANSFORM);
    if (!hxform)
    {
        WARNING((__TEXT("Could not allocate transform object\n")));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ReleaseColorMatchingModule(pCMMObj);
        goto EndCreateColorTransform;
    }

    pxformObj = (PTRANSFORMOBJ)HDLTOPTR(hxform);

    ASSERT(pxformObj != NULL);

    //
    // Call into CMM to create a color transform
    //

    ASSERT(pCMMObj->fns.pCMCreateTransformExt != NULL);

    ASSERT(pDestProfObj->pView != NULL);

    ASSERT(!pTargetProfObj || pTargetProfObj->pView);

    pxformObj->pCMMObj = pCMMObj;
    pxformObj->objHdr.dwUseCount = 1;

    pxformObj->hcmxform = pCMMObj->fns.pCMCreateTransformExt(
                            pLCS,
                            pDestProfObj->pView,
                            pTargetProfObj ? pTargetProfObj->pView : NULL,
                            dwFlags);

    TERSE((__TEXT("CMCreateTransform returned 0x%x\n"), pxformObj->hcmxform));

    //
    // If return value from CMM is less than 256, then it is an error code
    //

    if (pxformObj->hcmxform <= TRANSFORM_ERROR)
    {
        WARNING((__TEXT("CMCreateTransform failed\n")));
        if (GetLastError() == ERROR_SUCCESS)
        {
            WARNING((__TEXT("CMM did not set error code\n")));
            SetLastError(ERROR_INVALID_PROFILE);
        }
        ReleaseColorMatchingModule(pxformObj->pCMMObj);
        pxformObj->objHdr.dwUseCount--;        // decrement before freeing
        FreeHeapObject(hxform);
        hxform = NULL;
    }

EndCreateColorTransform:
    MemFree(pLCS);

    return hxform;
}

/******************************************************************************
 *
 *                            InternalRegisterCMM
 *
 *  Function:
 *       This function associates an ID with a CMM DLL, so that we can use
 *       the ID in profiles to find the CMM to use when creating a transform.
 *
 *  Arguments:
 *       pMachineName   - name identifying machine on which the CMM should be
 *                        registered
 *       cmmID          - ID of CMM to register
 *       pCMMdll        - pointer to CMM dll to register
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL  InternalRegisterCMM(
    PTSTR       pMachineName,
    DWORD       cmmID,
    PTSTR       pCMMdll
    )
{
    PCMMOBJ   pCMMObj;
    HKEY      hkCMM;
    DWORD     dwErr;
    BOOL      rc = TRUE;
    int       i;
    TCHAR     szCMMID[5];

    //
    // Validate parameters
    //

    if (!pCMMdll || IsBadReadPtr(pCMMdll, lstrlen(pCMMdll)*sizeof(TCHAR)))
    {
        WARNING((__TEXT("Invalid parameter to RegisterCMM\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    //
    // Only local installs are allowed now
    //

    if (pMachineName)
    {
        WARNING((__TEXT("Remote CMM registration attempted, failing...\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Check if the CMM actually exists, and it's valid CMM
    //

    rc = ValidColorMatchingModule(cmmID,pCMMdll);

    if (rc)
    {
        //
        // Open the registry key for ICMatchers
        //

        if ((dwErr = RegCreateKey(HKEY_LOCAL_MACHINE, gszICMatcher, &hkCMM)) != ERROR_SUCCESS)
        {
            ERR((__TEXT("Could not open ICMatcher registry key: %d\n"), dwErr));
            SetLastError(dwErr);
            return FALSE;
        }

        //
        // Make a string with the CMM ID
        //

        for (i=0; i<4; i++)
        {
            szCMMID[i]  = (TCHAR)(((char*)&cmmID)[3-i]);
        }
        szCMMID[4] = '\0';

        //
        // Set the file name of the CMM dll in the registry
        //

        if ((dwErr = RegSetValueEx(hkCMM, (PTSTR)szCMMID, 0, REG_SZ, (BYTE *)pCMMdll,
            (lstrlen(pCMMdll)+1)*sizeof(TCHAR))) != ERROR_SUCCESS)
        {
            WARNING((__TEXT("Could not set CMM dll in the registry %s: %d\n"), szCMMID, dwErr));
            SetLastError(dwErr);
            rc = FALSE;
        }

        RegCloseKey(hkCMM);
    }

    return rc;
}



/******************************************************************************
 *
 *                            InternalUnregisterCMM
 *
 *  Function:
 *       This function unregisters a CMM from the system by dissociating the
 *       ID from the CMM dll in the registry.
 *
 *  Arguments:
 *       pMachineName   - name identifying machine on which the CMM is
 *                        registered
 *       cmmID          - ID of CMM to unregister
 *
 *  Returns:
 *       TRUE if successful, FALSE otherwise
 *
 ******************************************************************************/

BOOL  InternalUnregisterCMM(
    PTSTR       pMachineName,
    DWORD       cmmID
    )
{
    HKEY      hkCMM;
    TCHAR     szCMMID[5];
    DWORD     dwErr;
    BOOL      rc = TRUE;

    //
    // Only local installs are allowed now
    //

    if (pMachineName)
    {
        WARNING((__TEXT("Remote CMM unregistration attempted, failing...\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Open the registry key for ICMatchers
    //

    if ((dwErr = RegOpenKey(HKEY_LOCAL_MACHINE, gszICMatcher, &hkCMM)) != ERROR_SUCCESS)
    {
        ERR((__TEXT("Could not open ICMatcher registry key: %d\n"), dwErr));
        SetLastError(dwErr);
        return FALSE;
    }

    //
    // Make a string with the CMM ID
    //

    szCMMID[0] = ((char *)&cmmID)[3];
    szCMMID[1] = ((char *)&cmmID)[2];
    szCMMID[2] = ((char *)&cmmID)[1];
    szCMMID[3] = ((char *)&cmmID)[0];
    szCMMID[4] = '\0';

    //
    // Delete the file name of the CMM dll from the registry
    //

    if ((dwErr = RegDeleteValue(hkCMM, (PTSTR)szCMMID)) != ERROR_SUCCESS)
    {
        WARNING((__TEXT("Could not delete CMM dll from the registry %s: %d\n"), szCMMID, dwErr));
        SetLastError(dwErr);
        rc = FALSE;
    }

    RegCloseKey(hkCMM);

    return rc;
}


/******************************************************************************
 *
 *                           GetBitmapBytes
 *
 *  Function:
 *       This functions returns number of bytes required for a bitmap given
 *       the format, the width in pixels and number of scanlines
 *
 *  Arguments:
 *       bmFmt          - format of bitmap
 *       deWidth        - number of pixels per scanline
 *       dwHeight       - number of scanlines in bitmap
 *
 *  Returns:
 *       Number of bytes required for bitmap on success, 0 on failure
 *
 ******************************************************************************/

DWORD GetBitmapBytes(
    BMFORMAT bmFmt,
    DWORD    dwWidth,
    DWORD    dwHeight
    )
{
    DWORD nBytes;

    switch (bmFmt)
    {
    //
    // 1 byte per pixel
    //

    case BM_GRAY:
        nBytes = dwWidth;
        break;

    //
    // 2 bytes per pixel
    //

    case BM_x555RGB:
    case BM_x555XYZ:
    case BM_x555Yxy:
    case BM_x555Lab:
    case BM_x555G3CH:
    case BM_16b_GRAY:
    case BM_565RGB:
        nBytes = dwWidth*2;
        break;

    //
    // 3 bytes per pixel
    //

    case BM_BGRTRIPLETS:
    case BM_RGBTRIPLETS:
    case BM_XYZTRIPLETS:
    case BM_YxyTRIPLETS:
    case BM_LabTRIPLETS:
    case BM_G3CHTRIPLETS:
        nBytes = dwWidth*3;
        break;

    //
    // 4 bytes per pixel
    //

    case BM_xRGBQUADS:
    case BM_xBGRQUADS:
    #if 0
    case BM_xXYZQUADS:
    case BM_xYxyQUADS:
    case BM_xLabQUADS:
    #endif
    case BM_xG3CHQUADS:
    case BM_KYMCQUADS:
    case BM_CMYKQUADS:
    case BM_10b_RGB:
    case BM_10b_XYZ:
    case BM_10b_Yxy:
    case BM_10b_Lab:
    case BM_10b_G3CH:
    case BM_NAMED_INDEX:
        nBytes = dwWidth*4;
        break;

    //
    // 5 bytes per pixel
    //

    case BM_5CHANNEL:
        nBytes = dwWidth*5;
        break;

    //
    // 6 bytes per pixel
    //

    case BM_16b_RGB:
    case BM_16b_XYZ:
    case BM_16b_Yxy:
    case BM_16b_Lab:
    case BM_16b_G3CH:
    case BM_6CHANNEL:
        nBytes = dwWidth*6;
        break;

    //
    // 7 bytes per pixel
    //

    case BM_7CHANNEL:
        nBytes = dwWidth*7;
        break;

    //
    // 8 bytes per pixel
    //

    case BM_8CHANNEL:
        nBytes = dwWidth*8;
        break;

    //
    // Error case
    //

    default:
        nBytes = 0;
        break;
    }

    //
    // Align to DWORD boundary
    //

    nBytes = (nBytes + 3) & ~3;

    return nBytes*dwHeight;
}



