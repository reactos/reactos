/*
	File:		MsLinoCMM.c

	Contains:
		Interface to MS ICM
	Written by:	U. J. Krabbenhoeft

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

	Version:	
*/

#include "Windef.h"
#include "WinGdi.h"
#include <wtypes.h>
#include <winbase.h>
#include <WindowsX.h>
#include "ICM.h"

#ifndef LHGeneralIncs_h
#include "General.h"
#endif

#ifndef MSNewMemProfile_h
#include "MemProf.h"
#endif

#define CMM_WIN_VERSION		0
#define CMM_IDENT			1
#define CMM_DRIVER_LEVEL	2
#define CMM_DLL_VERSION		3
#define CMM_VERSION			4
#define CMS_LEVEL_1			1

typedef HANDLE  HCMTRANSFORM;
typedef LPVOID  LPDEVCHARACTER;
typedef HANDLE *LPHPROFILE;
typedef LPVOID  LPARGBQUAD;
typedef COLORREF FAR *LPCOLORREF;

#ifdef _DEBUG
//#define WRITE_PROFILE
#endif
/* ______________________________________________________________________
			static section for holding the CW pointers
   _____________________________________________________________________ */

long IndexTransform=0;
HCMTRANSFORM TheTransform[1000] = {0};

CRITICAL_SECTION GlobalCriticalSection;		/* for multithreaded dll */

/* ______________________________________________________________________ */

/* ______________________________________________________________________

BOOL WINAPI DllMain (	HINSTANCE hinstDLL,
						DWORD fdwReason,
						LPVOID lpvReserved )

        Abstract:
                DLL Entrypoint

        Params:
                standard

        Return:
                TRUE

   _____________________________________________________________________ */
BOOL WINAPI DllMain (	HINSTANCE hinstDLL,
						DWORD fdwReason,
						LPVOID lpvReserved )
{
switch (fdwReason)
   {
   case DLL_PROCESS_ATTACH:
      DisableThreadLibraryCalls(hinstDLL);
	  InitializeCriticalSection(&GlobalCriticalSection);
      break;
   case DLL_PROCESS_DETACH:
      DeleteCriticalSection(&GlobalCriticalSection);
      break;
   }
return TRUE;
}

#define DllExport	__declspec( dllexport )

HCMTRANSFORM  WINAPI CMCreateMultiProfileTransform(	LPHPROFILE	lpahProfiles,
													DWORD 		nProfiles,
													DWORD		*aIntentArr,
													DWORD		nIntents,
													DWORD		dwFlags );
long FillProfileFromLog(	LPLOGCOLORSPACEA	lpColorSpace,
							PPROFILE			theProf );
long FillProfileFromLogW(	LPLOGCOLORSPACEW	lpColorSpace,
							PPROFILE			theProf );
BOOL  WINAPI CMCreateProfile(	LPLOGCOLORSPACEA	lpColorSpace,
								LPDEVCHARACTER 		*lpProfileData );
CMWorldRef StoreTransform( CMWorldRef aRef );
CMBitmapColorSpace CMGetDataColorSpace( BMFORMAT c, long *pixelSize );
HCMTRANSFORM  WINAPI CMGetTransform( HCMTRANSFORM 	hcmTransform );
long  CMCreateMultiProfileTransformInternal(		CMWorldRef	*cw,
													LPHPROFILE	lpahProfiles,
													DWORD 		nProfiles,
													DWORD		*aIntentArr,
													DWORD		nIntents,
													DWORD		dwFlags,
													DWORD		dwCreateLink );
/* ______________________________________________________________________

DWORD WINAPI CMGetInfo( DWORD dwInfo );

Abstract:
	The CMGetInfo function retrieves various information about the ICM.

Parameter		Description
	
	dwInfo		Values that can have the following meaning:

	Type	Meaning
		
	CMS_VERSION	Retrieves the version of Windows supported.
	CMS_IDENT	Retrieves the identifier of the ICMDLL.
	CMS_DRIVER_LEVEL	Retrieves the support level of a device driver.

Returns
	CMGetInfo returns zero if an invalid parameter is passed in. If successful it returns a value that depends on the information requested.
	For CMS_VERSION CMGetInfo retrieves the version of Windows ICM interface supported by this module. For Windows 95 this should be 4.0, represented as 0x00040000.
	For CMS_IDENT CMGetInfo retrieves the identifier of the ICMDLL. This is the same as the ICC color profile header identifier.
	For CMS_DRIVER_LEVEL CMGetInfo retrieves the supported level of the device driver. ICMDLLs should return CMS_LEVEL_1. The values have been defined in a previous section.
   _____________________________________________________________________ */
DWORD WINAPI CMGetInfo( DWORD dwInfo )
{
	DWORD ret = 0;
	switch( dwInfo ){
	case CMM_VERSION:
		ret = 0x00050000;
		break;
	case CMM_WIN_VERSION:
		ret = 0x00040000;
		break;
	case CMM_DLL_VERSION:
		ret = 0x00010000;
		break;
	case CMM_IDENT:
		ret = 'Win ';
		break;
	case CMM_LOGOICON:
		ret = 100;
		break;
	case CMM_DESCRIPTION:
		ret = 100;
		break;
	case CMM_DRIVER_LEVEL:
		ret = CMS_LEVEL_1;
		break;
	default:
		ret = 0;
		break;
	}
	return 	ret;
}


long CMCreateTransformExtInternal(	CMWorldRef	*cwOut,
									UINT32		dwFlags,
									UINT32		lcsIntent,
									HPROFILE	aProf,
									LPBYTE		ptDeRef,
									LPBYTE		pdDeRef )
{
	CMWorldRef cw=0;
	CMWorldRef cw2=0;
	long err;
	HPROFILE saveProf;
	HPROFILE hArray[3];
	UINT32 aIntent[3];
	UINT32 aUINT32,count;
	CMMModelPtr theModelPtr;

	*cwOut = 0;
	aIntent[0] = icRelativeColorimetric;
	switch( lcsIntent ){
	  case LCS_GM_BUSINESS:
		aIntent[1] = icSaturation;
		break;
	  case LCS_GM_GRAPHICS:
		aIntent[1] = icRelativeColorimetric;
		break;
	  case LCS_GM_ABS_COLORIMETRIC:
		aIntent[1] = icAbsoluteColorimetric;
		break;
	  default:
		aIntent[1] = icPerceptual;
		break;
	}
	if( ptDeRef == 0 ){
		count = 2;
		hArray[0] =  aProf;
		hArray[1] =  pdDeRef;
	}
	else{
		count = 3;
		hArray[0] =  aProf;
		hArray[1] =  ptDeRef;
		hArray[2] =  pdDeRef;
		if( dwFlags & USE_RELATIVE_COLORIMETRIC )aIntent[2] = INTENT_RELATIVE_COLORIMETRIC;
		else aIntent[2] = INTENT_ABSOLUTE_COLORIMETRIC;
	}

	err = CMCreateMultiProfileTransformInternal( &cw, hArray, count, aIntent, count, dwFlags, 0 );
	if( err ){
		goto CleanupAndExit;
	}
	if( dwFlags & 0x80000000 ){				/* this is for the backward transform */
		count--;
		saveProf = hArray[count];
		hArray[count] =  hArray[0];
		hArray[0] =  saveProf;
		aUINT32 = aIntent[count];
		aIntent[count] = aIntent[0];
		aIntent[0] = aUINT32;

		count++;
		err = CMCreateMultiProfileTransformInternal( &cw2, hArray, count, aIntent, count, dwFlags, 0 );
		if( err ){
			CWDisposeColorWorld( cw );	/* delete other cw */
			goto CleanupAndExit;
		}
		LOCK_DATA( cw );
		theModelPtr = (CMMModelPtr)(DATA_2_PTR( cw ));
		theModelPtr->pBackwardTransform = cw2;
		UNLOCK_DATA( aTrans );
	}

CleanupAndExit:
	CloseColorProfile( aProf );
	if( err ){
		SetLastError( err );
		cw = (CMWorldRef)(ULONG_PTR)(err & 255);
	}
	else{
		cw = StoreTransform( cw );
	}
	*cwOut = cw;
	return 0;
}
/* ______________________________________________________________________

HCMTRANSFORM  WINAPI CMCreateTransformExt(	LPLOGCOLORSPACEA	lpColorSpace,
											LPDEVCHARACTER		lpDevCharacter,
											LPDEVCHARACTER		lpTargetDevCharacter,
											UINT32				dwFlags );

Abstract:
	The CMCreateTransformExt function creates a color transform that
	maps from an input LogColorSpace to an optional target to an
	output device.

Parameter					Description
	lpColorSpace			Pointer to input color space. If lcsFilename is non-zero,
							it is a pointer to the memory mapped profile.
	LpDevCharacter			Pointer to memory mapped device profile
	lpTargetDevCharacter	Pointer to memory mapped target profile
	dwFlags					Specifies flags to control creation of the transform.
							It is up to the CMM to determine how best to use these flags.
							Set the high-order word to ENABLE_GAMUT_CHECKING if the transform
							will be used for gamut checking.
							The low-order WORD can have one of the following constant values:
							PROOF_MODE, NORMAL_MODE, BEST_MODE.
							Moving from PROOF_MODE to BEST_MODE, output quality generally improves and transform speed declines.

Returns
	If the function is successful, it returns a color transform
	in the range 256 to 65535. Otherwise it returns an error code
	(return value < 255).

  _____________________________________________________________________ */
