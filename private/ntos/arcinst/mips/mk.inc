obj\mips\arcinst.exe: obj\mips\arcinst.lib makefile.inc
        link -out:obj\mips\arcinst.exe @<<
-machine:mips
-align:0x200
-rom
-fixed
-map:obj\mips\arcinst.map
-debug:notmapped
-debugtype:coff
-ignore:4001
-base:0xa0600000,0xa0618000
-entry:main
obj\mips\arcinst.lib
\nt\public\sdk\lib\mips\libcntpr.lib
<<NOKEEP
        binplace obj\mips\arcinst.exe
