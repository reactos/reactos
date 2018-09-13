/*
	File:		LHCalcND3_Lut16.c

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

#ifdef DEBUG_OUTPUT
#define kThisFile kLHCalcNDim_Lut16ID
#else
#define DebugPrint(x)
#endif

#define CLIPPWord(x,a,b) ((x)<(a)?(LH_UINT16)(a):((x)>(b)?(LH_UINT16)(b):(LH_UINT16)(x+.5)))
#define CLIPP(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))

#define UNROLL_NDIM 1
#if UNROLL_NDIM
#define NDIM_IN_DIM 3
#define NDIM_OUT_DIM 3
#define aElutShift (16-adr_breite_elut)
#define aElutShiftNum (1<<aElutShift)

CMError Calc323Dim_Data8To8_Lut16 	(CMCalcParamPtr calcParam,
									 CMLutParamPtr  lutParam)
	
{
	LH_UINT8 * inputData[8], *outputData[8];
	UINT32 OutputIncrement, inputDataRowOffset, outputDataRowOffset, Pixelcount, LineCount;
   	register unsigned long adr0;
    register unsigned long ko0;
    unsigned long accu[8];
    register long i;
    /*long Offsets[8];*/
    
    register long aAlutShift,aElutOffset,aAlutOffset;
    register long aElutWordSize;
    register long aAlutWordSize;
    register long aXlutAdrSize;
    register long aXlutAdrShift;
    register unsigned long aXlutWordSize;
	register unsigned long ii,jj;
    register long aOutputPackMode8Bit;
	long ein_Cache[8];
	
	LH_UINT16 * aus_lut	= (LH_UINT16*)lutParam->outputLut;
	LH_UINT16 * ein_lut	= (LH_UINT16*)lutParam->inputLut;
	LH_UINT16 * Xlut 	= (LH_UINT16*)lutParam->colorLut;
	
	#ifdef DEBUG_OUTPUT
	long err = noErr;
	#endif
	LH_START_PROC("Calc323Dim_Data8To8_Lut16")
	
	inputData[0] = (LH_UINT8 *)calcParam->inputData[0];
	inputData[1] = (LH_UINT8 *)calcParam->inputData[1];
	inputData[2] = (LH_UINT8 *)calcParam->inputData[2];
#if NDIM_IN_DIM == 4
	inputData[3] = (LH_UINT8 *)calcParam->inputData[3];
#endif
	outputData[0] = (LH_UINT8 *)calcParam->outputData[0];
	outputData[1] = (LH_UINT8 *)calcParam->outputData[1];
	outputData[2] = (LH_UINT8 *)calcParam->outputData[2];
#if NDIM_OUT_DIM == 4
	outputData[3] = (LH_UINT8 *)calcParam->outputData[3];
#endif

	OutputIncrement = calcParam->cmOutputPixelOffset;
	inputDataRowOffset = calcParam->cmInputBytesPerLine - calcParam->cmPixelPerLine * calcParam->cmInputPixelOffset + (NDIM_IN_DIM * 2);
	outputDataRowOffset = calcParam->cmOutputBytesPerLine - calcParam->cmPixelPerLine * calcParam->cmOutputPixelOffset + OutputIncrement;

	Pixelcount = calcParam->cmPixelPerLine;
	LineCount = calcParam->cmLineCount;

	aElutWordSize = lutParam->inputLutWordSize;
	aAlutWordSize = lutParam->outputLutWordSize;
	aXlutAdrSize = lutParam->colorLutGridPoints;
	for ( i = 1; (i < 32) && (aXlutAdrSize >> i); i++)
		aXlutAdrShift = i;
	aXlutWordSize = lutParam->colorLutWordSize;

 	aOutputPackMode8Bit = calcParam->cmOutputColorSpace & cm8PerChannelPacking || calcParam->cmOutputColorSpace & cmLong8ColorPacking;
   /*DebugPrint("DoNDim with %d input elements\n",aByteCount);*/
	#if FARBR_FILES
	WriteLuts( 	"DoNDim",1,adr_bereich_elut,aElutWordSize,ein_lut,
				NDIM_IN_DIM,NDIM_OUT_DIM,aXlutAdrSize,aXlutWordSize,(LH_UINT16 *)Xlut,adr_bereich_alut,aAlutWordSize,(LH_UINT16 *)aus_lut);
    #endif

	i=0;
					
	{
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
						adr_bereich_elut,adr_breite_elut,aElutWordSize,ein_lut);
			DebugPrint("aAlutAdrSize=%lx,aAlutAdrShift=%lx,aAlutWordSize=%lx,aus_lut=%lx,\n",
						adr_bereich_alut,adr_breite_alut,aAlutWordSize,aus_lut);
			DebugPrint("aXlutInDim=%lx,aXlutOutDim=%lx,aXlutAdrSize=%lx,aXlutAdrShift=%lx,aXlutWordSize=%lx,Xlut=%lx,\n",
						NDIM_IN_DIM,NDIM_OUT_DIM,aXlutAdrSize,aXlutAdrShift,aXlutWordSize,Xlut);
		}
    #endif
    
    /*if( 1 )*/
    if( aXlutAdrSize != (1<<aXlutAdrShift )){
    register long aXlutOffset;
    long theXlutOffsets[8]; 
    register unsigned long aAlutInShift;
    register long aAlutInShiftRemainder;
    register unsigned long aAlutInShiftNum;
    register long aElutWordSizeMask = (1<<aElutWordSize) - 1;
    register unsigned long aAlutRound;
   aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;
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
    aXlutOffset =NDIM_OUT_DIM;
    for( i=0; i<(long)NDIM_IN_DIM; i++){
        theXlutOffsets[ NDIM_IN_DIM-1-i] = aXlutOffset;
        aXlutOffset *=aXlutAdrSize;
    }
	aAlutRound = 1<<( aAlutInShift + aAlutShift - 1 );

#ifdef DEBUG_OUTPUT
        if ( DebugCheck(kThisFile, kDebugReserved1) )
            DebugPrint(" aElutWordSize((1<<aElutWordSize)-0) = %ld\n aAlutInShift:((1<<aXlutWordSize)*aElutWordSize+(adr_bereich_alut/2))/adr_bereich_alut = %ld\n",aElutWordSize,aAlutInShift);
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
			
			        ein_Cache[0]=jj=(*(LH_UINT16 *)inputData[0]);
					ko0 = jj - ( jj >> ( adr_breite_elut ));
					ko = ko0 & ( aElutShiftNum - 1 );
					ko0 = ko0 >> aElutShift;
					ko0 += aElutOffset;
			       	jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
			        jj *= aXlutAdrSize;
			        aElutOffset += adr_bereich_elut;
		        	adr[0] = jj & aElutWordSizeMask;
		        	jj = jj >> aElutWordSize;
		        	adr0 += (jj)*theXlutOffsets[0];
		        	ein_reg[0] = (LH_UINT16)jj;

			        ein_Cache[1]=jj=(*(LH_UINT16 *)inputData[1]);
					ko0 = jj - ( jj >> ( adr_breite_elut ));
					ko = ko0 & ( aElutShiftNum - 1 );
					ko0 = ko0 >> aElutShift;
					ko0 += aElutOffset;
			       	jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
			        jj *= aXlutAdrSize;
			        aElutOffset += adr_bereich_elut;
		        	adr[1] = jj & aElutWordSizeMask;
		        	jj = jj >> aElutWordSize;
		        	adr0 += (jj)*theXlutOffsets[1];
		        	ein_reg[1] = (LH_UINT16)jj;

					ein_Cache[2]=jj=(*(LH_UINT16 *)inputData[2]);
					ko0 = jj - ( jj >> ( adr_breite_elut ));
					ko = ko0 & ( aElutShiftNum - 1 );
					ko0 = ko0 >> aElutShift;
					ko0 += aElutOffset;
			       	jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
			        jj *= aXlutAdrSize;
			        aElutOffset += adr_bereich_elut;
		        	adr[2] = jj & aElutWordSizeMask;
		        	jj = jj >> aElutWordSize;
		        	adr0 += (jj)*theXlutOffsets[2];
		        	ein_reg[2] = (LH_UINT16)jj;

#if NDIM_IN_DIM == 4
					ein_Cache[3]=jj=(*(LH_UINT16 *)inputData[3]);
					ko0 = jj - ( jj >> ( adr_breite_elut ));
					ko = ko0 & ( aElutShiftNum - 1 );
					ko0 = ko0 >> aElutShift;
					ko0 += aElutOffset;
			       	jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
			        jj *= aXlutAdrSize;
			        aElutOffset += adr_bereich_elut;
		        	adr[3] = jj & aElutWordSizeMask;
		        	jj = jj >> aElutWordSize;
		        	adr0 += (jj)*theXlutOffsets[3];
		        	ein_reg[3] = (LH_UINT16)jj;
#endif
		
	

	        {								/* a kind of */
	            register long Hold;
	            
			
				Index[0] = 0;
				Index[1] = 1;
				Index[2] = 2;
#if NDIM_IN_DIM == 4
				Index[3] = 3;
#endif
					if( adr[0] < adr[1] ){
						Hold = Index[0];
						Index[0] = Index[1];
						Index[1] = Hold;
					}

					if( adr[Index[1]] < adr[2] ){
						Hold = Index[1];
						Index[1] = Index[2];
						Index[2] = Hold;
						if( adr[Index[0]] < adr[Index[1]] ){
							Hold = Index[0];
							Index[0] = Index[1];
							Index[1] = Hold;
						}
					}

#if NDIM_IN_DIM == 4
					if( adr[Index[2]] < adr[3] ){
						Hold = Index[2];
						Index[2] = Index[3];
						Index[3] = Hold;
						if( adr[Index[1]] < adr[Index[2]] ){
							Hold = Index[1];
							Index[1] = Index[2];
							Index[2] = Hold;
							if( adr[Index[0]] < adr[Index[1]] ){
								Hold = Index[0];
								Index[0] = Index[1];
								Index[1] = Hold;
							}
						}
					}
#endif
	        }

        accu[0]=0;
        accu[1]=0;
        accu[2]=0;
#if NDIM_OUT_DIM == 4
        accu[3]=0;
#endif
        ko0 = (1<<aElutWordSize);
        adrAdr=adr0;
        adrOffset=0;
        if( aXlutWordSize  == 16 ){
                jj = Index[0];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=Xlut[adrAdr+(0)]*ko;
                    accu[1]+=Xlut[adrAdr+(1)]*ko;
                    accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif

                adrAdr = (adr0 + adrOffset);

                jj = Index[1];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=Xlut[adrAdr+(0)]*ko;
                    accu[1]+=Xlut[adrAdr+(1)]*ko;
                    accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif

                adrAdr = (adr0 + adrOffset);

                jj = Index[2];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=Xlut[adrAdr+(0)]*ko;
                    accu[1]+=Xlut[adrAdr+(1)]*ko;
                    accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif

                adrAdr = (adr0 + adrOffset);

#if NDIM_IN_DIM == 4
                jj = Index[3];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=Xlut[adrAdr+(0)]*ko;
                    accu[1]+=Xlut[adrAdr+(1)]*ko;
                    accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif

                adrAdr = (adr0 + adrOffset);
#endif

                accu[0]+=Xlut[adrAdr+0]*ko0;
                accu[1]+=Xlut[adrAdr+1]*ko0;
                accu[2]+=Xlut[adrAdr+2]*ko0;
 #if NDIM_OUT_DIM == 4
               accu[3]+=Xlut[adrAdr+3]*ko0;
 #endif
       }
       else{

                jj = Index[0];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
                adrAdr = (adr0 + adrOffset);

                jj = Index[1];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
                adrAdr = (adr0 + adrOffset);

                jj = Index[2];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
                adrAdr = (adr0 + adrOffset);

#if NDIM_IN_DIM == 4
                jj = Index[3];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
                adrAdr = (adr0 + adrOffset);
#endif
                accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+0]*ko0;
                accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+1]*ko0;
                accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+2]*ko0;
#if NDIM_OUT_DIM == 4
               accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+3]*ko0;
