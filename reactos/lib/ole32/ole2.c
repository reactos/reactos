/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib\ole32\Ole2.c
 * PURPOSE:         Object Linking And Embedding implementation
 * PROGRAMMER:      jurgen van gael [jurgen.vangael@student.kuleuven.ac.be]
 * UPDATE HISTORY:
 *                  Created 12/05/2001
 */
/********************************************************************


This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.


********************************************************************/
#include "Ole32.h"

#if 0
WINOLEAPI_(DWORD) OleBuildVersion( VOID ){return S_OK;}
#endif

WINOLEAPI 
OleInitialize (IN LPVOID pvReserved)
{
  return E_FAIL;
}

WINOLEAPI_(void) 
OleUninitialize(void)
{
  return;
}

#if 0

WINOLEAPI  OleQueryLinkFromData(IN LPDATAOBJECT pSrcDataObject){return S_OK;}
WINOLEAPI  OleQueryCreateFromData(IN LPDATAOBJECT pSrcDataObject){return S_OK;}


WINOLEAPI  OleCreate(IN REFCLSID rclsid, IN REFIID riid, IN DWORD renderopt,
                IN LPFORMATETC pFormatEtc, IN LPOLECLIENTSITE pClientSite,
                IN LPSTORAGE pStg, OUT LPVOID FAR* ppvObj){return S_OK;}

WINOLEAPI  OleCreateEx(IN REFCLSID rclsid, IN REFIID riid, IN DWORD dwFlags,
                IN DWORD renderopt, IN ULONG cFormats, IN DWORD* rgAdvf,
                IN LPFORMATETC rgFormatEtc, IN IAdviseSink FAR* lpAdviseSink,
                OUT DWORD FAR* rgdwConnection, IN LPOLECLIENTSITE pClientSite,
                IN LPSTORAGE pStg, OUT LPVOID FAR* ppvObj){return S_OK;}

WINOLEAPI  OleCreateFromData(IN LPDATAOBJECT pSrcDataObj, IN REFIID riid,
                IN DWORD renderopt, IN LPFORMATETC pFormatEtc,
                IN LPOLECLIENTSITE pClientSite, IN LPSTORAGE pStg,
                OUT LPVOID FAR* ppvObj){return S_OK;}

WINOLEAPI  OleCreateFromDataEx(IN LPDATAOBJECT pSrcDataObj, IN REFIID riid,
                IN DWORD dwFlags, IN DWORD renderopt, IN ULONG cFormats, IN DWORD* rgAdvf,
                IN LPFORMATETC rgFormatEtc, IN IAdviseSink FAR* lpAdviseSink,
                OUT DWORD FAR* rgdwConnection, IN LPOLECLIENTSITE pClientSite,
                IN LPSTORAGE pStg, OUT LPVOID FAR* ppvObj){return S_OK;}

WINOLEAPI  OleCreateLinkFromData(IN LPDATAOBJECT pSrcDataObj, IN REFIID riid,
                IN DWORD renderopt, IN LPFORMATETC pFormatEtc,
                IN LPOLECLIENTSITE pClientSite, IN LPSTORAGE pStg,
                OUT LPVOID FAR* ppvObj){return S_OK;}

WINOLEAPI  OleCreateLinkFromDataEx(IN LPDATAOBJECT pSrcDataObj, IN REFIID riid,
                IN DWORD dwFlags, IN DWORD renderopt, IN ULONG cFormats, IN DWORD* rgAdvf,
                IN LPFORMATETC rgFormatEtc, IN IAdviseSink FAR* lpAdviseSink,
                OUT IN DWORD FAR* rgdwConnection, IN LPOLECLIENTSITE pClientSite,
                IN LPSTORAGE pStg, OUT LPVOID FAR* ppvObj){return S_OK;}

WINOLEAPI  OleCreateStaticFromData(IN LPDATAOBJECT pSrcDataObj, IN REFIID iid,
                IN DWORD renderopt, IN LPFORMATETC pFormatEtc,
                IN LPOLECLIENTSITE pClientSite, IN LPSTORAGE pStg,
                OUT LPVOID FAR* ppvObj){return S_OK;}


