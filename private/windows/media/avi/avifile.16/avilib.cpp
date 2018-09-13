/****************************************************************************
 *
 *  AVILIB.CPP
 *
 *  routines for reading a AVIStream
 *
 *  Copyright (c) 1992 Microsoft Corporation.  All Rights Reserved.
 *
 *  You have a royalty-free right to use, modify, reproduce and
 *  distribute the Sample Files (and/or any modified version) in
 *  any way you find useful, provided that you agree that
 *  Microsoft has no warranty obligations or liability for any
 *  Sample Application Files which are modified.
 *
 ***************************************************************************/

#include <win32.h>
#include <compobj.h>
#include <compman.h>
#include <shellapi.h>
#include <memory.h>     // for _fmemset

#include "avifile.h"
#include "aviopts.h"	// string resources
#include "avireg.h"
#include "debug.h"

#ifndef WIN32
#undef HKEY_CLASSES_ROOT
#define HKEY_CLASSES_ROOT       0x00000001
#endif

#define ValidPAVI(pavi)  (pavi != NULL)

#define V_PAVI(pavi, err)   \
    if (!ValidPAVI(pavi))   \
        return err;

/****************************************************************************

    strings

****************************************************************************/

#define SZCODE char _based(_segname("_CODE"))

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

EXTERN_C HINSTANCE ghMod;

static int iInit = 0;

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

/**************************************************************************
* @doc INTERNAL InitRegistry()
*
* @api void | write all the default AVIFile/AVIStream handlers to the
*             registry.
*
* @comm This function should be enhanced so that some of the key values
*	can be loaded from resources, instead of a static string table....
*
* @xref AVIStreamInit
*
*************************************************************************/

static void InitRegistry()
{
    char **ppch = aszReg;
    char ach[80];

#ifndef DEBUG
    LONG cb;

    // !!! This should have a version number or something in it....
    if (RegQueryValue(HKEY_CLASSES_ROOT, ppch[0], ach, (cb = sizeof(ach),&cb)) == ERROR_SUCCESS &&
        lstrcmpi(ach, ppch[1]) == 0)

        return;
#endif

    while (ppch[0])
    {
	if (((UINT) ppch[1]) < MAX_RC_CONSTANT) {
	    LoadString(ghMod, (UINT) ppch[1], ach, sizeof(ach));
	    RegSetValue(HKEY_CLASSES_ROOT, ppch[0], REG_SZ, ach, 0L);
	} else
	    RegSetValue(HKEY_CLASSES_ROOT, ppch[0], REG_SZ, ppch[1], 0L);
        ppch += 2;
    }
}

/**************************************************************************
* @doc EXTERNAL AVIFileInit
*
* @api void | AVIFileInit | This function initalizes the AVIFILE library.
*
* @comm Call this function before using any other AVIFILE functions.
*
* @xref <f AVIFileExit>
*
*************************************************************************/
STDAPI_(void) AVIFileInit()
{
    iInit++;

    CoInitialize(NULL);

    if (iInit == 1) {
        InitRegistry();
    }
}

/**************************************************************************
* @doc EXTERNAL AVIFileExit
*
* @api void | AVIFileExit | This function exits the AVIFILE library.
*
* @comm Call this function after using any other AVIFILE functions.
*
* @xref <f AVIFileInit>
*
*************************************************************************/
STDAPI_(void) AVIFileExit()
{
    extern HINSTANCE ghOLE2;

    iInit--;

    CoFreeUnusedLibraries();
    CoUninitialize();

    if (iInit == 0){
	if (ghOLE2) {
	    DPF("Freeing OLE2.DLL\n");
	    FreeLibrary(ghOLE2);
	    ghOLE2 = NULL;
	}
    }
}


/**************************************************************************
* @doc INTERNAL AVIFileCreate
*
* @api LONG | AVIFileCreate | Initializes an empty AVI File interface
*	pointer.
*
* @parm PAVIFILE FAR * | ppfile | Pointer to where the new <t PAVIFILE>
*	should be returned.
*
* @parm LONG | lParam | Specifies a parameter passed to the handler.
*
* @parm CLSID FAR * | pclsidHandler | Specifies a pointer to a
*       class ID used to create the file.
*
* @devnote Nobody should have to call this function, because AVIFileOpen
*   does it.  In fact, why do we even have this?
*
* @rdesc Returns zero if successful; otherwise it returns an error code.
*
* @xref AVIFileOpen
*
*************************************************************************/
STDAPI AVIFileCreate (PAVIFILE FAR *ppfile, LONG lParam,
		      CLSID FAR *pclsidHandler)
{
    CLSID   clsid;
    HRESULT hr;

    if (!iInit) {
	return ResultFromScode(CO_E_NOTINITIALIZED);
    }

//    AVIStreamInit();

    if (pclsidHandler)
	clsid = *pclsidHandler;
    else {
//    if (pfh == NULL)
//	pfh = &AVIFFileHandler;
    }

    if (FAILED(GetScode(hr = CoCreateInstance((REFCLSID) clsid,
					 NULL, CLSCTX_INPROC,
					 (REFIID) IID_IAVIFile,
					 (void FAR* FAR*)ppfile))))
	return hr;  // !!! PropagateHResult?

    return AVIERR_OK;
}

// Remove trailing spaces after a file...
void FixFourCC(LPSTR lp)
{
    int i;

    for (i = 3; i >= 0; i--) {
	if (lp[i] == ' ')
	    lp[i] = '\0';
	else
	    break;
    }
}

// Returns a pointer to the extension of a filename....
LPCSTR FindExtension(LPCSTR lp)
{
    LPCSTR lpExt = lp;
    int i;

// Goto end of string
    while (*lpExt != '\0')
    {
        ++lpExt;
    }

// Must be at least 2 characters in string
    if (lpExt - lp < 2)
        return NULL;

    lpExt -= 1;

// Does not count if last character is '.'
    if (*lpExt == '.')
        return NULL;

    lpExt -= 1;
// Now looking at second to the last character.  Check this and the two
// previous characters for a '.'

    for (i=1; i<=3; ++i)
    {
// Cannot have path separator here
        if (*lpExt == '/' || *lpExt == '\\')
            return NULL;

        if (*lpExt == '.')
        {
            ++lpExt;
	    return lpExt;
        }
        if (lpExt == lp)
            return NULL;
        --lpExt;
    }
    return NULL;
}

/**************************************************************************
* @doc INTERNAL GetHandlerFromFile
*
* @api PAVIFILEHANDLER | GetHandlerFromFile | Figure out what handler
*	to use for a file by looking at its extension, its RIFF type,
*	and possibly other things.
*
* @parm LPCSTR | szFile | The file to look at.
*
* @parm CLSID FAR * | pclsidHandler | Pointer to a classID.
*
* @comm We don't look at the extensions yet.  We need a better way to
*	add handlers.
*
* @rdesc Returns the <PAVIFILEHANDLER> to use, or NULL if it can't find
*	one.
*
* @xref AVIFileOpen AVIRegisterLoader
*
*************************************************************************/
#define	HKEY_AVIFILE_ROOT	HKEY_CLASSES_ROOT
static SZCODE aszRegRIFF[] = "AVIFile\\RIFFHandlers\\%.4s";
static SZCODE aszRegExt[] = "AVIFile\\Extensions";
static SZCODE aszRegClsid[] = "Clsid";
static SZCODE aszRegExtTmpl[] = "%s\\%.3s";

BOOL GetHandlerFromFile(LPCSTR szFile, CLSID FAR *pclsid)
{
    DWORD   dw[3];
    HMMIO   hmmio;
    LPCSTR   lpExt;
    char    achKey[100];
    char    achClass[100];
    LONG    lcbClass;

    // I hate share
    hmmio = mmioOpen((LPSTR) szFile, NULL, MMIO_READ | MMIO_DENYWRITE);

    if (hmmio == NULL)
        hmmio = mmioOpen((LPSTR) szFile, NULL, MMIO_READ | MMIO_DENYNONE);

    if (hmmio == NULL)
        hmmio = mmioOpen((LPSTR) szFile, NULL, MMIO_READ);

    if (hmmio == NULL)
        goto UseExtension;

    if (mmioRead(hmmio, (HPSTR) dw, sizeof(dw)) != sizeof(dw)) {
	mmioClose(hmmio, 0);
	goto UseExtension;
    }

    mmioClose(hmmio, 0);

    if (dw[0] != FOURCC_RIFF)
        goto UseExtension;

    FixFourCC((LPSTR) &dw[2]);

    // Look up the RIFF type in the registration database....
    wsprintf(achKey, aszRegRIFF, (LPSTR) &dw[2]);

    lcbClass = sizeof(achClass);
    RegQueryValue(HKEY_CLASSES_ROOT, achKey, achClass, &lcbClass);

    if (!FAILED(GetScode(CLSIDFromString(achClass, pclsid))))
	return TRUE;

UseExtension:
    lpExt = FindExtension(szFile);
    if (lpExt) {
	// Look up the extension in the registration database....
	wsprintf(achKey, aszRegExtTmpl, (LPSTR) aszRegExt, lpExt);
	
	lcbClass = sizeof(achClass);
	RegQueryValue(HKEY_CLASSES_ROOT, achKey, achClass, &lcbClass);

	if (!FAILED(GetScode(CLSIDFromString(achClass, pclsid))))
	    return TRUE;
    }

    // !!! Use IStorage?

    return FALSE;
}