#endif
        }

		aAlutOffset = 0;

	        if( aOutputPackMode8Bit ){
	        	for( ii=0; ii<NDIM_OUT_DIM; ++ii){
					jj = accu[ii];
					jj = jj + ( jj >> aXlutWordSize );

					ko0 = (jj - ( jj >> ( adr_breite_alut ))) >> aAlutInShiftRemainder; 	/*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/

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
	                aAlutOffset += adr_bereich_alut;
	            }
			}
			else{
				if( aXlutWordSize >= 16 ){
					for( ii=0; ii<NDIM_OUT_DIM; ++ii){
						jj = accu[ii];
						jj = jj + ( jj >> aXlutWordSize );
              			ko0 = (jj - ( jj >> ( adr_breite_alut ))) >> aAlutInShiftRemainder; /*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/
		
						ko = ko0 & (aAlutInShiftNum - 1 );
						ko0 = ko0 >> aAlutInShift;
						ko0 += aAlutOffset;
	 					if( aAlutWordSize <= 8)
		       				jj = ( ((LH_UINT8*)aus_lut)[ ko0 ] * ( aAlutInShiftNum - ko ) + ((LH_UINT8*)aus_lut)[ ko0 +1 ] * ko ) << ( aAlutShift - aAlutInShift );
	 					else
		       				jj = ( aus_lut[ ko0 ] * ( aAlutInShiftNum - ko ) + aus_lut[ ko0 +1 ] * ko ) >> (aAlutInShift- aAlutShift);
						*((LH_UINT16 *)outputData[ii]) =  (LH_UINT16)jj;
						aAlutOffset += adr_bereich_alut;
					}
				}
				else{
					for( ii=0; ii<NDIM_OUT_DIM; ++ii){
						jj = accu[ii];
						jj = jj + ( jj >> aXlutWordSize ) + ( jj >> 2*aXlutWordSize );
              			ko0 = (jj - ( jj >> ( adr_breite_alut ))) ; /*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/
		
						ko = ko0 & (aAlutInShiftNum - 1 );
						ko0 = ko0 >> aAlutInShift;
						ko0 += aAlutOffset;
	 					if( aAlutWordSize <= 8)
		       				jj = ( ((LH_UINT8*)aus_lut)[ ko0 ] * ( aAlutInShiftNum - ko ) + ((LH_UINT8*)aus_lut)[ ko0 +1 ] * ko ) << ( aAlutShift - aAlutInShift );
	 					else
		       				jj = ( aus_lut[ ko0 ] * ( aAlutInShiftNum - ko ) + aus_lut[ ko0 +1 ] * ko ) >> (aAlutInShift- aAlutShift);
						*((LH_UINT16 *)outputData[ii]) =  (LH_UINT16)jj;
						aAlutOffset += adr_bereich_alut;
					}
				}
			}
			while (--i){
			   	for( jj=0; jj<NDIM_IN_DIM; ++jj){
					inputData[jj] += (NDIM_IN_DIM * 2);
				}
			   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
					outputData[jj] += OutputIncrement;
				}
	
				{
					for( jj=0; jj<NDIM_IN_DIM; ++jj){
						if( *((LH_UINT16 *)inputData[jj]) ^ *(LH_UINT16 *)(&ein_Cache[jj]) )break;
					}
				}
				if( jj<NDIM_IN_DIM ) break;
				if( aOutputPackMode8Bit ){
				   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
						*outputData[jj] = outputData[jj][-(long)(NDIM_OUT_DIM )];
					}
				}
				else{
				   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
						*((LH_UINT16 *)outputData[jj]) = *(LH_UINT16 *)(&outputData[jj][-(long)(NDIM_OUT_DIM * 2)]);
					}
				}
			}
		}
		
	   	if( --LineCount ){
		   	for( jj=0; jj<NDIM_IN_DIM; ++jj){
				inputData[jj] += inputDataRowOffset;
			}
		   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
				outputData[jj] += outputDataRowOffset;
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
    aAlutInShift = (aXlutWordSize+bit_breit_selektor-adr_breite_alut);
    /*aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/
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
	                ein_Cache[0]=jj=(*(LH_UINT16 *)inputData[0]);
	                ko0 = jj - ( jj >> ( adr_breite_elut ));
	                ko = ko0 & ( aElutShiftNum - 1 );
	                ko0 = ko0 >> aElutShift;
	                ko0 += aElutOffset;
	                   jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
	                aElutOffset += adr_bereich_elut;
	                adr[0] = (jj & bit_maske_selektor);
	                adr0 |= ((jj & bit_maske_adr)>>bit_breit_selektor)<<((NDIM_IN_DIM-0-1)*bit_breit_adr);
	                ein_reg[0] = (LH_UINT16)jj;

	                ein_Cache[1]=jj=(*(LH_UINT16 *)inputData[1]);
	                ko0 = jj - ( jj >> ( adr_breite_elut ));
	                ko = ko0 & ( aElutShiftNum - 1 );
	                ko0 = ko0 >> aElutShift;
	                ko0 += aElutOffset;
	                   jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
	                aElutOffset += adr_bereich_elut;
	                adr[1] = (jj & bit_maske_selektor);
	                adr0 |= ((jj & bit_maske_adr)>>bit_breit_selektor)<<((NDIM_IN_DIM-1-1)*bit_breit_adr);
	                ein_reg[1] = (LH_UINT16)jj;

	                ein_Cache[2]=jj=(*(LH_UINT16 *)inputData[2]);
	                ko0 = jj - ( jj >> ( adr_breite_elut ));
	                ko = ko0 & ( aElutShiftNum - 1 );
	                ko0 = ko0 >> aElutShift;
	                ko0 += aElutOffset;
	                   jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
	                aElutOffset += adr_bereich_elut;
	                adr[2] = (jj & bit_maske_selektor);
	                adr0 |= ((jj & bit_maske_adr)>>bit_breit_selektor)<<((NDIM_IN_DIM-2-1)*bit_breit_adr);
	                ein_reg[2] = (LH_UINT16)jj;

#if NDIM_IN_DIM == 4
	                ein_Cache[3]=jj=(*(LH_UINT16 *)inputData[3]);
	                ko0 = jj - ( jj >> ( adr_breite_elut ));
	                ko = ko0 & ( aElutShiftNum - 1 );
	                ko0 = ko0 >> aElutShift;
	                ko0 += aElutOffset;
	                   jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
	                aElutOffset += adr_bereich_elut;
	                adr[3] = (jj & bit_maske_selektor);
	                adr0 |= ((jj & bit_maske_adr)>>bit_breit_selektor)<<((NDIM_IN_DIM-3-1)*bit_breit_adr);
	                ein_reg[3] = (LH_UINT16)jj;
#endif	
	        adr0 *= NDIM_OUT_DIM;
	
	        {								/* a kind of */
	            register long Hold;
	            
			
				Index[0] = 0;
				Index[1] = 1;
				Index[2] = 2;
#if NDIM_IN_DIM == 4
				Index[3] = 3;
#endif
					if( adr[0] < adr[1] ){
						Hold = Index[0];
						Index[0] = Index[1];
						Index[1] = Hold;
					}

					if( adr[Index[1]] < adr[2] ){
						Hold = Index[1];
						Index[1] = Index[2];
						Index[2] = Hold;
						if( adr[Index[0]] < adr[Index[1]] ){
							Hold = Index[0];
							Index[0] = Index[1];
							Index[1] = Hold;
						}
					}

#if NDIM_IN_DIM == 4
					if( adr[Index[2]] < adr[3] ){
						Hold = Index[2];
						Index[2] = Index[3];
						Index[3] = Hold;
						if( adr[Index[1]] < adr[Index[2]] ){
							Hold = Index[1];
							Index[1] = Index[2];
							Index[2] = Hold;
							if( adr[Index[0]] < adr[Index[1]] ){
								Hold = Index[0];
								Index[0] = Index[1];
								Index[1] = Hold;
							}
						}
					}
#endif
	        }
	
			accu[0]=0;
			accu[1]=0;
			accu[2]=0;
#if NDIM_OUT_DIM == 4
			accu[3]=0;
#endif
	
	        ko0 = bit_maske_selektor+1;
	        adrAdr=adr0;
	        adrOffset=0;
	
	        if( aXlutWordSize  == 16 ){
	                jj = Index[0];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
						accu[0]+=Xlut[adrAdr+(0)]*ko;
						accu[1]+=Xlut[adrAdr+(1)]*ko;
						accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
						accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

	                jj = Index[1];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
						accu[0]+=Xlut[adrAdr+(0)]*ko;
						accu[1]+=Xlut[adrAdr+(1)]*ko;
						accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
						accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

	                jj = Index[2];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
						accu[0]+=Xlut[adrAdr+(0)]*ko;
						accu[1]+=Xlut[adrAdr+(1)]*ko;
						accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
						accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

#if NDIM_IN_DIM == 4
	                jj = Index[3];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
						accu[0]+=Xlut[adrAdr+(0)]*ko;
						accu[1]+=Xlut[adrAdr+(1)]*ko;
						accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
						accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);
#endif
                accu[0]+=Xlut[adrAdr+0]*ko0;
                accu[1]+=Xlut[adrAdr+1]*ko0;
                accu[2]+=Xlut[adrAdr+2]*ko0;
#if NDIM_OUT_DIM == 4
				accu[3]+=Xlut[adrAdr+3]*ko0;
#endif
	        }
	        else{
	                jj = Index[0];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
	                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
	                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
	                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
	                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
	
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

	                jj = Index[1];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
	                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
	                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
	                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
	                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
	
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

	                jj = Index[2];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
	                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
	                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
	                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
	                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
	
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

#if NDIM_IN_DIM == 4
	                jj = Index[3];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
	                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
	                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
	                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
	                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
	
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);
#endif
				accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+0]*ko0;
                accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+1]*ko0;
                accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+2]*ko0;
#if NDIM_OUT_DIM == 4
				accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+3]*ko0;
