/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib\ole32\Storage.c
 * PURPOSE:         Storage helper functions
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

/*WINOLEAPI StgCreatePropStg( IUnknown* pUnk, REFFMTID fmtid, const CLSID *pclsid, DWORD grfFlags, DWORD dwReserved, IPropertyStorage **ppPropStg ){return S_OK;}
WINOLEAPI StgOpenPropStg( IUnknown* pUnk, REFFMTID fmtid, DWORD grfFlags, DWORD dwReserved, IPropertyStorage **ppPropStg ){return S_OK;}
WINOLEAPI StgCreatePropSetStg( IStorage *pStorage, DWORD dwReserved, IPropertySetStorage **ppPropSetStg){return S_OK;}

WINOLEAPI FmtIdToPropStgName( const FMTID *pfmtid, LPOLESTR oszName ){return S_OK;}
WINOLEAPI PropStgNameToFmtId( const LPOLESTR oszName, FMTID *pfmtid ){return S_OK;}

WINOLEAPI StgSetTimes(
  WCHAR const *lpszName,
  FILETIME const *pctime,   
  FILETIME const *patime,   
  FILETIME const *pmtime    
){return S_OK;}

// helper functions
WINOLEAPI ReadClassStg(IN LPSTORAGE pStg, OUT CLSID FAR* pclsid){return S_OK;}
WINOLEAPI WriteClassStg(IN LPSTORAGE pStg, IN REFCLSID rclsid){return S_OK;}
WINOLEAPI ReadClassStm(IN LPSTREAM pStm, OUT CLSID FAR* pclsid){return S_OK;}
WINOLEAPI WriteClassStm(IN LPSTREAM pStm, IN REFCLSID rclsid){return S_OK;}
WINOLEAPI WriteFmtUserTypeStg (IN LPSTORAGE pstg, IN CLIPFORMAT cf, IN LPOLESTR lpszUserType){return S_OK;}
WINOLEAPI ReadFmtUserTypeStg (IN LPSTORAGE pstg, OUT CLIPFORMAT FAR* pcf, OUT LPOLESTR FAR* lplpszUserType){return S_OK;}



WINOLEAPI StgCreateDocfile(IN const OLECHAR FAR* pwcsName,
            IN DWORD grfMode,
            IN DWORD reserved,
            OUT IStorage FAR * FAR *ppstgOpen)
{
	return S_OK;
}

WINOLEAPI StgCreateDocfileOnILockBytes(IN ILockBytes FAR *plkbyt,
                    IN DWORD grfMode,
                    IN DWORD reserved,
                    OUT IStorage FAR * FAR *ppstgOpen)
{
	return S_OK;
}


WINOLEAPI StgOpenStorage(IN const OLECHAR FAR* pwcsName,
              IN  IStorage FAR *pstgPriority,
              IN  DWORD grfMode,
              IN  SNB snbExclude,
              IN  DWORD reserved,
              OUT IStorage FAR * FAR *ppstgOpen)
{
	return S_OK;
}

WINOLEAPI StgOpenStorageOnILockBytes(IN ILockBytes FAR *plkbyt,
                  IN  IStorage FAR *pstgPriority,
                  IN  DWORD grfMode,
                  IN  SNB snbExclude,
                  IN  DWORD reserved,
                  OUT IStorage FAR * FAR *ppstgOpen)
{
	return S_OK;
}


WINOLEAPI StgIsStorageFile(IN const OLECHAR FAR* pwcsName)
{
	return S_OK;
}

WINOLEAPI StgIsStorageILockBytes(IN ILockBytes FAR* plkbyt)
{
	return S_OK;
}


WINOLEAPI StgSetTimes(IN OLECHAR const FAR* lpszName,
                   IN FILETIME const FAR* pctime,
                   IN FILETIME const FAR* patime,
                   IN FILETIME const FAR* pmtime);

WINOLEAPI StgOpenAsyncDocfileOnIFillLockBytes( IN IFillLockBytes *pflb,
             IN  DWORD grfMode,
             IN  DWORD asyncFlags,
             OUT IStorage **ppstgOpen)
{
	return S_OK;
}


WINOLEAPI StgGetIFillLockBytesOnILockBytes( IN ILockBytes *pilb,
             OUT IFillLockBytes **ppflb)
{
	return S_OK;
}


WINOLEAPI StgGetIFillLockBytesOnFile(IN OLECHAR const *pwcsName,
             OUT IFillLockBytes **ppflb)
{
	return S_OK;
}



WINOLEAPI StgOpenLayoutDocfile(IN OLECHAR const *pwcsDfName,
             IN  DWORD grfMode,
             IN  DWORD reserved,
             OUT IStorage **ppstgOpen)
{
	return S_OK;
}


WINOLEAPI StgCreateStorageEx (IN const WCHAR* pwcsName,
            IN  DWORD grfMode,
            IN  DWORD stgfmt,              // enum
            IN  DWORD grfAttrs,             // reserved
            IN  STGOPTIONS * pStgOptions,
            IN  void * reserved,
            IN  REFIID riid,
            OUT void ** ppObjectOpen)
{
	return S_OK;
}


WINOLEAPI StgOpenStorageEx (IN const WCHAR* pwcsName,
            IN  DWORD grfMode,
            IN  DWORD stgfmt,              // enum
            IN  DWORD grfAttrs,             // reserved
            IN  STGOPTIONS * pStgOptions,
            IN  void * reserved,
            IN  REFIID riid,
            OUT void ** ppObjectOpen)
{
	return S_OK;
}*/
