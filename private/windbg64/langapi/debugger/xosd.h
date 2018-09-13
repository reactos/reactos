//
// xosdNone must be 0.  The rest do not matter
//

DECL_XOSD(xosdNone,                 0, "No Error")

DECL_XOSD(xosdContinue,             1, "Continue processing EMF")
DECL_XOSD(xosdPunt,                 2, "Pass to next EM")

DECL_XOSD(xosdGeneral,              3, "API failed")
#ifdef DEBUG
DECL_XOSD(xosdUnknown,              4, "xosdUnknown Should be xosdGeneral")
#else
DECL_XOSD(xosdUnknown,              4, "General Debugger error")
#endif
DECL_XOSD(xosdUnsupported,          5, "Feature not available")
DECL_XOSD(xosdInvalidHandle,        6, "Invalid handle passed to API")
DECL_XOSD(xosdInvalidParameter,     7, "Invalid parameter")
DECL_XOSD(xosdDuplicate,            8, "Duplicate EM or TL")
DECL_XOSD(xosdInUse,                9, "EM or TL is in use")
DECL_XOSD(xosdOutOfMemory,         10, "Insufficient memory available")
DECL_XOSD(xosdFileNotFound,        11, "File not found")
DECL_XOSD(xosdAccessDenied,        12, "Access denied")

DECL_XOSD(xosdBadProcess,          13, "Inappropriate or nonexistent process")
DECL_XOSD(xosdBadThread,           14, "Inappropriate or nonexistent thread")
DECL_XOSD(xosdBadAddress,          15, "Invalid address")
DECL_XOSD(xosdInvalidBreakPoint,   16, "nonexistent breakpoint")

DECL_XOSD(xosdBadVersion,          17, "Debugger component versions mismatched")

DECL_XOSD(xosdQueueEmpty,          18, "Queue Empty - no error")
DECL_XOSD(xosdProcRunning,         19, "Operation invalid when process is running")

DECL_XOSD(xosdRead,                20, "Read Failure")
DECL_XOSD(xosdWrite,               21, "Write Failure")

DECL_XOSD(xosdIORedirSyntax,       22, "Syntax error in IO redirection")
DECL_XOSD(xosdIORedirBadFile,      23, "Cannot redirect with this file")
DECL_XOSD(xosdAllThreadsSuspended, 24, "All threads are suspended")

#ifdef DEBUG
DECL_XOSD(xosdAttachDeadlock,      25, "Attach deadlock: This should be a dbcError")
#else
DECL_XOSD(xosdAttachDeadlock,      25, "Error attaching to process")
#endif

DECL_XOSD(xosdEndOfStack,          26, "end of stack")
DECL_XOSD(xosdTargetIsRunning,     27, "Cannot perform operation while target is running")

#ifdef DEBUG
DECL_XOSD(xosdAsmTooFew,           40, "Assembler - Too Few")
DECL_XOSD(xosdAsmTooMany,          41, "Assembler - Too Many")
DECL_XOSD(xosdAsmSize,             42, "Assembler - Size")
DECL_XOSD(xosdAsmBadRange,         43, "Assembler - BadRange")
DECL_XOSD(xosdAsmOverFlow,         44, "Assembler - OverFlow")
DECL_XOSD(xosdAsmSyntax,           45, "Assembler - Syntax")
DECL_XOSD(xosdAsmBadOpcode,        46, "Assembler - Bad Opcode")
DECL_XOSD(xosdAsmExtraChars,       47, "Assembler - Extra Chars")
DECL_XOSD(xosdAsmOperand,          48, "Assembler - Operand")
DECL_XOSD(xosdAsmBadSeg,           49, "Assembler - Bad Seg")
DECL_XOSD(xosdAsmBadReg,           50, "Assembler - Bad Reg")
DECL_XOSD(xosdAsmDivide,           51, "Assembler - Divide")
DECL_XOSD(xosdAsmSymbol,           52, "Assembler - Symbol")
#else
DECL_XOSD(xosdAsmTooFew,           40, "Assembler")
DECL_XOSD(xosdAsmTooMany,          41, "Assembler")
DECL_XOSD(xosdAsmSize,             42, "Assembler")
DECL_XOSD(xosdAsmBadRange,         43, "Assembler")
DECL_XOSD(xosdAsmOverFlow,         44, "Assembler")
DECL_XOSD(xosdAsmSyntax,           45, "Assembler")
DECL_XOSD(xosdAsmBadOpcode,        46, "Assembler")
DECL_XOSD(xosdAsmExtraChars,       47, "Assembler")
DECL_XOSD(xosdAsmOperand,          48, "Assembler")
DECL_XOSD(xosdAsmBadSeg,           49, "Assembler")
DECL_XOSD(xosdAsmBadReg,           50, "Assembler")
DECL_XOSD(xosdAsmDivide,           51, "Assembler")
DECL_XOSD(xosdAsmSymbol,           52, "Assembler")
#endif

DECL_XOSD(xosdLineNotConnected,    70, "Not connected")
DECL_XOSD(xosdCannotConnect,       71, "cannot connect")
DECL_XOSD(xosdCantOpenComPort,     72, "can't open com port")
DECL_XOSD(xosdBadComParameters,    73, "bad com params")
DECL_XOSD(xosdBadPipeServer,       74, "bad pipe server")
DECL_XOSD(xosdBadPipeName,         75, "bad pipe name")
DECL_XOSD(xosdNotRemote,           76, "not remote")
DECL_XOSD(xosdTransportTimedOut,   77, "Transport timed out")




DECL_XOSD(xosdCannotStep,          90, "cannot step")   //donc for mac dm
DECL_XOSD(xosdInvalidRegister,     91, "invalid register")      // donc for mac dm
DECL_XOSD(xosdOpenFailed,          92, "open failed") // donc for mac dm
DECL_XOSD(xosdBadFormat,           93, "bad format") // donc for mac dm
DECL_XOSD(xosdLoadChild,           94, "load child") // donc for mac dm
DECL_XOSD(xosdBPGeneral,           95, "BP General") // donc for mac dm
DECL_XOSD(xosdOutOfThreads,        96, "out of threads") // donc for mac dm


DECL_XOSD(xosdSyntax,             100, "Syntax")   // for mac em
DECL_XOSD(xosdTooManyObjects,     101, "too many objects")  // for mac em
DECL_XOSD(xosdNotFound,           102, "not found")  // for mac em
DECL_XOSD(xosdInvalidFunction,    103, "invalid function") // for mac em
DECL_XOSD(xosdInvalidTID,         104, "invalid TID") // for mac em
DECL_XOSD(xosdInvalidMTE,         105, "invalid MTE") // for mac em


DECL_XOSD(xosdDumpInvalidFile,    130, "Invalid dump file. Please run DUMPCHK.EXE.") // for dmp files
DECL_XOSD(xosdDumpWrongPlatform,  131, "Cross-platform debugging of dump files is not supported. Please run DUMPCHK.EXE.") // for dmp files