#endif
	        }
	
	        aAlutOffset = 0;
	
	        if( aOutputPackMode8Bit ){
	        	for( ii=0; ii<NDIM_OUT_DIM; ++ii){
					jj = accu[ii];
					jj = jj + ( jj >> aXlutWordSize );

					ko0 = (jj - ( jj >> ( adr_breite_alut ))) >> aAlutInShiftRemainder; 	/*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/

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
	                aAlutOffset += adr_bereich_alut;
	            }
	        }
	        else{
				if( aXlutWordSize >= 16 ){
					for( ii=0; ii<NDIM_OUT_DIM; ++ii){
						jj = accu[ii];
						jj = jj + ( jj >> aXlutWordSize );
              			ko0 = (jj - ( jj >> ( adr_breite_alut ))) >> aAlutInShiftRemainder; /*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/
		
						ko = ko0 & (aAlutInShiftNum - 1 );
						ko0 = ko0 >> aAlutInShift;
						ko0 += aAlutOffset;
	 					if( aAlutWordSize <= 8)
		       				jj = ( ((LH_UINT8*)aus_lut)[ ko0 ] * ( aAlutInShiftNum - ko ) + ((LH_UINT8*)aus_lut)[ ko0 +1 ] * ko ) << ( aAlutShift - aAlutInShift );
	 					else
		       				jj = ( aus_lut[ ko0 ] * ( aAlutInShiftNum - ko ) + aus_lut[ ko0 +1 ] * ko ) >> (aAlutInShift- aAlutShift);
						*((LH_UINT16 *)outputData[ii]) =  (LH_UINT16)jj;
						aAlutOffset += adr_bereich_alut;
					}
				}
				else{
					for( ii=0; ii<NDIM_OUT_DIM; ++ii){
						jj = accu[ii];
						jj = jj + ( jj >> aXlutWordSize ) + ( jj >> 2*aXlutWordSize );
              			ko0 = (jj - ( jj >> ( adr_breite_alut ))); /*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/
		
						ko = ko0 & (aAlutInShiftNum - 1 );
						ko0 = ko0 >> aAlutInShift;
						ko0 += aAlutOffset;
	 					if( aAlutWordSize <= 8)
		       				jj = ( ((LH_UINT8*)aus_lut)[ ko0 ] * ( aAlutInShiftNum - ko ) + ((LH_UINT8*)aus_lut)[ ko0 +1 ] * ko ) << ( aAlutShift - aAlutInShift );
	 					else
		       				jj = ( aus_lut[ ko0 ] * ( aAlutInShiftNum - ko ) + aus_lut[ ko0 +1 ] * ko ) >> (aAlutInShift- aAlutShift);
						*((LH_UINT16 *)outputData[ii]) =  (LH_UINT16)jj;
						aAlutOffset += adr_bereich_alut;
					}
				}
	        }
	
			while (--i){
			   	for( jj=0; jj<NDIM_IN_DIM; ++jj){
					inputData[jj] += (NDIM_IN_DIM * 2);
				}
			   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
					outputData[jj] += OutputIncrement;
				}
	
				{
				   	for( jj=0; jj<NDIM_IN_DIM; ++jj){
						if( *((LH_UINT16 *)inputData[jj]) ^ *(LH_UINT16 *)(&ein_Cache[jj]) )break;
					}
				}
				if( jj<NDIM_IN_DIM ) break;
				if( aOutputPackMode8Bit ){
				   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
						*outputData[jj] = outputData[jj][-(long)(NDIM_OUT_DIM )];
					}
				}
				else{
				   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
						*((LH_UINT16 *)outputData[jj]) = *(LH_UINT16 *)(&outputData[jj][-(long)(NDIM_OUT_DIM * 2)]);
					}
				}
			}
		}
		
	   	if( --LineCount ){
		   	for( jj=0; jj<NDIM_IN_DIM; ++jj){
				inputData[jj] += inputDataRowOffset;
			}
		   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
				outputData[jj] += outputDataRowOffset;
			}
		}
    }
    }

	/* UNLOCK_DATA( aElutHdle ); */
	/* UNLOCK_DATA( aAlutHdle ); */
	/* UNLOCK_DATA( aXlutHdle ); */

	LH_END_PROC("Calc323Dim_Data8To8_Lut16")
	return noErr;
}

#undef NDIM_IN_DIM
#undef NDIM_OUT_DIM
#undef aElutShift
#undef aElutShiftNum
#define NDIM_IN_DIM 3
#define NDIM_OUT_DIM 4
#define aElutShift (16-adr_breite_elut)
#define aElutShiftNum (1<<aElutShift)

CMError Calc324Dim_Data8To8_Lut16 	(CMCalcParamPtr calcParam,
									 CMLutParamPtr  lutParam)
	
{
	LH_UINT8 * inputData[8], *outputData[8];
	UINT32 OutputIncrement, inputDataRowOffset, outputDataRowOffset, Pixelcount, LineCount;
   	register unsigned long adr0;
    register unsigned long ko0;
    unsigned long accu[8];
    register long i;
    /*long Offsets[8];*/
    
    register long aAlutShift,aElutOffset,aAlutOffset;
    register long aElutWordSize;
    register long aAlutWordSize;
    register long aXlutAdrSize;
    register long aXlutAdrShift;
    register unsigned long aXlutWordSize;
	register unsigned long ii,jj;
    register long aOutputPackMode8Bit;
	long ein_Cache[8];
	
	LH_UINT16 * aus_lut	= (LH_UINT16*)lutParam->outputLut;
	LH_UINT16 * ein_lut	= (LH_UINT16*)lutParam->inputLut;
	LH_UINT16 * Xlut 	= (LH_UINT16*)lutParam->colorLut;
	
	#ifdef DEBUG_OUTPUT
	long err = noErr;
	#endif
	LH_START_PROC("Calc324Dim_Data8To8_Lut16")
	
	inputData[0] = (LH_UINT8 *)calcParam->inputData[0];
	inputData[1] = (LH_UINT8 *)calcParam->inputData[1];
	inputData[2] = (LH_UINT8 *)calcParam->inputData[2];
#if NDIM_IN_DIM == 4
	inputData[3] = (LH_UINT8 *)calcParam->inputData[3];
#endif
	outputData[0] = (LH_UINT8 *)calcParam->outputData[0];
	outputData[1] = (LH_UINT8 *)calcParam->outputData[1];
	outputData[2] = (LH_UINT8 *)calcParam->outputData[2];
#if NDIM_OUT_DIM == 4
	outputData[3] = (LH_UINT8 *)calcParam->outputData[3];
#endif

	OutputIncrement = calcParam->cmOutputPixelOffset;
	inputDataRowOffset = calcParam->cmInputBytesPerLine - calcParam->cmPixelPerLine * calcParam->cmInputPixelOffset + (NDIM_IN_DIM * 2);
	outputDataRowOffset = calcParam->cmOutputBytesPerLine - calcParam->cmPixelPerLine * calcParam->cmOutputPixelOffset + OutputIncrement;

	Pixelcount = calcParam->cmPixelPerLine;
	LineCount = calcParam->cmLineCount;

	aElutWordSize = lutParam->inputLutWordSize;
	aAlutWordSize = lutParam->outputLutWordSize;
	aXlutAdrSize = lutParam->colorLutGridPoints;
	for ( i = 1; (i < 32) && (aXlutAdrSize >> i); i++)
		aXlutAdrShift = i;
	aXlutWordSize = lutParam->colorLutWordSize;

 	aOutputPackMode8Bit = calcParam->cmOutputColorSpace & cm8PerChannelPacking || calcParam->cmOutputColorSpace & cmLong8ColorPacking;
   /*DebugPrint("DoNDim with %d input elements\n",aByteCount);*/
	#if FARBR_FILES
	WriteLuts( 	"DoNDim",1,adr_bereich_elut,aElutWordSize,ein_lut,
				NDIM_IN_DIM,NDIM_OUT_DIM,aXlutAdrSize,aXlutWordSize,(LH_UINT16 *)Xlut,adr_bereich_alut,aAlutWordSize,(LH_UINT16 *)aus_lut);
    #endif

	i=0;
					
	{
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
						adr_bereich_elut,adr_breite_elut,aElutWordSize,ein_lut);
			DebugPrint("aAlutAdrSize=%lx,aAlutAdrShift=%lx,aAlutWordSize=%lx,aus_lut=%lx,\n",
						adr_bereich_alut,adr_breite_alut,aAlutWordSize,aus_lut);
			DebugPrint("aXlutInDim=%lx,aXlutOutDim=%lx,aXlutAdrSize=%lx,aXlutAdrShift=%lx,aXlutWordSize=%lx,Xlut=%lx,\n",
						NDIM_IN_DIM,NDIM_OUT_DIM,aXlutAdrSize,aXlutAdrShift,aXlutWordSize,Xlut);
		}
    #endif
    
    /*if( 1 )*/
    if( aXlutAdrSize != (1<<aXlutAdrShift )){
    register long aXlutOffset;
    long theXlutOffsets[8]; 
    register unsigned long aAlutInShift;
    register long aAlutInShiftRemainder;
    register unsigned long aAlutInShiftNum;
    register long aElutWordSizeMask = (1<<aElutWordSize) - 1;
    register unsigned long aAlutRound;
   aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;
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
    aXlutOffset =NDIM_OUT_DIM;
    for( i=0; i<(long)NDIM_IN_DIM; i++){
        theXlutOffsets[ NDIM_IN_DIM-1-i] = aXlutOffset;
        aXlutOffset *=aXlutAdrSize;
    }
	aAlutRound = 1<<( aAlutInShift + aAlutShift - 1 );

#ifdef DEBUG_OUTPUT
        if ( DebugCheck(kThisFile, kDebugReserved1) )
            DebugPrint(" aElutWordSize((1<<aElutWordSize)-0) = %ld\n aAlutInShift:((1<<aXlutWordSize)*aElutWordSize+(adr_bereich_alut/2))/adr_bereich_alut = %ld\n",aElutWordSize,aAlutInShift);
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
			
			        ein_Cache[0]=jj=(*(LH_UINT16 *)inputData[0]);
					ko0 = jj - ( jj >> ( adr_breite_elut ));
					ko = ko0 & ( aElutShiftNum - 1 );
					ko0 = ko0 >> aElutShift;
					ko0 += aElutOffset;
			       	jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
			        jj *= aXlutAdrSize;
			        aElutOffset += adr_bereich_elut;
		        	adr[0] = jj & aElutWordSizeMask;
		        	jj = jj >> aElutWordSize;
		        	adr0 += (jj)*theXlutOffsets[0];
		        	ein_reg[0] = (LH_UINT16)jj;

			        ein_Cache[1]=jj=(*(LH_UINT16 *)inputData[1]);
					ko0 = jj - ( jj >> ( adr_breite_elut ));
					ko = ko0 & ( aElutShiftNum - 1 );
					ko0 = ko0 >> aElutShift;
					ko0 += aElutOffset;
			       	jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
			        jj *= aXlutAdrSize;
			        aElutOffset += adr_bereich_elut;
		        	adr[1] = jj & aElutWordSizeMask;
		        	jj = jj >> aElutWordSize;
		        	adr0 += (jj)*theXlutOffsets[1];
		        	ein_reg[1] = (LH_UINT16)jj;

					ein_Cache[2]=jj=(*(LH_UINT16 *)inputData[2]);
					ko0 = jj - ( jj >> ( adr_breite_elut ));
					ko = ko0 & ( aElutShiftNum - 1 );
					ko0 = ko0 >> aElutShift;
					ko0 += aElutOffset;
			       	jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
			        jj *= aXlutAdrSize;
			        aElutOffset += adr_bereich_elut;
		        	adr[2] = jj & aElutWordSizeMask;
		        	jj = jj >> aElutWordSize;
		        	adr0 += (jj)*theXlutOffsets[2];
		        	ein_reg[2] = (LH_UINT16)jj;

#if NDIM_IN_DIM == 4
					ein_Cache[3]=jj=(*(LH_UINT16 *)inputData[3]);
					ko0 = jj - ( jj >> ( adr_breite_elut ));
					ko = ko0 & ( aElutShiftNum - 1 );
					ko0 = ko0 >> aElutShift;
					ko0 += aElutOffset;
			       	jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
			        jj *= aXlutAdrSize;
			        aElutOffset += adr_bereich_elut;
		        	adr[3] = jj & aElutWordSizeMask;
		        	jj = jj >> aElutWordSize;
		        	adr0 += (jj)*theXlutOffsets[3];
		        	ein_reg[3] = (LH_UINT16)jj;
#endif
		
	

	        {								/* a kind of */
	            register long Hold;
	            
			
				Index[0] = 0;
				Index[1] = 1;
				Index[2] = 2;
#if NDIM_IN_DIM == 4
				Index[3] = 3;
#endif
					if( adr[0] < adr[1] ){
						Hold = Index[0];
						Index[0] = Index[1];
						Index[1] = Hold;
					}

					if( adr[Index[1]] < adr[2] ){
						Hold = Index[1];
						Index[1] = Index[2];
						Index[2] = Hold;
						if( adr[Index[0]] < adr[Index[1]] ){
							Hold = Index[0];
							Index[0] = Index[1];
							Index[1] = Hold;
						}
					}

#if NDIM_IN_DIM == 4
					if( adr[Index[2]] < adr[3] ){
						Hold = Index[2];
						Index[2] = Index[3];
						Index[3] = Hold;
						if( adr[Index[1]] < adr[Index[2]] ){
							Hold = Index[1];
							Index[1] = Index[2];
							Index[2] = Hold;
							if( adr[Index[0]] < adr[Index[1]] ){
								Hold = Index[0];
								Index[0] = Index[1];
								Index[1] = Hold;
							}
						}
					}
#endif
	        }

        accu[0]=0;
        accu[1]=0;
        accu[2]=0;
#if NDIM_OUT_DIM == 4
        accu[3]=0;
#endif
        ko0 = (1<<aElutWordSize);
        adrAdr=adr0;
        adrOffset=0;
        if( aXlutWordSize  == 16 ){
                jj = Index[0];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=Xlut[adrAdr+(0)]*ko;
                    accu[1]+=Xlut[adrAdr+(1)]*ko;
                    accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif

                adrAdr = (adr0 + adrOffset);

                jj = Index[1];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=Xlut[adrAdr+(0)]*ko;
                    accu[1]+=Xlut[adrAdr+(1)]*ko;
                    accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif

                adrAdr = (adr0 + adrOffset);

                jj = Index[2];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=Xlut[adrAdr+(0)]*ko;
                    accu[1]+=Xlut[adrAdr+(1)]*ko;
                    accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif

                adrAdr = (adr0 + adrOffset);

#if NDIM_IN_DIM == 4
                jj = Index[3];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=Xlut[adrAdr+(0)]*ko;
                    accu[1]+=Xlut[adrAdr+(1)]*ko;
                    accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif

                adrAdr = (adr0 + adrOffset);
#endif

                accu[0]+=Xlut[adrAdr+0]*ko0;
                accu[1]+=Xlut[adrAdr+1]*ko0;
                accu[2]+=Xlut[adrAdr+2]*ko0;
 #if NDIM_OUT_DIM == 4
               accu[3]+=Xlut[adrAdr+3]*ko0;
 #endif
       }
       else{

                jj = Index[0];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
                adrAdr = (adr0 + adrOffset);

                jj = Index[1];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
                adrAdr = (adr0 + adrOffset);

                jj = Index[2];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
                adrAdr = (adr0 + adrOffset);

#if NDIM_IN_DIM == 4
                jj = Index[3];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
                adrAdr = (adr0 + adrOffset);
#endif
                accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+0]*ko0;
                accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+1]*ko0;
                accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+2]*ko0;
#if NDIM_OUT_DIM == 4
               accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+3]*ko0;
