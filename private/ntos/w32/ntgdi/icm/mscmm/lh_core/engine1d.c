/*
	File:		Engine1D.c

	Contains:	

	Written by:	U. J. Krabbenhoeft

	Version:

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/

#define smallCode 1

#undef LH_DATA_IN_TYPE
#undef LH_DATA_OUT_TYPE
#undef LH_LUT_DATA_TYPE
#undef LH_DATA_IN_COUNT
#undef LH_DATA_SHR
#undef LH_DATA_SHR_CORR
#undef LH_LUT_DATA_SHR
#undef LH_BIT_BREIT_INTERNAL
#if LH_DATA_IN_SIZE_16
#define LH_DATA_IN_TYPE LH_UINT16
#else
#define LH_DATA_IN_TYPE LH_UINT8
#endif
#if LH_DATA_OUT_SIZE_16
#define LH_DATA_OUT_TYPE LH_UINT16
#else
#define LH_DATA_OUT_TYPE LH_UINT8
#endif
#if LH_LUT_DATA_SIZE_16
#define LH_BIT_BREIT_INTERNAL 	16
#define LH_LUT_DATA_TYPE LH_UINT16
#else
#if LH_DATA_IN_SIZE_16
#define LH_BIT_BREIT_INTERNAL 	16
#else
#define LH_BIT_BREIT_INTERNAL 	10
#endif
#define LH_LUT_DATA_TYPE LH_UINT8
#endif

#if LH_DATA_IN_COUNT_4
#define LH_DATA_IN_COUNT 		4
#else
#define LH_DATA_IN_COUNT 		3
#endif

#define LH_BIT_MASKE_ADR (((1<<7)-1)<< (LH_BIT_BREIT_INTERNAL-7))
#define LH_BIT_BREIT_SELEKTOR (LH_BIT_BREIT_INTERNAL-7)
#define LH_BIT_MASKE_SELEKTOR ((1<<LH_BIT_BREIT_SELEKTOR)-1)

#define LH_ADR_BEREICH_SEL 		(1<<LH_BIT_BREIT_SELEKTOR)

#if LH_LUT_DATA_SIZE_16
#define LH_DATA_SHR               (16+LH_BIT_BREIT_SELEKTOR-LH_ADR_BREIT_AUS_LUT)  /* z.B. 16+11-10=17 */
#define LH_DATA_SHR_CORR 8		/* notwendig bei LH_DATA_SHR > 16 */
#define LH_LUT_DATA_SHR  16		/* Normierung bei Alutinterpolation */
#else
#define LH_DATA_SHR               (8+LH_BIT_BREIT_SELEKTOR-LH_ADR_BREIT_AUS_LUT)   /* z.B. 8+7-10=5 */
#define LH_LUT_DATA_SHR  8		/* Normierung bei Alutinterpolation */
#endif

