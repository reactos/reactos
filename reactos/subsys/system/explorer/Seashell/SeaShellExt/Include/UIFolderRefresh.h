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

#ifndef __UI_FOLDER_REFRESH_H__
#define __UI_FOLDER_REFRESH_H__

class CTRL_EXT_CLASS CRefresh : public CObject
{
	DECLARE_DYNAMIC(CRefresh)
public:
	CRefresh(HTREEITEM hItem,LPARAM lParam) : m_hItem(hItem), m_lParam(lParam) {};
	~CRefresh() {};
	LPARAM GetExtData() const { return m_lParam; }
	HTREEITEM GetItem() const { return m_hItem; }
private:
	LPARAM m_lParam;
	HTREEITEM m_hItem;
};

class CTRL_EXT_CLASS CRefreshCategory : public CRefresh
{
	DECLARE_DYNAMIC(CRefreshCategory)
public:
	CRefreshCategory(HTREEITEM hItem,LPARAM lParam,long nCategory=0) 
		: m_nCategory(nCategory),CRefresh(hItem,lParam)
	{}
public:
	long GetCategory() const { return m_nCategory; }
private:
	long m_nCategory;
};

#endif //__UI_FOLDER_REFRESH_H__