#endif
        }

		aAlutOffset = 0;

	        if( aOutputPackMode8Bit ){
	        	for( ii=0; ii<NDIM_OUT_DIM; ++ii){
					jj = accu[ii];
					jj = jj + ( jj >> aXlutWordSize );

					ko0 = (jj - ( jj >> ( adr_breite_alut ))) >> aAlutInShiftRemainder; 	/*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/

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
	                aAlutOffset += adr_bereich_alut;
	            }
			}
			else{
				if( aXlutWordSize >= 16 ){
					for( ii=0; ii<NDIM_OUT_DIM; ++ii){
						jj = accu[ii];
						jj = jj + ( jj >> aXlutWordSize );
              			ko0 = (jj - ( jj >> ( adr_breite_alut ))) >> aAlutInShiftRemainder; /*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/
		
						ko = ko0 & (aAlutInShiftNum - 1 );
						ko0 = ko0 >> aAlutInShift;
						ko0 += aAlutOffset;
	 					if( aAlutWordSize <= 8)
		       				jj = ( ((LH_UINT8*)aus_lut)[ ko0 ] * ( aAlutInShiftNum - ko ) + ((LH_UINT8*)aus_lut)[ ko0 +1 ] * ko ) << ( aAlutShift - aAlutInShift );
	 					else
		       				jj = ( aus_lut[ ko0 ] * ( aAlutInShiftNum - ko ) + aus_lut[ ko0 +1 ] * ko ) >> (aAlutInShift- aAlutShift);
						*((LH_UINT16 *)outputData[ii]) =  (LH_UINT16)jj;
						aAlutOffset += adr_bereich_alut;
					}
				}
				else{
					for( ii=0; ii<NDIM_OUT_DIM; ++ii){
						jj = accu[ii];
						jj = jj + ( jj >> aXlutWordSize ) + ( jj >> 2*aXlutWordSize );
              			ko0 = (jj - ( jj >> ( adr_breite_alut ))) ; /*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/
		
						ko = ko0 & (aAlutInShiftNum - 1 );
						ko0 = ko0 >> aAlutInShift;
						ko0 += aAlutOffset;
	 					if( aAlutWordSize <= 8)
		       				jj = ( ((LH_UINT8*)aus_lut)[ ko0 ] * ( aAlutInShiftNum - ko ) + ((LH_UINT8*)aus_lut)[ ko0 +1 ] * ko ) << ( aAlutShift - aAlutInShift );
	 					else
		       				jj = ( aus_lut[ ko0 ] * ( aAlutInShiftNum - ko ) + aus_lut[ ko0 +1 ] * ko ) >> (aAlutInShift- aAlutShift);
						*((LH_UINT16 *)outputData[ii]) =  (LH_UINT16)jj;
						aAlutOffset += adr_bereich_alut;
					}
				}
			}
			while (--i){
			   	for( jj=0; jj<NDIM_IN_DIM; ++jj){
					inputData[jj] += (NDIM_IN_DIM * 2);
				}
			   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
					outputData[jj] += OutputIncrement;
				}
	
				{
					for( jj=0; jj<NDIM_IN_DIM; ++jj){
						if( *((LH_UINT16 *)inputData[jj]) ^ *(LH_UINT16 *)(&ein_Cache[jj]) )break;
					}
				}
				if( jj<NDIM_IN_DIM ) break;
				if( aOutputPackMode8Bit ){
				   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
						*outputData[jj] = outputData[jj][-(long)(NDIM_OUT_DIM )];
					}
				}
				else{
				   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
						*((LH_UINT16 *)outputData[jj]) = *(LH_UINT16 *)(&outputData[jj][-(long)(NDIM_OUT_DIM * 2)]);
					}
				}
			}
		}
		
	   	if( --LineCount ){
		   	for( jj=0; jj<NDIM_IN_DIM; ++jj){
				inputData[jj] += inputDataRowOffset;
			}
		   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
				outputData[jj] += outputDataRowOffset;
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
    aAlutInShift = (aXlutWordSize+bit_breit_selektor-adr_breite_alut);
    /*aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/
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
	                ein_Cache[0]=jj=(*(LH_UINT16 *)inputData[0]);
	                ko0 = jj - ( jj >> ( adr_breite_elut ));
	                ko = ko0 & ( aElutShiftNum - 1 );
	                ko0 = ko0 >> aElutShift;
	                ko0 += aElutOffset;
	                   jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
	                aElutOffset += adr_bereich_elut;
	                adr[0] = (jj & bit_maske_selektor);
	                adr0 |= ((jj & bit_maske_adr)>>bit_breit_selektor)<<((NDIM_IN_DIM-0-1)*bit_breit_adr);
	                ein_reg[0] = (LH_UINT16)jj;

	                ein_Cache[1]=jj=(*(LH_UINT16 *)inputData[1]);
	                ko0 = jj - ( jj >> ( adr_breite_elut ));
	                ko = ko0 & ( aElutShiftNum - 1 );
	                ko0 = ko0 >> aElutShift;
	                ko0 += aElutOffset;
	                   jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
	                aElutOffset += adr_bereich_elut;
	                adr[1] = (jj & bit_maske_selektor);
	                adr0 |= ((jj & bit_maske_adr)>>bit_breit_selektor)<<((NDIM_IN_DIM-1-1)*bit_breit_adr);
	                ein_reg[1] = (LH_UINT16)jj;

	                ein_Cache[2]=jj=(*(LH_UINT16 *)inputData[2]);
	                ko0 = jj - ( jj >> ( adr_breite_elut ));
	                ko = ko0 & ( aElutShiftNum - 1 );
	                ko0 = ko0 >> aElutShift;
	                ko0 += aElutOffset;
	                   jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
	                aElutOffset += adr_bereich_elut;
	                adr[2] = (jj & bit_maske_selektor);
	                adr0 |= ((jj & bit_maske_adr)>>bit_breit_selektor)<<((NDIM_IN_DIM-2-1)*bit_breit_adr);
	                ein_reg[2] = (LH_UINT16)jj;

#if NDIM_IN_DIM == 4
	                ein_Cache[3]=jj=(*(LH_UINT16 *)inputData[3]);
	                ko0 = jj - ( jj >> ( adr_breite_elut ));
	                ko = ko0 & ( aElutShiftNum - 1 );
	                ko0 = ko0 >> aElutShift;
	                ko0 += aElutOffset;
	                   jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
	                aElutOffset += adr_bereich_elut;
	                adr[3] = (jj & bit_maske_selektor);
	                adr0 |= ((jj & bit_maske_adr)>>bit_breit_selektor)<<((NDIM_IN_DIM-3-1)*bit_breit_adr);
	                ein_reg[3] = (LH_UINT16)jj;
#endif	
	        adr0 *= NDIM_OUT_DIM;
	
	        {								/* a kind of */
	            register long Hold;
	            
			
				Index[0] = 0;
				Index[1] = 1;
				Index[2] = 2;
#if NDIM_IN_DIM == 4
				Index[3] = 3;
#endif
					if( adr[0] < adr[1] ){
						Hold = Index[0];
						Index[0] = Index[1];
						Index[1] = Hold;
					}

					if( adr[Index[1]] < adr[2] ){
						Hold = Index[1];
						Index[1] = Index[2];
						Index[2] = Hold;
						if( adr[Index[0]] < adr[Index[1]] ){
							Hold = Index[0];
							Index[0] = Index[1];
							Index[1] = Hold;
						}
					}

#if NDIM_IN_DIM == 4
					if( adr[Index[2]] < adr[3] ){
						Hold = Index[2];
						Index[2] = Index[3];
						Index[3] = Hold;
						if( adr[Index[1]] < adr[Index[2]] ){
							Hold = Index[1];
							Index[1] = Index[2];
							Index[2] = Hold;
							if( adr[Index[0]] < adr[Index[1]] ){
								Hold = Index[0];
								Index[0] = Index[1];
								Index[1] = Hold;
							}
						}
					}
#endif
	        }
	
			accu[0]=0;
			accu[1]=0;
			accu[2]=0;
#if NDIM_OUT_DIM == 4
			accu[3]=0;
#endif
	
	        ko0 = bit_maske_selektor+1;
	        adrAdr=adr0;
	        adrOffset=0;
	
	        if( aXlutWordSize  == 16 ){
	                jj = Index[0];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
						accu[0]+=Xlut[adrAdr+(0)]*ko;
						accu[1]+=Xlut[adrAdr+(1)]*ko;
						accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
						accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

	                jj = Index[1];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
						accu[0]+=Xlut[adrAdr+(0)]*ko;
						accu[1]+=Xlut[adrAdr+(1)]*ko;
						accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
						accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

	                jj = Index[2];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
						accu[0]+=Xlut[adrAdr+(0)]*ko;
						accu[1]+=Xlut[adrAdr+(1)]*ko;
						accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
						accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

#if NDIM_IN_DIM == 4
	                jj = Index[3];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
						accu[0]+=Xlut[adrAdr+(0)]*ko;
						accu[1]+=Xlut[adrAdr+(1)]*ko;
						accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
						accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);
#endif
                accu[0]+=Xlut[adrAdr+0]*ko0;
                accu[1]+=Xlut[adrAdr+1]*ko0;
                accu[2]+=Xlut[adrAdr+2]*ko0;
#if NDIM_OUT_DIM == 4
				accu[3]+=Xlut[adrAdr+3]*ko0;
#endif
	        }
	        else{
	                jj = Index[0];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
	                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
	                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
	                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
	                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
	
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

	                jj = Index[1];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
	                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
	                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
	                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
	                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
	
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

	                jj = Index[2];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
	                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
	                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
	                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
	                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
	
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

#if NDIM_IN_DIM == 4
	                jj = Index[3];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
	                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
	                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
	                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
	                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
	
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);
#endif
				accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+0]*ko0;
                accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+1]*ko0;
                accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+2]*ko0;
#if NDIM_OUT_DIM == 4
				accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+3]*ko0;
