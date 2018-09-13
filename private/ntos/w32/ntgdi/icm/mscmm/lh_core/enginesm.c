/*
	File:		LHCalcEngine.c

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

#define LH_BIT_MASKE_ADR (((1<<LH_BIT_BREIT_ADR)-1)<< (LH_BIT_BREIT_INTERNAL-LH_BIT_BREIT_ADR))
#define LH_BIT_BREIT_SELEKTOR (LH_BIT_BREIT_INTERNAL-LH_BIT_BREIT_ADR)
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

#if LH_DATA_IN_COUNT_4
{
#if smallCode

	LH_UINT32 var_0_0_0_1;
	LH_UINT32 var_0_0_1_0;
	LH_UINT32 var_0_0_1_1;
	LH_UINT32 var_0_1_0_0;
	LH_UINT32 var_0_1_0_1;
	LH_UINT32 var_0_1_1_0;
	LH_UINT32 var_0_1_1_1;
	LH_UINT32 var_1_0_0_0;
	LH_UINT32 var_1_0_0_1;
	LH_UINT32 var_1_0_1_0;
	LH_UINT32 var_1_0_1_1;
	LH_UINT32 var_1_1_0_0;
	LH_UINT32 var_1_1_0_1;
	LH_UINT32 var_1_1_1_0;
	LH_UINT32 var_1_1_1_1;


 	LH_UINT32 ein_regY;
	LH_UINT32 ein_regM;
	LH_UINT32 ein_regC;
	LH_UINT32 ein_regK;
	LH_DATA_IN_TYPE ein_cache[4];
	LH_LUT_DATA_TYPE * paNewVal0;
	LH_UINT32 ako0;
	LH_UINT32 ako1;
	LH_UINT32 ako2;
	LH_UINT32 ako3;

	LH_UINT8 	Mode;
	LH_UINT32 	PixelCount, LineCount, i, j;
	LH_UINT8 	LH_DATA_OUT_COUNT;
	long inputOffset,outputOffset;
	LH_DATA_IN_TYPE * input0 = (LH_DATA_IN_TYPE *)calcParam->inputData[0];
	LH_DATA_IN_TYPE * input1 = (LH_DATA_IN_TYPE *)calcParam->inputData[1];
	LH_DATA_IN_TYPE * input2 = (LH_DATA_IN_TYPE *)calcParam->inputData[2];
	LH_DATA_IN_TYPE * input3 = (LH_DATA_IN_TYPE *)calcParam->inputData[3];
	LH_DATA_IN_TYPE * input4 = (LH_DATA_IN_TYPE *)calcParam->inputData[4];

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

	if( LH_DATA_OUT_COUNT_4 ){
		LH_DATA_OUT_COUNT 		= 4;
	}else{
		LH_DATA_OUT_COUNT 		= 3;
	}

	var_0_0_0_1 = (((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT);
	var_0_0_1_0 = (((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT);
	var_0_0_1_1 = (((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT);
	var_0_1_0_0 = (((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT);
	var_0_1_0_1 = (((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT);
	var_0_1_1_0 = (((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT);
	var_0_1_1_1 = (((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT);
	var_1_0_0_0 = (((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT);
	var_1_0_0_1 = (((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT);
	var_1_0_1_0 = (((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT);
	var_1_0_1_1 = (((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT);
	var_1_1_0_0 = (((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT);
	var_1_1_0_1 = (((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT);
	var_1_1_1_0 = (((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT);
	var_1_1_1_1 = (((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT);

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
				aValIn = (ein_cache[0]=*input0) - ( *input0 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 0 << LH_ADR_BREIT_EIN_LUT );
		       	ein_regC = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >>( 16-LH_ADR_BREIT_EIN_LUT );
		       	
				aValIn = (ein_cache[1]=*input1) - ( *input1 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 1 << LH_ADR_BREIT_EIN_LUT );
		       	ein_regM = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >> ( 16-LH_ADR_BREIT_EIN_LUT );
		       	
				aValIn = (ein_cache[2]=*input2) - ( *input2 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 2 << LH_ADR_BREIT_EIN_LUT );
		       	ein_regY = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >> ( 16-LH_ADR_BREIT_EIN_LUT );

				aValIn = (ein_cache[3]=*input3) - ( *input3 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 3 << LH_ADR_BREIT_EIN_LUT );
		       	ein_regK = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >> ( 16-LH_ADR_BREIT_EIN_LUT );
			#else
			ein_regC = My_InputLut[(ein_cache[0]=*input0) + ( 0 << LH_ADR_BREIT_EIN_LUT )];
			ein_regM = My_InputLut[(ein_cache[1]=*input1) + ( 1 << LH_ADR_BREIT_EIN_LUT )];
			ein_regY = My_InputLut[(ein_cache[2]=*input2) + ( 2 << LH_ADR_BREIT_EIN_LUT )];
			ein_regK = My_InputLut[(ein_cache[3]=*input3) + ( 3 << LH_ADR_BREIT_EIN_LUT )];
			#endif
			#else
			#if LH_DATA_IN_SIZE_16
	       		register LH_DATA_IN_TYPE aValIn;
    	   		register LH_UINT32 ko;
				aValIn = (ein_cache[0]=*input0) - ( *input0 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 0 << LH_ADR_BREIT_EIN_LUT );
		       	ein_regC = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >>( 10-LH_ADR_BREIT_EIN_LUT );
		       	
				aValIn = (ein_cache[1]=*input1) - ( *input1 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 1 << LH_ADR_BREIT_EIN_LUT );
		       	ein_regM = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >> ( 10-LH_ADR_BREIT_EIN_LUT );
		       	
				aValIn = (ein_cache[2]=*input2) - ( *input2 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 2 << LH_ADR_BREIT_EIN_LUT );
		       	ein_regY = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >> ( 10-LH_ADR_BREIT_EIN_LUT );

				aValIn = (ein_cache[3]=*input3) - ( *input3 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 3 << LH_ADR_BREIT_EIN_LUT );
		       	ein_regK = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >> ( 10-LH_ADR_BREIT_EIN_LUT );
			/*ein_regC = My_InputLut[(*input0>>( 10-LH_ADR_BREIT_EIN_LUT )) + ( 0 << LH_ADR_BREIT_EIN_LUT )];
			ein_regM = My_InputLut[(*input1>>( 10-LH_ADR_BREIT_EIN_LUT )) + ( 1 << LH_ADR_BREIT_EIN_LUT )];
			ein_regY = My_InputLut[(*input2>>( 10-LH_ADR_BREIT_EIN_LUT )) + ( 2 << LH_ADR_BREIT_EIN_LUT )];
			ein_regK = My_InputLut[(*input3>>( 10-LH_ADR_BREIT_EIN_LUT )) + ( 3 << LH_ADR_BREIT_EIN_LUT )];*/
			#else
			ein_regC = My_InputLut[(ein_cache[0]=*input0) + ( 0 << LH_ADR_BREIT_EIN_LUT )];
			ein_regM = My_InputLut[(ein_cache[1]=*input1) + ( 1 << LH_ADR_BREIT_EIN_LUT )];
			ein_regY = My_InputLut[(ein_cache[2]=*input2) + ( 2 << LH_ADR_BREIT_EIN_LUT )];
			ein_regK = My_InputLut[(ein_cache[3]=*input3) + ( 3 << LH_ADR_BREIT_EIN_LUT )];
			#endif
			#endif
			paNewVal0 = (LH_LUT_DATA_TYPE *)My_ColorLut +
						((((((((ein_regC & LH_BIT_MASKE_ADR) << LH_BIT_BREIT_ADR) +
							   (ein_regM & LH_BIT_MASKE_ADR)) << LH_BIT_BREIT_ADR) +
							   (ein_regY & LH_BIT_MASKE_ADR))>> (LH_BIT_BREIT_SELEKTOR-LH_BIT_BREIT_ADR)) +
						       (ein_regK >> LH_BIT_BREIT_SELEKTOR))*LH_DATA_OUT_COUNT);
			ein_regC &= LH_BIT_MASKE_SELEKTOR;
			ein_regM &= LH_BIT_MASKE_SELEKTOR;
			ein_regY &= LH_BIT_MASKE_SELEKTOR;
			ein_regK &= LH_BIT_MASKE_SELEKTOR;
			if (ein_regY >= ein_regC)
			{
		        if( ein_regM >= ein_regC )
		        {
		            if( ein_regY >= ein_regM )	 	   /*  YMCK !*/
		            {	   		
		            	if( ein_regC >= ein_regK )
		            	{
							ako0 = LH_ADR_BEREICH_SEL - ein_regY;
							ako1 = ein_regY - ein_regM;
							ako2 = ein_regM - ein_regC;
							ako3 = ein_regC - ein_regK;
							
							ein_regC = var_0_0_1_0;
							ein_regM = var_0_1_1_0;
							ein_regY = var_1_1_1_0;
							
 						}
						else if(ein_regM >= ein_regK )	/*  YMKC !*/
						{	 	
							ako0 = LH_ADR_BEREICH_SEL - ein_regY;
							ako1 = ein_regY - ein_regM;
							ako2 = ein_regM - ein_regK;
							ako3 = ein_regK - ein_regC;
							
							ein_regK = ein_regC;
							ein_regC = var_0_0_1_0;
							ein_regM = var_0_1_1_0;
							ein_regY = var_0_1_1_1;
							
						}
						else if(ein_regY >= ein_regK )	/*  YKMC !*/
						{	
							ako0 = LH_ADR_BEREICH_SEL - ein_regY;
							ako1 = ein_regY - ein_regK;
							ako2 = ein_regK - ein_regM;
							ako3 = ein_regM - ein_regC;
							
							ein_regK = ein_regC;
							ein_regC = var_0_0_1_0;
							ein_regM = var_0_0_1_1;
							ein_regY = var_0_1_1_1;
							
						}
						else{	 						/*  KYMC !*/
							ako0 = LH_ADR_BEREICH_SEL - ein_regK;
							ako1 = ein_regK - ein_regY;
							ako2 = ein_regY - ein_regM;
							ako3 = ein_regM - ein_regC;
							
							ein_regK = ein_regC;
							ein_regC = var_0_0_0_1;
							ein_regM = var_0_0_1_1;
							ein_regY = var_0_1_1_1;
							
						}	
		            }
		            else
		            { 								/*  MYCK !*/
		            	if( ein_regC >= ein_regK )
		            	{				
							ako0 = LH_ADR_BEREICH_SEL - ein_regM;
							ako1 = ein_regM - ein_regY;
							ako2 = ein_regY - ein_regC;						
							ako3 = ein_regC - ein_regK;
							
							ein_regC = var_0_1_0_0;
							ein_regM = var_0_1_1_0;
							ein_regY = var_1_1_1_0;
							
						}
						else if(ein_regY >= ein_regK )	/*  MYKC !*/
						{	 	
							ako0 = LH_ADR_BEREICH_SEL - ein_regM;
							ako1 = ein_regM - ein_regY;
							ako2 = ein_regY - ein_regK;
							ako3 = ein_regK - ein_regC;
							
							ein_regK = ein_regC;
							ein_regC = var_0_1_0_0;
							ein_regM = var_0_1_1_0;
							ein_regY = var_0_1_1_1;
							
						}
						else if(ein_regM >= ein_regK )	/*  MKYC !*/
						{	 	
							ako0 = LH_ADR_BEREICH_SEL - ein_regM;
							ako1 = ein_regM - ein_regK;
							ako2 = ein_regK - ein_regY;
							ako3 = ein_regY - ein_regC;
							
							ein_regK = ein_regC;
							ein_regC = var_0_1_0_0;
							ein_regM = var_0_1_0_1;
							ein_regY = var_0_1_1_1;
							
						}
						else
						{							/*  KMYC !*/
							ako0 = LH_ADR_BEREICH_SEL - ein_regK;
							ako1 = ein_regK - ein_regM;
							ako2 = ein_regM - ein_regY;
							ako3 = ein_regY - ein_regC;
							
							ein_regK = ein_regC;
							ein_regC = var_0_0_0_1;
							ein_regM = var_0_1_0_1;
							ein_regY = var_0_1_1_1;
							
		                }
					}
	            }
	            else
	            { 									/*  YCMK !*/
	            	if( ein_regM >= ein_regK )
	            	{	
							ako0 = LH_ADR_BEREICH_SEL - ein_regY;
							ako1 = ein_regY - ein_regC;
							ako2 = ein_regC - ein_regM;
							ako3 = ein_regM - ein_regK;
							
							ein_regC = var_0_0_1_0;
							ein_regM = var_1_0_1_0;
							ein_regY = var_1_1_1_0;
							
					}
					else if(ein_regC >= ein_regK )	/*  YCKM !*/
					{	 	
							ako0 = LH_ADR_BEREICH_SEL - ein_regY;
							ako1 = ein_regY - ein_regC;
							ako2 = ein_regC - ein_regK;
							ako3 = ein_regK - ein_regM;
							
							ein_regK = ein_regM;
							ein_regC = var_0_0_1_0;
							ein_regM = var_1_0_1_0;
							ein_regY = var_1_0_1_1;
							
					}
					else if(ein_regY >= ein_regK )	/*  YKCM !*/
					{	 	
							ako0 = LH_ADR_BEREICH_SEL - ein_regY;
							ako1 = ein_regY - ein_regK;
							ako2 = ein_regK - ein_regC;
							ako3 = ein_regC - ein_regM;
							
							ein_regK = ein_regM;
							ein_regC = var_0_0_1_0;
							ein_regM = var_0_0_1_1;
							ein_regY = var_1_0_1_1;
							
					}
					else
					{						 	/*  KYCM !*/
							ako0 = LH_ADR_BEREICH_SEL - ein_regK;
							ako1 = ein_regK - ein_regY;
							ako2 = ein_regY - ein_regC;
							ako3 = ein_regC - ein_regM;
							
							ein_regK = ein_regM;
							ein_regC = var_0_0_0_1;
							ein_regM = var_0_0_1_1;
							ein_regY = var_1_0_1_1;
							
	            	}
		        }
			}
	        else
	        {
            	if( ein_regM >= ein_regC )
            	{
          			if( ein_regY >= ein_regK )		/*  MCYK !*/
          			{	
							ako0 = LH_ADR_BEREICH_SEL - ein_regM;
							ako1 = ein_regM - ein_regC;
							ako2 = ein_regC - ein_regY;
							ako3 = ein_regY - ein_regK;

							ein_regC = var_0_1_0_0;
							ein_regM = var_1_1_0_0;
							ein_regY = var_1_1_1_0;
							
					}
					else if(ein_regC >= ein_regK )	/*  MCKY !*/
					{	 	
							ako0 = LH_ADR_BEREICH_SEL - ein_regM;
							ako1 = ein_regM - ein_regC;
							ako2 = ein_regC - ein_regK;
							ako3 = ein_regK - ein_regY;
							
							ein_regK = ein_regY;
							ein_regC = var_0_1_0_0;
							ein_regM = var_1_1_0_0;
							ein_regY = var_1_1_0_1;
							
				}
					else if(ein_regM >= ein_regK )	/*  MKCY !*/
					{	 	
							ako0 = LH_ADR_BEREICH_SEL - ein_regM;
							ako1 = ein_regM - ein_regK;
							ako2 = ein_regK - ein_regC;
							ako3 = ein_regC - ein_regY;
							
							ein_regK = ein_regY;
							ein_regC = var_0_1_0_0;
							ein_regM = var_0_1_0_1;
							ein_regY = var_1_1_0_1;
							
					}
					else
					{						 	/*  KMCY !*/
							ako0 = LH_ADR_BEREICH_SEL - ein_regK;
							ako1 = ein_regK - ein_regM;
							ako2 = ein_regM - ein_regC;
							ako3 = ein_regC - ein_regY;
							
							ein_regK = ein_regY;
							ein_regC = var_0_0_0_1;
							ein_regM = var_0_1_0_1;
							ein_regY = var_1_1_0_1;
							
					}
                }
                else
                {
                    if( ein_regY >= ein_regM )
                    {
	          			if( ein_regM >= ein_regK )	/*  CYMK !*/
	          			{	
							ako0 = LH_ADR_BEREICH_SEL - ein_regC;
							ako1 = ein_regC - ein_regY;
							ako2 = ein_regY - ein_regM;
							ako3 = ein_regM - ein_regK;
							
							ein_regC = var_1_0_0_0;
							ein_regM = var_1_0_1_0;
							ein_regY = var_1_1_1_0;
							
						}
						else if(ein_regY >= ein_regK )	/*  CYKM !*/
						{	
							ako0 = LH_ADR_BEREICH_SEL - ein_regC;
							ako1 = ein_regC - ein_regY;
							ako2 = ein_regY - ein_regK;
							ako3 = ein_regK - ein_regM;
							
							ein_regK = ein_regM;
							ein_regC = var_1_0_0_0;
							ein_regM = var_1_0_1_0;
							ein_regY = var_1_0_1_1;
							
						}
						else if(ein_regC >= ein_regK )	/*  CKYM */
						{	
							ako0 = LH_ADR_BEREICH_SEL - ein_regC;
							ako1 = ein_regC - ein_regK;
							ako2 = ein_regK - ein_regY;
							ako3 = ein_regY - ein_regM;
							
							ein_regK = ein_regM;
							ein_regC = var_1_0_0_0;
							ein_regM = var_1_0_0_1;
							ein_regY = var_1_0_1_1;
							
						}
						else
						{							/*  KCYM !*/
							ako0 = LH_ADR_BEREICH_SEL - ein_regK;
							ako1 = ein_regK - ein_regC;
							ako2 = ein_regC - ein_regY;
							ako3 = ein_regY - ein_regM;
							
							ein_regK = ein_regM;
							ein_regC = var_0_0_0_1;
							ein_regM = var_1_0_0_1;
							ein_regY = var_1_0_1_1;
							
						}
                    }
                    else if( ein_regY >= ein_regK )		/*  CMYK !*/
                    {	
							ako0 = LH_ADR_BEREICH_SEL - ein_regC;
							ako1 = ein_regC - ein_regM;
							ako2 = ein_regM - ein_regY;
							ako3 = ein_regY - ein_regK;
							
							ein_regC = var_1_0_0_0;
							ein_regM = var_1_1_0_0;
							ein_regY = var_1_1_1_0;
							
					}
					else if(ein_regM >= ein_regK )	/*  CMKY !*/
					{	
							ako0 = LH_ADR_BEREICH_SEL - ein_regC;
							ako1 = ein_regC - ein_regM;
							ako2 = ein_regM - ein_regK;
							ako3 = ein_regK - ein_regY;
							
							ein_regK = ein_regY;
							ein_regC = var_1_0_0_0;
							ein_regM = var_1_1_0_0;
							ein_regY = var_1_1_0_1;
							
					}
					else
					{
						if(ein_regC >= ein_regK )	/*  CKMY !*/
						{	
							ako0 = LH_ADR_BEREICH_SEL - ein_regC;
							ako1 = ein_regC - ein_regK;
							ako2 = ein_regK - ein_regM;
							ako3 = ein_regM - ein_regY;
							
							ein_regK = ein_regY;
							ein_regC = var_1_0_0_0;
							ein_regM = var_1_0_0_1;
							ein_regY = var_1_1_0_1;
							
						}
						else
						{							/*  KCMY !*/
							ako0 = LH_ADR_BEREICH_SEL - ein_regK;
							ako1 = ein_regK - ein_regC;
							ako2 = ein_regC - ein_regM;
							ako3 = ein_regM - ein_regY;
							
							ein_regK = ein_regY;
							ein_regC = var_0_0_0_1;
							ein_regM = var_1_0_0_1;
							ein_regY = var_1_1_0_1;
							
		                }
					}
		    	}
			}

			#if LH_LUT_DATA_SIZE_16
			#if LH_DATA_OUT_SIZE_16
	
			aVal =					 (	ako0 * paNewVal0[0] +
	        							ako1 * paNewVal0[ein_regC + 0] +
	        							ako2 * paNewVal0[ein_regM + 0] +
	        							ako3 * paNewVal0[ein_regY + 0] +
	        						ein_regK * paNewVal0[var_1_1_1_1 + 0] );
			aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
			aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
			ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
			aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
	       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

			aVal =					 (	ako0 * paNewVal0[1] +
	        							ako1 * paNewVal0[ein_regC + 1] +
	        							ako2 * paNewVal0[ein_regM + 1] +
	        							ako3 * paNewVal0[ein_regY + 1] +
	        						ein_regK * paNewVal0[var_1_1_1_1 + 1] );
			aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
			aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
			ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
			aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
	       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

			aVal =					 (	ako0 * paNewVal0[2] +
	        							ako1 * paNewVal0[ein_regC + 2] +
	        							ako2 * paNewVal0[ein_regM + 2] +
	        							ako3 * paNewVal0[ein_regY + 2] +
	        						ein_regK * paNewVal0[var_1_1_1_1 + 2] );
			aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
			aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
			ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
			aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
	       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

			if( LH_DATA_OUT_COUNT_4 ){
			aVal =					 (	ako0 * paNewVal0[3] +
	        							ako1 * paNewVal0[ein_regC + 3] +
	        							ako2 * paNewVal0[ein_regM + 3] +
	        							ako3 * paNewVal0[ein_regY + 3] +
	        						ein_regK * paNewVal0[var_1_1_1_1 + 3] );
			aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
			aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
			ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
			aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
	       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
			}
			
			#else
			*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
	        							ako1 * paNewVal0[ein_regC + 0] +
	        							ako2 * paNewVal0[ein_regM + 0] +
	        							ako3 * paNewVal0[ein_regY + 0] +
	        						ein_regK * paNewVal0[var_1_1_1_1 + 0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
			*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
	        							ako1 * paNewVal0[ein_regC + 1] +
	        							ako2 * paNewVal0[ein_regM + 1] +
	        							ako3 * paNewVal0[ein_regY + 1] +
	        						ein_regK * paNewVal0[var_1_1_1_1 + 1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
			*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
	        							ako1 * paNewVal0[ein_regC + 2] +
	        							ako2 * paNewVal0[ein_regM + 2] +
	        							ako3 * paNewVal0[ein_regY + 2] +
	        						ein_regK * paNewVal0[var_1_1_1_1 + 2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
			if( LH_DATA_OUT_COUNT_4 ){
			*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
	        							ako1 * paNewVal0[ein_regC + 3] +
	        							ako2 * paNewVal0[ein_regM + 3] +
	        							ako3 * paNewVal0[ein_regY + 3] +
	        						ein_regK * paNewVal0[var_1_1_1_1 + 3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
			}
			#endif
			
			#else
			*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
	        							ako1 * paNewVal0[ein_regC + 0] +
	        							ako2 * paNewVal0[ein_regM + 0] +
	        							ako3 * paNewVal0[ein_regY + 0] +
	        						ein_regK * paNewVal0[var_1_1_1_1 + 0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
			*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
	        							ako1 * paNewVal0[ein_regC + 1] +
	        							ako2 * paNewVal0[ein_regM + 1] +
	        							ako3 * paNewVal0[ein_regY + 1] +
	        						ein_regK * paNewVal0[var_1_1_1_1 + 1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
			*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
	        							ako1 * paNewVal0[ein_regC + 2] +
	        							ako2 * paNewVal0[ein_regM + 2] +
	        							ako3 * paNewVal0[ein_regY + 2] +
	        						ein_regK * paNewVal0[var_1_1_1_1 + 2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
			if( LH_DATA_OUT_COUNT_4 ){
			*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
	        							ako1 * paNewVal0[ein_regC + 3] +
	        							ako2 * paNewVal0[ein_regM + 3] +
	        							ako3 * paNewVal0[ein_regY + 3] +
	        						ein_regK * paNewVal0[var_1_1_1_1 + 3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
			}
			#endif

			#if LH_DATA_OUT_SIZE_16 && ! LH_LUT_DATA_SIZE_16
			*output0 |= (*output0 << 8);
			*output1 |= (*output1 << 8);
			*output2 |= (*output2 << 8);
			if( LH_DATA_OUT_COUNT_4 ){
			*output3 |= (*output3 << 8);
			}
			#endif
			
			if (Mode == LH_CALC_ENGINE_P_TO_P)
			{
				while (--i)
				{
					input0 += inputOffset;
					input1 += inputOffset;
					input2 += inputOffset;
					input3 += inputOffset;
					output0 += outputOffset;
					output1 += outputOffset;
					output2 += outputOffset;
					if( LH_DATA_OUT_COUNT_4 ){
					output3 += outputOffset;
					}

					if ((*input0 ^ ein_cache[0]) || (*input1 ^ ein_cache[1]) || (*input2 ^ ein_cache[2]) || (*input3 ^ ein_cache[3]))
					{
						break;
					}
					*output0 = output0[-outputOffset];
					*output1 = output1[-outputOffset];
					*output2 = output2[-outputOffset];
					if( LH_DATA_OUT_COUNT_4 ){
					*output3 = output3[-outputOffset];
					}
				}
			}
			else if (Mode == LH_CALC_ENGINE_P_TO_U)
			{
				if( LH_DATA_OUT_COUNT_4 ){
				*output4 &= Mask;
				}else{
				*output3 &= Mask;
				}
				while (--i)
				{
					input0 += inputOffset;
					input1 += inputOffset;
					input2 += inputOffset;
					input3 += inputOffset;
					output0 += outputOffset;
					output1 += outputOffset;
					output2 += outputOffset;
					output3 += outputOffset;
					if( LH_DATA_OUT_COUNT_4 ){
					output4 += outputOffset;
					}
					if ((*input0 ^ ein_cache[0]) || (*input1 ^ ein_cache[1]) || (*input2 ^ ein_cache[2]) || (*input3 ^ ein_cache[3]))
					{
						break;
					}
					*output0 = output0[-outputOffset];
					*output1 = output1[-outputOffset];
					*output2 = output2[-outputOffset];
					if( LH_DATA_OUT_COUNT_4 ){
					*output3 = output3[-outputOffset];
					*output4 &= Mask;
					}else{
					*output3 &= Mask;
					}
				}
			}
			else
			{
				if( LH_DATA_OUT_COUNT_4 ){
				*output4 = (LH_DATA_OUT_TYPE)*input4;
				}else{
				*output3 = (LH_DATA_OUT_TYPE)*input4;
				}
				while (--i)
				{								/*U_TO_U*/
					input0 += inputOffset;
					input1 += inputOffset;
					input2 += inputOffset;
					input3 += inputOffset;
					input4 += inputOffset;

					output0 += outputOffset;
					output1 += outputOffset;
					output2 += outputOffset;
					output3 += outputOffset;
					if( LH_DATA_OUT_COUNT_4 ){
					output4 += outputOffset;
					}
					if ((*input0 ^ ein_cache[0]) || (*input1 ^ ein_cache[1]) || (*input2 ^ ein_cache[2]) || (*input3 ^ ein_cache[3]))
					{
						break;
					}
					*output0 = output0[-outputOffset];
					*output1 = output1[-outputOffset];
					*output2 = output2[-outputOffset];
					if( LH_DATA_OUT_COUNT_4 ){
					*output3 = output3[-outputOffset];
					*output4 = (LH_DATA_OUT_TYPE)*input4;
					}else{
					*output3 = (LH_DATA_OUT_TYPE)*input4;
					}
				}
			}
		}
		if (--LineCount)
		{
			j++;
			input0 = (LH_DATA_IN_TYPE *)((LH_UINT8 *)calcParam->inputData[0] + j * calcParam->cmInputBytesPerLine);
			input1 = (LH_DATA_IN_TYPE *)((LH_UINT8 *)calcParam->inputData[1] + j * calcParam->cmInputBytesPerLine);
			input2 = (LH_DATA_IN_TYPE *)((LH_UINT8 *)calcParam->inputData[2] + j * calcParam->cmInputBytesPerLine);
			input3 = (LH_DATA_IN_TYPE *)((LH_UINT8 *)calcParam->inputData[3] + j * calcParam->cmInputBytesPerLine);
			input4 = (LH_DATA_IN_TYPE *)((LH_UINT8 *)calcParam->inputData[4] + j * calcParam->cmInputBytesPerLine);

			output0 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[0] + j * calcParam->cmOutputBytesPerLine);
			output1 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[1] + j * calcParam->cmOutputBytesPerLine);
			output2 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[2] + j * calcParam->cmOutputBytesPerLine);
			output3 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[3] + j * calcParam->cmOutputBytesPerLine);
			if( LH_DATA_OUT_COUNT_4 ){
			output4 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[4] + j * calcParam->cmOutputBytesPerLine);
			}
		}
	}
	LH_END_PROC(LH_CALC_PROC_NAME)
	return 0;

#else

 	LH_UINT32 ein_regY;
	LH_UINT32 ein_regM;
	LH_UINT32 ein_regC;
	LH_UINT32 ein_regK;
	LH_LUT_DATA_TYPE * paNewVal0;
	LH_UINT32 ako0;
	LH_UINT32 ako1;
	LH_UINT32 ako2;
	LH_UINT32 ako3;

	LH_UINT8 	Mode;
	LH_UINT32 	PixelCount, LineCount, i, j;
	LH_UINT8 	LH_DATA_OUT_COUNT;
	long inputOffset,outputOffset;
	LH_DATA_IN_TYPE * input0 = (LH_DATA_IN_TYPE *)calcParam->inputData[0];
	LH_DATA_IN_TYPE * input1 = (LH_DATA_IN_TYPE *)calcParam->inputData[1];
	LH_DATA_IN_TYPE * input2 = (LH_DATA_IN_TYPE *)calcParam->inputData[2];
	LH_DATA_IN_TYPE * input3 = (LH_DATA_IN_TYPE *)calcParam->inputData[3];
	LH_DATA_IN_TYPE * input4 = (LH_DATA_IN_TYPE *)calcParam->inputData[4];

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

	if( LH_DATA_OUT_COUNT_4 ){
		LH_DATA_OUT_COUNT 		= 4;
	}else{
		LH_DATA_OUT_COUNT 		= 3;
	}
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
				aValIn = (ein_cache[0]=*input0) - ( *input0 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 0 << LH_ADR_BREIT_EIN_LUT );
		       	ein_regC = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >>( 16-LH_ADR_BREIT_EIN_LUT );
		       	
				aValIn = (ein_cache[1]=*input1) - ( *input1 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 1 << LH_ADR_BREIT_EIN_LUT );
		       	ein_regM = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >> ( 16-LH_ADR_BREIT_EIN_LUT );
		       	
				aValIn = (ein_cache[2]=*input2) - ( *input2 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 2 << LH_ADR_BREIT_EIN_LUT );
		       	ein_regY = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >> ( 16-LH_ADR_BREIT_EIN_LUT );

				aValIn = (ein_cache[3]=*input3) - ( *input3 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 3 << LH_ADR_BREIT_EIN_LUT );
		       	ein_regK = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >> ( 16-LH_ADR_BREIT_EIN_LUT );
			#else
			ein_regC = My_InputLut[(ein_cache[0]=*input0) + ( 0 << LH_ADR_BREIT_EIN_LUT )];
			ein_regM = My_InputLut[(ein_cache[1]=*input1) + ( 1 << LH_ADR_BREIT_EIN_LUT )];
			ein_regY = My_InputLut[(ein_cache[2]=*input2) + ( 2 << LH_ADR_BREIT_EIN_LUT )];
			ein_regK = My_InputLut[(ein_cache[3]=*input3) + ( 3 << LH_ADR_BREIT_EIN_LUT )];
			#endif
			#else
			#if LH_DATA_IN_SIZE_16
	       		register LH_DATA_IN_TYPE aValIn;
    	   		register LH_UINT32 ko;
				aValIn = (ein_cache[0]=*input0) - ( *input0 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 0 << LH_ADR_BREIT_EIN_LUT );
		       	ein_regC = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >>( 10-LH_ADR_BREIT_EIN_LUT );
		       	
				aValIn = (ein_cache[1]=*input1) - ( *input1 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 1 << LH_ADR_BREIT_EIN_LUT );
		       	ein_regM = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >> ( 10-LH_ADR_BREIT_EIN_LUT );
		       	
				aValIn = (ein_cache[2]=*input2) - ( *input2 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 2 << LH_ADR_BREIT_EIN_LUT );
		       	ein_regY = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >> ( 10-LH_ADR_BREIT_EIN_LUT );

				aValIn = (ein_cache[3]=*input3) - ( *input3 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 3 << LH_ADR_BREIT_EIN_LUT );
		       	ein_regK = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >> ( 10-LH_ADR_BREIT_EIN_LUT );
			/*ein_regC = My_InputLut[(*input0>>( 10-LH_ADR_BREIT_EIN_LUT )) + ( 0 << LH_ADR_BREIT_EIN_LUT )];
			ein_regM = My_InputLut[(*input1>>( 10-LH_ADR_BREIT_EIN_LUT )) + ( 1 << LH_ADR_BREIT_EIN_LUT )];
			ein_regY = My_InputLut[(*input2>>( 10-LH_ADR_BREIT_EIN_LUT )) + ( 2 << LH_ADR_BREIT_EIN_LUT )];
			ein_regK = My_InputLut[(*input3>>( 10-LH_ADR_BREIT_EIN_LUT )) + ( 3 << LH_ADR_BREIT_EIN_LUT )];*/
			#else
			ein_regC = My_InputLut[(ein_cache[0]=*input0) + ( 0 << LH_ADR_BREIT_EIN_LUT )];
			ein_regM = My_InputLut[(ein_cache[1]=*input1) + ( 1 << LH_ADR_BREIT_EIN_LUT )];
			ein_regY = My_InputLut[(ein_cache[2]=*input2) + ( 2 << LH_ADR_BREIT_EIN_LUT )];
			ein_regK = My_InputLut[(ein_cache[3]=*input3) + ( 3 << LH_ADR_BREIT_EIN_LUT )];
			#endif
			#endif
			paNewVal0 = (LH_LUT_DATA_TYPE *)My_ColorLut +
						((((((((ein_regC & LH_BIT_MASKE_ADR) << LH_BIT_BREIT_ADR) +
							   (ein_regM & LH_BIT_MASKE_ADR)) << LH_BIT_BREIT_ADR) +
							   (ein_regY & LH_BIT_MASKE_ADR))>> (LH_BIT_BREIT_SELEKTOR-LH_BIT_BREIT_ADR)) +
						       (ein_regK >> LH_BIT_BREIT_SELEKTOR))*LH_DATA_OUT_COUNT);
			ein_regC &= LH_BIT_MASKE_SELEKTOR;
			ein_regM &= LH_BIT_MASKE_SELEKTOR;
			ein_regY &= LH_BIT_MASKE_SELEKTOR;
			ein_regK &= LH_BIT_MASKE_SELEKTOR;
			if (ein_regY >= ein_regC)
			{
		        if( ein_regM >= ein_regC )
		        {
		            if( ein_regY >= ein_regM )	 	   /*  YMCK !*/
		            {	   		
		            	if( ein_regC >= ein_regK )
		            	{
							ako0 = LH_ADR_BEREICH_SEL - ein_regY;
							ako1 = ein_regY - ein_regM;
							ako2 = ein_regM - ein_regC;
							ako3 = ein_regC - ein_regK;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
						}
						else if(ein_regM >= ein_regK )	/*  YMKC !*/
						{	 	
							ako0 = LH_ADR_BEREICH_SEL - ein_regY;
							ako1 = ein_regY - ein_regM;
							ako2 = ein_regM - ein_regK;
							ako3 = ein_regK - ein_regC;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
						}
						else if(ein_regY >= ein_regK )	/*  YKMC !*/
						{	
							ako0 = LH_ADR_BEREICH_SEL - ein_regY;
							ako1 = ein_regY - ein_regK;
							ako2 = ein_regK - ein_regM;
							ako3 = ein_regM - ein_regC;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
						}
						else{	 						/*  KYMC !*/
							ako0 = LH_ADR_BEREICH_SEL - ein_regK;
							ako1 = ein_regK - ein_regY;
							ako2 = ein_regY - ein_regM;
							ako3 = ein_regM - ein_regC;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
						}	
		            }
		            else
		            { 								/*  MYCK !*/
		            	if( ein_regC >= ein_regK )
		            	{				
							ako0 = LH_ADR_BEREICH_SEL - ein_regM;
							ako1 = ein_regM - ein_regY;
							ako2 = ein_regY - ein_regC;						
							ako3 = ein_regC - ein_regK;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
						}
						else if(ein_regY >= ein_regK )	/*  MYKC !*/
						{	 	
							ako0 = LH_ADR_BEREICH_SEL - ein_regM;
							ako1 = ein_regM - ein_regY;
							ako2 = ein_regY - ein_regK;
							ako3 = ein_regK - ein_regC;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
						}
						else if(ein_regM >= ein_regK )	/*  MKYC !*/
						{	 	
							ako0 = LH_ADR_BEREICH_SEL - ein_regM;
							ako1 = ein_regM - ein_regK;
							ako2 = ein_regK - ein_regY;
							ako3 = ein_regY - ein_regC;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
						}
						else
						{							/*  KMYC !*/
							ako0 = LH_ADR_BEREICH_SEL - ein_regK;
							ako1 = ein_regK - ein_regM;
							ako2 = ein_regM - ein_regY;
							ako3 = ein_regY - ein_regC;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
		                }
					}
	            }
	            else
	            { 									/*  YCMK !*/
	            	if( ein_regM >= ein_regK )
	            	{	
						ako0 = LH_ADR_BEREICH_SEL - ein_regY;
						ako1 = ein_regY - ein_regC;
						ako2 = ein_regC - ein_regM;
						ako3 = ein_regM - ein_regK;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
					}
					else if(ein_regC >= ein_regK )	/*  YCKM !*/
					{	 	
						ako0 = LH_ADR_BEREICH_SEL - ein_regY;
						ako1 = ein_regY - ein_regC;
						ako2 = ein_regC - ein_regK;
						ako3 = ein_regK - ein_regM;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
					}
					else if(ein_regY >= ein_regK )	/*  YKCM !*/
					{	 	
						ako0 = LH_ADR_BEREICH_SEL - ein_regY;
						ako1 = ein_regY - ein_regK;
						ako2 = ein_regK - ein_regC;
						ako3 = ein_regC - ein_regM;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
					}
					else
					{						 	/*  KYCM !*/
						ako0 = LH_ADR_BEREICH_SEL - ein_regK;
						ako1 = ein_regK - ein_regY;
						ako2 = ein_regY - ein_regC;
						ako3 = ein_regC - ein_regM;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
	            	}
		        }
			}
	        else
	        {
            	if( ein_regM >= ein_regC )
            	{
          			if( ein_regY >= ein_regK )		/*  MCYK !*/
          			{	
						ako0 = LH_ADR_BEREICH_SEL - ein_regM;
						ako1 = ein_regM - ein_regC;
						ako2 = ein_regC - ein_regY;
						ako3 = ein_regY - ein_regK;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
					}
					else if(ein_regC >= ein_regK )	/*  MCKY !*/
					{	 	
						ako0 = LH_ADR_BEREICH_SEL - ein_regM;
						ako1 = ein_regM - ein_regC;
						ako2 = ein_regC - ein_regK;
						ako3 = ein_regK - ein_regY;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
				}
					else if(ein_regM >= ein_regK )	/*  MKCY !*/
					{	 	
						ako0 = LH_ADR_BEREICH_SEL - ein_regM;
						ako1 = ein_regM - ein_regK;
						ako2 = ein_regK - ein_regC;
						ako3 = ein_regC - ein_regY;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
					}
					else
					{						 	/*  KMCY !*/
						ako0 = LH_ADR_BEREICH_SEL - ein_regK;
						ako1 = ein_regK - ein_regM;
						ako2 = ein_regM - ein_regC;
						ako3 = ein_regC - ein_regY;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
					}
                }
                else
                {
                    if( ein_regY >= ein_regM )
                    {
	          			if( ein_regM >= ein_regK )	/*  CYMK !*/
	          			{	
							ako0 = LH_ADR_BEREICH_SEL - ein_regC;
							ako1 = ein_regC - ein_regY;
							ako2 = ein_regY - ein_regM;
							ako3 = ein_regM - ein_regK;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
						}
						else if(ein_regY >= ein_regK )	/*  CYKM !*/
						{	
							ako0 = LH_ADR_BEREICH_SEL - ein_regC;
							ako1 = ein_regC - ein_regY;
							ako2 = ein_regY - ein_regK;
							ako3 = ein_regK - ein_regM;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
						}
						else if(ein_regC >= ein_regK )	/*  CKYM */
						{	
							ako0 = LH_ADR_BEREICH_SEL - ein_regC;
							ako1 = ein_regC - ein_regK;
							ako2 = ein_regK - ein_regY;
							ako3 = ein_regY - ein_regM;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
						}
						else
						{							/*  KCYM !*/
							ako0 = LH_ADR_BEREICH_SEL - ein_regK;
							ako1 = ein_regK - ein_regC;
							ako2 = ein_regC - ein_regY;
							ako3 = ein_regY - ein_regM;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
						}
                    }
                    else if( ein_regY >= ein_regK )		/*  CMYK !*/
                    {	
						ako0 = LH_ADR_BEREICH_SEL - ein_regC;
						ako1 = ein_regC - ein_regM;
						ako2 = ein_regM - ein_regY;
						ako3 = ein_regY - ein_regK;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
					}
					else if(ein_regM >= ein_regK )	/*  CMKY !*/
					{	
						ako0 = LH_ADR_BEREICH_SEL - ein_regC;
						ako1 = ein_regC - ein_regM;
						ako2 = ein_regM - ein_regK;
						ako3 = ein_regK - ein_regY;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
					}
					else
					{
						if(ein_regC >= ein_regK )	/*  CKMY !*/
						{	
							ako0 = LH_ADR_BEREICH_SEL - ein_regC;
							ako1 = ein_regC - ein_regK;
							ako2 = ein_regK - ein_regM;
							ako3 = ein_regM - ein_regY;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
						}
						else
						{							/*  KCMY !*/
							ako0 = LH_ADR_BEREICH_SEL - ein_regK;
							ako1 = ein_regK - ein_regC;
							ako2 = ein_regC - ein_regM;
							ako3 = ein_regM - ein_regY;
							#if LH_LUT_DATA_SIZE_16
							#if LH_DATA_OUT_SIZE_16
			   		
							aVal =					 (	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
					       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
					       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							aVal =					 (	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
					       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
	
							if( LH_DATA_OUT_COUNT_4 ){
							aVal =					 (	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] );
							aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
							aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
							ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
							aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
					       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
							}
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							}
							#endif
							
							#else
							*output0 = My_OutputLut[((	ako0 * paNewVal0[0] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+0] )>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
							*output1 = My_OutputLut[((	ako0 * paNewVal0[1] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+1] )>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
							*output2 = My_OutputLut[((	ako0 * paNewVal0[2] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+2] )>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
							if( LH_DATA_OUT_COUNT_4 ){
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] +
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] +
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
							}
							#endif
		                }
					}
		    	}
			}
			#if LH_DATA_OUT_SIZE_16 && ! LH_LUT_DATA_SIZE_16
			*output0 |= (*output0 << 8);
			*output1 |= (*output1 << 8);
			*output2 |= (*output2 << 8);
			if( LH_DATA_OUT_COUNT_4 ){
			*output3 |= (*output3 << 8);
			}
			#endif
			
			if (Mode == LH_CALC_ENGINE_P_TO_P)
			{
				while (--i)
				{
					input0 += inputOffset;
					input1 += inputOffset;
					input2 += inputOffset;
					input3 += inputOffset;
					output0 += outputOffset;
					output1 += outputOffset;
					output2 += outputOffset;
					if( LH_DATA_OUT_COUNT_4 ){
					output3 += outputOffset;
					}

					if ((*input0 ^ ein_cache[0]) || (*input1 ^ ein_cache[1]) || (*input2 ^ ein_cache[2]) || (*input3 ^ ein_cache[3]))
					{
						break;
					}
					*output0 = output0[-outputOffset];
					*output1 = output1[-outputOffset];
					*output2 = output2[-outputOffset];
					if( LH_DATA_OUT_COUNT_4 ){
					*output3 = output3[-outputOffset];
					}
				}
			}
			else if (Mode == LH_CALC_ENGINE_P_TO_U)
			{
				if( LH_DATA_OUT_COUNT_4 ){
				*output4 &= Mask;
				}else{
				*output3 &= Mask;
				}
				while (--i)
				{
					input0 += inputOffset;
					input1 += inputOffset;
					input2 += inputOffset;
					input3 += inputOffset;
					output0 += outputOffset;
					output1 += outputOffset;
					output2 += outputOffset;
					output3 += outputOffset;
					if( LH_DATA_OUT_COUNT_4 ){
					output4 += outputOffset;
					}
					if ((*input0 ^ ein_cache[0]) || (*input1 ^ ein_cache[1]) || (*input2 ^ ein_cache[2]) || (*input3 ^ ein_cache[3]))
					{
						break;
					}
					*output0 = output0[-outputOffset];
					*output1 = output1[-outputOffset];
					*output2 = output2[-outputOffset];
					if( LH_DATA_OUT_COUNT_4 ){
					*output3 = output3[-outputOffset];
					*output4 &= Mask;
					}else{
					*output3 &= Mask;
					}
				}
			}
			else
			{
				if( LH_DATA_OUT_COUNT_4 ){
				*output4 = (LH_DATA_OUT_TYPE)*input4;
				}else{
				*output3 = (LH_DATA_OUT_TYPE)*input4;
				}
				while (--i)
				{								/*U_TO_U*/
					input0 += inputOffset;
					input1 += inputOffset;
					input2 += inputOffset;
					input3 += inputOffset;
					input4 += inputOffset;

					output0 += outputOffset;
					output1 += outputOffset;
					output2 += outputOffset;
					output3 += outputOffset;
					if( LH_DATA_OUT_COUNT_4 ){
					output4 += outputOffset;
					}
					if ((*input0 ^ ein_cache[0]) || (*input1 ^ ein_cache[1]) || (*input2 ^ ein_cache[2]) || (*input3 ^ ein_cache[3]))
					{
						break;
					}
					*output0 = output0[-outputOffset];
					*output1 = output1[-outputOffset];
					*output2 = output2[-outputOffset];
					if( LH_DATA_OUT_COUNT_4 ){
					*output3 = output3[-outputOffset];
					*output4 = (LH_DATA_OUT_TYPE)*input4;
					}else{
					*output3 = (LH_DATA_OUT_TYPE)*input4;
					}
				}
			}
		}
		if (--LineCount)
		{
			j++;
			input0 = (LH_DATA_IN_TYPE *)((LH_UINT8 *)calcParam->inputData[0] + j * calcParam->cmInputBytesPerLine);
			input1 = (LH_DATA_IN_TYPE *)((LH_UINT8 *)calcParam->inputData[1] + j * calcParam->cmInputBytesPerLine);
			input2 = (LH_DATA_IN_TYPE *)((LH_UINT8 *)calcParam->inputData[2] + j * calcParam->cmInputBytesPerLine);
			input3 = (LH_DATA_IN_TYPE *)((LH_UINT8 *)calcParam->inputData[3] + j * calcParam->cmInputBytesPerLine);
			input4 = (LH_DATA_IN_TYPE *)((LH_UINT8 *)calcParam->inputData[4] + j * calcParam->cmInputBytesPerLine);

			output0 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[0] + j * calcParam->cmOutputBytesPerLine);
			output1 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[1] + j * calcParam->cmOutputBytesPerLine);
			output2 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[2] + j * calcParam->cmOutputBytesPerLine);
			output3 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[3] + j * calcParam->cmOutputBytesPerLine);
			if( LH_DATA_OUT_COUNT_4 ){
			output4 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[4] + j * calcParam->cmOutputBytesPerLine);
			}
		}
	}

