/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
/******************************************************************************
Module name: Optex.h
Written by:  Jeffrey Richter
Purpose:     Defines the COptex (optimized mutex) synchronization object
******************************************************************************/

#ifndef OPTEX_HXX
#define OPTEX_HXX

#ifdef PRFDATA

///////////////////////////////////////////////////////////////////////////////


class COptex {
public:
   COptex(LPCSTR pszName,  DWORD dwSpinCount = 4000);
   COptex(LPCWSTR pszName, DWORD dwSpinCount = 4000);
   ~COptex();
   void SetSpinCount(DWORD dwSpinCount);
   void Enter();
   BOOL TryEnter();
   void Leave();

private:
   typedef struct {
      DWORD m_dwSpinCount;
      long  m_lLockCount;
      DWORD m_dwThreadId;
      long  m_lRecurseCount;
   } SHAREDINFO, *PSHAREDINFO;

   BOOL        m_fUniprocessorHost;
   HANDLE      m_hevt;
   HANDLE      m_hfm;
   PSHAREDINFO m_pSharedInfo;

private:
   BOOL CommonConstructor(PVOID pszName, BOOL fUnicode, DWORD dwSpinCount);
};


#endif

///////////////////////////////// End of File /////////////////////////////////

#endif
