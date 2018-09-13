/* copied from ..\htmed\lexer.cpp */
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved

/*++

  Copyright (c) 1995 Microsoft Corporation

  File: lexer.h

  Abstract:
        Nitty gritty lexer stuff

  Contents:

  History:
      2/14/97   cgomes:   Created


--*/
#if !defined __INC_LEXER_H__
#define __INC_LEXER_H__

#include "token.h"

extern CTableSet*   g_ptabASP;
extern PSUBLANG     g_psublangASP;
extern PTABLESET    g_arpTables[CV_MAX+1];

typedef enum tag_COMMENTTYPE
{
    CT_NORMAL       = 0,
    CT_METADATA     = -1,
    CT_IECOMMENT    = 1
} COMMENTTYPE;

HINT GetHint         (LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token);
HINT GetTextHint     (LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token);
UINT GetToken        (LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token);
UINT GetTokenLength  (LPCTSTR pchLine, UINT cbLen, UINT cbCur);
UINT FindEndEntity   (LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token);
COMMENTTYPE IfHackComment   (LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token);
UINT FindEntityRef   (LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token);
UINT FindEndComment  (LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token);
UINT FindServerScript(LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token);
UINT FindValue       (LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token);
UINT FindEndString   (LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token);
UINT FindNextToken   (LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token);
UINT FindTagOpen     (LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token);
UINT FindEndTag      (LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token);
UINT FindText        (LPCTSTR pchLine, UINT cbLen, UINT cbCur, TXTB & token);
BOOL IsUnknownID     (LPCTSTR pchLine, UINT cbLen, UINT cbCur, TXTB & token);
BOOL IsNumber        (LPCTSTR pchLine, UINT cbLen, UINT cbCur, TXTB & token);
BOOL IsElementName   (LPCTSTR pchLine, UINT cbCur, int cbTokLen, TXTB & token);
BOOL IsAttributeName (LPCTSTR pchLine, UINT cbCur, int cbTokLen, TXTB & token);
BOOL IsIdentifier(int iTokenLength, TXTB & token);
int  IndexFromElementName(LPCTSTR pszName);


CTableSet * MakeTableSet(CTableSet ** rgpts, RWATT_T att, UINT nIdName);
void SetLanguage(TCHAR * /*const CString & */strDefault, PSUBLANG rgSublang,
                 PTABLESET pTab, UINT & index, UINT nIdTemplate, CLSID clsid);
CTableSet * FindTable(CTableSet ** rgpts, /*const CString & */TCHAR *strName);
CTableSet * FindTable(CTableSet ** rgpts, CTableSet * pts);


#endif /* __INC_LEXER_H__ */
