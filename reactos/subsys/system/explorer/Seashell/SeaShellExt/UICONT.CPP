//*******************************************************************************
// COPYRIGHT NOTES
// ---------------
// You may use this source code, compile or redistribute it as part of your application 
// for free. You cannot redistribute it as a part of a software development 
// library without the agreement of the author. If the sources are 
// distributed along with the application, you should leave the original 
// copyright notes in the source code without any changes.
// This code can be used WITHOUT ANY WARRANTIES at your own risk.
// 
// For the latest updates to this code, check this site:
// http://www.masmex.com 
// after Sept 2000
// 
// Copyright(C) 2000 Philip Oldaker <email: philip@masmex.com>
//*******************************************************************************

#include "stdafx.h"
#include "UIData.h"
#include "UICont.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CListObList

IMPLEMENT_SERIAL(CUIObjList, CObList, 0)

CUIObjList::CUIObjList()
{
}

CUIObjList::~CUIObjList()
{
	    DeleteAll();
}


/////////////////////////////////////////////////////////////////////////////
// CListObList commands

// Delete al objects in the list
void CUIObjList::DeleteAll()
{
	  while(!IsEmpty()) 
	  {
			CUIListCtrlData* ptr = RemoveHead();
			if (ptr->GetAutoDelete())
			{
				CObject *p =(CObject*)ptr->GetExtData();
				ASSERT_KINDOF(CObject,p);
				delete p;
			}
			delete ptr;
	  }
}

// Add a new object to the end of the list
void CUIObjList::Append(CUIListCtrlData* pListObj)
{
	  ASSERT(pListObj);
	  ASSERT(pListObj->IsKindOf(RUNTIME_CLASS(CUIListCtrlData)));
	  POSITION pos = CObList::AddTail(pListObj);
}

// Remove an object from the list but don't delete it
BOOL CUIObjList::Remove(CUIListCtrlData* pListObj)
{
	  POSITION pos = Find(pListObj);
	  if (!pos) 
		return FALSE;
	  RemoveAt(pos);
	  return TRUE;
}
