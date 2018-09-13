//
// Copyright (c) 1997-1999 Microsoft Corporation.
//
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

typedef WCHAR UNALIGNED *LPUNATSTR;

#ifdef __cplusplus
extern "C"
{
#endif

HKL 
RegisterTable( 
HWND            hWnd,
LPUSRDICIMHDR   lpIsvUsrDic,
DWORD           dwFileSize,
UINT            uCodePage);

HKL 
MatchImeName( 
LPCTSTR         szStr);

int
CodePageInfo(
    UINT uCodePage);

#ifdef __cplusplus
}
#endif
