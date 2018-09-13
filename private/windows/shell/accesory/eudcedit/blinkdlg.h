/**************************************************/
/*                                                */
/*                                                */
/*      Chinese IME Batch Mode                    */
/*              (Dialogbox)                       */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#if 0 // move to imeblink.h!

#ifndef RC_INVOKED
#pragma pack(1)
#endif

//	data structure of IME table 
typedef struct tagUSRDICIMHDR {
    WORD  uHeaderSize;                  // 0x00
    BYTE  idUserCharInfoSign[8];        // 0x02
    BYTE  idMajor;                      // 0x0A
    BYTE  idMinor;                      // 0x0B
    DWORD ulTableCount;                 // 0x0C
    WORD  cMethodKeySize;               // 0x10
    BYTE  uchBankID;                    // 0x12
    WORD  idInternalBankID;             // 0x13
    BYTE  achCMEXReserved1[43];         // 0x15
    WORD  uInfoSize;                    // 0x40
    BYTE  chCmdKey;                     // 0x42
    BYTE  idStlnUpd;                    // 0x43
    BYTE  cbField;                      // 0x44
    WORD  idCP;                         // 0x45
    BYTE  achMethodName[6];             // 0x47
    BYTE  achCSIReserved2[51];          // 0x4D
    BYTE  achCopyRightMsg[128];         // 0x80
} USRDICIMHDR;

#ifndef RC_INVOKED
#pragma pack()
#endif

typedef USRDICIMHDR FAR *LPUSRDICIMHDR;

#endif 0 // move to imeblink.h!

class CBLinkDlg : public CDialog
{
public:
    CBLinkDlg(CWnd* pParent = NULL);

    //{{AFX_DATA(CBLinkDlg)
    enum { IDD = IDD_LINKBATCH };
    //}}AFX_DATA

    //{{AFX_VIRTUAL(CBLinkDlg)
    protected:
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

protected:
    BOOL    RegistStringTable();
#if 0 // move to imeblink.h!
    HKL     MatchImeName( LPCTSTR szStr);
    HKL     RegisterTable( HWND hWnd, LPUSRDICIMHDR lpIsvUsrDic,
            DWORD dwFileSize, UINT  uCodePage);
    int     CodePageInfo( UINT uCodePage);
#endif 0 // move to imeblink.h!

protected:
    //{{AFX_MSG(CBLinkDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnBrowsetable();
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
