/*
	File:		PI_Val.c

	Contains:	
				
	Written by:	U. J. Krabbenhoeft

	Version:

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/
#ifndef LHGeneralIncs_h
#include "General.h"
#endif

PI_Boolean CMValInput(		CMProfileRef prof,
							icHeader* aHeader );
PI_Boolean CMValDisplay(	CMProfileRef prof,
							icHeader* aHeader );
PI_Boolean CMValOutput(		CMProfileRef prof,
							icHeader* aHeader );
PI_Boolean CMValLink(		CMProfileRef prof );
PI_Boolean CMValColorSpace(	CMProfileRef prof,
							icHeader* aHeader );
PI_Boolean CMValAbstract(	CMProfileRef prof,
							icHeader* aHeader );
PI_Boolean CMValNamed(		CMProfileRef prof,
							icHeader* aHeader );

PI_Boolean CMValGray(		CMProfileRef prof );
PI_Boolean CMValRGB(		CMProfileRef prof );
PI_Boolean CMValAToB(		CMProfileRef prof );
PI_Boolean CMValBToA(		CMProfileRef prof );
PI_Boolean CMValMftOutput(	CMProfileRef prof );

PI_Boolean CMValInput(	CMProfileRef prof,
						icHeader* aHeader )
{
	PI_Boolean valid;
	switch ( aHeader->colorSpace)
	{

		case icSigGrayData:
			valid = CMValGray(prof);
			break;

		case icSigRgbData:
			if (aHeader->pcs == icSigLabData)
			{
				valid = CMValAToB(prof);
			}
			else
			{
				valid = CMValRGB(prof);
			}
			break;

		case icSigCmyData:
		case icSigCmykData:
		case icSigMCH2Data:
		case icSigMCH3Data:
		case icSigMCH4Data:
		case icSigMCH5Data:
		case icSigMCH6Data:
		case icSigMCH7Data:
		case icSigMCH8Data:
		case icSigMCH9Data:
		case icSigMCHAData:
		case icSigMCHBData:
		case icSigMCHCData:
		case icSigMCHDData:
		case icSigMCHEData:
		case icSigMCHFData:
			valid = CMValAToB(prof);
			break;

		default:
			valid = FALSE;
	}

	if (aHeader->pcs != icSigXYZData && aHeader->pcs != icSigLabData){
		valid = FALSE;
	}
	return (valid);
}

PI_Boolean CMValDisplay(	CMProfileRef prof,
							icHeader* aHeader )
{
	PI_Boolean valid;
	switch (aHeader->colorSpace)
	{

		case icSigGrayData:
			valid = CMValGray(prof);
			break;

		case icSigRgbData:
			if (aHeader->pcs == icSigLabData){
				valid = CMValBToA(prof);
			}
			else{
				valid = CMValRGB(prof);
			}
			break;
		case icSigMCH3Data:
		case icSigMCH4Data:
		case icSigCmyData:
		case icSigCmykData:
			valid = CMValBToA(prof);
			break;

		default:
			valid = FALSE;
	}

	if (aHeader->pcs != icSigXYZData && aHeader->pcs != icSigLabData){
		valid = FALSE;
	}
	return (valid);
}

PI_Boolean CMValOutput(	CMProfileRef prof,
						icHeader* aHeader )
{
	PI_Boolean valid;
	switch (aHeader->colorSpace)
	{

		case icSigGrayData:
			valid = CMValGray(prof);
			break;

		case icSigRgbData:
		case icSigCmyData:
		case icSigCmykData:
		case icSigMCH2Data:
		case icSigMCH3Data:
		case icSigMCH4Data:
		case icSigMCH5Data:
		case icSigMCH6Data:
		case icSigMCH7Data:
		case icSigMCH8Data:
		case icSigMCH9Data:
		case icSigMCHAData:
		case icSigMCHBData:
		case icSigMCHCData:
		case icSigMCHDData:
		case icSigMCHEData:
		case icSigMCHFData:
			valid = CMValMftOutput(prof);
			break;

		default:
			valid = FALSE;
	}

	if (aHeader->pcs != icSigXYZData && aHeader->pcs != icSigLabData){
		valid = FALSE;
	}
	return (valid);
}

PI_Boolean CMValLink(	CMProfileRef prof )
{
	PI_Boolean valid;
	valid = CMProfileElementExists((CMProfileRef)prof, icSigProfileDescriptionTag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigAToB0Tag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigProfileSequenceDescTag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigCopyrightTag);

	return (valid);
}

PI_Boolean CMValColorSpace(	CMProfileRef prof,
							icHeader* aHeader )
{
	PI_Boolean valid;
	if (aHeader->pcs != icSigXYZData && aHeader->pcs != icSigLabData){
		valid = FALSE;
		return (valid);
	}

	valid = CMProfileElementExists((CMProfileRef)prof, icSigProfileDescriptionTag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigAToB0Tag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigBToA0Tag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigMediaWhitePointTag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigCopyrightTag);

	return (valid);
}


PI_Boolean CMValAbstract(	CMProfileRef prof,
							icHeader* aHeader )
{
	PI_Boolean valid;
	switch (aHeader->pcs)
	{

		case icSigXYZData:
		case icSigLabData:
			valid = CMValAToB(prof);
			break;

		default:
			valid = FALSE;
	}

	return (valid);
}

PI_Boolean CMValNamed(		CMProfileRef prof,
							icHeader* aHeader )
{
	PI_Boolean valid;
	if (aHeader->pcs != icSigXYZData && aHeader->pcs != icSigLabData){
		valid = FALSE;
		return (valid);
	}

	valid = CMProfileElementExists((CMProfileRef)prof, icSigProfileDescriptionTag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigNamedColor2Tag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigMediaWhitePointTag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigCopyrightTag);

	return (valid);
}

PI_Boolean CMValGray(	CMProfileRef prof )
{
	PI_Boolean valid;
	valid = CMProfileElementExists((CMProfileRef)prof, icSigProfileDescriptionTag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigGrayTRCTag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigMediaWhitePointTag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigCopyrightTag);

	return (valid);
}

PI_Boolean CMValAToB(	CMProfileRef prof )
{
	PI_Boolean valid;
	valid = CMProfileElementExists((CMProfileRef)prof, icSigProfileDescriptionTag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigAToB0Tag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigMediaWhitePointTag);/* change to enum */

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigCopyrightTag);/* change to enum */

	return (valid);
}

