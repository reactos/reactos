/*
 * @implemented
 */
VOID
NTAPI
DbgUnLoadImageSymbols(IN PSTRING Name,
                      IN PVOID Base,
                      IN ULONG_PTR ProcessId)
{
    KD_SYMBOLS_INFO SymbolInfo;

    /* Setup the symbol data */
    SymbolInfo.BaseOfDll = Base;
    SymbolInfo.ProcessId = ProcessId;
    SymbolInfo.CheckSum = SymbolInfo.SizeOfImage = 0;

    /* Load the symbols */
    DebugService2(Name, &SymbolInfo, BREAKPOINT_UNLOAD_SYMBOLS);
}

