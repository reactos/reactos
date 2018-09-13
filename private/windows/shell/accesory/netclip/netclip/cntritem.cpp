// cntritem.cpp : implementation of the CNetClipCntrItem class
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.
//
// BUGBUG: This class is probably not needed.  Just use
// CRichEditCntrItem direclty instead.

#include "stdafx.h"
#include "netclip.h"

#include "doc.h"
#include "View.h"
#include "cntritem.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNetClipCntrItem implementation

IMPLEMENT_SERIAL(CNetClipCntrItem, CRichEditCntrItem, 0)

CNetClipCntrItem::CNetClipCntrItem(REOBJECT *preo, CNetClipDoc* pContainer)
	: CRichEditCntrItem(preo, pContainer)
{
}

/////////////////////////////////////////////////////////////////////////////
// CNetClipCntrItem diagnostics

#ifdef _DEBUG
void CNetClipCntrItem::AssertValid() const
{
	CRichEditCntrItem::AssertValid();
}

void CNetClipCntrItem::Dump(CDumpContext& dc) const
{
	CRichEditCntrItem::Dump(dc);
}
#endif

/////////////////////////////////////////////////////////////////////////////