WINOLEAPI  OleCreateLink(IN LPMONIKER pmkLinkSrc, IN REFIID riid,
            IN DWORD renderopt, IN LPFORMATETC lpFormatEtc,
            IN LPOLECLIENTSITE pClientSite, IN LPSTORAGE pStg, OUT LPVOID FAR* ppvObj){return S_OK;}

WINOLEAPI  OleCreateLinkEx(IN LPMONIKER pmkLinkSrc, IN REFIID riid,
            IN DWORD dwFlags, IN DWORD renderopt, IN ULONG cFormats, IN DWORD* rgAdvf,
            IN LPFORMATETC rgFormatEtc, IN IAdviseSink FAR* lpAdviseSink,
            OUT DWORD FAR* rgdwConnection, IN LPOLECLIENTSITE pClientSite,
            IN LPSTORAGE pStg, OUT LPVOID FAR* ppvObj){return S_OK;}

WINOLEAPI  OleCreateLinkToFile(IN LPCOLESTR lpszFileName, IN REFIID riid,
            IN DWORD renderopt, IN LPFORMATETC lpFormatEtc,
            IN LPOLECLIENTSITE pClientSite, IN LPSTORAGE pStg, OUT LPVOID FAR* ppvObj){return S_OK;}

WINOLEAPI  OleCreateLinkToFileEx(IN LPCOLESTR lpszFileName, IN REFIID riid,
            IN DWORD dwFlags, IN DWORD renderopt, IN ULONG cFormats, IN DWORD* rgAdvf,
            IN LPFORMATETC rgFormatEtc, IN IAdviseSink FAR* lpAdviseSink,
            OUT DWORD FAR* rgdwConnection, IN LPOLECLIENTSITE pClientSite,
            IN LPSTORAGE pStg, OUT LPVOID FAR* ppvObj){return S_OK;}

WINOLEAPI  OleCreateFromFile(IN REFCLSID rclsid, IN LPCOLESTR lpszFileName, IN REFIID riid,
            IN DWORD renderopt, IN LPFORMATETC lpFormatEtc,
            IN LPOLECLIENTSITE pClientSite, IN LPSTORAGE pStg, OUT LPVOID FAR* ppvObj){return S_OK;}

WINOLEAPI  OleCreateFromFileEx(IN REFCLSID rclsid, IN LPCOLESTR lpszFileName, IN REFIID riid,
            IN DWORD dwFlags, IN DWORD renderopt, IN ULONG cFormats, IN DWORD* rgAdvf,
            IN LPFORMATETC rgFormatEtc, IN IAdviseSink FAR* lpAdviseSink,
            OUT DWORD FAR* rgdwConnection, IN LPOLECLIENTSITE pClientSite,
            IN LPSTORAGE pStg, OUT LPVOID FAR* ppvObj){return S_OK;}

WINOLEAPI  OleLoad(IN LPSTORAGE pStg, IN REFIID riid, IN LPOLECLIENTSITE pClientSite,
            OUT LPVOID FAR* ppvObj){return S_OK;}

WINOLEAPI  OleSave(IN LPPERSISTSTORAGE pPS, IN LPSTORAGE pStg, IN BOOL fSameAsLoad){return S_OK;}

WINOLEAPI  OleLoadFromStream( IN LPSTREAM pStm, IN REFIID iidInterface, OUT LPVOID FAR* ppvObj){return S_OK;}
WINOLEAPI  OleSaveToStream( IN LPPERSISTSTREAM pPStm, IN LPSTREAM pStm ){return S_OK;}


WINOLEAPI  OleSetContainedObject(IN LPUNKNOWN pUnknown, IN BOOL fContained){return S_OK;}
WINOLEAPI  OleNoteObjectVisible(IN LPUNKNOWN pUnknown, IN BOOL fVisible){return S_OK;}