#endif
	        }
	
	        aAlutOffset = 0;
	
	        if( aOutputPackMode8Bit ){
	        	for( ii=0; ii<NDIM_OUT_DIM; ++ii){
					jj = accu[ii];
					jj = jj + ( jj >> aXlutWordSize );

					ko0 = (jj - ( jj >> ( adr_breite_alut ))) >> aAlutInShiftRemainder; 	/*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/

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
	                aAlutOffset += adr_bereich_alut;
	            }
	        }
	        else{
				if( aXlutWordSize >= 16 ){
					for( ii=0; ii<NDIM_OUT_DIM; ++ii){
						jj = accu[ii];
						jj = jj + ( jj >> aXlutWordSize );
              			ko0 = (jj - ( jj >> ( adr_breite_alut ))) >> aAlutInShiftRemainder; /*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/
		
						ko = ko0 & (aAlutInShiftNum - 1 );
						ko0 = ko0 >> aAlutInShift;
						ko0 += aAlutOffset;
	 					if( aAlutWordSize <= 8)
		       				jj = ( ((LH_UINT8*)aus_lut)[ ko0 ] * ( aAlutInShiftNum - ko ) + ((LH_UINT8*)aus_lut)[ ko0 +1 ] * ko ) << ( aAlutShift - aAlutInShift );
	 					else
		       				jj = ( aus_lut[ ko0 ] * ( aAlutInShiftNum - ko ) + aus_lut[ ko0 +1 ] * ko ) >> (aAlutInShift- aAlutShift);
						*((LH_UINT16 *)outputData[ii]) =  (LH_UINT16)jj;
						aAlutOffset += adr_bereich_alut;
					}
				}
				else{
					for( ii=0; ii<NDIM_OUT_DIM; ++ii){
						jj = accu[ii];
						jj = jj + ( jj >> aXlutWordSize ) + ( jj >> 2*aXlutWordSize );
              			ko0 = (jj - ( jj >> ( adr_breite_alut ))); /*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/
		
						ko = ko0 & (aAlutInShiftNum - 1 );
						ko0 = ko0 >> aAlutInShift;
						ko0 += aAlutOffset;
	 					if( aAlutWordSize <= 8)
		       				jj = ( ((LH_UINT8*)aus_lut)[ ko0 ] * ( aAlutInShiftNum - ko ) + ((LH_UINT8*)aus_lut)[ ko0 +1 ] * ko ) << ( aAlutShift - aAlutInShift );
	 					else
		       				jj = ( aus_lut[ ko0 ] * ( aAlutInShiftNum - ko ) + aus_lut[ ko0 +1 ] * ko ) >> (aAlutInShift- aAlutShift);
						*((LH_UINT16 *)outputData[ii]) =  (LH_UINT16)jj;
						aAlutOffset += adr_bereich_alut;
					}
				}
	        }
	
			while (--i){
			   	for( jj=0; jj<NDIM_IN_DIM; ++jj){
					inputData[jj] += (NDIM_IN_DIM * 2);
				}
			   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
					outputData[jj] += OutputIncrement;
				}
	
				{
				   	for( jj=0; jj<NDIM_IN_DIM; ++jj){
						if( *((LH_UINT16 *)inputData[jj]) ^ *(LH_UINT16 *)(&ein_Cache[jj]) )break;
					}
				}
				if( jj<NDIM_IN_DIM ) break;
				if( aOutputPackMode8Bit ){
				   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
						*outputData[jj] = outputData[jj][-(long)(NDIM_OUT_DIM )];
					}
				}
				else{
				   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
						*((LH_UINT16 *)outputData[jj]) = *(LH_UINT16 *)(&outputData[jj][-(long)(NDIM_OUT_DIM * 2)]);
					}
				}
			}
		}
		
	   	if( --LineCount ){
		   	for( jj=0; jj<NDIM_IN_DIM; ++jj){
				inputData[jj] += inputDataRowOffset;
			}
		   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
				outputData[jj] += outputDataRowOffset;
			}
		}
    }
    }

	/* UNLOCK_DATA( aElutHdle ); */
	/* UNLOCK_DATA( aAlutHdle ); */
	/* UNLOCK_DATA( aXlutHdle ); */

	LH_END_PROC("Calc324Dim_Data8To8_Lut16")
	return noErr;
}

#undef NDIM_IN_DIM
#undef NDIM_OUT_DIM
#undef aElutShift
#undef aElutShiftNum
#define NDIM_IN_DIM 4
#define NDIM_OUT_DIM 3
#define aElutShift (16-adr_breite_elut)
#define aElutShiftNum (1<<aElutShift)

CMError Calc423Dim_Data8To8_Lut16 	(CMCalcParamPtr calcParam,
									 CMLutParamPtr  lutParam)
	
{
	LH_UINT8 * inputData[8], *outputData[8];
	UINT32 OutputIncrement, inputDataRowOffset, outputDataRowOffset, Pixelcount, LineCount;
   	register unsigned long adr0;
    register unsigned long ko0;
    unsigned long accu[8];
    register long i;
    /*long Offsets[8];*/
    
    register long aAlutShift,aElutOffset,aAlutOffset;
    register long aElutWordSize;
    register long aAlutWordSize;
    register long aXlutAdrSize;
    register long aXlutAdrShift;
    register unsigned long aXlutWordSize;
	register unsigned long ii,jj;
    register long aOutputPackMode8Bit;
	long ein_Cache[8];
	
	LH_UINT16 * aus_lut	= (LH_UINT16*)lutParam->outputLut;
	LH_UINT16 * ein_lut	= (LH_UINT16*)lutParam->inputLut;
	LH_UINT16 * Xlut 	= (LH_UINT16*)lutParam->colorLut;
	
	#ifdef DEBUG_OUTPUT
	long err = noErr;
	#endif
	LH_START_PROC("Calc423Dim_Data8To8_Lut16")
	
	inputData[0] = (LH_UINT8 *)calcParam->inputData[0];
	inputData[1] = (LH_UINT8 *)calcParam->inputData[1];
	inputData[2] = (LH_UINT8 *)calcParam->inputData[2];
#if NDIM_IN_DIM == 4
	inputData[3] = (LH_UINT8 *)calcParam->inputData[3];
#endif
	outputData[0] = (LH_UINT8 *)calcParam->outputData[0];
	outputData[1] = (LH_UINT8 *)calcParam->outputData[1];
	outputData[2] = (LH_UINT8 *)calcParam->outputData[2];
#if NDIM_OUT_DIM == 4
	outputData[3] = (LH_UINT8 *)calcParam->outputData[3];
#endif

	OutputIncrement = calcParam->cmOutputPixelOffset;
	inputDataRowOffset = calcParam->cmInputBytesPerLine - calcParam->cmPixelPerLine * calcParam->cmInputPixelOffset + (NDIM_IN_DIM * 2);
	outputDataRowOffset = calcParam->cmOutputBytesPerLine - calcParam->cmPixelPerLine * calcParam->cmOutputPixelOffset + OutputIncrement;

	Pixelcount = calcParam->cmPixelPerLine;
	LineCount = calcParam->cmLineCount;

	aElutWordSize = lutParam->inputLutWordSize;
	aAlutWordSize = lutParam->outputLutWordSize;
	aXlutAdrSize = lutParam->colorLutGridPoints;
	for ( i = 1; (i < 32) && (aXlutAdrSize >> i); i++)
		aXlutAdrShift = i;
	aXlutWordSize = lutParam->colorLutWordSize;

 	aOutputPackMode8Bit = calcParam->cmOutputColorSpace & cm8PerChannelPacking || calcParam->cmOutputColorSpace & cmLong8ColorPacking;
   /*DebugPrint("DoNDim with %d input elements\n",aByteCount);*/
	#if FARBR_FILES
	WriteLuts( 	"DoNDim",1,adr_bereich_elut,aElutWordSize,ein_lut,
				NDIM_IN_DIM,NDIM_OUT_DIM,aXlutAdrSize,aXlutWordSize,(LH_UINT16 *)Xlut,adr_bereich_alut,aAlutWordSize,(LH_UINT16 *)aus_lut);
    #endif

	i=0;
					
	{
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
						adr_bereich_elut,adr_breite_elut,aElutWordSize,ein_lut);
			DebugPrint("aAlutAdrSize=%lx,aAlutAdrShift=%lx,aAlutWordSize=%lx,aus_lut=%lx,\n",
						adr_bereich_alut,adr_breite_alut,aAlutWordSize,aus_lut);
			DebugPrint("aXlutInDim=%lx,aXlutOutDim=%lx,aXlutAdrSize=%lx,aXlutAdrShift=%lx,aXlutWordSize=%lx,Xlut=%lx,\n",
						NDIM_IN_DIM,NDIM_OUT_DIM,aXlutAdrSize,aXlutAdrShift,aXlutWordSize,Xlut);
		}
    #endif
    
    /*if( 1 )*/
    if( aXlutAdrSize != (1<<aXlutAdrShift )){
    register long aXlutOffset;
    long theXlutOffsets[8]; 
    register unsigned long aAlutInShift;
    register long aAlutInShiftRemainder;
    register unsigned long aAlutInShiftNum;
    register long aElutWordSizeMask = (1<<aElutWordSize) - 1;
    register unsigned long aAlutRound;
   aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;
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
    aXlutOffset =NDIM_OUT_DIM;
    for( i=0; i<(long)NDIM_IN_DIM; i++){
        theXlutOffsets[ NDIM_IN_DIM-1-i] = aXlutOffset;
        aXlutOffset *=aXlutAdrSize;
    }
	aAlutRound = 1<<( aAlutInShift + aAlutShift - 1 );

#ifdef DEBUG_OUTPUT
        if ( DebugCheck(kThisFile, kDebugReserved1) )
            DebugPrint(" aElutWordSize((1<<aElutWordSize)-0) = %ld\n aAlutInShift:((1<<aXlutWordSize)*aElutWordSize+(adr_bereich_alut/2))/adr_bereich_alut = %ld\n",aElutWordSize,aAlutInShift);
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
			
			        ein_Cache[0]=jj=(*(LH_UINT16 *)inputData[0]);
					ko0 = jj - ( jj >> ( adr_breite_elut ));
					ko = ko0 & ( aElutShiftNum - 1 );
					ko0 = ko0 >> aElutShift;
					ko0 += aElutOffset;
			       	jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
			        jj *= aXlutAdrSize;
			        aElutOffset += adr_bereich_elut;
		        	adr[0] = jj & aElutWordSizeMask;
		        	jj = jj >> aElutWordSize;
		        	adr0 += (jj)*theXlutOffsets[0];
		        	ein_reg[0] = (LH_UINT16)jj;

			        ein_Cache[1]=jj=(*(LH_UINT16 *)inputData[1]);
					ko0 = jj - ( jj >> ( adr_breite_elut ));
					ko = ko0 & ( aElutShiftNum - 1 );
					ko0 = ko0 >> aElutShift;
					ko0 += aElutOffset;
			       	jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
			        jj *= aXlutAdrSize;
			        aElutOffset += adr_bereich_elut;
		        	adr[1] = jj & aElutWordSizeMask;
		        	jj = jj >> aElutWordSize;
		        	adr0 += (jj)*theXlutOffsets[1];
		        	ein_reg[1] = (LH_UINT16)jj;

					ein_Cache[2]=jj=(*(LH_UINT16 *)inputData[2]);
					ko0 = jj - ( jj >> ( adr_breite_elut ));
					ko = ko0 & ( aElutShiftNum - 1 );
					ko0 = ko0 >> aElutShift;
					ko0 += aElutOffset;
			       	jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
			        jj *= aXlutAdrSize;
			        aElutOffset += adr_bereich_elut;
		        	adr[2] = jj & aElutWordSizeMask;
		        	jj = jj >> aElutWordSize;
		        	adr0 += (jj)*theXlutOffsets[2];
		        	ein_reg[2] = (LH_UINT16)jj;

#if NDIM_IN_DIM == 4
					ein_Cache[3]=jj=(*(LH_UINT16 *)inputData[3]);
					ko0 = jj - ( jj >> ( adr_breite_elut ));
					ko = ko0 & ( aElutShiftNum - 1 );
					ko0 = ko0 >> aElutShift;
					ko0 += aElutOffset;
			       	jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
			        jj *= aXlutAdrSize;
			        aElutOffset += adr_bereich_elut;
		        	adr[3] = jj & aElutWordSizeMask;
		        	jj = jj >> aElutWordSize;
		        	adr0 += (jj)*theXlutOffsets[3];
		        	ein_reg[3] = (LH_UINT16)jj;
#endif
		
	

	        {								/* a kind of */
	            register long Hold;
	            
			
				Index[0] = 0;
				Index[1] = 1;
				Index[2] = 2;
#if NDIM_IN_DIM == 4
				Index[3] = 3;
#endif
					if( adr[0] < adr[1] ){
						Hold = Index[0];
						Index[0] = Index[1];
						Index[1] = Hold;
					}

					if( adr[Index[1]] < adr[2] ){
						Hold = Index[1];
						Index[1] = Index[2];
						Index[2] = Hold;
						if( adr[Index[0]] < adr[Index[1]] ){
							Hold = Index[0];
							Index[0] = Index[1];
							Index[1] = Hold;
						}
					}

#if NDIM_IN_DIM == 4
					if( adr[Index[2]] < adr[3] ){
						Hold = Index[2];
						Index[2] = Index[3];
						Index[3] = Hold;
						if( adr[Index[1]] < adr[Index[2]] ){
							Hold = Index[1];
							Index[1] = Index[2];
							Index[2] = Hold;
							if( adr[Index[0]] < adr[Index[1]] ){
								Hold = Index[0];
								Index[0] = Index[1];
								Index[1] = Hold;
							}
						}
					}
#endif
	        }

        accu[0]=0;
        accu[1]=0;
        accu[2]=0;
#if NDIM_OUT_DIM == 4
        accu[3]=0;
#endif
        ko0 = (1<<aElutWordSize);
        adrAdr=adr0;
        adrOffset=0;
        if( aXlutWordSize  == 16 ){
                jj = Index[0];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=Xlut[adrAdr+(0)]*ko;
                    accu[1]+=Xlut[adrAdr+(1)]*ko;
                    accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif

                adrAdr = (adr0 + adrOffset);

                jj = Index[1];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=Xlut[adrAdr+(0)]*ko;
                    accu[1]+=Xlut[adrAdr+(1)]*ko;
                    accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif

                adrAdr = (adr0 + adrOffset);

                jj = Index[2];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=Xlut[adrAdr+(0)]*ko;
                    accu[1]+=Xlut[adrAdr+(1)]*ko;
                    accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif

                adrAdr = (adr0 + adrOffset);

#if NDIM_IN_DIM == 4
                jj = Index[3];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=Xlut[adrAdr+(0)]*ko;
                    accu[1]+=Xlut[adrAdr+(1)]*ko;
                    accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif

                adrAdr = (adr0 + adrOffset);
#endif

                accu[0]+=Xlut[adrAdr+0]*ko0;
                accu[1]+=Xlut[adrAdr+1]*ko0;
                accu[2]+=Xlut[adrAdr+2]*ko0;
 #if NDIM_OUT_DIM == 4
               accu[3]+=Xlut[adrAdr+3]*ko0;
 #endif
       }
       else{

                jj = Index[0];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
                adrAdr = (adr0 + adrOffset);

                jj = Index[1];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
                adrAdr = (adr0 + adrOffset);

                jj = Index[2];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
                adrAdr = (adr0 + adrOffset);

#if NDIM_IN_DIM == 4
                jj = Index[3];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
                adrAdr = (adr0 + adrOffset);
#endif
                accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+0]*ko0;
                accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+1]*ko0;
                accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+2]*ko0;
#if NDIM_OUT_DIM == 4
               accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+3]*ko0;
