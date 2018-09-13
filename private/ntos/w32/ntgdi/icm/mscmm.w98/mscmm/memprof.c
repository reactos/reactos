/*
	File:		MSNewMemProfile.c

	Contains:	
		creation of mem based profiles

	Written by:	U. J. Krabbenhoeft

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

	Version:	
*/

#include "Windef.h"
#include "WinGdi.h"
#include <wtypes.h>
#include <winbase.h>
#include <windowsX.h>
#include "ICM.h"

#ifndef LHGeneralIncs_h
#include "General.h"
#endif

#ifndef MSNewMemProfile_h
#include "MemProf.h"
#endif

#ifndef MemLink_h
#include "MemLink.h"
#endif

#ifdef _DEBUG
//#define WRITE_PROFILE
#endif
#if ! realThing
#ifdef DEBUG_OUTPUT
#define kThisFile kLHMemProfileID
#endif
#endif

#ifdef WRITE_PROFILE
void WriteProf( LPSTR name, icProfile *theProf, long currentSize );
#endif

CMError MyNewDeviceLink( CMWorldRef cw, CMConcatProfileSet *profileSet, LPSTR theProf )
{
	CMError			err = unimpErr;
	PROFILE			pProf;
	HPROFILE		aHProf;

	pProf.pProfileData = (PVOID *)theProf;
	pProf.dwType = PROFILE_FILENAME;
	pProf.cbDataSize = strlen((const unsigned char *)pProf.pProfileData) * sizeof(CHAR);
	aHProf = OpenColorProfile( &pProf, PROFILE_READWRITE, 0, CREATE_ALWAYS );
 	if( !aHProf ){
		err = GetLastError();
		goto CleanupAndExit;
	}
	err = MyNewDeviceLinkFill( cw, profileSet, aHProf );

CleanupAndExit:
	return err;
}

CMError MyNewDeviceLinkW( CMWorldRef cw, CMConcatProfileSet *profileSet, LPWSTR theProf )
{
	CMError			err = unimpErr;
	PROFILE			pProf;
	HPROFILE		aHProf;

	pProf.pProfileData = (PVOID *)theProf;
	pProf.dwType = PROFILE_FILENAME;
	pProf.cbDataSize = wcslen((const unsigned short *)pProf.pProfileData) * sizeof(WCHAR);
	aHProf = OpenColorProfileW( &pProf, PROFILE_READWRITE, 0, CREATE_ALWAYS );
 	if( !aHProf ){
		err = GetLastError();
		goto CleanupAndExit;
	}
	err = MyNewDeviceLinkFill( cw, profileSet, aHProf );
CleanupAndExit:
	return err;
}