#endif
}
#else
{

#if smallCode
	
	LH_UINT32 var_0_0_1;
	LH_UINT32 var_0_1_1;
	LH_UINT32 var_1_1_1;
	LH_UINT32 var_0_1_0;
	LH_UINT32 var_1_0_1;
	LH_UINT32 var_1_1_0;
	LH_UINT32 var_1_0_0;

 	LH_UINT32 ein_regb;
	LH_UINT32 ein_regg;
	LH_UINT32 ein_regr;
	LH_DATA_IN_TYPE ein_cache[3];
	LH_LUT_DATA_TYPE * paNewVal0;
	LH_UINT32 ako0;
	LH_UINT32 ako1;
	LH_UINT32 ako2;

	LH_UINT8 	Mode;
	LH_UINT32 	PixelCount, LineCount, i, j;
	LH_UINT8 	LH_DATA_OUT_COUNT;
	long inputOffset,outputOffset;
	LH_DATA_IN_TYPE * input0 = (LH_DATA_IN_TYPE *)calcParam->inputData[0];
	LH_DATA_IN_TYPE * input1 = (LH_DATA_IN_TYPE *)calcParam->inputData[1];
	LH_DATA_IN_TYPE * input2 = (LH_DATA_IN_TYPE *)calcParam->inputData[2];
	LH_DATA_IN_TYPE * input3 = (LH_DATA_IN_TYPE *)calcParam->inputData[3];

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

	if( LH_DATA_OUT_COUNT_4 ){
		LH_DATA_OUT_COUNT 		= 4;
	}else{
		LH_DATA_OUT_COUNT 		= 3;
	}

	var_0_0_1 = ((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT;
	var_0_1_1 = ((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT;
	var_1_1_1 = ((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT;
	var_0_1_0 = ((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT;
	var_1_0_1 = ((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT;
	var_1_1_0 = ((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT;
	var_1_0_0 = ((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT;

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
				aValIn = (ein_cache[0]=*input0) - ( *input0 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (LH_DATA_IN_TYPE)((aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 0 << LH_ADR_BREIT_EIN_LUT ));
		       	ein_regr = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >>( 16-LH_ADR_BREIT_EIN_LUT );
		       	
				aValIn = (ein_cache[1]=*input1) - ( *input1 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (LH_DATA_IN_TYPE)((aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 1 << LH_ADR_BREIT_EIN_LUT ));
		       	ein_regg = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >> ( 16-LH_ADR_BREIT_EIN_LUT );
		       	
				aValIn = (ein_cache[2]=*input2) - ( *input2 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (LH_DATA_IN_TYPE)((aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 2 << LH_ADR_BREIT_EIN_LUT ));
		       	ein_regb = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >> ( 16-LH_ADR_BREIT_EIN_LUT );
			#else
			ein_regr = My_InputLut[(ein_cache[0]=*input0) + ( 0 << LH_ADR_BREIT_EIN_LUT )];
			ein_regg = My_InputLut[(ein_cache[1]=*input1) + ( 1 << LH_ADR_BREIT_EIN_LUT )];
			ein_regb = My_InputLut[(ein_cache[2]=*input2) + ( 2 << LH_ADR_BREIT_EIN_LUT )];
			#endif
			#else
			#if LH_DATA_IN_SIZE_16
	       		register LH_DATA_IN_TYPE aValIn;
    	   		register LH_UINT32 ko;
				aValIn = (ein_cache[0]=*input0) - ( *input0 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (LH_DATA_IN_TYPE)((aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 0 << LH_ADR_BREIT_EIN_LUT ));
		       	ein_regr = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >>( 10-LH_ADR_BREIT_EIN_LUT );
		       	
				aValIn = (ein_cache[1]=*input1) - ( *input1 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (LH_DATA_IN_TYPE)((aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 1 << LH_ADR_BREIT_EIN_LUT ));
		       	ein_regg = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >> ( 10-LH_ADR_BREIT_EIN_LUT );
		       	
				aValIn = (ein_cache[2]=*input2) - ( *input2 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (LH_DATA_IN_TYPE)((aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 2 << LH_ADR_BREIT_EIN_LUT ));
		       	ein_regb = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >> ( 10-LH_ADR_BREIT_EIN_LUT );
			/*ein_regr = My_InputLut[(*input0>>( 10-LH_ADR_BREIT_EIN_LUT )) + ( 0 << LH_ADR_BREIT_EIN_LUT )];
			ein_regg = My_InputLut[(*input1>>( 10-LH_ADR_BREIT_EIN_LUT )) + ( 1 << LH_ADR_BREIT_EIN_LUT )];
			ein_regb = My_InputLut[(*input2>>( 10-LH_ADR_BREIT_EIN_LUT )) + ( 2 << LH_ADR_BREIT_EIN_LUT )];*/
			#else
			ein_regr = My_InputLut[(ein_cache[0]=*input0) + ( 0 << LH_ADR_BREIT_EIN_LUT )];
			ein_regg = My_InputLut[(ein_cache[1]=*input1) + ( 1 << LH_ADR_BREIT_EIN_LUT )];
			ein_regb = My_InputLut[(ein_cache[2]=*input2) + ( 2 << LH_ADR_BREIT_EIN_LUT )];
			#endif
			#endif
			paNewVal0 = (LH_LUT_DATA_TYPE *)My_ColorLut +
						((((((ein_regr & LH_BIT_MASKE_ADR) << LH_BIT_BREIT_ADR) +
							 (ein_regg & LH_BIT_MASKE_ADR))>> (LH_BIT_BREIT_SELEKTOR-LH_BIT_BREIT_ADR)) +
						     (ein_regb >> LH_BIT_BREIT_SELEKTOR))*LH_DATA_OUT_COUNT);
			ein_regr &= LH_BIT_MASKE_SELEKTOR;
			ein_regg &= LH_BIT_MASKE_SELEKTOR;
			ein_regb &= LH_BIT_MASKE_SELEKTOR;
			if (ein_regb >= ein_regr)
			{
				if (ein_regg >= ein_regr)
				{
					if (ein_regb >= ein_regg)
					{
						ako0 = (LH_ADR_BEREICH_SEL) - ein_regb;
						ako1 = ein_regb - ein_regg;
						ako2 = ein_regg - ein_regr;
						ein_regb = ein_regr;
						ein_regr = var_0_0_1;
						ein_regg = var_0_1_1;
					}
					else
					{
						ako0 = (LH_ADR_BEREICH_SEL) - ein_regg;
						ako1 = ein_regg - ein_regb;
						ako2 = ein_regb - ein_regr;
						ein_regb = ein_regr;
						ein_regr = var_0_1_0;
						ein_regg = var_0_1_1;
					}
				}
				else
				{
						ako0 = (LH_ADR_BEREICH_SEL) - ein_regb;
						ako1 = ein_regb - ein_regr;
						ako2 = ein_regr - ein_regg;
						ein_regb = ein_regg;
						ein_regr = var_0_0_1;
						ein_regg = var_1_0_1;
				}
			}
			else
			{
				if (ein_regg >= ein_regr)
				{
					ako0 = (LH_ADR_BEREICH_SEL) - ein_regg;
					ako1 = ein_regg - ein_regr;
					ako2 = ein_regr - ein_regb;
						ein_regr = var_0_1_0;
						ein_regg = var_1_1_0;
				}
				else
				{
					if (ein_regb >= ein_regg)
					{
						ako0 = (LH_ADR_BEREICH_SEL) - ein_regr;
						ako1 = ein_regr - ein_regb;
						ako2 = ein_regb - ein_regg;
						ein_regb = ein_regg;
						ein_regr = var_1_0_0;
						ein_regg = var_1_0_1;
					}
					else
					{
						ako0 = (LH_ADR_BEREICH_SEL) - ein_regr;
						ako1 = ein_regr - ein_regg;
						ako2 = ein_regg - ein_regb;
						ein_regr = var_1_0_0;
						ein_regg = var_1_1_0;
					}
				}
			}

#if LH_LUT_DATA_SIZE_16
#if LH_DATA_OUT_SIZE_16
			aVal =					 (ako0 		* paNewVal0[0] +
									  ako1 		* paNewVal0[ein_regr] +
									  ako2 		* paNewVal0[ein_regg] +
									  ein_regb	* paNewVal0[var_1_1_1]);
			aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
			aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
			ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
			aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
	       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

			paNewVal0++;
			aVal =					 (ako0 		* paNewVal0[0] +
									  ako1 		* paNewVal0[ein_regr] +
									  ako2 		* paNewVal0[ein_regg] +
									  ein_regb	* paNewVal0[var_1_1_1]);
			aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
			aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
			ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
			aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
	       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

			paNewVal0++;
			aVal =					 (ako0 		* paNewVal0[0] +
									  ako1 		* paNewVal0[ein_regr] +
									  ako2 		* paNewVal0[ein_regg] +
									  ein_regb	* paNewVal0[var_1_1_1]);
			aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
			aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
			ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
			aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
	       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

			if( LH_DATA_OUT_COUNT_4 ){
			paNewVal0++;
			aVal =					 (ako0 		* paNewVal0[0] +
									  ako1 		* paNewVal0[ein_regr] +
									  ako2 		* paNewVal0[ein_regg] +
									  ein_regb	* paNewVal0[var_1_1_1]);
			aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
			aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
			ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
			aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
	       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
			}
#else
			*output0 = My_OutputLut[((ako0 		* paNewVal0[0] +
									  ako1 		* paNewVal0[ein_regr] +
									  ako2 		* paNewVal0[ein_regg] +
									  ein_regb	* paNewVal0[var_1_1_1])>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
			paNewVal0++;
			*output1 = My_OutputLut[((ako0 		* paNewVal0[0] +
									  ako1 		* paNewVal0[ein_regr] +
									  ako2 		* paNewVal0[ein_regg] +
									  ein_regb	* paNewVal0[var_1_1_1])>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
			paNewVal0++;
			*output2 = My_OutputLut[((ako0	 	* paNewVal0[0] +
								      ako1 		* paNewVal0[ein_regr] +
								      ako2 		* paNewVal0[ein_regg] +
								      ein_regb 	* paNewVal0[var_1_1_1])>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
			if( LH_DATA_OUT_COUNT_4 ){
			paNewVal0++;
			*output3 = My_OutputLut[((ako0 		* paNewVal0[0] +
								      ako1 		* paNewVal0[ein_regr] +
								      ako2 		* paNewVal0[ein_regg] +
								      ein_regb 	* paNewVal0[var_1_1_1])>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
			}
#endif

#else

			*output0 = My_OutputLut[((ako0 * paNewVal0[0] +
				ako1 * paNewVal0[ein_regr] +
				ako2 * paNewVal0[ein_regg] +
				ein_regb * paNewVal0[var_1_1_1])>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];

			paNewVal0++;
			*output1 = My_OutputLut[((ako0 * paNewVal0[0] +
				ako1 * paNewVal0[ein_regr] +
				ako2 * paNewVal0[ein_regg] +
				ein_regb * paNewVal0[var_1_1_1])>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];

			paNewVal0++;
			*output2 = My_OutputLut[((ako0 * paNewVal0[0] +
				ako1 * paNewVal0[ein_regr] +
				ako2 * paNewVal0[ein_regg] +
				ein_regb * paNewVal0[var_1_1_1])>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];

			if( LH_DATA_OUT_COUNT_4 ){
				paNewVal0++;
				*output3 = My_OutputLut[((ako0 * paNewVal0[0] +
					ako1 * paNewVal0[ein_regr] +
					ako2 * paNewVal0[ein_regg] +
					ein_regb * paNewVal0[var_1_1_1])>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
			}
#endif

			#if LH_DATA_OUT_SIZE_16 && ! LH_LUT_DATA_SIZE_16
			*output0 |= (*output0 << 8);
			*output1 |= (*output1 << 8);
			*output2 |= (*output2 << 8);
			if( LH_DATA_OUT_COUNT_4 ){
			*output3 |= (*output3 << 8);
			}
			#endif
			
			if (Mode == LH_CALC_ENGINE_P_TO_P)
			{
				while (--i)
				{
					input0 += inputOffset;
					input1 += inputOffset;
					input2 += inputOffset;
					output0 += outputOffset;
					output1 += outputOffset;
					output2 += outputOffset;
					if( LH_DATA_OUT_COUNT_4 ){
					output3 += outputOffset;
					}

					if ((*input0 ^ ein_cache[0]) || (*input1 ^ ein_cache[1]) || (*input2 ^ ein_cache[2]))
					{
						break;
					}
					*output0 = output0[-outputOffset];
					*output1 = output1[-outputOffset];
					*output2 = output2[-outputOffset];
					if( LH_DATA_OUT_COUNT_4 ){
					*output3 = output3[-outputOffset];
					}
				}
			}
			else if (Mode == LH_CALC_ENGINE_P_TO_U)
			{
				if( LH_DATA_OUT_COUNT_4 ){
				*output4 &= Mask;
				}else{
				*output3 &= Mask;
				}
				while (--i)
				{
					input0 += inputOffset;
					input1 += inputOffset;
					input2 += inputOffset;
					output0 += outputOffset;
					output1 += outputOffset;
					output2 += outputOffset;
					output3 += outputOffset;
					if( LH_DATA_OUT_COUNT_4 ){
					output4 += outputOffset;
					}
					if ((*input0 ^ ein_cache[0]) || (*input1 ^ ein_cache[1]) || (*input2 ^ ein_cache[2]))
					{
						break;
					}
					*output0 = output0[-outputOffset];
					*output1 = output1[-outputOffset];
					*output2 = output2[-outputOffset];
					if( LH_DATA_OUT_COUNT_4 ){
					*output3 = output3[-outputOffset];
					*output4 &= Mask;
					}else{
					*output3 &= Mask;
					}
				}
			}
			else
			{
				if( LH_DATA_OUT_COUNT_4 ){
				*output4 = (LH_DATA_OUT_TYPE)*input3;
				}else{
				*output3 = (LH_DATA_OUT_TYPE)*input3;
				}
				while (--i)
				{								/*U_TO_U*/
					input0 += inputOffset;
					input1 += inputOffset;
					input2 += inputOffset;
					input3 += inputOffset;

					output0 += outputOffset;
					output1 += outputOffset;
					output2 += outputOffset;
					output3 += outputOffset;
					if( LH_DATA_OUT_COUNT_4 ){
					output4 += outputOffset;
					}
					if ((*input0 ^ ein_cache[0]) || (*input1 ^ ein_cache[1]) || (*input2 ^ ein_cache[2]))
					{
						break;
					}
					*output0 = output0[-outputOffset];
					*output1 = output1[-outputOffset];
					*output2 = output2[-outputOffset];
					if( LH_DATA_OUT_COUNT_4 ){
					*output3 = output3[-outputOffset];
					*output4 = (LH_DATA_OUT_TYPE)*input3;
					}else{
					*output3 = (LH_DATA_OUT_TYPE)*input3;
					}
				}
			}
		}
		if (--LineCount)
		{
			j++;
			input0 = (LH_DATA_IN_TYPE *)((LH_UINT8 *)calcParam->inputData[0] + j * calcParam->cmInputBytesPerLine);
			input1 = (LH_DATA_IN_TYPE *)((LH_UINT8 *)calcParam->inputData[1] + j * calcParam->cmInputBytesPerLine);
			input2 = (LH_DATA_IN_TYPE *)((LH_UINT8 *)calcParam->inputData[2] + j * calcParam->cmInputBytesPerLine);
			input3 = (LH_DATA_IN_TYPE *)((LH_UINT8 *)calcParam->inputData[3] + j * calcParam->cmInputBytesPerLine);

			output0 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[0] + j * calcParam->cmOutputBytesPerLine);
			output1 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[1] + j * calcParam->cmOutputBytesPerLine);
			output2 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[2] + j * calcParam->cmOutputBytesPerLine);
			output3 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[3] + j * calcParam->cmOutputBytesPerLine);
			if( LH_DATA_OUT_COUNT_4 ){
			output4 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[4] + j * calcParam->cmOutputBytesPerLine);
			}
		}
	}
	LH_END_PROC(LH_CALC_PROC_NAME)
	return 0;
#else
 	LH_UINT32 ein_regb;
	LH_UINT32 ein_regg;
	LH_UINT32 ein_regr;
	LH_LUT_DATA_TYPE * paNewVal0;
	LH_UINT32 ako0;
	LH_UINT32 ako1;
	LH_UINT32 ako2;

	LH_UINT8 	Mode;
	LH_UINT32 	PixelCount, LineCount, i, j;
	LH_UINT8 	LH_DATA_OUT_COUNT;
	long inputOffset,outputOffset;
	LH_DATA_IN_TYPE * input0 = (LH_DATA_IN_TYPE *)calcParam->inputData[0];
	LH_DATA_IN_TYPE * input1 = (LH_DATA_IN_TYPE *)calcParam->inputData[1];
	LH_DATA_IN_TYPE * input2 = (LH_DATA_IN_TYPE *)calcParam->inputData[2];
	LH_DATA_IN_TYPE * input3 = (LH_DATA_IN_TYPE *)calcParam->inputData[3];

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

	if( LH_DATA_OUT_COUNT_4 ){
		LH_DATA_OUT_COUNT 		= 4;
	}else{
		LH_DATA_OUT_COUNT 		= 3;
	}
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
				aValIn = (ein_cache[0]=*input0) - ( *input0 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 0 << LH_ADR_BREIT_EIN_LUT );
		       	ein_regr = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >>( 16-LH_ADR_BREIT_EIN_LUT );
		       	
				aValIn = (ein_cache[1]=*input1) - ( *input1 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 1 << LH_ADR_BREIT_EIN_LUT );
		       	ein_regg = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >> ( 16-LH_ADR_BREIT_EIN_LUT );
		       	
				aValIn = (ein_cache[2]=*input2) - ( *input2 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 2 << LH_ADR_BREIT_EIN_LUT );
		       	ein_regb = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >> ( 16-LH_ADR_BREIT_EIN_LUT );
			#else
			ein_regr = My_InputLut[(ein_cache[0]=*input0) + ( 0 << LH_ADR_BREIT_EIN_LUT )];
			ein_regg = My_InputLut[(ein_cache[1]=*input1) + ( 1 << LH_ADR_BREIT_EIN_LUT )];
			ein_regb = My_InputLut[(ein_cache[2]=*input2) + ( 2 << LH_ADR_BREIT_EIN_LUT )];
			#endif
			#else
			#if LH_DATA_IN_SIZE_16
	       		register LH_DATA_IN_TYPE aValIn;
    	   		register LH_UINT32 ko;
				aValIn = (ein_cache[0]=*input0) - ( *input0 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 0 << LH_ADR_BREIT_EIN_LUT );
		       	ein_regr = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >>( 10-LH_ADR_BREIT_EIN_LUT );
		       	
				aValIn = (ein_cache[1]=*input1) - ( *input1 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 1 << LH_ADR_BREIT_EIN_LUT );
		       	ein_regg = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >> ( 10-LH_ADR_BREIT_EIN_LUT );
		       	
				aValIn = (ein_cache[2]=*input2) - ( *input2 >> ( LH_ADR_BREIT_EIN_LUT ));
				ko = aValIn & ( (1<<( 16-LH_ADR_BREIT_EIN_LUT ))-1 );
				aValIn = (aValIn >> ( 16-LH_ADR_BREIT_EIN_LUT )) + ( 2 << LH_ADR_BREIT_EIN_LUT );
		       	ein_regb = ( My_InputLut[aValIn] * ( (1<<( 16-LH_ADR_BREIT_EIN_LUT )) - ko ) + My_InputLut[aValIn +1] * ko ) >> ( 10-LH_ADR_BREIT_EIN_LUT );
			/*ein_regr = My_InputLut[(*input0>>( 10-LH_ADR_BREIT_EIN_LUT )) + ( 0 << LH_ADR_BREIT_EIN_LUT )];
			ein_regg = My_InputLut[(*input1>>( 10-LH_ADR_BREIT_EIN_LUT )) + ( 1 << LH_ADR_BREIT_EIN_LUT )];
			ein_regb = My_InputLut[(*input2>>( 10-LH_ADR_BREIT_EIN_LUT )) + ( 2 << LH_ADR_BREIT_EIN_LUT )];*/
			#else
			ein_regr = My_InputLut[(ein_cache[0]=*input0) + ( 0 << LH_ADR_BREIT_EIN_LUT )];
			ein_regg = My_InputLut[(ein_cache[1]=*input1) + ( 1 << LH_ADR_BREIT_EIN_LUT )];
			ein_regb = My_InputLut[(ein_cache[2]=*input2) + ( 2 << LH_ADR_BREIT_EIN_LUT )];
			#endif
			#endif
			paNewVal0 = (LH_LUT_DATA_TYPE *)My_ColorLut +
						((((((ein_regr & LH_BIT_MASKE_ADR) << LH_BIT_BREIT_ADR) +
							 (ein_regg & LH_BIT_MASKE_ADR))>> (LH_BIT_BREIT_SELEKTOR-LH_BIT_BREIT_ADR)) +
						     (ein_regb >> LH_BIT_BREIT_SELEKTOR))*LH_DATA_OUT_COUNT);
			ein_regr &= LH_BIT_MASKE_SELEKTOR;
			ein_regg &= LH_BIT_MASKE_SELEKTOR;
			ein_regb &= LH_BIT_MASKE_SELEKTOR;
			if (ein_regb >= ein_regr)
			{
				if (ein_regg >= ein_regr)
				{
					if (ein_regb >= ein_regg)
					{
						ako0 = (LH_ADR_BEREICH_SEL) - ein_regb;
						ako1 = ein_regb - ein_regg;
						ako2 = ein_regg - ein_regr;
						#if LH_LUT_DATA_SIZE_16
						#if LH_DATA_OUT_SIZE_16
		   		
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regr	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
				       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

						paNewVal0++;
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regr	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
				       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

						paNewVal0++;
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regr	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
				       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

						if( LH_DATA_OUT_COUNT_4 ){
						paNewVal0++;
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regr	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
				       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
						}
						
						#else
						*output0 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regr	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
						paNewVal0++;
						*output1 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regr	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
						paNewVal0++;
						*output2 = My_OutputLut[((ako0	 	* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ein_regr 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
						if( LH_DATA_OUT_COUNT_4 ){
						paNewVal0++;
						*output3 = My_OutputLut[((ako0 		* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ein_regr 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
						}
						#endif
						
						#else
						*output0 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regr	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
						paNewVal0++;
						*output1 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regr	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
						paNewVal0++;
						*output2 = My_OutputLut[((ako0	 	* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ein_regr 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
						if( LH_DATA_OUT_COUNT_4 ){
						paNewVal0++;
						*output3 = My_OutputLut[((ako0 		* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ein_regr 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
						}
						#endif
					}
					else
					{
						ako0 = (LH_ADR_BEREICH_SEL) - ein_regg;
						ako1 = ein_regg - ein_regb;
						ako2 = ein_regb - ein_regr;
						#if LH_LUT_DATA_SIZE_16
						#if LH_DATA_OUT_SIZE_16
		   		
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regr	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
				       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

						paNewVal0++;
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regr	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
				       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

						paNewVal0++;
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regr	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
				       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

						if( LH_DATA_OUT_COUNT_4 ){
						paNewVal0++;
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regr	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
				       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
						}
						
						#else
						*output0 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regr	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
						paNewVal0++;
						*output1 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regr	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
						paNewVal0++;
						*output2 = My_OutputLut[((ako0	 	* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ein_regr 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
						if( LH_DATA_OUT_COUNT_4 ){
						paNewVal0++;
						*output3 = My_OutputLut[((ako0 		* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ein_regr 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
						}
						#endif
						
						#else
						*output0 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regr	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
						paNewVal0++;
						*output1 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regr	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
						paNewVal0++;
						*output2 = My_OutputLut[((ako0	 	* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ein_regr 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
						if( LH_DATA_OUT_COUNT_4 ){
						paNewVal0++;
						*output3 = My_OutputLut[((ako0 		* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ein_regr 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
						}
						#endif
					}
				}
				else
				{
						ako0 = (LH_ADR_BEREICH_SEL) - ein_regb;
						ako1 = ein_regb - ein_regr;
						ako2 = ein_regr - ein_regg;
						#if LH_LUT_DATA_SIZE_16
						#if LH_DATA_OUT_SIZE_16
		   		
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regg	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
				       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

						paNewVal0++;
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regg	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
				       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

						paNewVal0++;
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regg	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
				       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

						if( LH_DATA_OUT_COUNT_4 ){
						paNewVal0++;
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regg	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
				       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
						}
						
						#else
						*output0 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regg	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
						paNewVal0++;
						*output1 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regg	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
						paNewVal0++;
						*output2 = My_OutputLut[((ako0	 	* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ein_regg 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
						if( LH_DATA_OUT_COUNT_4 ){
						paNewVal0++;
						*output3 = My_OutputLut[((ako0 		* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ein_regg 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
						}
						#endif
						
						#else
						*output0 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regg	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
						paNewVal0++;
						*output1 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regg	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
						paNewVal0++;
						*output2 = My_OutputLut[((ako0	 	* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ein_regg 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
						if( LH_DATA_OUT_COUNT_4 ){
						paNewVal0++;
						*output3 = My_OutputLut[((ako0 		* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ein_regg 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
						}
						#endif
				}
			}
			else
			{
				if (ein_regg >= ein_regr)
				{
					ako0 = (LH_ADR_BEREICH_SEL) - ein_regg;
					ako1 = ein_regg - ein_regr;
					ako2 = ein_regr - ein_regb;
						#if LH_LUT_DATA_SIZE_16
						#if LH_DATA_OUT_SIZE_16
		   		
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ein_regb	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
				       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

						paNewVal0++;
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ein_regb	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
				       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

						paNewVal0++;
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ein_regb	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
				       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

						if( LH_DATA_OUT_COUNT_4 ){
						paNewVal0++;
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ein_regb	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
				       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
						}
						
						#else
						*output0 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ein_regb	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
						paNewVal0++;
						*output1 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ein_regb	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
						paNewVal0++;
						*output2 = My_OutputLut[((ako0	 	* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ein_regb 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
						if( LH_DATA_OUT_COUNT_4 ){
						paNewVal0++;
						*output3 = My_OutputLut[((ako0 		* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ein_regb 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
						}
						#endif
						
						#else
						*output0 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ein_regb	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
						paNewVal0++;
						*output1 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ein_regb	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
						paNewVal0++;
						*output2 = My_OutputLut[((ako0	 	* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ein_regb 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
						if( LH_DATA_OUT_COUNT_4 ){
						paNewVal0++;
						*output3 = My_OutputLut[((ako0 		* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ein_regb 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
						}
						#endif
				}
				else
				{
					if (ein_regb >= ein_regg)
					{
						ako0 = (LH_ADR_BEREICH_SEL) - ein_regr;
						ako1 = ein_regr - ein_regb;
						ako2 = ein_regb - ein_regg;
						#if LH_LUT_DATA_SIZE_16
						#if LH_DATA_OUT_SIZE_16
		   		
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regg	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
				       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

						paNewVal0++;
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regg	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
				       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

						paNewVal0++;
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regg	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
				       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

						if( LH_DATA_OUT_COUNT_4 ){
						paNewVal0++;
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regg	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
				       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
						}
						
						#else
						*output0 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regg	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
						paNewVal0++;
						*output1 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regg	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
						paNewVal0++;
						*output2 = My_OutputLut[((ako0	 	* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ein_regg 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
						if( LH_DATA_OUT_COUNT_4 ){
						paNewVal0++;
						*output3 = My_OutputLut[((ako0 		* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ein_regg 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
						}
						#endif
						
						#else
						*output0 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regg	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
						paNewVal0++;
						*output1 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
												  ein_regg	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
						paNewVal0++;
						*output2 = My_OutputLut[((ako0	 	* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ein_regg 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
						if( LH_DATA_OUT_COUNT_4 ){
						paNewVal0++;
						*output3 = My_OutputLut[((ako0 		* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] +
											      ein_regg 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
						}
						#endif
					}
					else
					{
						ako0 = (LH_ADR_BEREICH_SEL) - ein_regr;
						ako1 = ein_regr - ein_regg;
						ako2 = ein_regg - ein_regb;
						#if LH_LUT_DATA_SIZE_16
						#if LH_DATA_OUT_SIZE_16
		   		
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ein_regb	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 0 << LH_ADR_BREIT_AUS_LUT );
				       	*output0 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

						paNewVal0++;
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ein_regb	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 1 << LH_ADR_BREIT_AUS_LUT );
				       	*output1 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

						paNewVal0++;
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ein_regb	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 2 << LH_ADR_BREIT_AUS_LUT );
				       	*output2 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );

						if( LH_DATA_OUT_COUNT_4 ){
						paNewVal0++;
						aVal =					 (ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ein_regb	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]);
						aVal = aVal + ( aVal >> ( LH_LUT_DATA_SHR ));
						aVal = aVal - ( aVal >> ( LH_ADR_BREIT_AUS_LUT ));
						ko = (aVal>>LH_DATA_SHR_CORR) & ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR))-1 );
						aVal = (aVal >> ( LH_DATA_SHR )) + ( 3 << LH_ADR_BREIT_AUS_LUT );
				       	*output3 = (LH_DATA_OUT_TYPE)((My_OutputLut[aVal] * ( (1<<( LH_DATA_SHR - LH_DATA_SHR_CORR )) - ko ) + My_OutputLut[aVal +1] * ko)>>(LH_DATA_SHR - LH_DATA_SHR_CORR) );
						}
						
						#else
						*output0 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ein_regb	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)]>>8;
						paNewVal0++;
						*output1 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ein_regb	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)]>>8;
						paNewVal0++;
						*output2 = My_OutputLut[((ako0	 	* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ein_regb 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)]>>8;
						if( LH_DATA_OUT_COUNT_4 ){
						paNewVal0++;
						*output3 = My_OutputLut[((ako0 		* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ein_regb 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
						}
						#endif
						
						#else
						*output0 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ein_regb	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (0<<LH_ADR_BREIT_AUS_LUT)];
						paNewVal0++;
						*output1 = My_OutputLut[((ako0 		* paNewVal0[0] +
												  ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
												  ein_regb	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (1<<LH_ADR_BREIT_AUS_LUT)];
						paNewVal0++;
						*output2 = My_OutputLut[((ako0	 	* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ein_regb 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (2<<LH_ADR_BREIT_AUS_LUT)];
						if( LH_DATA_OUT_COUNT_4 ){
						paNewVal0++;
						*output3 = My_OutputLut[((ako0 		* paNewVal0[0] +
											      ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] +
											      ein_regb 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)];
						}
						#endif
					}
				}
			}
			#if LH_DATA_OUT_SIZE_16 && ! LH_LUT_DATA_SIZE_16
			*output0 |= (*output0 << 8);
			*output1 |= (*output1 << 8);
			*output2 |= (*output2 << 8);
			if( LH_DATA_OUT_COUNT_4 ){
			*output3 |= (*output3 << 8);
			}
			#endif
			
			if (Mode == LH_CALC_ENGINE_P_TO_P)
			{
				while (--i)
				{
					input0 += inputOffset;
					input1 += inputOffset;
					input2 += inputOffset;
					output0 += outputOffset;
					output1 += outputOffset;
					output2 += outputOffset;
					if( LH_DATA_OUT_COUNT_4 ){
					output3 += outputOffset;
					}

					if ((*input0 ^ ein_cache[0]) || (*input1 ^ ein_cache[1]) || (*input2 ^ ein_cache[2]))
					{
						break;
					}
					*output0 = output0[-outputOffset];
					*output1 = output1[-outputOffset];
					*output2 = output2[-outputOffset];
					if( LH_DATA_OUT_COUNT_4 ){
					*output3 = output3[-outputOffset];
					}
				}
			}
			else if (Mode == LH_CALC_ENGINE_P_TO_U)
			{
				if( LH_DATA_OUT_COUNT_4 ){
				*output4 &= Mask;
				}else{
				*output3 &= Mask;
				}
				while (--i)
				{
					input0 += inputOffset;
					input1 += inputOffset;
					input2 += inputOffset;
					output0 += outputOffset;
					output1 += outputOffset;
					output2 += outputOffset;
					output3 += outputOffset;
					if( LH_DATA_OUT_COUNT_4 ){
					output4 += outputOffset;
					}
					if ((*input0 ^ ein_cache[0]) || (*input1 ^ ein_cache[1]) || (*input2 ^ ein_cache[2]))
					{
						break;
					}
					*output0 = output0[-outputOffset];
					*output1 = output1[-outputOffset];
					*output2 = output2[-outputOffset];
					if( LH_DATA_OUT_COUNT_4 ){
					*output3 = output3[-outputOffset];
					*output4 &= Mask;
					}else{
					*output3 &= Mask;
					}
				}
			}
			else
			{
				if( LH_DATA_OUT_COUNT_4 ){
				*output4 = (LH_DATA_OUT_TYPE)*input3;
				}else{
				*output3 = (LH_DATA_OUT_TYPE)*input3;
				}
				while (--i)
				{								/*U_TO_U*/
					input0 += inputOffset;
					input1 += inputOffset;
					input2 += inputOffset;
					input3 += inputOffset;

					output0 += outputOffset;
					output1 += outputOffset;
					output2 += outputOffset;
					output3 += outputOffset;
					if( LH_DATA_OUT_COUNT_4 ){
					output4 += outputOffset;
					}
					if ((*input0 ^ ein_cache[0]) || (*input1 ^ ein_cache[1]) || (*input2 ^ ein_cache[2]))
					{
						break;
					}
					*output0 = output0[-outputOffset];
					*output1 = output1[-outputOffset];
					*output2 = output2[-outputOffset];
					if( LH_DATA_OUT_COUNT_4 ){
					*output3 = output3[-outputOffset];
					*output4 = (LH_DATA_OUT_TYPE)*input3;
					}else{
					*output3 = (LH_DATA_OUT_TYPE)*input3;
					}
				}
			}
		}
		if (--LineCount)
		{
			j++;
			input0 = (LH_DATA_IN_TYPE *)((LH_UINT8 *)calcParam->inputData[0] + j * calcParam->cmInputBytesPerLine);
			input1 = (LH_DATA_IN_TYPE *)((LH_UINT8 *)calcParam->inputData[1] + j * calcParam->cmInputBytesPerLine);
			input2 = (LH_DATA_IN_TYPE *)((LH_UINT8 *)calcParam->inputData[2] + j * calcParam->cmInputBytesPerLine);
			input3 = (LH_DATA_IN_TYPE *)((LH_UINT8 *)calcParam->inputData[3] + j * calcParam->cmInputBytesPerLine);

			output0 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[0] + j * calcParam->cmOutputBytesPerLine);
			output1 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[1] + j * calcParam->cmOutputBytesPerLine);
			output2 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[2] + j * calcParam->cmOutputBytesPerLine);
			output3 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[3] + j * calcParam->cmOutputBytesPerLine);
			if( LH_DATA_OUT_COUNT_4 ){
			output4 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[4] + j * calcParam->cmOutputBytesPerLine);
			}
		}
	}
	LH_END_PROC(LH_CALC_PROC_NAME)
	return 0;
#endif
}
#endif
#undef LH_CALC_PROC_NAME