/**************************************************************************
* @doc EXTERNAL AVIFileOpen
*
* @api LONG | AVIFileOpen | Opens an AVI file and returns a file interface
*	pointer used to access it.
*
* @parm PAVIFILE FAR * | ppfile | Pointer to the location used to return
*       the new <t PAVIFILE> file pointer.
*
* @parm LPCSTR | szFile | Specifies a zero-terminated string
*       containing the name of the file to open.
*
* @parm UINT | mode | Specifies the mode to use when opening the file.
*
*
*       @flag	OF_READ | Opens the file for reading only. This is the
*       	default, if OF_WRITE and OF_READWRITE are not specified.
*
*       @flag	OF_WRITE | Opens the file for writing. You should not
*       	read from a file opened in this mode.
*
*       @flag	OF_READWRITE | Opens the file for both reading and writing.
*
*       @flag	OF_CREATE | Creates a new file.
*       	If the file already exists, it is truncated to zero length.
*
*       @flag	OF_DENYWRITE | Opens the file and denies other
*       	processes write access to the file. <f AVIFileOpen> fails
*       	if the file has been opened in compatibility or for write
*       	access by any other process.
*
*       @flag	OF_DENYREAD | Opens the file and denies other
*       	processes read access to the file. <f AVIFileOpen> fails if the
*       	file has been opened in compatibility mode or for read access
*       	by any other process.
*
*       @flag	OF_DENYNONE | Opens the file without denying other
*       	processes read or write access to the file. <f AVIFileOpen>
*       	fails if the file has been opened in compatibility mode
*       	by any other process.
*
*	@flag	OF_EXCLUSIVE | Opens the file and denies other processes
*		any access to the file.  <f AVIFileOpen> will fail if any
*		other process has opened the file.
*
*       See <f OpenFile> for more information about these flags.
*
* @parm CLSID FAR * | pclsidHandler | Specifies a pointer to a class ID
*       identifying the handler you want to use. If NULL, the system
*       chooses one from the registration database based on the file
*       extension or the file's RIFF type.
*
* @comm In general, the mode specified is used to open
*	     the file.
*
*	Be sure to call <f AVIFileInit> at least once in your
*	application before calling this function, and to balance each
*	call to <f AVIFileInit> with a call to <f AVIFileExit>.
*
* @rdesc Returns zero if successful; otherwise returns an error code.
*	Possible error returns include:
*
*	@flag AVIERR_BADFORMAT | The file was corrupted or not in the
*	    proper format, and could not be read.
*
*	@flag AVIERR_MEMORY | The file could not be opened because
*	    there was not enough memory.
*
*	@flag AVIERR_FILEREAD | A disk error occurred while reading the
*	    file.
*
*	@flag AVIERR_FILEOPEN | A disk error occurred while opening the
*	    file.
*
*	@flag REGDB_E_CLASSNOTREG | No handler could be found to open
*	    this type of file.
*
* @xref <f AVIFileRelease> <f AVIFileInit>
*
*************************************************************************/
STDAPI AVIFileOpen (PAVIFILE FAR *ppfile,
			 LPCSTR szFile,
			 UINT mode,
			 CLSID FAR *pclsidHandler)
{
    CLSID   clsid;
    HRESULT hr;

// We used to just fail if AVIFileInit wasn't called
#if 0
    if (!iInit) {
	return ResultFromScode(E_UNEXPECTED);
    }
#endif

    // Now we do it for them
    hr = CoInitialize(NULL);

    // Let them know what they did wrong
    if (GetScode(hr) == NOERROR) {
#ifdef DEBUG
	MessageBox(NULL, "You didn't call AVIFileInit!", "Bad dog!",
		MB_OK | MB_ICONHAND);
#endif
    } else
	CoUninitialize();

    *ppfile = 0;

    if (pclsidHandler)
	clsid = *pclsidHandler;
    else {
	if (!GetHandlerFromFile(szFile, &clsid)) {
	    DPF("Couldn't find handler for %s\n", (LPSTR) szFile);
	    return ResultFromScode(REGDB_E_CLASSNOTREG);
	}
    }

    if (FAILED(GetScode(hr = CoCreateInstance((RCLSID) clsid,
					 NULL, CLSCTX_INPROC,
					 (REFIID) IID_IAVIFile,
					 (void FAR* FAR*)ppfile)))) {
	DPF("CoCreateInstance returns %08lx\n", (DWORD) hr);
	return hr;
    }

    //
    // Let's simplify things for the handlers.  They will only see...
    //		OF_CREATE | OF_READWRITE	or...
    //		OF_READWRITE			or...
    //		OF_READ
    //

    if (mode & OF_READWRITE)
	mode &= ~(OF_WRITE | OF_READ);

    if (mode & OF_CREATE) {
	mode &= ~(OF_WRITE | OF_READ);
	mode |= OF_READWRITE;
    }

    if (mode & OF_WRITE) {
	mode &= ~(OF_WRITE | OF_READ);
	mode |= OF_READWRITE;
    }

    if (FAILED(GetScode(hr = (*ppfile)->Open(szFile, mode)))) {
	DPF("Open method returns %08lx\n", (DWORD) hr);
	(*ppfile)->Release();
	*ppfile = NULL;
    }

    return hr;
}

/**************************************************************************
* @doc EXTERNAL AVIFileAddRef
*
* @api LONG | AVIFileAddRef | Increases the reference count of an AVI file.
*
* @parm PAVIFILE | pfile | Specifies the handle for an open AVI file.
*
* @rdesc Returns zero on success; otherwise returns an error code.
*
* @comm Balance each call to <f AVIFileAddRef> with a call to
*       <f AVIFileRelease>.
*
* @xref <f AVIFileRelease>
*
*************************************************************************/
STDAPI_(ULONG) AVIFileAddRef(PAVIFILE pfile)
{
    return pfile->AddRef();
}

/**************************************************************************
* @doc EXTERNAL AVIFileRelease
*
* @api LONG | AVIFileRelease | Reduces the reference count of an AVI file
*	interface handle by one, and closes the file if the count reaches
*	zero.
*
* @parm PAVIFILE | pfile | Specifies a handle to an open AVI file.
*
* @comm Balance each call to <f AVIFileAddRef> or <f AVIFileOpen>
*       a call to <f AVIFileRelease>.
*
* @devnote Currently, this saves all changes to the file.  Should a separate
*	Save command be needed to do this?
*
* @rdesc Returns the reference count of the file.  This return value
*	should be used only for debugging purposes.
*
* @xref AVIFileOpen AVIFileAddRef
*
*************************************************************************/
STDAPI_(ULONG) AVIFileRelease(PAVIFILE pfile)
{
    return pfile->Release();
}

/**************************************************************************
* @doc EXTERNAL AVIFileInfo
*
* @api LONG | AVIFileInfo | Obtains information about an AVI file.
*
* @parm PAVIFILE | pfile | Specifies a handle to an open AVI file.
*
* @parm AVIFILEINFO FAR * | pfi | Pointer to the structure used to
*       return file information.
*
* @parm LONG | lSize | Specifies the size of the structure.  This value
*	should be at least sizeof(AVIFILEINFO), obviously.
*
* @rdesc Returns zero if successful; otherwise it returns an error code.
*
*************************************************************************/
STDAPI AVIFileInfo	         (PAVIFILE pfile, AVIFILEINFO FAR * pfi,
				  LONG lSize)
{
    _fmemset(pfi, 0, (int)lSize);
    return pfile->Info(pfi, lSize);
}

