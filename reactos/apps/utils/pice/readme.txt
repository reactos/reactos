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
be loaded after keyboard.sys driver. You should add:

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

Key combination to break into debugger is CTRL-D.
You may need to press CTRL button upon return from the debugger if you get
"funny" symbols when you type.

List of commands:

gdt      display current global descriptor table
idt	 display current interrupt descriptor table
x 	 return to Reactos
t	 single step one instruction
vma	 displays VMAs
h	 list help on commands
page	 dump page directories
proc	 list all processes
dd	 display dword memory
db	 display byte memory
u	 disassemble at address
mod	 displays all modules
bpx	 set code breakpoint
bl	 list breakpoints
bc	 clear breakpoints
ver	 display pICE version and state information
hboot	 hard boot the system
cpu	 display CPU special registers
stack	 display call stack
.	 unassemble at current instruction
p	 single step over call
i	 single step into call
locals	 display local symbols
table	 display loaded symbol tables
file	 display source files in symbol table
sym	 list known symbol information
?	 evaluate an expression     (global symbols only)
src	 sets disassembly mode
wc	 change size of code window
wd	 change size of data window
r	 sets or displays registers
cls	 clear output window
pci	 show PCI devices
next	 advance EIP to next instruction
i3here	 catch INT 3s
layout	 sets keyboard layout
syscall	 displays syscall (table)
altkey	 set alternate break key
addr	 show/set address contexts

[CTRL/SHIFT/ALT] arrow up/down
TAB

Not implemented yet:

dpd	 display dword physical memory
code	 toggle code display
peek	 peek at physical memory
poke	 poke to physical memory
phys	 show all mappings for linear address
timers	 show all active timers

TODO:
1. Evaluation of pointers.
2. Virtual breakpoints
3. Unimplemented commands.
4. Video mode switching (to debug gdi applications).


Enjoy,
Eugene


