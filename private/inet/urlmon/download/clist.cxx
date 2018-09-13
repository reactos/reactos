//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1995                    **
//*********************************************************************

//
//  CLIST.CPP
//

//  HISTORY:
//  
//  9/10/95     philco      Created.
//  4/05/96     VatsanP     Copied from MSHTML to URLMON\DOWNLOAD
//                          to use for the code downloader
//                          changed name to CLIST.CXX to make JoahnnP happy :)
//

//
//  Templated list class "borrowed" from MFC 3.0.  Used to manage a
//  list of IOleObject pointers to embedded items.
//

#include <cdlpch.h>
#ifndef unix
#include "..\inc\clist.hxx"
#else
#include "../inc/clist.hxx"
#endif /* unix */

BOOL AFXAPI AfxIsValidAddress(const void* lp, UINT nBytes, BOOL bReadWrite)
{
    // simple version using Win-32 APIs for pointer validation.
    return (lp != NULL && !IsBadReadPtr(lp, nBytes) &&
        (!bReadWrite || !IsBadWritePtr((LPVOID)lp, nBytes)));
}

CPlex* PASCAL CPlex::Create(CPlex*& pHead, UINT nMax, UINT cbElement)
{
    ASSERT(nMax > 0 && cbElement > 0);


    CPlex* p = (CPlex*) new (BYTE[sizeof(CPlex) + nMax* cbElement]);
            // may throw exception
    p->nMax = nMax;
    p->nCur = 0;
    p->pNext = pHead;
    pHead = p;  // change head (adds in reverse order for simplicity)
    return p;
}

void CPlex::FreeDataChain()     // free this one and links
{
    CPlex* p = this;
    while (p != NULL)
    {
        BYTE* bytes = (BYTE*) p;
        CPlex* pNext = p->pNext;
        delete (bytes);
        p = pNext;
    }
}