#endif
        }

		aAlutOffset = 0;

	        if( aOutputPackMode8Bit ){
	        	for( ii=0; ii<NDIM_OUT_DIM; ++ii){
					jj = accu[ii];
					jj = jj + ( jj >> aXlutWordSize );

					ko0 = (jj - ( jj >> ( adr_breite_alut ))) >> aAlutInShiftRemainder; 	/*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/

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
	                aAlutOffset += adr_bereich_alut;
	            }
			}
			else{
				if( aXlutWordSize >= 16 ){
					for( ii=0; ii<NDIM_OUT_DIM; ++ii){
						jj = accu[ii];
						jj = jj + ( jj >> aXlutWordSize );
              			ko0 = (jj - ( jj >> ( adr_breite_alut ))) >> aAlutInShiftRemainder; /*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/
		
						ko = ko0 & (aAlutInShiftNum - 1 );
						ko0 = ko0 >> aAlutInShift;
						ko0 += aAlutOffset;
	 					if( aAlutWordSize <= 8)
		       				jj = ( ((LH_UINT8*)aus_lut)[ ko0 ] * ( aAlutInShiftNum - ko ) + ((LH_UINT8*)aus_lut)[ ko0 +1 ] * ko ) << ( aAlutShift - aAlutInShift );
	 					else
		       				jj = ( aus_lut[ ko0 ] * ( aAlutInShiftNum - ko ) + aus_lut[ ko0 +1 ] * ko ) >> (aAlutInShift- aAlutShift);
						*((LH_UINT16 *)outputData[ii]) =  (LH_UINT16)jj;
						aAlutOffset += adr_bereich_alut;
					}
				}
				else{
					for( ii=0; ii<NDIM_OUT_DIM; ++ii){
						jj = accu[ii];
						jj = jj + ( jj >> aXlutWordSize ) + ( jj >> 2*aXlutWordSize );
              			ko0 = (jj - ( jj >> ( adr_breite_alut ))) ; /*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/
		
						ko = ko0 & (aAlutInShiftNum - 1 );
						ko0 = ko0 >> aAlutInShift;
						ko0 += aAlutOffset;
	 					if( aAlutWordSize <= 8)
		       				jj = ( ((LH_UINT8*)aus_lut)[ ko0 ] * ( aAlutInShiftNum - ko ) + ((LH_UINT8*)aus_lut)[ ko0 +1 ] * ko ) << ( aAlutShift - aAlutInShift );
	 					else
		       				jj = ( aus_lut[ ko0 ] * ( aAlutInShiftNum - ko ) + aus_lut[ ko0 +1 ] * ko ) >> (aAlutInShift- aAlutShift);
						*((LH_UINT16 *)outputData[ii]) =  (LH_UINT16)jj;
						aAlutOffset += adr_bereich_alut;
					}
				}
			}
			while (--i){
			   	for( jj=0; jj<NDIM_IN_DIM; ++jj){
					inputData[jj] += (NDIM_IN_DIM * 2);
				}
			   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
					outputData[jj] += OutputIncrement;
				}
	
				{
					for( jj=0; jj<NDIM_IN_DIM; ++jj){
						if( *((LH_UINT16 *)inputData[jj]) ^ *(LH_UINT16 *)(&ein_Cache[jj]) )break;
					}
				}
				if( jj<NDIM_IN_DIM ) break;
				if( aOutputPackMode8Bit ){
				   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
						*outputData[jj] = outputData[jj][-(long)(NDIM_OUT_DIM )];
					}
				}
				else{
				   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
						*((LH_UINT16 *)outputData[jj]) = *(LH_UINT16 *)(&outputData[jj][-(long)(NDIM_OUT_DIM * 2)]);
					}
				}
			}
		}
		
	   	if( --LineCount ){
		   	for( jj=0; jj<NDIM_IN_DIM; ++jj){
				inputData[jj] += inputDataRowOffset;
			}
		   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
				outputData[jj] += outputDataRowOffset;
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
    aAlutInShift = (aXlutWordSize+bit_breit_selektor-adr_breite_alut);
    /*aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/
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
	                ein_Cache[0]=jj=(*(LH_UINT16 *)inputData[0]);
	                ko0 = jj - ( jj >> ( adr_breite_elut ));
	                ko = ko0 & ( aElutShiftNum - 1 );
	                ko0 = ko0 >> aElutShift;
	                ko0 += aElutOffset;
	                   jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
	                aElutOffset += adr_bereich_elut;
	                adr[0] = (jj & bit_maske_selektor);
	                adr0 |= ((jj & bit_maske_adr)>>bit_breit_selektor)<<((NDIM_IN_DIM-0-1)*bit_breit_adr);
	                ein_reg[0] = (LH_UINT16)jj;

	                ein_Cache[1]=jj=(*(LH_UINT16 *)inputData[1]);
	                ko0 = jj - ( jj >> ( adr_breite_elut ));
	                ko = ko0 & ( aElutShiftNum - 1 );
	                ko0 = ko0 >> aElutShift;
	                ko0 += aElutOffset;
	                   jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
	                aElutOffset += adr_bereich_elut;
	                adr[1] = (jj & bit_maske_selektor);
	                adr0 |= ((jj & bit_maske_adr)>>bit_breit_selektor)<<((NDIM_IN_DIM-1-1)*bit_breit_adr);
	                ein_reg[1] = (LH_UINT16)jj;

	                ein_Cache[2]=jj=(*(LH_UINT16 *)inputData[2]);
	                ko0 = jj - ( jj >> ( adr_breite_elut ));
	                ko = ko0 & ( aElutShiftNum - 1 );
	                ko0 = ko0 >> aElutShift;
	                ko0 += aElutOffset;
	                   jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
	                aElutOffset += adr_bereich_elut;
	                adr[2] = (jj & bit_maske_selektor);
	                adr0 |= ((jj & bit_maske_adr)>>bit_breit_selektor)<<((NDIM_IN_DIM-2-1)*bit_breit_adr);
	                ein_reg[2] = (LH_UINT16)jj;

#if NDIM_IN_DIM == 4
	                ein_Cache[3]=jj=(*(LH_UINT16 *)inputData[3]);
	                ko0 = jj - ( jj >> ( adr_breite_elut ));
	                ko = ko0 & ( aElutShiftNum - 1 );
	                ko0 = ko0 >> aElutShift;
	                ko0 += aElutOffset;
	                   jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
	                aElutOffset += adr_bereich_elut;
	                adr[3] = (jj & bit_maske_selektor);
	                adr0 |= ((jj & bit_maske_adr)>>bit_breit_selektor)<<((NDIM_IN_DIM-3-1)*bit_breit_adr);
	                ein_reg[3] = (LH_UINT16)jj;
#endif	
	        adr0 *= NDIM_OUT_DIM;
	
	        {								/* a kind of */
	            register long Hold;
	            
			
				Index[0] = 0;
				Index[1] = 1;
				Index[2] = 2;
#if NDIM_IN_DIM == 4
				Index[3] = 3;
#endif
					if( adr[0] < adr[1] ){
						Hold = Index[0];
						Index[0] = Index[1];
						Index[1] = Hold;
					}

					if( adr[Index[1]] < adr[2] ){
						Hold = Index[1];
						Index[1] = Index[2];
						Index[2] = Hold;
						if( adr[Index[0]] < adr[Index[1]] ){
							Hold = Index[0];
							Index[0] = Index[1];
							Index[1] = Hold;
						}
					}

#if NDIM_IN_DIM == 4
					if( adr[Index[2]] < adr[3] ){
						Hold = Index[2];
						Index[2] = Index[3];
						Index[3] = Hold;
						if( adr[Index[1]] < adr[Index[2]] ){
							Hold = Index[1];
							Index[1] = Index[2];
							Index[2] = Hold;
							if( adr[Index[0]] < adr[Index[1]] ){
								Hold = Index[0];
								Index[0] = Index[1];
								Index[1] = Hold;
							}
						}
					}
#endif
	        }
	
			accu[0]=0;
			accu[1]=0;
			accu[2]=0;
#if NDIM_OUT_DIM == 4
			accu[3]=0;
#endif
	
	        ko0 = bit_maske_selektor+1;
	        adrAdr=adr0;
	        adrOffset=0;
	
	        if( aXlutWordSize  == 16 ){
	                jj = Index[0];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
						accu[0]+=Xlut[adrAdr+(0)]*ko;
						accu[1]+=Xlut[adrAdr+(1)]*ko;
						accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
						accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

	                jj = Index[1];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
						accu[0]+=Xlut[adrAdr+(0)]*ko;
						accu[1]+=Xlut[adrAdr+(1)]*ko;
						accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
						accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

	                jj = Index[2];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
						accu[0]+=Xlut[adrAdr+(0)]*ko;
						accu[1]+=Xlut[adrAdr+(1)]*ko;
						accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
						accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

#if NDIM_IN_DIM == 4
	                jj = Index[3];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
						accu[0]+=Xlut[adrAdr+(0)]*ko;
						accu[1]+=Xlut[adrAdr+(1)]*ko;
						accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
						accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);
#endif
                accu[0]+=Xlut[adrAdr+0]*ko0;
                accu[1]+=Xlut[adrAdr+1]*ko0;
                accu[2]+=Xlut[adrAdr+2]*ko0;
#if NDIM_OUT_DIM == 4
				accu[3]+=Xlut[adrAdr+3]*ko0;
#endif
	        }
	        else{
	                jj = Index[0];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
	                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
	                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
	                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
	                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
	
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

	                jj = Index[1];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
	                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
	                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
	                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
	                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
	
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

	                jj = Index[2];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
	                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
	                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
	                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
	                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
	
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

#if NDIM_IN_DIM == 4
	                jj = Index[3];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
	                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
	                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
	                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
	                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
	
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);
#endif
				accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+0]*ko0;
                accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+1]*ko0;
                accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+2]*ko0;
#if NDIM_OUT_DIM == 4
				accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+3]*ko0;
