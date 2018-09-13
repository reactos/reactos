#ifndef SNMPELEA_P_H
#define SNMPELEA_P_H

	// prototype definitions for all log message functions

extern VOID WriteLog(NTSTATUS);
extern VOID WriteLog(NTSTATUS, DWORD);
extern VOID WriteLog(NTSTATUS, DWORD, DWORD);
extern VOID WriteLog(NTSTATUS, LPTSTR, DWORD, DWORD);
extern VOID WriteLog(NTSTATUS, DWORD, LPTSTR, LPTSTR, DWORD);
extern VOID WriteLog(NTSTATUS, DWORD, LPTSTR, DWORD, DWORD);
extern VOID WriteLog(NTSTATUS, LPTSTR, DWORD);
extern VOID WriteLog(NTSTATUS, LPTSTR);
extern VOID WriteLog(NTSTATUS, LPCTSTR, LPCTSTR);

#endif						// end of snmpelep.h