void WriteProf( LPSTR name, icProfile *theProf, long currentSize );
HCMTRANSFORM  WINAPI CMCreateTransformExt(	LPLOGCOLORSPACEA	lpColorSpace,
											LPDEVCHARACTER		lpDevCharacter,
											LPDEVCHARACTER		lpTargetDevCharacter,
											DWORD				dwFlags )
{
	CMWorldRef cw=0;
	long err;
	LPBYTE pt,pd, ptDeRef, pdDeRef;
	HPROFILE aProf;
	PROFILE theProf;
    BOOL   bWin95;
    OSVERSIONINFO osvi;

#if 0				/* Test for CMCreateProfile */
	CMCreateProfile( lpColorSpace, &pt );
	err = *(long *)pt;
	SwapLong(&err);
	WriteProf( "CMCreateProfile.icc", (icProfile *)pt, err );
	GlobalFreePtr( pt );
#endif


	err = FillProfileFromLog( lpColorSpace, &theProf );
	if( err ){
		SetLastError( err );
		goto CleanupAndExit;
	}
	aProf = OpenColorProfile(&theProf, PROFILE_READ, FILE_SHARE_READ, OPEN_EXISTING );
	if( !aProf ){
		err = GetLastError();
		goto CleanupAndExit;
	}
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
    bWin95 = osvi.dwMajorVersion == 4 &&
             osvi.dwMinorVersion == 0 &&
             osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS;

    if (bWin95)
    {
        PROFILE myProf;
        DWORD   l;

        //
        // Handles are not provided below LCS structure, so create handles
        //

        myProf.dwType = PROFILE_MEMBUFFER;
        myProf.pProfileData = lpDevCharacter;
        l = *(DWORD *)(myProf.pProfileData);
        myProf.cbDataSize = SwapLong(&l);
        pdDeRef = OpenColorProfile(&myProf, PROFILE_READ, FILE_SHARE_READ, OPEN_EXISTING );
        if( !pdDeRef ){
                err = GetLastError();
                goto CleanupAndExit;
        }

        ptDeRef = 0;
        if (lpTargetDevCharacter)
        {
            myProf.dwType = PROFILE_MEMBUFFER;
            myProf.pProfileData = lpTargetDevCharacter;
            l = *(DWORD *)(myProf.pProfileData);
            myProf.cbDataSize = SwapLong(&l);
            ptDeRef = OpenColorProfile(&myProf, PROFILE_READ, FILE_SHARE_READ, OPEN_EXISTING );
            if( !ptDeRef ){
                    err = GetLastError();
                    goto CleanupAndExit;
            }
        }
    }
    else
    {
		pd = ((LPBYTE)lpColorSpace+sizeof(LOGCOLORSPACEA));
		pdDeRef= (LPBYTE)*(PULONG_PTR)pd;

		pt = ((LPBYTE)lpColorSpace+sizeof(LOGCOLORSPACEA)+sizeof(HPROFILE));
		ptDeRef= (LPBYTE)*(PULONG_PTR)pt;
	}

	err = CMCreateTransformExtInternal(	&cw, dwFlags,
				lpColorSpace->lcsIntent, aProf, ptDeRef, pdDeRef );

CleanupAndExit:
	if( lpColorSpace->lcsFilename[0] == 0 ){
		if( theProf.pProfileData )GlobalFreePtr( theProf.pProfileData );
		theProf.pProfileData = 0;
	}
    if (bWin95)
    {
        if (pdDeRef)
        {
            CloseColorProfile(pdDeRef);
        }
        if (ptDeRef)
        {
            CloseColorProfile(ptDeRef);
        }
    }
	if( err ){
		return (HCMTRANSFORM)(ULONG_PTR)(err & 255);
	}
	return (HCMTRANSFORM)cw;
}

/* ______________________________________________________________________

HCMTRANSFORM  WINAPI CMCreateTransform(		LPLOGCOLORSPACEA	lpColorSpace,
											LPDEVCHARACTER		lpDevCharacter,
											LPDEVCHARACTER 		lpTargetDevCharacter );

Abstract:
	The CMCreateTransform function creates a color transform that
	maps from an input LogColorSpace to an optional target to an
	output device.

Parameter					Description
	lpColorSpace			Pointer to input color space. If lcsFilename is non-zero,
							it is a pointer to the memory mapped profile.
	LpDevCharacter			Pointer to memory mapped device profile
	lpTargetDevCharacter	Pointer to memory mapped target profile

Returns
	If the function is successful, it returns a color transform
	in the range 256 to 65535. Otherwise it returns an error code
	(return value < 255).

  _____________________________________________________________________ */
HCMTRANSFORM  WINAPI CMCreateTransform	(	LPLOGCOLORSPACEA	lpColorSpace,
											LPDEVCHARACTER		lpDevCharacter,
											LPDEVCHARACTER 		lpTargetDevCharacter )
{
	return CMCreateTransformExt( lpColorSpace, lpDevCharacter, lpTargetDevCharacter, PROOF_MODE | ENABLE_GAMUT_CHECKING | 0x80000000 );
}
/* ______________________________________________________________________

HCMTRANSFORM  WINAPI CMCreateTransformExtW(	LPLOGCOLORSPACEW	lpColorSpace,
											LPDEVCHARACTER		lpDevCharacter,
											LPDEVCHARACTER 		lpTargetDevCharacter,
											DWORD				dwFlags );

Abstract:
	The CMCreateTransformExtW function creates a color transform that
	maps from an input LogColorSpace to an optional target to an
	output device.

Parameter					Description
	lpColorSpace			Pointer to input color space. If lcsFilename is non-zero, it is a pointer to the memory mapped profile.
	LpDevCharacter			Pointer to memory mapped device profile
	lpTargetDevCharacter	Pointer to memory mapped target profile
	dwFlags					Specifies flags to control creation of the transform.
							It is up to the CMM to determine how best to use these flags.
							Set the high-order word to ENABLE_GAMUT_CHECKING if the transform
							will be used for gamut checking.
							The low-order WORD can have one of the following constant values:
							PROOF_MODE, NORMAL_MODE, BEST_MODE.
							Moving from PROOF_MODE to BEST_MODE, output quality generally improves and transform speed declines.

Returns
	If the function is successful, it returns a color transform
	in the range 256 to 65535. Otherwise it returns an error code
	(return value < 255).

   _____________________________________________________________________ */
HCMTRANSFORM  WINAPI CMCreateTransformExtW(	LPLOGCOLORSPACEW 	lpColorSpace,
											LPDEVCHARACTER		lpDevCharacter,
											LPDEVCHARACTER 		lpTargetDevCharacter,
											DWORD				dwFlags )
{
	CMWorldRef cw=0;
	long err;
	LPBYTE pt,pd, ptDeRef, pdDeRef;
	HPROFILE aProf;
	PROFILE theProf;

	err = FillProfileFromLogW( lpColorSpace, &theProf );
	if( err ){
		SetLastError( err );
		goto CleanupAndExit;
	}
	aProf = OpenColorProfileW(&theProf, PROFILE_READ, FILE_SHARE_READ, OPEN_EXISTING );
	if( !aProf ){
		err = GetLastError();
		goto CleanupAndExit;
	}
	pd = ((LPBYTE)lpColorSpace+sizeof(LOGCOLORSPACEW));
	pdDeRef= (LPBYTE)*(PULONG_PTR)pd;

	pt = ((LPBYTE)lpColorSpace+sizeof(LOGCOLORSPACEW)+sizeof(HPROFILE));
	ptDeRef= (LPBYTE)*(PULONG_PTR)pt;

	err = CMCreateTransformExtInternal(	&cw, dwFlags,
				lpColorSpace->lcsIntent, aProf, ptDeRef, pdDeRef );

CleanupAndExit:
	if( lpColorSpace->lcsFilename[0] == 0 ){
		if( theProf.pProfileData )GlobalFreePtr( theProf.pProfileData );
		theProf.pProfileData = 0;
	}
	if( err ){
		return (HCMTRANSFORM)(ULONG_PTR)(err & 255);
	}
	return (HCMTRANSFORM)cw;
}

