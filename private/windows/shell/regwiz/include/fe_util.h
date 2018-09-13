/*
	FE_UTIL.H
	Far East Countries helper 
	03/02/98

*/

#ifndef __FE_UTIL__

#define __FE_UTIL__
#include <windows.h>

#define JAPAN_LCID   			411
#define KOREAN_LCID   			412
#define TRADITIONAL_CHINA_LCID  404
#define SIMPLIFIED_CHINA_LCID   804
#define MAX_FE_COUNTRIES_SUPPORTED 256
typedef enum
{
	kNotInitialised,
	kFarEastCountry,   
	kNotAFECountry,
	UnknownCountry
}FeCountriesIndex;

typedef enum {
	kFEWithNonJapaneaseScreen,
	kFEWithJapaneaseScreen // Returned for screen type
}FeScreenType;


extern  FeCountriesIndex   gWhatFECountry; // This is a global variable
										   // Which holds the present FE Country	
extern  FeScreenType       gWhichFEScreenTye; // This is a global variable
										   // Which holds the FE Screen Type	
FeCountriesIndex  IsFarEastCountry(HINSTANCE hIns);
FeScreenType      GetFeScreenType();
#endif
