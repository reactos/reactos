
VOID DbgBreakPoint(VOID);
ULONG DbgPrint(PCH Format,...);   

#define DBG_GET_SHOW_FACILITY 0x0001
#define DBG_GET_SHOW_SEVERITY 0x0002
#define DBG_GET_SHOW_ERRCODE  0x0004
#define DBG_GET_SHOW_ERRTEXT  0x0008
VOID DbgGetErrorText(NTSTATUS ErrorCode, PUNICODE_STRING ErrorText, ULONG Flags);
VOID DbgPrintErrorMessage(NTSTATUS ErrorCode);