#endif
	        }
	
	        aAlutOffset = 0;
	
	        if( aOutputPackMode8Bit ){
	        	for( ii=0; ii<NDIM_OUT_DIM; ++ii){
					jj = accu[ii];
					jj = jj + ( jj >> aXlutWordSize );

					ko0 = (jj - ( jj >> ( adr_breite_alut ))) >> aAlutInShiftRemainder; 	/*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/

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
	                aAlutOffset += adr_bereich_alut;
	            }
	        }
	        else{
				if( aXlutWordSize >= 16 ){
					for( ii=0; ii<NDIM_OUT_DIM; ++ii){
						jj = accu[ii];
						jj = jj + ( jj >> aXlutWordSize );
              			ko0 = (jj - ( jj >> ( adr_breite_alut ))) >> aAlutInShiftRemainder; /*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/
		
						ko = ko0 & (aAlutInShiftNum - 1 );
						ko0 = ko0 >> aAlutInShift;
						ko0 += aAlutOffset;
	 					if( aAlutWordSize <= 8)
		       				jj = ( ((LH_UINT8*)aus_lut)[ ko0 ] * ( aAlutInShiftNum - ko ) + ((LH_UINT8*)aus_lut)[ ko0 +1 ] * ko ) << ( aAlutShift - aAlutInShift );
	 					else
		       				jj = ( aus_lut[ ko0 ] * ( aAlutInShiftNum - ko ) + aus_lut[ ko0 +1 ] * ko ) >> (aAlutInShift- aAlutShift);
						*((LH_UINT16 *)outputData[ii]) =  (LH_UINT16)jj;
						aAlutOffset += adr_bereich_alut;
					}
				}
				else{
					for( ii=0; ii<NDIM_OUT_DIM; ++ii){
						jj = accu[ii];
						jj = jj + ( jj >> aXlutWordSize ) + ( jj >> 2*aXlutWordSize );
              			ko0 = (jj - ( jj >> ( adr_breite_alut ))); /*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/
		
						ko = ko0 & (aAlutInShiftNum - 1 );
						ko0 = ko0 >> aAlutInShift;
						ko0 += aAlutOffset;
	 					if( aAlutWordSize <= 8)
		       				jj = ( ((LH_UINT8*)aus_lut)[ ko0 ] * ( aAlutInShiftNum - ko ) + ((LH_UINT8*)aus_lut)[ ko0 +1 ] * ko ) << ( aAlutShift - aAlutInShift );
	 					else
		       				jj = ( aus_lut[ ko0 ] * ( aAlutInShiftNum - ko ) + aus_lut[ ko0 +1 ] * ko ) >> (aAlutInShift- aAlutShift);
						*((LH_UINT16 *)outputData[ii]) =  (LH_UINT16)jj;
						aAlutOffset += adr_bereich_alut;
					}
				}
	        }
	
			while (--i){
			   	for( jj=0; jj<NDIM_IN_DIM; ++jj){
					inputData[jj] += (NDIM_IN_DIM * 2);
				}
			   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
					outputData[jj] += OutputIncrement;
				}
	
				{
				   	for( jj=0; jj<NDIM_IN_DIM; ++jj){
						if( *((LH_UINT16 *)inputData[jj]) ^ *(LH_UINT16 *)(&ein_Cache[jj]) )break;
					}
				}
				if( jj<NDIM_IN_DIM ) break;
				if( aOutputPackMode8Bit ){
				   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
						*outputData[jj] = outputData[jj][-(long)(NDIM_OUT_DIM )];
					}
				}
				else{
				   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
						*((LH_UINT16 *)outputData[jj]) = *(LH_UINT16 *)(&outputData[jj][-(long)(NDIM_OUT_DIM * 2)]);
					}
				}
			}
		}
		
	   	if( --LineCount ){
		   	for( jj=0; jj<NDIM_IN_DIM; ++jj){
				inputData[jj] += inputDataRowOffset;
			}
		   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
				outputData[jj] += outputDataRowOffset;
			}
		}
    }
    }

	/* UNLOCK_DATA( aElutHdle ); */
	/* UNLOCK_DATA( aAlutHdle ); */
	/* UNLOCK_DATA( aXlutHdle ); */

	LH_END_PROC("Calc423Dim_Data8To8_Lut16")
	return noErr;
}

#undef NDIM_IN_DIM
#undef NDIM_OUT_DIM
#undef aElutShift
#undef aElutShiftNum
#define NDIM_IN_DIM 4
#define NDIM_OUT_DIM 4
#define aElutShift (16-adr_breite_elut)
#define aElutShiftNum (1<<aElutShift)

CMError Calc424Dim_Data8To8_Lut16 	(CMCalcParamPtr calcParam,
									 CMLutParamPtr  lutParam)
	
{
	LH_UINT8 * inputData[8], *outputData[8];
	UINT32 OutputIncrement, inputDataRowOffset, outputDataRowOffset, Pixelcount, LineCount;
   	register unsigned long adr0;
    register unsigned long ko0;
    unsigned long accu[8];
    register long i;
    /*long Offsets[8];*/
    
    register long aAlutShift,aElutOffset,aAlutOffset;
    register long aElutWordSize;
    register long aAlutWordSize;
    register long aXlutAdrSize;
    register long aXlutAdrShift;
    register unsigned long aXlutWordSize;
	register unsigned long ii,jj;
    register long aOutputPackMode8Bit;
	long ein_Cache[8];
	
	LH_UINT16 * aus_lut	= (LH_UINT16*)lutParam->outputLut;
	LH_UINT16 * ein_lut	= (LH_UINT16*)lutParam->inputLut;
	LH_UINT16 * Xlut 	= (LH_UINT16*)lutParam->colorLut;
	
	#ifdef DEBUG_OUTPUT
	long err = noErr;
	#endif
	LH_START_PROC("Calc424Dim_Data8To8_Lut16")
	
	inputData[0] = (LH_UINT8 *)calcParam->inputData[0];
	inputData[1] = (LH_UINT8 *)calcParam->inputData[1];
	inputData[2] = (LH_UINT8 *)calcParam->inputData[2];
#if NDIM_IN_DIM == 4
	inputData[3] = (LH_UINT8 *)calcParam->inputData[3];
#endif
	outputData[0] = (LH_UINT8 *)calcParam->outputData[0];
	outputData[1] = (LH_UINT8 *)calcParam->outputData[1];
	outputData[2] = (LH_UINT8 *)calcParam->outputData[2];
#if NDIM_OUT_DIM == 4
	outputData[3] = (LH_UINT8 *)calcParam->outputData[3];
#endif

	OutputIncrement = calcParam->cmOutputPixelOffset;
	inputDataRowOffset = calcParam->cmInputBytesPerLine - calcParam->cmPixelPerLine * calcParam->cmInputPixelOffset + (NDIM_IN_DIM * 2);
	outputDataRowOffset = calcParam->cmOutputBytesPerLine - calcParam->cmPixelPerLine * calcParam->cmOutputPixelOffset + OutputIncrement;

	Pixelcount = calcParam->cmPixelPerLine;
	LineCount = calcParam->cmLineCount;

	aElutWordSize = lutParam->inputLutWordSize;
	aAlutWordSize = lutParam->outputLutWordSize;
	aXlutAdrSize = lutParam->colorLutGridPoints;
	for ( i = 1; (i < 32) && (aXlutAdrSize >> i); i++)
		aXlutAdrShift = i;
	aXlutWordSize = lutParam->colorLutWordSize;

 	aOutputPackMode8Bit = calcParam->cmOutputColorSpace & cm8PerChannelPacking || calcParam->cmOutputColorSpace & cmLong8ColorPacking;
   /*DebugPrint("DoNDim with %d input elements\n",aByteCount);*/
	#if FARBR_FILES
	WriteLuts( 	"DoNDim",1,adr_bereich_elut,aElutWordSize,ein_lut,
				NDIM_IN_DIM,NDIM_OUT_DIM,aXlutAdrSize,aXlutWordSize,(LH_UINT16 *)Xlut,adr_bereich_alut,aAlutWordSize,(LH_UINT16 *)aus_lut);
    #endif

	i=0;
					
	{
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
						adr_bereich_elut,adr_breite_elut,aElutWordSize,ein_lut);
			DebugPrint("aAlutAdrSize=%lx,aAlutAdrShift=%lx,aAlutWordSize=%lx,aus_lut=%lx,\n",
						adr_bereich_alut,adr_breite_alut,aAlutWordSize,aus_lut);
			DebugPrint("aXlutInDim=%lx,aXlutOutDim=%lx,aXlutAdrSize=%lx,aXlutAdrShift=%lx,aXlutWordSize=%lx,Xlut=%lx,\n",
						NDIM_IN_DIM,NDIM_OUT_DIM,aXlutAdrSize,aXlutAdrShift,aXlutWordSize,Xlut);
		}
    #endif
    
    /*if( 1 )*/
    if( aXlutAdrSize != (1<<aXlutAdrShift )){
    register long aXlutOffset;
    long theXlutOffsets[8]; 
    register unsigned long aAlutInShift;
    register long aAlutInShiftRemainder;
    register unsigned long aAlutInShiftNum;
    register long aElutWordSizeMask = (1<<aElutWordSize) - 1;
    register unsigned long aAlutRound;
   aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;
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
    aXlutOffset =NDIM_OUT_DIM;
    for( i=0; i<(long)NDIM_IN_DIM; i++){
        theXlutOffsets[ NDIM_IN_DIM-1-i] = aXlutOffset;
        aXlutOffset *=aXlutAdrSize;
    }
	aAlutRound = 1<<( aAlutInShift + aAlutShift - 1 );

#ifdef DEBUG_OUTPUT
        if ( DebugCheck(kThisFile, kDebugReserved1) )
            DebugPrint(" aElutWordSize((1<<aElutWordSize)-0) = %ld\n aAlutInShift:((1<<aXlutWordSize)*aElutWordSize+(adr_bereich_alut/2))/adr_bereich_alut = %ld\n",aElutWordSize,aAlutInShift);
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
			
			        ein_Cache[0]=jj=(*(LH_UINT16 *)inputData[0]);
					ko0 = jj - ( jj >> ( adr_breite_elut ));
					ko = ko0 & ( aElutShiftNum - 1 );
					ko0 = ko0 >> aElutShift;
					ko0 += aElutOffset;
			       	jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
			        jj *= aXlutAdrSize;
			        aElutOffset += adr_bereich_elut;
		        	adr[0] = jj & aElutWordSizeMask;
		        	jj = jj >> aElutWordSize;
		        	adr0 += (jj)*theXlutOffsets[0];
		        	ein_reg[0] = (LH_UINT16)jj;

			        ein_Cache[1]=jj=(*(LH_UINT16 *)inputData[1]);
					ko0 = jj - ( jj >> ( adr_breite_elut ));
					ko = ko0 & ( aElutShiftNum - 1 );
					ko0 = ko0 >> aElutShift;
					ko0 += aElutOffset;
			       	jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
			        jj *= aXlutAdrSize;
			        aElutOffset += adr_bereich_elut;
		        	adr[1] = jj & aElutWordSizeMask;
		        	jj = jj >> aElutWordSize;
		        	adr0 += (jj)*theXlutOffsets[1];
		        	ein_reg[1] = (LH_UINT16)jj;

					ein_Cache[2]=jj=(*(LH_UINT16 *)inputData[2]);
					ko0 = jj - ( jj >> ( adr_breite_elut ));
					ko = ko0 & ( aElutShiftNum - 1 );
					ko0 = ko0 >> aElutShift;
					ko0 += aElutOffset;
			       	jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
			        jj *= aXlutAdrSize;
			        aElutOffset += adr_bereich_elut;
		        	adr[2] = jj & aElutWordSizeMask;
		        	jj = jj >> aElutWordSize;
		        	adr0 += (jj)*theXlutOffsets[2];
		        	ein_reg[2] = (LH_UINT16)jj;

#if NDIM_IN_DIM == 4
					ein_Cache[3]=jj=(*(LH_UINT16 *)inputData[3]);
					ko0 = jj - ( jj >> ( adr_breite_elut ));
					ko = ko0 & ( aElutShiftNum - 1 );
					ko0 = ko0 >> aElutShift;
					ko0 += aElutOffset;
			       	jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
			        jj *= aXlutAdrSize;
			        aElutOffset += adr_bereich_elut;
		        	adr[3] = jj & aElutWordSizeMask;
		        	jj = jj >> aElutWordSize;
		        	adr0 += (jj)*theXlutOffsets[3];
		        	ein_reg[3] = (LH_UINT16)jj;
#endif
		
	

	        {								/* a kind of */
	            register long Hold;
	            
			
				Index[0] = 0;
				Index[1] = 1;
				Index[2] = 2;
#if NDIM_IN_DIM == 4
				Index[3] = 3;
#endif
					if( adr[0] < adr[1] ){
						Hold = Index[0];
						Index[0] = Index[1];
						Index[1] = Hold;
					}

					if( adr[Index[1]] < adr[2] ){
						Hold = Index[1];
						Index[1] = Index[2];
						Index[2] = Hold;
						if( adr[Index[0]] < adr[Index[1]] ){
							Hold = Index[0];
							Index[0] = Index[1];
							Index[1] = Hold;
						}
					}

#if NDIM_IN_DIM == 4
					if( adr[Index[2]] < adr[3] ){
						Hold = Index[2];
						Index[2] = Index[3];
						Index[3] = Hold;
						if( adr[Index[1]] < adr[Index[2]] ){
							Hold = Index[1];
							Index[1] = Index[2];
							Index[2] = Hold;
							if( adr[Index[0]] < adr[Index[1]] ){
								Hold = Index[0];
								Index[0] = Index[1];
								Index[1] = Hold;
							}
						}
					}
#endif
	        }

        accu[0]=0;
        accu[1]=0;
        accu[2]=0;
#if NDIM_OUT_DIM == 4
        accu[3]=0;
#endif
        ko0 = (1<<aElutWordSize);
        adrAdr=adr0;
        adrOffset=0;
        if( aXlutWordSize  == 16 ){
                jj = Index[0];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=Xlut[adrAdr+(0)]*ko;
                    accu[1]+=Xlut[adrAdr+(1)]*ko;
                    accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif

                adrAdr = (adr0 + adrOffset);

                jj = Index[1];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=Xlut[adrAdr+(0)]*ko;
                    accu[1]+=Xlut[adrAdr+(1)]*ko;
                    accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif

                adrAdr = (adr0 + adrOffset);

                jj = Index[2];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=Xlut[adrAdr+(0)]*ko;
                    accu[1]+=Xlut[adrAdr+(1)]*ko;
                    accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif

                adrAdr = (adr0 + adrOffset);

#if NDIM_IN_DIM == 4
                jj = Index[3];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=Xlut[adrAdr+(0)]*ko;
                    accu[1]+=Xlut[adrAdr+(1)]*ko;
                    accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif

                adrAdr = (adr0 + adrOffset);
#endif

                accu[0]+=Xlut[adrAdr+0]*ko0;
                accu[1]+=Xlut[adrAdr+1]*ko0;
                accu[2]+=Xlut[adrAdr+2]*ko0;
 #if NDIM_OUT_DIM == 4
               accu[3]+=Xlut[adrAdr+3]*ko0;
 #endif
       }
       else{

                jj = Index[0];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
                adrAdr = (adr0 + adrOffset);

                jj = Index[1];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
                adrAdr = (adr0 + adrOffset);

                jj = Index[2];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
                adrAdr = (adr0 + adrOffset);

#if NDIM_IN_DIM == 4
                jj = Index[3];
                ko = ko0 - adr[jj];
                ko0 = adr[jj];

                if( ein_reg[jj] < (LH_UINT16)(aXlutAdrSize-1) )
                    adrOffset += theXlutOffsets[jj];

                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
                adrAdr = (adr0 + adrOffset);
#endif
                accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+0]*ko0;
                accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+1]*ko0;
                accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+2]*ko0;
#if NDIM_OUT_DIM == 4
               accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+3]*ko0;
