/*
	File:		PI_CMMInitialization.h

	Contains:	Initialization procdures 
				
	Written by:	U. J. Krabbenhoeft

	Version:

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

	File Ownership:

*/
#ifndef PI_CMMInitialization_h
#define PI_CMMInitialization_h
CMError CMMInitPrivate( 			CMMModelPtr 		storage, 
						 			CMProfileRef 		srcProfile, 
						 			CMProfileRef 		dstProfile );
CMError CMMConcatInitPrivate	( 	CMMModelPtr 		storage, 
						  			CMConcatProfileSet 	*profileSet);
#endif