CMError MyNewDeviceLinkFill( CMWorldRef cw, CMConcatProfileSet *profileSet, HPROFILE aHProf )
{
	CMError			err = unimpErr;
	OSErr			aOSerr = unimpErr;
#ifdef __MWERKS__
        unsigned char			theText[] = "\pLogColorSpProfile¥";
#else
      char			theText[] = "\024DeviceLinkProfile   ";
#endif
	char			copyrightText[] = "\060©1996 by Heidelberger Druckmaschinen AG  U.J.K.";
	Ptr 			aPtr=0;
	long 			theHeaderSize;
	long 			theDescSize;
	long 			theMediaSize;
	long 			theCopyRightSize;
	long 			currentSize=0;
	long 			theTotalSize=0;
	long 			theSequenceDescSize = 0;
	long			theA2B0Size;
	unsigned long	aIntent;
	UINT32			sCS,dCS,clutSize;
	OSErr			aOSErr;


	MyDoubleXYZ D50XYZ = { 0.9642, 1.0000, 0.8249 };
	icXYZNumber D50 = { (unsigned long)(D50XYZ.X * 65536), (unsigned long)(D50XYZ.Y * 65536), (unsigned long)(D50XYZ.Z * 65536)};
	theHeaderSize = sizeof(icHeader);

	theDescSize   = 									  sizeof(OSType) 			/* type descriptor */
														+ sizeof(unsigned long) 	/* reserved */
														+ sizeof(unsigned long) 	/* ASCII length */
														+ theText[0]				/* ASCII profile description */
														+ sizeof(unsigned long)		/* Unicode code */
														+ sizeof(unsigned long)		/* Unicode character count */
														+ sizeof(unsigned short)	/* Macintosh script code */
														+ sizeof(unsigned char)		/* Macintosh string length */
														+ 67						/* Macintosh string */
														;
	theCopyRightSize   = 								  sizeof(OSType) 			/* type descriptor */
														+ sizeof(unsigned long) 	/* reserved */
														+ copyrightText[0]			/* ASCII profile description */
														;
	theMediaSize = sizeof( icXYZType );;

	aPtr = SmartNewPtrClear( 5000, &aOSerr );
	err = aOSerr;
	if (err)
		goto CleanupAndExit;

	aIntent = 0;

	err = MyGetColorSpaces( profileSet, &sCS, &dCS );
	if (err)
		goto CleanupAndExit;
		
	err = MyAdd_NL_HeaderMS(theHeaderSize+4, (icHeader*)aPtr, aIntent, sCS, dCS );
	if (err)
		goto CleanupAndExit;
		
	err = SetColorProfileHeader( aHProf, (PPROFILEHEADER)aPtr);
	if (!err){
		err = GetLastError();
		goto CleanupAndExit;
	}

	err =MyAdd_NL_DescriptionTag	( (LHTextDescriptionType *)aPtr, (unsigned char *)theText );
	if (err)
		goto CleanupAndExit;
	err = SetColorProfileElementSize( aHProf, icSigProfileDescriptionTag, theDescSize );
	if (!err){
		err = GetLastError();
		goto CleanupAndExit;
	}
	err = SetColorProfileElement( aHProf, icSigProfileDescriptionTag, 0, (DWORD *)&theDescSize, aPtr );
	if (!err){
		err = GetLastError();
		goto CleanupAndExit;
	}

	err = MyAdd_NL_ColorantTag((icXYZType *)aPtr, &D50);
	if (err)
		goto CleanupAndExit;

	err = SetColorProfileElementSize( aHProf, icSigMediaWhitePointTag, theMediaSize );
	if (!err){
		err = GetLastError();
		goto CleanupAndExit;
	}
	err = SetColorProfileElement( aHProf, icSigMediaWhitePointTag, 0, (DWORD *)&theMediaSize, aPtr );
	if (!err){
		err = GetLastError();
		goto CleanupAndExit;
	}

	err = MyAdd_NL_CopyrightTag( (unsigned char *)copyrightText, (LHTextType *)aPtr);
	if (err)
		goto CleanupAndExit;
	err = SetColorProfileElementSize( aHProf, icSigCopyrightTag, theCopyRightSize );
	if (!err){
		err = GetLastError();
		goto CleanupAndExit;
	}
	err = SetColorProfileElement( aHProf, icSigCopyrightTag, 0, (DWORD *)&theCopyRightSize, aPtr );
	if (!err){
		err = GetLastError();
		goto CleanupAndExit;
	}
		
	err = MyAdd_NL_SequenceDescTag( profileSet, (icProfileSequenceDescType *)aPtr, &theSequenceDescSize );
	if (err)
		goto CleanupAndExit;
	err = SetColorProfileElementSize( aHProf, icSigProfileSequenceDescTag, theSequenceDescSize );
	if (!err){
		err = GetLastError();
		goto CleanupAndExit;
	}
	err = SetColorProfileElement( aHProf, icSigProfileSequenceDescTag, 0, (DWORD *)&theSequenceDescSize, aPtr );
	if (!err){
		err = GetLastError();
		goto CleanupAndExit;
	}
	
	aPtr = DisposeIfPtr( aPtr );

    theA2B0Size = GetSizes( (CMMModelPtr)cw, &clutSize );

	aPtr =  SmartNewPtr( theA2B0Size, &aOSErr );
	err = aOSErr;
	if (err)
		goto CleanupAndExit;

	if ( ((CMMModelPtr)cw)->lutParam.colorLutWordSize == 8)
        err = MyAdd_NL_AToB0Tag_mft1( (CMMModelPtr)cw, (icLut8Type *)aPtr, clutSize );
    else
        err = MyAdd_NL_AToB0Tag_mft2( (CMMModelPtr)cw, (icLut16Type *)aPtr, clutSize );

	if (err)
		goto CleanupAndExit;

	//if( theA2B0Size > 12000 ) theA2B0Size = 12000;
	err = SetColorProfileElementSize( aHProf, icSigAToB0Tag, theA2B0Size );
	if (!err){
		err = GetLastError();
		goto CleanupAndExit;
	}
	err = SetColorProfileElement( aHProf, icSigAToB0Tag, 0, (DWORD *)&theA2B0Size, aPtr );
	if (!err){
		err = GetLastError();
		goto CleanupAndExit;
	}
		
	err = noErr;
	
CleanupAndExit:
	CloseColorProfile( aHProf );
	aPtr = DisposeIfPtr( aPtr );
	return err;
}

#ifdef WRITE_PROFILE
#ifdef IS_MAC
/*#include <unistd.h>*/
/*#include <fcntl.h¥>*/
#else
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#endif
#include <stdio.h>

void WriteProf( LPSTR name, icProfile *theProf, long currentSize )
{
	int fh;
	
	fh = open( name, O_CREAT|O_RDWR|O_BINARY, _S_IREAD | _S_IWRITE );
	if( fh <= 0 ){
#ifdef DEBUG_OUTPUT
		printf("Open %s failed\n",name);
#endif
		return;
	}
	write( fh, (Ptr)theProf, currentSize );
	close(fh);
}
#endif

#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
long SaveMyProfile( LPSTR lpProfileName, LPWSTR lpProfileNameW, PPROFILE theProf )
{
	long ret;
	int fh;

	if( theProf->dwType != PROFILE_MEMBUFFER ){
		SetLastError( (unsigned long) unimpErr );
		return 0;
	}
	if( lpProfileNameW == 0 ){
		if( lpProfileName == 0 ) return 0;
		fh = _open( lpProfileName, _O_BINARY | _O_CREAT  | _O_EXCL, _S_IREAD | _S_IWRITE );
	}
	else{
		fh = _wopen( lpProfileNameW, _O_BINARY | _O_CREAT  | _O_EXCL, _S_IREAD | _S_IWRITE );
	}
	if( fh == -1 ) return -1;

	if( theProf->pProfileData == 0 ){
		SetLastError( ERROR_INVALID_DATA );
		return 0;
	}
	ret = _write( fh, theProf->pProfileData, theProf->cbDataSize );
	if( ret == -1 ) return -1;

	ret = _close( fh );
	if( ret == -1 ) return -1;
	return 0;
}
