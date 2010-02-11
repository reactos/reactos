VOID
NTAPI
DbgLoadImageSymbols(IN PSTRING Name,
                    IN PVOID Base,
                    IN ULONG_PTR ProcessId)
{
    PIMAGE_NT_HEADERS NtHeader;
    KD_SYMBOLS_INFO SymbolInfo;

    /* Setup the symbol data */
    SymbolInfo.BaseOfDll = Base;
    SymbolInfo.ProcessId = ProcessId;

    /* Get NT Headers */
    NtHeader = RtlImageNtHeader(Base);
    if (NtHeader)
    {
        /* Get the rest of the data */
        SymbolInfo.CheckSum = NtHeader->OptionalHeader.CheckSum;
        SymbolInfo.SizeOfImage = NtHeader->OptionalHeader.SizeOfImage;
    }
    else
    {
        /* No data available */
        SymbolInfo.CheckSum =
        SymbolInfo.SizeOfImage = 0;
    }

    /* Load the symbols */
    DebugService2(Name, &SymbolInfo, BREAKPOINT_LOAD_SYMBOLS);
}

