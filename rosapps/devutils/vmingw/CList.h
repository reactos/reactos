/********************************************************************
*	Module:	CList.h. This is part of WinUI.
*
*	License:	WinUI is covered by GNU General Public License, 
*			Copyright (C) 2001  Manu B.
*			See license.htm for more details.
*
********************************************************************/
#ifndef CLIST_H
#define CLIST_H

#include <stdlib.h>

/********************************************************************
*	Generic CNode.
********************************************************************/
class CNode
{
	public:
	CNode() {next = prev = NULL; /*type = 0;*/};
	virtual ~CNode() {};

	CNode * 	prev;
	CNode * 	next;
	long		type;

	protected:

	private:
};

/********************************************************************
*	Generic List.
********************************************************************/
#define FILE_FOUND		0
#define EMPTY_LIST		1
#define INSERT_FIRST		2
#define INSERT_LAST		3
#define INSERT_BEFORE	4
#define INSERT_AFTER		5

class CList
{
	public:
	CList() {first = last = current = NULL; count = 0;};
	virtual ~CList();

	CNode *	GetCurrent() {return current;};
	CNode *	First();
	CNode *	Last();
	CNode *	Prev();
	CNode *	Next();

	void	InsertFirst(CNode *node);
	void	InsertLast(CNode *node);
	void	InsertBefore(CNode *node);
	void	InsertAfter(CNode *node);
	bool	InsertSorted(CNode * newNode);
	int InsertSorted_New(CNode * newNode);

	void	DestroyCurrent();
	void	Destroy(CNode * node);
	void	DestroyList();

	int	Length();

	protected:
	virtual int Compare(CNode *, CNode *) {return 0;};

	CNode *first;
	CNode *last;
	CNode *current;
	int count;

	private:
};

#endif