/* ______________________________________________________________________

HCMTRANSFORM  WINAPI CMCreateTransformW(	LPLOGCOLORSPACEW	lpColorSpace,
											LPDEVCHARACTER		lpDevCharacter,
											LPDEVCHARACTER 		lpTargetDevCharacter );

Abstract:
	The CMCreateTransformW function creates a color transform that
	maps from an input LogColorSpace to an optional target to an
	output device.

Parameter					Description
	lpColorSpace			Pointer to input color space. If lcsFilename is non-zero, it is a pointer to the memory mapped profile.
	LpDevCharacter			Pointer to memory mapped device profile
	lpTargetDevCharacter	Pointer to memory mapped target profile

Returns
	If the function is successful, it returns a color transform
	in the range 256 to 65535. Otherwise it returns an error code
	(return value < 255).

   _____________________________________________________________________ */
HCMTRANSFORM  WINAPI CMCreateTransformW(	LPLOGCOLORSPACEW 	lpColorSpace,
											LPDEVCHARACTER		lpDevCharacter,
											LPDEVCHARACTER 		lpTargetDevCharacter )
{
	return CMCreateTransformExtW( lpColorSpace, lpDevCharacter, lpTargetDevCharacter, PROOF_MODE | ENABLE_GAMUT_CHECKING );
}

long  CMCreateMultiProfileTransformInternal(		CMWorldRef	*cw,
													LPHPROFILE	lpahProfiles,
													DWORD 		nProfiles,
													DWORD		*aIntentArr,
													DWORD		nIntents,
													DWORD		dwFlags,
													DWORD		dwCreateLink )
{
	CMConcatProfileSet	*profileSet;
	OSErr				aOSErr;
	DWORD				i;
	long				err;
	UINT32				theFlags;
	UINT32				*arrIntents = 0;;

	profileSet = (CMConcatProfileSet *)SmartNewPtrClear( sizeof (CMConcatProfileSet) + (nProfiles)* sizeof(CMProfileRef), &aOSErr );
	if (aOSErr == 0 )
	{
		profileSet->keyIndex = 0;
		profileSet->count = (unsigned short)nProfiles;
		for( i=0; i<nProfiles; i++)
			profileSet->profileSet[i] = (CMProfileRef)((DWORD *)(((PVOID *)lpahProfiles)[i]));
	}
	else return ERROR_NOT_ENOUGH_MEMORY;

	switch( dwFlags & 0xffff ){
		case BEST_MODE:
			theFlags = cmBestMode;
			break;
		case PROOF_MODE:
			theFlags = cmDraftMode;
			break;
		default:
			theFlags = cmNormalMode;
			break;
	}
	if( ! (dwFlags & ENABLE_GAMUT_CHECKING) ){
		theFlags |= kCreateGamutLutMask;
	}
	if( dwFlags & USE_RELATIVE_COLORIMETRIC ){
		theFlags |= kUseRelColorimetric;
	}
	if( dwFlags & FAST_TRANSLATE ){
		theFlags |= kLookupOnlyMask;
	}
	if( nProfiles > 1 && nIntents == 1 ){
		arrIntents = (UINT32 *)SmartNewPtrClear( nProfiles * sizeof (UINT32), &aOSErr);
		if (aOSErr != 0 ){
			err = ERROR_NOT_ENOUGH_MEMORY;
			goto CleanupAndExit;
		}
		arrIntents[0] = icPerceptual;
		arrIntents[1] = aIntentArr[0];
		for( i=2; i<nProfiles; i++){
			arrIntents[i] = icAbsoluteColorimetric;
			if( dwFlags & kUseRelColorimetric ) arrIntents[i] = icRelativeColorimetric;
		}
		err = CWConcatColorWorld4MS( cw, profileSet, arrIntents, nProfiles, theFlags );
		arrIntents= (UINT32 *)DisposeIfPtr( (Ptr) arrIntents );
	}
	else{
		err = CWConcatColorWorld4MS( cw, profileSet, aIntentArr, nIntents, theFlags );
	}
#ifdef WRITE_PROFILE
	if( err == 0 ){
		err = MyNewDeviceLink( *cw, profileSet, "MyCreateTransform.pf" );
		//goto CleanupAndExit;
	}
#endif
CleanupAndExit:
	profileSet= (CMConcatProfileSet *)DisposeIfPtr( (Ptr) profileSet );
	return err;
}

/* ______________________________________________________________________

HCMTRANSFORM  WINAPI CMCreateMultiProfileTransform(	LPHPROFILE	lpahProfiles,
													DWORD 		nProfiles,
													UINT32		*aIntentArr,
													UINT32		nIntents,
													UINT32		dwFlags );

Abstract:
	The CMCreateMultiProfileTransform function accepts
	an array of profiles or a single device link profile
	and creates a color transform.
	This transform is a mapping from the color space specified
	by the first profile to that of the second profile
	and so on until the last one.

Parameter			Description
	lpahProfiles	Pointer to an array of profile handles
	nProfiles		Number of profiles in the array
	padwIntents		(in)		Points to an array of intent structures.
								0 = default behavior ( intents out of profiles )

	nIntents		(in)		Specifies the number of intents in the intent array.
								Can be 1, or the same value as nProfiles.

	dwFlags			(in)		Specifies flags to control creation of the transform.
								These flags are intended only as hints, and it is up to the CMM
								to determine how best to use these flags.
								Set the high-order word to ENABLE_GAMUT_CHECKING if the transform will be used
								for gamut checking.
								The low-order WORD can have one of the following constant values:
								PROOF_MODE, NORMAL_MODE, BEST_MODE. Moving from PROOF_MODE to BEST_MODE,
Returns
	If the function is successful, it returns a color transform.
	Otherwise it returns an error code (return value < 255).
   _____________________________________________________________________ */
HCMTRANSFORM  WINAPI CMCreateMultiProfileTransform(	LPHPROFILE	lpahProfiles,
													DWORD 		nProfiles,
													DWORD		*aIntentArr,
													DWORD		nIntents,
													DWORD		dwFlags )
{
	long				err;
	CMWorldRef			cw;

	err = CMCreateMultiProfileTransformInternal( &cw, lpahProfiles, nProfiles, aIntentArr, nIntents, dwFlags, 0 );
	if( err ){
		SetLastError( err );
		return (HCMTRANSFORM)(ULONG_PTR)(err & 255);
	}
	cw = StoreTransform( cw );
	return (HCMTRANSFORM)cw;
}

/* ______________________________________________________________________
BOOL  WINAPI CMDeleteTransform( HCMTRANSFORM 	hcmTransform );

Abstract:
	The CMDeleteTransform function deletes the given color transform,
	and frees any memory associated with it.

Parameter			Description
	hcmTransform	Identifies the color transform

Returns
	If the function is successful, the return value is nonzero.
	Otherwise it is zero.
   _____________________________________________________________________ */
BOOL  WINAPI CMDeleteTransform( HCMTRANSFORM 	hcmTransform )
{
	long actTransform = (long)(ULONG_PTR)hcmTransform - 256;
	HCMTRANSFORM aTrans = NULL;
	CMMModelPtr theModelPtr;
	CMWorldRef theWorldRef;
    BOOL bReturn = 0;

	__try {
		EnterCriticalSection(&GlobalCriticalSection);
		if( actTransform < IndexTransform && actTransform >= 0 ){
			aTrans = TheTransform[actTransform];
			TheTransform[ actTransform ] = 0;
			if( actTransform == IndexTransform-1 )IndexTransform--;
			bReturn = 1;
		}
	}
	__finally{
		LeaveCriticalSection(&GlobalCriticalSection);
		
		LOCK_DATA( aTrans );
		theModelPtr = (CMMModelPtr)(DATA_2_PTR( aTrans ));
		theWorldRef = theModelPtr->pBackwardTransform;
		UNLOCK_DATA( aTrans );
		if( theWorldRef != 0 ){
			CWDisposeColorWorld( theWorldRef );
		}

		CWDisposeColorWorld( aTrans );
	}
    return bReturn;
}

/* ______________________________________________________________________
BOOL  WINAPI CMCreateProfile(	LPLOGCOLORSPACEA		lpColorSpace,
								LPBYTE 					*lpProfileData );

Abstract:
	The CMCreateProfile function creates a display color profile
	from a LogColorSpace structure.

Parameter			Description
	lpColorSpace	Pointer to color space. lcsFilename field will be NULL.
	pProfileData	Points to a pointer to a buffer.
					If successful the function allocates and fills this buffer.
					It is then the calling application’s responsibility to
					free this buffer with GlobalFreePtr( lpProfileData )
					when it is no longer needed.

Returns
	If the function is successful, it returns nonzero.
	Otherwise it returns zero.

   _____________________________________________________________________ */
