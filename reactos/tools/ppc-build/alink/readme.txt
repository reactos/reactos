What's here as been modified for use with a PowerPC elf toolchain, but retains
every original feature for x86 as well.  

General modifications:

- Addition of elf32 input support
- PowerPC and x86 relocation types for elf
- Small bugfixes

elf.c is based on coff.c, modified to deal with elf sections, symbols etc.
