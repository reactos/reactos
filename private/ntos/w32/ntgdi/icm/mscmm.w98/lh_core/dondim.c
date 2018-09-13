/*
	File:		LHDoNDim.c

	Contains:	

	Written by:	U. J. Krabbenhoeft

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

	Version:	
*/

#ifndef LHGeneralIncs_h
#include "General.h"
#endif

#ifndef LHDoNDim_h
#include "DoNDim.h"
#endif

#define SHRINK_FACTOR 13
/*
#define FARBR_FILES 1
#define FARBR_DEBUG 1
#define FARBR_DEBUG0 1
*/
#if FARBR_FILES
#include "stdio.h"
#include "string.h"
void WriteLuts(	char *theName,long WordSize,long aElutAdrSize,long aElutWordSize,UINT16 *Elut,
				long aXlutInDim,long aXlutOutDim,long aXlutAdrSize,long aXlutWordSize,UINT16 *Xlut,
				long aAlutAdrSize,long aAlutWordSize,UINT16 *aus_lut);
#endif

#if UWEs_eigene_Umgebung
#include <stdio.h>
#define DebugPrint printf
#else
#ifdef DEBUG_OUTPUT
#define kThisFile kLHDoNDimID
/*#include "DebugSpecial.h"*/
#else
#define DebugPrint(x)
#endif
#endif

#define CLIPPWord(x,a,b) ((x)<(a)?(UINT16)(a):((x)>(b)?(UINT16)(b):(UINT16)(x+.5)))
#define CLIPP(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))
/*#if 0*/

/*ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
  ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ*/
#define Round(a) (((a)>0.)?((a)+.5):((a)-.5))

/*ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
   DoOnlyMatrixForCube
  ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ*/
void DoOnlyMatrixForCube( Matrix2D	*theMatrix, Ptr aXlut, long aPointCount, long gridPointsCube )
{
    register long 	ii,jj;
    register long 	i;
    register long 	aLong0;
    register long 	aLong1;
    register long 	aLong2;
    register long 	theEnd;
    register long 	aVal;
    long 			aMatrix[3][3];
    register double aFactor,dVal;
    register UINT16 	accu,aMax;
    register UINT8 *theArr = (UINT8 *)aXlut;
    /*   FILE *aSt; */
 
    
#ifdef DEBUG_OUTPUT
	long err=0;
#endif
	LH_START_PROC("DoOnlyMatrixForCube")
	jj=aPointCount;
	theEnd = 3 * aPointCount;
	/*for( i=1; i<100; ++i)if( i*i*i == jj )break;*/		  /* calculate gridpoints*/
	/*if( i<= 0 || i >= 100 ) return;*/
	i = gridPointsCube;
	aMax = 256 - 1;
	aFactor = 4096.*255./(256.*(i-1)/i);
	for( ii=0; ii<3; ii++){ 
		for( jj=0; jj<3; jj++){ 
			dVal = (*theMatrix)[ii][jj]*aFactor;
			aMatrix[ii][jj] = (long)Round(dVal);
		}
	}
   	#if FARBR_DEBUG
    DebugPrint("theArr=(d)");for(ii=0; ii<3; ++ii) for( jj=0; jj<3; jj++) DebugPrint("%f ",aMatrix[ii][jj]); DebugPrint("\n");
    #endif
    /*
    aSt = fopen("Matrix","a");
    for(ii=0; ii<3; ++ii) for( jj=0; jj<3; jj++) fprintf(aSt,"%f ",(*(aStructPtr->theMatrix))[ii][jj]); fprintf(aSt,"\n");
    for(ii=0; ii<3; ++ii) for( jj=0; jj<3; jj++) fprintf(aSt,"%d ",aMatrix[ii][jj]); fprintf(aSt,"\n");
	fclose(aSt);
	*/
	for (i = 0; i < theEnd; i +=3){		/* Schleife der Points */
	       aLong0=theArr[i+0];
	       aLong1=theArr[i+1];
	       aLong2=theArr[i+2];
		for( ii=0; ii<3; ii++){ 
				aVal = aMatrix[ii][0] * aLong0;
				aVal += aMatrix[ii][1] * aLong1;
				aVal += aMatrix[ii][2] * aLong2;
			if( aVal > 0) aVal = (aVal+2047)>>12;
			else aVal = (aVal-2047)>>12;
			accu = (UINT16)CLIPP(aVal,0,(long)aMax);
        	theArr[i+ii] = (UINT8)accu;
		}
       	#if FARBR_DEBUG
        DebugPrint("i=%ld\n",i);
        DebugPrint("theArr=(d)");for(ii=0; ii<3; ++ii) DebugPrint("%ld ",theArr[i+ii]); DebugPrint("\n");
        DebugPrint("ein_reg=(d)");DebugPrint("%ld ",aLong0);DebugPrint("%ld ",aLong1);DebugPrint("%ld ",aLong2); DebugPrint("\n");
        #endif
	}

	LH_END_PROC("DoOnlyMatrixForCube")
}


