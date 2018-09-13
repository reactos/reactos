/*
	File:		LHCalcEngine.c

	Contains:	

	Written by:	U. J. Krabbenhoeft

	Version:

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/

#undef LH_DATA_IN_TYPE
#undef LH_DATA_OUT_TYPE
#undef LH_LUT_DATA_TYPE
#undef LH_DATA_IN_COUNT
#undef LH_DATA_OUT_COUNT
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

#if LH_DATA_OUT_COUNT_4
#define LH_DATA_OUT_COUNT 		4
#else
#define LH_DATA_OUT_COUNT 		3
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
static LH_UINT16 MMXTab [] = 
{
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0001, 0x0001, 0x0001, 0x0001,
    0x0002, 0x0002, 0x0002, 0x0002,
    0x0003, 0x0003, 0x0003, 0x0003,
    0x0004, 0x0004, 0x0004, 0x0004,
    0x0005, 0x0005, 0x0005, 0x0005,
    0x0006, 0x0006, 0x0006, 0x0006,
    0x0007, 0x0007, 0x0007, 0x0007,
    0x0008, 0x0008, 0x0008, 0x0008,
    0x0009, 0x0009, 0x0009, 0x0009,
    0x000A, 0x000A, 0x000A, 0x000a,
    0x000B, 0x000B, 0x000B, 0x000b,
    0x000C, 0x000C, 0x000C, 0x000c,
    0x000D, 0x000D, 0x000D, 0x000d,
    0x000E, 0x000E, 0x000E, 0x000e,
    0x000F, 0x000F, 0x000F, 0x000f,
    0x0010, 0x0010, 0x0010, 0x0010,
    0x0011, 0x0011, 0x0011, 0x0011,
    0x0012, 0x0012, 0x0012, 0x0012,
    0x0013, 0x0013, 0x0013, 0x0013,
    0x0014, 0x0014, 0x0014, 0x0014,
    0x0015, 0x0015, 0x0015, 0x0015,
    0x0016, 0x0016, 0x0016, 0x0016,
    0x0017, 0x0017, 0x0017, 0x0017,
    0x0018, 0x0018, 0x0018, 0x0018,
    0x0019, 0x0019, 0x0019, 0x0019,
    0x001A, 0x001A, 0x001A, 0x001a,
    0x001B, 0x001B, 0x001B, 0x001b,
    0x001C, 0x001C, 0x001C, 0x001c,
    0x001D, 0x001D, 0x001D, 0x001d,
    0x001E, 0x001E, 0x001E, 0x001e,
    0x001F, 0x001F, 0x001F, 0x001f,
    0x0020, 0x0020, 0x0020, 0x0020,
    0x0021, 0x0021, 0x0021, 0x0021,
    0x0022, 0x0022, 0x0022, 0x0022,
    0x0023, 0x0023, 0x0023, 0x0023,
    0x0024, 0x0024, 0x0024, 0x0024,
    0x0025, 0x0025, 0x0025, 0x0025,
    0x0026, 0x0026, 0x0026, 0x0026,
    0x0027, 0x0027, 0x0027, 0x0027,
    0x0028, 0x0028, 0x0028, 0x0028,
    0x0029, 0x0029, 0x0029, 0x0029,
    0x002A, 0x002A, 0x002A, 0x002a,
    0x002B, 0x002B, 0x002B, 0x002b,
    0x002C, 0x002C, 0x002C, 0x002c,
    0x002D, 0x002D, 0x002D, 0x002d,
    0x002E, 0x002E, 0x002E, 0x002e,
    0x002F, 0x002F, 0x002F, 0x002f,
    0x0030, 0x0030, 0x0030, 0x0030,
    0x0031, 0x0031, 0x0031, 0x0031,
    0x0032, 0x0032, 0x0032, 0x0032,
    0x0033, 0x0033, 0x0033, 0x0033,
    0x0034, 0x0034, 0x0034, 0x0034,
    0x0035, 0x0035, 0x0035, 0x0035,
    0x0036, 0x0036, 0x0036, 0x0036,
    0x0037, 0x0037, 0x0037, 0x0037,
    0x0038, 0x0038, 0x0038, 0x0038,
    0x0039, 0x0039, 0x0039, 0x0039,
    0x003A, 0x003A, 0x003A, 0x003a,
    0x003B, 0x003B, 0x003B, 0x003b,
    0x003C, 0x003C, 0x003C, 0x003c,
    0x003D, 0x003D, 0x003D, 0x003d,
    0x003E, 0x003E, 0x003E, 0x003e,
    0x003F, 0x003F, 0x003F, 0x003f,
    0x0040, 0x0040, 0x0040, 0x0040,
    0x0041, 0x0041, 0x0041, 0x0041,
    0x0042, 0x0042, 0x0042, 0x0042,
    0x0043, 0x0043, 0x0043, 0x0043,
    0x0044, 0x0044, 0x0044, 0x0044,
    0x0045, 0x0045, 0x0045, 0x0045,
    0x0046, 0x0046, 0x0046, 0x0046,
    0x0047, 0x0047, 0x0047, 0x0047,
    0x0048, 0x0048, 0x0048, 0x0048,
    0x0049, 0x0049, 0x0049, 0x0049,
    0x004A, 0x004A, 0x004A, 0x004a,
    0x004B, 0x004B, 0x004B, 0x004b,
    0x004C, 0x004C, 0x004C, 0x004c,
    0x004D, 0x004D, 0x004D, 0x004d,
    0x004E, 0x004E, 0x004E, 0x004e,
    0x004F, 0x004F, 0x004F, 0x004f,
    0x0050, 0x0050, 0x0050, 0x0050,
    0x0051, 0x0051, 0x0051, 0x0051,
    0x0052, 0x0052, 0x0052, 0x0052,
    0x0053, 0x0053, 0x0053, 0x0053,
    0x0054, 0x0054, 0x0054, 0x0054,
    0x0055, 0x0055, 0x0055, 0x0055,
    0x0056, 0x0056, 0x0056, 0x0056,
    0x0057, 0x0057, 0x0057, 0x0057,
    0x0058, 0x0058, 0x0058, 0x0058,
    0x0059, 0x0059, 0x0059, 0x0059,
    0x005A, 0x005A, 0x005A, 0x005a,
    0x005B, 0x005B, 0x005B, 0x005b,
    0x005C, 0x005C, 0x005C, 0x005c,
    0x005D, 0x005D, 0x005D, 0x005d,
    0x005E, 0x005E, 0x005E, 0x005e,
    0x005F, 0x005F, 0x005F, 0x005f,
    0x0060, 0x0060, 0x0060, 0x0060,
    0x0061, 0x0061, 0x0061, 0x0061,
    0x0062, 0x0062, 0x0062, 0x0062,
    0x0063, 0x0063, 0x0063, 0x0063,
    0x0064, 0x0064, 0x0064, 0x0064,
    0x0065, 0x0065, 0x0065, 0x0065,
    0x0066, 0x0066, 0x0066, 0x0066,
    0x0067, 0x0067, 0x0067, 0x0067,
    0x0068, 0x0068, 0x0068, 0x0068,
    0x0069, 0x0069, 0x0069, 0x0069,
    0x006A, 0x006A, 0x006A, 0x006a,
    0x006B, 0x006B, 0x006B, 0x006b,
    0x006C, 0x006C, 0x006C, 0x006c,
    0x006D, 0x006D, 0x006D, 0x006d,
    0x006E, 0x006E, 0x006E, 0x006e,
    0x006F, 0x006F, 0x006F, 0x006f,
    0x0070, 0x0070, 0x0070, 0x0070,
    0x0071, 0x0071, 0x0071, 0x0071,
    0x0072, 0x0072, 0x0072, 0x0072,
    0x0073, 0x0073, 0x0073, 0x0073,
    0x0074, 0x0074, 0x0074, 0x0074,
    0x0075, 0x0075, 0x0075, 0x0075,
    0x0076, 0x0076, 0x0076, 0x0076,
    0x0077, 0x0077, 0x0077, 0x0077,
    0x0078, 0x0078, 0x0078, 0x0078,
    0x0079, 0x0079, 0x0079, 0x0079,
    0x007A, 0x007A, 0x007A, 0x007a,
    0x007B, 0x007B, 0x007B, 0x007b,
    0x007C, 0x007C, 0x007C, 0x007c,
    0x007D, 0x007D, 0x007D, 0x007d,
    0x007E, 0x007E, 0x007E, 0x007e,
    0x007F, 0x007F, 0x007F, 0x007f,
    0x0080, 0x0080, 0x0080, 0x0080
};
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
	LH_UINT16 ak[4];
	LH_UINT8    bFPUState [108];
#define Test_mode
#ifdef Test_mode
	LH_UINT16 TestRam[4];
#endif

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

	__asm {
            fnsave  bFPUState
			pxor		mm0,mm0
			pxor		mm1,mm1
			pxor		mm2,mm2
			pxor		mm3,mm3
			pxor		mm4,mm4
			pxor		mm5,mm5
			pxor		mm6,mm6
			pxor		mm7,mm7
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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regK
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regC
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regC
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regC
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regK
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regC
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regC
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
					        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
					        							ako3 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
					        						ein_regC * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regC
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regK
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regM
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regM
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
				        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regM
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regK
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regY
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regY
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
				        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
				        							ako2 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regY
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
					        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regK
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regM
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regM
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
					        						ein_regM * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regM
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
				        						ein_regK * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regK
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
				        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
				        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
				        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
				        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regY
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
					        							ako1 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)+3] + 
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regY
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
	
							#if LH_DATA_OUT_COUNT_4
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
							#endif
							
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
							#if LH_DATA_OUT_COUNT_4
							*output3 = My_OutputLut[((	ako0 * paNewVal0[3] + 
					        							ako1 * paNewVal0[(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
					        							ako2 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
					        							ako3 * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] + 
					        						ein_regY * paNewVal0[(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)+3] )>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
							#endif
							#endif
							
							#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((((0<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako3
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm6
#endif
 			punpcklbw   mm2,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 0)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regY
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((((1<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1)<<LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1
			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm5
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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
			#if LH_DATA_OUT_COUNT_4
			*output3 |= (*output3 << 8);
			#endif
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
					#if LH_DATA_OUT_COUNT_4
					output3 += outputOffset;
					#endif

					if ((*input0 ^ ein_cache[0]) || (*input1 ^ ein_cache[1]) || (*input2 ^ ein_cache[2]) || (*input3 ^ ein_cache[3]))
					{
						break;
					}
					*output0 = output0[-outputOffset];
					*output1 = output1[-outputOffset];
					*output2 = output2[-outputOffset];
					#if LH_DATA_OUT_COUNT_4
					*output3 = output3[-outputOffset];
					#endif
				}
			}
			else if (Mode == LH_CALC_ENGINE_P_TO_U)
			{
				#if LH_DATA_OUT_COUNT_4
				*output4 &= Mask;
				#else
				*output3 &= Mask;
				#endif
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
					#if LH_DATA_OUT_COUNT_4
					output4 += outputOffset;
					#endif
					if ((*input0 ^ ein_cache[0]) || (*input1 ^ ein_cache[1]) || (*input2 ^ ein_cache[2]) || (*input3 ^ ein_cache[3]))
					{
						break;
					}
					*output0 = output0[-outputOffset];
					*output1 = output1[-outputOffset];
					*output2 = output2[-outputOffset];
					#if LH_DATA_OUT_COUNT_4
					*output3 = output3[-outputOffset];
					*output4 &= Mask;
					#else
					*output3 &= Mask;
					#endif
				}
			}
			else
			{
				#if LH_DATA_OUT_COUNT_4
				*output4 = (LH_DATA_OUT_TYPE)*input4;
				#else
				*output3 = (LH_DATA_OUT_TYPE)*input4;
				#endif
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
					#if LH_DATA_OUT_COUNT_4
					output4 += outputOffset;
					#endif
					if ((*input0 ^ ein_cache[0]) || (*input1 ^ ein_cache[1]) || (*input2 ^ ein_cache[2]) || (*input3 ^ ein_cache[3]))
					{
						break;
					}
					*output0 = output0[-outputOffset];
					*output1 = output1[-outputOffset];
					*output2 = output2[-outputOffset];
					#if LH_DATA_OUT_COUNT_4
					*output3 = output3[-outputOffset];
					*output4 = (LH_DATA_OUT_TYPE)*input4;
					#else
					*output3 = (LH_DATA_OUT_TYPE)*input4;
					#endif
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
			#if LH_DATA_OUT_COUNT_4
			output4 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[4] + j * calcParam->cmOutputBytesPerLine);
			#endif
		}
	}
    __asm {
		emms
		frstor      bFPUState
	}
	LH_END_PROC(LH_CALC_PROC_NAME)
	return 0;
}
#else
{

static LH_UINT16 MMXTab [260] = 
{
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0001, 0x0001, 0x0001, 0x0001,
    0x0002, 0x0002, 0x0002, 0x0002,
    0x0003, 0x0003, 0x0003, 0x0003,
    0x0004, 0x0004, 0x0004, 0x0004,
    0x0005, 0x0005, 0x0005, 0x0005,
    0x0006, 0x0006, 0x0006, 0x0006,
    0x0007, 0x0007, 0x0007, 0x0007,
    0x0008, 0x0008, 0x0008, 0x0008,
    0x0009, 0x0009, 0x0009, 0x0009,
    0x000A, 0x000A, 0x000A, 0x000a,
    0x000B, 0x000B, 0x000B, 0x000b,
    0x000C, 0x000C, 0x000C, 0x000c,
    0x000D, 0x000D, 0x000D, 0x000d,
    0x000E, 0x000E, 0x000E, 0x000e,
    0x000F, 0x000F, 0x000F, 0x000f,
    0x0010, 0x0010, 0x0010, 0x0010,
    0x0011, 0x0011, 0x0011, 0x0011,
    0x0012, 0x0012, 0x0012, 0x0012,
    0x0013, 0x0013, 0x0013, 0x0013,
    0x0014, 0x0014, 0x0014, 0x0014,
    0x0015, 0x0015, 0x0015, 0x0015,
    0x0016, 0x0016, 0x0016, 0x0016,
    0x0017, 0x0017, 0x0017, 0x0017,
    0x0018, 0x0018, 0x0018, 0x0018,
    0x0019, 0x0019, 0x0019, 0x0019,
    0x001A, 0x001A, 0x001A, 0x001a,
    0x001B, 0x001B, 0x001B, 0x001b,
    0x001C, 0x001C, 0x001C, 0x001c,
    0x001D, 0x001D, 0x001D, 0x001d,
    0x001E, 0x001E, 0x001E, 0x001e,
    0x001F, 0x001F, 0x001F, 0x001f,
    0x0020, 0x0020, 0x0020, 0x0020,
    0x0021, 0x0021, 0x0021, 0x0021,
    0x0022, 0x0022, 0x0022, 0x0022,
    0x0023, 0x0023, 0x0023, 0x0023,
    0x0024, 0x0024, 0x0024, 0x0024,
    0x0025, 0x0025, 0x0025, 0x0025,
    0x0026, 0x0026, 0x0026, 0x0026,
    0x0027, 0x0027, 0x0027, 0x0027,
    0x0028, 0x0028, 0x0028, 0x0028,
    0x0029, 0x0029, 0x0029, 0x0029,
    0x002A, 0x002A, 0x002A, 0x002a,
    0x002B, 0x002B, 0x002B, 0x002b,
    0x002C, 0x002C, 0x002C, 0x002c,
    0x002D, 0x002D, 0x002D, 0x002d,
    0x002E, 0x002E, 0x002E, 0x002e,
    0x002F, 0x002F, 0x002F, 0x002f,
    0x0030, 0x0030, 0x0030, 0x0030,
    0x0031, 0x0031, 0x0031, 0x0031,
    0x0032, 0x0032, 0x0032, 0x0032,
    0x0033, 0x0033, 0x0033, 0x0033,
    0x0034, 0x0034, 0x0034, 0x0034,
    0x0035, 0x0035, 0x0035, 0x0035,
    0x0036, 0x0036, 0x0036, 0x0036,
    0x0037, 0x0037, 0x0037, 0x0037,
    0x0038, 0x0038, 0x0038, 0x0038,
    0x0039, 0x0039, 0x0039, 0x0039,
    0x003A, 0x003A, 0x003A, 0x003a,
    0x003B, 0x003B, 0x003B, 0x003b,
    0x003C, 0x003C, 0x003C, 0x003c,
    0x003D, 0x003D, 0x003D, 0x003d,
    0x003E, 0x003E, 0x003E, 0x003e,
    0x003F, 0x003F, 0x003F, 0x003f,
    0x0040, 0x0040, 0x0040, 0x0040
};

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

	LH_UINT16 ak[4];
	LH_UINT8    bFPUState [108];
//#define Test_mode
#ifdef Test_mode
	LH_UINT16 TestRam[4];
#endif
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
    __asm {
            fnsave  bFPUState
			pxor		mm0,mm0
			pxor		mm1,mm1
			pxor		mm2,mm2
			pxor		mm3,mm3
			pxor		mm4,mm4
			pxor		mm5,mm5
			pxor		mm6,mm6
			pxor		mm7,mm7
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

						#if LH_DATA_OUT_COUNT_4
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
						#endif
						
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
						#if LH_DATA_OUT_COUNT_4
						paNewVal0++;
						*output3 = My_OutputLut[((ako0 		* paNewVal0[0] + 
											      ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] + 
											      ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] + 
											      ein_regr 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
						#endif
						#endif
						
						#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regr
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm2,[eax+(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2
			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm6
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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

						#if LH_DATA_OUT_COUNT_4
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
						#endif
						
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
						#if LH_DATA_OUT_COUNT_4
						paNewVal0++;
						*output3 = My_OutputLut[((ako0 		* paNewVal0[0] + 
											      ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] + 
											      ako2 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] + 
											      ein_regr 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
						#endif
						#endif
						
						#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regr
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm2,[eax+(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2
			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm6
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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

						#if LH_DATA_OUT_COUNT_4
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
						#endif
						
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
						#if LH_DATA_OUT_COUNT_4
						paNewVal0++;
						*output3 = My_OutputLut[((ako0 		* paNewVal0[0] + 
											      ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] + 
											      ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] + 
											      ein_regg 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
						#endif
						#endif
						
						#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((0 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regg
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm2,[eax+(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2
			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm6
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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

						#if LH_DATA_OUT_COUNT_4
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
						#endif
						
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
						#if LH_DATA_OUT_COUNT_4
						paNewVal0++;
						*output3 = My_OutputLut[((ako0 		* paNewVal0[0] + 
											      ako1 		* paNewVal0[(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] + 
											      ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] + 
											      ein_regb 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
						#endif
						#endif
						
						#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((0 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regb
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm2,[eax+(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2
			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm6
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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

						#if LH_DATA_OUT_COUNT_4
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
						#endif
						
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
						#if LH_DATA_OUT_COUNT_4
						paNewVal0++;
						*output3 = My_OutputLut[((ako0 		* paNewVal0[0] + 
											      ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] + 
											      ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)] + 
											      ein_regg 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
						#endif
						#endif
						
						#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regg
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm2,[eax+(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2
			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm6
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

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

						#if LH_DATA_OUT_COUNT_4
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
						#endif
						
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
						#if LH_DATA_OUT_COUNT_4
						paNewVal0++;
						*output3 = My_OutputLut[((ako0 		* paNewVal0[0] + 
											      ako1 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] + 
											      ako2 		* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)] + 
											      ein_regb 	* paNewVal0[(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)])>>LH_DATA_SHR) + (3<<LH_ADR_BREIT_AUS_LUT)]>>8;
						#endif
						#endif
						
						#else
     __asm { 
            mov         eax, paNewVal0

 			mov			ebx, ako0
            movq        mm5, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm5
#endif
			punpcklbw	mm1,[eax] 
 			mov			ebx, ako1
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif

 			pmullw		mm5,mm1
            movq        mm6, MMXTab [ebx*8]
#ifdef Test_mode
			movq		TestRam,mm6
#endif
			punpcklbw   mm2,[eax+(((((1 << LH_BIT_BREIT_ADR) | 0) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ako2
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2

			movq        mm7, MMXTab [ebx*8]
 			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm1,[eax+(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 0) * LH_DATA_OUT_COUNT)]
			mov			ebx, ein_regb
			psrlw	    mm1,8
#ifdef Test_mode
			movq		TestRam,mm1
#endif
 			pmullw		mm7,mm1

			movq        mm6, MMXTab [ebx*8]
 			paddusw		mm5,mm7
#ifdef Test_mode
			movq		TestRam,mm7
#endif
 			punpcklbw   mm2,[eax+(((((1 << LH_BIT_BREIT_ADR) | 1) << LH_BIT_BREIT_ADR) | 1) * LH_DATA_OUT_COUNT)]
			psrlw	    mm2,8
#ifdef Test_mode
			movq		TestRam,mm2
#endif
 			pmullw		mm6,mm2
			paddusw		mm5,mm6
#ifdef Test_mode
			movq		TestRam,mm6
#endif


			xor			eax,eax
			psrlw		mm5,LH_DATA_SHR
			movq		ak,mm5

			mov			ax,ak
			mov			esi,My_OutputLut
			mov			al,BYTE PTR[eax+esi]
			mov			edi,output0
			mov			[edi], al
			mov			ax,ak+2
			mov			al,BYTE PTR[eax+esi+(1<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output1
			mov			[edi], al
			mov			ax,ak+4
			mov			al,BYTE PTR[eax+esi+(2<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output2
			mov			[edi], al
			#if LH_DATA_OUT_COUNT_4
			mov			ax,ak+6
			mov			al,BYTE PTR[eax+esi+(3<<LH_ADR_BREIT_AUS_LUT)]
			mov			edi,output3
			mov			[edi], al
			#endif
			

	 }
						#endif
					}
				}
			}
			#if LH_DATA_OUT_SIZE_16 && ! LH_LUT_DATA_SIZE_16
			*output0 |= (*output0 << 8);
			*output1 |= (*output1 << 8);
			*output2 |= (*output2 << 8);
			#if LH_DATA_OUT_COUNT_4
			*output3 |= (*output3 << 8);
			#endif
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
					#if LH_DATA_OUT_COUNT_4
					output3 += outputOffset;
					#endif

					if ((*input0 ^ ein_cache[0]) || (*input1 ^ ein_cache[1]) || (*input2 ^ ein_cache[2]))
					{
						break;
					}
					*output0 = output0[-outputOffset];
					*output1 = output1[-outputOffset];
					*output2 = output2[-outputOffset];
					#if LH_DATA_OUT_COUNT_4
					*output3 = output3[-outputOffset];
					#endif
				}
			}
			else if (Mode == LH_CALC_ENGINE_P_TO_U)
			{
				#if LH_DATA_OUT_COUNT_4
				*output4 &= Mask;
				#else
				*output3 &= Mask;
				#endif
				while (--i)
				{
					input0 += inputOffset;
					input1 += inputOffset;
					input2 += inputOffset;
					output0 += outputOffset;
					output1 += outputOffset;
					output2 += outputOffset;
					output3 += outputOffset;
					#if LH_DATA_OUT_COUNT_4
					output4 += outputOffset;
					#endif
					if ((*input0 ^ ein_cache[0]) || (*input1 ^ ein_cache[1]) || (*input2 ^ ein_cache[2]))
					{
						break;
					}
					*output0 = output0[-outputOffset];
					*output1 = output1[-outputOffset];
					*output2 = output2[-outputOffset];
					#if LH_DATA_OUT_COUNT_4
					*output3 = output3[-outputOffset];
					*output4 &= Mask;
					#else
					*output3 &= Mask;
					#endif
				}
			}
			else
			{
				#if LH_DATA_OUT_COUNT_4
				*output4 = (LH_DATA_OUT_TYPE)*input3;
				#else
				*output3 = (LH_DATA_OUT_TYPE)*input3;
				#endif
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
					#if LH_DATA_OUT_COUNT_4
					output4 += outputOffset;
					#endif
					if ((*input0 ^ ein_cache[0]) || (*input1 ^ ein_cache[1]) || (*input2 ^ ein_cache[2]))
					{
						break;
					}
					*output0 = output0[-outputOffset];
					*output1 = output1[-outputOffset];
					*output2 = output2[-outputOffset];
					#if LH_DATA_OUT_COUNT_4
					*output3 = output3[-outputOffset];
					*output4 = (LH_DATA_OUT_TYPE)*input3;
					#else
					*output3 = (LH_DATA_OUT_TYPE)*input3;
					#endif
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
			#if LH_DATA_OUT_COUNT_4
			output4 = (LH_DATA_OUT_TYPE *)((LH_UINT8 *)calcParam->outputData[4] + j * calcParam->cmOutputBytesPerLine);
			#endif
		}
	}
    __asm {
		emms
		frstor      bFPUState
	}
	LH_END_PROC(LH_CALC_PROC_NAME)
	return 0;
}
#endif
#undef LH_CALC_PROC_NAME