/**************************************************************************
* @doc EXTERNAL AVIFileGetStream
*
* @api LONG | AVIFileGetStream | Returns a pointer to a stream interface
*      that is a component of a file.
*
* @parm PAVIFILE | pfile | Specifies a handle to an open AVI file.
*
* @parm PAVISTREAM FAR * | ppavi | Pointer to the return location
*       for the new stream interface pointer.
*
* @parm DWORD | fccType | Specifies a four-character code
*       indicating the type of stream to be opened.
*       Zero indicates that any stream can be opened. The following
*       definitions apply to the data commonly
*       found in AVI streams:
*
* @flag streamtypeAUDIO | Indicates an audio stream.
* @flag streamtypeMIDI | Indicates a MIDI stream.
* @flag streamtypeTEXT | Indicates a text stream.
* @flag streamtypeVIDEO | Indicates a video stream.
*
* @parm LONG | lParam | Specifies an integer indicating which stream
*       of the type defined by <p fccType> should actually be accessed.
*
* @comm Balance each call to <f AVIFileGetStream> with a call to
*       <f AVIStreamRelease> using the stream handle returned.
*
* @rdesc Returns zero if successful; otherwise it returns an error code.
*	Possible error codes include:
*
*	@flag AVIERR_NODATA | There is no stream in the file corresponding
*	    to the values passed in for <p fccType> and <p lParam>.
*	@flag AVIERR_MEMORY | Not enough memory.
*
* @xref <f AVIStreamRelease>
*
*************************************************************************/
STDAPI AVIFileGetStream     (PAVIFILE pfile, PAVISTREAM FAR * ppavi, DWORD fccType, LONG lParam)
{
    return pfile->GetStream(ppavi, fccType, lParam);
}

#if 0
// !!! This would be used to save changes, if AVIFileRelease didn't do that.
STDAPI AVIFileSave		 (PAVIFILE pfile,
					  LPCSTR szFile,
					  AVISAVEOPTIONS FAR *lpOptions,
					  AVISAVECALLBACK lpfnCallback,
					  PAVIFILEHANDLER pfh)
{
    if (pfile->FileSave == NULL)
	return -1;

    return pfile->FileSave(pfile, szFile, lpOptions, lpfnCallback);
}
#endif

/**************************************************************************
* @doc EXTERNAL AVIFileCreateStream
*
* @api LONG | AVIFileCreateStream | Creates a new stream in an existing file,
*      and returns a stream interface pointer for it.
*
* @parm PAVIFILE | pfile | Specifies a handle to an open AVI file.
*
* @parm PAVISTREAM FAR * | ppavi | Specifies a pointer used to return the new
*       stream interface pointer.
*
* @parm AVISTREAMINFO FAR * | psi | Specifies a pointer to a structure
*       containing information about the new stream. This structure
*       contains the type of the new stream and its sample rate.
*
* @comm Balance each call to <f AVIFileCreateStream> with a call to
*       <f AVIStreamRelease> using the returned stream handle.
*
*       This function fails with a return value of AVIERR_READONLY unless
*       the file was opened with write permission.
*
*       After creating the stream, call <f AVIStreamSetFormat>
*       before using <f AVIStreamWrite> to write to the stream.
*
* @rdesc Returns zero if successful; otherwise it returns an error code.
*
* @xref <f AVIStreamRelease> <f AVIFileGetStream> <f AVIStreamSetFormat>
*
*************************************************************************/
STDAPI AVIFileCreateStream  (PAVIFILE pfile,
					 PAVISTREAM FAR *ppavi,
					 AVISTREAMINFO FAR *psi)
{
    *ppavi = NULL;
    return pfile->CreateStream(ppavi, psi);
}

/**************************************************************************
* @doc INTERNAL AVIFileAddStream
*
* @api LONG | AVIFileAddStream | Adds an existing stream into
*	an existing file, and returns a stream interface pointer for it.
*
* @parm PAVIFILE | pfile | Specifies a handle to an open AVI file.
*
* @parm PAVISTREAM | pavi | Specifies a stream interface pointer
*       for the stream being added.
*
* @parm PAVISTREAM FAR * | ppaviNew | Pointer to a buffer used
*       to return the new stream interface pointer.
*
* @comm Balance each call to <f AVIFileAddStream> with a call to
*       <f AVIStreamRelease> using the returned stream handle.
*
*	This call fails with a return value of AVIERR_READONLY unless
*	the file was opened with write permission.
*
* @devnote This function still doesn't really work.  Perhaps it should just
*	be a helper function that gets data from the stream and calls
*	AVIFileCreateStream, then copies the data from one stream to another.
*
* @rdesc Returns zero if successful; otherwise it returns an error code.
*
* @xref AVIStreamRelease AVIFileGetStream AVIFileCreateStream
*
*************************************************************************/
#if 0
STDAPI AVIFileAddStream	(PAVIFILE pfile,
					 PAVISTREAM pavi,
					 PAVISTREAM FAR * ppaviNew)
{
//    if (pfile->FileAddStream == NULL)
//	return -1;

    return pfile->AddStream(pavi, ppaviNew);
}
#endif

/**************************************************************************
* @doc EXTERNAL AVIFileWriteData
*
* @api LONG | AVIFileWriteData | Writes some additional data to the file.
*
* @parm PAVIFILE | pfile | Specifies a handle to an open AVI file.
*
* @parm DWORD | ckid | Specifies a four-character code identifying the data.
*
* @parm LPVOID | lpData | Specifies a pointer to the data to write.
*
* @parm LONG | cbData | Specifies the size of the memory block
*       referenced by <p lpData>.
*
* @comm This function fails with a return value of AVIERR_READONLY unless
*       the file was opened with write permission.
*
*       Use <f AVIStreamWriteData> instead of this function to write
*       data that applies to an individual stream.
*
* @devnote !!! Somewhere, we should specify some types.
*	!!! Should the data block contain the ckid and cksize?
*
* @rdesc Returns zero if successful; otherwise it returns an error code.
*
* @xref <f AVIStreamWriteData> <f AVIFileReadData>
*
*************************************************************************/
STDAPI AVIFileWriteData	(PAVIFILE pfile,
					 DWORD ckid,
					 LPVOID lpData,
					 LONG cbData)
{
//    if (pfile->FileWriteData == NULL)
//	return -1;

    return pfile->WriteData(ckid, lpData, cbData);
}

/**************************************************************************
* @doc EXTERNAL AVIFileReadData
*
* @api LONG | AVIFileReadData | Reads optional header data from the file.
*
* @parm PAVIFILE | pfile | Specifies a handle to an open AVI file.
*
* @parm DWORD | ckid | Specifies a four-character code identifying the data.
*
* @parm LPVOID | lpData | Specifies a pointer to a buffer used to return
*       the data read.
*
* @parm LONG FAR * | lpcbData | Specifies a pointer to a location indicating
*	the size of the memory block referred to by <p lpData>. If
*	the read is successful, the value is changed to indicate the
*	amount of data read.
*
* @devnote !!! Somewhere, we should specify some types.
*	!!! Should the data block contain the ckid and cksize?
*
*	@comm Do not use this function to read video and audio data. Use it
*  only to read additional information such as author
*	information or copyright information that applies to the file
*	as a whole. Information that applies to a single stream should
*	be read using <f AVIStreamReadData>.
*	
* @rdesc Returns zero if successful; otherwise it returns an error code.
*       The return value AVIERR_NODATA indicates that data with the
*       requested chunk ID does not exist.
*
* @xref <f AVIStreamReadData> <f AVIFileWriteData>
*
*************************************************************************/
STDAPI AVIFileReadData	(PAVIFILE pfile,
					 DWORD ckid,
					 LPVOID lpData,
					 LONG FAR * lpcbData)
{
    return pfile->ReadData(ckid, lpData, lpcbData);
}

/**************************************************************************
* @doc EXTERNAL AVIFileEndRecord
*
* @api LONG | AVIFileEndRecord | Marks the end of a record, if writing out
*	a strictly interleaved file.
*
* @parm PAVIFILE | pfile | Specifies a handle to a currently open AVI file.
*
* @comm <f AVIFileSave> uses this function when writing files that are
*	have audio interleaved every frame.  In general, applications
*	should not need to use this function.
*
* @rdesc Returns zero if successful; otherwise it returns an error code.
*
* @xref <f AVIFileSave> <f AVIStreamWrite>
*
*************************************************************************/
STDAPI AVIFileEndRecord	(PAVIFILE pfile)
{
//    if (pfile->FileEndRecord == NULL)
//	return -1;

    return pfile->EndRecord();
}