#define VAL_MAX 65536
#define VAL_MAXM1 (VAL_MAX-1)

/*ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
   DoMatrixForCube16
  ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ*/
void DoMatrixForCube16( DoMatrixForCubeStructPtr aStructPtr )
{
    register long 	ii,jj;
    register long 	i;
    /*long 		  	aLong[3];*/
    register long 	aLong0;
    register long 	aLong1;
    register long 	aLong2;
    register long 	theEnd;
    register long 	aElutShift,aElutShiftNum,aAlutShift,aElutOffset,aAlutOffset;
    register unsigned long 	aElutAdrSize,separateEluts,separateAluts;
    register long 	aVal;
    long 			aMatrix[3][3];
    register double aFactor,dVal;
    register unsigned long 	aMax;
    register UINT16 *ein_lut;
    register UINT16 *aus_lut;
    register UINT8 *aus_lutByte;
    register unsigned long aAlutAdrSize;
    register UINT16 *theArr;
    register UINT8 *theByteArr;
    register unsigned long ko,ko0;
    register long aElutShiftRight;
    /*   FILE *aSt;*/
 
#ifdef DEBUG_OUTPUT
	long err=0;
#endif
	LH_START_PROC("DoMatrixForCube16")

    /*DebugPrint("DoMatrixForCube16 with %d input pixels\n",aPointCount);*/
	ein_lut = aStructPtr->ein_lut;
	aus_lut = (UINT16 *)aStructPtr->aus_lut;
	aus_lutByte = (UINT8 *)aus_lut;
	theArr = (UINT16 *)aStructPtr->theArr;
	theByteArr = (UINT8 *)theArr;
	aAlutAdrSize = aStructPtr->aAlutAdrSize;
	aElutAdrSize = aStructPtr->aElutAdrSize;
	separateEluts = aStructPtr->separateEluts;
	separateAluts = aStructPtr->separateAluts;
	theEnd =  3*aStructPtr->aPointCount;
	aMax = aAlutAdrSize-1;
	
	for( i=0; i<33; i++) if( (1L<<i) == (long)aElutAdrSize )break;
	if( i > 32 ) return;
	aElutShift = 16-i;
	aElutShiftRight = aStructPtr->aElutWordSize - aElutShift + 0; /* use only 16 bit from elut*/
	if( aElutShiftRight < 0 ) return;
	aElutShiftNum = 1<<aElutShift;
	
	aFactor = 2*8. * aMax /(aStructPtr->gridPoints-1)*(double)aStructPtr->gridPoints;
	for( ii=0; ii<3; ii++){ 
		for( jj=0; jj<3; jj++){ 
			dVal = (*(aStructPtr->theMatrix))[ii][jj]*aFactor;
      		aMatrix[ii][jj] = (long)Round(dVal);
		}
	}
   	#if FARBR_DEBUG
    DebugPrint("theArr=(d)");for(ii=0; ii<3; ++ii) for( jj=0; jj<3; jj++) DebugPrint("%f ",aMatrix[ii][jj]); DebugPrint("\n");
    /*
    aSt = fopen("Matrix","a");
    for(ii=0; ii<3; ++ii) for( jj=0; jj<3; jj++) fprintf(aSt,"%f ",(*(aStructPtr->theMatrix))[ii][jj]); fprintf(aSt,"\n");
    for(ii=0; ii<3; ++ii) for( jj=0; jj<3; jj++) fprintf(aSt,"%d ",aMatrix[ii][jj]); fprintf(aSt,"\n");
	fclose(aSt);
	*/
    #endif
    if( aStructPtr->aBufferByteCount == 2 ){
		aAlutShift = 16-aStructPtr->aAlutWordSize;
		for (i = 0; i < theEnd; i +=3){		/* Schleife der Points */
	
			aElutOffset = 0;
			ko = (theArr[i+0]);
			ko0 = ko - ( ko >> ( 16 - aElutShift ));
			ko = ko0 & ( aElutShiftNum - 1 );
			ko0 = ko0 >> aElutShift;
			if( ko0 >= (aElutAdrSize-1) ){
		        	aLong0 = ein_lut[ ko0 + aElutOffset ]<<(aElutShift-aElutShiftRight);
			}
			else{
				ko0 += aElutOffset;
		       	aLong0 = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 + 1 ] * ko )>>aElutShiftRight;
		    }   
			if( separateEluts )aElutOffset += aElutAdrSize;
			
			ko = (theArr[i+1]);
			ko0 = ko - ( ko >> ( 16 - aElutShift ));
			ko = ko0 & ( aElutShiftNum - 1 );
			ko0 = ko0 >> aElutShift;
			if( ko0 >= (aElutAdrSize-1) ){
		        	aLong1 = ein_lut[ ko0 + aElutOffset ]<<(aElutShift-aElutShiftRight);
			}
			else{
				ko0 += aElutOffset;
		       	aLong1 = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 + 1 ] * ko )>>aElutShiftRight;
		    }   
			if( separateEluts )aElutOffset += aElutAdrSize;
			
			ko = (theArr[i+2]);
			ko0 = ko - ( ko >> ( 16 - aElutShift ));
			ko = ko0 & ( aElutShiftNum - 1 );
			ko0 = ko0 >> aElutShift;
			if( ko0 >= (aElutAdrSize-1) ){
		        	aLong2 = ein_lut[ ko0 + aElutOffset ]<<(aElutShift-aElutShiftRight);
			}
			else{
				ko0 += aElutOffset;
		       	aLong2 = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 + 1 ] * ko )>>aElutShiftRight;
		    }   
				
			aLong0 = ( aLong0 +2 )>>2;	       
			aLong1 = ( aLong1 +2 )>>2;	       
			aLong2 = ( aLong2 +2 )>>2;	       
			aAlutOffset = 0;
			for( ii=0; ii<3; ii++){ 
					aVal = aMatrix[ii][0] * aLong0;
					aVal += aMatrix[ii][1] * aLong1;
					aVal += aMatrix[ii][2] * aLong2;
				aVal = (aVal+((1<<9)-1))>>10;
				if( aVal < 0 ) aVal = 0;
				
		        ko0= ( aVal>> (3+2+3) );
		        if( ko0 >= (aAlutAdrSize-1) ){
		        	theArr[i+ii] = aus_lut[ (aAlutAdrSize-1) + aAlutOffset ] <<aAlutShift;
		        }
		        else{
		        	ko0 += aAlutOffset;
			        ko = ( aVal & ((1<<(3+2+3))-1) );
			        theArr[i+ii] = (UINT16)(( aus_lut[ ko0 ] * ( (1<<(3+2+3)) - ko ) + aus_lut[ ko0 +1 ] * ko)>>((3+2+3)-aAlutShift));
				}
		       	if( separateAluts )aAlutOffset += aAlutAdrSize;
			}
	       	#if FARBR_DEBUG
	        DebugPrint("i=%ld\n",i);
	        DebugPrint("theArr=(d)");for(ii=0; ii<3; ++ii) DebugPrint("%ld ",theArr[i+ii]); DebugPrint("\n");
	        DebugPrint("ein_reg=(d)");DebugPrint("%ld ",aLong0);DebugPrint("%ld ",aLong1);DebugPrint("%ld ",aLong2); DebugPrint("\n");
	        #endif
		}
	}
	else{
		aAlutShift = aStructPtr->aAlutWordSize - 8;
		for (i = 0; i < theEnd; i +=3){		/* Schleife der Points*/
	
	
			aElutOffset = 0;
			ko = (theArr[i+0]);
			ko0 = ko - ( ko >> ( 16 - aElutShift ));
			ko = ko0 & ( aElutShiftNum - 1 );
			ko0 = ko0 >> aElutShift;
			if( ko0 >= (aElutAdrSize-1) ){
		        	aLong0 = ein_lut[ ko0 + aElutOffset ]<<(aElutShift-aElutShiftRight);
			}
			else{
				ko0 += aElutOffset;
		       	aLong0 = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 + 1 ] * ko )>>aElutShiftRight;
		    }   
			if( separateEluts )aElutOffset += aElutAdrSize;
			
			ko = (theArr[i+1]);
			ko0 = ko - ( ko >> ( 16 - aElutShift ));
			ko = ko0 & ( aElutShiftNum - 1 );
			ko0 = ko0 >> aElutShift;
			if( ko0 >= (aElutAdrSize-1) ){
		        	aLong1 = ein_lut[ ko0 + aElutOffset ]<<(aElutShift-aElutShiftRight);
			}
			else{
				ko0 += aElutOffset;
		       	aLong1 = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 + 1 ] * ko )>>aElutShiftRight;
		    }   
			if( separateEluts )aElutOffset += aElutAdrSize;
			
			ko = (theArr[i+2]);
			ko0 = ko - ( ko >> ( 16 - aElutShift ));
			ko = ko0 & ( aElutShiftNum - 1 );
			ko0 = ko0 >> aElutShift;
			if( ko0 >= (aElutAdrSize-1) ){
		        	aLong2 = ein_lut[ ko0 + aElutOffset ]<<(aElutShift-aElutShiftRight);
			}
			else{
				ko0 += aElutOffset;
		       	aLong2 = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 + 1 ] * ko )>>aElutShiftRight;
		    }   
		       
			aLong0 = ( aLong0 +2 )>>2;	       
			aLong1 = ( aLong1 +2 )>>2;	       
			aLong2 = ( aLong2 +2 )>>2;	       
			aAlutOffset = 0;
			if( aStructPtr->aAlutWordSize > 8 ){
				for( ii=0; ii<3; ii++){ 
						aVal = aMatrix[ii][0] * aLong0;
						aVal += aMatrix[ii][1] * aLong1;
						aVal += aMatrix[ii][2] * aLong2;
					aVal = (aVal+((1<<12)-1))>>13;
					if( aVal < 0 ) aVal = 0;
					
			        ko0= (aVal+((1<<(3+2-1))-1)) >> (3+2);
			        if( ko0 >= (aAlutAdrSize-1) ){
			        	theByteArr[i+ii] = aus_lut[ (aAlutAdrSize-1) + aAlutOffset ] >>aAlutShift;
			        }
			        else{
			        	theByteArr[i+ii] = aus_lut[ ko0 + aAlutOffset ] >>aAlutShift;
					}
			       	if( separateAluts )aAlutOffset += aAlutAdrSize;
				}
			}
			else{
				for( ii=0; ii<3; ii++){ 
						aVal = aMatrix[ii][0] * aLong0;
						aVal += aMatrix[ii][1] * aLong1;
						aVal += aMatrix[ii][2] * aLong2;
					aVal = (aVal+((1<<12)-1))>>13;
					if( aVal < 0 ) aVal = 0;
					
			        ko0= (aVal+((1<<(3+2-1))-1)) >> (3+2);
			        if( ko0 >= (aAlutAdrSize-1) ){
			        	theByteArr[i+ii] = aus_lutByte[ (aAlutAdrSize-1) + aAlutOffset ] ;
			        }
			        else{
			        	theByteArr[i+ii] = aus_lutByte[ ko0 + aAlutOffset ] ;
					}
			       	if( separateAluts )aAlutOffset += aAlutAdrSize;
				}
			}
	       	#if FARBR_DEBUG
	        DebugPrint("i=%ld\n",i);
	        DebugPrint("theArr=(d)");for(ii=0; ii<3; ++ii) DebugPrint("%ld ",theArr[i+ii]); DebugPrint("\n");
	        DebugPrint("ein_reg=(d)");DebugPrint("%ld ",aLong0);DebugPrint("%ld ",aLong1);DebugPrint("%ld ",aLong2); DebugPrint("\n");
	        #endif
		}
	}
	LH_END_PROC("DoMatrixForCube16")
}

