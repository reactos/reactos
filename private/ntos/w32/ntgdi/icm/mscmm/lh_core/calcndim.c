/*
	File:		LHCalcNDim_Lut16.c

	Contains:	

	Written by:	U. J. Krabbenhoeft

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

	Version:	
*/

#ifndef LHGeneralIncs_h
#include "General.h"
#endif

#ifndef LHCalcEngine_h
#include "CalcEng.h"
#endif

#ifndef LHCalcNDim_h
#include "CalcNDim.h"
#endif

#define UNROLL_NDIM 1
#if UNROLL_NDIM

CMError Calc323Dim_Data8To8_Lut16 	(CMCalcParamPtr calcParam,
									 CMLutParamPtr  lutParam);
CMError Calc324Dim_Data8To8_Lut16 	(CMCalcParamPtr calcParam,
									 CMLutParamPtr  lutParam);
CMError Calc423Dim_Data8To8_Lut16 	(CMCalcParamPtr calcParam,
									 CMLutParamPtr  lutParam);
CMError Calc424Dim_Data8To8_Lut16 	(CMCalcParamPtr calcParam,
									 CMLutParamPtr  lutParam);
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
void WriteLuts(	char *theName,long WordSize,long aElutAdrSize,long aElutWordSize,LH_UINT16 *Elut,
				long aXlutInDim,long aXlutOutDim,long aXlutAdrSize,long aXlutWordSize,LH_UINT16 *Xlut,
				long aAlutAdrSize,long aAlutWordSize,LH_UINT16 *aus_lut);
#endif

#ifdef DEBUG_OUTPUT
#define kThisFile kLHCalcNDim_Lut16ID
#else
#define DebugPrint(x)
#endif

#define CLIPPWord(x,a,b) ((x)<(a)?(LH_UINT16)(a):((x)>(b)?(LH_UINT16)(b):(LH_UINT16)(x+.5)))
#define CLIPP(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))
/*ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
	 DoNDim
  ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ*/
CMError CalcNDim_Data8To8_Lut16 	(CMCalcParamPtr calcParam,
									 CMLutParamPtr  lutParam)
	
