//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       syncbuf.cxx
//
//  Contents:   Tree Syncronization Opcode/Operand Buffer stuff
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_SYNCBUF_HXX
#define X_SYNCBUF_HXX
#include "syncbuf.hxx"
#endif


MtDefine(CTreeSyncLogger, Mem, "CTreeSyncLogger")
MtDefine(CTreeSyncLogger_aryLogSinks_pv, CTreeSyncLogger, "CTreeSyncLogger::_aryLogSinks::_pv")


//
// CTreeSyncLogger methods
//

CTreeSyncLogger::CTreeSyncLogger()
{
    _bufOut.Init(&_bufpool); // reinit as write (output) buffer
}

CTreeSyncLogger::~CTreeSyncLogger()
{
    Assert(_aryLogSinks.Size() == 0); // bug(tomlaw): forgot to write code to free everything
}