/**************************************************************************
* @doc EXTERNAL AVIStreamAddRef
*
* @api LONG | AVIStreamAddRef | Increases the reference count of an AVI stream.
*
* @parm PAVISTREAM | pavi | Specifies a handle to an open AVI stream.
*
* @comm Balance each call to <f AVIStreamAddRef> with a call to
*       <f AVIStreamRelease>.
*
* @rdesc Returns the current reference count of the stream.  This value
*	should only be used for debugging purposes.
*
* @xref <f AVIStreamRelease>
*
*************************************************************************/
STDAPI_(ULONG) AVIStreamAddRef       (PAVISTREAM pavi)
{
    return pavi->AddRef();
}

/**************************************************************************
* @doc EXTERNAL AVIStreamRelease
*
* @api LONG | AVIStreamRelease | Reduces the reference count of an AVI stream
*	interface handle by one, and closes the stream if the count reaches
*	zero.
*
* @parm PAVISTREAM | pavi | Specifies a handle to an open stream.
*
* @comm Balance each call to <f AVIStreamAddRef> or <f AVIFileGetStream>
*       with a call to <f AVIStreamRelease>.
*
* @rdesc Returns the current reference count of the stream.  This value
*	should only be used for debugging purposes.
*
* @xref <f AVIFileGetStream> <f AVIStreamAddRef>
*
*************************************************************************/
STDAPI_(ULONG) AVIStreamRelease        (PAVISTREAM pavi)
{
    return pavi->Release();
}

/**************************************************************************
* @doc EXTERNAL AVIStreamInfo
*
* @api LONG | AVIStreamInfo | Obtains stream header information.
*
* @parm PAVISTREAM | pavi | Specifies a handle to an open stream.
*
* @parm AVISTREAMINFO FAR * | psi | Specifies a pointer to a structure
*       used to return stream information.
*
* @parm LONG | lSize | Specifies the size of the structure used for
*       <p psi>.
*
* @rdesc Returns zero if successful; otherwise it returns an error code.
*
*************************************************************************/
STDAPI AVIStreamInfo         (PAVISTREAM pavi, AVISTREAMINFO FAR * psi, LONG lSize)
{
    _fmemset(psi, 0, (int)lSize);

    return pavi->Info(psi, lSize);
}

/**************************************************************************
* @doc EXTERNAL AVIStreamFindSample
*
* @api LONG | AVIStreamFindSample | Returns the position of
*      a key frames or non-empty frame relative to the specified position.
*
* @parm PAVISTREAM | pavi | Specifies a handle to an open stream.
*
* @parm LONG | lPos | Specifies the starting position
*       for the search.
*
* @parm LONG | lFlags | The following flags are defined:
*
* @flag FIND_KEY     | Finds a key frame.
* @flag FIND_ANY     | Finds a non-empty sample.
* @flag FIND_FORMAT  | Finds a format change.
*
* @flag FIND_NEXT    | Finds nearest sample, frame, or format change
*                      searching forward. The current sample is
*                      included in the search. Use this flag with the
*                      FIND_ANY, FIND_KEY, or FIND_FORMAT flag.
*
* @flag FIND_PREV    | Finds nearest sample, frame, or format change
*                      searching backwards. The current sample is
*                      included in the search. Use this flag with the
*                      FIND_ANY, FIND_KEY, or FIND_FORMAT flag.
*
*
* @comm The FIND_KEY, FIND_ANY, and FIND_FORMAT flags are mutually exclusive.
*       The FIND_NEXT and FIND_PREV flags are also mutually exclusive.
*       For example:
*
* @ex       FIND_PREV|FIND_KEY      Returns the first key sample prior to or at
*                               <p lPos>.
*
*       FIND_PREV|FIND_ANY      Returns the first non-empty sample prior to
*                               or at <p lPos>.
*
*       FIND_NEXT|FIND_KEY      Returns the first key sample after <p lPos>,
*                               or -1 if a key sample does not follow <p lPos>.
*
*       FIND_NEXT|FIND_ANY      Returns the first non-empty sample after <p lPos>,
*                               or -1 if a sample does not exist after <p lPos>.
*
*       FIND_NEXT|FIND_FORMAT   Returns the first format change after or
*                               at <p lPos>, or -1 if the stream does not
*                               have format changes.
*
*       FIND_PREV|FIND_FORMAT   Returns the first format change prior to
*                               or at <p lPos>. If the stream does not
*                               have format changes, it returns the first sample
*
* @rdesc Returns the position found.  In many boundary cases, this
*	function will return -1; see the example above for details.
*
*************************************************************************/
STDAPI_(LONG) AVIStreamFindSample(PAVISTREAM pavi, LONG lPos, LONG lFlags)
{
    // Default to Find Previous Key Frame
    if ((lFlags & FIND_TYPE) == 0)
        lFlags |= FIND_KEY;
    if ((lFlags & FIND_DIR) == 0)
        lFlags |= FIND_PREV;

    return pavi->FindSample(lPos, lFlags);
}

/**************************************************************************
* @doc EXTERNAL AVIStreamReadFormat
*
* @api LONG | AVIStreamReadFormat | Reads the stream format data.
*
* @parm PAVISTREAM | pavi | Specifies a handle to an open stream.
*
* @parm LONG | lPos | Specifies the position in the stream
*       used to obtain the format data.
*
* @parm LPVOID | lpFormat | Specifies a pointer to a buffer
*       used to return the format data.
*
* @parm LONG FAR * | lpcbFormat | Specifies a pointer to a
*       location indicating the size of the memory block
*       referred to by <p lpFormat>. On return, the value is
*       changed to indicate the amount of data read. If
*       <p lpFormat> is NULL, this parameter can be used
*       to obtain the amount of memory needed to return the format.
*
* @comm This function will return part of the format even if the buffer
*	provided is not large enough to hold the entire format. In this case
*	the return value will be AVIERR_BUFFERTOOSMALL, and the location
*	referenced by <p lpcbFormat> will be filled in with the size
*	of the entire format.
*
*	This is useful because it allows you to use a buffer the
*	size of a <t BITMAPINFOHEADER> structure and
*	retrieve just the common part of the video format if you are not
*	interested in extended format information or palette information.
*
* @rdesc Returns zero if successful, otherwise it returns an error code.
*
*************************************************************************/
STDAPI AVIStreamReadFormat   (PAVISTREAM pavi, LONG lPos,
					  LPVOID lpFormat, LONG FAR *lpcbFormat)
{
//    if (pavi->StreamReadFormat == NULL)
//	return -1;

    return pavi->ReadFormat(lPos, lpFormat, lpcbFormat);
}

/**************************************************************************
* @doc EXTERNAL AVIStreamSetFormat
*
* @api LONG | AVIStreamSetFormat | Sets the format of a stream at the
*      specified position.
*
* @parm PAVISTREAM | pavi | Specifies a handle to open stream.
*
* @parm LONG | lPos | Specifies the position in the stream to
*       receive the format.
*
* @parm LPVOID | lpFormat | Specifies a pointer to a structure
*       containing the new format.
*
* @parm LONG | cbFormat | Specifies the size of the block of memory
*       referred to by <p lpFormat> in bytes.
*
* @comm After creating a new stream with <f AVIFileCreateStream>,
*       call this function to set the stream's format.
*
*      The handler for writing AVI files does not, in general, accept
*      format changes. Aside from setting the initial format for a
*      stream, only changes in the palette of a video stream are allowed
*      in an AVI file. The palette change must be after
*      any frames already written to the AVI file.  Other handlers may
*     impose different restrictions.
*
* @rdesc Returns zero if successful, otherwise it returns an error code.
*
* @xref <f AVIFileCreateStream> <f AVIStreamReadFormat>
*
*************************************************************************/
STDAPI AVIStreamSetFormat   (PAVISTREAM pavi, LONG lPos,
					 LPVOID lpFormat, LONG cbFormat)
{
//    if (pavi->StreamSetFormat == NULL)
//	return -1;

    return pavi->SetFormat(lPos, lpFormat, cbFormat);
}