BOOL  WINAPI CMCreateProfile(	LPLOGCOLORSPACEA	lpColorSpace,
								LPBYTE 				*lpProfileData )
{
	CMWorldRef cw=0;
	long err;

	if( lpColorSpace->lcsFilename[0] ) return 0;
	err = MyNewAbstract( lpColorSpace, (icProfile **)lpProfileData );
	//err = FillProfileFromLog( lpColorSpace, &theProf );
	if( err ){
		SetLastError( err );
		goto CleanupAndExit;
	}
	return 1;
CleanupAndExit:
	return 0;
}

/* ______________________________________________________________________
BOOL  WINAPI CMCreateProfileW(	LPLOGCOLORSPACEW	lpColorSpace,
								LPBYTE 				*lpProfileData );

Abstract:
	The CMCreateProfileW function creates a display color profile
	from a LogColorSpace structure.

Parameter			Description
	lpColorSpace	Pointer to color space. lcsFilename field will be NULL.
	pProfileData	Points to a pointer to a buffer.
					If successful the function allocates and fills this buffer.
					It is then the calling application’s responsibility to
					free this buffer with GlobalFreePtr( lpProfileData )
					when it is no longer needed.

Returns
	If the function is successful, it returns nonzero.
	Otherwise it returns zero.

   _____________________________________________________________________ */
BOOL  WINAPI CMCreateProfileW(	LPLOGCOLORSPACEW	lpColorSpace,
								LPBYTE 				*lpProfileData )
{
	CMWorldRef cw=0;
	long err;

	if( lpColorSpace->lcsFilename[0] ) return 0;
	err = MyNewAbstractW( lpColorSpace, (icProfile **)lpProfileData );
	//err = FillProfileFromLogW( lpColorSpace, &theProf );
	if( err ){
		SetLastError( err );
		goto CleanupAndExit;
	}
	return 1;
CleanupAndExit:
	return 0;
}

/* ______________________________________________________________________
BOOL  WINAPI CMCreateDeviceLinkProfile(		LPHPROFILE	lpahProfiles,
											DWORD 		nProfiles,
											UINT32		*aIntentArr,
											UINT32		nIntents,
											UINT32		dwFlags,
											LPBYTE		*lpProfileData );

Abstract:
	The CMCreateDeviceLinkProfile function creates a device link
	profile as specified by the "ICC Profile Format Specification."

Parameter			Description
	lpahProfiles	Pointer to an array of profile handles
	nProfiles		Number of profiles in the array
	padwIntents		Points to an array of rendering intents.
					Each rendering intent is represented by one of the following values:
						INTENT_PERCEPTUAL
						INTENT_SATURATION
						INTENT_RELATIVE_COLORIMETRIC
						INTENT_ABSOLUTE_COLORIMETRIC
					For more information, see Rendering Intents.

	nIntents		Specifies the number of intents in the intent array. Can be 1, or the same value as nProfiles.
	dwFlags			Specifies flags to control creation of the transform. These flags are intended only as hints,
					and it is up to the CMM to determine how best to use these flags.
					Set the high-order word to ENABLE_GAMUT_CHECKING if the transform will be used for gamut checking.
					The low-order WORD can have one of the following constant values:
						PROOF_MODE, NORMAL_MODE, BEST_MODE. Moving from PROOF_MODE to BEST_MODE,
						output quality generally improves.

	lpProfileData	Points to a pointer to a buffer.
					If successful the function allocates and fills this buffer.
					It is then the calling application’s responsibility to
					free this buffer with GlobalFreePtr( lpProfileData )
					when it is no longer needed.

Returns
	If the function is successful, it returns nonzero.
	Otherwise it returns zero. SetLastError is used.

   _____________________________________________________________________ */

BOOL  WINAPI CMCreateDeviceLinkProfile(	LPHPROFILE	lpahProfiles,
										DWORD 		nProfiles,
										DWORD		*aIntentArr,
										DWORD		nIntents,
										DWORD		dwFlags,
										LPBYTE		*lpProfileData )
{
	long				err;
	OSErr				aOSErr;
	CMWorldRef			cw;
	CMConcatProfileSet	*profileSet;
	UINT32				i;

	*lpProfileData = 0;
	
	err = CMCreateMultiProfileTransformInternal( &cw, lpahProfiles, nProfiles, aIntentArr, nIntents, dwFlags, 0 );
	if( err ){
		SetLastError( err );
		return 0;
	}
	profileSet = (CMConcatProfileSet *)SmartNewPtrClear(sizeof (CMConcatProfileSet) + (nProfiles)* sizeof(CMProfileRef),&aOSErr);
	err = aOSErr;
	if (aOSErr == 0 )
	{
		profileSet->keyIndex = 0;
		profileSet->count = (unsigned short)nProfiles;
		for( i=0; i<nProfiles; i++)
			profileSet->profileSet[i] = (CMProfileRef)((DWORD *)(((PVOID *)lpahProfiles)[i]));
	}
	else goto CleanupAndExit;

	err = DeviceLinkFill( (CMMModelPtr)cw, profileSet, (icProfile **)lpProfileData, aIntentArr[0]  );
	profileSet= (CMConcatProfileSet *)DisposeIfPtr( (Ptr) profileSet );

	if( err )goto CleanupAndExit;
	CWDisposeColorWorld ( cw );
	return 1;

CleanupAndExit:
	CWDisposeColorWorld ( cw );
	SetLastError( err );
	return 0;
}

/* ______________________________________________________________________
BOOL  WINAPI CMCreateDeviceLinkProfile(	LPHPROFILE	lpahProfiles,
										DWORD 		nProfiles,
										LPBYTE		*lpProfileData );

Abstract:
	The CMCreateDeviceLinkProfile function creates a device link
	profile as specified by the "ICC Profile Format Specification."

Parameter			Description
	lpahProfiles	Pointer to an array of profile handles
	nProfiles		Number of profiles in the array
	pProfileData	Points to a pointer to a buffer.
					If successful the function allocates and fills this buffer.
					It is then the calling application’s responsibility to
					free this buffer with GlobalFreePtr( lpProfileData )
					when it is no longer needed.

Returns
	If the function is successful, it returns nonzero.
	Otherwise it returns zero. SetLastError is used.

   _____________________________________________________________________ */

/*BOOL  WINAPI CMCreateDeviceLinkProfile(	LPHPROFILE	lpahProfiles,
										DWORD 		nProfiles,
										LPBYTE		*lpProfileData )
{
	long			err;
	UINT32			*arrIntents = 0;
	OSErr			aOSErr;
	DWORD			i;

	arrIntents = (UINT32 *)SmartNewPtrClear( nProfiles * sizeof (UINT32), &aOSErr);
	if (aOSErr != 0 ) return ERROR_NOT_ENOUGH_MEMORY;

	for( i=0; i<nProfiles; i++){
		arrIntents[i] = icPerceptual;
	}

	err = CMCreateDeviceLinkProfileExt( lpahProfiles, nProfiles, arrIntents, nProfiles, BEST_MODE, lpProfileData );
	
	arrIntents = (UINT32 *)DisposeIfPtr( (Ptr)arrIntents );
	return err;
}
*/
/* ______________________________________________________________________
BOOL  WINAPI CMIsProfileValid (	HPROFILE	hProfile,	
								LPBOOL		lpbValid	
							  );

Abstract:
	The CMIsProfileValid function reports if the given profile is a valid ICC profile that can be used for color matching.

Parameter			Description
	lpDevCharacter	Pointer to memory mapped profile
	lpbValid		Points to a variable that is set on exit to TRUE if the profile is a valid ICC profile, or FALSE if not.

Returns
	If it is a valid ICC profile that can be used for color matching,
	the return value is nonzero. Otherwise it is zero.
   _____________________________________________________________________ */
BOOL  WINAPI CMIsProfileValid(	HPROFILE	hProfile,	
								LPBOOL		lpbValid	
							  )
{
	Boolean valid;

	CMValidateProfile( (CMProfileRef)hProfile, &valid );

	*lpbValid = valid;
	return valid;
}

/* ______________________________________________________________________
BOOL  WINAPI CMTranslateColors(	HCMTRANSFORM	hcmTransform,
								LPCOLOR 		lpaInputColors,
								DWORD 			nColors,
								COLORTYPE		ctInput,
								LPCOLOR 		lpaOutputColors,
								COLORTYPE		ctOutput );
Abstract:
	The CMTranslateColors function translates an array of colors from
	the source colorspace to the destination colorspace as defined by
	the color transform.

Parameter			Description
	hcmTransform	Identifies the color transform to use
	lpaInputColors	Pointer to an array of COLOR structures to translate
	nColors			Number of elements in the array
	ctInput			Specifies the input color type
	lpaOutputColors	Pointer to an array of COLOR structures that receive the translated colors
	ctOutput		Specifies the output color type

Returns
	If the function is successful, the return value is nonzero.
	Otherwise it is zero.

   _____________________________________________________________________ */
