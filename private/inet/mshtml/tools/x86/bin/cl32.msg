                          C/C++ COMPILER OPTIONS

                              -OPTIMIZATION-
/O1 minimize space                       /Op[-] improve floating-pt consistency
/O2 maximize speed                       /Os favor code space
/Oa assume no aliasing                   /Ot favor code speed
/Ob<n> inline expansion (default n=0)    /Ow assume cross-function aliasing
/Od disable optimizations (default)      /Ox maximum opts. (/Ogityb1 /Gs)
/Og enable global optimization           /Oy[-] enable frame pointer omission
/Oi enable intrinsic functions
                             -CODE GENERATION-
/G3 optimize for 80386                   /Gf enable string pooling
/G4 optimize for 80486                   /GF enable read-only string pooling
/G5 optimize for Pentium                 /Ge enable stack checking calls
/GB optimize for blended model (default) /Gs[num] disable stack checking calls
/Gd __cdecl calling convention           /Gh enable hook function call
/Gr __fastcall calling convention        /GR[-] enable C++ RTTI
/Gz __stdcall calling convention         /GT enable fiber-safe TLS
/GA optimize for Windows Application     /GX[-] enable C++ EH
/GD optimize for Windows DLL             /Gi[-] enable incremental compilation
/QIfdiv[-] generate code for Pentium FDIV fix /Gm[-] enable minimal rebuild
/QI05[-] generate code for Pentium 0x0f erratum fix/Gy separate functions for linker
                              -OUTPUT FILES-
/Fa[file] name assembly listing file     /Fo<file> name object file
/FA[sc] configure assembly listing       /Fp<file> name precompiled header file
/Fd[file] name .PDB file                 /Fr[file] name source browser file
/Fe<file> name executable file           /FR[file] name extended .SBR file
/Fm[file] name map file
                              -PREPROCESSOR-
/C don't strip comments                  /FI<file> name forced include file
/D<name>{=|#}<text> define macro         /U<name> remove predefined macro
/E preprocess to stdout                  /u remove all predefined macros
/EP preprocess to stdout, no #line       /I<dir> add to include search path
/P preprocess to file                    /X ignore "standard places"
                                -LANGUAGE-
/vd{0|1} disable/enable vtordisp         /Za disable extensions (implies /Op)
/vm<x> type of pointers to members       /Ze enable extensions (default)
/Zi generate debugging information       /Zg generate function prototypes
/Z7 generate old-style debug info        /Zl omit default library name in .OBJ
/Zd line number debugging info only      /Zs syntax check only
/Zp[n] pack structs on n-byte boundary
                              -MISCELLANEOUS-
/?, /help print this help message        /V<string> set version string
/c compile only, no link                 /w disable all warnings
/H<num> max external name length         /W<n> set warning level (default n=1)
/J default char type is unsigned         /WX treat warnings as errors
/nologo suppress copyright message       /Yc[file] create .PCH file
/Tc<source file> compile file as .c      /Yd put debug info in every .OBJ
/Tp<source file> compile file as .cpp    /Yu[file] use .PCH file
/TC compile all files as .c              /YX[file] automatic .PCH
/TP compile all files as .cpp            /Zn turn off SBRPACK for .SBR files
                                 -LINKING-
/MD link with MSVCRT.LIB                 /MTd link with LIBCMTD.LIB debug library
/MDd link with MSVCRTD.LIB debug library /F<num> set stack size
/ML link with LIBC.LIB                   /LD Create .DLL
/MLd link with LIBCD.LIB debug library   /LDd Create .DLL debug libary
/MT link with LIBCMT.LIB                 /link [linker options and libraries]