WINOLEAPI  RegisterDragDrop(IN HWND hwnd, IN LPDROPTARGET pDropTarget){return S_OK;}
WINOLEAPI  RevokeDragDrop(IN HWND hwnd){return S_OK;}
WINOLEAPI  DoDragDrop(IN LPDATAOBJECT pDataObj, IN LPDROPSOURCE pDropSource,
            IN DWORD dwOKEffects, OUT LPDWORD pdwEffect){return S_OK;}


WINOLEAPI  OleSetClipboard(IN LPDATAOBJECT pDataObj){return S_OK;}
WINOLEAPI  OleGetClipboard(OUT LPDATAOBJECT FAR* ppDataObj){return S_OK;}
WINOLEAPI  OleFlushClipboard(void){return S_OK;}
WINOLEAPI  OleIsCurrentClipboard(IN LPDATAOBJECT pDataObj){return S_OK;}


WINOLEAPI_(HOLEMENU)   OleCreateMenuDescriptor (IN HMENU hmenuCombined,
                                IN LPOLEMENUGROUPWIDTHS lpMenuWidths){return NULL;}
WINOLEAPI              OleSetMenuDescriptor (IN HOLEMENU holemenu, IN HWND hwndFrame,
                                IN HWND hwndActiveObject,
                                IN LPOLEINPLACEFRAME lpFrame,
                                IN LPOLEINPLACEACTIVEOBJECT lpActiveObj){return S_OK;}
WINOLEAPI              OleDestroyMenuDescriptor (IN HOLEMENU holemenu){return S_OK;}

WINOLEAPI              OleTranslateAccelerator (IN LPOLEINPLACEFRAME lpFrame,
                            IN LPOLEINPLACEFRAMEINFO lpFrameInfo, IN LPMSG lpmsg){return S_OK;}


WINOLEAPI_(HANDLE) OleDuplicateData (IN HANDLE hSrc, IN CLIPFORMAT cfFormat,
                        IN UINT uiFlags){return NULL;}

WINOLEAPI          OleDraw (IN LPUNKNOWN pUnknown, IN DWORD dwAspect, IN HDC hdcDraw,
                    IN LPCRECT lprcBounds){return S_OK;}

WINOLEAPI          OleRun(IN LPUNKNOWN pUnknown){return S_OK;}
WINOLEAPI_(BOOL)   OleIsRunning(IN LPOLEOBJECT pObject){return S_OK;}
WINOLEAPI          OleLockRunning(IN LPUNKNOWN pUnknown, IN BOOL fLock, IN BOOL fLastUnlockCloses){return S_OK;}
WINOLEAPI_(void)   ReleaseStgMedium(IN LPSTGMEDIUM){return;}
WINOLEAPI          CreateOleAdviseHolder(OUT LPOLEADVISEHOLDER FAR* ppOAHolder){return S_OK;}

WINOLEAPI          OleCreateDefaultHandler(IN REFCLSID clsid, IN LPUNKNOWN pUnkOuter,
                    IN REFIID riid, OUT LPVOID FAR* lplpObj){return S_OK;}

WINOLEAPI          OleCreateEmbeddingHelper(IN REFCLSID clsid, IN LPUNKNOWN pUnkOuter,
                    IN DWORD flags, IN LPCLASSFACTORY pCF,
                    IN REFIID riid, OUT LPVOID FAR* lplpObj){return S_OK;}

WINOLEAPI_(BOOL)   IsAccelerator(IN HACCEL hAccel, IN int cAccelEntries, IN LPMSG lpMsg,
                                        OUT WORD FAR* lpwCmd){return S_OK;}

WINOLEAPI_(HGLOBAL) OleGetIconOfFile(IN LPOLESTR lpszPath, IN BOOL fUseFileAsLabel){return NULL;}

WINOLEAPI_(HGLOBAL) OleGetIconOfClass(IN REFCLSID rclsid,     IN LPOLESTR lpszLabel,
                                        IN BOOL fUseTypeAsLabel){return NULL;}

