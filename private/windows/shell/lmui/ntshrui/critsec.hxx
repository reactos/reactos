//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       critsec.hxx
//
//  Contents:   CTakeCriticalSection class
//
//  History:    11-Apr-95 BruceFo Created
//
//--------------------------------------------------------------------------

#ifndef __CRITSEC_HXX__
#define __CRITSEC_HXX__

class CTakeCriticalSection
{
public:

    CTakeCriticalSection(
        IN PCRITICAL_SECTION pcs
        )
        :
        m_pcs(pcs)
    {
        EnterCriticalSection(m_pcs);
    }

    ~CTakeCriticalSection()
    {
        LeaveCriticalSection(m_pcs);
    }

private:

    //
    // Class variables
    //

    PCRITICAL_SECTION m_pcs;

};

#endif  // __CRITSEC_HXX__