/**************************************************************************
* @doc EXTERNAL AVIStreamReadData
*
* @api LONG | AVIStreamReadData | Reads optional header data from a stream.
*
* @parm PAVISTREAM | pavi | Specifies a handle to an open stream.
*
* @parm DWORD | ckid | Specifies a four-character code identifying the data.
*
* @parm LPVOID | lpData | Specifies a pointer to used to return
*       the data read.
*
* @parm LONG FAR * | lpcbData | Points to a location which
*       specifies the buffer size used for <p lpData>.
*	If the read is successful, AVIFile changes this value
*       to indicate the amount of data written into the buffer for
*       <p lpData>.
*
* @comm This function only retrieves header information
*       from the stream. To read the actual multimedia content of the
*       stream, use <f AVIStreamRead>.
*
* @devnote !!! Somewhere, we should specify some types.
*	!!! Should the data block contain the ckid and cksize?
*
* @rdesc Returns zero if successful; otherwise it returns an error code.
*        The return value AVIERR_NODATA indicates the system could not
*        find any data with the specified chunk ID.
*
* @xref <f AVIFileReadData> <f AVIStreamWriteData> <f AVIStreamWrite>
*
*************************************************************************/
STDAPI AVIStreamReadData     (PAVISTREAM pavi, DWORD ckid, LPVOID lpData, LONG FAR *lpcbData)
{
//    if (pavi->StreamReadData == NULL)
//	return -1;

    return pavi->ReadData(ckid, lpData, lpcbData);
}

/**************************************************************************
* @doc EXTERNAL AVIStreamWriteData
*
* @api LONG | AVIStreamWriteData | Writes optional data to the stream.
*
* @parm PAVISTREAM | pavi | Specifies a handle to an open stream.
*
* @parm DWORD | ckid | Specifies a four-character code identifying the data.
*
* @parm LPVOID | lpData | Specifies a pointer to a buffer containing
*       the data to write.
*
* @parm LONG | cbData | Indicates the number of bytes of data to be copied
*	from <p lpData> into the stream.
*
* @comm This function only writes header information to the stream.
*       To write the actual multimedia content of the stream, use
*       <f AVIStreamWrite>. Use <f AVIFileWriteData> to write
*       data that applies to an entire file.
*
*       This call fails with a return value of AVIERR_READONLY unless
*       the file was opened with write permission.
*
* @devnote !!! Somewhere, we should specify some types.
*	!!! Should the data block contain the ckid and cksize?
*
* @rdesc Returns zero if successful; otherwise it returns an error code.
*
* @xref <f AVIFileWriteData> <f AVIStreamReadData> <f AVIStreamWrite>
*
*************************************************************************/
STDAPI AVIStreamWriteData     (PAVISTREAM pavi, DWORD ckid, LPVOID lpData, LONG cbData)
{
    return pavi->WriteData(ckid, lpData, cbData);
}

/**************************************************************************
* @doc EXTERNAL AVIStreamRead
*
* @api LONG | AVIStreamRead | Reads audio or video data from a stream.
*
* @parm PAVISTREAM | pavi | Specifies a handle to an open stream.
*
* @parm LONG | lStart | Specifies the starting sample to read.
*
* @parm LONG | lSamples | Specifies the number of samples to read.
*
* @parm LPVOID | lpBuffer | Specifies a pointer to a buffer used to
*       return the data.
*
* @parm LONG | cbBuffer | Specifies the size of buffer pointed to by <p lpBuffer>.
*
* @parm LONG FAR * | plBytes | Specifies a pointer to the location
*       used to return number of bytes of data written into the
*       buffer for <p lpBuffer>.  <p plBytes> can be NULL.
*
* @parm LONG FAR * | plSamples | Specifies a pointer to the location
*       used to return the number of samples written into the buffer for
*       for <p lpBuffer>.  <p plSamples> can be NULL.
*
* @comm If <p lpBuffer> is NULL, this function does not read
*       any data; it returns information about the size of data
*       that would be read.
*
*	See <f AVIStreamLength> for a discussion of how sample numbers
*	correspond to the data you want to read.
*
* @rdesc Returns zero if successful, or an error code.  Use <p plBytes>
*	and <p plSamples> to find out how much was actually read.
*
*	Possible errors include:
*
*	@flag AVIERR_BUFFERTOOSMALL | The buffer size <p cbBuffer> was
*	    too small to read in even a single sample of data.
*
*	@flag AVIERR_MEMORY | There was not enough memory for some
*	    reason to complete the read operation.
*
*	@flag AVIERR_FILEREAD | A disk error occurred while reading the
*	    file.
*
* @xref <f AVIFileGetStream> <f AVIStreamFindSample> <f AVIStreamWrite>
*
*************************************************************************/
STDAPI AVIStreamRead         (PAVISTREAM pavi,
					  LONG lStart, LONG lSamples,
					  LPVOID lpBuffer, LONG cbBuffer,
					  LONG FAR * plBytes, LONG FAR * plSamples)
{
//    if (pavi->StreamRead == NULL)
//	return -1;

    return pavi->Read(lStart, lSamples, lpBuffer, cbBuffer, plBytes, plSamples);
}

/**************************************************************************
* @doc EXTERNAL AVIStreamWrite
*
* @api LONG | AVIStreamWrite | Writes data to a stream.
*
* @parm PAVISTREAM | pavi | Specifies a handle to an open stream.
*
* @parm LONG | lStart | Specifies the starting sample to write.
*
* @parm LONG | lSamples | Specifies the number of samples to write.
*
* @parm LPVOID | lpBuffer | Specifies a pointer to buffer
*       containing the data to write.
*
* @parm LONG | cbBuffer | Specifies the size of buffer used by <p lpBuffer>.
*
* @parm DWORD | dwFlags | Specifies any flags associated with this data.
*       The following flags are defined:
*
* @flag AVIIF_KEYFRAME | Indicates this data does not rely on preceding
*       data in the file.
*
* @parm LONG FAR * | plSampWritten | Specifies a pointer to a location
*       used to return the number of samples written. This can be set
*       to NULL.
*
* @parm LONG FAR * | plBytesWritten | Specifies a pointer to a location
*       used to return the number of bytes written. This can be set
*       to NULL.
*
* @comm The default AVI file handler only supports writing to the end
*	of a stream.  The WAVE file handler supports writing anywhere.
*
*	This function overwrites existing data, rather than inserting
*	new data.
*
*	See <f AVIStreamLength> for a discussion of how sample numbers
*	correspond to the data you want to read.
*
* @rdesc Returns zero if successful; otherwise it returns an error code.
*
* @xref <f AVIFileGetStream> <f AVIFileCreateStream> <f AVIStreamRead>
*
*************************************************************************/
STDAPI AVIStreamWrite        (PAVISTREAM pavi,
			      LONG lStart, LONG lSamples,
			      LPVOID lpBuffer, LONG cbBuffer,
			      DWORD dwFlags,
			      LONG FAR *plSampWritten,
			      LONG FAR *plBytesWritten)
{
//    if (pavi->StreamWrite == NULL)
//	return -1;

    return pavi->Write(lStart, lSamples, lpBuffer, cbBuffer,
		       dwFlags, plSampWritten, plBytesWritten);
}

/**************************************************************************
* @doc INTERNAL AVIStreamDelete
*
* @api LONG | AVIStreamDelete | Deletes data from a stream.
*
* @parm PAVISTREAM | pavi | Specifies a handle to an open stream.
*
* @parm LONG | lStart | Specifies the starting sample to delete.
*
* @parm LONG | lSamples | Specifies the number of samples to delete.
*
* @devnote This isn't implemented by anybody yet.  Should it be?  Wave files,
*	for instance, would have to copy lots of data around....
*
* @rdesc Returns zero if successful; otherwise it returns an error code.
*
* @xref
*
*************************************************************************/
STDAPI AVIStreamDelete       (PAVISTREAM pavi, LONG lStart, LONG lSamples)
{
//    if (pavi->StreamDelete == NULL)
//	return -1;

    return pavi->Delete(lStart, lSamples);
}

#if 0
// !!! should this exist?
STDAPI AVIStreamClone	 (PAVISTREAM pavi, PAVISTREAM FAR *ppaviNew)
{
//    if (pavi->StreamClone == NULL)
//	return -1;

    return pavi->Clone(ppaviNew);
}
#endif

/**************************************************************************
* @doc EXTERNAL AVIStreamStart
*
* @api LONG | AVIStreamStart | Returns the starting sample of the stream.
*
* @parm PAVISTREAM | pavi | Specifies a handle to an open stream.
*
* @rdesc Returns the starting sample number for the stream, or -1 on error.
*
* @comm See <f AVIStreamLength> for a discussion of how sample numbers
*	correspond to the data you want to read.
*
* @xref <f AVIStreamSampleToTime> <f AVIStreamLength>
*
*************************************************************************/
STDAPI_(LONG) AVIStreamStart        (PAVISTREAM pavi)
{
    AVISTREAMINFO	    avistream;

    pavi->Info(&avistream, sizeof(avistream));

    return (LONG) avistream.dwStart;
}

