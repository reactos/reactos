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

#ifndef __REFRESH_H__
#define __REFRESH_H__

#include "UIFolderRefresh.h"

// SHELL ITEM DATA STRUCTURES

// List Control
typedef struct tagLVID
{
   LPSHELLFOLDER lpsfParent;
   LPITEMIDLIST  lpi;
   LPITEMIDLIST  lpifq;
   ULONG         ulAttribs;
   LPARAM		 lParam;		
} LVITEMDATA, *LPLVITEMDATA;

// Tree Control
typedef struct tagTVID
{
   LPSHELLFOLDER lpsfParent;
   LPITEMIDLIST  lpi;
   LPITEMIDLIST  lpifq;
   LPARAM		 lParam;		
} TVITEMDATA, *LPTVITEMDATA;

#include <vector>
using namespace std;
typedef vector<LPTVITEMDATA> vecItemData;

class CRefreshIEFolder : public CRefresh
{
private:
	CRefreshIEFolder(const CRefreshIEFolder &rRefresh);
	CRefreshIEFolder &operator=(const CRefreshIEFolder &rRefresh);
public:
	CRefreshIEFolder(HTREEITEM hItem,LPARAM lParam) : CRefresh(hItem,lParam) {}
	LPTVITEMDATA GetItemData() const { return (LPTVITEMDATA)GetExtData(); }
};

class CRefreshShellFolder : public CRefresh
{
private:
	CRefreshShellFolder(const CRefreshShellFolder &rRefresh);
	CRefreshShellFolder &operator=(const CRefreshShellFolder &rRefresh);
public:
	CRefreshShellFolder(HTREEITEM hItem,LPARAM lParam) : CRefresh(hItem,lParam) {}
	LPTVITEMDATA GetItemData() const { return (LPTVITEMDATA)GetExtData(); }
};

#endif //__REFRESH_H__
