NOTES on SHELL NTSD EXTENSION (cdturner 11/06/97)
=============================

If adding a command, add a DOIT line to exts.h, this provides both the help text and the declaration of the
entry point that gets generated into the .def file.

If adding a command in a c++ file, don't forget to extern "C" it.

Note, all commands start with I<command name>

If you need to access memory within a command, then you need to use the tryMove or TryMoveDword commands 
to copy it from the debuggee to the memory context of the debugger extension. See Iflags() for example.

If you wish to add a flags set of enums, these are added to shlexts.c, add an array of the bits in order starting
from 0. If a bit is not used, then use the NO_FLAG macro. Add the array to the GF_FLAGS enum before the GF_MAX 
and to the aargFlag[] array. The rest is magic.

\nt\private\windows\inc\stdexts.h and stdext.c provide a bunch of useful functions for writing commands, including option cracking macros (if you chose not to let DOIT do it for you), Memory Access functions ..etc.