BOOL  WINAPI CMTranslateColors(	HCMTRANSFORM	hcmTransform,
								LPCOLOR 		lpaInputColors,
								DWORD 			nColors,
								COLORTYPE		ctInput,
								LPCOLOR 		lpaOutputColors,
								COLORTYPE		ctOutput )
{
	long matchErr;
	DWORD i;

    if (lpaOutputColors != lpaInputColors){
    	for( i=0; i<nColors; i++) lpaOutputColors[ i ] = lpaInputColors[ i ];
    }

	matchErr = CWMatchColors( CMGetTransform( hcmTransform ), (CMColor *)lpaOutputColors, nColors );
	if( matchErr ){
		SetLastError( matchErr );
		return 0;
	}
	return 1;
}

/* ______________________________________________________________________
BOOL  WINAPI CMCheckColors(	HCMTRANSFORM	hcmTransform,
							LPCOLOR 		lpaInputColors,
							DWORD			nColors,
							COLORTYPE		ctInput,
							LPBYTE 			lpaResult );
Abstract:
	The CMCheckColors function determines if the given colors lie
	within the output gamut of the given transform.

Parameter			Description
	hcmTransform	Identifies the color transform to use
	lpaInputColors	Pointer to an array of COLOR structures
	nColors			Number of elements in the array
	ctInput			Input color type
	lpaResult		Pointer to an array of bytes that hold the result

Returns
	If the function is successful, the return value is nonzero.
	Otherwise it is zero.
	The lpaResult array holds the results, each byte corresponds to a
	COLOR element and has a value between 0 and 255.
	The value 0 denotes that the color is in gamut; a non-zero value
	implies that it is out of gamut, with the number "n+1" being at
	least as far out of gamut as the number "n". These values are
	usually generated from the gamutTag in the ICC profile.

   _____________________________________________________________________ */
BOOL  WINAPI CMCheckColors(	HCMTRANSFORM	hcmTransform,
							LPCOLOR 		lpaInputColors,
							DWORD			nColors,
							COLORTYPE		ctInput,
							LPBYTE 			lpaResult )
{
	long matchErr;

	matchErr = CWCheckColorsMS( CMGetTransform( hcmTransform ), (CMColor *)lpaInputColors, nColors, lpaResult );
	if( matchErr ){
		SetLastError( matchErr );
		return 0;
	}
	return 1;
}

/* ______________________________________________________________________
BOOL  WINAPI CMTranslateRGBs(	HCMTRANSFORM	hcmTransform,
								LPVOID			lpSrcBits,
								BMFORMAT		bmInput,
								DWORD			dwWidth,
								DWORD			dwHeight,
								DWORD			dwStride,
								LPVOID			lpDestBits,
								BMFORMAT		bmOutput,
								DWORD			dwTranslateDirection );
Abstract:
	The CMTranslateRGBs function takes a bitmap in one of the defined
	formats and translates the colors in the bitmap producing another
	bitmap in the requested format.

Parameter					Description
	hcmTransform			Identifies the color transform to use
	lpSrcBits				Pointer to bitmap to translate
	bmInput					Input bitmap format
	dwWidth					Number of pixels per scanline of input data
	dwHeight				Number of	 scanlines of input data
	dwStride				Number of bytes from beginning of one scanline to beginning of next
	lpDestBits				Pointer to buffer to receive translated data
	bmOutput				Output bitmap format
	dwTranslateDirection	Describes direction of transform
	Value	Meaning
	CMS_FORWARD	Use forward transform
	CMS_BACKWARD	Use reverse transform  // NOT supported

Returns
	If the function is successful, the return value is nonzero.
	Otherwise it is zero.
   _____________________________________________________________________ */
BOOL  WINAPI CMTranslateRGBs(	HCMTRANSFORM	hcmTransform,
								LPVOID			lpSrcBits,
								BMFORMAT		bmInput,
								DWORD			dwWidth,
								DWORD			dwHeight,
								DWORD			dwStride,
								LPVOID			lpDestBits,
								BMFORMAT		bmOutput,
								DWORD			dwTranslateDirection )
{
	CMBitmapColorSpace 		spaceIn,spaceOut;
	CMBitmap				InBitmap,OutBitmap;
	long matchErr, inPixelSize, outPixelSize;
	BOOL aBgrMode = FALSE;
	CMMModelPtr theModelPtr;
	CMWorldRef theWorldRef;
	HCMTRANSFORM theTransform = CMGetTransform( hcmTransform );
	
	if( dwTranslateDirection == CMS_BACKWARD ){
		if( theTransform == 0 ){
			SetLastError( (DWORD)cmparamErr );
			return 0;
		}
		LOCK_DATA( theTransform );
		theModelPtr = (CMMModelPtr)(DATA_2_PTR( theTransform ));
		theWorldRef = theModelPtr->pBackwardTransform;
		UNLOCK_DATA( theTransform );
		if( theWorldRef == 0 ){
			SetLastError( (DWORD)cmparamErr );
			return 0;
		}
		theTransform = (HCMTRANSFORM)theWorldRef;
	}

	spaceIn = CMGetDataColorSpace( bmInput, &inPixelSize );
	if( spaceIn == 0 ){
		SetLastError( (DWORD)cmInvalidColorSpace );
		return 0;
	}
	spaceOut = CMGetDataColorSpace( bmOutput, &outPixelSize );
	if( spaceOut == 0 ){
		SetLastError( (DWORD)cmInvalidColorSpace );
		return 0;
	}
	InBitmap.image		= lpSrcBits;
	InBitmap.width		= dwWidth;
	InBitmap.height		= dwHeight;
	if( dwStride == 0 ){
		InBitmap.rowBytes = ( dwWidth * (inPixelSize / 8) + 3 ) & ~3;
	}
	else{
		InBitmap.rowBytes	= dwStride;
	}
	InBitmap.pixelSize	= inPixelSize;
	InBitmap.space		= spaceIn;
		
	OutBitmap.image		= lpDestBits;
	OutBitmap.width		= dwWidth;
	OutBitmap.height	= dwHeight;
	OutBitmap.rowBytes	= ( dwWidth * (outPixelSize / 8) + 3 ) & ~3;
	OutBitmap.pixelSize	= outPixelSize;
	OutBitmap.space		= spaceOut;

	matchErr = CWMatchBitmap( theTransform, &InBitmap,
							  (CMBitmapCallBackUPP)0,(void *)NULL,&OutBitmap );
	if( matchErr ){
		SetLastError( matchErr );
		return 0;
	}
	return 1;
}

/* ______________________________________________________________________
BOOL  WINAPI CMTranslateRGBsExt(	HCMTRANSFORM	hcmTransform,
									LPVOID			lpSrcBits,
									BMFORMAT		bmInput,
									DWORD			dwWidth,
									DWORD			dwHeight,
									DWORD			dwInputStride,
									LPVOID			lpDestBits,
									BMFORMAT		bmOutput,
									DWORD			dwOutputStride,
									LPBMCALLBACKFN  lpfnCallback,
									LPARAM			ulCallbackData )
Abstract:
	The CMTranslateRGBs function takes a bitmap in one of the defined
	formats and translates the colors in the bitmap producing another
	bitmap in the requested format.

Parameter					Description
	hcmTransform			Specifies the color transform to use.
	lpSrcBits				Points to the bitmap to translate.
	bmInput					Specifies the input bitmap format.
	dwWidth					Specifies the number of pixels per scanline in the input bitmap.
	dwHeight				Specifies the number of scanlines in the input bitmap.
	dwInputStride			Specifies the number of bytes from the beginning of one scanline to
							the beginning of the next in the input bitmap.
							If dwInputStride is set to zero, the CMM should assume that scanlines
							are padded so as to be DWORD aligned.
	lpDestBits				Points to a destination buffer in which to place the translated bitmap.
	bmOutput				Specifies the output bitmap format.
	dwOutputStride			Specifies the number of bytes from the beginning of one scanline to the
							beginning of the next in the input bitmap.
							If dwOutputStride is set to zero, the CMM should pad scanlines so
							that they are DWORD aligned.
	lpfnCallback			Pointer to an application-supplied callback function called periodically
							by CMTranslateRGBsExt to report progress and allow the calling process
							to cancel the translation. (See ICMProgressProc.)
	ulCallbackData			Data passed back to the callback function, for example to identify the
							translation that is reporting progress.

Returns
	If the function is successful, the return value is nonzero.
	Otherwise it is zero.
   _____________________________________________________________________ */