/**************************************************************************
* @doc EXTERNAL AVIStreamLength
*
* @api LONG | AVIStreamLength | Returns the length of the stream in samples.
*
* @parm PAVISTREAM | pavi | Specifies a handle to an open stream.
*
* @devnote Currently, this doesn't call a handler function at all.
*
* @rdesc Returns the stream's length in samples, or -1 on error.
*
* @comm Values in samples can be converted to milliseconds using
*	the <f AVIStreamSampleToTime> function.
*
*	For video streams, each sample generally corresponds to a
*	frame of video.  There may, however, be sample numbers for
*	which no video data is actually present: If <f AVIStreamRead>
*	is called at those positions, it will return a data length
*	of zero bytes.  You can use <f AVIStreamFindSample> with the
*	FIND_ANY flag to find sample numbers which actually have data.
*
*	For audio streams, each sample corresponds to one "block"
*	of data.  Note the conflicting terminology here: if you're
*	working with 22KHz ADPCM data, each block of audio data is
*	256 bytes, corresponding to about 500 "audio samples" which
*	will be presented to the speaker each 22000th of a second.
*	From the point of view of the AVIFile APIs, however, each 256-byte
*	block is a single sample, because they cannot be subdivided.
*
*	Note that the stream's starting position may not be zero; see
*	<f AVIStreamStart>.  Valid positions within a stream range from
*	start to start+length; there is no actual data present at position
*	start+length, but that corresponds to a time after the last data
*	has been rendered.
*
* @xref <f AVIStreamInfo>
*
*************************************************************************/
STDAPI_(LONG) AVIStreamLength       (PAVISTREAM pavi)
{
    AVISTREAMINFO	    avistream;
    HRESULT		    hr;

    hr = pavi->Info(&avistream, sizeof(avistream));

    if (hr != NOERROR) {
	DPF("Error in AVIStreamLength!\n");
	return 1;
    }

    return (LONG) avistream.dwLength;
}

/**************************************************************************
* @doc EXTERNAL AVIStreamTimeToSample
*
* @api LONG | AVIStreamTimeToSample | Converts from milliseconds to samples.
*
* @parm PAVISTREAM | pavi | Specifies a handle to an open stream.
*
* @parm LONG | lTime | Specifies the time in milliseconds.
*
* @devnote Currently, this doesn't call a handler function at all.
*
* @comm Samples typically correspond to audio samples or video frames.
*       Other stream types might support different formats than these.

* @rdesc Returns the converted time, or -1 on error.
*
* @xref AVIStreamSampleToTime
*
*************************************************************************/
STDAPI_(LONG) AVIStreamTimeToSample (PAVISTREAM pavi, LONG lTime)
{
    AVISTREAMINFO	    avistream;
    HRESULT		    hr;
    LONG		    lSample;

    // Invalid time
    if (lTime < 0)
	return -1;

    hr = pavi->Info(&avistream, sizeof(avistream));

    if (hr != NOERROR || avistream.dwScale == 0) {
	DPF("Error in AVIStreamTimeToSample!\n");
	return lTime;
    }

    // This is likely to overflow if we're not careful for long AVIs
    // so keep the 1000 inside the brackets.

    if (avistream.dwRate / avistream.dwScale < 1000)
	lSample =  muldivrd32(lTime, avistream.dwRate, avistream.dwScale * 1000);
    else
	lSample =  muldivru32(lTime, avistream.dwRate, avistream.dwScale * 1000);

    lSample = min(max(lSample, (LONG) avistream.dwStart),
		  (LONG) (avistream.dwStart + avistream.dwLength));

    return lSample;
}

/**************************************************************************
* @doc EXTERNAL AVIStreamSampleToTime
*
* @api LONG | AVIStreamSampleToTime | Converts from samples to milliseconds.
*   Samples can correspond to blocks of audio samples, video frames, or other format
*   depending on the stream type.
*
* @parm PAVISTREAM | pavi | Specifies a handle to an open stream.
*
* @parm LONG | lSample | Specifies the position information.
*
* @rdesc Returns the converted time, or -1 on error.
*
* @xref <f AVIStreamTimeToSample>
*
*************************************************************************/
STDAPI_(LONG) AVIStreamSampleToTime (PAVISTREAM pavi, LONG lSample)
{
    AVISTREAMINFO	    avistream;
    HRESULT		    hr;

    hr = pavi->Info(&avistream, sizeof(avistream));

    if (hr != NOERROR || avistream.dwRate == 0) {
	DPF("Error in AVIStreamSampleToTime!\n");
	return lSample;
    }

    lSample = min(max(lSample, (LONG) avistream.dwStart),
		  (LONG) (avistream.dwStart + avistream.dwLength));

    // lSample * 1000 would overflow too easily
    if (avistream.dwRate / avistream.dwScale < 1000)
	return muldivrd32(lSample, avistream.dwScale * 1000, avistream.dwRate);
    else
	return muldivru32(lSample, avistream.dwScale * 1000, avistream.dwRate);
}


/**************************************************************************
* @doc EXTERNAL AVIStreamOpenFromFile
*
* @api LONG | AVIStreamOpenFromFile | This function provides a convenient
*      way to open a single stream from a file.
*
* @parm PAVISTREAM FAR * | ppavi | Specifies a pointer to the location
*       used to return the new stream handle.
*
* @parm LPCSTR | szFile | Specifies a zero-terminated string containing
*       the name of the file to open.
*
* @parm DWORD | fccType | Specifies a four-character code
*       indicating the type of stream to be opened.
*       Zero indicates that any stream can be opened. The following
*       definitions apply to the data commonly
*       found in AVI streams:
*
* @flag streamtypeAUDIO | Indicates an audio stream.
* @flag streamtypeMIDI | Indicates a MIDI stream.
* @flag streamtypeTEXT | Indicates a text stream.
* @flag streamtypeVIDEO | Indicates a video stream.
*
* @parm LONG | lParam | Indicates which stream of the type specified in
*	<p fccType> should actually be accessed.
*
* @parm UINT | mode | Specifies the mode to use when opening the file.
*       This function can only open existing streams so the OF_CREATE
*       mode flag cannot be used. See
*       <f OpenFile> for more information about the available flags.
*
* @parm CLSID FAR * | pclsidHandler | Specifies a pointer to a class ID
*       identifying the handler you want to use. If NULL, the system
*       chooses one from the registration database based on the file
*       extension or the file's RIFF type.
*
* @comm Balance each call to <f AVIStreamOpenFromFile> with a
*       call to <f AVIStreamRelease> using the stream handle returned.
*
*	This function calls <f AVIFileOpen>, <f AVIFileGetStream>, and
*       <f AVIFileRelease>.
*
* @rdesc Returns zero if successful; otherwise it returns an error code.
*
* @xref <f AVIFileOpen> <f AVIFileGetStream>
*
*************************************************************************/
STDAPI AVIStreamOpenFromFile(PAVISTREAM FAR *ppavi,
				  LPCSTR szFile,
				  DWORD fccType, LONG lParam,
				  UINT mode, CLSID FAR *pclsidHandler)
{
    PAVIFILE	pfile;
    HRESULT	hr;

    hr = AVIFileOpen(&pfile, szFile, mode, pclsidHandler);

    if (!FAILED(GetScode(hr))) {
	hr  = AVIFileGetStream(pfile, ppavi, fccType, lParam);

        AVIFileRelease(pfile);  // the stream still has a reference to the file
    }

    return hr;
}

/**************************************************************************
* @doc EXTERNAL AVIStreamCreate
*
* @api LONG | AVIStreamCreate | Creates a stream not associated with any
*	file.
*
* @parm PAVISTREAM FAR * | ppavi | Pointer to a location to return the
*	new stream handle.
*
* @parm LONG | lParam1 | Specifies stream-handler specific information.
*
* @parm LONG | lParam2 | Specifies stream-handler specific information.
*
* @parm CLSID FAR * | pclsidHandler | Pointer to the class ID used
*       for the stream.
*
* @comm Balance each call to <f AVIStreamCreate> with a
*       call to <f AVIStreamRelease>.
*
*	You should not need to call this function; functions like
*	<f CreateEditableStream> and <f AVIMakeCompressedStream>
*	use it internally.
*
* @rdesc Returns zero if successful; otherwise returns an error code.
*
* @xref <f AVIFileOpen> <f AVIFileGetStream>
*
*************************************************************************/
STDAPI AVIStreamCreate (PAVISTREAM FAR *ppavi, LONG lParam1, LONG lParam2,
		      CLSID FAR *pclsidHandler)
{
    CLSID   clsid;
    HRESULT hr;

    if (!iInit) {
	return ResultFromScode(E_UNEXPECTED);
    }

    if (pclsidHandler)
	clsid = *pclsidHandler;
    else {
	return ResultFromScode(REGDB_E_CLASSNOTREG);
    }

    if (FAILED(GetScode(hr = CoCreateInstance((REFCLSID) clsid,
					 NULL, CLSCTX_INPROC,
					 (REFIID) IID_IAVIStream,
					 (void FAR* FAR*)ppavi))))
	return hr;

    if (FAILED(GetScode(hr = (*ppavi)->Create(lParam1, lParam2)))) {
	(*ppavi)->Release();
	// AVIStreamExit();
    }

    return AVIERR_OK;
}


