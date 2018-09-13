
////////////////////////////////////////////////////////////////////////////
//
// stuff needed for MCIAVI to run-time-link to AVIFile for playback.
// because most everything from AVIFILE.DLL is a OLE interface, we dont need
// to RTL to many functions.
//
////////////////////////////////////////////////////////////////////////////

#define USEAVIFILE      //!!! hey lets use AVIFile.

#ifdef USEAVIFILE

//#define _INC_AVIFMT     100     /* version number * 100 + revision */
#include <avifile.h>

UINT    uAVIFILE;
HMODULE hdllAVIFILE;
HMODULE hdllCOMPOBJ;

//
//  RTL to AVIFile...
//
void    (STDAPICALLTYPE *XAVIFileInit)(void);
void    (STDAPICALLTYPE *XAVIFileExit)(void);
HRESULT (STDAPICALLTYPE *XAVIFileOpen)(PAVIFILE FAR * ppfile,LPCTSTR szFile,UINT uMode,LPCLSID lpHandler);
HRESULT (STDAPICALLTYPE *XAVIMakeFileFromStreams)(PAVIFILE FAR *,int,PAVISTREAM FAR *);
HRESULT (STDAPICALLTYPE *XAVIStreamBeginStreaming)(PAVISTREAM   pavi,
			       LONG	    lStart,
			       LONG	    lEnd,
			       LONG	    lRate);
HRESULT (STDAPICALLTYPE *XAVIStreamEndStreaming)(PAVISTREAM   pavi);


#undef  AVIFileInit
#undef  AVIFileExit
#undef AVIFileOpen
#undef AVIFileInfo

#define AVIFileInit         XAVIFileInit
#define AVIFileExit         XAVIFileExit
#define AVIFileOpen         XAVIFileOpen
#define AVIMakeFileFromStreams  XAVIMakeFileFromStreams
#define AVIStreamBeginStreaming  XAVIStreamBeginStreaming
#define AVIStreamEndStreaming  XAVIStreamEndStreaming

#undef  AVIFileClose
#define AVIFileClose(p)                 (p)->lpVtbl->Release(p)
#define AVIFileInfo(p,a,b)              (p)->lpVtbl->Info(p, a, b)
#define AVIFileGetStream(p,a,b,c)       (p)->lpVtbl->GetStream(p,a,b,c)

#undef  AVIStreamClose
#define AVIStreamClose(p)               (p)->lpVtbl->Release(p)
#define AVIStreamInfo(p,a,b)            (p)->lpVtbl->Info(p, a, b)
#define AVIStreamReadFormat(p,a,b,c)    (p)->lpVtbl->ReadFormat(p, a, b, c)
#define AVIStreamReadData(p,a,b,c)      (p)->lpVtbl->ReadData(p, a, b, c)
#define AVIStreamFindSample(p,a,b)      (p)->lpVtbl->FindSample(p, a, b)

#define AVIStreamRead(p,a,b,c,d,e,f)    (p)->lpVtbl->Read(p,a,b,c,d,e,f)

// RTL to COMPOBJ
EXTERN_C BOOL STDAPICALLTYPE IsValidInterface(LPVOID pv);
BOOL    (STDAPICALLTYPE *XIsValidInterface)(LPVOID pv);

#define IsValidInterface XIsValidInterface

#endif  // USEAVIFILE

