/*
	File:		LHStdConversionLuts.h

	Contains:	prototypes for standard table funktions

	Written by:	H.Siegeritz

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/

#ifndef LHStdConversionLuts_h
#define	LHStdConversionLuts_h

#ifndef LHDefines_h
#include "Defines.h"
#endif

void	Lab2XYZ_forCube16(unsigned short *theCube, long count);
void	XYZ2Lab_forCube16(unsigned short *theCube, long count);

#endif 
