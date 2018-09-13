/*
	File:		LHTheRoutines.h

	Contains:	

	Written by:	U. J. Krabbenhoeft

	Version:

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/
#ifndef LHTheRoutines_h
#define LHTheRoutines_h

OSErr
CalcGridPoints4Cube ( long	theCubeSize,
					  long	inputDim,
					  long* theGridPoints,
					  long* theGridBits );
/*
					MakeCube
	Fills Array Poi with the whole 'inputDim' dimensional color space with
	'inputDim' pixel entries depending on size of Poi memory
	Return: # of address bits for one dimension ( gridPoints = 1<< # )
*/
OSErr	MakeCube( long 				inputDim, 
				  long 				*thePtrSize,
				  CUBE_DATA_TYPE	*theCube,
				  long 				*theBits );
/*					MakeCMColorCube
	Fills Array Poi with the whole 'inputDim' dimensional color space with
	'inputDim' pixel entries depending on size of Poi memory
	Return: # of address bits for one dimension ( gridPoints = 1<< # )

*/

OSErr	MakeCMColorCube( 	long inputDim, 
				 			long *theCubeSize,
							CUBE_DATA_TYPE *aHdlPtr,
			  	 			long *theBits );
#define UWE 2
#ifdef UWE
/*					MakeCube16
	Fills Array Poi with the whole 'inputDim' dimensional color space with
	'inputDim' pixel entries depending on size of Poi memory with WORD values
	Return: # of address bits for one dimension ( gridPoints = 1<< # )
*/
OSErr
MakeCube16( long 			inputDim, 
			long 			*theCubeSize,
			CUBE_DATA_TYPE	*theCube,
			long 			*theBits,
			long 			*theExtraSize );
#endif
#endif