BOOL  WINAPI CMTranslateRGBsExt(	HCMTRANSFORM	hcmTransform,
									LPVOID			lpSrcBits,
									BMFORMAT		bmInput,
									DWORD			dwWidth,
									DWORD			dwHeight,
									DWORD			dwInputStride,
									LPVOID			lpDestBits,
									BMFORMAT		bmOutput,
									DWORD			dwOutputStride,
									LPBMCALLBACKFN  lpfnCallback,
									LPARAM		ulCallbackData )
{
	CMBitmapColorSpace 		spaceIn,spaceOut;
	CMBitmap				InBitmap,OutBitmap;
	long matchErr, inPixelSize, outPixelSize;
	BOOL aBgrMode = FALSE;

	spaceIn = CMGetDataColorSpace( bmInput, &inPixelSize );
	if( spaceIn == 0 ){
		SetLastError( (DWORD)cmInvalidColorSpace );
		return 0;
	}
	spaceOut = CMGetDataColorSpace( bmOutput, &outPixelSize );
	if( spaceOut == 0 ){
		SetLastError( (DWORD)cmInvalidColorSpace );
		return 0;
	}
	InBitmap.image		= lpSrcBits;
	InBitmap.width		= dwWidth;
	InBitmap.height		= dwHeight;
	if( dwInputStride == 0 ){
		InBitmap.rowBytes = ( dwWidth * (inPixelSize / 8) + 3 ) & ~3;
	}
	else{
		InBitmap.rowBytes	= dwInputStride;
	}
	InBitmap.pixelSize	= inPixelSize;
	InBitmap.space		= spaceIn;
		
	OutBitmap.image		= lpDestBits;
	OutBitmap.width		= dwWidth;
	OutBitmap.height	= dwHeight;
	if( dwOutputStride == 0 ){
		OutBitmap.rowBytes = ( dwWidth * (outPixelSize / 8) + 3 ) & ~3;
	}
	else{
		OutBitmap.rowBytes	= dwOutputStride;
	}
	OutBitmap.pixelSize	= outPixelSize;
	OutBitmap.space		= spaceOut;

	matchErr = CWMatchBitmap( CMGetTransform( hcmTransform ), &InBitmap,
							  (CMBitmapCallBackUPP)lpfnCallback,(void *)ulCallbackData,&OutBitmap );
	if( matchErr ){
		SetLastError( matchErr );
		return 0;
	}
	return 1;
}

/* ______________________________________________________________________
BOOL  WINAPI CMCheckRGBs(	HCMTRANSFORM	hcmTransform,
							LPVOID			lpSrcBits,
							BMFORMAT		bmInput,
							DWORD			dwWidth,
							DWORD			dwHeight,
							DWORD			dwStride,
							LPBYTE			lpDestBits,
							PBMCALLBACKFN	pfnCallback,	
							LPARAM		ulCallbackData );
Abstract:
	The CMCheckRGBs function checks if the pixels in the bitmap lie
	within the output gamut of the given transform.

Parameter			Description

	hcmTransform	Specifies the color transform to use.
	lpSrcBits		Points to the bitmap to check against an output gamut.
	bmInput			Specifies the input bitmap format.
	dwWidth			Specifies the number of pixels per scanline in the input bitmap.
	dwHeight		Specifies the number of scanlines in the input bitmap.
	dwStride		Specifies the number of bytes from the beginning of one scanline
					to the beginning of the next in the input bitmap.
					If dwStride is set to zero, the CMM should assume that scanlines
					are padded so as to be DWORD-aligned.
	lpaResult		Points to a buffer in which the test results are to be placed.
					The results are represented by an array of bytes.
					Each byte in the array corresponds to a pixel in the bitmap,
					and on exit is set to an unsigned value between 0 and 255.
					The value 0 denotes that the color is in gamut,
					while a nonzero value denotes that it is out of gamut.
					For any integer n such that 0 < n < 255, a result value of n+1
					indicates that the corresponding color is at least as far out of
					gamut as would be indicated by a result value of n.
					These values are usually generated from the gamutTag in the ICC profile.
	pfnCallback		Pointer to an application-supplied callback function called periodically
					by CMCheckRGBs to report progress and allow the calling process
					to cancel the translation. (See ICMProgressProc.)
	ulCallbackData	Data passed back to the callback function, for example to identify
					the bitmap test that is reporting progress.

Returns
	If the function is successful, the return value is nonzero.
	Otherwise it is zero.
	The lpaResult array holds the results, each byte corresponds to a pixel
	and has a value between 0 and 255. The value 0 denotes that the color is in gamut;
	a non-zero value implies that it is out of gamut, with the number "n+1" being at
	least as far out of gamut as the number "n". These values are usually generated
	from the gamutTag in the ICC profile.
   _____________________________________________________________________ */
BOOL  WINAPI CMCheckRGBs(	HCMTRANSFORM	hcmTransform,
							LPVOID			lpSrcBits,
							BMFORMAT		bmInput,
							DWORD			dwWidth,
							DWORD			dwHeight,
							DWORD			dwStride,
							LPBYTE			lpDestBits,
							PBMCALLBACKFN	pfnCallback,	
							LPARAM		ulCallbackData )

{
	CMBitmapColorSpace 		spaceIn,spaceOut;
	CMBitmap				InBitmap,OutBitmap;
	long matchErr, inPixelSize;
	BOOL aBgrMode = FALSE;

	spaceIn = CMGetDataColorSpace( bmInput, &inPixelSize );
	if( spaceIn == 0 ){
		SetLastError( (DWORD)cmInvalidColorSpace );
		return 0;
	}
	spaceOut = cm8PerChannelPacking + cmGraySpace;

	if( spaceOut == 0 )return 0;
	InBitmap.image		= lpSrcBits;
	InBitmap.width		= dwWidth;
	InBitmap.height		= dwHeight;
	if( dwStride == 0 ){
		InBitmap.rowBytes = ( dwWidth * (inPixelSize / 8) + 3 ) & ~3;
	}
	else{
		InBitmap.rowBytes	= dwStride;
	}
	InBitmap.pixelSize	= inPixelSize;
	InBitmap.space		= spaceIn;
		
	OutBitmap.image		= lpDestBits;
	OutBitmap.width		= dwWidth;
	OutBitmap.height	= dwHeight;
	OutBitmap.rowBytes	= dwWidth;	// perhaps wrong format?
	OutBitmap.pixelSize	= 8;
	OutBitmap.space		= cmGamutResultSpace;

	matchErr = CWCheckBitmap(	CMGetTransform( hcmTransform ), &InBitmap,
								(CMBitmapCallBackUPP)pfnCallback,(void *)ulCallbackData,&OutBitmap );
	
	if( matchErr ){
		SetLastError( matchErr );
		return 0;
	}
	return 1;
}

/* ______________________________________________________________________
BOOL WINAPI CMTranslateRGB(	HCMTRANSFORM	hcmTransform,
							COLORREF		colorRef,
							LPCOLORREF		lpColorRef,
							DWORD			dwFlags );
Abstract:
	The CMTranslateRGB function translates an application supplied RGBQuad into the
	device color coordinate space.

Parameter			Description
	hcmTransform	Handle of transform to use.
	colorRef		RGBQuad to translate.
	lpColorRef		Pointer to buffer to store result.
	dwFlags			Flags that can have the following meaning

	Type			Meaning
		
	CMS_FORWARD		Specifies that the forward transform is to be used.
	CMS_BACKWARD	Specifies that the backward transform is to be used.  // NOT supported

Returns
	The return value is TRUE if the function is successful. Otherwise, it is NULL.

   _____________________________________________________________________ */
BOOL WINAPI CMTranslateRGB(	HCMTRANSFORM	hcmTransform,
							COLORREF		colorRef,
							LPCOLORREF		lpColorRef,
							DWORD			dwFlags )