WINOLEAPI_(HGLOBAL) OleMetafilePictFromIconAndLabel(IN HICON hIcon, IN LPOLESTR lpszLabel,
                                        IN LPOLESTR lpszSourceFile, IN UINT iIconIndex){return NULL;}



WINOLEAPI                  OleRegGetUserType (IN REFCLSID clsid, IN DWORD dwFormOfType,
                                        OUT LPOLESTR FAR* pszUserType){return S_OK;}

WINOLEAPI                  OleRegGetMiscStatus     (IN REFCLSID clsid, IN DWORD dwAspect,
                                        OUT DWORD FAR* pdwStatus){return S_OK;}

WINOLEAPI                  OleRegEnumFormatEtc     (IN REFCLSID clsid, IN DWORD dwDirection,
                                        OUT LPENUMFORMATETC FAR* ppenum){return S_OK;}

WINOLEAPI                  OleRegEnumVerbs (IN REFCLSID clsid, OUT LPENUMOLEVERB FAR* ppenum){return S_OK;}


//	OLE 1.0
WINOLEAPI OleConvertOLESTREAMToIStorage
    (IN LPOLESTREAM                lpolestream,
    OUT LPSTORAGE                   pstg,
    IN const DVTARGETDEVICE FAR*   ptd){return S_OK;}

WINOLEAPI OleConvertIStorageToOLESTREAM
    (IN LPSTORAGE      pstg,
    OUT LPOLESTREAM     lpolestream){return S_OK;}


WINOLEAPI GetHGlobalFromILockBytes (IN LPLOCKBYTES plkbyt, OUT HGLOBAL FAR* phglobal){return S_OK;}
WINOLEAPI CreateILockBytesOnHGlobal (IN HGLOBAL hGlobal, IN BOOL fDeleteOnRelease,
                                    OUT LPLOCKBYTES FAR* pplkbyt){return S_OK;}

WINOLEAPI GetHGlobalFromStream (IN LPSTREAM pstm, OUT HGLOBAL FAR* phglobal){return S_OK;}
WINOLEAPI CreateStreamOnHGlobal (IN HGLOBAL hGlobal, IN BOOL fDeleteOnRelease,
                                OUT LPSTREAM FAR* ppstm){return S_OK;}



WINOLEAPI OleDoAutoConvert(IN LPSTORAGE pStg, OUT LPCLSID pClsidNew){return S_OK;}
WINOLEAPI OleGetAutoConvert(IN REFCLSID clsidOld, OUT LPCLSID pClsidNew){return S_OK;}
WINOLEAPI OleSetAutoConvert(IN REFCLSID clsidOld, IN REFCLSID clsidNew){return S_OK;}
WINOLEAPI GetConvertStg(IN LPSTORAGE pStg){return S_OK;}
WINOLEAPI SetConvertStg(IN LPSTORAGE pStg, IN BOOL fConvert){return S_OK;}


WINOLEAPI OleConvertIStorageToOLESTREAMEx
    (IN LPSTORAGE          pstg,
                                    // Presentation data to OLESTREAM
     IN CLIPFORMAT         cfFormat,   //      format
     IN LONG               lWidth,     //      width
     IN LONG               lHeight,    //      height
     IN DWORD              dwSize,     //      size in bytes
     IN LPSTGMEDIUM        pmedium,    //      bits
     OUT LPOLESTREAM        polestm){return S_OK;}

WINOLEAPI OleConvertOLESTREAMToIStorageEx
    (IN LPOLESTREAM        polestm,
     OUT LPSTORAGE          pstg,
                                    // Presentation data from OLESTREAM
     OUT CLIPFORMAT FAR*    pcfFormat,  //      format
     OUT LONG FAR*          plwWidth,   //      width
     OUT LONG FAR*          plHeight,   //      height
     OUT DWORD FAR*         pdwSize,    //      size in bytes
     OUT LPSTGMEDIUM        pmedium){return S_OK;}   //      bits

#endif


