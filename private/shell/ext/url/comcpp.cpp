/*
 * comcpp.cpp - Common C++ functions implementation.
 */


/* Headers
 **********/

#include "project.hpp"
#pragma hdrstop


/****************************** Public Functions *****************************/


PUBLIC_CODE HRESULT MyReleaseStgMedium(PSTGMEDIUM pstgmed)
{
   HRESULT hr;

   ASSERT(IS_VALID_STRUCT_PTR(pstgmed, CSTGMEDIUM));

   if (pstgmed->pUnkForRelease)
   {
        hr = pstgmed->pUnkForRelease->Release();
        switch(pstgmed->tymed)
        {
        case TYMED_FILE:
            SHFree(pstgmed->lpszFileName);
            hr = S_OK;
            break;

        case TYMED_ISTREAM:
            hr = pstgmed->pstm->Release();
            break;

        case TYMED_ISTORAGE:
            hr = pstgmed->pstm->Release();
            break;
        }
   }
   else
   {
      switch(pstgmed->tymed)
      {
         case TYMED_HGLOBAL:
            hr = (! GlobalFree(pstgmed->hGlobal)) ? S_OK : E_HANDLE;
            break;

         case TYMED_ISTREAM:
            hr = pstgmed->pstm->Release();
            break;

         case TYMED_ISTORAGE:
            hr = pstgmed->pstm->Release();
            break;

         case TYMED_FILE:
            SHFree(pstgmed->lpszFileName);
            hr = S_OK;
            break;

         case TYMED_GDI:
            hr = (DeleteObject(pstgmed->hBitmap)) ? S_OK : E_HANDLE;
            break;

         case TYMED_MFPICT:
            hr = (DeleteMetaFile((HMETAFILE)(pstgmed->hMetaFilePict)) &&
                  ! GlobalFree(pstgmed->hMetaFilePict)) ? S_OK : E_HANDLE;
            break;

         case TYMED_ENHMF:
            hr = (DeleteEnhMetaFile(pstgmed->hEnhMetaFile)) ? S_OK : E_HANDLE;
            break;

         default:
            ASSERT(pstgmed->tymed == TYMED_NULL);
            hr = S_OK;
            break;
      }
   }

   return(hr);
}