{
	CMBitmapColorSpace 		spaceIn;
	CMBitmap				InBitmap,OutBitmap;
	long matchErr;
	COLORREF aColorRef = colorRef;
	BOOL aBgrMode = FALSE;
	CMBitmapColorSpace In,Out;
	CMMModelPtr theModelPtr;
	CMWorldRef theWorldRef;
	HCMTRANSFORM theTransform = CMGetTransform( hcmTransform );
	
	if( dwFlags == CMS_BACKWARD ){
		if( theTransform == 0 ){
			SetLastError( (DWORD)cmparamErr );
			return 0;
		}
		LOCK_DATA( theTransform );
		theModelPtr = (CMMModelPtr)(DATA_2_PTR( theTransform ));
		theWorldRef = theModelPtr->pBackwardTransform;
		UNLOCK_DATA( theTransform );
		if( theWorldRef == 0 ){
			SetLastError( (DWORD)cmparamErr );
			return 0;
		}
		theTransform = (HCMTRANSFORM)theWorldRef;
	}

	spaceIn = cmRGBA32Space;
	InBitmap.image		= (char *)(&aColorRef);
	InBitmap.width		= 1;
	InBitmap.height		= 1;
	InBitmap.rowBytes	= 4;
	InBitmap.pixelSize	= 32;
	InBitmap.space		= spaceIn;
	OutBitmap = InBitmap;	
	OutBitmap.image		= (char *)lpColorRef;

	matchErr = CWGetColorSpaces( theTransform, &In, &Out );
	if( matchErr ){
		SetLastError( matchErr );
		return 0;
	}
	if( Out == icSigCmykData ) OutBitmap.space = cmKYMC32Space;
	matchErr = CWMatchBitmap(	theTransform, &InBitmap,
								(CMBitmapCallBackUPP)0,(void *)NULL,&OutBitmap );
	if( matchErr ){
		SetLastError( matchErr );
		return 0;
	}
	return 1;
}


/* ______________________________________________________________________
BOOL WINAPI CMCheckColorsInGamut(	HCMTRANSFORM	hcmTransform,
									LPARGBQUAD		lpaRGBTriplet,
									LPBYTE			lpBuffer,
									UINT			nCount );
Abstract:
	The CMCheckColorInGamut determines if the given RGBs lie in the output gamut of the
	given transform.

Parameter			Description	
	hcmTransform	Handle of transform to use.
	lpaRGBTriples	Pointer to array of RGB triples to check.
	lpBuffer		Pointer to buffer to put results.
	nCount			Count of elements in array.


Returns
	The return value is TRUE if the function is successful. Otherwise, it is NULL.
	The lpBuffer holds the results, each byte corresponding to an RGB triple is a value in
	the range 0 to 255.
   _____________________________________________________________________ */
BOOL WINAPI CMCheckColorsInGamut(	HCMTRANSFORM	hcmTransform,
									RGBTRIPLE		*lpaRGBTriplet,
									LPBYTE			lpBuffer,
									UINT			nCount )
{
	CMBitmap				InBitmap,OutBitmap;
	long matchErr;
	BOOL aBgrMode = FALSE;

	InBitmap.image		= (char *)(lpaRGBTriplet);
	InBitmap.width		= nCount;
	InBitmap.height		= 1;
	InBitmap.rowBytes	= 3*nCount;
	InBitmap.pixelSize	= 24;
	InBitmap.space		= cm8PerChannelPacking + cmRGBSpace;
	OutBitmap = InBitmap;	
	OutBitmap.rowBytes	= nCount;	// perhaps wrong format?
	OutBitmap.pixelSize	= 8;
	OutBitmap.image		= (char *)lpBuffer;

	matchErr = CWCheckBitmap(	CMGetTransform( hcmTransform ), &InBitmap,
								(CMBitmapCallBackUPP)0,(void *)NULL,&OutBitmap );
	if( matchErr ){
		SetLastError( matchErr );
		return 0;
	}
	return 1;
}
/* ______________________________________________________________________
long FillProfileFromLog(	LPLOGCOLORSPACEA	lpColorSpace,
							PPROFILE			theProf )
Abstract:
	The FillProfileFromLog function convertes a lpColorSpace to a PROFILE.
	If lpColorSpace has a profile name the function returns a file based profile.
	Else it returns a memory based profile.

Parameter			Description	
	lpColorSpace	Handle of transform to use.
	theProf			Pointer to the profile.

Returns
	The return value is 0 if the function is successful. Otherwise, it is an error code.
   _____________________________________________________________________ */
long FillProfileFromLog(	LPLOGCOLORSPACEA	lpColorSpace,
							PPROFILE			theProf )
{
	long l;
	icProfile *aProf;
	CMError  err = -1;

	if( lpColorSpace->lcsFilename[0] ){
		theProf->pProfileData = (char *)lpColorSpace->lcsFilename;
		theProf->dwType = PROFILE_FILENAME;
		theProf->cbDataSize = lstrlenA((const unsigned char *)theProf->pProfileData) * sizeof(CHAR);
		err = 0;
	}
	else if( lpColorSpace->lcsCSType == LCS_CALIBRATED_RGB ){
		err = MyNewAbstract( lpColorSpace, &aProf );
		theProf->pProfileData = ((PVOID *)aProf);
		theProf->dwType = PROFILE_MEMBUFFER;
		l = *(DWORD *)(theProf->pProfileData);
		theProf->cbDataSize = SwapLong(&l);
	}
	else  theProf->pProfileData = 0;

	return err;
}

/* ______________________________________________________________________
long FillProfileFromLogW(	LPLOGCOLORSPACEW	lpColorSpace,
							PPROFILE			theProf )
Abstract:
	The FillProfileFromLog function convertes a lpColorSpace to a PROFILE.
	If lpColorSpace has a profile name the function returns a file based profile.
	Else it returns a memory based profile.

Parameter			Description	
	lpColorSpace	Handle of transform to use.
	theProf			Pointer to the profile.

Returns
	The return value is 0 if the function is successful. Otherwise, it is an error code.
   _____________________________________________________________________ */
long FillProfileFromLogW(	LPLOGCOLORSPACEW	lpColorSpace,
							PPROFILE			theProf )
{
	long l;
	icProfile *aProf;
	CMError  err = -1;

	if( lpColorSpace->lcsFilename[0] ){
		theProf->pProfileData = (char *)lpColorSpace->lcsFilename;
		theProf->dwType = PROFILE_FILENAME;
		theProf->cbDataSize = lstrlenW((const unsigned short *)theProf->pProfileData) * sizeof(WCHAR);
		err = 0;
	}
	else if( lpColorSpace->lcsCSType == LCS_CALIBRATED_RGB ){
		err = MyNewAbstractW( lpColorSpace, &aProf );
		theProf->pProfileData = ((PVOID *)aProf);
		theProf->dwType = PROFILE_MEMBUFFER;
		l = *(DWORD *)(theProf->pProfileData);
		theProf->cbDataSize = SwapLong(&l);
	}
	else  theProf->pProfileData = 0;

	return err;
}

