/*
 *  @doc
 *
 *  @module QUXCOPY.HXX -- CQuickUnixCopy
 *
 *      This class implements the middle button copy/copy for unix
 *
 *  Authors: <nl>
 *      David Dawson <nl>
 *
 *  Copyright (c) 1995-1996, Microsoft Corporation. All rights reserved.
 */

#ifndef _QUXCOPY_H
#define _QUXCOPY_H

#ifdef UNIX

class CQuickCopyBuffer
{
public:

    CQuickCopyBuffer() { _hLastCopiedString = NULL; }
    ~CQuickCopyBuffer();

    HRESULT NewTextSelection( CDoc *pDoc );

    HRESULT GetTextSelection( HGLOBAL     hszText,
                              BOOL        bUnicode, 
                              VARIANTARG *pvarCopiedTextHandle );
    
    HRESULT ClearSelection();

private:

    HGLOBAL _hLastCopiedString;
};


extern CQuickCopyBuffer g_uxQuickCopyBuffer;

#endif // UNIX
#endif // _QUXCOPY_H
