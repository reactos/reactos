/*
	File:		MSNewMemProfile.h

	Contains:	

	Written by:	U. J. Krabbenhoeft

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/
#ifndef MSNewMemProfile_h
#define MSNewMemProfile_h


CMError MyNewAbstractW( LPLOGCOLORSPACEW	lpColorSpace, icProfile **theProf ); 
CMError MyNewAbstract(	LPLOGCOLORSPACEA	lpColorSpace, icProfile **theProf ); 
 
CMError MyNewDeviceLink( CMWorldRef cw, CMConcatProfileSet *profileSet, LPSTR theProf );
CMError MyNewDeviceLinkW( CMWorldRef cw, CMConcatProfileSet *profileSet, LPWSTR theProf );

CMError MyNewDeviceLinkFill( CMWorldRef cw, CMConcatProfileSet *profileSet, HPROFILE aHProf );
long	SaveMyProfile( LPSTR lpProfileName, LPWSTR lpProfileNameW, PPROFILE theProf );

CMError DeviceLinkFill(	CMMModelPtr cw, 
						CMConcatProfileSet *profileSet, 
						icProfile **theProf,
						unsigned long aIntent );
UINT32	GetSizes( CMMModelPtr cw, UINT32 *clutSize );

#endif
