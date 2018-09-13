#ifndef _INC_CSCVIEW_CNXCACHE_H
#define _INC_CSCVIEW_CNXCACHE_H

#ifndef _WINDOWS_
#   include <windows.h>
#endif

#ifndef _INC_CSCVIEW_STRCLASS_H
#   include "strclass.h"
#endif

//
// A cache of "\\server\share" - to - Drive Letter pairings.
// This information is maintained so we can quickly display strings
// like "ntspecs on 'worf' (E:)".  We only want to go to the net
// layer once so we cache the relationship information.
//
class CnxNameCache
{
    public:
        CnxNameCache(void) throw();
        ~CnxNameCache(void);

        void Refresh(void) throw();

        TCHAR GetShareDriveLetter(const CString& strShare) const;

        bool GetShareFromDriveLetter(TCHAR chDrive,  CString *pstrShare) const;

    private:
        CString *m_prgCnx;       
        static const int NUM_ENTRIES;

        bool Loaded(void) const
            { return NULL != m_prgCnx; }

        void Load(void);

        int Count(void) const throw()
            { return (NULL != m_prgCnx) ? ARRAYSIZE(m_prgCnx) : 0; }

        //
        // Prevent copy.
        //
        CnxNameCache(const CnxNameCache& rhs);
        CnxNameCache& operator = (const CnxNameCache& rhs);
};


#endif // _INC_CSCVIEW_CNXCACHE_H
