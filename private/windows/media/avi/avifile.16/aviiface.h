/****************************************************************************
 *
 *  AVIIFACE.H
 *
 *  Interface definitions for AVIFile
 *
 *  Copyright (c) 1992, 1993 Microsoft Corporation.  All Rights Reserved.
 *
 *  You have a royalty-free right to use, modify, reproduce and
 *  distribute the Sample Files (and/or any modified version) in
 *  any way you find useful, provided that you agree that
 *  Microsoft has no warranty obligations or liability for any
 *  Sample Application Files which are modified.
 *
 ***************************************************************************/

#include <compobj.h>


#ifndef RIID
#if defined(__cplusplus)
#define	RIID	IID FAR&
#define	RCLSID	CLSID FAR&
#else
#define	RIID	IID FAR*
#define	RCLSID	CLSID FAR*
#endif
#endif


/*	-	-	-	-	-	-	-	-	*/


/****** AVI Stream Interface *******************************************/

#undef  INTERFACE
#define INTERFACE   IAVIStream

DECLARE_INTERFACE_(IAVIStream, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IAVIStream methods ***
    STDMETHOD(Create)      (THIS_ LPARAM lParam1, LPARAM lParam2) PURE ;
    STDMETHOD(Info)        (THIS_ AVISTREAMINFO FAR * psi, LONG lSize) PURE ;
    STDMETHOD_(LONG, FindSample)(THIS_ LONG lPos, LONG lFlags) PURE ;
    STDMETHOD(ReadFormat)  (THIS_ LONG lPos,
			    LPVOID lpFormat, LONG FAR *lpcbFormat) PURE ;
    STDMETHOD(SetFormat)   (THIS_ LONG lPos,
			    LPVOID lpFormat, LONG cbFormat) PURE ;
    STDMETHOD(Read)        (THIS_ LONG lStart, LONG lSamples,
			    LPVOID lpBuffer, LONG cbBuffer,
			    LONG FAR * plBytes, LONG FAR * plSamples) PURE ;
    STDMETHOD(Write)       (THIS_ LONG lStart, LONG lSamples,
			    LPVOID lpBuffer, LONG cbBuffer,
			    DWORD dwFlags,
			    LONG FAR *plSampWritten,
			    LONG FAR *plBytesWritten) PURE ;
    STDMETHOD(Delete)      (THIS_ LONG lStart, LONG lSamples) PURE;
    STDMETHOD(ReadData)    (THIS_ DWORD fcc, LPVOID lp, LONG FAR *lpcb) PURE ;
    STDMETHOD(WriteData)   (THIS_ DWORD fcc, LPVOID lp, LONG cb) PURE ;
    STDMETHOD(Reserved1)            (THIS) PURE;
    STDMETHOD(Reserved2)            (THIS) PURE;
    STDMETHOD(Reserved3)            (THIS) PURE;
    STDMETHOD(Reserved4)            (THIS) PURE;
    STDMETHOD(Reserved5)            (THIS) PURE;
};

typedef       IAVIStream FAR* PAVISTREAM;


#undef  INTERFACE
#define INTERFACE   IAVIStreaming

DECLARE_INTERFACE_(IAVIStreaming, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IAVIStreaming methods ***
    STDMETHOD(Begin) (THIS_
		      LONG  lStart,		    // start of what we expect
						    // to play
		      LONG  lEnd,		    // expected end, or -1
		      LONG  lRate) PURE;	    // Should this be a float?
    STDMETHOD(End)   (THIS) PURE;
};

typedef       IAVIStreaming FAR* PAVISTREAMING;


#undef  INTERFACE
#define INTERFACE   IAVIEditStream

DECLARE_INTERFACE_(IAVIEditStream, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IAVIEditStream methods ***
    STDMETHOD(Cut) (THIS_ LONG FAR *plStart,
			  LONG FAR *plLength,
			  PAVISTREAM FAR * ppResult) PURE;
    STDMETHOD(Copy) (THIS_ LONG FAR *plStart,
			   LONG FAR *plLength,
			   PAVISTREAM FAR * ppResult) PURE;
    STDMETHOD(Paste) (THIS_ LONG FAR *plPos,
			    LONG FAR *plLength,
			    PAVISTREAM pstream,
			    LONG lStart,
			    LONG lEnd) PURE;
    STDMETHOD(Clone) (THIS_ PAVISTREAM FAR *ppResult) PURE;
    STDMETHOD(SetInfo) (THIS_ AVISTREAMINFO FAR * lpInfo,
			    LONG cbInfo) PURE;
};