/* ______________________________________________________________________
CMBitmapColorSpace CMGetDataColorSpace( BMFORMAT bmMode, long *pixelSize );

Abstract:
	The CMGetDataColorSpace function retrieves the CMBitmapColorSpace and
	the pixel size from the BMFORMAT.

Parameter			Description
	bmMode			Identifies the data format.
	pixelSize		Pointer to pixelsize.

Returns
	function returns the internal data format
________________________________________________________________ */
CMBitmapColorSpace CMGetDataColorSpace( BMFORMAT bmMode, long *pixelSize )
{
	switch(  bmMode ){
	case BM_565RGB:
		*pixelSize = 16;
		return cmWord565ColorPacking + cmRGBSpace;
		break;
    case BM_x555RGB:
		*pixelSize = 16;
		return cmWord5ColorPacking + cmRGBSpace;
		break;
	case BM_x555XYZ:
		*pixelSize = 16;
		return cmWord5ColorPacking + cmXYZSpace;
		break;
	case BM_x555Yxy:
		*pixelSize = 16;
		return cmWord5ColorPacking + cmYXYSpace;
		break;
	case BM_x555Lab:
		*pixelSize = 16;
		return cmWord5ColorPacking + cmLABSpace;
		break;
	case BM_x555G3CH:
		*pixelSize = 16;
		return cmWord5ColorPacking + cmGenericSpace;
		break;
	case BM_RGBTRIPLETS:
		*pixelSize = 24;
		return cm8PerChannelPacking + cmBGRSpace;
		break;
	case BM_BGRTRIPLETS:
		*pixelSize = 24;
		return cm8PerChannelPacking + cmRGBSpace;
		break;
	case BM_XYZTRIPLETS:
		*pixelSize = 24;
		return cm8PerChannelPacking + cmXYZSpace;
		break;
	case BM_YxyTRIPLETS:
		*pixelSize = 24;
		return cm8PerChannelPacking + cmYXYSpace;
		break;
	case BM_LabTRIPLETS:
		*pixelSize = 24;
		return cm8PerChannelPacking + cmLABSpace;
		break;
	case BM_G3CHTRIPLETS:
		*pixelSize = 24;
		return cm8PerChannelPacking + cmGenericSpace;
		break;
	case BM_5CHANNEL:
		*pixelSize = 40;
		return cmMCFive8Space;
		break;
	case BM_6CHANNEL:
		*pixelSize = 48;
		return cmMCSix8Space;
		break;
	case BM_7CHANNEL:
		*pixelSize = 56;
		return cmMCSeven8Space;
		break;
	case BM_8CHANNEL:
		*pixelSize = 64;
		return cmMCEight8Space;
		break;
	case BM_GRAY:
		*pixelSize = 8;
		return cm8PerChannelPacking + cmGraySpace;
		break;
	case BM_xRGBQUADS:
		*pixelSize = 32;
		return cmBGR32Space;
		break;
	case BM_xBGRQUADS:
		*pixelSize = 32;
		return cmRGBA32Space;
		break;
#if 0
	case BM_xXYZQUADS:
		*pixelSize = 32;
		return cmLong8ColorPacking + cmXYZSpace;
		break;
	case BM_xYxyQUADS:
		*pixelSize = 32;
		return cmLong8ColorPacking + cmYXYSpace;
		break;
	case BM_xLabQUADS:
		*pixelSize = 32;
		return cmLong8ColorPacking + cmLABSpace;
		break;
#endif
    case BM_xG3CHQUADS:
		*pixelSize = 32;
		return cmLong8ColorPacking + cmGenericSpace;
		break;
	case BM_CMYKQUADS:
		*pixelSize = 32;
		return cmLong8ColorPacking + cmKYMCSpace;
		break;
	case BM_KYMCQUADS:
		*pixelSize = 32;
		return cmLong8ColorPacking + cmCMYKSpace;
		break;
	case BM_10b_RGB:
		*pixelSize = 32;
		return cmLong10ColorPacking + cmRGBSpace;
		break;
	case BM_10b_XYZ:
		*pixelSize = 32;
		return cmLong10ColorPacking + cmXYZSpace;
		break;
	case BM_10b_Yxy:
		*pixelSize = 32;
		return cmLong10ColorPacking + cmYXYSpace;
		break;
	case BM_10b_Lab:
		*pixelSize = 32;
		return cmLong10ColorPacking + cmLABSpace;
		break;
	case BM_10b_G3CH:
		*pixelSize = 32;
		return cmLong10ColorPacking + cmGenericSpace;
		break;
	case BM_16b_RGB:
		*pixelSize = 48;
		return cm16PerChannelPacking + cmBGRSpace;
		break;
	case BM_16b_XYZ:
		*pixelSize = 48;
		return cm16PerChannelPacking + cmXYZSpace;
		break;
	case BM_16b_Yxy:
		*pixelSize = 48;
		return cm16PerChannelPacking + cmYXYSpace;
		break;
	case BM_16b_Lab:
		*pixelSize = 48;
		return cm16PerChannelPacking + cmLABSpace;
		break;
	case BM_16b_G3CH:
		*pixelSize = 48;
		return cm16PerChannelPacking + cmGenericSpace;
		break;
	case BM_16b_GRAY:
		*pixelSize = 16;
		return cmGraySpace;
		break;
	case BM_NAMED_INDEX:
		*pixelSize = 32;
		return cmNamedIndexed32Space;
		break;
	default:
		*pixelSize = 0;
		return 0;
	}
}

/* ______________________________________________________________________
HCMTRANSFORM  WINAPI CMGetTransform( HCMTRANSFORM 	hcmTransform );

Abstract:
	The CMGetTransform function retrieves the actual transform out of the static array
	in the critical section.

Parameter			Description
	hcmTransform	Handle to the transform.

Returns
	the actual pointer to the transform
________________________________________________________________ */
HCMTRANSFORM  WINAPI CMGetTransform( HCMTRANSFORM 	hcmTransform )
{
	long actTransform = (long)(ULONG_PTR)hcmTransform - 256;
	HCMTRANSFORM aTrans = NULL;

	__try {
		EnterCriticalSection(&GlobalCriticalSection);
		if( actTransform < IndexTransform && actTransform >= 0 ){
			aTrans = TheTransform[actTransform];
		}
	}
	__finally{
		LeaveCriticalSection(&GlobalCriticalSection);
 	}
    return aTrans;
}

/* ______________________________________________________________________
CMWorldRef StoreTransform( CMWorldRef aRef );

Abstract:
	The StoreTransform function stores the actual transform in the static array
	in the critical section.

Parameter		Description
	aRef		Ptr to the transform.

Returns
	valid (255 < handle < 65536 ) handle of the transform
________________________________________________________________ */
CMWorldRef StoreTransform( CMWorldRef aRef )
{
	long i;
    CMWorldRef cw = NULL;
	
	__try {
		EnterCriticalSection(&GlobalCriticalSection);

		if( IndexTransform >= 1000 )return (HCMTRANSFORM)ERROR_NOT_ENOUGH_MEMORY;
		for( i = 0; i<IndexTransform ; i++ ){
			if( TheTransform[i] == 0 ){
				TheTransform[i] = aRef;
				cw = (CMWorldRef)(ULONG_PTR)(i + 256 );
			}
		}
		if( i >= IndexTransform ){
			TheTransform[IndexTransform] = aRef;
			IndexTransform++;
			cw = (CMWorldRef)(ULONG_PTR)(IndexTransform - 1 + 256);
		}
	}
	__finally{
		LeaveCriticalSection(&GlobalCriticalSection);
	}

   return cw;
}

BOOL  WINAPI CMConvertColorNameToIndex( HPROFILE aProf, LPCOLOR_NAME aColorNameArr, LPDWORD aDWordArr, DWORD count )
{
	CMError err;

	err = CMConvNameToIndexProfile( aProf, aColorNameArr, aDWordArr, count );
	if( err ){
		SetLastError( err );
		return 0;
	}
	return 1;
}

BOOL  WINAPI CMConvertIndexToColorName( HPROFILE aProf, LPDWORD aDWordArr, LPCOLOR_NAME aColorNameArr, DWORD count )
{
	long matchErr;

	matchErr = CMConvIndexToNameProfile( aProf, aDWordArr, aColorNameArr, count );
	if( matchErr ){
		SetLastError( matchErr );
		return 0;
	}
	return 1;
}

BOOL  WINAPI CMGetNamedProfileInfo( HPROFILE aProf, LPNAMED_PROFILE_INFO Info )
{
	long matchErr;

	matchErr = CMGetNamedProfileInfoProfile( aProf, (pCMNamedProfileInfo)Info );
	if( matchErr ){
		SetLastError( matchErr );
		return 0;
	}
	return 1;
}

/*
CMBitmapColorSpace CMGetColorType( COLORTYPE bmMode, long *pixelSize )
{
	switch(  bmMode ){
	case COLOR_GRAY:
		*pixelSize = 16;
		return cm16PerChannelPacking + cmGraySpace;
		break;
	case COLOR_	:
	case COLOR_XYZ:
	case COLOR_Yxy:
	case COLOR_Lab:
	case COLOR_3_CHANNEL:
	case COLOR_CMYK:
		*pixelSize = 64;
		return cm16PerChannelPacking + cmRGBSpace;
		break;
	case COLOR_5_CHANNEL:
	case COLOR_6_CHANNEL:
	case COLOR_7_CHANNEL:
	case COLOR_8_CHANNEL:
		*pixelSize = 64;
		return cm8PerChannelPacking + cmMCFiveSpace + bmMode - COLOR_5_CHANNEL;
		break;
	default:
		*pixelSize = 0;
		return 0;
	}
}

#define CMS_x555WORD      0x00000000
#define CMS_565WORD       0x00000001
#define CMS_RGBTRIPLETS   0x00000002
#define CMS_BGRTRIPLETS   0x00000004
#define CMS_XRGBQUADS     0x00000008
#define CMS_XBGRQUADS     0x00000010
#define CMS_QUADS         0x00000020

CMBitmapColorSpace CMGetCMSType( DWORD bmMode, long *pixelSize )
{
	if(  bmMode & CMS_x555WORD ){
		*pixelSize = 16;
		return cmWord5ColorPacking + cmRGBSpace;
	}
	else if( bmMode & CMS_RGBTRIPLETS ){
		*pixelSize = 24;
		return cm8PerChannelPacking + cmRGBSpace;
	}
	else if( bmMode &  CMS_BGRTRIPLETS ){
		*pixelSize = 24;
		return cm8PerChannelPacking + cmBGRSpace;
	}
	else if( bmMode &  CMS_XRGBQUADS ){
		*pixelSize = 32;
		return cmLong8ColorPacking + cmRGBSpace;
	}
	else if( bmMode &  CMS_XBGRQUADS ){
		*pixelSize = 32;
		return cmLong8ColorPacking + cmBGRSpace;
	}
	else if( bmMode &  CMS_QUADS	 ){
		*pixelSize = 32;
		return cmLong8ColorPacking + cmCMYKSpace;
	}
	else{
		*pixelSize = 0;
		return 0;
	}
}
*/