{
	LH_UINT8 * inputData[8], *outputData[8];
	UINT32 InputIncrement, OutputIncrement, inputDataRowOffset, outputDataRowOffset, Pixelcount, LineCount;
   	register unsigned long adr0;
    register unsigned long ko0;
    unsigned long accu[8];
    register long i;
    /*long Offsets[8];*/
    
    register unsigned long nDim;
    register long aElutShift,aAlutShift,aElutOffset,aAlutOffset;
    register unsigned long aElutAdrSize;
    register long aElutAdrShift;
    register long aElutWordSize;
    register long aAlutAdrSize;
    register long aAlutAdrShift;
    register long aAlutWordSize;
    register unsigned long aXlutInDim;
    register unsigned long aXlutOutDim;
    register long aXlutAdrSize;
    register long aXlutAdrShift;
    register unsigned long aXlutWordSize;
    register long aInputPackMode8Bit;
    register long aOutputPackMode8Bit;
    register long aElutShiftNum;
	register unsigned long ii,jj;
	LH_UINT16 ein_Cache[8];
	
	
	LH_UINT16 * aus_lut	= (LH_UINT16*)lutParam->outputLut;
	LH_UINT16 * ein_lut	= (LH_UINT16*)lutParam->inputLut;
	LH_UINT16 * Xlut 	= (LH_UINT16*)lutParam->colorLut;
	
	Boolean aCopyAlpha;
   
	#ifdef DEBUG_OUTPUT
	long err = noErr;
	#endif
	LH_START_PROC("CalcNDim_Data8To8_Lut16")
#if UNROLL_NDIM
	if( lutParam->colorLutInDim == 3 &&
		calcParam->cmInputPixelOffset == 6 ){
		if(	lutParam->colorLutOutDim == 3 &&
			(	calcParam->cmOutputPixelOffset == 3 ||
				calcParam->cmOutputPixelOffset == 6)){
			return Calc323Dim_Data8To8_Lut16( calcParam, lutParam );
		}
		if(	lutParam->colorLutOutDim == 4 &&
			(	calcParam->cmOutputPixelOffset == 4 ||
				calcParam->cmOutputPixelOffset == 8) ){
			return Calc324Dim_Data8To8_Lut16( calcParam, lutParam );
		}
	}
	if( lutParam->colorLutInDim == 4 &&
		calcParam->cmInputPixelOffset == 8 ){
		if(	lutParam->colorLutOutDim == 3 &&
			(	calcParam->cmOutputPixelOffset == 3 ||
				calcParam->cmOutputPixelOffset == 6) ){
			return Calc423Dim_Data8To8_Lut16( calcParam, lutParam );
		}
		if(	lutParam->colorLutOutDim == 4 &&
			(	calcParam->cmOutputPixelOffset == 4 ||
				calcParam->cmOutputPixelOffset == 8) ){
			return Calc424Dim_Data8To8_Lut16( calcParam, lutParam );
		}
	}
#endif
	
	inputData[0] = (LH_UINT8 *)calcParam->inputData[0];
	inputData[1] = (LH_UINT8 *)calcParam->inputData[1];
	inputData[2] = (LH_UINT8 *)calcParam->inputData[2];
	inputData[3] = (LH_UINT8 *)calcParam->inputData[3];
	inputData[4] = (LH_UINT8 *)calcParam->inputData[4];
	inputData[5] = (LH_UINT8 *)calcParam->inputData[5];
	inputData[6] = (LH_UINT8 *)calcParam->inputData[6];
	inputData[7] = (LH_UINT8 *)calcParam->inputData[7];

	outputData[0] = (LH_UINT8 *)calcParam->outputData[0];
	outputData[1] = (LH_UINT8 *)calcParam->outputData[1];
	outputData[2] = (LH_UINT8 *)calcParam->outputData[2];
	outputData[3] = (LH_UINT8 *)calcParam->outputData[3];
	outputData[4] = (LH_UINT8 *)calcParam->outputData[4];
	outputData[5] = (LH_UINT8 *)calcParam->outputData[5];
	outputData[6] = (LH_UINT8 *)calcParam->outputData[6];
	outputData[7] = (LH_UINT8 *)calcParam->outputData[7];
	InputIncrement = calcParam->cmInputPixelOffset;
	OutputIncrement = calcParam->cmOutputPixelOffset;
	inputDataRowOffset = calcParam->cmInputBytesPerLine - calcParam->cmPixelPerLine * calcParam->cmInputPixelOffset + InputIncrement;
	outputDataRowOffset = calcParam->cmOutputBytesPerLine - calcParam->cmPixelPerLine * calcParam->cmOutputPixelOffset + OutputIncrement;

	Pixelcount = calcParam->cmPixelPerLine;
	LineCount = calcParam->cmLineCount;

	aElutAdrSize = lutParam->inputLutEntryCount;
	for ( i = 1; (i < 32) && (aElutAdrSize >> i); i++)
		aElutAdrShift = i;
	aElutWordSize = lutParam->inputLutWordSize;
	aAlutAdrSize = lutParam->outputLutEntryCount;
	for ( i = 1; (i < 32) && (aAlutAdrSize >> i); i++)
		aAlutAdrShift = i;
	aAlutWordSize = lutParam->outputLutWordSize;
	aXlutInDim = lutParam->colorLutInDim;
	aXlutOutDim = lutParam->colorLutOutDim;
	aXlutAdrSize = lutParam->colorLutGridPoints;
	for ( i = 1; (i < 32) && (aXlutAdrSize >> i); i++)
		aXlutAdrShift = i;
	aXlutWordSize = lutParam->colorLutWordSize;

	aInputPackMode8Bit = calcParam->cmInputColorSpace & cm8PerChannelPacking || calcParam->cmInputColorSpace & cmLong8ColorPacking;
	aOutputPackMode8Bit = calcParam->cmOutputColorSpace & cm8PerChannelPacking || calcParam->cmOutputColorSpace & cmLong8ColorPacking;
	
    /*DebugPrint("DoNDim with %d input elements\n",aByteCount);*/
	#if FARBR_FILES
	WriteLuts( 	"DoNDim",1,aElutAdrSize,aElutWordSize,ein_lut,
				aXlutInDim,aXlutOutDim,aXlutAdrSize,aXlutWordSize,(LH_UINT16 *)Xlut,aAlutAdrSize,aAlutWordSize,(LH_UINT16 *)aus_lut);
    #endif

	i=0;
			
	
	if( calcParam->copyAlpha )aCopyAlpha = 1;
	else aCopyAlpha = 0;
	if( aXlutInDim > 7 || aXlutOutDim > 7 )aCopyAlpha = 0;
	if( aInputPackMode8Bit != aOutputPackMode8Bit )aCopyAlpha = 0;
	
	nDim=aXlutInDim;
	
	if( aInputPackMode8Bit ){
		aElutShift = aElutAdrShift-8;
		if( aElutShift < 0 )
		{
			#ifdef DEBUG_OUTPUT
			DebugPrint("¥ DoNDim-Error: aElutShift < 0 (aElutShift = %d)\n",aElutShift);
			#endif
			return cmparamErr;
		}
	}
	else{
		aElutShift = 16-aElutAdrShift;
		if( aElutShift < 0 )
		{
			#ifdef DEBUG_OUTPUT
			DebugPrint("¥ DoNDim-Error: aElutShift < 0 (aElutShift = %d)\n",aElutShift);
			#endif
			return cmparamErr;
		}
	}
	
	if( aOutputPackMode8Bit ){
		aAlutShift = (aAlutWordSize-8);		
	}
	else{
		aAlutShift = (16 - aAlutWordSize);		
	}
        
	#ifdef DEBUG_OUTPUT
		if ( DebugCheck(kThisFile, kDebugReserved1) ){
		    DebugPrint("aElutAdrSize=%lx,aElutAdrShift=%lx,aElutWordSize=%lx,ein_lut=%lx,\n",
						aElutAdrSize,aElutAdrShift,aElutWordSize,ein_lut);
			DebugPrint("aAlutAdrSize=%lx,aAlutAdrShift=%lx,aAlutWordSize=%lx,aus_lut=%lx,\n",
						aAlutAdrSize,aAlutAdrShift,aAlutWordSize,aus_lut);
			DebugPrint("aXlutInDim=%lx,aXlutOutDim=%lx,aXlutAdrSize=%lx,aXlutAdrShift=%lx,aXlutWordSize=%lx,Xlut=%lx,\n",
						aXlutInDim,aXlutOutDim,aXlutAdrSize,aXlutAdrShift,aXlutWordSize,Xlut);
			DebugPrint("aInputPackMode8Bit=%lx,aOutputPackMode8Bit=%lx\n",
						aInputPackMode8Bit,aOutputPackMode8Bit );
		}
    #endif
    aElutShiftNum = 1<<aElutShift;
    
    /*if( 1 )*/
    if( aXlutAdrSize != (1<<aXlutAdrShift )){
    register long aXlutOffset;
#if FARBR_DEBUG
    register long aXlutPlaneOffset;
#endif
    long theXlutOffsets[8]; 
    register unsigned long aAlutInShift;
    register long aAlutInShiftRemainder;
    register unsigned long aAlutInShiftNum;
    register long aElutWordSizeMask = (1<<aElutWordSize) - 1;
    register unsigned long aAlutRound;
   aAlutInShift = aXlutWordSize + aElutWordSize - aAlutAdrShift;
    aAlutInShiftRemainder = 0;
    if( aAlutInShift > 16 ){
    	aAlutInShiftRemainder = aAlutInShift - 16;
    	aAlutInShift = 16;
    }
    aAlutInShiftNum = (1<<aAlutInShift);

#ifdef DEBUG_OUTPUT
        if ( DebugCheck(kThisFile, kDebugMiscInfo) )
            DebugPrint("  DoNDim gripoints = %ld\n",aXlutAdrSize);
#endif
    if( aElutWordSize <= 0 ){
#ifdef DEBUG_OUTPUT
        DebugPrint("¥ DoNDim-Error: (1<<aElutWordSize)/aXlutAdrSize <= 0 %d\n",(1<<aElutWordSize)/aXlutAdrSize);
#endif
        return cmparamErr;
    }
    if( aAlutInShift <= 0 ){
#ifdef DEBUG_OUTPUT
        DebugPrint("¥ DoNDim-Error: aAlutInShift <= 0 %d\n",aAlutInShift);
#endif
        return cmparamErr;
    }
    aXlutOffset =aXlutOutDim;
    for( i=0; i<(long)nDim; i++){
        theXlutOffsets[ nDim-1-i] = aXlutOffset;
        aXlutOffset *=aXlutAdrSize;
    }
	aAlutRound = 1<<( aAlutInShift + aAlutShift - 1 );

#ifdef DEBUG_OUTPUT
        if ( DebugCheck(kThisFile, kDebugReserved1) )
            DebugPrint(" aElutWordSize((1<<aElutWordSize)-0) = %ld\n aAlutInShift:((1<<aXlutWordSize)*aElutWordSize+(aAlutAdrSize/2))/aAlutAdrSize = %ld\n",aElutWordSize,aAlutInShift);
#endif
    
	while (LineCount){
		i = Pixelcount;
		
		while (i){
	
	        long adr[8],Index[8];
	        LH_UINT16 ein_reg[8];
	       	register unsigned long  adrAdr,ko,adrOffset;
	
	        adr0=0;
			aElutOffset = 0;
			jj=0;
			if( aInputPackMode8Bit ){
				for( ii=0; ii<nDim; ii++){ 
			        jj=ein_lut[((*(LH_UINT8 *)&ein_Cache[ii]=*inputData[ii])<<aElutShift)+aElutOffset];
			        jj *= aXlutAdrSize;
			        aElutOffset += aElutAdrSize;
		        	adr[ii] = jj & aElutWordSizeMask;
		        	jj = jj >> aElutWordSize;
		        	adr0 += (jj)*theXlutOffsets[ii];
		        	ein_reg[ii] = (LH_UINT16)jj;
				}
			}
			else{
				for( ii=0; ii<nDim; ii++){ 
			        jj=ein_Cache[ii]=(*(LH_UINT16 *)inputData[ii]);
					ko0 = jj - ( jj >> ( aElutAdrShift ));
					ko = ko0 & ( aElutShiftNum - 1 );
					ko0 = ko0 >> aElutShift;
					ko0 += aElutOffset;
			       	jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
			        jj *= aXlutAdrSize;
			        aElutOffset += aElutAdrSize;
		        	adr[ii] = jj & aElutWordSizeMask;
		        	jj = jj >> aElutWordSize;
		        	adr0 += (jj)*theXlutOffsets[ii];
		        	ein_reg[ii] = (LH_UINT16)jj;
				}
			}
	
       	#if FARBR_DEBUG
		aXlutPlaneOffset = nDim;
        DebugPrint("i=%ld o=%ld\n",i,o);
        if( aInputPackMode < k3ShortsUnpacked ){ DebugPrint("ein_arr=(d)");for(ii=0; ii<nDim; ++ii) DebugPrint("%ld ",ein_arr[i+ii]); DebugPrint("\n");}
        else{ DebugPrint("ein_arr=(d)");for(ii=0; ii<nDim; ++ii) DebugPrint("%ld ",aEinArr[i+ii]); DebugPrint("\n");}
        DebugPrint("ein_reg=(d)");for(ii=0; ii<nDim; ++ii) DebugPrint("%ld ",ein_reg[ii]/aXlutAdrSize); DebugPrint("\n");
	    DebugPrint("adr=(d)");for( ii=0; ii<nDim; ++ii) DebugPrint("%ld ",adr[ii] ); DebugPrint("\n");
        #endif

       	{								/* a kind of*/
			register unsigned long Top, Gap;
			register long Hold,Switches;
			
			Gap = nDim;
			
			for( ii=0; ii<nDim; ++ii){
				Index[ii] = ii;
			}
			do{
				/*Gap = (Gap * 10 ) / SHRINK_FACTOR;*/
				Gap = (Gap * ((10*16)/SHRINK_FACTOR) ) >>4;
				if( Gap == 0 ) Gap = 1;
				Switches = 0;
				Top = nDim - Gap;
				for( ii=0; ii<Top; ++ii){
					jj = ii + Gap;
					if( adr[Index[ii]] < adr[Index[jj]] ){
						Hold = Index[ii];
						Index[ii] = Index[jj];
						Index[jj] = Hold;
						Switches = 1;
					}
				}
			}while( Switches || Gap > 1 );
		}
		#if FARBR_DEBUG
	    DebugPrint("Index=");
	    for( ii=0; ii<nDim; ++ii){
	    	DebugPrint("%3ld ",Index[ii] );
	    }
	    DebugPrint("\n");
        #endif

        for( jj=0; jj<aXlutOutDim; ++jj)accu[jj]=0;

        ko0 = (1<<aElutWordSize);
        adrAdr=adr0;
        adrOffset=0;
        if( aXlutWordSize  == 16 ){
            for( ii=0; ii<nDim; ++ii){
                jj = Index[ii];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                   for( jj=0; jj<aXlutOutDim; ++jj){		    		
#if FARBR_DEBUG
                    DebugPrint("jj=(d)%ld adrAdr=%lx Address=%lx Xlut[Address]=(d)%ld ko=(d)%ld\n",jj,adrAdr,adrAdr+(jj*aXlutPlaneOffset),Xlut[adrAdr+(jj*aXlutPlaneOffset)],ko);
#endif
                    accu[jj]+=Xlut[adrAdr+(jj)]*ko;
                }

                adrAdr = (adr0 + adrOffset);
            }
               for( jj=0; jj<aXlutOutDim; ++jj){		    		
#if FARBR_DEBUG
                    DebugPrint("jj=(d)%ld adrAdr=%lx Address=%lx Xlut[Address]=(d)%ld ko=(d)%ld\n",jj,adrAdr,adrAdr+(jj*aXlutPlaneOffset),Xlut[adrAdr+(jj*aXlutPlaneOffset)],ko0);
#endif
                accu[jj]+=Xlut[adrAdr+jj]*ko0;
            }
        }
        else{

            for( ii=0; ii<nDim; ++ii){
                jj = Index[ii];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                   for( jj=0; jj<aXlutOutDim; ++jj){		    		
#if FARBR_DEBUG
                    DebugPrint("jj=(d)%ld adrAdr=%lx Address=%lx Xlut[Address]=(d)%ld ko=(d)%ld\n",jj,adrAdr,adrAdr+(jj*aXlutPlaneOffset),Xlut[adrAdr+(jj*aXlutPlaneOffset)],ko);
#endif
                    accu[jj]+=((LH_UINT8 *)Xlut)[adrAdr+(jj)]*ko;
                }

                adrAdr = (adr0 + adrOffset);
            }
               for( jj=0; jj<aXlutOutDim; ++jj){		    		
#if FARBR_DEBUG
                    DebugPrint("jj=(d)%ld adrAdr=%lx Address=%lx Xlut[Address]=(d)%ld ko=(d)%ld\n",jj,adrAdr,adrAdr+(jj*aXlutPlaneOffset),Xlut[adrAdr+(jj*aXlutPlaneOffset)],ko0);
#endif
                accu[jj]+=((LH_UINT8 *)Xlut)[adrAdr+jj]*ko0;
                /*ii = accu[jj]+((LH_UINT8 *)Xlut)[adrAdr+jj]*ko0;
                accu[jj] = ii +( ii >> 8 );*/
            }
        }

#if FARBR_DEBUG
        DebugPrint("accu=(d)");for( ii=0; ii<aXlutOutDim; ++ii) DebugPrint("%3ld ",accu[ii] ); DebugPrint("\n");
#endif
		aAlutOffset = 0;

	        if( aOutputPackMode8Bit ){
	        	for( ii=0; ii<aXlutOutDim; ++ii){
	#if FARBR_DEBUG
	                ii = ((accu[ii]) >> aAlutInShift)+aAlutOffset;
	                DebugPrint("adr:((accu[ii]) >> aAlutInShift)+aAlutOffset = %ld\n",ii);
	                DebugPrint("aus_lut[%ld]=%ld aus_lut[%ld]=%ld aus_lut[%ld]=%ld \n",ii-1,aus_lut[ii-1],ii,aus_lut[ii],ii+1,aus_lut[ii+1]);
	#endif
					jj = accu[ii];
					jj = jj + ( jj >> aXlutWordSize );

					ko0 = (jj - ( jj >> ( aAlutAdrShift ))) >> aAlutInShiftRemainder; 	/*	aAlutInShift = aXlutWordSize + aElutWordSize - aAlutAdrShift;*/

					ko = ko0 & (aAlutInShiftNum - 1 );
					ko0 = ko0 >> aAlutInShift;
					ko0 += aAlutOffset;
 					if( aAlutWordSize <= 8)
	       				jj = ( ((LH_UINT8*)aus_lut)[ ko0 ] * ( aAlutInShiftNum - ko ) + ((LH_UINT8*)aus_lut)[ ko0 +1 ] * ko ) >> ( aAlutInShift );
 					else{
	       				jj = ( aus_lut[ ko0 ] * ( aAlutInShiftNum - ko ) + aus_lut[ ko0 +1 ] * ko );
						jj = jj - ( jj >> aAlutShift );
						jj = ( jj + aAlutRound ) >> (aAlutInShift + aAlutShift);
					}
					*outputData[ii] = (LH_UINT8)jj;
	                aAlutOffset += aAlutAdrSize;
	            }
	        }
	        else{
				if( aXlutWordSize >= 16 ){
					for( ii=0; ii<aXlutOutDim; ++ii){
						jj = accu[ii];
						jj = jj + ( jj >> aXlutWordSize );
              			ko0 = (jj - ( jj >> ( aAlutAdrShift ))) >> aAlutInShiftRemainder; /*	aAlutInShift = aXlutWordSize + aElutWordSize - aAlutAdrShift;*/
		
						ko = ko0 & (aAlutInShiftNum - 1 );
						ko0 = ko0 >> aAlutInShift;
						ko0 += aAlutOffset;
	 					if( aAlutWordSize <= 8)
		       				jj = ( ((LH_UINT8*)aus_lut)[ ko0 ] * ( aAlutInShiftNum - ko ) + ((LH_UINT8*)aus_lut)[ ko0 +1 ] * ko ) << ( aAlutShift - aAlutInShift );
	 					else
		       				jj = ( aus_lut[ ko0 ] * ( aAlutInShiftNum - ko ) + aus_lut[ ko0 +1 ] * ko ) >> (aAlutInShift- aAlutShift);
						*((LH_UINT16 *)outputData[ii]) =  (LH_UINT16)jj;
						aAlutOffset += aAlutAdrSize;
					}
				}
				else{
					for( ii=0; ii<aXlutOutDim; ++ii){
						jj = accu[ii];
						jj = jj + ( jj >> aXlutWordSize ) + ( jj >> 2*aXlutWordSize );
              			ko0 = (jj - ( jj >> ( aAlutAdrShift ))) ; /*	aAlutInShift = aXlutWordSize + aElutWordSize - aAlutAdrShift;*/
		
						ko = ko0 & (aAlutInShiftNum - 1 );
						ko0 = ko0 >> aAlutInShift;
						ko0 += aAlutOffset;
	 					if( aAlutWordSize <= 8)
		       				jj = ( ((LH_UINT8*)aus_lut)[ ko0 ] * ( aAlutInShiftNum - ko ) + ((LH_UINT8*)aus_lut)[ ko0 +1 ] * ko ) << ( aAlutShift - aAlutInShift );
	 					else
		       				jj = ( aus_lut[ ko0 ] * ( aAlutInShiftNum - ko ) + aus_lut[ ko0 +1 ] * ko ) >> (aAlutInShift- aAlutShift);
						*((LH_UINT16 *)outputData[ii]) =  (LH_UINT16)jj;
						aAlutOffset += aAlutAdrSize;
					}
				}
	        }
	        #if FARBR_DEBUG
		    if( aOutputPackMode8Bit ){DebugPrint("outputData=(d)");for( ii=0; ii<aXlutOutDim; ++ii) DebugPrint("%3ld ",outputData[ii+o] ); DebugPrint("\n");}
		    else{DebugPrint("outputData=(d)");for( ii=0; ii<aXlutOutDim; ++ii) DebugPrint("%3ld ",aAusArr[ii+o] ); DebugPrint("\n");}
			#endif
			if( aCopyAlpha ){
				if( aOutputPackMode8Bit )
					*outputData[aXlutOutDim] = *inputData[aXlutInDim];
				else
					*((LH_UINT16 *)outputData[aXlutOutDim]) = *((LH_UINT16 *)inputData[aXlutInDim]);
			}
			while (--i){
			   	for( jj=0; jj<aXlutInDim; ++jj){
					inputData[jj] += InputIncrement;
				}
			   	for( jj=0; jj<aXlutOutDim; ++jj){
					outputData[jj] += OutputIncrement;
				}
				if( aCopyAlpha ){
					inputData[aXlutInDim] += InputIncrement;
					outputData[aXlutOutDim] += OutputIncrement;
				}
	
				if( aInputPackMode8Bit ){
				   	for( jj=0; jj<aXlutInDim; ++jj){
						if( *inputData[jj] ^ *(LH_UINT8 *)(&ein_Cache[jj]) )break;
					}
				}
				else{
				   	for( jj=0; jj<aXlutInDim; ++jj){
						if( *((LH_UINT16 *)inputData[jj]) ^ ein_Cache[jj] )break;
					}
				}
				if( jj<aXlutInDim ) break;
				if( aOutputPackMode8Bit ){
				   	for( jj=0; jj<aXlutOutDim; ++jj){
						*outputData[jj] = outputData[jj][-(long)OutputIncrement];
					}
					if( aCopyAlpha ){
						*outputData[aXlutOutDim] = *inputData[aXlutInDim];
					}
				}
				else{
				   	for( jj=0; jj<aXlutOutDim; ++jj){
						*((LH_UINT16 *)outputData[jj]) = *(LH_UINT16 *)(&outputData[jj][-(long)OutputIncrement]);
					}
					if( aCopyAlpha ){
						*((LH_UINT16 *)outputData[aXlutOutDim]) = *((LH_UINT16 *)inputData[aXlutInDim]);
					}
				}
			}
		}
		
	   	if( --LineCount ){
		   	for( jj=0; jj<aXlutInDim; ++jj){
				inputData[jj] += inputDataRowOffset;
			}
		   	for( jj=0; jj<aXlutOutDim; ++jj){
				outputData[jj] += outputDataRowOffset;
			}
			if( aCopyAlpha ){
				inputData[aXlutInDim] += inputDataRowOffset;
				outputData[aXlutOutDim] += outputDataRowOffset;
			}
		}
    }
    }
    else{

    register unsigned long  bit_breit_selektor;
    register unsigned long  bit_maske_selektor;
    register unsigned long  bit_breit_adr;
    register unsigned long  bit_maske_adr;
    register unsigned long  aAlutInShiftNum;
    register long aAlutInShift;
    register long aAlutInShiftRemainder;
    register unsigned long aAlutRound;
    /*register long aXlutPlaneShift = aXlutAdrShift*aXlutInDim;*/
    bit_breit_selektor=aElutWordSize-aXlutAdrShift;
    if( aElutWordSize-aXlutAdrShift < 0 )
    {
#ifdef DEBUG_OUTPUT
        DebugPrint("¥ DoNDim-Error: bit_breit_selektor < 0 (bit_breit_selektor = %d)\n",bit_breit_selektor);
#endif
        return cmparamErr;
    }
    bit_maske_selektor=(1<<bit_breit_selektor)-1;
    bit_breit_adr=aXlutAdrShift;
    bit_maske_adr=((1<<bit_breit_adr)-1)<<bit_breit_selektor;
    aAlutInShift = (aXlutWordSize+bit_breit_selektor-aAlutAdrShift);
    /*aAlutInShift = aXlutWordSize + aElutWordSize - aAlutAdrShift;*/
    aAlutInShiftRemainder = 0;
    if( aAlutInShift > 16 ){
    	aAlutInShiftRemainder = aAlutInShift - 16;
    	aAlutInShift = 16;
    }
    	
    aAlutInShiftNum = (1<<aAlutInShift);
    
	aAlutRound = 1<<( aAlutInShift + aAlutShift - 1 );
	while (LineCount){
		i = Pixelcount;
		
		while (i){
	
	        long adr[8],Index[8];
	        /*LH_UINT16 *ein_lut = (LH_UINT16 *)ein_lut;*/
	        LH_UINT16 ein_reg[8];
	           register unsigned long  adrAdr,ko,adrOffset;
	        /*register unsigned long aIndex;*/
	
	        adr0=0;
	        aElutOffset = 0;
	        jj=0;
			if( aInputPackMode8Bit ){
	            for( ii=0; ii<nDim; ii++){ 
	                jj=ein_lut[((*(LH_UINT8 *)&ein_Cache[ii]=*inputData[ii])<<aElutShift)+aElutOffset];
	                aElutOffset += aElutAdrSize;
	                adr[ii] = (jj & bit_maske_selektor);
	                adr0 |= ((jj & bit_maske_adr)>>bit_breit_selektor)<<((nDim-ii-1)*bit_breit_adr);
	                ein_reg[ii] = (LH_UINT16)jj;
	            }
	        }
	        else{
	            for( ii=0; ii<nDim; ii++){ 
	                jj=ein_Cache[ii]=(*(LH_UINT16 *)inputData[ii]);
	                ko0 = jj - ( jj >> ( aElutAdrShift ));
	                ko = ko0 & ( aElutShiftNum - 1 );
	                ko0 = ko0 >> aElutShift;
	                ko0 += aElutOffset;
	                   jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
	                aElutOffset += aElutAdrSize;
	                adr[ii] = (jj & bit_maske_selektor);
	                adr0 |= ((jj & bit_maske_adr)>>bit_breit_selektor)<<((nDim-ii-1)*bit_breit_adr);
	                ein_reg[ii] = (LH_UINT16)jj;
	            }
	        }
	
	        adr0 *= aXlutOutDim;
	#if FARBR_DEBUG
	        DebugPrint("i=%ld o=%ld\n",i,o);
	        DebugPrint("adr0=%ld\n",adr0);
	        if( aInputPackMode < k3ShortsUnpacked ){ DebugPrint("ein_arr=(d)");for(ii=0; ii<nDim; ++ii) DebugPrint("%ld ",ein_arr[i+ii]); DebugPrint("\n");}
	        else{ DebugPrint("ein_arr=(d)");for(ii=0; ii<nDim; ++ii) DebugPrint("%ld ",aEinArr[i+ii]); DebugPrint("\n");}
	        DebugPrint("ein_reg=(d)");for(ii=0; ii<nDim; ++ii) DebugPrint("%ld ",ein_reg[ii]); DebugPrint("\n");
	        DebugPrint("adr=(d)");for( ii=0; ii<nDim; ++ii) DebugPrint("%ld ",adr[ii] ); DebugPrint("\n");
	#endif
	
	           {								/* a kind of */
	            register unsigned long Top, Gap;
	            register long Hold,Switches;
	            
	            Gap = nDim;
	            
	            for( ii=0; ii<nDim; ++ii){
	                Index[ii] = ii;
	            }
	            do{
	                /*Gap = (Gap * 10 ) / SHRINK_FACTOR;*/
	                Gap = (Gap * ((10*16)/SHRINK_FACTOR) ) >>4;
	                if( Gap == 0 ) Gap = 1;
	                Switches = 0;
	                Top = nDim - Gap;
	                for( ii=0; ii<Top; ++ii){
	                    jj = ii + Gap;
	                    if( adr[Index[ii]] < adr[Index[jj]] ){
	                        Hold = Index[ii];
	                        Index[ii] = Index[jj];
	                        Index[jj] = Hold;
	                        Switches = 1;
	                    }
	                }
	            }while( Switches || Gap > 1 );
	        }
	#if FARBR_DEBUG
	        DebugPrint("Index=");
	        for( ii=0; ii<nDim; ++ii){
	            DebugPrint("%3ld ",Index[ii] );
	        }
	        DebugPrint("\n");
	#endif
	
	           for( jj=0; jj<aXlutOutDim; ++jj)accu[jj]=0;
	
	        ko0 = bit_maske_selektor+1;
	        adrAdr=adr0;
	        adrOffset=0;
	
	        if( aXlutWordSize  == 16 ){
	            for( ii=0; ii<nDim; ++ii){
	                jj = Index[ii];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(nDim-1-jj)*bit_breit_adr);
	
	                   for( jj=0; jj<aXlutOutDim; ++jj){		    		
	#if FARBR_DEBUG
	                    DebugPrint("jj=(d)%ld adrAdr=%lx Address=%lx Xlut[Address]=(d)%ld ko=(d)%ld\n",jj,adrAdr,adrAdr+(jj),Xlut[adrAdr+(jj)],ko);
	#endif
	                    accu[jj]+=Xlut[adrAdr+(jj)]*ko;
	                }
	
	                adrAdr = (adr0 + aXlutOutDim*adrOffset);
	            }
	               for( jj=0; jj<aXlutOutDim; ++jj){		    		
	#if FARBR_DEBUG
	                    DebugPrint("jj=(d)%ld adrAdr=%lx Address=%lx Xlut[Address]=(d)%ld ko=(d)%ld\n",jj,adrAdr,adrAdr+(jj),Xlut[adrAdr+(jj)],ko0);
	#endif
	                accu[jj]+=Xlut[adrAdr+(jj)]*ko0;
	            }
	        }
	        else{
	
	            for( ii=0; ii<nDim; ++ii){
	                jj = Index[ii];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(nDim-1-jj)*bit_breit_adr);
	
	                   for( jj=0; jj<aXlutOutDim; ++jj){		    		
	#if FARBR_DEBUG
	                    DebugPrint("jj=(d)%ld adrAdr=%lx Address=%lx Xlut[Address]=(d)%ld ko=(d)%ld\n",jj,adrAdr,adrAdr+(jj),Xlut[adrAdr+(jj)],ko);
	#endif
	                    accu[jj]+=((LH_UINT8 *)Xlut)[adrAdr+(jj)]*ko;
	                }
	
	                adrAdr = (adr0 + aXlutOutDim*adrOffset);
	            }
	               for( jj=0; jj<aXlutOutDim; ++jj){		    		
	#if FARBR_DEBUG
	                    DebugPrint("jj=(d)%ld adrAdr=%lx Address=%lx Xlut[Address]=(d)%ld ko=(d)%ld\n",jj,adrAdr,adrAdr+(jj),Xlut[adrAdr+(jj)],ko0);
	#endif
	                accu[jj]+=((LH_UINT8 *)Xlut)[adrAdr+(jj)]*ko0;
	            }
	        }
	
	#if FARBR_DEBUG
	        DebugPrint("accu=(d)");for( ii=0; ii<aXlutOutDim; ++ii) DebugPrint("%3ld ",accu[ii] ); DebugPrint("\n");
	#endif
	        aAlutOffset = 0;
	
	        if( aOutputPackMode8Bit ){
	        	for( ii=0; ii<aXlutOutDim; ++ii){
	#if FARBR_DEBUG
	                ii = ((accu[ii]) >> aAlutInShift)+aAlutOffset;
	                DebugPrint("adr:((accu[ii]) >> aAlutInShift)+aAlutOffset = %ld\n",ii);
	                DebugPrint("aus_lut[%ld]=%ld aus_lut[%ld]=%ld aus_lut[%ld]=%ld \n",ii-1,aus_lut[ii-1],ii,aus_lut[ii],ii+1,aus_lut[ii+1]);
	#endif
					jj = accu[ii];
					jj = jj + ( jj >> aXlutWordSize );

					ko0 = (jj - ( jj >> ( aAlutAdrShift ))) >> aAlutInShiftRemainder; 	/*	aAlutInShift = aXlutWordSize + aElutWordSize - aAlutAdrShift;*/

					ko = ko0 & (aAlutInShiftNum - 1 );
					ko0 = ko0 >> aAlutInShift;
					ko0 += aAlutOffset;
 					if( aAlutWordSize <= 8)
	       				jj = ( ((LH_UINT8*)aus_lut)[ ko0 ] * ( aAlutInShiftNum - ko ) + ((LH_UINT8*)aus_lut)[ ko0 +1 ] * ko ) >> ( aAlutInShift );
 					else{
						jj = ( aus_lut[ ko0 ] * ( aAlutInShiftNum - ko ) + aus_lut[ ko0 +1 ] * ko );
						jj = jj - ( jj >> aAlutShift );
						jj = ( jj + aAlutRound ) >> (aAlutInShift + aAlutShift);
					}

					*outputData[ii] = (LH_UINT8)jj;
	                aAlutOffset += aAlutAdrSize;
	            }
	        }
	        else{
				if( aXlutWordSize >= 16 ){
					for( ii=0; ii<aXlutOutDim; ++ii){
						jj = accu[ii];
						jj = jj + ( jj >> aXlutWordSize );
              			ko0 = (jj - ( jj >> ( aAlutAdrShift ))) >> aAlutInShiftRemainder; /*	aAlutInShift = aXlutWordSize + aElutWordSize - aAlutAdrShift;*/
		
						ko = ko0 & (aAlutInShiftNum - 1 );
						ko0 = ko0 >> aAlutInShift;
						ko0 += aAlutOffset;
	 					if( aAlutWordSize <= 8)
		       				jj = ( ((LH_UINT8*)aus_lut)[ ko0 ] * ( aAlutInShiftNum - ko ) + ((LH_UINT8*)aus_lut)[ ko0 +1 ] * ko ) << ( aAlutShift - aAlutInShift );
	 					else
		       				jj = ( aus_lut[ ko0 ] * ( aAlutInShiftNum - ko ) + aus_lut[ ko0 +1 ] * ko ) >> (aAlutInShift- aAlutShift);
						*((LH_UINT16 *)outputData[ii]) =  (LH_UINT16)jj;
						aAlutOffset += aAlutAdrSize;
					}
				}
				else{
					for( ii=0; ii<aXlutOutDim; ++ii){
						jj = accu[ii];
						jj = jj + ( jj >> aXlutWordSize ) + ( jj >> 2*aXlutWordSize );
              			ko0 = (jj - ( jj >> ( aAlutAdrShift ))); /*	aAlutInShift = aXlutWordSize + aElutWordSize - aAlutAdrShift;*/
		
						ko = ko0 & (aAlutInShiftNum - 1 );
						ko0 = ko0 >> aAlutInShift;
						ko0 += aAlutOffset;
	 					if( aAlutWordSize <= 8)
		       				jj = ( ((LH_UINT8*)aus_lut)[ ko0 ] * ( aAlutInShiftNum - ko ) + ((LH_UINT8*)aus_lut)[ ko0 +1 ] * ko ) << ( aAlutShift - aAlutInShift );
	 					else
		       				jj = ( aus_lut[ ko0 ] * ( aAlutInShiftNum - ko ) + aus_lut[ ko0 +1 ] * ko ) >> (aAlutInShift- aAlutShift);
						*((LH_UINT16 *)outputData[ii]) =  (LH_UINT16)jj;
						aAlutOffset += aAlutAdrSize;
					}
				}
	        }
	
	#if FARBR_DEBUG
	        if( aOutputPackMode < k3ShortsUnpacked ){DebugPrint("aus_arr=(d)");for( ii=0; ii<aXlutOutDim; ++ii) DebugPrint("%3ld ",aus_arr[ii+o] ); DebugPrint("\n");}
	        else{DebugPrint("aus_arr=(d)");for( ii=0; ii<aXlutOutDim; ++ii) DebugPrint("%3ld ",aAusArr[ii+o] ); DebugPrint("\n");}
	#endif
			if( aCopyAlpha ){
				if( aOutputPackMode8Bit )
					*outputData[aXlutOutDim] = *inputData[aXlutInDim];
				else
					*((LH_UINT16 *)outputData[aXlutOutDim]) = *((LH_UINT16 *)inputData[aXlutInDim]);
			}
			while (--i){
			   	for( jj=0; jj<aXlutInDim; ++jj){
					inputData[jj] += InputIncrement;
				}
			   	for( jj=0; jj<aXlutOutDim; ++jj){
					outputData[jj] += OutputIncrement;
				}
				if( aCopyAlpha ){
					inputData[aXlutInDim] += InputIncrement;
					outputData[aXlutOutDim] += OutputIncrement;
				}
	
				if( aInputPackMode8Bit ){
				   	for( jj=0; jj<aXlutInDim; ++jj){
						if( *inputData[jj] ^ *(LH_UINT8 *)(&ein_Cache[jj]) )break;
					}
				}
				else{
				   	for( jj=0; jj<aXlutInDim; ++jj){
						if( *((LH_UINT16 *)inputData[jj]) ^ ein_Cache[jj] )break;
					}
				}
				if( jj<aXlutInDim ) break;
				if( aOutputPackMode8Bit ){
				   	for( jj=0; jj<aXlutOutDim; ++jj){
						*outputData[jj] = outputData[jj][-(long)OutputIncrement];
					}
					if( aCopyAlpha ){
						*outputData[aXlutOutDim] = *inputData[aXlutInDim];
					}
				}
				else{
				   	for( jj=0; jj<aXlutOutDim; ++jj){
						*((LH_UINT16 *)outputData[jj]) = *(LH_UINT16 *)(&outputData[jj][-(long)OutputIncrement]);
					}
					if( aCopyAlpha ){
						*((LH_UINT16 *)outputData[aXlutOutDim]) = *((LH_UINT16 *)inputData[aXlutInDim]);
					}
				}
			}
		}
		
	   	if( --LineCount ){
		   	for( jj=0; jj<aXlutInDim; ++jj){
				inputData[jj] += inputDataRowOffset;
			}
		   	for( jj=0; jj<aXlutOutDim; ++jj){
				outputData[jj] += outputDataRowOffset;
			}
			if( aCopyAlpha ){
				inputData[aXlutInDim] += inputDataRowOffset;
				outputData[aXlutOutDim] += outputDataRowOffset;
			}
		}
    }
    }

	/* UNLOCK_DATA( aElutHdle ); */
	/* UNLOCK_DATA( aAlutHdle ); */
	/* UNLOCK_DATA( aXlutHdle ); */

	LH_END_PROC("CalcNDim_Data8To8_Lut16")
	return noErr;
}


