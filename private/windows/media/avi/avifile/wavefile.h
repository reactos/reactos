
/****************************************************************************
 *
 *  WAVEFILE.H
 *
 *  header file for routines for reading WAVE files
 *
 ***************************************************************************/
/*	-	-	-	-	-	-	-	-	*/
#ifdef WIN32
#define _export // EXPORT in def file does everything necessary in WIN32
#endif

#include "avifile.rc"

extern HMODULE ghModule; // global HMODULE/HINSTANCE for resource access
/*	-	-	-	-	-	-	-	-	*/

/*
** This class is used to implement a handler for a type of file with only
** one stream.  In this case, we don't have to worry about allocating more
** than one stream object for each file object, so we can combine the
** two together in a single class.
**
*/

#ifdef __cplusplus
extern "C"             /* Assume C declarations for C++ */
#endif	/* __cplusplus */

HRESULT WaveFileCreate(
	IUnknown FAR*	pUnknownOuter,
	REFIID		riid,
	void FAR* FAR*	ppv);
/*	-	-	-	-	-	-	-	-	*/

/*
** These variables help keep track of whether the DLL is still in use,
** so that when our DllCanUnloadNow() function is called, we know what
** to say. 
*/

extern UINT	uUseCount;
extern UINT	uLockCount;

/*	-	-	-	-	-	-	-	-	*/

//
// This is our unique identifier
//
//  NOTE: If you modify this sample code to do something else, you MUST
//	    CHANGE THIS!
//
//  Run uuidgen.exe from the tools directory and get your own GUID.
//  DO NOT USE THIS ONE!
//
//
//
DEFINE_GUID(CLSID_AVIWaveFileReader, 0x00020003, 0, 0, 0xC0,0,0,0,0,0,0,0x46);
