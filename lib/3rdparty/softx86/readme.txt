Softx86: Software x86 CPU emulator library v0.00.0032

UNPACKING THE SOURCE TARBALLS
=============================

Softx86 is distributed as several tarballs. The base tarball,
softx86-vx.xx.xxxx, contains the source code for softx86
itself. No prebuild binaries are provided. The other source
tarballs are meant to be obtained and then extracted in the
same directory that you extracted softx86 itself.

The additional tarballs contain:
- The sample binaries, already assembled
- Prebuilt Win32 versions of the libraries
  and softx86dbg in DLL and static form.

The Softx86 make system automatically detects the
presence of the softx87 directory (which exists
only if you extracted the softx87 tarball correctly).
If it is found, makefiles are modified so that
softx86dbg is built and compiled to use it.

as of v0.00.0030 the softx87 source tree is now
distributed with softx86.

EXAMPLE OF UNPACKING THE ENTIRE SOFTX86 PROJECT:

cd /usr/src
tar -xzf softx86-v0.00.0032
cd softx86
make
make install

PLATFORMS SUPPORTED
===================

Softx86 is carefully written and maintained so that the same source
tree can be used to produce Win32 and Linux binaries as well as
Win32 DLLs and Linux shared libraries. Compiler-specific optimizations
are welcome though they will be separated with #ifdef...#else...#endif
statements and macros for full portability. Optimizations and code
organization sensitive to various characteristics of the platform can
be controlled via #defines in include/config.h.

COMPILING FOR WIN32
===================

To compile the Win32 versions you will need Microsoft Visual C++ 32-bit
for Visual Studio 6.0 or higher. Service pack 5 and the latest processor pack
are recommended but not required.

From Visual Studio, select "open workspace" and open softx86.dsw. Select
"softx86 - release", then hilight both projects in the workspace window.
Right click and select "build selection only" from the popup menu.

This should make the following files:

bin/softx86dbg.exe	Win32 console application (NOT DOS!) not unlike
                    DOS DEBUG to test softx86.

lib/softx86.lib		Static library of softx86.

WARNING: The interfaces and structures can and will change! If you distribute
         a binary version of your program that uses Softx86, later versions
         may break your program! This is alpha software, the context structure
	 has not been finalized!

To recompile the test COM programs, you will need the Win32 or DOS version of
NASM installed somewhere referred to in your PATH= variable or in the same
directory.

COMPILING FOR LINUX
===================

To compile for Linux you will need GCC 3.2.xx or higher, NASM 0.98 or higher,
make 3.79 or higher, and GNU ld 2.13 or higher.

If you extracted the source tarball over the source of older versions please
type "make distclean" to clean up the source tree.

Type "make" to compile the entire project.
This will make the following files:

bin/softx86dbg          console application not unlike DOS DEBUG to test
                        softx86.

bin/softx86dbg-static	statically linked version of softx86dbg.

lib/libsoftx86.a        Static library of softx86.

lib/libsoftx86.so       Shared library of softx86.

Then, super-user to or logon as "root" and type "make install". This
will run a bash shell script that copies libsoftx86.a and libsoftx86.so
to your /usr/lib directory. This script will also copy softx86.h and
softx86cfg.h to your /usr/include directory. If any errors occur during
the installation process the script will immediately exit with a message.

WARNING: The interfaces and structures can and will change! If you distribute
         a binary version of your program that uses Softx86, later versions
         may break your program! This is alpha software, the context structure
         has not been finalized!

USING SOFTX86DBG
================

Softx86dbg is a DOS DEBUG-style debugger that uses Softx86. It allows
very simple debugging of COM executable binary images written for DOS.
It does *NOT* emulate directly from an assembly language listing!

Softx86dbg does not load a COM image by default. You must specify one
on the command line.

DESIGN GOALS
============

The goal of Softx86 is to eventually become a full functioning software
clone of the Intel 80x86 CPU. I want to stress, however, that this library is
intended to become exactly that and no more. The interface between this
library and the host application is designed so that this library is only
responsible for fetching data and executing/decompiling instructions; the host
application is responsible for emulating other aspects of the PC platform
hardware (i.e. system timer or keyboard controller) and providing the
simulated memory for the CPU to fetch data from or write data to, thus
this library can be used to emulate ANY platform or environment involving
some use of the x86.

WHY NOT C++
===========

I am writing the majority of this library in C (even though the softx86dbg
program is in fact written in C++) so that it can be linked to either C or
C++ programs (whatever your preference). Writing this library in C++ in
contrast would render it unusable for C programs or require the use of
"wrappers".

HOW DOES THE LIBRARY INTERACT WITH THE HOST APPLICATION?
========================================================

The host application allocates memory for a "context structure" that
represents one CPU (of type struct softx86_ctx). The host app then calls
softx86_init() to initialize the memebrs of that structure, given a value
specifying which revision of the 80x86 to emulate. Once done, the host app
can use other library API functions like softx86_set_instruction_ptr() to
set the instruction pointer, register values, etc.

The library CPU does not automatically start executing instructions on
it's own. It only executes instructions when the host app tells it to by
repeatedly calling softx86_step() per instruction. Thus, how fast the
software CPU runs is highly dependant on how efficient the host application
is.

The library can (and will) request memory and I/O data from the host app
via callback function pointers contained within the context structure. There
is currently no API function to set them, it is assumed that the host
application will assign to them the address of functions within itself that
will fullfill the I/O request.

There are currently four callback functions to the host app:

void (*on_read_memory)(/* softx86_ctx */ void* _ctx,
	sx86_udword address,sx86_ubyte *buf,int size);

void (*on_read_io)(/* softx86_ctx */ void* _ctx,sx86_udword address,
	sx86_ubyte *buf,int size);

void (*on_write_memory)(/* softx86_ctx */ void* _ctx,sx86_udword address,
	sx86_ubyte *buf,int size);

void (*on_write_io)(/* softx86_ctx */ void* _ctx,sx86_udword address,
	sx86_ubyte *buf,int size);

on_read_memory:  called when the CPU wants to fetch from simulated memory
on_write_memory: called when the CPU wants to store to simulated memory
on_read_io:      called when the CPU wants to fetch from an I/O port
                 (typically the IN/INSB instruction)
on_write_io:     called when the CPU wants to store to an I/O port
                 (typically the OUT/OUTSB instruction)

SOFTX86 DOESN'T SUPPORT FPU INSTRUCTIONS BY ITSELF
==================================================

Softx86 does not support FPU instructions by itself,
following the design of the original chip. Instead,
FPU opcodes are handed off to a callback function
which can then call an external library that functions
as the 8087. If the callback function is not assigned,
the FPU opcodes D8h thru DFh are treated as unknown and
invalid opcodes.

/* this is called when the CPU is asked to execute an FPU opcode */
int (*on_fpu_opcode_exec)
	(/* softx86_ctx */void* _ctx,sx86_ubyte opcode);

/* this is called when the CPU is asked to decompile an FPU opcode */
int (*on_fpu_opcode_dec)
	(/* softx86_ctx */void* _ctx,sx86_ubyte opcode,char buf[128]);

