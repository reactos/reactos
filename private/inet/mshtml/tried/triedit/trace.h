/****************************************************************************
*
* Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved
*
*
*
*
*
*
*
****************************************************************************/

#ifndef __TRACE_H__
#define __TRACE_H__

#ifdef _DEBUG
    void __cdecl Trace(LPSTR lprgchFormat, ...);
#else
    inline void __cdecl Trace(LPSTR /*lprgchFormat*/, ...) {}
#endif //_DEBUG

#endif //__TRACE_H__

