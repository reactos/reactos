                          C/C++ COMPILER OPTIONS

                              -OPTIMIZATION-

/O1 minimize space (/Osgb1 /Gfsy)        /Oi[-] enable intrinsic functions
/O2 maximize speed (/Ox /Gfsy)           /Op[-] improve floating-pt consistency
/Oa assume no aliasing                   /Os favor code space
/Ob<n> inline expansion (Ob0 *)          /Ot favor code speed *
/Od disable optimizations *              /Ow assume cross-function aliasing
/Og enable global optimization           /Ox maximum opts. (/Ogitb1 /Gs)

                             -CODE GENERATION-

/Gd __cdecl calling convention *         /Gy separate functions for linker
/Ge enable stack overflow checking       /Gz __stdcall calling convention
/Gf enable string pooling                /QP601 generate 601 instructions
/GF enable read-only string pooling      /QP603 generate 603 instructions
/Gh enable profiler hooks                /QP604 generate 604 instructions *
/GR[-] enable C++ RTTI                   /QP620 generate 620 instructions
/Gs[num] disable stack checking calls    /QPi all DLL imports are declared
/GX[-] enable C++ EH

                              -OUTPUT FILES-

/Fa[file] name assembly listing file     /Fo<file> name object file
/FA[sc] configure assembly listing       /Fp<file> name precompiled header file
/Fd[file] name .PDB file                 /Fr[file] name source browser file
/Fe<file> name executable file           /FR[file] name extended .SBR file
/Fm[file] name map file

                              -PREPROCESSOR-

/C don't strip comments                  /I<dir> add to include search path
/D<name>{=|#}<text> define macro         /P preprocess to file
/E preprocess to stdout                  /U<name> remove predefined macro
/EP preprocess to stdout, no #line       /u remove all predefined macros
/FI<file> name forced include file       /X ignore "standard places"

                                -LANGUAGE-

/vd{0|1} disable/enable vtordisp         /Zi generate debugging information
/vm<x> type of pointers to members       /Zg generate function prototypes
/Z7 generate old-style debug info        /Zl omit default library name
/Za disable extensions (implies /Op)     /Zp[n] pack structs on n-bytes (Zp8 *)
/Zd line number debugging info only      /Zs syntax check only
/Ze enable Microsoft extensions *

                              -MISCELLANEOUS-

/?, /help print this help message        /w disable all warnings
/c compile only, no link                 /W<n> set warning level (default n=1)
/H<num> max external name length         /WX treat warnings as errors
/J default char type is unsigned         /Yc[file] create .PCH file
/nologo suppress copyright message       /Yd put debug info in every .OBJ
/Tc<source file> compile file as .c      /Yu[file] use .PCH file
/Tp<source file> compile file as .cpp    /YX[file] automatic .PCH
/V<string> set version string            /Zn turn off SBRPACK for .SBR files

                                 -LINKING-

/F<num> set stack size                   /MT link with LIBCMT.LIB
/MD link with MSVCRT.LIB                 /MTd link with LIBCMTD.LIB (debug)
/MDd link with MSVCRTD.LIB (debug)       /LD Create .DLL
/ML link with LIBC.LIB                   /LDd Create .DLL using debug library
/MLd link with LIBCD.LIB (debug)         /link [linker options and libraries]

                                    ---

                            * default switches
