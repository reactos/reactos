/*
 * refcount.cpp - RefCount class implementation.
 */


/* Headers
 **********/

#include "project.hpp"
#pragma hdrstop

#include "clsfact.h"

/****************************** Public Functions *****************************/


#ifdef DEBUG

PUBLIC_CODE BOOL IsValidPCRefCount(PCRefCount pcrefcnt)
{
   // m_ulcRef may be any value.

   return(IS_VALID_READ_PTR(pcrefcnt, CRefCount));
}

#endif


/********************************** Methods **********************************/


RefCount::RefCount(void)
{
   DebugEntry(RefCount::RefCount);

   /* Don't validate this until after initialization. */

   m_ulcRef = 1;
   DLLAddRef();

   ASSERT(IS_VALID_STRUCT_PTR(this, CRefCount));

   DebugExitVOID(RefCount::RefCount);

   return;
}


RefCount::~RefCount(void)
{
   DebugEntry(RefCount::~RefCount);

   ASSERT(IS_VALID_STRUCT_PTR(this, CRefCount));

   // m_ulcRef may be any value.

   DLLRelease();

   ASSERT(IS_VALID_STRUCT_PTR(this, CRefCount));

   DebugExitVOID(RefCount::~RefCount);

   return;
}


ULONG STDMETHODCALLTYPE RefCount::AddRef(void)
{
   DebugEntry(RefCount::AddRef);

   ASSERT(IS_VALID_STRUCT_PTR(this, CRefCount));

   ASSERT(m_ulcRef < ULONG_MAX);
   m_ulcRef++;

   ASSERT(IS_VALID_STRUCT_PTR(this, CRefCount));

   DebugExitULONG(RefCount::AddRef, m_ulcRef);

   return(m_ulcRef);
}


ULONG STDMETHODCALLTYPE RefCount::Release(void)
{
   ULONG ulcRef;

   DebugEntry(RefCount::Release);

   ASSERT(IS_VALID_STRUCT_PTR(this, CRefCount));

   if (EVAL(m_ulcRef > 0))
      m_ulcRef--;

   ulcRef = m_ulcRef;

   if (! ulcRef)
      delete this;

   DebugExitULONG(RefCount::Release, ulcRef);

   return(ulcRef);
}

