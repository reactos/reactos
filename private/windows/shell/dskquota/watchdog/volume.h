#ifndef __VOLUME_H
#define __VOLUME_H
///////////////////////////////////////////////////////////////////////////////
/*  File: volume.h

    Description: The CVolume class maintains information about a particular
        disk volume.  During initialization, the object determines if the
        associated volume supports quotas.  The object's client can then query
        it for volume-specific quota information.  Objects of this class are used
        in the CStatistics class to store information about a user/volume
        pair.

            CVolume


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////

#ifndef _WINDOWS_
#   include <windows.h>
#endif

class CVolume
{
    public:
        CVolume(TCHAR chLetter, LPCTSTR pszDisplayName);
        ~CVolume(VOID);

        BOOL operator == (const CVolume& rhs) const
            { return m_dwSerialNo == rhs.m_dwSerialNo; }

        TCHAR GetLetter(VOID) const
            { return m_chLetter; }

        VOID GetDisplayName(CString& strOut) const
            { strOut = m_strDisplayName; }

        LPCTSTR GetDisplayName(VOID) const
            { return (LPCTSTR)m_strDisplayName; }

        BOOL SupportsQuotas(VOID) const
            { return m_bSupportsQuotas; }

    private:
        TCHAR   m_chLetter;          // Driver letter
        DWORD   m_dwSerialNo;        // Drive serial number (unique ID)
        CString m_strDisplayName;    // Display name from shell.
        BOOL    m_bSupportsQuotas;   // Does volume support quotas?

        //
        // Prevent copy.
        //
        CVolume(const CVolume& rhs);
        CVolume& operator = (const CVolume& rhs);
};


#endif // __VOLUME_H
