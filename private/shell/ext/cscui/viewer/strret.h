//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       strret.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCVIEW_STRRET_H
#define _INC_CSCVIEW_STRRET_H
///////////////////////////////////////////////////////////////////////////////
/*  File: strret.h

    Description: Class to handle STRRET objects used in returning strings
        from the Windows Shell.  Main function is to properly clean up any
        dynamic storage.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////

class StrRet : public STRRET
{
    public:
        StrRet(LPITEMIDLIST pidl, UINT type = STRRET_CSTR);
        StrRet(const StrRet& rhs);
        ~StrRet(VOID);

        StrRet& operator = (const StrRet& rhs);

        LPTSTR GetString(LPTSTR pszStr, INT cchStr) const;

        operator STRRET*()
            { return static_cast<STRRET *>(this); }

    private:
        IMalloc     *m_pMalloc;
        LPITEMIDLIST m_pidl;

        VOID FreeOleStr(VOID);
        VOID CopyOleStr(LPCWSTR pszOleStr);
};

#endif //_INC_CSCVIEW_STRRET_H
