//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       osver.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "osver.h"


int
OsVersion::Get(
    void
    ) const
{
    if (UNKNOWN == m_os)
    {
        OSVERSIONINFO osvi;
        osvi.dwOSVersionInfoSize = sizeof(osvi);

        if (0 != GetVersionEx(&osvi))
        {
            //
            // This code uses the same logic as that used by the shell
            // to determine OS version.
            //
            switch(osvi.dwPlatformId)
            {
                case VER_PLATFORM_WIN32_WINDOWS:
                    if (osvi.dwMajorVersion > 4 || (osvi.dwMajorVersion == 4 &&
                                                    osvi.dwMinorVersion >= 10))
                    {
                        m_os = WIN98;
                    }
                    else if (osvi.dwMajorVersion >= 4)
                    {
                        m_os = WIN95;
                    }
                    break;
            
                case VER_PLATFORM_WIN32_NT:
                    if (osvi.dwMajorVersion >= 5)
                    {
                        m_os = NT5;
                    }
                    else if (osvi.dwMajorVersion >= 4)
                    {
                        m_os = NT4;
                    }
                    break;

                default:
                    break;
            }
        }
    }
    return m_os;
}