PI_Boolean CMValBToA(	CMProfileRef prof )
{
	PI_Boolean valid;
	valid = CMProfileElementExists((CMProfileRef)prof, icSigProfileDescriptionTag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigBToA0Tag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigMediaWhitePointTag);/* change to enum */

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigCopyrightTag);/* change to enum */

	return (valid);
}

CMError CMValidateProfile(	CMProfileRef	prof,
							PI_Boolean		*valid )
{
	CMError err = noErr;
	icHeader aHeader;

	*valid = FALSE;
	err = CMGetProfileHeader((CMProfileRef)prof, &aHeader);

	if (!err)
	{
		if ((aHeader.version & 0xff000000) >= icVersionNumber ){
			switch (aHeader.deviceClass)
			  {
				case icSigInputClass:
				*valid = CMValInput(prof, &aHeader );
				break;

				case icSigDisplayClass:
				*valid = CMValDisplay(prof, &aHeader );
				break;

				case icSigOutputClass:
				*valid = CMValOutput(prof, &aHeader );
				break;

				case icSigLinkClass:
				*valid = CMValLink(prof);
				break;

				case icSigColorSpaceClass:
				*valid = CMValColorSpace(prof, &aHeader );
				break;

				case icSigAbstractClass:
				*valid = CMValAbstract(prof, &aHeader );
				break;

				case icSigNamedColorClass:
				*valid = CMValNamed(prof, &aHeader );
				break;

				default:
				*valid = FALSE;
			  }

		}
		else
		{										/* unknown profile */
			*valid = FALSE;
			return (cmProfileError);
		}
	}

	return (err);
}

PI_Boolean CMValMftOutput (	CMProfileRef prof )
{
	PI_Boolean valid;
	valid = CMProfileElementExists((CMProfileRef)prof, icSigProfileDescriptionTag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigAToB0Tag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigAToB1Tag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigAToB2Tag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigBToA0Tag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigBToA1Tag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigBToA2Tag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigGamutTag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigMediaWhitePointTag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigCopyrightTag);

	return (valid);
}

PI_Boolean CMValRGB(			CMProfileRef prof )
{
	PI_Boolean valid;
	valid = CMProfileElementExists((CMProfileRef)prof, icSigProfileDescriptionTag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigRedColorantTag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigGreenColorantTag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigBlueColorantTag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigRedTRCTag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigGreenTRCTag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigBlueTRCTag);

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigMediaWhitePointTag);/* wtpt change to enum */

	if (valid)
		valid = CMProfileElementExists((CMProfileRef)prof, icSigCopyrightTag);/* cprt change to enum */

	return (valid);
}

