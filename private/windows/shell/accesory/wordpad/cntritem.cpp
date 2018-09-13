// cntritem.cpp : implementation of the CWordPadCntrItem class
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

#include "stdafx.h"
#include "wordpad.h"

#include "wordpdoc.h"
#include "wordpvw.h"
#include "cntritem.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWordPadCntrItem implementation

IMPLEMENT_SERIAL(CWordPadCntrItem, CRichEdit2CntrItem, 0)

CWordPadCntrItem::CWordPadCntrItem(REOBJECT *preo, CWordPadDoc* pContainer)
	: CRichEdit2CntrItem(preo, pContainer)
{
}

/////////////////////////////////////////////////////////////////////////////
// CWordPadCntrItem diagnostics

#ifdef _DEBUG
void CWordPadCntrItem::AssertValid() const
{
	CRichEdit2CntrItem::AssertValid();
}

void CWordPadCntrItem::Dump(CDumpContext& dc) const
{
	CRichEdit2CntrItem::Dump(dc);
}
#endif

/////////////////////////////////////////////////////////////////////////////
