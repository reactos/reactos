//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cscutils.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCVIEW_CSCUTILS_H
#define _INC_CSCVIEW_CSCUTILS_H


//-----------------------------------------------------------------------------
// class CscFindHandle
//-----------------------------------------------------------------------------
//
// Trivial class to ensure cleanup of FindFirst/FindNext handle.
//
class CscFindHandle
{
    public:
        explicit CscFindHandle(HANDLE hFind) throw()
            : m_hFind(hFind) { }

        ~CscFindHandle(void) throw()
            { 
                if (IsValidHandle()) 
                    CSCFindClose(m_hFind);
            }

        operator HANDLE() const throw()
            { return m_hFind; }

        bool IsValidHandle(void) const throw()
            { return (INVALID_HANDLE_VALUE != m_hFind); }

        HANDLE Handle(void) const throw()
            { return m_hFind; }

    private:
        HANDLE m_hFind;
};



#endif //_INC_CSCVIEW_CSCUTILS_H
