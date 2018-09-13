C COMPILER OPTIONS
-MEMORY MODEL-
/AS small model (default)
/AC compact model
/AM medium model
/AL large model
/AH huge model
/AT tiny model (.COM files)
/A<string> (custom memory model)
-OPTIMIZATION-
/O enable optimization (same as /Ot)
/O1 minimize space
/O2 maximize speed
/Oa assume no aliasing
/Ob<n> inline expansion (default n=0)
/Oc local common subexpression opt.
/Od disable optimization (default)
/Oe enable registers allocation
/Of[-] toggle p-code quoting
/Og global common subexpression opt.
/Oi enable intrinsic functions
/Ol enable loop optimizations
/On disable "unsafe" optimizations
/Oo[-] toggle post code-gen. opt.
/Op[-] improve floating-pt consistency
/Oq enable maximum p-code optimization
/Or gen. common exit code (CodeView)
/Os favor code size
/Ot favor code speed
/Ov[-] toggle p-code frame sorting
/OV<n> control inlining by func. size
/Ow assume cross-function aliasing
/Ox maximum opts. (/Ob1cegilnot /Gs)
/Oz enable "unsafe" optimizations
-CODE GENERATION-
/G0 8086 instructions (default)
/G1  186 instructions
/G2  286 instructions
/G3  386 instructions
/GA protected-mode Win entry/exit code
/GD protected-mode Win entry/exit code
/GE<x> customize Windows entry/exit
/Gq backwards compatibility with v. 6
/Gc Pascal style calling conventions
/Gd C style calling conventions
/Ge use stack-check calls
/Gf enable string pooling
/Gs remove stack-check calls
/Gn remove p-code native entry points
/Gp<num> specify p-code entry tables
/Gr _fastcall style calling convention
/Gt[num] data size threshold
/GW real-mode Windows entry/exit code
/Gw real-mode Windows entry/exit code
/Gx assume that data is near
/Gx- assume that data is far
/Gy separate functions for linker
/Zr check null pointers (/f only)
-OUTPUT FILES-
/Fa[file] name assembly listing file
/Fc[file] name source/object listing
/Fd[file] name .PDB filename
/Fe<file> name executable filename
/Fl[file] name object listing filename
/Fm[file] name map filename
/Fo<file> name object filename
/Fr[file] name .SBR filename
/Fp<file> name .PCH filename
/FR[file] name extended .SBR filename
/Fs[file] name source listing filename
-PREPROCESSOR-
/C don't strip comments
/D<name>[=|#text] define macro
/E preprocess to stdout
/EP same as /E but no #line
/I<directory> add #include path
/P preprocess to file
/U<name> remove predefined macro
/u remove all defined macros
/X ignore "standard places"
-LANGUAGE-
/vd{0|1} disable/enable vtordisp
/vm<x> type of pointers to members
/Z7 C 7 style CodeView information
/Za disable extensions (implies /Op)
/Zd line number information
/Ze enable extensions (default)
/Zg generate function prototypes
/Zi prepare for debugging (CodeView)
/Zl omit default library name in .OBJ
/Zp[n] pack structs on n-byte boundary
/Zs check syntax only
-FLOATING POINT-
/FPa calls with altmath
/FPc calls with emulator
/FPc87 calls with 8087 library
/FPi inline with emulator (default)
/FPi87 inline with 8087
-SOURCE LISTING-
/Sl<columns> set line width
/Sp<lines> set page length
/St<string> set title string
/Ss<string> set subtitle string
-MISCELLANEOUS-
/batch specify batch mode compilation
/Bm<num> set compiler's available mem.
/c compile only, no link
/H<num> external name length
/J default char type is unsigned
/f select fast compiler (default)
/f- select optimizing compiler
/Yc create .PCH file
/Yd put debug info in .PCH file
/Yu use .PCH file
/YX automatic precompiled header
/nologo suppress copyright message
/Mq compile for QuickWin
/ND<name> name data segment
/NM<name> name code segment
/NQ<name> combine p-code temp segments
/NT<name> name code segment
/NV<name> name far v-table segment
/Tc<file> compile file without .c
/Tp<file> compile file without .cpp
/V<string> set version string
/W<n> warning level (default n=1)
/w disable all warnings
/WX treat all warnings as errors
/Zn turn off SBRPACK for .SBR files
-MASM SUPPORT-
/MA<MASM switch>
/Ta<file> assemble file without .asm
-LINKING-
/F <hex_num> stack size (hex. bytes)
/Lr append 'r' to default lib in .OBJ
/link [lib] give lib name to linker
/Ln do not link CRT.LIB
/Ld select dynamically-linked library
/Lw select statically-linked library
