/*
	File:		MSICMProfileAccess.c

	Contains:	

	Written by:	U. J. Krabbenhoeft

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

	Version:	
*/
#include "Windef.h"
#include "WinGdi.h"
#include <wtypes.h>
#include "ICM.h"
#include "General.h"
/* --------------------------------------------------------------------------

CMError CMGetProfileHeader(	CMProfileRef			prof,
							CMCoreProfileHeader*	header );

	Abstract:

	Params:
		
	Return:
		noErr		successful

   -------------------------------------------------------------------------- */
CMError CMGetProfileHeader(	CMProfileRef			prof,
							CMCoreProfileHeader*	header )
{
	BOOL bool;
	CMError ret = badProfileError;

	bool = GetColorProfileHeader( (HPROFILE)prof, (PPROFILEHEADER) header );
	if (header->magic == icMagicNumber && bool )
		ret = noErr;
	
	return (ret);
}


/* --------------------------------------------------------------------------

CMError CMGetProfileElement(	CMProfileRef 		prof,
								OSType 				tag,
								unsigned long*		elementSize,
								void* 				elementData );

	Abstract:
		This function gives an pointer to the requested tag back
	Params:
		
	Return:
		noErr		successful

   -------------------------------------------------------------------------- */
CMError CMGetProfileElement(	CMProfileRef 		prof,
								OSType 				tag,
								unsigned long*		elementSize,
								void* 				elementData )
{
	return (CMGetPartialProfileElement(prof, tag, 0, elementSize, elementData));
}


/* --------------------------------------------------------------------------

	CMError CMGetPartialProfileElement(	CMProfileRef 		prof,
										OSType 				tag,
										unsigned long		offset,
										unsigned long		*byteCount,
										void				*elementData )

	Abstract:
		Core likes small amounts of memory we too but this is ascetic.
	Params:
		
	Return:
		noErr		successful

   -------------------------------------------------------------------------- */

CMError CMGetPartialProfileElement(	CMProfileRef 		prof,
									OSType 				tag,
									unsigned long		offset,
									unsigned long		*byteCount,
									void				*elementData )
{
	BOOL bool;
	BOOL ret;

	if (!byteCount)
	{
		return -1;
	}
	SetLastError(0);
	/*ret = IsColorProfileTagPresent( (HPROFILE)prof, (TAGTYPE)tag, &bool );*/
	if( elementData == 0 ) *byteCount = 0;
	ret = GetColorProfileElement( (HPROFILE)prof, (TAGTYPE)tag, offset, byteCount, elementData, &bool );
	if( ret )		return (noErr);
	/*					GetColorProfileElement returns FALSE for calls with elementData = 0
						but byteCount is set correctly */		
	else if( elementData == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER )	return (noErr);
	
	return (cmElementTagNotFound);
}

/* --------------------------------------------------------------------------

Boolean CMProfileElementExists(		CMProfileRef 	prof,
									OSType 			tag );

	Abstract:

	Params:
		
	Return:
		noErr		successful

   -------------------------------------------------------------------------- */
Boolean CMProfileElementExists(		CMProfileRef 	prof,
									OSType 			tag )
{
	BOOL bool;
	IsColorProfileTagPresent( (HPROFILE)prof, (TAGTYPE)tag, &bool );
	return (BOOLEAN)bool;
}