{
	LH_UINT32 ein_regr;
	LH_DATA_IN_TYPE ein_cache;
	LH_LUT_DATA_TYPE * paNewVal0;
	LH_UINT32 ako0;

	LH_UINT8 	Mode;
	LH_UINT8	LH_DATA_OUT_COUNT = (LH_UINT8)lutParam->colorLutOutDim;
	LH_UINT32 	PixelCount, LineCount, i, j;
	long inputOffset,outputOffset;
	LH_DATA_IN_TYPE * input0 = (LH_DATA_IN_TYPE *)calcParam->inputData[0];
	LH_DATA_IN_TYPE * input1 = (LH_DATA_IN_TYPE *)calcParam->inputData[1];

	LH_DATA_OUT_TYPE * output0 = (LH_DATA_OUT_TYPE *)calcParam->outputData[0];
	LH_DATA_OUT_TYPE * output1 = (LH_DATA_OUT_TYPE *)calcParam->outputData[1];
	LH_DATA_OUT_TYPE * output2 = (LH_DATA_OUT_TYPE *)calcParam->outputData[2];
	LH_DATA_OUT_TYPE * output3 = (LH_DATA_OUT_TYPE *)calcParam->outputData[3];
	LH_DATA_OUT_TYPE * output4 = (LH_DATA_OUT_TYPE *)calcParam->outputData[4];

	LH_UINT16 * My_InputLut = (LH_UINT16 *)lutParam->inputLut;
	LH_LUT_DATA_TYPE * My_OutputLut = (LH_LUT_DATA_TYPE *)lutParam->outputLut;
	LH_LUT_DATA_TYPE * My_ColorLut = (LH_LUT_DATA_TYPE *)lutParam->colorLut;

	LH_DATA_OUT_TYPE Mask = (LH_DATA_OUT_TYPE)-1;

#ifdef DEBUG_OUTPUT
	CMError err = noErr;
#endif
	LH_START_PROC(LH_CALC_PROC_NAME)

	#if LH_DATA_IN_SIZE_16
	inputOffset = (long)calcParam->cmInputPixelOffset / 2;
	#else
	inputOffset = (long)calcParam->cmInputPixelOffset;
	#endif
	#if LH_DATA_OUT_SIZE_16
	outputOffset = (long)calcParam->cmOutputPixelOffset / 2;
	#else
	outputOffset = (long)calcParam->cmOutputPixelOffset;
	#endif

	if (calcParam->clearMask)
		Mask = 0;
	Mode = LH_CALC_ENGINE_UNDEF_MODE;


	if ((calcParam->cmInputPixelOffset * calcParam->cmPixelPerLine == calcParam->cmInputBytesPerLine) && (calcParam->cmOutputPixelOffset * calcParam->cmPixelPerLine == calcParam->cmOutputBytesPerLine))
	{
		PixelCount = calcParam->cmPixelPerLine * calcParam->cmLineCount;
		LineCount = 1;
	}
	else
	{
		PixelCount = calcParam->cmPixelPerLine;
		LineCount = calcParam->cmLineCount;
	}
	if (calcParam->copyAlpha )
	{
			Mode = LH_CALC_ENGINE_U_TO_U;
	}
	else
	{
		if (calcParam->clearMask)
			Mode = LH_CALC_ENGINE_P_TO_U;
		else
			Mode = LH_CALC_ENGINE_P_TO_P;
	}
 	j = 0;
	while (LineCount)
	{
		i = PixelCount;
		while (i)
		{
			#if LH_LUT_DATA_SIZE_16
			#if LH_DATA_IN_SIZE_16 || LH_DATA_OUT_SIZE_16
    	   		register LH_UINT32 ko;
			#endif
			#if LH_DATA_IN_SIZE_16
	       		register LH_DATA_IN_TYPE aValIn;
    	   	#endif
			#if LH_DATA_OUT_SIZE_16
	       		register LH_UINT32 aVal;
    	   	#endif
			#if LH_DATA_IN_SIZE_16
				aValIn = (ein_cache=*input0) - ( *input0 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (LH_DATA_IN_TYPE)((aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 0 << LH_ADR_BREIT_EIN_LUT ));
		       	ein_regr = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >>( 16-LH_ADR_BREIT_EIN_LUT );
			#else
			ein_regr = My_InputLut[(ein_cache=*input0) + ( 0 << LH_ADR_BREIT_EIN_LUT )];
			#endif
			#else
			#if LH_DATA_IN_SIZE_16
	       		register LH_DATA_IN_TYPE aValIn;
    	   		register LH_UINT32 ko;
				aValIn = (ein_cache=*input0) - ( *input0 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (LH_DATA_IN_TYPE)((aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 0 << LH_ADR_BREIT_EIN_LUT ));
		       	ein_regr = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >>( 10-LH_ADR_BREIT_EIN_LUT );
			#else
			ein_regr = My_InputLut[(ein_cache=*input0) + ( 0 << LH_ADR_BREIT_EIN_LUT )];
			#endif
			#endif
			paNewVal0 = (LH_LUT_DATA_TYPE *)My_ColorLut +
						((ein_regr >> LH_BIT_BREIT_SELEKTOR)*LH_DATA_OUT_COUNT);
			ein_regr &= LH_BIT_MASKE_SELEKTOR;

			ako0 = (LH_ADR_BEREICH_SEL) - ein_regr;
			#if LH_LUT_DATA_SIZE_16
			#if LH_DATA_OUT_SIZE_16
	
			aVal =					 (ako0 		* paNewVal0[0] +
									  ein_regr	* paNewVal0[LH_DATA_OUT_COUNT]);
			aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
			aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
			ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
			aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
			*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

			if( LH_DATA_OUT_COUNT > 1 ){
			paNewVal0++;
			aVal =					 (ako0 		* paNewVal0[0] +
									  ein_regr	* paNewVal0[LH_DATA_OUT_COUNT]);
			aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
			aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
			ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
			aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
			*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

			paNewVal0++;
			aVal =					 (ako0 		* paNewVal0[0] +
									  ein_regr	* paNewVal0[LH_DATA_OUT_COUNT]);
			aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
			aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
			ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
			aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
			*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

			if( LH_DATA_OUT_COUNT > 3 ){
			paNewVal0++;
			aVal =					 (ako0 		* paNewVal0[0] +
									  ein_regr	* paNewVal0[LH_DATA_OUT_COUNT]);
			aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
			aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
			ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
			aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
			*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
			}}
			
			#else
			*output0 = My_OutputLut[((ako0 		* paNewVal0[0] +
									  ein_regr	* paNewVal0[LH_DATA_OUT_COUNT])>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
			if( LH_DATA_OUT_COUNT > 1 ){
			paNewVal0++;
			*output1 = My_OutputLut[((ako0 		* paNewVal0[0] +
									  ein_regr	* paNewVal0[LH_DATA_OUT_COUNT])>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
			paNewVal0++;
			*output2 = My_OutputLut[((ako0	 	* paNewVal0[0] +
									  ein_regr	* paNewVal0[LH_DATA_OUT_COUNT])>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
			if( LH_DATA_OUT_COUNT > 3 ){
			paNewVal0++;
			*output3 = My_OutputLut[((ako0 		* paNewVal0[0] +
									  ein_regr	* paNewVal0[LH_DATA_OUT_COUNT])>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
			}}
			#endif
			
			#else
			*output0 = My_OutputLut[((ako0 		* paNewVal0[0] +
									  ein_regr	* paNewVal0[LH_DATA_OUT_COUNT])>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
			if( LH_DATA_OUT_COUNT > 1 ){
			paNewVal0++;
			*output1 = My_OutputLut[((ako0 		* paNewVal0[0] +
									  ein_regr	* paNewVal0[LH_DATA_OUT_COUNT])>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
			paNewVal0++;
			*output2 = My_OutputLut[((ako0	 	* paNewVal0[0] +
									  ein_regr	* paNewVal0[LH_DATA_OUT_COUNT])>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
			if( LH_DATA_OUT_COUNT > 3 ){
			paNewVal0++;
			*output3 = My_OutputLut[((ako0 		* paNewVal0[0] +
									  ein_regr	* paNewVal0[LH_DATA_OUT_COUNT])>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
			}}
			#endif

			#if LH_DATA_OUT_SIZE_16 && ! LH_LUT_DATA_SIZE_16
			*output0 |= (*output0 << 8);
			if( LH_DATA_OUT_COUNT > 1 ){
			*output1 |= (*output1 << 8);
			*output2 |= (*output2 << 8);
			if( LH_DATA_OUT_COUNT > 3 ){
			*output3 |= (*output3 << 8);
			}}
			#endif
			
			if (Mode == LH_CALC_ENGINE_P_TO_P)
			{
				while (--i)
				{
					input0 += inputOffset;
					output0 += outputOffset;
					output1 += outputOffset;
					output2 += outputOffset;
					output3 += outputOffset;

					if ((*input0 ^ ein_cache)){
						break;
					}
					*output0 = output0[-outputOffset];
					if( LH_DATA_OUT_COUNT > 1 ){
					*output1 = output1[-outputOffset];
					*output2 = output2[-outputOffset];
					if( LH_DATA_OUT_COUNT > 3 ){
					*output3 = output3[-outputOffset];
					}}
				}
			}
			else if (Mode == LH_CALC_ENGINE_P_TO_U)
			{
				if( LH_DATA_OUT_COUNT == 1 ){
					*output1 &= Mask;
				}else if( LH_DATA_OUT_COUNT == 3 ){
					*output3 &= Mask;
				}else{
					*output4 &= Mask;
				}
				while (--i)
				{
					input0 += inputOffset;
					output0 += outputOffset;
					output1 += outputOffset;
					output2 += outputOffset;
					output3 += outputOffset;
					output4 += outputOffset;
					if ((*input0 ^ ein_cache)){
						break;
					}
					*output0 = output0[-outputOffset];
					if( LH_DATA_OUT_COUNT > 1 ){
						*output1 = output1[-outputOffset];
						*output2 = output2[-outputOffset];
						if( LH_DATA_OUT_COUNT > 3 ){
							*output3 = output3[-outputOffset];
							*output4 &= Mask;
						}
						else{
							*output3 &= Mask;
						}
					}
					else{
						*output1 &= Mask;
					}
				}
			}
			else
			{
				if( LH_DATA_OUT_COUNT == 1 ){
					*output1 = (LH_DATA_OUT_TYPE)*input1;
				}else if( LH_DATA_OUT_COUNT == 3 ){
					*output3 = (LH_DATA_OUT_TYPE)*input1;
				}else{
					*output4 = (LH_DATA_OUT_TYPE)*input1;
				}
				while (--i)
				{								/*U_TO_U*/
					input0 += inputOffset;
					input1 += inputOffset;
					output0 += outputOffset;
					output1 += outputOffset;
					output2 += outputOffset;
					output3 += outputOffset;
					output4 += outputOffset;
					if ((*input0 ^ ein_cache))
					{
						break;
					}
					*output0 = output0[-outputOffset];
					if( LH_DATA_OUT_COUNT > 1 ){
						*output1 = output1[-outputOffset];
						*output2 = output2[-outputOffset];
						if( LH_DATA_OUT_COUNT > 3 ){
							*output3 = output3[-outputOffset];
							*output4 = (LH_DATA_OUT_TYPE)*input1;
						}
						else{
							*output3 = (LH_DATA_OUT_TYPE)*input1;
						}
					}
					else{
						*output1 = (LH_DATA_OUT_TYPE)*input1;
					}
				}
			}
		}
		if (--LineCount)
		{
			j++;
			input0 = (LH_DATA_IN_TYPE *)((LH_UINT8 *)calcParam->inputData[0] + j * calcParam->cmInputBytesPerLine);
			input1 = (LH_DATA_IN_TYPE *)((LH_UINT8 *)calcParam->inputData[1] + j * calcParam->cmInputBytesPerLine);

			output0 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[0] + j * calcParam->cmOutputBytesPerLine);
			output1 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[1] + j * calcParam->cmOutputBytesPerLine);
			output2 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[2] + j * calcParam->cmOutputBytesPerLine);
			output3 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[3] + j * calcParam->cmOutputBytesPerLine);
			output4 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[4] + j * calcParam->cmOutputBytesPerLine);
		}
	}
	LH_END_PROC(LH_CALC_PROC_NAME)
	return 0;
}
#undef LH_CALC_PROC_NAME
