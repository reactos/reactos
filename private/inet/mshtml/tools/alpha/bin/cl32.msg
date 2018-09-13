			  ALPHA C/C++ COMPILER OPTIONS
                                 -OPTIMIZATION-
/O1 minimize space                       /Op[-] improve floating-pt consistency
/O2 maximize speed                       /Os favor code space
/Ob<n> inline expansion (default n=2)    /Ox maximum optimization
/Od disable optimizations                /QAOu[n] set loop unrolling
/Oi[-] enable intrinsic functions
                                -CODE GENERATION-
/Ge enable stack limit checks (default)  /GR[-] enable C++ RTTI
/Gf enable writable string pooling       /Gs disable stack limit checks
/GF enable read-only string pooling      /Gt<n> threshold for gp-relative data
/Gh enable hook function call            /Gy separate functions for linker
/Gm[-] enable minimal rebuild		 /GX[-] enable C++ EH           
                                     -OUTPUT FILES-
/FA[a|c|s] select <asm|codebytes|source> /Fo[file] name object file
/Fa[file] name assembly listing file     /Fp[file] name precompiled header file
/Fd[file] name .PDB file                 /FR[file] name extended .SBR file
/Fe[file] name executable file           /Fr[file] name source browser file
/Fm[file] name map file
                                 -PREPROCESSOR-
/C don't strip comments                  /I<dir> add to include search path
/D<name>{=|#}<text> define macro         /P preprocess to file
/E preprocess to stdout                  /U<name> remove predefined macro
/EP preprocess to stdout, no #line       /u remove all predefined macros
/FI<file> name forced include file       /X ignore "standard places"
                                   -LANGUAGE-
/vd{0|1} disable/enable vtordisp         /Zg generate function prototypes
/vm<x> type of pointers to members       /Zh home arguments (low-level debug)
/Z7 include Windbg debug info            /Zi prepare for debugging (CodeView)
/Za disable extensions (implies /Op)     /Zl omit default library name in .OBJ
/Zd include line number debug info       /Zp[n] pack structs on n-byte boundary
/Ze enable extensions (default)          /Zs syntax check only
                                 -MISCELLANEOUS-
/?, /help, /HELP  print this message     /V<string> set version string
/c compile only, no link                 /W<n> set warning level (default n=1)
/H<number> max external name length      /w disable all warnings
/J default char type is unsigned         /WX treat warnings as errors
/nologo suppress copyright message       /Yc[file] create .PCH file
/QAgl use longword granularity (default) /Yd put debug info in every .OBJ
/QAgq use quadword granularity           /Yu[file] use .PCH file
/QAieee<0|1|2> control ieee support      /YX[file] automatic .PCH
/Tc<source file> compile file as .c      /Zn turn off SBRPACK for .SBR files
/Tp<source file> compile file as .cpp
                                    -LINKING-
/F<number> stack size (hex bytes)        /MDd link with MSVCRTD.LIB debug library
/LD Create .DLL                          /ML  link with LIBC.LIB
/LDd Create .DLL debug library           /MLd link with LIBCD.LIB debug library
/link [linker options and libraries]     /MT  link with LIBCMT.LIB
/MD link with MSVCRT.LIB                 /MTd link with LIBCMTD.LIB debug library

    -ACCEPTED FOR COMPATIBILITY WITH OTHER MICROSOFT COMPILERS, BUT IGNORED-
/G3 /G4 /G5 /GB /Gd /Gr /Gz /Oa /Og /Ot /OV /Ow /Oy