/**************************************************************************
* @doc EXTERNAL AVIStreamBeginStreaming
*
* @api LONG | AVIStreamBeginStreaming | Specifies the parameters for
*      streaming and lets a stream handler prepare for streaming.
*
* @parm PAVISTREAM | pavi | Specifies a pointer to a stream.
*
* @parm LONG | lStart | Specifies the starting point for streaming.
*
* @parm LONG | lEnd | Specifies the ending point for streaming.
*
* @parm LONG | lRate | Specifies the speed at which the file will be
*	read relative to its natural speed.  Specify 1000 for the normal speed.
*
* @comm Many stream implementations ignore this function.
*
* @rdesc Returns zero if successful; otherwise returns an error code.
*
* @xref <f AVIStreamEndStreaming>
*
*************************************************************************/
STDAPI AVIStreamBeginStreaming(PAVISTREAM   pavi,
			       LONG	    lStart,
			       LONG	    lEnd,
			       LONG	    lRate)
{
    IAVIStreaming FAR * pi;
    HRESULT hr;

    if (FAILED(GetScode(pavi->QueryInterface(IID_IAVIStreaming,
					     (void FAR* FAR*) &pi))))
	return AVIERR_OK;

    hr = pi->Begin(lStart, lEnd, lRate);

    pi->Release();

    return hr;
}


/**************************************************************************
* @doc EXTERNAL AVIStreamEndStreaming
*
* @api LONG | AVIStreamEndStreaming | Ends streaming.
*
* @parm PAVISTREAM | pavi | Specifies a pointer to a stream.
*
* @comm Many stream implementations ignore this function.
*
* @rdesc Returns zero if successful; otherwise it returns an error code.
*
* @xref AVIStreamBeginStreaming
*
*************************************************************************/
STDAPI AVIStreamEndStreaming(PAVISTREAM   pavi)
{
    IAVIStreaming FAR * pi;
    HRESULT hr;

    if (FAILED(GetScode(pavi->QueryInterface(IID_IAVIStreaming, (LPVOID FAR *) &pi))))
	return AVIERR_OK;

    hr = pi->End();

    pi->Release();

    return hr;
}

#if 0
/*******************************************************************
* @doc INTERNAL AVIStreamHasChanged
*
* @api LONG | AVIStreamHasChanged | This function forces an update
* of the strem information for the specified stream.
*
* @parm PAVISTREAM | pavi | Interface pointer for an AVI stream instance.
*
* @rdesc Returns AVIERR_OK on success.
*
****************************************************************/
STDAPI AVIStreamHasChanged(PAVISTREAM pavi)
{
    pavi->lFrame = -4224;   // bogus value

    AVIStreamInfo(pavi, &pavi->avistream, sizeof(pavi->avistream));

    // !!! Only need to do this if format changes?
    AVIReleaseCachedData(pavi);

    return AVIERR_OK;
}
#endif

static SZCODE aszRegCompressors[] = "AVIFile\\Compressors\\%.4ls";

/*******************************************************************
* @doc EXTERNAL AVIMakeCompressedStream
*
* @api HRESULT | AVIMakeCompressedStream | Returns a pointer to a
*      compressed stream created from an uncompressed stream.
*      The uncompressed stream is compressed using
*      the compression options specified.
*
* @parm PAVISTREAM FAR * | ppsCompressed | Specifies a pointer to
*       the location used to return the compressed stream pointer.
*
* @parm PAVISTREAM | psSource | Specifies a pointer to the stream to be compressed.
*
* @parm AVICOMPRESSOPTIONS FAR * | lpOptions | Specifies a pointer to a
*       structure indicating the type compression to use and the options
*       to apply.
*
* @parm CLSID FAR * | pclsidHandler | Specifies a pointer to a
*       class ID used to create the stream.
*
* @comm This supports both audio and video compression. Applications
*       can use the created stream for reading or writing.
*
*   For video compression, either specify a handler to use or specify
*   the format for the compressed data.
*
*   For audio compression, you can only specify a format for the compressed
*   data.
*
* @rdesc Returns AVIERR_OK on success, or an error code.
*	Possible errors include:
*
*   @flag AVIERR_NOCOMPRESSOR | No suitable compressor can be found.
*
*   @flag AVIERR_MEMORY | There was not enough memory to complete the operation.
*
*   @flag AVIERR_UNSUPPORTED | Compression is not supported for this type
*	of data.  This error may be returned if you try to compress
*	data that is not audio or video.
*
*
*
****************************************************************/
STDAPI AVIMakeCompressedStream(
		PAVISTREAM FAR *	    ppsCompressed,
		PAVISTREAM		    psSource,
		AVICOMPRESSOPTIONS FAR *    lpOptions,
		CLSID FAR *pclsidHandler)
{
    CLSID   clsid;
    char    achKey[100];
    char    achClass[100];
    LONG    lcbClass;
    AVISTREAMINFO strhdr;
    HRESULT hr;


    *ppsCompressed = NULL;

    if (pclsidHandler) {
	clsid = *pclsidHandler;
    } else {
	if (FAILED(GetScode(hr = AVIStreamInfo(psSource,
					       &strhdr,
					       sizeof(strhdr)))))
	    return hr;

	// Look up the stream type in the registration database to find
	// the appropriate compressor....
	wsprintf(achKey, aszRegCompressors, (LPSTR) &strhdr.fccType);

	lcbClass = sizeof(achClass);
	RegQueryValue(HKEY_CLASSES_ROOT, achKey, achClass, &lcbClass);

	if (FAILED(GetScode(CLSIDFromString(achClass, &clsid))))
	    return ResultFromScode(AVIERR_UNSUPPORTED);
    }

    if (FAILED(GetScode(hr = CoCreateInstance((REFCLSID) clsid,
					 NULL, CLSCTX_INPROC,
					 (REFIID) IID_IAVIStream,
					 (void FAR* FAR*)ppsCompressed))))
	return hr;  // !!! PropagateHResult?

    if (FAILED(GetScode(hr = (*ppsCompressed)->Create((LONG) psSource,
						  (LONG) lpOptions)))) {
	(*ppsCompressed)->Release();
	*ppsCompressed = NULL;
	return hr;
    }

    return AVIERR_OK;
}


typedef struct {
    char	achClsid[64];
    char	achExtString[128];
} TEMPFILTER, FAR * LPTEMPFILTER;

SZCODE aszAnotherExtension[] = ";*.%s";

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api LONG | atol | local version of atol
 *
 ***************************************************************************/

static LONG NEAR PASCAL atol(char FAR *sz)
{
    LONG l = 0;

    while (*sz)
    	l = l*10 + *sz++ - '0';
    	
    return l;    	
}	



