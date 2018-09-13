/*++

    This file contains the symbolic names and test for SH errors

--*/

DECL_STR( sheNone,                 0, "symbols loaded"               )
DECL_STR( sheNoSymbols,            1, "no symbols loaded"            )
DECL_STR( sheFutureSymbols,        2, "symbol format not supported"  )
DECL_STR( sheMustRelink,           3, "symbol format not supported"  )
DECL_STR( sheNotPacked,            4, "must run cvpack on symbols"   )
DECL_STR( sheOutOfMemory,          5, "out of memory"                )
DECL_STR( sheCorruptOmf,           6, "symbol information corrupt"  )
DECL_STR( sheFileOpen,             7, "could not open symbol file"   )
DECL_STR( sheSuppressSyms,         8, "symbol loading suppressed"    )
DECL_STR( sheDeferSyms,            9, "symbol loading deferred"      )
DECL_STR( sheSymbolsConverted,    10, "symbols converted & loaded"   )
DECL_STR( sheBadTimeStamp,        11, "has mismatched timestamps"    )
DECL_STR( sheBadChecksum,         12, "has mismatched checksums"     )
DECL_STR( shePdbNotFound,         13, "can't find/open pdb file"     )
DECL_STR( shePdbBadSig,           14, "internal pdb signature doesn't match sym handler" )
DECL_STR( shePdbInvalidAge,       15, "pdb info doesn't match image" )
DECL_STR( shePdbOldFormat,        16, "pdb format is obsolete"       )
DECL_STR( sheConvertTIs,          17, "16 bit data was converted to 32 bits")  // NB09 format, needs conversion to 32-bit type indices
DECL_STR( sheJavaInvalidClass,    18, "Invalid Java class"           )  // the class given is invalid
DECL_STR( sheJavaNoCurrentProcess,19, "No Java process"              )  // no current process
DECL_STR( sheJavaInvalidFile,     20, "Invalid Java file"            )  // the file given is invalid
DECL_STR( sheExportsConverted,    21, "exports loaded"               )
DECL_STR( sheCouldntReadImageHeader, 22, "Could not verify that symbols match, because the image header is paged out" )
        // When the debugger cannot read the image header (because it 
        // is paged out), it gets -1 for timestamps and checksums.  
        // This message really means that it could not verify that the
        // symbols match, not that it thinks they don't.

//
// Last resort error returned by SHLszGetErrorText()
// always the last one...
//
DECL_STR( sheMax,                 23, "unknown symbol handler error" )  // marker for count of she's
