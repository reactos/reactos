/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib\ole32\Moniker.c
 * PURPOSE:         Moniker implementation
 * PROGRAMMER:      jurgen van gael [jurgen.vangael@student.kuleuven.ac.be]
 * UPDATE HISTORY:
 *                  Created 10/05/2001
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
#include "ole32.h"

/*
//
//
//
WINOLEAPI BindMoniker(LPMONIKER pmk, DWORD grfOpt, REFIID iidResult, LPVOID* ppvResult)
{
	return S_OK;
}


WINOLEAPI  CoInstall(
    IN  IBindCtx     * pbc,
    IN  DWORD          dwFlags,
    IN  uCLSSPEC     * pClassSpec,
    IN  QUERYCONTEXT * pQuery,
    IN  LPWSTR         pszCodeBase)
{
	return S_OK;
}


WINOLEAPI  MkParseDisplayName(IN LPBC pbc, IN LPCOLESTR szUserName,
                OUT ULONG FAR * pchEaten, OUT LPMONIKER FAR * ppmk)
{
	return S_OK;
}

WINOLEAPI  MonikerRelativePathTo(IN LPMONIKER pmkSrc, IN LPMONIKER pmkDest, OUT LPMONIKER
                FAR* ppmkRelPath, IN BOOL dwReserved)
{
	return S_OK;
}

WINOLEAPI  MonikerCommonPrefixWith(IN LPMONIKER pmkThis, IN LPMONIKER pmkOther,
                OUT LPMONIKER FAR* ppmkCommon)
{
	return S_OK;
}

WINOLEAPI  CreateBindCtx(IN DWORD reserved, OUT LPBC FAR* ppbc)
{
	return S_OK;
}

WINOLEAPI  CreateGenericComposite(IN LPMONIKER pmkFirst, IN LPMONIKER pmkRest,
    OUT LPMONIKER FAR* ppmkComposite)
{
	return S_OK;
}

WINOLEAPI  GetClassFile (IN LPCOLESTR szFilename, OUT CLSID FAR* pclsid)
{
	//	wine stuff

	/*IStorage *pstg=0;
    HRESULT res;
    int nbElm=0,length=0,i=0;
    LONG sizeProgId=20;
    LPOLESTR *pathDec=0,absFile=0,progId=0;
    WCHAR extention[100]={0};

    TRACE("()\n");

    /* if the file contain a storage object the return the CLSID writen by IStorage_SetClass method
    if((StgIsStorageFile(filePathName))==S_OK){

        res=StgOpenStorage(filePathName,NULL,STGM_READ | STGM_SHARE_DENY_WRITE,NULL,0,&pstg);

        if (SUCCEEDED(res))
            res=ReadClassStg(pstg,pclsid);

        IStorage_Release(pstg);

        return res;
    }
    /* if the file is not a storage object then attemps to match various bits in the file against a
       pattern in the registry. this case is not frequently used ! so I present only the psodocode for
       this case
       
     for(i=0;i<nFileTypes;i++)

        for(i=0;j<nPatternsForType;j++){

            PATTERN pat;
            HANDLE  hFile;

            pat=ReadPatternFromRegistry(i,j);
            hFile=CreateFileW(filePathName,,,,,,hFile);
            SetFilePosition(hFile,pat.offset);
            ReadFile(hFile,buf,pat.size,NULL,NULL);
            if (memcmp(buf&pat.mask,pat.pattern.pat.size)==0){

                *pclsid=ReadCLSIDFromRegistry(i);
                return S_OK;
            }
        }
     */

    /* if the obove strategies fail then search for the extension key in the registry 

    /* get the last element (absolute file) in the path name 
    nbElm=FileMonikerImpl_DecomposePath(filePathName,&pathDec);
    absFile=pathDec[nbElm-1];

    /* failed if the path represente a directory and not an absolute file name
    if (lstrcmpW(absFile,(LPOLESTR)"\\"))
        return MK_E_INVALIDEXTENSION;

    /* get the extension of the file 
    length=lstrlenW(absFile);
    for(i=length-1; ( (i>=0) && (extention[i]=absFile[i]) );i--);
        
    /* get the progId associated to the extension 
    progId=CoTaskMemAlloc(sizeProgId);

    res=RegQueryValueW(HKEY_CLASSES_ROOT,extention,progId,&sizeProgId);

    if (res==ERROR_MORE_DATA){

        progId = CoTaskMemRealloc(progId,sizeProgId);
        res=RegQueryValueW(HKEY_CLASSES_ROOT,extention,progId,&sizeProgId);
    }
    if (res==ERROR_SUCCESS)
        /* return the clsid associated to the progId 
        res= CLSIDFromProgID(progId,pclsid);

    for(i=0; pathDec[i]!=NULL;i++)
        CoTaskMemFree(pathDec[i]);
    CoTaskMemFree(pathDec);

    CoTaskMemFree(progId);

    if (res==ERROR_SUCCESS)
        return res;

    return MK_E_INVALIDEXTENSION;
	return E_FAIL;
}


WINOLEAPI  CreateClassMoniker(IN REFCLSID rclsid, OUT LPMONIKER FAR* ppmk)
{
	return S_OK;
}


WINOLEAPI  CreateFileMoniker(IN LPCOLESTR lpszPathName, OUT LPMONIKER FAR* ppmk)
{
	return S_OK;
}


WINOLEAPI  CreateItemMoniker(IN LPCOLESTR lpszDelim, IN LPCOLESTR lpszItem,
    OUT LPMONIKER FAR* ppmk)
{
	return S_OK;
}

WINOLEAPI  CreateAntiMoniker(OUT LPMONIKER FAR* ppmk)
{
	return S_OK;
}

WINOLEAPI  CreatePointerMoniker(IN LPUNKNOWN punk, OUT LPMONIKER FAR* ppmk)
{
	return S_OK;
}

WINOLEAPI  CreateObjrefMoniker(IN LPUNKNOWN punk, OUT LPMONIKER FAR * ppmk)
{
	return S_OK;
}


WINOLEAPI  GetRunningObjectTable( IN DWORD reserved, OUT LPRUNNINGOBJECTTABLE FAR* pprot)
{
	return S_OK;
}*/