/*******************************************************************
* @doc EXTERNAL AVIBuildFilter
*
* @api HRESULT | AVIBuildFilter | Builds a filter specification for passing
*   to <f GetOpenFileName> or <f GetSaveFileName>.
*
* @parm LPSTR | lpszFilter | Pointer to buffer where the filter string
*   should be returned.
*
* @parm LONG | cbFilter | Size of buffer pointed to by <p lpszFilter>.
*
* @parm BOOL | fSaving | Indicates whether the filter should include only
*   formats that can be written, or all formats that can be read.
*
* @rdesc Returns AVIERR_OK on success.
*
* @comm This function does not check if the DLLs referenced
*       in the registration database actually exist.
*
****************************************************************/
STDAPI AVIBuildFilter(LPSTR lpszFilter, LONG cbFilter, BOOL fSaving)
{
#define MAXFILTERS  256
    LPTEMPFILTER    lpf;
    int		    i;
    int		    cf = 0;
    HKEY    hkey;
    LONG    lRet;
    DWORD   dwSubKey;
    char    ach[128];
    char    ach2[128];
    char    achExt[10];
    LONG    cb;
    char    achAllFiles[40];
    int	    cbAllFiles;

    // This string has a NULL in it, so remember its length for real....
    cbAllFiles = LoadString(ghMod,
			    IDS_ALLFILES,
			    achAllFiles,
			    sizeof(achAllFiles));
    for (i = 0; i < cbAllFiles; i++)
	if (achAllFiles[i] == '@')
	    achAllFiles[i] = '\0';


    lpf = (LPTEMPFILTER) GlobalAllocPtr(GHND, sizeof(TEMPFILTER) * MAXFILTERS);

    if (!lpf) {
	return ResultFromScode(AVIERR_MEMORY);
    }

    lRet = RegOpenKey(HKEY_CLASSES_ROOT, aszRegExt, &hkey);

    if (lRet != ERROR_SUCCESS) {

    }

    // Make sure that AVI files come first in the list....
    // !!! Should use StringFromClsid here!
    _fstrcpy(lpf[1].achClsid, "{00020000-0000-0000-C000-000000000046}");
    cf = 1;

    //
    // First, scan through the Extensions list looking for all of the
    // handlers that are installed
    //
    for (dwSubKey = 0; ; dwSubKey++) {
	lRet = RegEnumKey(hkey, dwSubKey, achExt, sizeof(achExt));

	if (lRet != ERROR_SUCCESS) {
	    break;
	}

	cb = sizeof(ach);
	lRet = RegQueryValue(hkey, achExt, ach, &cb);
	
	if (lRet != ERROR_SUCCESS) {
	    break;
	}

	//
	// See if we've seen this handler before
	//
	for (i = 1; i <= cf; i++) {
	    if (_fstrcmp(ach, lpf[i].achClsid) == 0) {
		break;

	    }
	}

	//
	// If not, add it to our list of handlers
	//
	if (i == cf + 1) {
	    if (cf == MAXFILTERS) {
		DPF("Too many filters!\n");
		continue;
	    }
	
	    _fstrcpy(lpf[i].achClsid, ach);
	
	    cb = sizeof(ach);
	    wsprintf(ach2, "%s\\AVIFile", (LPSTR) ach);
	    lRet = RegQueryValue(hkey, ach2, ach, &cb);
	    if (lRet == 0) {
		lRet = atol(ach);

		if (fSaving) {
		    if (!(lRet & AVIFILEHANDLER_CANWRITE))
			continue;
		} else {
		    if (!(lRet & AVIFILEHANDLER_CANREAD))
			continue;
		}
	    }

	    cf++;
	}
	
	wsprintf(ach, aszAnotherExtension, (LPSTR) achExt);
	
	_fstrcat(lpf[i].achExtString, lpf[i].achExtString[0] ?
						ach : ach + 1);
	
	_fstrcat(lpf[0].achExtString, lpf[0].achExtString[0] ?
						ach : ach + 1);
    }

    RegCloseKey(hkey);

    lRet = RegOpenKey(HKEY_CLASSES_ROOT, aszRegClsid, &hkey);

    if (lRet != ERROR_SUCCESS) {
	
    }

    //
    // Now, scan through our list of handlers and build up the
    // filter to use....
    //
    for (i = 0; i <= cf; i++) {
	if (i == 0) {
	    cb = wsprintf(lpszFilter, "All multimedia files") + 1;  // !!!
	} else {
	    cb = sizeof(ach);
	    lRet = RegQueryValue(hkey, lpf[i].achClsid, ach, &cb);

	    if (cbFilter < (LONG)(_fstrlen(lpf[i].achExtString) +
			    (LONG)_fstrlen(ach) + 10)) {
		break; // !!!
	    }

	    cb = wsprintf(lpszFilter, "%s", // "%s (%s)", Todd doesn't like this
			  (LPSTR) ach, (LPSTR) lpf[i].achExtString) + 1;
	}

	cbFilter -= cb;
	lpszFilter += cb;

	_fstrncpy(lpszFilter, lpf[i].achExtString, (int) cbFilter);
	cbFilter -= _fstrlen(lpf[i].achExtString) + 1;
	lpszFilter += _fstrlen(lpf[i].achExtString) + 1;

	if (cbFilter <= 0) {
	    GlobalFreePtr(lpf);
	    return ResultFromScode(AVIERR_BUFFERTOOSMALL);
	}
    }

    if (cbFilter > cbAllFiles) {
	_fmemcpy(lpszFilter, achAllFiles, cbAllFiles);
	cbFilter -= cbAllFiles;
	lpszFilter += cbAllFiles;
    }

    RegCloseKey(hkey);
	
    *lpszFilter++ = '\0';
    --cbFilter;

    GlobalFreePtr(lpf);

    return AVIERR_OK;
}

/*****************************************************************************
 *
 * dprintf() is called by the DPF macro if DEBUG is defined at compile time.
 *
 * The messages will be send to COM1: like any debug message. To
 * enable debug output, add the following to WIN.INI :
 *
 * [debug]
 * ICSAMPLE=1
 *
 ****************************************************************************/

#ifdef DEBUG

//
// I wish languages would make up their mind about defines!!!!!
//
#ifndef WINDLL
#define WINDLL
#endif

#ifndef _WINDLL
#define _WINDLL
#endif

#ifndef __WINDLL
#define __WINDLL
#endif

#include <stdarg.h>

#define MODNAME "AVIFILE"
static int iDebug = -1;

void cdecl dprintf(LPSTR szFormat, ...)
{
    char ach[128];

    if (iDebug == -1)
        iDebug = GetProfileIntA("Debug", MODNAME, 0);

    if (iDebug < 1)
        return;

    va_list va;
    va_start(va, szFormat);

    if (szFormat[0] == '!')
        ach[0]=0, szFormat++;
    else
        lstrcpyA(ach, MODNAME ": ");

    wvsprintfA(ach+lstrlenA(ach),szFormat,(LPSTR)va);
    va_end(va);
//  lstrcatA(ach, "\r\r\n");
    OutputDebugStringA(ach);
}

void cdecl dprintf2(LPSTR szFormat, ...)
{
    char ach[128];

    if (iDebug == -1)
        iDebug = GetProfileIntA("Debug", MODNAME, 0);

    if (iDebug < 2)
        return;

    va_list va;
    va_start(va, szFormat);
    if (szFormat[0] == '!')
        ach[0]=0, szFormat++;
    else
        lstrcpyA(ach, MODNAME ": ");

    wvsprintfA(ach+lstrlenA(ach),szFormat,(LPSTR)va);
    va_end(va);
//  lstrcatA(ach, "\r\r\n");
    OutputDebugStringA(ach);
}

void cdecl dprintf3(LPSTR szFormat, ...)
{
    char ach[128];

    if (iDebug == -1)
        iDebug = GetProfileIntA("Debug", MODNAME, 0);

    if (iDebug < 3)
        return;

    va_list va;
    va_start(va, szFormat);
    if (szFormat[0] == '!')
        ach[0]=0, szFormat++;
    else
        lstrcpyA(ach, MODNAME ": ");

    wvsprintfA(ach+lstrlenA(ach),szFormat,(LPSTR)va);
    va_end(va);

//  lstrcatA(ach, "\r\r\n");
    OutputDebugStringA(ach);
}

#endif

#ifdef DEBUG

/* _Assert(szExpr, szFile, iLine)
 *
 * If <fExpr> is TRUE, then do nothing.  If <fExpr> is FALSE, then display
 * an "assertion failed" message box allowing the user to abort the program,
 * enter the debugger (the "Retry" button), or igore the error.
 *
 * <szFile> is the name of the source file; <iLine> is the line number
 * containing the _Assert() call.
 */
void FAR PASCAL
_Assert(char *szExp, char *szFile, int iLine)
{
	static char	ach[300];	// debug output (avoid stack overflow)
	int		id;
	void FAR PASCAL DebugBreak(void);

        /* display error message */

        if (szExp)
            wsprintfA(ach, "(%s)\nFile %s, line %d", (LPSTR)szExp, (LPSTR)szFile, iLine);
        else
            wsprintfA(ach, "File %s, line %d", (LPSTR)szFile, iLine);

	MessageBeep(MB_ICONHAND);
	id = MessageBoxA(NULL, ach, "Assertion Failed",
#ifdef BIDI
		MB_RTL_READING |
#endif
		MB_SYSTEMMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE);

	/* abort, debug, or ignore */
	switch (id)
	{
	case IDABORT:
                FatalAppExit(0, "Good Bye");
		break;

	case IDRETRY:
		/* break into the debugger */
		DebugBreak();
		break;

	case IDIGNORE:
		/* ignore the assertion failure */
		break;
	}
}
#endif
