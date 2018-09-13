#ifndef PORTSTR_H
#define PORTSTR_H

//
//  exported from ports.c
//
extern TCHAR  m_szColon[];
extern TCHAR  m_szComma[];
extern TCHAR  m_szCloseParen[];
extern TCHAR  m_szPorts[];
extern TCHAR  m_szCOM[];
extern TCHAR  m_szSERIAL[];

extern TCHAR m_szRegSerialMap[];

extern TCHAR m_szFIFO[];
extern TCHAR m_szDosDev[];
extern TCHAR m_szPollingPeriod[];
extern TCHAR m_szPortName[];


// Strings needed for parallel port installation.
//
extern TCHAR m_szPortSubClass[];
extern TCHAR m_szParallelClassDevName[];
extern TCHAR m_szParallelClassHwId[];

extern int m_nBaudRates[];

extern TCHAR m_sz9600[];

extern TCHAR m_szDefParams[];
extern short m_nDataBits[];
extern TCHAR *m_pszParitySuf[];
extern TCHAR *m_pszLenSuf[];
extern TCHAR *m_pszStopSuf[];
extern TCHAR *m_pszFlowSuf[];

#endif // PORTSTR_H
