This is some preliminary information on using PICE. I am planning to write
a detailed manual later.

BETA-BETA-BETA-BETA-BETA-BETA-BETA-BETA-BETA-BETA-BETA-BETA-BETA-BETA-BETA-BETA-BETA-BETA

    PICE for Reactos is in early beta stage of development. It still has many bugs.

BETA-BETA-BETA-BETA-BETA-BETA-BETA-BETA-BETA-BETA-BETA-BETA-BETA-BETA-BETA-BETA-BETA-BETA


PICE is a kernel debugger that was ported for Reactos (the original Linux
project by Klaus P. Gerlicher and Goran Devic may be found here:
http://pice.sourceforge.net).

Installation and use:

1. PICE is loaded like a regular device driver. The only limitation - it must
be loaded before keyboard.sys driver. You should add:

   LdrLoadAutoConfigDriver( L"pice.sys" );

in ntoskrnl/ldr/loader.c after the line loading keyboard driver.

2. You should copy pice.cfg and ntoskrnl.sym to \SystemRoot\symbols directory
of Reactos.

3. If you want to add symbolic information you should use loader.exe to
create .dbg file from the unstrippped version of exe or driver:
For example:
pice\loader\loader.exe -t ntoskrnl.nostrip.exe

After that copy .dbg file to \SystemRoot\symbols and add a line to pice.cfg:
\\SystemRoot\symbols\ntoskrnl.dbg.

Pice will load the symbols during boot. For large .dbg files it may take a
while (ntoskrnl.dbg is ~3Mb). You may find that loading time under bochs is
quite slow, although otherwise performance should be fine.

Enjoy,
Eugene


