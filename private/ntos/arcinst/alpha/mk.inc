obj\alpha\arcinst.exe: obj\alpha\arcinst.lib makefile.inc
        link -out:obj\alpha\arcinst.exe @<<
-machine:alpha
-align:0x100
-rom
-fixed
-map:obj\alpha\arcinst.map
-debug:notmapped
-debugtype:coff
-ignore:4001
-base:0x80600000,0x80614000
-entry:main
obj\alpha\arcinst.lib
$(SDK_LIB_PATH)\libcntpr.lib
<<NOKEEP
    binplace obj\alpha\arcinst.exe
