/*
 *  @doc
 *
 *  @module UNIXMODELESS.HXX -- CUnixModeless
 *
 *      This class handles non-marshalling modeless dialog messages of Unix
 *
 *  Authors: <nl>
 *      Steve Shih <nl> 
 *
 *  Copyright (c) 1995-1996, Microsoft Corporation. All rights reserved.
 */

#ifdef NO_MARSHALLING

class CUnixModeless
{
private:
    CHTMLDlg** m_pHead;
    int   m_iUsed;
    int   m_iCurrent;
    int   m_iSize;

public:
    CUnixModeless();
    ~CUnixModeless();

public:
    CHTMLDlg* GetFirstDlg();
    CHTMLDlg* GetNextDlg();

    BOOL Append(CHTMLDlg*);
    BOOL Remove(CHTMLDlg*);
};

extern CUnixModeless g_Modeless;

#endif