/*ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
   CalcNDim_Data8To16_Lut16
  ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ*/
CMError CalcNDim_Data8To16_Lut16 	(CMCalcParamPtr calcParam,
									 CMLutParamPtr  lutParam)
{
	return CalcNDim_Data8To8_Lut16( calcParam, lutParam );
}

/*ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
   CalcNDim_Data8To8_Lut16
  ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ*/
CMError CalcNDim_Data16To8_Lut16 	(CMCalcParamPtr calcParam,
									 CMLutParamPtr  lutParam)
{
	return CalcNDim_Data8To8_Lut16( calcParam, lutParam );
}

/*ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
   CalcNDim_Data16To16_Lut16
  ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ*/
CMError CalcNDim_Data16To16_Lut16 	(CMCalcParamPtr calcParam,
									 CMLutParamPtr  lutParam)
{
	return CalcNDim_Data8To8_Lut16( calcParam, lutParam );
}

/*ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
   CalcNDim_Data8To16_Lut16
  ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ*/
CMError CalcNDim_Data8To8_Lut8 	(CMCalcParamPtr calcParam,
									 CMLutParamPtr  lutParam)
{
	return CalcNDim_Data8To8_Lut16( calcParam, lutParam );
}

/*ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
   CalcNDim_Data8To8_Lut16
  ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ*/
CMError CalcNDim_Data16To8_Lut8 	(CMCalcParamPtr calcParam,
									 CMLutParamPtr  lutParam)
{
	return CalcNDim_Data8To8_Lut16( calcParam, lutParam );
}

/*ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
   CalcNDim_Data16To16_Lut16
  ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ*/
CMError CalcNDim_Data8To16_Lut8 	(CMCalcParamPtr calcParam,
									 CMLutParamPtr  lutParam)
{
	return CalcNDim_Data8To8_Lut16( calcParam, lutParam );
}

/*ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
   CalcNDim_Data8To16_Lut16
  ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ*/
CMError CalcNDim_Data16To16_Lut8 	(CMCalcParamPtr calcParam,
									 CMLutParamPtr  lutParam)
{
	return CalcNDim_Data8To8_Lut16( calcParam, lutParam );
}

