BOOLEAN NTAPI GopVidInitialize(_In_ PLOADER_PARAMETER_BLOCK LoaderBlock);
VOID    NTAPI GopVidCleanUp(VOID);
VOID    NTAPI GopVidResetDisplay(_In_ BOOLEAN HalReset);
VOID    NTAPI GopVidSolidColorFill(_In_ ULONG L,_In_ ULONG T,_In_ ULONG R,_In_ ULONG B,_In_ UCHAR Color);
VOID    NTAPI GopVidBufferToScreenBlt(_In_reads_bytes_(Delta*Height) PUCHAR Buf,_In_ ULONG L,_In_ ULONG T,_In_ ULONG W,_In_ ULONG H,_In_ ULONG Delta);
VOID    NTAPI GopVidScreenToBufferBlt(_Out_writes_bytes_(Delta*Height) PUCHAR Buf,_In_ ULONG L,_In_ ULONG T,_In_ ULONG W,_In_ ULONG H,_In_ ULONG Delta);
VOID    NTAPI GopVidDisplayString(_In_z_ PUCHAR String);
VOID    NTAPI GopVidBitBlt(_In_ PUCHAR Buffer,_In_ ULONG Left,_In_ ULONG Top);