void DoOnlyMatrixForCube16( Matrix2D	*theMatrix, Ptr aXlut, long aPointCount, long gridPointsCube )
{
    register long 	ii,jj;
    register long 	i;
    register long 	aLong0;
    register long 	aLong1;
    register long 	aLong2;
    register long 	theEnd;
    register long 	aVal;
    long 			aMatrix[3][3];
    register double aFactor,dVal;
    register UINT16 	accu,aMax;
    register UINT16 *theArr = (UINT16 *)aXlut;
    /*   FILE *aSt;*/
 
    
#ifdef DEBUG_OUTPUT
	long err = 0;
#endif
	LH_START_PROC("DoOnlyMatrixForCube16")
	jj=aPointCount;
	theEnd = 3 * aPointCount;
	/*for( i=1; i<100; ++i)if( i*i*i == jj )break; */  /* calculate gridpoints*/
	/*if( i<= 0 || i >= 100 ) return;*/
	i = gridPointsCube;
	aMax = VAL_MAXM1;
	aFactor = 4096.;
	for( ii=0; ii<3; ii++){ 
		for( jj=0; jj<3; jj++){ 
			dVal = (*theMatrix)[ii][jj]*aFactor;
			aMatrix[ii][jj] = (long)Round(dVal);
		}
	}
   	#if FARBR_DEBUG
    DebugPrint("theArr=(d)");for(ii=0; ii<3; ++ii) for( jj=0; jj<3; jj++) DebugPrint("%f ",aMatrix[ii][jj]); DebugPrint("\n");
    #endif
    /*
    aSt = fopen("Matrix","a");
    for(ii=0; ii<3; ++ii) for( jj=0; jj<3; jj++) fprintf(aSt,"%f ",(*(aStructPtr->theMatrix))[ii][jj]); fprintf(aSt,"\n");
    for(ii=0; ii<3; ++ii) for( jj=0; jj<3; jj++) fprintf(aSt,"%d ",aMatrix[ii][jj]); fprintf(aSt,"\n");
	fclose(aSt);
	*/
	for (i = 0; i < theEnd; i +=3){		/* Schleife der Points */
	       aLong0=theArr[i+0];
	       aLong1=theArr[i+1];
	       aLong2=theArr[i+2];
		for( ii=0; ii<3; ii++){ 
				aVal = aMatrix[ii][0] * aLong0;
				aVal += aMatrix[ii][1] * aLong1;
				aVal += aMatrix[ii][2] * aLong2;
			if( aVal > 0) aVal = (aVal+2047)>>12;
			else aVal = (aVal-2047)>>12;
			accu = (UINT16)CLIPP(aVal,0,(long)aMax);
        	theArr[i+ii] = accu;
		}
       	#if FARBR_DEBUG
        DebugPrint("i=%ld\n",i);
        DebugPrint("theArr=(d)");for(ii=0; ii<3; ++ii) DebugPrint("%ld ",theArr[i+ii]); DebugPrint("\n");
        DebugPrint("ein_reg=(d)");DebugPrint("%ld ",aLong0);DebugPrint("%ld ",aLong1);DebugPrint("%ld ",aLong2); DebugPrint("\n");
        #endif
	}
	LH_END_PROC("DoOnlyMatrixForCube16")
}