typedef       IAVIEditStream FAR* PAVIEDITSTREAM;


/****** AVI File Interface *******************************************/


#undef  INTERFACE
#define INTERFACE   IAVIFile
#define PAVIFILE IAVIFile FAR*

DECLARE_INTERFACE_(IAVIFile, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IAVIFile methods ***
    STDMETHOD(Open)		    (THIS_
                                     LPCTSTR szFile,
                                     UINT mode) PURE;
    STDMETHOD(Info)                 (THIS_
                                     AVIFILEINFO FAR * pfi,
                                     LONG lSize) PURE;
    STDMETHOD(GetStream)            (THIS_
                                     PAVISTREAM FAR * ppStream,
				     DWORD fccType,
                                     LONG lParam) PURE;
    STDMETHOD(CreateStream)         (THIS_
                                     PAVISTREAM FAR * ppStream,
                                     AVISTREAMINFO FAR * psi) PURE;
    STDMETHOD(Save)                 (THIS_
                                     LPCTSTR szFile,
                                     AVICOMPRESSOPTIONS FAR *lpOptions,
                                     AVISAVECALLBACK lpfnCallback) PURE;
    STDMETHOD(WriteData)            (THIS_
                                     DWORD ckid,
                                     LPVOID lpData,
                                     LONG cbData) PURE;
    STDMETHOD(ReadData)             (THIS_
                                     DWORD ckid,
                                     LPVOID lpData,
                                     LONG FAR *lpcbData) PURE;
    STDMETHOD(EndRecord)            (THIS) PURE;
    STDMETHOD(Reserved1)            (THIS) PURE;
    STDMETHOD(Reserved2)            (THIS) PURE;
    STDMETHOD(Reserved3)            (THIS) PURE;
    STDMETHOD(Reserved4)            (THIS) PURE;
    STDMETHOD(Reserved5)            (THIS) PURE;
};

#undef PAVIFILE
typedef       IAVIFile FAR* PAVIFILE;

/****** GetFrame Interface *******************************************/

#undef  INTERFACE
#define INTERFACE   IGetFrame
#define PGETFRAME   IGetFrame FAR*

DECLARE_INTERFACE_(IGetFrame, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IGetFrame methods ***

    STDMETHOD_(LPVOID,GetFrame) (THIS_ LONG lPos) PURE;
//  STDMETHOD_(LPVOID,GetFrameData) (THIS_ LONG lPos) PURE;

    STDMETHOD(Begin) (THIS_ LONG lStart, LONG lEnd, LONG lRate) PURE;
    STDMETHOD(End) (THIS) PURE;

    STDMETHOD(SetFormat) (THIS_ LPBITMAPINFOHEADER lpbi, LPVOID lpBits, int x, int y, int dx, int dy) PURE;

//  STDMETHOD(DrawFrameStart) (THIS) PURE;
//  STDMETHOD(DrawFrame) (THIS_ LONG lPos, HDC hdc, int x, int y, int dx, int dy) PURE;
//  STDMETHOD(DrawFrameEnd) (THIS) PURE;
};

#undef PGETFRAME
typedef IGetFrame FAR* PGETFRAME;

/****** GUIDs *******************************************/

#define DEFINE_AVIGUID(name, l, w1, w2) \
    DEFINE_GUID(name, l, w1, w2, 0xC0,0,0,0,0,0,0,0x46)

DEFINE_AVIGUID(IID_IAVIFile,            0x00020020, 0, 0);
DEFINE_AVIGUID(IID_IAVIStream,          0x00020021, 0, 0);
DEFINE_AVIGUID(IID_IAVIStreaming,       0x00020022, 0, 0);
DEFINE_AVIGUID(IID_IGetFrame,           0x00020023, 0, 0);
DEFINE_AVIGUID(IID_IAVIEditStream,      0x00020024, 0, 0);
DEFINE_AVIGUID(CLSID_AVISimpleUnMarshal,        0x00020009, 0, 0);

#define	AVIFILEHANDLER_CANREAD		0x0001
#define	AVIFILEHANDLER_CANWRITE		0x0002
#define	AVIFILEHANDLER_CANACCEPTNONRGB	0x0004