#endif
        }

		aAlutOffset = 0;

	        if( aOutputPackMode8Bit ){
	        	for( ii=0; ii<NDIM_OUT_DIM; ++ii){
					jj = accu[ii];
					jj = jj + ( jj >> aXlutWordSize );

					ko0 = (jj - ( jj >> ( adr_breite_alut ))) >> aAlutInShiftRemainder; 	/*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/

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
	                aAlutOffset += adr_bereich_alut;
	            }
			}
			else{
				if( aXlutWordSize >= 16 ){
					for( ii=0; ii<NDIM_OUT_DIM; ++ii){
						jj = accu[ii];
						jj = jj + ( jj >> aXlutWordSize );
              			ko0 = (jj - ( jj >> ( adr_breite_alut ))) >> aAlutInShiftRemainder; /*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/
		
						ko = ko0 & (aAlutInShiftNum - 1 );
						ko0 = ko0 >> aAlutInShift;
						ko0 += aAlutOffset;
	 					if( aAlutWordSize <= 8)
		       				jj = ( ((LH_UINT8*)aus_lut)[ ko0 ] * ( aAlutInShiftNum - ko ) + ((LH_UINT8*)aus_lut)[ ko0 +1 ] * ko ) << ( aAlutShift - aAlutInShift );
	 					else
		       				jj = ( aus_lut[ ko0 ] * ( aAlutInShiftNum - ko ) + aus_lut[ ko0 +1 ] * ko ) >> (aAlutInShift- aAlutShift);
						*((LH_UINT16 *)outputData[ii]) =  (LH_UINT16)jj;
						aAlutOffset += adr_bereich_alut;
					}
				}
				else{
					for( ii=0; ii<NDIM_OUT_DIM; ++ii){
						jj = accu[ii];
						jj = jj + ( jj >> aXlutWordSize ) + ( jj >> 2*aXlutWordSize );
              			ko0 = (jj - ( jj >> ( adr_breite_alut ))) ; /*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/
		
						ko = ko0 & (aAlutInShiftNum - 1 );
						ko0 = ko0 >> aAlutInShift;
						ko0 += aAlutOffset;
	 					if( aAlutWordSize <= 8)
		       				jj = ( ((LH_UINT8*)aus_lut)[ ko0 ] * ( aAlutInShiftNum - ko ) + ((LH_UINT8*)aus_lut)[ ko0 +1 ] * ko ) << ( aAlutShift - aAlutInShift );
	 					else
		       				jj = ( aus_lut[ ko0 ] * ( aAlutInShiftNum - ko ) + aus_lut[ ko0 +1 ] * ko ) >> (aAlutInShift- aAlutShift);
						*((LH_UINT16 *)outputData[ii]) =  (LH_UINT16)jj;
						aAlutOffset += adr_bereich_alut;
					}
				}
			}
			while (--i){
			   	for( jj=0; jj<NDIM_IN_DIM; ++jj){
					inputData[jj] += (NDIM_IN_DIM * 2);
				}
			   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
					outputData[jj] += OutputIncrement;
				}
	
				{
					for( jj=0; jj<NDIM_IN_DIM; ++jj){
						if( *((LH_UINT16 *)inputData[jj]) ^ *(LH_UINT16 *)(&ein_Cache[jj]) )break;
					}
				}
				if( jj<NDIM_IN_DIM ) break;
				if( aOutputPackMode8Bit ){
				   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
						*outputData[jj] = outputData[jj][-(long)(NDIM_OUT_DIM )];
					}
				}
				else{
				   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
						*((LH_UINT16 *)outputData[jj]) = *(LH_UINT16 *)(&outputData[jj][-(long)(NDIM_OUT_DIM * 2)]);
					}
				}
			}
		}
		
	   	if( --LineCount ){
		   	for( jj=0; jj<NDIM_IN_DIM; ++jj){
				inputData[jj] += inputDataRowOffset;
			}
		   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
				outputData[jj] += outputDataRowOffset;
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
    aAlutInShift = (aXlutWordSize+bit_breit_selektor-adr_breite_alut);
    /*aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/
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
	                ein_Cache[0]=jj=(*(LH_UINT16 *)inputData[0]);
	                ko0 = jj - ( jj >> ( adr_breite_elut ));
	                ko = ko0 & ( aElutShiftNum - 1 );
	                ko0 = ko0 >> aElutShift;
	                ko0 += aElutOffset;
	                   jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
	                aElutOffset += adr_bereich_elut;
	                adr[0] = (jj & bit_maske_selektor);
	                adr0 |= ((jj & bit_maske_adr)>>bit_breit_selektor)<<((NDIM_IN_DIM-0-1)*bit_breit_adr);
	                ein_reg[0] = (LH_UINT16)jj;

	                ein_Cache[1]=jj=(*(LH_UINT16 *)inputData[1]);
	                ko0 = jj - ( jj >> ( adr_breite_elut ));
	                ko = ko0 & ( aElutShiftNum - 1 );
	                ko0 = ko0 >> aElutShift;
	                ko0 += aElutOffset;
	                   jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
	                aElutOffset += adr_bereich_elut;
	                adr[1] = (jj & bit_maske_selektor);
	                adr0 |= ((jj & bit_maske_adr)>>bit_breit_selektor)<<((NDIM_IN_DIM-1-1)*bit_breit_adr);
	                ein_reg[1] = (LH_UINT16)jj;

	                ein_Cache[2]=jj=(*(LH_UINT16 *)inputData[2]);
	                ko0 = jj - ( jj >> ( adr_breite_elut ));
	                ko = ko0 & ( aElutShiftNum - 1 );
	                ko0 = ko0 >> aElutShift;
	                ko0 += aElutOffset;
	                   jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
	                aElutOffset += adr_bereich_elut;
	                adr[2] = (jj & bit_maske_selektor);
	                adr0 |= ((jj & bit_maske_adr)>>bit_breit_selektor)<<((NDIM_IN_DIM-2-1)*bit_breit_adr);
	                ein_reg[2] = (LH_UINT16)jj;

#if NDIM_IN_DIM == 4
	                ein_Cache[3]=jj=(*(LH_UINT16 *)inputData[3]);
	                ko0 = jj - ( jj >> ( adr_breite_elut ));
	                ko = ko0 & ( aElutShiftNum - 1 );
	                ko0 = ko0 >> aElutShift;
	                ko0 += aElutOffset;
	                   jj = ( ein_lut[ ko0 ] * ( aElutShiftNum - ko ) + ein_lut[ ko0 +1 ] * ko ) >> aElutShift;
	
	                aElutOffset += adr_bereich_elut;
	                adr[3] = (jj & bit_maske_selektor);
	                adr0 |= ((jj & bit_maske_adr)>>bit_breit_selektor)<<((NDIM_IN_DIM-3-1)*bit_breit_adr);
	                ein_reg[3] = (LH_UINT16)jj;
#endif	
	        adr0 *= NDIM_OUT_DIM;
	
	        {								/* a kind of */
	            register long Hold;
	            
			
				Index[0] = 0;
				Index[1] = 1;
				Index[2] = 2;
#if NDIM_IN_DIM == 4
				Index[3] = 3;
#endif
					if( adr[0] < adr[1] ){
						Hold = Index[0];
						Index[0] = Index[1];
						Index[1] = Hold;
					}

					if( adr[Index[1]] < adr[2] ){
						Hold = Index[1];
						Index[1] = Index[2];
						Index[2] = Hold;
						if( adr[Index[0]] < adr[Index[1]] ){
							Hold = Index[0];
							Index[0] = Index[1];
							Index[1] = Hold;
						}
					}

#if NDIM_IN_DIM == 4
					if( adr[Index[2]] < adr[3] ){
						Hold = Index[2];
						Index[2] = Index[3];
						Index[3] = Hold;
						if( adr[Index[1]] < adr[Index[2]] ){
							Hold = Index[1];
							Index[1] = Index[2];
							Index[2] = Hold;
							if( adr[Index[0]] < adr[Index[1]] ){
								Hold = Index[0];
								Index[0] = Index[1];
								Index[1] = Hold;
							}
						}
					}
#endif
	        }
	
			accu[0]=0;
			accu[1]=0;
			accu[2]=0;
#if NDIM_OUT_DIM == 4
			accu[3]=0;
#endif
	
	        ko0 = bit_maske_selektor+1;
	        adrAdr=adr0;
	        adrOffset=0;
	
	        if( aXlutWordSize  == 16 ){
	                jj = Index[0];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
						accu[0]+=Xlut[adrAdr+(0)]*ko;
						accu[1]+=Xlut[adrAdr+(1)]*ko;
						accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
						accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

	                jj = Index[1];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
						accu[0]+=Xlut[adrAdr+(0)]*ko;
						accu[1]+=Xlut[adrAdr+(1)]*ko;
						accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
						accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

	                jj = Index[2];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
						accu[0]+=Xlut[adrAdr+(0)]*ko;
						accu[1]+=Xlut[adrAdr+(1)]*ko;
						accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
						accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

#if NDIM_IN_DIM == 4
	                jj = Index[3];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
						accu[0]+=Xlut[adrAdr+(0)]*ko;
						accu[1]+=Xlut[adrAdr+(1)]*ko;
						accu[2]+=Xlut[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
						accu[3]+=Xlut[adrAdr+(3)]*ko;
#endif
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);
#endif
                accu[0]+=Xlut[adrAdr+0]*ko0;
                accu[1]+=Xlut[adrAdr+1]*ko0;
                accu[2]+=Xlut[adrAdr+2]*ko0;
#if NDIM_OUT_DIM == 4
				accu[3]+=Xlut[adrAdr+3]*ko0;
#endif
	        }
	        else{
	                jj = Index[0];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
	                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
	                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
	                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
	                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
	
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

	                jj = Index[1];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
	                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
	                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
	                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
	                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
	
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

	                jj = Index[2];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
	                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
	                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
	                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
	                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
	
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);

#if NDIM_IN_DIM == 4
	                jj = Index[3];
	                ko = ko0 - adr[jj];
	                ko0 = adr[jj];
	
	                if( ein_reg[jj] < bit_maske_adr )
	                    adrOffset |= (1<<(NDIM_IN_DIM-1-jj)*bit_breit_adr);
	
	                    accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+(0)]*ko;
	                    accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+(1)]*ko;
	                    accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+(2)]*ko;
#if NDIM_OUT_DIM == 4
	                    accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+(3)]*ko;
#endif
	
	                adrAdr = (adr0 + NDIM_OUT_DIM*adrOffset);
#endif
				accu[0]+=((LH_UINT8 *)Xlut)[adrAdr+0]*ko0;
                accu[1]+=((LH_UINT8 *)Xlut)[adrAdr+1]*ko0;
                accu[2]+=((LH_UINT8 *)Xlut)[adrAdr+2]*ko0;
#if NDIM_OUT_DIM == 4
				accu[3]+=((LH_UINT8 *)Xlut)[adrAdr+3]*ko0;
#endif
	        }
	
	        aAlutOffset = 0;
	
	        if( aOutputPackMode8Bit ){
	        	for( ii=0; ii<NDIM_OUT_DIM; ++ii){
					jj = accu[ii];
					jj = jj + ( jj >> aXlutWordSize );

					ko0 = (jj - ( jj >> ( adr_breite_alut ))) >> aAlutInShiftRemainder; 	/*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/

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
	                aAlutOffset += adr_bereich_alut;
	            }
	        }
	        else{
				if( aXlutWordSize >= 16 ){
					for( ii=0; ii<NDIM_OUT_DIM; ++ii){
						jj = accu[ii];
						jj = jj + ( jj >> aXlutWordSize );
              			ko0 = (jj - ( jj >> ( adr_breite_alut ))) >> aAlutInShiftRemainder; /*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/
		
						ko = ko0 & (aAlutInShiftNum - 1 );
						ko0 = ko0 >> aAlutInShift;
						ko0 += aAlutOffset;
	 					if( aAlutWordSize <= 8)
		       				jj = ( ((LH_UINT8*)aus_lut)[ ko0 ] * ( aAlutInShiftNum - ko ) + ((LH_UINT8*)aus_lut)[ ko0 +1 ] * ko ) << ( aAlutShift - aAlutInShift );
	 					else
		       				jj = ( aus_lut[ ko0 ] * ( aAlutInShiftNum - ko ) + aus_lut[ ko0 +1 ] * ko ) >> (aAlutInShift- aAlutShift);
						*((LH_UINT16 *)outputData[ii]) =  (LH_UINT16)jj;
						aAlutOffset += adr_bereich_alut;
					}
				}
				else{
					for( ii=0; ii<NDIM_OUT_DIM; ++ii){
						jj = accu[ii];
						jj = jj + ( jj >> aXlutWordSize ) + ( jj >> 2*aXlutWordSize );
              			ko0 = (jj - ( jj >> ( adr_breite_alut ))); /*	aAlutInShift = aXlutWordSize + aElutWordSize - adr_breite_alut;*/
		
						ko = ko0 & (aAlutInShiftNum - 1 );
						ko0 = ko0 >> aAlutInShift;
						ko0 += aAlutOffset;
	 					if( aAlutWordSize <= 8)
		       				jj = ( ((LH_UINT8*)aus_lut)[ ko0 ] * ( aAlutInShiftNum - ko ) + ((LH_UINT8*)aus_lut)[ ko0 +1 ] * ko ) << ( aAlutShift - aAlutInShift );
	 					else
		       				jj = ( aus_lut[ ko0 ] * ( aAlutInShiftNum - ko ) + aus_lut[ ko0 +1 ] * ko ) >> (aAlutInShift- aAlutShift);
						*((LH_UINT16 *)outputData[ii]) =  (LH_UINT16)jj;
						aAlutOffset += adr_bereich_alut;
					}
				}
	        }
	
			while (--i){
			   	for( jj=0; jj<NDIM_IN_DIM; ++jj){
					inputData[jj] += (NDIM_IN_DIM * 2);
				}
			   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
					outputData[jj] += OutputIncrement;
				}
	
				{
				   	for( jj=0; jj<NDIM_IN_DIM; ++jj){
						if( *((LH_UINT16 *)inputData[jj]) ^ *(LH_UINT16 *)(&ein_Cache[jj]) )break;
					}
				}
				if( jj<NDIM_IN_DIM ) break;
				if( aOutputPackMode8Bit ){
				   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
						*outputData[jj] = outputData[jj][-(long)(NDIM_OUT_DIM )];
					}
				}
				else{
				   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
						*((LH_UINT16 *)outputData[jj]) = *(LH_UINT16 *)(&outputData[jj][-(long)(NDIM_OUT_DIM * 2)]);
					}
				}
			}
		}
		
	   	if( --LineCount ){
		   	for( jj=0; jj<NDIM_IN_DIM; ++jj){
				inputData[jj] += inputDataRowOffset;
			}
		   	for( jj=0; jj<NDIM_OUT_DIM; ++jj){
				outputData[jj] += outputDataRowOffset;
			}
		}
    }
    }

	/* UNLOCK_DATA( aElutHdle ); */
	/* UNLOCK_DATA( aAlutHdle ); */
	/* UNLOCK_DATA( aXlutHdle ); */

	LH_END_PROC("Calc424Dim_Data8To8_Lut16")
	return noErr;
}
#endif
