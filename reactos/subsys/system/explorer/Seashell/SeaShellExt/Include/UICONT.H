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

#ifndef __OBJLIST_H__
#define __OBJLIST_H__

#include "UIData.h"

class CTRL_EXT_CLASS CUIObjList : public CObList 
{
  DECLARE_SERIAL(CUIObjList)
public:
  CUIObjList(); 
  ~CUIObjList();
  void DeleteAll();
  CUIListCtrlData* RemoveHead();
  CUIListCtrlData* GetNext(POSITION& rPos);
  void Append(CUIListCtrlData* pListObj);
  BOOL Remove(CUIListCtrlData* pListObj);                       
};

inline CUIListCtrlData* CUIObjList::RemoveHead()
{
	return (CUIListCtrlData*) CObList::RemoveHead();
}

inline CUIListCtrlData* CUIObjList::GetNext(POSITION& rPos)
{
	return (CUIListCtrlData*) CObList::GetNext(rPos);
}

#endif // __OBJLIST_H__
