//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       osver.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCUI_OSVER_H
#define _INC_CSCUI_OSVER_H

class OsVersion
{
    public:
        enum { UNKNOWN, WIN95, WIN98, NT4, NT5 };

        OsVersion(void)
            : m_os(UNKNOWN) { }

        int Get(void) const;

        bool IsNT(void) const
            { return NT4 == Get() || NT5 == Get(); }

        bool IsWindows(void) const
            { return WIN95 == Get() || WIN98 == Get(); }

    private:
        mutable int  m_os;
};

#endif // _INC_CSCUI_OSVER_H