#if FARBR_FILES
static FileCount = 0;
static FILE *stream1;

/*ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
   WriteLuts
  ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ*/
void WriteLuts(	char *theName,long WordSize,long aElutAdrSize,long aElutWordSize,UINT16 *Elut,
				long aXlutInDim,long aXlutOutDim,long aXlutAdrSize,long aXlutWordSize,UINT16 *Xlut,
				long aAlutAdrSize,long aAlutWordSize,UINT16 *Alut)
{
	register unsigned long Size,i,ii,l,lMax;
	char FileNameBuffer[256];
	
	Size = aElutAdrSize*aXlutInDim;
    sprintf(FileNameBuffer,"%s Elut%d",theName,FileCount);
    stream1=fopen(FileNameBuffer,"wb");             /*  oeffne Schreibedatei            */
    if(stream1 == NULL){
        DebugPrint("Open %s failed \n",FileNameBuffer);
        return;
    }
	lMax=0;
	if( 2 == 2 ){ for(i=0; i<Size; ++i)if( lMax < Elut[i] )lMax = Elut[i];}
	else{ for(i=0; i<Size; ++i)if( lMax < ((UINT8 *)Elut)[i] )lMax = ((UINT8 *)Elut)[i];}
	sprintf(FileNameBuffer,"%s InputDimension=%d OutputDimension=%d AdrSize=%ld EndWert=%ld WordSize=%ld",
			FileNameBuffer,aXlutInDim,aXlutInDim,aElutAdrSize,lMax,aElutWordSize);
	i=strlen( FileNameBuffer )+1;
	for( ii=i; ii<((i+15)/16)*16; ii++)FileNameBuffer[ii-1]=' ';
	FileNameBuffer[ii-1]='\0';
	l=fprintf(stream1,"%s\n",FileNameBuffer);
	
    if(fwrite(Elut,sizeof(UINT16),Size,stream1) != Size){
            DebugPrint("Write Error %s\n",FileNameBuffer);
            return;
    }
    fclose(stream1);
    Size = 1;
    for( i=0; i<aXlutInDim; ++i)Size *= aXlutAdrSize;			/* Calc aXlutAdrSize^aXlutInDim */
	Size = Size*aXlutOutDim;
    sprintf(FileNameBuffer,"DoNDim Xlut%d",FileCount);
    stream1=fopen(FileNameBuffer,"wb");             /*  oeffne Schreibedatei            */
    if(stream1 == NULL){
        DebugPrint("Open %s failed \n",FileNameBuffer);
        return;
    }
	lMax=0;
	if( WordSize == 2 ){ for(i=0; i<Size; ++i)if( lMax < Xlut[i] )lMax = Xlut[i];}
	else{ for(i=0; i<Size; ++i)if( lMax < ((UINT8 *)Xlut)[i] )lMax = ((UINT8 *)Xlut)[i];}
	sprintf(FileNameBuffer,"%s InputDimension=%d OutputDimension=%d AdrSize=%ld EndWert=%ld WordSize=%ld",
			FileNameBuffer,aXlutInDim,aXlutOutDim,aXlutAdrSize,lMax,aXlutWordSize);
	i=strlen( FileNameBuffer )+1;
	for( ii=i; ii<((i+15)/16)*16; ii++)FileNameBuffer[ii-1]=' ';
	FileNameBuffer[ii-1]='\0';
	l=fprintf(stream1,"%s\n",FileNameBuffer);
	
    if(fwrite(Xlut,WordSize,Size,stream1) != Size){
            DebugPrint("Write Error %s\n",FileNameBuffer);
            return;
    }
    fclose(stream1);
	Size = aAlutAdrSize*aXlutOutDim;
    sprintf(FileNameBuffer,"DoNDim Alut%d",FileCount);
    stream1=fopen(FileNameBuffer,"wb");             /*  oeffne Schreibedatei            */
    if(stream1 == NULL){
        DebugPrint("Open %s failed \n",FileNameBuffer);
        return;
    }
	lMax=0;
	if( WordSize == 2 ){ for(i=0; i<Size; ++i)if( lMax < Alut[i] )lMax = Alut[i];}
	else{ for(i=0; i<Size; ++i)if( lMax < ((UINT8 *)Alut)[i] )lMax = ((UINT8 *)Alut)[i];}
	sprintf(FileNameBuffer,"%s InputDimension=%d OutputDimension=%d AdrSize=%ld EndWert=%ld WordSize=%ld",
			FileNameBuffer,aXlutOutDim,aXlutOutDim,aAlutAdrSize,lMax,aAlutWordSize);
	i=strlen( FileNameBuffer )+1;
	for( ii=i; ii<((i+15)/16)*16; ii++)FileNameBuffer[ii-1]=' ';
	FileNameBuffer[ii-1]='\0';
	l=fprintf(stream1,"%s\n",FileNameBuffer);
	
    if(fwrite(Alut,WordSize,Size,stream1) != Size){
            DebugPrint("Write Error %s\n",FileNameBuffer);
            return;
    }
    fclose(stream1);
    FileCount++;
}
#endif
