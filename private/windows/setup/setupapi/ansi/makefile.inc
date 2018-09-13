!include ..\makefile.inc

msg.h msg00001.bin msg.rc: ..\msg.mc
        mc -a -v ..\msg.mc

