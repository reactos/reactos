/*
	File:		LHDoNDim.h

	Contains:	

	Written by:	U. J. Krabbenhoeft

	Version:

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/

#ifndef LHDoNDim_h
#define LHDoNDim_h

#if powerc
#pragma options align=mac68k
#endif

/* typedef double Matrix2D[3][3]; 	*/
typedef struct{ 	
                long 		aElutAdrSize;					/* Count of Adresses of Elut ( for one Dimension )*/
                long 		aElutAdrShift;					/* Count of used bits ( MUST BE = 2^ aElutAdrSize )*/
                long 		aElutWordSize;					/* Count of bits of each entry ( e.g. 10 for partly used UINT16 )*/
                Boolean 	separateEluts;					/* 0= same Elut for all Dimensions; 1= separate Eluts*/
                UINT16 		*ein_lut;						/* Pointer to Elut*/
                long 		aAlutAdrSize;					/* Count of Adresses of Alut ( for one Dimension )		*/
                long		aAlutAdrShift;					/* Count of used bits ( MUST BE = 2^ aAlutAdrSize )*/
                long 		aAlutWordSize;					/* Count of bits of each entry ( e.g. 16 for fully used UINT16 )*/
                Boolean 	separateAluts;					/* 0= same Alut for all Dimensions; 1= separate Aluts*/
                UINT8 		*aus_lut;						/* Pointer to Alut*/
                Matrix2D	*theMatrix;						/* Pointer to Matrix*/
                long 		aPointCount;					/* Count of input pixels*/
                long 		gridPoints;						/* gridpoints*/
                long 		aBufferByteCount;				/* BufferByteCount*/
                UINT8		*theArr;						/* Input/Output array*/
}DoMatrixForCubeStruct,*DoMatrixForCubeStructPtr;

#if powerc
#pragma options align=reset
#endif
/*					DoMatrixForCube*/
/*	works with planeinterleaved Elut, Alut*/
/*	but with pixelinterleaved data ( BYTES/WORDs).*/
/*	Elut, Alut must have 2^n UINT16 entries. */
/*	Matrix is 3*3 double.*/
#ifdef __cplusplus
extern "C" {
#endif
void DoMatrixForCube16( DoMatrixForCubeStructPtr aStructPtr );
void DoOnlyMatrixForCube16( Matrix2D	*theMatrix, Ptr aXlut, long aPointCount, long gridPointsCube );
void DoOnlyMatrixForCube( Matrix2D	*theMatrix, Ptr aXlut, long aPointCount, long gridPointsCube );

#ifdef __cplusplus
}
#endif
#endif
