/* File generated automatically from dbghelp.spec; do not edit! */
/* This file can be copied, modified and distributed without restriction. */

#ifdef __GNUC__
static void __wine_unimplemented( const char *func ) __attribute__((noreturn));
#endif

struct exc_record {
  unsigned int code, flags;
  void *rec, *addr;
  unsigned int params;
  const void *info[15];
};

extern void __stdcall RtlRaiseException( struct exc_record * );

static void __wine_unimplemented( const char *func )
{
  struct exc_record rec;
  rec.code    = 0x80000100;
  rec.flags   = 1;
  rec.rec     = 0;
  rec.params  = 2;
  rec.info[0] = "dbghelp.dll";
  rec.info[1] = func;
#ifdef __GNUC__
  rec.addr = __builtin_return_address(1);
#else
  rec.addr = 0;
#endif
  for (;;) RtlRaiseException( &rec );
}

void __wine_stub_dbghelp_dll_1(void) { __wine_unimplemented("DbgHelpCreateUserDump"); }
void __wine_stub_dbghelp_dll_2(void) { __wine_unimplemented("DbgHelpCreateUserDumpW"); }
void __wine_stub_dbghelp_dll_5(void) { __wine_unimplemented("EnumerateLoadedModules64"); }
void __wine_stub_dbghelp_dll_6(void) { __wine_unimplemented("ExtensionApiVersion"); }
void __wine_stub_dbghelp_dll_10(void) { __wine_unimplemented("FindExecutableImageEx"); }
void __wine_stub_dbghelp_dll_11(void) { __wine_unimplemented("FindFileInPath"); }
void __wine_stub_dbghelp_dll_12(void) { __wine_unimplemented("FindFileInSearchPath"); }
void __wine_stub_dbghelp_dll_15(void) { __wine_unimplemented("ImageDirectoryEntryToDataEx"); }
void __wine_stub_dbghelp_dll_23(void) { __wine_unimplemented("MiniDumpReadDumpStream"); }
void __wine_stub_dbghelp_dll_24(void) { __wine_unimplemented("MiniDumpWriteDump"); }
void __wine_stub_dbghelp_dll_27(void) { __wine_unimplemented("StackWalk64"); }
void __wine_stub_dbghelp_dll_30(void) { __wine_unimplemented("SymEnumSym"); }
void __wine_stub_dbghelp_dll_34(void) { __wine_unimplemented("SymEnumerateModules64"); }
void __wine_stub_dbghelp_dll_36(void) { __wine_unimplemented("SymEnumerateSymbols64"); }
void __wine_stub_dbghelp_dll_37(void) { __wine_unimplemented("SymEnumerateSymbolsW"); }
void __wine_stub_dbghelp_dll_38(void) { __wine_unimplemented("SymEnumerateSymbolsW64"); }
void __wine_stub_dbghelp_dll_43(void) { __wine_unimplemented("SymFunctionTableAccess64"); }
void __wine_stub_dbghelp_dll_44(void) { __wine_unimplemented("SymGetFileLineOffsets64"); }
void __wine_stub_dbghelp_dll_46(void) { __wine_unimplemented("SymGetLineFromAddr64"); }
void __wine_stub_dbghelp_dll_47(void) { __wine_unimplemented("SymGetLineFromName"); }
void __wine_stub_dbghelp_dll_48(void) { __wine_unimplemented("SymGetLineFromName64"); }
void __wine_stub_dbghelp_dll_50(void) { __wine_unimplemented("SymGetLineNext64"); }
void __wine_stub_dbghelp_dll_52(void) { __wine_unimplemented("SymGetLinePrev64"); }
void __wine_stub_dbghelp_dll_54(void) { __wine_unimplemented("SymGetModuleBase64"); }
void __wine_stub_dbghelp_dll_56(void) { __wine_unimplemented("SymGetModuleInfo64"); }
void __wine_stub_dbghelp_dll_57(void) { __wine_unimplemented("SymGetModuleInfoW"); }
void __wine_stub_dbghelp_dll_58(void) { __wine_unimplemented("SymGetModuleInfoW64"); }
void __wine_stub_dbghelp_dll_62(void) { __wine_unimplemented("SymGetSymFromAddr64"); }
void __wine_stub_dbghelp_dll_64(void) { __wine_unimplemented("SymGetSymFromName64"); }
void __wine_stub_dbghelp_dll_66(void) { __wine_unimplemented("SymGetSymNext64"); }
void __wine_stub_dbghelp_dll_68(void) { __wine_unimplemented("SymGetSymPrev64"); }
void __wine_stub_dbghelp_dll_73(void) { __wine_unimplemented("SymLoadModule64"); }
void __wine_stub_dbghelp_dll_74(void) { __wine_unimplemented("SymLoadModuleEx"); }
void __wine_stub_dbghelp_dll_76(void) { __wine_unimplemented("SymMatchString"); }
void __wine_stub_dbghelp_dll_78(void) { __wine_unimplemented("SymRegisterCallback64"); }
void __wine_stub_dbghelp_dll_79(void) { __wine_unimplemented("SymRegisterFunctionEntryCallback"); }
void __wine_stub_dbghelp_dll_80(void) { __wine_unimplemented("SymRegisterFunctionEntryCallback64"); }
void __wine_stub_dbghelp_dll_84(void) { __wine_unimplemented("SymSetSymWithAddr64"); }
void __wine_stub_dbghelp_dll_86(void) { __wine_unimplemented("SymUnDName64"); }
void __wine_stub_dbghelp_dll_88(void) { __wine_unimplemented("SymUnloadModule64"); }
void __wine_stub_dbghelp_dll_91(void) { __wine_unimplemented("WinDbgExtensionDllInit"); }
